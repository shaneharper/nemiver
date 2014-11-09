//Author: Dodji Seketeli
/*
 *This file is part of the Nemiver project
 *
 *Nemiver is free software; you can redistribute
 *it and/or modify it under the terms of
 *the GNU General Public License as published by the
 *Free Software Foundation; either version 2,
 *or (at your option) any later version.
 *
 *Nemiver is distributed in the hope that it will
 *be useful, but WITHOUT ANY WARRANTY;
 *without even the implied warranty of
 *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *See the GNU General Public License for more details.
 *
 *You should have received a copy of the
 *GNU General Public License along with Nemiver;
 *see the file COPYING.
 *If not, write to the Free Software Foundation,
 *Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *See COPYRIGHT file copyright information.
 */
#include "config.h"

#include <map>
#include <list>
#include <glib/gi18n.h>
#include "common/nmv-exception.h"
#include "nmv-global-vars-inspector-dialog.h"
#include "nmv-variables-utils.h"
#include "nmv-vars-treeview.h"
#include "nmv-ui-utils.h"
#include "nmv-i-workbench.h"
#include "nmv-i-var-list-walker.h"

using namespace nemiver::common;
namespace vutil = nemiver::variables_utils2;

NEMIVER_BEGIN_NAMESPACE (nemiver)

struct GlobalVarsInspectorDialog::Priv : public sigc::trackable {
private:
    Priv ();
public:
    Gtk::Dialog &dialog;
    Glib::RefPtr<Gtk::Builder> gtkbuilder;
    IDebuggerSafePtr debugger;
    IVarListWalkerSafePtr global_variables_walker_list;

    IWorkbench &workbench;
    VarsTreeView* tree_view;
    Glib::RefPtr<Gtk::TreeStore> tree_store;
    Gtk::TreeModel::iterator cur_selected_row;
    SafePtr<Gtk::Menu> contextual_menu;
    UString previous_function_name;

    Priv (Gtk::Dialog &a_dialog,
          const Glib::RefPtr<Gtk::Builder> &a_gtkbuilder,
          IDebuggerSafePtr &a_debugger,
          IWorkbench &a_workbench) :
        dialog (a_dialog),
        gtkbuilder (a_gtkbuilder),
        workbench (a_workbench),
        tree_view (0)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (a_debugger);
        debugger = a_debugger;
        build_tree_view ();
        re_init_tree_view ();
        connect_to_debugger_signals ();
        init_graphical_signals ();
        build_dialog ();
        debugger->list_global_variables ();
    }

    void build_dialog ()
    {
        Gtk::Box *box =
            ui_utils::get_widget_from_gtkbuilder<Gtk::Box> (gtkbuilder,
                                                       "inspectorwidgetbox");
        THROW_IF_FAIL (box);
        Gtk::ScrolledWindow *scr = Gtk::manage (new Gtk::ScrolledWindow);
        THROW_IF_FAIL (scr);
        scr->set_policy (Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
        scr->set_shadow_type (Gtk::SHADOW_IN);
        THROW_IF_FAIL (tree_view);
        scr->add (*tree_view);
        box->pack_start (*scr);
        dialog.show_all ();
    }

    void build_tree_view ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        if (tree_view) {return;}
        tree_view = VarsTreeView::create ();
        THROW_IF_FAIL (tree_view);
        tree_store = tree_view->get_tree_store ();
        THROW_IF_FAIL (tree_store);
    }

    void re_init_tree_view ()
    {
        NEMIVER_TRY

        LOG_FUNCTION_SCOPE_NORMAL_DD;
        THROW_IF_FAIL (tree_view);
        THROW_IF_FAIL (tree_store);
        //tree_store->clear ();
        previous_function_name = "";
        NEMIVER_CATCH
    }

    IVarListWalkerSafePtr get_global_variables_walker_list ()
    {
        if (!global_variables_walker_list) {
            global_variables_walker_list = create_variable_walker_list ();
            THROW_IF_FAIL (global_variables_walker_list);
            global_variables_walker_list->variable_visited_signal ().connect
            (sigc::mem_fun
             (*this,
              &GlobalVarsInspectorDialog::Priv::on_global_variable_visited_signal));
        }
        return global_variables_walker_list;
    }

    IVarListWalkerSafePtr create_variable_walker_list ()
    {
        DynamicModule::Loader *loader =
            workbench.get_dynamic_module ().get_module_loader ();
        THROW_IF_FAIL (loader);
        DynamicModuleManager *module_manager =
                                    loader->get_dynamic_module_manager ();
        THROW_IF_FAIL (module_manager);
        IVarListWalkerSafePtr result =
            module_manager->load_iface<IVarListWalker> ("varlistwalker",
                                                        "IVarListWalker");
        THROW_IF_FAIL (result);
        result->initialize (debugger.get ());
        return result;
    }


    void connect_to_debugger_signals ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (debugger);
        debugger->global_variables_listed_signal ().connect
            (sigc::mem_fun (*this,
                            &Priv::on_global_variables_listed_signal));
    }

    void init_graphical_signals ()
    {
        THROW_IF_FAIL (tree_view);
        Glib::RefPtr<Gtk::TreeSelection> selection =
                                                tree_view->get_selection ();
        THROW_IF_FAIL (selection);
        selection->signal_changed ().connect
            (sigc::mem_fun (*this,
                            &Priv::on_tree_view_selection_changed_signal));
        tree_view->signal_row_expanded ().connect
            (sigc::mem_fun (*this, &Priv::on_tree_view_row_expanded_signal));
        tree_view->signal_row_activated ().connect
            (sigc::mem_fun (*this,
                            &Priv::on_tree_view_row_activated_signal));
    }

    void set_global_variables
            (const std::list<IDebugger::VariableSafePtr> &a_vars)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (tree_view && tree_store);
        tree_store->clear ();
        std::list<IDebugger::VariableSafePtr>::const_iterator it;
        for (it = a_vars.begin (); it != a_vars.end (); ++it) {
            THROW_IF_FAIL ((*it)->name () != "");
            append_a_global_variable (*it);
        }
    }

    void append_a_global_variable (const IDebugger::VariableSafePtr a_var)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (tree_view && tree_store && a_var);

        LOG_DD ("going to append variable '"
                << a_var->name ()
                << "'");

        Gtk::TreeModel::iterator iter;
        vutil::append_a_variable (a_var,
                                  static_cast<Gtk::TreeView&> (*tree_view),
                                  iter /* no parent */,
                                  iter /* result iter */,
                                  false /* do not truncate type */);
        tree_view->expand_row (tree_store->get_path (iter), false);
    }

    void update_a_global_variable (const IDebugger::VariableSafePtr a_var)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (tree_view);
        Gtk::TreeModel::iterator parent_row_it;
        vutil::update_a_variable (a_var, *tree_view, parent_row_it,
                                  false /* Do not truncate type */ ,
                                  true /* Handle highlight */ ,
                                  false /* The frame is not new */ );
    }

    //****************************
    //<debugger signal handlers>
    //****************************
    void on_global_variables_listed_signal
                            (const list<IDebugger::VariableSafePtr> a_vars,
                             const UString &a_cookie)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        if (a_cookie == "") {}

        NEMIVER_TRY

        IVarListWalkerSafePtr walker_list =
                                get_global_variables_walker_list ();
        THROW_IF_FAIL (walker_list);

        walker_list->remove_variables ();
        walker_list->append_variables (a_vars);
        walker_list->do_walk_variables ();

        NEMIVER_CATCH
    }

    //****************************
    //</debugger signal handlers>
    //****************************

    void on_global_variable_visited_signal (const IVarWalkerSafePtr &a_walker)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        NEMIVER_TRY

        THROW_IF_FAIL (a_walker->get_variable ());

        append_a_global_variable (a_walker->get_variable ());

        NEMIVER_CATCH
    }

    void on_tree_view_selection_changed_signal ()
    {
    }

    void on_tree_view_row_expanded_signal
                                (const Gtk::TreeModel::iterator &a_it,
                                 const Gtk::TreeModel::Path &a_path)
    {
        if (a_it) {}
        if (a_path.size ()) {}
    }

    void on_tree_view_row_activated_signal
                                    (const Gtk::TreeModel::Path &a_path,
                                     Gtk::TreeViewColumn *a_col)
    {
        NEMIVER_TRY

        THROW_IF_FAIL (tree_view && tree_store);
        Gtk::TreeModel::iterator it = tree_store->get_iter (a_path);
        UString type =
            (Glib::ustring) it->get_value
                                (vutil::get_variable_columns ().type);
        if (type == "") {return;}

        if (a_col != tree_view->get_column (2)) {return;}
        cur_selected_row = it;
        show_variable_type_in_dialog ();

        NEMIVER_CATCH
    }

    void show_variable_type_in_dialog ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        if (!cur_selected_row) {return;}
        UString type =
        (Glib::ustring) (*cur_selected_row)[vutil::get_variable_columns ().type];
        UString message;
        message.printf (_("Variable type is: \n %s"), type.c_str ());

        IDebugger::VariableSafePtr variable =
        (IDebugger::VariableSafePtr)
        cur_selected_row->get_value (vutil::get_variable_columns ().variable);
        THROW_IF_FAIL (variable);
        //message += "\nDumped for debug: \n";
        //variable->to_string (message, false);
        ui_utils::display_info (workbench.get_root_window (),
                                message);
    }

};//end GlobalVarsInspectorDialog::Priv

/// Constructor of the GlobalVarsInspectorDialog type.
///
/// \param a_parent the parent window of the dialog.
///
/// \param a_root_path the path to the root directory of the
/// resources of the dialog.
///
/// \param a_debugger the IDebugger interface to use to inspect the
/// variables.
///
/// \param a_workbench the IWorkbench interface to use.
GlobalVarsInspectorDialog::GlobalVarsInspectorDialog
(const UString &a_root_path,
 IDebuggerSafePtr &a_debugger,
 IWorkbench &a_workbench) :
    Dialog (a_root_path,
            "globalvarsinspector.ui",
            "globalvarsinspector",
            a_workbench.get_root_window ())
{
    m_priv.reset (new Priv (widget (), gtkbuilder (), a_debugger, a_workbench));
}

GlobalVarsInspectorDialog::~GlobalVarsInspectorDialog ()
{
    LOG_D ("deleted", "destructor-domain");
}

NEMIVER_END_NAMESPACE (nemiver)

