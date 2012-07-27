#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.50])
AC_INIT([kingso_updater],[KINGSO_UPDATER_VERSION],[pianqian.gc@taobao.com])

KINGSO_DETAIL_MAJOR_VERSION=0
KINGSO_DETAIL_MINOR_VERSION=0
KINGSO_DETAIL_MICRO_VERSION=1
KINGSO_DETAIL_RELEASE=rc_4
KINGSO_DETAIL_VERSION=$KINGSO_DETAIL_MAJOR_VERSION.$KINGSO_DETAIL_MINOR_VERSION.$KINGSO_DETAIL_MICRO_VERSION
LT_MAJOR=0
LT_REVISION=0
LT_AGE=0
AC_SUBST(LT_MAJOR)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)
AC_SUBST(KINGSO_DETAIL_MAJOR_VERSION)
AC_SUBST(KINGSO_DETAIL_MINOR_VERSION)
AC_SUBST(KINGSO_DETAIL_MICRO_VERSION)
AC_SUBST(KINGSO_DETAIL_RELEASE)
AC_SUBST(KINGSO_DETAIL_VERSION)

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
WITHWANGWANGONLINE=""
WITHONLINEREADER=""
WITHSTATISTIC=""
WITHSORT=""
WITHSIMON=""
WITHLIBDETAIL=""

AM_INIT_AUTOMAKE(kingso_updater, $KINGSO_UPDATER_VERSION)
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

CPPFLAGS="-fPIC -D_FILE_OFFSET_BITS=64 -DSHMEM -I. $CPPFLAGS -I`pwd`/include -I`pwd`/src -I`pwd`/src/common -I`pwd` -Wall"
LDFLAGS="-D_FILE_OFFSET_BITS=64 -DSHMEM -I. $LDFLAGS -I`pwd`/include -I`pwd`/src -I`pwd`/src/common -I`pwd`"
LIBS="-lpthread -lz"


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
    			if test -d "$sdir/lib" ; then
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
    	CPPFLAGS="$saved_CPPFLAGS"
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
    		CPPFLAGS="-I$ac_cv_kslib_dir/include $CPPFLAGS"
    	else
    		CPPFLAGS="-I$ac_cv_kslib_dir $CPPFLAGS"
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

    commdef_lib_path_name="lib"
    AC_CACHE_CHECK([for commdef directory], ac_cv_commdef_dir, [
                    saved_LIBS="$LIBS"
                    saved_LDFLAGS="$LDFLAGS"
                    saved_CPPFLAGS="$CPPFLAGS"
                    found=no
                    for sdir in $trylib "" $prefix /usr/local ; do
                        commdef_lib_path_name="lib"
                        LDFLAGS="$saved_LDFLAGS"
                        LIBS="-lcommdef $saved_LIBS"
                        # Skip the directory if it isn't there.
                        if test ! -z "$sdir" -a ! -d "$sdir" ; then
                            continue;
                        fi
                        if test ! -z "$sdir" ; then
                            if test -e "$sdir/lib64/libcommdef.so" ; then
                                LDFLAGS="-L$sdir/lib64 $LDFLAGS"
                                commdef_lib_path_name="lib64"
                            elif test -d "$sdir/lib" ; then
                                LDFLAGS="-L$sdir/lib $LDFLAGS"
                                commdef_lib_path_name="lib"
                            else
                                LDFLAGS="-L$sdir $LDFLAGS"
                                commdef_lib_path_name=""
                            fi
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
                    LIBS="$saved_LIBS"
                    LDFLAGS="$saved_LDFLAGS"
                    CPPFLAGS="$saved_CPPFLAGS"
                    if test $found = no ;
                    then
                        AC_MSG_ERROR([commdef is required. If it's already installed, specify its path using --with-commdef=/dir/])
                        fi
                        ])
    LIBS="-lcommdef $LIBS"
    if test $ac_cv_commdef_dir != "(system)"; then
        if test -d "$ac_cv_commdef_dir/$commdef_lib_path_name" ; then
            LDFLAGS="-L$ac_cv_commdef_dir/$commdef_lib_path_name $LDFLAGS"
            commdefroot="$ac_cv_commdef_dir/$commdef_lib_path_name"
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
            LIBS="$saved_LIBS"

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
    LIBS="$LIBS"
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

#libbuild
    trylib=""
    AC_ARG_WITH(libbuild, 
            [  --with-libbuild=PATH     Specify path to libbuild installation. Default /usr/local ],
            [
            if test "x$withval" != "xno" ; then
            trylib=$withval
            fi
            ]
            )
    libbuild_lib_path_name="lib"
    AC_CACHE_CHECK([for libbuild directory], ac_cv_libbuild_dir, [
            saved_LIBS="$LIBS"
            saved_LDFLAGS="$LDFLAGS"
            saved_CPPFLAGS="$CPPFLAGS"
            found=no
            for sdir in $trylib "" $prefix /usr/local ; do
            LDFLAGS="$saved_LDFLAGS"
            LIBS="$saved_LIBS -lbuild"

# Skip the directory if it isn't there.
            if test ! -z "$sdir" -a ! -d "$sdir" ; then
            continue; 
            fi      
            if test ! -z "$sdir" ; then
            if test -d "$sdir/lib" ; then
            LDFLAGS="-L$sdir/lib $LDFLAGS"
            libbuild_lib_path_name="lib"
            fi      
            if test -d "$sdir/include" ; then
            CPPFLAGS="-I$sdir/include $CPPFLAGS"
            else
    CPPFLAGS="-I$sdir $CPPFLAGS"
    fi
    fi
    AC_TRY_LINK([#include <ks_build/ks_build.h>], [ ], [ libbuild_linked=yes ], [ libbuild_linked=no 
            ])

    if test $libbuild_linked = yes; then
    if test ! -z "$sdir" ; then
    ac_cv_libbuild_dir=$sdir
    else
    ac_cv_libbuild_dir="(system)"
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
    AC_MSG_ERROR([libbuild is required. If it's already installed, specify its path using --with-libbuild=/dir/])
    fi
    ])
    if test $ac_cv_libbuild_dir != "(system)"; then
    if test -d "$ac_cv_libbuild_dir/$libbuild_lib_path_name" ; then
    LDFLAGS="-L$ac_cv_libbuild_dir/$libbuild_lib_path_name $LDFLAGS"
    else
    LDFLAGS="-L$ac_cv_libbuild_dir $LDFLAGS"
    fi
    if test -d "$ac_cv_libbuild_dir/include" ; then
    CPPFLAGS="-I$ac_cv_libbuild_dir/include $CPPFLAGS"
    else
    CPPFLAGS="-I$ac_cv_libbuild_dir $CPPFLAGS"
    fi
    fi

#Deal with --with-libdetail
    trylib=""
    AC_ARG_WITH(libdetail,
            [  --with-libdetail=PATH     Specify path to libdetail installation. Default /usr/local ],
            [   
            if test "x$withval" != "xno" ; then
            trylib=$withval
            WITHLIBDETAIL="--with-libdetail="${trylib}
            fi  
            ]   
            )   
    AC_CACHE_CHECK([for libdetail directory], ac_cv_libdetail_dir, [
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
    if test ! -f "$sdir/include/libdetail/detail.h" ; then
    continue;
    fi
    libdetail_linked=yes
    if test $libdetail_linked = yes; then
    if test ! -z "$sdir" ; then
    ac_cv_libdetail_dir=$sdir
    else
    ac_cv_libdetail_dir="(system)"
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
    AC_MSG_ERROR([libdetail is required. If it's already installed, specify its path using --with-libdetail=/dir/])
    fi
    ])
    LIBS="$LIBS"
    if test $ac_cv_libdetail_dir != "(system)"; then
    if test -d "$ac_cv_libdetail_dir/lib" ; then
    LDFLAGS="-L$ac_cv_libdetail_dir/lib $LDFLAGS"
    detailroot="$ac_cv_libdetail_dir/lib"
    else
    LDFLAGS="-L$ac_cv_libdetail_dir $LDFLAGS"
    detailroot="$ac_cv_libdetail_dir"
    fi
    if test -d "$ac_cv_libdetail_dir/include" ; then
    CPPFLAGS="-I$ac_cv_libdetail_dir/include $CPPFLAGS"
    else
    CPPFLAGS="-I$ac_cv_libdetail_dir $CPPFLAGS"
    fi
    fi

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
AC_SUBST(WITHSIMON)

AC_CONFIG_FILES([Makefile \
                 include/Makefile \
                 include/update/Makefile \
                 src/Makefile \
                 src/api/Makefile \
                 src/app/Makefile \
                 src/common/Makefile \
                 src/plugin/Makefile \
                 conf/Makefile \
                 test/Makefile])
AC_OUTPUT
