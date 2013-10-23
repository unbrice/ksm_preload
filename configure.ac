AC_PREREQ([2.68])
AC_INIT([ksm_preload], [0.10], [https://github.com/unbrice/ksm_preload])
AM_INIT_AUTOMAKE([foreign subdir-objects])
LT_INIT

AC_PROG_CC_C99
AS_IF([test "x$ac_cv_prog_cc_c99" = "xno"], [
  AC_MSG_ERROR([no C99 compiler found, $PACKAGE requires a C99 compiler.])
])

AC_SEARCH_LIBS([dlopen], [dl])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
