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

#ifndef AI5_S4_H
#define AI5_S4_H

#include <stdint.h>
#include "nulib/vector.h"

struct port;

#define S4_DRAW_CALL_SIZE 33

enum s4_opcode {
#define S4_OP_DRAW (-1)
	// stall 1 cycle
	S4_OP_NOOP = 0,
	// check if VM requested stop
	S4_OP_CHECK_STOP = 1,
	// stall N cycles
	S4_OP_STALL = 2,
	// reset IP to 0 (loop from beginning)
	S4_OP_RESET = 3,
	// stop
	S4_OP_HALT = 4,
	// loop N times
	S4_OP_LOOP_START = 5,
	// go to start of loop (or continue if loop finished)
	S4_OP_LOOP_END = 6,
	// same as S4_LOOP1_START (for nested loop)
	S4_OP_LOOP2_START = 7,
	// save as S4_LOOP2_END (for nested loop)
	S4_OP_LOOP2_END = 8,
#define S4_NR_OPCODES (S4_OP_8+1)
};

enum s4_draw_opcode {
	S4_DRAW_OP_COPY = 0x10,
	S4_DRAW_OP_COPY_MASKED = 0x20,
	S4_DRAW_OP_SWAP = 0x30,
	S4_DRAW_OP_SET_COLOR = 0x40,
	S4_DRAW_OP_COMPOSE = 0x50,
	S4_DRAW_OP_FILL = 0x60,
	S4_DRAW_OP_SET_PALETTE = 0x80,
};

struct s4_target {
	uint8_t i;
	int x, y;
};

struct s4_size {
	int w, h;
};

struct s4_fill_args {
	struct s4_target dst;
	struct s4_size dim;
};

struct s4_copy_args {
	struct s4_target src;
	struct s4_target dst;
	struct s4_size dim;
};

struct s4_compose_args {
	struct s4_target fg;
	struct s4_target bg;
	struct s4_target dst;
	struct s4_size dim;
};

struct s4_color {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

struct s4_set_color_args {
	uint8_t i;
	struct s4_color color;
};

struct s4_set_palette_args {
	struct s4_color colors[16];
};

struct s4_draw_call {
	enum s4_draw_opcode op;
	union {
		struct s4_fill_args fill;
		struct s4_copy_args copy;
		struct s4_compose_args compose;
		struct s4_set_color_args set_color;
		struct s4_set_palette_args set_palette;
	};
};
typedef vector_t(struct s4_draw_call) s4_draw_call_list;

struct s4_instruction {
	int op; // s4_opcode or -1 for draw call
	uint8_t arg;
};
typedef vector_t(struct s4_instruction) s4_stream;

#define S4_MAX_STREAMS 10

struct s4 {
	s4_stream streams[S4_MAX_STREAMS];
	s4_draw_call_list draw_calls;
};

struct s4 *s4_parse(uint8_t *data, size_t data_size);
bool s4_parse_draw_call(uint8_t *data, struct s4_draw_call *out);
void s4_print(struct port *out, struct s4 *s4);
void s4_free(struct s4 *s4);

#endif // AI5_S4_H
