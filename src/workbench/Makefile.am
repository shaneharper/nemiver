workbenchmod_LTLIBRARIES=libworkbenchmod.la
workbenchmoddir=@NEMIVER_MODULES_DIR@

h=$(abs_srcdir)

headers= \
$(h)/nmv-i-workbench.h \
$(h)/nmv-i-pref-mgr.h

libworkbenchmod_la_SOURCES= \
$(headers) \
$(h)/nmv-workbench.cc \
$(h)/nmv-pref-mgr.cc

publicheaders_DATA=$(headers)
publicheadersdir=$(NEMIVER_INCLUDE_DIR)/dynmods

libworkbenchmod_la_LDFLAGS=-module -avoid-version -Wl,--as-needed
libworkbenchmod_la_LIBADD=@NEMIVERWORKBENCH_LIBS@ \
        $(abs_top_builddir)/src/common/libnemivercommon.la \
        $(abs_top_builddir)/src/uicommon/libnemiveruicommon.la

config_DATA=workbench.conf
configdir=@NEMIVER_SYSTEM_CONFIG_DIR@

INCLUDES=@NEMIVERWORKBENCH_CFLAGS@ -DENABLE_NLS=1 -DDATADIR=\"${datadir}\" \
-I$(abs_top_srcdir)/src \
-I$(abs_top_srcdir)/src/confmgr \
-I$(abs_top_srcdir)/src/dbgengine \
-I$(abs_top_srcdir)/src/uicommon \
-I$(abs_top_srcdir)/src/persp

EXTRA_DIST=workbench.conf.in

