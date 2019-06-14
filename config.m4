dnl config.m4 for extension sandbox

PHP_ARG_ENABLE(sandbox, whether to enable sandbox support,
[  --enable-sandbox          Enable sandbox support], no)

PHP_ARG_ENABLE(sandbox-coverage,      whether to enable sandbox coverage support,
[  --enable-sandbox-coverage          Enable sandbox coverage support], no, no)


if test "$PHP_SANDBOX" != "no"; then

  AC_MSG_CHECKING([for ZTS])
  if test "$PHP_THREAD_SAFETY" != "no"; then
    AC_MSG_RESULT([ok])
  else
    AC_MSG_ERROR([parallel requires ZTS, please use PHP with ZTS enabled])
  fi

  AC_DEFINE(HAVE_SANDBOX, 1, [ Have sandbox support ])

  PHP_NEW_EXTENSION(sandbox, sandbox.c src/monitor.c src/sandbox.c src/copy.c, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)

  PHP_ADD_BUILD_DIR($ext_builddir/src, 1)
  PHP_ADD_INCLUDE($ext_builddir)

  AC_MSG_CHECKING([sandbox coverage])
  if test "$PHP_SANDBOX_COVERAGE" != "no"; then
    AC_MSG_RESULT([enabled])

    PHP_ADD_MAKEFILE_FRAGMENT
  else
    AC_MSG_RESULT([disabled])
  fi

fi
