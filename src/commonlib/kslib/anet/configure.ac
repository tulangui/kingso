#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
define([ANET_MAJOR_VERSION],[0])
define([ANET_MINOR_VERSION],[9])
define([ANET_MICRO_VERSION],[3])
AC_INIT([ANet], [ANET_MAJOR_VERSION.ANET_MINOR_VERSION.ANET_MICRO_VERSION],
        [asc-engine@list.alibaba-inc.com])
AC_CONFIG_SRCDIR([src/anet/anet.h])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_HEADERS([config/config.h])
AM_INIT_AUTOMAKE

define([ANET_LT_MAJOR],[1])
define([ANET_LT_REVISION],[0])
define([ANET_LT_AGE],[0])
LT_VERSION_INFO=ANET_LT_MAJOR:ANET_LT_REVISION:ANET_LT_AGE
AC_SUBST([LT_VERSION_INFO])


ANET_RELEASE=rc3

AC_SUBST([ANET_RELEASE])
AC_SUBST([USER])
AC_SUBST([HOSTNAME])

DOXYGEN_OUTPUT_DIR=docs/doxygen
AC_SUBST([DOXYGEN_OUTPUT_DIR])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL
AC_LANG_CPLUSPLUS 

AC_CHECK_LIB([pthread], [main], [], AC_MSG_FAILURE([pthread not found!]))


CXXFLAGS="-Wall $CXXFLAGS"
soname_spec="\${libname}\${shared_ext}"
# Checks for header files.

AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h string.h strings.h\
                  sys/socket.h sys/time.h unistd.h],
                 [],
                 [AC_MSG_FAILURE([headers checking failed!])])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_C_VOLATILE

#checking hashmap
AC_CHECK_HEADERS([ext/hash_map], [], AC_MSG_FAILURE([hash_map not found]))
AC_RUN_IFELSE([AC_LANG_PROGRAM([[#include <ext/hash_map>]],
                               [[#ifdef _BACKWARD_BACKWARD_WARNING_H
                                 return 1;
                                 #endif]])
              ], 
              [],
              [CXXFLAGS="-Wno-deprecated $CXXFLAGS"])

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_TYPE_SIGNAL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([dup2 gethostbyname gethostname gettimeofday memmove memset\
                munmap socket strcasecmp strdup strerror],
               [],
               [AC_MSG_FAILURE([funcs check failed!])])

#dnl do alog library checking
AC_ARG_WITH([alog],
  [AS_HELP_STRING([--with-alog],
    [do logging with alog @<:@default=yes@:>@])],
  [],
  [with_alog=yes])

AC_SUBST([ALOG_FLAG])

AC_MSG_CHECKING([whether do logging with alog])
AS_IF([test "x$with_alog" != "xno"],
      [ AC_MSG_RESULT([yes])
        AC_MSG_NOTICE(checking alog library...)
        AS_IF([test "x$with_alog" != "xyes"],
              [AC_MSG_NOTICE([checking alog in $with_alog...])
               CPPFLAGS="$CPPFLAGS -I$with_alog/include"
               LDFLAGS="$LDFLAGS -L$with_alog/lib"
              ], 
              [AC_MSG_NOTICE([checking alog in system default location])]
             )

        [LIBS="$LIBS -lalog"]
        AC_CHECK_HEADERS([alog/Logger.h alog/Configurator.h],,
                         [AC_MSG_FAILURE([checking alog headers Failed])]
                        )
        AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <alog/Logger.h>]],
                             [[return alog::Logger::getRootLogger() ? 0 : 1;]])
                       ],
                       [AC_MSG_RESULT([OK])],
                       [AC_MSG_FAILURE([getRootLogger() checking failure])]
                      )
        [ALOG_FLAG="undef HAVE_ALOG"]
      ],
      [ AC_MSG_RESULT([no])
        [ALOG_FLAG="undef HAVE_ALOG"]
      ]
     )  

AC_CONFIG_FILES([Makefile
                 src/anet/Makefile
                 test/Makefile
                 util/Makefile
                 config/anet/logconfig.h:src/anet/logconfig.h.in
                 anet.spec
                 Doxyfile])
AC_OUTPUT
