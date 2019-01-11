/*
  +----------------------------------------------------------------------+
  | sandbox                                                              |
  +----------------------------------------------------------------------+
  | Copyright (c) Joe Watkins 2019                                       |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: krakjoe                                                      |
  +----------------------------------------------------------------------+
 */
#ifndef HAVE_SANDBOX_COPY
#define HAVE_SANDBOX_COPY

#include "php.h"
#include "sandbox.h"

#include "copy.h"

#include <Zend/zend_vm.h>

extern zend_string* php_sandbox_main;

/* {{{ */
static inline HashTable* php_sandbox_copy_statics(HashTable *old) {
	return zend_array_dup(old);
} /* }}} */

/* {{{ */
static inline zend_string** php_sandbox_copy_variables(zend_string **old, int end) {
	zend_string **variables = safe_emalloc(end, sizeof(zend_string*), 0);
	int it = 0;
	
	while (it < end) {
		variables[it] = zend_string_copy(old[it]);
		it++;
	}
	
	return variables;
} /* }}} */

/* {{{ */
static inline zend_try_catch_element* php_sandbox_copy_try(zend_try_catch_element *old, int end) {	
	zend_try_catch_element *try_catch = safe_emalloc(end, sizeof(zend_try_catch_element), 0);
	
	memcpy(
		try_catch, 
		old,
		sizeof(zend_try_catch_element) * end);
	
	return try_catch;
} /* }}} */

static inline zend_live_range* php_sandbox_copy_live(zend_live_range *old, int end) { /* {{{ */
	zend_live_range *range = safe_emalloc(end, sizeof(zend_live_range), 0);

	memcpy(
		range,
		old,
		sizeof(zend_live_range) * end);

	return range;
} /* }}} */

/* {{{ */
static inline zval* php_sandbox_copy_literals(zval *old, int end) {
	zval *literals = (zval*) safe_emalloc(end, sizeof(zval), 0);
	int it = 0;

	memcpy(literals, old, sizeof(zval) * end);

	while (it < end) {
		zval_copy_ctor(&literals[it]);	
		it++;
	}
	
	return literals;
} /* }}} */

/* {{{ */
static inline zend_op* php_sandbox_copy_opcodes(zend_op_array *op_array, zval *literals) {
	zend_op *copy = safe_emalloc(
		op_array->last, sizeof(zend_op), 0);

	memcpy(copy, op_array->opcodes, sizeof(zend_op) * op_array->last);

	{
		zend_op *opline = copy;
		zend_op *end    = copy + op_array->last;

		for (; opline < end; opline++) {
#if ZEND_USE_ABS_CONST_ADDR
			if (opline->op1_type == IS_CONST)
				opline->op1.zv = (zval*)((char*)opline->op1.zv + ((char*)op_array->literals - (char*)literals));
			if (opline->op2_type == IS_CONST) 
				opline->op2.zv = (zval*)((char*)opline->op2.zv + ((char*)op_array->literals - (char*)literals));
#elif PHP_VERSION_ID >= 70300
			if (opline->op1_type == IS_CONST) {
				opline->op1.constant =
					(char*)(op_array->literals +
						((zval*)((char*)(op_array->opcodes + (opline - copy)) +
						(int32_t)opline->op1.constant) - literals)) -
					(char*)opline;
				if (opline->opcode == ZEND_SEND_VAL
				 || opline->opcode == ZEND_SEND_VAL_EX
				 || opline->opcode == ZEND_QM_ASSIGN) {
					zend_vm_set_opcode_handler_ex(opline, 0, 0, 0);
				}
			}
			if (opline->op2_type == IS_CONST) {
				opline->op2.constant =
					(char*)(op_array->literals +
						((zval*)((char*)(op_array->opcodes + (opline - copy)) +
						(int32_t)opline->op2.constant) - literals)) -
					(char*)opline;
			}
#endif

#if ZEND_USE_ABS_JMP_ADDR
			if ((op_array->fn_flags & ZEND_ACC_DONE_PASS_TWO) != 0) {
				switch (opline->opcode) {
					case ZEND_JMP:
					case ZEND_FAST_CALL:
					case ZEND_DECLARE_ANON_CLASS:
					case ZEND_DECLARE_ANON_INHERITED_CLASS:
						 opline->op1.jmp_addr = &copy[opline->op1.jmp_addr - op_array->opcodes];
					break;

					case ZEND_JMPZNZ:
					case ZEND_JMPZ:
					case ZEND_JMPNZ:
					case ZEND_JMPZ_EX:
					case ZEND_JMPNZ_EX:
					case ZEND_JMP_SET:
					case ZEND_COALESCE:
					case ZEND_NEW:
					case ZEND_FE_RESET_R:
					case ZEND_FE_RESET_RW:
					case ZEND_ASSERT_CHECK:
						opline->op2.jmp_addr = &copy[opline->op2.jmp_addr - op_array->opcodes];
					break;
				}
			}
#endif
		}
	}

	return copy;
} /* }}} */

/* {{{ */
static inline zend_arg_info* php_sandbox_copy_arginfo(zend_op_array *op_array, zend_arg_info *old, uint32_t end) {
	zend_arg_info *info;
	uint32_t it = 0;

	if (op_array->fn_flags & ZEND_ACC_HAS_RETURN_TYPE) {
		old--;
		end++;
	}

	if (op_array->fn_flags & ZEND_ACC_VARIADIC) {
		end++;
	}

	info = safe_emalloc
		(end, sizeof(zend_arg_info), 0);
	memcpy(info, old, sizeof(zend_arg_info) * end);	

	while (it < end) {
		if (info[it].name)
			info[it].name = zend_string_copy(old[it].name);
		it++;
	}
	
	if (op_array->fn_flags & ZEND_ACC_HAS_RETURN_TYPE) {
		info++;
	}
	
	return info;
} /* }}} */

static zend_always_inline zend_bool php_sandbox_copying_lexical(zend_execute_data *execute_data, zend_function *function, zend_op *bind) { /* {{{ */
	zend_op *opline, *end;

	if (EX(func)->type != ZEND_USER_FUNCTION) {
		return 0;
	}
	
	opline = EX(func)->op_array.opcodes;
	end    = opline + EX(func)->op_array.last;

	while (opline < end) {
		if (opline->opcode == ZEND_BIND_LEXICAL) {
			if (zend_string_equals(
				zend_get_compiled_variable_name((zend_op_array*)function, bind->op1.var), 
				zend_get_compiled_variable_name((zend_op_array*)EX(func), opline->op2.var))) {
				return 1;
			}
		}
		opline++;
	}

	return 0;
} /* }}} */

zend_bool php_sandbox_copy_arginfo_check(zend_function *function) { /* {{{ */
	zend_arg_info *it, *end;
	int argc = 1;

	if (!function->op_array.arg_info) {
		return 1;
	}

	if (function->common.fn_flags & ZEND_ACC_HAS_RETURN_TYPE) {
		it = function->op_array.arg_info - 1;

#if PHP_VERSION_ID >= 70200
		if (ZEND_TYPE_IS_SET(it->type) && (ZEND_TYPE_CODE(it->type) == IS_OBJECT || ZEND_TYPE_IS_CLASS(it->type))) {
#else
		if (it->type_hint == IS_OBJECT || it->class_name) {
#endif
			zend_throw_error(NULL,
				"cannot return an object directly from the sandbox");
			return 0;
		}

#if PHP_VERSION_ID >= 70200
		if (ZEND_TYPE_IS_SET(it->type) && ZEND_TYPE_CODE(it->type) == IS_ARRAY) {
#else
		if (it->type_hint == IS_ARRAY) {
#endif
			zend_throw_error(NULL,
				"cannot return an array directly from the sandbox");
			return 0;
		}

		if (it->pass_by_reference) {
			zend_throw_error(NULL,
				"cannot return by reference directly from the sandbox");
			return 0;
		}
	}

	it = function->op_array.arg_info;
	end = it + function->op_array.num_args;

	if (function->common.fn_flags & ZEND_ACC_VARIADIC) {
		end++;
	}

	while (it < end) {
#if PHP_VERSION_ID >= 70200
		if (ZEND_TYPE_IS_SET(it->type) && (ZEND_TYPE_CODE(it->type) == IS_OBJECT || ZEND_TYPE_IS_CLASS(it->type))) {
#else
		if (it->type_hint == IS_OBJECT || it->class_name) {
#endif
			zend_throw_error(NULL,
				"cannot pass an object directly to the sandbox at argument %d", argc);
			return 0;
		}

#if PHP_VERSION_ID >= 70200
		if (ZEND_TYPE_IS_SET(it->type) && ZEND_TYPE_CODE(it->type) == IS_ARRAY) {
#else
		if (it->type_hint == IS_ARRAY) {
#endif
			zend_throw_error(NULL,
				"cannot pass an array directly to the sandbox at argument %d", argc);
			return 0;
		}

		if (it->pass_by_reference) {
			zend_throw_error(NULL,
				"cannot pass by reference directly to the sandbox at argument %d", argc);
			return 0;
		}
		it++;
		argc++;
	}

	return 1;
} /* }}} */

static zend_always_inline zend_bool php_sandbox_copy_argv_check(zval *args) { /* {{{ */
	zval *arg;
	int   argc = 1;

	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(args), arg) {
		if (Z_TYPE_P(arg) == IS_OBJECT) {
			zend_throw_error(NULL, 
				"cannot pass an object directly to the sandbox at argument %d", argc);
			return 0;
		}

		if (Z_TYPE_P(arg) == IS_ARRAY) {
			zend_throw_error(NULL, 
				"cannot pass an array directly to the sandbox at argument %d", argc);
			return 0;
		}
		argc++;	
	} ZEND_HASH_FOREACH_END();

	return 1;
} /* }}} */

zend_bool php_sandbox_copy_check(php_sandbox_t *sandbox, zend_execute_data *execute_data, zend_function * function, int argc, zval *argv) { /* {{{ */
	zend_op *it = function->op_array.opcodes,
		*end = it + function->op_array.last;

	if (!php_sandbox_copy_arginfo_check(function)) {
		return 0;
	}

	if (argc && !php_sandbox_copy_argv_check(argv)) {
		return 0;
	}

	while (it < end) {
		switch (it->opcode) {
			case ZEND_YIELD:
			case ZEND_YIELD_FROM:
				zend_throw_error(NULL,
					"cannot yield directly from the sandbox on line %d",
					it->lineno);
				return 0;
				
			case ZEND_DECLARE_ANON_CLASS:
				zend_throw_error(NULL,
					"cannot declare anonymous class directly in the sandbox on line %d",
					it->lineno);
				return 0;

			case ZEND_DECLARE_LAMBDA_FUNCTION:
				zend_throw_error(NULL,
					"cannot declare anonymous function directly in the sandbox on line %d",
					it->lineno);
				return 0;

			case ZEND_DECLARE_FUNCTION:
				zend_throw_error(NULL,
					"cannot declare function directly in the sandbox on line %d",
					it->lineno);
				return 0;

			case ZEND_DECLARE_CLASS:
			case ZEND_DECLARE_INHERITED_CLASS:
			case ZEND_DECLARE_INHERITED_CLASS_DELAYED:
				zend_throw_error(NULL,
					"cannot declare class directly in the sandbox on line %d", 
					it->lineno);
				return 0;

			case ZEND_BIND_STATIC:	
				if (php_sandbox_copying_lexical(execute_data, function, it)) {
					zend_throw_error(NULL,
						"cannot bind lexical vars in the sandbox");
					return 0;
				}
			break;
		}
		it++;
	}

	sandbox->entry.point = function;

	if (argc) {
		ZVAL_COPY_VALUE(&sandbox->entry.argv, argv);
	} else  ZVAL_UNDEF(&sandbox->entry.argv);

	return 1;
} /* }}} */

zend_function* php_sandbox_copy(zend_function *function) { /* {{{ */
	zend_function  *copy;	
	zend_op_array  *op_array;
	zend_string   **variables;
	zval           *literals;
	zend_arg_info  *arg_info;

	copy = (zend_function*) ecalloc(1, sizeof(zend_op_array));

	memcpy(copy, function, sizeof(zend_op_array));
	
	op_array = &copy->op_array;
	variables = op_array->vars;
	literals = op_array->literals;
	arg_info = op_array->arg_info;

	op_array->function_name = zend_string_copy(php_sandbox_main);
	op_array->refcount = (uint32_t*) emalloc(sizeof(uint32_t));
	(*op_array->refcount) = 1;

	op_array->fn_flags &= ~ ZEND_ACC_CLOSURE;
	op_array->fn_flags &= ~ ZEND_ACC_DONE_PASS_TWO;
	op_array->fn_flags |= ZEND_ACC_PUBLIC;
	op_array->scope = NULL;
	op_array->prototype = NULL;
	op_array->doc_comment = NULL;
#if PHP_VERSION_ID >= 70400
	ZEND_MAP_PTR_NEW(op_array->run_time_cache);
#else
	op_array->run_time_cache = NULL;
#endif

	if (op_array->literals) {
		op_array->literals = php_sandbox_copy_literals (literals, op_array->last_literal);
	}

	op_array->opcodes = php_sandbox_copy_opcodes(op_array, literals);

	if (op_array->arg_info) {
		op_array->arg_info = php_sandbox_copy_arginfo(op_array, arg_info, op_array->num_args);
	}

	if (op_array->live_range) {
		op_array->live_range = php_sandbox_copy_live(op_array->live_range, op_array->last_live_range);
	}

	if (op_array->try_catch_array) {
		op_array->try_catch_array = php_sandbox_copy_try(op_array->try_catch_array, op_array->last_try_catch);
	}

	if (op_array->vars) {
		op_array->vars = php_sandbox_copy_variables(variables, op_array->last_var);
	}

	if (op_array->static_variables) {
#if PHP_VERSION_ID >= 70400
		ZEND_MAP_PTR_NEW(op_array->static_variables_ptr);
		
		op_array->fn_flags |= ZEND_ACC_IMMUTABLE;
#else
		op_array->static_variables = php_sandbox_copy_statics(op_array->static_variables);
#endif

	}

	return copy;
} /* }}} */
#endif


