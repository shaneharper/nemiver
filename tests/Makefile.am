if AUTOTESTS
# 'make check' automatically runs programs listed in the TESTS variable


#runtestoverloads
#runtestglobalvariables
TESTS=\
runtestgdbmi runtestunicode \
runtestcpptrait runtestvarlist \
runtestvarwalker runtestbreakpoint \
runtestderef \
runtestlocalvarslist runtestcpplexer \
runtestcppparser  \
runtestlibtoolwrapperdetection \
runtestenv


else

TESTS=

endif

noinst_PROGRAMS= \
$(TESTS) \
runtestcore \
runteststdout \
docore inout \
pointerderef \
fooprog \
templatedvar \
gtkmm-test

runtestgdbmi_SOURCES=test-gdbmi.cc
runtestgdbmi_LDADD= @NEMIVERCOMMON_LIBS@ \
@BOOST_UNIT_TEST_FRAMEWORK_STATIC_LIB@ \
$(top_builddir)/src/common/libnemivercommon.la \
$(top_builddir)/src/dbgengine/libgdbmiparser.la

gtkmm_test_SOURCES=gtkmm-test.cc
gtkmm_test_CXXFLAGS= @LIBGTKMM_CFLAGS@
gtkmm_test_LDADD= @LIBGTKMM_LIBS@

fooprog_SOURCES=fooprog.cc
fooprog_LDADD=@NEMIVERCOMMON_LIBS@ \
$(top_builddir)/src/common/libnemivercommon.la

runtestbreakpoint_SOURCES=test-breakpoint.cc
runtestbreakpoint_LDADD=@NEMIVERCOMMON_LIBS@ @BOOST_TEST_EXEC_MONITOR_LIB@ \
$(top_builddir)/src/common/libnemivercommon.la

runtestoverloads_SOURCES=test-overloads.cc
runtestoverloads_LDADD=@NEMIVERCOMMON_LIBS@ @BOOST_TEST_EXEC_MONITOR_LIB@ \
$(top_builddir)/src/common/libnemivercommon.la

runtestcore_SOURCES=test-core.cc
runtestcore_LDADD=@NEMIVERCOMMON_LIBS@ \
$(top_builddir)/src/common/libnemivercommon.la

runteststdout_SOURCES=test-stdout.cc
runteststdout_LDADD=@NEMIVERCOMMON_LIBS@ \
$(top_builddir)/src/common/libnemivercommon.la

runtestcpptrait_SOURCES=test-cpptrait.cc
runtestcpptrait_LDADD=@NEMIVERCOMMON_LIBS@ \
@BOOST_UNIT_TEST_FRAMEWORK_STATIC_LIB@ \
$(top_builddir)/src/common/libnemivercommon.la


runtestunicode_SOURCES=test-unicode.cc
runtestunicode_LDADD=@NEMIVERCOMMON_LIBS@ \
@BOOST_UNIT_TEST_FRAMEWORK_STATIC_LIB@ \
$(top_builddir)/src/common/libnemivercommon.la

runtestvarlist_SOURCES=test-var-list.cc
runtestvarlist_LDADD=@NEMIVERCOMMON_LIBS@ @BOOST_TEST_EXEC_MONITOR_LIB@ \
$(top_builddir)/src/common/libnemivercommon.la

runtestvarwalker_SOURCES=test-var-walker.cc
runtestvarwalker_LDADD=@NEMIVERCOMMON_LIBS@ @BOOST_TEST_EXEC_MONITOR_LIB@ \
$(top_builddir)/src/common/libnemivercommon.la

runtestderef_SOURCES=test-deref.cc
runtestderef_LDADD=@NEMIVERCOMMON_LIBS@ @BOOST_TEST_EXEC_MONITOR_LIB@ \
$(top_builddir)/src/common/libnemivercommon.la

runtestlocalvarslist_SOURCES=test-local-vars-list.cc
runtestlocalvarslist_LDADD=@NEMIVERCOMMON_LIBS@ @BOOST_TEST_EXEC_MONITOR_LIB@ \
$(top_builddir)/src/common/libnemivercommon.la

runtestglobalvariables_SOURCES=test-global-variables.cc
runtestglobalvariables_LDADD=@NEMIVERCOMMON_LIBS@ @BOOST_TEST_EXEC_MONITOR_LIB@ \
$(top_builddir)/src/common/libnemivercommon.la

runtestcpplexer_SOURCES=test-cpp-lexer.cc
runtestcpplexer_LDADD=@NEMIVERCOMMON_LIBS@ \
@BOOST_UNIT_TEST_FRAMEWORK_STATIC_LIB@ \
$(top_builddir)/src/langs/libnemivercparser.la \
$(top_builddir)/src/common/libnemivercommon.la

runtestcppparser_SOURCES=test-cpp-parser.cc
runtestcppparser_LDADD=@NEMIVERCOMMON_LIBS@ \
@BOOST_UNIT_TEST_FRAMEWORK_STATIC_LIB@ \
$(top_builddir)/src/langs/libnemivercparser.la \
$(top_builddir)/src/common/libnemivercommon.la

runtestlibtoolwrapperdetection_SOURCES=test-libtool-wrapper-detection.cc
runtestlibtoolwrapperdetection_LDADD=@NEMIVERCOMMON_LIBS@ \
@BOOST_UNIT_TEST_FRAMEWORK_STATIC_LIB@ \
$(top_builddir)/src/langs/libnemivercparser.la \
$(top_builddir)/src/common/libnemivercommon.la

runtestenv_SOURCES=test-env.cc
runtestenv_LDADD=@NEMIVERCOMMON_LIBS@ \
@BOOST_UNIT_TEST_FRAMEWORK_STATIC_LIB@ \
$(top_builddir)/src/common/libnemivercommon.la

docore_SOURCES=do-core.cc
docore_LDADD=@NEMIVERCOMMON_LIBS@ \
$(top_builddir)/src/common/libnemivercommon.la

inout_SOURCES=inout.cc
inout_LDADD=@NEMIVERCOMMON_LIBS@ \
$(top_builddir)/src/common/libnemivercommon.la

pointerderef_SOURCES=pointer-deref.cc
pointerderef_LDADD=@NEMIVERCOMMON_LIBS@ \
$(top_builddir)/src/common/libnemivercommon.la

templatedvar_SOURCES=templated-var.cc
templatedvar_LDADD=@NEMIVERCOMMON_LIBS@ \
$(top_builddir)/src/common/libnemivercommon.la


INCLUDES=-I$(top_srcdir)/src \
-I$(top_srcdir)/src/confmgr \
-I$(top_srcdir)/src/dbgengine \
@NEMIVERCOMMON_CFLAGS@ -DBOOST_TEST_DYN_LINK

LDADD=$(top_builddir)/src/common/libnemivercommon.la

