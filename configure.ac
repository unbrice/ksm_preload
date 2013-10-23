# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([ksm_preload], [0.10], [https://github.com/unbrice/ksm_preload])
AM_INIT_AUTOMAKE([0.10 foreign])
#AC_CONFIG_SRCDIR([src/ksm_preload.c])
LT_PREREQ([2.2])
LT_INIT([dlopen])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
# FIXME: Replace `main' with a function in `-ldl':
AC_CHECK_LIB([lpthread], [pthread_create])

# Checks for header files.
AC_CHECK_HEADERS([limits.h stdint.h stdlib.h unistd.h dlfcn.h sys/mman.h sys/syscall.h assert.h error.h errno.h pthread.h stdbool.h stdarg.h stdio.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
# AC_CHECK_FUNCS([strtol])

AC_CONFIG_FILES([Makefile
		 src/Makefile])
AC_OUTPUT
