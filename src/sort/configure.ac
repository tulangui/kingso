#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.50])
AC_INIT(statistic, 0.0.1, yanhui.tk@taobao.com)
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

#Deal with --with-index_lib
trylib=""
AC_ARG_WITH(index_lib,
            [  --with-index_lib=PATH     Specify path to index_lib installation. Default /usr/local ],
            [
             if test "x$withval" != "xno" ; then
                 trylib=$withval
             fi
             ]
            )
AC_CACHE_CHECK([for index_lib directory], ac_cv_index_lib_dir, [
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
                CPPFLAGS="$saved_CPPFLAGS"
                if test $found = no ;
                then
                    AC_MSG_ERROR([index_lib is required. If it's already installed, specify its path using --with-index_lib=/dir/])
                    fi
                    ])
if test $ac_cv_index_lib_dir != "(system)"; then
    if test -d "$ac_cv_index_lib_dir/include" ; then
        CPPFLAGS="-I$ac_cv_index_lib_dir/include $CPPFLAGS"
    else
        CPPFLAGS="-I$ac_cv_index_lib_dir $CPPFLAGS"
    fi
fi

#Deal with --with-framework
trylib=""
AC_ARG_WITH(framework,
            [  --with-framework=PATH     Specify path to index_lib installation. Default /usr/local ],
            [
             if test "x$withval" != "xno" ; then
                 trylib=$withval
             fi
             ]
            )
AC_CACHE_CHECK([for framework directory], ac_cv_framework_dir, [
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
                    if test ! -f "$sdir/include/framework/Context.h" ; then
                        continue;
                    fi
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
                CPPFLAGS="$saved_CPPFLAGS"
                if test $found = no ;
                then
                    AC_MSG_ERROR([framework is required. If it's already installed, specify its path using --with-framework=/dir/])
                    fi
                    ])
if test $ac_cv_framework_dir != "(system)"; then
    if test -d "$ac_cv_framework_dir/include" ; then
        CPPFLAGS="-I$ac_cv_framework_dir/include $CPPFLAGS"
    else
        CPPFLAGS="-I$ac_cv_framework_dir $CPPFLAGS"
    fi
fi

   #Deal with --with-queryparser
    trylib=""
    AC_ARG_WITH(queryparser,
                [  --with-queryparser=PATH     Specify path to queryparser installation. Default /usr/local ],
                [
                 if test "x$withval" != "xno" ; then
                     trylib=$withval
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

#Deal with --with-commdef
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
if test $ac_cv_commdef_dir != "(system)"; then
    if test -d "$ac_cv_commdef_dir/include" ; then
        CPPFLAGS="-I$ac_cv_commdef_dir/include $CPPFLAGS"
    else
        CPPFLAGS="-I$ac_cv_commdef_dir $CPPFLAGS"
    fi
fi


CPPFLAGS="-D_FILE_OFFSET_BITS=64 -DSHMEM -I. $CPPFLAGS -I`pwd`/include -I`pwd`/src -I`pwd` "
LDFLAGS="-D_FILE_OFFSET_BITS=64 -DSHMEM -I. $LDFLAGS  -I`pwd`/include -I`pwd`/src -I`pwd`" 

AC_CONFIG_FILES([Makefile include/Makefile include/sort/Makefile src/Makefile])
AC_OUTPUT
