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

#ifndef PHP_SANDBOX_H
# define PHP_SANDBOX_H

extern zend_module_entry sandbox_module_entry;
# define phpext_sandbox_ptr &sandbox_module_entry

# define PHP_SANDBOX_VERSION "0.1.2"

# if defined(ZTS) && defined(COMPILE_DL_SANDBOX)
ZEND_TSRMLS_CACHE_EXTERN()
# endif

#endif	/* PHP_SANDBOX_H */

