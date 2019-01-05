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
#ifndef HAVE_SANDBOX_MONITOR
#define HAVE_SANDBOX_MONITOR

#include "php.h"
#include "monitor.h"

php_sandbox_monitor_t* php_sandbox_monitor_create(void) {
	pthread_mutexattr_t at;
	php_sandbox_monitor_t *monitor = 
		(php_sandbox_monitor_t*) 
			calloc(1, sizeof(php_sandbox_monitor_t));

	pthread_mutexattr_init(&at);
#if defined(PTHREAD_MUTEX_RECURSIVE) || defined(__FreeBSD__)
	pthread_mutexattr_settype(&at, PTHREAD_MUTEX_RECURSIVE);
#else
	pthread_mutexattr_settype(&at, PTHREAD_MUTEX_RECURSIVE_NP);
#endif
	pthread_mutex_init(&monitor->mutex, &at);
	pthread_mutexattr_destroy(&at);

	pthread_cond_init(&monitor->condition, NULL);

	return monitor;
}

int php_sandbox_monitor_lock(php_sandbox_monitor_t *monitor) {
	return pthread_mutex_lock(&monitor->mutex);
}

zend_bool php_sandbox_monitor_check(php_sandbox_monitor_t *monitor, uint32_t state) {
	return (monitor->state & state) == state;
}

int php_sandbox_monitor_unlock(php_sandbox_monitor_t *monitor) {
	pthread_mutex_unlock(&monitor->mutex);
}

uint32_t php_sandbox_monitor_wait(php_sandbox_monitor_t *monitor, uint32_t state, zend_bool lock) {
	uint32_t changed = FAILURE;

	if (lock) {
		php_sandbox_monitor_lock(monitor);
	}

	while (!(monitor->state & state)) {
		if (pthread_cond_wait(
				&monitor->condition, &monitor->mutex) != SUCCESS) {
			php_sandbox_monitor_unlock(monitor);
			return changed;
		}
	}

	changed = monitor->state;

	monitor->state &= ~state;

	if (lock) {
		php_sandbox_monitor_unlock(monitor);
	}

	return changed;
}

void php_sandbox_monitor_set(php_sandbox_monitor_t *monitor, uint32_t state, zend_bool lock) {
	if (lock) {
		php_sandbox_monitor_lock(monitor);
	}

	monitor->state |= state;

	pthread_cond_signal(&monitor->condition);

	if (lock) {
		php_sandbox_monitor_unlock(monitor);
	}
}

void php_sandbox_monitor_unset(php_sandbox_monitor_t *monitor, uint32_t state, zend_bool lock) {
	if (lock) {
		php_sandbox_monitor_lock(monitor);
	}

	monitor->state &= ~state;

	pthread_cond_signal(&monitor->condition);

	if (lock) {
		php_sandbox_monitor_unlock(monitor);
	}
}

void php_sandbox_monitor_destroy(php_sandbox_monitor_t *monitor) {
	pthread_mutex_destroy(&monitor->mutex);
	pthread_cond_destroy(&monitor->condition);

	free(monitor);
}
#endif
