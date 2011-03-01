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
on_variable_type_signal (const UString &a_variable_name,
                         const UString &a_variable_type,
                         const UString &/*a_cookie*/)
{
    MESSAGE ("variable name is: " << a_variable_name);
    BOOST_REQUIRE (a_variable_name == "i");
    MESSAGE ("the type of variable is: " << a_variable_type);
    BOOST_REQUIRE (a_variable_type == "int");
}

void
on_variable_value_signal (const UString &a_variable_name,
                          const IDebugger::VariableSafePtr a_var,
                          const UString&)
{
    MESSAGE ("name of variable is: " << a_variable_name);
    BOOST_REQUIRE (a_variable_name == "i");
    MESSAGE ("variable value: " << a_var->value ());
    BOOST_REQUIRE (a_var && a_var->value () == "17");
}

void
on_stopped_signal (IDebugger::StopReason,
                   bool a_has_frame,
                   const IDebugger::Frame &a_frame,
                   int,
                   int,
                   const UString&,
                   IDebuggerSafePtr a_debugger)
{
    THROW_IF_FAIL (a_debugger);

    if (a_has_frame && a_frame.function_name () == "func1") {
        static int nb_stops_in_func1 = 0;
        nb_stops_in_func1++;
        if (nb_stops_in_func1 == 2) {
            a_debugger->print_variable_type ("i");
            a_debugger->print_variable_value ("i");
        }
    }
    a_debugger->step_over ();
}

NEMIVER_API int
test_main (int, char **)
{
    NEMIVER_TRY

    Initializer::do_init();
    THROW_IF_FAIL (loop);

    IDebuggerSafePtr debugger =
      debugger_utils::load_debugger_iface_with_confmgr ();

    debugger->set_event_loop_context (loop->get_context ());

    debugger->engine_died_signal ().connect (&on_engine_died_signal);

    debugger->program_finished_signal ().connect
                                            (&on_program_finished_signal);

    debugger->stopped_signal ().connect
                            (sigc::bind (&on_stopped_signal, debugger));

    debugger->variable_type_signal ().connect
                                            (&on_variable_type_signal);

    debugger->variable_value_signal ().connect (&on_variable_value_signal);

    debugger->enable_pretty_printing (false);
    std::vector<UString> args, source_search_dir;
    source_search_dir.push_back (".");

    debugger->load_program ("fooprog", args, ".", source_search_dir, "", false);
    debugger->set_breakpoint ("func1");

    debugger->run ();
    loop->run ();

    NEMIVER_CATCH_NOX
    return 0;
}
