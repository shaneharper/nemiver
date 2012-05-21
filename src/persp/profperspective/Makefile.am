h=$(abs_srcdir)

plugin_config_files= \
plugin-descriptor.xml \
profperspective.conf

EXTRA_DIST=$(plugin_config_files)

pluginconfig_DATA=$(plugin_config_files)

PLUGIN_NAME=profperspective
pluginconfigdir=@NEMIVER_PLUGINS_DIR@/$(PLUGIN_NAME)

plugin_LTLIBRARIES=libprofperspectiveplugin.la
plugindir=@NEMIVER_PLUGINS_DIR@/$(PLUGIN_NAME)
sources= \
$(h)/nmv-prof-perspective.cc \
$(h)/nmv-prof-perspective.h

libprofperspectiveplugin_la_SOURCES=$(sources)
libprofperspectiveplugin_la_LDFLAGS= -module -avoid-version -Wl,--as-needed
libprofperspectiveplugin_la_LIBADD= \
@NEMIVERPROFPERSP_LIBS@ \
$(abs_top_builddir)/src/common/libnemivercommon.la \
$(abs_top_builddir)/src/uicommon/libnemiveruicommon.la

INCLUDES=@NEMIVERPROFPERSP_CFLAGS@ -DENABLE_NLS=1 -DDATADIR=\"${datadir}\" \
-I$(abs_top_srcdir)/src \
-I$(abs_top_srcdir)/src/confmgr \
-I$(abs_top_srcdir)/src/uicommon \
-I$(abs_top_srcdir)/src/workbench \
-I$(abs_top_srcdir)/src/persp \
-I$(abs_top_srcdir)/src/profperspective

