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

static unsigned int nb_bp = 0;
static unsigned int nb_stops = 0;

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
    MESSAGE ("nb of breakpoint hit: " <<  (int)nb_bp);
    BOOST_REQUIRE(nb_bp == 5);
    BOOST_REQUIRE(nb_stops > 5);
    loop->quit ();
}

void
on_command_done_signal (const UString &a_command,
                        const UString &a_cookie)
{
    MESSAGE ("command done: '"
              << a_command
              << "', cookie: '"
              << a_cookie);
}

void
on_breakpoints_set_signal (const std::map<int, IDebugger::Breakpoint> &a_breaks,
                           const UString &a_cookie)
{
    if (a_cookie.empty ()) {}

    MESSAGE ("breakpoints set:");
    std::map<int, IDebugger::Breakpoint>::const_iterator it;
    for (it = a_breaks.begin (); it != a_breaks.end () ; ++it) {
        MESSAGE ("<break><num>" << it->first <<"</num><line>"
                 << it->second.file_name () << ":" << it->second.line ()
                 << "</line></break>");
    }
}

void
on_running_signal ()
{
    MESSAGE ("debugger running ...");
}

void
on_got_proc_info_signal (int a_pid, const UString &a_exe_path)
{
    MESSAGE ("debugging program '"<< a_exe_path
              << "' of pid: '" << a_pid << "'");
}

//this breakpoint condition is good because it must be satisfied
static const char *good_break_condition = "j == 900";
//this breakpoint condition is good because it can't be satisfied
static const char *bad_break_condition = "j == 1010";
static bool cond_breakpoint_set = 0;
void
on_stopped_signal (IDebugger::StopReason a_reason,
                   bool a_has_frame,
                   const IDebugger::Frame &a_frame,
                   int a_thread_id,
                   int a_bp_num,
                   const UString &/*a_cookie*/,
                   IDebuggerSafePtr &a_debugger)
{
    BOOST_REQUIRE (a_debugger);

    ++nb_stops;

    MESSAGE ("stopped, reason is '" << a_reason << "'");
    MESSAGE ("thread-id is '" << a_thread_id);

    if (a_reason == IDebugger::BREAKPOINT_HIT) {
        ++nb_bp;
    }

    if (a_has_frame) {
        MESSAGE ("in frame: " << a_frame.function_name ());
        if (a_frame.function_name () == "main") {
            a_debugger->list_threads ();
            a_debugger->select_thread (1);
            MESSAGE ("continue from main");
            a_debugger->do_continue ();
        } else if (a_frame.function_name () == "func1") {
            MESSAGE ("stepping from func1");
            a_debugger->step_over ();
        } else if (a_frame.function_name () == "func2") {
            MESSAGE ("stepping from func2");
            a_debugger->step_over ();
        } else if (a_frame.function_name () == "func4") {
            if (!cond_breakpoint_set) {
                MESSAGE ("set conditional breakpoint with cond: "
                         << good_break_condition
                         << "; we expect this breakpoint to be hit");
                a_debugger->set_breakpoint ("fooprog.cc", 83,
                                            good_break_condition);
                MESSAGE ("set conditional breakpoint with cond: "
                         << bad_break_condition
                         << "; this one should never be hit");
                a_debugger->set_breakpoint ("fooprog.cc", 83,
                                            bad_break_condition);
                cond_breakpoint_set = true;
            } else {
                MESSAGE ("hit conditional breakpoint! bp number: "
                         << a_bp_num);
            }
            map<int, IDebugger::Breakpoint>::const_iterator it;
            map<int, IDebugger::Breakpoint>::const_iterator null_iter =
                                    a_debugger->get_cached_breakpoints ().end ();

            if ((it = a_debugger->get_cached_breakpoints ().find (a_bp_num))
                 != null_iter
                 && it->second.has_condition ()) {
                MESSAGE ("hit conditional breakpoint. condition was: "
                         << it->second.condition ());
            }
            a_debugger->do_continue ();
        } else {
            BOOST_FAIL ("Stopped, for an unknown reason");
        }
    }
}

void
on_variable_type_signal (const UString &a_variable_name,
                         const UString &a_variable_type,
                         const UString &a_cookie)
{
    if (a_cookie.empty ()) {}

    MESSAGE ("type of variable '" << a_variable_name
              << "' is '" << a_variable_type);
}

void
on_threads_listed_signal (const std::list<int> &a_thread_ids,
                          const UString &a_cookie)
{

    if (a_cookie.empty ()) {}

    MESSAGE ("number of threads: '" << (int)a_thread_ids.size ());
    std::list<int>::const_iterator it;
    for (it = a_thread_ids.begin (); it != a_thread_ids.end () ; ++it) {
        MESSAGE ("thread-id: '" << *it);
    }
}

void
on_thread_selected_signal (int a_thread_id,
                           const IDebugger::Frame * const a_frame,
                           const UString &a_cookie)
{
    if (a_cookie.empty ()) {}

    MESSAGE ("thread selected: '" << a_thread_id) ;
    if (a_frame) {
        MESSAGE ("frame in thread : '" << a_frame->level ()) ;
        MESSAGE ("frame.function: '" << a_frame->function_name ());
    } else {
        MESSAGE ("no frame provided");
    }
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

    NEMIVER_TRY

    Initializer::do_init ();

    THROW_IF_FAIL (loop);

    IDebuggerSafePtr debugger =
        debugger_utils::load_debugger_iface_with_gconf ();

    debugger->set_event_loop_context (loop->get_context ());

    //*****************************
    //<connect to IDebugger events>
    //*****************************
    debugger->engine_died_signal ().connect (&on_engine_died_signal);

    debugger->program_finished_signal ().connect
                                            (&on_program_finished_signal);

    debugger->command_done_signal ().connect (&on_command_done_signal);

    debugger->breakpoints_list_signal ().connect
                                            (&on_breakpoints_set_signal);

    debugger->running_signal ().connect (&on_running_signal);

    debugger->got_target_info_signal ().connect (&on_got_proc_info_signal);

    debugger->stopped_signal ().connect
                            (sigc::bind (&on_stopped_signal, debugger));

    debugger->variable_type_signal ().connect
                                            (&on_variable_type_signal);

    debugger->threads_listed_signal ().connect
                                        (&on_threads_listed_signal);

    debugger->thread_selected_signal ().connect
                                        (&on_thread_selected_signal);

    //*****************************
    //</connect to IDebugger events>
    //*****************************

    std::vector<UString> args, source_search_dir;
    source_search_dir.push_back (".");
    debugger->load_program ("fooprog", args, ".",
                            source_search_dir, "",
                            false);
    debugger->set_breakpoint ("main");
    debugger->set_breakpoint ("func1");
    debugger->set_breakpoint ("func2");
    debugger->set_breakpoint ("func4");
    debugger->run ();
    loop->run ();

    NEMIVER_CATCH_NOX

    return 0;
}
