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

zend_class_entry *php_sandbox_ce;
zend_object_handlers php_sandbox_handlers;

void* php_sandbox_routine(void *arg);

PHP_METHOD(Sandbox, __construct)
{
	php_sandbox_t *sandbox = php_sandbox_from(getThis());
	zval          *configuration = NULL;

	if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS(), "|a", &configuration) != SUCCESS) {
		zend_throw_error(NULL, "only optional configuration expected");
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
	php_sandbox_entry_point_t entry;

	if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS(), "f", &entry.fci, &entry.fcc) != SUCCESS) {
		zend_throw_error(NULL, "no entry point provided");
		return;
	}

	if (php_sandbox_monitor_lock(sandbox->monitor) != SUCCESS) {
		zend_throw_error(NULL, "cannot lock sandbox");
		return;
	}

	if (php_sandbox_monitor_check(sandbox->monitor, PHP_SANDBOX_CLOSE|PHP_SANDBOX_DONE)) {
		zend_throw_error(NULL, "sandbox closed");

		php_sandbox_monitor_unlock(sandbox->monitor);
		return;
	}

	memcpy(&sandbox->entry.fci, &entry.fci, sizeof(zend_fcall_info));
	memcpy(&sandbox->entry.fcc, &entry.fcc, sizeof(zend_fcall_info_cache));

	php_sandbox_monitor_set(sandbox->monitor, PHP_SANDBOX_EXEC);
	php_sandbox_monitor_unlock(sandbox->monitor);

	php_sandbox_monitor_wait(sandbox->monitor, PHP_SANDBOX_WAIT);

	if (!Z_ISUNDEF(sandbox->entry.retval)) {
		ZVAL_COPY(return_value, &sandbox->entry.retval);
	}
}

PHP_METHOD(Sandbox, close)
{
	php_sandbox_t *sandbox = 
		php_sandbox_from(getThis());

	if (php_sandbox_monitor_check(sandbox->monitor, PHP_SANDBOX_CLOSED)) {
		return;
	}

	php_sandbox_monitor_set(
		sandbox->monitor, PHP_SANDBOX_CLOSE);
	php_sandbox_monitor_wait(
		sandbox->monitor, PHP_SANDBOX_DONE);
	php_sandbox_monitor_set(
		sandbox->monitor, PHP_SANDBOX_CLOSED);
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

	if (!php_sandbox_monitor_check(sandbox->monitor, PHP_SANDBOX_CLOSED)) {
		php_sandbox_monitor_set(
			sandbox->monitor, 
			PHP_SANDBOX_CLOSE);
		php_sandbox_monitor_wait(
			sandbox->monitor,
			PHP_SANDBOX_DONE);
	}

	php_sandbox_monitor_destroy(sandbox->monitor);

	pthread_join(sandbox->thread, NULL);

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
		zend_string *name;
		zval        *value;

		ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL(sandbox->configuration), name, value) {
			zend_string *chars;

			if (Z_TYPE_P(value) == IS_STRING) {
				chars = Z_STR_P(value);
			} else {
				chars = zval_get_string(value);
			}

			zend_alter_ini_entry_chars(name, 
				ZSTR_VAL(chars), ZSTR_LEN(chars), 
				ZEND_INI_SYSTEM, ZEND_INI_STAGE_ACTIVATE);

			if (Z_TYPE_P(value) != IS_STRING) {
				zend_string_release(chars);
			}
		} ZEND_HASH_FOREACH_END();
	}

	php_sandbox_monitor_set(sandbox->monitor, PHP_SANDBOX_READY);

	php_request_startup();

	SG(sapi_started)            = 0;
	SG(headers_sent)            = 1;
	SG(request_info).no_headers = 1;

	ZVAL_UNDEF(&sandbox->entry.retval);

	while ((state = php_sandbox_monitor_wait(sandbox->monitor, PHP_SANDBOX_EXEC|PHP_SANDBOX_CLOSE))) {
		if (state & PHP_SANDBOX_CLOSE) {
			if (!Z_ISUNDEF(sandbox->entry.retval)) {
				if (Z_REFCOUNTED(sandbox->entry.retval)) {
					zval_ptr_dtor(&sandbox->entry.retval);
				}
				ZVAL_UNDEF(&sandbox->entry.retval);
			}
			php_sandbox_monitor_set(sandbox->monitor, PHP_SANDBOX_WAIT);
			break;
		}

		zend_first_try {
			zval rv;
			php_sandbox_entry_point_t entry;

			memcpy(&entry, &sandbox->entry, sizeof(php_sandbox_entry_point_t));

			entry.fci.retval = &rv;
			entry.fcc.function_handler =
				(zend_function*) php_sandbox_copy(entry.fcc.function_handler);

			ZVAL_UNDEF(&rv);

			if (!Z_ISUNDEF(sandbox->entry.retval)) {
				if (Z_REFCOUNTED(sandbox->entry.retval)) {
					zval_ptr_dtor(&sandbox->entry.retval);
				}
				ZVAL_UNDEF(&sandbox->entry.retval);
			}

			zend_try {
				zend_call_function(&entry.fci, &entry.fcc);
			} zend_end_try();

			if (!Z_ISUNDEF(rv)) {
				switch (Z_TYPE(rv)) {
					case IS_TRUE:
					case IS_FALSE:
					case IS_NULL:
					case IS_LONG:
					case IS_DOUBLE:
						ZVAL_COPY(&sandbox->entry.retval, &rv);
					break;

					default:
						if (Z_REFCOUNTED(rv)) {
							zval_ptr_dtor(&rv);
						}
				}
			}

			destroy_op_array((zend_op_array*) entry.fcc.function_handler);
			efree(entry.fcc.function_handler);
		} zend_end_try();

		php_sandbox_monitor_set(sandbox->monitor, PHP_SANDBOX_WAIT);
	}

	php_sandbox_monitor_set(sandbox->monitor, PHP_SANDBOX_DONE);

	php_request_shutdown(NULL);

	pthread_exit(NULL);
}
#endif
