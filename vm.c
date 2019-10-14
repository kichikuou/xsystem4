/* Copyright (C) 2019 Nunuhara Cabbage <nunuhara@haniwa.technology>
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
#include <string.h>

#include "system4.h"
#include "vm_string.h"
#include "ain.h"
#include "instructions.h"
#include "little_endian.h"

#define INITIAL_STACK_SIZE 1024
#define INITIAL_HEAP_SIZE  4096
#define INITIAL_PAGES_SIZE 4096

/*
 * NOTE: The current implementation is a simple bytecode interpreter.
 *       System40.exe uses a JIT compiler, and we should too.
 */

// Non-heap values. Stored in pages and on the stack.
union vm_value {
	int32_t i;
	int64_t i64;
	float f;
};

enum vm_pointer_type {
	VM_PAGE,
	VM_STRING
};

// Heap-backed objects. Reference counted.
struct vm_pointer {
	int ref;
	enum vm_pointer_type type;
	union {
		struct string *s;
		union vm_value *page;
	};
};

struct function_call {
	int32_t fno;
	int32_t return_address;
	int32_t page_slot;
	int32_t page_ptr;
};

// The stack
static union vm_value *stack = NULL; // the stack
static int32_t stack_ptr = 0;        // pointer to the top of the stack
static size_t stack_size;            // current size of the stack

// The heap
// An array of pointers to heap-allocated objects, plus reference counts.
static struct vm_pointer *heap;
static size_t heap_size;

// Heap free list
// This is a list of unused indices into the 'heap' array.
static int32_t *heap_free_stack;
static int32_t heap_free_ptr = 0;

// Memory for global page + local pages
static union vm_value *page_stack;
static int32_t page_ptr = 0; // points to start of current local page
static int32_t pages_size;

// Stack of function call frames
static struct function_call call_stack[4096];
static int32_t call_stack_ptr = 0;

static struct ain *ain;
static size_t instr_ptr = 0;

static int32_t heap_alloc_slot(enum vm_pointer_type type)
{
	int32_t slot = heap_free_stack[heap_free_ptr++];
	heap[slot].ref = 1;
	heap[slot].type = type;
	return slot;
}

static void heap_free_slot(int32_t slot)
{
	heap_free_stack[--heap_free_ptr] = slot;
}

static void heap_ref(int32_t slot)
{
	heap[slot].ref++;
}

static void heap_unref(int32_t slot)
{
	if (--heap[slot].ref <= 0) {
		switch (heap[slot].type) {
		case VM_PAGE:
			break;
		case VM_STRING:
			free_string(heap[slot].s);
			break;
		}
		heap_free_slot(slot);
	}
}

static union vm_value _vm_id(union vm_value v)
{
	return v;
}

static union vm_value vm_int(int32_t v)
{
	return (union vm_value) { .i = v };
}

static union vm_value vm_long(int64_t v)
{
	return (union vm_value) { .i64 = v };
}

static union vm_value vm_float(float v)
{
	return (union vm_value) { .f = v };
}

#define vm_value_cast(v) _Generic((v),				\
				  union vm_value: _vm_id,	\
				  int32_t: vm_int,		\
				  int64_t: vm_long,		\
				  float: vm_float)(v)

static int32_t local_get(int varno)
{
        return page_stack[page_ptr + varno].i;
}

static void local_set(int varno, int32_t value)
{
	page_stack[page_ptr + varno].i = value;
}

static enum ain_data_type local_type(int varno)
{
	struct ain_function *f = &ain->functions[call_stack[call_stack_ptr-1].fno];
	return f->vars[varno].data_type;
}

static int32_t global_get(int varno)
{
	return heap[0].page[varno].i;
}

static enum ain_data_type global_type(int varno)
{
	return ain->globals[varno].data_type;
}

// Read the opcode at ADDR.
static int16_t get_opcode(size_t addr)
{
	return LittleEndian_getW(ain->code, addr);
}

// Read argument N for the current instruction.
static int32_t get_argument(int n)
{
	return LittleEndian_getDW(ain->code, instr_ptr + 2 + n*4);
}

// XXX: not strictly portable
static float get_argument_float(int n)
{
	union vm_value v;
	v.i = LittleEndian_getDW(ain->code, instr_ptr + 2 + n*4);
	return v.f;
}

static union vm_value stack_peek(int n)
{
	return stack[stack_ptr - (1 + n)];
}

// Set the Nth value from the top of the stack to V.
#define stack_set(n, v) (stack[stack_ptr - (1 + (n))] = vm_value_cast(v))

#define stack_push(v) (stack[stack_ptr++] = vm_value_cast(v))

static union vm_value stack_pop(void)
{
	stack_ptr--;
	return stack[stack_ptr];
}

// Pop a reference off the stack, returning the address of the referenced object.
static union vm_value *stack_pop_ref(void)
{
	int32_t page_index = stack_pop().i;
	int32_t heap_index = stack_pop().i;
	return &heap[heap_index].page[page_index];
}

static void stack_push_string(struct string *s)
{
	int32_t heap_slot = heap_alloc_slot(VM_STRING);
	heap[heap_slot].s = s;
	stack[stack_ptr++].i = heap_slot;
}

static struct string *stack_peek_string(void)
{
	return heap[stack_peek(0).i].s;
}

/*
 * System 4 calling convention:
 *   - caller pushes arguments, in order
 *   - CALLFUNC creates stack frame, pops arguments into local page
 *   - callee pushes return value on the stack
 *   - RETURN jumps to return address (saved in stack frame)
 */
static void function_call(int32_t no)
{
	struct ain_function *f = &ain->functions[no];
	int32_t cur_fno = call_stack[call_stack_ptr-1].fno;
	//int32_t new_fp = frame_ptr + 1 + ain->functions[cur_fno].nr_vars;
	int32_t page_slot = heap_alloc_slot(VM_PAGE);
	int32_t new_pp = page_ptr + ain->functions[cur_fno].nr_vars;

	// create new stack frame
	call_stack[call_stack_ptr++] = (struct function_call) {
		.fno = no,
		.return_address = instr_ptr + instruction_width(CALLFUNC),
		.page_slot = page_slot,
		.page_ptr = page_ptr
	};
	// create local page
	heap[page_slot].page = page_stack + new_pp;
	for (int i = f->nr_args - 1; i >= 0; i--) {
		page_stack[new_pp + i] = stack_pop();
	}
	// heap-backed variables need a allocate a slot
	for (int i = f->nr_args; i < f->nr_vars; i++) {
		int32_t slot;
		switch (f->vars[i].data_type) {
		case AIN_STRING:
			slot = heap_alloc_slot(VM_STRING);
			heap[slot].s = NULL;
			page_stack[new_pp + i].i = slot;
			break;
		default:
			break;
		}
	}

	// update stack/instruction pointers
	page_ptr  = new_pp;
	instr_ptr = ain->functions[no].address;
}

static void function_return(void)
{
	call_stack_ptr--;

	// unref slots for heap-backed variables
	struct ain_function *f = &ain->functions[call_stack[call_stack_ptr].fno];
	for (int i = f->nr_args; i < f->nr_vars; i++) {
		switch (f->vars[i].data_type) {
		case AIN_STRING:
			heap_unref(local_get(i));
			break;
		default:
			break;
		}
	}

	heap_free_slot(call_stack[call_stack_ptr].page_slot);
	instr_ptr = call_stack[call_stack_ptr].return_address;
	page_ptr  = call_stack[call_stack_ptr].page_ptr;


}

static void system_call(int32_t code)
{
	switch (code) {
	case 0x0: // system.Exit(int nResult)
		sys_exit(stack_pop().i);
		break;
	case 0x6: // system.Output(string szText)
		sys_message("%s", stack_peek_string()->text);
		// XXX: caller S_POPs
		break;
	case 0x14: // system.Peek()
		break;
	case 0x15: // system.Sleep(int nSleep)
		stack_pop();
		break;
	default:
		WARNING("Unimplemented syscall: 0x%X", code);
	}
}

static void execute_instruction(int16_t opcode)
{
	int32_t index, a, b, c, v;
	float f;
	union vm_value val;
	union vm_value *ref;
	const char *opcode_name = "UNKNOWN";
	switch (opcode) {
	//
	// --- Stack Management ---
	//
	case PUSH:
		stack_push(get_argument(0));
		break;
	case POP:
		stack_pop();
		break;
	case F_PUSH:
		stack_push(get_argument_float(0));
		break;
	case S_PUSH:
		stack_push_string(ain->strings[get_argument(0)]);
		break;
	case S_POP:
		index = stack_pop().i;
		heap_unref(index);
		break;
	case REF:
		// Dereference a reference to a value.
		stack_push(stack_pop_ref()[0]);
		break;
	case S_REF:
		// Dereference a reference to a string
		index = stack_pop_ref()->i;
		heap_ref(index);
		stack_push(index);
		break;
	case REFREF:
		// Dereference a reference to a reference.
		ref = stack_pop_ref();
		stack_push(ref[0].i);
		stack_push(ref[1].i);
		break;
	case DUP:
		// A -> AA
		stack_push(stack_peek(0).i);
		break;
	case DUP2:
		// AB -> ABAB
		a = stack_peek(1).i;
		b = stack_peek(0).i;
		stack_push(a);
		stack_push(b);
		break;
	case DUP_X2:
		// ABC -> CABC
		a = stack_peek(2).i;
		b = stack_peek(1).i;
		c = stack_peek(0).i;
		stack_set(2, c);
		stack_set(1, a);
		stack_set(0, b);
		stack_push(c);
		break;
	case DUP2_X1:
		// ABC -> BCABC
		a = stack_peek(2).i;
		b = stack_peek(1).i;
		c = stack_peek(0).i;
		stack_set(2, b);
		stack_set(1, c);
		stack_set(0, a);
		stack_push(b);
		stack_push(c);
		break;
	case PUSHGLOBALPAGE:
		stack_push(0);
		break;
	case PUSHLOCALPAGE:
		stack_push(call_stack[call_stack_ptr-1].page_slot);
		break;
	case ASSIGN:
	case F_ASSIGN:
		val = stack_pop();
		stack_pop_ref()[0] = val;
		break;
	case SH_GLOBALREF: // VARNO
		index = get_argument(0);
		stack_push(global_get(index));
		switch (global_type(index)) {
		case AIN_STRING:
			heap_ref(global_get(index));
			break;
		default:
			break;
		}
		break;
	case SH_LOCALREF: // VARNO
		index = get_argument(0);
		stack_push(local_get(index));
		switch (local_type(index)) {
		case AIN_STRING:
			heap_ref(local_get(index));
			break;
		default:
			break;
		}
		break;
	case SH_LOCALASSIGN: // VARNO, VALUE
		// Assign VALUE to local VARNO
		local_set(get_argument(0), get_argument(1));
		break;
	case SH_LOCALINC: // VARNO
		index = get_argument(0);
		local_set(index, local_get(index)+1);
		break;
	case SH_LOCALDEC: // VARNO
		index = get_argument(0);
		local_set(index, local_get(index)-1);
		break;
	//
	// --- Function Calls ---
	//
	case CALLFUNC:
		function_call(get_argument(0));
		break;
	case RETURN:
		function_return();
		break;
	case CALLSYS:
		system_call(get_argument(0));
		break;
	//
	// --- Control Flow ---
	//
	case JUMP: // ADDR
		instr_ptr = get_argument(0);
		break;
	case IFZ: // ADDR
		if (!stack_pop().i)
			instr_ptr = get_argument(0);
		else
			instr_ptr += instruction_width(IFZ);
		break;
	case IFNZ: // ADDR
		if (stack_pop().i)
			instr_ptr = get_argument(0);
		else
			instr_ptr += instruction_width(IFNZ);
		break;
	//
	// --- Arithmetic ---
	//
	case INV:
		stack[stack_ptr-1].i = -stack[stack_ptr-1].i;
		break;
	case NOT:
		stack[stack_ptr-1].i = !stack[stack_ptr-1].i;
		break;
	case COMPL:
		stack[stack_ptr-1].i = ~stack[stack_ptr-1].i;
		break;
	case ADD:
		stack[stack_ptr-2].i += stack[stack_ptr-1].i;
		stack_ptr--;
		break;
	case SUB:
		stack[stack_ptr-2].i -= stack[stack_ptr-1].i;
		stack_ptr--;
		break;
	case MUL:
		stack[stack_ptr-2].i *= stack[stack_ptr-1].i;
		stack_ptr--;
		break;
	case DIV:
		stack[stack_ptr-2].i /= stack[stack_ptr-1].i;
		stack_ptr--;
		break;
	case MOD:
		stack[stack_ptr-2].i %= stack[stack_ptr-1].i;
		stack_ptr--;
		break;
	case AND:
		stack[stack_ptr-2].i &= stack[stack_ptr-1].i;
		stack_ptr--;
		break;
	case OR:
		stack[stack_ptr-2].i |= stack[stack_ptr-1].i;
		stack_ptr--;
		break;
	case XOR:
		stack[stack_ptr-2].i ^= stack[stack_ptr-1].i;
		stack_ptr--;
		break;
	case LSHIFT:
		stack[stack_ptr-2].i <<= stack[stack_ptr-1].i;
		stack_ptr--;
		break;
	case RSHIFT:
		stack[stack_ptr-2].i >>= stack[stack_ptr-1].i;
		stack_ptr--;
		break;
	// Numeric Comparisons
	case LT:
		b = stack_pop().i;
		a = stack_pop().i;
		stack_push(a < b ? 1 : 0);
		break;
	case GT:
		b = stack_pop().i;
		a = stack_pop().i;
		stack_push(a > b ? 1 : 0);
		break;
	case LTE:
		b = stack_pop().i;
		a = stack_pop().i;
		stack_push(a <= b ? 1 : 0);
		break;
	case GTE:
		b = stack_pop().i;
		a = stack_pop().i;
		stack_push(a >= b ? 1 : 0);
		break;
	case NOTE:
		b = stack_pop().i;
		a = stack_pop().i;
		stack_push(a != b ? 1 : 0);
		break;
	case EQUALE:
		b = stack_pop().i;
		a = stack_pop().i;
		stack_push(a == b ? 1 : 0);
		break;
	// +=, -=, etc.
	case PLUSA:
		v = stack_pop().i;
		stack_pop_ref()[0].i += v;
		break;
	case MINUSA:
		v = stack_pop().i;
		stack_pop_ref()[0].i -= v;
		break;
	case MULA:
		v = stack_pop().i;
		stack_pop_ref()[0].i *= v;
		break;
	case DIVA:
		v = stack_pop().i;
		stack_pop_ref()[0].i /= v;
		break;
	case MODA:
		v = stack_pop().i;
		stack_pop_ref()[0].i %= v;
		break;
	case ANDA:
		v = stack_pop().i;
		stack_pop_ref()[0].i &= v;
		break;
	case ORA:
		v = stack_pop().i;
		stack_pop_ref()[0].i |= v;
		break;
	case XORA:
		v = stack_pop().i;
		stack_pop_ref()[0].i ^= v;
		break;
	case LSHIFTA:
		v = stack_pop().i;
		stack_pop_ref()[0].i <<= v;
		break;
	case RSHIFTA:
		v = stack_pop().i;
		stack_pop_ref()[0].i >>= v;
		break;
	case INC:
		stack_pop_ref()[0].i++;
		break;
	case DEC:
		stack_pop_ref()[0].i--;
		break;
	//
	// --- Floating Point Arithmetic ---
	//
	case FTOI:
		stack_set(0, (int32_t)stack_peek(0).f);
		break;
	case ITOF:
		stack_set(0, (float)stack_peek(0).i);
		break;
	case F_INV:
		stack_set(0, -stack_peek(0).f);
		break;
	case F_ADD:
		f = stack_pop().f;
		stack_set(0, stack_peek(0).f + f);
		break;
	case F_SUB:
		f = stack_pop().f;
		stack_set(0, stack_peek(0).f - f);
		break;
	case F_MUL:
		f = stack_pop().f;
		stack_set(0, stack_peek(0).f * f);
		break;
	case F_DIV:
		f = stack_pop().f;
		stack_set(0, stack_peek(0).f / f);
		break;
	// floating point comparison
	case F_LT:
		f = stack_pop().f;
		stack_set(0, stack_peek(0).f < f ? 1 : 0);
		break;
	case F_GT:
		f = stack_pop().f;
		stack_set(0, stack_peek(0).f > f ? 1 : 0);
		break;
	case F_LTE:
		f = stack_pop().f;
		stack_set(0, stack_peek(0).f <= f ? 1 : 0);
		break;
	case F_GTE:
		f = stack_pop().f;
		stack_set(0, stack_peek(0).f >= f ? 1 : 0);
		break;
	case F_NOTE:
		f = stack_pop().f;
		stack_set(0, stack_peek(0).f != f ? 1 : 0);
		break;
	case F_EQUALE:
		f = stack_pop().f;
		stack_set(0, stack_peek(0).f == f ? 1 : 0);
		break;
	//
	// --- Strings ---
	//
	case S_ASSIGN: // A = B
		b = stack_pop().i;
		a = stack_peek(0).i;
		if (heap[a].s) {
			free_string(heap[a].s);
		}
		heap[a].s = string_dup(heap[b].s);
		heap_unref(b);
		break;
	case S_ADD:
		b = stack_pop().i;
		a = stack_pop().i;
		stack_push_string(string_append(heap[a].s, heap[b].s));
		heap_unref(a);
		heap_unref(b);
		break;
	case I_STRING:
		stack_push_string(integer_to_string(stack_pop().i));
		break;
	case FTOS:
		v = stack_pop().i; // precision
		stack_push_string(float_to_string(stack_pop().f, v));
		break;
	// -- NOOPs ---
	case FUNC:
		break;
	default:
		if (opcode >= 0 && opcode < NR_OPCODES && instructions[opcode].name) {
			opcode_name = instructions[opcode].name;
		}
		WARNING("Unimplemented instruction: 0x%X(%s)", opcode, opcode_name);
	}
}

void vm_execute(struct ain *program)
{
	// initialize VM state
	stack_size = INITIAL_STACK_SIZE;
	stack = xmalloc(INITIAL_STACK_SIZE * sizeof(union vm_value));
	stack_ptr = 0;

	heap_size = INITIAL_HEAP_SIZE;
	heap = xmalloc(INITIAL_HEAP_SIZE * sizeof(struct vm_pointer));

	heap_free_stack = xmalloc(INITIAL_HEAP_SIZE * sizeof(int32_t));
	for (size_t i = 0; i < INITIAL_HEAP_SIZE; i++) {
		heap_free_stack[i] = i;
	}
	heap_free_ptr = 1; // global page at index 0

	pages_size = INITIAL_PAGES_SIZE;
	page_stack = xmalloc(INITIAL_PAGES_SIZE * sizeof(union vm_value));
	page_ptr = 0;

	ain = program;

	// Initialize globals
	heap[0].page = xmalloc(sizeof(union vm_value) * ain->nr_globals);
	for (int i = 0; i < ain->nr_globals; i++) {
		switch (ain->globals[i].data_type) {
		case AIN_STRING:
			heap[0].page[i].i = heap_alloc_slot(VM_STRING);
			break;
		default:
			break;
		}
	}
	for (int i = 0; i < ain->nr_initvals; i++) {
		int32_t index;
		struct ain_initval *v = &ain->global_initvals[i];
		switch (v->data_type) {
		case AIN_STRING:
			index = heap_alloc_slot(VM_STRING);
			heap[0].page[v->global_index].i = index;
			heap[index].s = make_string(v->string_value, strlen(v->string_value));
			break;
		default:
			heap[0].page[v->global_index].i = v->int_value;
			break;
		}
	}

	// Jump to main. We set up a stack frame so that when main returns,
	// the first instruction past the end of the code section is executed.
	// (When we read the AIN file, CALLSYS 0x0 was placed there.)
	instr_ptr = ain->code_size - instruction_width(CALLFUNC);
	function_call(ain->main);

	// fetch/decode/execute loop
	for (;;)
	{
		if (instr_ptr >= ain->code_size + 6) {
			ERROR("Illegal instruction pointer: 0x%lX", instr_ptr);
		}
		int16_t opcode = get_opcode(instr_ptr);
		execute_instruction(opcode);
		instr_ptr += instructions[opcode].ip_inc;
	}
}
