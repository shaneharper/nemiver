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
#include <sstream>
#include <gtkmm/treeview.h>
#include <gtkmm/liststore.h>
#include <glib/gi18n.h>
#include "common/nmv-exception.h"
#include "nmv-call-stack.h"
#include "nmv-ui-utils.h"
#include "nmv-i-workbench.h"
#include "nmv-i-perspective.h"

namespace nemiver {

struct GObjectMMRef {

    void operator () (Glib::Object *a_obj)
    {
        if (a_obj) {
            a_obj->reference () ;
        }
    }
};//end GlibObjectRef

struct GObjectMMUnref {
    void operator () (Glib::Object *a_obj)
    {
        if (a_obj) {
            a_obj->unreference () ;
        }
    }
};//end GlibObjectRef

typedef SafePtr<Glib::Object, GObjectMMRef, GObjectMMUnref> GObjectMMSafePtr ;

struct CallStackCols : public Gtk::TreeModelColumnRecord {
    Gtk::TreeModelColumn<Glib::ustring> location ;
    Gtk::TreeModelColumn<Glib::ustring> function_name ;
    Gtk::TreeModelColumn<Glib::ustring> function_args;
    Gtk::TreeModelColumn<Glib::ustring> frame_index_caption;
    Gtk::TreeModelColumn<int> frame_index;
    Gtk::TreeModelColumn<bool> is_expansion_row;

    enum Index {
        LOCATION=0,
        FUNCTION_NAME,
        FUNCTION_ARGS
    };

    CallStackCols ()
    {
        add (location) ;
        add (function_name) ;
        add (function_args) ;
        add (frame_index_caption) ;
        add (frame_index) ;
        add (is_expansion_row) ;
    }
};//end cols

static CallStackCols&
columns ()
{
    static CallStackCols s_cols ;
    return s_cols ;
}

static const UString CONF_KEY_NEMIVER_CALLSTACK_EXPANSION_CHUNK =
    "/apps/nemiver/dbgperspective/callstack-expansion-chunk";

struct CallStack::Priv {
    IDebuggerSafePtr debugger ;
    IWorkbench& workbench;
    IPerspective& perspective;
    vector<IDebugger::Frame> frames ;
    map<int, list<IDebugger::VariableSafePtr> > params;
    Glib::RefPtr<Gtk::ListStore> store ;
    SafePtr<Gtk::TreeView> widget ;
    bool waiting_for_stack_args ;
    bool in_set_cur_frame_trans ;
    IDebugger::Frame cur_frame ;
    int cur_frame_index;
    unsigned nb_frames_expansion_chunk;
    unsigned max_frames_to_show;
    sigc::signal<void, int, const IDebugger::Frame&> frame_selected_signal ;
    sigc::connection on_selection_changed_connection ;
    Gtk::Widget *callstack_menu ;
    Glib::RefPtr<Gtk::ActionGroup> call_stack_action_group;

    Priv (IDebuggerSafePtr a_dbg,
          IWorkbench& a_workbench,
          IPerspective& a_perspective) :
        debugger (a_dbg),
        workbench (a_workbench),
        perspective (a_perspective),
        waiting_for_stack_args (false),
        in_set_cur_frame_trans (false),
        cur_frame_index (-1),
        nb_frames_expansion_chunk (5),
        max_frames_to_show (nb_frames_expansion_chunk),
        callstack_menu (0)
    {
        connect_debugger_signals () ;
        init_actions ();
        init_conf ();
    }

    void init_conf ()
    {
        IConfMgrSafePtr conf_mgr = workbench.get_configuration_manager ();
        if (!conf_mgr)
            return;

        int chunk = 0;
        conf_mgr->get_key_value (CONF_KEY_NEMIVER_CALLSTACK_EXPANSION_CHUNK,
                                 chunk);
        if (chunk) {
            nb_frames_expansion_chunk = chunk;
            max_frames_to_show = chunk;
        }
        conf_mgr->add_key_to_notify
                            (CONF_KEY_NEMIVER_CALLSTACK_EXPANSION_CHUNK);
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
                ""
            }
        };

        call_stack_action_group =
            Gtk::ActionGroup::create ("callstack-action-group") ;
        call_stack_action_group->set_sensitive (true) ;
        int num_actions =
            sizeof (s_call_stack_action_entries)
                /
            sizeof (ui_utils::ActionEntry) ;

        ui_utils::add_action_entries_to_action_group
            (s_call_stack_action_entries,
             num_actions,
             call_stack_action_group) ;

        workbench.get_ui_manager ()->insert_action_group
                                                (call_stack_action_group);
    }

    Gtk::Widget* load_menu (UString a_filename, UString a_widget_name)
    {
        NEMIVER_TRY
        string relative_path = Glib::build_filename ("menus", a_filename) ;
        string absolute_path ;
        THROW_IF_FAIL (perspective.build_absolute_resource_path
                (Glib::locale_to_utf8 (relative_path),
                 absolute_path)) ;

        workbench.get_ui_manager ()->add_ui_from_file
            (Glib::locale_to_utf8 (absolute_path)) ;

        NEMIVER_CATCH
        return workbench.get_ui_manager ()->get_widget (a_widget_name);
    }

    Gtk::Widget* get_call_stack_menu ()
    {
        if (!callstack_menu) {
            callstack_menu = load_menu ("callstackpopup.xml",
                                        "/CallStackPopup");
            THROW_IF_FAIL (callstack_menu);
        }
        return callstack_menu;
    }


    void on_debugger_stopped_signal (IDebugger::StopReason a_reason,
                                     bool a_has_frame,
                                     const IDebugger::Frame &a_frame,
                                     int a_thread_id,
                                     const UString &a_cookie)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD ;

        NEMIVER_TRY
        LOG_DD ("stopped, reason: " << a_reason) ;

        if (a_has_frame || a_frame.line () || a_thread_id || a_cookie.empty ()) {}

        if (a_reason == IDebugger::EXITED_SIGNALLED
            || a_reason == IDebugger::EXITED_NORMALLY
            || a_reason == IDebugger::EXITED) {
            return ;
        }

        THROW_IF_FAIL (debugger) ;
        debugger->list_frames () ;

        NEMIVER_CATCH
    }

    void on_frames_listed_signal (const vector<IDebugger::Frame> &a_stack,
                                  const UString &a_cookie)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD ;

        if (a_cookie.empty ()) {}

        NEMIVER_TRY

        THROW_IF_FAIL (debugger) ;
        waiting_for_stack_args = true ;

        //**************************************************************
        //set the frame list without frame parameters,
        //then, request IDebugger for frame parameters.
        //When the frame params arrives, we will set the
        //the frame list again, that time with the parameters.
        //Okay, this forces us to set the frame list twice,
        //but it is more robust because sometimes, the second
        //request to IDebugger fail (the one to get the parameters).
        //This way, we have at least the frame list withouht params.
        //**************************************************************
        map<int, list<IDebugger::VariableSafePtr> > frames_params ;
        set_frame_list (a_stack, frames_params) ;
        debugger->list_frames_arguments () ;

        NEMIVER_CATCH
    }

    void on_frames_params_listed_signal
            (const map<int, list<IDebugger::VariableSafePtr> > &a_frames_params,
             const UString &a_cookie)
    {
        LOG_D ("frames params listed", NMV_DEFAULT_DOMAIN) ;
        if (a_cookie.empty ()) {}

        if (waiting_for_stack_args) {
            set_frame_list (frames, a_frames_params) ;
            waiting_for_stack_args = false ;
        } else {
            LOG_D ("not in the frame setting transaction", NMV_DEFAULT_DOMAIN) ;
        }
    }

    void on_command_done_signal (const UString &a_command,
                                 const UString &a_cookie)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD ;

        if (a_cookie == "") {}

        NEMIVER_TRY

        if (in_set_cur_frame_trans
            && a_command == "select-frame") {
            in_set_cur_frame_trans = false ;
            frame_selected_signal.emit (cur_frame_index, cur_frame) ;
            LOG_DD ("sent the frame selected signal") ;
        }
        NEMIVER_CATCH
    }

    void on_selection_changed_signal ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD ;

        NEMIVER_TRY

        Gtk::TreeView *tree_view = dynamic_cast<Gtk::TreeView*> (widget.get ());
        THROW_IF_FAIL (tree_view) ;
        Glib::RefPtr<Gtk::TreeSelection> selection = tree_view->get_selection () ;
        THROW_IF_FAIL (selection) ;
        list<Gtk::TreePath> selected_rows = selection->get_selected_rows () ;
        if (selected_rows.empty ()) {return;}

        Gtk::TreeModel::iterator row_iter =
                store->get_iter (selected_rows.front ()) ;


        cur_frame_index = (*row_iter)[columns ().frame_index] ;
        cur_frame = frames[cur_frame_index] ;
        THROW_IF_FAIL (cur_frame.level () >= 0) ;
        in_set_cur_frame_trans = true ;

        //if the selected row is the "expand number of stack lines" row, trigger
        //a redraw of the with more raws.
        if ((*row_iter)[columns ().is_expansion_row]) {
            max_frames_to_show += nb_frames_expansion_chunk;
            set_frame_list (frames, params);
            return;
        }

        LOG_DD ("frame selected: '"<<  (int) cur_frame_index << "'") ;
        LOG_DD ("frame level: '" << (int) cur_frame.level () << "'") ;
        debugger->select_frame (cur_frame_index) ;

        NEMIVER_CATCH
    }

    void on_config_value_changed_signal (const UString &a_key,
                                         IConfMgr::Value &a_val)
    {
        LOG_DD ("key " << a_key << " changed");
        if (a_key == CONF_KEY_NEMIVER_CALLSTACK_EXPANSION_CHUNK) {
            nb_frames_expansion_chunk = boost::get<int> (a_val);
            max_frames_to_show = nb_frames_expansion_chunk;
        }
    }

    void connect_debugger_signals ()
    {
        THROW_IF_FAIL (debugger) ;

        debugger->stopped_signal ().connect (sigc::mem_fun
                    (*this, &CallStack::Priv::on_debugger_stopped_signal)) ;
        debugger->frames_listed_signal ().connect (sigc::mem_fun
                    (*this, &CallStack::Priv::on_frames_listed_signal)) ;
        debugger->frames_arguments_listed_signal ().connect (sigc::mem_fun
                    (*this, &CallStack::Priv::on_frames_params_listed_signal)) ;
        debugger->command_done_signal ().connect (sigc::mem_fun
                    (*this, &CallStack::Priv::on_command_done_signal)) ;
    }

    void on_call_stack_button_press_signal (GdkEventButton *a_event)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD ;

        NEMIVER_TRY

        // right-clicking should pop up a context menu
        if ((a_event->type == GDK_BUTTON_PRESS) && (a_event->button == 3)) {
            popup_call_stack_menu (a_event) ;
        }

        NEMIVER_CATCH
    }

    void popup_call_stack_menu (GdkEventButton *a_event)
    {
        THROW_IF_FAIL (a_event);
        THROW_IF_FAIL (widget);

        Gtk::Menu *menu = dynamic_cast<Gtk::Menu*> (get_call_stack_menu ()) ;
        THROW_IF_FAIL (menu) ;

        //only pop up a menu if a row exists at that position
        Gtk::TreeModel::Path path;
        Gtk::TreeViewColumn* column=NULL;
        int cell_x=0, cell_y=0;
        if (widget->get_path_at_pos (static_cast<int> (a_event->x),
                                     static_cast<int> (a_event->y),
                                     path,
                                     column,
                                     cell_x,
                                     cell_y)) {
            menu->popup (a_event->button, a_event->time) ;
        }
    }

    void on_call_stack_copy_to_clipboard_action ()
    {
        NEMIVER_TRY

        int i = 0;
        std::ostringstream frame_stream;
        vector<IDebugger::Frame>::const_iterator frame_iter;
        map<int, list<IDebugger::VariableSafePtr> >::const_iterator params_iter;
        // convert list of stack frames to a string (FIXME: maybe Frame should
        // just implement operator<< ?
        for (frame_iter= frames.begin(), params_iter = params.begin();
             frame_iter != frames.end ();
             ++frame_iter)
        {
            frame_stream << "#" << UString::from_int(i++) << "  " <<
                frame_iter->function_name () << " (";
            // if the params map exists, add the function params to the stack trace
            if (params_iter != params.end())
            {
                for (list<IDebugger::VariableSafePtr>::const_iterator it =
                        params_iter->second.begin();
                        it != params_iter->second.end(); ++it)
                {
                    if (it != params_iter->second.begin())
                        frame_stream << ", ";
                    frame_stream << (*it)->name() << "=" << (*it)->value();
                }
                // advance to the list of params for the next stack frame
                ++params_iter;
            }
            frame_stream << ") at " << frame_iter->file_name () << ":"
                << UString::from_int(frame_iter->line ()) << std::endl;
        }
        Gtk::Clipboard::get ()->set_text (frame_stream.str());

        NEMIVER_CATCH
    }

    Gtk::Widget* get_widget ()
    {
        if (!widget) {return 0;}
        return widget.get () ;
    }

    void build_widget ()
    {
        if (widget) {
            return ;
        }
        store = Gtk::ListStore::create (columns ());
        Gtk::TreeView *tree_view = new Gtk::TreeView (store) ;
        THROW_IF_FAIL (tree_view) ;
        widget.reset (tree_view) ;
        tree_view->append_column (_("Frame"), columns ().frame_index_caption) ;
        tree_view->append_column (_("Location"), columns ().location) ;
        tree_view->append_column (_("Function"), columns ().function_name) ;
        tree_view->append_column (_("Arguments"), columns ().function_args) ;
        tree_view->set_headers_visible (true) ;
        tree_view->get_selection ()->set_mode (Gtk::SELECTION_SINGLE) ;

        on_selection_changed_connection =
            tree_view->get_selection ()->signal_changed ().connect
            (sigc::mem_fun (*this,
                            &CallStack::Priv::on_selection_changed_signal)) ;

        Gtk::TreeViewColumn* column =
                            tree_view->get_column (CallStackCols::FUNCTION_NAME) ;
        THROW_IF_FAIL (column) ;
        column->set_clickable (false) ;
        column->set_reorderable (false) ;

        THROW_IF_FAIL (column = tree_view->get_column (CallStackCols::LOCATION)) ;
        column->set_clickable (false) ;
        column->set_reorderable (false) ;

        THROW_IF_FAIL (column = tree_view->get_column
                                                (CallStackCols::FUNCTION_ARGS)) ;
        column->set_clickable (false) ;
        column->set_reorderable (false) ;

        tree_view->signal_button_press_event ().connect_notify
            (sigc::mem_fun (*this,
                            &Priv::on_call_stack_button_press_signal));
    }

    void set_frame_list
                (const vector<IDebugger::Frame> &a_frames,
                 const map<int, list<IDebugger::VariableSafePtr> >&a_params,
                 bool a_emit_signal=false)
    {
        THROW_IF_FAIL (get_widget ()) ;

        clear_frame_list () ;
        frames = a_frames ;

        // save the list of params around so that we can use it when converting
        // to a string later
        params = a_params;

        Gtk::TreeModel::iterator store_iter ;
        unsigned nb_frames = MIN (a_frames.size (), max_frames_to_show);
        unsigned i = 0;
        for (i = 0; i < nb_frames; ++i) {
            store_iter = store->append () ;
            (*store_iter)[columns ().is_expansion_row] = false;
            (*store_iter)[columns ().function_name] = a_frames[i].function_name ();
            if (!a_frames[i].file_name ().empty ()) {
                (*store_iter)[columns ().location] =
                    a_frames[i].file_name () + ":"
                    + UString::from_int (a_frames[i].line ()) ;
            } else {
                (*store_iter)[columns ().location] =
                    a_frames[i].address ();
            }

            (*store_iter)[columns ().frame_index] = i ;
            (*store_iter)[columns ().frame_index_caption] =  UString::from_int (i) ;
            UString params_string = "(";
            map<int, list<IDebugger::VariableSafePtr> >::const_iterator iter ;
            list<IDebugger::VariableSafePtr>::const_iterator params_iter ;
            iter = a_params.find (i) ;
            if (iter != a_params.end ()) {
                LOG_D ("for frame "
                       << (int) i
                       << " NB params: "
                       << (int) iter->second.size (), NMV_DEFAULT_DOMAIN) ;

                params_iter = iter->second.begin () ;
                if (params_iter != iter->second.end ()) {
                    if (*params_iter) {
                        params_string += (*params_iter)->name () ;
                    }
                    ++params_iter ;
                }
                for ( ; params_iter != iter->second.end (); ++params_iter) {
                    if (!*params_iter) {continue;}
                     params_string += ", " + (*params_iter)->name () ;
                }
            }
            params_string += ")" ;
            (*store_iter)[columns ().function_args] = params_string ;
        }
        if (a_frames.size () && i < a_frames.size () - 1) {
            store_iter = store->append () ;
            UString msg;
            msg.printf (_("(Click here to see the next %d rows of the %d "
                          "call stack rows)"),
                        nb_frames_expansion_chunk,
                        frames.size ());
            (*store_iter)[columns ().frame_index_caption] = "...";
            (*store_iter)[columns ().location] = msg ;
            (*store_iter)[columns ().is_expansion_row] = true;
        }

        Gtk::TreeView *tree_view =
            dynamic_cast<Gtk::TreeView*> (widget.get ()) ;
        THROW_IF_FAIL (tree_view) ;

        if (!a_emit_signal) {
            on_selection_changed_connection.block () ;
        }
        tree_view->get_selection ()->select (Gtk::TreePath ("0")) ;
        if (!a_emit_signal) {
            on_selection_changed_connection.unblock () ;
        }
    }

    void clear_frame_list ()
    {
        THROW_IF_FAIL (store) ;
        store->clear () ;
    }
};//end struct CallStack::Priv

CallStack::CallStack (IDebuggerSafePtr &a_debugger,
                      IWorkbench& a_workbench,
                      IPerspective &a_perspective)
{
    THROW_IF_FAIL (a_debugger) ;
    m_priv.reset (new Priv (a_debugger, a_workbench, a_perspective)) ;
}

CallStack::~CallStack ()
{
    LOG_D ("deleted", "destructor-domain") ;
}

bool
CallStack::is_empty ()
{
    THROW_IF_FAIL (m_priv) ;
    return m_priv->frames.empty () ;
}

const vector<IDebugger::Frame>&
CallStack::frames () const
{
    THROW_IF_FAIL (m_priv) ;
    return m_priv->frames ;
}

Gtk::Widget&
CallStack::widget () const
{
    THROW_IF_FAIL (m_priv) ;

    if (!m_priv->get_widget ()) {
        m_priv->build_widget () ;
        THROW_IF_FAIL (m_priv->widget) ;
    }
    return *m_priv->get_widget () ;
}

void
CallStack::update_stack ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;
    THROW_IF_FAIL (m_priv) ;
    THROW_IF_FAIL (m_priv->debugger) ;

    m_priv->debugger->list_frames () ;
}

void
CallStack::clear ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;
    THROW_IF_FAIL (m_priv) ;

    if (m_priv->store) {
        m_priv->store->clear () ;
    }
    m_priv->cur_frame_index = - 1 ;
    m_priv->waiting_for_stack_args  = false ;
    m_priv->in_set_cur_frame_trans = false ;
}

sigc::signal<void, int, const IDebugger::Frame&>&
CallStack::frame_selected_signal () const
{
    THROW_IF_FAIL (m_priv) ;
    return m_priv->frame_selected_signal ;
}
}//end namespace nemiver


