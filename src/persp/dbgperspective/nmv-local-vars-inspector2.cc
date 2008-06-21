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

#ifdef WITH_VARIABLE_WALKER

#include <map>
#include <list>
#include <glib/gi18n.h>
#include <gtkmm/treestore.h>
#include <gtkmm/treerowreference.h>
#include "common/nmv-exception.h"
#include "nmv-local-vars-inspector2.h"
#include "nmv-variables-utils2.h"
#include "nmv-ui-utils.h"
#include "nmv-i-workbench.h"
#include "nmv-i-var-list-walker.h"
#include "nmv-vars-treeview.h"

using namespace nemiver::common;
namespace vutil=nemiver::variables_utils2;

NEMIVER_BEGIN_NAMESPACE (nemiver)

struct LocalVarsInspector2::Priv : public sigc::trackable {
private:
    Priv ();
public:
    IDebuggerSafePtr debugger;
    IVarListWalkerSafePtr local_var_list_walker;
    IVarListWalkerSafePtr function_args_var_list_walker;
    IVarListWalkerSafePtr derefed_variables_walker_list;

    IWorkbench &workbench;
    IPerspective& perspective;
    VarsTreeViewSafePtr tree_view ;
    Glib::RefPtr<Gtk::TreeStore> tree_store;
    Gtk::TreeModel::iterator cur_selected_row;
    SafePtr<Gtk::TreeRowReference> local_variables_row_ref;
    SafePtr<Gtk::TreeRowReference> function_arguments_row_ref;
    SafePtr<Gtk::TreeRowReference> derefed_variables_row_ref;
    std::map<UString, IDebugger::VariableSafePtr> local_vars_to_set;
    std::map<UString, IDebugger::VariableSafePtr> function_arguments_to_set;
    SafePtr<Gtk::Menu> contextual_menu;
    Gtk::MenuItem *dereference_mi;
    Gtk::Widget *context_menu;
    UString previous_function_name;
    Glib::RefPtr<Gtk::ActionGroup> var_inspector_action_group;
    bool is_new_frame;

    Priv (IDebuggerSafePtr &a_debugger,
          IWorkbench &a_workbench,
          IPerspective& a_perspective) :
        workbench (a_workbench),
        perspective (a_perspective),
        tree_view (VarsTreeView::create ()),
        dereference_mi (0),
        context_menu (0),
        is_new_frame (false)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (a_debugger);
        debugger = a_debugger;
        THROW_IF_FAIL (tree_view);
        tree_store = tree_view->get_tree_store ();
        THROW_IF_FAIL (tree_store) ;
        re_init_tree_view ();
        connect_to_debugger_signals ();
        init_graphical_signals ();
        init_actions ();
    }

    void init_actions ()
    {
        Gtk::StockID nil_stock_id ("");
        static ui_utils::ActionEntry var_inspector_action_entries [] = {
            {
                "DereferencePointerMenuItemAction",
                nil_stock_id,
                _("Dereference the pointer"),
                _("Dereference the selected pointer variable"),
                sigc::mem_fun
                    (*this,
                     &Priv::dereference_pointer_action),
                ui_utils::ActionEntry::DEFAULT,
                ""
            }
        };

        var_inspector_action_group =
            Gtk::ActionGroup::create ("var-inspector-action-group");
        var_inspector_action_group->set_sensitive (true);
        int num_actions =
            sizeof (var_inspector_action_entries)
                /
            sizeof (ui_utils::ActionEntry);

        ui_utils::add_action_entries_to_action_group
            (var_inspector_action_entries,
             num_actions,
             var_inspector_action_group);

        workbench.get_ui_manager ()->insert_action_group
                                                (var_inspector_action_group);
    }

    void re_init_tree_view ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        THROW_IF_FAIL (tree_store);
        tree_store->clear ();
        IVarListWalkerSafePtr walker_list = get_local_vars_walker_list ();
        walker_list->remove_variables ();
        walker_list = get_function_args_vars_walker_list ();
        walker_list->remove_variables ();
        previous_function_name = "";
        is_new_frame = true;

        //****************************************************
        //add four rows: local variables, function arguments,
        //dereferenced variables and global variables.
        //Store row refs off both rows
        //****************************************************
        Gtk::TreeModel::iterator it = tree_store->append ();
        THROW_IF_FAIL (it);
        (*it)[vutil::get_variable_columns ().name] = _("Local Variables");
        local_variables_row_ref.reset
                    (new Gtk::TreeRowReference (tree_store,
                                                tree_store->get_path (it)));
        THROW_IF_FAIL (local_variables_row_ref);

        it = tree_store->append ();
        THROW_IF_FAIL (it);
        (*it)[vutil::get_variable_columns ().name] = _("Function Arguments");
        function_arguments_row_ref.reset
            (new Gtk::TreeRowReference (tree_store,
                                        tree_store->get_path (it)));
        THROW_IF_FAIL (function_arguments_row_ref);

        it = tree_store->append ();
        THROW_IF_FAIL (it);
        (*it)[vutil::get_variable_columns ().name] =
                                        _("Dereferenced Variables");
        derefed_variables_row_ref.reset (new Gtk::TreeRowReference
                                                (tree_store,
                                                 tree_store->get_path (it)));
        THROW_IF_FAIL (derefed_variables_row_ref);

    }

    void get_function_arguments_row_iterator
                                    (Gtk::TreeModel::iterator &a_it) const
    {
        THROW_IF_FAIL (function_arguments_row_ref);
        a_it = tree_store->get_iter
                            (function_arguments_row_ref->get_path ());
    }

    bool is_function_arguments_subtree_empty () const
    {
        Gtk::TreeModel::iterator it;

        get_function_arguments_row_iterator (it);
        return it->children ().empty ();
    }

    void get_local_variables_row_iterator (Gtk::TreeModel::iterator &a_it)
    {
        THROW_IF_FAIL (local_variables_row_ref);
        a_it = tree_store->get_iter (local_variables_row_ref->get_path ());
    }

    void get_derefed_variables_row_iterator (Gtk::TreeModel::iterator &a_it)
    {
        THROW_IF_FAIL (derefed_variables_row_ref);
        a_it = tree_store->get_iter (derefed_variables_row_ref->get_path ());
    }

    IVarListWalkerSafePtr get_local_vars_walker_list ()
    {
        if (!local_var_list_walker) {
            local_var_list_walker = create_variable_walker_list ();
            THROW_IF_FAIL (local_var_list_walker);
            local_var_list_walker->variable_visited_signal ().connect
            (sigc::mem_fun
             (*this,
              &LocalVarsInspector2::Priv::on_local_variable_visited_signal));
        }
        return local_var_list_walker;
    }

    IVarListWalkerSafePtr get_function_args_vars_walker_list ()
    {
        if (!function_args_var_list_walker) {
            function_args_var_list_walker = create_variable_walker_list ();
            THROW_IF_FAIL (function_args_var_list_walker);
            function_args_var_list_walker->variable_visited_signal ().connect
            (sigc::mem_fun
             (*this,
              &LocalVarsInspector2::Priv::on_func_arg_visited_signal));
        }
        return function_args_var_list_walker;
    }

    IVarListWalkerSafePtr get_derefed_variables_walker_list ()
    {
        if (!derefed_variables_walker_list) {
            derefed_variables_walker_list = create_variable_walker_list ();
            THROW_IF_FAIL (derefed_variables_walker_list);
            derefed_variables_walker_list->variable_visited_signal ().connect
            (sigc::mem_fun
             (*this,
              &LocalVarsInspector2::Priv::on_derefed_variable_visited_signal));
        }
        return derefed_variables_walker_list;
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
        result->initialize (debugger);
        return result;
    }

    void connect_to_debugger_signals ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (debugger);
        debugger->stopped_signal ().connect
            (sigc::mem_fun (*this, &Priv::on_stopped_signal));
        debugger->local_variables_listed_signal ().connect
            (sigc::mem_fun (*this, &Priv::on_local_variables_listed_signal));
        debugger->frames_arguments_listed_signal ().connect
            (sigc::mem_fun (*this, &Priv::on_frames_params_listed_signal));
        debugger->variable_dereferenced_signal ().connect
            (sigc::mem_fun (*this, &Priv::on_variable_dereferenced_signal));
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
        tree_view->signal_button_press_event ().connect_notify
            (sigc::mem_fun (this, &Priv::on_button_press_signal));
    }

    void set_local_variables
                    (const std::list<IDebugger::VariableSafePtr> &a_vars)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        clear_local_variables ();
        std::list<IDebugger::VariableSafePtr>::const_iterator it;
        for (it = a_vars.begin (); it != a_vars.end (); ++it) {
            THROW_IF_FAIL ((*it)->name () != "");
            append_a_local_variable (*it);
        }
    }

    void set_function_arguments
                    (const std::list<IDebugger::VariableSafePtr> &a_vars)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        clear_function_arguments ();
        std::list<IDebugger::VariableSafePtr>::const_iterator it;
        for (it = a_vars.begin (); it != a_vars.end (); ++it) {
            THROW_IF_FAIL ((*it)->name () != "");
            append_a_function_argument (*it);
        }
    }

    void clear_local_variables ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (tree_store);
        Gtk::TreeModel::iterator row_it;
        get_local_variables_row_iterator (row_it);
        Gtk::TreeModel::Children rows = row_it->children ();
        for (row_it = rows.begin (); row_it != rows.end (); ++row_it) {
            tree_store->erase (row_it);
        }
    }

    void clear_function_arguments ()
    {
        THROW_IF_FAIL (tree_store);
        Gtk::TreeModel::iterator row_it;
        get_function_arguments_row_iterator (row_it);
        Gtk::TreeModel::Children rows = row_it->children ();
        for (row_it = rows.begin (); row_it != rows.end (); ++row_it) {
            tree_store->erase (*row_it);
        }
    }

    void clear_derefed_variables ()
    {
        THROW_IF_FAIL (tree_store);
        Gtk::TreeModel::iterator row_it;
        get_derefed_variables_row_iterator (row_it);
        Gtk::TreeModel::Children rows = row_it->children ();
        for (row_it = rows.begin (); row_it != rows.end (); ++row_it) {
            tree_store->erase (*row_it);
        }
    }

    void append_a_local_variable (const IDebugger::VariableSafePtr a_var)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (tree_view && tree_store);

        Gtk::TreeModel::iterator parent_row_it;
        get_local_variables_row_iterator (parent_row_it);
        vutil::append_a_variable (a_var,
                                  *tree_view,
                                  tree_store,
                                  parent_row_it);
        tree_view->expand_row (tree_store->get_path (parent_row_it), false);

    }

    void append_a_function_argument (const IDebugger::VariableSafePtr a_var)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (tree_view && tree_store);

        Gtk::TreeModel::iterator parent_row_it;
        get_function_arguments_row_iterator (parent_row_it);
        vutil::append_a_variable (a_var,
                                  *tree_view,
                                  tree_store,
                                  parent_row_it);
        tree_view->expand_row (tree_store->get_path (parent_row_it), false);
    }

    void append_a_derefed_variable (const IDebugger::VariableSafePtr a_var)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (tree_view && tree_store);
        THROW_IF_FAIL (a_var && a_var->is_dereferenced ());

        Gtk::TreeModel::iterator parent_row_it;
        get_derefed_variables_row_iterator (parent_row_it);
        THROW_IF_FAIL (parent_row_it);
        vutil::append_a_variable (a_var->get_dereferenced (),
                                  *tree_view,
                                  tree_store,
                                  parent_row_it);
    }

    void update_a_local_variable (const IDebugger::VariableSafePtr a_var)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (tree_view);

        Gtk::TreeModel::iterator parent_row_it;
        get_local_variables_row_iterator (parent_row_it);
        vutil::update_a_variable (a_var, *tree_view,
                                  parent_row_it,
                                  true, false);
    }

    void update_a_function_argument (const IDebugger::VariableSafePtr a_var)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        THROW_IF_FAIL (tree_view);

        Gtk::TreeModel::iterator parent_row_it;
        get_function_arguments_row_iterator (parent_row_it);
        vutil::update_a_variable (a_var, *tree_view, parent_row_it,
                                  true, false);
    }

    void update_a_derefed_variable (const IDebugger::VariableSafePtr a_var)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        THROW_IF_FAIL (tree_view && a_var && a_var->is_dereferenced ());

        Gtk::TreeModel::iterator parent_row_it;
        get_derefed_variables_row_iterator (parent_row_it);
        vutil::update_a_variable (a_var->get_dereferenced (),
                                  *tree_view, parent_row_it,
                                  true, false);
    }

    void popup_context_menu (GdkEventButton *a_event)
    {
        THROW_IF_FAIL (a_event);
        THROW_IF_FAIL (tree_view);

        Gtk::Menu *menu = dynamic_cast<Gtk::Menu*> (get_context_menu ());
        THROW_IF_FAIL (menu);

        //only pop up a menu if a row exists at that position
        Gtk::TreeModel::Path path;
        Gtk::TreeViewColumn* column=NULL;
        int cell_x=0, cell_y=0;
        if (tree_view->get_path_at_pos (static_cast<int> (a_event->x),
                                        static_cast<int> (a_event->y),
                                        path,
                                        column,
                                        cell_x,
                                        cell_y)) {
            menu->popup (a_event->button, a_event->time);
        }
    }

    Gtk::Widget* get_context_menu ()
    {
        if (!context_menu) {
            context_menu = load_menu ("varinspectorpopup.xml",
                                      "/VarInspectorPopup");
            THROW_IF_FAIL (context_menu);
        }

            return context_menu;
    }

    Gtk::Widget* load_menu (UString a_filename, UString a_widget_name)
    {
        NEMIVER_TRY

        string relative_path = Glib::build_filename ("menus", a_filename);
        string absolute_path;
        THROW_IF_FAIL (perspective.build_absolute_resource_path
                (Glib::locale_to_utf8 (relative_path),
                 absolute_path));

        workbench.get_ui_manager ()->add_ui_from_file
            (Glib::locale_to_utf8 (absolute_path));

        NEMIVER_CATCH

        return workbench.get_ui_manager ()->get_widget (a_widget_name);
    }

    void dereference_pointer_action ()
    {
        if (!cur_selected_row) {
            LOG_ERROR ("no row was selected");
            return;
        }
        THROW_IF_FAIL (debugger);
        IDebugger::VariableSafePtr variable =
            (IDebugger::VariableSafePtr) cur_selected_row->get_value
                                (vutil::get_variable_columns ().variable);
        if (!variable) {
            LOG_ERROR ("got null variable from selected row!");
            return;
        }
        debugger->dereference_variable (variable);
    }

    //****************************
    //<debugger signal handlers>
    //****************************
    void on_stopped_signal (IDebugger::StopReason a_reason,
                            bool a_has_frame,
                            const IDebugger::Frame &a_frame,
                            int a_thread_id,
                            const UString &a_cookie)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        if (a_frame.line () || a_thread_id || a_cookie.empty ()) {}

        NEMIVER_TRY

        LOG_DD ("stopped, reason: " << a_reason);
        if (a_reason == IDebugger::EXITED_SIGNALLED
            || a_reason == IDebugger::EXITED_NORMALLY
            || a_reason == IDebugger::EXITED) {
            return;
        }

        THROW_IF_FAIL (debugger);
        if (a_has_frame) {
            LOG_DD ("prev frame address: '"
                    << previous_function_name
                    << "'");
            LOG_DD ("cur frame address: "
                    << a_frame.function_name ()
                    << "'");
            if (previous_function_name == a_frame.function_name ()) {
                is_new_frame = false;
            } else {
                is_new_frame = true;
            }
            THROW_IF_FAIL (tree_store);
            if (is_new_frame) {
                LOG_DD ("init tree view");
                re_init_tree_view ();
                debugger->list_local_variables ();
            } else {
                IVarListWalkerSafePtr walker_list =
                                    get_local_vars_walker_list ();
                THROW_IF_FAIL (walker_list);
                walker_list->do_walk_variables ();
                walker_list = get_function_args_vars_walker_list ();
                THROW_IF_FAIL (walker_list);
                walker_list->do_walk_variables ();
                walker_list = get_derefed_variables_walker_list ();
                THROW_IF_FAIL (walker_list);
                walker_list->do_walk_variables ();
            }
            previous_function_name = a_frame.function_name ();
        }

        NEMIVER_CATCH
    }

    void on_local_variables_listed_signal
                            (const list<IDebugger::VariableSafePtr> &a_vars,
                             const UString &a_cookie)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        if (a_cookie == "") {}

        NEMIVER_TRY

        IVarListWalkerSafePtr walker_list = get_local_vars_walker_list ();
        THROW_IF_FAIL (walker_list);
        walker_list->remove_variables ();
        walker_list->append_variables (a_vars);
        walker_list->do_walk_variables ();
        NEMIVER_CATCH
    }

    void on_frames_params_listed_signal
        (const map<int, list<IDebugger::VariableSafePtr> >&a_frames_params,
         const UString &a_cookie)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        if (a_cookie == "") {}

        NEMIVER_TRY

        IVarListWalkerSafePtr walker_list =
                                    get_function_args_vars_walker_list ();
        THROW_IF_FAIL (walker_list);

        map<int, list<IDebugger::VariableSafePtr> >::const_iterator it;
        it = a_frames_params.find (0);
        if (it == a_frames_params.end ()) {return;}

        walker_list->remove_variables ();
        walker_list->append_variables (it->second);
        walker_list->do_walk_variables ();

        NEMIVER_CATCH
    }

    void on_variable_dereferenced_signal
                                    (const IDebugger::VariableSafePtr a_var,
                                     const UString &a_cookie)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        if (a_cookie.empty ()) {/*keep compiler happy*/}

        NEMIVER_TRY

        THROW_IF_FAIL (a_var);

        IVarListWalkerSafePtr walker_list =
                                    get_derefed_variables_walker_list ();
        THROW_IF_FAIL (walker_list);
        walker_list->append_variable (a_var);
        UString qname;
        a_var->build_qname (qname);
        THROW_IF_FAIL (walker_list->do_walk_variable (qname))

        NEMIVER_CATCH
    }

    //****************************
    //</debugger signal handlers>
    //****************************

    void on_local_variable_visited_signal (const IVarWalkerSafePtr a_walker)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        NEMIVER_TRY

        THROW_IF_FAIL (a_walker->get_variable ());

        if (is_new_frame) {
            LOG_DD ("going to append: "
                    << a_walker->get_variable ()->name ());
            append_a_local_variable (a_walker->get_variable ());
        } else {
            LOG_DD ("going to update: "
                    << a_walker->get_variable ()->name ());
            update_a_local_variable (a_walker->get_variable ());
        }

        NEMIVER_CATCH
    }

    void on_func_arg_visited_signal (const IVarWalkerSafePtr a_walker)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        NEMIVER_TRY

        THROW_IF_FAIL (a_walker->get_variable ());

        if (is_new_frame) {
            LOG_DD ("appending an argument to substree");
            append_a_function_argument (a_walker->get_variable ());
        } else {
            if (is_function_arguments_subtree_empty ()) {
                LOG_DD ("appending an argument to substree");
                append_a_function_argument (a_walker->get_variable ());
            } else {
                LOG_DD ("updating an argument in substree");
                update_a_function_argument (a_walker->get_variable ());
            }
        }

        NEMIVER_CATCH
    }

    void on_derefed_variable_visited_signal
                                        (const IVarWalkerSafePtr a_walker)

    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        NEMIVER_TRY

        THROW_IF_FAIL (a_walker->get_variable ());

        //if the derefered variable has been already
        //added to the derefed variables node of the treeview, update it.
        //Otherwise, append it to the derefed variables node of the
        //treeview.
        Gtk::TreeModel::iterator parent_row_it, row_it;
        get_derefed_variables_row_iterator (parent_row_it);
        THROW_IF_FAIL (parent_row_it);
        IDebugger::VariableSafePtr var = a_walker->get_variable ();
        if (vutil::find_a_variable (var, parent_row_it, row_it)) {
            update_a_derefed_variable (var);
        } else {
            append_a_derefed_variable (var);
        }

        NEMIVER_CATCH
    }

    void on_tree_view_selection_changed_signal ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        NEMIVER_TRY

        THROW_IF_FAIL (tree_view) ;
        Glib::RefPtr<Gtk::TreeSelection> sel = tree_view->get_selection () ;
        THROW_IF_FAIL (sel) ;
        cur_selected_row = sel->get_selected () ;

        NEMIVER_CATCH
    }

    void on_tree_view_row_expanded_signal
                                    (const Gtk::TreeModel::iterator &a_it,
                                     const Gtk::TreeModel::Path &a_path)
    {
        NEMIVER_TRY

        if (a_it) {}
        if (a_path.size ()) {}

        NEMIVER_CATCH
    }

    void on_tree_view_row_activated_signal
                                    (const Gtk::TreeModel::Path &a_path,
                                     Gtk::TreeViewColumn *a_col)
    {
        NEMIVER_TRY

        THROW_IF_FAIL (tree_store);
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

    void on_button_press_signal (GdkEventButton *a_event)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        NEMIVER_TRY
        if ((a_event->type == GDK_BUTTON_PRESS) && (a_event->button == 3)) {
            popup_context_menu (a_event);
        }
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
                cur_selected_row->get_value
                        (vutil::get_variable_columns ().variable);
        THROW_IF_FAIL (variable);
        //message += "\nDumped for debug: \n";
        //variable->to_string (message, false);
        ui_utils::display_info (message);
    }

};//end LocalVarInspector2::Priv

LocalVarsInspector2::LocalVarsInspector2 (IDebuggerSafePtr &a_debugger,
                                          IWorkbench &a_workbench,
                                          IPerspective &a_perspective)
{
    m_priv.reset (new Priv (a_debugger, a_workbench, a_perspective));
}

LocalVarsInspector2::~LocalVarsInspector2 ()
{
    LOG_D ("deleted", "destructor-domain");
}

Gtk::Widget&
LocalVarsInspector2::widget () const
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->tree_view);
    return *m_priv->tree_view;
}

void
LocalVarsInspector2::show_local_variables_of_current_function ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->debugger);

    re_init_widget ();
    m_priv->debugger->list_local_variables ();
    m_priv->debugger->list_frames_arguments ();
}

void
LocalVarsInspector2::re_init_widget ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);

    m_priv->re_init_tree_view ();
}

NEMIVER_END_NAMESPACE (nemiver)

#endif //WITH_VARIABLE_WALKER

