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

#include <glib/gi18n.h>
#include <sstream>
#include <gtkmm/treestore.h>
#include "common/nmv-exception.h"
#include "common/nmv-dynamic-module.h"
#include "nmv-var-inspector.h"
#include "nmv-variables-utils.h"
#include "nmv-i-var-walker.h"
#include "nmv-ui-utils.h"
#include "nmv-vars-treeview.h"
#include "nmv-debugger-utils.h"

namespace uutil = nemiver::ui_utils;
namespace vutil = nemiver::variables_utils2;
namespace dutil = nemiver::debugger_utils;
namespace cmn = nemiver::common;

using cmn::DynamicModuleManager;

NEMIVER_BEGIN_NAMESPACE (nemiver)

class VarInspector::Priv : public sigc::trackable {
    friend class VarInspector;
    Priv ();

    bool requested_variable;
    bool requested_type;
    bool expand_variable;
    bool re_visualize;
    bool enable_contextual_menu;
    IDebuggerSafePtr debugger;
    // Variable that is being inspected
    // at a given point in time
    IDebugger::VariableSafePtr variable;
    IPerspective &perspective;
    VarsTreeView *tree_view;
    Glib::RefPtr<Gtk::TreeStore> tree_store;
    Gtk::TreeModel::iterator var_row_it;
    Gtk::TreeModel::iterator cur_selected_row;
    Glib::RefPtr<Gtk::ActionGroup> var_inspector_action_group;
    Gtk::Widget *var_inspector_menu;
    IVarWalkerSafePtr varobj_walker;
    DynamicModuleManager *module_manager;
    Glib::RefPtr<Gtk::UIManager> ui_manager;

    void
    build_widget ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        tree_view = Gtk::manage (VarsTreeView::create ());
        THROW_IF_FAIL (tree_view);
        tree_store = tree_view->get_tree_store ();
        THROW_IF_FAIL (tree_store);
        init_actions ();
    }

    void
    connect_to_signals ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        Glib::RefPtr<Gtk::TreeSelection> selection =
                                        tree_view->get_selection ();
        THROW_IF_FAIL (selection);
        selection->signal_changed ().connect
            (sigc::mem_fun (*this,
                            &Priv::on_tree_view_selection_changed_signal));
        tree_view->signal_row_activated ().connect
            (sigc::mem_fun (*this, &Priv::on_tree_view_row_activated_signal));

        tree_view->signal_row_expanded ().connect
            (sigc::mem_fun (*this, &Priv::on_tree_view_row_expanded_signal));

        tree_view->signal_button_press_event ().connect_notify
            (sigc::mem_fun (this, &Priv::on_button_press_signal));

        Gtk::CellRenderer *r = tree_view->get_column_cell_renderer
            (VarsTreeView::VARIABLE_VALUE_COLUMN_INDEX);
        THROW_IF_FAIL (r);

        Gtk::CellRendererText *t =
            dynamic_cast<Gtk::CellRendererText*> (r);
        t->signal_edited ().connect (sigc::mem_fun
                                     (*this, &Priv::on_cell_edited_signal));
    }

    void
    re_init_tree_view ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        THROW_IF_FAIL (tree_store);
        tree_store->clear ();
    }

    void init_actions ()
    {
        ui_utils::ActionEntry s_var_inspector_action_entries [] = {
            {
                "CopyVariablePathMenuItemAction",
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
            }
        };

        var_inspector_action_group =
            Gtk::ActionGroup::create ("var-inspector-action-group");
        var_inspector_action_group->set_sensitive (true);
        int num_actions =
            sizeof (s_var_inspector_action_entries)
                /
            sizeof (ui_utils::ActionEntry);

        ui_utils::add_action_entries_to_action_group
            (s_var_inspector_action_entries,
             num_actions,
             var_inspector_action_group);

        get_ui_manager ()->insert_action_group (var_inspector_action_group);
    }

    // If the variable we are inspected was created
    // with a backend counterpart (variable objects for GDB),
    // instruct the backend to delete its variable counterpart.
    void
    delete_variable_if_needed ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        if (variable
            && !variable->internal_name ().empty ()
            && debugger) {
            debugger->delete_variable (variable);
        }
    }

    void
    graphically_set_variable (const IDebugger::VariableSafePtr a_variable,
                              bool a_expand)
    {
        Gtk::TreeModel::iterator parent_iter =
            tree_store->children ().begin ();
        Gtk::TreeModel::iterator var_row;
        vutil::append_a_variable (a_variable,
                                  *tree_view,
                                  tree_store,
                                  parent_iter,
                                  var_row,
                                  true /* Do truncate type */);
        LOG_DD ("set variable" << a_variable->name ());

        // If the variable has children, unfold it so that we can see them.
        if (a_expand
            && var_row
            && (a_variable->members ().size ()
                || a_variable->needs_unfolding ()))
            tree_view->expand_row (tree_store->get_path (var_row), false);
        variable = a_variable;
    }

    void
    set_variable (const IDebugger::VariableSafePtr a_variable,
                  bool a_expand,
                  bool a_re_visualize)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (tree_view && tree_store);

        re_visualize = a_re_visualize;

        re_init_tree_view ();
        delete_variable_if_needed ();
        variable = a_variable;
        if (a_re_visualize) {
            debugger->revisualize_variable (a_variable,
                                            sigc::bind
                                            (sigc::mem_fun
                                             (*this,
                                              &Priv::on_var_revisualized),
                                             a_expand));
        } else {
            graphically_set_variable (a_variable, a_expand);
        }
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
        // message += "\nDumped for debug: \n";
        // variable->to_string (message, false);
        ui_utils::display_info (message);
    }

    void
    create_variable (const UString &a_name,
                     bool a_expand)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        expand_variable = a_expand;
        debugger->create_variable
            (a_name, sigc::mem_fun
                    (this, &VarInspector::Priv::on_variable_created_signal));
    }

    Glib::RefPtr<Gtk::UIManager> get_ui_manager ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        if (!ui_manager) {
            ui_manager = Gtk::UIManager::create ();
        }
        return ui_manager;
    }

    Gtk::Widget* get_var_inspector_menu ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        if (!var_inspector_menu) {
            string relative_path =
                Glib::build_filename ("menus", "varinspectorpopup.xml");
            string absolute_path;
            THROW_IF_FAIL (perspective.build_absolute_resource_path
                                                (relative_path, absolute_path));
            get_ui_manager ()->add_ui_from_file (absolute_path);
            get_ui_manager ()->ensure_update ();
            var_inspector_menu =
                get_ui_manager ()->get_widget ("/VarInspectorPopup");
        }
        return var_inspector_menu;
    }

    void
    popup_var_inspector_menu (GdkEventButton *a_event)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        Gtk::Menu *menu =
            dynamic_cast<Gtk::Menu*> (get_var_inspector_menu ());
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

    // ******************
    // <signal handlers>
    // ******************

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

    void
    on_var_revisualized (const IDebugger::VariableSafePtr a_var,
                         bool a_expand)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        NEMIVER_TRY;

        graphically_set_variable (a_var, a_expand);

        NEMIVER_CATCH;
    }

    void
    on_tree_view_selection_changed_signal ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        NEMIVER_TRY

        THROW_IF_FAIL (tree_view);
        Glib::RefPtr<Gtk::TreeSelection> sel = tree_view->get_selection ();
        THROW_IF_FAIL (sel);
        cur_selected_row = sel->get_selected ();
        if (!cur_selected_row) {return;}
        IDebugger::VariableSafePtr var =
            (IDebugger::VariableSafePtr)cur_selected_row->get_value
                                    (vutil::get_variable_columns ().variable);
        if (!var)
            return;

        variable = var;

        // If the variable should be editable, set the cell of the variable value
        // editable.
        cur_selected_row->set_value
                    (vutil::get_variable_columns ().variable_value_editable,
                     debugger->is_variable_editable (variable));

        // Dump some log about the variable that got selected.
        UString qname;
        variable->build_qname (qname);
        LOG_DD ("row of variable '" << qname << "'");

        NEMIVER_CATCH
    }

    void
    on_tree_view_row_activated_signal (const Gtk::TreeModel::Path &a_path,
                                            Gtk::TreeViewColumn *a_col)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

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
    on_tree_view_row_expanded_signal (const Gtk::TreeModel::iterator &a_row_it,
                                      const Gtk::TreeModel::Path &a_row_path)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        NEMIVER_TRY

        if (!(*a_row_it)[vutil::get_variable_columns ().needs_unfolding]) {
            return;
        }
        LOG_DD ("The variable needs unfolding");

        IDebugger::VariableSafePtr var =
            (*a_row_it)[vutil::get_variable_columns ().variable];
        debugger->unfold_variable
        (var, sigc::bind (sigc::mem_fun (*this,
                                         &Priv::on_variable_unfolded_signal),
                          a_row_path));
        LOG_DD ("variable unfolding triggered");

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

    void on_button_press_signal (GdkEventButton *a_event)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        NEMIVER_TRY

        // right-clicking should pop up a context menu
        if (a_event->type == GDK_BUTTON_PRESS
            && a_event->button == 3
            && enable_contextual_menu) {
            popup_var_inspector_menu (a_event);
        }

        NEMIVER_CATCH
    }

    void
    on_variable_created_signal (const IDebugger::VariableSafePtr a_var)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        NEMIVER_TRY;

        set_variable (a_var, expand_variable, re_visualize);

        NEMIVER_CATCH;
    }

    void
    on_variable_unfolded_signal (const IDebugger::VariableSafePtr a_var,
                                 const Gtk::TreeModel::Path &a_var_node)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        NEMIVER_TRY

        Gtk::TreeModel::iterator var_it = tree_store->get_iter (a_var_node);
        vutil::update_unfolded_variable (a_var, *tree_view,
                                         tree_store,
                                         var_it,
                                         true /* Do truncate type */);
        tree_view->expand_row (a_var_node, false);

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
                                       true /* do truncate types */,
                                       false /* don't handle highlight ... */ ,
                                       false /* ... really */);

        NEMIVER_CATCH
    }

    void on_variable_path_expr_copy_to_clipboard_action ()
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

    void on_variable_value_copy_to_clipboard_action ()
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

    void on_variable_path_expression_signal
                                    (const IDebugger::VariableSafePtr a_var)
    {
        NEMIVER_TRY

        Gtk::Clipboard::get ()->set_text (a_var->path_expression ());

        NEMIVER_CATCH
    }
    // ******************
    // </signal handlers>
    // ******************

public:

    Priv (IDebuggerSafePtr a_debugger,
          IPerspective &a_perspective) :
          requested_variable (false),
          requested_type (false),
          expand_variable (false),
          re_visualize (false),
          enable_contextual_menu (false),
          debugger (a_debugger),
          perspective (a_perspective),
          tree_view (0),
          var_inspector_menu (0),
          module_manager (0)
    {
        build_widget ();
        re_init_tree_view ();
        connect_to_signals ();
    }

    ~Priv ()
    {
        delete_variable_if_needed ();
    }
};//end class VarInspector::Priv

VarInspector::VarInspector (IDebuggerSafePtr a_debugger,
                            IPerspective &a_perspective)
{
    m_priv.reset (new Priv (a_debugger, a_perspective));
}

VarInspector::~VarInspector ()
{
    LOG_D ("deleted", "destructor-domain");
}

Gtk::Widget&
VarInspector::widget () const
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->tree_view);
    return *m_priv->tree_view;
}

void
VarInspector::set_variable (IDebugger::VariableSafePtr a_variable,
                            bool a_expand, bool a_revisualize)
{
    THROW_IF_FAIL (m_priv);

    m_priv->set_variable (a_variable, a_expand, a_revisualize);
}

void
VarInspector::inspect_variable (const UString &a_variable_name,
                                 bool a_expand)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    if (a_variable_name == "") {return;}
    THROW_IF_FAIL (m_priv);
    m_priv->re_init_tree_view ();
    m_priv->delete_variable_if_needed ();
    m_priv->create_variable (a_variable_name, a_expand);
}

IDebugger::VariableSafePtr
VarInspector::get_variable () const
{
    THROW_IF_FAIL (m_priv);

    return m_priv->variable;
}

void
VarInspector::enable_contextual_menu (bool a_flag)
{
    THROW_IF_FAIL (m_priv);
    m_priv->enable_contextual_menu = a_flag;
}

bool
VarInspector::is_contextual_menu_enabled () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->enable_contextual_menu;
}

void
VarInspector::clear ()
{
    THROW_IF_FAIL (m_priv);
    m_priv->re_init_tree_view ();
    m_priv->delete_variable_if_needed ();
}

NEMIVER_END_NAMESPACE (nemiver)

