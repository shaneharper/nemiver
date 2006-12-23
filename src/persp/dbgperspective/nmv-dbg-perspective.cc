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

#include <algorithm>
#include <iostream>
#include <fstream>
#include <glib/gi18n.h>
#include <libgnomevfs/gnome-vfs-mime-utils.h>
#include <gtksourceviewmm/init.h>
#include <gtksourceviewmm/sourcelanguagesmanager.h>
#include <gtkmm/clipboard.h>
#include <gtkmm/separatortoolitem.h>
#include "nmv-dbg-perspective.h"
#include "nmv-source-editor.h"
#include "nmv-ui-utils.h"
#include "nmv-env.h"
#include "nmv-run-program-dialog.h"
#include "nmv-load-core-dialog.h"
#include "nmv-locate-file-dialog.h"
#include "nmv-saved-sessions-dialog.h"
#include "nmv-proc-list-dialog.h"
#include "nmv-ui-utils.h"
#include "nmv-sess-mgr.h"
#include "nmv-date-utils.h"
#include "nmv-call-stack.h"
#include "nmv-ephy-throbber.h"
#include "nmv-local-vars-inspector.h"
#include "nmv-terminal.h"
#include "nmv-breakpoints-view.h"
#include "nmv-i-conf-mgr.h"
#include "nmv-preferences-dialog.h"
#include "nmv-popup-tip.h"
#include "nmv-thread-list.h"
#include "nmv-var-inspector-dialog.h"

using namespace std ;
using namespace nemiver::common ;
using namespace nemiver::ui_utils;
using namespace gtksourceview ;

NEMIVER_BEGIN_NAMESPACE (nemiver)

const char *SET_BREAKPOINT    = "nmv-set-breakpoint" ;
const char *CONTINUE          = "nmv-continue" ;
const char *STOP_DEBUGGER     = "nmv-stop-debugger" ;
const char *RUN_DEBUGGER      = "nmv-run-debugger" ;
const char *LINE_POINTER      = "nmv-line-pointer" ;
const char *RUN_TO_CURSOR     = "nmv-run-to-cursor" ;
const char *STEP_INTO         = "nmv-step-into" ;
const char *STEP_OVER         = "nmv-step-over" ;
const char *STEP_OUT          = "nmv-step-out" ;

const char *SESSION_NAME = "sessionname" ;
const char *PROGRAM_NAME= "programname" ;
const char *PROGRAM_ARGS= "programarguments" ;
const char *PROGRAM_CWD= "programcwd" ;
const char * DBG_PERSPECTIVE_MOUSE_MOTION_DOMAIN =
                                "dbg-perspective-mouse-motion-domain" ;

static const UString CONF_KEY_NEMIVER_SOURCE_DIRS =
                "/apps/nemiver/dbgperspective/source-search-dirs" ;
static const UString CONF_KEY_SHOW_DBG_ERROR_DIALOGS =
                "/apps/nemiver/dbgperspective/show-dbg-error-dialogs";
static const UString CONF_KEY_SHOW_SOURCE_LINE_NUMBERS =
                "/apps/nemiver/dbgperspective/show-source-line-numbers" ;
static const UString CONF_KEY_HIGHLIGHT_SOURCE_CODE =
                "/apps/nemiver/dbgperspective/highlight-source-code" ;
static const UString CONF_KEY_STATUS_WIDGET_MINIMUM_WIDTH=
                "/apps/nemiver/dbgperspective/status-widget-minimum-width" ;
static const UString CONF_KEY_STATUS_WIDGET_MINIMUM_HEIGHT=
                "/apps/nemiver/dbgperspective/status-widget-minimum-height" ;

const Gtk::StockID STOCK_SET_BREAKPOINT (SET_BREAKPOINT) ;
const Gtk::StockID STOCK_CONTINUE (CONTINUE) ;
const Gtk::StockID STOCK_STOP_DEBUGGER (STOP_DEBUGGER) ;
const Gtk::StockID STOCK_RUN_DEBUGGER (RUN_DEBUGGER) ;
const Gtk::StockID STOCK_LINE_POINTER (LINE_POINTER) ;
const Gtk::StockID STOCK_RUN_TO_CURSOR (RUN_TO_CURSOR) ;
const Gtk::StockID STOCK_STEP_INTO (STEP_INTO) ;
const Gtk::StockID STOCK_STEP_OVER (STEP_OVER) ;
const Gtk::StockID STOCK_STEP_OUT (STEP_OUT) ;

class DBGPerspective : public IDBGPerspective {
    //non copyable
    DBGPerspective (const IPerspective&) ;
    DBGPerspective& operator= (const IPerspective&) ;
    struct Priv ;
    SafePtr<Priv> m_priv ;

private:

    struct SlotedButton : Gtk::Button {
        UString file_path;
        DBGPerspective *perspective ;
        SafePtr<Gtk::Tooltips> tooltips ;

        SlotedButton () :
            Gtk::Button (),
            perspective (NULL)
        {}

        SlotedButton (const Gtk::StockID &a_id) :
            Gtk::Button (a_id),
            perspective (NULL)
        {}

        void on_clicked ()
        {
            if (perspective) {
                perspective->close_file (file_path) ;
            }
        }

        ~SlotedButton ()
        {
        }
    };

    //************
    //<signal slots>
    //************
    void on_open_action () ;
    void on_close_action () ;
    void on_execute_program_action () ;
    void on_load_core_file_action () ;
    void on_attach_to_program_action () ;
    void on_saved_sessions_action () ;
    void on_current_session_properties_action () ;
    void on_stop_debugger_action ();
    void on_run_action () ;
    void on_save_session_action () ;
    void on_next_action () ;
    void on_step_into_action () ;
    void on_step_out_action () ;
    void on_continue_action () ;
    void on_continue_until_action () ;
    void on_toggle_breakpoint_action () ;
    void on_inspect_variable_action () ;
    void on_show_commands_action () ;
    void on_show_errors_action () ;
    void on_show_target_output_action () ;
    void on_breakpoint_delete_action (const IDebugger::BreakPoint& a_breakpoint) ;
    void on_breakpoint_go_to_source_action (const IDebugger::BreakPoint& a_breakpoint) ;
    void on_thread_list_thread_selected_signal (int a_tid) ;

    void on_switch_page_signal (GtkNotebookPage *a_page, guint a_page_num) ;

    void on_attached_to_target_signal (bool a_is_attached) ;

    void on_debugger_ready_signal (bool a_is_ready) ;

    void on_going_to_run_target_signal () ;

    void on_insert_in_command_view_signal (const Gtk::TextBuffer::iterator &a_iter,
                                           const Glib::ustring &a_text,
                                           int a_dont_know) ;

    void on_source_view_markers_region_clicked_signal (int a_line) ;

    bool on_button_pressed_in_source_view_signal (GdkEventButton *a_event) ;

    bool on_motion_notify_event_signal (GdkEventMotion *a_event) ;

    void on_leave_notify_event_signal (GdkEventCrossing *a_event) ;

    bool on_mouse_immobile_timer_signal () ;

    void on_shutdown_signal () ;

    void on_show_command_view_changed_signal (bool) ;

    void on_show_target_output_view_changed_signal (bool) ;

    void on_show_log_view_changed_signal (bool) ;

    void on_conf_key_changed_signal (const UString &a_key,
                                     IConfMgr::Value &a_value) ;

    void on_debugger_got_target_info_signal (int a_pid,
                                           const UString &a_exe_path) ;

    void on_debugger_console_message_signal (const UString &a_msg) ;

    void on_debugger_target_output_message_signal (const UString &a_msg);

    void on_debugger_log_message_signal (const UString &a_msg) ;

    void on_debugger_command_done_signal (const UString &a_command_name,
                                          const UString &a_cookie) ;

    void on_debugger_breakpoints_set_signal
                                (const map<int, IDebugger::BreakPoint> &) ;

    void on_debugger_breakpoint_deleted_signal
                                        (const IDebugger::BreakPoint&, int) ;

    void on_debugger_stopped_signal (const UString &a_reason,
                                     bool a_has_frame,
                                     const IDebugger::Frame &,
                                     int a_thread_id) ;
    void on_program_finished_signal () ;
    void on_frame_selected_signal (int, const IDebugger::Frame &) ;

    void on_debugger_running_signal () ;

    void on_signal_received_by_target_signal (const UString &a_signal,
                                              const UString &a_meaning) ;

    void on_debugger_error_signal (const UString &a_msg) ;

    void on_debugger_state_changed_signal (IDebugger::State a_state) ;

    void on_debugger_variable_value_signal
                                    (const UString &a_var_name,
                                     const IDebugger::VariableSafePtr &a_var) ;
    //************
    //</signal slots>
    //************

    string build_resource_path (const UString &a_dir, const UString &a_name) ;
    void add_stock_icon (const UString &a_stock_id,
                         const UString &icon_dir,
                         const UString &icon_name) ;
    void add_perspective_menu_entries () ;
    void init_perspective_menu_entries () ;
    void add_perspective_toolbar_entries () ;
    void init_icon_factory () ;
    void init_actions () ;
    void init_toolbar () ;
    void init_body () ;
    void init_signals () ;
    void init_debugger_signals () ;
    void append_source_editor (SourceEditor &a_sv,
                               const UString &a_path) ;
    SourceEditor* get_current_source_editor () ;
    ISessMgr* session_manager_ptr () ;
    UString get_current_file_path () ;
    SourceEditor* get_source_editor_from_path (const UString &a_path,
                                               bool a_basename_only=false) ;
    IWorkbench& workbench () const ;
    void bring_source_as_current (const UString &a_path) ;
    int get_n_pages () ;
    void popup_source_view_contextual_menu (GdkEventButton *a_event) ;
    void record_and_save_new_session () ;
    void record_and_save_session (ISessMgr::Session &a_session) ;
    IProcMgr* get_process_manager () ;
    void try_to_request_show_variable_value_at_position (int a_x, int a_y) ;
    void show_underline_tip_at_position (int a_x, int a_y,
                                         const UString &a_text);
    void restart_mouse_immobile_timer () ;
    void stop_mouse_immobile_timer () ;
    PopupTip& get_popup_tip () ;

public:

    DBGPerspective () ;

    virtual ~DBGPerspective () ;

    void get_info (Info &a_info) const ;

    void do_init () ;

    void do_init (IWorkbench *a_workbench) ;

    const UString& get_perspective_identifier () ;

    void get_toolbars (list<Gtk::Widget*> &a_tbs)  ;

    Gtk::Widget* get_body ()  ;

    void edit_workbench_menu () ;

    void open_file () ;

    bool open_file (const UString &a_path,
                    int current_line=-1) ;

    void close_current_file () ;

    void close_file (const UString &a_path) ;

    ISessMgr& session_manager () ;

    void execute_session (ISessMgr::Session &a_session) ;

    void execute_program () ;

    void execute_program (const UString &a_prog_and_args,
                          const map<UString, UString> &a_env,
                          const UString &a_cwd=".") ;

    void execute_program (const UString &a_prog,
                          const UString &a_args,
                          const map<UString, UString> &a_env,
                          const UString &a_cwd,
                          const vector<IDebugger::BreakPoint> &a_breaks) ;

    void attach_to_program () ;
    void attach_to_program (unsigned int a_pid) ;
    void load_core_file () ;
    void load_core_file (const UString &a_prog_file,
                         const UString &a_core_file_path) ;
    void saved_sessions () ;
    void edit_preferences () ;

    void run () ;
    void stop () ;
    void step_over () ;
    void step_into () ;
    void step_out () ;
    void do_continue () ;
    void do_continue_until () ;
    void set_breakpoint () ;
    void set_breakpoint (const UString &a_file,
                         int a_line) ;
    void append_breakpoints (const map<int, IDebugger::BreakPoint> &a_breaks) ;
    bool get_breakpoint_number (const UString &a_file_name,
                                int a_linenum,
                                int &a_break_num) ;
    bool delete_breakpoint () ;
    bool delete_breakpoint (int a_breakpoint_num) ;
    bool delete_breakpoint (const UString &a_file_path,
                            int a_linenum) ;
    bool is_breakpoint_set_at_line (const UString &a_file_path,
                                    int a_linenum) ;
    void toggle_breakpoint (const UString &a_file_path,
                            int a_linenum) ;
    void inspect_variable () ;
    void inspect_variable (const UString &a_variable_name) ;
    void toggle_breakpoint () ;
    void append_visual_breakpoint (const UString &a_file_name,
                                   int a_linenum) ;
    void delete_visual_breakpoint (const UString &a_file_name, int a_linenum) ;
    void delete_visual_breakpoint (int a_breaknum) ;

    IDebuggerSafePtr& debugger () ;

    IConfMgr& conf_mgr () ;

    Gtk::TextView& get_command_view () ;

    Gtk::ScrolledWindow& get_command_view_scrolled_win () ;

    Gtk::TextView& get_target_output_view () ;

    Gtk::ScrolledWindow& get_target_output_view_scrolled_win () ;

    Gtk::TextView& get_log_view () ;

    Gtk::ScrolledWindow& get_log_view_scrolled_win () ;

    CallStack& get_call_stack () ;

    Gtk::ScrolledWindow& get_call_stack_scrolled_win () ;

    LocalVarsInspector& get_local_vars_inspector () ;

    Gtk::ScrolledWindow& get_local_vars_inspector_scrolled_win () ;

    Terminal& get_terminal () ;

    Gtk::Box& get_terminal_box () ;

    Gtk::ScrolledWindow& get_breakpoints_scrolled_win () ;

    BreakpointsView& get_breakpoints_view () ;

    ThreadList& get_thread_list () ;

    void set_show_command_view (bool) ;

    void set_show_target_output_view (bool) ;

    void set_show_log_view (bool) ;

    void set_show_call_stack_view (bool) ;

    void set_show_variables_editor_view (bool) ;

    void set_show_terminal_view (bool) ;

    void set_show_breakpoints_view (bool) ;

    void add_text_to_command_view (const UString &a_text,
                                   bool a_no_repeat=false) ;

    void add_text_to_target_output_view (const UString &a_text) ;

    void add_text_to_log_view (const UString &a_text) ;

    void set_where (const UString &a_path, int line) ;

    void unset_where () ;

    Gtk::Widget* load_menu (UString a_filename, UString a_widget_name) ;
    Gtk::Widget* get_contextual_menu () ;
    Gtk::Widget* get_call_stack_menu () ;

    void init_conf_mgr () ;

    vector<UString>& get_source_dirs () ;

    bool find_file_in_source_dirs (const UString &a_file_name,
                                   UString &a_file_path) ;

    sigc::signal<void, bool>& show_command_view_signal () ;
    sigc::signal<void, bool>& show_target_output_view_signal () ;
    sigc::signal<void, bool>& show_log_view_signal () ;
    sigc::signal<void, bool>& activated_signal () ;
    sigc::signal<void, bool>& attached_to_target_signal () ;
    sigc::signal<void, bool>& debugger_ready_signal () ;
    sigc::signal<void>& going_to_run_target_signal () ;
};//end class DBGPerspective

struct RefGObject {
    void operator () (Glib::Object *a_object)
    {
        if (a_object) {a_object->reference ();}
    }
};

struct UnrefGObject {
    void operator () (Glib::Object *a_object)
    {
        if (a_object) {a_object->unreference ();}
    }
};

struct RefGObjectNative {
    void operator () (void *a_object)
    {
        if (a_object && G_IS_OBJECT (a_object)) {
            g_object_ref (G_OBJECT (a_object));
        }
    }
};

struct UnrefGObjectNative {
    void operator () (void *a_object)
    {
        if (a_object && G_IS_OBJECT (a_object)) {
            g_object_unref (G_OBJECT (a_object)) ;
        }
    }
};


struct DBGPerspective::Priv {
    bool initialized ;
    bool reused_session ;
    bool debugger_has_just_run ;
    UString prog_path ;
    UString prog_args ;
    UString prog_cwd ;
    map<UString, UString> env_variables ;
    list<UString> search_paths ;
    Glib::RefPtr<Gnome::Glade::Xml> body_glade ;
    SafePtr<Gtk::Window> body_window ;
    SafePtr<Gtk::TextView> command_view ;
    SafePtr<Gtk::ScrolledWindow> command_view_scrolled_win ;
    SafePtr<Gtk::TextView> target_output_view;
    SafePtr<Gtk::ScrolledWindow> target_output_view_scrolled_win;
    SafePtr<Gtk::TextView> log_view ;
    SafePtr<Gtk::ScrolledWindow> log_view_scrolled_win ;
    SafePtr<CallStack> call_stack ;
    SafePtr<Gtk::ScrolledWindow> call_stack_scrolled_win ;

    Glib::RefPtr<Gtk::ActionGroup> target_connected_action_group ;
    Glib::RefPtr<Gtk::ActionGroup> target_not_started_action_group ;
    Glib::RefPtr<Gtk::ActionGroup> debugger_ready_action_group ;
    Glib::RefPtr<Gtk::ActionGroup> debugger_busy_action_group ;
    Glib::RefPtr<Gtk::ActionGroup> default_action_group;
    Glib::RefPtr<Gtk::ActionGroup> opened_file_action_group;
    Glib::RefPtr<Gtk::UIManager> ui_manager ;
    Glib::RefPtr<Gtk::IconFactory> icon_factory ;
    Gtk::UIManager::ui_merge_id menubar_merge_id ;
    Gtk::UIManager::ui_merge_id toolbar_merge_id ;
    Gtk::UIManager::ui_merge_id contextual_menu_merge_id;
    Gtk::Widget *contextual_menu ;
    Gtk::Box *top_box;
    SafePtr<Gtk::Paned> body_main_paned ;
    IWorkbench *workbench ;
    SafePtr<Gtk::HBox> toolbar ;
    ThrobberSafePtr throbber ;
    sigc::signal<void, bool> activated_signal;
    sigc::signal<void, bool> attached_to_target_signal;
    sigc::signal<void, bool> debugger_ready_signal;
    sigc::signal<void> going_to_run_target_signal ;
    sigc::signal<void, bool> show_command_view_signal  ;
    sigc::signal<void, bool> show_target_output_view_signal  ;
    sigc::signal<void, bool> show_log_view_signal ;
    bool command_view_is_visible ;
    bool target_output_view_is_visible ;
    bool log_view_is_visible ;
    bool call_stack_view_is_visible ;
    bool variables_editor_view_is_visible ;
    bool terminal_view_is_visible ;
    bool breakpoints_view_is_visible ;
    Gtk::Notebook *sourceviews_notebook ;
    map<UString, int> path_2_pagenum_map ;
    map<UString, int> basename_2_pagenum_map ;
    map<int, SourceEditor*> pagenum_2_source_editor_map ;
    map<int, UString> pagenum_2_path_map ;
    Gtk::Notebook *statuses_notebook ;
    SafePtr<LocalVarsInspector> variables_editor ;
    SafePtr<Gtk::ScrolledWindow> variables_editor_scrolled_win ;
    SafePtr<Terminal> terminal ;
    SafePtr<Gtk::Box> terminal_box ;
    SafePtr<Gtk::ScrolledWindow> breakpoints_scrolled_win ;
    SafePtr<BreakpointsView> breakpoints_view ;
    SafePtr<ThreadList> thread_list ;

    int current_page_num ;
    IDebuggerSafePtr debugger ;
    map<int, IDebugger::BreakPoint> breakpoints ;
    ISessMgrSafePtr session_manager ;
    ISessMgr::Session session ;
    IProcMgrSafePtr process_manager ;
    UString last_command_text ;
    vector<UString> source_dirs ;
    bool show_dbg_errors ;
    sigc::connection timeout_source_connection ;
    //**************************************
    //<detect mouse immobility > N seconds
    //**************************************
    int mouse_in_source_editor_x ;
    int mouse_in_source_editor_y ;
    //**************************************
    //</detect mouse immobility > N seconds
    //**************************************

    //****************************************
    //<variable value popup tip related data>
    //****************************************
    SafePtr<PopupTip> popup_tip ;
    bool in_show_var_value_at_pos_transaction ;
    UString var_to_popup ;
    int var_popup_tip_x ;
    int var_popup_tip_y ;
    //****************************************
    //</variable value popup tip related data>
    //****************************************


    Priv () :
        initialized (false),
        reused_session (false),
        debugger_has_just_run (false),
        menubar_merge_id (0),
        toolbar_merge_id (0),
        contextual_menu_merge_id(0),
        contextual_menu (0),
        top_box (0),
        /*body_main_paned (0),*/
        workbench (0),
        command_view_is_visible (false),
        target_output_view_is_visible (false),
        log_view_is_visible (false),
        call_stack_view_is_visible (false),
        variables_editor_view_is_visible (false),
        terminal_view_is_visible (false),
        breakpoints_view_is_visible (false),
        sourceviews_notebook (NULL),
        statuses_notebook (NULL),
        current_page_num (0),
        show_dbg_errors (false),
        mouse_in_source_editor_x (0),
        mouse_in_source_editor_y (0),
        in_show_var_value_at_pos_transaction (false),
        var_popup_tip_x (0),
        var_popup_tip_y (0)
    {}
};//end struct DBGPerspective::Priv

enum ViewsIndex{
    COMMAND_VIEW_INDEX=0,
    TARGET_OUTPUT_VIEW_INDEX,
    ERROR_VIEW_INDEX,
    CALL_STACK_VIEW_INDEX,
    VARIABLES_VIEW_INDEX,
    TERMINAL_VIEW_INDEX,
    BREAKPOINTS_VIEW_INDEX
};

#ifndef CHECK_P_INIT
#define CHECK_P_INIT THROW_IF_FAIL(m_priv && m_priv->initialized) ;
#endif

ostream&
operator<< (ostream &a_out,
            const IDebugger::Frame &a_frame)
{
    a_out << "file-full-name: " << a_frame.file_full_name () << "\n"
          << "file-name: "      << a_frame.file_name () << "\n"
          << "line number: "    << a_frame.line () << "\n";

    return a_out ;
}

//****************************
//<slots>
//***************************
void
DBGPerspective::on_open_action ()
{
    NEMIVER_TRY

    open_file () ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_close_action ()
{
    NEMIVER_TRY

    close_current_file () ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_execute_program_action ()
{
    NEMIVER_TRY

    execute_program () ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_load_core_file_action ()
{
    NEMIVER_TRY

    load_core_file () ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_attach_to_program_action ()
{
    NEMIVER_TRY

    attach_to_program () ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_saved_sessions_action ()
{
    NEMIVER_TRY

    saved_sessions () ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_current_session_properties_action ()
{
    NEMIVER_TRY

    edit_preferences () ;
    NEMIVER_CATCH
}

void
DBGPerspective::on_run_action ()
{
    NEMIVER_TRY

    run () ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_save_session_action ()
{
    NEMIVER_TRY

    if (m_priv->reused_session) {
        record_and_save_session (m_priv->session) ;
        LOG_DD ("saved current session") ;
    } else {
        LOG_DD ("recorded a new session") ;
        record_and_save_new_session () ;
    }

    NEMIVER_CATCH
}

void
DBGPerspective::on_stop_debugger_action (void)
{
    LOG_FUNCTION_SCOPE_NORMAL_D (NMV_DEFAULT_DOMAIN) ;
    NEMIVER_TRY

    stop () ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_next_action ()
{
    NEMIVER_TRY

    step_over () ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_step_into_action ()
{
    NEMIVER_TRY

    step_into () ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_step_out_action ()
{
    NEMIVER_TRY

    step_out () ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_continue_action ()
{
    NEMIVER_TRY
    do_continue () ;
    NEMIVER_CATCH
}

void
DBGPerspective::on_continue_until_action ()
{
    NEMIVER_TRY
    do_continue_until () ;
    NEMIVER_CATCH
}

void
DBGPerspective::on_toggle_breakpoint_action ()
{
    NEMIVER_TRY
    toggle_breakpoint () ;
    NEMIVER_CATCH
}

void
DBGPerspective::on_inspect_variable_action ()
{
    NEMIVER_TRY
    inspect_variable () ;
    NEMIVER_CATCH
}

void
DBGPerspective::on_show_commands_action ()
{
    NEMIVER_TRY
    Glib::RefPtr<Gtk::ToggleAction> action =
        Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic
            (workbench ().get_ui_manager ()->get_action
                 ("/MenuBar/MenuBarAdditions/ViewMenu/ShowCommandsMenuItem")) ;
    THROW_IF_FAIL (action) ;

    set_show_command_view (action->get_active ()) ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_show_errors_action ()
{
    NEMIVER_TRY

    Glib::RefPtr<Gtk::ToggleAction> action =
        Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic
            (workbench ().get_ui_manager ()->get_action
                 ("/MenuBar/MenuBarAdditions/ViewMenu/ShowErrorsMenuItem")) ;
    THROW_IF_FAIL (action) ;

    set_show_log_view (action->get_active ()) ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_show_target_output_action ()
{
    NEMIVER_TRY

    Glib::RefPtr<Gtk::ToggleAction> action =
        Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic
            (workbench ().get_ui_manager ()->get_action
                 ("/MenuBar/MenuBarAdditions/ViewMenu/ShowTargetOutputMenuItem")) ;
    THROW_IF_FAIL (action) ;

    set_show_target_output_view (action->get_active ()) ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_breakpoint_delete_action (const IDebugger::BreakPoint& a_breakpoint)
{
    NEMIVER_TRY
    delete_breakpoint (a_breakpoint.number ());
    NEMIVER_CATCH
}

void
DBGPerspective::on_breakpoint_go_to_source_action (const IDebugger::BreakPoint& a_breakpoint)
{
    // FIXME: this should put the same effort into finding the source file that
    // append_visual_breakpoint() does.  Maybe this should be abstracted out
    // somehow
    NEMIVER_TRY
    UString file_path = a_breakpoint.file_full_name ();
    if (file_path == "") {
        file_path = a_breakpoint.file_name () ;
        if (!find_file_in_source_dirs (file_path, file_path)) {
            display_warning (_("File path info is missing "
                             "for breakpoint '")
                             + UString::from_int (a_breakpoint.number ()) + "'") ;
            return ;
        }
    }

    bring_source_as_current (file_path);
    SourceEditor *source_editor = get_source_editor_from_path (file_path) ;
    THROW_IF_FAIL (source_editor);
    source_editor->scroll_to_line (a_breakpoint.line ()) ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_thread_list_thread_selected_signal (int a_tid)
{
    if (a_tid) {}

    NEMIVER_TRY

    get_call_stack ().update_stack () ;
    get_local_vars_inspector ().show_local_variables_of_current_function () ;

    NEMIVER_CATCH
}


void
DBGPerspective::on_switch_page_signal (GtkNotebookPage *a_page, guint a_page_num)
{
    if (a_page) {}

    NEMIVER_TRY
    m_priv->current_page_num = a_page_num;
    NEMIVER_CATCH
}

void
DBGPerspective::on_debugger_ready_signal (bool a_is_ready)
{
    NEMIVER_TRY

    THROW_IF_FAIL (m_priv) ;
    THROW_IF_FAIL (m_priv->debugger_ready_action_group) ;
    THROW_IF_FAIL (m_priv->throbber) ;

    if (a_is_ready) {
        m_priv->throbber->stop () ;
        m_priv->debugger_ready_action_group->set_sensitive (true) ;
        m_priv->target_not_started_action_group->set_sensitive (true) ;
        m_priv->debugger_busy_action_group->set_sensitive (false) ;
        attached_to_target_signal ().emit (true) ;
    } else {
        m_priv->target_not_started_action_group->set_sensitive (false) ;
        m_priv->debugger_ready_action_group->set_sensitive (false) ;
        m_priv->debugger_busy_action_group->set_sensitive (true) ;
    }

    NEMIVER_CATCH
}

void
DBGPerspective::on_going_to_run_target_signal ()
{
    NEMIVER_TRY
    get_local_vars_inspector ().re_init_widget () ;
    NEMIVER_CATCH
}

void
DBGPerspective::on_attached_to_target_signal (bool a_is_ready)
{
    NEMIVER_TRY

    if (a_is_ready) {
        m_priv->target_connected_action_group->set_sensitive (true) ;
    } else {
        m_priv->target_connected_action_group->set_sensitive (false) ;
    }

    NEMIVER_CATCH
}

void
DBGPerspective::on_insert_in_command_view_signal (const Gtk::TextBuffer::iterator &a_it,
                                                  const Glib::ustring &a_text,
                                                  int a_dont_know)
{
    NEMIVER_TRY
    if (a_dont_know) {}
    if (a_text == "") {return;}

    if (a_text == "\n") {
        //get the command that is on the current line
        UString line ;
        Gtk::TextBuffer::iterator iter = a_it, tmp_iter, eol_iter = a_it ;
        for (;;) {
            --iter ;
            if (iter.is_start ()) {break;}
            tmp_iter = iter ;
            if (tmp_iter.get_char () == ')'
                && (--tmp_iter).get_char () == 'b'
                && (--tmp_iter).get_char () == 'd'
                && (--tmp_iter).get_char () == 'g'
                && (--tmp_iter).get_char () == '(') {
                ++ iter ;
                line = iter.get_visible_text (eol_iter) ;
                break ;
            }
        }
        if (!line.empty ()) {
            IDebuggerSafePtr dbg = debugger () ;
            THROW_IF_FAIL (dbg) ;
            //dbg->execute_command (IDebugger::Command (line)) ;
            m_priv->last_command_text = "" ;
        }
    }
    NEMIVER_CATCH
}

void
DBGPerspective::on_source_view_markers_region_clicked_signal (int a_line)
{
    NEMIVER_TRY

    SourceEditor *cur_editor = get_current_source_editor () ;
    THROW_IF_FAIL (cur_editor) ;
    UString path ;
    cur_editor->get_path (path) ;
    toggle_breakpoint (path, a_line + 1 ) ;

    NEMIVER_CATCH
}

bool
DBGPerspective::on_button_pressed_in_source_view_signal (GdkEventButton *a_event)
{
    NEMIVER_TRY

    if (get_popup_tip ().is_visible ()) {
        get_popup_tip ().hide () ;
    }

    if (a_event->type != GDK_BUTTON_PRESS) {
        return false ;
    }

    if (a_event->button == 3) {
        popup_source_view_contextual_menu (a_event) ;
        return true ;
    }

    NEMIVER_CATCH

    return false ;
}


bool
DBGPerspective::on_motion_notify_event_signal (GdkEventMotion *a_event)
{
    LOG_FUNCTION_SCOPE_NORMAL_D(DBG_PERSPECTIVE_MOUSE_MOTION_DOMAIN) ;
    NEMIVER_TRY
    int x=0, y=0;
    GdkModifierType state=(GdkModifierType)0;

    if (a_event->is_hint) {
        gdk_window_get_pointer (a_event->window, &x, &y, &state) ;
    } else {
        x = (int) a_event->x;
        y = (int) a_event->y;
        state = (GdkModifierType)a_event->state;
    }

    LOG_D ("(x,y) => (" << (int)x << ", " << (int)y << ")",
           DBG_PERSPECTIVE_MOUSE_MOTION_DOMAIN) ;
    m_priv->mouse_in_source_editor_x = x ;
    m_priv->mouse_in_source_editor_y = y ;
    restart_mouse_immobile_timer () ;
    NEMIVER_CATCH
    return false ;
}

void
DBGPerspective::on_leave_notify_event_signal (GdkEventCrossing *a_event)
{
    LOG_FUNCTION_SCOPE_NORMAL_D(DBG_PERSPECTIVE_MOUSE_MOTION_DOMAIN) ;
    if (a_event) {}
    stop_mouse_immobile_timer () ;
}

bool
DBGPerspective::on_mouse_immobile_timer_signal ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD
    NEMIVER_TRY

    if (get_contextual_menu ()
        && get_contextual_menu ()->is_visible ())
    {
        return false ;
    }

    try_to_request_show_variable_value_at_position
                                        (m_priv->mouse_in_source_editor_x,
                                         m_priv->mouse_in_source_editor_y) ;
    NEMIVER_CATCH
    return false ;
}

void
DBGPerspective::on_shutdown_signal ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;
    NEMIVER_TRY

    if (m_priv->prog_path == "") {
        return ;
    }

    // stop the debugger so that the target executable doesn't go on running
    // after we shut down
    debugger ()->exit_engine ();

    if (m_priv->reused_session) {
        record_and_save_session (m_priv->session) ;
        LOG_DD ("saved current session") ;
    } else {
        LOG_DD ("recorded a new session") ;
        record_and_save_new_session () ;
    }

    NEMIVER_CATCH
}

void
DBGPerspective::on_show_command_view_changed_signal (bool a_show)
{
    Glib::RefPtr<Gtk::ToggleAction> action =
            Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic
                (workbench ().get_ui_manager ()->get_action
                    ("/MenuBar/MenuBarAdditions/ViewMenu/ShowCommandsMenuItem"));
    THROW_IF_FAIL (action) ;
    action->set_active (a_show) ;
}

void
DBGPerspective::on_show_target_output_view_changed_signal (bool a_show)
{
    m_priv->target_output_view_is_visible = a_show ;

    Glib::RefPtr<Gtk::ToggleAction> action =
        Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic
            (workbench ().get_ui_manager ()->get_action
                ("/MenuBar/MenuBarAdditions/ViewMenu/ShowTargetOutputMenuItem"));
    THROW_IF_FAIL (action) ;
    action->set_active (a_show) ;
}

void
DBGPerspective::on_show_log_view_changed_signal (bool a_show)
{
    m_priv->log_view_is_visible = a_show ;

    Glib::RefPtr<Gtk::ToggleAction> action =
        Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic
            (workbench ().get_ui_manager ()->get_action
                    ("/MenuBar/MenuBarAdditions/ViewMenu/ShowErrorsMenuItem"));
    THROW_IF_FAIL (action) ;

    action->set_active (a_show) ;
}

void
DBGPerspective::on_conf_key_changed_signal (const UString &a_key,
                                            IConfMgr::Value &a_value)
{
    NEMIVER_TRY
    if (a_key == CONF_KEY_NEMIVER_SOURCE_DIRS) {
        LOG_DD ("updated key source-dirs") ;
        m_priv->source_dirs = boost::get<UString> (a_value).split (":") ;
    } else if (a_key == CONF_KEY_SHOW_DBG_ERROR_DIALOGS) {
        m_priv->show_dbg_errors = boost::get<bool> (a_value) ;
    } else if (a_key == CONF_KEY_SHOW_SOURCE_LINE_NUMBERS) {
        map<int, SourceEditor*>::iterator it ;
        for (it = m_priv->pagenum_2_source_editor_map.begin () ;
             it != m_priv->pagenum_2_source_editor_map.end ();
             ++it) {
            if (it->second) {
                it->second->source_view ().set_show_line_numbers
                    (boost::get<bool> (a_value)) ;
            }
        }
    } else if (a_key == CONF_KEY_HIGHLIGHT_SOURCE_CODE) {
        map<int, SourceEditor*>::iterator it ;
        for (it = m_priv->pagenum_2_source_editor_map.begin () ;
             it != m_priv->pagenum_2_source_editor_map.end ();
             ++it) {
            if (it->second && it->second->source_view ().get_buffer ()) {
                it->second->source_view ().get_source_buffer ()->set_highlight
                                                (boost::get<bool> (a_value)) ;
            }
        }
    }
    NEMIVER_CATCH
}

void
DBGPerspective::on_debugger_got_target_info_signal (int a_pid,
                                                  const UString &a_exe_path)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;
    if (a_exe_path == "" || a_pid) {}
    NEMIVER_TRY
    THROW_IF_FAIL (m_priv) ;
    if (a_exe_path != "") {
        m_priv->prog_path = a_exe_path ;
    }
    NEMIVER_CATCH
}

void
DBGPerspective::on_debugger_console_message_signal (const UString &a_msg)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;
    NEMIVER_TRY

    add_text_to_command_view (a_msg + "\n") ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_debugger_target_output_message_signal
                                            (const UString &a_msg)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;
    NEMIVER_TRY

    add_text_to_target_output_view (a_msg + "\n") ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_debugger_log_message_signal (const UString &a_msg)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;
    NEMIVER_TRY

    add_text_to_log_view (a_msg + "\n") ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_debugger_command_done_signal (const UString &a_command,
                                                 const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;

    LOG_DD ("a_command: " << a_command) ;
    LOG_DD ("a_cookie: " << a_cookie) ;

    NEMIVER_TRY
    if (a_command == "attach-to-program") {
        //attached_to_target_signal ().emit (true) ;
        debugger ()->step_over () ;
        debugger ()->get_target_info () ;
    }
    NEMIVER_CATCH
}

void
DBGPerspective::on_debugger_breakpoints_set_signal
                                (const map<int, IDebugger::BreakPoint> &a_breaks)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;
    NEMIVER_TRY
    LOG_DD ("debugger engine set breakpoints") ;
    append_breakpoints (a_breaks) ;
    get_breakpoints_view ().set_breakpoints (a_breaks);
    NEMIVER_CATCH
}


void
DBGPerspective::on_debugger_stopped_signal (const UString &a_reason,
                                            bool a_has_frame,
                                            const IDebugger::Frame &a_frame,
                                            int a_thread_id)
{

    LOG_FUNCTION_SCOPE_NORMAL_DD ;
    if (a_reason == "" || a_thread_id) {}

    NEMIVER_TRY

    LOG_DD ("stopped, reason: " << a_reason) ;

    THROW_IF_FAIL (m_priv) ;

    UString file_path (a_frame.file_full_name ());
    if (a_has_frame
        && a_frame.file_full_name () == ""
        && a_frame.file_name () != "") {
        file_path = a_frame.file_name () ;
        if (!find_file_in_source_dirs (file_path, file_path)) {
            display_error (_("Could not find file ") + file_path) ;
            return ;
        }
    }
    if (a_has_frame && file_path != "") {
        set_where (file_path, a_frame.line ()) ;
    } else if (a_has_frame && 
               a_frame.file_full_name () == ""
               && a_frame.file_name () == "") {
            display_warning (_("File path info is missing "
                               "for function ")
                             + UString ("'") + a_frame.function_name () + "'") ;
    }

    if (m_priv->debugger_has_just_run) {
        debugger ()->get_target_info () ;
        m_priv->debugger_has_just_run = false ;
    }

    add_text_to_command_view ("\n(gdb)", true) ;
    NEMIVER_CATCH
}

void
DBGPerspective::on_program_finished_signal ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;
    NEMIVER_TRY

    unset_where () ;
    attached_to_target_signal ().emit (true) ;
    display_info (_("Program exited")) ;

    //****************************
    //grey out all the menu
    //items but the one
    //to restart the debugger
    //***************************
    THROW_IF_FAIL (m_priv) ;

    m_priv->target_not_started_action_group->set_sensitive (true) ;
    m_priv->debugger_ready_action_group->set_sensitive (false) ;
    m_priv->debugger_busy_action_group->set_sensitive (false) ;

    //**********************
    //clear threads list and
    //call stack
    //**********************
    get_thread_list ().clear () ;
    get_call_stack ().clear () ;
    get_local_vars_inspector ().re_init_widget () ;
    NEMIVER_CATCH
}

void
DBGPerspective::on_frame_selected_signal (int a_index,
                                          const IDebugger::Frame &a_frame)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;

    if (a_index) {}
    NEMIVER_TRY

    UString file_path = a_frame.file_full_name () ;

    if (file_path == "") {
        file_path = a_frame.file_name () ;
        if (!find_file_in_source_dirs (file_path, file_path)) {
            display_warning (_("File path info is missing "
                             "for function ")
                             + UString ("'") + a_frame.function_name () + "'") ;
            return ;
            //TODO: we should disassemble the current frame and display it.
        }
    }

    if (a_frame.line () == 0) {
        display_warning ("Line info is missing for function '"
                         + a_frame.function_name () + "'") ;
        return ;
        //TODO: we should disassemble the current frame and display it.
    }

    get_local_vars_inspector ().show_local_variables_of_current_function () ;
    set_where (file_path, a_frame.line ()) ;

    NEMIVER_CATCH
}


void
DBGPerspective::on_debugger_breakpoint_deleted_signal
                                        (const IDebugger::BreakPoint &a_break,
                                         int a_break_number)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;

    if (a_break.number ()) {}
    NEMIVER_TRY
    delete_visual_breakpoint (a_break_number) ;
    get_breakpoints_view ().set_breakpoints (m_priv->breakpoints);
    NEMIVER_CATCH
}

void
DBGPerspective::on_debugger_running_signal ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;
    NEMIVER_TRY
    THROW_IF_FAIL (m_priv->throbber) ;
    m_priv->throbber->start () ;
    NEMIVER_CATCH
}

void
DBGPerspective::on_signal_received_by_target_signal (const UString &a_signal,
                                                     const UString &a_meaning)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;
    NEMIVER_TRY

    ui_utils::display_info (_("Target received a signal : ")
                            + a_signal + ", " + a_meaning) ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_debugger_error_signal (const UString &a_msg)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;
    NEMIVER_TRY

    if (m_priv->show_dbg_errors) {
        ui_utils::display_error (_("An error occured: ") + a_msg) ;
    }

    NEMIVER_CATCH
}

void
DBGPerspective::on_debugger_state_changed_signal (IDebugger::State a_state)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;
    NEMIVER_TRY

    LOG_DD ("state is '" << IDebugger::state_to_string (a_state) << "'") ;

    if (a_state == IDebugger::READY) {
        debugger_ready_signal ().emit (true) ;
    } else if (a_state == IDebugger::PROGRAM_EXITED) {
        debugger_ready_signal ().emit (true) ;
    } else {
        debugger_ready_signal ().emit (false) ;
    }

    NEMIVER_CATCH
}

void
DBGPerspective::on_debugger_variable_value_signal
                                        (const UString &a_var_name,
                                         const IDebugger::VariableSafePtr &a_var)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;
    NEMIVER_TRY
    THROW_IF_FAIL (m_priv) ;

    UString var_str ;
    if (m_priv->in_show_var_value_at_pos_transaction
        && m_priv->var_to_popup == a_var_name) {
        a_var->to_string (var_str, true) ;
        show_underline_tip_at_position (m_priv->var_popup_tip_x,
                                        m_priv->var_popup_tip_y,
                                        var_str) ;
        m_priv->in_show_var_value_at_pos_transaction = false ;
        m_priv->var_to_popup = "" ;
    }
    NEMIVER_CATCH
}

//****************************
//</slots>
//***************************

//*******************
//<private methods>
//*******************


string
DBGPerspective::build_resource_path (const UString &a_dir, const UString &a_name)
{
    string relative_path = Glib::build_filename (Glib::locale_from_utf8 (a_dir),
                                                 Glib::locale_from_utf8 (a_name));
    string absolute_path ;
    THROW_IF_FAIL (build_absolute_resource_path
                    (Glib::locale_to_utf8 (relative_path),
                                           absolute_path)) ;
    return absolute_path ;
}


void
DBGPerspective::add_stock_icon (const UString &a_stock_id,
                                const UString &a_icon_dir,
                                const UString &a_icon_name)
{
    if (!m_priv->icon_factory) {
        m_priv->icon_factory = Gtk::IconFactory::create () ;
        m_priv->icon_factory->add_default () ;
    }

    Gtk::StockID stock_id (a_stock_id) ;
    string icon_path = build_resource_path (a_icon_dir, a_icon_name) ;
    Glib::RefPtr<Gdk::Pixbuf> pixbuf= Gdk::Pixbuf::create_from_file (icon_path) ;
    Gtk::IconSet icon_set (pixbuf) ;
    m_priv->icon_factory->add (stock_id, icon_set) ;
}

void
DBGPerspective::add_perspective_menu_entries ()
{
    string relative_path = Glib::build_filename ("menus",
                                                 "menus.xml") ;
    string absolute_path ;
    THROW_IF_FAIL (build_absolute_resource_path
                    (Glib::locale_to_utf8 (relative_path),
                                           absolute_path)) ;

    m_priv->menubar_merge_id =
        workbench ().get_ui_manager ()->add_ui_from_file
                                        (Glib::locale_to_utf8 (absolute_path)) ;

    relative_path = Glib::build_filename ("menus", "contextualmenu.xml") ;
    THROW_IF_FAIL (build_absolute_resource_path
                    (Glib::locale_to_utf8 (relative_path),
                                           absolute_path)) ;
    m_priv->contextual_menu_merge_id =
        workbench ().get_ui_manager ()->add_ui_from_file
                                        (Glib::locale_to_utf8 (absolute_path)) ;
}

void
DBGPerspective::init_perspective_menu_entries ()
{
    set_show_command_view (false) ;
    set_show_target_output_view (false) ;
    set_show_log_view (false) ;
    set_show_call_stack_view (true) ;
    set_show_variables_editor_view (true) ;
    set_show_terminal_view (true) ;
    set_show_breakpoints_view (true) ;
    m_priv->statuses_notebook->set_current_page (0) ;
}

void
DBGPerspective::add_perspective_toolbar_entries ()
{
    string relative_path = Glib::build_filename ("menus",
                                                 "toolbar.xml") ;
    string absolute_path ;
    THROW_IF_FAIL (build_absolute_resource_path
                    (Glib::locale_to_utf8 (relative_path),
                                           absolute_path)) ;

    m_priv->toolbar_merge_id =
        workbench ().get_ui_manager ()->add_ui_from_file
                                            (Glib::locale_to_utf8 (absolute_path)) ;
}

void
DBGPerspective::init_icon_factory ()
{
    add_stock_icon (nemiver::SET_BREAKPOINT, "icons", "set-breakpoint.xpm") ;
    add_stock_icon (nemiver::CONTINUE, "icons", "continue.xpm") ;
    add_stock_icon (nemiver::STOP_DEBUGGER, "icons", "stop-debugger.xpm") ;
    add_stock_icon (nemiver::RUN_DEBUGGER, "icons", "run-debugger.xpm") ;
    add_stock_icon (nemiver::LINE_POINTER, "icons", "line-pointer.xpm") ;
    add_stock_icon (nemiver::RUN_TO_CURSOR, "icons", "run-to-cursor.xpm") ;
    add_stock_icon (nemiver::STEP_INTO, "icons", "step-into.xpm") ;
    add_stock_icon (nemiver::STEP_OVER, "icons", "step-over.xpm") ;
    add_stock_icon (nemiver::STEP_OUT, "icons", "step-out.xpm") ;
}

void
DBGPerspective::init_actions ()
{
    Gtk::StockID nil_stock_id ("") ;
    sigc::slot<void> nil_slot ;
    static ui_utils::ActionEntry s_target_connected_action_entries [] = {
        {
            "SaveSessionMenuItemAction",
            Gtk::Stock::SAVE,
            _("_Save session to disk"),
            _("save the current debugging session to disk"),
            sigc::mem_fun (*this, &DBGPerspective::on_save_session_action),
            ActionEntry::DEFAULT,
            ""
        },
    };

    static ui_utils::ActionEntry s_target_not_started_action_entries [] = {
        {
            "RunMenuItemAction",
            nemiver::STOCK_RUN_DEBUGGER,
            _("_Restart"),
            _("Restart the target, killing this process and starting a new one"),
            sigc::mem_fun (*this, &DBGPerspective::on_run_action),
            ActionEntry::DEFAULT,
            "<shift>F5"
        }
    };

    static ui_utils::ActionEntry s_debugger_ready_action_entries [] = {
        {
            "NextMenuItemAction",
            nemiver::STOCK_STEP_OVER,
            _("_Next"),
            _("Execute next line stepping over the next function, if any"),
            sigc::mem_fun (*this, &DBGPerspective::on_next_action),
            ActionEntry::DEFAULT,
            "F6"
        }
        ,
        {
            "StepMenuItemAction",
            nemiver::STOCK_STEP_INTO,
            _("_Step"),
            _("Execute next line, stepping into the next function, if any"),
            sigc::mem_fun (*this, &DBGPerspective::on_step_into_action),
            ActionEntry::DEFAULT,
            "F7"
        }
        ,
        {
            "StepOutMenuItemAction",
            nemiver::STOCK_STEP_OUT,
            _("Step _out"),
            _("Finish the execution of the current function"),
            sigc::mem_fun (*this, &DBGPerspective::on_step_out_action),
            ActionEntry::DEFAULT,
            "<shift>F7"
        }
        ,
        {
            "ContinueMenuItemAction",
            nemiver::STOCK_CONTINUE,
            _("_Continue"),
            _("Continue program execution until the next breakpoint"),
            sigc::mem_fun (*this, &DBGPerspective::on_continue_action),
            ActionEntry::DEFAULT,
            "F5"
        }
        ,
        {
            "ContinueUntilMenuItemAction",
            nil_stock_id,
            _("Run to cursor"),
            _("Continue program execution until the currently selected "
              "line is reached"),
            sigc::mem_fun (*this, &DBGPerspective::on_continue_until_action),
            ActionEntry::DEFAULT,
            "F11"
        }
        ,
        {
            "ToggleBreakPointMenuItemAction",
            nemiver::STOCK_SET_BREAKPOINT,
            _("Toggle _break"),
            _("Set/Unset a breakpoint at the current cursor location"),
            sigc::mem_fun (*this, &DBGPerspective::on_toggle_breakpoint_action),
            ActionEntry::DEFAULT,
            "F8"
        },

        {
            "InspectVariableMenuItemAction",
            nil_stock_id,
            _("Inspect a variable"),
            _("Inspect a global or local variable"),
            sigc::mem_fun (*this, &DBGPerspective::on_inspect_variable_action),
            ActionEntry::DEFAULT,
            "F12"
        }
    };

    static ui_utils::ActionEntry s_debugger_busy_action_entries [] = {
        {
            "StopMenuItemAction",
            Gtk::Stock::STOP,
            _("Stop"),
            _("Stop the debugger"),
            sigc::mem_fun (*this, &DBGPerspective::on_stop_debugger_action),
            ActionEntry::DEFAULT,
            "F9"
        }
    };

    static ui_utils::ActionEntry s_default_action_entries [] = {
        {
            "ViewMenuAction",
            nil_stock_id,
            _("_View"),
            "",
            nil_slot,
            ActionEntry::DEFAULT,
            ""
        },
        {
            "ShowCommandsMenuAction",
            nil_stock_id,
            _("Show commands"),
            _("Show the debugger commands tab"),
            sigc::mem_fun (*this, &DBGPerspective::on_show_commands_action),
            ActionEntry::TOGGLE,
            ""
        },
        {
            "ShowErrorsMenuAction",
            nil_stock_id,
            _("Show errors"),
            _("Show the errors commands tab"),
            sigc::mem_fun (*this, &DBGPerspective::on_show_errors_action),
            ActionEntry::TOGGLE,
            ""
        },
        {
            "ShowTargetOutputMenuAction",
            nil_stock_id,
            _("Show output"),
            _("Show the debugged target output tab"),
            sigc::mem_fun (*this, &DBGPerspective::on_show_target_output_action),
            ActionEntry::TOGGLE,
            ""
        },
        {
            "DebugMenuAction",
            nil_stock_id,
            _("_Debug"),
            "",
            nil_slot,
            ActionEntry::DEFAULT,
            ""
        }
        ,
        {
            "OpenMenuItemAction",
            Gtk::Stock::OPEN,
            _("_Open source file ..."),
            _("Open a source file for viewing"),
            sigc::mem_fun (*this, &DBGPerspective::on_open_action),
            ActionEntry::DEFAULT,
            ""
        },
        {
            "ExecuteProgramMenuItemAction",
            Gtk::Stock::EXECUTE,
            _("_Execute..."),
            _("Execute a program"),
            sigc::mem_fun (*this,
                           &DBGPerspective::on_execute_program_action),
            ActionEntry::DEFAULT,
            ""
        },
        {
            "LoadCoreMenuItemAction",
            nil_stock_id,
            _("_Load core file..."),
            _("Load a core file from disk"),
            sigc::mem_fun (*this,
                           &DBGPerspective::on_load_core_file_action),
            ActionEntry::DEFAULT,
            ""
        },
        {
            "AttachToProgramMenuItemAction",
            nil_stock_id,
            _("_Attach to running program..."),
            _("Debug a program that's already running"),
            sigc::mem_fun (*this,
                           &DBGPerspective::on_attach_to_program_action),
            ActionEntry::DEFAULT,
            ""
        },
        {
            "SavedSessionsMenuItemAction",
            nil_stock_id,
            _("Resume Sa_ved Session..."),
            _("Manage previously saved debugging sessions"),
            sigc::mem_fun (*this,
                           &DBGPerspective::on_saved_sessions_action),
            ActionEntry::DEFAULT,
            ""
        },
        {
            "CurrentSessionPropertiesMenuItemAction",
            Gtk::Stock::PREFERENCES,
            _("_Preferences"),
            _("Edit the properties of the current session"),
            sigc::mem_fun (*this,
                           &DBGPerspective::on_current_session_properties_action),
            ActionEntry::DEFAULT,
            ""
        }
    };

    static ui_utils::ActionEntry s_file_opened_action_entries [] = {
        {
            "CloseMenuItemAction",
            Gtk::Stock::CLOSE,
            _("_Close"),
            _("Close the opened file"),
            sigc::mem_fun (*this, &DBGPerspective::on_close_action),
            ActionEntry::DEFAULT,
            ""
        }
    };

    m_priv->target_connected_action_group =
                Gtk::ActionGroup::create ("target-connected-action-group") ;
    m_priv->target_connected_action_group->set_sensitive (false) ;

    m_priv->target_not_started_action_group =
                Gtk::ActionGroup::create ("target-not-started-action-group") ;
    m_priv->target_not_started_action_group->set_sensitive (false) ;

    m_priv->debugger_ready_action_group =
                Gtk::ActionGroup::create ("debugger-ready-action-group") ;
    m_priv->debugger_ready_action_group->set_sensitive (false) ;

    m_priv->debugger_busy_action_group =
                Gtk::ActionGroup::create ("debugger-busy-action-group") ;
    m_priv->debugger_busy_action_group->set_sensitive (false) ;

    m_priv->default_action_group =
                Gtk::ActionGroup::create ("debugger-default-action-group") ;
    m_priv->default_action_group->set_sensitive (true) ;

    m_priv->opened_file_action_group =
                Gtk::ActionGroup::create ("opened-file-action-group") ;
    m_priv->opened_file_action_group->set_sensitive (false) ;

    int num_actions =
     sizeof (s_target_connected_action_entries)/sizeof (ui_utils::ActionEntry);
    ui_utils::add_action_entries_to_action_group
                        (s_target_connected_action_entries,
                         num_actions,
                         m_priv->target_connected_action_group) ;

    num_actions =
     sizeof (s_target_not_started_action_entries)/sizeof (ui_utils::ActionEntry);
    ui_utils::add_action_entries_to_action_group
                        (s_target_not_started_action_entries,
                         num_actions,
                         m_priv->target_not_started_action_group) ;

    num_actions =
         sizeof (s_debugger_ready_action_entries)/sizeof (ui_utils::ActionEntry) ;

    ui_utils::add_action_entries_to_action_group
                        (s_debugger_ready_action_entries,
                         num_actions,
                         m_priv->debugger_ready_action_group) ;

    num_actions =
         sizeof (s_debugger_busy_action_entries)/sizeof (ui_utils::ActionEntry) ;

    ui_utils::add_action_entries_to_action_group
                        (s_debugger_busy_action_entries,
                         num_actions,
                         m_priv->debugger_busy_action_group) ;

    num_actions =
         sizeof (s_default_action_entries)/sizeof (ui_utils::ActionEntry) ;

    ui_utils::add_action_entries_to_action_group
                        (s_default_action_entries,
                         num_actions,
                         m_priv->default_action_group) ;

    num_actions =
         sizeof (s_file_opened_action_entries)/sizeof (ui_utils::ActionEntry) ;

    ui_utils::add_action_entries_to_action_group
                        (s_file_opened_action_entries,
                         num_actions,
                         m_priv->opened_file_action_group) ;

    workbench ().get_ui_manager ()->insert_action_group
                                        (m_priv->target_connected_action_group);
    workbench ().get_ui_manager ()->insert_action_group
                                        (m_priv->target_not_started_action_group);
    workbench ().get_ui_manager ()->insert_action_group
                                            (m_priv->debugger_busy_action_group) ;
    workbench ().get_ui_manager ()->insert_action_group
                                            (m_priv->debugger_ready_action_group);
    workbench ().get_ui_manager ()->insert_action_group
                                            (m_priv->default_action_group);
    workbench ().get_ui_manager ()->insert_action_group
                                            (m_priv->opened_file_action_group);

    workbench ().get_root_window ().add_accel_group
        (workbench ().get_ui_manager ()->get_accel_group ()) ;
}


void
DBGPerspective::init_toolbar ()
{
    add_perspective_toolbar_entries () ;

    m_priv->throbber = EphyThrobber::create () ;
    m_priv->toolbar.reset ((new Gtk::HBox)) ;
    THROW_IF_FAIL (m_priv->toolbar) ;
    Gtk::Toolbar *glade_toolbar = dynamic_cast<Gtk::Toolbar*>
            (workbench ().get_ui_manager ()->get_widget ("/ToolBar")) ;
    THROW_IF_FAIL (glade_toolbar) ;
    Gtk::SeparatorToolItem *sep = Gtk::manage (new Gtk::SeparatorToolItem) ;
    gtk_separator_tool_item_set_draw (sep->gobj (), false) ;
    sep->set_expand (true) ;
    glade_toolbar->insert (*sep, -1) ;
    glade_toolbar->insert (m_priv->throbber->get_widget (), -1) ;
    m_priv->toolbar->pack_start (*glade_toolbar);
    m_priv->toolbar->show_all () ;
}

void
DBGPerspective::init_body ()
{
    string relative_path = Glib::build_filename ("glade",
                                                 "bodycontainer.glade") ;
    string absolute_path ;
    THROW_IF_FAIL (build_absolute_resource_path
                    (Glib::locale_to_utf8 (relative_path),
                                           absolute_path)) ;
    m_priv->body_glade = Gnome::Glade::Xml::create (absolute_path) ;
    m_priv->body_window.reset
        (ui_utils::get_widget_from_glade<Gtk::Window> (m_priv->body_glade,
                                                      "bodycontainer")) ;
    m_priv->top_box =
        ui_utils::get_widget_from_glade<Gtk::Box> (m_priv->body_glade,
                                                   "topbox") ;
    m_priv->body_main_paned.reset
        (ui_utils::get_widget_from_glade<Gtk::Paned> (m_priv->body_glade,
                                                      "mainbodypaned")) ;

    m_priv->sourceviews_notebook =
        ui_utils::get_widget_from_glade<Gtk::Notebook> (m_priv->body_glade,
                                                        "sourceviewsnotebook") ;
    m_priv->sourceviews_notebook->remove_page () ;
    m_priv->sourceviews_notebook->set_show_tabs () ;
    m_priv->sourceviews_notebook->set_scrollable () ;

    m_priv->statuses_notebook =
        ui_utils::get_widget_from_glade<Gtk::Notebook> (m_priv->body_glade,
                                                        "statusesnotebook") ;
    IConfMgr &conf_mgr = workbench ().get_configuration_manager () ;
    int width=100, height=70 ;
    conf_mgr.get_key_value (CONF_KEY_STATUS_WIDGET_MINIMUM_WIDTH, width) ;
    conf_mgr.get_key_value (CONF_KEY_STATUS_WIDGET_MINIMUM_HEIGHT, height) ;
    LOG_DD ("setting status widget min size: width: "
            << width
            << ", height: "
            << height) ;
    m_priv->statuses_notebook->set_size_request (width, height) ;

    m_priv->command_view.reset (new Gtk::TextView) ;
    THROW_IF_FAIL (m_priv->command_view) ;
    get_command_view_scrolled_win ().add (*m_priv->command_view) ;
    m_priv->command_view->set_editable (true) ;
    m_priv->command_view->get_buffer ()->signal_insert ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_insert_in_command_view_signal)) ;

    m_priv->target_output_view.reset (new Gtk::TextView);
    THROW_IF_FAIL (m_priv->target_output_view) ;
    get_target_output_view_scrolled_win ().add (*m_priv->target_output_view) ;
    m_priv->target_output_view->set_editable (false) ;

    m_priv->log_view.reset (new Gtk::TextView) ;
    get_log_view_scrolled_win ().add (*m_priv->log_view) ;
    m_priv->log_view->set_editable (false) ;

    Gtk::HPaned *p = Gtk::manage (new Gtk::HPaned) ;
    THROW_IF_FAIL (p) ;
    p->add1 (get_thread_list ().widget ()) ;
    p->add2 (get_call_stack ().widget ()) ;
    get_call_stack_scrolled_win ().add (*p) ;
    get_local_vars_inspector_scrolled_win ().add (get_local_vars_inspector ().widget ());
    get_breakpoints_scrolled_win ().add (get_breakpoints_view ().widget());

    /*
    set_show_call_stack_view (true) ;
    set_show_variables_editor_view (true) ;
    */

    //unparent the body_main_paned, so that we can pack it
    //in the workbench later
    m_priv->top_box->remove (*m_priv->body_main_paned) ;
    m_priv->body_main_paned->show_all () ;

    //must be last
    init_perspective_menu_entries () ;
}

void
DBGPerspective::init_signals ()
{
    m_priv->sourceviews_notebook->signal_switch_page ().connect
        (sigc::mem_fun (*this, &DBGPerspective::on_switch_page_signal)) ;

    debugger_ready_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_debugger_ready_signal)) ;

    going_to_run_target_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_going_to_run_target_signal)) ;

    attached_to_target_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_attached_to_target_signal)) ;

    show_command_view_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_show_command_view_changed_signal)) ;

    show_target_output_view_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_show_target_output_view_changed_signal));

    show_log_view_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_show_log_view_changed_signal));

    get_call_stack ().frame_selected_signal ().connect
        (sigc::mem_fun (*this, &DBGPerspective::on_frame_selected_signal));

    get_breakpoints_view ().go_to_breakpoint_signal ().connect
        (sigc::mem_fun (*this, &DBGPerspective::on_breakpoint_go_to_source_action));

    get_breakpoints_view ().delete_breakpoint_signal ().connect
        (sigc::mem_fun (*this, &DBGPerspective::on_breakpoint_delete_action));

    get_thread_list ().thread_selected_signal ().connect (sigc::mem_fun
        (*this, &DBGPerspective::on_thread_list_thread_selected_signal)) ;
}

void
DBGPerspective::init_debugger_signals ()
{
    debugger ()->console_message_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_debugger_console_message_signal)) ;

    debugger ()->target_output_message_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_debugger_target_output_message_signal)) ;

    debugger ()->log_message_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_debugger_log_message_signal)) ;

    debugger ()->command_done_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_debugger_command_done_signal)) ;

    debugger ()->breakpoints_set_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_debugger_breakpoints_set_signal)) ;

    debugger ()->breakpoint_deleted_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_debugger_breakpoint_deleted_signal)) ;

    debugger ()->stopped_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_debugger_stopped_signal)) ;

    debugger ()->program_finished_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_program_finished_signal)) ;

    debugger ()->running_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_debugger_running_signal)) ;

    debugger ()->signal_received_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_signal_received_by_target_signal)) ;

    debugger ()->error_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_debugger_error_signal)) ;

    debugger ()->state_changed_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_debugger_state_changed_signal)) ;

    debugger ()->variable_value_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_debugger_variable_value_signal)) ;

    debugger ()->got_target_info_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_debugger_got_target_info_signal)) ;
}


void
DBGPerspective::append_source_editor (SourceEditor &a_sv,
                                      const UString &a_path)
{
    if (a_path == "") {return;}

    if (m_priv->path_2_pagenum_map.find (a_path)
        != m_priv->path_2_pagenum_map.end ()) {
        THROW (UString ("File of '") + a_path + "' is already loaded") ;
    }

    UString basename = Glib::locale_to_utf8
        (Glib::path_get_basename (Glib::locale_from_utf8 (a_path))) ;

    SafePtr<Gtk::Label> label (Gtk::manage
                            (new Gtk::Label (basename))) ;
    SafePtr<Gtk::Image> cicon (manage
                (new Gtk::Image (Gtk::StockID (Gtk::Stock::CLOSE),
                                               Gtk::ICON_SIZE_MENU))) ;

    SafePtr<SlotedButton> close_button (Gtk::manage (new SlotedButton ())) ;
    //okay, make the button as small as possible.
    /*
    Glib::RefPtr<Gtk::Style> style = close_button->get_style ()->copy () ;
    THROW_IF_FAIL (style) ;
    style->set_xthickness (1) ;
    style->set_ythickness (1) ;
    close_button->set_style (style) ;
    */
    int w=0, h=0 ;
    Gtk::IconSize::lookup (Gtk::ICON_SIZE_MENU, w, h) ;
    close_button->set_size_request (w+2, h+2) ;

    close_button->perspective = this ;
    close_button->set_relief (Gtk::RELIEF_NONE) ;
    //close_button->set_focus_on_click (false) ;
    close_button->add (*cicon) ;
    close_button->file_path = a_path ;
    close_button->signal_clicked ().connect
            (sigc::mem_fun (*close_button, &SlotedButton::on_clicked)) ;
    close_button->tooltips.reset (new Gtk::Tooltips) ;
    close_button->tooltips->set_tip (*close_button,
                                     _("close") +  UString (" ") + a_path) ;

    SafePtr<Gtk::Table> table (Gtk::manage (new Gtk::Table (1, 2))) ;

    Gtk::EventBox *event_box = Gtk::manage (new Gtk::EventBox) ;
    event_box->add (*label) ;
    table->attach (*event_box, 0, 1, 0, 1) ;
    table->attach (*close_button, 1, 2, 0, 1) ;
    close_button->tooltips->set_tip (*event_box, a_path) ;
    table->show_all () ;
    int page_num = m_priv->sourceviews_notebook->insert_page (a_sv,
                                                              *table,
                                                              -1);
    m_priv->path_2_pagenum_map[a_path] = page_num ;
    std::string base_name =
                    Glib::path_get_basename (Glib::locale_from_utf8 (a_path)) ;
    THROW_IF_FAIL (base_name != "") ;
    m_priv->basename_2_pagenum_map[Glib::locale_to_utf8 (base_name)]= page_num ;
    m_priv->pagenum_2_source_editor_map[page_num] = &a_sv;
    m_priv->pagenum_2_path_map[page_num] = a_path ;

    table.release () ;
    close_button.release () ;
    label.release () ;
    cicon.release () ;
}

SourceEditor*
DBGPerspective::get_current_source_editor ()
{
    THROW_IF_FAIL (m_priv) ;

    if (!m_priv->sourceviews_notebook) {return NULL;}

    if (m_priv->sourceviews_notebook
        && !m_priv->sourceviews_notebook->get_n_pages ()) {
        return NULL ;
    }

    map<int, SourceEditor*>::iterator iter, nil ;
    nil = m_priv->pagenum_2_source_editor_map.end () ;

    iter = m_priv->pagenum_2_source_editor_map.find (m_priv->current_page_num) ;
    if (iter == nil) {return NULL ;}

    return iter->second ;
}

ISessMgr*
DBGPerspective::session_manager_ptr ()
{
    THROW_IF_FAIL (m_priv) ;

    if (!m_priv->session_manager) {
        m_priv->session_manager = ISessMgr::create (plugin_path ()) ;
        THROW_IF_FAIL (m_priv->session_manager) ;
    }
    return m_priv->session_manager.get () ;
}

UString
DBGPerspective::get_current_file_path ()
{
    SourceEditor *source_editor = get_current_source_editor () ;
    if (!source_editor) {return "";}
    UString path ;
    source_editor->get_path (path) ;
    return path ;
}

SourceEditor*
DBGPerspective::get_source_editor_from_path (const UString &a_path,
                                             bool a_basename_only)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD

    LOG_DD ("a_path: " << a_path) ;
    LOG_DD ("a_basename_only" << (int) a_basename_only) ;

    if (a_path == "") {return 0;}

    map<UString, int>::iterator iter, nil ;
    SourceEditor *result=0;

    if (a_basename_only) {
        std::string basename =
            Glib::path_get_basename (Glib::locale_from_utf8 (a_path)) ;
        THROW_IF_FAIL (basename != "") ;
        iter = m_priv->basename_2_pagenum_map.find (Glib::locale_to_utf8 (basename)) ;
        nil = m_priv->basename_2_pagenum_map.end () ;
        result = m_priv->pagenum_2_source_editor_map[iter->second] ;
    } else {
        iter = m_priv->path_2_pagenum_map.find (a_path) ;
        nil = m_priv->path_2_pagenum_map.end () ;
        result = m_priv->pagenum_2_source_editor_map[iter->second] ;
    }
    if (iter == nil) {
        return 0 ;
    }
    THROW_IF_FAIL (result) ;
    return result ;
}

IWorkbench&
DBGPerspective::workbench () const
{
    THROW_IF_FAIL (m_priv) ;
    THROW_IF_FAIL (m_priv->workbench) ;

    return *m_priv->workbench ;
}

void
DBGPerspective::bring_source_as_current (const UString &a_path)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;
    LOG_DD ("file pat: '" << a_path << "'") ;

    SourceEditor *source_editor = get_source_editor_from_path (a_path) ;
    if (!source_editor) {
        open_file (a_path) ;
    }
    if (!source_editor) {
        source_editor = get_source_editor_from_path (a_path, true) ;
    }
    source_editor = get_source_editor_from_path (a_path) ;
    THROW_IF_FAIL (source_editor) ;
    map<UString, int>::iterator iter =
        m_priv->path_2_pagenum_map.find (a_path) ;
    THROW_IF_FAIL (iter != m_priv->path_2_pagenum_map.end ()) ;
    m_priv->sourceviews_notebook->set_current_page (iter->second) ;
}

void
DBGPerspective::set_where (const UString &a_path,
                           int a_line)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;

    bring_source_as_current (a_path) ;
    SourceEditor *source_editor = get_source_editor_from_path (a_path) ;
    if (!source_editor) {
        source_editor = get_source_editor_from_path (a_path, true) ;
    }
    THROW_IF_FAIL (source_editor) ;
    source_editor->move_where_marker_to_line (a_line) ;
    Gtk::TextBuffer::iterator iter =
        source_editor->source_view().get_buffer ()->get_iter_at_line (a_line-1) ;
    if (!iter) {return;}
        source_editor->source_view().get_buffer ()->place_cursor (iter) ;
}

void
DBGPerspective::unset_where ()
{
    map<int, SourceEditor*>::iterator iter ;
    for (iter = m_priv->pagenum_2_source_editor_map.begin ();
         iter !=m_priv->pagenum_2_source_editor_map.end ();
         ++iter) {
        iter->second->unset_where_marker () ;
    }
}

Gtk::Widget*
DBGPerspective::get_contextual_menu ()
{
    THROW_IF_FAIL (m_priv && m_priv->contextual_menu_merge_id) ;

    if (!m_priv->contextual_menu) {

        workbench ().get_ui_manager ()->add_ui
            (m_priv->contextual_menu_merge_id,
             "/ContextualMenu",
             "InspectVariableMenuItem",
             "InspectVariableMenuItemAction",
             Gtk::UI_MANAGER_AUTO,
             false) ;

        workbench ().get_ui_manager ()->add_ui_separator
            (m_priv->contextual_menu_merge_id, "/ContextualMenu") ;

        workbench ().get_ui_manager ()->add_ui
            (m_priv->contextual_menu_merge_id,
             "/ContextualMenu",
             "ToggleBreakPointMenuItem",
             "ToggleBreakPointMenuItemAction",
             Gtk::UI_MANAGER_AUTO,
             false) ;

        workbench ().get_ui_manager ()->add_ui
            (m_priv->contextual_menu_merge_id,
             "/ContextualMenu",
             "NextMenuItem",
             "NextMenuItemAction",
             Gtk::UI_MANAGER_AUTO,
             false) ;

        workbench ().get_ui_manager ()->add_ui
            (m_priv->contextual_menu_merge_id,
             "/ContextualMenu",
             "StepMenuItem",
             "StepMenuItemAction",
             Gtk::UI_MANAGER_AUTO,
             false) ;

        workbench ().get_ui_manager ()->add_ui
            (m_priv->contextual_menu_merge_id,
             "/ContextualMenu",
             "StepMenuItem",
             "StepMenuItemAction",
             Gtk::UI_MANAGER_AUTO,
             false) ;

        workbench ().get_ui_manager ()->add_ui
            (m_priv->contextual_menu_merge_id,
             "/ContextualMenu",
             "StepOutMenuItem",
             "StepOutMenuItemAction",
             Gtk::UI_MANAGER_AUTO,
             false) ;

        workbench ().get_ui_manager ()->add_ui
            (m_priv->contextual_menu_merge_id,
             "/ContextualMenu",
             "ContinueMenuItem",
             "ContinueMenuItemAction",
             Gtk::UI_MANAGER_AUTO,
             false) ;

        workbench ().get_ui_manager ()->add_ui
            (m_priv->contextual_menu_merge_id,
             "/ContextualMenu",
             "ContinueUntilMenuItem",
             "ContinueUntilMenuItemAction",
             Gtk::UI_MANAGER_AUTO,
             false) ;

        workbench ().get_ui_manager ()->add_ui
            (m_priv->contextual_menu_merge_id,
             "/ContextualMenu",
             "StopMenuItem",
             "StopMenuItemAction",
             Gtk::UI_MANAGER_AUTO,
             false) ;

        workbench ().get_ui_manager ()->add_ui
            (m_priv->contextual_menu_merge_id,
             "/ContextualMenu",
             "RunMenuItem",
             "RunMenuItemAction",
             Gtk::UI_MANAGER_AUTO,
             false) ;


        workbench ().get_ui_manager ()->ensure_update () ;
        m_priv->contextual_menu =
            workbench ().get_ui_manager ()->get_widget
            ("/ContextualMenu") ;
        THROW_IF_FAIL (m_priv->contextual_menu) ;
    }
    return m_priv->contextual_menu ;
}

Gtk::Widget*
DBGPerspective::load_menu (UString a_filename, UString a_widget_name)
{
    THROW_IF_FAIL (m_priv) ;
    NEMIVER_TRY
    string relative_path = Glib::build_filename ("menus", a_filename) ;
    string absolute_path ;
    THROW_IF_FAIL (build_absolute_resource_path
            (Glib::locale_to_utf8 (relative_path),
             absolute_path)) ;

    workbench ().get_ui_manager ()->add_ui_from_file
        (Glib::locale_to_utf8 (absolute_path)) ;

    NEMIVER_CATCH
    return workbench ().get_ui_manager ()->get_widget (a_widget_name);
}

ThreadList&
DBGPerspective::get_thread_list ()
{
    THROW_IF_FAIL (m_priv) ;
    THROW_IF_FAIL (debugger ()) ;
    if (!m_priv->thread_list) {
        m_priv->thread_list.reset  (new ThreadList (debugger ()));
    }
    THROW_IF_FAIL (m_priv->thread_list) ;
    return *m_priv->thread_list ;
}

vector<UString>&
DBGPerspective::get_source_dirs ()
{

    THROW_IF_FAIL (m_priv) ;

    if (m_priv->source_dirs.empty ()) {
        init_conf_mgr () ;
    }
    return m_priv->source_dirs ;
}

bool
DBGPerspective::find_file_in_source_dirs (const UString &a_file_name,
                                          UString &a_file_path)
{
    THROW_IF_FAIL (m_priv) ;

    string file_name = Glib::locale_from_utf8 (a_file_name), path, candidate ;
    // first look in the working directory
    candidate = Glib::build_filename (m_priv->prog_cwd, file_name) ;
    if (Glib::file_test (candidate, Glib::FILE_TEST_IS_REGULAR)) {
        a_file_path = Glib::locale_to_utf8 (candidate) ;
        return true ;
    }
    // then look in the session-specific search paths
    list<UString>::const_iterator session_iter ;
    for (session_iter = m_priv->search_paths.begin () ;
         session_iter != m_priv->search_paths.end ();
         ++session_iter) {
        path = Glib::locale_from_utf8 (*session_iter) ;
        candidate = Glib::build_filename (path, file_name) ;
        if (Glib::file_test (candidate, Glib::FILE_TEST_IS_REGULAR)) {
            a_file_path = Glib::locale_to_utf8 (candidate) ;
            return true ;
        }
    }
    // if not found, then look in the global search paths
    vector<UString>::const_iterator global_iter ;
    for (global_iter = m_priv->source_dirs.begin () ;
         global_iter != m_priv->source_dirs.end ();
         ++global_iter) {
        path = Glib::locale_from_utf8 (*global_iter) ;
        candidate = Glib::build_filename (path, file_name) ;
        if (Glib::file_test (candidate, Glib::FILE_TEST_IS_REGULAR)) {
            a_file_path = Glib::locale_to_utf8 (candidate) ;
            return true ;
        }
    }
    return false ;
}

void
DBGPerspective::init_conf_mgr ()
{
    if (m_priv->source_dirs.empty ()) {
        THROW_IF_FAIL (m_priv->workbench) ;
        IConfMgr &conf_mgr = workbench ().get_configuration_manager () ;

        UString dirs ;
        conf_mgr.get_key_value (CONF_KEY_NEMIVER_SOURCE_DIRS, dirs) ;
        LOG_DD ("got source dirs '" << dirs << "' from conf mgr") ;
        m_priv->source_dirs = dirs.split (":") ;
        LOG_DD ("that makes '" <<(int)m_priv->source_dirs.size()<< "' dir paths");

        conf_mgr.get_key_value (CONF_KEY_SHOW_DBG_ERROR_DIALOGS,
                                m_priv->show_dbg_errors);

        conf_mgr.value_changed_signal ().connect
            (sigc::mem_fun (*this, &DBGPerspective::on_conf_key_changed_signal)) ;
    }
}

int
DBGPerspective::get_n_pages ()
{
    THROW_IF_FAIL (m_priv && m_priv->sourceviews_notebook) ;

    return m_priv->sourceviews_notebook->get_n_pages () ;
}

void
DBGPerspective::popup_source_view_contextual_menu (GdkEventButton *a_event)
{
    int buffer_x=0, buffer_y=0, line_top=0;
    Gtk::TextBuffer::iterator cur_iter ;
    UString file_name ;

    SourceEditor *editor = get_current_source_editor () ;
    THROW_IF_FAIL (editor) ;

    editor->source_view ().window_to_buffer_coords (Gtk::TEXT_WINDOW_TEXT,
                                                    (int)a_event->x,
                                                    (int)a_event->y,
                                                    buffer_x, buffer_y) ;
    editor->source_view ().get_line_at_y (cur_iter, buffer_y, line_top) ;

    editor->get_path (file_name) ;

    Gtk::Menu *menu = dynamic_cast<Gtk::Menu*> (get_contextual_menu ()) ;
    THROW_IF_FAIL (menu) ;

    Gtk::TextIter start, end ;
    Glib::RefPtr<gtksourceview::SourceBuffer> buffer =
                            editor->source_view ().get_source_buffer () ;
    THROW_IF_FAIL (buffer) ;
    bool has_selected_text=false ;
    if (buffer->get_selection_bounds (start, end)) {
        has_selected_text = true ;
    }
    editor->source_view ().get_buffer ()->place_cursor (cur_iter) ;
    if (has_selected_text) {
        buffer->select_range (start, end) ;
    }
    menu->popup (a_event->button, a_event->time) ;
}

void
DBGPerspective::record_and_save_new_session ()
{
    THROW_IF_FAIL (m_priv) ;
    ISessMgr::Session session ;
    record_and_save_session (session) ;
}

IProcMgr*
DBGPerspective::get_process_manager ()
{
    THROW_IF_FAIL (m_priv) ;
    if (!m_priv->process_manager) {
        m_priv->process_manager = IProcMgr::create () ;
        THROW_IF_FAIL (m_priv->process_manager) ;
    }
    return m_priv->process_manager.get () ;
}


void
DBGPerspective::try_to_request_show_variable_value_at_position (int a_x, int a_y)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD
    SourceEditor *editor = get_current_source_editor () ;
    THROW_IF_FAIL (editor);

    UString var_name ;
    Gdk::Rectangle start_rect, end_rect ;
    if (!get_current_source_editor ()->get_word_at_position (a_x, a_y,
                                                             var_name,
                                                             start_rect,
                                                             end_rect)) {
        return ;
    }

    if (var_name == "") {return;}
    Glib::RefPtr<Gdk::Window> gdk_window =
                        ((Gtk::Widget&)editor->source_view ()).get_window () ;
    THROW_IF_FAIL (gdk_window) ;
    int abs_x=0, abs_y=0 ;
    gdk_window->get_origin (abs_x, abs_y) ;
    abs_x += a_x ;
    abs_y += a_y + start_rect.get_height () / 2 ;
    m_priv->in_show_var_value_at_pos_transaction = true ;
    m_priv->var_popup_tip_x = abs_x ;
    m_priv->var_popup_tip_y = abs_y ;
    m_priv->var_to_popup = var_name ;
    debugger ()->print_variable_value (var_name) ;
}

void
DBGPerspective::show_underline_tip_at_position (int a_x,
                                                int a_y,
                                                const UString &a_text)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD
    LOG_DD ("showing text in popup: '"
            << Glib::locale_from_utf8 (a_text)
            << "'") ;
    get_popup_tip ().text (a_text) ;
    get_popup_tip ().show_at_position (a_x, a_y) ;
}

void
DBGPerspective::restart_mouse_immobile_timer ()
{
    LOG_FUNCTION_SCOPE_NORMAL_D (DBG_PERSPECTIVE_MOUSE_MOTION_DOMAIN) ;

    THROW_IF_FAIL (m_priv) ;
    THROW_IF_FAIL (m_priv->workbench) ;

    m_priv->timeout_source_connection.disconnect () ;
    m_priv->timeout_source_connection = Glib::signal_timeout ().connect
        (sigc::mem_fun (*this, &DBGPerspective::on_mouse_immobile_timer_signal),
         1000);
    get_popup_tip ().hide () ;
}

void
DBGPerspective::stop_mouse_immobile_timer ()
{
    LOG_FUNCTION_SCOPE_NORMAL_D (DBG_PERSPECTIVE_MOUSE_MOTION_DOMAIN) ;
    m_priv->timeout_source_connection.disconnect () ;
}

PopupTip&
DBGPerspective::get_popup_tip ()
{
    THROW_IF_FAIL (m_priv) ;

    if (!m_priv->popup_tip) {
        m_priv->popup_tip.reset (new PopupTip) ;
    }
    THROW_IF_FAIL (m_priv->popup_tip) ;
    return *m_priv->popup_tip ;
}

void
DBGPerspective::record_and_save_session (ISessMgr::Session &a_session)
{
    THROW_IF_FAIL (m_priv) ;
    UString session_name = Glib::path_get_basename
        (Glib::locale_from_utf8 (m_priv->prog_path)) ;

    if (session_name == "") {return;}

    if (a_session.session_id ()) {
        session_manager ().clear_session (a_session.session_id ()) ;
        LOG_DD ("cleared current session: "
                << (int) m_priv->session.session_id ()) ;
    }

    UString today ;
    dateutils::get_current_datetime (today) ;
    session_name += "-" + today ;

    a_session.properties ().clear () ;
    a_session.properties ()[SESSION_NAME] = session_name ;
    a_session.properties ()[PROGRAM_NAME] = m_priv->prog_path ;
    a_session.properties ()[PROGRAM_ARGS] = m_priv->prog_args ;
    a_session.properties ()[PROGRAM_CWD] = m_priv->prog_cwd ;
    a_session.env_variables () = m_priv->env_variables;

    a_session.opened_files ().clear () ;
    map<UString, int>::const_iterator path_iter =
        m_priv->path_2_pagenum_map.begin () ;
    for (;
         path_iter != m_priv->path_2_pagenum_map.end ();
         ++path_iter) {
        a_session.opened_files ().push_back (path_iter->first) ;
    }

    a_session.breakpoints ().clear () ;
    map<int, IDebugger::BreakPoint>::const_iterator break_iter ;
    for (break_iter = m_priv->breakpoints.begin ();
         break_iter != m_priv->breakpoints.end ();
         ++break_iter) {
        a_session.breakpoints ().push_back
            (ISessMgr::BreakPoint (break_iter->second.file_name (),
                                   break_iter->second.file_full_name (),
                                   break_iter->second.line ())) ;
    }
    THROW_IF_FAIL (session_manager_ptr ()) ;

    a_session.search_paths ().clear () ;
    list<UString>::const_iterator search_path_iter ;
    for (search_path_iter = m_priv->search_paths.begin ();
         search_path_iter != m_priv->search_paths.end ();
         ++search_path_iter) {
        a_session.search_paths ().push_back (*search_path_iter);
    }
    THROW_IF_FAIL (session_manager_ptr ()) ;

    //erase all sessions but the 5 last ones, otherwise, the number
    //of debugging session stored will explode with time.
    std::list<ISessMgr::Session> sessions = session_manager_ptr ()->sessions () ;
    int nb_sessions = sessions.size () ;
    if (nb_sessions > 5) {
        int nb_sessions_to_erase = sessions.size () - 5 ;
        std::list<ISessMgr::Session>::const_iterator it ;
        for (int i=0 ; i < nb_sessions_to_erase ; ++i) {
            THROW_IF_FAIL (sessions.begin () != sessions.end ()) ;
            session_manager_ptr ()->delete_session
                (sessions.begin ()->session_id ()) ;
            sessions.erase (sessions.begin ()) ;
        }
    }

    //now store the current session
    session_manager_ptr ()->store_session
                            (a_session,
                             session_manager_ptr ()->default_transaction ()) ;
}


//*******************
//</private methods>
//*******************

DBGPerspective::DBGPerspective () :
    m_priv (new Priv)
{
}

void
DBGPerspective::get_info (Info &a_info) const
{
    static Info s_info ("Debugger perspective plugin",
                        "The debugger perspective of Nemiver",
                        "1.0") ;
    a_info = s_info ;
}

void
DBGPerspective::do_init ()
{
}

void
DBGPerspective::do_init (IWorkbench *a_workbench)
{
    THROW_IF_FAIL (m_priv) ;
    m_priv->workbench = a_workbench ;
    init_icon_factory () ;
    init_actions () ;
    init_toolbar () ;
    init_body () ;
    init_signals () ;
    init_debugger_signals () ;
    init_conf_mgr () ;
    session_manager ().load_sessions (session_manager ().default_transaction ());
    workbench ().shutting_down_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_shutdown_signal)) ;
    m_priv->initialized = true ;
}

DBGPerspective::~DBGPerspective ()
{
    LOG_D ("deleted", "destructor-domain") ;
}

const UString&
DBGPerspective::get_perspective_identifier ()
{
    static UString s_id = "org.nemiver.DebuggerPerspective" ;
    return s_id ;
}

void
DBGPerspective::get_toolbars (list<Gtk::Widget*>  &a_tbs)
{
    CHECK_P_INIT ;
    a_tbs.push_back (m_priv->toolbar.get ()) ;
}

Gtk::Widget*
DBGPerspective::get_body ()
{
    CHECK_P_INIT ;
    return m_priv->body_main_paned.get () ;
}

void
DBGPerspective::edit_workbench_menu ()
{
    CHECK_P_INIT ;

    add_perspective_menu_entries () ;
    //init_perspective_menu_entries () ;
}

void
DBGPerspective::open_file ()
{
    Gtk::FileChooserDialog file_chooser (_("Open file"),
                                         Gtk::FILE_CHOOSER_ACTION_OPEN) ;
    file_chooser.set_current_folder (m_priv->prog_cwd) ;

    file_chooser.add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL) ;
    file_chooser.add_button (Gtk::Stock::OK, Gtk::RESPONSE_OK) ;
    file_chooser.set_select_multiple (true) ;

    int result = file_chooser.run () ;

    if (result != Gtk::RESPONSE_OK) {return;}

    list<UString> paths = file_chooser.get_filenames () ;
    list<UString>::const_iterator iter ;

    for (iter = paths.begin () ; iter != paths.end () ; ++iter) {
        open_file (*iter) ;
    }
}


bool
DBGPerspective::open_file (const UString &a_path,
                           int a_current_line)
{
    if (a_path == "") {return false;}
    if (get_source_editor_from_path (a_path)) {return true ;}

    ifstream file (Glib::filename_from_utf8 (a_path).c_str ()) ;
    if (!file.good () && !file.eof ()) {
        LOG_ERROR ("Could not open file " + a_path) ;
        ui_utils::display_error ("Could not open file: " + a_path) ;
        return false ;
    }

    NEMIVER_TRY

    UString base_name = Glib::locale_to_utf8
        (Glib::path_get_basename (Glib::filename_from_utf8 (a_path))) ;

    UString mime_type = gnome_vfs_get_mime_type_for_name (base_name.c_str ()) ;
    if (mime_type == "") {
        mime_type = "text/x-c++" ;
    }

    Glib::RefPtr<SourceLanguagesManager> lang_manager =
                                    SourceLanguagesManager::create () ;
    Glib::RefPtr<SourceLanguage> lang =
        lang_manager->get_language_from_mime_type (mime_type) ;

    Glib::RefPtr<SourceBuffer> source_buffer = SourceBuffer::create (lang) ;
    THROW_IF_FAIL (source_buffer) ;

    gint buf_size = 10 * 1024 ;
    SafePtr<gchar> buf (new gchar [buf_size + 1]) ;

    for (;;) {
        file.read (buf.get (), buf_size) ;
        THROW_IF_FAIL (file.good () || file.eof ()) ;
        source_buffer->insert (source_buffer->end (), buf.get (),
                               buf.get () + file.gcount ()) ;
        if (file.gcount () != buf_size) {break;}
    }
    file.close () ;

    bool do_highlight=true;
    conf_mgr ().get_key_value (CONF_KEY_HIGHLIGHT_SOURCE_CODE, do_highlight) ;
    source_buffer->set_highlight (do_highlight) ;
    SourceEditor *source_editor (Gtk::manage (new SourceEditor (plugin_path (),
                                                                source_buffer)));
    bool show_line_numbers=true ;
    conf_mgr ().get_key_value (CONF_KEY_SHOW_SOURCE_LINE_NUMBERS,
                               show_line_numbers) ;
    source_editor->source_view ().set_show_line_numbers (show_line_numbers) ;
    source_editor->set_path (a_path) ;
    source_editor->marker_region_got_clicked_signal ().connect
        (sigc::mem_fun
                 (*this,
                  &DBGPerspective::on_source_view_markers_region_clicked_signal));

    if (a_current_line > 0) {
        Gtk::TextIter cur_line_iter =
                source_buffer->get_iter_at_line (a_current_line) ;
        if (cur_line_iter) {
            Glib::RefPtr<SourceMarker> where_marker =
                source_buffer->create_marker ("where-marker",
                                              "line-pointer-marker",
                                              cur_line_iter) ;
            THROW_IF_FAIL (where_marker) ;
        }
    }
    source_editor->show_all () ;
    append_source_editor (*source_editor, a_path) ;

    if (!source_editor->source_view ().has_no_window ()) {
        source_editor->source_view ().add_events
                        (Gdk::BUTTON3_MOTION_MASK |Gdk::KEY_PRESS_MASK) ;
        source_editor->source_view ().signal_button_press_event ().connect
            (sigc::mem_fun
             (*this,
              &DBGPerspective::on_button_pressed_in_source_view_signal)) ;

        source_editor->source_view ().signal_motion_notify_event ().connect
            (sigc::mem_fun
             (*this,
              &DBGPerspective::on_motion_notify_event_signal)) ;

        source_editor->source_view ().signal_leave_notify_event ().connect_notify
            (sigc::mem_fun (*this,
                            &DBGPerspective::on_leave_notify_event_signal)) ;
    }

    m_priv->opened_file_action_group->set_sensitive (true) ;

    NEMIVER_CATCH_AND_RETURN (false);
    return true ;
}

void
DBGPerspective::close_current_file ()
{
    if (!get_n_pages ()) {return;}

    close_file (m_priv->pagenum_2_path_map[m_priv->current_page_num]) ;
}

void
DBGPerspective::close_file (const UString &a_path)
{
    map<UString, int>::const_iterator nil, iter ;
    nil = m_priv->path_2_pagenum_map.end () ;
    iter = m_priv->path_2_pagenum_map.find (a_path) ;
    if (iter == nil) {return;}

    int page_num = m_priv->path_2_pagenum_map[a_path] ;
    m_priv->sourceviews_notebook->remove_page (page_num) ;
    m_priv->path_2_pagenum_map.erase (a_path) ;
    std::string basename = Glib::path_get_basename (Glib::locale_from_utf8 (a_path)) ;
    m_priv->basename_2_pagenum_map.erase (Glib::locale_from_utf8 (basename)) ;
    m_priv->pagenum_2_source_editor_map.erase (page_num) ;
    m_priv->pagenum_2_path_map.erase (page_num) ;

    if (!get_n_pages ()) {
        m_priv->opened_file_action_group->set_sensitive (false) ;
    }
}

ISessMgr&
DBGPerspective::session_manager ()
{
    return *session_manager_ptr () ;
}

void
DBGPerspective::execute_session (ISessMgr::Session &a_session)
{
    m_priv->session = a_session ;

    IDebugger::BreakPoint breakpoint ;
    vector<IDebugger::BreakPoint> breakpoints ;
    list<ISessMgr::BreakPoint>::const_iterator it ;
    for (it = m_priv->session.breakpoints ().begin () ;
         it != m_priv->session.breakpoints ().end ();
         ++it) {
        breakpoint.clear () ;
        breakpoint.line (it->line_number ()) ;
        breakpoint.file_name (it->file_name ()) ;
        breakpoint.file_full_name (it->file_full_name ()) ;
        breakpoints.push_back (breakpoint) ;
    }

    // populate the list of search paths from the current session
    list<UString>::const_iterator path_iter;
    m_priv->search_paths.clear();
    for (path_iter = m_priv->session.search_paths ().begin ();
            path_iter != m_priv->session.search_paths ().end ();
            ++path_iter)
    {
        m_priv->search_paths.push_back (*path_iter);
    }

    // open the previously opened files
    for (path_iter = m_priv->session.opened_files ().begin ();
            path_iter != m_priv->session.opened_files ().end ();
            ++path_iter)
    {
        open_file(*path_iter);
    }

    execute_program (a_session.properties ()[PROGRAM_NAME],
                     a_session.properties ()[PROGRAM_ARGS],
                     a_session.env_variables (),
                     a_session.properties ()[PROGRAM_CWD],
                     breakpoints) ;
    m_priv->reused_session = true ;
}

void
DBGPerspective::execute_program ()
{
    RunProgramDialog dialog (plugin_path ()) ;

    // set defaults from session
    if (debugger ()->get_target_path () != "") {
        dialog.program_name (debugger ()->get_target_path ()) ;
    }
    dialog.arguments (m_priv->prog_args) ;
    if (m_priv->prog_cwd == "") {
        m_priv->prog_cwd = Glib::filename_to_utf8 (Glib::get_current_dir ()) ;
    }
    dialog.working_directory (m_priv->prog_cwd) ;
    dialog.environment_variables (m_priv->env_variables) ;

    int result = dialog.run () ;
    if (result != Gtk::RESPONSE_OK) {
        return;
    }

    UString prog, args, cwd ;
    prog = dialog.program_name () ;
    THROW_IF_FAIL (prog != "") ;
    args = dialog.arguments () ;
    cwd = dialog.working_directory () ;
    THROW_IF_FAIL (cwd != "") ;
    map<UString, UString> env = dialog.environment_variables();

    vector<IDebugger::BreakPoint> breaks ;
    execute_program (prog, args, env, cwd, breaks) ;
    m_priv->reused_session = false ;
}

void
DBGPerspective::execute_program (const UString &a_prog_and_args,
                                 const map<UString, UString> &a_env,
                                 const UString &a_cwd)
{
    UString cwd ;
    if (a_cwd == "." || a_cwd == "") {
        cwd = Glib::filename_to_utf8 (Glib::get_current_dir ()) ;
    } else {
        cwd = a_cwd ;
    }
    vector<UString> argv = a_prog_and_args.split (" ") ;
    vector<UString>::const_iterator iter = argv.begin () ;
    vector<UString>::const_iterator end_iter = argv.end () ;
    ++iter ;
    UString prog_name=argv[0], args = UString::join (iter, end_iter);
    vector<IDebugger::BreakPoint> breaks ;
    execute_program (prog_name, args, a_env, cwd, breaks) ;
    m_priv->reused_session = false ;
}

void
DBGPerspective::execute_program (const UString &a_prog,
                                 const UString &a_args,
                                 const map<UString, UString> &a_env,
                                 const UString &a_cwd,
                                 const vector<IDebugger::BreakPoint> &a_breaks)
{
    NEMIVER_TRY

    IDebuggerSafePtr dbg_engine = debugger () ;
    THROW_IF_FAIL (dbg_engine) ;
    vector<UString> args = a_args.split (" ") ;
    args.insert (args.begin (), a_prog) ;
    vector<UString> source_search_dirs = a_cwd.split (" ") ;

    dbg_engine->load_program (args, a_cwd, source_search_dirs,
                              get_terminal ().slave_pts_name ()) ;

    dbg_engine->add_env_variables (a_env) ;

    if (a_breaks.empty ()) {
        dbg_engine->set_breakpoint ("main") ;
    } else {
        vector<IDebugger::BreakPoint>::const_iterator it ;
        for (it = a_breaks.begin () ; it != a_breaks.end () ; ++it) {
            dbg_engine->set_breakpoint (it->file_full_name (),
                                        it->line ()) ;
        }
    }

    dbg_engine->run () ;
    m_priv->debugger_has_just_run = true ;

    attached_to_target_signal ().emit (true) ;

    m_priv->prog_path = a_prog ;
    m_priv->prog_args = a_args ;
    m_priv->prog_cwd = a_cwd ;
    m_priv->env_variables = a_env ;

    NEMIVER_CATCH
}

void
DBGPerspective::attach_to_program ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD

    IProcMgr *process_manager = get_process_manager () ;
    THROW_IF_FAIL (process_manager) ;
    ProcListDialog dialog (plugin_path (),
                           *process_manager);
    int result = dialog.run () ;
    if (result != Gtk::RESPONSE_OK) {
        return;
    }
    if (dialog.has_selected_process ()) {
        IProcMgr::Process process ;
        THROW_IF_FAIL (dialog.get_selected_process (process));
        attach_to_program (process.pid ()) ;
    }
}

void
DBGPerspective::attach_to_program (unsigned int a_pid)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD

    LOG_DD ("a_pid: " << (int) a_pid) ;

    if (a_pid == (unsigned int) getpid ()) {
        ui_utils::display_warning ("You can not attach to nemiver itself") ;
        return ;
    }
    if (!debugger ()->attach_to_program (a_pid,
                                         get_terminal ().slave_pts_name ())) {
        ui_utils::display_warning ("You can not attach to the "
                                   "underlying debugger engine") ;
    }
}

void
DBGPerspective::saved_sessions ()
{
    SavedSessionsDialog dialog (plugin_path (), session_manager_ptr ()) ;
    int result = dialog.run ();
    if (result != Gtk::RESPONSE_OK) {
        return;
    }
    ISessMgr::Session session = dialog.session ();
    execute_session (session);
}

void
DBGPerspective::edit_preferences ()
{
    THROW_IF_FAIL (m_priv) ;
    PreferencesDialog dialog (workbench (), plugin_path ());
    dialog.run () ;
}

void
DBGPerspective::run ()
{
    THROW_IF_FAIL (m_priv) ;
    going_to_run_target_signal ().emit () ;
    debugger ()->run () ;
    m_priv->debugger_has_just_run = true ;
}

void
DBGPerspective::load_core_file ()
{
    LoadCoreDialog dialog (plugin_path ()) ;

    int result = dialog.run () ;
    if (result != Gtk::RESPONSE_OK) {
        return;
    }

    UString prog_path, core_path ;
    prog_path = dialog.program_name () ;
    THROW_IF_FAIL (prog_path != "") ;
    core_path = dialog.core_file () ;
    THROW_IF_FAIL (core_path != "") ;

    load_core_file (prog_path, core_path) ;
}

void
DBGPerspective::load_core_file (const UString &a_prog_path,
                                const UString &a_core_file_path)
{
    debugger ()->load_core_file (a_prog_path, a_core_file_path) ;
    debugger ()->list_frames () ;
}

void
DBGPerspective::stop ()
{
    LOG_FUNCTION_SCOPE_NORMAL_D (NMV_DEFAULT_DOMAIN) ;
    if (!debugger ()->stop_target ()) {
        ui_utils::display_error ("Failed to stop the debugger") ;
    }
}

void
DBGPerspective::step_over ()
{
    debugger ()->step_over () ;
}

void
DBGPerspective::step_into ()
{
    debugger ()->step_in () ;
}

void
DBGPerspective::step_out ()
{
    debugger ()->step_out () ;
}

void
DBGPerspective::do_continue ()
{
    debugger ()->do_continue () ;
}

void
DBGPerspective::do_continue_until ()
{
    SourceEditor *editor = get_current_source_editor () ;
    THROW_IF_FAIL (editor) ;

    UString file_path ;
    editor->get_file_name (file_path) ;
    int current_line = editor->current_line () ;

    debugger ()->continue_to_position (file_path, current_line) ;
}

void
DBGPerspective::set_breakpoint ()
{
    SourceEditor *source_editor = get_current_source_editor () ;
    THROW_IF_FAIL (source_editor) ;
    UString path ;
    source_editor->get_path (path) ;
    THROW_IF_FAIL (path != "") ;

    //line numbers start at 0 in GtkSourceView, but at 1 in GDB <grin/>
    //so in DBGPerspective, the line number are set in the GDB's reference.

    gint current_line =
        source_editor->source_view ().get_source_buffer ()->get_insert
            ()->get_iter ().get_line () + 1;
    set_breakpoint (path, current_line) ;
}

void
DBGPerspective::set_breakpoint (const UString &a_file_path,
                                int a_line)
{
    LOG_DD ("set bkpoint request for " << a_file_path << ":" << a_line) ;
    debugger ()->set_breakpoint (a_file_path, a_line) ;
}

void
DBGPerspective::append_breakpoints
                        (const map<int, IDebugger::BreakPoint> &a_breaks)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;

    map<int, IDebugger::BreakPoint>::const_iterator iter ;
    UString file_path ;
    for (iter = a_breaks.begin () ; iter != a_breaks.end () ; ++iter) {
         file_path = iter->second.file_full_name () ;
        //if the file full path info is not present,
        //1/ lookup in the files opened in the perspective already.
        //2/ lookup in the list of source directories
        if (file_path == "") {
            UString file_name = iter->second.file_name () ;
            LOG_DD ("no full path info present for file '"
                    + file_name + "'") ;
            if (file_name == "") {
                ui_utils::display_error
                    (_("There is no file name info for symbol@addr: ")
                     +iter->second.function () + "@" + iter->second.address ()) ;
            }
            else
            {
                file_path = file_name ;
            }
        }
        LOG_DD ("record breakpoint " << file_path << ":"
                << iter->second.line () - 1) ;
        m_priv->breakpoints[iter->first] = iter->second ;
        m_priv->breakpoints[iter->first].file_full_name (file_path) ;
        append_visual_breakpoint (file_path, iter->second.line () - 1) ;
    }
}

bool
DBGPerspective::get_breakpoint_number (const UString &a_file_name,
                                       int a_line_num,
                                       int &a_break_num)
{
    UString breakpoint = a_file_name + ":" + UString::from_int (a_line_num) ;

    LOG_DD ("searching for breakpoint " << breakpoint << ": ") ;

    map<int, IDebugger::BreakPoint>::const_iterator iter ;
    for (iter = m_priv->breakpoints.begin () ;
         iter != m_priv->breakpoints.end () ;
         ++iter) {
        LOG_DD ("got breakpoint " << iter->second.file_full_name ()
                << ":" << iter->second.line () << "...") ;
        // because some versions of gdb don't return the full file path info for
        // breakpoints, we have to also check to see if the basenames match
        if (((iter->second.file_full_name () == a_file_name) ||
                    (Glib::path_get_basename (iter->second.file_full_name ()) ==
                     Glib::path_get_basename (a_file_name))
            )
            && (iter->second.line () == a_line_num)) {
            a_break_num= iter->second.number () ;
            LOG_DD ("found breakpoint " << breakpoint << " !") ;
            return true ;
        }
    }
    LOG_DD ("did not find breakpoint " + breakpoint) ;
    return false ;
}

bool
DBGPerspective::delete_breakpoint ()
{
    SourceEditor *source_editor = get_current_source_editor () ;
    THROW_IF_FAIL (source_editor) ;
    UString path ;
    source_editor->get_path (path);
    THROW_IF_FAIL (path  != "") ;

    gint current_line =
        source_editor->source_view ().get_source_buffer ()->get_insert
            ()->get_iter ().get_line () + 1;
    int break_num=0 ;
    if (!get_breakpoint_number (path,
                                current_line,
                                break_num)) {
        return false ;
    }
    THROW_IF_FAIL (break_num) ;
    return delete_breakpoint (break_num) ;
}

bool
DBGPerspective::delete_breakpoint (int a_breakpoint_num)
{
    map<int, IDebugger::BreakPoint>::iterator iter =
        m_priv->breakpoints.find (a_breakpoint_num) ;
    if (iter == m_priv->breakpoints.end ()) {
        LOG_ERROR ("breakpoint " << (int) a_breakpoint_num << " not found") ;
        return false ;
    }
    debugger ()->delete_breakpoint (a_breakpoint_num) ;
    return true ;
}

void
DBGPerspective::append_visual_breakpoint (const UString &a_file_name,
                                          int a_linenum)
{
    THROW_IF_FAIL (!a_file_name.empty());
    THROW_IF_FAIL(m_priv);
    LOG_FUNCTION_SCOPE_NORMAL_DD

    LOG_DD ("a_file_name: " << a_file_name) ;
    LOG_DD ("a_linenum: " << (int)a_linenum) ;

    if (a_linenum < 0) {a_linenum = 0;}

    SourceEditor *source_editor = get_source_editor_from_path (a_file_name) ;
    // first assume that it's a full pathname and just try to open it
    if (!source_editor) {
        if (Glib::file_test (a_file_name, Glib::FILE_TEST_IS_REGULAR)) {
            open_file (a_file_name) ;
            source_editor = get_source_editor_from_path (a_file_name) ;
        }
    }
    // if that didn't work, look for an open source editor that matches the base
    // name
    if (!source_editor) {
        source_editor = get_source_editor_from_path (a_file_name, true) ;
    }
    // if that still didn't work, look for a file of that name in the search
    // directories and then as a last resort ask teh user to locate it manually
    if (!source_editor) {
        UString file_path;
        if (!find_file_in_source_dirs (a_file_name, file_path)) {
            LOG_DD ("didn't find file either in opened files "
                    " or in source dirs:") ;
            // Pop up a dialog asking user to select the specified file
            LocateFileDialog dialog (plugin_path (), a_file_name) ;
            // start looking in the working directory
            dialog.file_location(m_priv->prog_cwd);
            int result = dialog.run () ;
            if (result == Gtk::RESPONSE_OK) {
                file_path = dialog.file_location () ;
                THROW_IF_FAIL(Glib::file_test(file_path, Glib::FILE_TEST_IS_REGULAR));
                THROW_IF_FAIL(Glib::path_get_basename(a_file_name) ==
                        Glib::path_get_basename(file_path));
                UString parent_dir = Glib::path_get_dirname(dialog.file_location ());
                THROW_IF_FAIL(Glib::file_test(parent_dir, Glib::FILE_TEST_IS_DIR));

                // Also add the parent directory to the list of paths to search
                // for this session so you don't have to keep selecting files if
                // they're all in the same directory.
                // We can assume that the parent directory is not already in the
                // list of source dirs (or else it would have been found and the
                // user wouldn't have had to locate it manually) so just tack it
                // on the end of the list
                m_priv->search_paths.push_back (parent_dir);
            }
        }

        open_file (file_path) ;
        source_editor = get_source_editor_from_path (file_path) ;
    }

    // finally, if none of these things worked, display an error
    if (!source_editor) {
        LOG_ERROR ("Could not find source editor for file: '"
                << a_file_name
                << "'") ;
        ui_utils::display_error (_("Could not find file: ")
                + a_file_name 
                + "\n") ;
    } else {
        source_editor->set_visual_breakpoint_at_line (a_linenum) ;
    }

}

void
DBGPerspective::delete_visual_breakpoint (const UString &a_file_name, int a_linenum)
{
    SourceEditor *source_editor = get_source_editor_from_path (a_file_name) ;
    if (!source_editor) {
        source_editor = get_source_editor_from_path (a_file_name, true) ;
    }
    THROW_IF_FAIL (source_editor) ;
    source_editor->remove_visual_breakpoint_from_line (a_linenum) ;
}

void
DBGPerspective::delete_visual_breakpoint (int a_breakpoint_num)
{
    map<int, IDebugger::BreakPoint>::iterator iter =
        m_priv->breakpoints.find (a_breakpoint_num) ;
    if (iter == m_priv->breakpoints.end ()) {
        LOG_ERROR ("breakpoint " << (int) a_breakpoint_num << " not found") ;
        return ;
    }

    SourceEditor *source_editor =
        get_source_editor_from_path (iter->second.file_full_name ()) ;
    if (!source_editor) {
        source_editor = get_source_editor_from_path (iter->second.file_full_name (),
                                                     true) ;
    }
    THROW_IF_FAIL (source_editor) ;
    source_editor->remove_visual_breakpoint_from_line (iter->second.line ()-1) ;
    m_priv->breakpoints.erase (iter);
    LOG_DD ("erased breakpoint number " << (int) a_breakpoint_num) ;
}

bool
DBGPerspective::delete_breakpoint (const UString &a_file_name,
                                   int a_line_num)
{
    int breakpoint_number=0 ;
    if (!get_breakpoint_number (a_file_name,
                                a_line_num,
                                breakpoint_number)) {
        return false ;
    }
    if (breakpoint_number < 1) {return false;}

    return delete_breakpoint (breakpoint_number) ;
}

bool
DBGPerspective::is_breakpoint_set_at_line (const UString &a_file_path,
                                           int a_line_num)
{
    int break_num=0 ;
    if (get_breakpoint_number (a_file_path, a_line_num, break_num)) {
        return true ;
    }
    return false ;
}

void
DBGPerspective::toggle_breakpoint (const UString &a_file_path,
                                   int a_line_num)
{
    LOG_DD ("file_path:" << a_file_path
           << ", line_num: " << a_file_path) ;

    if (is_breakpoint_set_at_line (a_file_path, a_line_num)) {
        LOG_DD ("breakpoint set already, delete it!") ;
        delete_breakpoint (a_file_path, a_line_num) ;
    } else {
        LOG_DD ("breakpoint no set yet, set it!") ;
        set_breakpoint (a_file_path, a_line_num) ;
    }
}

void
DBGPerspective::toggle_breakpoint ()
{
    SourceEditor *source_editor = get_current_source_editor () ;
    THROW_IF_FAIL (source_editor) ;
    UString path ;
    source_editor->get_path (path) ;
    THROW_IF_FAIL (path != "") ;

    gint current_line =
        source_editor->source_view ().get_source_buffer ()->get_insert
                ()->get_iter ().get_line () + 1;
    toggle_breakpoint (path, current_line) ;
}

void
DBGPerspective::inspect_variable ()
{
    THROW_IF_FAIL (m_priv) ;

    Gtk::TextIter start, end ;
    SourceEditor *source_editor = get_current_source_editor () ;
    THROW_IF_FAIL (source_editor) ;
    Glib::RefPtr<gtksourceview::SourceBuffer> buffer =
                            source_editor->source_view ().get_source_buffer () ;
    THROW_IF_FAIL (buffer) ;
    UString variable_name ;
    if (buffer->get_selection_bounds (start, end)) {
        variable_name= buffer->get_slice (start, end) ;
    }
    inspect_variable (variable_name) ;
}

void
DBGPerspective::inspect_variable (const UString &a_variable_name)
{
    THROW_IF_FAIL (debugger ()) ;
    VarInspectorDialog dialog (plugin_path (), *debugger ()) ;
    if (a_variable_name != "") {
        dialog.inspect_variable (a_variable_name) ;
    }
    dialog.run () ;
}

IDebuggerSafePtr&
DBGPerspective::debugger ()
{
    if (!m_priv->debugger) {
        DynamicModule::Loader *loader = workbench ().get_module_loader () ;
        THROW_IF_FAIL (loader) ;
        DynamicModuleManager *module_manager =
                            loader->get_dynamic_module_manager () ;
        THROW_IF_FAIL (module_manager) ;

        m_priv->debugger =
            module_manager->load<IDebugger> ("gdbengine") ;
        m_priv->debugger->set_event_loop_context
                                    (Glib::MainContext::get_default ()) ;
    }
    THROW_IF_FAIL (m_priv->debugger) ;
    return m_priv->debugger ;
}

IConfMgr&
DBGPerspective::conf_mgr ()
{
    return workbench ().get_configuration_manager () ;
}

Gtk::TextView&
DBGPerspective::get_command_view ()
{
    THROW_IF_FAIL (m_priv && m_priv->command_view) ;
    return *m_priv->command_view ;
}

Gtk::ScrolledWindow&
DBGPerspective::get_command_view_scrolled_win ()
{
    THROW_IF_FAIL (m_priv) ;

    if (!m_priv->command_view_scrolled_win) {
        m_priv->command_view_scrolled_win.reset (new Gtk::ScrolledWindow) ;
        m_priv->command_view_scrolled_win->set_policy (Gtk::POLICY_AUTOMATIC,
                                                       Gtk::POLICY_AUTOMATIC) ;
        THROW_IF_FAIL (m_priv->command_view_scrolled_win) ;
    }
    return *m_priv->command_view_scrolled_win ;
}

Gtk::TextView&
DBGPerspective::get_target_output_view ()
{
    THROW_IF_FAIL (m_priv && m_priv->target_output_view) ;
    return *m_priv->target_output_view ;
}

Gtk::ScrolledWindow&
DBGPerspective::get_target_output_view_scrolled_win ()
{
    THROW_IF_FAIL (m_priv) ;
    if (!m_priv->target_output_view_scrolled_win) {
        m_priv->target_output_view_scrolled_win.reset (new Gtk::ScrolledWindow) ;
        m_priv->target_output_view_scrolled_win->set_policy
                                                    (Gtk::POLICY_AUTOMATIC,
                                                     Gtk::POLICY_AUTOMATIC) ;
        THROW_IF_FAIL (m_priv->target_output_view_scrolled_win) ;
    }
    return *m_priv->target_output_view_scrolled_win ;
}

Gtk::TextView&
DBGPerspective::get_log_view ()
{
    THROW_IF_FAIL (m_priv && m_priv->log_view) ;
    return *m_priv->log_view ;
}

Gtk::ScrolledWindow&
DBGPerspective::get_log_view_scrolled_win ()
{
    THROW_IF_FAIL (m_priv) ;
    if (!m_priv->log_view_scrolled_win) {
        m_priv->log_view_scrolled_win.reset (new Gtk::ScrolledWindow) ;
        m_priv->log_view_scrolled_win->set_policy (Gtk::POLICY_AUTOMATIC,
                                                     Gtk::POLICY_AUTOMATIC) ;
        THROW_IF_FAIL (m_priv->log_view_scrolled_win) ;
    }
    return *m_priv->log_view_scrolled_win ;
}

CallStack&
DBGPerspective::get_call_stack ()
{
    THROW_IF_FAIL (m_priv) ;
    if (!m_priv->call_stack) {
        m_priv->call_stack.reset (new CallStack (debugger (), workbench (), *this)) ;
        THROW_IF_FAIL (m_priv) ;
    }
    return *m_priv->call_stack ;
}

Gtk::ScrolledWindow&
DBGPerspective::get_call_stack_scrolled_win ()
{
    THROW_IF_FAIL (m_priv) ;
    if (!m_priv->call_stack_scrolled_win) {
        m_priv->call_stack_scrolled_win.reset (new Gtk::ScrolledWindow ()) ;
        m_priv->call_stack_scrolled_win->set_policy (Gtk::POLICY_AUTOMATIC,
                                                     Gtk::POLICY_AUTOMATIC) ;
        THROW_IF_FAIL (m_priv->call_stack_scrolled_win) ;
    }
    return *m_priv->call_stack_scrolled_win ;
}

LocalVarsInspector&
DBGPerspective::get_local_vars_inspector ()
{
    THROW_IF_FAIL (m_priv) ;
    THROW_IF_FAIL (m_priv->workbench) ;

    if (!m_priv->variables_editor) {
        m_priv->variables_editor.reset
            (new LocalVarsInspector (debugger (), *m_priv->workbench)) ;
    }
    THROW_IF_FAIL (m_priv->variables_editor) ;
    return *m_priv->variables_editor ;
}

Gtk::ScrolledWindow&
DBGPerspective::get_local_vars_inspector_scrolled_win ()
{
    THROW_IF_FAIL (m_priv) ;
    if (!m_priv->variables_editor_scrolled_win) {
        m_priv->variables_editor_scrolled_win.reset (new Gtk::ScrolledWindow) ;
        m_priv->variables_editor_scrolled_win->set_policy (Gtk::POLICY_AUTOMATIC,
                                                           Gtk::POLICY_AUTOMATIC);
    }
    THROW_IF_FAIL (m_priv->variables_editor_scrolled_win) ;
    return *m_priv->variables_editor_scrolled_win ;
}


Terminal&
DBGPerspective::get_terminal ()
{
    THROW_IF_FAIL (m_priv) ;
    if (!m_priv->terminal) {
        m_priv->terminal.reset (new Terminal) ;
    }
    THROW_IF_FAIL (m_priv->terminal) ;
    return *m_priv->terminal ;
}

Gtk::Box &
DBGPerspective::get_terminal_box ()
{
    THROW_IF_FAIL (m_priv) ;
    if (!m_priv->terminal_box) {
        m_priv->terminal_box.reset (new Gtk::HBox) ;
        THROW_IF_FAIL (m_priv->terminal_box) ;
        Gtk::VScrollbar *scrollbar = Gtk::manage (new Gtk::VScrollbar) ;
        m_priv->terminal_box->pack_end (*scrollbar, false, false, 0) ;
        m_priv->terminal_box->pack_start (get_terminal ().widget ()) ;
        scrollbar->set_adjustment (get_terminal ().adjustment ()) ;
    }
    THROW_IF_FAIL (m_priv->terminal_box) ;
    return *m_priv->terminal_box;
}

Gtk::ScrolledWindow&
DBGPerspective::get_breakpoints_scrolled_win ()
{
    THROW_IF_FAIL (m_priv) ;
    if (!m_priv->breakpoints_scrolled_win) {
        m_priv->breakpoints_scrolled_win.reset (new Gtk::ScrolledWindow) ;
        THROW_IF_FAIL (m_priv->breakpoints_scrolled_win) ;
        m_priv->breakpoints_scrolled_win->set_policy (Gtk::POLICY_AUTOMATIC,
                                                   Gtk::POLICY_AUTOMATIC) ;
    }
    THROW_IF_FAIL (m_priv->breakpoints_scrolled_win) ;
    return *m_priv->breakpoints_scrolled_win ;
}

BreakpointsView&
DBGPerspective::get_breakpoints_view ()
{
    THROW_IF_FAIL (m_priv) ;
    if (!m_priv->breakpoints_view) {
        m_priv->breakpoints_view.reset (new BreakpointsView (
                    workbench (), *this)) ;
    }
    THROW_IF_FAIL (m_priv->breakpoints_view) ;
    return *m_priv->breakpoints_view ;
}

void
DBGPerspective::set_show_command_view (bool a_show)
{
    if (a_show) {
        if (!get_command_view_scrolled_win ().get_parent ()
            && m_priv->command_view_is_visible == false) {
            get_command_view_scrolled_win ().show_all () ;
            int pagenum =
            m_priv->statuses_notebook->insert_page
                            (get_command_view_scrolled_win (),
                             _("Commands"),
                             COMMAND_VIEW_INDEX) ;
            m_priv->statuses_notebook->set_current_page (pagenum) ;
            m_priv->command_view_is_visible = true ;
        }
    } else {
        if (get_command_view_scrolled_win ().get_parent ()
            && m_priv->command_view_is_visible) {
            m_priv->statuses_notebook->remove_page
                                        (get_command_view_scrolled_win ());
            m_priv->command_view_is_visible = false;
        }
    }
    show_command_view_signal ().emit (a_show) ;
}

void
DBGPerspective::set_show_target_output_view (bool a_show)
{
    if (a_show) {
        if (!get_target_output_view_scrolled_win ().get_parent ()
            && m_priv->target_output_view_is_visible == false) {
            get_target_output_view_scrolled_win ().show_all () ;
            int page_num =
                m_priv->statuses_notebook->insert_page
                    (get_target_output_view_scrolled_win (),
                     _("Output"),
                     TARGET_OUTPUT_VIEW_INDEX) ;
            m_priv->target_output_view_is_visible = true ;
            m_priv->statuses_notebook->set_current_page (page_num);
        }
    } else {
        if (get_target_output_view_scrolled_win ().get_parent ()
            && m_priv->target_output_view_is_visible) {
            m_priv->statuses_notebook->remove_page
                                    (get_target_output_view_scrolled_win ());
            m_priv->target_output_view_is_visible = false;
        }
        m_priv->target_output_view_is_visible = false;
    }
    show_target_output_view_signal ().emit (a_show) ;
}

void
DBGPerspective::set_show_log_view (bool a_show)
{
    if (a_show) {
        if (!get_log_view_scrolled_win ().get_parent ()
            && m_priv->log_view_is_visible == false) {
            get_log_view_scrolled_win ().show_all () ;
            int page_num =
                m_priv->statuses_notebook->insert_page
                    (get_log_view_scrolled_win (), _("Logs"), ERROR_VIEW_INDEX) ;
            m_priv->log_view_is_visible = true ;
            m_priv->statuses_notebook->set_current_page (page_num);
        }
    } else {
        if (get_log_view_scrolled_win ().get_parent ()
            && m_priv->log_view_is_visible) {
            LOG_DD ("removing log view") ;
            m_priv->statuses_notebook->remove_page
                                        (get_log_view_scrolled_win ());
        }
        m_priv->log_view_is_visible = false;
    }
    show_log_view_signal ().emit (a_show) ;
}

void
DBGPerspective::set_show_call_stack_view (bool a_show)
{
    if (a_show) {
        if (!get_call_stack_scrolled_win ().get_parent ()
            && m_priv->call_stack_view_is_visible == false) {
            get_call_stack_scrolled_win ().show_all () ;
            int page_num = m_priv->statuses_notebook->insert_page
                                            (get_call_stack_scrolled_win (),
                                             _("Call stack"),
                                             CALL_STACK_VIEW_INDEX) ;
            m_priv->call_stack_view_is_visible = true ;
            m_priv->statuses_notebook->set_current_page
                                                (page_num);
        }
    } else {
        if (get_call_stack_scrolled_win ().get_parent ()
            && m_priv->call_stack_view_is_visible) {
            LOG_DD ("removing call stack view") ;
            m_priv->statuses_notebook->remove_page
                                        (get_call_stack_scrolled_win ());
            m_priv->call_stack_view_is_visible = false;
        }
        m_priv->call_stack_view_is_visible = false;
    }
}

void
DBGPerspective::set_show_variables_editor_view (bool a_show)
{
    if (a_show) {
        if (!get_local_vars_inspector_scrolled_win ().get_parent ()
            && m_priv->variables_editor_view_is_visible == false) {
            get_local_vars_inspector_scrolled_win ().show_all () ;
            int page_num = m_priv->statuses_notebook->insert_page
                                            (get_local_vars_inspector_scrolled_win (),
                                             _("Variables"),
                                             VARIABLES_VIEW_INDEX) ;
            m_priv->variables_editor_view_is_visible = true ;
            m_priv->statuses_notebook->set_current_page
                                                (page_num);
        }
    } else {
        if (get_local_vars_inspector_scrolled_win ().get_parent ()
            && m_priv->variables_editor_view_is_visible) {
            LOG_DD ("removing variables editor") ;
            m_priv->statuses_notebook->remove_page
                                        (get_local_vars_inspector_scrolled_win ());
            m_priv->variables_editor_view_is_visible = false;
        }
        m_priv->variables_editor_view_is_visible = false;
    }
}

void
DBGPerspective::set_show_terminal_view (bool a_show)
{
    if (a_show) {
        if (!get_terminal_box ().get_parent ()
            && m_priv->terminal_view_is_visible == false) {
            get_terminal_box ().show_all () ;
            int page_num = m_priv->statuses_notebook->insert_page
                                            (get_terminal_box (),
                                             _("Target terminal"),
                                             TERMINAL_VIEW_INDEX) ;
            m_priv->terminal_view_is_visible = true ;
            m_priv->statuses_notebook->set_current_page (page_num);
        }
    } else {
        if (get_terminal_box ().get_parent ()
            && m_priv->terminal_view_is_visible) {
            LOG_DD ("removing terminal view") ;
            m_priv->statuses_notebook->remove_page
                                        (get_terminal_box ());
            m_priv->terminal_view_is_visible = false;
        }
        m_priv->terminal_view_is_visible = false;
    }
}

void
DBGPerspective::set_show_breakpoints_view (bool a_show)
{
    if (a_show) {
        if (!get_breakpoints_scrolled_win ().get_parent ()
            && m_priv->breakpoints_view_is_visible == false) {
            get_breakpoints_scrolled_win ().show_all () ;
            int page_num = m_priv->statuses_notebook->insert_page
                                            (get_breakpoints_scrolled_win (),
                                             _("Breakpoints"),
                                             BREAKPOINTS_VIEW_INDEX) ;
            m_priv->breakpoints_view_is_visible = true ;
            m_priv->statuses_notebook->set_current_page (page_num);
        }
    } else {
        if (get_breakpoints_scrolled_win ().get_parent ()
            && m_priv->breakpoints_view_is_visible) {
            LOG_DD ("removing breakpoints view") ;
            m_priv->statuses_notebook->remove_page
                                        (get_breakpoints_scrolled_win ());
            m_priv->breakpoints_view_is_visible = false;
        }
        m_priv->breakpoints_view_is_visible = false;
    }
}

struct ScrollTextViewToEndClosure {
    Gtk::TextView* text_view ;

    ScrollTextViewToEndClosure (Gtk::TextView *a_view=NULL) :
        text_view (a_view)
    {
    }

    bool do_exec ()
    {
        if (!text_view) {return false;}
        if (!text_view->get_buffer ()) {return false;}

        Gtk::TextIter end_iter = text_view->get_buffer ()->end () ;
        text_view->scroll_to (end_iter) ;
        return false ;
    }
};//end struct ScrollTextViewToEndClosure

void
DBGPerspective::add_text_to_command_view (const UString &a_text,
                                          bool a_no_repeat)
{
    if (a_no_repeat) {
        if (a_text == m_priv->last_command_text)
            return ;
    }
    THROW_IF_FAIL (m_priv && m_priv->command_view) ;
    m_priv->command_view->get_buffer ()->insert
        (get_command_view ().get_buffer ()->end (), a_text ) ;
    static ScrollTextViewToEndClosure s_scroll_to_end_closure ;
    s_scroll_to_end_closure.text_view = m_priv->command_view.get () ;
    Glib::signal_idle ().connect (sigc::mem_fun
            (s_scroll_to_end_closure, &ScrollTextViewToEndClosure::do_exec)) ;
    m_priv->last_command_text = a_text ;
}

void
DBGPerspective::add_text_to_target_output_view (const UString &a_text)
{
    THROW_IF_FAIL (m_priv && m_priv->target_output_view) ;
    m_priv->target_output_view->get_buffer ()->insert
        (get_target_output_view ().get_buffer ()->end (),
         a_text) ;
    static ScrollTextViewToEndClosure s_scroll_to_end_closure ;
    s_scroll_to_end_closure.text_view = m_priv->target_output_view.get () ;
    Glib::signal_idle ().connect (sigc::mem_fun
            (s_scroll_to_end_closure, &ScrollTextViewToEndClosure::do_exec)) ;
}

void
DBGPerspective::add_text_to_log_view (const UString &a_text)
{
    THROW_IF_FAIL (m_priv && m_priv->log_view) ;
    m_priv->log_view->get_buffer ()->insert
        (get_log_view ().get_buffer ()->end (), a_text) ;
    static ScrollTextViewToEndClosure s_scroll_to_end_closure ;
    s_scroll_to_end_closure.text_view = m_priv->log_view.get () ;
    Glib::signal_idle ().connect (sigc::mem_fun
            (s_scroll_to_end_closure, &ScrollTextViewToEndClosure::do_exec)) ;
}

sigc::signal<void, bool>&
DBGPerspective::activated_signal ()
{
    CHECK_P_INIT ;
    return m_priv->activated_signal ;
}

sigc::signal<void, bool>&
DBGPerspective::attached_to_target_signal ()
{
    return m_priv->attached_to_target_signal ;
}

sigc::signal<void, bool>&
DBGPerspective::debugger_ready_signal ()
{
    return m_priv->debugger_ready_signal ;
}

sigc::signal<void>&
DBGPerspective::going_to_run_target_signal ()
{
    return m_priv->going_to_run_target_signal ;
}

sigc::signal<void, bool>&
DBGPerspective::show_command_view_signal ()
{
    return m_priv->show_command_view_signal ;
}

sigc::signal<void, bool>&
DBGPerspective::show_target_output_view_signal ()
{
    return m_priv->show_target_output_view_signal ;
}

sigc::signal<void, bool>&
DBGPerspective::show_log_view_signal ()
{
    return m_priv->show_log_view_signal ;
}

NEMIVER_END_NAMESPACE (nemiver)

//the dynmod initial factory.
extern "C" {
bool
NEMIVER_API nemiver_common_create_dynamic_module_instance (void **a_new_instance)
{
    gtksourceview::init () ;
    *a_new_instance = new nemiver::DBGPerspective () ;
    return (*a_new_instance != 0) ;
}

}//end extern C
