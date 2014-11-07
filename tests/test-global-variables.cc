#include <iostream>
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

typedef std::list<IDebugger::VariableSafePtr> DebuggerVariableList;
typedef std::map<std::string, IVarListWalkerSafePtr> VarListWalkerMap;
typedef std::map<std::string, string> StringMap;
typedef std::map<UString, UString> UStringMap;

Glib::RefPtr<Glib::MainLoop> s_loop =
    Glib::MainLoop::create (Glib::MainContext::get_default ());

static IDebugger::Frame s_current_frame;
static int s_nb_listed_vars;
static int s_nb_visited_vars;
static UString s_last_listed_var;
static UStringMap s_listed_vars_map;

static const int NB_VARIABLES_TO_LIST=24;
static const int NB_VARIABLES_TO_VISIT=14;

void
on_command_done_signal (const UString &a_name,
                        const UString & /*a_cookie*/)
{
    MESSAGE ("command " << a_name << " done");
}

void
on_engine_died_signal ()
{
}

void
on_program_finished_signal ()
{
}

void
on_running_signal ()
{
}

void
on_stopped_signal (const UString &a_reason,
                   bool a_has_frame,
                   const IDebugger::Frame &a_frame,
                   int /*a_thread_id*/,
                   const UString & /*a_cookie*/,
                   IDebuggerSafePtr &a_debugger)
{
    BOOST_REQUIRE (a_debugger);

    if (a_reason == "exited-normally") {
        MESSAGE ("program exited normally");
        s_loop->quit ();
        return;
    }

    if (!a_has_frame) {
        MESSAGE ("stopped, but not in a frame, reason: " << a_reason);
        return;
    }

    s_current_frame = a_frame;

    MESSAGE ("stopped in function: '"
             << a_frame.function_name ()
             << "()'");

    a_debugger->list_global_variables ();
}

void
on_variable_visited_signal (const IVarWalkerSafePtr a_walker,
                            IDebuggerSafePtr a_debugger)
{
    NEMIVER_TRY

    THROW_IF_FAIL (a_walker);
    THROW_IF_FAIL (a_walker->get_variable ());
    THROW_IF_FAIL (a_debugger);

    cout << "=====================================================================\n";
    cout << "visited-var-name: " << a_walker->get_variable ()->name () << "\n"
         << "visited-var-type: " << a_walker->get_variable ()->type () << "\n";
    cout << "=====================================================================\n";

    s_nb_visited_vars++;

    cout << "visited " << s_nb_visited_vars << " variables \n";

    s_listed_vars_map.erase (a_walker->get_variable ()->name ());

    if (s_nb_visited_vars == NB_VARIABLES_TO_VISIT) {
        cout << "this was the last variable to visit. Continuing the program ...\n";
        a_debugger->do_continue ();
    }

    NEMIVER_CATCH_NOX

}

void
on_variable_list_visited_signal (IDebuggerSafePtr a_debugger)
{
    NEMIVER_TRY

    THROW_IF_FAIL (a_debugger);
    cout << "finished visiting variables list\n";
    a_debugger->do_continue ();

    NEMIVER_CATCH_NOX
}

IVarListWalkerSafePtr
create_var_list_walker (IDebuggerSafePtr a_debugger)
{
    THROW_IF_FAIL (a_debugger);

    IVarListWalkerSafePtr result  =
        DynamicModuleManager::load_iface_with_default_manager<IVarListWalker>
                                                                ("varlistwalker",
                                                                 "IVarListWalker");
    result->initialize (a_debugger);
    result->variable_visited_signal ().connect
        (sigc::bind (&on_variable_visited_signal, a_debugger));
    result->variable_list_visited_signal ().connect
        (sigc::bind (&on_variable_list_visited_signal, a_debugger));
    return result;
}

IVarListWalkerSafePtr
get_var_list_walker (IDebuggerSafePtr a_debugger)
{
    static IVarListWalkerSafePtr s_walker;

    if (!s_walker) {
        s_walker = create_var_list_walker (a_debugger);
    }
    return s_walker;
}

void
on_global_variables_listed_signal (const std::list<IDebugger::VariableSafePtr> &a_vars,
                                   const UString & /*a_cookie*/,
                                   IDebuggerSafePtr &a_debugger)
{
    NEMIVER_TRY

    THROW_IF_FAIL (a_debugger);

    if (a_vars.size () == 0) {
         cout << "got zero global variables\n";
        return;
    }
    cout << "got global variables\n";
    IVarListWalkerSafePtr walker = get_var_list_walker (a_debugger);
    std::list<IDebugger::VariableSafePtr>::const_iterator it;
    for (it = a_vars.begin (); it != a_vars.end (); ++it) {
        cout << "------------------------------\n";
        cout << "listed-var-name: " << (*it)->name () << "\n";
        cout << "listed-var-type: " << (*it)->type () << "\n";
        cout << "------------------------------\n";
        walker->append_variable (*it);
        s_nb_listed_vars++;
        s_listed_vars_map[(*it)->name ()] = "";
        s_last_listed_var = (*it)->name ();
    }

    walker->do_walk_variables ();


    NEMIVER_CATCH_NOX
}


NEMIVER_API int
test_main (int /*argc*/, char ** /*argv*/)
{
    NEMIVER_TRY

    Initializer::do_init ();

    //load the IDebugger interface
    IDebuggerSafePtr debugger =
        debugger_utils::load_debugger_iface_with_confmgr ();

    //setup the debugger with the glib mainloop
    debugger->set_event_loop_context (Glib::MainContext::get_default ());

    //*******************************
    //<connect to IDebugger events>
    //******************************
    debugger->command_done_signal ().connect (&on_command_done_signal);
    debugger->engine_died_signal ().connect (&on_engine_died_signal);
    debugger->program_finished_signal ().connect (&on_program_finished_signal);
    debugger->running_signal ().connect (&on_running_signal);
    debugger->stopped_signal ().connect (sigc::bind (&on_stopped_signal,
                                                     debugger));
    debugger->global_variables_listed_signal ().connect
                            (sigc::bind (&on_global_variables_listed_signal, debugger));
    //*******************************
    //</connect to IDebugger events>
    //******************************

    debugger->load_program ("fooprog", ".");
    debugger->set_breakpoint ("main");
    debugger->run ();

    //********************
    //run the event loop.
    //********************
    s_loop->run ();

    cout << "variables listed: " << s_nb_listed_vars << "\n";
    cout << "variables visited: " << s_nb_visited_vars << "\n";

    cout << "======variables listed but not visited:=========\n";
    UStringMap::const_iterator it;
    for (it = s_listed_vars_map.begin (); it != s_listed_vars_map.end (); ++it) {
        cout << "name: " << it->first << "\n";
    }
    cout << "========end of variables listed but not visited========\n";

    if (s_nb_listed_vars != NB_VARIABLES_TO_LIST) {
        BOOST_FAIL ("was expecting to list " << NB_VARIABLES_TO_LIST
                   << " variables but listed " << s_nb_listed_vars << " instead");
    }
    if (s_nb_visited_vars != NB_VARIABLES_TO_VISIT) {
        BOOST_FAIL ("was expecting to visit " << NB_VARIABLES_TO_VISIT
                    << " variables but visited " << s_nb_visited_vars << " instead");
    }

    NEMIVER_CATCH_AND_RETURN_NOX (-1)

    return 0;
}


