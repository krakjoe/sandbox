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
#ifndef HAVE_SANDBOX_SANDBOX
#define HAVE_SANDBOX_SANDBOX

#include "sandbox.h"
#include "copy.h"

#include "php_main.h"
#include "SAPI.h"
#include "TSRM.h"

#include "zend_closures.h"

zend_class_entry *php_sandbox_ce;
zend_object_handlers php_sandbox_handlers;

void* php_sandbox_routine(void *arg);

PHP_METHOD(Sandbox, __construct)
{
	php_sandbox_t *sandbox = php_sandbox_from(getThis());
	zval          *configuration = NULL;

	if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS(), "|a", &configuration) != SUCCESS) {
		php_sandbox_monitor_set(sandbox->monitor, PHP_SANDBOX_ERROR);

		zend_throw_error(NULL, "optional configuration array expected");
		return;
	}

	if (configuration) {
		ZVAL_COPY_VALUE(&sandbox->configuration, configuration);
	}

	if (pthread_create(&sandbox->thread, NULL, php_sandbox_routine, sandbox) != SUCCESS) {
		zend_throw_error(NULL, "cannot create sandbox thread");
		return;
	}

	php_sandbox_monitor_wait(sandbox->monitor, PHP_SANDBOX_READY);
}

PHP_METHOD(Sandbox, enter)
{
	php_sandbox_t *sandbox = php_sandbox_from(getThis());
	zval *closure = NULL;
	zval *argv = NULL;

	if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS(), "O|a", &closure, zend_ce_closure, &argv) != SUCCESS) {
		zend_throw_error(NULL, "Closure, or Closure and args expected");
		return;
	}

	if (php_sandbox_monitor_lock(sandbox->monitor) != SUCCESS) {
		zend_throw_error(NULL, "cannot lock sandbox");
		return;
	}

	if (php_sandbox_monitor_check(sandbox->monitor, PHP_SANDBOX_CLOSE|PHP_SANDBOX_DONE|PHP_SANDBOX_ERROR)) {
		php_sandbox_monitor_unlock(sandbox->monitor);
		zend_throw_error(NULL, "sandbox unusable");
		return;
	}

	if (!php_sandbox_copy_check(sandbox, 
		EG(current_execute_data)->prev_execute_data,
		(zend_function*) (((char*)Z_OBJ_P(closure)) + sizeof(zend_object)), 
		ZEND_NUM_ARGS() - 1, argv)) {
		php_sandbox_monitor_unlock(sandbox->monitor);
		return;
	}

	php_sandbox_monitor_set(sandbox->monitor, PHP_SANDBOX_EXEC);
	php_sandbox_monitor_unlock(sandbox->monitor);

	php_sandbox_monitor_wait(sandbox->monitor, PHP_SANDBOX_WAIT);

	if (!Z_ISUNDEF(sandbox->entry.retval)) {
		if (Z_TYPE(sandbox->entry.retval) == IS_STRING) {
			zend_string *rv = zend_string_init(
						Z_STRVAL(sandbox->entry.retval), 
						Z_STRLEN(sandbox->entry.retval), 0);

			zend_string_release(Z_STR(sandbox->entry.retval));

			RETURN_STR(rv);
		} else 	ZVAL_COPY(return_value, &sandbox->entry.retval);
	}
}

PHP_METHOD(Sandbox, close)
{
	php_sandbox_t *sandbox = 
		php_sandbox_from(getThis());

	if (php_sandbox_monitor_check(sandbox->monitor, PHP_SANDBOX_CLOSED|PHP_SANDBOX_ERROR)) {
		return;
	}

	php_sandbox_monitor_set(
		sandbox->monitor, PHP_SANDBOX_CLOSE);
	php_sandbox_monitor_wait(
		sandbox->monitor, PHP_SANDBOX_DONE);
	php_sandbox_monitor_set(
		sandbox->monitor, PHP_SANDBOX_CLOSED);

	pthread_join(sandbox->thread, NULL);
}

zend_function_entry php_sandbox_methods[] = {
	PHP_ME(Sandbox, __construct, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Sandbox, enter, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Sandbox, close, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

zend_object* php_sandbox_create(zend_class_entry *type) {
	php_sandbox_t *sandbox = ecalloc(1, 
			sizeof(php_sandbox_t) + zend_object_properties_size(type));

	zend_object_std_init(&sandbox->std, type);

	sandbox->std.handlers = &php_sandbox_handlers;

	sandbox->monitor = php_sandbox_monitor_create();
	sandbox->creator = ts_resource(0);

	ZVAL_UNDEF(&sandbox->entry.retval);
	ZVAL_UNDEF(&sandbox->configuration);

	return &sandbox->std;
}

void php_sandbox_destroy(zend_object *o) {
	php_sandbox_t *sandbox = 
		php_sandbox_fetch(o);

	if (!php_sandbox_monitor_check(sandbox->monitor, PHP_SANDBOX_ERROR|PHP_SANDBOX_CLOSED)) {
		php_sandbox_monitor_set(
			sandbox->monitor, 
			PHP_SANDBOX_CLOSE);
		
		php_sandbox_monitor_wait(
			sandbox->monitor,
			PHP_SANDBOX_DONE);

		pthread_join(sandbox->thread, NULL);
	}

	php_sandbox_monitor_destroy(sandbox->monitor);

	zend_object_std_dtor(o);
}

void php_sandbox_startup(void) {
	zend_class_entry ce;

	memcpy(&php_sandbox_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

	php_sandbox_handlers.offset = XtOffsetOf(php_sandbox_t, std);
	php_sandbox_handlers.free_obj = php_sandbox_destroy;

	INIT_NS_CLASS_ENTRY(ce, "sandbox", "Runtime", php_sandbox_methods);

	php_sandbox_ce = zend_register_internal_class(&ce);
	php_sandbox_ce->create_object = php_sandbox_create;
}

void php_sandbox_shutdown(void) {

}

static zend_always_inline void php_sandbox_execute(zend_function *function, zval *argv, zval *retval) {
	zval rv;
	zend_fcall_info fci = empty_fcall_info;
	zend_fcall_info_cache fcc = empty_fcall_info_cache;

	fci.size = sizeof(zend_fcall_info);
	fci.retval = &rv;
#if PHP_VERSION_ID < 70300
	fcc.initialized = 1;
#endif
	fcc.function_handler = php_sandbox_copy(function);

	if (!Z_ISUNDEF_P(argv)) {
		zend_fcall_info_args(&fci, argv);
	}

	ZVAL_UNDEF(&rv);

	zend_try {
		zend_call_function(&fci, &fcc);
	} zend_end_try();

	if (!Z_ISUNDEF(rv)) {
		switch (Z_TYPE(rv)) {
			case IS_TRUE:
			case IS_FALSE:
			case IS_NULL:
			case IS_LONG:
			case IS_DOUBLE:
				ZVAL_COPY(retval, &rv);
			break;

			case IS_STRING: {
				zend_string *out = zend_string_init(
						Z_STRVAL(rv), Z_STRLEN(rv), 1);
				ZVAL_STR(retval, out);
				zval_ptr_dtor(&rv);
			} break;

			default:
				if (Z_REFCOUNTED(rv)) {
					zval_ptr_dtor(&rv);
				}
		}
	}

	if (!Z_ISUNDEF_P(argv)) {
		zend_fcall_info_args_clear(&fci, 1);
	}

	destroy_op_array((zend_op_array*) fcc.function_handler);
	efree(fcc.function_handler);
}

static zend_always_inline void php_sandbox_configure(zval *configuration) {
	zend_string *name;
	zval        *value;

	ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(configuration), name, value) {
		zend_string *chars;
		zend_string *local = zend_string_dup(name, 1);

		if (Z_TYPE_P(value) == IS_STRING) {
			chars = Z_STR_P(value);
		} else {
			chars = zval_get_string(value);
		}

		zend_alter_ini_entry_chars(local, 
			ZSTR_VAL(chars), ZSTR_LEN(chars), 
			ZEND_INI_SYSTEM, ZEND_INI_STAGE_ACTIVATE);

		if (Z_TYPE_P(value) != IS_STRING) {
			zend_string_release(chars);
		}

		zend_string_release(local);
	} ZEND_HASH_FOREACH_END();
}

void* php_sandbox_routine(void *arg) {	
	php_sandbox_t *sandbox = (php_sandbox_t*) arg;
	
	uint32_t state;

	sandbox->context = ts_resource(0);

	TSRMLS_CACHE_UPDATE();

	SG(server_context) = (((sapi_globals_struct*) 
		(*((void ***) sandbox->creator))[
			TSRM_UNSHUFFLE_RSRC_ID(sapi_globals_id)
	])->server_context);

	PG(expose_php) = 0;
	PG(auto_globals_jit) = 0;

	if (!Z_ISUNDEF(sandbox->configuration)) {
		php_sandbox_configure(&sandbox->configuration);
	}

	php_sandbox_monitor_set(sandbox->monitor, PHP_SANDBOX_READY);

	php_request_startup();

	SG(sapi_started)            = 0;
	SG(headers_sent)            = 1;
	SG(request_info).no_headers = 1;

	ZVAL_UNDEF(&sandbox->entry.retval);

	while ((state = php_sandbox_monitor_wait(sandbox->monitor, PHP_SANDBOX_EXEC|PHP_SANDBOX_CLOSE))) {
		if (state & PHP_SANDBOX_CLOSE) {
			php_sandbox_monitor_set(sandbox->monitor, PHP_SANDBOX_WAIT);
			break;
		}

		zend_first_try {
			php_sandbox_execute(
				sandbox->entry.point, 
				&sandbox->entry.argv, 
				&sandbox->entry.retval);
		} zend_end_try();

		php_sandbox_monitor_set(sandbox->monitor, PHP_SANDBOX_WAIT);
	}

	php_sandbox_monitor_set(sandbox->monitor, PHP_SANDBOX_DONE);

	php_request_shutdown(NULL);

	ts_free_thread();

	pthread_exit(NULL);
}
#endif
