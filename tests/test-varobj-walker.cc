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

static std::string serialized_variable;

static IVarWalkerSafePtr
create_varobj_walker ()
{
    IVarWalkerSafePtr result  =
        DynamicModuleManager::load_iface_with_default_manager<IVarWalker>
                                            ("varobjwalker", "IVarWalker");
    return result;
}

static IVarWalkerSafePtr
get_varobj_walker ()
{
    static IVarWalkerSafePtr var;
    if (!var)
        var = create_varobj_walker ();
    return var;
}

static void
gen_white_spaces (int a_nb_ws,
                  std::string &a_ws_str)
{
    for (int i = 0; i < a_nb_ws; i++) {
        a_ws_str += ' ';
    }

}


static void
dump_variable_value (IDebugger::VariableSafePtr a_var,
                     int a_indent_num,
                     std::ostringstream &a_os)
{
    THROW_IF_FAIL (a_var);

    std::string ws_string;
    gen_white_spaces (a_indent_num, ws_string);

    a_os << ws_string << a_var->name ();

    if (!a_var->members ().empty ()) {
        a_os << "\n"  << ws_string << "{\n";
        IDebugger::VariableList::const_iterator it;
        for (it = a_var->members ().begin ();
             it != a_var->members ().end ();
             ++it) {
            dump_variable_value (*it, a_indent_num + 2, a_os);
        }
        a_os << "\n" << ws_string <<  "}";
    } else {
        a_os << " = " << a_var->value ();
    }
}

static void
dump_variable_value (IDebugger::VariableSafePtr a_var,
                     int a_indent_num,
                     std::string &a_out_str)
{
    std::ostringstream os;
    dump_variable_value (a_var, a_indent_num, os);
    a_out_str = os.str ();
}

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
on_variable_visited_signal (const IDebugger::VariableSafePtr a_var)
{
    MESSAGE ("dumping the variable ...");
    dump_variable_value (a_var, 0, serialized_variable);
    MESSAGE ("dumping the variable: DONE");
    s_loop->quit ();
}


static void
do_varobj_walker_stuff (IDebuggerSafePtr a_debugger)
{
    MESSAGE ("do varobj walker stuff");

    IVarWalkerSafePtr var_walker = get_varobj_walker ();

    var_walker->visited_variable_signal ().connect (&on_variable_visited_signal);
    var_walker->connect (a_debugger, "person");
    var_walker->do_walk_variable ();
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
        MESSAGE ("broke in main");
        // We did break in main. We need to step once to pass the
        // initialization of the person variable.
        a_debugger->step_over ();
        return;
    } else if (a_frame.function_name () == "main") {
        MESSAGE ("okay, stopped in main right after stepping");
        do_varobj_walker_stuff (a_debugger);
    }

    NEMIVER_CATCH_NOX

}

NEMIVER_API int
test_main (int, char **)
{
    NEMIVER_TRY

    Initializer::do_init ();

    // load the IDebugger interface
    IDebuggerSafePtr debugger =
        debugger_utils::load_debugger_iface_with_confmgr ();

    // setup the debugger with the glib mainloop
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

    NEMIVER_CATCH_AND_RETURN_NOX (-1)

    BOOST_REQUIRE (!serialized_variable.empty ());
    MESSAGE (serialized_variable);

    return 0;
}

