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

#ifndef AI5_MES_H
#define AI5_MES_H

#include <stdint.h>
#include <stdio.h>

#include "nulib.h"
#include "nulib/vector.h"

typedef char *string;

struct port;
enum ai5_game_id;

#define MES_ADDRESS_SYNTHETIC 0xFFFFFFFF

/*
 * XXX: These opcode values don't necessarily correspond with those
 *      used in any particular game. They should be considered
 *      virtual opcodes for internal use. When parsing or compiling,
 *      the correct opcode should be looked up in the appropriate
 *      table for the target game.
 */
enum mes_statement_op {
	MES_STMT_END = 0x00,
	MES_STMT_TXT = 0x01,
	MES_STMT_STR = 0x02,
	MES_STMT_SETRBC = 0x03,
	MES_STMT_SETV = 0x04,
	MES_STMT_SETRBE = 0x05,
	MES_STMT_SETAC = 0x06,
	MES_STMT_SETA_AT = 0x07,
	MES_STMT_SETAD = 0x08,
	MES_STMT_SETAW = 0x09,
	MES_STMT_SETAB = 0x0A,
	MES_STMT_JZ = 0x0B,
	MES_STMT_JMP = 0x0C,
	MES_STMT_SYS = 0x0D,
	MES_STMT_GOTO = 0x0E,
	MES_STMT_CALL = 0x0F,
	MES_STMT_MENUI = 0x10,
	MES_STMT_PROC = 0x11,
	MES_STMT_UTIL = 0x12,
	MES_STMT_LINE = 0x13,
	MES_STMT_PROCD = 0x14,
	MES_STMT_MENUS = 0x15,
	MES_STMT_SETRD = 0x16,
};
#define MES_STMT_OP_MAX (MES_STMT_SETRD+1)

#define MES_CODE_INVALID 0xFF

enum mes_expression_op {
	MES_EXPR_IMM = 0x00, // not a real op
	MES_EXPR_VAR = 0x80,
	MES_EXPR_ARRAY16_GET16 = 0xA0,
	MES_EXPR_ARRAY16_GET8 = 0xC0,
	MES_EXPR_PLUS = 0xE0,
	MES_EXPR_MINUS = 0xE1,
	MES_EXPR_MUL = 0xE2,
	MES_EXPR_DIV = 0xE3,
	MES_EXPR_MOD = 0xE4,
	MES_EXPR_RAND = 0xE5,
	MES_EXPR_AND = 0xE6,
	MES_EXPR_OR = 0xE7,
	MES_EXPR_BITAND = 0xE8,
	MES_EXPR_BITIOR = 0xE9,
	MES_EXPR_BITXOR = 0xEA,
	MES_EXPR_LT = 0xEB,
	MES_EXPR_GT = 0xEC,
	MES_EXPR_LTE = 0xED,
	MES_EXPR_GTE = 0xEE,
	MES_EXPR_EQ = 0xEF,
	MES_EXPR_NEQ = 0xF0,
	MES_EXPR_IMM16 = 0xF1,
	MES_EXPR_IMM32 = 0xF2,
	MES_EXPR_REG16 = 0xF3, // 16-bit *index*
	MES_EXPR_REG8 = 0xF4, // 8-bit *index*
	MES_EXPR_ARRAY32_GET32 = 0xF5,
	MES_EXPR_ARRAY32_GET16 = 0xF6,
	MES_EXPR_ARRAY32_GET8 = 0xF7,
	MES_EXPR_VAR32 = 0xF8,
	MES_EXPR_END = 0xFF
};
#define MES_EXPR_OP_MAX (MES_EXPR_END+1)

enum mes_system_var16 {
	// offset to usable heap memory
	MES_SYS_VAR_HEAP,
	// destination surface for graphics operations
	MES_SYS_VAR_DST_SURFACE,
	// VM flags
	MES_SYS_VAR_FLAGS,
	// cursor location
	MES_SYS_VAR_CURSOR_X,
	MES_SYS_VAR_CURSOR_Y,
	// text area
	MES_SYS_VAR_TEXT_START_X,
	MES_SYS_VAR_TEXT_START_Y,
	MES_SYS_VAR_TEXT_END_X,
	MES_SYS_VAR_TEXT_END_Y,
	// text cursor location
	MES_SYS_VAR_TEXT_CURSOR_X,
	MES_SYS_VAR_TEXT_CURSOR_Y,
	// text colors (bgr555)
	MES_SYS_VAR_BG_COLOR,
	MES_SYS_VAR_FG_COLOR,
	// System.display_number flags
	MES_SYS_VAR_DISPLAY_NUMBER_FLAGS,
	// font parameters
	MES_SYS_VAR_FONT_WIDTH,
	MES_SYS_VAR_FONT_HEIGHT,
	MES_SYS_VAR_FONT_WEIGHT,
	MES_SYS_VAR_CHAR_SPACE,
	MES_SYS_VAR_LINE_SPACE,
	// CG rectangle
	MES_SYS_VAR_CG_X,
	MES_SYS_VAR_CG_Y,
	MES_SYS_VAR_CG_W,
	MES_SYS_VAR_CG_H,
	// number of menu entries
	MES_SYS_VAR_NR_MENU_ENTRIES,
	// return value of System.menu_get_no
	MES_SYS_VAR_MENU_NO,
	// mask color for graphics operations
	MES_SYS_VAR_MASK_COLOR,
};

enum mes_system_var32 {
	// pointer to memory
	MES_SYS_VAR_MEMORY,
	// offset to CG file in file_data
	MES_SYS_VAR_CG_OFFSET,
	// offset to data file in file_data
	MES_SYS_VAR_DATA_OFFSET,
	// pointer to palette
	MES_SYS_VAR_PALETTE,
	// offset to A6 file in file_data
	MES_SYS_VAR_A6_OFFSET,
	// pointer to file_data
	MES_SYS_VAR_FILE_DATA,
	// pointer to menu_entry_addresses
	MES_SYS_VAR_MENU_ENTRY_ADDRESSES,
	// pointer to menu_entry_numbers
	MES_SYS_VAR_MENU_ENTRY_NUMBERS,
	// offset to map file (dungeon)
	MES_SYS_VAR_MAP_OFFSET,
	// 24-bit mask color
	MES_SYS_VAR_MASK_COLOR24,
};

#define MES_NR_SYSTEM_VARIABLES 26

typedef struct mes_path_component *mes_namespace_t;

struct mes_code_tables {
	uint8_t stmt_op_to_int[MES_STMT_OP_MAX];
	enum mes_statement_op int_to_stmt_op[MES_STMT_OP_MAX];

	uint8_t expr_op_to_int[MES_EXPR_OP_MAX];
	enum mes_expression_op int_to_expr_op[MES_EXPR_OP_MAX];

	uint8_t sysvar16_to_int[MES_NR_SYSTEM_VARIABLES];
	enum mes_system_var16 int_to_sysvar16[MES_NR_SYSTEM_VARIABLES];

	uint8_t sysvar32_to_int[MES_NR_SYSTEM_VARIABLES];
	enum mes_system_var32 int_to_sysvar32[MES_NR_SYSTEM_VARIABLES];

	mes_namespace_t system;
	mes_namespace_t util;
};
extern struct mes_code_tables mes_code_tables;

// aliases for system variable codes
#define mes_sysvar16_dst_surface (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_DST_SURFACE])
#define mes_sysvar16_flags (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_FLAGS])
#define mes_sysvar16_cursor_x (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_CURSOR_X])
#define mes_sysvar16_cursor_y (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_CURSOR_Y])
#define mes_sysvar16_text_start_x (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_TEXT_START_X])
#define mes_sysvar16_text_start_y (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_TEXT_START_Y])
#define mes_sysvar16_text_end_x (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_TEXT_END_X])
#define mes_sysvar16_text_end_y (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_TEXT_END_Y])
#define mes_sysvar16_text_cursor_x (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_TEXT_CURSOR_X])
#define mes_sysvar16_text_cursor_y (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_TEXT_CURSOR_Y])
#define mes_sysvar16_bg_color (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_BG_COLOR])
#define mes_sysvar16_fg_color (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_FG_COLOR])
#define mes_sysvar16_display_number_flags (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_DISPLAY_NUMBER_FLAGS])
#define mes_sysvar16_font_width (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_FONT_WIDTH])
#define mes_sysvar16_font_height (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_FONT_HEIGHT])
#define mes_sysvar16_font_weight (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_FONT_WEIGHT])
#define mes_sysvar16_char_space (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_CHAR_SPACE])
#define mes_sysvar16_line_space (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_LINE_SPACE])
#define mes_sysvar16_cg_x (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_CG_X])
#define mes_sysvar16_cg_y (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_CG_Y])
#define mes_sysvar16_cg_w (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_CG_W])
#define mes_sysvar16_cg_h (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_CG_H])
#define mes_sysvar16_nr_menu_entries (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_NR_MENU_ENTRIES])
#define mes_sysvar16_menu_no (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_MENU_NO])
#define mes_sysvar16_mask_color (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_MASK_COLOR])
#define mes_sysvar32_memory (mes_code_tables.sysvar32_to_int[MES_SYS_VAR_MEMORY])
#define mes_sysvar32_cg_offset (mes_code_tables.sysvar32_to_int[MES_SYS_VAR_CG_OFFSET])
#define mes_sysvar32_data_offset (mes_code_tables.sysvar32_to_int[MES_SYS_VAR_DATA_OFFSET])
#define mes_sysvar32_palette (mes_code_tables.sysvar32_to_int[MES_SYS_VAR_PALETTE])
#define mes_sysvar32_a6_offset (mes_code_tables.sysvar32_to_int[MES_SYS_VAR_A6_OFFSET])
#define mes_sysvar32_file_data (mes_code_tables.sysvar32_to_int[MES_SYS_VAR_FILE_DATA])
#define mes_sysvar32_menu_entry_addresses (mes_code_tables.sysvar32_to_int[MES_SYS_VAR_MENU_ENTRY_ADDRESSES])
#define mes_sysvar32_menu_entry_numbers (mes_code_tables.sysvar32_to_int[MES_SYS_VAR_MENU_ENTRY_NUMBERS])
#define mes_sysvar32_map_offset (mes_code_tables.sysvar32_to_int[MES_SYS_VAR_MAP_OFFSET])
#define mes_sysvar32_mask_color (mes_code_tables.sysvar32_to_int[MES_SYS_VAR_MASK_COLOR24])

/*
 * Convert a game-specific statement opcode to a virtual opcode.
 */
static inline enum mes_statement_op mes_opcode_to_stmt(uint8_t op)
{
	if (unlikely(op >= ARRAY_SIZE(mes_code_tables.int_to_stmt_op)))
		return MES_CODE_INVALID;
	return mes_code_tables.int_to_stmt_op[op];
}

/*
 * Convert a game-specific expression opcode to a virtual expression opcode.
 */
static inline enum mes_expression_op mes_opcode_to_expr(uint8_t op)
{
	return mes_code_tables.int_to_expr_op[op];
}

/*
 * Convert a game-specific index to a mes_system_var16.
 */
static inline enum mes_system_var16 mes_index_to_sysvar16(uint8_t i)
{
	if (unlikely(i >= ARRAY_SIZE(mes_code_tables.int_to_sysvar16)))
		return MES_CODE_INVALID;
	return mes_code_tables.int_to_sysvar16[i];
}

/*
 * Convert a game-specific index to a mes_system_var16.
 */
static inline enum mes_system_var32 mes_index_to_sysvar32(uint8_t i)
{
	if (unlikely(i >= ARRAY_SIZE(mes_code_tables.int_to_sysvar32)))
		return MES_CODE_INVALID;
	return mes_code_tables.int_to_sysvar32[i];
}
/*
 * Convert a virtual statement opcode to a game-specific opcode.
 */
static inline uint8_t mes_stmt_opcode(enum mes_statement_op op)
{
	return mes_code_tables.stmt_op_to_int[op];
}

/*
 * Convert a virtual expression opcode to a game-specific opcode.
 */
static inline uint8_t mes_expr_opcode(enum mes_expression_op op)
{
	return mes_code_tables.expr_op_to_int[op];
}

/*
 * Convert a mes_system_var16 to a game-specific index.
 */
static inline uint8_t mes_sysvar16_index(enum mes_system_var16 v)
{
	return mes_code_tables.sysvar16_to_int[v];
}

/*
 * Convert a mes_system_var32 to a game-specific index.
 */
static inline uint8_t mes_sysvar32_index(enum mes_system_var32 v)
{
	return mes_code_tables.sysvar32_to_int[v];
}

extern const char *mes_system_var16_names[MES_NR_SYSTEM_VARIABLES];
extern const char *mes_system_var32_names[MES_NR_SYSTEM_VARIABLES];

enum mes_parameter_type {
	MES_PARAM_STRING = 1,
	MES_PARAM_EXPRESSION = 2,
};

struct mes_expression {
	enum mes_expression_op op;
	union {
		uint8_t arg8;
		uint16_t arg16;
		uint32_t arg32;
	};
	// unary operand / binary LHS operand
	struct mes_expression *sub_a;
	// binary RHS operand
	struct mes_expression *sub_b;
};

struct mes_parameter {
	enum mes_parameter_type type;
	union {
		string str;
		struct mes_expression *expr;
	};
};

typedef vector_t(struct mes_expression*) mes_expression_list;
typedef vector_t(struct mes_parameter) mes_parameter_list;

struct mes_qname_part {
	enum { MES_QNAME_IDENT, MES_QNAME_NUMBER } type;
	union { string ident; unsigned number; };
};

typedef vector_t(struct mes_qname_part) mes_qname;

struct mes_statement {
	enum mes_statement_op op;
	uint32_t address;
	uint32_t next_address;
	bool is_jump_target;
	union {
		struct {
			string text;
			bool terminated;
			bool unprefixed;
		} TXT;
		struct {
			uint16_t reg_no;
			mes_expression_list exprs;
		} SETRBC;
		struct {
			uint8_t var_no;
			mes_expression_list exprs;
		} SETV;
		struct {
			struct mes_expression *reg_expr;
			mes_expression_list val_exprs;
		} SETRBE;
		struct {
			uint8_t var_no;
			struct mes_expression *off_expr;
			mes_expression_list val_exprs;
		} SETAC;
		struct {
			uint8_t var_no;
			struct mes_expression *off_expr;
			mes_expression_list val_exprs;
		} SETA_AT;
		struct {
			uint8_t var_no;
			struct mes_expression *off_expr;
			mes_expression_list val_exprs;
		} SETAD;
		struct {
			uint8_t var_no;
			struct mes_expression *off_expr;
			mes_expression_list val_exprs;
		} SETAW;
		struct {
			uint8_t var_no;
			struct mes_expression *off_expr;
			mes_expression_list val_exprs;
		} SETAB;
		struct {
			uint32_t addr;
			struct mes_expression *expr;
		} JZ;
		struct {
			uint32_t addr;
		} JMP;
		struct {
			struct mes_expression *expr;
			mes_parameter_list params;
		} SYS;
		struct {
			mes_parameter_list params;
		} GOTO;
		struct {
			mes_parameter_list params;
		} CALL;
		struct {
			uint32_t addr; // ???
			mes_parameter_list params;
		} MENUI;
		struct {
			mes_parameter_list params;
		} PROC;
		struct {
			mes_parameter_list params;
		} UTIL;
		struct {
			uint8_t arg;
		} LINE;
		struct {
			uint32_t skip_addr;
			struct mes_expression *no_expr;
		} PROCD;
		struct {
			uint8_t var_no;
			mes_expression_list val_exprs;
		} SETRD;
	};
};

typedef vector_t(struct mes_statement*) mes_statement_list;

/* parse.c */
bool mes_char_is_hankaku(uint8_t b);
bool mes_char_is_zenkaku(uint8_t b);

struct mes_expression *mes_parse_expression(uint8_t *data, size_t data_size);
struct mes_statement *mes_parse_statement(uint8_t *data, size_t data_size);
bool mes_parse_statements(uint8_t *data, size_t data_size, mes_statement_list *statements);
void mes_qname_free(mes_qname name);
void mes_expression_free(struct mes_expression *expr);
void mes_parameter_list_free(mes_parameter_list list);
void mes_statement_free(struct mes_statement *stmt);
void mes_statement_list_free(mes_statement_list list);

enum ai5_game_id;
void mes_set_game(enum ai5_game_id id);

/* print.c */
void mes_expression_print(struct mes_expression *expr, struct port *out);
void mes_expression_list_print(mes_expression_list list, struct port *out);
void mes_parameter_print(struct mes_parameter *param, struct port *out);
void mes_parameter_list_print(mes_parameter_list list, struct port *out);
void _mes_statement_print(struct mes_statement *stmt, struct port *out, int indent);
void mes_statement_print(struct mes_statement *stmt, struct port *out);
void _mes_statement_list_print(mes_statement_list statements, struct port *out, int indent);
void mes_asm_statement_list_print(mes_statement_list statements, struct port *out);
void mes_flat_statement_list_print(mes_statement_list statements, struct port *out);
void mes_statement_list_print(mes_statement_list statements, struct port *out);

/* system.c */
mes_parameter_list mes_resolve_syscall(mes_qname name, int *no);
mes_parameter_list mes_resolve_util(mes_qname name);
string mes_get_syscall_name(unsigned no, mes_parameter_list params, unsigned *skip_params);
string mes_get_util_name(mes_parameter_list params, unsigned *skip_params);
int mes_resolve_sysvar(string name, bool *dword);

#endif // AI5_MES_H
