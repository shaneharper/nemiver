#include <iostream>
#include <boost/test/minimal.hpp>
#include <boost/test/test_tools.hpp>
#include <glibmm.h>
#include "nmv-i-debugger.h"
#include "common/nmv-initializer.h"

using namespace nemiver ;
using namespace nemiver::common ;
using namespace sigc ;

static Glib::RefPtr<Glib::MainLoop> loop =
    Glib::MainLoop::create (Glib::MainContext::get_default ()) ;

static int nb_stops = 0 ;
static int nb_derefed = 0 ;
static int nb_type_set = 0 ;

void
on_variable_derefed_signal (const IDebugger::VariableSafePtr &a_var,
                            const UString &a_cookie)
{
    if (a_cookie.empty ()) {/*keep compiler happy*/}
    BOOST_REQUIRE (a_var) ;

    BOOST_REQUIRE (a_var->is_dereferenced ()) ;
    MESSAGE ("derefed '" << a_var->name () << "', got '"
             << a_var->get_dereferenced ()->name ()
             << "'") ;
    ++nb_derefed ;

    if (a_var->name () == "foo_ptr") {
        BOOST_REQUIRE_MESSAGE
            (a_var->get_dereferenced ()->members ().size () == 1,
             "got: " << a_var->get_dereferenced ()->members ().size ()) ;
    } else if (a_var->name () == "bar_ptr") {
        BOOST_REQUIRE_MESSAGE
            (a_var->get_dereferenced ()->members ().size () == 1,
             "got: " << a_var->get_dereferenced ()->members ().size ()) ;
    } else if (a_var->name () == "baz_ptr") {
        BOOST_REQUIRE_MESSAGE
            (a_var->get_dereferenced ()->members ().size () == 2,
             "got: " << a_var->get_dereferenced ()->members ().size ()) ;
    }
}

void
on_variable_value_signal (const UString &a_var_name,
                          const IDebugger::VariableSafePtr &a_var,
                          const UString &a_cookie,
                          const IDebuggerSafePtr &a_debugger)
{
    if (a_cookie.empty () || a_var_name.empty ()) {/*keep compiler happy*/}
    BOOST_REQUIRE (a_debugger && a_var) ;
    BOOST_REQUIRE (a_var->name () == a_var_name) ;

    MESSAGE ("got variable '" << a_var->name () << "'") ;

    if (a_var_name == "foo_ptr" ||
        a_var_name == "bar_ptr" ||
        a_var_name == "baz_ptr") {
        a_debugger->get_variable_type (a_var) ;
    } else {
        BOOST_FAIL ("Got variable name: " << a_var_name) ;
    }
}

void
on_variable_type_set_signal (const IDebugger::VariableSafePtr &a_var,
                             const UString &a_cookie,
                             IDebuggerSafePtr &a_debugger)
{
    if (!a_cookie.empty ()) {}

    BOOST_REQUIRE (a_var) ;
    BOOST_REQUIRE (a_debugger) ;

    ++nb_type_set ;

    MESSAGE ("got type of var set '" << a_var->name () << "':'"
             << a_var->type () << ":") ;

    if (a_var->name () == "foo_ptr" ||
        a_var->name () == "bar_ptr" ||
        a_var->name () == "baz_ptr") {
        ILangTraitSafePtr lang_trait = a_debugger->get_language_trait () ;
        BOOST_REQUIRE (lang_trait->get_name () == "cpptrait") ;
        BOOST_REQUIRE (lang_trait->has_pointers ()) ;
        BOOST_REQUIRE (lang_trait->is_type_a_pointer (a_var->type ())) ;
        a_debugger->dereference_variable (a_var) ;
    } else {
        BOOST_FAIL ("Got variable name: " << a_var->name ()) ;
    }
}
void
on_stopped_signal (IDebugger::StopReason a_reason,
                   bool a_has_frame,
                   const IDebugger::Frame &a_frame,
                   int a_thread_id,
                   const UString &a_cookie,
                   IDebuggerSafePtr &a_debugger)
{
    if (a_has_frame || a_frame.level () || a_thread_id || a_cookie.empty ()) {}

    BOOST_REQUIRE (a_debugger) ;

    if (a_reason == IDebugger::EXITED_NORMALLY) {
        loop->quit ();
        BOOST_REQUIRE_MESSAGE (nb_derefed == 3,
                               "nb_derefed is " << nb_derefed) ;
        BOOST_REQUIRE_MESSAGE (nb_type_set == 3,
                               "nb_type_set is " << nb_type_set) ;
        return;
    }
    ++nb_stops;

    if (a_frame.function_name () == "main" && nb_stops == 4) {
        a_debugger->print_variable_value ("foo_ptr") ;
        a_debugger->print_variable_value ("bar_ptr") ;
        a_debugger->print_variable_value ("baz_ptr") ;
        a_debugger->step_over () ;
    } else if (a_frame.function_name () == "main") {
        if (nb_stops < 8) {
            a_debugger->step_over () ;
        } else {
            a_debugger->do_continue () ;
        }
    }
}

NEMIVER_API int
test_main (int argc, char **argv)
{
    if (argc || argv) {}

    NEMIVER_TRY

    Initializer::do_init () ;
    BOOST_REQUIRE (loop) ;

    IDebuggerSafePtr debugger =
        DynamicModuleManager::load_iface_with_default_manager<IDebugger>
                                                                ("gdbengine",
                                                                 "IDebugger") ;
    //setup the debugger with the glib mainloop
    debugger->set_event_loop_context (Glib::MainContext::get_default ()) ;

    debugger->stopped_signal ().connect (sigc::bind
                                            (&on_stopped_signal, debugger)) ;
    debugger->variable_value_signal ().connect (sigc::bind
                                        (&on_variable_value_signal, debugger)) ;

    debugger->variable_type_set_signal ().connect (sigc::bind
                                    (&on_variable_type_set_signal, debugger)) ;

    debugger->variable_dereferenced_signal ().connect
                                            (&on_variable_derefed_signal) ;


    debugger->load_program (".libs/pointerderef", ".") ;
    debugger->set_breakpoint ("main") ;
    debugger->run () ;

    loop->run () ;
    NEMIVER_CATCH_AND_RETURN_NOX(-1)
    return 0 ;
}
