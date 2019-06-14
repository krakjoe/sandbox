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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "php.h"
#include "ext/standard/info.h"
#include "php_sandbox.h"

#include "src/sandbox.h"

/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(sandbox)
{
	php_sandbox_startup();

	return SUCCESS;
} /* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION */
PHP_MSHUTDOWN_FUNCTION(sandbox)
{
	php_sandbox_shutdown();

	return SUCCESS;
} /* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(sandbox)
{
#if defined(ZTS) && defined(COMPILE_DL_SANDBOX)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(sandbox)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "sandbox support", "enabled");
	php_info_print_table_row(2, "sandbox version", PHP_SANDBOX_VERSION);
	php_info_print_table_end();
}
/* }}} */

/* {{{ sandbox_module_entry
 */
zend_module_entry sandbox_module_entry = {
	STANDARD_MODULE_HEADER,
	"sandbox",
	NULL,
	PHP_MINIT(sandbox),
	PHP_MSHUTDOWN(sandbox),
	PHP_RINIT(sandbox),
	NULL,
	PHP_MINFO(sandbox),
	PHP_SANDBOX_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_SANDBOX
# ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
# endif
ZEND_GET_MODULE(sandbox)
#endif

