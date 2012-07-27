#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(query_parser, 0.0.1, pianqian.gc@taobao.com)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([include/queryparser/QPResult.h])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL
AC_LANG_CPLUSPLUS

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdint.h stdlib.h string.h])
AC_PROG_LIBTOOL
AC_PROG_RANLIB

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset strtoul strtoull])

LIBS="-ldl"
CPPFLAGS="-fPIC"
#CXXFLAGS="-Wall"
#CPPFLAGS="-D_FILE_OFFSET_BITS=64 -DSHMEM -I. $CPPFLAGS -I`pwd`/include -I`pwd`/src -I`pwd`"
#LDFLAGS="-D_FILE_OFFSET_BITS=64 -DSHMEM -I. $LDFLAGS -I`pwd`/include -I`pwd`/src -I`pwd`"


usedebug="no"
AC_ARG_ENABLE(debug, [AS_HELP_STRING([--enable-debug],[Enable debug, default false])], 
[
 if test "$enableval" != "no"; then
	CPPFLAGS="$CPPFLAGS -DDEBUG"
	usedebug="yes"
 fi
 ]
)


trylib=""
AC_ARG_WITH(commdef,
            [  --with-commdef=PATH     Specify path to commdef installation. Default /usr/local ],
            [
             if test "x$withval" != "xno" ; then
                 trylib=$withval
             fi
             ]
            )

found=no
AC_CACHE_CHECK([for commdef directory], ac_cv_commdef_dir, [
                saved_CPPFLAGS="$CPPFLAGS"
                for sdir in $trylib "" $prefix /usr/local ; do
                    # Skip the directory if it isn't there.
                    if test ! -z "$sdir" -a ! -d "$sdir" ; then
                        continue;
                    fi
                    if test ! -z "$sdir" ; then
                        if test -d "$sdir/include" ; then
                            CPPFLAGS="-I$sdir/include $CPPFLAGS"
                        else
                            CPPFLAGS="-I$sdir $CPPFLAGS"
                        fi
                    fi
                    if test ! -z "$sdir/include/commdef/commdef.h"; then
                        commdef_linked=yes
                    else
                        commdef_linked=no
                    fi
                    if test $commdef_linked = yes; then
                        if test ! -z "$sdir" ; then
                            ac_cv_commdef_dir=$sdir
                        else
                            ac_cv_commdef_dir="(system)"
                        fi
                        found=yes
                        break
                    fi
                done
                CPPFLAGS="$saved_CPPFLAGS"
                if test $found = no ;
                then
                    AC_MSG_ERROR([commdef is required. If it is already installed, specify its path using --with-commdef=/dir/])
                    fi
                    ])
if test "$found" != "no"; then
    if test $ac_cv_commdef_dir != "(system)"; then
        if test -d "$ac_cv_commdef_dir/include" ; then
            CPPFLAGS="-I$ac_cv_commdef_dir/include $CPPFLAGS"
        else
            CPPFLAGS="-I$ac_cv_commdef_dir $CPPFLAGS"
        fi
    fi
fi


trylib=""
AC_ARG_WITH(analysis,
            [  --with-analysis=PATH     Specify path to analysis installation. Default /usr/local ],
            [
             if test "x$withval" != "xno" ; then
                 trylib=$withval
             fi
             ]
            )

found=no
AC_CACHE_CHECK([for analysis directory], ac_cv_analysis_dir, [
                saved_CPPFLAGS="$CPPFLAGS"
                for sdir in $trylib "" $prefix /usr/local ; do
                    # Skip the directory if it isn't there.
                    if test ! -z "$sdir" -a ! -d "$sdir" ; then
                        continue;
                    fi
                    if test ! -z "$sdir" ; then
                        if test -d "$sdir/include" ; then
                            CPPFLAGS="-I$sdir/include $CPPFLAGS"
                        else
                            CPPFLAGS="-I$sdir $CPPFLAGS"
                        fi
                    fi
                    analysis_linked=yes
                    if test $analysis_linked = yes; then
                        if test ! -z "$sdir" ; then
                            ac_cv_analysis_dir=$sdir
                        else
                            ac_cv_analysis_dir="(system)"
                        fi
                        found=yes
                        break
                    fi
                done
                CPPFLAGS="$saved_CPPFLAGS"
                if test $found = no ;
                then
                    AC_MSG_ERROR([analysis is required. If it is already installed, specify its path using --with-analysis=/dir/])
                    fi
                    ])
if test "$found" != "no"; then
    if test $ac_cv_analysis_dir != "(system)"; then
        if test -d "$ac_cv_analysis_dir/include" ; then
            CPPFLAGS="-I$ac_cv_analysis_dir/include $CPPFLAGS"
        else
            CPPFLAGS="-I$ac_cv_analysis_dir $CPPFLAGS"
        fi
    fi
fi

trylib=""
AC_ARG_WITH(kslib, 
            [  --with-kslib=PATH     Specify path to kslib installation. Default /usr/local ],
            [
             if test "x$withval" != "xno" ; then
                 trylib=$withval
             fi
             ]
            )

kslib_lib_path_name="lib"
AC_CACHE_CHECK([for kslib directory], ac_cv_kslib_dir, [
                saved_LIBS="$LIBS"
                saved_LDFLAGS="$LDFLAGS"
                saved_CXXFLAGS="$CXXFLAGS"
                found=no
                for sdir in $trylib "" $prefix /usr/local ; do
                    kslib_lib_path_name="lib"
                    LDFLAGS="$saved_LDFLAGS"
                    LIBS="$saved_LIBS -lkslib"
                    # Skip the directory if it isn't there.
                    if test ! -z "$sdir" -a ! -d "$sdir" ; then
                        continue;
                    fi
                    if test ! -z "$sdir" ; then
                        if test -d "$sdir/lib" ; then
                            LDFLAGS="-L$sdir/lib $LDFLAGS"
                            kslib_lib_path_name="lib"
                        else
                            LDFLAGS="-L$sdir $LDFLAGS"
                            kslib_lib_path_name=""
                        fi
                        if test -d "$sdir/include" ; then
                            CXXFLAGS="-I$sdir/include $CXXFLAGS"
                        else
                            CXXFLAGS="-I$sdir $CXXFLAGS"
                        fi
                    fi

                    #test exist
                    if test ! -f "$sdir/lib/libkslib.a" ; then
                        continue;
                    fi
                    kslib_linked=yes;
                    if test $kslib_linked = yes; then
                        if test ! -z "$sdir" ; then
                            ac_cv_kslib_dir=$sdir
                        else
				ac_cv_kslib_dir="(system)"
			fi
			found=yes
			break
		fi
	done
	LIBS="$saved_LIBS"
	LDFLAGS="$saved_LDFLAGS"
	CXXFLAGS="$saved_CXXFLAGS"
	if test $found = no ; 
	then
		AC_MSG_ERROR([kslib is required. If it is already installed, specify its path using --with-kslib=/dir/])
		fi
		])
LIBS="-lkslib $LIBS"
if test $ac_cv_kslib_dir != "(system)"; then
	if test -d "$ac_cv_kslib_dir/$kslib_lib_path_name" ; then
		LDFLAGS="-L$ac_cv_kslib_dir/$kslib_lib_path_name $LDFLAGS"
		kslibroot="$ac_cv_kslib_dir/$kslib_lib_path_name"
	else
		LDFLAGS="-L$ac_cv_kslib_dir $LDFLAGS"
		kslibroot="$ac_cv_kslib_dir"
	fi
	if test -d "$ac_cv_kslib_dir/include" ; then
		CXXFLAGS="-I$ac_cv_kslib_dir/include $CXXFLAGS"
	else
		CXXFLAGS="-I$ac_cv_kslib_dir $CXXFLAGS"
	fi
fi


#index_lib
trylib=""
AC_ARG_WITH(index_lib, 
						[  --with-index_lib=PATH     Specify path to index_lib installation. Default /usr/local ],
						[
						 if test "x$withval" != "xno" ; then
							 trylib=$withval
						 fi
						]
)

index_lib_lib_path_name="lib"
AC_CACHE_CHECK([for index_lib directory], ac_cv_index_lib_dir, [
  saved_LIBS="$LIBS"
  saved_LDFLAGS="$LDFLAGS"
  saved_CPPFLAGS="$CPPFLAGS"
  found=no
	for sdir in $trylib "" $prefix /usr/local ; do
		LDFLAGS="$saved_LDFLAGS"
		LIBS="$saved_LIBS -lindex_lib"

		# Skip the directory if it isn't there.
		if test ! -z "$sdir" -a ! -d "$sdir" ; then
			continue;
		fi
		if test ! -z "$sdir" ; then
			if test -d "$sdir/lib" ; then
				LDFLAGS="-L$sdir/lib $LDFLAGS"
				index_lib_lib_path_name="lib"
			fi
			if test -d "$sdir/include" ; then
				CPPFLAGS="-I$sdir/include $CPPFLAGS"
			else
				CPPFLAGS="-I$sdir $CPPFLAGS"
			fi
		fi
        AC_TRY_LINK([#include <index_lib/IndexLib.h>], [ ], [ index_lib_linked=yes ], [ index_lib_linked=no ])
		if test $index_lib_linked = yes; then
			if test ! -z "$sdir" ; then
				ac_cv_index_lib_dir=$sdir
			else
				ac_cv_index_lib_dir="(system)"
			fi
			found=yes
			break
		fi
	done
	LIBS="$saved_LIBS"
	LDFLAGS="$saved_LDFLAGS"
	CPPFLAGS="$saved_CPPFLAGS"
	if test $found = no ; 
	then
		AC_MSG_ERROR([index_lib is required. If it's already installed, specify its path using --with-index_lib=/dir/])
		fi
		])
LIBS="-lindex_lib $LIBS"
if test $ac_cv_index_lib_dir != "(system)"; then
	if test -d "$ac_cv_index_lib_dir/$index_lib_lib_path_name" ; then
		LDFLAGS="-L$ac_cv_index_lib_dir/$index_lib_lib_path_name $LDFLAGS"
		index_libroot="$ac_cv_index_lib_dir/$index_lib_lib_path_name"
	else
		LDFLAGS="-L$ac_cv_index_lib_dir $LDFLAGS"
		index_libroot="$ac_cv_index_lib_dir"
	fi
	if test -d "$ac_cv_index_lib_dir/include" ; then
		CPPFLAGS="-I$ac_cv_index_lib_dir/include $CPPFLAGS"
	else
		CPPFLAGS="-I$ac_cv_index_lib_dir $CPPFLAGS"
	fi
fi


trylib=""
AC_ARG_WITH(framework,
            [  --with-framework=PATH     Specify path to framework installation. Default /usr/lo
             cal ],
             [
              if test "x$withval" != "xno" ; then
                  trylib=$withval
              fi
              ]
             )

framework_lib_path_name="lib"
AC_CACHE_CHECK([for framework directory], ac_cv_framework_dir, [
                saved_LIBS="$LIBS"
                saved_LDFLAGS="$LDFLAGS"
                saved_CPPFLAGS="$CPPFLAGS"
                found=no
                for sdir in $trylib "" $prefix /usr/local ; do
                    framework_lib_path_name="lib"
                    LDFLAGS="$saved_LDFLAGS"
                    LIBS="-lframework $saved_LIBS"
                    # Skip the directory if it isn't there.
                    if test ! -z "$sdir" -a ! -d "$sdir" ; then
                        continue;
                    fi
                    if test ! -z "$sdir" ; then
                        if test -e "$sdir/lib64/libframework.so" ; then
                            LDFLAGS="-L$sdir/lib64 $LDFLAGS"
                            framework_lib_path_name="lib64"
                        elif test -d "$sdir/lib" ; then
                            LDFLAGS="-L$sdir/lib $LDFLAGS"
                            framework_lib_path_name="lib"
                        else
                            LDFLAGS="-L$sdir $LDFLAGS"
                            framework_lib_path_name=""
                        fi
                        if test -d "$sdir/include" ; then
                            CPPFLAGS="-I$sdir/include $CPPFLAGS"
                        else
                            CPPFLAGS="-I$sdir $CPPFLAGS"
                        fi
                    fi

                    AC_CHECK_HEADERS([framework/Session.h],
                                     AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <framewo
                                                     rk/Session.h>]],


                                                     [[return framework::Session::getCurrentCount();]])
                                     ],
                                     [framework_linked=yes],
                                     [framework_linked=no]
                                     ),
                                     [framework_linked=no]
                                     )
                    framework_linked=yes
                    if test $framework_linked = yes; then
                        if test ! -z "$sdir" ; then
                            ac_cv_framework_dir=$sdir
                        else
                            ac_cv_framework_dir="(system)"
                        fi
                        found=yes
                        break
                    fi
                done
                LIBS="$saved_LIBS"
                LDFLAGS="$saved_LDFLAGS"
                CPPFLAGS="$saved_CPPFLAGS"
                if test $found = no ;
                then
                    AC_MSG_ERROR([framework is required. If it is already installed, specify its path using --with-framework=/dir/])
                    fi
                    ])
LIBS="-lframework $LIBS"
if test $ac_cv_framework_dir != "(system)"; then
    if test -d "$ac_cv_framework_dir/$framework_lib_path_name" ; then
        LDFLAGS="-L$ac_cv_framework_dir/$framework_lib_path_name $LDFLAGS"
        frameworkroot="$ac_cv_framework_dir/$framework_lib_path_name"
    else
        LDFLAGS="-L$ac_cv_framework_dir $LDFLAGS"
        frameworkroot="$ac_cv_framework_dir"
    fi
    if test -d "$ac_cv_framework_dir/include" ; then
        CPPFLAGS="-I$ac_cv_framework_dir/include $CPPFLAGS"
    else
        CPPFLAGS="-I$ac_cv_framework_dir $CPPFLAGS"
    fi
fi


LIBS=""
LDFLAGS=""

AC_CONFIG_FILES([Makefile     \
                 src/Makefile \
                 include/Makefile \
                 include/queryparser/Makefile \
                 ])  
                 AC_OUTPUT

