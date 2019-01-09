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
#ifndef HAVE_SANDBOX_MONITOR_H
#define HAVE_SANDBOX_MONITOR_H

#include <pthread.h>

typedef struct _php_sandbox_monitor_t {
	pthread_mutex_t mutex;
	pthread_cond_t  condition;
	volatile uint32_t        state;
} php_sandbox_monitor_t;

#define PHP_SANDBOX_READY  0x00000001 // 1
#define PHP_SANDBOX_EXEC   0x00000010 // 16
#define PHP_SANDBOX_WAIT   0x00000100 // 256
#define PHP_SANDBOX_CLOSE  0x00001000 // 4096
#define PHP_SANDBOX_DONE   0x00010000 // 65536
#define PHP_SANDBOX_CLOSED 0x00100000 // 
#define PHP_SANDBOX_ERROR  0x10000000

php_sandbox_monitor_t* php_sandbox_monitor_create(void);
int php_sandbox_monitor_lock(php_sandbox_monitor_t *m);
uint32_t php_sandbox_monitor_check(php_sandbox_monitor_t *m, uint32_t state);
int php_sandbox_monitor_unlock(php_sandbox_monitor_t *m);
uint32_t php_sandbox_monitor_wait(php_sandbox_monitor_t *m, uint32_t state);
void php_sandbox_monitor_set(php_sandbox_monitor_t *m, uint32_t state);
void php_sandbox_monitor_set_and_wait(php_sandbox_monitor_t *m, uint32_t set, uint32_t wait);
void php_sandbox_monitor_unset(php_sandbox_monitor_t *m, uint32_t state);
void php_sandbox_monitor_destroy(php_sandbox_monitor_t *);
#endif
