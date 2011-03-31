#include <iostream>
#include <boost/test/minimal.hpp>
#include <glibmm.h>
#include "common/nmv-initializer.h"
#include "common/nmv-safe-ptr-utils.h"
#include "nmv-debugger-utils.h"

using namespace nemiver;
using namespace nemiver::common;

Glib::RefPtr<Glib::MainLoop> loop =
    Glib::MainLoop::create (Glib::MainContext::get_default ());

static bool broke_in_overload=false;

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
    if (!broke_in_overload) {
        BOOST_FAIL ("did not break in the overload");
    }
}

void
on_command_done_signal (const UString &a_command,
                        const UString &a_cookie)
{
    MESSAGE ("command done: '" << a_command << "', cookie: '" << a_cookie);
}

void
on_breakpoints_set_signal (const std::map<int, IDebugger::BreakPoint> &a_breaks,
                           const UString &a_cookie)
{
    if (a_cookie.empty ()) {/*keep compiler happy*/}

    MESSAGE ("breakpoints set:");
    typedef std::map<int, IDebugger::BreakPoint> Breakpoints;
    for (Breakpoints::const_iterator it=a_breaks.begin (); it != a_breaks.end (); ++it) {
        MESSAGE ("breakpoint num:" << it->second.number ()
                 << ", line: " << it->second.line ());
    }
}

void
on_running_signal ()
{
    MESSAGE ("debugger running ...");
}


void
on_stopped_signal (const UString &a_reason,
                   bool a_has_frame,
                   const IDebugger::Frame &a_frame,
                   int a_thread_id,
                   const UString &a_cookie,
                   IDebuggerSafePtr &a_debugger)
{
    BOOST_REQUIRE (a_debugger);
    if (a_thread_id || a_cookie.empty ()) {/*keep compiler happy*/}
    MESSAGE ("debugger stopped. reason == " << a_reason);

    if (a_reason == "breakpoint-hit") {
        if (a_has_frame ) {
            if (a_frame.function_name () == "func1") {
                a_debugger->set_breakpoint ("Person::overload");
            }  else if (a_frame.function_name ().find ("Person::overload")
                        != UString::npos) {
                broke_in_overload = true;
                MESSAGE ("broke in " << a_frame.function_name () << " woot.");
            }
        }
    }
    a_debugger->do_continue ();
}

void
on_got_overloads_choice_signal (const vector<IDebugger::OverloadsChoiceEntry> &a_choice,
                                const UString &a_reason,
                                IDebuggerSafePtr a_debugger)
{
    BOOST_REQUIRE (a_debugger);
    if (a_reason.empty ()) {/*keep compiler happy*/}

    vector<IDebugger::OverloadsChoiceEntry>::const_iterator it;
    for (it = a_choice.begin (); it != a_choice.end (); ++it) {
        if (it->kind () == IDebugger::OverloadsChoiceEntry::ALL) {
            cout << "[ALL]\n";
        } else if (it->kind () == IDebugger::OverloadsChoiceEntry::LOCATION) {
            cout << "[" << it->index () << "] " << it->function_name () << "\n";
        }
    }
    a_debugger->choose_function_overload (2);
}

NEMIVER_API int
test_main (int argc, char *argv[])
{
    if (argc || argv) {/*keep compiler happy*/}

    try {
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

        debugger->command_done_signal ().connect (&on_command_done_signal);

        debugger->breakpoints_set_signal ().connect
                                                (&on_breakpoints_set_signal);

        debugger->running_signal ().connect (&on_running_signal);

        debugger->stopped_signal ().connect
                                (sigc::bind (&on_stopped_signal, debugger));

        debugger->got_overloads_choice_signal ().connect
            (sigc::bind (&on_got_overloads_choice_signal, debugger));

        //*****************************
        //</connect to IDebugger events>
        //*****************************

        std::vector<UString> args, source_search_dir;
        args.push_back ("fooprog");
        source_search_dir.push_back (".");

        debugger->load_program (args, "", source_search_dir);
        debugger->set_breakpoint ("func1");
        debugger->run ();
        loop->run ();
    } catch (Glib::Exception &e) {
        LOG_ERROR ("got error: " << e.what () << "\n");
        return -1;
    } catch (exception &e) {
        LOG_ERROR ("got error: " << e.what () << "\n");
        return -1;
    } catch (...) {
        LOG_ERROR ("got an unknown error\n");
        return -1;
    }
    return 0;
}

