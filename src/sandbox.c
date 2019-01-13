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
#include "zend_exceptions.h"

#define php_sandbox_exception(m, ...) zend_throw_exception_ex(php_sandbox_exception_ce, 0, m, ##__VA_ARGS__)

zend_class_entry *php_sandbox_ce;
zend_class_entry *php_sandbox_exception_ce;
zend_object_handlers php_sandbox_handlers;
zend_string *php_sandbox_main;

void* php_sandbox_routine(void *arg);

typedef int (*php_sapi_deactivate_t)(void);

php_sapi_deactivate_t php_sapi_deactivate_function;

ZEND_BEGIN_MODULE_GLOBALS(sandbox)
	zend_bool sandbox;	
ZEND_END_MODULE_GLOBALS(sandbox)

ZEND_DECLARE_MODULE_GLOBALS(sandbox);

#define ZG(v) ZEND_TSRMG(sandbox_globals_id, zend_sandbox_globals *, v)

PHP_METHOD(Sandbox, __construct)
{
	php_sandbox_t *sandbox = php_sandbox_from(getThis());
	zval          *configuration = NULL;

	if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS(), "|a", &configuration) != SUCCESS) {
		php_sandbox_monitor_set(sandbox->monitor, PHP_SANDBOX_ERROR);

		php_sandbox_exception("optional configuration array expected");
		return;
	}

	if (ZG(sandbox)) {
		php_sandbox_monitor_set(sandbox->monitor, PHP_SANDBOX_ERROR);

		php_sandbox_exception("sandboxes cannot create sandboxes");
		return;
	}

	if (configuration) {
		ZVAL_COPY_VALUE(&sandbox->configuration, configuration);
	}

	if (pthread_create(&sandbox->thread, NULL, php_sandbox_routine, sandbox) != SUCCESS) {
		php_sandbox_exception("cannot create sandbox thread");
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
		php_sandbox_exception("Closure, or Closure and args expected");
		return;
	}

	if (php_sandbox_monitor_lock(sandbox->monitor) != SUCCESS) {
		php_sandbox_exception("cannot lock sandbox");
		return;
	}

	if (php_sandbox_monitor_check(sandbox->monitor, PHP_SANDBOX_CLOSE|PHP_SANDBOX_DONE|PHP_SANDBOX_ERROR)) {
		php_sandbox_monitor_unlock(sandbox->monitor);
		php_sandbox_exception("sandbox unusable");
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

	php_sandbox_monitor_wait(sandbox->monitor, PHP_SANDBOX_WAKE);

	if (php_sandbox_monitor_check(sandbox->monitor, PHP_SANDBOX_ERROR)) {
		php_sandbox_exception(
			"sandbox bailed out");
		php_sandbox_monitor_unset(sandbox->monitor, PHP_SANDBOX_ERROR);
		return;
	}

	if (!Z_ISUNDEF(sandbox->entry.retval)) {
		php_sandbox_copy_zval(return_value, &sandbox->entry.retval, 0);

		if (Z_REFCOUNTED(sandbox->entry.retval)) {
			php_sandbox_zval_dtor(&sandbox->entry.retval);
		}
	}

	php_sandbox_zval_dtor(&sandbox->entry.argv);
}

PHP_METHOD(Sandbox, close)
{
	php_sandbox_t *sandbox = 
		php_sandbox_from(getThis());

	if (php_sandbox_monitor_check(sandbox->monitor, PHP_SANDBOX_CLOSED|PHP_SANDBOX_ERROR)) {
		php_sandbox_exception("sandbox unusable");
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

static void php_sandbox_globals_memset(zend_sandbox_globals *zg) {
	memset(zg, 0, sizeof(zend_sandbox_globals));
}

void php_sandbox_startup(void) {
	zend_class_entry ce;
	
	ZEND_INIT_MODULE_GLOBALS(sandbox, php_sandbox_globals_memset, NULL);

	memcpy(&php_sandbox_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

	php_sandbox_handlers.offset = XtOffsetOf(php_sandbox_t, std);
	php_sandbox_handlers.free_obj = php_sandbox_destroy;

	INIT_NS_CLASS_ENTRY(ce, "sandbox", "Runtime", php_sandbox_methods);

	php_sandbox_ce = zend_register_internal_class(&ce);
	php_sandbox_ce->create_object = php_sandbox_create;
	php_sandbox_ce->ce_flags |= ZEND_ACC_FINAL;

	INIT_NS_CLASS_ENTRY(ce, "sandbox", "Exception", NULL);

	php_sandbox_exception_ce = zend_register_internal_class_ex(&ce, zend_ce_error_exception);
	
	php_sapi_deactivate_function = sapi_module.deactivate;

	sapi_module.deactivate = NULL;

	php_sandbox_main = zend_string_init(ZEND_STRL("\\sandbox\\Runtime::enter"), 1);
}

void php_sandbox_shutdown(void) {
	sapi_module.deactivate = php_sapi_deactivate_function;

	zend_string_release(php_sandbox_main);
}

static void php_sandbox_execute(php_sandbox_monitor_t *monitor, zend_function *function, zval *argv, zval *retval) {
	zval rv;
	zend_fcall_info fci = empty_fcall_info;
	zend_fcall_info_cache fcc = empty_fcall_info_cache;
	int rc = FAILURE;

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
		rc = zend_call_function(&fci, &fcc);
	} zend_catch {
		php_sandbox_monitor_set(monitor,
			PHP_SANDBOX_ERROR|PHP_SANDBOX_WAKE);
	} zend_end_try();

	if (rc == SUCCESS && !Z_ISUNDEF(rv)) {
		php_sandbox_copy_zval(retval, &rv, 1);

		if (Z_REFCOUNTED(rv)) {
			zval_ptr_dtor(&rv);
		}
	}

	if (!Z_ISUNDEF_P(argv)) {
		zend_fcall_info_args_clear(&fci, 1);
	}

	destroy_op_array((zend_op_array*) fcc.function_handler);
	efree(fcc.function_handler);
}

static zend_always_inline void php_sandbox_configure_callback(int (*zend_callback) (char *, size_t), zval *value) {
	if (Z_TYPE_P(value) == IS_ARRAY) {
		zval *val;
		ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(value), val) {
			if (Z_TYPE_P(val) == IS_STRING) {
				zend_callback(Z_STRVAL_P(val), Z_STRLEN_P(val));
			}
		} ZEND_HASH_FOREACH_END();
	} else if (Z_TYPE_P(value) == IS_STRING) {
		char *start  = Z_STRVAL_P(value),
		     *end    = Z_STRVAL_P(value) + Z_STRLEN_P(value),
		     *next   = (char *) php_memnstr(Z_STRVAL_P(value), ZEND_STRL(","), end);

		if (next == NULL) {
			zend_callback(Z_STRVAL_P(value), Z_STRLEN_P(value));
			return;
		}

		do {
			zend_callback(start, next - start);
			start = next + 1;
			next  = (char *) php_memnstr(start, ZEND_STRL(","), end);
		} while(next);

		if (start <= end) {
			zend_callback(start, end - start);
		}
	}
}

static zend_always_inline void php_sandbox_configure(zval *configuration) {
	zend_string *name;
	zval        *value;

	ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(configuration), name, value) {
		zend_string *chars;
		zend_string *local = zend_string_dup(name, 1);

		if (zend_string_equals_literal_ci(local, "disable_functions")) {
			php_sandbox_configure_callback(zend_disable_function, value);
		} else if (zend_string_equals_literal_ci(local, "disable_classes")) {
			php_sandbox_configure_callback(zend_disable_class, value);
		} else {
			switch (Z_TYPE_P(value)) {
				case IS_STRING:
				case IS_TRUE:
				case IS_FALSE:
				case IS_LONG:
				case IS_DOUBLE:
					chars = zval_get_string(value);
				break;

				default:
					continue;
			}

			zend_alter_ini_entry_chars(local, 
				ZSTR_VAL(chars), ZSTR_LEN(chars), 
				ZEND_INI_SYSTEM, ZEND_INI_STAGE_ACTIVATE);

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

	ZG(sandbox)          = 1;
	PG(expose_php)       = 0;
	PG(auto_globals_jit) = 0;

	if (!Z_ISUNDEF(sandbox->configuration)) {
		php_sandbox_configure(&sandbox->configuration);
	}

	php_sandbox_monitor_set(sandbox->monitor, PHP_SANDBOX_READY);

	php_request_startup();

	SG(sapi_started)            = 0;
	SG(headers_sent)            = 1;
	SG(request_info).no_headers = 1;
	PG(during_request_startup)  = 0;

	ZVAL_UNDEF(&sandbox->entry.retval);

	while ((state = php_sandbox_monitor_wait(sandbox->monitor, PHP_SANDBOX_EXEC|PHP_SANDBOX_CLOSE))) {
		if (state & PHP_SANDBOX_CLOSE) {
			php_sandbox_monitor_set(sandbox->monitor, PHP_SANDBOX_WAKE);
			break;
		}

		zend_first_try {
			php_sandbox_execute(
				sandbox->monitor,
				sandbox->entry.point, 
				&sandbox->entry.argv, 
				&sandbox->entry.retval);
		} zend_end_try();

		php_sandbox_monitor_set(sandbox->monitor, PHP_SANDBOX_WAKE);
	}

	php_sandbox_monitor_set(sandbox->monitor, PHP_SANDBOX_DONE);

	php_request_shutdown(NULL);

	ts_free_thread();

	pthread_exit(NULL);

	return NULL;
}
#endif
