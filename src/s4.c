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

#include "nulib.h"
#include "nulib/buffer.h"
#include "nulib/port.h"
#include "ai5/game.h"
#include "ai5/s4.h"

unsigned s4_draw_call_size = 33;

void s4_set_game(enum ai5_game_id game)
{
	switch (game) {
	case GAME_SHANGRLIA:
		s4_draw_call_size = 10;
		break;
	default:
		s4_draw_call_size = 33;
		break;
	}
}

static void parse_color(struct buffer *in, struct s4_color *out)
{
	uint8_t b1 = buffer_read_u8(in);
	uint8_t b2 = buffer_read_u8(in);
	out->r = (b2 & 0xf0) | (b2 >> 4);
	out->g = (b2 & 0x0f) | (b2 << 4);
	out->b = (b1 & 0x0f) | (b1 << 4);
}

static bool parse_draw_call(struct buffer *in, struct s4_draw_call *out)
{
	size_t start = in->index;

	uint8_t op = buffer_read_u8(in);
	switch ((out->op = op & 0xf0)) {
	case S4_DRAW_OP_FILL:
		out->fill.dst.i = (op >> 1) & 1;
		out->fill.dst.x = buffer_read_u8(in) * 8;
		out->fill.dst.y = buffer_read_u16(in);
		out->fill.dim.w = (buffer_read_u8(in) + 1) * 8 - out->fill.dst.x;
		out->fill.dim.h = (buffer_read_u16(in) + 1) - out->fill.dst.y;
		break;
	case S4_DRAW_OP_COPY:
	case S4_DRAW_OP_COPY_MASKED:
	case S4_DRAW_OP_SWAP:
		out->copy.src.i = (op >> 1) & 1;
		out->copy.dst.i = op & 1;
		out->copy.src.x = buffer_read_u8(in) * 8;
		out->copy.src.y = buffer_read_u16(in);
		out->copy.dim.w = (buffer_read_u8(in) + 1) * 8 - out->copy.src.x;
		out->copy.dim.h = (buffer_read_u16(in) + 1) - out->copy.src.y;
		out->copy.dst.x = buffer_read_u8(in) * 8;
		out->copy.dst.y = buffer_read_u16(in);
		break;
	case S4_DRAW_OP_COMPOSE:
		out->compose.bg.i = op & 1;
		out->compose.fg.i = (op >> 1) & 1;
		out->compose.dst.i = (op >> 2) & 1;
		out->compose.fg.x = buffer_read_u8(in) * 8;
		out->compose.fg.y = buffer_read_u16(in);
		out->compose.dim.w = (buffer_read_u8(in) + 1) * 8 - out->compose.fg.x;
		out->compose.dim.h = (buffer_read_u16(in) + 1) - out->compose.fg.y;
		out->compose.bg.x = buffer_read_u8(in) * 8;
		out->compose.bg.y = buffer_read_u16(in);
		out->compose.dst.x = buffer_read_u8(in) * 8;
		out->compose.dst.y = buffer_read_u16(in);
		break;
	case S4_DRAW_OP_SET_COLOR:
		// XXX: index is also blue value...
		out->set_color.i = buffer_peek_u8(in);
		parse_color(in, &out->set_color.color);
		break;
	case S4_DRAW_OP_SET_PALETTE:
		for (int i = 0; i < 16; i++) {
			parse_color(in, &out->set_palette.colors[i]);
		}
		break;
	default:
		WARNING("Invalid draw call opcode: %02x", op);
		return false;
	}

	buffer_seek(in, start + s4_draw_call_size);
	return true;
}

bool s4_parse_draw_call(uint8_t *data, struct s4_draw_call *out)
{
	struct buffer b;
	buffer_init(&b, data, s4_draw_call_size);
	return parse_draw_call(&b, out);
}

static bool s4_parse_instruction(struct buffer *in, struct s4 *s4, struct s4_instruction *out)
{
	uint8_t op = buffer_read_u8(in);
	switch (op) {
	case S4_OP_NOOP:
	case S4_OP_CHECK_STOP:
	case S4_OP_RESET:
	case S4_OP_HALT:
	case S4_OP_LOOP_END:
	case S4_OP_LOOP2_END:
		out->op = op;
		break;
	case S4_OP_STALL:
	case S4_OP_LOOP_START:
	case S4_OP_LOOP2_START:
		out->op = op;
		out->arg = buffer_read_u8(in);
		break;
	default:
		if (op < 20 || op - 20 >= vector_length(s4->draw_calls)) {
			WARNING("at %zx", in->index-1);
			WARNING("Invalid draw call index: %u", op);
			return false;
		}
		out->op = S4_OP_DRAW;
		out->arg = op - 20;
		break;
	}
	return true;
}

void s4_free(struct s4 *s4)
{
	for (int i = 0; i < S4_MAX_STREAMS; i++) {
		vector_destroy(s4->streams[i]);
	}
	vector_destroy(s4->draw_calls);
	free(s4);
}

struct s4 *s4_parse(uint8_t *data, size_t data_size)
{
	struct buffer in;
	buffer_init(&in, data, data_size);

	uint16_t stream_ptr[S4_MAX_STREAMS];
	uint16_t stream_end[S4_MAX_STREAMS];

	uint8_t nr_streams = buffer_read_u8(&in);
	if (nr_streams > S4_MAX_STREAMS) {
		WARNING("Too many streams in s4 file");
		return NULL;
	}

	for (unsigned i = 0; i < nr_streams; i++) {
		stream_ptr[i] = buffer_read_u16(&in);
	}

	// determine start of stream bytecode section
	unsigned stream_start = stream_ptr[0];
	for (unsigned i = 1; i < nr_streams; i++) {
		if (stream_ptr[i] < stream_start)
			stream_start = stream_ptr[i];
	}

	struct s4 *s4 = xcalloc(1, sizeof(struct s4));

	// read draw calls
	while (!buffer_end(&in) && in.index < stream_start) {
		struct s4_draw_call call;
		if (!parse_draw_call(&in, &call))
			goto err;
		vector_push(struct s4_draw_call, s4->draw_calls, call);
	}

	// read bytecode for each stream
	for (unsigned i = 0; i < nr_streams; i++) {
		buffer_seek(&in, stream_ptr[i]);
		// XXX: Many S4 files are broken in that they contain stream pointers which
		//      point into previous streams. We fix these broken files by treating
		//      such streams as if they are empty.
		//      (The cause of the breakage seems to be the LOOP2_START
		//      instruction: the start of stream position is off by the number of
		//      occurrences of this instruction.)
		for (unsigned j = 0; j < i; j++) {
			if (stream_ptr[j] < stream_ptr[i] && stream_end[j] > stream_ptr[i]) {
				sys_warning("Streams %u and %u overlap (possibly broken file)\n", i, j);
				goto skip;
			}
		}
		while (!buffer_end(&in) && buffer_peek_u8(&in) != 0xff) {
			struct s4_instruction instr;
			if (!s4_parse_instruction(&in, s4, &instr))
				goto err;
			vector_push(struct s4_instruction, s4->streams[i], instr);
		}
skip:
		stream_end[i] = in.index;
	}

	return s4;
err:
	s4_free(s4);
	return NULL;
}

static void print_draw_args(struct port *out, struct s4_draw_call *call)
{
	switch (call->op) {
	case S4_DRAW_OP_COPY:
	case S4_DRAW_OP_COPY_MASKED:
	case S4_DRAW_OP_SWAP:
		port_printf(out, " %u(%u, %u) -> %u(%u, %u) @ (%u, %u);\n",
				call->copy.src.i, call->copy.src.x, call->copy.src.y,
				call->copy.dst.i, call->copy.dst.x, call->copy.dst.y,
				call->copy.dim.w, call->copy.dim.h);
		break;
	case S4_DRAW_OP_COMPOSE:
		port_printf(out, " %u(%u, %u) + %u(%u, %u) -> %u(%u, %u) @ (%u, %u);\n",
				call->compose.bg.i, call->compose.bg.x, call->compose.bg.y,
				call->compose.fg.i, call->compose.fg.x, call->compose.fg.y,
				call->compose.dst.i, call->compose.dst.x, call->compose.dst.y,
				call->compose.dim.w, call->compose.dim.h);
		break;
	case S4_DRAW_OP_FILL:
		port_printf(out, " %u(%u, %u) @ (%u, %u);\n", call->fill.dst,
				call->fill.dst.x, call->fill.dst.y, call->fill.dim.w,
				call->fill.dim.h);
		break;
	case S4_DRAW_OP_SET_COLOR:
		port_printf(out, " %u -> (%u,%u,%u);\n", call->set_color.i,
				call->set_color.color.r & 0xf, call->set_color.color.g & 0xf,
				call->set_color.color.b & 0xf);
		break;
	case S4_DRAW_OP_SET_PALETTE:
		for (int i = 0; i < 16; i++) {
			port_printf(out, " (%u,%u,%u)", call->set_palette.colors[i].r & 0xf,
					call->set_palette.colors[i].g & 0xf,
					call->set_palette.colors[i].b & 0xf);
		}
		port_puts(out, ";\n");
		break;
	}
}

static void s4_print_draw_call(struct port *out, struct s4_draw_call *call, int indent)
{
	for (int i = 0; i < indent; i++) {
		port_puts(out, "\t");
	}
	switch (call->op) {
	case S4_DRAW_OP_COPY:
		port_puts(out, "COPY");
		break;
	case S4_DRAW_OP_COPY_MASKED:
		port_puts(out, "COPY_MASKED");
		break;
	case S4_DRAW_OP_SWAP:
		port_puts(out, "SWAP");
		break;
	case S4_DRAW_OP_SET_COLOR:
		port_puts(out, "SET_COLOR");
		break;
	case S4_DRAW_OP_COMPOSE:
		port_puts(out, "COMPOSE");
		break;
	case S4_DRAW_OP_FILL:
		port_puts(out, "FILL");
		break;
	case S4_DRAW_OP_SET_PALETTE:
		port_puts(out, "SET_PALETTE");
		break;
	default:
		ERROR("Invalid draw call: %d", call->op);
	}
	print_draw_args(out, call);
}

static void s4_print_instruction(struct port *out, struct s4_instruction *instr, int indent)
{
	for (int i = 0; i < indent; i++) {
		port_puts(out, "\t");
	}
	switch (instr->op) {
	case S4_OP_NOOP:
		port_puts(out, "NOOP;\n");
		break;
	case S4_OP_CHECK_STOP:
		port_puts(out, "CHECK_STOP;\n");
		break;
	case S4_OP_STALL:
		port_printf(out, "STALL %u;\n", instr->arg);
		break;
	case S4_OP_RESET:
		port_puts(out, "RESET;\n");
		break;
	case S4_OP_HALT:
		port_puts(out, "HALT;\n");
		break;
	case S4_OP_LOOP_START:
		port_printf(out, "LOOP_START %u;\n", instr->arg);
		break;
	case S4_OP_LOOP_END:
		port_puts(out, "LOOP_END;\n");
		break;
	case S4_OP_LOOP2_START:
		port_printf(out, "LOOP2_START %u;\n", instr->arg);
		break;
	case S4_OP_LOOP2_END:
		port_puts(out, "LOOP2_END;\n");
		break;
	default:
		ERROR("Invalid opcode: %d", instr->op);
	}
}

void s4_print(struct port *out, struct s4 *s4)
{
	for (int i = 0; i < S4_MAX_STREAMS; i++) {
		if (vector_empty(s4->streams[i]))
			continue;
		port_printf(out, "STREAM:\n", i);
		int indent = 0;
		struct s4_instruction *p;
		vector_foreach_p(p, s4->streams[i]) {
			if (p->op == S4_OP_LOOP_END || p->op == S4_OP_LOOP2_END)
				indent--;
			if (p->op == S4_OP_DRAW) {
				struct s4_draw_call *call = &vector_A(s4->draw_calls, p->arg);
				s4_print_draw_call(out, call, indent);
			} else {
				s4_print_instruction(out, p, indent);
			}
			if (p->op == S4_OP_LOOP_START || p->op == S4_OP_LOOP2_START)
				indent++;
		}
	}
}
