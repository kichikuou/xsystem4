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
#include <assert.h>
#include "system4.h"
#include "system4/string.h"
#include "ainedit.h"
#include "jaf.h"

// TODO: better error messages
#define TYPE_ERROR(expr, expected) ERROR("Type error (expected %s; got %s)", ain_strtype(NULL, expected, -1), ain_strtype(NULL, expr->valuetype.data, -1))
#define TYPE_CHECK(expr, expected) { if (expr->valuetype.data != expected) TYPE_ERROR(expr, expected); }

const char *jaf_typestr(enum jaf_type type)
{
	switch (type) {
	case JAF_VOID:     return "void";
	case JAF_INT:      return "int";
	case JAF_FLOAT:    return "float";
	case JAF_STRING:   return "string";
	case JAF_STRUCT:   return "struct";
	case JAF_ENUM:     return "enum";
	case JAF_TYPEDEF:  return "typedef";
	case JAF_FUNCTION: return "functype";
	}
	return "unknown";
}

static enum ain_data_type strip_ref(enum ain_data_type type)
{
	switch (type) {
	case AIN_REF_INT:             return AIN_INT;
	case AIN_REF_FLOAT:           return AIN_FLOAT;
	case AIN_REF_STRING:          return AIN_STRING;
	case AIN_REF_STRUCT:          return AIN_STRUCT;
	case AIN_REF_ARRAY_INT:       return AIN_ARRAY_INT;
	case AIN_REF_ARRAY_FLOAT:     return AIN_ARRAY_FLOAT;
	case AIN_REF_ARRAY_STRING:    return AIN_ARRAY_STRING;
	case AIN_REF_ARRAY_STRUCT:    return AIN_ARRAY_STRUCT;
	case AIN_REF_FUNC_TYPE:       return AIN_FUNC_TYPE;
	case AIN_REF_ARRAY_FUNC_TYPE: return AIN_ARRAY_FUNC_TYPE;
	case AIN_REF_BOOL:            return AIN_BOOL;
	case AIN_REF_ARRAY_BOOL:      return AIN_ARRAY_BOOL;
	case AIN_REF_LONG_INT:        return AIN_LONG_INT;
	case AIN_REF_ARRAY_LONG_INT:  return AIN_ARRAY_LONG_INT;
	default:                      return type;
	}
}

static bool jaf_type_equal(struct ain_type *a, struct ain_type *b)
{
	enum ain_data_type a_data = strip_ref(a->data), b_data = strip_ref(b->data);
	if (a_data != b_data)
		return false;
	if (a_data == AIN_STRUCT && a->struc != b->struc)
		return false;
	if (a_data == AIN_FUNC_TYPE && a->struc != b->struc)
		return false;
	return true;
}

static enum jaf_type jaf_type_check_numeric(struct jaf_expression *expr)
{
	switch (expr->valuetype.data) {
	case AIN_INT:
	case AIN_FLOAT:
	case AIN_BOOL:
	case AIN_LONG_INT:
		return expr->valuetype.data;
	case AIN_REF_INT:
		return AIN_INT;
	case AIN_REF_FLOAT:
		return AIN_FLOAT;
	case AIN_REF_BOOL:
		return AIN_BOOL;
	case AIN_REF_LONG_INT:
		return AIN_LONG_INT;
	default:
		TYPE_ERROR(expr, AIN_INT);
	}
}

static enum jaf_type jaf_type_check_int(struct jaf_expression *expr)
{
	switch (expr->valuetype.data) {
	case AIN_INT:
	case AIN_LONG_INT:
		return expr->valuetype.data;
	case AIN_REF_INT:
		return AIN_INT;
	case AIN_REF_LONG_INT:
		return AIN_LONG_INT;
	default:
		TYPE_ERROR(expr, AIN_INT);
	}
}

// Determine the result type when combining different numeric types.
// Precedence is: float > lint > int
static enum ain_data_type jaf_merge_types(enum ain_data_type a, enum ain_data_type b)
{
	if (a == b)
		return a;
	if (a == AIN_INT) {
		if (b == AIN_LONG_INT)
			return AIN_LONG_INT;
		if (b == AIN_FLOAT)
			return AIN_FLOAT;
	} else if (a == AIN_LONG_INT) {
		if (b == AIN_INT)
			return AIN_LONG_INT;
		if (b == AIN_FLOAT)
			return AIN_FLOAT;
	} else if (a == AIN_FLOAT) {
		if (a == AIN_INT || a == AIN_LONG_INT)
			return AIN_FLOAT;
	}
	ERROR("Incompatible types");
}

static void jaf_check_types_lvalue(possibly_unused struct jaf_env *env, struct jaf_expression *e)
{
	// TODO: array subscripts
	if (e->type != JAF_EXP_IDENTIFIER && e->type != JAF_EXP_MEMBER && e->type != JAF_EXP_SUBSCRIPT)
		ERROR("Invalid expression as lvalue");
	switch (e->valuetype.data) {
	case AIN_INT:
	case AIN_FLOAT:
	case AIN_BOOL:
	case AIN_LONG_INT:
	case AIN_STRING:
	case AIN_REF_INT:
	case AIN_REF_FLOAT:
	case AIN_REF_BOOL:
	case AIN_REF_LONG_INT:
	case AIN_REF_STRING:
		break;
	default:
		ERROR("Invalid type as lvalue: %d", e->valuetype.data);
	}
}

static void jaf_check_types_unary(struct jaf_env *env, struct jaf_expression *expr)
{
	jaf_derive_types(env, expr->expr);
	switch (expr->op) {
	case JAF_UNARY_PLUS:
	case JAF_UNARY_MINUS:
		expr->valuetype.data = jaf_type_check_numeric(expr->expr);
		break;
	case JAF_PRE_INC:
	case JAF_PRE_DEC:
	case JAF_POST_INC:
	case JAF_POST_DEC:
		jaf_check_types_lvalue(env, expr->expr);
		expr->valuetype.data = jaf_type_check_numeric(expr->expr);
		break;
	case JAF_BIT_NOT:
	case JAF_LOG_NOT:
		expr->valuetype.data = jaf_type_check_int(expr->expr);
		break;
	case JAF_AMPERSAND:
		ERROR("Function types not supported");
	default:
		ERROR("Unhandled unary operator");
	}
}

static bool is_string_type(struct jaf_expression *expr)
{
	return expr->valuetype.data == AIN_STRING || expr->valuetype.data == AIN_REF_STRING;
}

static void jaf_check_types_binary(struct jaf_env *env, struct jaf_expression *expr)
{
	jaf_derive_types(env, expr->lhs);
	jaf_derive_types(env, expr->rhs);

	switch (expr->op) {
	// real ops
	case JAF_MULTIPLY:
	case JAF_DIVIDE:
	case JAF_MINUS:
		expr->valuetype.data = jaf_merge_types(jaf_type_check_numeric(expr->lhs), jaf_type_check_numeric(expr->rhs));
		break;
	case JAF_PLUS:
		if (is_string_type(expr->lhs)) {
			if (!is_string_type(expr->rhs))
				TYPE_ERROR(expr->rhs, AIN_STRING);
			expr->valuetype.data = AIN_STRING;
		} else {
			expr->valuetype.data = jaf_merge_types(jaf_type_check_numeric(expr->lhs), jaf_type_check_numeric(expr->rhs));
		}
		break;
	// integer ops
	case JAF_REMAINDER:
	case JAF_LSHIFT:
	case JAF_RSHIFT:
	case JAF_BIT_AND:
	case JAF_BIT_XOR:
	case JAF_BIT_IOR:
		jaf_type_check_int(expr->lhs);
		jaf_type_check_int(expr->rhs);
		expr->valuetype.data = AIN_INT;
		break;
	// comparison ops
	case JAF_LT:
	case JAF_GT:
	case JAF_LTE:
	case JAF_GTE:
	case JAF_EQ:
	case JAF_NEQ:
		if (is_string_type(expr->lhs)) {
			if (!is_string_type(expr->rhs))
				TYPE_ERROR(expr->rhs, AIN_STRING);
			expr->valuetype.data = AIN_INT;
		} else {
			jaf_type_check_numeric(expr->lhs);
			jaf_type_check_numeric(expr->rhs);
			expr->valuetype.data = AIN_INT;
		}
		break;
	// boolean ops
	case JAF_LOG_AND:
	case JAF_LOG_OR:
		jaf_type_check_int(expr->lhs);
		jaf_type_check_int(expr->rhs);
		expr->valuetype.data = AIN_INT;
		break;
	case JAF_ASSIGN:
	case JAF_MUL_ASSIGN:
	case JAF_DIV_ASSIGN:
	case JAF_MOD_ASSIGN:
	case JAF_ADD_ASSIGN:
	case JAF_SUB_ASSIGN:
	case JAF_LSHIFT_ASSIGN:
	case JAF_RSHIFT_ASSIGN:
	case JAF_AND_ASSIGN:
	case JAF_XOR_ASSIGN:
	case JAF_OR_ASSIGN:
		jaf_check_types_lvalue(env, expr->lhs);
		// FIXME: need to coerce types (?)
		jaf_check_type(expr->rhs, &expr->lhs->valuetype);
		expr->valuetype.data = expr->lhs->valuetype.data;
		break;
	case JAF_REF_ASSIGN: // TODO
	default:
		ERROR("Unhandled binary operator");
	}
}

static void jaf_check_types_ternary(struct jaf_env *env, struct jaf_expression *expr)
{
	jaf_derive_types(env, expr->condition);
	jaf_derive_types(env, expr->consequent);
	jaf_derive_types(env, expr->alternative);

	TYPE_CHECK(expr->condition, AIN_INT);
	if (!jaf_type_equal(&expr->consequent->valuetype, &expr->alternative->valuetype)) {
		// TODO: better error message
		ERROR("Mismatched types in conditional expression");
	}

	expr->valuetype = expr->consequent->valuetype;
}

static struct ain_variable *jaf_scope_lookup(struct jaf_env *env, char *name, int *var_no)
{
	for (size_t i = 0; i < env->nr_locals; i++) {
		if (!strcmp(env->locals[i]->name, name)) {
			*var_no = i;
			return env->locals[i];
		}
	}
	return NULL;
}

struct ain_variable *jaf_env_lookup(struct jaf_env *env, struct string *name, int *var_no)
{
	char *u = encode_text_to_input_format(name->text);
	struct jaf_env *scope = env;
        while (scope) {
		struct ain_variable *v = jaf_scope_lookup(scope, u, var_no);
		if (v) {
			free(u);
			return v;
		}
		scope = scope->parent;
	}

	int no = ain_get_global_no(env->ain, u);
	free(u);

	if (no >= 0) {
		*var_no = no;
		return &env->ain->globals[no];
	}

	return NULL;
}

static void jaf_check_types_identifier(struct jaf_env *env, struct jaf_expression *expr)
{
	int var_no;
	struct ain_variable *v = jaf_env_lookup(env, expr->ident.name, &var_no);
	if (v) {
		expr->valuetype = v->type;
		expr->ident.var_type = v->var_type;
		expr->ident.var_no = var_no;
		return;
	}

	char *u = encode_text_to_input_format(expr->s->text);
	int func_no = ain_get_function_no(env->ain, u);
	free(u);
	if (func_no < 0)
		ERROR("Undefined variable: %s", expr->s->text);

	// FIXME: this isn't quite right...
	expr->valuetype.data = AIN_FUNC_TYPE;
	expr->valuetype.struc = func_no;
}

static bool ain_wide_type(enum ain_data_type type) {
	switch (type) {
	case AIN_REF_INT:
	case AIN_REF_FLOAT:
	case AIN_REF_BOOL:
	case AIN_REF_LONG_INT:
		return true;
	default:
		return false;
	}
}

static void jaf_check_types_funcall(struct jaf_env *env, struct jaf_expression *expr)
{
	jaf_derive_types(env, expr->call.fun);
	TYPE_CHECK(expr->call.fun, AIN_FUNC_TYPE);

	unsigned nr_args = expr->call.args ? expr->call.args->nr_items : 0;
	for (size_t i = 0; i < nr_args; i++) {
		jaf_derive_types(env, expr->call.args->items[i]);
	}

	int func_no = expr->call.fun->valuetype.struc;
	assert(func_no >= 0 && func_no < env->ain->nr_functions);
	expr->valuetype = env->ain->functions[func_no].return_type;
	expr->call.func_no = func_no;

	struct ain_function *f = &env->ain->functions[func_no];
	expr->call.args->var_nos = xcalloc(expr->call.args->nr_items, sizeof(int));

	// type check arguments against function prototype.
	int arg = 0;
	for (size_t i = 0; i < nr_args; i++, arg++) {
		if (arg >= f->nr_args)
			ERROR("Too many arguments to function");

		jaf_check_type(expr->call.args->items[i], &f->vars[arg].type);

		expr->call.args->var_nos[i] = arg;
		if (ain_wide_type(f->vars[arg].type.data))
			arg++;
	}
	if (arg != f->nr_args)
		ERROR("Too few arguments to function");
}

static void jaf_check_types_syscall(struct jaf_env *env, struct jaf_expression *expr)
{
	int no = expr->call.func_no;
	unsigned nr_args = expr->call.args ? expr->call.args->nr_items : 0;
	for (size_t i = 0; i < nr_args; i++) {
		jaf_derive_types(env, expr->call.args->items[i]);
	}

	assert(no >= 0 && no < NR_SYSCALLS);
	if (nr_args != syscalls[no].nr_args) {
		ERROR("Wrong number of arguments to syscall 'system.%s' (expected %d; got %d)",
		      syscalls[no].name, syscalls[no].nr_args, nr_args);
	}

	for (unsigned i = 0; i < nr_args; i++) {
		struct ain_type type = {
			.data = syscalls[no].argtypes[i],
			.struc = -1,
			.rank = 0
		};
		jaf_check_type(expr->call.args->items[i], &type);
	}

	expr->valuetype = syscalls[no].return_type;
}

static void jaf_check_types_member(struct jaf_env *env, struct jaf_expression *expr)
{
	jaf_derive_types(env, expr->member.struc);
	TYPE_CHECK(expr->member.struc, AIN_STRUCT);

	char *u = encode_text_to_input_format(expr->member.name->text);
	struct ain_struct *s = &env->ain->structures[expr->member.struc->valuetype.struc];
	for (int i = 0; i < s->nr_members; i++) {
		if (!strcmp(s->members[i].name, u)) {
			expr->valuetype = s->members[i].type;
			break;
		}
	}
	free(u);
}

static void jaf_check_types_seq(struct jaf_env *env, struct jaf_expression *expr)
{
	jaf_derive_types(env, expr->seq.head);
	jaf_derive_types(env, expr->seq.tail);
	expr->valuetype = expr->seq.tail->valuetype;
}

static enum ain_data_type array_data_type(enum ain_data_type type)
{
	switch (type) {
	case AIN_ARRAY_INT:       case AIN_REF_ARRAY_INT:       return AIN_INT;
	case AIN_ARRAY_FLOAT:     case AIN_REF_ARRAY_FLOAT:     return AIN_FLOAT;
	case AIN_ARRAY_STRING:    case AIN_REF_ARRAY_STRING:    return AIN_STRING;
	case AIN_ARRAY_STRUCT:    case AIN_REF_ARRAY_STRUCT:    return AIN_STRUCT;
	case AIN_ARRAY_FUNC_TYPE: case AIN_REF_ARRAY_FUNC_TYPE: return AIN_FUNC_TYPE;
	case AIN_ARRAY_BOOL:      case AIN_REF_ARRAY_BOOL:      return AIN_BOOL;
	case AIN_ARRAY_LONG_INT:  case AIN_REF_ARRAY_LONG_INT:  return AIN_LONG_INT;
	case AIN_ARRAY_DELEGATE:  case AIN_REF_ARRAY_DELEGATE:  return AIN_DELEGATE;
		break;
	case AIN_ARRAY:
		ERROR("ain v11+ arrays not supported");
	default:
		return AIN_VOID;
	}
}

static void jaf_check_types_subscript(struct jaf_env *env, struct jaf_expression *expr)
{
	jaf_derive_types(env, expr->subscript.expr);
	jaf_derive_types(env, expr->subscript.index);
	jaf_type_check_int(expr->subscript.index);
	expr->valuetype.data = array_data_type(expr->subscript.expr->valuetype.data);
	expr->valuetype.struc = expr->subscript.expr->valuetype.struc;
	if (expr->valuetype.data == AIN_VOID)
		TYPE_ERROR(expr->subscript.expr, AIN_ARRAY);
}

void jaf_derive_types(struct jaf_env *env, struct jaf_expression *expr)
{
	switch (expr->type) {
	case JAF_EXP_VOID:
		expr->valuetype.data = AIN_VOID;
		break;
	case JAF_EXP_INT:
		expr->valuetype.data = AIN_INT;
		break;
	case JAF_EXP_FLOAT:
		expr->valuetype.data = AIN_FLOAT;
		break;
	case JAF_EXP_STRING:
		expr->valuetype.data = AIN_STRING;
		break;
	case JAF_EXP_IDENTIFIER:
		jaf_check_types_identifier(env, expr);
		break;
	case JAF_EXP_UNARY:
		jaf_check_types_unary(env, expr);
		break;
	case JAF_EXP_BINARY:
		jaf_check_types_binary(env, expr);
		break;
	case JAF_EXP_TERNARY:
		jaf_check_types_ternary(env, expr);
		break;
	case JAF_EXP_FUNCALL:
		jaf_check_types_funcall(env, expr);
		break;
	case JAF_EXP_SYSCALL:
		jaf_check_types_syscall(env, expr);
		break;
	case JAF_EXP_CAST:
		expr->valuetype.data = jaf_to_ain_data_type(expr->cast.type, 0);
		break;
	case JAF_EXP_MEMBER:
		jaf_check_types_member(env, expr);
		break;
	case JAF_EXP_SEQ:
		jaf_check_types_seq(env, expr);
		break;
	case JAF_EXP_SUBSCRIPT:
		jaf_check_types_subscript(env, expr);
		break;
	}
}

void jaf_check_type(struct jaf_expression *expr, struct ain_type *type)
{
	if (!jaf_type_equal(&expr->valuetype, type)) {
		// treat ints and floats as interchangable
		if (expr->valuetype.data == AIN_INT && type->data == AIN_FLOAT)
			return;
		if (expr->valuetype.data == AIN_FLOAT && type->data == AIN_INT)
			return;
		TYPE_ERROR(expr, type->data);
	}
}
