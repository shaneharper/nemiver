#include <iostream>
#include <map>
#include <string>
#include <boost/test/test_tools.hpp>
#include <boost/test/minimal.hpp>
#include <glibmm.h>
#include "common/nmv-initializer.h"
#include "common/nmv-exception.h"
#include "nmv-i-var-list-walker.h"

using namespace nemiver ;
using namespace nemiver::common ;

Glib::RefPtr<Glib::MainLoop> s_loop =
    Glib::MainLoop::create (Glib::MainContext::get_default ()) ;

IDebugger::Frame s_current_frame ;

typedef std::list<IDebugger::VariableSafePtr> DebuggerVariableList ;
typedef std::map<std::string, IVarListWalkerSafePtr> VarListWalkerMap ;
typedef std::map<std::string, string> StringMap ;

VarListWalkerMap&
var_list_walker ()
{
    static VarListWalkerMap s_var_list_walker ;
    return s_var_list_walker ;
}

StringMap&
expected_variables ()
{
    static StringMap s_expected_variables ;
    return s_expected_variables ;
}

StringMap&
actual_variables ()
{
    static StringMap s_actual_variables ;
    return s_actual_variables ;
}

void on_variable_visited_signal (const IVarWalkerSafePtr &a_walker,
                                 IDebuggerSafePtr &a_debugger) ;

void on_variable_list_visited_signal (IDebuggerSafePtr &a_debugger) ;

IVarListWalkerSafePtr
create_var_list_walker (IDebuggerSafePtr a_debugger)
{
    THROW_IF_FAIL (a_debugger) ;

    IVarListWalkerSafePtr result  =
        DynamicModuleManager::load_iface_with_default_manager<IVarListWalker>
                                                                ("varlistwalker",
                                                                 "IVarListWalker") ;
    result->initialize (a_debugger) ;
    result->variable_visited_signal ().connect
        (sigc::bind (&on_variable_visited_signal, a_debugger)) ;
    result->variable_list_visited_signal ().connect
        (sigc::bind (&on_variable_list_visited_signal, a_debugger)) ;
    return result ;
}

void
on_command_done_signal (const UString &a_name,
                        const UString &a_cookie)
{
    if (a_cookie.empty ()) {
    }
    MESSAGE ("command " << a_name << " done") ;
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
                   int a_thread_id,
                   const UString &a_cookie,
                   IDebuggerSafePtr &a_debugger)
{
    BOOST_REQUIRE (a_debugger) ;

    if (a_has_frame || a_frame.level () || a_thread_id ||
        a_cookie.empty () || a_debugger) {
        /*keeps compiler happy*/
    }

    if (a_reason == IDebugger::EXITED_NORMALLY) {
        MESSAGE ("program exited normally") ;
        s_loop->quit () ;
        return ;
    }

    if (!a_has_frame) {
        MESSAGE ("stopped, but not in a frame, reason: " << a_reason) ;
        return ;
    }

    s_current_frame = a_frame ;

    MESSAGE ("stopped in function: '"
             << a_frame.function_name ()
             << "()'") ;

    a_debugger->list_local_variables () ;
    a_debugger->list_frames_arguments () ;
}

void
on_frames_arguments_listed_signal
        (const map<int, list<IDebugger::VariableSafePtr> > &a_frames_params,
         const UString &a_cookie)
{
    if (a_cookie.empty ()) {/*keep compiler happy*/}
    map<int, list<IDebugger::VariableSafePtr> >::const_iterator it ;
    it = a_frames_params.find (0) ;
    if (it == a_frames_params.end ()) {
        LOG_ERROR ("Could not find current frame") ;
        return ;
    }
    BOOST_REQUIRE (var_list_walker ().find (s_current_frame.function_name ())
                   != var_list_walker ().end ()) ;
    IVarListWalkerSafePtr walker = var_list_walker ()[s_current_frame.function_name ()];
    BOOST_REQUIRE (walker) ;
    walker->remove_variables () ;
    walker->append_variables (it->second) ;
    walker->do_walk_variables () ;
}

void
on_local_variables_listed_signal (const DebuggerVariableList &a_variables,
                                  const UString &a_cookie)
{
    if (a_variables.empty () || a_cookie.empty ()) {
    }
    BOOST_REQUIRE (var_list_walker ().find (s_current_frame.function_name ())
                   != var_list_walker ().end ()) ;
    IVarListWalkerSafePtr walker = var_list_walker ()[s_current_frame.function_name ()];
    BOOST_REQUIRE (walker) ;
    walker->remove_variables () ;
    walker->append_variables (a_variables) ;
    walker->do_walk_variables () ;
}

void
on_variable_visited_signal (const IVarWalkerSafePtr &a_walker,
                            IDebuggerSafePtr &a_debugger)
{
    if (a_debugger) {}

    MESSAGE ("in function "
             << s_current_frame.function_name ()
             << ", visited "
             << a_walker->get_variable ()->name ()) ;
    actual_variables ()[s_current_frame.function_name ()] +=
        " " + a_walker->get_variable ()->name () ;
}

void
on_variable_list_visited_signal (IDebuggerSafePtr &a_debugger)
{
    a_debugger->do_continue () ;
    MESSAGE ("continuing from frame " << s_current_frame.function_name ()) ;
}

NEMIVER_API int
test_main (int argc, char **argv)
{
    if (argc || argv) {/*keep compiler happy*/}

    NEMIVER_TRY

    Initializer::do_init () ;


    //load the IDebugger interface
    IDebuggerSafePtr debugger =
        DynamicModuleManager::load_iface_with_default_manager<IDebugger>
                                                                ("gdbengine",
                                                                 "IDebugger") ;
    //setup the debugger with the glib mainloop
    debugger->set_event_loop_context (Glib::MainContext::get_default ()) ;

    //*******************************
    //<connect to IDebugger events>
    //******************************
    debugger->command_done_signal ().connect (&on_command_done_signal) ;
    debugger->engine_died_signal ().connect (&on_engine_died_signal) ;
    debugger->program_finished_signal ().connect (&on_program_finished_signal);
    debugger->running_signal ().connect (&on_running_signal) ;
    debugger->stopped_signal ().connect (sigc::bind (&on_stopped_signal,
                                                      debugger)) ;
    debugger->frames_arguments_listed_signal ().connect
                            (&on_frames_arguments_listed_signal) ;
    debugger->local_variables_listed_signal ().connect
                            (&on_local_variables_listed_signal) ;
    //*******************************
    //</connect to IDebugger events>
    //******************************

    debugger->load_program (".libs/fooprog", ".") ;
    debugger->set_breakpoint ("main") ;
    debugger->set_breakpoint ("func1") ;
    debugger->set_breakpoint ("func2") ;
    debugger->set_breakpoint ("func3") ;

    var_list_walker ()["main"] = create_var_list_walker (debugger) ;
    expected_variables ()["main"] = " person" ;
    var_list_walker ()["func1"] = create_var_list_walker (debugger) ;
    expected_variables ()["func1"] = " i" ;
    var_list_walker ()["func2"] = create_var_list_walker (debugger) ;
    expected_variables ()["func2"] = " j a_a a_b" ;
    var_list_walker ()["func3"] = create_var_list_walker (debugger) ;
    expected_variables ()["func3"] = " a_param" ;
    debugger->run () ;

    //****************************************
    //run the event loop.
    //****************************************
    s_loop->run () ;

    BOOST_REQUIRE (actual_variables ().find ("main")  != actual_variables ().end ()) ;
    BOOST_REQUIRE (actual_variables ().find ("func1") != actual_variables ().end ()) ;
    BOOST_REQUIRE (actual_variables ().find ("func2") != actual_variables ().end ()) ;
    BOOST_REQUIRE (actual_variables ().find ("func3") != actual_variables ().end ()) ;

    if (actual_variables ()["main"] != expected_variables ()["main"]) {
        string str ("main vars: ") ;
        str += actual_variables ()["main"] ;
        BOOST_FAIL (str.c_str ()) ;
    }

    if (actual_variables ()["func1"] != expected_variables ()["func1"]) {
        string str ("func1 vars: ") ;
        str += actual_variables ()["func1"] ;
        BOOST_FAIL (str.c_str ()) ;
    }

    if (actual_variables ()["func2"] != expected_variables ()["func2"]) {
        string str ("func2 vars: ") ;
        str += actual_variables ()["func2"] ;
        BOOST_FAIL (str.c_str ()) ;
    }

    if (actual_variables ()["func3"] != expected_variables ()["func3"]) {
        string str ("func3 vars: ") ;
        str += actual_variables ()["func3"] ;
        BOOST_FAIL (str.c_str ()) ;
    }

    NEMIVER_CATCH_AND_RETURN_NOX (-1)

    return 0 ;
}
