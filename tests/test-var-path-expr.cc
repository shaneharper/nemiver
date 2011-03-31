#include <iostream>
#include <boost/test/minimal.hpp>
#include <glibmm.h>
#include "common/nmv-initializer.h"
#include "common/nmv-safe-ptr-utils.h"
#include "common/nmv-exception.h"
#include "nmv-i-debugger.h"
#include "nmv-debugger-utils.h"

using namespace nemiver;
using namespace nemiver::common;

Glib::RefPtr<Glib::MainLoop> loop =
    Glib::MainLoop::create (Glib::MainContext::get_default ());

static void
on_engine_died_signal ()
{
    MESSAGE ("engine died");
    loop->quit ();
}

static void
on_program_finished_signal ()
{
    MESSAGE ("program finished");
    loop->quit ();
}

static void
on_breakpoints_set_signal (const std::map<int, IDebugger::Breakpoint> &a_breaks,
                           const UString &a_cookie)
{
    if (a_cookie.empty ()) {}

    MESSAGE ("breakpoints set:");
    std::map<int, IDebugger::Breakpoint>::const_iterator it;
    for (it = a_breaks.begin (); it != a_breaks.end () ; ++it) {
        MESSAGE ("<break><num>" << it->first <<"</num><line>"
                 << it->second.file_name () << ":" << it->second.line ()
                 << "</line></break>");
    }
}

static void
on_variable_expr_path (const IDebugger::VariableSafePtr a_var)
{
    MESSAGE ("var expr path: " << a_var->path_expression ());
    BOOST_REQUIRE (a_var->path_expression ()
                   == "((((person).m_first_name)).npos)");
    loop->quit ();
}

static void
on_variable_unfolded (const IDebugger::VariableSafePtr a_var,
                      IDebuggerSafePtr a_debugger)
{
    MESSAGE ("var unfolded: " << a_var->name ());

    BOOST_REQUIRE (!a_var->members ().empty ());
    if (a_var->members ().front ()->needs_unfolding ()) {
        MESSAGE ("unfolding variable " << a_var->members ().front ()->name ());
        a_debugger->unfold_variable (a_var->members ().front (),
                                     sigc::bind (&on_variable_unfolded,
                                                 a_debugger));
    } else {
        a_debugger->query_variable_path_expr (a_var->members ().front (),
                                              &on_variable_expr_path);
    }
}

static void
on_variable_created (const IDebugger::VariableSafePtr a_var,
                     IDebuggerSafePtr a_debugger)
{
    MESSAGE ("variable created: " << a_var->name ());

    if (a_var->needs_unfolding ()) {
        MESSAGE ("unfolding variable " << a_var->name ());
        a_debugger->unfold_variable (a_var,
                                     sigc::bind (&on_variable_unfolded,
                                                 a_debugger));
    }
}

static void
on_stopped_signal (IDebugger::StopReason a_reason,
                   bool /*a_has_frame*/,
                   const IDebugger::Frame &a_frame,
                   int /*a_thread_id*/,
                   int /*a_bp_num*/,
                   const UString &/*a_cookie*/,
                   IDebuggerSafePtr &a_debugger)
{
    MESSAGE ("stopped at: "
             << a_frame.function_name ()
             << ":"
             << a_frame.line ());

    if (a_reason == IDebugger::BREAKPOINT_HIT) {
        a_debugger->step_over ();
    } else if (a_reason == IDebugger::END_STEPPING_RANGE) {
        a_debugger->create_variable ("person",
                                     sigc::bind (&on_variable_created,
                                                 a_debugger));
    } else {
        a_debugger->do_continue ();
    }
}

NEMIVER_API int
test_main (int argc, char *argv[])
{
    if (argc || argv) {/*keep compiler happy*/}

    NEMIVER_TRY

    Initializer::do_init ();

    THROW_IF_FAIL (loop);

    IDebuggerSafePtr debugger =
        debugger_utils::load_debugger_iface_with_confmgr ();

    debugger->set_event_loop_context (loop->get_context ());

    //*****************************
    //<connect to IDebugger events>
    //*****************************
    debugger->engine_died_signal ().connect (&on_engine_died_signal);

    debugger->program_finished_signal ().connect
                                            (&on_program_finished_signal);

    debugger->breakpoints_list_signal ().connect
                                            (&on_breakpoints_set_signal);

    debugger->stopped_signal ().connect (sigc::bind (&on_stopped_signal,
                                                     debugger));

    std::vector<UString> args, source_search_dir;
    source_search_dir.push_back (".");
    debugger->load_program ("fooprog", args, ".",
                            source_search_dir, "", false);
    debugger->set_breakpoint ("main");
    debugger->run ();
    loop->run ();

    NEMIVER_CATCH_NOX

    return 0;
}

