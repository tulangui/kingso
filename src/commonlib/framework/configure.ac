#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.50])
AC_INIT(server_framework,0.0.1,taiyi.zjh@taobao.com)
AM_INIT_AUTOMAKE
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

RELEASE=alpha
LIBS="-lpthread"
CPPFLAGS="-D_FILE_OFFSET_BITS=64 -DSHMEM -I. $CPPFLAGS -I`pwd`/include -I`pwd`/src -I`pwd`"
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
             fi
             ]
            )

clustermap_lib_path_name="lib"
AC_CACHE_CHECK([for clustermap directory], ac_cv_clustermap_dir, [
                saved_CPPFLAGS="$CPPFLAGS"
                found=no
                for sdir in $trylib "" $prefix /usr/local ; do
                    clustermap_lib_path_name="lib"
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
                    if test ! -f "$sdir/include/clustermap/CMClient.h" ; then
                        continue;
                    fi
                    clustermap_linked=yes;
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
                CPPFLAGS="$saved_CPPFLAGS"
                if test $found = no ;
                then
                    AC_MSG_ERROR([clustermap is required. If it is already installed, specify its path using --with-clustermap=/dir/])
                    fi
                    ])
if test $ac_cv_clustermap_dir != "(system)"; then
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
             fi
             ]
            )

AC_CACHE_CHECK([for zlib directory], ac_cv_zlib_dir, [
                saved_CPPFLAGS="$CPPFLAGS"
                found=no
                for sdir in $trylib "" $prefix /usr/local /usr ; do
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
                    if test ! -f "$sdir/include/zlib.h" ; then
                        continue;
                    fi
                    zlib_linked=yes;
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
                CPPFLAGS="$saved_CPPFLAGS"
                if test $found = no ; 
                then
                    AC_MSG_ERROR([zlib is required. If it's already installed, specify its path using --with-zlib=/dir/])
                    fi
                    ])
if test $ac_cv_zlib_dir != "(system)"; then
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

AC_SUBST([RELEASE])
AC_SUBST([USER])
AC_SUBST([HOSTNAME])

    AC_CONFIG_FILES([Makefile src/Makefile src/buildinfo.h include/Makefile include/framework/Makefile])
    AC_OUTPUT
