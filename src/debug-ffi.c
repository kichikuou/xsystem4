/* Automatically generated by chibi-ffi; version: 0.4 */

#include <chibi/eval.h>

#include "system4/ain.h"

#include "vm/page.h"

#include "vm.h"
/*
types: (variable vm_value page)
enums: ()
*/

sexp sexp_get_local_variable_stub (sexp ctx, sexp self, sexp_sint_t n, sexp arg0) {
  sexp res;
  if (! sexp_stringp(arg0))
    return sexp_type_exception(ctx, self, SEXP_STRING, arg0);
  res = sexp_make_cpointer(ctx, sexp_unbox_fixnum(sexp_opcode_return_type(self)), get_local_by_name(sexp_string_data(arg0)), SEXP_FALSE, 1);
  return res;
}

sexp sexp_get_global_variable_stub (sexp ctx, sexp self, sexp_sint_t n, sexp arg0) {
  sexp res;
  if (! sexp_stringp(arg0))
    return sexp_type_exception(ctx, self, SEXP_STRING, arg0);
  res = sexp_make_cpointer(ctx, sexp_unbox_fixnum(sexp_opcode_return_type(self)), get_global_by_name(sexp_string_data(arg0)), SEXP_FALSE, 1);
  return res;
}

sexp sexp_set_address_breakpoint_stub (sexp ctx, sexp self, sexp_sint_t n, sexp arg0, sexp arg1) {
  sexp res;
  if (! sexp_exact_integerp(arg0))
    return sexp_type_exception(ctx, self, SEXP_FIXNUM, arg0);
  res = ((set_address_breakpoint(sexp_uint_value(arg0), arg1)), SEXP_VOID);
  return res;
}

sexp sexp_set_function_breakpoint_stub (sexp ctx, sexp self, sexp_sint_t n, sexp arg0, sexp arg1) {
  sexp res;
  if (! sexp_stringp(arg0))
    return sexp_type_exception(ctx, self, SEXP_STRING, arg0);
  res = ((set_function_breakpoint(sexp_string_data(arg0), arg1)), SEXP_VOID);
  return res;
}

sexp sexp_page_ref_stub (sexp ctx, sexp self, sexp_sint_t n, sexp arg0, sexp arg1) {
  sexp res;
  if (! (sexp_pointerp(arg0) && (sexp_pointer_tag(arg0) == sexp_unbox_fixnum(sexp_opcode_arg1_type(self)))))
    return sexp_type_exception(ctx, self, sexp_unbox_fixnum(sexp_opcode_arg1_type(self)), arg0);
  if (! sexp_exact_integerp(arg1))
    return sexp_type_exception(ctx, self, SEXP_FIXNUM, arg1);
  res = sexp_make_cpointer(ctx, sexp_unbox_fixnum(sexp_opcode_return_type(self)), page_ref((struct page*)sexp_cpointer_value(arg0), sexp_sint_value(arg1)), SEXP_FALSE, 1);
  return res;
}

sexp sexp_frame_page_stub (sexp ctx, sexp self, sexp_sint_t n, sexp arg0) {
  sexp res;
  if (! sexp_exact_integerp(arg0))
    return sexp_type_exception(ctx, self, SEXP_FIXNUM, arg0);
  res = sexp_make_cpointer(ctx, sexp_unbox_fixnum(sexp_opcode_return_type(self)), frame_page(sexp_sint_value(arg0)), SEXP_FALSE, 0);
  return res;
}

sexp sexp_get_page_stub (sexp ctx, sexp self, sexp_sint_t n, sexp arg0) {
  sexp res;
  if (! sexp_exact_integerp(arg0))
    return sexp_type_exception(ctx, self, SEXP_FIXNUM, arg0);
  res = sexp_make_cpointer(ctx, sexp_unbox_fixnum(sexp_opcode_return_type(self)), get_page(sexp_sint_value(arg0)), SEXP_FALSE, 0);
  return res;
}

sexp sexp_backtrace_stub (sexp ctx, sexp self, sexp_sint_t n) {
  sexp res;
  res = ((dbg_print_stack_trace()), SEXP_VOID);
  return res;
}

sexp sexp_vm_value_string_stub (sexp ctx, sexp self, sexp_sint_t n, sexp arg0) {
  sexp res;
  if (! (sexp_pointerp(arg0) && (sexp_pointer_tag(arg0) == sexp_unbox_fixnum(sexp_opcode_arg1_type(self)))))
    return sexp_type_exception(ctx, self, sexp_unbox_fixnum(sexp_opcode_arg1_type(self)), arg0);
  res = sexp_c_string(ctx, vm_value_string((union vm_value*)sexp_cpointer_value(arg0)), -1);
  return res;
}

sexp sexp_free_variable_stub (sexp ctx, sexp self, sexp_sint_t n, sexp x) {
  if (sexp_cpointer_freep(x)) {
    free_variable(
#ifdef __cplusplus
(variable*)
#endif
sexp_cpointer_value(x));
    sexp_cpointer_freep(x) = 0;
  }
  return SEXP_VOID;
}

sexp sexp_variable_get_data_type (sexp ctx, sexp self, sexp_sint_t n, sexp x) {
  if (! (sexp_pointerp(x) && (sexp_pointer_tag(x) == sexp_unbox_fixnum(sexp_opcode_arg1_type(self)))))
    return sexp_type_exception(ctx, self, sexp_unbox_fixnum(sexp_opcode_arg1_type(self)), x);
  return sexp_make_integer(ctx, ((struct variable*)sexp_cpointer_value(x))->data_type);
}

sexp sexp_variable_get_struct_type (sexp ctx, sexp self, sexp_sint_t n, sexp x) {
  if (! (sexp_pointerp(x) && (sexp_pointer_tag(x) == sexp_unbox_fixnum(sexp_opcode_arg1_type(self)))))
    return sexp_type_exception(ctx, self, sexp_unbox_fixnum(sexp_opcode_arg1_type(self)), x);
  return sexp_make_integer(ctx, ((struct variable*)sexp_cpointer_value(x))->struct_type);
}

sexp sexp_variable_get_name (sexp ctx, sexp self, sexp_sint_t n, sexp x) {
  if (! (sexp_pointerp(x) && (sexp_pointer_tag(x) == sexp_unbox_fixnum(sexp_opcode_arg1_type(self)))))
    return sexp_type_exception(ctx, self, sexp_unbox_fixnum(sexp_opcode_arg1_type(self)), x);
  return sexp_c_string(ctx, ((struct variable*)sexp_cpointer_value(x))->name, -1);
}

sexp sexp_variable_get_varno (sexp ctx, sexp self, sexp_sint_t n, sexp x) {
  if (! (sexp_pointerp(x) && (sexp_pointer_tag(x) == sexp_unbox_fixnum(sexp_opcode_arg1_type(self)))))
    return sexp_type_exception(ctx, self, sexp_unbox_fixnum(sexp_opcode_arg1_type(self)), x);
  return sexp_make_integer(ctx, ((struct variable*)sexp_cpointer_value(x))->varno);
}

sexp sexp_variable_get_value (sexp ctx, sexp self, sexp_sint_t n, sexp x) {
  if (! (sexp_pointerp(x) && (sexp_pointer_tag(x) == sexp_unbox_fixnum(sexp_opcode_arg1_type(self)))))
    return sexp_type_exception(ctx, self, sexp_unbox_fixnum(sexp_opcode_arg1_type(self)), x);
  return sexp_make_cpointer(ctx, sexp_unbox_fixnum(sexp_opcode_return_type(self)), ((struct variable*)sexp_cpointer_value(x))->value, SEXP_FALSE, 0);
}

sexp sexp_vm_value_get_i (sexp ctx, sexp self, sexp_sint_t n, sexp x) {
  if (! (sexp_pointerp(x) && (sexp_pointer_tag(x) == sexp_unbox_fixnum(sexp_opcode_arg1_type(self)))))
    return sexp_type_exception(ctx, self, sexp_unbox_fixnum(sexp_opcode_arg1_type(self)), x);
  return sexp_make_integer(ctx, ((union vm_value*)sexp_cpointer_value(x))->i);
}

sexp sexp_vm_value_get_f (sexp ctx, sexp self, sexp_sint_t n, sexp x) {
  if (! (sexp_pointerp(x) && (sexp_pointer_tag(x) == sexp_unbox_fixnum(sexp_opcode_arg1_type(self)))))
    return sexp_type_exception(ctx, self, sexp_unbox_fixnum(sexp_opcode_arg1_type(self)), x);
  return sexp_make_flonum(ctx, ((union vm_value*)sexp_cpointer_value(x))->f);
}

sexp sexp_page_get_type (sexp ctx, sexp self, sexp_sint_t n, sexp x) {
  if (! (sexp_pointerp(x) && (sexp_pointer_tag(x) == sexp_unbox_fixnum(sexp_opcode_arg1_type(self)))))
    return sexp_type_exception(ctx, self, sexp_unbox_fixnum(sexp_opcode_arg1_type(self)), x);
  return sexp_make_integer(ctx, ((struct page*)sexp_cpointer_value(x))->type);
}

sexp sexp_page_get_index (sexp ctx, sexp self, sexp_sint_t n, sexp x) {
  if (! (sexp_pointerp(x) && (sexp_pointer_tag(x) == sexp_unbox_fixnum(sexp_opcode_arg1_type(self)))))
    return sexp_type_exception(ctx, self, sexp_unbox_fixnum(sexp_opcode_arg1_type(self)), x);
  return sexp_make_integer(ctx, ((struct page*)sexp_cpointer_value(x))->index);
}

sexp sexp_page_get_nr_vars (sexp ctx, sexp self, sexp_sint_t n, sexp x) {
  if (! (sexp_pointerp(x) && (sexp_pointer_tag(x) == sexp_unbox_fixnum(sexp_opcode_arg1_type(self)))))
    return sexp_type_exception(ctx, self, sexp_unbox_fixnum(sexp_opcode_arg1_type(self)), x);
  return sexp_make_integer(ctx, ((struct page*)sexp_cpointer_value(x))->nr_vars);
}

sexp sexp_page_get_a_type (sexp ctx, sexp self, sexp_sint_t n, sexp x) {
  if (! (sexp_pointerp(x) && (sexp_pointer_tag(x) == sexp_unbox_fixnum(sexp_opcode_arg1_type(self)))))
    return sexp_type_exception(ctx, self, sexp_unbox_fixnum(sexp_opcode_arg1_type(self)), x);
  return sexp_make_integer(ctx, ((struct page*)sexp_cpointer_value(x))->a_type);
}

sexp sexp_page_get__page_struct_type (sexp ctx, sexp self, sexp_sint_t n, sexp x) {
  if (! (sexp_pointerp(x) && (sexp_pointer_tag(x) == sexp_unbox_fixnum(sexp_opcode_arg1_type(self)))))
    return sexp_type_exception(ctx, self, sexp_unbox_fixnum(sexp_opcode_arg1_type(self)), x);
  return sexp_make_integer(ctx, ((struct page*)sexp_cpointer_value(x))->_page_struct_type);
}

sexp sexp_page_get__page_rank (sexp ctx, sexp self, sexp_sint_t n, sexp x) {
  if (! (sexp_pointerp(x) && (sexp_pointer_tag(x) == sexp_unbox_fixnum(sexp_opcode_arg1_type(self)))))
    return sexp_type_exception(ctx, self, sexp_unbox_fixnum(sexp_opcode_arg1_type(self)), x);
  return sexp_make_integer(ctx, ((struct page*)sexp_cpointer_value(x))->_page_rank);
}


sexp sexp_init_library (sexp ctx, sexp self, sexp_sint_t n, sexp env, const char* version, const sexp_abi_identifier_t abi) {
  sexp sexp_variable_type_obj;
  sexp sexp_vm_value_type_obj;
  sexp sexp_page_type_obj;
  sexp_gc_var3(name, tmp, op);
  if (!(sexp_version_compatible(ctx, version, sexp_version)
        && sexp_abi_compatible(ctx, abi, SEXP_ABI_IDENTIFIER)))
    return SEXP_ABI_ERROR;
  sexp_gc_preserve3(ctx, name, tmp, op);
  name = sexp_intern(ctx, "page-type/array", 15);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, ARRAY_PAGE));
  name = sexp_intern(ctx, "page-type/struct", 16);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, STRUCT_PAGE));
  name = sexp_intern(ctx, "page-type/local", 15);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, LOCAL_PAGE));
  name = sexp_intern(ctx, "page-type/global", 16);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, GLOBAL_PAGE));
  name = sexp_intern(ctx, "type/ref-array-delegate", 23);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, AIN_REF_ARRAY_DELEGATE));
  name = sexp_intern(ctx, "type/array-delegate", 19);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, AIN_ARRAY_DELEGATE));
  name = sexp_intern(ctx, "type/delegate", 13);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, AIN_DELEGATE));
  name = sexp_intern(ctx, "type/ref-array-long", 19);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, AIN_REF_ARRAY_LONG_INT));
  name = sexp_intern(ctx, "type/ref-long", 13);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, AIN_REF_LONG_INT));
  name = sexp_intern(ctx, "type/array-long", 15);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, AIN_ARRAY_LONG_INT));
  name = sexp_intern(ctx, "type/long", 9);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, AIN_LONG_INT));
  name = sexp_intern(ctx, "type/ref-array-bool", 19);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, AIN_REF_ARRAY_BOOL));
  name = sexp_intern(ctx, "type/ref-bool", 13);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, AIN_REF_BOOL));
  name = sexp_intern(ctx, "type/array-bool", 15);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, AIN_ARRAY_BOOL));
  name = sexp_intern(ctx, "type/bool", 9);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, AIN_BOOL));
  name = sexp_intern(ctx, "type/ref-array-func-type", 24);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, AIN_REF_ARRAY_FUNC_TYPE));
  name = sexp_intern(ctx, "type/ref-func-type", 18);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, AIN_REF_FUNC_TYPE));
  name = sexp_intern(ctx, "type/array-func-type", 20);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, AIN_ARRAY_FUNC_TYPE));
  name = sexp_intern(ctx, "type/func-type", 14);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, AIN_FUNC_TYPE));
  name = sexp_intern(ctx, "type/imain-system", 17);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, AIN_IMAIN_SYSTEM));
  name = sexp_intern(ctx, "type/ref-array-struct", 21);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, AIN_REF_ARRAY_STRUCT));
  name = sexp_intern(ctx, "type/ref-array-string", 21);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, AIN_REF_ARRAY_STRING));
  name = sexp_intern(ctx, "type/ref-array-float", 20);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, AIN_REF_ARRAY_FLOAT));
  name = sexp_intern(ctx, "type/ref-array-int", 18);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, AIN_REF_ARRAY_INT));
  name = sexp_intern(ctx, "type/ref-struct", 15);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, AIN_REF_STRUCT));
  name = sexp_intern(ctx, "type/ref-string", 15);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, AIN_REF_STRING));
  name = sexp_intern(ctx, "type/ref-float", 14);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, AIN_REF_FLOAT));
  name = sexp_intern(ctx, "type/ref-int", 12);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, AIN_REF_INT));
  name = sexp_intern(ctx, "type/array-struct", 17);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, AIN_ARRAY_STRUCT));
  name = sexp_intern(ctx, "type/array-string", 17);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, AIN_ARRAY_STRING));
  name = sexp_intern(ctx, "type/array-float", 16);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, AIN_ARRAY_FLOAT));
  name = sexp_intern(ctx, "type/array-int", 14);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, AIN_ARRAY_INT));
  name = sexp_intern(ctx, "type/struct", 11);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, AIN_STRUCT));
  name = sexp_intern(ctx, "type/string", 11);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, AIN_STRING));
  name = sexp_intern(ctx, "type/float", 10);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, AIN_FLOAT));
  name = sexp_intern(ctx, "type/int", 8);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, AIN_INT));
  name = sexp_intern(ctx, "type/void", 9);
  sexp_env_define(ctx, env, name, tmp=sexp_make_integer(ctx, AIN_VOID));
  name = sexp_c_string(ctx, "variable", -1);
  sexp_variable_type_obj = sexp_register_c_type(ctx, name, sexp_free_variable_stub);
  tmp = sexp_string_to_symbol(ctx, name);
  sexp_env_define(ctx, env, tmp, sexp_variable_type_obj);
  sexp_type_slots(sexp_variable_type_obj) = SEXP_NULL;
  sexp_push(ctx, sexp_type_slots(sexp_variable_type_obj), sexp_intern(ctx, "value", -1));
  sexp_push(ctx, sexp_type_slots(sexp_variable_type_obj), sexp_intern(ctx, "varno", -1));
  sexp_push(ctx, sexp_type_slots(sexp_variable_type_obj), sexp_intern(ctx, "name", -1));
  sexp_push(ctx, sexp_type_slots(sexp_variable_type_obj), sexp_intern(ctx, "struct_type", -1));
  sexp_push(ctx, sexp_type_slots(sexp_variable_type_obj), sexp_intern(ctx, "data_type", -1));
  sexp_type_getters(sexp_variable_type_obj) = sexp_make_vector(ctx, SEXP_FIVE, SEXP_FALSE);
  sexp_type_setters(sexp_variable_type_obj) = sexp_make_vector(ctx, SEXP_FIVE, SEXP_FALSE);
  tmp = sexp_make_type_predicate(ctx, name, sexp_variable_type_obj);
  name = sexp_intern(ctx, "variable?", 9);
  sexp_env_define(ctx, env, name, tmp);
  name = sexp_c_string(ctx, "vm_value", -1);
  sexp_vm_value_type_obj = sexp_register_c_type(ctx, name, sexp_finalize_c_type);
  tmp = sexp_string_to_symbol(ctx, name);
  sexp_env_define(ctx, env, tmp, sexp_vm_value_type_obj);
  sexp_type_slots(sexp_vm_value_type_obj) = SEXP_NULL;
  sexp_push(ctx, sexp_type_slots(sexp_vm_value_type_obj), sexp_intern(ctx, "f", -1));
  sexp_push(ctx, sexp_type_slots(sexp_vm_value_type_obj), sexp_intern(ctx, "i", -1));
  sexp_type_getters(sexp_vm_value_type_obj) = sexp_make_vector(ctx, SEXP_TWO, SEXP_FALSE);
  sexp_type_setters(sexp_vm_value_type_obj) = sexp_make_vector(ctx, SEXP_TWO, SEXP_FALSE);
  tmp = sexp_make_type_predicate(ctx, name, sexp_vm_value_type_obj);
  name = sexp_intern(ctx, "vm-value?", 9);
  sexp_env_define(ctx, env, name, tmp);
  name = sexp_c_string(ctx, "page", -1);
  sexp_page_type_obj = sexp_register_c_type(ctx, name, sexp_finalize_c_type);
  tmp = sexp_string_to_symbol(ctx, name);
  sexp_env_define(ctx, env, tmp, sexp_page_type_obj);
  sexp_type_slots(sexp_page_type_obj) = SEXP_NULL;
  sexp_push(ctx, sexp_type_slots(sexp_page_type_obj), sexp_intern(ctx, "_page_rank", -1));
  sexp_push(ctx, sexp_type_slots(sexp_page_type_obj), sexp_intern(ctx, "_page_struct_type", -1));
  sexp_push(ctx, sexp_type_slots(sexp_page_type_obj), sexp_intern(ctx, "a_type", -1));
  sexp_push(ctx, sexp_type_slots(sexp_page_type_obj), sexp_intern(ctx, "nr_vars", -1));
  sexp_push(ctx, sexp_type_slots(sexp_page_type_obj), sexp_intern(ctx, "index", -1));
  sexp_push(ctx, sexp_type_slots(sexp_page_type_obj), sexp_intern(ctx, "type", -1));
  sexp_type_getters(sexp_page_type_obj) = sexp_make_vector(ctx, SEXP_SIX, SEXP_FALSE);
  sexp_type_setters(sexp_page_type_obj) = sexp_make_vector(ctx, SEXP_SIX, SEXP_FALSE);
  tmp = sexp_make_type_predicate(ctx, name, sexp_page_type_obj);
  name = sexp_intern(ctx, "page?", 5);
  sexp_env_define(ctx, env, name, tmp);
  op = sexp_define_foreign(ctx, env, "array-rank", 1, sexp_page_get__page_rank);
  if (sexp_opcodep(op)) {
    sexp_opcode_return_type(op) = sexp_make_fixnum(SEXP_FIXNUM);
    sexp_opcode_arg1_type(op) = sexp_make_fixnum(sexp_type_tag(sexp_page_type_obj));
  }
  if (sexp_vectorp(sexp_type_getters(sexp_page_type_obj))) sexp_vector_set(sexp_type_getters(sexp_page_type_obj), SEXP_FIVE, op);
  op = sexp_define_foreign(ctx, env, "array-struct-type", 1, sexp_page_get__page_struct_type);
  if (sexp_opcodep(op)) {
    sexp_opcode_return_type(op) = sexp_make_fixnum(SEXP_FIXNUM);
    sexp_opcode_arg1_type(op) = sexp_make_fixnum(sexp_type_tag(sexp_page_type_obj));
  }
  if (sexp_vectorp(sexp_type_getters(sexp_page_type_obj))) sexp_vector_set(sexp_type_getters(sexp_page_type_obj), SEXP_FOUR, op);
  op = sexp_define_foreign(ctx, env, "array-data-type", 1, sexp_page_get_a_type);
  if (sexp_opcodep(op)) {
    sexp_opcode_return_type(op) = sexp_make_fixnum(SEXP_FIXNUM);
    sexp_opcode_arg1_type(op) = sexp_make_fixnum(sexp_type_tag(sexp_page_type_obj));
  }
  if (sexp_vectorp(sexp_type_getters(sexp_page_type_obj))) sexp_vector_set(sexp_type_getters(sexp_page_type_obj), SEXP_THREE, op);
  op = sexp_define_foreign(ctx, env, "page-nr-vars", 1, sexp_page_get_nr_vars);
  if (sexp_opcodep(op)) {
    sexp_opcode_return_type(op) = sexp_make_fixnum(SEXP_FIXNUM);
    sexp_opcode_arg1_type(op) = sexp_make_fixnum(sexp_type_tag(sexp_page_type_obj));
  }
  if (sexp_vectorp(sexp_type_getters(sexp_page_type_obj))) sexp_vector_set(sexp_type_getters(sexp_page_type_obj), SEXP_TWO, op);
  op = sexp_define_foreign(ctx, env, "page-type-index", 1, sexp_page_get_index);
  if (sexp_opcodep(op)) {
    sexp_opcode_return_type(op) = sexp_make_fixnum(SEXP_FIXNUM);
    sexp_opcode_arg1_type(op) = sexp_make_fixnum(sexp_type_tag(sexp_page_type_obj));
  }
  if (sexp_vectorp(sexp_type_getters(sexp_page_type_obj))) sexp_vector_set(sexp_type_getters(sexp_page_type_obj), SEXP_ONE, op);
  op = sexp_define_foreign(ctx, env, "page-type", 1, sexp_page_get_type);
  if (sexp_opcodep(op)) {
    sexp_opcode_return_type(op) = sexp_make_fixnum(SEXP_FIXNUM);
    sexp_opcode_arg1_type(op) = sexp_make_fixnum(sexp_type_tag(sexp_page_type_obj));
  }
  if (sexp_vectorp(sexp_type_getters(sexp_page_type_obj))) sexp_vector_set(sexp_type_getters(sexp_page_type_obj), SEXP_ZERO, op);
  op = sexp_define_foreign(ctx, env, "vm-value-float", 1, sexp_vm_value_get_f);
  if (sexp_opcodep(op)) {
    sexp_opcode_return_type(op) = sexp_make_fixnum(SEXP_FLONUM);
    sexp_opcode_arg1_type(op) = sexp_make_fixnum(sexp_type_tag(sexp_vm_value_type_obj));
  }
  if (sexp_vectorp(sexp_type_getters(sexp_vm_value_type_obj))) sexp_vector_set(sexp_type_getters(sexp_vm_value_type_obj), SEXP_ONE, op);
  op = sexp_define_foreign(ctx, env, "vm-value-int", 1, sexp_vm_value_get_i);
  if (sexp_opcodep(op)) {
    sexp_opcode_return_type(op) = sexp_make_fixnum(SEXP_FIXNUM);
    sexp_opcode_arg1_type(op) = sexp_make_fixnum(sexp_type_tag(sexp_vm_value_type_obj));
  }
  if (sexp_vectorp(sexp_type_getters(sexp_vm_value_type_obj))) sexp_vector_set(sexp_type_getters(sexp_vm_value_type_obj), SEXP_ZERO, op);
  op = sexp_define_foreign(ctx, env, "variable-vm-value", 1, sexp_variable_get_value);
  if (sexp_opcodep(op)) {
    sexp_opcode_return_type(op) = sexp_make_fixnum(sexp_type_tag(sexp_vm_value_type_obj));
    sexp_opcode_arg1_type(op) = sexp_make_fixnum(sexp_type_tag(sexp_variable_type_obj));
  }
  if (sexp_vectorp(sexp_type_getters(sexp_variable_type_obj))) sexp_vector_set(sexp_type_getters(sexp_variable_type_obj), SEXP_FOUR, op);
  op = sexp_define_foreign(ctx, env, "variable-varno", 1, sexp_variable_get_varno);
  if (sexp_opcodep(op)) {
    sexp_opcode_return_type(op) = sexp_make_fixnum(SEXP_FIXNUM);
    sexp_opcode_arg1_type(op) = sexp_make_fixnum(sexp_type_tag(sexp_variable_type_obj));
  }
  if (sexp_vectorp(sexp_type_getters(sexp_variable_type_obj))) sexp_vector_set(sexp_type_getters(sexp_variable_type_obj), SEXP_THREE, op);
  op = sexp_define_foreign(ctx, env, "variable-name", 1, sexp_variable_get_name);
  if (sexp_opcodep(op)) {
    sexp_opcode_return_type(op) = sexp_make_fixnum(SEXP_STRING);
    sexp_opcode_arg1_type(op) = sexp_make_fixnum(sexp_type_tag(sexp_variable_type_obj));
  }
  if (sexp_vectorp(sexp_type_getters(sexp_variable_type_obj))) sexp_vector_set(sexp_type_getters(sexp_variable_type_obj), SEXP_TWO, op);
  op = sexp_define_foreign(ctx, env, "variable-struct-type", 1, sexp_variable_get_struct_type);
  if (sexp_opcodep(op)) {
    sexp_opcode_return_type(op) = sexp_make_fixnum(SEXP_FIXNUM);
    sexp_opcode_arg1_type(op) = sexp_make_fixnum(sexp_type_tag(sexp_variable_type_obj));
  }
  if (sexp_vectorp(sexp_type_getters(sexp_variable_type_obj))) sexp_vector_set(sexp_type_getters(sexp_variable_type_obj), SEXP_ONE, op);
  op = sexp_define_foreign(ctx, env, "variable-data-type", 1, sexp_variable_get_data_type);
  if (sexp_opcodep(op)) {
    sexp_opcode_return_type(op) = sexp_make_fixnum(SEXP_FIXNUM);
    sexp_opcode_arg1_type(op) = sexp_make_fixnum(sexp_type_tag(sexp_variable_type_obj));
  }
  if (sexp_vectorp(sexp_type_getters(sexp_variable_type_obj))) sexp_vector_set(sexp_type_getters(sexp_variable_type_obj), SEXP_ZERO, op);
  op = sexp_define_foreign(ctx, env, "free_variable", 1, sexp_free_variable_stub);
  if (sexp_opcodep(op)) {
    sexp_opcode_return_type(op) = SEXP_VOID;
    sexp_opcode_arg1_type(op) = sexp_make_fixnum(sexp_type_tag(sexp_variable_type_obj));
  }
  op = sexp_define_foreign(ctx, env, "get-local-variable", 1, sexp_get_local_variable_stub);
  if (sexp_opcodep(op)) {
    sexp_opcode_return_type(op) = sexp_make_fixnum(sexp_type_tag(sexp_variable_type_obj));
    sexp_opcode_arg1_type(op) = sexp_make_fixnum(SEXP_STRING);
  }
  op = sexp_define_foreign(ctx, env, "get-global-variable", 1, sexp_get_global_variable_stub);
  if (sexp_opcodep(op)) {
    sexp_opcode_return_type(op) = sexp_make_fixnum(sexp_type_tag(sexp_variable_type_obj));
    sexp_opcode_arg1_type(op) = sexp_make_fixnum(SEXP_STRING);
  }
  op = sexp_define_foreign(ctx, env, "set-address-breakpoint", 2, sexp_set_address_breakpoint_stub);
  if (sexp_opcodep(op)) {
    sexp_opcode_return_type(op) = SEXP_VOID;
    sexp_opcode_arg1_type(op) = sexp_make_fixnum(SEXP_FIXNUM);
  }
  op = sexp_define_foreign(ctx, env, "set-function-breakpoint", 2, sexp_set_function_breakpoint_stub);
  if (sexp_opcodep(op)) {
    sexp_opcode_return_type(op) = SEXP_VOID;
    sexp_opcode_arg1_type(op) = sexp_make_fixnum(SEXP_STRING);
  }
  op = sexp_define_foreign(ctx, env, "page-ref", 2, sexp_page_ref_stub);
  if (sexp_opcodep(op)) {
    sexp_opcode_return_type(op) = sexp_make_fixnum(sexp_type_tag(sexp_variable_type_obj));
    sexp_opcode_arg1_type(op) = sexp_make_fixnum(sexp_type_tag(sexp_page_type_obj));
    sexp_opcode_arg2_type(op) = sexp_make_fixnum(SEXP_FIXNUM);
  }
  op = sexp_define_foreign(ctx, env, "frame-page", 1, sexp_frame_page_stub);
  if (sexp_opcodep(op)) {
    sexp_opcode_return_type(op) = sexp_make_fixnum(sexp_type_tag(sexp_page_type_obj));
    sexp_opcode_arg1_type(op) = sexp_make_fixnum(SEXP_FIXNUM);
  }
  op = sexp_define_foreign(ctx, env, "get-page", 1, sexp_get_page_stub);
  if (sexp_opcodep(op)) {
    sexp_opcode_return_type(op) = sexp_make_fixnum(sexp_type_tag(sexp_page_type_obj));
    sexp_opcode_arg1_type(op) = sexp_make_fixnum(SEXP_FIXNUM);
  }
  op = sexp_define_foreign(ctx, env, "backtrace", 0, sexp_backtrace_stub);
  if (sexp_opcodep(op)) {
    sexp_opcode_return_type(op) = SEXP_VOID;
  }
  op = sexp_define_foreign(ctx, env, "vm-value-string", 1, sexp_vm_value_string_stub);
  if (sexp_opcodep(op)) {
    sexp_opcode_return_type(op) = sexp_make_fixnum(SEXP_STRING);
    sexp_opcode_arg1_type(op) = sexp_make_fixnum(sexp_type_tag(sexp_vm_value_type_obj));
  }
  sexp_gc_release3(ctx);
  return SEXP_VOID;
}

