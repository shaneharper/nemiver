// Author: Dodji Seketeli
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
#ifndef __NMV_GDB_ENGINE_H__
#define __NMV_GDB_ENGINE_H__

#include "nmv-dbg-common.h"
#include "nmv-gdbmi-parser.h"

NEMIVER_BEGIN_NAMESPACE (nemiver)

class GDBEngine : public IDebugger {

    GDBEngine (const GDBEngine &) ;
    GDBEngine& operator= (const GDBEngine &) ;

    struct Priv ;
    SafePtr<Priv> m_priv ;
    friend struct Priv ;

public:

    GDBEngine (DynamicModule *a_dynmod) ;
    virtual ~GDBEngine () ;

    //*************
    //<signals>
    //*************
    sigc::signal<void, Output&>& pty_signal () const ;

    sigc::signal<void, CommandAndOutput&>& stdout_signal () const ;

    sigc::signal<void, Output&>& stderr_signal () const ;

    sigc::signal<void>& engine_died_signal () const ;

    sigc::signal<void, const UString&>& console_message_signal () const ;

    sigc::signal<void, const UString&>& target_output_message_signal () const ;

    sigc::signal<void, const UString&>& log_message_signal () const ;

    sigc::signal<void, const UString&, const UString&>&
                                        command_done_signal () const ;

    sigc::signal<void>& connected_to_server_signal () const ;

    sigc::signal<void>& detached_from_target_signal () const ;

    sigc::signal<void, const map<int, IDebugger::BreakPoint>&, const UString&>&
                                            breakpoints_set_signal () const ;

    sigc::signal<void, const vector<OverloadsChoiceEntry>&, const UString&>&
                                    got_overloads_choice_signal () const;

    sigc::signal<void, const IDebugger::BreakPoint&, int, const UString&>&
                                            breakpoint_deleted_signal () const ;

    sigc::signal<void, const IDebugger::BreakPoint&, int>&
                                        breakpoint_disabled_signal () const ;

    sigc::signal<void, const IDebugger::BreakPoint&, int>&
                                        breakpoint_enabled_signal () const ;


    sigc::signal<void,
                IDebugger::StopReason,
                bool,
                const IDebugger::Frame&,
                int,
                const UString& /*cookie*/>& stopped_signal () const ;

    sigc::signal<void,
                 const list<int>,
                 const UString& >& threads_listed_signal () const ;

    sigc::signal<void, const vector<UString>&, const UString& >&
                                                files_listed_signal () const;

    sigc::signal<void,
                 int,
                 const Frame&,
                 const UString&>& thread_selected_signal () const  ;

    sigc::signal<void,
                 const vector<IDebugger::Frame>&,
                 const UString&>& frames_listed_signal () const ;

    sigc::signal<void,
                 const map<int, list<IDebugger::VariableSafePtr> >&,
                 const UString& /*cookie*/>&
                                    frames_arguments_listed_signal () const;

    sigc::signal<void, const IDebugger::Frame&, const UString&>&
                                                current_frame_signal () const ;

    sigc::signal<void, const list<VariableSafePtr>&, const UString&>&
                        local_variables_listed_signal () const ;

    sigc::signal<void, const list<VariableSafePtr>&, const UString&>&
                        global_variables_listed_signal () const ;


    sigc::signal<void,
                 const UString&,
                 const IDebugger::VariableSafePtr&,
                 const UString&>& variable_value_signal () const  ;

    sigc::signal<void,
                 const VariableSafePtr&/*variable*/,
                 const UString& /*cookie*/>&
                                     variable_value_set_signal () const ;

    sigc::signal<void, const UString&, const VariableSafePtr&, const UString&>&
                                    pointed_variable_value_signal () const  ;

    sigc::signal<void, const UString&, const UString&, const UString&>&
                                        variable_type_signal () const ;

    sigc::signal<void, const VariableSafePtr&, const UString&>&
                                    variable_type_set_signal () const ;

    sigc::signal<void, const VariableSafePtr, const UString&>
                                      variable_dereferenced_signal () const ;

    sigc::signal<void, int, const UString&>& got_target_info_signal () const  ;

    sigc::signal<void>& running_signal () const ;

    sigc::signal<void, const UString&, const UString&>&
                                        signal_received_signal () const ;

    sigc::signal<void, const UString&>& error_signal () const ;

    sigc::signal<void>& program_finished_signal () const ;

    sigc::signal<void, IDebugger::State>& state_changed_signal () const;

    sigc::signal<void, const UString&, const UString&, const UString& >&
                                         register_value_changed_signal () const;

    sigc::signal<void, const std::map<register_id_t, UString>&, const UString& >&
                                         register_names_listed_signal () const;

    sigc::signal<void, const std::list<register_id_t>&, const UString& >&
                                         changed_registers_listed_signal () const;

    sigc::signal<void, const std::map<register_id_t, UString>&, const UString& >&
                                                 register_values_listed_signal () const;
    sigc::signal <void, size_t, const std::vector<uint8_t>&, const UString& >&
                                                    read_memory_signal () const;
    sigc::signal <void, size_t, const std::vector<uint8_t>&, const UString& >&
                                                      set_memory_signal () const;
    //*************
    //</signals>
    //*************

    //***************
    //<signal handlers>
    //***************

    void on_debugger_stdout_signal (CommandAndOutput &a_cao) ;
    void on_got_target_info_signal (int a_pid, const UString& a_exe_path) ;
    void on_stopped_signal (IDebugger::StopReason a_reason,
                            bool has_frame,
                            const IDebugger::Frame &a_frame,
                            int a_thread_id,
                            const UString &a_cookie);
    void on_detached_from_target_signal ();

    void on_program_finished_signal ();
    //***************
    //</signal handlers>
    //***************

    //for internal use
    void init () ;

    // to be called by client code
    void do_init (IConfMgrSafePtr &a_conf_mgr) ;

    IConfMgr& get_conf_mgr () ;

    map<UString, UString>& properties () ;
    void set_event_loop_context (const Glib::RefPtr<Glib::MainContext> &) ;
    void run_loop_iterations (int a_nb_iters) ;
    void set_state (IDebugger::State a_state) ;
    bool stop_target ()  ;
    void exit_engine () ;
    void execute_command (const Command &a_command) ;
    bool queue_command (const Command &a_command) ;
    bool busy () const ;
    void set_debugger_full_path (const UString &a_full_path) ;
    const UString& get_debugger_full_path () const ;
    void set_debugger_parameter (const UString &a_name,
                                 const UString &a_value) ;
    void set_solib_prefix_path (const UString &a_name) ;
    void load_program (const UString &a_prog_with_args,
                       const UString &a_working_dir) ;

    void load_program (const vector<UString> &a_argv,
                       const UString &working_dir,
                       const vector<UString> &a_source_search_dirs,
                       const UString &a_tty_path) ;

    void load_core_file (const UString &a_prog_file,
                         const UString &a_core_path);

    bool attach_to_target (unsigned int a_pid,
                           const UString &a_tty_path) ;

    bool attach_to_remote_target (const UString &a_host, int a_port) ;

    bool attach_to_remote_target (const UString &a_serial_line) ;

    void detach_from_target (const UString &a_cookie="") ;

    bool is_attached_to_target () const;

    void set_attached_to_target (bool a_is_attached);

    void add_env_variables (const map<UString, UString> &a_vars) ;

    map<UString, UString>& get_env_variables ()  ;

    const UString& get_target_path () ;

    void init_output_handlers () ;

    void append_breakpoints_to_cache (const map<int, IDebugger::BreakPoint>&) ;

    void do_continue (const UString &a_cookie) ;

    void run (const UString &a_cookie)  ;

    void get_target_info (const UString &a_cookie) ;

    ILangTraitSafePtr create_language_trait () ;

    ILangTraitSafePtr get_language_trait () ;

    IDebugger::State get_state () const ;

    void step_in (const UString &a_cookie) ;

    void step_out (const UString &a_cookie) ;

    void step_over (const UString &a_cookie) ;

    void continue_to_position (const UString &a_path,
                               gint a_line_num,
                               const UString &a_cookie)  ;

    void set_breakpoint (const UString &a_path,
                         gint a_line_num,
                         const UString &a_cookie)  ;

    void list_breakpoints (const UString &a_cookie) ;

    map<int, IDebugger::BreakPoint>& get_cached_breakpoints () ;

    void set_breakpoint (const UString &a_func_name,
                         const UString &a_cookie)  ;

    void set_catch (const UString &a_event,
					const UString &a_cookie)  ;

    void enable_breakpoint (gint a_break_num,
                            const UString &a_cookie="");

    void disable_breakpoint (gint a_break_num,
                             const UString &a_cookie="");

    void delete_breakpoint (const UString &a_path,
                            gint a_line_num,
                            const UString &a_cookie) ;

    void choose_function_overload (int a_overload_number,
                                   const UString &a_cookie) ;

    void choose_function_overloads (const vector<int> &a_numbers,
                                    const UString &a_cookie) ;

    void list_threads (const UString &a_cookie) ;

    void select_thread (unsigned int a_thread_id,
                        const UString &a_cookie) ;

    void delete_breakpoint (gint a_break_num,
                            const UString &a_cookie) ;

    void select_frame (int a_frame_id,
                       const UString &a_cookie) ;

    void list_frames (const UString &a_cookie) ;

    void list_frames_arguments (int a_low_frame,
                                int a_high_frame,
                                const UString &a_cookie) ;

    void list_local_variables (const UString &a_cookie) ;

    void list_global_variables ( const UString &a_cookie ) ;

    void evaluate_expression (const UString &a_expr,
                              const UString &a_cookie) ;

    void print_variable_value (const UString &a_var_name,
                              const UString &a_cookie) ;

    void get_variable_value (const VariableSafePtr &a_var,
                             const UString &a_cookie) ;

    void print_pointed_variable_value (const UString &a_var_name,
                                       const UString &a_cookie) ;

    void print_variable_type (const UString &a_var_name,
                              const UString &a_cookie) ;

    void get_variable_type (const VariableSafePtr &a_var,
                            const UString &a_cookie) ;

    bool dereference_variable (const VariableSafePtr &a_var,
                               const UString &a_cookie) ;

    void list_files (const UString &a_cookie);

    bool extract_proc_info (Output &a_output,
                            int &a_proc_pid,
                            UString &a_exe_path) ;

    typedef std::map<UString, std::list<IDebugger::VariableSafePtr> > VarsPerFilesMap ;
    bool extract_global_variable_list (Output &a_output,
                                       VarsPerFilesMap &a_vars) ;

    void list_register_names (const UString &a_cookie="");

    void list_register_values (std::list<register_id_t> a_registers,
                               const UString &a_cookie="");

    virtual void set_register_value (const UString& a_reg_name,
                                     const UString& a_value,
                                     const UString& a_cookie);

    void list_changed_registers (const UString &a_cookie="");

    void list_register_values (const UString &a_cookie="");

    void read_memory (size_t a_start_addr,
                      size_t a_num_bytes,
                      const UString& a_cookie="") ;
    void set_memory (size_t a_addr,
                     const std::vector<uint8_t>& a_bytes,
                     const UString& a_cookie="");
};//end class GDBEngine

NEMIVER_END_NAMESPACE (nemiver)

#endif //__NMV_GDB_ENGINE_H__

