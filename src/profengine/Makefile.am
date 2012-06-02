perfmod_LTLIBRARIES=libperfmod.la
perfmoddir=@NEMIVER_MODULES_DIR@

noinst_LTLIBRARIES=\
libperfengine.la

h=$(abs_srcdir)

iprofilerheaders= \
$(h)/nmv-i-profiler.h

dynmodheaders = \
$(iprofilerheaders)

libperfmod_la_SOURCES= \
$(iprofilerheaders)

libperfengine_la_SOURCES= \
$(h)/nmv-perf-engine.cc \
$(h)/nmv-perf-engine.h \
$(h)/nmv-call-graph-node.cc \
$(h)/nmv-call-graph-node.h

libperfengine_la_CFLAGS=-fPIC -DPIC

publicheaders_DATA=$(dynmodheaders)
publicheadersdir=$(NEMIVER_INCLUDE_DIR)/dynmods

libperfmod_la_LDFLAGS=-module -avoid-version -Wl,--as-needed
libperfmod_la_LIBADD=libperfengine.la @NEMIVERCOMMON_LIBS@ \
$(abs_top_builddir)/src/common/libnemivercommon.la


config_DATA=perfengine.conf
configdir=@NEMIVER_SYSTEM_CONFIG_DIR@

INCLUDES=@NEMIVERCOMMON_CFLAGS@ -DENABLE_NLS=1 -DDATADIR=\"${datadir}\" \
-I$(top_srcdir)/src -I$(top_srcdir)/src/confmgr
