#include <iostream>
#include <boost/test/minimal.hpp>
#include <glibmm.h>
#include "nmv-i-debugger.h"
#include "nmv-i-lang-trait.h"
#include "common/nmv-initializer.h"
#include "nmv-debugger-utils.h"

using namespace nemiver;
using namespace nemiver::common;
using namespace sigc;

static Glib::RefPtr<Glib::MainLoop> loop =
    Glib::MainLoop::create (Glib::MainContext::get_default ());

static int nb_stops = 0;
static int nb_derefed = 0;
static int nb_type_set = 0;

void
on_variable_derefed_signal (const IDebugger::VariableSafePtr &a_var,
                            const UString &a_cookie,
                            IDebuggerSafePtr a_debugger)
{
    if (a_cookie.empty ()) {/*keep compiler happy*/}
    BOOST_REQUIRE (a_var);

    BOOST_REQUIRE (a_var->is_dereferenced ());
    MESSAGE ("derefed '" << a_var->name () << "', got '"
             << a_var->get_dereferenced ()->name ()
             << "'");
    ++nb_derefed;

    if (a_var->name () == "foo_ptr") {
        BOOST_REQUIRE (a_var->get_dereferenced ()->members ().size () == 1);
    } else if (a_var->name () == "bar_ptr") {
        BOOST_REQUIRE
            (a_var->get_dereferenced ()->members ().size () == 1);
    } else if (a_var->name () == "baz_ptr") {
        BOOST_REQUIRE
            (a_var->get_dereferenced ()->members ().size () == 2);
    }
    a_debugger->step_over ();
}

void
on_variable_value_signal (const UString &a_var_name,
                          const IDebugger::VariableSafePtr &a_var,
                          const UString &a_cookie,
                          const IDebuggerSafePtr &a_debugger)
{
    if (a_cookie.empty () || a_var_name.empty ()) {/*keep compiler happy*/}
    BOOST_REQUIRE (a_debugger && a_var);
    BOOST_REQUIRE (a_var->name () == a_var_name);

    MESSAGE ("got variable '" << a_var->name () << "'");

    if (a_var_name == "foo_ptr" ||
        a_var_name == "bar_ptr" ||
        a_var_name == "baz_ptr") {
        a_debugger->get_variable_type (a_var);
    } else {
        UString msg = "Got variable name " + a_var_name;
        BOOST_FAIL (msg.c_str ());
    }
}

void
on_variable_type_set_signal (const IDebugger::VariableSafePtr &a_var,
                             const UString &a_cookie,
                             IDebuggerSafePtr &a_debugger)
{
    if (!a_cookie.empty ()) {}

    BOOST_REQUIRE (a_var);
    BOOST_REQUIRE (a_debugger);

    ++nb_type_set;

    MESSAGE ("got type of var set '" << a_var->name () << "':'"
             << a_var->type () << ":");

    if (a_var->name () == "foo_ptr" ||
        a_var->name () == "bar_ptr" ||
        a_var->name () == "baz_ptr") {
        ILangTrait &lang_trait = a_debugger->get_language_trait ();
        BOOST_REQUIRE (lang_trait.get_name () == "cpptrait");
        BOOST_REQUIRE (lang_trait.has_pointers ());
        BOOST_REQUIRE (lang_trait.is_type_a_pointer (a_var->type ()));
        a_debugger->dereference_variable (a_var);
    } else {
        UString msg = "Got variable name: "+ a_var->name ();
        BOOST_FAIL (msg.c_str ());
    }
}
void
on_stopped_signal (IDebugger::StopReason a_reason,
                   bool a_has_frame,
                   const IDebugger::Frame &a_frame,
                   int /*a_thread_id*/,
                   int /*bp num*/,
                   const UString &/*a_cookie*/,
                   IDebuggerSafePtr &a_debugger)
{
    BOOST_REQUIRE (a_debugger);

    if (a_reason == IDebugger::EXITED_NORMALLY) {
        loop->quit ();
        BOOST_REQUIRE (nb_derefed == 3);
        BOOST_REQUIRE (nb_type_set == 3);
        return;
    }
    ++nb_stops;

    if (a_has_frame && a_frame.function_name () == "main" && nb_stops == 4) {
        a_debugger->print_variable_value ("foo_ptr");
        a_debugger->print_variable_value ("bar_ptr");
        a_debugger->print_variable_value ("baz_ptr");
    } else if (a_frame.function_name () == "main") {
        if (nb_stops < 8) {
            a_debugger->step_over ();
        } else {
            a_debugger->do_continue ();
        }
    }
}

NEMIVER_API int
test_main (int argc, char **argv)
{
    if (argc || argv) {}

    NEMIVER_TRY;

    Initializer::do_init ();
    BOOST_REQUIRE (loop);

    IDebuggerSafePtr debugger =
      debugger_utils::load_debugger_iface_with_confmgr ();

    //setup the debugger with the glib mainloop
    debugger->set_event_loop_context (Glib::MainContext::get_default ());

    debugger->stopped_signal ().connect (sigc::bind
                                            (&on_stopped_signal, debugger));
    debugger->variable_value_signal ().connect (sigc::bind
                                        (&on_variable_value_signal, debugger));

    debugger->variable_type_set_signal ().connect (sigc::bind
                                    (&on_variable_type_set_signal, debugger));

    debugger->variable_dereferenced_signal ().connect
                                    (sigc::bind (&on_variable_derefed_signal,
                                                 debugger));


    vector<UString> args;
    debugger->load_program (".libs/pointerderef", args, ".");
    debugger->set_breakpoint ("main");
    debugger->run ();

    loop->run ();
    NEMIVER_CATCH_AND_RETURN_NOX(-1)
    return 0;
}
