#include <iostream>
#include <sstream>
#include <map>
#include <string>
#include <boost/test/minimal.hpp>
#include <glibmm.h>
#include "common/nmv-initializer.h"
#include "common/nmv-exception.h"
#include "nmv-i-var-list-walker.h"
#include "nmv-debugger-utils.h"

using namespace nemiver;
using namespace nemiver::common;

Glib::RefPtr<Glib::MainLoop> s_loop =
    Glib::MainLoop::create (Glib::MainContext::get_default ());

static bool s_got_format;
static bool s_got_value;

static void
on_engine_died_signal ()
{
    s_loop->quit ();
}

static void
on_program_finished_signal ()
{
    s_loop->quit ();
}

static void
on_var_format (const IDebugger::VariableSafePtr a_var,
	       IDebuggerSafePtr /*a_debugger*/)
{
    BOOST_REQUIRE (a_var);
    BOOST_REQUIRE (a_var->format () ==
		   IDebugger::Variable::HEXADECIMAL_FORMAT);
    s_got_format = true;
}

static void
on_var_evaluated (const IDebugger::VariableSafePtr a_var,
		  IDebuggerSafePtr /*a_debugger*/)
{
    BOOST_REQUIRE (a_var);
    BOOST_REQUIRE (a_var->format () ==
		   IDebugger::Variable::HEXADECIMAL_FORMAT);
    BOOST_REQUIRE (a_var->value () == "0x12");
    s_got_value = true;
    s_loop->quit ();
}

static void
on_variable_created (const IDebugger::VariableSafePtr a_var,
                     IDebuggerSafePtr a_debugger)
{
    BOOST_REQUIRE (a_var);
    BOOST_REQUIRE (a_debugger);
    a_debugger->step_over ();
    a_debugger->set_variable_format
      (a_var, IDebugger::Variable::HEXADECIMAL_FORMAT);
    a_debugger->query_variable_format (a_var,
                                       sigc::bind (&on_var_format,
                                                   a_debugger));
    a_debugger->evaluate_variable_expr (a_var,
					sigc::bind
					(&on_var_evaluated,
					 a_debugger));
}

static void
on_stopped_signal (IDebugger::StopReason a_reason,
                   bool /*a_has_frame*/,
                   const IDebugger::Frame &a_frame,
                   int /*a_thread_id*/,
                   int /*bp num*/,
                   const UString &/*a_cookie*/,
                   IDebuggerSafePtr &a_debugger)
{
    NEMIVER_TRY

    BOOST_REQUIRE (a_debugger);

    if (a_reason == IDebugger::BREAKPOINT_HIT) {
        if (a_frame.function_name () == "main") {
            MESSAGE ("broke in main, setting breakoint "
		     "into func1_1");
            a_debugger->set_breakpoint ("func1_1");
            a_debugger->do_continue ();
        } else if (a_frame.function_name () == "func1_1") {
            MESSAGE ("broke in func1_1");
            a_debugger->create_variable ("i_i",
                                         sigc::bind
					 (&on_variable_created,
					  a_debugger));
        }
    } else {
        a_debugger->do_continue ();
    }

    NEMIVER_CATCH_NOX
}

NEMIVER_API int
test_main (int, char **)
{
    NEMIVER_TRY;

    Initializer::do_init ();

    //load the IDebugger interface
    IDebuggerSafePtr debugger =
        debugger_utils::load_debugger_iface_with_confmgr ();

    //setup the debugger with the glib mainloop
    debugger->set_event_loop_context (Glib::MainContext::get_default ());

    //*******************************
    //<connect to IDebugger events>
    //******************************
    debugger->engine_died_signal ().connect (&on_engine_died_signal);
    debugger->program_finished_signal ().connect (&on_program_finished_signal);
    debugger->stopped_signal ().connect (sigc::bind (&on_stopped_signal,
                                                     debugger));
    //*******************************
    //</connect to IDebugger events>
    //******************************

    vector<UString> args;
    debugger->load_program ("fooprog", args, ".");
    debugger->set_breakpoint ("main");

    debugger->run ();

    //****************************************
    //run the event loop.
    //****************************************
    s_loop->run ();

    NEMIVER_CATCH_AND_RETURN_NOX (-1);

    BOOST_REQUIRE (s_got_format);

    return 0;
}
