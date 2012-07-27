#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.50])
AC_INIT(libutil,0.0.1,gongyi.cl@taobao.com)
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

FLAGS="-Wall -D_FILE_OFFSET_BITS=64 -DSHMEM -I. -I`pwd`/include -I`pwd`/src -I`pwd`"
CPPFLAGS="$FLAGS $CPPFLAGS -fno-omit-frame-pointer"
CFLAGS="$FLAGS $CFLAGS -fno-omit-frame-pointer"

LDFLAGS="-D_FILE_OFFSET_BITS=64 -DSHMEM -I. $LDFLAGS -I`pwd`/include -I`pwd`/src -I`pwd`"

trylib=""
AC_ARG_WITH(mxml, 
						[  --with-mxml=PATH     Specify path to mxml installation. Default /usr/local ],
						[
						 if test "x$withval" != "xno" ; then
							 trylib=$withval
						 fi
						 ]
						)
AC_CACHE_CHECK([for mxml directory], ac_cv_mxml_dir, [
                                LIBS="$LIBS -lpthread"
								saved_LIBS="$LIBS"
								saved_LDFLAGS="$LDFLAGS"
								saved_CPPFLAGS="$CPPFLAGS"
								found=no
								for sdir in $trylib "" $prefix /usr/local ; do
									LDFLAGS="$saved_LDFLAGS"
									LIBS="$saved_LIBS -lmxml"

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
											CPPFLAGS="-I$sdir/include $CPPFLAGS"
										else
											CPPFLAGS="-I$sdir $CPPFLAGS"
										fi
									fi
									AC_TRY_LINK([#include <mxml.h>], [ mxml_error("") ], [ mxml_linked=yes ], [ mxml_linked=no ])

									if test $mxml_linked = yes; then
										if test ! -z "$sdir" ; then
											ac_cv_mxml_dir=$sdir
										else
											ac_cv_mxml_dir="(system)"
										fi
										found=yes
										break
									fi
								done
								LIBS="$saved_LIBS"
								LDFLAGS="$saved_LDFLAGS"
								CPPFLAGS="$saved_CPPFLAGS"
								if test $found = no ; then
									AC_MSG_ERROR([mxml is required. If it is already installed, specify its path using --with-mxml=/dir/
																])
									fi
									])
LIBS="-lmxml $LIBS"
if test $ac_cv_mxml_dir != "(system)"; then
	if test -d "$ac_cv_mxml_dir/lib" ; then
		LDFLAGS="-L$ac_cv_mxml_dir/lib $LDFLAGS"
		mxmlroot="$ac_cv_mxml_dir/lib"
	else
		LDFLAGS="-L$ac_cv_mxml_dir $LDFLAGS"
		mxmlroot="$ac_cv_mxml_dir"
	fi
	if test -d "$ac_cv_mxml_dir/include" ; then
		CPPFLAGS="-I$ac_cv_mxml_dir/include $CPPFLAGS"
	else
		CPPFLAGS="-I$ac_cv_mxml_dir $CPPFLAGS"
	fi
fi

trylib=""
AC_ARG_WITH(alog, 
						[  --with-alog=PATH     Specify path to alog installation. Default /usr/local ],
						[
						 if test "x$withval" != "xno" ; then
							 trylib=$withval
						 fi
						]
)

alog_lib_path_name="lib"
AC_CACHE_CHECK([for alog directory], ac_cv_alog_dir, [
  saved_LIBS="$LIBS"
  saved_LDFLAGS="$LDFLAGS"
  saved_CPPFLAGS="$CPPFLAGS"
  found=no
	for sdir in $trylib "" $prefix /usr/local ; do
		alog_lib_path_name="lib"
		LDFLAGS="$saved_LDFLAGS"
		LIBS="$saved_LIBS -lalog"
		# Skip the directory if it isn't there.
		if test ! -z "$sdir" -a ! -d "$sdir" ; then
			continue;
		fi
		if test ! -z "$sdir" ; then
			if test -e "$sdir/lib64/libalog.so" ; then
				LDFLAGS="-L$sdir/lib64 $LDFLAGS"
				alog_lib_path_name="lib64"
			elif test -d "$sdir/lib" ; then
				LDFLAGS="-L$sdir/lib $LDFLAGS"
				alog_lib_path_name="lib"
			else
				LDFLAGS="-L$sdir $LDFLAGS"
				alog_lib_path_name=""
			fi
			if test -d "$sdir/include" ; then
				CPPFLAGS="-I$sdir/include $CPPFLAGS"
			else
				CPPFLAGS="-I$sdir $CPPFLAGS"
			fi
		fi
		AC_CHECK_HEADERS([alog/Logger.h alog/Configurator.h],
										 AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <alog/Logger.h>]],
																		[[return alog::Logger::getRootLogger() ? 0 : 1;]])
										 ],
										 [alog_linked=yes],
										 [alog_linked=no]
										 ),
										 [
											alog_linked=no
											]
										 )
		if test $alog_linked = yes; then
			if test ! -z "$sdir" ; then
				ac_cv_alog_dir=$sdir
			else
				ac_cv_alog_dir="(system)"
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
		AC_MSG_ERROR([alog is required. If it is already installed, specify its path using --with-alog=/dir/])
		fi
		])

LIBS="-lalog $LIBS"
if test $ac_cv_alog_dir != "(system)"; then
	if test -d "$ac_cv_alog_dir/$alog_lib_path_name" ; then
		LDFLAGS="-L$ac_cv_alog_dir/$alog_lib_path_name $LDFLAGS"
		alogroot="$ac_cv_alog_dir/$alog_lib_path_name"
	else
		LDFLAGS="-L$ac_cv_alog_dir $LDFLAGS"
		alogroot="$ac_cv_alog_dir"
	fi
	if test -d "$ac_cv_alog_dir/include" ; then
		CPPFLAGS="-I$ac_cv_alog_dir/include $CPPFLAGS"
	else
		CPPFLAGS="-I$ac_cv_alog_dir $CPPFLAGS"
	fi
fi

trylib=""
AC_ARG_WITH(pool, 
						[  --with-pool=PATH     Specify path to pool installation. Default /usr/local ],
						[
						 if test "x$withval" != "xno" ; then
							 trylib=$withval
						 fi
						]
)

  found=no
pool_lib_path_name="lib"
AC_CACHE_CHECK([for pool directory], ac_cv_pool_dir, [
  saved_LIBS="$LIBS"
  saved_LDFLAGS="$LDFLAGS"
  saved_CPPFLAGS="$CPPFLAGS"
	for sdir in $trylib "" $prefix /usr/local ; do
		pool_lib_path_name="lib"
		LDFLAGS="$saved_LDFLAGS"
		LIBS="$saved_LIBS -lpool -lalog"

		# Skip the directory if it isn't there.
		if test ! -z "$sdir" -a ! -d "$sdir" ; then
			continue;
		fi
		if test ! -z "$sdir" ; then
			if test -a "$sdir/lib64/libpool.so" ; then
				LDFLAGS="-L$sdir/lib64 $LDFLAGS"
				pool_lib_path_name="lib64"
			elif test -d "$sdir/lib" ; then
				LDFLAGS="-L$sdir/lib $LDFLAGS"
				pool_lib_path_name="lib"
			else
				LDFLAGS="-L$sdir $LDFLAGS"
				pool_lib_path_name=""
			fi
			if test -d "$sdir/include" ; then
				CPPFLAGS="-I$sdir/include $CPPFLAGS"
			else
				CPPFLAGS="-I$sdir $CPPFLAGS"
			fi
		fi
		AC_CHECK_HEADERS([pool/Pool.h],
										 AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <pool/ObjectPool.h>]],
																		[[return pool::ObjectPool<>::getPool() ? 0 : 1;]])
										 ],
										 [pool_linked=yes],
										 [pool_linked=no]
										 ),
										 [
											pool_linked=no
											]
										 )
		if test $pool_linked = yes; then
			if test ! -z "$sdir" ; then
				ac_cv_pool_dir=$sdir
			else
				ac_cv_pool_dir="(system)"
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
		AC_MSG_ERROR([pool is required. If it is already installed, specify its path using --with-pool=/dir/])
		fi
		])
LIBS="-lpool $LIBS"
if test "$found" != "no"; then
if test $ac_cv_pool_dir != "(system)"; then
	if test -d "$ac_cv_pool_dir/$pool_lib_path_name" ; then
		LDFLAGS="-L$ac_cv_pool_dir/$pool_lib_path_name $LDFLAGS"
		poolroot="$ac_cv_pool_dir/$pool_lib_path_name"
	else
		LDFLAGS="-L$ac_cv_pool_dir $LDFLAGS"
		poolroot="$ac_cv_pool_dir"
	fi
	if test -d "$ac_cv_pool_dir/include" ; then
		CPPFLAGS="-I$ac_cv_pool_dir/include $CPPFLAGS"
	else
		CPPFLAGS="-I$ac_cv_pool_dir $CPPFLAGS"
	fi
fi
fi

AC_CONFIG_FILES([Makefile test/Makefile src/Makefile include/Makefile include/dlmodule/Makefile include/util/Makefile])
AC_OUTPUT
