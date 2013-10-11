#include "config.h"
#include <iostream>
#include <boost/test/minimal.hpp>
#include <glibmm.h>
#include "common/nmv-initializer.h"
#include "common/nmv-safe-ptr-utils.h"
#include "common/nmv-exception.h"
#include "nmv-i-debugger.h"
#include "nmv-debugger-utils.h"
#include "nmv-cmd-interpreter.h"

using namespace nemiver;
using namespace nemiver::common;

Glib::RefPtr<Glib::MainLoop> loop =
    Glib::MainLoop::create (Glib::MainContext::get_default ());
SafePtr<CmdInterpreter> interpreter;
std::ostringstream out;
unsigned int nb_stops = 0;

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
display_help ()
{
    MESSAGE ("test-basic <prog-to-debug>\n");
}

void
on_stopped_signal (IDebugger::StopReason a_reason,
                   bool a_has_frame,
                   const IDebugger::Frame &a_frame,
                   int /*a_thread_id*/,
                   const string &/*a_bp_num*/,
                   const UString &/*a_cookie*/)
{
    if (!a_has_frame) {
        return;
    }

    nb_stops++;

    if (a_reason == nemiver::IDebugger::BREAKPOINT_HIT) {
        if (a_frame.function_name () == "func1_1") {
            interpreter->execute_command ("next");
            interpreter->execute_command ("print i_i");
            interpreter->execute_command ("break func2");
            interpreter->execute_command ("continue");
        } else if (a_frame.function_name () == "func2") {
            BOOST_REQUIRE (out.str () == "i_i = 19\n");
            interpreter->execute_command ("break func3");
            interpreter->execute_command ("continue");
        } else if (a_frame.function_name () == "func3") {
            interpreter->execute_command ("step");
        }
    } else {
        if (a_frame.function_name () == "Person::do_this") {
            interpreter->execute_command ("finish");
        } else if (a_frame.function_name () == "func3") {
            interpreter->execute_command ("continue");
        }
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
    debugger->enable_pretty_printing (false);

    //*****************************
    //<connect to IDebugger events>
    //*****************************
    debugger->engine_died_signal ().connect (&on_engine_died_signal);
    debugger->program_finished_signal ().connect (&on_program_finished_signal);
    debugger->stopped_signal ().connect (&on_stopped_signal);

    interpreter.reset (new CmdInterpreter (*debugger, out));
    interpreter->execute_command ("load-exec ./fooprog");
    interpreter->execute_command ("break func1_1");
    interpreter->execute_command ("run");
    loop->run ();

    BOOST_REQUIRE (nb_stops == 6);

    NEMIVER_CATCH_NOX

    return 0;
}
