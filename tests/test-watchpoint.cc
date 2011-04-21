#include <iostream>
#include <boost/test/minimal.hpp>
#include <glibmm.h>
#include "common/nmv-initializer.h"
#include "common/nmv-safe-ptr-utils.h"
#include "nmv-i-debugger.h"
#include "nmv-debugger-utils.h"

using namespace nemiver;
using namespace nemiver::common;

Glib::RefPtr<Glib::MainLoop> loop =
    Glib::MainLoop::create (Glib::MainContext::get_default ());

static int nb_watchpoint_trigger;
static int nb_watchpoint_out_of_scope;

void
on_engine_died_signal ()
{
    MESSAGE ("engine died");
    loop->quit ();
}

void
on_program_finished_signal ()
{
    MESSAGE ("program finished");
    loop->quit ();
}

void
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

void
on_stopped_signal (IDebugger::StopReason a_reason,
                   bool a_has_frame,
                   const IDebugger::Frame &a_frame,
                   int /*a_thread_id*/,
                   int /*a_bp_num*/,
                   const UString &/*a_cookie*/,
                   IDebuggerSafePtr &a_debugger)
{
    MESSAGE ("stopped in: "
             << a_frame.function_name ());

    if (a_reason == IDebugger::BREAKPOINT_HIT
        && a_has_frame
        && a_frame.function_name () == "func1") {
        a_debugger->set_watchpoint ("i");
    }
    if (a_reason == IDebugger::WATCHPOINT_TRIGGER) {
        MESSAGE ("watchpoint triggered");
        ++nb_watchpoint_trigger;
    }
    if (a_reason == IDebugger::WATCHPOINT_SCOPE) {
        MESSAGE ("watchpoint gone out of scope");
        ++nb_watchpoint_out_of_scope;
    }
    a_debugger->do_continue ();
}

NEMIVER_API int
test_main (int, char **)
{
    NEMIVER_TRY

    Initializer::do_init ();

    THROW_IF_FAIL (loop);

    IDebuggerSafePtr debugger =
        debugger_utils::load_debugger_iface_with_gconf ();

    debugger->set_event_loop_context (loop->get_context ());

    //*****************************
    //<connect to IDebugger events>
    //*****************************
    debugger->engine_died_signal ().connect (&on_engine_died_signal);

    debugger->program_finished_signal ().connect
                                            (&on_program_finished_signal);

    debugger->breakpoints_list_signal ().connect
        (&on_breakpoints_set_signal);

    debugger->stopped_signal ().connect
                            (sigc::bind (&on_stopped_signal, debugger));

    std::vector<UString> args, source_search_dir;
    source_search_dir.push_back (".");
    debugger->load_program ("fooprog", args, ".",
                            source_search_dir, "", false);
    debugger->set_breakpoint ("main");
    debugger->set_breakpoint ("func1");
    debugger->run ();

    loop->run ();

    NEMIVER_CATCH_NOX

    BOOST_REQUIRE (nb_watchpoint_trigger == 1);
    BOOST_REQUIRE (nb_watchpoint_out_of_scope == 1);
    return 0;
}

