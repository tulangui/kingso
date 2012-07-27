#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.50])
AC_INIT(search, 0.0.1, qianshi.yf@taobao.com)
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_LIBTOOL
AC_LANG_CPLUSPLUS
# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
# AC_TYPE_INT32_T

# Checks for library functions.


CPPFLAGS="-D_FILE_OFFSET_BITS=64 -DSHMEM -I. $CPPFLAGS -I`pwd`/include -I`pwd`/src -I`pwd` -Wall"
LDFLAGS="-D_FILE_OFFSET_BITS=64 -DSHMEM -I. $LDFLAGS  -I`pwd`/include -I`pwd`/src -I`pwd`"


AC_CONFIG_FILES([Makefile include/Makefile src/Makefile test/Makefile])
AC_OUTPUT
