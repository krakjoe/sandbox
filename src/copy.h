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
#ifndef HAVE_SANDBOX_COPY_H
#define HAVE_SANDBOX_COPY_H

zend_function* php_sandbox_copy(zend_function *function);
void php_sandbox_copy_zval(zval *dest, zval *source, zend_bool persistent);
zend_bool php_sandbox_copy_check(php_sandbox_t *sandbox, zend_execute_data *execute_data, zend_function * function, int argc, zval *argv);

static zend_always_inline void php_sandbox_zval_dtor(zval *zv) {
	if (Z_TYPE_P(zv) == IS_ARRAY) {
		zend_hash_destroy(Z_ARRVAL_P(zv));
#if PHP_VERSION_ID < 70300
		pefree(Z_ARRVAL_P(zv), Z_ARRVAL_P(zv)->u.flags & HASH_FLAG_PERSISTENT);
#else
		pefree(Z_ARRVAL_P(zv), GC_FLAGS(Z_ARRVAL_P(zv)) & IS_ARRAY_PERSISTENT);
#endif
	} else if (Z_TYPE_P(zv) == IS_STRING) {
		zend_string_release(Z_STR_P(zv));
	} else {
		if (Z_REFCOUNTED_P(zv)) {
			zval_ptr_dtor(zv);
		}
	}
}
#endif
