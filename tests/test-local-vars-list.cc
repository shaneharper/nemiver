#include <iostream>
#include <boost/test/minimal.hpp>
#include <boost/test/test_tools.hpp>
#include <glibmm.h>
#include "common/nmv-initializer.h"
#include "common/nmv-exception.h"
#include "nmv-i-var-list.h"

using namespace std ;
using namespace nemiver ;
using namespace nemiver::common ;

Glib::RefPtr<Glib::MainLoop> loop =
    Glib::MainLoop::create (Glib::MainContext::get_default ()) ;

static int nb_stops=0 ;
static int nb_var_type_set=0 ;
static int nb_var_value_set=0 ;

void
on_stopped_signal (IDebugger::StopReason a_reason,
                   bool a_has_frame,
                   const IDebugger::Frame &a_frame,
                   int a_thread_id,
                   const UString &a_cookie,
                   IDebuggerSafePtr &a_debugger,
                   IVarListSafePtr &a_var_list)
{
    if (a_thread_id || a_cookie.empty ()) {}

    BOOST_REQUIRE (a_debugger) ;
    BOOST_REQUIRE (a_var_list) ;

    MESSAGE ("stopped, reason: " << (int)a_reason) ;

    if (a_reason == IDebugger::EXITED_NORMALLY) {
        //okay, time to get out. Let's check if the overall test
        //went like we want
        BOOST_REQUIRE_MESSAGE (nb_var_type_set == 3,
                               "got nb_var_type_set: " << nb_var_type_set) ;
        BOOST_REQUIRE_MESSAGE (nb_var_value_set == 3,
                               "got nb_var_value_set: " << nb_var_value_set) ;
        BOOST_REQUIRE_MESSAGE (a_var_list->get_raw_list ().size () == 3,
                               "size:"
                                << (int)a_var_list->get_raw_list ().size ()) ;
        IDebugger::VariableSafePtr var ;
        BOOST_REQUIRE (a_var_list->find_variable ("foo_ptr", var)) ;
        BOOST_REQUIRE (var) ;
        BOOST_REQUIRE (var->name () != "") ;
        BOOST_REQUIRE_MESSAGE (var->type () != "", "var: " << var->name ()) ;

        BOOST_REQUIRE (a_var_list->find_variable ("bar_ptr", var)) ;
        BOOST_REQUIRE (var) ;
        BOOST_REQUIRE (var->name () != "") ;
        BOOST_REQUIRE (var->type () != "") ;

        BOOST_REQUIRE (a_var_list->find_variable ("baz_ptr", var)) ;
        BOOST_REQUIRE (var) ;
        BOOST_REQUIRE (var->name () != "") ;
        BOOST_REQUIRE (var->type () != "") ;
        loop->quit () ;
        return ;
    }


    if (a_has_frame && a_frame.function_name () == "main") {
        MESSAGE ("in main:" << (int)a_frame.line ());
        ++nb_stops ;
        if (nb_stops == 1) {
            a_debugger->list_local_variables () ;
        } else if (nb_stops == 4) {
            a_var_list->update_state () ;
            a_debugger->do_continue () ;
            return ;
        }
        a_debugger->step_over () ;
    }

}

void
on_local_variables_listed_signal (const DebuggerVariableList &a_vars,
                                  const UString &a_cookie,
                                  IVarListSafePtr &a_var_list)
{
    if (a_cookie.empty ()) {}

    BOOST_REQUIRE (a_var_list) ;
    BOOST_REQUIRE (a_var_list->get_raw_list ().empty ()) ;
    DebuggerVariableList::const_iterator it ;
    for (it = a_vars.begin () ; it != a_vars.end () ; ++it) {
        a_var_list->append_variable (*it) ;
    }

}

void
on_var_type_set (const IDebugger::VariableSafePtr &a_var)
{
    BOOST_REQUIRE (a_var) ;
    BOOST_REQUIRE (a_var->name () != "") ;
    BOOST_REQUIRE (a_var->type () != "") ;

    if (a_var->name () == "foo_ptr"
        ||a_var->name () == "bar_ptr"
        ||a_var->name () == "baz_ptr") {
        ++nb_var_type_set ;
        MESSAGE ("variable type set: "
                 <<a_var->name () << ":" << a_var->type ()) ;
    } else {
        BOOST_FAIL ("unexpected variable: " << a_var->name ()) ;
    }
}

void
on_var_value_set (const IDebugger::VariableSafePtr &a_var)
{
    BOOST_REQUIRE (a_var) ;
    BOOST_REQUIRE (a_var->name () != "") ;

    if (a_var->name () == "foo_ptr"
        ||a_var->name () == "bar_ptr"
        ||a_var->name () == "baz_ptr") {
        ++nb_var_value_set ;
        MESSAGE ("variable value set: "
                 << a_var->name ()) ;
    }
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

    //load the variable list interface
    IVarListSafePtr var_list =
        DynamicModuleManager::load_iface_with_default_manager<IVarList>
                                                                ("varlist",
                                                                 "IVarList");
    var_list->initialize (debugger) ;

    //set debugger slots
    debugger->stopped_signal ().connect (sigc::bind (&on_stopped_signal,
                                                     debugger,
                                                     var_list)) ;
    debugger->local_variables_listed_signal ().connect (sigc::bind
                            (&on_local_variables_listed_signal, var_list)) ;

    //set variable list slots
    var_list->variable_value_set_signal ().connect (&on_var_value_set) ;
    var_list->variable_type_set_signal ().connect (&on_var_type_set) ;

    //TODO: finish this: connect to var_list and debugger signals
    //load .libs/pointerderef program, list its local variables
    //add them to the list, step a bit, update the varlist state at
    //each step, check the state has been updated.
    debugger->load_program (".libs/pointerderef", ".") ;
    debugger->set_breakpoint ("main") ;
    debugger->run () ;

    loop->run () ;
    NEMIVER_CATCH_AND_RETURN_NOX (-1)

    return 0 ;
}
