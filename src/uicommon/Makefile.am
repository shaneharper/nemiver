noinst_LTLIBRARIES=libnemiveruicommon.la

h=$(abs_srcdir)

sources = \
$(public_headers) \
$(h)/nmv-ui-utils.h \
$(h)/nmv-ui-utils.cc \
$(h)/nmv-dialog.cc \
$(h)/nmv-dialog.h \
$(h)/nmv-locate-file-dialog.cc \
$(h)/nmv-locate-file-dialog.h \
$(h)/nmv-spinner-tool-item.h \
$(h)/nmv-spinner-tool-item.cc \
$(h)/nmv-popup-tip.cc \
$(h)/nmv-popup-tip.h \
$(h)/nmv-terminal.cc \
$(h)/nmv-terminal.h \
$(h)/nmv-source-editor.cc \
$(h)/nmv-source-editor.h \
$(h)/nmv-layout.h \
$(h)/nmv-layout-manager.cc \
$(h)/nmv-layout-manager.h \
$(h)/nmv-layout-selector.cc \
$(h)/nmv-layout-selector.h

if BUILD_MEMORYVIEW
memoryview_sources = \
$(h)/nmv-hex-editor.h \
$(h)/nmv-hex-editor.cc \
$(h)/nmv-hex-document.h \
$(h)/nmv-hex-document.cc
else
memoryview_sources =
endif

libnemiveruicommon_la_SOURCES= $(sources) $(memoryview_sources)
libnemiveruicommon_la_LIBADD= @NEMIVERUICOMMON_LIBS@ -lutil
libnemiveruicommon_la_CFLAGS= -fPIC -DPIC

INCLUDES=@NEMIVERUICOMMON_CFLAGS@ \
-I$(abs_top_srcdir) \
-I$(abs_top_srcdir)/src \
-I$(abs_top_srcdir)/src/confmgr \
-I$(abs_top_srcdir)/src/uicommon \
-I$(abs_top_srcdir)/src/workbench
