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
#ifndef HAVE_SANDBOX_SANDBOX_H
#define HAVE_SANDBOX_SANDBOX_H

#include "php.h"
#include "monitor.h"

extern zend_class_entry *php_sandbox_ce;
extern zend_class_entry *php_sandbox_exception_ce;

typedef struct _php_sandbox_entry_point_t {
	zend_function *point;
	zval argv;
	zval retval;
} php_sandbox_entry_point_t;

typedef struct _php_sandbox_t {
	pthread_t thread;
	void      ***creator;
	void      ***context;
	php_sandbox_monitor_t *monitor;
	php_sandbox_entry_point_t entry;
	zval configuration;
	zend_object std;
} php_sandbox_t;

void php_sandbox_shutdown(void);
void php_sandbox_startup(void);

static zend_always_inline php_sandbox_t* php_sandbox_fetch(zend_object *o) {
	return (php_sandbox_t*) (((char*) o) - XtOffsetOf(php_sandbox_t, std));
}

static zend_always_inline php_sandbox_t* php_sandbox_from(zval *z) {
	return php_sandbox_fetch(Z_OBJ_P(z));
}

zend_object* php_sandbox_create(zend_class_entry *);
void         php_sandbox_destroy(zend_object *);
#endif
