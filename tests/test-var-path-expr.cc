#include "config.h"
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

typedef std::list<IDebugger::VariableSafePtr> VariablesList;

static int num_variables_created;

// This container holds variables backed up by backend-side
// variable objects created during this test, so that they stay
// alive during the life of this test.
VariablesList variables;

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

/// Counts the number of deleted variables.  If it equals the number
/// of created variables, then exit the event loop, effectively
/// allowing the test to exit.
static void
on_variable_deleted_signal (const IDebugger::VariableSafePtr a_var,
                            const UString&)
{
    BOOST_REQUIRE (!a_var);
    MESSAGE ("a backend-side variable object got deleted!");

    static int num_variables_deleted;
    num_variables_deleted++;

    if (num_variables_deleted == num_variables_created)
        loop->quit ();
}

static void
on_breakpoints_set_signal (const std::map<string, IDebugger::Breakpoint> &a_breaks,
                           const UString & /*a_cookie*/)
{
    MESSAGE ("breakpoints set:");
    std::map<string, IDebugger::Breakpoint>::const_iterator it;
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
    BOOST_REQUIRE (a_var->path_expression ().find (".m_first_name")
                   != UString::npos);

    // This should help delete all the variables (and their
    // backend-side variable objects) created during this test, along
    // with a_var, after this function returns.
    variables.clear ();
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
    // Ensure that a_var lives throughout the test.
    variables.push_back (a_var);
}

static void
on_variable_created (const IDebugger::VariableSafePtr a_var,
                     IDebuggerSafePtr a_debugger)
{
    MESSAGE ("variable created: " << a_var->name ());

    // Add a_var to the list of live variables, so that it remains
    // alive during the whole life of the main function.
    variables.push_back (a_var);
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
                   const string &/*a_bp_num*/,
                   const UString &/*a_cookie*/,
                   IDebuggerSafePtr a_debugger)
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
        ++num_variables_created;
    } else {
        a_debugger->do_continue ();
    }
}

NEMIVER_API int
test_main (int, char **)
{
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

    debugger->variable_deleted_signal ().connect (&on_variable_deleted_signal);

    debugger->breakpoints_list_signal ().connect
                                            (&on_breakpoints_set_signal);

    debugger->stopped_signal ().connect (sigc::bind (&on_stopped_signal,
                                                     debugger));

    std::vector<UString> args, source_search_dir;
    debugger->enable_pretty_printing (false);
    source_search_dir.push_back (".");
    debugger->load_program ("fooprog", args, ".",
                            source_search_dir, "", false);
    debugger->set_breakpoint ("main");
    debugger->run ();
    loop->run ();

    NEMIVER_CATCH_NOX

    return 0;
}
