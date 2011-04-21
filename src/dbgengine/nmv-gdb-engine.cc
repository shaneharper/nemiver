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
#include <cstring>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <sstream>
#include <boost/variant.hpp>
#include <algorithm>
#include <memory>
#include <fstream>
#include <iostream>
#include "nmv-i-debugger.h"
#include "common/nmv-env.h"
#include "common/nmv-exception.h"
#include "common/nmv-sequence.h"
#include "common/nmv-proc-utils.h"
#include "common/nmv-str-utils.h"
#include "nmv-gdb-engine.h"
#include "langs/nmv-cpp-parser.h"
#include "langs/nmv-cpp-ast-utils.h"
#include "nmv-i-lang-trait.h"
#include "nmv-debugger-utils.h"

using namespace std;
using namespace nemiver::common;
using namespace nemiver::cpp;

static const UString GDBMI_OUTPUT_DOMAIN = "gdbmi-output-domain";
static const UString DEFAULT_GDB_BINARY = "default-gdb-binary";

using nemiver::debugger_utils::null_const_variable_slot;
using nemiver::debugger_utils::null_const_variable_list_slot;
using nemiver::debugger_utils::null_frame_vector_slot;
using nemiver::debugger_utils::null_frame_args_slot;
using nemiver::debugger_utils::null_disass_slot;
using nemiver::debugger_utils::null_breakpoints_slot;

NEMIVER_BEGIN_NAMESPACE (nemiver)

extern const char* CONF_KEY_GDB_BINARY;
extern const char* CONF_KEY_FOLLOW_FORK_MODE;

// Helper function to handle escaping the arguments 
static UString
quote_args (const vector<UString> &a_prog_args)
{
    UString args;
    if (!a_prog_args.empty ()) {
        for (vector<UString>::size_type i = 0;
             i < a_prog_args.size ();
             ++i) {
            if (!a_prog_args[i].empty ())
                args += Glib::shell_quote (a_prog_args[i].raw ()) + " ";
        }
    }
    return args;
}

//**************************************************************
// <Helper functions to generate a serialized form of location>
//**************************************************************

/// Generate a location string from a an instance of source location
/// type.
///
/// \param a_loc the source input location
///
/// \param a_str the resulting location string
static void
location_to_string (const SourceLoc &a_loc,
                    UString &a_str)
{
    a_str =
        a_loc.file_path ()
        + ":" + UString::from_int (a_loc.line_number ());
}

/// Generate a location string from a an instance of address location
/// type.
///
/// \param a_loc the input address location
///
/// \param a_str the resulting location string
static void
location_to_string (const AddressLoc &a_loc,
                    UString &a_str)
{
    a_str = "*" + a_loc.address ().to_string ();
}

/// Generate a location string from a an instance of function location
/// type.
///
/// \param a_loc the input function location
///
/// \param a_str the resulting location string
static void
location_to_string (const FunctionLoc &a_loc,
                    UString &a_str)
{
    a_str = a_loc.function_name ();
}

/// Generate a location string from a generic instance of location
/// type.
///
/// \param a_loc the input location
///
/// \param a_str the resulting location string
static void
location_to_string (const Loc &a_loc,
                    UString &a_str)
{
    switch (a_loc.kind ()) {
    case Loc::UNDEFINED_LOC_KIND:
        THROW ("Should not be reached");

    case Loc::SOURCE_LOC_KIND: {
        const SourceLoc *loc = static_cast<const SourceLoc*> (&a_loc);
        location_to_string (*loc, a_str);
    }
        break;

    case Loc::FUNCTION_LOC_KIND: {
        const FunctionLoc *loc = static_cast<const FunctionLoc*> (&a_loc);
        location_to_string (*loc, a_str);
    }
        break;

    case Loc::ADDRESS_LOC_KIND: {
        const AddressLoc *loc = static_cast<const AddressLoc*> (&a_loc);
        location_to_string (*loc, a_str);
    }
        break;
    }
}

//**************************************************************
// </Helper functions to generate a serialized form of location>
//**************************************************************

//*************************
//<GDBEngine::Priv struct>
//*************************

class GDBEngine::Priv {
    Priv () {}
    Priv (const Priv&);
    Priv& operator= (const Priv&);

public:
    //***********************
    //<GDBEngine attributes>
    //************************
    DynamicModule *dynmod;
    map<UString, UString> properties;
    mutable IConfMgrSafePtr conf_mgr;
    UString cwd;
    vector<UString> argv;
    vector<UString> source_search_dirs;
    map<UString, UString> env_variables;
    UString exe_path;
    Glib::Pid gdb_pid;
    Glib::Pid target_pid;
    int gdb_stdout_fd;
    int gdb_stderr_fd;
    int master_pty_fd;
    bool is_attached;
    Glib::RefPtr<Glib::IOChannel> gdb_stdout_channel;
    Glib::RefPtr<Glib::IOChannel> gdb_stderr_channel;
    Glib::RefPtr<Glib::IOChannel> master_pty_channel;
    std::string gdb_stdout_buffer;
    std::string gdb_stderr_buffer;
    list<Command> queued_commands;
    list<Command> started_commands;
    bool line_busy;
    map<int, IDebugger::Breakpoint> cached_breakpoints;
    enum InBufferStatus {
        DEFAULT,
        FILLING,
        FILLED
    };
    InBufferStatus error_buffer_status;
    Glib::RefPtr<Glib::MainContext> loop_context;

    OutputHandlerList output_handler_list;
    IDebugger::State state;
    int cur_frame_level;
    int cur_thread_num;
    Address cur_frame_address;
    ILangTraitSafePtr lang_trait;
    UString debugger_full_path;
    UString follow_fork_mode;
    GDBMIParser gdbmi_parser;
    sigc::signal<void> gdb_died_signal;
    sigc::signal<void, const UString& > master_pty_signal;
    sigc::signal<void, const UString& > gdb_stdout_signal;
    sigc::signal<void, const UString& > gdb_stderr_signal;

    mutable sigc::signal<void, Output&> pty_signal;

    mutable sigc::signal<void, CommandAndOutput&> stdout_signal;

    mutable sigc::signal<void, Output&> stderr_signal;

    mutable sigc::signal<void, const UString&> console_message_signal;
    mutable sigc::signal<void, const UString&> target_output_message_signal;

    mutable sigc::signal<void, const UString&> log_message_signal;

    mutable sigc::signal<void, const UString&, const UString&>
                                                        command_done_signal;

    mutable sigc::signal<void> connected_to_server_signal;

    mutable sigc::signal<void> detached_from_target_signal;

    mutable sigc::signal<void,
                         const map<int, IDebugger::Breakpoint>&,
                         const UString&> breakpoints_list_signal;

    mutable sigc::signal<void,
                         const std::pair<int, const IDebugger::Breakpoint&>&,
                         const UString& /*cookie*/> breakpoint_set_signal;

    mutable sigc::signal<void,
                         const vector<OverloadsChoiceEntry>&,
                         const UString&> got_overloads_choice_signal;

    mutable sigc::signal<void,
                         const IDebugger::Breakpoint&,
                         int,
                         const UString&> breakpoint_deleted_signal;

    mutable sigc::signal<void, const IDebugger::Breakpoint&, int>
                                                breakpoint_disabled_signal;

    mutable sigc::signal<void, const IDebugger::Breakpoint&, int>
                                                breakpoint_enabled_signal;

    mutable sigc::signal<void, IDebugger::StopReason,
                         bool, const IDebugger::Frame&,
                         int, int, const UString&> stopped_signal;

    mutable sigc::signal<void,
                         const list<int>,
                         const UString& > threads_listed_signal;

    mutable sigc::signal<void,
                         const vector<UString>&,
                         const UString& > files_listed_signal;

    mutable sigc::signal<void,
                         int,
                         const Frame * const,
                         const UString&> thread_selected_signal;

    mutable sigc::signal<void, const vector<IDebugger::Frame>&, const UString&>
                                                    frames_listed_signal;

    mutable sigc::signal<void,
                         const map<int, list<IDebugger::VariableSafePtr> >&,
                         const UString&> frames_arguments_listed_signal;

    mutable sigc::signal<void, const IDebugger::Frame&, const UString&>
                                                        current_frame_signal;

    mutable sigc::signal<void, const list<VariableSafePtr>&, const UString& >
                                    local_variables_listed_signal;

    mutable sigc::signal<void, const list<VariableSafePtr>&, const UString& >
                                    global_variables_listed_signal;

    mutable sigc::signal<void,
                         const UString&,
                         const IDebugger::VariableSafePtr,
                         const UString&>             variable_value_signal;

    mutable sigc::signal<void,
                         const VariableSafePtr/*variable*/,
                         const UString& /*cookie*/>
                                     variable_value_set_signal;

    mutable sigc::signal<void,
                         const UString&,
                         const VariableSafePtr,
                         const UString&> pointed_variable_value_signal;

    mutable sigc::signal<void, const UString&, const UString&, const UString&>
                                                        variable_type_signal;

    mutable sigc::signal<void, const VariableSafePtr, const UString&>
                                                    variable_type_set_signal;

    mutable sigc::signal<void, const VariableSafePtr, const UString&>
                                      variable_dereferenced_signal;

    mutable sigc::signal<void, int, const UString&> got_target_info_signal;

    mutable sigc::signal<void> running_signal;

    mutable sigc::signal<void, const UString&, const UString&>
                                                        signal_received_signal;
    mutable sigc::signal<void, const UString&> error_signal;

    mutable sigc::signal<void> program_finished_signal;

    mutable sigc::signal<void, IDebugger::State> state_changed_signal;

    mutable sigc::signal<void,
                         const std::map<register_id_t,
                         UString>&, const UString& >
                                                register_names_listed_signal;

    mutable sigc::signal<void,
                        const std::list<register_id_t>&,
                        const UString& >
                                     changed_registers_listed_signal;

    mutable sigc::signal<void,
                         const std::map<register_id_t, UString>&,
                         const UString& >
                                    register_values_listed_signal;

    mutable sigc::signal<void,
                         const UString&,
                         const UString&,
                         const UString&>
                                   register_value_changed_signal;

    mutable sigc::signal <void,
                          size_t,
                          const std::vector<uint8_t>&,
                          const UString&>
                                  read_memory_signal;

    mutable sigc::signal <void,
                          size_t,
                          const std::vector<uint8_t>&,
                          const UString& >
                                set_memory_signal;

    mutable sigc::signal<void,
                 const common::DisassembleInfo&,
                 const std::list<common::Asm>&,
                 const UString& /*cookie*/> instructions_disassembled_signal;

    mutable sigc::signal<void, const VariableSafePtr, const UString&>
                                                        variable_created_signal;

    mutable sigc::signal<void, const VariableSafePtr, const UString&>
                                                        variable_deleted_signal;

    mutable sigc::signal<void, const VariableSafePtr, const UString&>
                                                        variable_unfolded_signal;

    mutable sigc::signal<void, const VariableSafePtr, const UString&>
                                        variable_expression_evaluated_signal;

    mutable sigc::signal<void, const list<VariableSafePtr>&, const UString&>
                                                changed_variables_signal;

    mutable sigc::signal<void, VariableSafePtr, const UString&>
                                                assigned_variable_signal;

    //***********************
    //</GDBEngine attributes>
    //************************

    IConfMgrSafePtr get_conf_mgr () const
    {
        THROW_IF_FAIL (conf_mgr);
        return conf_mgr;
    }

    const UString& get_debugger_full_path () const
    {
        NEMIVER_TRY
        get_conf_mgr ()->get_key_value (CONF_KEY_GDB_BINARY,
                                        const_cast<Priv*>
                                                (this)->debugger_full_path);
        NEMIVER_CATCH_NOX

        if (debugger_full_path == "" ||
            debugger_full_path == DEFAULT_GDB_BINARY) {
            const_cast<Priv*> (this)->debugger_full_path =
                                                    env::get_gdb_program ();
        }
        LOG_DD ("debugger: '" << debugger_full_path << "'");
        return debugger_full_path;
    }

    void set_event_loop_context (const Glib::RefPtr<Glib::MainContext> &a_ctxt)
    {
        loop_context = a_ctxt;
    }

    void run_loop_iterations_real (int a_nb_iters)
    {
        if (!a_nb_iters) return;

        if (a_nb_iters < 0) {
            while (get_event_loop_context ()->pending ()) {
                get_event_loop_context ()->iteration (false);
            }
        } else {
            while (a_nb_iters--) {
                get_event_loop_context ()->iteration (false);
            }
        }
    }

    Glib::RefPtr<Glib::MainContext>& get_event_loop_context ()
    {
        if (!loop_context) {
            loop_context = Glib::MainContext::get_default ();
        }
        THROW_IF_FAIL (loop_context);
        return loop_context;
    }

    void on_master_pty_signal (const UString &a_buf)
    {
        LOG_D ("<debuggerpty>\n" << a_buf << "\n</debuggerpty>",
               GDBMI_OUTPUT_DOMAIN);
        Output result (a_buf);
        pty_signal.emit (result);
    }

    void on_gdb_stderr_signal (const UString &a_buf)
    {
        LOG_D ("<debuggerstderr>\n" << a_buf << "\n</debuggerstderr>",
               GDBMI_OUTPUT_DOMAIN);
        Output result (a_buf);
        stderr_signal.emit (result);
    }

    void on_gdb_stdout_signal (const UString &a_buf)
    {
        LOG_D ("<debuggeroutput>\n" << a_buf << "\n</debuggeroutput>",
               GDBMI_OUTPUT_DOMAIN);

        Output output (a_buf);

        UString::size_type from (0), to (0), end (a_buf.size ());
        gdbmi_parser.push_input (a_buf);
        for (; from < end;) {
            if (!gdbmi_parser.parse_output_record (from, to, output)) {
                LOG_ERROR ("output record parsing failed: "
                        << a_buf.substr (from, end - from)
                        << "\npart of buf: " << a_buf
                        << "\nfrom: " << (int) from
                        << "\nto: " << (int) to << "\n"
                        << "\nstrlen: " << (int) a_buf.size ());
                gdbmi_parser.skip_output_record (from, to);
                output.parsing_succeeded (false);
            } else {
                output.parsing_succeeded (true);
            }


            // Check if the output contains the result to a command issued by
            // the user. If yes, build the CommandAndResult, update the
            // command queue and notify the user that the command it issued
            // has a result.

            UString output_value;
            output_value.assign (a_buf, from, to - from +1);
            output.raw_value (output_value);
            CommandAndOutput command_and_output;
            if (output.has_result_record ()) {
                if (!started_commands.empty ()) {
                    command_and_output.command (*started_commands.begin ());
                }
            }
            command_and_output.output (output);
            LOG_DD ("received command was: '"
                    << command_and_output.command ().name ()
                    << "'");
            stdout_signal.emit (command_and_output);
            from = to;
            while (from < end && isspace (a_buf.raw ()[from])) {++from;}
            if (output.has_result_record ()/*gdb acknowledged previous
                                             cmd*/
                || !output.parsing_succeeded ()) {
                LOG_DD ("here");
                if (!started_commands.empty ()) {
                    started_commands.erase (started_commands.begin ());
                    LOG_DD ("clearing the line");
                    // we can send another cmd down the wire
                    line_busy = false;
                }
                if (!line_busy
                    && started_commands.empty ()
                    && !queued_commands.empty ()) {
                    issue_command (*queued_commands.begin ());
                    queued_commands.erase (queued_commands.begin ());
                }
            }
        }
        gdbmi_parser.pop_input ();
    }

    Priv (DynamicModule *a_dynmod) :
        dynmod (a_dynmod), cwd ("."),
        gdb_pid (0), target_pid (0),
        gdb_stdout_fd (0), gdb_stderr_fd (0),
        master_pty_fd (0),
        is_attached (false),
        line_busy (false),
        error_buffer_status (DEFAULT),
        state (IDebugger::NOT_STARTED),
        cur_frame_level (0),
        cur_thread_num (1),
        follow_fork_mode ("parent"),
        gdbmi_parser (GDBMIParser::BROKEN_MODE)
    {
        gdb_stdout_signal.connect (sigc::mem_fun
                (*this, &Priv::on_gdb_stdout_signal));
        master_pty_signal.connect (sigc::mem_fun
                (*this, &Priv::on_master_pty_signal));
        gdb_stderr_signal.connect (sigc::mem_fun
                (*this, &Priv::on_gdb_stderr_signal));

        state_changed_signal.connect (sigc::mem_fun
                (*this, &Priv::on_state_changed_signal));

        thread_selected_signal.connect (sigc::mem_fun
               (*this, &Priv::on_thread_selected_signal));

        stopped_signal.connect (sigc::mem_fun
               (*this, &Priv::on_stopped_signal));

        frames_listed_signal.connect (sigc::mem_fun
               (*this, &Priv::on_frames_listed_signal));
    }

    void free_resources ()
    {
        if (gdb_pid) {
            g_spawn_close_pid (gdb_pid);
            gdb_pid = 0;
        }
        if (gdb_stdout_channel) {
            gdb_stdout_channel->close ();
            gdb_stdout_channel.clear ();
        }
        if (master_pty_channel) {
            master_pty_channel->close ();
            master_pty_channel.clear ();
        }
        if (gdb_stderr_channel) {
            gdb_stderr_channel->close ();
            gdb_stderr_channel.clear ();
        }
    }

    void on_child_died_signal (Glib::Pid a_pid, int a_priority)
    {
        if (a_pid) {}
        if (a_priority) {}
        gdb_died_signal.emit ();
        free_resources ();
    }

    void set_state (IDebugger::State a_state)
    {
        //okay, changing state "too much"
        //can cause serious performance problems
        //to client code because some clients
        //have to update their user interface at
        //each state change.
        //So we try to minimize the number of useless
        //signal emission here.

        //If the command queue is _NOT_ empty
        //and we get a request to set the state
        //to IDebugger::READY, it is very likely
        //that the state quickly switches to IDebugger::RUNNING
        //right after. Yeah, the next command in the queue will somehow
        //have to be issued to the underlying debugger, leading to
        //the state being switched to IDebugger::RUNNING
        if (a_state == IDebugger::READY &&
            !queued_commands.empty ()) {
            return;
        }

        //don't emit any signal if a_state equals the
        //current state.
        if (state == a_state) {
            return;
        }

        //if we reach this line, just emit the signal.
        state_changed_signal.emit (a_state);
    }

    bool is_gdb_running ()
    {
        if (gdb_pid) {return true;}
        return false;
    }

    void kill_gdb ()
    {
        if (is_gdb_running ()) {
            kill (gdb_pid, SIGKILL);
        }
        free_resources();
    }


    void set_communication_charset (const string &a_charset)
    {
        if (a_charset.empty ()) {
            gdb_stdout_channel->set_encoding ();
            gdb_stderr_channel->set_encoding ();
            master_pty_channel->set_encoding ();
        } else {
            gdb_stdout_channel->set_encoding (a_charset);
            gdb_stderr_channel->set_encoding (a_charset);
            master_pty_channel->set_encoding (a_charset);
        }
    }

    bool launch_gdb_real (const vector<UString> a_argv)
    {
        RETURN_VAL_IF_FAIL (launch_program (a_argv,
                                            gdb_pid,
                                            master_pty_fd,
                                            gdb_stdout_fd,
                                            gdb_stderr_fd),
                            false);

        RETURN_VAL_IF_FAIL (gdb_pid, false);

        gdb_stdout_channel = Glib::IOChannel::create_from_fd (gdb_stdout_fd);
        THROW_IF_FAIL (gdb_stdout_channel);

        gdb_stderr_channel = Glib::IOChannel::create_from_fd (gdb_stderr_fd);
        THROW_IF_FAIL (gdb_stderr_channel);

        master_pty_channel = Glib::IOChannel::create_from_fd
                                                            (master_pty_fd);
        THROW_IF_FAIL (master_pty_channel);

        set_communication_charset (string ());


        attach_channel_to_loop_context_as_source
                                (Glib::IO_IN | Glib::IO_PRI
                                 | Glib::IO_HUP | Glib::IO_ERR,
                                 sigc::mem_fun
                                     (this,
                                      &Priv::on_gdb_stderr_has_data_signal),
                                 gdb_stderr_channel,
                                 get_event_loop_context ());

        attach_channel_to_loop_context_as_source
                                (Glib::IO_IN | Glib::IO_PRI
                                 | Glib::IO_HUP | Glib::IO_ERR,
                                 sigc::mem_fun
                                     (this,
                                      &Priv::on_gdb_stdout_has_data_signal),
                                 gdb_stdout_channel,
                                 get_event_loop_context ());

        return true;
    }

    bool find_prog_in_path (const UString &a_prog,
                            UString &a_prog_path)
    {
        const char *tmp = g_getenv ("PATH");
        if (!tmp) {return false;}
        vector<UString> path_dirs =
            UString (Glib::filename_to_utf8 (tmp)).split (":");
        path_dirs.insert (path_dirs.begin (), ("."));
        vector<UString>::const_iterator it;
        string file_path;
        for (it = path_dirs.begin (); it != path_dirs.end (); ++it) {
            file_path = Glib::build_filename (Glib::filename_from_utf8 (*it),
                                              Glib::filename_from_utf8 (a_prog));
            if (Glib::file_test (file_path,
                                 Glib::FILE_TEST_IS_REGULAR)) {
                a_prog_path = Glib::filename_to_utf8 (file_path);
                return true;
            }
        }
        return false;
    }

    bool launch_gdb (const UString &working_dir,
                     const vector<UString> &a_source_search_dirs,
                     const UString &a_prog,
                     const vector<UString> &a_gdb_options)
    {
        if (is_gdb_running ()) {
            kill_gdb ();
        }
        argv.clear ();

        UString prog_path;
        if (!a_prog.empty ()) {
            prog_path = a_prog;
            if (!Glib::file_test (Glib::filename_from_utf8 (prog_path),
                                  Glib::FILE_TEST_IS_REGULAR)) {
                // So we haven't found the file. Let's look for it in
                // the current working directory and in the PATH.
                bool found = false;
                if (!working_dir.empty ()) {
                    list<UString> where;
                    where.push_back (working_dir);
                    if (common::env::find_file (prog_path, where, prog_path))
                        found = true;
                }

                if (!found && !find_prog_in_path (prog_path, prog_path)) {
                    LOG_ERROR ("Could not find program '" << prog_path << "'");
                    return false;
                }
            }
        }
        // if the executable program to be debugged is a
        // libtool wrapper script,
        // run the debugging session under libtool
        if (is_libtool_executable_wrapper (prog_path)) {
            argv.push_back ("libtool");
            argv.push_back ("--mode=execute");
        }

        THROW_IF_FAIL (get_debugger_full_path () != "");
        argv.push_back (get_debugger_full_path ());
        if (working_dir != "") {
            argv.push_back ("--cd=" + working_dir);
        }
        argv.push_back ("--interpreter=mi2");
        if (!a_gdb_options.empty ()) {
            for (vector<UString>::const_iterator it = a_gdb_options.begin ();
                 it != a_gdb_options.end ();
                 ++it) {
                argv.push_back (*it);
            }
        }
        argv.push_back (prog_path);

        source_search_dirs = a_source_search_dirs;
        return launch_gdb_real (argv);
    }

    bool launch_gdb_and_set_args (const UString &working_dir,
                                  const vector<UString> &a_src_search_dirs,
                                  const UString &a_prog,
                                  const vector<UString> &a_prog_args,
                                  const vector<UString> a_gdb_options)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        bool result (false);
        result = launch_gdb (working_dir, a_src_search_dirs,
                             a_prog, a_gdb_options);

        LOG_DD ("workingdir:" << working_dir
                << "\nsearchpath: " << UString::join (a_src_search_dirs)
                << "\nprog: " << a_prog
                << "\nprogargs: " << UString::join (a_prog_args, " ")
                << "\ngdboptions: " << UString::join (a_gdb_options));

        if (!result) {return false;}
        UString args = quote_args (a_prog_args);
        if (!args.empty ())
	  issue_command (Command ("set args " + args));
        set_debugger_parameter ("follow-fork-mode", follow_fork_mode);

        return true;
    }

    bool launch_gdb_on_core_file (const UString &a_prog_path,
                                  const UString &a_core_path)
    {
        vector<UString> argv;

        // if the executable program to be debugged is a libtool wrapper script,
        // run the debugging session under libtool
        if (is_libtool_executable_wrapper (a_prog_path)) {
            argv.push_back ("libtool");
            argv.push_back ("--mode=execute");
        }

        argv.push_back (env::get_gdb_program ());
        argv.push_back ("--interpreter=mi2");
        argv.push_back (a_prog_path);
        argv.push_back (a_core_path);
        return launch_gdb_real (argv);
    }

    bool issue_command (const Command &a_command,
                        bool a_do_record = true)
    {
        if (!master_pty_channel) {
            return false;
        }

        LOG_DD ("issuing command: '" << a_command.value () << "': name: '"
                << a_command.name () << "'");

        if (master_pty_channel->write
                (a_command.value () + "\n") == Glib::IO_STATUS_NORMAL) {
            master_pty_channel->flush ();
            THROW_IF_FAIL (started_commands.size () <= 1);

            if (a_do_record)
                started_commands.push_back (a_command);

            //usually, when we send a command to the debugger,
            //it becomes busy (in a running state), untill it gets
            //back to us saying the converse.
            set_state (IDebugger::RUNNING);
            return true;
        }
        return false;
    }

    bool queue_command (const Command &a_command)
    {
        bool result (false);
        LOG_DD ("queuing command: '" << a_command.value () << "'");
        queued_commands.push_back (a_command);
        if (!line_busy && started_commands.empty ()) {
            result = issue_command (*queued_commands.begin (), true);
            queued_commands.erase (queued_commands.begin ());
        }
        return result;
    }

    /// Resets the GDB command queue so that it is in its initial
    /// state.  Just as is the GDBEngine object has just been
    /// instantiated.  This is useful when we are about to launch a
    /// new GDB just after the previous one crashed "during flight".
    /// Use this with great care.
    void
    reset_command_queue (void)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        started_commands.clear ();
        queued_commands.clear ();
        line_busy = false;
    }

    void set_debugger_parameter (const UString &a_name,
                                 const UString &a_value)
    {

        LOG_FUNCTION_SCOPE_NORMAL_DD;
        if (a_name == "")
            return;
        UString param_str = a_name + " " + a_value;
        queue_command (Command ("set-debugger-parameter", "set " + param_str));
    }

    void read_default_config ()
    {
        get_conf_mgr ()->get_key_value (CONF_KEY_FOLLOW_FORK_MODE,
                                        follow_fork_mode);
    }

    /// Lists the frames which numbers are in a given range.
    ///
    /// Upon completion of the GDB side of this command, the signal
    /// Priv::frames_listed_signal is emitted.
    ///
    /// \param a_low_frame the lower bound of the frame range to list.
    ///
    /// \param a_high_frame the upper bound of the frame range to list.
    ///
    /// \param a_cookie a string to be passed to the
    /// Priv::frames_listed_signal.
    void list_frames (int a_low_frame,
                      int a_high_frame,
                      const UString &a_cookie)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        list_frames (a_low_frame, a_high_frame,
                     &null_frame_vector_slot,
                     a_cookie);
    }

    /// A subroutine of the list_frame overload above.
    /// 
    /// Lists the frames which numbers are in a given range.
    ///
    /// Upon completion of the GDB side of this command, the signal
    /// Priv::frames_listed_signal is emitted.  The callback slot
    /// given in parameter is called as well.
    ///
    /// \param a_low_frame the lower bound of the frame range to list.
    ///
    /// \param a_high_frame the upper bound of the frame range to list.
    ///
    /// \param a_slot a callback slot to be called upon completion of
    /// the GDB-side command.
    ///
    /// \param a_cookie a string to be passed to the
    /// Priv::frames_listed_signal.
    void list_frames (int a_low_frame,
                      int a_high_frame,
                      const FrameVectorSlot &a_slot,
                      const UString &a_cookie)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        string low, high, stack_window, cmd_str;

        if (a_low_frame >= 0)
            low = UString::from_int (a_low_frame).raw ();
        if (a_high_frame >= 0)
            high = UString::from_int (a_high_frame).raw ();

        if (!low.empty () && !high.empty ())
            stack_window = low + " " + high;

        cmd_str = (stack_window.empty ())
                  ? "-stack-list-frames"
                  : "-stack-list-frames " + stack_window;
        
        Command command ("list-frames", cmd_str, a_cookie);
        command.set_slot (a_slot);
        queue_command (command);
    }

    void set_tty_path (const UString &a_tty_path)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        if (!a_tty_path.empty ())
            queue_command (Command ("set inferior-tty " + a_tty_path));
    }

    bool on_gdb_stdout_has_data_signal (Glib::IOCondition a_cond)
    {
        if (!gdb_stdout_channel) {
            LOG_ERROR_DD ("lost stdout channel");
            return false;
        }

        NEMIVER_TRY

        if ((a_cond & Glib::IO_IN) || (a_cond & Glib::IO_PRI)) {
            gsize nb_read (0), CHUNK_SIZE(10*1024);
            char buf[CHUNK_SIZE+1];
            Glib::IOStatus status (Glib::IO_STATUS_NORMAL);
            std::string meaningful_buffer;
            while (true) {
                memset (buf, 0, CHUNK_SIZE + 1);
                status = gdb_stdout_channel->read (buf, CHUNK_SIZE, nb_read);
                if (status == Glib::IO_STATUS_NORMAL &&
                    nb_read && (nb_read <= CHUNK_SIZE)) {
                    std::string raw_str (buf, nb_read);
                    gdb_stdout_buffer.append (raw_str);
                } else {
                    break;
                }
                nb_read = 0;
            }
            LOG_DD ("gdb_stdout_buffer: <buf>"
                    << gdb_stdout_buffer
                    << "</buf>");

            UString::size_type i = 0;
            while ((i = gdb_stdout_buffer.find ("\n(gdb)")) !=
                    std::string::npos) {
                i += 6;/*is the offset in the buffer of the end of
                         *of the '(gdb)' prompt
                         */
                int size = i+1;
                //basically, gdb can send more or less than a complete
                //output record. So let's take that in account in the way
                //we manage he incoming buffer.
                //TODO: optimize the way we handle this so that we do
                //less allocation and copying.
                meaningful_buffer = gdb_stdout_buffer.substr (0, size);
                str_utils::chomp (meaningful_buffer);
                meaningful_buffer += '\n';
                LOG_DD ("emiting gdb_stdout_signal () with '"
                        << meaningful_buffer << "'");
                gdb_stdout_signal.emit (meaningful_buffer);
                gdb_stdout_buffer.erase (0, size);
                while (!gdb_stdout_buffer.empty ()
                       && isspace (gdb_stdout_buffer[0])) {
                    gdb_stdout_buffer.erase (0,1);
                }
            }
            if (gdb_stdout_buffer.find ("[0] cancel")
                    != std::string::npos
                && gdb_stdout_buffer.find ("> ")
                    != std::string::npos) {
                // this is not a gdbmi ouptut, but rather a plain gdb
                // command line. It is actually a prompt sent by gdb
                // to let the user choose between a list of
                // overloaded functions
                LOG_DD ("emitting gdb_stdout_signal.emit()");
                gdb_stdout_signal.emit (gdb_stdout_buffer);
                gdb_stdout_buffer.clear ();
            }
        }
        if (a_cond & Glib::IO_HUP) {
            LOG_ERROR ("Connection lost from stdout channel to gdb");
            gdb_stdout_channel.clear ();
            kill_gdb ();
            gdb_died_signal.emit ();
            LOG_ERROR ("GDB killed");
        }
        if (a_cond & Glib::IO_ERR) {
            LOG_ERROR ("Error over the wire");
        }

        NEMIVER_CATCH_NOX

        return true;
    }

    bool on_gdb_stderr_has_data_signal (Glib::IOCondition a_cond)
    {
       if (!gdb_stderr_channel) {
           LOG_ERROR_DD ("lost stderr channel");
           return false;
       }

        try {

            if (a_cond & Glib::IO_IN || a_cond & Glib::IO_PRI) {
                char buf[513] = {0};
                gsize nb_read (0), CHUNK_SIZE(512);
                Glib::IOStatus status (Glib::IO_STATUS_NORMAL);
                bool got_data (false);
                while (true) {
                    status = gdb_stderr_channel->read (buf,
                            CHUNK_SIZE,
                            nb_read);
                    if (status == Glib::IO_STATUS_NORMAL
                            && nb_read && (nb_read <= CHUNK_SIZE)) {
                        if (error_buffer_status == FILLED) {
                            gdb_stderr_buffer.clear ();
                            error_buffer_status = FILLING;
                        }
                        std::string raw_str(buf, nb_read);
                        UString tmp = Glib::locale_to_utf8 (raw_str);
                        gdb_stderr_buffer.append (tmp);
                        got_data = true;

                    } else {
                        break;
                    }
                    nb_read = 0;
                }
                if (got_data) {
                    error_buffer_status = FILLED;
                    gdb_stderr_signal.emit (gdb_stderr_buffer);
                    gdb_stderr_buffer.clear ();
                }
            }

            if (a_cond & Glib::IO_HUP) {
                gdb_stderr_channel.clear ();
                kill_gdb ();
                gdb_died_signal.emit ();
            }
        } catch (exception e) {
        } catch (Glib::Error &e) {
        }
        return true;
    }

    void on_state_changed_signal (IDebugger::State a_state)
    {
        state = a_state;
    }

    void on_thread_selected_signal (unsigned a_thread_id,
                                    const IDebugger::Frame * const a_frame,
                                    const UString &/*a_cookie*/)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        NEMIVER_TRY

        cur_thread_num = a_thread_id;
        if (a_frame)
            cur_frame_level = a_frame->level ();

        NEMIVER_CATCH_NOX
    }

    void on_stopped_signal (IDebugger::StopReason,
                            bool a_has_frame,
                            const IDebugger::Frame &,
                            int,
                            int,
                            const UString &a_cookie)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        NEMIVER_TRY

        if (a_has_frame)
            // List frames so that we can get the @ of the current frame.
            list_frames (0, 0, a_cookie);

        NEMIVER_CATCH_NOX
    }

    void on_frames_listed_signal (const vector<IDebugger::Frame> &a_frames,
                                  const UString &)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        NEMIVER_TRY

        if (!a_frames.empty () && a_frames[0].level () == 0)
            cur_frame_address = a_frames[0].address ();

        NEMIVER_CATCH_NOX
    }

    void on_conf_key_changed_signal (const UString &a_key,
                                     const UString &a_namespace)
    {
        NEMIVER_TRY

        if (a_key == CONF_KEY_FOLLOW_FORK_MODE
                && conf_mgr->get_key_value (a_key, follow_fork_mode, a_namespace)) {
            set_debugger_parameter ("follow-fork-mode", follow_fork_mode);
        }

        NEMIVER_CATCH_NOX
    }

    bool breakpoint_has_failed (const CommandAndOutput &a_in)
    {
        if (a_in.has_command ()
                && a_in.command ().value ().compare (0, 5, "break")) {
            return false;
        }
        if (a_in.output ().has_result_record ()
                && a_in.output ().result_record ().breakpoints ().empty ()) {
            return true;
        }
        return false;
    }

    ~Priv ()
    {
        kill_gdb ();
    }
};//end GDBEngine::Priv

//*************************
//</GDBEngine::Priv struct>
//*************************

//****************************
//<GDBengine output handlers
//***************************


struct OnStreamRecordHandler: OutputHandler {
    GDBEngine *m_engine;

    OnStreamRecordHandler (GDBEngine *a_engine) :
        m_engine (a_engine)
    {}

    bool can_handle (CommandAndOutput &a_in)
    {
        if (!a_in.output ().has_out_of_band_record ()) {
            return false;
        }
        LOG_DD ("handler selected");
        return true;
    }

    void do_handle (CommandAndOutput &a_in)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (m_engine);

        list<Output::OutOfBandRecord>::const_iterator iter;
        UString debugger_console, target_output, debugger_log;

        for (iter = a_in.output ().out_of_band_records ().begin ();
             iter != a_in.output ().out_of_band_records ().end ();
             ++iter) {
            if (iter->has_stream_record ()) {
                if (iter->stream_record ().debugger_console () != ""){
                    debugger_console +=
                        iter->stream_record ().debugger_console ();
                }
                if (iter->stream_record ().target_output () != ""){
                    target_output += iter->stream_record ().target_output ();
                }
                if (iter->stream_record ().debugger_log () != ""){
                    debugger_log += iter->stream_record ().debugger_log ();
                }
            }
        }

        if (!debugger_console.empty ()) {
            m_engine->console_message_signal ().emit (debugger_console);
        }

        if (!target_output.empty ()) {
            m_engine->target_output_message_signal ().emit (target_output);
        }

        if (!debugger_log.empty ()) {
            m_engine->log_message_signal ().emit (debugger_log);
        }
    }
};//end struct OnStreamRecordHandler

struct OnDetachHandler : OutputHandler {
    GDBEngine * m_engine;

    OnDetachHandler (GDBEngine *a_engine = 0) :
        m_engine (a_engine)
    {
    }

    bool can_handle (CommandAndOutput &a_in)
    {
        if (a_in.output ().has_result_record ()
            && a_in.output ().result_record ().kind ()
                    == Output::ResultRecord::DONE
            && a_in.command ().name () == "detach-from-target") {
            LOG_DD ("handler selected");
            return true;
        }
        return false;
    }

    void do_handle (CommandAndOutput &a_in)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        if (a_in.command ().name () == "") {
        }
        THROW_IF_FAIL (m_engine);
        m_engine->detached_from_target_signal ().emit ();
        m_engine->set_state (IDebugger::NOT_STARTED);
    }
};//end OnDetachHandler

struct OnBreakpointHandler: OutputHandler {
    GDBEngine * m_engine;
    vector<UString>m_prompt_choices;

    OnBreakpointHandler (GDBEngine *a_engine = 0) :
        m_engine (a_engine)
    {
    }

    bool has_overloads_prompt (CommandAndOutput &a_in)
    {
        if (a_in.output ().has_out_of_band_record ()) {
            list<Output::OutOfBandRecord>::const_iterator it;
            for (it = a_in.output ().out_of_band_records ().begin ();
                 it != a_in.output ().out_of_band_records ().end ();
                 ++it) {
                if (it->has_stream_record ()
                    && !it->stream_record ().debugger_console ().empty ()
                    && !it->stream_record ().debugger_console ().compare
                            (0, 10, "[0] cancel")) {
                    return true;
                }
            }
        }
        return false;
    }


    bool extract_overloads_choice_prompt_values
                        (CommandAndOutput &a_in,
                         vector<IDebugger::OverloadsChoiceEntry> &a_prompts)
    {
        UString input;
        UString::size_type cur = 0;
        vector<IDebugger::OverloadsChoiceEntry> prompts;
        list<Output::OutOfBandRecord>::const_iterator it;
        for (it = a_in.output ().out_of_band_records ().begin ();
             it != a_in.output ().out_of_band_records ().end ();
             ++it) {
            if (it->has_stream_record ()
                && !it->stream_record ().debugger_console ().empty ()
                && !it->stream_record ().debugger_console ().compare
                        (0, 1, "[")) {
                input += it->stream_record ().debugger_console ();
            }
        }
        LOG_DD ("going to parse overloads: >>>" << input << "<<<");
        GDBMIParser gdbmi_parser (input, GDBMIParser::BROKEN_MODE);
        gdbmi_parser.push_input (input);
        return gdbmi_parser.parse_overloads_choice_prompt (cur, cur, a_prompts);
    }

    bool has_breakpoints_set (CommandAndOutput &a_in)
    {
        if (a_in.output ().has_result_record ()
            && a_in.output ().result_record ().breakpoints ().size ()) {
            return true;
        }
        return false;
    }

    bool can_handle (CommandAndOutput &a_in)
    {
        if (!a_in.output ().has_result_record ()
            && !has_overloads_prompt (a_in)) {
            return false;
        }
        LOG_DD ("handler selected");
        return true;
    }

    void do_handle (CommandAndOutput &a_in)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (m_engine);

        //first check if gdb asks us to
        //choose between several overloaded
        //functions. I call this a prompt (proposed by gdb)
        //note: maybe I should put this into another output handler
        if (has_overloads_prompt (a_in)) {
            //try to extract the choices out of
            //the prompt
            LOG_DD ("got overloads prompt");
            vector<IDebugger::OverloadsChoiceEntry> prompts;
            if (extract_overloads_choice_prompt_values (a_in, prompts)
                && !prompts.empty ()) {
                LOG_DD ("firing got_overloads_choice_signal () ");
                m_engine->got_overloads_choice_signal ().emit
                                    (prompts, a_in.command ().cookie ());
            } else {
                LOG_ERROR ("failed to parse overloads choice prompt");
            }
            m_engine->set_state (IDebugger::READY);
            return;
        }

        bool has_breaks = false;
        //if breakpoint where set, put them in cache !
        if (has_breakpoints_set (a_in)) {
            LOG_DD ("adding BPs to cache");
            m_engine->append_breakpoints_to_cache
                (a_in.output ().result_record ().breakpoints ());
            if (a_in.command ().name () == "set-countpoint") {
                THROW_IF_FAIL (a_in.output ().result_record ().
                               breakpoints ().size () == 1);
                m_engine->enable_countpoint
                    (a_in.output ().result_record ().
                     breakpoints ().begin ()->second.number (),
                     true);
            }
            has_breaks = true;
        }

        if (has_breaks
            && (a_in.command ().name () == "set-breakpoint"
                || a_in.command ().name () == "set-countpoint")) {
            // If we are getting this reply b/c we did set a
            // breakpoint, then only one breakpoint should be reported
            // back to us from GDB.
            const map<int, IDebugger::Breakpoint> &bps =
                a_in.output ().result_record ().breakpoints ();
            THROW_IF_FAIL (bps.size () == 1);

            std::pair<int,
                      const IDebugger::Breakpoint&> p (bps.begin ()->first,
                                                       bps.begin ()->second);

            Command &c = a_in.command ();
            if (c.name () == "set-breakpoint"
                && c.has_slot ()) {
                IDebugger::BreakpointSlot slot =
                    c.get_slot<IDebugger::BreakpointSlot> ();
                slot (p);
            }
            m_engine->breakpoint_set_signal ().emit
                (p, a_in.command ().cookie ());
            m_engine->set_state (IDebugger::READY);
        } else if (a_in.output ().has_result_record ()
            && a_in.output ().result_record ().kind ()
            == Output::ResultRecord::DONE
            && a_in.command ().value ().find ("-break-delete")
            != Glib::ustring::npos) {

            LOG_DD ("detected break-delete");
            UString tmp = a_in.command ().value ();
            tmp = tmp.erase (0, 13);
            if (tmp.size () == 0) {return;}
            tmp.chomp ();
            int bkpt_number = atoi (tmp.c_str ());
            if (bkpt_number) {
                map<int, IDebugger::Breakpoint>::iterator iter;
                map<int, IDebugger::Breakpoint> &breaks =
                                        m_engine->get_cached_breakpoints ();
                iter = breaks.find (bkpt_number);
                if (iter != breaks.end ()) {
                    LOG_DD ("firing IDebugger::breakpoint_deleted_signal()");
                    m_engine->breakpoint_deleted_signal ().emit
                    (iter->second, iter->first, a_in.command ().cookie ());
                    breaks.erase (iter);
                }
                m_engine->set_state (IDebugger::READY);
            } else {
                LOG_ERROR ("Got deleted breakpoint number '"
                           << tmp
                           << "', but that's not a well formed number dude.");
            }
        } else if (has_breaks) {
            LOG_DD ("firing IDebugger::breakpoint_set_signal()");
            m_engine->breakpoints_list_signal ().emit
                (m_engine->get_cached_breakpoints (),
                 a_in.command ().cookie ());
            m_engine->set_state (IDebugger::READY);
        } else {
            LOG_DD ("finally, no breakpoint was detected as set/deleted");
        }
    }
};//end struct OnBreakpointHandler

struct OnStoppedHandler: OutputHandler {
    GDBEngine *m_engine;
    Output::OutOfBandRecord m_out_of_band_record;
    bool m_is_stopped;

    OnStoppedHandler (GDBEngine *a_engine) :
        m_engine (a_engine),
        m_is_stopped (false)
    {}

    bool can_handle (CommandAndOutput &a_in)
    {
        if (!a_in.output ().has_out_of_band_record ()) {
            return false;
        }
        list<Output::OutOfBandRecord>::iterator iter;

        // We want to detect the last out-of-band-record that would
        // possibly tell us if the target was stopped somewhere. As we
        // can have multiple contiguous OOBRs sent by GDB (e.g, when a
        // countpoint is hit multiple times, each time it's hit we get
        // an OOBR saying the target is stopped, followed by another
        // one saying the target is running). So we need to start
        // walking the OOBRs from the end, and stop at the first OOBR
        // that tells us that the target has stopped. Then if before
        // that we saw an OOBR telling us that that the target was
        // running, then the target is running; otherwise it's stopped
        iter = a_in.output ().out_of_band_records ().end ();
        for (--iter;
             iter != a_in.output ().out_of_band_records ().end ();
             --iter) {
            if (iter->is_running ())
                break;
            if (iter->is_stopped ()) {
                m_is_stopped = true;
                m_out_of_band_record = *iter;
                return true;
            }
        }
        return false;
    }

    void do_handle (CommandAndOutput &a_in)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (m_is_stopped && m_engine);
        LOG_DD ("stopped. Command name was: '"
                << a_in.command ().name () << "' "
                << "Cookie was '" << a_in.command ().cookie () << "'");

        int thread_id = m_out_of_band_record.thread_id ();
        int breakpoint_number = -1;
        IDebugger::StopReason reason = m_out_of_band_record.stop_reason ();
        if (reason == IDebugger::BREAKPOINT_HIT
            || reason == IDebugger::WATCHPOINT_SCOPE)
            breakpoint_number = m_out_of_band_record.breakpoint_number ();

        if (m_out_of_band_record.has_frame ()) {
            m_engine->set_current_frame_level
                    (m_out_of_band_record.frame ().level ());
        }

        m_engine->stopped_signal ().emit
                    (m_out_of_band_record.stop_reason (),
                     m_out_of_band_record.has_frame (),
                     m_out_of_band_record.frame (),
                     thread_id, breakpoint_number,
                     a_in.command ().cookie ());


        if (reason == IDebugger::EXITED_SIGNALLED
            || reason == IDebugger::EXITED_NORMALLY
            || reason == IDebugger::EXITED) {
            m_engine->set_state (IDebugger::PROGRAM_EXITED);
            m_engine->detached_from_target_signal ().emit ();
            m_engine->program_finished_signal ().emit ();
        } else {
            m_engine->set_state (IDebugger::READY);
        }
    }
};//end struct OnStoppedHandler

struct OnFileListHandler : OutputHandler {
    GDBEngine *m_engine;

    OnFileListHandler (GDBEngine *a_engine) :
        m_engine (a_engine)
    {}

    bool can_handle (CommandAndOutput &a_in)
    {
        THROW_IF_FAIL (m_engine);
        if (a_in.output ().has_result_record ()
            && a_in.output ().result_record ().has_file_list ()) {
            LOG_DD ("handler selected");
            return true;
        }
        return false;
    }

    void do_handle (CommandAndOutput &a_in)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (m_engine);
        LOG_DD ("num files parsed: "
                << (int) a_in.output ().result_record ().file_list ().size ());
        m_engine->files_listed_signal ().emit
            (a_in.output ().result_record ().file_list (),
             a_in.command ().cookie ());
        m_engine->set_state (IDebugger::READY);
    }
};//end OnFileListHandler

struct OnThreadListHandler : OutputHandler {
    GDBEngine *m_engine;

    OnThreadListHandler (GDBEngine *a_engine) :
        m_engine (a_engine)
    {}

    bool can_handle (CommandAndOutput &a_in)
    {
        THROW_IF_FAIL (m_engine);
        if (a_in.output ().has_result_record ()
            && a_in.output ().result_record ().has_thread_list ()) {
            return true;
        }
        return false;
    }

    void do_handle (CommandAndOutput &a_in)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (m_engine);
        m_engine->threads_listed_signal ().emit
            (a_in.output ().result_record ().thread_list (),
             a_in.command ().cookie ());
    }
};//end OnThreadListHandler

struct OnThreadSelectedHandler : OutputHandler {
    GDBEngine *m_engine;
    long thread_id;
    bool has_frame;

    OnThreadSelectedHandler (GDBEngine *a_engine) :
        m_engine (a_engine),
        thread_id (0),
        has_frame (false)
    {}

    bool can_handle (CommandAndOutput &a_in)
    {
        THROW_IF_FAIL (m_engine);
        if (a_in.output ().has_result_record ()
            && a_in.output ().result_record ().thread_id_got_selected ()) {
            thread_id = a_in.output ().result_record ().thread_id ();
            return true;
        }
        if (a_in.output ().has_out_of_band_record ()) {
            list<Output::OutOfBandRecord>::const_iterator it;
            for (it = a_in.output ().out_of_band_records ().begin ();
                 it != a_in.output ().out_of_band_records ().end ();
                 ++it) {
                if (it->thread_id ()) {
                    thread_id = it->thread_id ();
                    return false; // GDB is broken currently
                                  // for this. So return false
                                  // for now. We'll be able to return
                                  // true later.;
                }
            }
        }
        return false;
    }

    void do_handle (CommandAndOutput &a_in)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (m_engine);

        m_engine->thread_selected_signal ().emit
            (thread_id,
             has_frame
                ? &a_in.output ().result_record ().frame_in_thread ()
                : 0,
             a_in.command ().cookie ());
    }
};//end OnThreadSelectedHandler

struct OnCommandDoneHandler : OutputHandler {

    GDBEngine *m_engine;

    OnCommandDoneHandler (GDBEngine *a_engine) :
        m_engine (a_engine)
    {}

    bool flag_breakpoint_as_countpoint (int a_break_num,
                                        bool a_yes)
    {
        typedef map<int, IDebugger::Breakpoint> BPMap;
        BPMap &bp_cache = m_engine->get_cached_breakpoints ();
        BPMap::const_iterator nil = bp_cache.end ();

        BPMap::iterator it = bp_cache.find (a_break_num);
        if (it == nil)
            return false;

        if (a_yes && it->second.type ()
            == IDebugger::Breakpoint::STANDARD_BREAKPOINT_TYPE)
            it->second.type (IDebugger::Breakpoint::COUNTPOINT_TYPE);
        else
            it->second.type
                (IDebugger::Breakpoint::STANDARD_BREAKPOINT_TYPE);

        return true;
    }

    bool can_handle (CommandAndOutput &a_in)
    {
        if (a_in.output ().has_result_record () &&
                a_in.output ().result_record ().kind ()
                == Output::ResultRecord::DONE) {
            return true;
        }
        return false;
    }

    void do_handle (CommandAndOutput &a_in)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        if (a_in.command ().name () == "attach-to-program") {
            m_engine->set_attached_to_target (true);
        }
        if (a_in.command ().name () == "select-frame") {
            m_engine->set_current_frame_level (a_in.command ().tag2 ());
        }
        if (a_in.command ().name () == "enable-countpoint"
            || a_in.command ().name () == "disable-countpoint") {
            if (a_in.command ().name () == "enable-countpoint") {
                flag_breakpoint_as_countpoint (a_in.command ().tag2 (), true);
            } else if (a_in.command ().name () == "disable-countpoint") {
                flag_breakpoint_as_countpoint (a_in.command ().tag2 (), false);
            }

            m_engine->breakpoints_list_signal ().emit
                (m_engine->get_cached_breakpoints (),
                 a_in.command ().cookie ());
        }

        m_engine->command_done_signal ().emit (a_in.command ().name (),
                                               a_in.command ().cookie ());

        if (a_in.command ().name () == "query-variable-path-expr"
            && a_in.command ().variable ()
            && a_in.output ().result_record ().has_path_expression ()) {
            a_in.command ().variable ()->path_expression
                        (a_in.output ().result_record ().path_expression ());
            // Call the slot associated to
            // IDebugger::query_varaible_path_expr
            if (a_in.command ().has_slot ()) {
                typedef sigc::slot<void, const IDebugger::VariableSafePtr>
                                                                    SlotType;
                SlotType slot = a_in.command ().get_slot<SlotType> ();
                slot (a_in.command ().variable ());
            }
        }

        // So, if we are still attached to the target and we receive
        // a "DONE" response from GDB, it means we are READY.
        // But if we are not attached -- which can mean that the
        // inferior just completed its execution or we did attached to
        // an already inferior and we detached from it -- then we should
        // not pretend we are in the READY state.
        // Even if we are attached and this GDB response comes right
        // after we sent the "detach-from-target" command, the
        // OnDetachHandler output handler should have already taken care
        // of updating our state, so let that state alone.
        if (m_engine->is_attached_to_target ()
            && (a_in.command ().name () != "detach-from-target")) {
            m_engine->set_state (IDebugger::READY);
        }
    }
};//struct OnCommandDoneHandler

struct OnRunningHandler : OutputHandler {

    GDBEngine *m_engine;

    OnRunningHandler (GDBEngine *a_engine) :
        m_engine (a_engine)
    {}

    bool can_handle (CommandAndOutput &a_in)
    {
        if (a_in.output ().has_result_record () &&
            a_in.output ().result_record ().kind ()
                == Output::ResultRecord::RUNNING) {
            return true;
        }
        return false;
    }

    void do_handle (CommandAndOutput &a_in)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        if (a_in.has_command ()) {}
        m_engine->running_signal ().emit ();
    }
};//struct OnRunningHandler

struct OnConnectedHandler : OutputHandler {
    GDBEngine *m_engine;

    OnConnectedHandler (GDBEngine *a_engine) :
        m_engine (a_engine)
    {}

    bool can_handle (CommandAndOutput &a_in)
    {
        if (a_in.output ().has_result_record () &&
            a_in.output ().result_record ().kind () ==
              Output::ResultRecord::CONNECTED) {
            return true;
        }
        return false;
    }

    void do_handle (CommandAndOutput &a_in)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        if (a_in.has_command ()) {}
        m_engine->set_state (IDebugger::READY);
        m_engine->connected_to_server_signal ().emit ();
    }

};//struct OnConnectedHandler
struct OnFramesListedHandler : OutputHandler {

    GDBEngine *m_engine;

    OnFramesListedHandler (GDBEngine *a_engine) :
        m_engine (a_engine)
    {}

    bool can_handle (CommandAndOutput &a_in)
    {
        if (a_in.output ().has_result_record ()
            && (a_in.output ().result_record ().kind ()
                == Output::ResultRecord::DONE)
            && (a_in.output ().result_record ().has_call_stack ())) {
            LOG_DD ("handler selected");
            return true;
        }
        return false;
    }

    void do_handle (CommandAndOutput &a_in)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        if (!a_in.output ().result_record ().call_stack ().empty ()
            && a_in.output ().result_record ().call_stack ()[0].level ()
                == 0)
            m_engine->set_current_frame_address
            (a_in.output ().result_record ().call_stack ()[0].address ());

        vector<IDebugger::Frame> &frames =
            a_in.output ().result_record ().call_stack ();

        if (a_in.command ().has_slot ()) {
            IDebugger::FrameVectorSlot slot =
                a_in.command ().get_slot<IDebugger::FrameVectorSlot> ();
            slot (frames);
        }

        m_engine->frames_listed_signal ().emit (frames,
                                                a_in.command ().cookie ());
        m_engine->set_state (IDebugger::READY);
    }
};//struct OnFramesListedHandler

struct OnFramesParamsListedHandler : OutputHandler {

    GDBEngine *m_engine;

    OnFramesParamsListedHandler (GDBEngine *a_engine) :
        m_engine (a_engine)
    {}

    bool can_handle (CommandAndOutput &a_in)
    {
        if (a_in.output ().has_result_record ()
            && (a_in.output ().result_record ().kind ()
                == Output::ResultRecord::DONE)
            && (a_in.output ().result_record ().has_frames_parameters ())) {
            LOG_DD ("handler selected");
            return true;
        }
        return false;
    }

    void do_handle (CommandAndOutput &a_in)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        
        const map<int, list<IDebugger::VariableSafePtr> > &frame_args =
            a_in.output ().result_record ().frames_parameters ();

        if (a_in.command ().has_slot ()) {
            IDebugger::FrameArgsSlot slot =
                a_in.command ().get_slot<IDebugger::FrameArgsSlot> ();
            slot (frame_args);
        }

        m_engine->frames_arguments_listed_signal ().emit
            (a_in.output ().result_record ().frames_parameters (),
             a_in.command ().cookie ());
        m_engine->set_state (IDebugger::READY);
    }
};//struct OnFramesParamsListedHandler

struct OnCurrentFrameHandler : OutputHandler {
    GDBEngine *m_engine;

    OnCurrentFrameHandler (GDBEngine *a_engine) :
        m_engine (a_engine)
    {
    }

    bool can_handle (CommandAndOutput &a_in)
    {
        if (a_in.output ().result_record
                ().has_current_frame_in_core_stack_trace ()) {
            LOG_DD ("handler selected");
            return true;
        }
        return false;
    }

    void do_handle (CommandAndOutput &a_in)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        m_engine->current_frame_signal ().emit
            (a_in.output ().result_record ().current_frame_in_core_stack_trace (),
             "");
    }
};//struct OnCurrentFrameHandler

struct OnInfoProcHandler : OutputHandler {

    GDBEngine *m_engine;

    OnInfoProcHandler (GDBEngine *a_engine) :
        m_engine (a_engine)
    {}

    bool can_handle (CommandAndOutput &a_in)
    {
        if (a_in.has_command ()
            && (a_in.command ().value ().find ("info proc")
                != Glib::ustring::npos)
            && (a_in.output ().has_out_of_band_record ())) {

            LOG_DD ("handler selected");
            return true;
        }
        return false;
    }

    void do_handle (CommandAndOutput &a_in)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        THROW_IF_FAIL (m_engine);

        int pid = 0; UString exe_path;
        if (!m_engine->extract_proc_info (a_in.output (), pid, exe_path)) {
            LOG_ERROR ("failed to extract proc info");
            return;
        }
        THROW_IF_FAIL (pid);
        m_engine->got_target_info_signal ().emit (pid, exe_path);
        m_engine->set_state (IDebugger::READY);
    }
};//struct OnInfoProcHandler

struct OnLocalVariablesListedHandler : OutputHandler {

    GDBEngine *m_engine;

    OnLocalVariablesListedHandler (GDBEngine *a_engine) :
        m_engine (a_engine)
    {}

    bool can_handle (CommandAndOutput &a_in)
    {
        if (a_in.output ().has_result_record ()
            && (a_in.output ().result_record ().kind ()
                == Output::ResultRecord::DONE)
            && (a_in.output ().result_record ().has_local_variables ())) {
            LOG_DD ("handler selected");
            return true;
        }
        return false;
    }

    void do_handle (CommandAndOutput &a_in)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        THROW_IF_FAIL (m_engine);

        m_engine->local_variables_listed_signal ().emit
            (a_in.output ().result_record ().local_variables (),
             a_in.command ().cookie ());
        m_engine->set_state (IDebugger::READY);
    }
};//struct OnLocalVariablesListedHandler

struct OnGlobalVariablesListedHandler : OutputHandler {

    GDBEngine *m_engine;

    OnGlobalVariablesListedHandler (GDBEngine *a_engine) :
        m_engine (a_engine)
    {}

    bool can_handle (CommandAndOutput &a_in)
    {
        if (a_in.command ().name () == "list-global-variables") {
            LOG_DD ("list-global-variables / -symbol-list-variables handler selected");
            return true;
        }
        return false;
    }

    void do_handle (CommandAndOutput &a_in)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        THROW_IF_FAIL (m_engine);

        list<IDebugger::VariableSafePtr> var_list;
        GDBEngine::VarsPerFilesMap vars_per_files_map;
        if (!m_engine->extract_global_variable_list (a_in.output (),
                                                     vars_per_files_map)) {
            LOG_ERROR ("failed to extract global variable list");
            return;
        }

        //now build a single list of global variables, out of 
        //vars_per_files_map, and notify the client with it.

        map<string, bool> recorded_var_names;
        list<IDebugger::VariableSafePtr>::const_iterator var_it;
        GDBEngine::VarsPerFilesMap::const_iterator it;
        for (it = vars_per_files_map.begin ();
             it != vars_per_files_map.end ();
             ++it) {
            for (var_it = it->second.begin ();
                 var_it != it->second.end ();
                 ++var_it) {
                if (recorded_var_names.find ((*var_it)->name ().raw ())
                    != recorded_var_names.end ()) {
                    //make sure to avoid duplicated global variables names.
                    continue;
                }
                var_list.push_back (*var_it);
                recorded_var_names[(*var_it)->name ().raw ()] = true;
            }
        }
        m_engine->global_variables_listed_signal ().emit
                                    (var_list, a_in.command ().cookie ());
        m_engine->set_state (IDebugger::READY);
    }
};//struct OnGlobalVariablesListedHandler

struct OnResultRecordHandler : OutputHandler {

    GDBEngine *m_engine;

    OnResultRecordHandler (GDBEngine *a_engine) :
        m_engine (a_engine)
    {}

    // TODO: split this OutputHandler into several different handlers.
    // Ideally there should be one handler per command sent to GDB.
    bool can_handle (CommandAndOutput &a_in)
    {
        if ((a_in.command ().name () == "print-variable-value"
             || a_in.command ().name () == "get-variable-value"
             || a_in.command ().name () == "print-pointed-variable-value"
             || a_in.command ().name () == "dereference-variable"
             || a_in.command ().name () == "set-register-value"
             || a_in.command ().name () == "set-memory"
             || a_in.command ().name () == "assign-variable"
             || a_in.command ().name () == "evaluate-expression")
            && a_in.output ().has_result_record ()
            && (a_in.output ().result_record ().kind ()
                == Output::ResultRecord::DONE)
            && (a_in.output ().result_record ().has_variable_value ())) {
            LOG_DD ("handler selected");
            return true;
        }
        return false;
    }

    void do_handle (CommandAndOutput &a_in)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        THROW_IF_FAIL (m_engine);

        UString var_name = a_in.command ().tag1 ();
        if (var_name == "") {
            THROW_IF_FAIL (a_in.command ().variable ());
            var_name = a_in.command ().variable ()->name ();
        }
        THROW_IF_FAIL (a_in.output ().result_record ().variable_value ());

        if (a_in.output ().result_record ().variable_value ()->name () == "") {
            var_name.chomp ();
            a_in.output ().result_record ().variable_value ()->name (var_name);
        } else {
            THROW_IF_FAIL
                (a_in.output ().result_record ().variable_value ()->name ()
                 == var_name);
        }
        if (a_in.command ().name () == "print-variable-value") {
            LOG_DD ("got print-variable-value");
            THROW_IF_FAIL (var_name != "");
            if (a_in.output ().result_record ().variable_value ()->name ()
                == "") {
                a_in.output ().result_record ().variable_value ()->name
                    (a_in.command ().tag1 ());
            }
            m_engine->variable_value_signal ().emit
                        (a_in.command ().tag1 (),
                         a_in.output ().result_record ().variable_value (),
                         a_in.command ().cookie ());
        } else if (a_in.command ().name () == "get-variable-value") {
            IDebugger::VariableSafePtr var = a_in.command ().variable ();
            THROW_IF_FAIL (var);
            a_in.output ().result_record ().variable_value ()->name
                                                                (var->name ());
            a_in.output ().result_record ().variable_value ()->type
                                                                (var->type ());
            if (var->is_dereferenced ()) {
                a_in.output ().result_record ().variable_value
                        ()->set_dereferenced (var->get_dereferenced ());
            }
            THROW_IF_FAIL (a_in.output ().result_record ().variable_value ());
            var->set (*a_in.output ().result_record ().variable_value ());
            m_engine->variable_value_set_signal ().emit
                                            (var, a_in.command ().cookie ());
        } else if (a_in.command ().name () == "assign-variable") {
            THROW_IF_FAIL (a_in.command ().variable ());
            THROW_IF_FAIL (a_in.output ().result_record ().variable_value ());

            // Set the result we got from gdb into the variable
            // handed to us by the client code.
            a_in.command ().variable ()->value
                (a_in.output ().result_record ().variable_value ()->value ());

            // Call the slot associated to IDebugger::assign_variable(), if
            // any.
            if (a_in.command ().has_slot ()) {
                typedef sigc::slot<void, const IDebugger::VariableSafePtr>
                                                                    SlotType;
                SlotType slot = a_in.command ().get_slot<SlotType> ();
                slot (a_in.command ().variable ());
            }

            // Tell the world that the variable got assigned
            m_engine->assigned_variable_signal ().emit
                (a_in.command ().variable (), a_in.command ().cookie ());
        } else if (a_in.command ().name () == "evaluate-expression") {
            THROW_IF_FAIL (a_in.command ().variable ());
            THROW_IF_FAIL (a_in.output ().result_record ().variable_value ());

            // Set the result we got from gdb into the variable
            // handed to us by the client code.
            a_in.command ().variable ()->value
                (a_in.output ().result_record ().variable_value ()->value ());

            // Call the slot associated to
            // IDebugger::evaluate_variable_expr(), if any.
            if (a_in.command ().has_slot ()) {
                typedef sigc::slot<void, const IDebugger::VariableSafePtr>
                                                                    SlotType;
                SlotType slot = a_in.command ().get_slot<SlotType> ();
                slot (a_in.command ().variable ());
            }

            // Tell the world that the expression got evaluated
            m_engine->variable_expression_evaluated_signal ().emit
                (a_in.command ().variable (), a_in.command ().cookie ());
        } else if (a_in.command ().name () == "print-pointed-variable-value") {
            LOG_DD ("got print-pointed-variable-value");
            THROW_IF_FAIL (var_name != "");
            IDebugger::VariableSafePtr variable =
                        a_in.output ().result_record ().variable_value ();
            variable->name ("*" + variable->name ());
            m_engine->pointed_variable_value_signal ().emit
                                                (a_in.command ().tag1 (),
                                                 variable,
                                                 a_in.command ().cookie ());
        } else if (a_in.command ().name () == "dereference-variable") {
            LOG_DD ("got dereference-variable");
            //the variable we where dereferencing must be
            //in a_in.command ().variable ().
            THROW_IF_FAIL (a_in.command ().variable ());

            //construct the resulting variable of the dereferencing
            IDebugger::VariableSafePtr derefed =
                        a_in.output ().result_record ().variable_value ();
            THROW_IF_FAIL (derefed);
            //set the name of the resulting variable.
            derefed->name (a_in.command ().variable ()->name ());
            //set the name caption of the resulting variable.
            UString name_caption;
            a_in.command ().variable ()->build_qname (name_caption);
            name_caption = "*" + name_caption;
            derefed->name_caption (name_caption);

            //now associate the resulting variable or the dereferencing to
            //the variable that was given in parameter to
            //the IDebugger::dereference_variable() call.
            a_in.command ().variable ()->set_dereferenced (derefed);
            m_engine->variable_dereferenced_signal ().emit
                                                (a_in.command ().variable (),
                                                 a_in.command ().cookie ());
        } else if (a_in.command ().name () == "set-register-value") {
            IDebugger::VariableSafePtr var =
                a_in.output ().result_record ().variable_value ();
            THROW_IF_FAIL (var);
            THROW_IF_FAIL (!a_in.command ().tag1().empty ());
            m_engine->register_value_changed_signal ().emit
                                                (a_in.command ().tag1 (),
                                                 var->value (),
                                                 a_in.command ().cookie ());
        } else if (a_in.command ().name () == "set-memory") {
            IDebugger::VariableSafePtr var =
                a_in.output ().result_record ().variable_value ();
            THROW_IF_FAIL (var);
            THROW_IF_FAIL (!a_in.command ().tag1().empty ());
            m_engine->register_value_changed_signal ().emit
                                                (a_in.command ().tag1 (),
                                                 var->value (),
                                                 a_in.command ().cookie ());
        } else {
            THROW ("unknown command : " + a_in.command ().name ());
        }
        m_engine->set_state (IDebugger::READY);
    }
};//struct OnResultRecordHandler

struct OnVariableTypeHandler : OutputHandler {
    GDBEngine *m_engine;

    OnVariableTypeHandler (GDBEngine *a_engine) :
        m_engine (a_engine)
    {
        THROW_IF_FAIL (m_engine);
    }

    bool can_handle (CommandAndOutput &a_in)
    {
        if ((a_in.command ().name () == "print-variable-type"
            ||a_in.command ().name () == "get-variable-type")
            && a_in.output ().has_out_of_band_record ()) {
            list<Output::OutOfBandRecord>::const_iterator it;
            for (it = a_in.output ().out_of_band_records ().begin ();
                 it != a_in.output ().out_of_band_records ().end ();
                 ++it) {
                
                LOG_DD ("checking debugger console: "
                        << it->stream_record ().debugger_console ());
                if (it->has_stream_record ()
                    && (!it->stream_record ().debugger_console ().compare
                        (0, 6, "ptype ")
                        || !it->stream_record () .debugger_log ().compare
                        (0, 6, "ptype "))) {
                    LOG_DD ("handler selected");
                    return true;
                }
            }
        }
        return false;
    }

    void do_handle (CommandAndOutput &a_in)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (m_engine);
        UString type;
        list<Output::OutOfBandRecord>::const_iterator it;
        it = a_in.output ().out_of_band_records ().begin ();
        ++it;
        if (!it->has_stream_record ()
            || it->stream_record ().debugger_console ().compare
                                               (0, 7, "type = "))
        {
            if (!it->has_stream_record ()) {
                LOG_ERROR ("no more stream record !");
                return;
            }
            //TODO: debug this further. I don't know why we would get
            //empty stream records after the result of ptype stream record.
            //This happens though.
            LOG_ERROR_DD ("expected result of ptype, got : '"
                          <<it->stream_record ().debugger_console () << "'");
            return;
        }

        UString console = it->stream_record ().debugger_console ();
        console.erase (0, 7);
        type += console;
        ++it;
        for (;
             it != a_in.output ().out_of_band_records ().end ();
             ++it) {
            if (it->has_stream_record ()
                && it->stream_record ().debugger_console () != "") {
                console = it->stream_record ().debugger_console ();
                type += console;
            }
        }
        type.chomp ();
        LOG_DD ("got type: " << type);
        if (type != "") {
            if (a_in.command ().name () == "print-variable-type") {
                UString var_name = a_in.command ().tag1 ();
                THROW_IF_FAIL (var_name != "");
                m_engine->variable_type_signal ().emit
                                            (var_name,
                                             type,
                                             a_in.command ().cookie ());
            } else if (a_in.command ().name () == "get-variable-type") {
                IDebugger::VariableSafePtr var;
                var = a_in.command ().variable ();
                THROW_IF_FAIL (var);
                THROW_IF_FAIL (var->name () != "");
                var->type (type);
                m_engine->variable_type_set_signal ().emit
                                        (var, a_in.command ().cookie ());
            } else {
                THROW ("should not be reached");
            }
        }
        m_engine->set_state (IDebugger::READY);
    }
};//struct VariableTypeHandler

struct OnSignalReceivedHandler : OutputHandler {

    GDBEngine *m_engine;
    Output::OutOfBandRecord oo_record;

    OnSignalReceivedHandler (GDBEngine *a_engine) :
        m_engine (a_engine)
    {}

    bool can_handle (CommandAndOutput &a_in)
    {
        if (!a_in.output ().has_out_of_band_record ()) {
            return false;
        }
        list<Output::OutOfBandRecord>::const_iterator it;
        for (it = a_in.output ().out_of_band_records ().begin ();
             it != a_in.output ().out_of_band_records ().end ();
             ++it) {
            if (it->stop_reason () == IDebugger::SIGNAL_RECEIVED) {
                oo_record = *it;
                LOG_DD ("output handler selected");
                return true;
            }
        }
        return false;
    }

    void do_handle (CommandAndOutput &a_in)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        if (a_in.has_command ()) {}
        THROW_IF_FAIL (m_engine);
        m_engine->signal_received_signal ().emit (oo_record.signal_type (),
                                                  oo_record.signal_meaning ());
        m_engine->set_state (IDebugger::READY);
    }
};//struct OnSignalReceivedHandler

struct OnRegisterNamesListedHandler : OutputHandler {

    GDBEngine *m_engine;

    OnRegisterNamesListedHandler (GDBEngine *a_engine) :
        m_engine (a_engine)
    {}

    bool can_handle (CommandAndOutput &a_in)
    {
        if (a_in.output ().has_result_record ()
            && (a_in.output ().result_record ().kind ()
                == Output::ResultRecord::DONE)
            && (a_in.output ().result_record ().has_register_names ())) {
            LOG_DD ("handler selected");
            return true;
        }
        return false;
    }

    void do_handle (CommandAndOutput &a_in)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        m_engine->register_names_listed_signal ().emit
            (a_in.output ().result_record ().register_names (),
             a_in.command ().cookie ());
        m_engine->set_state (IDebugger::READY);
    }
};//struct OnRegisterNamesListedHandler

struct OnChangedRegistersListedHandler : OutputHandler {

    GDBEngine *m_engine;

    OnChangedRegistersListedHandler (GDBEngine *a_engine) :
        m_engine (a_engine)
    {}

    bool can_handle (CommandAndOutput &a_in)
    {
        if (a_in.output ().has_result_record ()
            && (a_in.output ().result_record ().kind ()
                == Output::ResultRecord::DONE)
            && (a_in.output ().result_record ().has_changed_registers ())) {
            LOG_DD ("handler selected");
            return true;
        }
        return false;
    }

    void do_handle (CommandAndOutput &a_in)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        m_engine->changed_registers_listed_signal ().emit
            (a_in.output ().result_record ().changed_registers (),
             a_in.command ().cookie ());
        m_engine->set_state (IDebugger::READY);
    }
};//struct OnChangedRegistersListedHandler

struct OnRegisterValuesListedHandler : OutputHandler {

    GDBEngine *m_engine;

    OnRegisterValuesListedHandler (GDBEngine *a_engine) :
        m_engine (a_engine)
    {}

    bool can_handle (CommandAndOutput &a_in)
    {
        if (a_in.output ().has_result_record ()
            && (a_in.output ().result_record ().kind ()
                == Output::ResultRecord::DONE)
            && (a_in.output ().result_record ().has_register_values ())) {
            LOG_DD ("handler selected");
            return true;
        }
        return false;
    }

    void do_handle (CommandAndOutput &a_in)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        m_engine->register_values_listed_signal ().emit
            (a_in.output ().result_record ().register_values (),
             a_in.command ().cookie ());
        m_engine->set_state (IDebugger::READY);
    }
};//struct OnRegisterValuesListedHandler

struct OnSetRegisterValueHandler : OutputHandler {

    GDBEngine *m_engine;

    OnSetRegisterValueHandler (GDBEngine *a_engine) :
        m_engine (a_engine)
    {}

    bool can_handle (CommandAndOutput &a_in)
    {
        if (a_in.output ().has_result_record ()
            && (a_in.output ().result_record ().kind ()
                == Output::ResultRecord::DONE)
            && (a_in.command ().name () == "set-register-value")) {
            LOG_DD ("handler selected");
            return true;
        }
        return false;
    }

    void do_handle (CommandAndOutput &a_in)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        m_engine->register_value_changed_signal ().emit
            (a_in.command ().tag1 (),
             // FIXME: get register value here
             UString(),
             a_in.command ().cookie ());
        m_engine->set_state (IDebugger::READY);
    }
};//struct OnSetRegisterValueHandler

struct OnReadMemoryHandler : OutputHandler {

    GDBEngine *m_engine;

    OnReadMemoryHandler (GDBEngine *a_engine) :
        m_engine (a_engine)
    {}

    bool can_handle (CommandAndOutput &a_in)
    {
        if (a_in.output ().has_result_record ()
            && (a_in.output ().result_record ().kind ()
                == Output::ResultRecord::DONE)
            && (a_in.output ().result_record ().has_memory_values ())) {
            LOG_DD ("handler selected");
            return true;
        }
        return false;
    }

    void do_handle (CommandAndOutput &a_in)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        m_engine->read_memory_signal ().emit
            (a_in.output ().result_record ().memory_address (),
             a_in.output ().result_record ().memory_values (),
             a_in.command ().cookie ());
        m_engine->set_state (IDebugger::READY);
    }
};//struct OnReadMemoryHandler

struct OnSetMemoryHandler : OutputHandler
{
    GDBEngine *m_engine;

    OnSetMemoryHandler (GDBEngine *a_engine) :
        m_engine (a_engine)
    {}

    bool can_handle (CommandAndOutput &a_in)
    {
        if (a_in.output ().has_result_record ()
            && (a_in.output ().result_record ().kind ()
                == Output::ResultRecord::DONE)
            && (a_in.command ().name () == "set-memory")) {
            LOG_DD ("handler selected");
            return true;
        }
        return false;
    }

    void do_handle (CommandAndOutput &a_in)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        size_t addr = 0;
        std::istringstream istream (a_in.command ().tag1 ());
        istream >> std::hex >> addr;
        m_engine->set_memory_signal ().emit
            (addr,
             std::vector<uint8_t>(), // FIXME: get memory value here
             a_in.command ().cookie ());
        m_engine->set_state (IDebugger::READY);
    }
};//struct OnSetRegisterValueHandler


struct OnErrorHandler : OutputHandler {

    GDBEngine *m_engine;

    OnErrorHandler (GDBEngine *a_engine) :
        m_engine (a_engine)
    {}

    bool can_handle (CommandAndOutput &a_in)
    {
        if (a_in.output ().has_result_record ()
            && (a_in.output ().result_record ().kind ()
                == Output::ResultRecord::ERROR)) {
            LOG_DD ("handler selected");
            return true;
        }
        return false;
    }

    void do_handle (CommandAndOutput &a_in)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (m_engine);
        m_engine->error_signal ().emit
            (a_in.output ().result_record ().attrs ()["msg"]);

        if (m_engine->get_state () != IDebugger::PROGRAM_EXITED
            || m_engine->get_state () != IDebugger::NOT_STARTED) {
            m_engine->set_state (IDebugger::READY);
        }
    }
};//struct OnErrorHandler

struct OnDisassembleHandler : OutputHandler {

    GDBEngine *m_engine;

    OnDisassembleHandler (GDBEngine *a_engine) :
        m_engine (a_engine)
    {}

    bool can_handle (CommandAndOutput &a_in)
    {
        if (!a_in.command ().name ().raw ().compare (0,
                                                     strlen ("disassemble"),
                                                     "disassemble")
            && a_in.output ().has_result_record ()
            && a_in.output ().result_record ().has_asm_instruction_list ()) {
            LOG_DD ("handler selected");
            return true;
        }
        return false;
    }

    void do_handle (CommandAndOutput &a_in)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (m_engine);

        const std::list<common::Asm>& instrs =
            a_in.output ().result_record ().asm_instruction_list ();
        common::DisassembleInfo info;

        if (a_in.command ().name () == "disassemble-line-range-in-file") {
            info.file_name (a_in.command ().tag0 ());
        }
        if (!instrs.empty ()) {
            std::list<common::Asm>::const_iterator it = instrs.begin ();
            if (!it->empty ()) {
                info.start_address ((*it).instr ().address ());
                it = instrs.end ();
                it--;
                info.end_address ((*it).instr ().address ());
            }
        }
        // Call the slot associated to IDebugger::disassemble, if any.
        if (a_in.command ().has_slot ()) {
            IDebugger::DisassSlot slot =
                a_in.command ().get_slot<IDebugger::DisassSlot> ();
            slot (info, instrs);
        }
        m_engine->instructions_disassembled_signal ().emit
                               (info, instrs, a_in.command ().cookie ());

        m_engine->set_state (IDebugger::READY);
    }
};//struct OnDisassembleHandler


struct OnCreateVariableHandler : public OutputHandler
{
    GDBEngine *m_engine;

    OnCreateVariableHandler (GDBEngine *a_engine) :
        m_engine (a_engine)
    {
    }

    bool can_handle (CommandAndOutput &a_in)
    {
        if (a_in.output ().has_result_record ()
            && (a_in.output ().result_record ().kind ()
            == Output::ResultRecord::DONE)
            && (a_in.command ().name () == "create-variable")
            && (a_in.output ().result_record ().has_variable ())) {
            LOG_DD ("handler selected");
            return true;
        }
        return false;
    }

    void do_handle (CommandAndOutput &a_in)
    {
        // Set the name of the variable to the name that got stored
        // in the tag0 member of the command.
        a_in.output ().result_record ().variable ()->name
                                                (a_in.command ().tag0 ());

        // Call the slot associated to IDebugger::create_variable (), if
        // any.
        if (a_in.command ().has_slot ()) {
            LOG_DD ("calling IDebugger::create_variable slot");
            typedef sigc::slot<void, IDebugger::VariableSafePtr> SlotType;
            SlotType slot = a_in.command ().get_slot<SlotType> ();
            slot (a_in.output ().result_record ().variable ());
        }
        LOG_DD ("emit IDebugger::variable_create_signal");
        // Emit the general IDebugger::variable_create_signal () signal
        m_engine->variable_created_signal ().emit
                            (a_in.output ().result_record ().variable (),
                             a_in.command ().cookie ());
        if (m_engine->get_state () != IDebugger::PROGRAM_EXITED
            || m_engine->get_state () != IDebugger::NOT_STARTED) {
            m_engine->set_state (IDebugger::READY);
        }
    }
};// end OnCreateVariableHandler

struct OnDeleteVariableHandler : public OutputHandler {
    GDBEngine *m_engine;

    OnDeleteVariableHandler (GDBEngine *a_engine) :
        m_engine (a_engine)
    {
    }

    bool can_handle (CommandAndOutput &a_in)
    {
        if (a_in.output ().has_result_record ()
            && (a_in.output ().result_record ().kind ()
            == Output::ResultRecord::DONE)
            && (a_in.command ().name () == "delete-variable")
            && (a_in.output ().result_record ().number_of_variables_deleted ())) {
            LOG_DD ("handler selected");
            return true;
        }
        return false;
    }

    void do_handle (CommandAndOutput &a_in)
    {
        THROW_IF_FAIL (a_in.command ().variable ());
        THROW_IF_FAIL (m_engine);

        // Call the slot associated to IDebugger::delete_variable (), if
        // Any.
        if (a_in.command ().has_slot ()) {
            typedef sigc::slot<void, const IDebugger::VariableSafePtr> SlotType;
            SlotType slot = a_in.command ().get_slot<SlotType> ();
            slot (a_in.command ().variable ());
        }
        // Emit the general IDebugger::variable_deleted_signal ().
        m_engine->variable_deleted_signal ().emit
                (a_in.command ().variable (),
                 a_in.command ().cookie ());
    }
}; // end OnDeleteVariableHandler

struct OnUnfoldVariableHandler : public OutputHandler {
    GDBEngine *m_engine;

    OnUnfoldVariableHandler (GDBEngine *a_engine) :
        m_engine (a_engine)
    {
    }

    bool can_handle (CommandAndOutput &a_in)
    {
        if ((a_in.output ().result_record ().kind ()
             == Output::ResultRecord::DONE)
            && a_in.output ().result_record ().has_variable_children ()
            && a_in.command ().name () == "unfold-variable") {
            LOG_DD ("handler selected");
            return true;
        }
        return false;
    }

    void do_handle (CommandAndOutput &a_in)
    {
        // *************************************************************
        // append the unfolded variable children to the parent variable
        // supplied by the client code.
        // *************************************************************
        IDebugger::VariableSafePtr parent_var = a_in.command ().variable ();
        THROW_IF_FAIL (parent_var);
        typedef vector<IDebugger::VariableSafePtr> Variables;
        Variables children_vars =
            a_in.output ().result_record ().variable_children ();
        for (Variables::const_iterator it = children_vars.begin ();
             it != children_vars.end ();
             ++it) {
            parent_var->append (*it);
        }

        // Call the slot associated to IDebugger::unfold_variable (), if
        // any.
        if (a_in.command ().has_slot ()) {
            typedef sigc::slot<void, const IDebugger::VariableSafePtr> SlotType;
            SlotType slot = a_in.command ().get_slot<SlotType> ();
            slot (a_in.command ().variable ());
        }

        // Now tell the world we have an unfolded variable.
        m_engine->variable_unfolded_signal ().emit
                                    (parent_var, a_in.command ().cookie ());
    }
};// End struct OnUnfoldVariableHandler

struct OnListChangedVariableHandler : public OutputHandler
{
    GDBEngine *m_engine;

    OnListChangedVariableHandler (GDBEngine *a_engine) :
        m_engine (a_engine)
    {
    }

    bool can_handle (CommandAndOutput &a_in)
    {
        if (a_in.output ().has_result_record ()
            && a_in.output ().result_record ().kind ()
                == Output::ResultRecord::DONE
            && a_in.output ().result_record ().has_changed_var_list ()
            && a_in.command ().name () == "list-changed-variables") {
            LOG_DD ("handler selected");
            return true;
        }
        return false;
    }

    void do_handle (CommandAndOutput &a_in)
    {
        THROW_IF_FAIL (a_in.command ().variable ());
        THROW_IF_FAIL (a_in.output ().result_record ().has_changed_var_list ());

        // We are going to build the list of descendants of
        // a_in.command ().variable () that got returned as being
        // changed.
        list<IDebugger::VariableSafePtr> vars;
        const list<IDebugger::VariableSafePtr> &changed_vars =
                    a_in.output ().result_record ().changed_var_list ();
        list<IDebugger::VariableSafePtr>::const_iterator it;
        IDebugger::VariableSafePtr v;
        for (it = changed_vars.begin (); it != changed_vars.end (); ++it) {
            v = a_in.command ().variable ()->get_descendant
                                                ((*it)->internal_name ());
            // Okay, so v now contains the a descendant variable of
            // a_in.command ().variable () which value got modified.
            // Let's set the value of v to the new updated value.
            v->value ((*it)->value ());
            if (v) {
                vars.push_back (v);
            }
        }
        // Call the slot associated to
        // IDebugger::list_changed_variables(), if any.
        if (a_in.command ().has_slot ()) {
            typedef sigc::slot<void,
                               const list<IDebugger::VariableSafePtr>&>
                                                                SlotType;
            SlotType slot = a_in.command ().get_slot<SlotType> ();
            slot (vars);
        }
        // Tell the world about the descendant variables that changed.
        m_engine->changed_variables_signal ().emit
                            (vars, a_in.command ().cookie ());
    }
};//end OnListChangedVariableHandler

//****************************
//</GDBengine output handlers
//***************************

    //****************************
    //<GDBEngine methods>
    //****************************
GDBEngine::GDBEngine (DynamicModule *a_dynmod) : IDebugger (a_dynmod)
{
    m_priv.reset (new Priv (a_dynmod));
    init ();
}

GDBEngine::~GDBEngine ()
{
    LOG_D ("delete", "destructor-domain");
}

bool
GDBEngine::load_program (const UString &a_prog,
                         const vector<UString> &a_args,
                         const UString &a_working_dir,
                         bool a_force)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    vector<UString> search_paths;
    UString tty_path;
    return load_program (a_prog, a_args, a_working_dir,
                         search_paths, tty_path, a_force);
}

bool
GDBEngine::load_program (const UString &a_prog,
                         const vector<UString> &a_argv,
                         const UString &working_dir,
                         const vector<UString> &a_source_search_dirs,
                         const UString &a_tty_path,
                         bool a_force)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    THROW_IF_FAIL (!a_prog.empty ());
    vector<UString> argv (a_argv);

    if (!m_priv->is_gdb_running ()) {
        vector<UString> gdb_opts;
        if (m_priv->launch_gdb_and_set_args (working_dir,
                                             a_source_search_dirs, 
                                             a_prog, a_argv,
                                             gdb_opts) == false)
            return false;

        Command command;

        // In case we are restarting GDB after a crash, the command
        // queue might be stuck.  Let's restart it.
        if (a_force)
            m_priv->reset_command_queue ();

        queue_command (Command ("set breakpoint pending on"));

        // tell gdb not to pass the SIGINT signal to the target.
        queue_command (Command ("handle SIGINT stop print nopass"));

	// Tell gdb not to pass the SIGHUP signal to the target.  This
	// is useful when the debugger follows a child during a
	// fork. In that case, a SIGHUP is sent to the debugger if it
	// has some controlling term attached to the target.. Make
	// sure the signal is not not passed to the target and the
	// debugger doesn't stop upon that signal.
	queue_command (Command ("handle SIGHUP nostop print nopass"));

        // tell the linker to do all relocations at program load
        // time so that some "step into" don't take for ever.
        // On GDB, it seems that stepping into a function that is
        // in a share lib takes stepping through GNU ld, so it can take time.
        const char *nmv_ld_bind_now = g_getenv ("NMV_LD_BIND_NOW");
        if (nmv_ld_bind_now && atoi (nmv_ld_bind_now)) {
            LOG_DD ("setting LD_BIND_NOW=1");
            queue_command (Command ("set env LD_BIND_NOW 1"));
        } else {
            LOG_DD ("not setting LD_BIND_NOW environment variable ");
        }
    } else {
        Command command ("load-program",
                         UString ("-file-exec-and-symbols ") + a_prog);
        queue_command (command);

        UString args = quote_args (argv);
        if (!args.empty ()) {
            command.value ("set args " + args);
            queue_command (command);
        }
    }
    set_tty_path (a_tty_path);
    return true;
}

void
GDBEngine::load_core_file (const UString &a_prog_path,
                           const UString &a_core_path)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    if (m_priv->is_gdb_running ()) {
        m_priv->kill_gdb ();
    }
    THROW_IF_FAIL (m_priv->launch_gdb_on_core_file (a_prog_path,
                                                    a_core_path));
}

bool
GDBEngine::attach_to_target (unsigned int a_pid,
                             const UString &a_tty_path)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    vector<UString> args, source_search_dirs;

    if (!m_priv->is_gdb_running ()) {
        vector<UString> gdb_opts;
        THROW_IF_FAIL (m_priv->launch_gdb ("" /* no cwd */,
                                           source_search_dirs,
                                           "" /* no inferior*/,
                                           gdb_opts));

        Command command;
        command.value ("set breakpoint pending auto");
        queue_command (command);
        //tell the linker to do all relocations at program load
        //time so that some "step into" don't take for ever.
        //On GDB, it seems that stepping into a function that is
        //in a share lib takes stepping through GNU ld, so it can take time.
        const char *nmv_dont_ld_bind_now = g_getenv ("NMV_DONT_LD_BIND_NOW");
        if (!nmv_dont_ld_bind_now || !atoi (nmv_dont_ld_bind_now)) {
            LOG_DD ("setting LD_BIND_NOW=1");
            queue_command (Command ("set env LD_BIND_NOW environment variable to 1"));
        } else {
            LOG_DD ("not setting LD_BIND_NOW environment variable ");
        }
    }
    if (a_pid == (unsigned int)m_priv->gdb_pid) {
        return false;
    }
    queue_command (Command ("attach-to-program",
                            "attach " + UString::from_int (a_pid)));
    queue_command (Command ("info proc"));
    set_tty_path (a_tty_path);
    return true;
}

bool
GDBEngine::attach_to_remote_target (const UString &a_host,
				    unsigned a_port)
{
    queue_command (Command ("-target-select remote " + a_host +
                            ":" + UString::from_int (a_port)));
    return true;
}

bool
GDBEngine::attach_to_remote_target (const UString &a_serial_line)
{
    queue_command (Command ("-target-select remote " + a_serial_line));
    return true;
}

void
GDBEngine::detach_from_target (const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    queue_command (Command ("detach-from-target", "-target-detach", a_cookie));
}

void
GDBEngine::disconnect_from_remote_target (const UString &a_cookie)
{
      LOG_FUNCTION_SCOPE_NORMAL_DD;

    queue_command (Command ("disconnect-from-remote-target",
			    "-target-disconnect", a_cookie));
}

bool
GDBEngine::is_attached_to_target () const
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    LOG_DD ("is_attached: " << (int)m_priv->is_attached);
    return m_priv->is_gdb_running () && m_priv->is_attached;
}

void
GDBEngine::set_attached_to_target (bool a_is_attached)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    m_priv->is_attached = a_is_attached;
}

void
GDBEngine::set_tty_path (const UString &a_tty_path)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    m_priv->set_tty_path (a_tty_path);
}

void
GDBEngine::add_env_variables (const map<UString, UString> &a_vars)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    THROW_IF_FAIL (m_priv->is_gdb_running ());

    m_priv->env_variables = a_vars;

    Command command;
    map<UString, UString>::const_iterator it;
    for (it = a_vars.begin (); it != a_vars.end (); ++it) {
        command.value ("set environment " + it->first + " " + it->second);
        queue_command (command);
    }
}

map<UString, UString>&
GDBEngine::get_env_variables ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    return m_priv->env_variables;
}

const UString&
GDBEngine::get_target_path ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    return m_priv->exe_path;
}

IDebugger::State
GDBEngine::get_state () const
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;


    LOG_DD ("state: " << m_priv->state);
    return m_priv->state;
}

int
GDBEngine::get_current_frame_level () const
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;


    LOG_DD ("frame level: " << m_priv->cur_frame_level);
    return m_priv->cur_frame_level;
}

void
GDBEngine::set_current_frame_level (int a_level)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;


    LOG_DD ("cur frame level: " << (int) a_level);
    m_priv->cur_frame_level = a_level;
}

const Address&
GDBEngine::get_current_frame_address () const
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;


    return m_priv->cur_frame_address;
}

void
GDBEngine::set_current_frame_address (const Address &a_address)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;


    m_priv->cur_frame_address = a_address;
}

void
GDBEngine::get_mi_thread_and_frame_location (UString &a_str) const
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    UString frame_level =
        "--frame " + UString::from_int (get_current_frame_level ());

    a_str = "--thread " + UString::from_int (get_current_thread ())
            + " " + frame_level;

    LOG_DD ("a_str: " << a_str);
}

void
GDBEngine::get_mi_thread_location (UString &a_str) const
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    a_str = "--thread " + UString::from_int (get_current_thread ());
    LOG_DD ("a_str: " << a_str);
}

void
GDBEngine::init_output_handlers ()
{
    m_priv->output_handler_list.add
                (OutputHandlerSafePtr (new OnStreamRecordHandler (this)));
    m_priv->output_handler_list.add
                (OutputHandlerSafePtr (new OnDetachHandler (this)));
    m_priv->output_handler_list.add
                (OutputHandlerSafePtr (new OnStoppedHandler (this)));
    m_priv->output_handler_list.add
                (OutputHandlerSafePtr (new OnBreakpointHandler (this)));
    m_priv->output_handler_list.add
                (OutputHandlerSafePtr (new OnCommandDoneHandler (this)));
    m_priv->output_handler_list.add
                (OutputHandlerSafePtr (new OnRunningHandler (this)));
    m_priv->output_handler_list.add
                (OutputHandlerSafePtr (new OnConnectedHandler (this)));
    m_priv->output_handler_list.add
                (OutputHandlerSafePtr (new OnFramesListedHandler (this)));
    m_priv->output_handler_list.add
            (OutputHandlerSafePtr (new OnFramesParamsListedHandler (this)));
    m_priv->output_handler_list.add
            (OutputHandlerSafePtr (new OnInfoProcHandler (this)));
    m_priv->output_handler_list.add
        (OutputHandlerSafePtr (new OnLocalVariablesListedHandler (this)));
    m_priv->output_handler_list.add
        (OutputHandlerSafePtr (new OnGlobalVariablesListedHandler (this)));
    m_priv->output_handler_list.add
            (OutputHandlerSafePtr (new OnResultRecordHandler (this)));
    m_priv->output_handler_list.add
            (OutputHandlerSafePtr (new OnVariableTypeHandler (this)));
    m_priv->output_handler_list.add
            (OutputHandlerSafePtr (new OnSignalReceivedHandler (this)));
    m_priv->output_handler_list.add
            (OutputHandlerSafePtr (new OnErrorHandler (this)));
     m_priv->output_handler_list.add
            (OutputHandlerSafePtr (new OnDisassembleHandler (this)));
    m_priv->output_handler_list.add
            (OutputHandlerSafePtr (new OnThreadListHandler (this)));
    m_priv->output_handler_list.add
            (OutputHandlerSafePtr (new OnThreadSelectedHandler (this)));
    m_priv->output_handler_list.add
            (OutputHandlerSafePtr (new OnFileListHandler (this)));
    m_priv->output_handler_list.add
            (OutputHandlerSafePtr (new OnCurrentFrameHandler (this)));
    m_priv->output_handler_list.add
            (OutputHandlerSafePtr (new OnRegisterNamesListedHandler (this)));
    m_priv->output_handler_list.add
            (OutputHandlerSafePtr (new OnChangedRegistersListedHandler (this)));
    m_priv->output_handler_list.add
            (OutputHandlerSafePtr (new OnRegisterValuesListedHandler (this)));
    m_priv->output_handler_list.add
            (OutputHandlerSafePtr (new OnReadMemoryHandler (this)));
    m_priv->output_handler_list.add
            (OutputHandlerSafePtr (new OnSetMemoryHandler (this)));
    m_priv->output_handler_list.add
            (OutputHandlerSafePtr (new OnCreateVariableHandler (this)));
    m_priv->output_handler_list.add
            (OutputHandlerSafePtr (new OnDeleteVariableHandler (this)));
    m_priv->output_handler_list.add
            (OutputHandlerSafePtr (new OnUnfoldVariableHandler (this)));
    m_priv->output_handler_list.add
            (OutputHandlerSafePtr (new OnListChangedVariableHandler (this)));
}

sigc::signal<void, Output&>&
GDBEngine::pty_signal () const
{
    return m_priv->pty_signal;
}

sigc::signal<void, Output&>&
GDBEngine::stderr_signal () const
{
    return m_priv->stderr_signal;
}

sigc::signal<void, CommandAndOutput&>&
GDBEngine::stdout_signal () const
{
    return m_priv->stdout_signal;
}

sigc::signal<void>&
GDBEngine::engine_died_signal () const
{
    return m_priv->gdb_died_signal;
}

sigc::signal<void, const UString&>&
GDBEngine::console_message_signal () const
{
    return m_priv->console_message_signal;
}

sigc::signal<void, const UString&>&
GDBEngine::target_output_message_signal () const
{
    return m_priv->target_output_message_signal;
}

sigc::signal<void, const UString&>&
GDBEngine::log_message_signal () const
{
    return m_priv->log_message_signal;
}

sigc::signal<void, const UString&, const UString&>&
GDBEngine::command_done_signal () const
{
    return m_priv->command_done_signal;
}

sigc::signal<void>&
GDBEngine::connected_to_server_signal () const
{
    return m_priv->connected_to_server_signal;
}

sigc::signal<void>&
GDBEngine::detached_from_target_signal () const
{
    return m_priv->detached_from_target_signal;
}

sigc::signal<void, const IDebugger::Breakpoint&, int, const UString&>&
GDBEngine::breakpoint_deleted_signal () const
{
    return m_priv->breakpoint_deleted_signal;
}

sigc::signal<void, const map<int, IDebugger::Breakpoint>&, const UString&>&
GDBEngine::breakpoints_list_signal () const
{
    return m_priv->breakpoints_list_signal;
}

sigc::signal<void,
             const std::pair<int, const IDebugger::Breakpoint&>&,
             const UString& /*cookie*/>&
GDBEngine::breakpoint_set_signal () const
{
    return m_priv->breakpoint_set_signal;
}

sigc::signal<void, const vector<IDebugger::OverloadsChoiceEntry>&, const UString&>&
GDBEngine::got_overloads_choice_signal () const
{
    return m_priv->got_overloads_choice_signal;
}

sigc::signal<void, IDebugger::StopReason,
             bool, const IDebugger::Frame&,
             int, int, const UString&>&
GDBEngine::stopped_signal () const
{
    return m_priv->stopped_signal;
}

sigc::signal<void, const list<int>, const UString& >&
GDBEngine::threads_listed_signal () const
{
    return m_priv->threads_listed_signal;
}


sigc::signal<void, const vector<UString>&, const UString&>&
GDBEngine::files_listed_signal () const
{
    return m_priv->files_listed_signal;
}

sigc::signal<void, int, const IDebugger::Frame* const, const UString&>&
GDBEngine::thread_selected_signal () const
{
    return m_priv->thread_selected_signal;
}

sigc::signal<void, const vector<IDebugger::Frame>&, const UString&>&
GDBEngine::frames_listed_signal () const
{
    return m_priv->frames_listed_signal;
}

sigc::signal<void, int, const UString&>&
GDBEngine::got_target_info_signal () const
{
    return m_priv->got_target_info_signal;
}

sigc::signal<void, const map< int, list<IDebugger::VariableSafePtr> >&, const UString&>&
GDBEngine::frames_arguments_listed_signal () const
{
    return m_priv->frames_arguments_listed_signal;
}

sigc::signal<void, const IDebugger::Frame&, const UString&> &
GDBEngine::current_frame_signal () const
{
    return m_priv->current_frame_signal;
}

sigc::signal<void, const list<IDebugger::VariableSafePtr>&, const UString& >&
GDBEngine::local_variables_listed_signal () const
{
    return m_priv->local_variables_listed_signal;
}

sigc::signal<void, const list<IDebugger::VariableSafePtr>&, const UString& >&
GDBEngine::global_variables_listed_signal () const
{
    return m_priv->global_variables_listed_signal;
}

sigc::signal<void,
             const UString&,
             const IDebugger::VariableSafePtr,
             const UString&>&
GDBEngine::variable_value_signal () const
{
    return m_priv->variable_value_signal;
}

sigc::signal<void, const IDebugger::VariableSafePtr, const UString&>&
GDBEngine::variable_value_set_signal () const
{
    return m_priv->variable_value_set_signal;
}

sigc::signal<void, const UString&, const IDebugger::VariableSafePtr, const UString&>&
GDBEngine::pointed_variable_value_signal () const
{
    return m_priv->pointed_variable_value_signal;
}

sigc::signal<void, const UString&, const UString&, const UString&>&
GDBEngine::variable_type_signal () const
{
    return m_priv->variable_type_signal;
}

sigc::signal<void, const IDebugger::VariableSafePtr, const UString&>&
GDBEngine::variable_type_set_signal () const
{
    return m_priv->variable_type_set_signal;
}

sigc::signal<void, const IDebugger::VariableSafePtr, const UString&>&
GDBEngine::variable_dereferenced_signal () const
{
    return m_priv->variable_dereferenced_signal;
}

sigc::signal<void, const std::map<IDebugger::register_id_t, UString>&, const UString& >&
GDBEngine::register_names_listed_signal () const
{

    return m_priv->register_names_listed_signal;
}

sigc::signal<void, const std::map<IDebugger::register_id_t, UString>&, const UString& >&
GDBEngine::register_values_listed_signal () const
{

    return m_priv->register_values_listed_signal;
}

sigc::signal<void, const UString&, const UString&, const UString& >&
GDBEngine::register_value_changed_signal () const
{

    return m_priv->register_value_changed_signal;
}

sigc::signal<void, const std::list<IDebugger::register_id_t>&, const UString& >&
GDBEngine::changed_registers_listed_signal () const
{

    return m_priv->changed_registers_listed_signal;
}

sigc::signal <void, size_t, const std::vector<uint8_t>&, const UString&>&
GDBEngine::read_memory_signal () const
{

    return m_priv->read_memory_signal;
}

sigc::signal <void, size_t, const std::vector<uint8_t>&, const UString& >&
GDBEngine::set_memory_signal () const
{

    return m_priv->set_memory_signal;
}

sigc::signal<void>&
GDBEngine::running_signal () const
{
    return m_priv->running_signal;
}

sigc::signal<void, const UString&, const UString&>&
GDBEngine::signal_received_signal () const
{
    return m_priv->signal_received_signal;
}

sigc::signal<void, const UString&>&
GDBEngine::error_signal () const
{
    return m_priv->error_signal;
}

sigc::signal<void>&
GDBEngine::program_finished_signal () const
{
    return m_priv->program_finished_signal;
}

sigc::signal<void,
             const common::DisassembleInfo&,
             const std::list<common::Asm>&,
             const UString& /*cookie*/>&
GDBEngine::instructions_disassembled_signal () const
 {
    THROW_IF_FAIL (m_priv);
    return m_priv->instructions_disassembled_signal;
 }

sigc::signal<void, IDebugger::State>&
GDBEngine::state_changed_signal () const
{
    return m_priv->state_changed_signal;
}

sigc::signal<void, const IDebugger::VariableSafePtr, const UString&>&
GDBEngine::variable_created_signal () const
{
    return m_priv->variable_created_signal;
}

sigc::signal<void, const IDebugger::VariableSafePtr, const UString&>&
GDBEngine::variable_deleted_signal () const
{
    return m_priv->variable_deleted_signal;
}

sigc::signal<void, const IDebugger::VariableSafePtr, const UString&>&
GDBEngine::variable_unfolded_signal () const
{
    return m_priv->variable_unfolded_signal;
}

sigc::signal<void, const IDebugger::VariableSafePtr, const UString&>&
GDBEngine::variable_expression_evaluated_signal () const
{
    return m_priv->variable_expression_evaluated_signal;
}

sigc::signal<void, const list<IDebugger::VariableSafePtr>&, const UString&>&
GDBEngine::changed_variables_signal () const
{
    return m_priv->changed_variables_signal;
}

sigc::signal<void, IDebugger::VariableSafePtr, const UString&>&
GDBEngine::assigned_variable_signal () const
{
    return m_priv->assigned_variable_signal;
}

//******************
//<signal handlers>
//******************
void
GDBEngine::on_debugger_stdout_signal (CommandAndOutput &a_cao)
{
    //****************************************
    //call the output handlers so that we can
    //emit proper, more precise signal
    //to give greater benefit to the clients.
    //****************************************

    NEMIVER_TRY


    m_priv->output_handler_list.submit_command_and_output (a_cao);

    NEMIVER_CATCH_NOX
}

void
GDBEngine::on_got_target_info_signal (int a_pid, const UString &a_exe_path)
{
    NEMIVER_TRY

    LOG_DD ("target pid: '" << (int) a_pid << "'");
    m_priv->target_pid = a_pid;
    m_priv->exe_path = a_exe_path;

    NEMIVER_CATCH_NOX
}

void
GDBEngine::on_stopped_signal (IDebugger::StopReason a_reason,
                              bool a_has_frame,
                              const IDebugger::Frame &a_frame,
                              int a_thread_id,
                              int a_bkpt_num,
                              const UString &a_cookie)
{
    if (a_has_frame || a_frame.line ()
        || a_thread_id || a_cookie.empty ()
        || a_bkpt_num) {
        //keep compiler happy
    }

    NEMIVER_TRY

    if (a_reason == IDebugger::EXITED_SIGNALLED
        || a_reason == IDebugger::EXITED_NORMALLY
        || a_reason == IDebugger::EXITED) {
        return;
    }

    m_priv->is_attached = true;

    NEMIVER_CATCH_NOX
}

void
GDBEngine::on_detached_from_target_signal ()
{
    NEMIVER_TRY


    m_priv->is_attached = false;

    NEMIVER_CATCH_NOX
}

void
GDBEngine::on_program_finished_signal ()
{
    NEMIVER_TRY


    m_priv->is_attached = false;

    NEMIVER_CATCH_NOX
}

//******************
//</signal handlers>
//******************
void
GDBEngine::init ()
{
    stdout_signal ().connect (sigc::mem_fun
            (*this, &GDBEngine::on_debugger_stdout_signal));
    got_target_info_signal ().connect (sigc::mem_fun
            (*this, &GDBEngine::on_got_target_info_signal));
    stopped_signal ().connect (sigc::mem_fun
            (*this, &GDBEngine::on_stopped_signal));
    detached_from_target_signal ().connect (sigc::mem_fun
            (*this, &GDBEngine::on_detached_from_target_signal));
    program_finished_signal ().connect (sigc::mem_fun
            (*this, &GDBEngine::on_program_finished_signal));

    init_output_handlers ();
}

// Put here the initialization that must happen once we are called
// for initialization, i.e, after we are sure we have stuff like
// conf manager ready, etc ...
void
GDBEngine::do_init (IConfMgrSafePtr a_conf_mgr)
{
    m_priv->conf_mgr = a_conf_mgr;

    m_priv->read_default_config ();

    m_priv->get_conf_mgr ()->value_changed_signal ().connect (sigc::mem_fun
        (*m_priv, &Priv::on_conf_key_changed_signal));
}

IConfMgr&
GDBEngine::get_conf_mgr ()
{


    return *m_priv->get_conf_mgr ();
}

map<UString, UString>&
GDBEngine::properties ()
{
    return m_priv->properties;
}

void
GDBEngine::set_event_loop_context (const Glib::RefPtr<Glib::MainContext> &a_ctxt)
{
    m_priv->set_event_loop_context (a_ctxt);
}

void
GDBEngine::run_loop_iterations (int a_nb_iters)
{

    m_priv->run_loop_iterations_real (a_nb_iters);
}

void
GDBEngine::set_state (IDebugger::State a_state)
{

    m_priv->set_state (a_state);
}


void
GDBEngine::execute_command (const Command &a_command)
{
    THROW_IF_FAIL (m_priv && m_priv->is_gdb_running ());
    queue_command (a_command);
}

bool
GDBEngine::queue_command (const Command &a_command)
{
    return m_priv->queue_command (a_command);
}

bool
GDBEngine::busy () const
{
    return false;
}

const UString&
GDBEngine::get_debugger_full_path () const
{
    return m_priv->get_debugger_full_path ();
}

void
GDBEngine::set_debugger_parameter (const UString &a_name,
                                   const UString &a_value)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    m_priv->set_debugger_parameter (a_name, a_value);
}

void
GDBEngine::set_solib_prefix_path (const UString &a_name)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    if (a_name.empty ())
      return;
    set_debugger_parameter ("solib-absolute-prefix", a_name);
}

void
GDBEngine::do_continue (const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    Command command ("do-continue",
                     "-exec-continue ",
                     a_cookie);
    queue_command (command);
}

void
GDBEngine::run (const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    Command command ("run",
                     "-exec-run",
                     a_cookie);
    queue_command (command);
}

void
GDBEngine::get_target_info (const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    queue_command (Command ("get-target-info", "info proc", a_cookie));
}

ILangTraitSafePtr
GDBEngine::create_language_trait ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    //TODO: detect the actual language of the target being
    //debugged and create a matching language trait on the fly for it.
    //For now, let's say we just debug only c++
    DynamicModule::Loader* loader =
        get_dynamic_module ().get_module_loader ();
    THROW_IF_FAIL (loader);
    DynamicModuleManager *mgr = loader->get_dynamic_module_manager ();
    THROW_IF_FAIL (mgr);

    ILangTraitSafePtr trait =
        mgr->load_iface<ILangTrait> ("cpptrait", "ILangTrait");

    return trait;
}

ILangTrait&
GDBEngine::get_language_trait ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    if (!m_priv->lang_trait) {
        m_priv->lang_trait = create_language_trait ();
    }
    THROW_IF_FAIL (m_priv->lang_trait);
    return *m_priv->lang_trait;
}

/// Dectect if the variable should be editable or not.
/// For now, only scalar variable are said to be editable.
/// An aggregate (array or structure) is not editable yet, as
/// we need lots of hacks to detect for instance if an array is a string,
/// for instance.
/// \param a_var the variable to consider.
/// \return true if the variable is editable, false otherwise.
bool
GDBEngine::is_variable_editable (const VariableSafePtr a_var) const
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    // TODO: make this depend on the current language trait, maybe ?
    // Also, be less strict and allow editing of certain aggregate
    // variables.
    if (!a_var)
        return false;
    if (!a_var->value ().empty ()
        && !const_cast<GDBEngine*>
                (this)->get_language_trait ().is_variable_compound (a_var))
        return true;
    return false;
}

bool
GDBEngine::stop_target ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    if (!m_priv->is_gdb_running ()) {
        LOG_ERROR_D ("GDB is not running", NMV_DEFAULT_DOMAIN);
        return false;
    }

    if (!m_priv->gdb_pid) {
        return false;
    }

    //return  (kill (m_priv->target_pid, SIGINT) == 0);
    return  (kill (m_priv->gdb_pid, SIGINT) == 0);
}

void
GDBEngine::exit_engine ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;


    //**************************************************
    //don't queue the command, send it the gdb directly,
    //because we want the engine to exit _now_
    //okay we SHOULD NEVER DO THIS but exit engine is an emergency case.
    //**************************************************

    //erase the pending commands queue. this is bad but well, gdb is getting
    //killed anyway.
    m_priv->queued_commands.clear ();

    //send the lethal command and run the event loop to flush everything.
    m_priv->issue_command (Command ("quit"), false);
    set_state (IDebugger::NOT_STARTED);
}

void
GDBEngine::step_in (const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    Command command ("step-in",
                     "-exec-step",
                     a_cookie);
    queue_command (command);
}

void
GDBEngine::step_out (const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    Command command ("step-out",
                     "-exec-finish",
                     a_cookie);
    queue_command (command);
}

void
GDBEngine::step_over (const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    Command command ("step-over",
                     "-exec-next ",
                     a_cookie);
    queue_command (command);
}

void
GDBEngine::step_over_asm (const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    Command command ("step-over-asm",
                     "-exec-next-instruction",
                     a_cookie);
    queue_command (command);
}

void
GDBEngine::step_in_asm (const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    Command command ("step-in-asm",
                     "-exec-step-instruction",
                     a_cookie);
    queue_command (command);
}

void
GDBEngine::continue_to_position (const UString &a_path,
                                 gint a_line_num,
                                 const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    queue_command (Command ("continue-to-position",
                            "-exec-until "
                            + a_path
                            + ":"
                            + UString::from_int (a_line_num),
                            a_cookie));
}

/// Set a breakpoint at a location in the inferior.
///
/// \param a_loc the location of the breakpoint.
///
/// \param a_condition the condition of the breakpoin.  If there is no
/// condition, the argument should be "".
///
/// \param a_ignore_count the number of time the breakpoint should be
/// hit before execution of the inferior is stopped.
///
/// \param a_slot a callback slot to be invoked once the breakpoint is
/// set.
///
/// \param a_cookie a string to be passed to
/// IDebugger::breakpoints_set_signals once that signal emitted as a
/// result of the breakpoint being set.  Note both a_slot and
/// IDebugger::breakpoint_set_signals are 'called' upon breakpoint
/// actual setting.  Eventually, IDebugger::breakpoints_set_signals
/// should be dropped, so this whole cookie business would disapear.
/// We are still in a transitional period.
void
GDBEngine::set_breakpoint (const Loc &a_loc,
                           const UString &a_condition,
                           gint a_ignore_count,
                           const BreakpointSlot &a_slot,
                           const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    THROW_IF_FAIL (a_loc.kind () != Loc::UNDEFINED_LOC_KIND);

    UString loc_str;

    location_to_string (a_loc, loc_str);

    UString break_cmd = "-break-insert -f ";
    if (!a_condition.empty ()) {
        LOG_DD ("setting breakpoint with condition: " << a_condition);
        break_cmd += " -c \"" + a_condition + "\"";
    } else {
        LOG_DD ("setting breakpoint without condition");
    }

    bool count_point = (a_ignore_count < 0);
    if (!count_point)
        break_cmd += " -i " + UString::from_int (a_ignore_count);

    break_cmd += " " + loc_str;
    string cmd_name = count_point ? "set-countpoint" : "set-breakpoint";
    
    Command command (cmd_name, break_cmd, a_cookie);
    command.set_slot (a_slot);
    queue_command (command);
}

void
GDBEngine::set_breakpoint (const UString &a_path,
                           gint a_line_num,
                           const UString &a_condition,
                           gint a_ignore_count,
                           const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    THROW_IF_FAIL (!a_path.empty ());

    UString break_cmd ("-break-insert -f ");
    if (!a_condition.empty ()) {
        LOG_DD ("setting breakpoint with condition: " << a_condition);
        break_cmd += " -c \"" + a_condition + "\"";
    } else {
        LOG_DD ("setting breakpoint without condition");
    }

    bool count_point = (a_ignore_count < 0);
    if (!count_point)
      break_cmd += " -i " + UString::from_int (a_ignore_count);

    if (!a_path.empty ()) {
        break_cmd += " " + a_path + ":";
    }
    break_cmd += UString::from_int (a_line_num);
    string cmd_name = count_point ? "set-countpoint" : "set-breakpoint";
    queue_command (Command (cmd_name,
			    break_cmd, a_cookie));
}


void
GDBEngine::set_breakpoint (const Address &a_address,
                           const UString &a_condition,
                           gint a_ignore_count,
                           const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    THROW_IF_FAIL (!a_address.empty ());

    UString break_cmd ("-break-insert -f ");
    if (!a_condition.empty ()) {
        LOG_DD ("setting breakpoint with condition: " << a_condition);
        break_cmd += " -c \"" + a_condition + "\"";
    } else {
        LOG_DD ("setting breakpoint without condition");
    }
    bool count_point = a_ignore_count < 0;
    if (!count_point)
      break_cmd += " -i " + UString::from_int (a_ignore_count);
    break_cmd += " *" + (const string&) a_address;

    string cmd_name = count_point ? "set-countpoint" : "set-breakpoint";
    queue_command (Command (cmd_name, break_cmd, a_cookie));
}

void
GDBEngine::enable_breakpoint (gint a_break_num,
                              const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    queue_command (Command ("enable-breakpoint",
                            "-break-enable "
                             + UString::from_int (a_break_num),
                            a_cookie));
    list_breakpoints (a_cookie);
}

void
GDBEngine::disable_breakpoint (gint a_break_num,
                               const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    UString cur_frame;
    queue_command (Command ("disable-breakpoint",
                            "-break-disable "
                            + UString::from_int (a_break_num),
                            a_cookie));
    list_breakpoints (a_cookie);
}

void
GDBEngine::set_breakpoint_ignore_count (gint a_break_num,
                                        gint a_ignore_count,
                                        const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    RETURN_IF_FAIL (a_break_num >= 0 && a_ignore_count >= 0);

    Command command ("set-breakpoint-ignore-count",
                     "-break-after " + UString::from_int (a_break_num)
                     + " " + UString::from_int (a_ignore_count),
                     a_cookie);
    queue_command (command);
    list_breakpoints (a_cookie);
}

void
GDBEngine::set_breakpoint_condition (gint a_break_num,
                                     const UString &a_condition,
                                     const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    RETURN_IF_FAIL (a_break_num >= 0);

    Command command ("set-breakpoint-condition",
                     "-break-condition " + UString::from_int (a_break_num)
                     + " " + a_condition, a_cookie);
    queue_command (command);
    list_breakpoints (a_cookie);
}

void
GDBEngine::enable_countpoint (gint a_break_num,
			      bool a_yes,
			      const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    typedef map<int, IDebugger::Breakpoint> BPMap;
    BPMap &bp_cache = get_cached_breakpoints ();
    BPMap::const_iterator nil = bp_cache.end ();

    if (bp_cache.find (a_break_num) == nil)
        return;

    std::ostringstream command_str;
    UString command_name;

    if (a_yes) {
        command_str << "-break-commands " << a_break_num << " \"continue\"";
        command_name = "enable-countpoint";
    } else {
        command_str << "-break-commands " << a_break_num << " \"\"";
        command_name = "disable-countpoint";
    }
    Command command (command_name, command_str.str (), a_cookie);
    command.tag2 (a_break_num);
    queue_command (command);
}

bool
GDBEngine::is_countpoint (gint a_bp_num) const
{
    Breakpoint bp;
    if (get_breakpoint_from_cache (a_bp_num, bp))
        return is_countpoint (bp);
    return false;
}

bool
GDBEngine::is_countpoint (const Breakpoint &a_breakpoint) const
{
    return (a_breakpoint.type () == Breakpoint::COUNTPOINT_TYPE);
}

void
GDBEngine::delete_breakpoint (const UString &a_path,
                              gint a_line_num,
                              const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    queue_command (Command ("delete-breakpoint",
                            "-break-delete "
                            + a_path
                            + ":"
                            + UString::from_int (a_line_num),
                            a_cookie));
}

void
GDBEngine::set_watchpoint (const UString &a_expression,
                           bool a_write, bool a_read,
                           const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    if (a_expression.empty ())
        return;

    string cmd_str = "-break-watch";

    if (a_write && a_read)
        cmd_str += " -a";
    else if (a_read == true)
        cmd_str += " -r";

    cmd_str += " " + a_expression;

    Command command ("set-watchpoint", cmd_str, a_cookie);
    queue_command (command);
    list_breakpoints (a_cookie);
}

void
GDBEngine::set_breakpoint (const UString &a_func_name,
                           const UString &a_condition,
                           gint a_ignore_count,
                           const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    UString break_cmd;
    break_cmd += "-break-insert -f ";
    if (!a_condition.empty ()) {
        LOG_DD ("setting breakpoint with condition: " << a_condition);
        break_cmd += " -c \"" + a_condition + "\"";
    } else {
        LOG_DD ("setting breakpoint without condition");
    }
    break_cmd += " -i " + UString::from_int (a_ignore_count);
    break_cmd +=  " " + a_func_name;

    queue_command (Command ("set-breakpoint", break_cmd, a_cookie));
}

void
GDBEngine::list_breakpoints (const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    queue_command (Command ("list-breakpoints", "-break-list", a_cookie));
}

map<int, IDebugger::Breakpoint>&
GDBEngine::get_cached_breakpoints ()
{

    return m_priv->cached_breakpoints;
}

bool
GDBEngine::get_breakpoint_from_cache (int a_num,
                                      IDebugger::Breakpoint &a_bp) const
{
    typedef map<int, IDebugger::Breakpoint> BPMap;
    BPMap &bp_cache =
        const_cast<GDBEngine*> (this)->get_cached_breakpoints ();
    BPMap::const_iterator nil = bp_cache.end ();
    BPMap::iterator it;

    if ((it = bp_cache.find (a_num)) == nil)
        return false;
    a_bp = it->second;
    return true;
}

void
GDBEngine::append_breakpoint_to_cache (const IDebugger::Breakpoint &a_break)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    typedef map<int, IDebugger::Breakpoint> BpMap;
    typedef BpMap::iterator BpIt;
    typedef BpMap::const_iterator ConstBpIt;

    BpMap &bp_cache = m_priv->cached_breakpoints;
    BpIt cur, nil = bp_cache.end ();
    bool preserve_count_point = false;

    if (a_break.type () == IDebugger::Breakpoint::COUNTPOINT_TYPE) {
        LOG_DD ("breakpoint number "
                << a_break.number ()
                << " is a count point");
    } else {
        LOG_DD ("breakpoint number "
                << a_break.number ()
                << " is not a count point");
    }
        
    // First, let's see if a_break is already in our cache.
    cur = bp_cache.find (a_break.number ());
    if (cur != nil) {
        // So the breakpoint a_break is already in the
        // cache. If we flagged it as a countpoint, let's remember
        // that so that we don't loose the countpointness of the
        // breakpoint in the cache when we update its state with
        // the content of a_break.
        if (cur->second.type () == IDebugger::Breakpoint::COUNTPOINT_TYPE)
            preserve_count_point = true;
    }

    // So now is the cache updating time.

    // If the breakpoint a_break was already in the cache and
    // was flagged as a countpoint, let's preserve that
    // countpointness attribute.
    if (cur != nil) {
        cur->second = a_break;
        if (preserve_count_point) {
            cur->second.type (IDebugger::Breakpoint::COUNTPOINT_TYPE);
            LOG_DD ("propagated countpoinness to bp number " << cur->first);
        }
    } else {
        // Its the first time we are adding this breakpoint to the
        // cache. So its countpointness is going to be kept
        // anyway.
        
        std::pair<BpIt,bool> where =
            bp_cache.insert (BpMap::value_type (a_break.number (), a_break));

        if (preserve_count_point) {
            where.first->second.type (IDebugger::Breakpoint::COUNTPOINT_TYPE);
            LOG_DD ("propagated countpoinness to bp number " << cur->first);
        }
    }
}

/// Append a set of breakpoints to our breakpoint cache.
/// This function supports the countpoint feature. That is, as a
/// countpoint is a concept not known to GDB, we have to mark an
/// otherwise normal breakpoint [from GDB's standpoint] as a
/// countpoint, in our cache. So whenever we see a breakpoint that we
/// have previously marked as a countpoint in our cache, we make sure
/// to not loose the countpointness.
/// \param a_breaks the set of breakpoints to append to the cache.
void
GDBEngine::append_breakpoints_to_cache
                            (const map<int, IDebugger::Breakpoint> &a_breaks)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    map<int, IDebugger::Breakpoint>::const_iterator iter;
    for (iter = a_breaks.begin (); iter != a_breaks.end (); ++iter)
        append_breakpoint_to_cache (iter->second);
}


void
GDBEngine::set_catch (const UString &a_event,
		      const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    queue_command (Command ("catch",
                            "catch " + a_event,
                            a_cookie));
    // explicitly request the breakpoints to be listed otherwise the newly added
    // catchpoint won't show up in the breakpoint list
    list_breakpoints(a_cookie);
}



void
GDBEngine::list_threads (const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    queue_command (Command ("list-threads", "-thread-list-ids", a_cookie));
}

void
GDBEngine::select_thread (unsigned int a_thread_id,
                          const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    THROW_IF_FAIL (a_thread_id);
    queue_command (Command ("select-thread", "-thread-select "
                            + UString::from_int (a_thread_id),
                            a_cookie));
}

unsigned int
GDBEngine::get_current_thread () const
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;


    return m_priv->cur_thread_num;
}


void
GDBEngine::choose_function_overload (int a_overload_number,
                                     const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    if (a_cookie.empty ()) {}

    m_priv->issue_command (UString::from_int (a_overload_number), false);
}

void
GDBEngine::choose_function_overloads (const vector<int> &a_nums,
                                      const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    UString str;

    if (a_cookie.empty ()) {}

    for (unsigned int i = 0; i < a_nums.size (); ++i) {
        str += UString::from_int (a_nums[i]) + " ";
    }
    if (!str.empty ())
        m_priv->issue_command (str, false);
}

void
GDBEngine::delete_breakpoint (gint a_break_num,
                              const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    queue_command (Command ("delete-breakpoint",
                            "-break-delete " + UString::from_int (a_break_num),
                            a_cookie));
}

/// Lists the frames which numbers are in a given range.
///
/// Upon completion of the GDB side of this command, the signal
/// Priv::frames_listed_signal is emitted.  The callback slot
/// given in parameter is called as well.
///
/// \param a_low_frame the lower bound of the frame range to list.
///
/// \param a_high_frame the upper bound of the frame range to list.
///
/// \param a_slot a callback slot to be called upon completion of
/// the GDB-side command.
///
/// \param a_cookie a string to be passed to the
/// Priv::frames_listed_signal. 
void
GDBEngine::list_frames (int a_low_frame,
                        int a_high_frame,
                        const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    m_priv->list_frames (a_low_frame, a_high_frame, a_cookie);
}

/// A subroutine of the list_frame overload above.
/// 
/// Lists the frames which numbers are in a given range.
///
/// Upon completion of the GDB side of this command, the signal
/// Priv::frames_listed_signal is emitted.  The callback slot
/// given in parameter is called as well.
///
/// \param a_low_frame the lower bound of the frame range to list.
///
/// \param a_high_frame the upper bound of the frame range to list.
///
/// \param a_slot a callback slot to be called upon completion of
/// the GDB-side command.
///
/// \param a_cookie a string to be passed to the
/// Priv::frames_listed_signal. 
void
GDBEngine::list_frames (int a_low_frame,
                        int a_high_frame,
                        const FrameVectorSlot &a_slot,
                        const UString &a_cookie)

{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    m_priv->list_frames (a_low_frame,
                         a_high_frame,
                         a_slot, a_cookie);
}

void
GDBEngine::select_frame (int a_frame_id,
                         const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    Command command ("select-frame",
                     "-stack-select-frame "
                        + UString::from_int (a_frame_id),
                     a_cookie);
    command.tag2 (a_frame_id);
    queue_command (command);

}

/// List the arguments of the frames which numbers are in a given
/// range.
///
/// Upon completion of the GDB-side of this command the signal
/// GDBEngine::frames_arguments_listed_signal is emitted.
/// 
/// \param a_low_frame the lower bound of the range of frames which
/// arguments to list.
///
/// \param a_high_frame the uper bound of the range of frames which
/// arguments to list.
///
/// \param a_cookie a string to be passed to the
/// GDBEngine::frames_arguments_listed_signal signal.
void
GDBEngine::list_frames_arguments (int a_low_frame,
                                  int a_high_frame,
                                  const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    list_frames_arguments (a_low_frame, a_high_frame,
                           &null_frame_args_slot, a_cookie);
}

/// List the arguments of the frames which numbers are in a given
/// range.
///
/// Upon completion of the GDB-side command emitted by this function
/// the signal GDBEngine::frames_arguments_listed_signal is emitted.
/// In addition, a callback slot passed in parameter of this function
/// is invoked upon completion.
/// 
/// \param a_low_frame the lower bound of the range of frames which
/// arguments to list.
///
/// \param a_high_frame the uper bound of the range of frames which
/// arguments to list.
///
/// \param a_slot a callback slot called upon completion of the
/// GDB-side command emitted by this function.
///
/// \param a_cookie a string to be passed to the
/// GDBEngine::frames_arguments_listed_signal signal.
void
GDBEngine::list_frames_arguments (int a_low_frame,
                                  int a_high_frame,
                                  const FrameArgsSlot &a_slot,
                                  const UString &a_cookie)
{
    UString cmd_str;

    if (a_low_frame < 0 || a_high_frame < 0) {
        cmd_str = "-stack-list-arguments 1";
    } else {
        cmd_str = UString ("-stack-list-arguments 1 ")
            + UString::from_int (a_low_frame)
            + " "
            + UString::from_int (a_high_frame);
    }
    Command command ("list-frames-arguments", cmd_str, a_cookie);
    command.set_slot (a_slot);
    queue_command (command);
}

void
GDBEngine::list_local_variables (const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    Command command ("list-local-variables",
                     "-stack-list-locals 2",
                     a_cookie);
    queue_command (command);
}

void
GDBEngine::list_global_variables (const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    Command command ("list-global-variables",
                     "info variables",
                     a_cookie);
    queue_command (command);
}

void
GDBEngine::evaluate_expression (const UString &a_expr,
                                const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    if (a_expr == "") {return;}

    Command command ("evaluate-expression",
                     "-data-evaluate-expression " + a_expr,
                     a_cookie);
    queue_command (command);
}

void
GDBEngine::call_function (const UString &a_expr,
                          const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    if (a_expr.empty ()) {return;}

    Command command ("call-function",
                     "-data-evaluate-expression " + a_expr,
                     a_cookie);
    queue_command (command);
}

void
GDBEngine::print_variable_value (const UString &a_var_name,
                                 const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    if (a_var_name == "") {
        LOG_ERROR ("got empty variable name");
        return;
    }

    Command command ("print-variable-value",
                     "-data-evaluate-expression " + a_var_name,
                     a_cookie);
    command.tag0 ("print-variable-value");
    command.tag1 (a_var_name);

    queue_command (command);
}

void
GDBEngine::get_variable_value (const VariableSafePtr &a_var,
                               const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    RETURN_IF_FAIL (a_var);
    RETURN_IF_FAIL (a_var->name ());

    UString qname;
    a_var->build_qname (qname);

    Command command ("get-variable-value",
                     "-data-evaluate-expression " + qname,
                     a_cookie);
    command.variable (a_var);

    queue_command (command);
}

void
GDBEngine::print_pointed_variable_value (const UString &a_var_name,
                                         const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD
    if (a_var_name == "") {
        LOG_ERROR ("got empty variable name");
        return;
    }

    Command command ("print-pointed-variable-value",
                     "-data-evaluate-expression *" + a_var_name,
                     a_cookie);
    command.tag0 ("print-pointed-variable-value");
    command.tag1 (a_var_name);

    queue_command (command);
}

void
GDBEngine::print_variable_type (const UString &a_var_name,
                                const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    if (a_var_name == "") {return;}

    Command command ("print-variable-type",
                     "ptype " + a_var_name,
                     a_cookie);
    command.tag0 ("print-variable-type");
    command.tag1 (a_var_name);

    queue_command (command);
}

void
GDBEngine::get_variable_type (const VariableSafePtr &a_var,
                              const UString &a_cookie = "")
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    THROW_IF_FAIL (a_var);
    THROW_IF_FAIL (a_var->name () != "");

    UString qname;
    a_var->build_qname (qname);
    LOG_DD ("variable qname: " << qname);
    Command command ("get-variable-type",
                     "ptype " + qname,
                     a_cookie);
    command.variable (a_var);

    queue_command (command);
}

bool
GDBEngine::dereference_variable (const VariableSafePtr &a_var,
                                 const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (a_var);
    THROW_IF_FAIL (!a_var->name ().empty ());

    ILangTrait &lang_trait = get_language_trait ();

    if (!lang_trait.has_pointers ()) {
        LOG_ERROR ("current language does not support pointers");
        return false;
    }

    if (!a_var->type ().empty () &&
        !lang_trait.is_type_a_pointer (a_var->type ())) {
        LOG_ERROR ("The variable you want to dereference is not a pointer:"
                   "name: " << a_var->name ()
                   << ":type: " << a_var->type ());
        return false;
    }

    UString var_qname;
    a_var->build_qname (var_qname);
    THROW_IF_FAIL (!var_qname.empty ());
    Command command ("dereference-variable",
                     "-data-evaluate-expression *" + var_qname,
                     a_cookie);
    command.variable (a_var);

    queue_command (command);
    return true;
}

/// Lists the source files htat make up the executable
void
GDBEngine::list_files (const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    queue_command (Command ("list-files",
                            "-file-list-exec-source-files",
                            a_cookie));
}


/// Extracts proc info from the out of band records
bool
GDBEngine::extract_proc_info (Output &a_output,
                              int &a_pid,
                              UString &a_exe_path)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    if (!a_output.has_out_of_band_record ()) {
        LOG_ERROR_D ("output has no out of band record", NMV_DEFAULT_DOMAIN);
        return false;
    }

    //********************************************
    //search the out of band records
    //that contains the debugger console
    //stream record with the string 'process <pid>'
    //and the one that contains the string 'exe = <exepath>'
    //********************************************
    UString record, process_record, exe_record;
    UString::size_type process_index = 0, exe_index = 0, index = 0;
    list<Output::OutOfBandRecord>::const_iterator record_iter =
                                    a_output.out_of_band_records ().begin ();
    for (;
         record_iter != a_output.out_of_band_records ().end ();
         ++record_iter) {
        if (!record_iter->has_stream_record ()) {continue;}

        record = record_iter->stream_record ().debugger_console ();
        if (record == "") {continue;}

        LOG_DD ("found a debugger console stream record '" << record << "'");

        index = record.find ("process ");
        if (index != Glib::ustring::npos) {
            process_record = record;
            process_index = index;
            LOG_DD ("found process stream record: '" << process_record << "'");
            LOG_DD ("process_index: '" << (int)process_index << "'");
            continue;
        }
        index = record.find ("exe = '");
        if (index != Glib::ustring::npos) {
            exe_record = record;
            exe_index = index;
            continue;
        }
    }
    if (process_record == "" || exe_record == "") {
        LOG_ERROR_DD ("output has no process info");
        return false;
    }

    //extract pid
    process_index += 7;
    UString pid;
    while (process_index < process_record.size ()
           && isspace (process_record[process_index])) {
        ++process_index;
    }
    RETURN_VAL_IF_FAIL (process_index < process_record.size (), false);
    while (process_index < process_record.size ()
           && isdigit (process_record[process_index])) {
        pid += process_record[process_index];
        ++process_index;
    }
    RETURN_VAL_IF_FAIL (process_index < process_record.size (), false);
    LOG_DD ("extracted PID: '" << pid << "'");
    a_pid = atoi (pid.c_str ());

    //extract exe path
    exe_index += 3;
    while (exe_index < exe_record.size ()
           && isspace (exe_record[exe_index])) {
        ++exe_index;
    }
    RETURN_VAL_IF_FAIL (exe_index < exe_record.size (), false);
    RETURN_VAL_IF_FAIL (exe_record[exe_index] == '=', false);
    ++exe_index;
    while (exe_index < exe_record.size ()
           && isspace (exe_record[exe_index])) {
        ++exe_index;
    }
    RETURN_VAL_IF_FAIL (exe_index < exe_record.size (), false);
    RETURN_VAL_IF_FAIL (exe_record[exe_index] == '\'', false);
    ++exe_index;
    UString::size_type exe_path_start = exe_index;

    while (exe_index < exe_record.size ()
           && exe_record[exe_index] != '\'') {
        ++exe_index;
    }
    RETURN_VAL_IF_FAIL (exe_index < exe_record.size (), false);
    UString::size_type exe_path_end = exe_index - 1;
    UString exe_path;
    exe_path.assign (exe_record, exe_path_start, exe_path_end-exe_path_start+1);
    LOG_DD ("extracted exe path: '" << exe_path << "'");
    a_exe_path = exe_path;

    return true;
}

bool
GDBEngine::extract_global_variable_list (Output &a_output,
                                         VarsPerFilesMap &a_vars)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    if (!a_output.has_out_of_band_record ()) {
        LOG_ERROR ("output has no out of band record");
        return false;
    }
    IDebugger::VariableSafePtr var;
    VarsPerFilesMap result;
    list<IDebugger::VariableSafePtr> var_list;

    //*************************************************
    //search the out of band records that
    //contain the debugger console
    //stream record with the string:
    //"File <file-name>:".
    //That stream record is then followed by
    //series of stream records containing the string:
    //"<type of variable> <variable-name>;"
    //*************************************************
    UString str, file_name;
    string var_name, tmp_str;
    SimpleDeclarationPtr simple_decl;
    InitDeclaratorPtr init_decl;
    ParserPtr parser;
    bool found = false;
    list<Output::OutOfBandRecord>::const_iterator oobr_it =
                                    a_output.out_of_band_records ().begin ();
fetch_file:
    var_list.clear ();
    //we are looking for a string of the form "File <file-name>:\n"
    for (; oobr_it != a_output.out_of_band_records ().end (); ++oobr_it) {
        if (!oobr_it->has_stream_record ()) {continue;}

        str = oobr_it->stream_record ().debugger_console ();
        if (str.raw ().compare (0, 5, "File ")) {continue;}

        //we found the string "File <file-name>:\n"
        found = true;
        break;
    }
    if (!found)
        goto out;
    file_name = str.substr (5);
    file_name.chomp ();
    file_name.erase (file_name.length ()-1, 1);
    THROW_IF_FAIL (!file_name.empty ());
    ++oobr_it;

fetch_variable:
    found = false;
    //we are looking for a string that end's up with a ";\n"
    for (; oobr_it != a_output.out_of_band_records ().end ();
         ++oobr_it) {
        if (!oobr_it->has_stream_record ()) {continue;}

        str = oobr_it->stream_record ().debugger_console ();
        if (str.raw ()[str.raw ().length () - 2] != ';'
            || str.raw ()[str.raw ().length () - 1] != '\n') {
            continue;
        }
        found = true;
        break;
    }
    if (!found)
        goto out;
    str.chomp ();
    THROW_IF_FAIL (str.raw ()[str.raw ().length ()-1] == ';');

    //now we must must parse the line to extract its
    //type and name parts.
    LOG_DD ("going to parse variable decl: '" << str.raw () << "'");
    parser.reset (new Parser (str.raw ()));
    simple_decl.reset ();
    if (!parser->parse_simple_declaration (simple_decl)
        || !simple_decl) {
        LOG_ERROR ("declaration parsing failed");
        goto skip_oobr;
    }
    simple_decl->to_string (tmp_str);
    LOG_DD ("parsed decl: '" << tmp_str << "'");

    if (!simple_decl->get_init_declarators ().empty ()) {
        init_decl = *simple_decl->get_init_declarators ().begin ();
        if (!get_declarator_id_as_string (init_decl, var_name)) {
            LOG_ERROR ("could not get declarator id "
                       "as string for parsed decl: "
                       << tmp_str);
            goto skip_oobr;
        }
    } else {
        LOG_ERROR ("got empty init declarator list after parsing: '" << str
                   << "' into: '" << tmp_str << "'");
        goto skip_oobr;
    }
    LOG_DD ("globals: got variable name: " << var_name );

    var.reset (new IDebugger::Variable (var_name));
    var_list.push_back (var);

skip_oobr:
    for (++oobr_it; oobr_it != a_output.out_of_band_records ().end (); ++oobr_it) {
        if (!oobr_it->has_stream_record ()) {continue;}
        break;
    }
    if (oobr_it == a_output.out_of_band_records ().end ()) {
        goto out;
    }

    str = oobr_it->stream_record ().debugger_console ();
    if (!str.raw ().compare (0, 5, "File ")) {
        result[file_name] = var_list;
        goto fetch_file;
    } else if (str.raw ()[str.raw ().length () - 2] == ';') {
        goto fetch_variable;
    } else {
        goto skip_oobr;
    }

out:
    if (!found)
        return false;

    a_vars = result;
    return true;
}

void
GDBEngine::list_register_names (const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    queue_command (Command ("list-register-names",
                            "-data-list-register-names ",
                            a_cookie));
}


void
GDBEngine::list_changed_registers (const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    queue_command (Command ("list-changed-registers",
                            "-data-list-changed-registers",
                            a_cookie));
}

void
GDBEngine::list_register_values (const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    queue_command (Command ("list-register-values",
                            "-data-list-register-values "
                            " x " /*x=hex format*/ ,
                            a_cookie));
}

void
GDBEngine::list_register_values
                    (std::list<IDebugger::register_id_t> a_registers,
                     const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    UString regs_str;
    std::list<register_id_t>::const_iterator iter;
    for (iter = a_registers.begin ();
         iter != a_registers.end ();
         ++ iter) {
        regs_str += UString::from_int (*iter) + " ";
    }

    queue_command (Command ("list-register-values",
                            "-data-list-register-values "
                            " x " + regs_str,
                            a_cookie));
}

void
GDBEngine::set_register_value (const UString& a_reg_name,
                               const UString& a_value,
                               const UString& a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    UString command_str;

    command_str = "-data-evaluate-expression "
                  " $" + a_reg_name + "=" + a_value;
    Command command ("set-register-value",
                     command_str,
                     a_cookie);
    command.tag0 ("set-register-value");
    command.tag1 (a_reg_name);
    queue_command (command);
}


void
GDBEngine::read_memory (size_t a_start_addr,
                        size_t a_num_bytes,
                        const UString& a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    UString cmd;
    // format: -data-read-memory ADDR WORD_FORMAT WORD_SIZE NR_ROW NR_COLS
    // We assume the following for now:
    //  - output values in hex format (x)
    //  - word size of 1 byte
    //  - a single row of output
    // When we parse the output from the command,
    // we assume that there's only a
    // single row of output -- if this ever changes,
    // the parsing function will need to be updated
    cmd.printf ("-data-read-memory %zu x 1 1 %zu",
                a_start_addr,
                a_num_bytes);
    queue_command (Command ("read-memory",
                            cmd,
                            a_cookie));
}

void
GDBEngine::set_memory (size_t a_addr,
                       const std::vector<uint8_t>& a_bytes,
                       const UString& a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    for (std::vector<uint8_t>::const_iterator iter = a_bytes.begin ();
            iter != a_bytes.end (); ++iter)
    {
        UString cmd_str;
        cmd_str.printf ("-data-evaluate-expression "
                        "\"*(unsigned char*)%zu = 0x%X\"",
                a_addr++,
                *iter);
        Command command ("set-memory", cmd_str, a_cookie);
        command.tag0 ("set-memory");
        command.tag1 (UString ().printf ("0x%X",a_addr));
        queue_command (command);
    }
}

void
GDBEngine::disassemble (size_t a_start_addr,
                        bool a_start_addr_relative_to_pc,
                        size_t a_end_addr,
                        bool a_end_addr_relative_to_pc,
                        bool a_pure_asm,
                        const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    disassemble (a_start_addr, a_start_addr_relative_to_pc, a_end_addr,
                 a_end_addr_relative_to_pc, &null_disass_slot,
                 a_pure_asm, a_cookie);
}

void
GDBEngine::disassemble (size_t a_start_addr,
                        bool a_start_addr_relative_to_pc,
                        size_t a_end_addr,
                        bool a_end_addr_relative_to_pc,
                        const DisassSlot &a_slot,
                        bool a_pure_asm,
                        const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    UString cmd_str;

    // <build the command string>
    cmd_str = "-data-disassemble";
    if (a_start_addr_relative_to_pc) {
        cmd_str += " -s \"$pc";
        if (a_start_addr) {
            cmd_str += " + " + UString::from_int (a_start_addr);
        }
        cmd_str += "\"";
    } else {
        cmd_str += " -s " + UString::from_int (a_start_addr);
    }

    if (a_end_addr_relative_to_pc) {
        cmd_str += " -e \"$pc";
        if (a_end_addr) {
            cmd_str += " + " + UString::from_int (a_end_addr);
        }
        cmd_str += "\"";
    } else {
        cmd_str += " -e " + UString::from_int (a_end_addr);
    }

    if (a_pure_asm)
        cmd_str += " -- 0";
    else
        cmd_str += " -- 1";

    // </build the command string>

    LOG_DD ("cmd_str: " << cmd_str);

    Command command ("disassemble-address-range", cmd_str, a_cookie);
    command.set_slot (a_slot);
    queue_command (command);
}

void
GDBEngine::disassemble_lines (const UString &a_file_name,
                              int a_line_num,
                              int a_nb_disassembled_lines,
                              bool a_pure_asm,
                              const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    disassemble_lines (a_file_name, a_line_num, a_nb_disassembled_lines,
                       &null_disass_slot, a_pure_asm, a_cookie);
}

void
GDBEngine::disassemble_lines (const UString &a_file_name,
                              int a_line_num,
                              int a_nb_disassembled_lines,
                              const DisassSlot &a_slot,
                              bool a_pure_asm,
                              const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    UString cmd_str = "-data-disassemble";

    cmd_str += " -f " + a_file_name + " -l "
               + UString::from_int (a_line_num);
    if (a_nb_disassembled_lines) {
        cmd_str += " -n " + UString::from_int (a_nb_disassembled_lines);
    }
    if (a_pure_asm)
        cmd_str += " -- 0";
    else
        cmd_str += " -- 1";

    LOG_DD ("cmd_str: " << cmd_str);

    Command command ("disassemble-line-range-in-file", cmd_str, a_cookie);
    command.tag0 (a_file_name);
    command.tag1 (UString::from_int (a_line_num));
    command.set_slot (a_slot);
    queue_command (command);
}

void
GDBEngine::create_variable (const UString &a_name,
                            const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    create_variable (a_name,
                     &null_const_variable_slot,
                     a_cookie);
}

void
GDBEngine::create_variable (const UString &a_name ,
                            const ConstVariableSlot &a_slot,
                            const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    if (a_name.empty ()) {
        LOG ("got empty name");
        return;
    }

    UString cur_frame;
    get_mi_thread_and_frame_location (cur_frame);

    Command command ("create-variable",
                     "-var-create " + cur_frame
                     + " - " // automagic varobj name
                       " * " // current frame @
                     + a_name,
                     a_cookie);
    command.tag0 (a_name);
    command.set_slot (a_slot);
    queue_command (command);
}

void
GDBEngine::delete_variable (const VariableSafePtr a_var,
                            const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    delete_variable (a_var,
                     &null_const_variable_slot,
                     a_cookie);
}

void
GDBEngine::delete_variable (const VariableSafePtr a_var,
                            const ConstVariableSlot &a_slot,
                            const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    THROW_IF_FAIL (a_var);
    THROW_IF_FAIL (!a_var->internal_name ().empty ());

    Command command ("delete-variable",
                     "-var-delete " + a_var->internal_name (),
                     a_cookie);
    command.variable (a_var);
    command.set_slot (a_slot);
    queue_command (command);
}

void
GDBEngine::unfold_variable (const VariableSafePtr a_var,
                            const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    unfold_variable (a_var,
                     &null_const_variable_slot,
                     a_cookie);
}

void
GDBEngine::unfold_variable (const VariableSafePtr a_var,
                            const ConstVariableSlot &a_slot,
                            const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    THROW_IF_FAIL (a_var);
    if (a_var->internal_name ().empty ()) {
        UString qname;
        a_var->build_qualified_internal_name (qname);
        a_var->internal_name (qname);
    }
    THROW_IF_FAIL (!a_var->internal_name ().empty ());

    Command command ("unfold-variable",
                     "-var-list-children "
                     " --all-values "
                     + a_var->internal_name (),
                     a_cookie);
    command.variable (a_var);
    command.set_slot (a_slot);
    queue_command (command);
}

void
GDBEngine::assign_variable (const VariableSafePtr a_var,
                            const UString &a_expression,
                            const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    assign_variable (a_var,
                     a_expression,
                     &null_const_variable_slot,
                     a_cookie);
}

void
GDBEngine::assign_variable
                    (const VariableSafePtr a_var,
                     const UString &a_expression,
                     const ConstVariableSlot &a_slot,
                     const UString &a_cookie)
{
    THROW_IF_FAIL (a_var);
    THROW_IF_FAIL (!a_var->internal_name ().empty ());
    THROW_IF_FAIL (!a_expression.empty ());

    Command command ("assign-variable",
                     "-var-assign "
                     + a_var->internal_name ()
                     + " " + a_expression,
                     a_cookie);
    command.variable (a_var);
    command.set_slot (a_slot);
    queue_command (command);
}

void
GDBEngine::evaluate_variable_expr (const VariableSafePtr a_var,
                                   const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    evaluate_variable_expr (a_var,
                            &null_const_variable_slot,
                            a_cookie);
}

void
GDBEngine::evaluate_variable_expr
                        (const VariableSafePtr a_var,
                         const ConstVariableSlot &a_slot,
                         const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    THROW_IF_FAIL (a_var);
    THROW_IF_FAIL (!a_var->internal_name ().empty ());

    Command command ("evaluate-expression",
                     "-var-evaluate-expression "
                     + a_var->internal_name (),
                     a_cookie);
    command.variable (a_var);
    command.set_slot (a_slot);
    queue_command (command);
}

void
GDBEngine::list_changed_variables (VariableSafePtr a_var,
                                   const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    list_changed_variables (a_var,
                            &null_const_variable_list_slot,
                            a_cookie);
}

void
GDBEngine::list_changed_variables
                (VariableSafePtr a_var,
                 const ConstVariableListSlot &a_slot,
                 const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    THROW_IF_FAIL (a_var);
    THROW_IF_FAIL (!a_var->internal_name ().empty ());

    Command command ("list-changed-variables",
                     "-var-update "
                     " --all-values "
                     + a_var->internal_name (),
                     a_cookie);
    command.variable (a_var);
    command.set_slot (a_slot);
    queue_command (command);
}

void
GDBEngine::query_variable_path_expr (const VariableSafePtr a_var,
                                     const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    query_variable_path_expr (a_var, &null_const_variable_slot, a_cookie);
}

void
GDBEngine::query_variable_path_expr (const VariableSafePtr a_var,
                                     const ConstVariableSlot &a_slot,
                                     const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    THROW_IF_FAIL (a_var);
    THROW_IF_FAIL (!a_var->internal_name ().empty ());

    UString cmd_str = "-var-info-path-expression ";
    cmd_str += a_var->internal_name ();

    Command command ("query-variable-path-expr",
                     cmd_str, a_cookie);
    command.variable (a_var);
    command.set_slot (a_slot);
    queue_command (command);
}

//****************************
//</GDBEngine methods>
//****************************

class GDBEngineModule : public DynamicModule {

public:

    void get_info (Info &a_info) const
    {
        const static Info s_info ("debuggerengine",
                                  "The GDB debugger engine backend. "
                                  "Implements the IDebugger interface",
                                  "1.0");
        a_info = s_info;
    }

    void do_init ()
    {
    }

    bool lookup_interface (const std::string &a_iface_name,
                           DynModIfaceSafePtr &a_iface)
    {
        if (a_iface_name == "IDebugger") {
            a_iface.reset (new GDBEngine (this));
        } else {
            return false;
        }
        return true;
    }
};//end class GDBEngineModule

NEMIVER_END_NAMESPACE (nemiver)

//the dynmod initial factory.
extern "C" {
bool
NEMIVER_API nemiver_common_create_dynamic_module_instance (void **a_new_instance)
{
    *a_new_instance = new nemiver::GDBEngineModule ();
    return (*a_new_instance != 0);
}

}//end extern C

