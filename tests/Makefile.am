h=$(abs_srcdir)
hb=$(abs_builddir)

if AUTOTESTS
# 'make check' automatically runs programs listed in the TESTS variable

#runtestoverloads
#runtestglobalvariables
#runtestenv

TESTS=\
runtestgdbmi runtestunicode \
runtestvars runtestcpptrait runtestvarlist \
runtestvarwalker runtestvarobjwalker runtestbreakpoint \
runtestwatchpoint runtestderef \
runtestlocalvarslist runtestcpplexer \
runtestcppparser runtestvarpathexpr \
runtestlibtoolwrapperdetection \
runtesttypes runtestdisassemble \
runtestvariableformat runtestprettyprint \
runtestthreads runtestdbgstates

else

TESTS=

endif

noinst_PROGRAMS= \
$(TESTS) \
runtestcore  runteststdout  docore inout \
pointerderef fooprog localsinmiddle templatedvar \
gtkmmtest dostackoverflow bigvar threads \
forkparent forkchild prettyprint

runtestgdbmi_SOURCES=$(h)/test-gdbmi.cc
runtestgdbmi_LDADD= @NEMIVERCOMMON_LIBS@ \
@BOOST_UNIT_TEST_FRAMEWORK_STATIC_LIB@ \
$(top_builddir)/src/common/libnemivercommon.la \
$(top_builddir)/src/dbgengine/libgdbmiparser.la \
$(top_builddir)/src/dbgengine/libdebuggerutils.la

gtkmmtest_SOURCES=$(h)/gtkmm-test.cc
gtkmmtest_CXXFLAGS= @NEMIVERUICOMMON_CFLAGS@
gtkmmtest_LDADD= @NEMIVERUICOMMON_LIBS@

fooprog_SOURCES=$(h)/fooprog.cc
fooprog_LDADD=

localsinmiddle_SOURCES=$(h)/locals-in-middle.cc
localsinmiddle_LDADD=

forkparent_SOURCES=$(h)/fork-parent.cc
forkparent_LDADD=

forkchild_SOURCES=$(h)/fork-child.cc
forkchild_LDADD=

prettyprint_SOURCES=$(h)/pretty-print.cc
prettyprint_LDADD=

runtestdbgstates_SOURCES=$(h)/test-dbg-states.cc
runtestdbgstates_LDADD=@NEMIVERCOMMON_LIBS@ \
$(top_builddir)/src/common/libnemivercommon.la \
$(top_builddir)/src/dbgengine/libdebuggerutils.la

runtestbreakpoint_SOURCES=$(h)/test-breakpoint.cc
runtestbreakpoint_LDADD=@NEMIVERCOMMON_LIBS@ \
$(top_builddir)/src/common/libnemivercommon.la \
$(top_builddir)/src/dbgengine/libdebuggerutils.la

runtestwatchpoint_SOURCES=$(h)/test-watchpoint.cc
runtestwatchpoint_LDADD=@NEMIVERCOMMON_LIBS@ \
$(top_builddir)/src/common/libnemivercommon.la \
$(top_builddir)/src/dbgengine/libdebuggerutils.la

runtestvarpathexpr_SOURCES=$(h)/test-var-path-expr.cc
runtestvarpathexpr_LDADD=@NEMIVERCOMMON_LIBS@ \
$(top_builddir)/src/common/libnemivercommon.la \
$(top_builddir)/src/dbgengine/libdebuggerutils.la

runtestvarobjwalker_SOURCES=$(h)/test-varobj-walker.cc
runtestvarobjwalker_LDADD=@NEMIVERCOMMON_LIBS@ \
$(top_builddir)/src/common/libnemivercommon.la \
$(top_builddir)/src/dbgengine/libdebuggerutils.la

runtestvariableformat_SOURCES=$(h)/test-variable-format.cc
runtestvariableformat_LDADD=@NEMIVERCOMMON_LIBS@ \
$(top_builddir)/src/common/libnemivercommon.la \
$(top_builddir)/src/dbgengine/libdebuggerutils.la

runtestprettyprint_SOURCES=$(h)/test-pretty-print.cc
runtestprettyprint_LDADD=@NEMIVERCOMMON_LIBS@ \
$(top_builddir)/src/common/libnemivercommon.la \
$(top_builddir)/src/dbgengine/libdebuggerutils.la

runtestthreads_SOURCES=$(h)/test-threads.cc
runtestthreads_LDADD=@NEMIVERCOMMON_LIBS@ \
$(top_builddir)/src/common/libnemivercommon.la \
$(top_builddir)/src/dbgengine/libdebuggerutils.la

#runtestoverloads_SOURCES=$(h)/test-overloads.cc
#runtestoverloads_LDADD=@NEMIVERCOMMON_LIBS@ \
#$(top_builddir)/src/common/libnemivercommon.la

runtestcore_SOURCES=$(h)/test-core.cc
runtestcore_LDADD=@NEMIVERCOMMON_LIBS@ \
$(top_builddir)/src/common/libnemivercommon.la \
$(top_builddir)/src/dbgengine/libdebuggerutils.la

runteststdout_SOURCES=$(h)/test-stdout.cc
runteststdout_LDADD=@NEMIVERCOMMON_LIBS@ \
$(top_builddir)/src/common/libnemivercommon.la

runtestcpptrait_SOURCES=$(h)/test-cpptrait.cc
runtestcpptrait_LDADD=@NEMIVERCOMMON_LIBS@ \
@BOOST_UNIT_TEST_FRAMEWORK_STATIC_LIB@ \
$(top_builddir)/src/common/libnemivercommon.la \
$(top_builddir)/src/dbgengine/libdebuggerutils.la

runtestunicode_SOURCES=$(h)/test-unicode.cc
runtestunicode_LDADD=@NEMIVERCOMMON_LIBS@ \
@BOOST_UNIT_TEST_FRAMEWORK_STATIC_LIB@ \
$(top_builddir)/src/common/libnemivercommon.la

runtestvars_SOURCES=$(h)/test-vars.cc
runtestvars_LDADD=@NEMIVERCOMMON_LIBS@ \
$(top_builddir)/src/common/libnemivercommon.la \
$(top_builddir)/src/dbgengine/libdebuggerutils.la

runtestvarlist_SOURCES=$(h)/test-var-list.cc
runtestvarlist_LDADD=@NEMIVERCOMMON_LIBS@  \
$(top_builddir)/src/common/libnemivercommon.la \
$(top_builddir)/src/dbgengine/libdebuggerutils.la

runtestvarwalker_SOURCES=$(h)/test-var-walker.cc
runtestvarwalker_LDADD=@NEMIVERCOMMON_LIBS@ \
$(top_builddir)/src/common/libnemivercommon.la \
$(top_builddir)/src/dbgengine/libdebuggerutils.la

runtestderef_SOURCES=$(h)/test-deref.cc
runtestderef_LDADD=@NEMIVERCOMMON_LIBS@ \
$(top_builddir)/src/common/libnemivercommon.la \
$(top_builddir)/src/dbgengine/libdebuggerutils.la

runtestlocalvarslist_SOURCES=$(h)/test-local-vars-list.cc
runtestlocalvarslist_LDADD=@NEMIVERCOMMON_LIBS@  \
$(top_builddir)/src/common/libnemivercommon.la \
$(top_builddir)/src/dbgengine/libdebuggerutils.la

#runtestglobalvariables_SOURCES=$(h)/test-global-variables.cc
#runtestglobalvariables_LDADD=@NEMIVERCOMMON_LIBS@ \
#$(top_builddir)/src/common/libnemivercommon.la

runtestcpplexer_SOURCES=$(h)/test-cpp-lexer.cc
runtestcpplexer_LDADD=@NEMIVERCOMMON_LIBS@ \
@BOOST_UNIT_TEST_FRAMEWORK_STATIC_LIB@ \
$(top_builddir)/src/langs/libnemivercparser.la \
$(top_builddir)/src/common/libnemivercommon.la

runtestcppparser_SOURCES=$(h)/test-cpp-parser.cc
runtestcppparser_LDADD=@NEMIVERCOMMON_LIBS@ \
@BOOST_UNIT_TEST_FRAMEWORK_STATIC_LIB@ \
$(top_builddir)/src/langs/libnemivercparser.la \
$(top_builddir)/src/common/libnemivercommon.la

runtestlibtoolwrapperdetection_SOURCES=$(h)/test-libtool-wrapper-detection.cc
runtestlibtoolwrapperdetection_LDADD=@NEMIVERCOMMON_LIBS@ \
@BOOST_UNIT_TEST_FRAMEWORK_STATIC_LIB@ \
$(top_builddir)/src/langs/libnemivercparser.la \
$(top_builddir)/src/common/libnemivercommon.la

#runtestenv_SOURCES=$(h)/test-env.cc
#runtestenv_LDADD=@NEMIVERCOMMON_LIBS@ \
#@BOOST_UNIT_TEST_FRAMEWORK_STATIC_LIB@ \
#$(top_builddir)/src/common/libnemivercommon.la

runtesttypes_SOURCES=$(h)/test-types.cc
runtesttypes_LDADD=@NEMIVERCOMMON_LIBS@ \
$(top_builddir)/src/common/libnemivercommon.la \
$(top_builddir)/src/dbgengine/libdebuggerutils.la

runtestdisassemble_SOURCES=$(h)/test-disassemble.cc
runtestdisassemble_LDADD=@NEMIVERCOMMON_LIBS@ \
$(top_builddir)/src/common/libnemivercommon.la \
$(top_builddir)/src/dbgengine/libdebuggerutils.la

docore_SOURCES=$(h)/do-core.cc
docore_LDADD=@NEMIVERCOMMON_LIBS@

inout_SOURCES=$(h)/inout.cc
inout_LDADD=@NEMIVERCOMMON_LIBS@ \
$(top_builddir)/src/common/libnemivercommon.la

pointerderef_SOURCES=$(h)/pointer-deref.cc
pointerderef_LDADD=@NEMIVERCOMMON_LIBS@ \
$(top_builddir)/src/common/libnemivercommon.la

templatedvar_SOURCES=$(h)/templated-var.cc
templatedvar_LDADD=@NEMIVERCOMMON_LIBS@ \
$(top_builddir)/src/common/libnemivercommon.la

dostackoverflow_SOURCES=$(h)/do-stack-overflow.cc
dostackoverflow_LDADD=@NEMIVERCOMMON_LIBS@

bigvar_SOURCES=$(h)/big-var.c
bigvar_LDADD=@NEMIVERCOMMON_LIBS@

threads_SOURCES=$(h)/threads.cc
threads_LDADD=@NEMIVERCOMMON_LIBS@

AM_CPPFLAGS=-I$(top_srcdir)/src \
-I$(top_srcdir)/src/confmgr \
-I$(top_srcdir)/src/dbgengine \
@NEMIVERCOMMON_CFLAGS@ -DBOOST_TEST_DYN_LINK \
-DNEMIVER_SRCDIR=\"$(h)\" \
-DNEMIVER_BUILDDIR=\"$(hb)\"

LDADD=$(top_builddir)/src/common/libnemivercommon.la

EXTRA_DIST=$(h)/libtool-wrapper-with-dashes
