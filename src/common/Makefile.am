nemivercommon_LTLIBRARIES=libnemivercommon.la
nemivercommondir=$(libdir)/nemiver

headers= \
nmv-api-macros.h \
nmv-namespace.h \
nmv-ustring.h \
nmv-libxml-utils.h \
nmv-safe-ptr-utils.h \
nmv-object.h \
nmv-safe-ptr.h \
nmv-log-stream.h \
nmv-scope-logger.h \
nmv-exception.h \
nmv-initializer.h \
nmv-dynamic-module.h \
nmv-log-stream-utils.h \
nmv-date-utils.h \
nmv-i-input-stream.h \
nmv-env.h \
nmv-plugin.h \
nmv-sequence.h \
nmv-option-utils.h \
nmv-i-connection-driver.h \
nmv-connection.h \
nmv-i-connection-manager-driver.h \
nmv-connection-manager.h \
nmv-parsing-utils.h  \
nmv-conf-manager.h \
nmv-buffer.h \
nmv-tools.h \
nmv-transaction.h \
nmv-sql-statement.h \
nmv-insert-statement.h \
nmv-delete-statement.h \
nmv-proc-utils.h \
nmv-proc-mgr.h

libnemivercommon_la_SOURCES= $(headers) \
nmv-ustring.cc \
nmv-object.cc \
nmv-libxml-utils.cc \
nmv-log-stream.cc \
nmv-scope-logger.cc \
nmv-exception.cc \
nmv-initializer.cc \
nmv-dynamic-module.cc \
nmv-date-utils.cc \
nmv-env.cc \
nmv-plugin.cc \
nmv-sequence.cc \
nmv-option-utils.cc \
nmv-connection-manager.cc \
nmv-connection.cc \
nmv-parsing-utils.cc \
nmv-conf-manager.cc \
nmv-tools.cc \
nmv-transaction.cc \
nmv-sql-statement.cc \
nmv-insert-statement.cc \
nmv-delete-statement.cc \
nmv-proc-utils.cc \
nmv-proc-mgr.cc

publicheaders_DATA=$(headers)
publicheadersdir=$(NEMIVER_INCLUDE_DIR)/common
libnemivercommon_la_LDFLAGS=@NEMIVERCOMMON_LIBS@ -lutil \
		-avoid-version -rpath $(libdir)/nemiver \
		 -Wl,--as-needed

#don't install libnemivercommon.pc yet, as no other program
#but nemiver uses the libnemivercommon library yet.
#pkgconfigdir = $(libdir)/pkgconfig
#pkgconfig_DATA = libnemivercommon.pc
#EXTRA_DIST = $(pkgconfig_DATA)

INCLUDES=@NEMIVERCOMMON_CFLAGS@ \
-DENABLE_NLS=1 -DDATADIR=\"${datadir}\" \
-DSYSTEM_LIBDIR=\"/usr/lib\" \
-DNEMIVER_INSTALL_PREFIX=\"@NEMIVER_INSTALL_PREFIX@\" \
-DNEMIVER_SYSTEM_CONFIG_DIR=\"@NEMIVER_SYSTEM_CONFIG_DIR@\" \
-DNEMIVER_MODULES_DIR=\"@NEMIVER_MODULES_DIR@\" \
-DNEMIVER_PLUGINS_DIR=\"@NEMIVER_PLUGINS_DIR@\" \
-DNEMIVER_VERSION=\"@NEMIVER_VERSION@\" \
-DGDB_PROG=\"@GDB_PROG@\"

