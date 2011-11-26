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
#include <sstream>
#include <algorithm>
#include <gtkmm/treeview.h>
#include <gtkmm/liststore.h>
#include <glib/gi18n.h>
#include "common/nmv-exception.h"
#include "nmv-call-stack.h"
#include "nmv-ui-utils.h"
#include "nmv-i-workbench.h"
#include "nmv-i-perspective.h"
#include "nmv-conf-keys.h"

namespace nemiver {

struct GObjectMMRef {

    void operator () (Glib::Object *a_obj)
    {
        if (a_obj) {
            a_obj->reference ();
        }
    }
};//end GlibObjectRef

struct GObjectMMUnref {
    void operator () (Glib::Object *a_obj)
    {
        if (a_obj) {
            a_obj->unreference ();
        }
    }
};//end GlibObjectRef

typedef SafePtr<Glib::Object, GObjectMMRef, GObjectMMUnref> GObjectMMSafePtr;

struct CallStackCols : public Gtk::TreeModelColumnRecord {
    Gtk::TreeModelColumn<Glib::ustring> location;
    Gtk::TreeModelColumn<Glib::ustring> address;
    Gtk::TreeModelColumn<Glib::ustring> binary;
    Gtk::TreeModelColumn<Glib::ustring> function_name;
    Gtk::TreeModelColumn<Glib::ustring> function_args;
    Gtk::TreeModelColumn<Glib::ustring> frame_index_caption;
    Gtk::TreeModelColumn<int> frame_index;
    Gtk::TreeModelColumn<bool> is_expansion_row;

    /// Keep the enum in an order compatible with the how the diffent
    /// columns are appended in CallStack::Priv::build_widget.
    enum Index {
        FRAME_INDEX = 0,
        FUNCTION_NAME_INDEX,
        FUNCTION_ARGUMENTS_INDEX,
        LOCATION_INDEX,
        ADDRESS_INDEX,
        BINARY_INDEX
    };

    CallStackCols ()
    {
        add (location);
        add (address);
	add (binary);
        add (function_name);
        add (function_args);
        add (frame_index_caption);
        add (frame_index);
        add (is_expansion_row);
    }
};//end cols

static CallStackCols&
columns ()
{
    static CallStackCols s_cols;
    return s_cols;
}

static const char* COOKIE_CALL_STACK_IN_FRAME_PAGING_TRANS =
    "cookie-call-stack-in-frame-paging-trans";

typedef vector<IDebugger::Frame> FrameArray;
typedef map<int, list<IDebugger::VariableSafePtr> > FrameArgsMap;
typedef map<int, IDebugger::Frame> LevelFrameMap;
struct CallStack::Priv {
    IDebuggerSafePtr debugger;
    IConfMgrSafePtr conf_mgr;
    IWorkbench& workbench;
    IPerspective& perspective;
    FrameArray frames;
    FrameArgsMap params;
    LevelFrameMap level_frame_map;
    Glib::RefPtr<Gtk::ListStore> store;
    SafePtr<Gtk::TreeView> widget;
    IDebugger::Frame cur_frame;
    sigc::signal<void, int, const IDebugger::Frame&> frame_selected_signal;
    sigc::connection on_selection_changed_connection;
    Gtk::Widget *callstack_menu;
    Glib::RefPtr<Gtk::ActionGroup> call_stack_action_group;
    unsigned cur_frame_index;
    unsigned nb_frames_expansion_chunk;
    int frame_low;
    int frame_high;
    bool waiting_for_stack_args;
    bool in_set_cur_frame_trans;
    bool is_up2date;

    Priv (IDebuggerSafePtr a_dbg,
          IWorkbench& a_workbench,
          IPerspective& a_perspective) :
        debugger (a_dbg),
        conf_mgr (0),
        workbench (a_workbench),
        perspective (a_perspective),
        callstack_menu (0),
        cur_frame_index (-1),
        nb_frames_expansion_chunk (25),
        frame_low (0),
        frame_high (nb_frames_expansion_chunk),
        waiting_for_stack_args (false),
        in_set_cur_frame_trans (false),
        is_up2date (true)
    {
        connect_debugger_signals ();
        init_actions ();
        init_conf ();
    }

    void init_conf ()
    {
        conf_mgr = workbench.get_configuration_manager ();
        if (!conf_mgr)
            return;

        int chunk = 0;
        conf_mgr->get_key_value (CONF_KEY_NEMIVER_CALLSTACK_EXPANSION_CHUNK,
                                 chunk);
        if (chunk) {
            nb_frames_expansion_chunk = chunk;
        }
        conf_mgr->value_changed_signal ().connect
            (sigc::mem_fun (*this, &Priv::on_config_value_changed_signal));
    }

    void init_actions ()
    {

        static ui_utils::ActionEntry s_call_stack_action_entries [] = {
            {
                "CopyCallStackMenuItemAction",
                Gtk::Stock::COPY,
                _("_Copy"),
                _("Copy the call stack to the clipboard"),
                sigc::mem_fun
                    (*this,
                     &Priv::on_call_stack_copy_to_clipboard_action),
                ui_utils::ActionEntry::DEFAULT,
                "",
                false
            }
        };

        call_stack_action_group =
            Gtk::ActionGroup::create ("callstack-action-group");
        call_stack_action_group->set_sensitive (true);
        int num_actions =
            sizeof (s_call_stack_action_entries)
                /
            sizeof (ui_utils::ActionEntry);

        ui_utils::add_action_entries_to_action_group
            (s_call_stack_action_entries,
             num_actions,
             call_stack_action_group);

        workbench.get_ui_manager ()->insert_action_group
                                                (call_stack_action_group);
    }

    bool should_process_now ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        THROW_IF_FAIL (widget);
        bool is_visible = widget->get_is_drawable ();
        LOG_DD ("is visible: " << is_visible);
        return is_visible;
    }

    Gtk::Widget* get_call_stack_menu ()
    {
        if (!callstack_menu) {
            callstack_menu = perspective.load_menu ("callstackpopup.xml",
                                                    "/CallStackPopup");
            THROW_IF_FAIL (callstack_menu);
        }
        return callstack_menu;
    }

    /// If the selected frame is the "expand to see more frames" raw,
    /// ask the debugger engine for more frames.
    /// Otherwise, just set the "current frame" variable.
    void update_selected_frame (Gtk::TreeModel::iterator &a_row_iter)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (a_row_iter);

        // if the selected row is the "expand number of stack lines" row, trigger
        // a redraw of the call stack with more raws.
        if ((*a_row_iter)[columns ().is_expansion_row]) {
            frame_low = frame_high + 1;
            frame_high += nb_frames_expansion_chunk;
            debugger->list_frames (frame_low, frame_high,
                                   sigc::mem_fun
                                   (*this, &Priv::on_frames_listed_during_paging),
                                   "");
            return;
        }

        cur_frame_index = (*a_row_iter)[columns ().frame_index];
        THROW_IF_FAIL (cur_frame_index < frames.size ());
        cur_frame = frames[cur_frame_index];
        THROW_IF_FAIL (cur_frame.level () >= 0);
        in_set_cur_frame_trans = true;

        LOG_DD ("frame selected: '"<<  (int) cur_frame_index << "'");
        LOG_DD ("frame level: '" << (int) cur_frame.level () << "'");
        debugger->select_frame (cur_frame_index);
    }

    void finish_update_handling ()
    {
        THROW_IF_FAIL (debugger);
        debugger->list_frames (frame_low, frame_high,
			       sigc::mem_fun (*this, &Priv::on_frames_listed),
			       "");
    }

    void handle_update (const UString &a_cookie)
    {
        if (a_cookie != COOKIE_CALL_STACK_IN_FRAME_PAGING_TRANS) {
            // Restore the frame window, in case the user changed it by
            // requesting more call stack frames.
            frame_low = 0;
            frame_high = nb_frames_expansion_chunk;
        }

        if (should_process_now ()) {
            finish_update_handling ();
        } else {
            is_up2date = false;
        }
    }

    void on_debugger_stopped_signal (IDebugger::StopReason a_reason,
                                     bool /*a_has_frame*/,
                                     const IDebugger::Frame &/*a_frame*/,
                                     int /*a_thread_id*/,
                                     int /*a_bp_num*/,
                                     const UString &a_cookie)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        NEMIVER_TRY
        LOG_DD ("stopped, reason: " << a_reason);

        if (a_reason == IDebugger::EXITED_SIGNALLED
            || a_reason == IDebugger::EXITED_NORMALLY
            || a_reason == IDebugger::EXITED) {
            return;
        }

        handle_update (a_cookie);

        NEMIVER_CATCH
    }

    void on_thread_selected_signal (int /*a_thread_id*/,
                                    const IDebugger::Frame* const /*a_frame*/,
                                    const UString& a_cookie)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        handle_update (a_cookie);
    }

    void
    on_frames_listed_during_paging (const vector<IDebugger::Frame> &a_stack)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        NEMIVER_TRY;

        // We are in a mode where the user asked to see more stack
        // frames.  This is named frame paging. In this case, just
        // append the frames to the current ones. Again, the frames
        // will be appended without arguments. We'll request the
        // arguments right after this.
        FrameArgsMap frames_args;
        append_frames_to_tree_view (a_stack, frames_args);

        // Okay so now, ask for frame arguments matching the frames we
        // just received.
        debugger->list_frames_arguments (a_stack[0].level (),
                                         a_stack[a_stack.size () - 1].level (),
                                         sigc::mem_fun
                                         (*this, &Priv::on_frames_args_listed),
                                         "");

        NEMIVER_CATCH;
    }

    void
    on_frames_listed (const vector<IDebugger::Frame> &a_stack)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        NEMIVER_TRY;

        THROW_IF_FAIL (debugger);
        waiting_for_stack_args = true;

        FrameArgsMap frames_args;
        // Set the frame list without frame arguments, then, request
        // IDebugger for frame arguments.  When the arguments arrive,
        // we will update the call stack with those.
        set_frame_list (a_stack, frames_args);

	// If the current instruction pointer changed without the
	// debugging engine telling us so, but we could detect that by
	// querying the frame list, let's update our current state
	// accordingly.
	if (cur_frame_index == 0 && frames.front () != cur_frame) {
	  cur_frame = frames.front ();
	  frame_selected_signal.emit (cur_frame_index, cur_frame);
	}

        // Okay so now, ask for frame arguments matching the frames we
        // just received.
        debugger->list_frames_arguments (a_stack[0].level (),
                                         a_stack[a_stack.size () - 1].level (),
                                         sigc::mem_fun
                                         (*this, &Priv::on_frames_args_listed),
                                         "");

        NEMIVER_CATCH;
    }

    void
    on_frames_args_listed
    (const map<int, IDebugger::VariableList> &a_frames_args)
    {
        LOG_DD ("frames params listed");

        if (waiting_for_stack_args) {
            update_frames_arguments (a_frames_args);
            waiting_for_stack_args = false;
        } else {
            LOG_D ("not in the frame setting transaction", NMV_DEFAULT_DOMAIN);
        }
    }

    void on_command_done_signal (const UString &a_command,
                                 const UString &a_cookie)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        if (a_cookie == "") {}

        NEMIVER_TRY

        if (in_set_cur_frame_trans
            && a_command == "select-frame") {
            in_set_cur_frame_trans = false;
            frame_selected_signal.emit (cur_frame_index, cur_frame);
            LOG_DD ("sent the frame selected signal");
        }
        NEMIVER_CATCH
    }

    void on_selection_changed_signal ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        NEMIVER_TRY

        Gtk::TreeView *tree_view = dynamic_cast<Gtk::TreeView*> (widget.get ());
        THROW_IF_FAIL (tree_view);
        Glib::RefPtr<Gtk::TreeSelection> selection = tree_view->get_selection ();
        THROW_IF_FAIL (selection);
        vector<Gtk::TreePath> selected_rows = selection->get_selected_rows ();
        if (selected_rows.empty ()) {return;}

        Gtk::TreeModel::iterator row_iter =
                store->get_iter (selected_rows.front ());
        update_selected_frame (row_iter);
        NEMIVER_CATCH
    }

    void on_row_activated_signal ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        NEMIVER_TRY

        Gtk::TreeView *tree_view = dynamic_cast<Gtk::TreeView*> (widget.get ());
        THROW_IF_FAIL (tree_view);
        Glib::RefPtr<Gtk::TreeSelection> selection = tree_view->get_selection ();
        THROW_IF_FAIL (selection);
        Gtk::TreeModel::iterator row_it = selection->get_selected ();
        update_selected_frame (row_it);

        NEMIVER_CATCH
    }

    void on_draw_signal (const Cairo::RefPtr<Cairo::Context> &)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        NEMIVER_TRY
        if (!is_up2date) {
            finish_update_handling ();
            is_up2date = true;
        }
        NEMIVER_CATCH
    }

    void on_config_value_changed_signal (const UString &a_key,
                                         const UString &a_namespace)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        if (!conf_mgr) {
            return;
        }

        LOG_DD ("key " << a_key << " changed");
        if (a_key == CONF_KEY_NEMIVER_CALLSTACK_EXPANSION_CHUNK) {
            int chunk = 0;
            conf_mgr->get_key_value (a_key, chunk, a_namespace);
            if (chunk) {
                nb_frames_expansion_chunk = chunk;
            }
        }
    }

    /// Connect callback slots to the relevant signals of our
    /// implementation of the IDebugger interface.
    void connect_debugger_signals ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (debugger);

        debugger->stopped_signal ().connect (sigc::mem_fun
                    (*this, &CallStack::Priv::on_debugger_stopped_signal));
        debugger->thread_selected_signal ().connect (sigc::mem_fun
                     (*this, &CallStack::Priv::on_thread_selected_signal));
        debugger->command_done_signal ().connect (sigc::mem_fun
                    (*this, &CallStack::Priv::on_command_done_signal));
    }

    void on_call_stack_button_press_signal (GdkEventButton *a_event)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        NEMIVER_TRY

        // right-clicking should pop up a context menu
        if ((a_event->type == GDK_BUTTON_PRESS) && (a_event->button == 3)) {
            popup_call_stack_menu (a_event);
        }

        NEMIVER_CATCH
    }

    void popup_call_stack_menu (GdkEventButton *a_event)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (a_event);
        THROW_IF_FAIL (widget);

        Gtk::Menu *menu = dynamic_cast<Gtk::Menu*> (get_call_stack_menu ());
        THROW_IF_FAIL (menu);

        // only pop up a menu if a row exists at that position
        Gtk::TreeModel::Path path;
        Gtk::TreeViewColumn* column = 0;
        int cell_x = 0, cell_y = 0;
        if (widget->get_path_at_pos (static_cast<int> (a_event->x),
                                     static_cast<int> (a_event->y),
                                     path,
                                     column,
                                     cell_x,
                                     cell_y)) {
            menu->popup (a_event->button, a_event->time);
        }
    }

    void on_call_stack_copy_to_clipboard_action ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        NEMIVER_TRY

        int i = 0;
        std::ostringstream frame_stream;
        vector<IDebugger::Frame>::const_iterator frame_iter;
        map<int, list<IDebugger::VariableSafePtr> >::const_iterator
                                                                params_iter;
        UString args_string;
        // convert list of stack frames to a string
        // FIXME: maybe Frame should
        // just implement operator<< ?
        for (frame_iter = frames.begin (), params_iter = params.begin ();
             frame_iter != frames.end ();
             ++frame_iter, ++params_iter) {
            frame_stream << "#" << UString::from_int (i++) << "  " <<
                frame_iter->function_name ();

            // if the params map exists, add the
            // function params to the stack trace
            args_string = "()";
            if (params_iter != params.end ())
                format_args_string (params_iter->second, args_string);
            frame_stream << args_string.raw ();

            frame_stream << " at " << frame_iter->file_name () << ":"
                << UString::from_int(frame_iter->line ()) << std::endl;
        }
        Gtk::Clipboard::get ()->set_text (frame_stream.str ());

        NEMIVER_CATCH
    }

    Gtk::Widget* get_widget ()
    {
        if (!widget) {return 0;}
        return widget.get ();
    }

    void build_widget ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        if (widget) {
            return;
        }
        store = Gtk::ListStore::create (columns ());
        Gtk::TreeView *tree_view = new Gtk::TreeView (store);
        THROW_IF_FAIL (tree_view);
        widget.reset (tree_view);
	int nb_cols = 0;
        tree_view->append_column (_("Frame"), columns ().frame_index_caption);
        tree_view->append_column (_("Function"), columns ().function_name);
        tree_view->append_column (_("Arguments"), columns ().function_args);
        tree_view->append_column (_("Location"), columns ().location);
        tree_view->append_column (_("Address"), columns ().address);
	nb_cols = tree_view->append_column (_("Binary"), columns ().binary);
        Gtk::TreeViewColumn *col = 0;
	for (int i = 0; i < nb_cols; ++i) {
            col = tree_view->get_column (i);
            col->set_resizable ();
            col->set_clickable ();
	}

        THROW_IF_FAIL (col = tree_view->get_column (CallStackCols::BINARY_INDEX));
        col->get_first_cell ()->set_sensitive ();

        tree_view->set_headers_visible (true);
	tree_view->columns_autosize ();
        tree_view->get_selection ()->set_mode (Gtk::SELECTION_SINGLE);

        on_selection_changed_connection =
            tree_view->get_selection ()->signal_changed ().connect
            (sigc::mem_fun (*this,
                            &CallStack::Priv::on_selection_changed_signal));

        tree_view->signal_row_activated ().connect
            (sigc::hide (sigc::hide
                         (sigc::mem_fun (*this,
                                         &CallStack::Priv::on_row_activated_signal))));

        tree_view->signal_draw ().connect_notify
            (sigc::mem_fun (this, &Priv::on_draw_signal));

        tree_view->add_events (Gdk::EXPOSURE_MASK);

        tree_view->signal_button_press_event ().connect_notify
            (sigc::mem_fun (*this,
                            &Priv::on_call_stack_button_press_signal));
    }

    void store_frames_in_cache (const FrameArray &a_frames,
                                const FrameArgsMap &a_frames_args)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        if (a_frames.empty ())
            return;
        append_frames_to_cache (a_frames, a_frames_args);
    }

    void append_frames_to_cache (const FrameArray &a_frames,
                                 const FrameArgsMap &a_frames_args)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (!a_frames.empty ());

        int dest_start_index = a_frames[0].level (),
            dest_end_index = a_frames.size () + dest_start_index - 1;
        unsigned level = 0;
        FrameArray::const_iterator f;

        frames.reserve (dest_end_index + 1);
        for (f = a_frames.begin (); f != a_frames.end (); ++f) {
            level = f->level ();
            if (level >= frames.size ())
                frames.push_back (*f);
            else
                frames[level] = *f;
        }
        append_frame_args_to_cache (a_frames_args);
    }

    void append_frame_args_to_cache (const FrameArgsMap &a_frames_args)
    {
        FrameArgsMap::const_iterator fa;
        for (fa = a_frames_args.begin ();
             fa != a_frames_args.end ();
             ++fa) {
            params[fa->first] = fa->second;
        }
    }

    void format_args_string (const list<IDebugger::VariableSafePtr> &a_args,
                             UString &a_string)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        UString arg_string = "(";
        list<IDebugger::VariableSafePtr>::const_iterator arg_it = a_args.begin ();
        if (arg_it != a_args.end () && *arg_it) {
            arg_string += (*arg_it)->name () + " = " + (*arg_it)->value ();
            ++arg_it;
        }
        for (; arg_it != a_args.end (); ++arg_it) {
            if (!*arg_it)
                continue;
            arg_string += ", " + (*arg_it)->name ()
                          + " = " + (*arg_it)->value ();
        }
        arg_string += ")";
        a_string = arg_string;
    }

    void append_frames_to_tree_view (const FrameArray &a_frames,
                                     const FrameArgsMap &a_args)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        // Erase the expansion row, if it exists.
        if (store && !store->children ().empty ()) {
            LOG_DD ("does expansion row exist ?");
            Gtk::TreeRow last_row =
                store->children ()[store->children ().size () - 1];
            if (last_row[columns ().is_expansion_row]) {
                LOG_DD ("erasing expansion node");
                store->erase (last_row);
            } else {
                LOG_DD ("not an expansion node");
            }
        }
        // really append the frames to the tree view now
        Gtk::TreeModel::iterator store_iter;
        unsigned nb_frames = a_frames.size ();
        for (unsigned i = 0; i < nb_frames; ++i) {
            level_frame_map[a_frames[i].level ()] = a_frames[i];
            frames.push_back (a_frames[i]);
            store_iter = store->append ();
            (*store_iter)[columns ().is_expansion_row] = false;
            (*store_iter)[columns ().function_name] =
                a_frames[i].function_name ();
            if (!a_frames[i].file_name ().empty ()) {
                (*store_iter)[columns ().location] =
                    a_frames[i].file_name () + ":"
                    + UString::from_int (a_frames[i].line ());
            }
            (*store_iter)[columns ().address] =
                a_frames[i].address ().to_string ();
	    (*store_iter)[columns ().binary] = a_frames[i].library ();
            (*store_iter)[columns ().frame_index] = a_frames[i].level ();
            (*store_iter)[columns ().frame_index_caption] =
                UString::from_int (a_frames[i].level ());
            FrameArgsMap::const_iterator fa_it;
            UString arg_string;
            fa_it = a_args.find (a_frames[i].level ());
            if (fa_it != a_args.end ()) {
                format_args_string (fa_it->second, arg_string);
                params[a_frames[i].level ()] = fa_it->second;
            } else {
                arg_string = "()";
            }
            (*store_iter)[columns ().function_args] = arg_string;
        }
        if (a_frames.size () >= nb_frames_expansion_chunk) {
            store_iter = store->append ();
            UString msg;
            msg.printf (ngettext ("(Click here to see the next %d row of the "
                                  "call stack",
                                  "(Click here to see the next %d rows of the "
                                  "call stack)",
                                  nb_frames_expansion_chunk),
                        nb_frames_expansion_chunk);
            (*store_iter)[columns ().frame_index_caption] = "...";
            (*store_iter)[columns ().location] = msg;
            (*store_iter)[columns ().is_expansion_row] = true;
        }
        store_frames_in_cache (a_frames, a_args);
    }

    void set_frame_list
                (const FrameArray &a_frames,
                 const FrameArgsMap &a_params,
                 bool a_emit_signal=false)
    {
        THROW_IF_FAIL (get_widget ());

        clear_frame_list ();

        append_frames_to_tree_view (a_frames, a_params);

        Gtk::TreeView *tree_view =
            dynamic_cast<Gtk::TreeView*> (widget.get ());
        THROW_IF_FAIL (tree_view);

        if (!a_emit_signal) {
            on_selection_changed_connection.block ();
        }
        tree_view->get_selection ()->select (Gtk::TreePath ("0"));
        if (!a_emit_signal) {
            on_selection_changed_connection.unblock ();
        }
    }

    void update_frames_arguments (FrameArgsMap a_args)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        UString args_string;
        FrameArgsMap::const_iterator arg_it;
        LevelFrameMap::iterator lf_it;
        int level = 0;
        bool is_expansion_row = false;

        for (Gtk::TreeModel::iterator row = store->children ().begin ();
             row != store->children ().end ();
             ++row) {
            is_expansion_row = (*row)[columns ().is_expansion_row];
            if (is_expansion_row)
                continue;
            level = (*row)[columns ().frame_index];
            LOG_DD ("considering frame level " << level << " ...");
            if ((lf_it = level_frame_map.find (level))
                == level_frame_map.end ()) {
                LOG_ERROR ("Error: no frame found for level "
                           << arg_it->first);
                THROW ("Constraint error in CallStack widget");
            }
            if ((arg_it = a_args.find (level)) == a_args.end ()) {
                LOG_DD ("sorry, no arguments for this frame");
                continue;
            }
            format_args_string (arg_it->second, args_string);
            (*row)[columns ().function_args] = args_string;
            LOG_DD ("yesss, frame arguments are: " << args_string);
        }
        append_frame_args_to_cache (a_args);
    }

    /// Visually clear the frame list.
    /// \param a_reset_frame_window if true, reset the frame window range
    ///  to [0 - defaultmax]
    void clear_frame_list (bool a_reset_frame_window = false)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        if (a_reset_frame_window) {
            frame_low = 0;
            frame_high = nb_frames_expansion_chunk;
        }

        THROW_IF_FAIL (store);
        store->clear ();
        frames.clear ();
        params.clear ();
        level_frame_map.clear ();

    }

    void update_call_stack ()
    {
        THROW_IF_FAIL (debugger);
        debugger->list_frames (0, frame_high,
                               sigc::mem_fun (*this, &Priv::on_frames_listed),
                               "");
    }
};//end struct CallStack::Priv

CallStack::CallStack (IDebuggerSafePtr &a_debugger,
                      IWorkbench& a_workbench,
                      IPerspective &a_perspective)
{
    THROW_IF_FAIL (a_debugger);
    m_priv.reset (new Priv (a_debugger, a_workbench, a_perspective));
}

CallStack::~CallStack ()
{
    LOG_D ("deleted", "destructor-domain");
}

bool
CallStack::is_empty ()
{
    THROW_IF_FAIL (m_priv);
    return m_priv->frames.empty ();
}

const vector<IDebugger::Frame>&
CallStack::frames () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->frames;
}

IDebugger::Frame&
CallStack::current_frame () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->cur_frame;
}

Gtk::Widget&
CallStack::widget () const
{
    THROW_IF_FAIL (m_priv);

    if (!m_priv->get_widget ()) {
        m_priv->build_widget ();
        THROW_IF_FAIL (m_priv->widget);
    }
    return *m_priv->get_widget ();
}

void
CallStack::update_stack ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);

    m_priv->update_call_stack ();
}

void
CallStack::clear ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);

    m_priv->clear_frame_list (true /* reset frame window */ );
}

sigc::signal<void, int, const IDebugger::Frame&>&
CallStack::frame_selected_signal () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->frame_selected_signal;
}
}//end namespace nemiver


