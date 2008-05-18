gconfmgrmod_LTLIBRARIES=libgconfmgrmod.la
gconfmgrmoddir=@NEMIVER_MODULES_DIR@

headers= nmv-i-conf-mgr.h

libgconfmgrmod_la_SOURCES= \
$(headers) \
nmv-gconf-mgr.cc

publicheaders_DATA=$(headers)
publicheadersdir=$(NEMIVER_INCLUDE_DIR)/dynmods

libgconfmgrmod_la_LDFLAGS=-module -avoid-version -Wl,--as-needed
libgconfmgrmod_la_LIBADD=@NEMIVERCOMMON_LIBS@ @GCONF_LIBS@ \
        $(top_builddir)/src/common/libnemivercommon.la

config_DATA=gconfmgr.conf
configdir=@NEMIVER_SYSTEM_CONFIG_DIR@

INCLUDES=@NEMIVERCOMMON_CFLAGS@ @GCONF_CFLAGS@ -DENABLE_NLS=1 -DDATADIR=\"${datadir}\" \
-I$(top_srcdir)/src

EXTRA_DIST=gconfmgr.conf.in

