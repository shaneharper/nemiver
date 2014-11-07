#include "config.h"
#include <iostream>
#include <boost/test/minimal.hpp>
#include <glibmm.h>
#include "common/nmv-initializer.h"
#include "common/nmv-exception.h"
#include "nmv-i-var-list.h"
#include "nmv-debugger-utils.h"

using namespace std;
using namespace nemiver;
using namespace nemiver::common;


Glib::RefPtr<Glib::MainLoop> s_loop =
    Glib::MainLoop::create (Glib::MainContext::get_default ());

IDebugger::Frame s_current_frame;

void lookup_variable (IVarListSafePtr &a_var_list);

void
on_command_done_signal (const UString &a_name,
                        const UString &a_cookie)
{
    if (a_cookie.empty ()) {
    }
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
on_stopped_signal (IDebugger::StopReason a_reason,
                   bool a_has_frame,
                   const IDebugger::Frame &a_frame,
                   int /*a_thread_id*/,
                   const string & /*bp number*/,
                   const UString &/*a_cookie*/,
                   IDebuggerSafePtr &a_debugger)
{
    BOOST_REQUIRE (a_debugger);

    if (a_reason == IDebugger::EXITED_NORMALLY) {
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
    a_debugger->list_local_variables ();
    a_debugger->list_frames_arguments ();

    if (a_frame.function_name () == "main") {
        a_debugger->do_continue();
    } else if (a_frame.function_name () == "func1") {
        a_debugger->do_continue();
    } else if (a_frame.function_name () == "func2") {
        a_debugger->do_continue();
    } else if (a_frame.function_name () == "func3") {
        a_debugger->do_continue();
    }

}

void
on_frames_arguments_listed_signal
        (const map<int, list<IDebugger::VariableSafePtr> > &a_frames_params,
         const UString & /*a_cookie*/,
         IVarListSafePtr &a_var_list)
{
    BOOST_REQUIRE (a_var_list);
    map<int, list<IDebugger::VariableSafePtr> >::const_iterator it;
    it = a_frames_params.find (0);
    if (it == a_frames_params.end ()) {
        LOG_ERROR ("Could not find current frame");
        return;
    }
    a_var_list->remove_variables ();
    a_var_list->append_variables (it->second, false/*don't update type*/);
}

void
on_local_variables_listed_signal (const DebuggerVariableList &a_variables,
                                  const UString &a_cookie,
                                  IVarListSafePtr &a_var_list)
{
    if (a_variables.empty () || a_cookie.empty ()) {
    }
    BOOST_REQUIRE (a_var_list);
    a_var_list->remove_variables ();
    a_var_list->append_variables (a_variables, false/*don't update type*/);
}

void
on_variable_value_signal (const UString & /*a_variable_name*/,
                          const IDebugger::VariableSafePtr & /*a_variable*/,
                          const UString & /*a_cookie*/,
                          const IVarListSafePtr & /*a_var_list*/)
{
}

void
on_variable_type_signal (const UString &a_variable_name,
                         const UString &a_variable_type,
                         const UString &a_cookie,
                         const IVarListSafePtr &a_var_list)
{
    if (a_variable_name.empty () || a_variable_type.empty () ||
        a_cookie.empty () || a_var_list) {
    }
}

void
on_variable_removed_signal (const IDebugger::VariableSafePtr &a_var)
{
    BOOST_REQUIRE (a_var);
    MESSAGE ("variable removed: " << a_var->name ());

    /*
    cout << "=================\n";
    if (a_var) {
        UString str;
        a_var->to_string (str, true);
        cout << str;
    }
    cout << "\n=================" << endl;
    */
}

void
on_variable_added_signal (const IDebugger::VariableSafePtr &a_var,
                          IVarListSafePtr &a_var_list)
{
    BOOST_REQUIRE (a_var);
    BOOST_REQUIRE (a_var_list);

    MESSAGE ("variable added: " << a_var->name ());
    IDebugger::VariableSafePtr variable;
    BOOST_REQUIRE (a_var_list->find_variable (a_var->name (), variable));

    if (s_current_frame.function_name () == "func3") {
        lookup_variable (a_var_list);
    }
}

void
lookup_variable (IVarListSafePtr &a_var_list)
{
    BOOST_REQUIRE (a_var_list);

    if (s_current_frame.function_name () != "func3") {
        return;
    }
    IDebugger::VariableSafePtr variable;
    MESSAGE ("Looking for simple variable ...");
    BOOST_REQUIRE (a_var_list->find_variable ("a_param", variable));
    MESSAGE ("OK");
    MESSAGE ("Looking for fully qualified variable ...");
    BOOST_REQUIRE (a_var_list->find_variable ("a_param.m_first_name",
                                              variable));
    MESSAGE ("OK");
}

NEMIVER_API int
test_main (int /*argc*/, char ** /*argv*/)
{
    NEMIVER_TRY;

    Initializer::do_init ();

    //load the IDebugger interface
    IDebuggerSafePtr debugger =
        debugger_utils::load_debugger_iface_with_confmgr ();

    //setup the debugger with the glib mainloop
    debugger->set_event_loop_context (Glib::MainContext::get_default ());

    //load the variable list interface
    IVarListSafePtr var_list =
        DynamicModuleManager::load_iface_with_default_manager<IVarList>
                                                                ("varlist",
                                                                 "IVarList");
    var_list->initialize (debugger);
    //*****************************
    //<connect to var list signals>
    //*****************************
    var_list->variable_added_signal ().connect
        (sigc::bind (&on_variable_added_signal, var_list));
    var_list->variable_removed_signal ().connect (&on_variable_removed_signal);
    //*****************************
    //</connect to var list signals>
    //*****************************

    //*******************************
    //<connect to IDebugger events>
    //******************************
    debugger->command_done_signal ().connect (&on_command_done_signal);
    debugger->engine_died_signal ().connect (&on_engine_died_signal);
    debugger->program_finished_signal ().connect (&on_program_finished_signal);
    debugger->running_signal ().connect (&on_running_signal);
    debugger->stopped_signal ().connect (sigc::bind (&on_stopped_signal,
                                                     debugger));
    debugger->variable_value_signal ().connect
                        (sigc::bind (&on_variable_value_signal, var_list));
    debugger->variable_type_signal ().connect
                            (sigc::bind (&on_variable_type_signal, var_list));

    debugger->frames_arguments_listed_signal ().connect
                            (sigc::bind (&on_frames_arguments_listed_signal,
                                         var_list));
    debugger->local_variables_listed_signal ().connect
                            (sigc::bind (&on_local_variables_listed_signal,
                                         var_list));
    //*******************************
    //</connect to IDebugger events>
    //******************************

    vector<UString> args;
    debugger->load_program ("fooprog", args, ".");
    debugger->set_breakpoint ("main");
    debugger->set_breakpoint ("func1");
    debugger->set_breakpoint ("func2");
    debugger->set_breakpoint ("func3");
    debugger->run ();

    //****************************************
    //run the event loop.
    //****************************************
    s_loop->run ();
    NEMIVER_CATCH_AND_RETURN_NOX (-1);
    return 0;
}

