#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.50])
AC_INIT(libclustermap, 0.0.1, santai.ww@taobao.com)
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

backup_LIBS="$LIBS"

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
  saved_CPPFLAGS="$CPPFLAGS"
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
			if test -e "$sdir/lib64/libkslib.so" ; then
				LDFLAGS="-L$sdir/lib64 $LDFLAGS"
				kslib_lib_path_name="lib64"
			elif test -d "$sdir/lib" ; then
				LDFLAGS="-L$sdir/lib $LDFLAGS"
				kslib_lib_path_name="lib"
			else
				LDFLAGS="-L$sdir $LDFLAGS"
				kslib_lib_path_name=""
			fi
			if test -d "$sdir/include" ; then
				CPPFLAGS="-I$sdir/include $CPPFLAGS"
			else
				CPPFLAGS="-I$sdir $CPPFLAGS"
			fi
		fi
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
	CPPFLAGS="$saved_CPPFLAGS"
	if test $found = no ; 
	then
		AC_MSG_ERROR([kslib is required. If it's already installed, specify its path using --with-kslib=/dir/])
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
		CPPFLAGS="-I$ac_cv_kslib_dir/include $CPPFLAGS"
	else
		CPPFLAGS="-I$ac_cv_kslib_dir $CPPFLAGS"
	fi
fi

trylib=""
LIBS="$LIBS -lpthread"
AC_ARG_WITH(protobuf, 
						[  --with-protobuf=PATH     Specify path to protobuf installation. Default /usr/local ],
						[
						 if test "x$withval" != "xno" ; then
							 trylib=$withval
						 fi
						 ]
						)
AC_CACHE_CHECK([for protobuf directory], ac_cv_protobuf_dir, [
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
									AC_TRY_LINK([#include <google/protobuf/descriptor.pb.h>], [ ], [ protobuf_linked=yes ], [ protobuf_linked=no 
															 ])
									if test $protobuf_linked = yes; then
										if test ! -z "$sdir" ; then
											ac_cv_protobuf_dir=$sdir
										else
											ac_cv_protobuf_dir="(system)"
										fi
										found=yes
										break
									fi
								done
								LIBS="$saved_LIBS"
								LDFLAGS="$saved_LDFLAGS"
								CXXFLAGS="$saved_CXXFLAGS"
								if test $found = no ; then
									AC_MSG_ERROR([protobuf is required. If it's already installed, specify its path using --with-protobuf=/dir/
																])
									fi
									])
LIBS="$LIBS -lprotobuf"
if test $ac_cv_protobuf_dir != "(system)"; then
	if test -d "$ac_cv_protobuf_dir/lib" ; then
		LDFLAGS="-L$ac_cv_protobuf_dir/lib $LDFLAGS"
		protobufroot="$ac_cv_protobuf_dir/lib"
	else
		LDFLAGS="-L$ac_cv_protobuf_dir $LDFLAGS"
		protobufroot="$ac_cv_protobuf_dir"
	fi
	if test -d "$ac_cv_protobuf_dir/include" ; then
		CXXFLAGS="-I$ac_cv_protobuf_dir/include $CXXFLAGS"
	else
		CXXFLAGS="-I$ac_cv_protobuf_dir $CXXFLAGS"
	fi
fi

LIBS="$backup_LIBS"

CPPFLAGS="-D_FILE_OFFSET_BITS=64 -DSHMEM -I. $CPPFLAGS -I`pwd`/include -I`pwd`/src -I`pwd` -Wall"
LDFLAGS="-D_FILE_OFFSET_BITS=64 -DSHMEM -I. $LDFLAGS  -I`pwd`/include -I`pwd`/src -I`pwd`"

AC_CONFIG_FILES([Makefile src/Makefile test/Makefile])
AC_OUTPUT
