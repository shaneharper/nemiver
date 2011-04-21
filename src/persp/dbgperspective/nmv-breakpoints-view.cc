//Author: Dodji Seketeli, Jonathon Jongsma
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
#include <map>
#include <glib/gi18n.h>
#include <gtkmm/treeview.h>
#include <gtkmm/liststore.h>
#include <gtkmm/treerowreference.h>
#include "common/nmv-exception.h"
#include "nmv-breakpoints-view.h"
#include "nmv-ui-utils.h"
#include "nmv-i-workbench.h"
#include "nmv-i-perspective.h"
#include "nmv-debugger-utils.h"

namespace nemiver {

struct BPColumns : public Gtk::TreeModelColumnRecord {
    Gtk::TreeModelColumn<int> id;
    Gtk::TreeModelColumn<bool> enabled;
    Gtk::TreeModelColumn<Glib::ustring> address;
    Gtk::TreeModelColumn<Glib::ustring> filename;
    Gtk::TreeModelColumn<Glib::ustring> function;
    Gtk::TreeModelColumn<int> line;
    Gtk::TreeModelColumn<Glib::ustring> condition;
    Gtk::TreeModelColumn<bool> is_countpoint;
    Gtk::TreeModelColumn<Glib::ustring> type;
    Gtk::TreeModelColumn<int> hits;
    Gtk::TreeModelColumn<Glib::ustring> expression;
    Gtk::TreeModelColumn<int> ignore_count;
    Gtk::TreeModelColumn<IDebugger::Breakpoint> breakpoint;
    Gtk::TreeModelColumn<bool> is_standard;

    enum INDEXES {
        ENABLE_INDEX = 0,
        ID_INDEX,
        FILENAME_INDEX,
        LINE_INDEX,
        FUNCTION_INDEX,
        ADDRESS_INDEX,
        CONDITION_INDEX,
	IS_COUNTPOINT_INDEX,
        TYPE_INDEX,
        HITS_INDEX,
        EXPRESSION_INDEX,
        IGNORE_COUNT_INDEX
    };

    BPColumns ()
    {
        add (id);
        add (enabled);
        add (address);
        add (filename);
        add (function);
        add (line);
        add (breakpoint);
        add (condition);
	add (is_countpoint);
        add (type);
        add (hits);
        add (expression);
        add (ignore_count);
        add (is_standard);
    }
};//end Cols

static BPColumns&
get_bp_cols ()
{
    static BPColumns s_cols;
    return s_cols;
}

struct BreakpointsView::Priv {
public:
    SafePtr<Gtk::TreeView> tree_view;
    Glib::RefPtr<Gtk::ListStore> list_store;
    Gtk::Widget *breakpoints_menu;
    sigc::signal<void,
                 const IDebugger::Breakpoint&> go_to_breakpoint_signal;
    Glib::RefPtr<Gtk::ActionGroup> breakpoints_action_group;
    IWorkbench& workbench;
    IPerspective& perspective;
    IDebuggerSafePtr& debugger;
    bool is_up2date;

    Priv (IWorkbench& a_workbench,
          IPerspective& a_perspective,
          IDebuggerSafePtr& a_debugger) :
        breakpoints_menu(0),
        workbench(a_workbench),
        perspective(a_perspective),
        debugger(a_debugger),
        is_up2date (true)
    {
        init_actions ();
        build_tree_view ();

        // update breakpoint list when debugger indicates that the list of
        // breakpoints has changed.
        debugger->breakpoint_deleted_signal ().connect (sigc::mem_fun
                (*this, &Priv::on_debugger_breakpoint_deleted_signal));
        debugger->breakpoint_set_signal ().connect
            (sigc::mem_fun (*this, &Priv::on_debugger_breakpoint_set_signal));
        debugger->breakpoints_list_signal ().connect (sigc::mem_fun
                (*this, &Priv::on_debugger_breakpoints_list_signal));
        debugger->stopped_signal ().connect (sigc::mem_fun
                (*this, &Priv::on_debugger_stopped_signal));
        breakpoints_menu = load_menu ("breakpointspopup.xml",
                "/BreakpointsPopup");
    }

    void build_tree_view ()
    {
        if (tree_view) {return;}
        //create a default tree store and a tree view
        list_store = Gtk::ListStore::create (get_bp_cols ());
        tree_view.reset (new Gtk::TreeView (list_store));
        tree_view->get_selection ()->set_mode (Gtk::SELECTION_MULTIPLE);

        //create the columns of the tree view
	int nb_columns;
        tree_view->append_column_editable ("", get_bp_cols ().enabled);
        tree_view->append_column (_("ID"), get_bp_cols ().id);
        tree_view->append_column (_("Filename"), get_bp_cols ().filename);
        tree_view->append_column (_("Line"), get_bp_cols ().line);
        tree_view->append_column (_("Function"), get_bp_cols ().function);
        tree_view->append_column (_("Address"), get_bp_cols ().address);
        tree_view->append_column_editable (_("Condition"),
                                           get_bp_cols ().condition);
        tree_view->append_column_editable (_("Toggle countpoint"),
                                           get_bp_cols ().is_countpoint);
        tree_view->append_column (_("Type"), get_bp_cols ().type);
        tree_view->append_column (_("Hits"), get_bp_cols ().hits);
        tree_view->append_column (_("Expression"),
                                  get_bp_cols ().expression);
        nb_columns = tree_view->append_column_editable (_("Ignore count"),
							get_bp_cols ().ignore_count);
	
	for (int i = 0; i < nb_columns; ++i) {
            Gtk::TreeViewColumn *col = tree_view->get_column (i);
            col->set_clickable ();
            col->set_resizable ();
            col->set_reorderable ();
	}

        Gtk::CellRendererToggle *enabled_toggle =
            dynamic_cast<Gtk::CellRendererToggle*>
            (tree_view->get_column_cell_renderer (BPColumns::ENABLE_INDEX));
        if (enabled_toggle) {
            enabled_toggle->signal_toggled ().connect
                (sigc::mem_fun
                 (*this,
                  &BreakpointsView::Priv::on_breakpoint_enable_toggled));
        }
	Gtk::CellRendererToggle *countpoint_toggle =
	  dynamic_cast<Gtk::CellRendererToggle*>
	  (tree_view->get_column_cell_renderer (BPColumns::IS_COUNTPOINT_INDEX));
	if (countpoint_toggle) {
	  countpoint_toggle->signal_toggled ().connect
	    (sigc::mem_fun (*this,
			    &BreakpointsView::Priv::on_countpoint_toggled));
	  
	}

        Gtk::CellRendererText *r =
            dynamic_cast<Gtk::CellRendererText*>
            (tree_view->get_column_cell_renderer (BPColumns::IGNORE_COUNT_INDEX));
        r->signal_edited ().connect
            (sigc::mem_fun
             (*this,
              &BreakpointsView::Priv::on_breakpoint_ignore_count_edited));

        r = dynamic_cast<Gtk::CellRendererText*>
            (tree_view->get_column_cell_renderer (BPColumns::CONDITION_INDEX));
        r->signal_edited ().connect
	  (sigc::mem_fun
	   (*this, &BreakpointsView::Priv::on_breakpoint_condition_edited));

        // we must handle the button press event before the default button
        // handler since there are cases when we need to prevent the default
        // handler from running
        tree_view->signal_button_press_event ().connect
            (sigc::mem_fun
             (*this, &Priv::on_breakpoints_view_button_press_signal),
             false /*connect before*/);
        tree_view->get_selection ()->signal_changed ().connect
	  (sigc::mem_fun
	   (*this, &Priv::on_treeview_selection_changed));

        tree_view->signal_key_press_event ().connect (sigc::mem_fun
                                                      (*this, &Priv::on_key_press_event));
        tree_view->signal_expose_event ().connect_notify (sigc::mem_fun
                                                          (*this, &Priv::on_expose_event));
    }

    bool should_process_now ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        THROW_IF_FAIL (tree_view);
        bool is_visible = tree_view->is_drawable ();
        LOG_DD ("is visible: " << is_visible);
        return is_visible;
    }

    void set_breakpoints
        (const std::map<int, IDebugger::Breakpoint> &a_breakpoints)
    {
        if (a_breakpoints.empty ()) {
            return;
        }

        if (list_store->children ().empty ()) {
            // there are no breakpoints in the list yet, so no need to do any
            // searching for things to update, just add them all directly
            add_breakpoints (a_breakpoints);
        } else {
            //find breakpoints that need adding or updating
            std::map<int, IDebugger::Breakpoint>::const_iterator breakmap_iter;
            for (breakmap_iter = a_breakpoints.begin ();
                 breakmap_iter != a_breakpoints.end ();
                 ++breakmap_iter) {
                Gtk::TreeModel::iterator tree_iter =
                    find_breakpoint_in_model (breakmap_iter->second);
                if (tree_iter) {
                    LOG_DD ("Updating breakpoint "
                            << breakmap_iter->second.number ());
                    update_breakpoint (tree_iter, breakmap_iter->second);
                } else {
                    LOG_DD ("Adding breakpoint "
                            << breakmap_iter->second.number ());
                    append_breakpoint (breakmap_iter->second);
                }
            }
        }
    }

    void add_breakpoints
                (const std::map<int, IDebugger::Breakpoint> &a_breakpoints)
    {
        THROW_IF_FAIL (list_store);

        std::map<int, IDebugger::Breakpoint>::const_iterator break_iter;
        for (break_iter = a_breakpoints.begin ();
                break_iter != a_breakpoints.end ();
                ++break_iter) {
            append_breakpoint (break_iter->second);
        }
    }

    bool breakpoint_list_has_id
        (const std::map<int, IDebugger::Breakpoint> &a_breakpoints,
         int a_id)
    {
        std::map<int, IDebugger::Breakpoint>::const_iterator breakmap_iter;
        for (breakmap_iter = a_breakpoints.begin ();
                breakmap_iter != a_breakpoints.end (); ++ breakmap_iter) {
            if (a_id == breakmap_iter->second.number ()) {
                return true;
            }
        }
        return false;
    }

    Gtk::TreeModel::iterator find_breakpoint_in_model
                                (const IDebugger::Breakpoint &a_breakpoint)
    {
        THROW_IF_FAIL (list_store);

        Gtk::TreeModel::iterator iter;
        for (iter = list_store->children ().begin ();
             iter != list_store->children ().end ();
             ++iter) {
            if ((*iter)[get_bp_cols ().id] == a_breakpoint.number ()) {
                return iter;
            }
        }
        // Breakpoint not found in model, return an invalid iter
        return Gtk::TreeModel::iterator();
    }

    void update_breakpoint (Gtk::TreeModel::iterator& a_iter,
                            const IDebugger::Breakpoint &a_breakpoint)
    {
        (*a_iter)[get_bp_cols ().breakpoint] = a_breakpoint;
        (*a_iter)[get_bp_cols ().enabled] = a_breakpoint.enabled ();
        (*a_iter)[get_bp_cols ().id] = a_breakpoint.number ();
        (*a_iter)[get_bp_cols ().function] = a_breakpoint.function ();
        (*a_iter)[get_bp_cols ().address] =
                                (a_breakpoint.address ().empty ())
                                ? "<PENDING>"
                                : a_breakpoint.address ().to_string ();
        (*a_iter)[get_bp_cols ().filename] = a_breakpoint.file_name ();
        (*a_iter)[get_bp_cols ().line] = a_breakpoint.line ();
        (*a_iter)[get_bp_cols ().condition] = a_breakpoint.condition ();
        (*a_iter)[get_bp_cols ().expression] = a_breakpoint.expression ();
        (*a_iter)[get_bp_cols ().ignore_count] =
                                        a_breakpoint.ignore_count ();
        (*a_iter)[get_bp_cols ().is_standard] = false;
        (*a_iter)[get_bp_cols ().is_countpoint] =
            debugger->is_countpoint (a_breakpoint);

        switch (a_breakpoint.type ()) {
	case IDebugger::Breakpoint::STANDARD_BREAKPOINT_TYPE:
	  (*a_iter)[get_bp_cols ().type] = _("breakpoint");
	  (*a_iter)[get_bp_cols ().is_standard] = true;
	  break;
	case IDebugger::Breakpoint::WATCHPOINT_TYPE:
	  (*a_iter)[get_bp_cols ().type] = _("watchpoint");
	  break;
	case IDebugger::Breakpoint::COUNTPOINT_TYPE:
	  (*a_iter)[get_bp_cols ().type] = _("countpoint");
	  break;
	default:
	  (*a_iter)[get_bp_cols ().type] = _("unknown");
        }
        (*a_iter)[get_bp_cols ().hits] = a_breakpoint.nb_times_hit ();
    }

    Gtk::TreeModel::iterator append_breakpoint
                                    (const IDebugger::Breakpoint &a_breakpoint)
    {
        Gtk::TreeModel::iterator tree_iter = list_store->append ();
        update_breakpoint (tree_iter, a_breakpoint);

        return tree_iter;
    }

    void finish_handling_debugger_stopped_event ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        debugger->list_breakpoints ();
    }

    Gtk::Widget* load_menu (UString a_filename, UString a_widget_name)
    {
        NEMIVER_TRY
        string relative_path = Glib::build_filename ("menus", a_filename);
        string absolute_path;
        THROW_IF_FAIL (perspective.build_absolute_resource_path
                (Glib::locale_to_utf8 (relative_path), absolute_path));

        workbench.get_ui_manager ()->add_ui_from_file
                                        (Glib::locale_to_utf8 (absolute_path));
        NEMIVER_CATCH
        return workbench.get_ui_manager ()->get_widget (a_widget_name);
    }

    Gtk::Widget* get_breakpoints_menu ()
    {
        THROW_IF_FAIL (breakpoints_menu);
        return breakpoints_menu;
    }

    void popup_breakpoints_view_menu (GdkEventButton *a_event)
    {
        THROW_IF_FAIL (a_event);
        THROW_IF_FAIL (tree_view);
        Gtk::Menu *menu = dynamic_cast<Gtk::Menu*> (get_breakpoints_menu ());
        THROW_IF_FAIL (menu);
        menu->popup (a_event->button, a_event->time);
    }

    void init_actions()
    {
        static ui_utils::ActionEntry s_breakpoints_action_entries [] = {
            {
                "DeleteBreakpointMenuItemAction",
                Gtk::Stock::DELETE,
                _("_Delete"),
                _("Remove this breakpoint"),
                sigc::mem_fun (*this, &Priv::on_breakpoint_delete_action),
                ui_utils::ActionEntry::DEFAULT,
                "",
                false
            },
            {
                "GoToSourceBreakpointMenuItemAction",
                Gtk::Stock::JUMP_TO,
                _("_Go to Source"),
                _("Find this breakpoint in the source editor"),
                sigc::mem_fun (*this,
                        &Priv::on_breakpoint_go_to_source_action),
                ui_utils::ActionEntry::DEFAULT,
                "",
                false
            }
        };

        breakpoints_action_group =
            Gtk::ActionGroup::create ("breakpoints-action-group");
        breakpoints_action_group->set_sensitive (true);

        int num_actions =
            sizeof (s_breakpoints_action_entries)
                /
            sizeof (ui_utils::ActionEntry);

        ui_utils::add_action_entries_to_action_group
                        (s_breakpoints_action_entries, num_actions,
                         breakpoints_action_group);

        workbench.get_ui_manager ()->insert_action_group
                                                (breakpoints_action_group);
    }

    void re_init ()
    {
        debugger->list_breakpoints ();
    }

    void erase_breakpoint (int a_bp_num)
    {

        LOG_DD ("asked to erase bp num:" << (int) a_bp_num);

        Gtk::TreeModel::iterator iter;
        for (iter = list_store->children ().begin ();
             iter != list_store->children ().end ();
             ++iter) {
            if ((*iter)[get_bp_cols ().id] == a_bp_num) {
                break;
            }
        }

        if (iter != list_store->children ().end ()) {
            LOG_DD ("erased bp");
            list_store->erase (iter);
        }
    }

    void 
    on_debugger_breakpoints_list_signal
                            (const map<int, IDebugger::Breakpoint> &a_breaks,
                             const UString &a_cookie)
    {
        NEMIVER_TRY
        if (a_cookie.empty ()) {}
        set_breakpoints (a_breaks);
        NEMIVER_CATCH
    }

    void on_debugger_stopped_signal (IDebugger::StopReason a_reason,
                                     bool /*a_has_frame*/,
                                     const IDebugger::Frame &/*a_frame*/,
                                     int /*a_thread_id*/,
                                     int a_bkpt_num,
                                     const UString &/*a_cookie*/)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        NEMIVER_TRY

        LOG_DD ("a_reason: " << a_reason << " bkpt num: " << a_bkpt_num);

        if (a_reason == IDebugger::BREAKPOINT_HIT
            || a_reason == IDebugger::WATCHPOINT_TRIGGER
            || a_reason == IDebugger::READ_WATCHPOINT_TRIGGER
            || a_reason == IDebugger::ACCESS_WATCHPOINT_TRIGGER) {
            if (should_process_now ()) {
                finish_handling_debugger_stopped_event ();
            } else {
                is_up2date = false;
            }
        } else if (a_reason == IDebugger::WATCHPOINT_SCOPE) {
            LOG_DD ("erase watchpoint num: " << (int) a_bkpt_num);
            erase_breakpoint (a_bkpt_num);
        }

        NEMIVER_CATCH
    }

    void on_debugger_breakpoint_deleted_signal
            (const IDebugger::Breakpoint &a_break, int a_break_number,
             const UString &a_cookie)
    {
        if (a_break.number () || a_cookie.empty()) {}
        NEMIVER_TRY
        list<Gtk::TreeModel::iterator> iters_to_erase;
        for (Gtk::TreeModel::iterator iter = list_store->children ().begin ();
             iter != list_store->children ().end ();
             ++iter) {
            if ((*iter)[get_bp_cols ().id] == a_break_number) {
                iters_to_erase.push_back (iter);
                break;
            }
        }
        list<Gtk::TreeModel::iterator>::iterator it;
        for (it = iters_to_erase.begin ();
             it != iters_to_erase.end ();
             ++it) {
            list_store->erase (*it);
        }
        NEMIVER_CATCH
    }

    void
    on_debugger_breakpoint_set_signal
    (const std::pair<int, const IDebugger::Breakpoint&> &a,
     const UString &)
    {
        NEMIVER_TRY;

        LOG_DD ("Adding breakpoint "
                << a.second.number ());

        append_breakpoint (a.second);

        NEMIVER_CATCH;
    }

    bool
    on_breakpoints_view_button_press_signal (GdkEventButton *a_event)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        bool handled = false;

        NEMIVER_TRY

        // double-clicking a breakpoint item should go
        // to the source location for
        // the breakpoint
        if (a_event->type == GDK_2BUTTON_PRESS) {
            if (a_event->button == 1) {
                on_breakpoint_go_to_source_action ();
            }
        }

        // right-clicking should pop up a context menu
        else if (a_event->type == GDK_BUTTON_PRESS) {
            if (a_event->button == 3) {
                // only pop up a context menu if there's a valid item at the
                // point where the user clicked
                Gtk::TreeModel::Path path;
                Gtk::TreeViewColumn* p_column = NULL;
                int cell_x=0, cell_y=0;
                if (tree_view->get_path_at_pos (
                            static_cast<int>(a_event->x),
                            static_cast<int>(a_event->y),
                            path, p_column, cell_x, cell_y)) {
                    popup_breakpoints_view_menu (a_event);
                    Glib::RefPtr<Gtk::TreeView::Selection> selection =
                        tree_view->get_selection ();
                    if (selection->is_selected(path)) {
                        // don't continue to handle this event.
                        // This is necessary
                        // because if multiple items are selected,
                        // we don't want them to become de-selected
                        // when we pop up a context menu
                        handled = true;
                    }
                }
            }
        }

        NEMIVER_CATCH
        return handled;
    }

    void on_treeview_selection_changed ()
    {
        NEMIVER_TRY
        THROW_IF_FAIL(tree_view)

        Glib::RefPtr<Gtk::Action> action =
            workbench.get_ui_manager ()->get_action
                ("/BreakpointsPopup/GoToSourceBreakpointMenuItem");
        if (action) {
            if (tree_view->get_selection ()->count_selected_rows () > 1) {
                action->set_sensitive (false);
            } else {
                action->set_sensitive (true);
            }
        } else {
            LOG_ERROR
                ("Could not get action "
                 "/BreakpointsPopup/GoToSourceBreakpointMenuItem");
        }
        NEMIVER_CATCH
    }

    void on_breakpoint_go_to_source_action ()
    {
        THROW_IF_FAIL(tree_view)
        Glib::RefPtr<Gtk::TreeSelection> selection =
                                                tree_view->get_selection ();
        vector<Gtk::TreeModel::Path> paths = selection->get_selected_rows ();
        if (paths.empty ())
            return;
        Gtk::TreeModel::iterator tree_iter = list_store->get_iter (paths[0]);
        if (tree_iter) {
            go_to_breakpoint_signal.emit
                            ((*tree_iter)[get_bp_cols ().breakpoint]);
        }
    }

    void on_breakpoint_delete_action ()
    {
        THROW_IF_FAIL (tree_view)
        THROW_IF_FAIL (list_store);

        Glib::RefPtr<Gtk::TreeSelection> selection =
                                                tree_view->get_selection ();
        vector<Gtk::TreeModel::Path> paths = selection->get_selected_rows ();
        vector<Gtk::TreeModel::Path>::const_iterator it;
        Gtk::TreeModel::iterator tree_iter;
        for (it=paths.begin (); it != paths.end (); ++it) {
            tree_iter = list_store->get_iter (*it);
            if (tree_iter) {
                debugger->delete_breakpoint
                                ((*tree_iter)[get_bp_cols ().id]);
            }
        }
    }

    void on_expose_event (GdkEventExpose *)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        NEMIVER_TRY
        if (!is_up2date) {
            finish_handling_debugger_stopped_event ();
            is_up2date = true;
        }
        NEMIVER_CATCH
    }

    bool on_key_press_event (GdkEventKey* event)
    {
        if (event && event->keyval == GDK_Delete)
        {
            on_breakpoint_delete_action ();
        }
        return false;
    }

    void on_breakpoint_enable_toggled (const Glib::ustring& path)
    {
        NEMIVER_TRY

        THROW_IF_FAIL (tree_view);
        Gtk::TreeModel::iterator tree_iter =
                                    tree_view->get_model ()->get_iter (path);
        if (tree_iter) {
            if ((*tree_iter)[get_bp_cols ().enabled]) {
                debugger->enable_breakpoint
                                    ((*tree_iter)[get_bp_cols ().id]);
            } else {
                debugger->disable_breakpoint
                                    ((*tree_iter)[get_bp_cols ().id]);
            }
        }

        NEMIVER_CATCH
    }

    void on_countpoint_toggled (const Glib::ustring& path)
    {
        NEMIVER_TRY;

        THROW_IF_FAIL (tree_view);
        Gtk::TreeModel::iterator tree_iter =
            tree_view->get_model ()->get_iter (path);
        
        if (tree_iter) {
            if ((*tree_iter)[get_bp_cols ().is_countpoint]) {
                debugger->enable_countpoint
                    ((*tree_iter)[get_bp_cols ().id], true);
            } else {
                debugger->enable_countpoint
                    ((*tree_iter)[get_bp_cols ().id], false);
            }
        }
        NEMIVER_CATCH;
    }

    void on_breakpoint_ignore_count_edited (const Glib::ustring &a_path,
                                            const Glib::ustring &a_text)
    {
        NEMIVER_TRY

        THROW_IF_FAIL (tree_view);

        Gtk::TreeModel::iterator it =
                                tree_view->get_model ()->get_iter (a_path);

        bool is_standard_bp = false; //true if this is e.g. no watchpoint.
        if (it
            && (((IDebugger::Breakpoint)(*it)[get_bp_cols ().breakpoint])).
                type () == IDebugger::Breakpoint::STANDARD_BREAKPOINT_TYPE) {
            is_standard_bp = true;
            LOG_DD ("breakpoint is standard");
        } else {
            LOG_DD ("breakpoint is *NOT* standard");
        }

        if (is_standard_bp) {
            int count = atoi (a_text.raw ().c_str ());
            debugger->set_breakpoint_ignore_count ((*it)[get_bp_cols ().id],
                                                   count);
        }
        NEMIVER_CATCH
    }

    void on_breakpoint_condition_edited (const Glib::ustring &a_path,
                                         const Glib::ustring &a_text)
    {
        NEMIVER_TRY

        Gtk::TreeModel::iterator it = tree_view->get_model ()->get_iter (a_path);

        bool is_standard_bp =
            (((IDebugger::Breakpoint)(*it)[get_bp_cols ().breakpoint]).type ()
             == IDebugger::Breakpoint::STANDARD_BREAKPOINT_TYPE)
            ? true
            : false;

        if (is_standard_bp)
            debugger->set_breakpoint_condition ((*it)[get_bp_cols ().id],
                                                a_text);

        NEMIVER_CATCH
    }

};//end class BreakpointsView::Priv

BreakpointsView::BreakpointsView (IWorkbench& a_workbench,
                                  IPerspective& a_perspective,
                                  IDebuggerSafePtr& a_debugger)
{
    m_priv.reset (new Priv (a_workbench, a_perspective, a_debugger));
}

BreakpointsView::~BreakpointsView ()
{
    LOG_D ("deleted", "destructor-domain");
}

Gtk::Widget&
BreakpointsView::widget () const
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->tree_view);
    THROW_IF_FAIL (m_priv->list_store);
    return *m_priv->tree_view;
}

void
BreakpointsView::set_breakpoints
                (const std::map<int, IDebugger::Breakpoint> &a_breakpoints)
{
    THROW_IF_FAIL (m_priv);
    m_priv->set_breakpoints (a_breakpoints);
}

void
BreakpointsView::clear ()
{
    THROW_IF_FAIL (m_priv);
    if (m_priv->list_store) {
        m_priv->list_store->clear ();
    }
}

void
BreakpointsView::re_init ()
{
    THROW_IF_FAIL (m_priv);
    clear ();
    m_priv->re_init ();
}

sigc::signal<void, const IDebugger::Breakpoint&>&
BreakpointsView::go_to_breakpoint_signal () const
{
    THROW_IF_FAIL(m_priv);
    return m_priv->go_to_breakpoint_signal;
}

}//end namespace nemiver

