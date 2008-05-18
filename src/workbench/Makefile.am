SUBDIRS=schemas

install-data-local: install-schemas

uninstall-local: uninstall-schemas

install-schemas:
	$(MAKE) -C schemas $(AM_MAKEFLAGS) install-schemas

uninstall-schemas:
	$(MAKE) -C schemas $(AM_MAKEFLAGS)  uninstall-schemas

workbenchmod_LTLIBRARIES=libworkbenchmod.la
workbenchmoddir=@NEMIVER_MODULES_DIR@

headers= \
nmv-i-workbench.h \
nmv-i-pref-mgr.h

libworkbenchmod_la_SOURCES= \
$(headers) \
nmv-workbench.cc \
nmv-pref-mgr.cc

publicheaders_DATA=$(headers)
publicheadersdir=$(NEMIVER_INCLUDE_DIR)/dynmods

libworkbenchmod_la_LDFLAGS=-module -avoid-version -Wl,--as-needed
libworkbenchmod_la_LIBADD=@NEMIVERWORKBENCH_LIBS@ \
        $(top_builddir)/src/common/libnemivercommon.la \
        $(top_builddir)/src/uicommon/libnemiveruicommon.la

config_DATA=workbench.conf
configdir=@NEMIVER_SYSTEM_CONFIG_DIR@

INCLUDES=@NEMIVERWORKBENCH_CFLAGS@ -DENABLE_NLS=1 -DDATADIR=\"${datadir}\" \
-I$(top_srcdir)/src \
-I$(top_srcdir)/src/confmgr \
-I$(top_srcdir)/src/dbgengine \
-I$(top_srcdir)/src/uicommon \
-I$(top_srcdir)/src/persp

EXTRA_DIST=workbench.conf.in

