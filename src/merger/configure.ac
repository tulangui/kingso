#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.50])
AC_INIT([kingso_merger],[KINGSO_MERGER_VERSION],[taiyi.zjh@taobao.com])

KINGSO_MERGER_MAJOR_VERSION=0
KINGSO_MERGER_MINOR_VERSION=0
KINGSO_MERGER_MICRO_VERSION=1
KINGSO_MERGER_RELEASE=rc_4
KINGSO_MERGER_VERSION=$KINGSO_MERGER_MAJOR_VERSION.$KINGSO_MERGER_MINOR_VERSION.$KINGSO_MERGER_MICRO_VERSION
LT_MAJOR=0
LT_REVISION=0
LT_AGE=0
AC_SUBST(LT_MAJOR)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)
AC_SUBST(KINGSO_MERGER_MAJOR_VERSION)
AC_SUBST(KINGSO_MERGER_MINOR_VERSION)
AC_SUBST(KINGSO_MERGER_MICRO_VERSION)
AC_SUBST(KINGSO_MERGER_RELEASE)
AC_SUBST(KINGSO_MERGER_VERSION)

WITHPROTOBUF=""
WITHLIBDETAIL=""
WITHUPDATE=""
WITHANALYSIS=""
WITHCLUSTERMAP=""
WITHMXML=""
WITHALOG=""
WITHANET=""
WITHPOOL=""
WITHUTIL=""
WITHCOMMDEF=""
WITHFRAMEWORK=""
WITHINDEXLIB=""
WITHQUERYPARSER=""
WITHSEARCH=""
WITHSTATISTIC=""
WITHSORT=""

AM_INIT_AUTOMAKE(kingso_merger, $KINGSO_MERGER_VERSION)
AC_CONFIG_HEADER([config.h])

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

CPPFLAGS="-D_FILE_OFFSET_BITS=64 -DSHMEM -I. $CPPFLAGS -I`pwd`/include -I`pwd`/src/ -I`pwd`/src/frame -I`pwd`"
LDFLAGS="-D_FILE_OFFSET_BITS=64 -DSHMEM -I. $LDFLAGS -I`pwd`/include -I`pwd`/src -I`pwd`"
LIBS="-lpthread -lz -lssl"

#protobuf
trylib=""
AC_ARG_WITH(protobuf,
            [  --with-protobuf=PATH     Specify path to protobuf installation. Default /usr/local ],
            [
             if test "x$withval" != "xno" ; then
                 trylib=$withval
                 WITHPROTOBUF="--with-protobuf="${trylib}
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
LIBS="$LIBS"
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
LIBS="$LIBS -lkslib"
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
AC_ARG_WITH(clustermap,
            [  --with-clustermap=PATH     Specify path to clustermap installation. Default /usr/local ],
            [
             if test "x$withval" != "xno" ; then
                 trylib=$withval
                 WITHCLUSTERMAP="--with-clustermap="${trylib}
             fi
             ]
            )

clustermap_lib_path_name="lib"
AC_CACHE_CHECK([for clustermap directory], ac_cv_clustermap_dir, [
                saved_LIBS="$LIBS"
                saved_LDFLAGS="$LDFLAGS"
                saved_CPPFLAGS="$CPPFLAGS"
                found=no
                for sdir in $trylib "" $prefix /usr/local ; do
                    clustermap_lib_path_name="lib"
                    LDFLAGS="$saved_LDFLAGS"
                    LIBS="-lclustermap $saved_LIBS"
                    # Skip the directory if it isn't there.
                    if test ! -z "$sdir" -a ! -d "$sdir" ; then
                        continue;
                    fi
                    if test ! -z "$sdir" ; then
                        if test -e "$sdir/lib64/libclustermap.so" ; then
                            LDFLAGS="-L$sdir/lib64 $LDFLAGS"
                            clustermap_lib_path_name="lib64"
                        elif test -d "$sdir/lib" ; then
                            LDFLAGS="-L$sdir/lib $LDFLAGS"
                            clustermap_lib_path_name="lib"
                        else
                            LDFLAGS="-L$sdir $LDFLAGS"
                            clustermap_lib_path_name=""
                        fi
                        if test -d "$sdir/include" ; then
                            CPPFLAGS="-I$sdir/include $CPPFLAGS"
                        else
                            CPPFLAGS="-I$sdir $CPPFLAGS"
                        fi
                    fi

                    AC_CHECK_HEADERS([clustermap/CMClient.h], 
                                     AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include "clustermap/CMClient.h"]], 
                                                    [[clustermap::CMClient client;]],
                                                    [[return 0;]])],
                                                    [clustermap_linked=yes],
                                                    [clustermap_linked=no]),
                                                    [clustermap_linked=no])
                    clustermap_linked=yes
                    if test $clustermap_linked = yes; then
                        if test ! -z "$sdir" ; then
                            ac_cv_clustermap_dir=$sdir
                        else
                            ac_cv_clustermap_dir="(system)"
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
                    AC_MSG_ERROR([clustermap is required. If it is already installed, specify its path using --with-clustermap=/dir/])
                    fi
                    ])
LIBS="-lclustermap $LIBS"
if test $ac_cv_clustermap_dir != "(system)"; then
    if test -d "$ac_cv_clustermap_dir/$clustermap_lib_path_name" ; then
        LDFLAGS="-L$ac_cv_clustermap_dir/$clustermap_lib_path_name $LDFLAGS"
        clustermaproot="$ac_cv_clustermap_dir/$clustermap_lib_path_name"
    else
        LDFLAGS="-L$ac_cv_clustermap_dir $LDFLAGS"
        clustermaproot="$ac_cv_clustermap_dir"
    fi
    if test -d "$ac_cv_clustermap_dir/include" ; then
        CPPFLAGS="-I$ac_cv_clustermap_dir/include $CPPFLAGS"
    else
        CPPFLAGS="-I$ac_cv_clustermap_dir $CPPFLAGS"
    fi
fi

trylib=""
AC_ARG_WITH(zlib, 
            [  --with-zlib=PATH     Specify path to zlib installation. Default /usr/local ],
            [
             if test "x$withval" != "xno" ; then
                 trylib=$withval
                 WITHZLIB="--with-zlib="${trylib}
             fi
             ]
            )

AC_CACHE_CHECK([for zlib directory], ac_cv_zlib_dir, [
                saved_LIBS="$LIBS"
                saved_LDFLAGS="$LDFLAGS"
                saved_CPPFLAGS="$CPPFLAGS"
                found=no
                for sdir in $trylib "" $prefix /usr/local ; do
                    LDFLAGS="$saved_LDFLAGS"
                    LIBS="$saved_LIBS -lz"

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
                    AC_TRY_LINK([#include <zlib.h>], [ zlibVersion() ], [ zlib_linked=yes ], [ zlib_linked=no 
                                 ])
                    if test $zlib_linked = yes; then
                        if test ! -z "$sdir" ; then
                            ac_cv_zlib_dir=$sdir
                        else
                            ac_cv_zlib_dir="(system)"
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
                    AC_MSG_ERROR([zlib is required. If it's already installed, specify its path using --with-zlib=/dir/])
                    fi
                    ])
LIBS="-lz $LIBS"
if test $ac_cv_zlib_dir != "(system)"; then
    if test -d "$ac_cv_zlib_dir/lib" ; then
        LDFLAGS="-L$ac_cv_zlib_dir/lib $LDFLAGS"
        zlibroot="$ac_cv_zlib_dir/lib"
    else
        LDFLAGS="-L$ac_cv_zlib_dir $LDFLAGS"
        zlibroot="$ac_cv_zlib_dir"
    fi
    if test -d "$ac_cv_zlib_dir/include" ; then
        CPPFLAGS="-I$ac_cv_zlib_dir/include $CPPFLAGS"
    else
        CPPFLAGS="-I$ac_cv_zlib_dir $CPPFLAGS"
    fi
fi

#Deal with --with-commdef
trylib=""
AC_ARG_WITH(commdef,
            [  --with-commdef=PATH     Specify path to commdef installation. Default /usr/local ],
            [
             if test "x$withval" != "xno" ; then
                 trylib=$withval
                 WITHCOMMDEF="--with-commdef="${trylib}
             fi
             ]
            )

AC_CACHE_CHECK([for commdef directory], ac_cv_commdef_dir, [
                saved_CPPFLAGS="$CPPFLAGS"
                found=no
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
                    if test ! -f "$sdir/include/commdef/commdef.h" ; then
                        continue;
                    fi
                    commdef_linked=yes
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
                    AC_MSG_ERROR([commdef is required. If it's already installed, specify its path using --with-commdef=/dir/])
                    fi
                    ])
LIBS="-lcommdef $LIBS"
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


trylib=""
AC_ARG_WITH(framework,
            [  --with-framework=PATH     Specify path to framework installation. Default /usr/local ],
            [
             if test "x$withval" != "xno" ; then
                 trylib=$withval
                 WITHFRAMEWORK="--with-framework="${trylib}
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
                                     AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <framework/Session.h>]],
                                                    [[return framework::Session::getCurrentCount();]])
                                     ],
                                     [framework_linked=yes],
                                     [framework_linked=no]
                                     ),
                                     [framework_linked=no]
                                     )
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

#Deal with --with-index_lib
trylib=""
AC_ARG_WITH(index_lib,
            [  --with-index_lib=PATH     Specify path to index_lib installation. Default /usr/local ],
            [
             if test "x$withval" != "xno" ; then
                 trylib=$withval
                 WITHINDEXLIB="--with-index_lib="${trylib}
             fi
             ]
            )

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
                        else
                            LDFLAGS="-L$sdir $LDFLAGS"
                        fi
                        if test -d "$sdir/include" ; then
                            CPPFLAGS="-I$sdir/include $CPPFLAGS"
                        else
                            CPPFLAGS="-I$sdir $CPPFLAGS"
                        fi
                    fi
                    #AC_TRY_LINK([#include <index_lib.h>], [ index_libVersion() ], [ index_lib_linked=yes ], [ index_lib_linked=no
                                  #             ])
                    if test ! -f "$sdir/include/index_lib/IndexLib.h" ; then
                        continue;
                    fi
                    index_lib_linked=yes
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
    if test -d "$ac_cv_index_lib_dir/lib" ; then
        LDFLAGS="-L$ac_cv_index_lib_dir/lib $LDFLAGS"
        index_libroot="$ac_cv_index_lib_dir/lib"
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

#Deal with --with-queryparser
trylib=""
AC_ARG_WITH(queryparser,
            [  --with-queryparser=PATH     Specify path to queryparser installation. Default /usr/local ],
            [
             if test "x$withval" != "xno" ; then
                 trylib=$withval
                 WITHQUERYPARSER="--with-queryparser="${trylib}
             fi
             ]
            )
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
                        else
                            LDFLAGS="-L$sdir $LDFLAGS"
                        fi
                        if test -d "$sdir/include" ; then
                            CPPFLAGS="-I$sdir/include $CPPFLAGS"
                        else
                            CPPFLAGS="-I$sdir $CPPFLAGS"
                        fi
                    fi
                    if test ! -f "$sdir/include/queryparser/QueryParser.h" ; then
                        continue;
                    fi
                    queryparser_linked=yes
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
    if test -d "$ac_cv_queryparser_dir/lib" ; then
        LDFLAGS="-L$ac_cv_queryparser_dir/lib $LDFLAGS"
        queryparserroot="$ac_cv_queryparser_dir/lib"
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
AC_ARG_WITH(statistic,
            [  --with-statistic=PATH     Specify path to statistic installation. Default /usr/local ],
            [
             if test "x$withval" != "xno" ; then
                 trylib=$withval
                 WITHSTATISTIC="--with-statistic="${trylib}
             fi
             ]
            )
AC_CACHE_CHECK([for statistic directory], ac_cv_statistic_dir, [
                saved_LIBS="$LIBS"
                saved_LDFLAGS="$LDFLAGS"
                saved_CPPFLAGS="$CPPFLAGS"
                found=no
                for sdir in $trylib "" $prefix /usr/local ; do
                    LDFLAGS="$saved_LDFLAGS"
                    LIBS="$saved_LIBS -lstatistic"
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
                    if test ! -f "$sdir/include/statistic/StatisticResult.h" ; then
                        continue;
                    fi
                    if test ! -f "$sdir/include/statistic/StatisticManager.h" ; then
                        continue;
                    fi
                    statistic_linked=yes
                    if test $statistic_linked = yes; then
                        if test ! -z "$sdir" ; then
                            ac_cv_statistic_dir=$sdir
                        else
                            ac_cv_statistic_dir="(system)"
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
                    AC_MSG_ERROR([statistic is required. If it's already installed, specify its path using --with-statistic=/dir/])
                    fi
                    ])
LIBS="-lstatistic $LIBS"
if test $ac_cv_statistic_dir != "(system)"; then
    if test -d "$ac_cv_statistic_dir/lib" ; then
        LDFLAGS="-L$ac_cv_statistic_dir/lib $LDFLAGS"
        statisticroot="$ac_cv_statistic_dir/lib"
    else
        LDFLAGS="-L$ac_cv_statistic_dir $LDFLAGS"
        statisticroot="$ac_cv_statistic_dir"
    fi
    if test -d "$ac_cv_statistic_dir/include" ; then
        CPPFLAGS="-I$ac_cv_statistic_dir/include $CPPFLAGS"
    else
        CPPFLAGS="-I$ac_cv_statistic_dir $CPPFLAGS"
    fi
fi

#Deal with --with-search
trylib=""
AC_ARG_WITH(search,
            [  --with-search=PATH     Specify path to search installation. Default /usr/local ],
            [
             if test "x$withval" != "xno" ; then
                 trylib=$withval
                 WITHSEARCH="--with-search="${trylib}
             fi
             ]
            )
AC_CACHE_CHECK([for search directory], ac_cv_search_dir, [
                saved_LIBS="$LIBS"
                saved_LDFLAGS="$LDFLAGS"
                saved_CPPFLAGS="$CPPFLAGS"
                found=no
                for sdir in $trylib "" $prefix /usr/local ; do
                    LDFLAGS="$saved_LDFLAGS"
                    LIBS="$saved_LIBS -lsearch"
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
                    if test ! -f "$sdir/include/search/Search.h" ; then
                        continue;
                    fi
                    search_linked=yes
                    if test $search_linked = yes; then
                        if test ! -z "$sdir" ; then
                            ac_cv_search_dir=$sdir
                        else
                            ac_cv_search_dir="(system)"
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
                    AC_MSG_ERROR([search is required. If it's already installed, specify its path using --with-search=/dir/])
                    fi
                    ])
LIBS="-lsearch $LIBS"
if test $ac_cv_search_dir != "(system)"; then
    if test -d "$ac_cv_search_dir/lib" ; then
        LDFLAGS="-L$ac_cv_search_dir/lib $LDFLAGS"
        searchroot="$ac_cv_search_dir/lib"
    else
        LDFLAGS="-L$ac_cv_search_dir $LDFLAGS"
        searchroot="$ac_cv_search_dir"
    fi
    if test -d "$ac_cv_search_dir/include" ; then
        CPPFLAGS="-I$ac_cv_search_dir/include $CPPFLAGS"
    else
        CPPFLAGS="-I$ac_cv_search_dir $CPPFLAGS"
    fi
fi

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

LIBS="$LIBS -lkslib"
AC_SUBST(WITHPROTOBUF)
AC_SUBST(WITHLIBDETAIL)
AC_SUBST(WITHUPDATE)
AC_SUBST(WITHANALYSIS)
AC_SUBST(WITHCLUSTERMAP)
AC_SUBST(WITHMXML)
AC_SUBST(WITHALOG)
AC_SUBST(WITHANET)
AC_SUBST(WITHPOOL)
AC_SUBST(WITHUTIL)
AC_SUBST(WITHCOMMDEF)
AC_SUBST(WITHFRAMEWORK)
AC_SUBST(WITHINDEXLIB)
AC_SUBST(WITHQUERYPARSER)
AC_SUBST(WITHSEARCH)
AC_SUBST(WITHWANGWANGONLINE)
AC_SUBST(WITHONLINEREADER)
AC_SUBST(WITHSTATISTIC)
AC_SUBST(WITHSORT)

AC_CONFIG_FILES([Makefile src/frame/Makefile src/plugin/Makefile test/Makefile conf/Makefile])
AC_OUTPUT
