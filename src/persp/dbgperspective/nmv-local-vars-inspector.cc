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
#include <gtkmm/treestore.h>
#include <gtkmm/treerowreference.h>
#include "common/nmv-exception.h"
#include "nmv-local-vars-inspector.h"
#include "nmv-variables-utils.h"
#include "nmv-ui-utils.h"
#include "nmv-i-workbench.h"
#include "nmv-i-var-walker.h"
#include "nmv-vars-treeview.h"
#include "nmv-debugger-utils.h"

using namespace nemiver::common;
namespace vutil = nemiver::variables_utils2;
namespace dutil = nemiver::debugger_utils;
using Glib::RefPtr;

NEMIVER_BEGIN_NAMESPACE (nemiver)

struct LocalVarsInspector::Priv : public sigc::trackable {
private:
    Priv ();
public:
    IDebuggerSafePtr debugger;
    IWorkbench &workbench;
    IPerspective &perspective;
    VarsTreeView *tree_view;
    Glib::RefPtr<Gtk::TreeStore> tree_store;
    Gtk::TreeModel::iterator cur_selected_row;
    SafePtr<Gtk::TreeRowReference> local_variables_row_ref;
    SafePtr<Gtk::TreeRowReference> function_arguments_row_ref;
    IDebugger::VariableList local_vars;
    IDebugger::VariableList function_arguments;
    UString previous_function_name;
    Glib::RefPtr<Gtk::ActionGroup> local_vars_inspector_action_group;
    bool is_new_frame;
    bool is_up2date;
    IDebugger::StopReason saved_reason;
    bool saved_has_frame;
    IDebugger::Frame saved_frame;
    // The list of variables that changed at the previous stop
    // Those were probably highlighted in red.
    // We need to keep track of them
    // to make them look black again.
    //
    IDebugger::VariableList local_vars_changed_at_prev_stop;
    IDebugger::VariableList func_args_changed_at_prev_stop;
    Gtk::Widget* local_vars_inspector_menu;
    IVarWalkerSafePtr varobj_walker;
    DynamicModuleManager *module_manager;
    Glib::RefPtr<Gtk::UIManager> ui_manager;

    Priv (IDebuggerSafePtr &a_debugger,
          IWorkbench &a_workbench,
          IPerspective& a_perspective) :
        workbench (a_workbench),
        perspective (a_perspective),
        tree_view (Gtk::manage (VarsTreeView::create ())),
        is_new_frame (false),
        is_up2date (true),
        saved_reason (IDebugger::UNDEFINED_REASON),
        saved_has_frame (false),
        local_vars_inspector_menu (0),
        varobj_walker (0),
        module_manager (0)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        THROW_IF_FAIL (a_debugger);
        debugger = a_debugger;
        THROW_IF_FAIL (tree_view);
        tree_store = tree_view->get_tree_store ();
        THROW_IF_FAIL (tree_store);
        re_init_tree_view ();
        connect_to_debugger_signals ();
        init_graphical_signals ();
        init_actions ();
    }

    void
    re_init_tree_view ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (tree_store);
        clear_local_variables ();
        clear_function_arguments ();
        tree_store->clear ();
        previous_function_name = "";
        is_new_frame = true;

        //****************************************************
        //add two rows: local variables and function arguments,
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
    }

    bool
    get_function_arguments_row_iterator
                                    (Gtk::TreeModel::iterator &a_it) const
    {
        if (!function_arguments_row_ref) {
            LOG_DD ("There is no function arg row iter yet");
            return false;
        }
        a_it = tree_store->get_iter
                            (function_arguments_row_ref->get_path ());
        LOG_DD ("Returned function arg row iter OK");
        return true;
    }

    bool
    should_process_now ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        THROW_IF_FAIL (tree_view);
        bool is_visible = tree_view->is_drawable ();
        LOG_DD ("is visible: " << is_visible);
        return is_visible;
    }

    bool
    is_function_arguments_subtree_empty () const
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        Gtk::TreeModel::iterator it;

        if (!get_function_arguments_row_iterator (it)) {
            return true;
        }
        return it->children ().empty ();
    }

    bool
    get_local_variables_row_iterator (Gtk::TreeModel::iterator &a_it)
    {
        if (!local_variables_row_ref) {
            LOG_DD ("there is no variables row iter yet");
            return false;
        }
        a_it = tree_store->get_iter (local_variables_row_ref->get_path ());
        LOG_DD ("returned local variables row iter, OK.");
        return true;
    }

    /// Connect callback slots to the relevant signals of the
    /// debugging engine.k
    void
    connect_to_debugger_signals ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (debugger);
        debugger->stopped_signal ().connect
            (sigc::mem_fun (*this, &Priv::on_stopped_signal));
        debugger->local_variables_listed_signal ().connect
            (sigc::mem_fun (*this, &Priv::on_local_variables_listed_signal));
    }

    void
    init_graphical_signals ()
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
        tree_view->signal_expose_event ().connect_notify
            (sigc::mem_fun (this, &Priv::on_expose_event_signal));

        Gtk::CellRenderer *r = tree_view->get_column_cell_renderer
            (VarsTreeView::VARIABLE_VALUE_COLUMN_INDEX);
        THROW_IF_FAIL (r);

        Gtk::CellRendererText *t =
            dynamic_cast<Gtk::CellRendererText*> (r);
        t->signal_edited ().connect (sigc::mem_fun
                                     (*this, &Priv::on_cell_edited_signal));
    }

    void
    init_actions ()
    {
        ui_utils::ActionEntry s_local_vars_inspector_action_entries [] = {
            {
                "CopyLocalVariablePathMenuItemAction",
                Gtk::Stock::COPY,
                _("_Copy variable name"),
                _("Copy the variable path expression to the clipboard"),
                sigc::mem_fun
                    (*this,
                     &Priv::on_variable_path_expr_copy_to_clipboard_action),
                ui_utils::ActionEntry::DEFAULT,
                "",
                false
            },
            {
                "CopyVariableValueMenuItemAction",
                Gtk::Stock::COPY,
                _("_Copy variable value"),
                _("Copy the variable value to the clipboard"),
                sigc::mem_fun
                    (*this,
                     &Priv::on_variable_value_copy_to_clipboard_action),
                ui_utils::ActionEntry::DEFAULT,
                "",
                false
            },
            {
                "CreateWatchpointMenuItemAction",
                Gtk::Stock::COPY,
                _("Create watchpoint"),
                _("Create a watchpoint that triggers when the value "
                  "of the expression changes"),
                sigc::mem_fun
                    (*this,
                     &Priv::on_create_watchpoint_action),
                ui_utils::ActionEntry::DEFAULT,
                "",
                false
            },
        };

        local_vars_inspector_action_group =
            Gtk::ActionGroup::create ("local-vars-inspector-action-group");
        local_vars_inspector_action_group->set_sensitive (true);
        int num_actions =
            sizeof (s_local_vars_inspector_action_entries)
                /
            sizeof (ui_utils::ActionEntry);

        ui_utils::add_action_entries_to_action_group
            (s_local_vars_inspector_action_entries,
             num_actions,
             local_vars_inspector_action_group);

        get_ui_manager ()->insert_action_group
                                        (local_vars_inspector_action_group);
    }

    void
    set_local_variables
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

    void
    set_function_arguments
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

    void
    clear_local_variables ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (tree_store);
        Gtk::TreeModel::iterator row_it;
        if (get_local_variables_row_iterator (row_it)) {
            Gtk::TreeModel::Children rows = row_it->children ();
            for (row_it = rows.begin (); row_it != rows.end ();) {
                row_it = tree_store->erase (row_it);
            }
        }
        delete_vars_backend_peers (local_vars);
        local_vars.clear ();
        local_vars_changed_at_prev_stop.clear ();
    }

    void
    clear_function_arguments ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (tree_store);
        Gtk::TreeModel::iterator row_it;
        if (get_function_arguments_row_iterator (row_it)) {
            Gtk::TreeModel::Children rows = row_it->children ();
            for (row_it = rows.begin (); row_it != rows.end ();) {
                row_it = tree_store->erase (*row_it);
            }
        }
        delete_vars_backend_peers (function_arguments);
        function_arguments.clear ();
        func_args_changed_at_prev_stop.clear ();
    }

    void
    append_a_local_variable (const IDebugger::VariableSafePtr a_var)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (tree_view && tree_store);

        Gtk::TreeModel::iterator parent_row_it;
        if (get_local_variables_row_iterator (parent_row_it)) {
            vutil::append_a_variable (a_var,
                                      *tree_view,
                                      tree_store,
                                      parent_row_it,
                                      false /* do not truncate types */);
            tree_view->expand_row (tree_store->get_path (parent_row_it), false);
            local_vars.push_back (a_var);
        }
    }

    void
    append_a_function_argument (const IDebugger::VariableSafePtr a_var)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (tree_view && tree_store);

        Gtk::TreeModel::iterator parent_row_it;
        if (get_function_arguments_row_iterator (parent_row_it)) {
            vutil::append_a_variable (a_var,
                                      *tree_view,
                                      tree_store,
                                      parent_row_it,
                                      false /* do not truncate type */);
            tree_view->expand_row (tree_store->get_path (parent_row_it), false);
            function_arguments.push_back (a_var);
        }
    }

    void
    update_a_local_variable (const IDebugger::VariableSafePtr a_var,
                             bool a_update_members = true)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (tree_view);

        Gtk::TreeModel::iterator parent_row_it;
        if (get_local_variables_row_iterator (parent_row_it)) {
            vutil::update_a_variable (a_var, *tree_view,
                                      parent_row_it,
                                      false /* do not truncate type */,
                                      true /* handle highlight */,
                                      false /* is not a new frame */,
                                      a_update_members);
        }
    }

    /// \return true if the variable was found in the TreeModel, false
    ///  otherwise. If the variable was found, the function updates it.
    bool
    update_a_function_argument (const IDebugger::VariableSafePtr a_var)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        THROW_IF_FAIL (tree_view);

        Gtk::TreeModel::iterator parent_row_it;
        if (get_function_arguments_row_iterator (parent_row_it)) {
            return vutil::update_a_variable (a_var,
                                             *tree_view,
                                             parent_row_it,
                                             false /* do not truncate type */,
                                             true /* handle highlight */,
                                             false /* is not a new frame */);
        }
        return false;
    }

    void
    finish_handling_debugger_stopped_event
                                    (IDebugger::StopReason /*a_reason*/,
                                     bool a_has_frame,
                                     const IDebugger::Frame &a_frame)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        NEMIVER_TRY

        THROW_IF_FAIL (tree_store);

        if (!a_has_frame)
            return;

        saved_frame = a_frame;

        if (is_new_frame) {
            LOG_DD ("init tree view");
            re_init_tree_view ();
            LOG_DD ("list local variables");
            debugger->list_local_variables ();
            LOG_DD ("list frames arguments");
            debugger->list_frames_arguments (a_frame.level (),
                                             a_frame.level (),
                                             sigc::mem_fun
                                             (*this, &Priv::on_function_args_listed),
                                             "");
        } else {
            LOG_DD ("update local variables and function arguments");
            update_local_variables ();
            update_function_arguments ();
        }
        previous_function_name = a_frame.function_name ();

        NEMIVER_CATCH
    }

    void
    show_variable_type_in_dialog ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        if (!cur_selected_row) {return;}
        UString type =
            (Glib::ustring)
                    (*cur_selected_row)[vutil::get_variable_columns ().type];
        UString message;
        message.printf (_("Variable type is: \n %s"), type.c_str ());

        IDebugger::VariableSafePtr variable =
            (IDebugger::VariableSafePtr)
                cur_selected_row->get_value
                        (vutil::get_variable_columns ().variable);
        THROW_IF_FAIL (variable);
        ui_utils::display_info (message);
    }

    void
    update_local_variables ()
    {
        IDebugger::VariableList::const_iterator var_it;
        if (!is_new_frame) {
            // During the previous local variables update, some
            // variables whose value change were highlited.
            // At this point in time, we need to un-highlight them back
            // in case their value haven't change during this
            // update.
            for (var_it = local_vars_changed_at_prev_stop.begin ();
                 var_it != local_vars_changed_at_prev_stop.end ();
                 ++var_it) {
                update_a_local_variable (*var_it);
            }
            local_vars_changed_at_prev_stop.clear ();
        }
        for (IDebugger::VariableList::const_iterator it = local_vars.begin ();
             it != local_vars.end ();
             ++it) {
            debugger->list_changed_variables
                    (*it,
                     sigc::mem_fun (*this,
                                    &Priv::on_local_variable_updated_signal));
        }

    }

    void
    update_function_arguments ()
    {
        IDebugger::VariableList::const_iterator var_it;
        if (!is_new_frame) {
            // During the previous function argument update, some
            // arguments whose value change were highlighted.
            // At this point in time, we need to un-highlight them back
            // in case their value haven't change during this
            // update.
            for (var_it = func_args_changed_at_prev_stop.begin ();
                 var_it != func_args_changed_at_prev_stop.end ();
                 ++var_it) {
                update_a_function_argument (*var_it);
            }
            func_args_changed_at_prev_stop.clear ();
        }
        for (IDebugger::VariableList::const_iterator it =
                                            function_arguments.begin ();
             it != function_arguments.end ();
             ++it) {
            debugger->list_changed_variables
                    (*it,
                     sigc::mem_fun (*this,
                                    &Priv::on_function_args_updated_signal));
        }
    }

    void
    delete_vars_backend_peers (IDebugger::VariableList &a_vars)
    {
        for (IDebugger::VariableList::const_iterator it = a_vars.begin ();
             it != a_vars.end ();
             ++it) {
            if (!(*it) || (*it)->internal_name ().empty ()) {
                continue;
            }
            debugger->delete_variable (*it);
        }
    }

    Glib::RefPtr<Gtk::UIManager>
    get_ui_manager ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        if (!ui_manager) {
            ui_manager = Gtk::UIManager::create ();
        }
        return ui_manager;
    }

    Gtk::Widget*
    get_local_vars_inspector_menu ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        if (!local_vars_inspector_menu) {
            string relative_path =
                Glib::build_filename ("menus", "localvarsinspectorpopup.xml");
            string absolute_path;
            THROW_IF_FAIL (perspective.build_absolute_resource_path
                                                (relative_path, absolute_path));
            get_ui_manager ()->add_ui_from_file (absolute_path);
            get_ui_manager ()->ensure_update ();
            local_vars_inspector_menu =
                get_ui_manager ()->get_widget ("/LocalVarsInspectorPopup");
            THROW_IF_FAIL (local_vars_inspector_menu);
        }
        return local_vars_inspector_menu;
    }

    void
    popup_local_vars_inspector_menu (GdkEventButton *a_event)
    {
        Gtk::Menu *menu =
            dynamic_cast<Gtk::Menu*> (get_local_vars_inspector_menu ());
        THROW_IF_FAIL (menu);

        // only pop up a menu if a row exists at that position
        Gtk::TreeModel::Path path;
        Gtk::TreeViewColumn* column = 0;
        int cell_x = 0, cell_y = 0;
        THROW_IF_FAIL (tree_view);
        THROW_IF_FAIL (a_event);
        if (tree_view->get_path_at_pos (static_cast<int> (a_event->x),
                                        static_cast<int> (a_event->y),
                                        path,
                                        column,
                                        cell_x,
                                        cell_y)) {
            menu->popup (a_event->button, a_event->time);
        }
    }

    DynamicModuleManager*
    get_module_manager ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        if (!module_manager) {
            DynamicModule::Loader *loader =
                perspective.get_workbench ().get_dynamic_module
                                                    ().get_module_loader ();
            THROW_IF_FAIL (loader);
            module_manager = loader->get_dynamic_module_manager ();
            THROW_IF_FAIL (module_manager);
        }
        return module_manager;
    }

    IVarWalkerSafePtr
    create_varobj_walker ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        IVarWalkerSafePtr result  =
            get_module_manager ()->load_iface_with_default_manager<IVarWalker>
                                            ("varobjwalker", "IVarWalker");
        result->visited_variable_signal ().connect
            (sigc::mem_fun (*this, &Priv::on_visited_variable_signal));
        return result;
    }

    IVarWalkerSafePtr
    get_varobj_walker ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        if (!varobj_walker)
            varobj_walker = create_varobj_walker ();
        return varobj_walker;
    }

    //****************************
    //<debugger signal handlers>
    //****************************
    void
    on_stopped_signal (IDebugger::StopReason a_reason,
                       bool a_has_frame,
                       const IDebugger::Frame &a_frame,
                       int /* a_thread_id */,
                       int /* a_bp_num */,
                       const UString &/*a_cookie*/)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        NEMIVER_TRY

        LOG_DD ("stopped, reason: " << a_reason);
        if (a_reason == IDebugger::EXITED_SIGNALLED
            || a_reason == IDebugger::EXITED_NORMALLY
            || a_reason == IDebugger::EXITED) {
            return;
        }

        THROW_IF_FAIL (debugger);
        if (a_has_frame) {
            saved_frame = a_frame;
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

            if (should_process_now ()) {
                finish_handling_debugger_stopped_event (a_reason,
                                                        a_has_frame,
                                                        a_frame);
            } else {
                saved_reason = a_reason;
                saved_has_frame = a_has_frame;
                is_up2date = false;
            }
        }

        NEMIVER_CATCH
    }

    void
    on_local_variable_created_signal (const IDebugger::VariableSafePtr a_var)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        NEMIVER_TRY

        if (is_new_frame) {
            append_a_local_variable (a_var);
        } else {
            update_a_local_variable (a_var);
        }

        NEMIVER_CATCH
    }

    void
    on_local_variables_listed_signal
                            (const IDebugger::VariableList &a_vars,
                             const UString & /* a_cookie */)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        NEMIVER_TRY

        UString name;
        for (IDebugger::VariableList::const_iterator it = a_vars.begin ();
             it != a_vars.end ();
             ++it) {
            name = (*it)->name ();
            if (name.empty ()) {
                continue;
            }
            LOG_DD ("creating variable '" << name << "'");
            debugger->create_variable
                (name, sigc::mem_fun (*this,
                                      &Priv::on_local_variable_created_signal));
        }
        NEMIVER_CATCH
    }

    void
    on_local_variable_updated_signal (const IDebugger::VariableList &a_vars)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        NEMIVER_TRY

        for (IDebugger::VariableList::const_iterator it = a_vars.begin ();
             it != a_vars.end ();
             ++it) {
            // Update the local variable pointed to by it.
            // Note that we shall not recursively update the members of the
            // the *it variable (in case it has members variables itself)
            // because each member variable that needs updating is passed as
            // part of a_vars - and as a result will be updated
            // separately by a call to update_a_local_variable.
            // At least that's the current behaviour with the "GDB/MI
            // Variable Objects" based backend we are using.
            update_a_local_variable (*it,
                                     false /* do not update members */);
            local_vars_changed_at_prev_stop.push_back (*it);
        }

        NEMIVER_CATCH
    }

    // Slot of IDebugger::create_variable, for creating backend variables
    // objects for frame parameter variables.
    void
    on_function_arg_var_created_signal (const IDebugger::VariableSafePtr a_var)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        NEMIVER_TRY

        if (is_new_frame) {
            LOG_DD ("appending an argument to substree");
            append_a_function_argument (a_var);
        } else {
            if (is_function_arguments_subtree_empty ()) {
                LOG_DD ("appending an argument to substree");
                append_a_function_argument (a_var);
            } else {
                LOG_DD ("updating an argument in substree");
                if (!update_a_function_argument (a_var)) {
                    append_a_function_argument (a_var);
                }
            }
        }

        NEMIVER_CATCH
    }

    void
    on_function_args_listed (const map<int, IDebugger::VariableList> &a_args)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        NEMIVER_TRY;

        UString name;
        map<int, IDebugger::VariableList>::const_iterator frame_it;
        frame_it = a_args.find (debugger->get_current_frame_level ());
        if (frame_it == a_args.end ()) {
            LOG_DD ("Got empty frames parameters");
            return;
        }
        IDebugger::VariableList::const_iterator it;
        for (it = frame_it->second.begin ();
             it != frame_it->second.end ();
             ++it)  {
            name = (*it)->name ();
            if (name.empty ()) {
                continue;
            }
            LOG_DD ("creating variable '" << name << "'");
            debugger->create_variable
                (name,
                 sigc::mem_fun (*this,
                                &Priv::on_function_arg_var_created_signal));
        }
        NEMIVER_CATCH;
    }

    void
    on_function_args_updated_signal (const IDebugger::VariableList &a_vars)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        NEMIVER_TRY

        for (IDebugger::VariableList::const_iterator it = a_vars.begin ();
             it != a_vars.end ();
             ++it) {
            update_a_function_argument (*it);
            func_args_changed_at_prev_stop.push_back (*it);
        }

        NEMIVER_CATCH
    }

    void
    on_variable_unfolded_signal (const IDebugger::VariableSafePtr a_var,
                                 const Gtk::TreeModel::Path a_var_node)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        NEMIVER_TRY

        Gtk::TreeModel::iterator var_it = tree_store->get_iter (a_var_node);
        vutil::update_unfolded_variable (a_var,
                                         *tree_view,
                                         tree_store,
                                         var_it,
                                         false /* do not truncate type */);
        tree_view->expand_row (a_var_node, false);
        NEMIVER_CATCH
    }

    void
    on_variable_path_expression_signal (const IDebugger::VariableSafePtr a_var)
    {
        NEMIVER_TRY

        Gtk::Clipboard::get ()->set_text (a_var->path_expression ());

        NEMIVER_CATCH
    }

    void
    on_variable_path_expression_signal_set_wpt
                                        (const IDebugger::VariableSafePtr a_var)
    {
        NEMIVER_TRY

        debugger->set_watchpoint (a_var->path_expression ());

        NEMIVER_CATCH
    }

    void
    on_visited_variable_signal (const IDebugger::VariableSafePtr a_var)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        NEMIVER_TRY

        std::string str;
        dutil::dump_variable_value (a_var, 0, str);

        if (!str.empty ())
            Gtk::Clipboard::get ()->set_text (str);

        NEMIVER_CATCH
    }

    //****************************
    //</debugger signal handlers>
    //****************************

    void
    on_tree_view_selection_changed_signal ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        NEMIVER_TRY

        THROW_IF_FAIL (tree_view);
        Glib::RefPtr<Gtk::TreeSelection> sel = tree_view->get_selection ();
        THROW_IF_FAIL (sel);
        cur_selected_row = sel->get_selected ();
        if (!cur_selected_row)
            return;

        IDebugger::VariableSafePtr var =
         (*cur_selected_row)[vutil::get_variable_columns ().variable];
        if (!var)
            return;

        cur_selected_row->set_value
                    (vutil::get_variable_columns ().variable_value_editable,
                     debugger->is_variable_editable (var));

        // Dump some log about the variable that got selected.
        UString qname;
        var->build_qname (qname);
        LOG_DD ("row of variable '" << qname << "'");

        NEMIVER_CATCH
    }

    void
    on_tree_view_row_expanded_signal
                                (const Gtk::TreeModel::iterator &a_it,
                                 const Gtk::TreeModel::Path &a_path)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        NEMIVER_TRY

        if (!(*a_it)[vutil::get_variable_columns ().needs_unfolding]) {
            return;
        }
        LOG_DD ("A variable needs unfolding");

        IDebugger::VariableSafePtr var =
            (*a_it)[vutil::get_variable_columns ().variable];
        debugger->unfold_variable
            (var,
             sigc::bind  (sigc::mem_fun (*this,
                                         &Priv::on_variable_unfolded_signal),
                          a_path));

        NEMIVER_CATCH
    }

    void
    on_tree_view_row_activated_signal
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

    void
    on_button_press_signal (GdkEventButton *a_event)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        NEMIVER_TRY

        // right-clicking should pop up a context menu
        if ((a_event->type == GDK_BUTTON_PRESS) && (a_event->button == 3)) {
            popup_local_vars_inspector_menu (a_event);
        }

        NEMIVER_CATCH
    }

    void
    on_expose_event_signal (GdkEventExpose *)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        NEMIVER_TRY
        if (!is_up2date) {
            finish_handling_debugger_stopped_event (saved_reason,
                                                    saved_has_frame,
                                                    saved_frame);
            is_up2date = true;
        }
        NEMIVER_CATCH
    }

    void
    on_cell_edited_signal (const Glib::ustring &a_path,
                           const Glib::ustring &a_text)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        NEMIVER_TRY

        Gtk::TreeModel::iterator row = tree_store->get_iter (a_path);
        IDebugger::VariableSafePtr var =
            (*row)[vutil::get_variable_columns ().variable];
        THROW_IF_FAIL (var);

        debugger->assign_variable
            (var, a_text,
             sigc::bind (sigc::mem_fun
                                 (*this, &Priv::on_variable_assigned_signal),
                         a_path));

        NEMIVER_CATCH
    }

    void
    on_variable_assigned_signal (const IDebugger::VariableSafePtr a_var,
                                 const UString &a_var_path)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        NEMIVER_TRY

        Gtk::TreeModel::iterator var_row
                                = tree_store->get_iter (a_var_path);
        THROW_IF_FAIL (var_row);
        THROW_IF_FAIL (tree_view);
        vutil::update_a_variable_node (a_var, *tree_view,
                                       var_row,
                                       false /* do not truncate type */,
                                       false /* do not highlight variable */,
                                       false /* frame ain't new */);

        NEMIVER_CATCH
    }

    void
    on_variable_path_expr_copy_to_clipboard_action ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        NEMIVER_TRY

        THROW_IF_FAIL (cur_selected_row);

        IDebugger::VariableSafePtr variable =
            cur_selected_row->get_value
                (vutil::get_variable_columns ().variable);
        THROW_IF_FAIL (variable);

        debugger->query_variable_path_expr
            (variable,
             sigc::mem_fun (*this, &Priv::on_variable_path_expression_signal));

        NEMIVER_CATCH
    }

    void
    on_variable_value_copy_to_clipboard_action ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        NEMIVER_TRY

        THROW_IF_FAIL (cur_selected_row);

        IDebugger::VariableSafePtr variable =
            cur_selected_row->get_value
                (vutil::get_variable_columns ().variable);
        THROW_IF_FAIL (variable);

        IVarWalkerSafePtr walker = get_varobj_walker ();
        walker->connect (debugger, variable);
        walker->do_walk_variable ();

        NEMIVER_CATCH
    }

    void
    on_create_watchpoint_action ()
    {
        NEMIVER_TRY

        IDebugger::VariableSafePtr variable =
            cur_selected_row->get_value
                (vutil::get_variable_columns ().variable);
        THROW_IF_FAIL (variable);

        debugger->query_variable_path_expr
            (variable,
             sigc::mem_fun
                 (*this, &Priv::on_variable_path_expression_signal_set_wpt));

        NEMIVER_CATCH
    }
};//end LocalVarsInspector::Priv

LocalVarsInspector::LocalVarsInspector (IDebuggerSafePtr &a_debugger,
                                          IWorkbench &a_workbench,
                                          IPerspective &a_perspective)
{
    m_priv.reset (new Priv (a_debugger, a_workbench, a_perspective));
}

LocalVarsInspector::~LocalVarsInspector ()
{
    LOG_D ("deleted", "destructor-domain");
}

Gtk::Widget&
LocalVarsInspector::widget () const
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->tree_view);
    return *m_priv->tree_view;
}

/// List the local variables of the current function, as well as its
/// arguments.
///
/// \param a_frame the frame of the current function to act upon.
void
LocalVarsInspector::show_local_variables_of_current_function
                                            (const IDebugger::Frame &a_frame)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->debugger);

    m_priv->saved_frame = a_frame;

    re_init_widget ();
    m_priv->debugger->list_local_variables ();
    int frame_level = m_priv->debugger->get_current_frame_level ();
    LOG_DD ("current frame level: " <<  (int)frame_level);
    m_priv->debugger->list_frames_arguments (frame_level, frame_level,
                                             sigc::mem_fun
                                             (*m_priv, &Priv::on_function_args_listed),
                                             "");
}

void
LocalVarsInspector::re_init_widget ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);

    m_priv->re_init_tree_view ();
}

NEMIVER_END_NAMESPACE (nemiver)

