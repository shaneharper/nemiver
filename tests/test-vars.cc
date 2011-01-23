#include <iostream>
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
static int nb_vars_created;
static int nb_vars_created2;
static int nb_vars_deleted;
static int nb_vars_deleted2;
static int unfold_requests;
static IDebugger::VariableSafePtr var_to_delete;

void on_variable_deleted_signal (const IDebugger::VariableSafePtr a_var);
void on_variable_unfolded_signal (const IDebugger::VariableSafePtr a_var,
                                  IDebuggerSafePtr a_debugger);
void on_changed_variables_listed_signal
                        (const list<IDebugger::VariableSafePtr> &a_vars,
                         IDebuggerSafePtr a_debugger);
void on_assign_variable_signal (const IDebugger::VariableSafePtr a_var,
                                IDebuggerSafePtr a_debugger);
void on_evaluate_variable_expr_signal (const IDebugger::VariableSafePtr a_var,
                                       IDebuggerSafePtr a_debugger);

void
on_command_done_signal (const UString&, const UString&)
{
}

void
on_engine_died_signal ()
{
    s_loop->quit ();
}

void
on_program_finished_signal ()
{
    s_loop->quit ();
}

void
on_running_signal ()
{
}

void
on_variable_created_signal (IDebugger::VariableSafePtr a_var)
{
    UString var_str;
    a_var->to_string (var_str, true /*show var name*/);
    MESSAGE ("variable created " << var_str);

    nb_vars_created++;
    if (a_var->name () == "person") {
        // put the variable aside so that we
        // can delete it later
        var_to_delete = a_var;
    }
}

void
on_variable_created_signal2 (IDebugger::VariableSafePtr a_var,
                             const UString &,
                             IDebuggerSafePtr a_debugger)
{
    UString var_str;
    a_var->to_string (var_str, true /*show var name*/);
    MESSAGE ("variable created (second report): " << var_str);
    nb_vars_created2++;

    unfold_requests++;
    a_debugger->unfold_variable (a_var,
                                 sigc::bind (&on_variable_unfolded_signal,
                                             a_debugger));
}

void
on_variable_created_in_func4_signal (IDebugger::VariableSafePtr a_var,
                                     IDebuggerSafePtr a_debugger)
{
    BOOST_REQUIRE (a_var->name () == "i");
    MESSAGE ("variable " << a_var->name () << " created");
    nb_vars_created++;
    a_debugger->assign_variable (a_var, "3456",
                                 sigc::bind (&on_assign_variable_signal,
                                             a_debugger));
}

void
on_assign_variable_signal (const IDebugger::VariableSafePtr a_var,
                           IDebuggerSafePtr a_debugger)
{
    BOOST_REQUIRE (a_var);
    BOOST_REQUIRE (a_var->name () == "i");
    BOOST_REQUIRE (a_var->value () == "3456");
    MESSAGE (a_var->name () << " assigned to: " + a_var->value ());

    a_debugger->evaluate_variable_expr
                            (a_var,
                             sigc::bind (&on_evaluate_variable_expr_signal,
                                         a_debugger));
}

void
on_evaluate_variable_expr_signal (const IDebugger::VariableSafePtr a_var,
                                  IDebuggerSafePtr a_debugger)
{
    BOOST_REQUIRE (a_var);
    BOOST_REQUIRE (a_var->name () == "i");
    BOOST_REQUIRE (a_var->value () == "3456");
    MESSAGE (a_var->name () <<  " evaluated to: " + a_var->value ());

    a_debugger->do_continue ();
}

void
on_variable_unfolded_signal (const IDebugger::VariableSafePtr a_var,
                             IDebuggerSafePtr a_debugger)
{
    unfold_requests--;
    UString var_str;
    a_var->to_string (var_str, true /*show var name*/);
    MESSAGE ("variable unfolded: " << var_str);
    typedef list<IDebugger::VariableSafePtr>::const_iterator VarIter;
    for (VarIter it = a_var->members ().begin ();
         it != a_var->members ().end ();
         ++it) {
        if ((*it)->has_expected_children ()) {
            unfold_requests++;
            a_debugger->unfold_variable (*it,
                                         sigc::bind (&on_variable_unfolded_signal,
                                                     a_debugger));
        }
    }
    if (unfold_requests == 0)
        a_debugger->step_over ();
}

void
on_changed_variables_listed_signal
                        (const list<IDebugger::VariableSafePtr> &a_vars,
                         IDebuggerSafePtr a_debugger)
{
    IDebugger::VariableSafePtr root = a_vars.front ()->root ();
    THROW_IF_FAIL (root);
    MESSAGE ("nb_elems: " << (int) a_vars.size ());
    int nb_elems = a_vars.size ();
    // We can't know for sure how many elemens got changed, as that
    // depend on the version of stl we are testing against.
    BOOST_REQUIRE (nb_elems >= 2);
    MESSAGE ("The changed members are: ");
    for (list<IDebugger::VariableSafePtr>::const_iterator it = a_vars.begin ();
         it != a_vars.end ();
         ++it) {
        MESSAGE ("name: "  + (*it)->internal_name ());
        MESSAGE ("value: " + (*it)->value ());
    }
    a_debugger->delete_variable (root, &on_variable_deleted_signal);
}

void
on_variable_deleted_signal (const IDebugger::VariableSafePtr a_var)
{
    MESSAGE ("variable deleted. Name: "
             << a_var->name ()
             << ", Internal Name: "
             << a_var->internal_name ());
    nb_vars_deleted++;
}

void
on_variable_deleted_signal2 (const IDebugger::VariableSafePtr a_var,
                             const UString&,
                             IDebuggerSafePtr a_debugger)
{
    MESSAGE ("variable deleted. Name: "
             << a_var->name ()
             << ", Internal Name: "
             << a_var->internal_name ());
    nb_vars_deleted2++;
    a_debugger->step_over ();
}

void
on_stopped_signal (IDebugger::StopReason /*a_reason*/,
                   bool a_has_frame,
                   const IDebugger::Frame &a_frame,
                   int,
                   int,
                   const UString &/*a_cookie*/,
                   IDebuggerSafePtr &a_debugger)
{
    static int nb_stops = 0;

    nb_stops++;
    MESSAGE ("stopped " << nb_stops << " times");
    if (a_has_frame) {
        MESSAGE ("frame name: " << a_frame.function_name ());
    }

    if (a_has_frame && a_frame.function_name () == "func4") {
        static int nb_stops_in_func4;
        nb_stops_in_func4++;
        MESSAGE ("stopped in func4 " << nb_stops_in_func4 << " times");
        if (nb_stops_in_func4 == 3) {
            a_debugger->create_variable
                ("i",
                 sigc::bind (&on_variable_created_in_func4_signal,
                             a_debugger));
        } else if (nb_stops_in_func4 < 5 ) {
            a_debugger->step_over ("in-func4");
        } else {
            a_debugger->do_continue ();
        }
    } else if (nb_stops == 2) {
        // Okay we stepped once, now we can now create the variable
        // for the person variable.
        a_debugger->create_variable ("person",
                                     &on_variable_created_signal);
        MESSAGE ("Requested creation of variable 'person'");
    } else if (nb_stops == 6) {
        // We passed the point where we changed the value of the members
        // of the 'person' variable, in fooprog.
        // let's now ask the debugger to tell us which descendant variable
        // was changed exactly.
        a_debugger->list_changed_variables
                (var_to_delete,
                 sigc::bind (&on_changed_variables_listed_signal, a_debugger));
    } else {
        a_debugger->step_over ();
    }
}

NEMIVER_API int
test_main (int, char **)
{
    NEMIVER_TRY;

    Initializer::do_init ();

    IDebuggerSafePtr debugger =
        debugger_utils::load_debugger_iface_with_confmgr ();

    // setup the debugger with the glib mainloop
    debugger->set_event_loop_context (Glib::MainContext::get_default ());

    //*******************************
    //<connect to IDebugger events>
    //******************************
    debugger->command_done_signal ().connect (&on_command_done_signal);
    debugger->engine_died_signal ().connect (&on_engine_died_signal);
    debugger->program_finished_signal ().connect (&on_program_finished_signal);
    debugger->running_signal ().connect (&on_running_signal);
    debugger->stopped_signal ().connect (sigc::bind (&on_stopped_signal,
                                                      debugger));
    debugger->variable_created_signal ().connect (sigc::bind
                                              (&on_variable_created_signal2,
                                               debugger));
    debugger->variable_deleted_signal ().connect
                (sigc::bind (&on_variable_deleted_signal2, debugger));

    //*******************************
    //</connect to IDebugger events>
    //******************************
    vector<UString> args;
    debugger->disable_pretty_printing ();
    debugger->load_program ("fooprog", args, ".");
    debugger->set_breakpoint ("main");
    debugger->set_breakpoint ("func4");
    // Set a breakpoint right after the person variable members get
    // modified
    debugger->set_breakpoint ("fooprog.cc", 105);
    debugger->run ();

    //****************************************
    //run the event loop.
    //****************************************
    s_loop->run ();

    NEMIVER_CATCH_AND_RETURN_NOX (-1)

    BOOST_REQUIRE (nb_vars_created == nb_vars_created2);
    BOOST_REQUIRE (nb_vars_created > 0);
    BOOST_REQUIRE (nb_vars_deleted == nb_vars_deleted2);
    BOOST_REQUIRE (nb_vars_deleted > 0);

    return 0;
}

