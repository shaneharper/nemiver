perfmod_LTLIBRARIES=libperfmod.la
perfmoddir=@NEMIVER_MODULES_DIR@

noinst_LTLIBRARIES=\
libperfengine.la \
libperfcommon.la

h=$(abs_srcdir)

iprofilerheaders= \
$(h)/nmv-i-profiler.h

dynmodheaders = \
$(iprofilerheaders)

libperfmod_la_SOURCES= \
$(iprofilerheaders)

libperfcommon_la_SOURCES= \
$(h)/nmv-perf-engine.cc \
$(h)/nmv-perf-engine.h \
$(h)/nmv-record-options.h \
$(h)/nmv-call-graph-node.cc \
$(h)/nmv-call-graph-node.h
libperfcommon_la_CFLAGS=-fPIC -DPIC

libperfengine_la_SOURCES= \
$(h)/nmv-confmgr-record-options.h \
$(h)/nmv-confmgr-record-options.cc
libperfengine_la_CFLAGS=-fPIC -DPIC

publicheaders_DATA=$(dynmodheaders)
publicheadersdir=$(NEMIVER_INCLUDE_DIR)/dynmods

libperfmod_la_LDFLAGS=-module -avoid-version -Wl,--as-needed
libperfmod_la_LIBADD=libperfcommon.la libperfengine.la \
@NEMIVERCOMMON_LIBS@ \
@NEMIVERPROFPERSP_LIBS@ \
$(abs_top_builddir)/src/common/libnemivercommon.la


config_DATA=perfengine.conf
configdir=@NEMIVER_SYSTEM_CONFIG_DIR@

bin_PROGRAMS=nmvperfserver
nmvperfserver_SOURCES=$(h)/nmv-perf-server.cc
nmvperfserver=@NEMIVER_MODULES_DIR@
nmvperfserver_LDADD=@NEMIVERPROFPERSP_LIBS@ \
$(abs_top_builddir)/src/common/libnemivercommon.la libperfcommon.la

service_DATA=org.gnome.nemiver.profiler.service
servicedir=$(datadir)/dbus-1/system-services

bus_DATA=org.gnome.nemiver.profiler.conf.in
busdir=/etc/dbus-1/system.d/

INCLUDES=@NEMIVERCOMMON_CFLAGS@ @NEMIVERPROFPERSP_CFLAGS@ \
-DENABLE_NLS=1 -DDATADIR=\"${datadir}\" \
-I$(top_srcdir)/src -I$(top_srcdir)/src/confmgr
