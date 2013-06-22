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

Glib::RefPtr<Glib::MainLoop> loop =
    Glib::MainLoop::create (Glib::MainContext::get_default ());

void
on_engine_died_signal ()
{
    MESSAGE ("engine died");
    loop->quit ();
}

void
on_program_finished_signal (IDebuggerSafePtr a_debugger)
{
    MESSAGE ("program finished");
    loop->quit ();

    BOOST_REQUIRE (a_debugger);
    BOOST_REQUIRE (a_debugger->get_state () == IDebugger::PROGRAM_EXITED);
}

void
display_help ()
{
    MESSAGE ("test-basic <prog-to-debug>\n");
}

NEMIVER_API int
test_main (int argc, char *argv[])
{
    if (argc || argv) {/*keep compiler happy*/}

    NEMIVER_TRY;

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
        (sigc::bind<IDebuggerSafePtr> (&on_program_finished_signal, debugger));

    std::vector<UString> args, source_search_dir;
    debugger->enable_pretty_printing (false);
    source_search_dir.push_back (".");

    BOOST_REQUIRE (debugger->get_state () == IDebugger::NOT_STARTED);
    debugger->load_program ("fooprog", args, ".",
                            source_search_dir, "",
                            false);
    BOOST_REQUIRE (debugger->get_state () == IDebugger::INFERIOR_LOADED);

    debugger->run ();
    loop->run ();

    NEMIVER_CATCH_NOX;

    return 0;
}
