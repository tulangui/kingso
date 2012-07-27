#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.50)
AC_INIT(kingso_offline, 0.0.1, gongyi.cl@taobao.com)
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
# AC_PROG_AWK
AC_PROG_CXX
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_LIBTOOL
AC_LANG_CPLUSPLUS

# Checks for libraries.
# FIXME: Replace `main' with a function in `-ldl':
AC_CHECK_LIB([dl], [main])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h locale.h malloc.h stdlib.h string.h sys/time.h unistd.h wchar.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([gettimeofday memmove memset setlocale strcasecmp strchr strdup strtol strtoul])

FLAGS="-Wall -D_FILE_OFFSET_BITS=64 -DSHMEM -I. -I`pwd`/include -I`pwd`/src -I`pwd`"
CPPFLAGS="$FLAGS $CPPFLAGS -fno-omit-frame-pointer"
CFLAGS="$FLAGS $CFLAGS -fno-omit-frame-pointer"
LDFLAGS="-D_FILE_OFFSET_BITS=64 -DSHMEM -I. $LDFLAGS -I`pwd`/include -I`pwd`/src -I`pwd`"


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

trylib=""
AC_ARG_WITH(commdef, 
						[  --with-commdef=PATH     Specify path to commdef installation. Default /usr/local ],
						[
						 if test "x$withval" != "xno" ; then
							 trylib=$withval
						 fi
						 ]
						)
AC_CACHE_CHECK([for commdef directory], ac_cv_commdef_dir, [
								saved_LIBS="$LIBS"
								saved_LDFLAGS="$LDFLAGS"
								saved_CXXFLAGS="$CXXFLAGS"
								found=no
								for sdir in $trylib "" $prefix /usr/local ; do
									LDFLAGS="$saved_LDFLAGS"
									LIBS="$saved_LIBS"

									# Skip the directory if it isn't there.
									if test ! -z "$sdir" -a ! -d "$sdir" ; then
										continue;
									fi
									if test ! -z "$sdir" ; then
										if test -d "$sdir/lib" ; then
											LDFLAGS="-L$sdir/lib $LDFLAGS"
										else
											LDFLAGS="-L$sdir $LDFLAGS"
										fi
										if test -d "$sdir/include" ; then
											CXXFLAGS="-I$sdir/include $CXXFLAGS"
										else
											CXXFLAGS="-I$sdir $CXXFLAGS"
										fi
									fi
									AC_TRY_LINK([#include <commdef/commdef.h>], [ ], [ commdef_linked=yes ], [ commdef_linked=no 
															 ])
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
								LIBS="$saved_LIBS"
								LDFLAGS="$saved_LDFLAGS"
								CXXFLAGS="$saved_CXXFLAGS"
								if test $found = no ; then
									AC_MSG_ERROR([commdef is required. If it's already installed, specify its path using --with-commdef=/dir/
																])
									fi
									])
LIBS="$LIBS"
if test $ac_cv_commdef_dir != "(system)"; then
	if test -d "$ac_cv_commdef_dir/lib" ; then
		LDFLAGS="-L$ac_cv_commdef_dir/lib $LDFLAGS"
		commdefroot="$ac_cv_commdef_dir/lib"
	else
		LDFLAGS="-L$ac_cv_commdef_dir $LDFLAGS"
		commdefroot="$ac_cv_commdef_dir"
	fi
	if test -d "$ac_cv_commdef_dir/include" ; then
		CXXFLAGS="-I$ac_cv_commdef_dir/include $CXXFLAGS"
	else
		CXXFLAGS="-I$ac_cv_commdef_dir $CXXFLAGS"
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
		AC_MSG_ERROR([index_lib is required. If it"'"s already installed, specify its path using --with-index_lib=/dir/])
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



LDFLAGS="-lpthread -lssl $LDFLAGS"


AC_CONFIG_FILES([Makefile src/Makefile include/Makefile include/ks_build/Makefile test/Makefile conf/Makefile])
AC_OUTPUT
