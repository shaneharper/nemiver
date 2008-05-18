SUBDIRS=glade menus icons sqlscripts schemas

install-data-local: install-schemas

uninstall-local: uninstall-schemas

install-schemas:
	$(MAKE) -C schemas $(AM_MAKEFLAGS) install-schemas

uninstall-schemas:
	$(MAKE) -C schemas $(AM_MAKEFLAGS)  uninstall-schemas

plugin_config_files= \
plugin-descriptor.xml \
dbgperspective.conf

EXTRA_DIST=$(plugin_config_files)

pluginconfig_DATA=$(plugin_config_files)

PLUGIN_NAME=dbgperspective
pluginconfigdir=@NEMIVER_PLUGINS_DIR@/$(PLUGIN_NAME)

plugin_LTLIBRARIES=libdbgperspectiveplugin.la
plugindir=@NEMIVER_PLUGINS_DIR@/$(PLUGIN_NAME)
sources= \
nmv-dialog.cc \
nmv-dialog.h \
nmv-run-program-dialog.cc \
nmv-run-program-dialog.h \
nmv-locate-file-dialog.cc \
nmv-locate-file-dialog.h \
nmv-load-core-dialog.cc \
nmv-load-core-dialog.h \
nmv-saved-sessions-dialog.h \
nmv-saved-sessions-dialog.cc \
nmv-preferences-dialog.h \
nmv-preferences-dialog.cc \
nmv-proc-list-dialog.cc \
nmv-proc-list-dialog.h \
nmv-var-inspector-dialog.cc \
nmv-var-inspector-dialog.h \
nmv-find-text-dialog.cc \
nmv-find-text-dialog.h \
nmv-sess-mgr.cc \
nmv-sess-mgr.h \
nmv-call-stack.cc \
nmv-call-stack.h \
nmv-variables-utils.cc \
nmv-variables-utils.h \
nmv-variables-utils2.cc \
nmv-variables-utils2.h \
nmv-local-vars-inspector.cc \
nmv-local-vars-inspector.h \
nmv-local-vars-inspector2.cc \
nmv-local-vars-inspector2.h \
nmv-global-vars-inspector-dialog.cc \
nmv-global-vars-inspector-dialog.h \
nmv-var-inspector.cc \
nmv-var-inspector.h \
nmv-var-inspector2.cc \
nmv-var-inspector2.h \
nmv-breakpoints-view.cc \
nmv-breakpoints-view.h \
nmv-registers-view.cc \
nmv-registers-view.h \
nmv-thread-list.h \
nmv-thread-list.cc \
nmv-file-list.cc \
nmv-file-list.h \
nmv-open-file-dialog.cc \
nmv-open-file-dialog.h \
nmv-set-breakpoint-dialog.cc \
nmv-set-breakpoint-dialog.h \
nmv-choose-overloads-dialog.cc \
nmv-choose-overloads-dialog.h \
nmv-remote-target-dialog.cc \
nmv-remote-target-dialog.h \
nmv-dbg-perspective.cc \
nmv-dbg-perspective.h \
nmv-vars-treeview.h \
nmv-vars-treeview.cc

if BUILD_MEMORYVIEW
memoryview_sources = \
					  nmv-memory-view.cc \
					  nmv-memory-view.h
else
memoryview_sources =
endif

libdbgperspectiveplugin_la_SOURCES=$(sources) $(memoryview_sources)
libdbgperspectiveplugin_la_LDFLAGS= -module -avoid-version -Wl,--as-needed
libdbgperspectiveplugin_la_LIBADD= \
@NEMIVERDBGPERSP_LIBS@ \
$(top_builddir)/src/common/libnemivercommon.la \
$(top_builddir)/src/uicommon/libnemiveruicommon.la

INCLUDES=@NEMIVERDBGPERSP_CFLAGS@ -DENABLE_NLS=1 -DDATADIR=\"${datadir}\" \
-I$(top_srcdir)/src \
-I$(top_srcdir)/src/confmgr \
-I$(top_srcdir)/src/dbgengine \
-I$(top_srcdir)/src/uicommon \
-I$(top_srcdir)/src/workbench \
-I$(top_srcdir)/src/persp \
-I$(top_srcdir)/src/dbgperspective

