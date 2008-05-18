noinst_LTLIBRARIES=libnemiveruicommon.la

sources = \
$(public_headers) \
nmv-ui-utils.h \
nmv-ui-utils.cc \
ephy-spinner.c \
ephy-spinner.h \
ephy-spinner-tool-item.c \
ephy-spinner-tool-item.h \
nmv-spinner.cc \
nmv-spinner.h \
nmv-spinner-tool-item.h \
nmv-spinner-tool-item.cc \
nmv-popup-tip.cc \
nmv-popup-tip.h \
nmv-terminal.cc \
nmv-terminal.h \
nmv-source-editor.cc \
nmv-source-editor.h

if BUILD_MEMORYVIEW
memoryview_sources = \
nmv-hex-editor.h \
nmv-hex-editor.cc \
nmv-hex-document.h \
nmv-hex-document.cc
else
memoryview_sources =
endif

libnemiveruicommon_la_SOURCES= $(sources) $(memoryview_sources)
libnemiveruicommon_la_LIBADD= @NEMIVERUICOMMON_LIBS@ -lutil
libnemiveruicommon_la_CFLAGS= -fPIC -DPIC

INCLUDES=@NEMIVERUICOMMON_CFLAGS@ \
-I$(top_srcdir) \
-I$(top_srcdir)/src \
-I$(top_srcdir)/src/uicommon

