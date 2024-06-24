/* Copyright (C) 2023 Nunuhara Cabbage <nunuhara@haniwa.technology>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef _WIN32
#define mmap(...) (ERROR("mmap not supported on Windows"), NULL)
#define munmap(...) (ERROR("munmap not supported on Windows"), -1)
#define MAP_FAILED 0
#else
#include <sys/mman.h>
#endif

#include "nulib.h"
#include "nulib/file.h"
#include "nulib/hashtable.h"
#include "nulib/little_endian.h"
#include "nulib/lzss.h"
#include "nulib/string.h"
#include "nulib/utfsjis.h"
#include "nulib/vector.h"
#include "ai5/arc.h"
#include "ai5/game.h"

#define MAX_SANE_FILES 100000

define_hashtable_string(arcindex, int);

static bool read_u32(FILE *fp, uint32_t *out)
{
	uint8_t buf[4];
	if (fread(buf, 4, 1, fp) != 1) {
		WARNING("fread: %s", strerror(errno));
		return false;
	}
	*out = le_get32(buf, 0);
	return true;
}

static bool read_u32_at(FILE *fp, off_t offset, uint32_t *out)
{
	if (fseek(fp, offset, SEEK_SET)) {
		WARNING("fseek: %s", strerror(errno));
		return false;
	}
	return read_u32(fp, out);
}

static bool read_u8_at(FILE *fp, off_t offset, uint8_t *out)
{
	if (fseek(fp, offset, SEEK_SET)) {
		WARNING("fseek: %s", strerror(errno));
		return false;
	}
	if (fread(out, 1, 1, fp) != 1) {
		WARNING("fread: %s", strerror(errno));
		return false;
	}
	return true;
}

static off_t get_file_size(FILE *fp)
{
	// get size of archive
	if (fseek(fp, 0, SEEK_END)) {
		WARNING("fseek: %s", strerror(errno));
		return 0;
	}
	off_t r = ftell(fp);
	if (fseek(fp, 0, SEEK_SET)) {
		WARNING("fseek: %s", strerror(errno));
		return 0;
	}
	return r;
}

static bool arc_get_metadata(FILE *fp, struct arc_metadata *meta_out)
{
	struct arc_metadata meta = {0};

	// get size of archive
	if (!(meta.arc_size = get_file_size(fp)))
		return false;

	if (!read_u32(fp, &meta.nr_files))
		return false;
	if (meta.nr_files > MAX_SANE_FILES) {
		WARNING("archive file count is not sane: %u", meta.nr_files);
		return false;
	}

	const unsigned name_lengths[] = { 0x14, 0x1e, 0x20, 0x100 };
	for (int i = 0; i < ARRAY_SIZE(name_lengths); i++) {
		uint32_t first_size, first_offset, second_offset;
		if (!read_u8_at(fp, 3 + name_lengths[i], &meta.name_key))
			return false;
		if (!read_u32_at(fp, 4 + name_lengths[i], &first_size))
			return false;
		if (!read_u32_at(fp, 8 + name_lengths[i], &first_offset))
			return false;
		if (!read_u32_at(fp, (off_t)(8 + name_lengths[i]) * 2, &second_offset))
			return false;

		uint32_t data_offset = (name_lengths[i] + 8) * meta.nr_files + 4;
		meta.offset_key = data_offset ^ first_offset;
		second_offset ^= meta.offset_key;
		if (second_offset < data_offset || second_offset >= meta.arc_size)
			continue;

		meta.size_key = (second_offset - data_offset) ^ first_size;
		if (meta.offset_key && meta.size_key) {
			meta.name_length = name_lengths[i];
			break;
		}
	}
	if (!meta.name_length)
		return false;

	*meta_out = meta;
	return true;
}

static bool dat_get_metadata(FILE *fp, struct arc_metadata *meta_out)
{
	struct arc_metadata meta = {0};

	// get size of archive
	if (!(meta.arc_size = get_file_size(fp)))
		return false;

	uint32_t key;
	if (!read_u32_at(fp, 4, &key))
		return false;
	meta.offset_key = key;
	meta.size_key = key;

	if (!read_u32_at(fp, 0, &meta.nr_files))
		return false;
	meta.nr_files ^= key;
	if (meta.nr_files > MAX_SANE_FILES) {
		WARNING("archive file count is not sane: %u", meta.nr_files);
		return false;
	}
	if (!read_u8_at(fp, 0x23, &meta.name_key))
		return false;

	meta.name_length = 20;
	*meta_out = meta;
	return true;
}

static bool read_index(FILE *fp, struct archive *arc, unsigned size_off, unsigned offset_off,
		unsigned name_off)
{
	const struct arc_metadata *meta = &arc->meta;
	const size_t buf_len = (size_t)meta->nr_files * (meta->name_length + 8);
	size_t buf_pos = 0;
	uint8_t *buf = xmalloc(buf_len);
	if (fread(buf, buf_len, 1, fp) != 1) {
		WARNING("fread: %s", strerror(errno));
		return false;
	}

	// read file entries
	vector_init(arc->files);
	vector_resize(struct archive_data, arc->files, meta->nr_files);
	for (int i = 0; i < meta->nr_files; i++) {
		// decode file name
		for (int j = 0; j < meta->name_length; j++) {
			buf[buf_pos + name_off + j] ^= meta->name_key;
			if (buf[buf_pos + name_off + j] == 0)
				break;
		}
		buf[buf_pos + name_off + (meta->name_length - 1)] = 0;

		uint32_t offset = le_get32(buf, buf_pos + offset_off) ^ meta->offset_key;
		uint32_t raw_size = le_get32(buf, buf_pos + size_off) ^ meta->size_key;
		string name = sjis_cstring_to_utf8((char*)buf + buf_pos + name_off, 0);
		if (offset + raw_size > meta->arc_size) {
			ERROR("%s @ %u + %u extends beyond EOF (%u)", name, offset, raw_size,
					(unsigned)meta->arc_size);
		}

		// store entry
		vector_A(arc->files, i) = (struct archive_data) {
			.offset = offset,
			.raw_size = raw_size,
			.name = name,
			.data = NULL,
			.ref = 0,
			.archive = arc
		};
		buf_pos += meta->name_length + 8;
	}
	free(buf);

	// create index
	for (int i = 0; i < vector_length(arc->files); i++) {
		// upcase filename
		char *name = vector_A(arc->files, i).name;
		for (int i = 0; name[i]; i++) {
			name[i] = toupper(name[i]);
		}

		int ret;
		hashtable_iter_t k = hashtable_put(arcindex, &arc->index,
				vector_A(arc->files, i).name,
				&ret);
		if (ret == HASHTABLE_KEY_PRESENT) {
			WARNING("skipping duplicate file name in archive");
			continue;
		}
		hashtable_val(&arc->index, k) = i;
	}

	return true;
}

static bool arc_read_index(FILE *fp, struct archive *arc)
{
	if (fseek(fp, 4, SEEK_SET)) {
		WARNING("fseek: %s", strerror(errno));
		return false;
	}
	return read_index(fp, arc, arc->meta.name_length, arc->meta.name_length + 4, 0);
}

static bool dat_read_index(FILE *fp, struct archive *arc)
{
	if (fseek(fp, 8, SEEK_SET)) {
		WARNING("fseek: %s", strerror(errno));
		return false;
	}
	return read_index(fp, arc, 0, 4, 8);
}

struct archive *archive_open(const char *path, unsigned flags)
{
#ifdef _WIN32
	flags &= ~ARCHIVE_MMAP;
#endif
	FILE *fp = NULL;
	struct archive *arc = xcalloc(1, sizeof(struct archive));

	// open archive file
	if (!(fp = file_open_utf8(path, "rb"))) {
		WARNING("file_open_utf8: %s", strerror(errno));
		goto error;
	}

	if (!strcasecmp(file_extension(path), "dat")) {
		if (!dat_get_metadata(fp, &arc->meta)) {
			WARNING("failed to read archive metadata");
			goto error;
		}
		if (!dat_read_index(fp, arc)) {
			goto error;
		}
	} else {
		if (!arc_get_metadata(fp, &arc->meta)) {
			WARNING("failed to read archive metadata");
			goto error;
		}
		if (!arc_read_index(fp, arc)) {
			goto error;
		}
	}

	// store either mmap ptr/size or FILE* depending on flags
	if (flags & ARCHIVE_MMAP) {
		int fd = fileno(fp);
		if (fd == -1) {
			WARNING("fileno: %s", strerror(errno));
			goto error;
		}
		arc->map.data = mmap(0, arc->meta.arc_size, PROT_READ, MAP_SHARED, fd, 0);
		arc->map.size = arc->meta.arc_size;
		if (arc->map.data == MAP_FAILED) {
			WARNING("mmap: %s", strerror(errno));
			goto error;
		}
		if (fclose(fp)) {
			WARNING("fclose: %s", strerror(errno));
			goto error;
		}
		arc->mapped = true;
	} else {
		arc->fp = fp;
	}

	arc->flags = flags;
	return arc;
error:
	vector_destroy(arc->files);
	free(arc);
	if (fp && fclose(fp))
		WARNING("fclose: %s", strerror(errno));
	return NULL;
}

void archive_close(struct archive *arc)
{
	if (arc->mapped) {
		if (munmap(arc->map.data, arc->map.size))
			WARNING("munmap: %s", strerror(errno));
	} else {
		if (fclose(arc->fp))
			WARNING("fclose: %s", strerror(errno));
	}
	for (unsigned i = 0; i < vector_length(arc->files); i++) {
		string_free(vector_A(arc->files, i).name);
	}
	vector_destroy(arc->files);
	hashtable_destroy(arcindex, &arc->index);
	free(arc);
}

bool archive_file_compressed(const char *name)
{
	if (ai5_target_game == GAME_ALLSTARS)
		return false;

	static const char *compressed_ext[] = {
		"a",
		"a4",
		"a6",
		"bmp",
		"ccd",
		"dat",
		"eve",
		"fnt",
		"mes",
		"mpx",
		"msk",
		"pal",
		"lib",
		"s4",
		"tbl",
		"x",
	};
	const char *ext = file_extension(name);
	for (unsigned i = 0; i < ARRAY_SIZE(compressed_ext); i++) {
		if (!strcasecmp(ext, compressed_ext[i]))
			return true;
	}
	return false;
}

/*
 * Decompress compressed file types.
 */
static bool data_decompress(struct archive_data *data)
{
	if (!archive_file_compressed(data->name))
		return true;

	size_t decompressed_size;
	uint8_t *tmp = lzss_decompress(data->data, data->raw_size, &decompressed_size);
	if (!data->mapped)
		free(data->data);
	data->mapped = false;
	if (!tmp) {
		WARNING("lzss_decompress failed");
		data->data = NULL;
		data->size = 0;
		return false;
	}
	data->data = tmp;
	data->size = decompressed_size;
	return true;
}

bool archive_data_load(struct archive_data *data)
{
	// data already loaded by another caller
	if (data->ref) {
		data->ref++;
		return true;
	}

	assert(!data->data);

	// load data
	if (data->archive->mapped) {
		data->data = data->archive->map.data + data->offset;
		data->size = data->raw_size;
		data->mapped = true;
	} else {
		if (fseek(data->archive->fp, data->offset, SEEK_SET)) {
			WARNING("fseek: %s", strerror(errno));
			return false;
		}
		data->data = xmalloc(data->raw_size);
		if (fread(data->data, data->raw_size, 1, data->archive->fp) != 1) {
			WARNING("fread: %s", strerror(errno));
			free(data->data);
			return false;
		}
		data->size = data->raw_size;
	}

	if (!(data->archive->flags & ARCHIVE_RAW) && !data_decompress(data))
		return false;

	data->ref = 1;
	return true;
}

int archive_get_index(struct archive *arc, const char *name)
{
	// convert name to uppercase
	int i;
	char upname[256];
	for (i = 0; name[i] && i < 255; i++) {
		upname[i] = toupper(name[i]);
	}
	upname[i] = 0;

	hashtable_iter_t k = hashtable_get(arcindex, &arc->index, upname);
	if (k == hashtable_end(&arc->index))
		return -1;
	return hashtable_val(&arc->index, k);
}

struct archive_data *archive_get(struct archive *arc, const char *name)
{
	int i = archive_get_index(arc, name);
	if (i < 0)
		return NULL;

	// XXX: Because we're giving out pointers into arc->files, the vector must
	//      never be resized.
	struct archive_data *data = &vector_A(arc->files, i);
	if (archive_data_load(data))
		return data;
	return NULL;
}

struct archive_data *archive_get_by_index(struct archive *arc, unsigned i)
{
	if (i >= vector_length(arc->files))
		return NULL;

	struct archive_data *data = &vector_A(arc->files, i);
	if (archive_data_load(data))
		return data;
	return NULL;
}

void archive_data_release(struct archive_data *data)
{
	if (data->ref == 0)
		ERROR("double-free of archive data");
	if (--data->ref == 0) {
		if (!data->mapped)
			free(data->data);
		data->data = NULL;
		data->size = 0;
		if (data->allocated)
			free(data);
	}
}
