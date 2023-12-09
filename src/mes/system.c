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

#include <string.h>

#include "nulib.h"
#include "nulib/string.h"
#include "nulib/vector.h"
#include "ai5/mes.h"

struct mes_path_component {
	const char *name;
	int nr_children;
	struct mes_path_component **children;
};

#define LEAF(pre, _name) static struct mes_path_component pre##_##_name = { .name = #_name }

#define NODE(ident, _name, ...) \
	static struct mes_path_component * ident##_children[] = { __VA_ARGS__ }; \
	static struct mes_path_component ident = { \
		.name = #_name, \
		.nr_children = ARRAY_SIZE(ident##_children), \
		.children = ident##_children \
	}

// System.set_font_size
LEAF(sys, set_font_size);

// System.display_number
LEAF(sys, display_number);

// System.Cursor
LEAF(sys_cursor, reload);
LEAF(sys_cursor, unload);
LEAF(sys_cursor, save_pos);
LEAF(sys_cursor, set_pos);
LEAF(sys_cursor, load);
LEAF(sys_cursor, show);
LEAF(sys_cursor, hide);
NODE(sys_cursor, Cursor,
	[0] = &sys_cursor_reload,
	[1] = &sys_cursor_unload,
	[2] = &sys_cursor_save_pos,
	[3] = &sys_cursor_set_pos,
	[4] = &sys_cursor_load,
	[5] = &sys_cursor_show,
	[6] = &sys_cursor_hide,
);

// System.Anim
LEAF(sys_anim, init);
LEAF(sys_anim, start);
LEAF(sys_anim, stop);
LEAF(sys_anim, halt);
LEAF(sys_anim, stop_all);
LEAF(sys_anim, halt_all);
NODE(sys_anim, Anim,
	[0] = &sys_anim_init,
	[1] = &sys_anim_start,
	[2] = &sys_anim_stop,
	[3] = &sys_anim_halt,
	[5] = &sys_anim_stop_all,
	[6] = &sys_anim_halt_all,
);

// System.SaveData
LEAF(sys_savedata, resume_load);
LEAF(sys_savedata, resume_save);
LEAF(sys_savedata, load);
LEAF(sys_savedata, save);
LEAF(sys_savedata, load_var4);
LEAF(sys_savedata, save_var4);
LEAF(sys_savedata, save_union_var4);
LEAF(sys_savedata, load_var4_slice);
LEAF(sys_savedata, save_var4_slice);
LEAF(sys_savedata, copy);
LEAF(sys_savedata, set_mes_name);
NODE(sys_savedata, SaveData,
	[0] = &sys_savedata_resume_load,
	[1] = &sys_savedata_resume_save,
	[2] = &sys_savedata_load,
	[3] = &sys_savedata_save,
	[4] = &sys_savedata_load_var4,
	[5] = &sys_savedata_save_var4,
	[6] = &sys_savedata_save_union_var4,
	[7] = &sys_savedata_load_var4_slice,
	[8] = &sys_savedata_save_var4_slice,
	[9] = &sys_savedata_copy,
	[13] = &sys_savedata_set_mes_name,
);

// System.Audio
LEAF(sys_audio, bgm_play);
LEAF(sys_audio, bgm_stop);
LEAF(sys_audio, se_play);
LEAF(sys_audio, bgm_fade_sync);
LEAF(sys_audio, bgm_set_volume);
LEAF(sys_audio, bgm_fade);
LEAF(sys_audio, bgm_fade_out_sync);
LEAF(sys_audio, bgm_fade_out);
LEAF(sys_audio, se_stop);
LEAF(sys_audio, bgm_stop2);
NODE(sys_audio, Audio,
	[0] = &sys_audio_bgm_play,
	[2] = &sys_audio_bgm_stop,
	[3] = &sys_audio_se_play,
	[4] = &sys_audio_bgm_fade_sync,
	[5] = &sys_audio_bgm_set_volume,
	[7] = &sys_audio_bgm_fade,
	[9] = &sys_audio_bgm_fade_out_sync,
	[10] = &sys_audio_bgm_fade_out,
	[12] = &sys_audio_se_stop,
	[18] = &sys_audio_bgm_stop2,
);

// System.File
LEAF(sys_file, read);
LEAF(sys_file, write);
NODE(sys_file, File,
	[0] = &sys_file_read,
	[1] = &sys_file_write,
);

// System.load_image
LEAF(sys, load_image);

// System.Palette
LEAF(sys_palette, set);
LEAF(sys_palette, crossfade);
NODE(sys_palette, Palette,
	[0] = &sys_palette_set,
	[2] = &sys_palette_crossfade,
);

// System.Image
LEAF(sys_image, copy);
LEAF(sys_image, copy_masked);
LEAF(sys_image, fill_bg);
LEAF(sys_image, copy_swap);
LEAF(sys_image, swap_bg_fg);
LEAF(sys_image, copy_sprite_bg);
LEAF(sys_image, invert_colors);
LEAF(sys_image, copy_progressive);
NODE(sys_image, Image,
	[0] = &sys_image_copy,
	[1] = &sys_image_copy_masked,
	[2] = &sys_image_fill_bg,
	[3] = &sys_image_copy_swap,
	[4] = &sys_image_swap_bg_fg,
	[5] = &sys_image_copy_sprite_bg,
	[6] = &sys_image_invert_colors,
	[20] = &sys_image_copy_progressive,
);

// System.wait
LEAF(sys, wait);

// System.set_text_colors
LEAF(sys, set_text_colors);

// System.farcall
LEAF(sys, farcall);

// System.get_cursor_segment
LEAF(sys, get_cursor_segment);

// System.get_menu_no
LEAF(sys, get_menu_no);

// System.get_time
LEAF(sys, get_time);

// System.noop
LEAF(sys, noop);

// System.input_state
LEAF(sys, check_input);

// System.noop2
LEAF(sys, noop2);

// System.strlen
LEAF(sys, strlen);

// System.set_screen_surface
LEAF(sys, set_screen_surface);

static struct mes_path_component *system_children[] = {
	[0] = &sys_set_font_size,
	[1] = &sys_display_number,
	[2] = &sys_cursor,
	[3] = &sys_anim,
	[4] = &sys_savedata,
	[5] = &sys_audio,
	[7] = &sys_file,
	[8] = &sys_load_image,
	[9] = &sys_palette,
	[10] = &sys_image,
	[11] = &sys_wait,
	[12] = &sys_set_text_colors,
	[13] = &sys_farcall,
	[14] = &sys_get_cursor_segment,
	[15] = &sys_get_menu_no,
	[16] = &sys_get_time,
	[17] = &sys_noop,
	[18] = &sys_check_input,
	[20] = &sys_noop2,
	[21] = &sys_strlen,
	[23] = &sys_set_screen_surface,
};

static struct mes_path_component syscalls = {
	.name = "System",
	.nr_children = ARRAY_SIZE(system_children),
	.children = system_children
};

static struct mes_path_component *_resolve_qname(struct mes_path_component *ctx,
		struct mes_qname_part *part, int *no)
{
	if (part->type == MES_QNAME_NUMBER) {
		*no = part->number;
		if (*no < ctx->nr_children && ctx->children[*no])
			return ctx->children[*no];
		return NULL;
	}
	for (int i = 0; i < ctx->nr_children; i++) {
		if (!ctx->children[i])
			continue;
		if (!strcmp(ctx->children[i]->name, part->ident)) {
			*no = i;
			return ctx->children[i];
		}
	}
	*no = -1;
	return NULL;
}

mes_parameter_list mes_resolve_syscall(mes_qname name, int *no)
{
	mes_parameter_list params = vector_initializer;

	// get syscall number
	if (vector_length(name) < 1)
		goto error;

	struct mes_path_component *ctx = _resolve_qname(&syscalls, &kv_A(name, 0), no);
	if (*no < 0)
		goto error;

	// resolve remaining identifiers
	for (int i = 1; ctx && i < vector_length(name); i++) {
		int part_no;
		struct mes_qname_part *part = &kv_A(name, i);
		ctx = _resolve_qname(ctx, part, &part_no);
		if (part_no < 0)
			goto error;
		if (part->type == MES_QNAME_IDENT)
			string_free(part->ident);
		part->type = MES_QNAME_NUMBER;
		part->number = part_no;
	}

	// create parameter list from resolved identifiers
	for (int i = 1; i < vector_length(name); i++) {
		struct mes_qname_part *part = &kv_A(name, i);
		if (part->type == MES_QNAME_IDENT)
			goto error;
		struct mes_parameter param = { 
			.type = MES_PARAM_EXPRESSION,
			.expr = xcalloc(1, sizeof(struct mes_expression))
		};
		param.expr->op = MES_EXPR_IMM;
		param.expr->arg8 = part->number;
		vector_push(struct mes_parameter, params, param);
	}

	mes_qname_free(name);
	return params;
error:
	vector_destroy(params);
	*no = -1;
	return (mes_parameter_list)vector_initializer;
}

static struct mes_path_component *get_child(struct mes_path_component *parent, unsigned no)
{
	if (no >= parent->nr_children)
		return NULL;
	return parent->children[no];
}

static string _mes_get_syscall_name(string name, struct mes_path_component *parent,
		mes_parameter_list params, unsigned *skip_params)
{
	if (vector_length(params) <= *skip_params)
		return name;
	if (parent->nr_children == 0 || vector_A(params, *skip_params).type != MES_PARAM_EXPRESSION)
		return name;
	struct mes_expression *expr = vector_A(params, 0).expr;
	if (expr->op != MES_EXPR_IMM)
		return name;

	(*skip_params)++;

	struct mes_path_component *child = get_child(parent, expr->arg8);
	if (!child) {
		return string_concat_fmt(name, ".function[%u]", expr->arg8);
	}

	name = string_concat_fmt(name, ".%s", child->name);
	return _mes_get_syscall_name(name, child, params, skip_params);
}

string mes_get_syscall_name(unsigned no, mes_parameter_list params, unsigned *skip_params)
{
	*skip_params = 0;
	string name = string_new("System");
	struct mes_path_component *sys = get_child(&syscalls, no);
	if (!sys) {
		return string_concat_fmt(name, ".function[%u]", no);
	}

	name = string_concat_fmt(name, ".%s", sys->name);
	return _mes_get_syscall_name(name, sys, params, skip_params);
}

const char *mes_system_var16_names[MES_NR_SYSTEM_VARIABLES] = {
	[MES_SYS_VAR_DST_SURFACE] = "dst_surface",
	[MES_SYS_VAR_FLAGS] = "flags",
	[MES_SYS_VAR_CURSOR_X] = "cursor_x",
	[MES_SYS_VAR_CURSOR_Y] = "cursor_y",
	[MES_SYS_VAR_TEXT_START_X] = "text_start_x",
	[MES_SYS_VAR_TEXT_START_Y] = "text_start_y",
	[MES_SYS_VAR_TEXT_END_X] = "text_end_x",
	[MES_SYS_VAR_TEXT_END_Y] = "text_end_y",
	[MES_SYS_VAR_TEXT_CURSOR_X] = "text_cursor_x",
	[MES_SYS_VAR_TEXT_CURSOR_Y] = "text_cursor_y",
	[MES_SYS_VAR_FONT_WIDTH] = "font_width",
	[MES_SYS_VAR_FONT_HEIGHT] = "font_height",
	[MES_SYS_VAR_FONT_WEIGHT] = "font_weight",
	[MES_SYS_VAR_CHAR_SPACE] = "char_space",
	[MES_SYS_VAR_LINE_SPACE] = "line_space",
	[MES_SYS_VAR_CG_X] = "cg_x",
	[MES_SYS_VAR_CG_Y] = "cg_y",
	[MES_SYS_VAR_CG_W] = "cg_w",
	[MES_SYS_VAR_CG_W] = "cg_w",
	[MES_SYS_VAR_NR_MENU_ENTRIES] = "nr_menu_entries",
	[MES_SYS_VAR_MASK_COLOR] = "mask_color",
};

const char *mes_system_var32_names[MES_NR_SYSTEM_VARIABLES] = {
	[MES_SYS_VAR_MEMORY] = "memory",
	[MES_SYS_VAR_CG_OFFSET] = "cg_offset",
	[MES_SYS_VAR_DATA_OFFSET] = "data_offset",
	[MES_SYS_VAR_PALETTE] = "palette",
	[MES_SYS_VAR_FILE_DATA] = "file_data",
	[MES_SYS_VAR_MENU_ENTRY_ADDRESSES] = "menu_entry_addresses",
	[MES_SYS_VAR_MENU_ENTRY_NUMBERS] = "menu_entry_numbers",
};

int mes_resolve_sysvar(string name, bool *dword)
{
	for (int i = 0; i < MES_NR_SYSTEM_VARIABLES; i++) {
		if (mes_system_var16_names[i] && !strcmp(name, mes_system_var16_names[i])) {
			*dword = false;
			return i;
		}
		if (mes_system_var32_names[i] && !strcmp(name, mes_system_var32_names[i])) {
			*dword = true;
			return i;
		}
	}
	return -1;
}
