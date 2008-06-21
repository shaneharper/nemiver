dnl **************************************************************
dnl Process this file with autoconf to produce a configure script.
dnl **************************************************************
AC_INIT(nemiver, 0.5.4)
AC_PREREQ(2.59)
AC_CONFIG_SRCDIR(README)
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_TARGET
AM_MAINTAINER_MODE
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AC_CONFIG_HEADER(config.h)

MAJOR_VERSION=0
MINOR_VERSION=5
MICRO_VERSION=4

NEMIVER_VERSION="$MAJOR_VERSION.$MINOR_VERSION.$MICRO_VERSION"
AC_SUBST(NEMIVER_VERSION)

dnl ************************************************************
dnl First, Here goes the list of the version of the librairies
dnl we depend on. Some variables are AC_SUBSTed so that they can be used in the
dnl .pc files as well
dnl ************************************************************
LIBGMODULE_VERSION=2.14
AC_SUBST([LIBGMODULE_VERSION])
LIBGTHREAD_VERSION=2.14
AC_SUBST([LIBGTHREAD_VERSION])
LIBGLIBMM_VERSION=2.14
AC_SUBST([LIBGLIBMM_VERSION])
LIBGNOMEVFS_VERSION=2.14
AC_SUBST([LIBGNOMEVFS_VERSION])
LIBXML2_VERSION=2.6.22
AC_SUBST([LIBXML2_VERSION])
LIBGTKMM_VERSION=2.10.0
AC_SUBST([LIBGTKMM_VERSION])
LIBGLADEMM_VERSION=2.6.0
AC_SUBST([LIBGLADEMM_VERSION])
LIBGTKSOURCEVIEWMM1_VERSION=0.3.0
AC_SUBST([LIBGTKSOURCEVIEWMM1_VERSION])
LIBGTKSOURCEVIEWMM2_VERSION=1.9.4
AC_SUBST([LIBGTKSOURCEVIEWMM2_VERSION])
LIBGIOMM_VERSION=2.15.2
AC_SUBST([LIBGIOMM_VERSION])
SQLITE3_VERSION=3.0
AC_SUBST([SQLITE3_VERSION])
LIBGTOP_VERSION=2.14
AC_SUBST([LIBGTOP_VERSION])
LIBVTE_VERSION=0.12.0
AC_SUBST([LIBVTE_VERSION])
GCONF_VERSION=2.14.0
AC_SUBST([GCONF_VERSION])
CPPUNIT_VERSION=1.10.0
AC_SUBST([CPPUNIT_VERSION])
GTKHEX_VERSION=2.21.4
AC_SUBST([GTKHEX_VERSION])

dnl *********************
dnl Checks for programs.
dnl *********************
AC_PROG_CXX
AC_GNU_SOURCE
#AC_HEADER_STDC

AC_PROG_INSTALL
AC_PROG_LIBTOOL

AC_LANG_CPLUSPLUS
AC_LANG_COMPILER_REQUIRE

AC_PATH_PROG(GDB_PROG,gdb)

if test x$GDB_PROG = x ; then
    AC_MSG_ERROR([didn't find the gdb debugger. please, install it.])
fi
AC_SUBST(GDB_PROG)

ENABLE_IDEBUGGER_ONLY=no
ENABLE_DBGENGINE=yes
ENABLE_WORKBENCH=yes

dnl *************************************
dnl Internationalization support
dnl *************************************
dnl To add a new language, add language code xx to po/LINGUAS
IT_PROG_INTLTOOL([0.33.0])
GETTEXT_PACKAGE=nemiver
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"],
                   [The domain to use with gettext])
AM_GLIB_GNU_GETTEXT

nemiverlocaledir=[${datadir}/locale]
AC_SUBST(nemiverlocaledir)

dnl ***************************
dnl Initialize GNOME doc utils
dnl ***************************
GNOME_DOC_INIT

dnl ***************************
dnl a list of enablexxx options
dnl ***************************

AC_ARG_ENABLE(idebuggeronly,
              AC_HELP_STRING([--enable-idebugger-only=yes|no],
                            [compile the idebuger iface only (default is no)]),
              ENABLE_IDEBUGGER_ONLY=$enableval,
              ENABLE_IDEBUGGER_ONLY=no)
if test x$ENABLE_IDEBUGGER_ONLY = xyes ; then
    ENABLE_DBGENGINE=yes
    ENABLE_WORKBENCH=no
fi

AC_ARG_ENABLE(autotests,
              AC_HELP_STRING([--enable-autotests=yes|no],
                             [enable automated tests (default is no)]),
              ENABLE_AUTOTESTS=$enableval,
              ENABLE_AUTOTESTS=auto)
if test x$ENABLE_AUTOTESTS = xauto ; then
    if test x$NEMIVER_DEVEL = xon ; then
        ENABLE_AUTOTESTS=yes
    else
        ENABLE_AUTOTESTS=no
    fi
fi

if test x$ENABLE_AUTOTESTS = xyes ; then
#AM_PATH_CPPUNIT($CPP_UNIT_VERSION)
    AC_MSG_NOTICE(support of automated tests enabled)
else
    AC_MSG_NOTICE(disabled support of automated tests)
fi
AM_CONDITIONAL(AUTOTESTS, test x$ENABLE_AUTOTESTS = xyes)

AC_ARG_ENABLE(workbench,
              AC_HELP_STRING([--enable-workbench=yes|no],
                            [enable the graphical gd++ workbench (default is yes)]),
              ENABLE_WORKBENCH=$enableval,
              ENABLE_WORKBENCH=yes)
if test x$ENABLE_WORKBENCH = xyes ; then
    AC_DEFINE([WITH_WORKBENCH], 1, [compile the workbench])
else
    ENABLE_WORKBENCH=no
fi


AC_ARG_ENABLE(memoryview,
              AC_HELP_STRING([--enable-memoryview=yes|no],
                            [enable the memory view hex editor (default is yes)]),
              ENABLE_MEMORYVIEW=$enableval,
              [PKG_CHECK_EXISTS([gtkhex >= $GTKHEX_VERSION], [ENABLE_MEMORYVIEW=yes], [ENABLE_MEMORYVIEW=no])])
if test x$ENABLE_MEMORYVIEW = xyes ; then
    AC_DEFINE([WITH_MEMORYVIEW], 1, [compile the the memory view widget])
else
    ENABLE_MEMORYVIEW=no
fi
AM_CONDITIONAL(BUILD_MEMORYVIEW, test x$ENABLE_MEMORYVIEW = xyes)


AC_ARG_ENABLE(sourceviewmm2,
              AC_HELP_STRING([--enable-sourceviewmm2=yes|no],
                            [enable the memory view hex editor (default is auto)]),
              ENABLE_SOURCEVIEWMM2=$enableval,
              [PKG_CHECK_EXISTS([gtksourceviewmm-2.0 >= $LIBGTKSOURCEVIEWMM2_VERSION], [ENABLE_SOURCEVIEWMM2=yes], [ENABLE_SOURCEVIEWMM2=no])])
if test x$ENABLE_SOURCEVIEWMM2 = xyes ; then
    AC_DEFINE([WITH_SOURCEVIEWMM2], 1, [build with gtksourceviewmm 2.x instead of 1.x])
    LIBGTKSOURCEVIEWMM_VERSION=$LIBGTKSOURCEVIEWMM2_VERSION
    LIBGTKSOURCEVIEWMM_PKG=gtksourceviewmm-2.0
else
    ENABLE_SOURCEVIEWMM2=no
    LIBGTKSOURCEVIEWMM_VERSION=$LIBGTKSOURCEVIEWMM1_VERSION
    LIBGTKSOURCEVIEWMM_PKG=libgtksourceviewmm-1.0
fi

AC_ARG_ENABLE(gio,
              AC_HELP_STRING([--enable-gio=yes|no],
                            [use gio instead of gnomevfs (default is auto)]),
              ENABLE_GIO=$enableval,
              [PKG_CHECK_EXISTS([giomm-2.4 >= $LIBGIOMM_VERSION], [ENABLE_GIO=yes], [ENABLE_GIO=no])])
if test x$ENABLE_GIO = xyes ; then
    AC_DEFINE([WITH_GIO], 1, [build with gio instead of gnomevfs])
    VFS_PKG=giomm-2.4
    VFS_VERSION=$LIBGIOMM_VERSION
else
    VFS_PKG=gnome-vfs-2.0
    VFS_VERSION=$LIBGNOMEVFS_VERSION
    ENABLE_GIO=no
fi


ENABLE_DEBUG=yes
AC_ARG_ENABLE(debug,
              AC_HELP_STRING([--enable-debug=yes|no],
                             [enable a lot of debug message dumps(default is yes)]),
              ENABLE_DEBUG=$enableval,
              ENABLE_DEBUG=yes)
if test x$ENABLE_DEBUG = xyes ; then
        AC_DEFINE([NEMIVER_DEBUG], 1,
        [enable a lot of messages dumps in the terminal])
else
        ENABLE_DEBUG=no
fi


ENABLE_GCC_SYMBOLS_VISIBILITY=no
AC_ARG_ENABLE(symsvis,
              AC_HELP_STRING([--enable-symvis=yes|no],
                             [use the gcc symbols visibility feature. >=n gcc4.  default is no)]),
              ENABLE_GCC_SYMBOLS_VISIBILITY=$enableval,
              ENABLE_GCC_SYMBOLS_VISIBILITY=no)

ENABLE_VARIABLE_WALKER=yes
AC_ARG_ENABLE(variable-walker,
              AC_HELP_STRING([--enable-variable-walker],
                             [enable the variable walker interface. default is enabled]),
              ENABLE_VARIABLE_WALKER=$enableval,
              ENABLE_VARIABLE_WALKER=yes)
if test x$ENABLE_VARIABLE_WALKER = xyes ; then
    AC_DEFINE([WITH_VARIABLE_WALKER], 1, [enable variables walker interface])
    AC_MSG_NOTICE([enabled variable walker])
else
    AC_MSG_NOTICE([disabled variable walker])
fi

dnl *******************************
dnl checking for libraries we use
dnl *******************************

#try to detect boost-dev include-only libs
AC_CHECK_HEADERS([boost/variant.hpp],
                 [],
                 [AC_MSG_ERROR([Please, install libboost development headers])])

PKG_CHECK_MODULES(LIBXML2, [libxml-2.0 >= $LIBXML2_VERSION])
PKG_CHECK_MODULES(LIBGMODULE, [gmodule-2.0 >= $LIBGMODULE_VERSION])
PKG_CHECK_MODULES(LIBGTHREAD, [gthread-2.0 >= $LIBGTHREAD_VERSION])
PKG_CHECK_MODULES(LIBVFS, [$VFS_PKG >= $VFS_VERSION])
PKG_CHECK_MODULES(LIBGTOP, [libgtop-2.0 >= $LIBGTOP_VERSION])
PKG_CHECK_MODULES(GCONF,[gconf-2.0 >= $GCONF_VERSION])
PKG_CHECK_MODULES(LIBGLIBMM, [glibmm-2.4 >= $LIBGLIBMM_VERSION])
PKG_CHECK_MODULES(SQLITE3, [sqlite3 >= $SQLITE3_VERSION])
PKG_CHECK_MODULES(LIBVTE, [vte >= $LIBVTE_VERSION])

if test x$ENABLE_AUTOTESTS = xyes ; then
    AX_BOOST_BASE([1.33.1])
    AX_BOOST_UNIT_TEST_FRAMEWORK
    AX_BOOST_TEST_EXEC_MONITOR

    unit_framework_lib_candidates="/usr/lib/libboost_unit_test_framework-st.a /usr/lib/libboost_unit_test_framework.a"
    for i in $unit_framework_lib_candidates ; do
        if test -f $i ; then
            BOOST_UNIT_TEST_FRAMEWORK_STATIC_LIB=$i
            break
        fi
    done
    AC_SUBST(BOOST_UNIT_TEST_FRAMEWORK_STATIC_LIB)
fi

if test x$ENABLE_WORKBENCH = xyes ; then
    PKG_CHECK_MODULES(LIBGTKMM, [gtkmm-2.4 >= $LIBGTKMM_VERSION])
    PKG_CHECK_MODULES(LIBGLADEMM, [libglademm-2.4 >= $LIBGLADEMM_VERSION])
    PKG_CHECK_MODULES(LIBGTKSOURCEVIEWMM, [$LIBGTKSOURCEVIEWMM_PKG >= $LIBGTKSOURCEVIEWMM_VERSION])
fi

if test x$ENABLE_MEMORYVIEW = xyes ; then
    PKG_CHECK_MODULES(GTKHEX, [gtkhex >= $GTKHEX_VERSION])
fi

dnl *******************
dnl gconf
dnl *******************
AC_PATH_PROG(GCONFTOOL, gconftool-2)
AM_GCONF_SOURCE_2
AC_SUBST(GCONFTOOL)

dnl library dependencies for libnemivercommon and the IDebugger libraries
NEMIVERCOMMON_LIBS="$LIBGMODULE_LIBS $LIBGTHREAD_LIBS $LIBXML2_LIBS"
NEMIVERCOMMON_LIBS="$NEMIVERCOMMON_LIBS $LIBGMODULE_LIBS $LIBGLIBMM_LIBS"
NEMIVERCOMMON_LIBS="$NEMIVERCOMMON_LIBS $LIBGTOP_LIBS $CPPUNIT_LIBS"
NEMIVERCOMMON_CFLAGS="$LIBGMODULE_CFLAGS $LIBGTHREAD_CFLAGS $LIBXML2_CFLAGS"
NEMIVERCOMMON_CFLAGS="$NEMIVERCOMMON_CFLAGS $LIBGTOP_CFLAGS $LIBGLIBMM_CFLAGS"
NEMIVERCOMMON_CFLAGS="$NEMIVERCOMMON_CFLAGS $CPPUNIT_CFLAGS"

AC_SUBST(NEMIVERCOMMON_LIBS)
AC_SUBST(NEMIVERCOMMON_CFLAGS)

dnl library dependencies for the nemiver common UI module
NEMIVERUICOMMON_LIBS="$NEMIVERCOMMON_LIBS $LIBGTKMM_LIBS $LIBGLADEMM_LIBS"
NEMIVERUICOMMON_LIBS="$NEMIVERUICOMMON_LIBS $LIBVTE_LIBS $LIBGTKSOURCEVIEWMM_LIBS"
NEMIVERUICOMMON_LIBS="$NEMIVERUICOMMON_LIBS $GTKHEX_LIBS"
NEMIVERUICOMMON_CFLAGS="$NEMIVERCOMMON_CFLAGS $LIBGTKMM_CFLAGS $LIBGLADEMM_CFLAGS"
NEMIVERUICOMMON_CFLAGS="$NEMIVERUICOMMON_CFLAGS $LIBVTE_CFLAGS"
NEMIVERUICOMMON_CFLAGS="$NEMIVERUICOMMON_CFLAGS $LIBGTKSOURCEVIEWMM_CFLAGS"
NEMIVERUICOMMON_CFLAGS="$NEMIVERUICOMMON_CFLAGS $GTKHEX_CFLAGS"

AC_SUBST(NEMIVERUICOMMON_LIBS)
AC_SUBST(NEMIVERUICOMMON_CFLAGS)

dnl library dependencies for the nemiver workbench module
NEMIVERWORKBENCH_LIBS="$NEMIVERCOMMON_LIBS $LIBGTKMM_LIBS $LIBGLADEMM_LIBS"
NEMIVERWORKBENCH_LIBS="$NEMIVERWORKBENCH_LIBS $LIBVFS_LIBS"
NEMIVERWORKBENCH_CFLAGS="$NEMIVERCOMMON_CFLAGS $LIBGTKMM_CFLAGS "
NEMIVERWORKBENCH_CFLAGS="$NEMIVERWORKBENCH_CFLAGS $LIBGLADEMM_CFLAGS"
NEMIVERWORKBENCH_CFLAGS="$NEMIVERWORKBENCH_CFLAGS $LIBVFS_CFLAGS"

AC_SUBST(NEMIVERWORKBENCH_LIBS)
AC_SUBST(NEMIVERWORKBENCH_CFLAGS)

dnl library dependencies for the nemiver debug perspective plugin
NEMIVERDBGPERSP_LIBS="$NEMIVERUICOMMON_LIBS $LIBGLADEMM_LIBS"
NEMIVERDBGPERSP_LIBS="$NEMIVERDBGPERSP_LIBS $LIBVTE_LIBS"
NEMIVERDBGPERSP_LIBS="$NEMIVERDBGPERSP_LIBS $LIBVFS_LIBS"

NEMIVERDBGPERSP_CFLAGS="$NEMIVERUICOMMON_CFLAGS $LIBGTKSOURCEVIEWMM_CFLAGS"
NEMIVERDBGPERSP_CFLAGS="$NEMIVERDBGPERSP_CFLAGS $LIBVTE_CFLAGS "
NEMIVERDBGPERSP_CFLAGS="$NEMIVERDBGPERSP_CFLAGS $LIBGLADEMM_CFLAGS"
NEMIVERDBGPERSP_CFLAGS="$NEMIVERDBGPERSP_CFLAGS $LIBVFS_CFLAGS"

AC_SUBST(NEMIVERDBGPERSP_LIBS)
AC_SUBST(NEMIVERDBGPERSP_CFLAGS)

NEMIVER_LIBS="$NEMIVERUICOMMON_LIBS"
NEMIVER_CFLAGS="$NEMIVERUICOMMON_CFLAGS"

AC_SUBST(NEMIVER_LIBS)
AC_SUBST(NEMIVER_CFLAGS)

NEMIVER_INCLUDE_DIR=$includedir/nemiver

#has_lib_sixty_four=`echo $libdir | grep lib64 | wc -l`
#if test x$has_lib_sixty_four = x0 ; then
#NEMIVER_LIB_DIR_NAME=lib
#else
#NEMIVER_LIB_DIR_NAME=lib64
#fi

#expand libdir
if test x$prefix = xNONE ; then
        prefix=$ac_default_prefix
fi
if test x$exec_prefix = xNONE ; then
        exec_prefix=$prefix ;
fi
NEMIVER_LIBDIR=`eval echo $libdir`

AC_SUBST(NEMIVER_INCLUDE_DIR)
AC_SUBST(NEMIVER_LIB_DIR_NAME)
AC_SUBST(NEMIVER_LIBDIR)


if test x$ENABLE_GCC_SYMBOLS_VISIBILITY = xyes && test x$host_cpu != xx86_64 ; then
    AC_DEFINE(HAS_GCC_VISIBILITY_SUPPORT,1,[gcc visibility support])
    REQUIRED_FLAGS="-fvisibility=hidden -fvisibility-inlines-hidden"
else
    REQUIRED_FLAGS=""
fi

dnl ***************************
dnl Set Dodji devel environment.
dnl ***************************
if test  "x$NEMIVER_DEVEL" = "xon" ; then
    CFLAGS=$DODJI_FLAGS
    CXXFLAGS="-Wall -g -Wextra -Wno-deprecated -fuse-cxa-atexit $REQUIRED_FLAGS"
    if test x$NEMIVER_ALLOW_WARNINGS != xon ; then
        CXXFLAGS="$CXXFLAGS -Werror"
    fi
    ENABLE_DEBUG=yes
else
    CXXFLAGS="$CXXFLAGS -fuse-cxa-atexit $REQUIRED_FLAGS"
fi

if test x$ENABLE_DEBUG != x ; then
    AC_DEFINE([NEMIVER_DEBUG],1,[enable debug messages])
fi

NEMIVER_INSTALL_PREFIX=$prefix
NEMIVER_SYSTEM_CONFIG_DIR=$libdir/nemiver/config
NEMIVER_MODULES_DIR=$libdir/nemiver/modules
NEMIVER_PLUGINS_DIR=$libdir/nemiver/plugins
NEMIVER_TEST_MODULES_DIR=$NEMIVER_MODULES_DIR/test-modules


AC_SUBST(NEMIVER_INSTALL_PREFIX)
AC_SUBST(NEMIVER_SYSTEM_CONFIG_DIR)
AC_SUBST(NEMIVER_MODULES_DIR)
AC_SUBST(NEMIVER_PLUGINS_DIR)
AC_SUBST(NEMIVER_TEST_MODULES_DIR)

dnl don't try to install gconf schemas during distcheck
DISTCHECK_CONFIGURE_FLAGS="--disable-schemas-install"
AC_SUBST([DISTCHECK_CONFIGURE_FLAGS])

AC_PROG_MAKE_SET

AC_CONFIG_FILES([Makefile
docs/Makefile
docs/Doxyfile
src/Makefile
  src/common/Makefile
  src/common/libnemivercommon.pc
  src/dbdimpl/Makefile
  src/dbdimpl/sqlite/Makefile
  src/dbdimpl/sqlite/org.nemiver.db.sqlitedriver.conf
  src/langs/Makefile
  src/dbgengine/Makefile
  src/dbgengine/gdbengine.conf
  src/dbgengine/cpptrait.conf
  src/dbgengine/varlist.conf
  src/dbgengine/varwalker.conf
  src/dbgengine/varlistwalker.conf
  src/confmgr/Makefile
  src/confmgr/gconfmgr.conf
  src/uicommon/Makefile
  src/workbench/Makefile
  src/workbench/workbench.conf
  src/workbench/schemas/Makefile
  src/persp/Makefile
  src/persp/dbgperspective/Makefile
  src/persp/dbgperspective/dbgperspective.conf
    src/persp/dbgperspective/glade/Makefile
    src/persp/dbgperspective/menus/Makefile
    src/persp/dbgperspective/icons/Makefile
    src/persp/dbgperspective/sqlscripts/Makefile
    src/persp/dbgperspective/sqlscripts/sqlite/Makefile
    src/persp/dbgperspective/schemas/Makefile
data/Makefile
    data/icons/Makefile
        data/icons/16x16/Makefile
        data/icons/22x22/Makefile
        data/icons/24x24/Makefile
        data/icons/32x32/Makefile
        data/icons/48x48/Makefile
        data/icons/scalable/Makefile
glade/Makefile
menus/Makefile
tests/Makefile
po/Makefile.in
help/Makefile])

AC_OUTPUT()

cat <<EOT

=================================================================
         Nemiver: $MAJOR_VERSION.$MINOR_VERSION.$MICRO_VERSION
=================================================================
        Here is the configuration of the package:

    Prefix                          : ${prefix}
    Source code location            : ${srcdir}
    C Compiler                      : ${CC}
    C++ Compiler                    : ${CXX}

    NEMIVER_DEVEL env var           : ${NEMIVER_DEVEL}
    NEMIVER_ALLOW_WARNINGS env var  : ${NEMIVER_ALLOW_WARNINGS}
    CFLAGS                          : ${CFLAGS}
    CXXFLAGS                        : ${CXXFLAGS}
    NEMIVER_LIBS                    : ${NEMIVER_LIBS}
    NEMIVER_CFLAGS                  : ${NEMIVER_CFLAGS}

 OPTIONAL FEATURES:
    Enable autotests                : ${ENABLE_AUTOTESTS}
    Enable verbose debug messages   : ${ENABLE_DEBUG}
    Enable workbench                : ${ENABLE_WORKBENCH}
    Enable memory view              : ${ENABLE_MEMORYVIEW} (requires gtkhex > $GTKHEX_VERSION)
    gtksourceviewmm version         : ${LIBGTKSOURCEVIEWMM_PKG}
    VFS library                     : ${VFS_PKG}
    Enable variable walker          : ${ENABLE_VARIABLE_WALKER}
    Enable symbols visibility ctrl  : ${ENABLE_GCC_SYMBOLS_VISIBILITY}
    Maintainer mode                 : ${USER_MAINTAINER_MODE}

Now, Type 'make' to compile Nemiver
Once done, type 'make install' with sufficient privileges to install it to $prefix

I hope you will enjoy it.

--Dodji
EOT

