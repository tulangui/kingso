#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(index_lib, 0.0.1, xiaojie.lgx@taobao.com)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([include/index_lib/IndexTerm.h])
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
# AM_PATH_CPPUNIT(1.12)

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

LIBS="-lpthread"
backup_LIBS="$LIBS"

trylib=""
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
LIBS="$LIBS"
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

CXXFLAGS="-D_FILE_OFFSET_BITS=64 -DSHMEM -I. $CXXFLAGS -I`pwd`/include -I`pwd`/src -I`pwd` -Wall -Winline"
LDFLAGS="-D_FILE_OFFSET_BITS=64 -DSHMEM -I. $LDFLAGS -I`pwd`/include -I`pwd`/src -I`pwd`"

LIBS="$backup_LIBS"


AC_CONFIG_FILES([Makefile     \
                 src/Makefile \
                 src/index/Makefile \
                 src/profile/Makefile \
                 src/app/Makefile \
                 test/Makefile \
                 test/index/Makefile \
                 test/profile/Makefile \
		 ])  
AC_OUTPUT

