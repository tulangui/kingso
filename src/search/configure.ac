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

LIBS="$LIBS -lpthread -lz -lssl"
LIBSBACKUP="$LIBS"


#kslib
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
  kslib_linked=no
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
                if test -e "$sdir/lib/libkslib.a" ; then
                    kslib_linked=yes
                fi
            else
                LDFLAGS="-L$sdir $LDFLAGS"
                kslib_lib_path_name=""
                if test -e "$sdir/libkslib.a" ; then
                    kslib_linked=yes
                fi
            fi
            if test -d "$sdir/include" ; then
                CPPFLAGS="-I$sdir/include $CPPFLAGS"
            else
                CPPFLAGS="-I$sdir $CPPFLAGS"
            fi
        fi

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
    else
        LDFLAGS="-L$ac_cv_kslib_dir $LDFLAGS"
    fi
    if test -d "$ac_cv_kslib_dir/include" ; then
        CPPFLAGS="-I$ac_cv_kslib_dir/include $CPPFLAGS"
    else
        CPPFLAGS="-I$ac_cv_kslib_dir $CPPFLAGS"
    fi
fi

#commdef
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
								saved_CPPFLAGS="$CPPFLAGS"
								found=no
								for sdir in $trylib "" $prefix /usr/local ; do
									LDFLAGS="$saved_LDFLAGS"
									LIBS="$saved_LIBS -lcommdef"

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
									AC_TRY_LINK([#include <commdef/commdef.h>], [ ], [ commdef_linked=yes ], [ commdef_linked=no ])
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
								CPPFLAGS="$saved_CPPFLAGS"
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
		CPPFLAGS="-I$ac_cv_commdef_dir/include $CPPFLAGS"
	else
		CPPFLAGS="-I$ac_cv_commdef_dir $CPPFLAGS"
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

#queryparser
trylib=""
AC_ARG_WITH(queryparser, 
						[  --with-queryparser=PATH     Specify path to queryparser installation. Default /usr/local ],
						[
						 if test "x$withval" != "xno" ; then
							 trylib=$withval
						 fi
						]
)

queryparser_lib_path_name="lib"
AC_CACHE_CHECK([for queryparser directory], ac_cv_queryparser_dir, [
  saved_LIBS="$LIBS"
  saved_LDFLAGS="$LDFLAGS"
  saved_CPPFLAGS="$CPPFLAGS"
  found=no
	for sdir in $trylib "" $prefix /usr/local ; do
		LDFLAGS="$saved_LDFLAGS"
		LIBS="$saved_LIBS -lqueryparser"

		# Skip the directory if it isn't there.
		if test ! -z "$sdir" -a ! -d "$sdir" ; then
			continue;
		fi
		if test ! -z "$sdir" ; then
			if test -d "$sdir/lib" ; then
				LDFLAGS="-L$sdir/lib $LDFLAGS"
				queryparser_lib_path_name="lib"
			fi
			if test -d "$sdir/include" ; then
				CPPFLAGS="-I$sdir/include $CPPFLAGS"
			else
				CPPFLAGS="-I$sdir $CPPFLAGS"
			fi
		fi
        AC_TRY_LINK([#include <queryparser/QPResult.h>], [ ], [ queryparser_linked=yes ], [ queryparser_linked=no])
		if test $queryparser_linked = yes; then
			if test ! -z "$sdir" ; then
				ac_cv_queryparser_dir=$sdir
			else
				ac_cv_queryparser_dir="(system)"
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
		AC_MSG_ERROR([queryparser is required. If it's already installed, specify its path using --with-queryparser=/dir/])
		fi
		])
LIBS="-lqueryparser $LIBS"
if test $ac_cv_queryparser_dir != "(system)"; then
	if test -d "$ac_cv_queryparser_dir/$queryparser_lib_path_name" ; then
		LDFLAGS="-L$ac_cv_queryparser_dir/$queryparser_lib_path_name $LDFLAGS"
		queryparserroot="$ac_cv_queryparser_dir/$queryparser_lib_path_name"
	else
		LDFLAGS="-L$ac_cv_queryparser_dir $LDFLAGS"
		queryparserroot="$ac_cv_queryparser_dir"
	fi
	if test -d "$ac_cv_queryparser_dir/include" ; then
		CPPFLAGS="-I$ac_cv_queryparser_dir/include $CPPFLAGS"
	else
		CPPFLAGS="-I$ac_cv_queryparser_dir $CPPFLAGS"
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

trylib=""
AC_ARG_WITH(analysis,
            [  --with-analysis=PATH     Specify path to analysis installation. Default /usr/lo
             cal ],
             [
              if test "x$withval" != "xno" ; then
                  trylib=$withval
              fi
              ]
             )

analysis_lib_path_name="lib"
AC_CACHE_CHECK([for analysis directory], ac_cv_analysis_dir, [
                saved_LIBS="$LIBS"
                saved_LDFLAGS="$LDFLAGS"
                saved_CPPFLAGS="$CPPFLAGS"
                found=no
                for sdir in $trylib "" $prefix /usr/local ; do
                    analysis_lib_path_name="lib"
                    LDFLAGS="$saved_LDFLAGS"
                    LIBS="-lanalysis $saved_LIBS"
                    # Skip the directory if it isn't there.
                    if test ! -z "$sdir" -a ! -d "$sdir" ; then
                        continue;
                    fi
                    if test ! -z "$sdir" ; then
                        if test -e "$sdir/lib64/libanalysis.so" ; then
                            LDFLAGS="-L$sdir/lib64 $LDFLAGS"
                            analysis_lib_path_name="lib64"
                        elif test -d "$sdir/lib" ; then
                            LDFLAGS="-L$sdir/lib $LDFLAGS"
                            analysis_lib_path_name="lib"
                        else
                            LDFLAGS="-L$sdir $LDFLAGS"
                            analysis_lib_path_name=""
                        fi
                        if test -d "$sdir/include" ; then
                            CPPFLAGS="-I$sdir/include $CPPFLAGS"
                        else
                            CPPFLAGS="-I$sdir $CPPFLAGS"
                        fi
                    fi

                    AC_TRY_LINK([#include <analysis/MetaCharAnalyzer.h>], [ ], [ analysis_linked=yes ], [ analysis_linked=no ])
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
                LIBS="$saved_LIBS"
                LDFLAGS="$saved_LDFLAGS"
                CPPFLAGS="$saved_CPPFLAGS"
                if test $found = no ;
                then
                    AC_MSG_ERROR([analysis is required. If it is already installed, specify its path using --with-analysis=/dir/])
                    fi
                    ])
LIBS="-lanalysis $LIBS"
if test $ac_cv_analysis_dir != "(system)"; then
    if test -d "$ac_cv_analysis_dir/$analysis_lib_path_name" ; then
        LDFLAGS="-L$ac_cv_analysis_dir/$analysis_lib_path_name $LDFLAGS"
        analysisroot="$ac_cv_analysis_dir/$analysis_lib_path_name"
    else
        LDFLAGS="-L$ac_cv_analysis_dir $LDFLAGS"
        analysisroot="$ac_cv_analysis_dir"
    fi
    if test -d "$ac_cv_analysis_dir/include" ; then
        CPPFLAGS="-I$ac_cv_analysis_dir/include $CPPFLAGS"
    else
        CPPFLAGS="-I$ac_cv_analysis_dir $CPPFLAGS"
    fi
fi

#protobuf
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
                saved_CPPFLAGS="$CPPFLAGS"
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
                            CPPFLAGS="-I$sdir/include $CPPFLAGS"
                        else
                            CPPFLAGS="-I$sdir $CPPFLAGS"
                        fi
                    fi
                    #AC_TRY_LINK([#include <google/protobuf/message.h>], [ ], [ protobuf_linked=yes ], [ protobuf_linked=no ])
                    protobuf_linked=yes;
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
                CPPFLAGS="$saved_CPPFLAGS"
                if test $found = no ; then
                    AC_MSG_ERROR([protobuf is required. If it's already installed, specify its path using --with-protobuf=/dir/
                                  ])
                    fi
                    ])
LIBS="$LIBS -lprotobuf"
echo "$ac_cv_protobuf_dir"
if test $ac_cv_protobuf_dir != "(system)"; then
    PROTOBUF_ROOT=$ac_cv_protobuf_dir
    if test -d "$ac_cv_protobuf_dir/lib" ; then
        LDFLAGS="-L$ac_cv_protobuf_dir/lib $LDFLAGS"
    else
        LDFLAGS="-L$ac_cv_protobuf_dir $LDFLAGS"
    fi
    if test -d "$ac_cv_protobuf_dir/include" ; then
        CPPFLAGS="-I$ac_cv_protobuf_dir/include $CPPFLAGS"
    else
        CPPFLAGS="-I$ac_cv_protobuf_dir $CPPFLAGS"
    fi
else
    PROTOBUF_ROOT="/usr/local"
fi
 
#sort
trylib=""
AC_ARG_WITH(sort,
            [  --with-sort=PATH     Specify path to sort installation. Default /usr/local ],
            [
             if test "x$withval" != "xno" ; then
                 trylib=$withval
                 WITHSORT="--with-sort="${trylib}
             fi
             ]
            )
AC_CACHE_CHECK([for sort directory], ac_cv_sort_dir, [
                saved_LIBS="$LIBS"
                saved_LDFLAGS="$LDFLAGS"
                saved_CPPFLAGS="$CPPFLAGS"
                found=no
                for sdir in $trylib "" $prefix /usr/local ; do
                    LDFLAGS="$saved_LDFLAGS"
                    LIBS="$saved_LIBS -lsort"

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

                        if test -d "$sdir/lib64" ; then
                            LDFLAGS="-L$sdir/lib64 $LDFLAGS"
                        fi
                        if test -d "$sdir/include" ; then
                            CPPFLAGS="-I$sdir/include $CPPFLAGS"
                        else
                            CPPFLAGS="-I$sdir $CPPFLAGS"
                        fi
                    fi
                    if test ! -f "$sdir/include/sort/SortResult.h" ; then
                        continue;
                    fi
                    sort_linked=yes
                    if test $sort_linked = yes; then
                        if test ! -z "$sdir" ; then
                            ac_cv_sort_dir=$sdir
                        else
                            ac_cv_sort_dir="(system)"
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
                    AC_MSG_ERROR([sort is required. If it's already installed, specify its path using --with-sort=/dir/])
                    fi
                    ])
LIBS="-lsort $LIBS"
if test $ac_cv_sort_dir != "(system)"; then
    if test -d "$ac_cv_sort_dir/lib" ; then
        LDFLAGS="-L$ac_cv_sort_dir/lib $LDFLAGS"
        sortroot="$ac_cv_sort_dir/lib"
    else
        LDFLAGS="-L$ac_cv_sort_dir $LDFLAGS"
        sortroot="$ac_cv_sort_dir"
    fi
    if test -d "$ac_cv_sort_dir/lib64" ; then
        LDFLAGS="-L$ac_cv_sort_dir/lib64 $LDFLAGS"
        sortroot="$ac_cv_sort_dir/lib64"
    fi
    if test -d "$ac_cv_sort_dir/include" ; then
        CPPFLAGS="-I$ac_cv_sort_dir/include -I$ac_cv_sort_dir/include/sort $CPPFLAGS"
    else
        CPPFLAGS="-I$ac_cv_sort_dir $CPPFLAGS"
    fi
fi

CPPFLAGS="-D_FILE_OFFSET_BITS=64 -DSHMEM -I. $CPPFLAGS -I`pwd`/include -I`pwd`/src -I`pwd` -Wall"
LDFLAGS="-D_FILE_OFFSET_BITS=64 -DSHMEM -I. $LDFLAGS  -I`pwd`/include -I`pwd`/src -I`pwd`"

LIBS="$LIBSBACKUP"

#AC_CONFIG_FILES([Makefile include/Makefile include/search/Makefile src/Makefile])
AC_CONFIG_FILES([Makefile include/Makefile include/search/Makefile src/Makefile test/Makefile])
AC_OUTPUT
