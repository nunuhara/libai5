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

#include <stdlib.h>

#include "nulib.h"
#include "nulib/buffer.h"
#include "ai5/lzss.h"

#define FRAME_SIZE 0x1000
#define FRAME_MASK (FRAME_SIZE - 1)

uint8_t *lzss_decompress_with_limit(uint8_t *input, size_t input_size, size_t *output_size)
{
	uint8_t frame[FRAME_SIZE] = {0};
	unsigned frame_pos = 0xfee;

	struct buffer in;
	buffer_init(&in, input, input_size);

	struct buffer out;
	size_t limit = *output_size;
	if (!limit) {
		buffer_init(&out, NULL, 0);
		limit = 0xffffffff;
	} else {
		buffer_init(&out, xmalloc(limit), limit);
	}

	while (!buffer_end(&in)) {
		if (unlikely(out.index >= limit))
			break;
		buffer_reserve(&out, 18 * 8);
		uint8_t ctl = buffer_read_u8_uc(&in);
		for (unsigned bit = 1; bit != 0x100; bit <<= 1) {
			if (ctl & bit) {
				if (unlikely(buffer_remaining(&in) < 1))
					break;
				uint8_t b = buffer_read_u8_uc(&in);;
				frame[frame_pos++ & FRAME_MASK] = b;
				buffer_write_u8_uc(&out, b);
			} else {
				if (unlikely(buffer_remaining(&in) < 2))
					break;
				uint8_t lo = buffer_read_u8_uc(&in);
				uint8_t hi = buffer_read_u8_uc(&in);
				uint16_t offset = ((hi & 0xf0) << 4) | lo;
				for (int count = 3 + (hi & 0xf); count != 0; --count) {
					if (unlikely(out.index >= limit))
						break;
					uint8_t v = frame[offset++ & FRAME_MASK];
					frame[frame_pos++ & FRAME_MASK] = v;
					buffer_write_u8_uc(&out, v);
				}
			}
		}
	}
	*output_size = out.index;
	return out.buf;
}

uint8_t *lzss_decompress(uint8_t *input, size_t input_size, size_t *output_size)
{
	*output_size = 0;
	return lzss_decompress_with_limit(input, input_size, output_size);
}

uint8_t *lzss_compress(uint8_t *input, size_t input_size, size_t *output_size)
{
	struct buffer in;
	buffer_init(&in, input, input_size);

	struct buffer out;
	buffer_init(&out, NULL, 0);

	// TODO: write a proper implementation that actually compresses the data
	//       (this implementation inflates the size by 12.5%).
	while (!buffer_end(&in)) {
		// write control byte
		int rem = buffer_remaining(&in);
		if (rem < 8)
			buffer_write_u8(&out, (1 << rem) - 1);
		else
			buffer_write_u8(&out, 0xff);

		// write data
		int count = min(rem, 8);
		for (int i = 0; i < count; i++) {
			buffer_write_u8(&out, buffer_read_u8(&in));
		}
	}

	*output_size = out.index;
	return out.buf;
}
