// -*- c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4; -*-'
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
#include <pty.h>
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
#include "nmv-gdb-engine.h"
#include "langs/nmv-cpp-parser.h"
#include "langs/nmv-cpp-ast-utils.h"

using namespace std;
using namespace nemiver::common;
using namespace nemiver::cpp;

static const UString GDBMI_OUTPUT_DOMAIN = "gdbmi-output-domain";
static const UString DEFAULT_GDB_BINARY = "default-gdb-binary";
static const UString CONF_KEY_GDB_BINARY = "/apps/nemiver/dbgperspective/gdb-binary";

NEMIVER_BEGIN_NAMESPACE (nemiver)

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
    UString gdb_stdout_buffer;
    UString gdb_stderr_buffer;
    list<Command> queued_commands;
    list<Command> started_commands;
    bool line_busy;
    map<int, IDebugger::BreakPoint> cached_breakpoints;
    enum InBufferStatus {
        DEFAULT,
        FILLING,
        FILLED
    };
    InBufferStatus error_buffer_status;
    Glib::RefPtr<Glib::MainContext> loop_context;

    OutputHandlerList output_handler_list;
    IDebugger::State state;
    ILangTraitSafePtr lang_trait;
    UString debugger_full_path;
    sigc::signal<void> gdb_died_signal;
    sigc::signal<void, const UString& > master_pty_signal;
    sigc::signal<void, const UString& > gdb_stdout_signal;
    sigc::signal<void, const UString& > gdb_stderr_signal;

    mutable sigc::signal<void, Output&> pty_signal ;

    mutable sigc::signal<void, CommandAndOutput&> stdout_signal ;

    mutable sigc::signal<void, Output&> stderr_signal ;

    mutable sigc::signal<void, const UString&> console_message_signal;
    mutable sigc::signal<void, const UString&> target_output_message_signal;

    mutable sigc::signal<void, const UString&> log_message_signal;

    mutable sigc::signal<void, const UString&, const UString&>
                                                        command_done_signal;

    mutable sigc::signal<void> connected_to_server_signal;

    mutable sigc::signal<void> detached_from_target_signal;

    mutable sigc::signal<void,
                         const map<int, IDebugger::BreakPoint>&,
                         const UString&> breakpoints_set_signal;

    mutable sigc::signal<void,
                         const vector<OverloadsChoiceEntry>&,
                         const UString&> got_overloads_choice_signal;

    mutable sigc::signal<void,
                         const IDebugger::BreakPoint&,
                         int,
                         const UString&> breakpoint_deleted_signal;

    mutable sigc::signal<void, const IDebugger::BreakPoint&, int>
                                                breakpoint_disabled_signal;

    mutable sigc::signal<void, const IDebugger::BreakPoint&, int>
                                                breakpoint_enabled_signal;

    mutable sigc::signal<void, const UString&,
                         bool, const IDebugger::Frame&,
                         int, const UString&> stopped_signal;

    mutable sigc::signal<void,
                         const list<int>,
                         const UString& > threads_listed_signal;

    mutable sigc::signal<void,
                         const vector<UString>&,
                         const UString& > files_listed_signal;

    mutable sigc::signal<void, int, const Frame&, const UString&>
                                                        thread_selected_signal;

    mutable sigc::signal<void, const vector<IDebugger::Frame>&, const UString&>
                                                    frames_listed_signal;

    mutable sigc::signal<void,
                         const map<int, list<IDebugger::VariableSafePtr> >&,
                         const UString&> frames_arguments_listed_signal;

    mutable sigc::signal<void, const IDebugger::Frame&, const UString&>
                                                        current_frame_signal;

    mutable sigc::signal<void, const list<VariableSafePtr>&, const UString& >
                                    local_variables_listed_signal ;

    mutable sigc::signal<void, const list<VariableSafePtr>&, const UString& >
                                    global_variables_listed_signal ;

    mutable sigc::signal<void,
                         const UString&,
                         const IDebugger::VariableSafePtr&,
                         const UString&>             variable_value_signal;

    mutable sigc::signal<void,
                         const VariableSafePtr&/*variable*/,
                         const UString& /*cookie*/>
                                     variable_value_set_signal;

    mutable sigc::signal<void,
                         const UString&,
                         const VariableSafePtr&,
                         const UString&> pointed_variable_value_signal;

    mutable sigc::signal<void, const UString&, const UString&, const UString&>
                                                        variable_type_signal;

    mutable sigc::signal<void, const VariableSafePtr&, const UString&>
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

    mutable sigc::signal<void, const std::map<register_id_t, UString>&, const UString& >
                                                        register_names_listed_signal;

    mutable sigc::signal<void, const std::list<register_id_t>&, const UString& >
                                                    changed_registers_listed_signal;

    mutable sigc::signal<void, const std::map<register_id_t, UString>&, const UString& >
                                                        register_values_listed_signal;

    mutable sigc::signal<void, const UString&, const UString&, const UString&>
                                                        register_value_changed_signal;
    mutable sigc::signal <void, size_t, const std::vector<uint8_t>&, const UString&>
                                                                  read_memory_signal;
    mutable sigc::signal <void, size_t, const std::vector<uint8_t>&, const UString& >
                                                                  set_memory_signal;

    //***********************
    //</GDBEngine attributes>
    //************************

    IConfMgrSafePtr get_conf_mgr () const
    {
        if (!conf_mgr) {
            THROW_IF_FAIL (dynmod);
            DynamicModule::Loader *loader = dynmod->get_module_loader ();
            THROW_IF_FAIL (loader);
            DynamicModuleManager *module_manager =
                                        loader->get_dynamic_module_manager ();
            THROW_IF_FAIL (module_manager);
            conf_mgr = module_manager->load_iface<IConfMgr> ("gconfmgr",
                                                             "IConfMgr");
        }
        THROW_IF_FAIL (conf_mgr);
        return conf_mgr;
    }

    const UString& get_debugger_full_path () const
    {
        get_conf_mgr ()->get_key_value (CONF_KEY_GDB_BINARY,
                                 const_cast<Priv*> (this)->debugger_full_path);
        if (debugger_full_path == "" ||
            debugger_full_path == DEFAULT_GDB_BINARY) {
            const_cast<Priv*> (this)->debugger_full_path = env::get_gdb_program ();
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
        for (; from < end;) {
            if (!parse_output_record (a_buf, from, to, output)) {
                LOG_ERROR ("output record parsing failed: "
                        << a_buf.substr (from, end - from)
                        << "\npart of buf: " << a_buf
                        << "\nfrom: " << (int) from
                        << "\nto: " << (int) to << "\n"
                        << "\nstrlen: " << (int) a_buf.size ());
                break;
            }

            //parsing GDB/MI output succeeded.
            //Check if the output contains the result to a command issued by
            //the user. If yes, build the CommandAndResult, update the
            //command queue and notify the user that the command it issued
            //has a result.
            //
            output.parsing_succeeded (true);
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
            if (output.has_result_record ()/*gdb acknowledged previous cmd*/) {
                LOG_DD ("here");
                if (!started_commands.empty ()) {
                    started_commands.erase (started_commands.begin ());
                    LOG_DD ("clearing the line");
                    //we can send another cmd down the wire
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
    }

    Priv (DynamicModule *a_dynmod) :
        dynmod (a_dynmod), cwd ("."),
        gdb_pid (0), target_pid (0),
        gdb_stdout_fd (0), gdb_stderr_fd (0),
        master_pty_fd (0),
        is_attached (false),
        line_busy (false),
        error_buffer_status (DEFAULT),
        state (IDebugger::NOT_STARTED)
    {
        gdb_stdout_signal.connect (sigc::mem_fun
                (*this, &Priv::on_gdb_stdout_signal));
        master_pty_signal.connect (sigc::mem_fun
                (*this, &Priv::on_master_pty_signal));
        gdb_stderr_signal.connect (sigc::mem_fun
                (*this, &Priv::on_gdb_stderr_signal));

        state_changed_signal.connect (sigc::mem_fun
                (*this, &Priv::on_state_changed_signal));
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
        gdb_stdout_channel->set_encoding (a_charset);
        gdb_stderr_channel->set_encoding (a_charset);
        master_pty_channel->set_encoding (a_charset);
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

        string charset;
        Glib::get_charset (charset);
        set_communication_charset (charset);


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
        const char *tmp = getenv ("PATH");
        if (!tmp) {return false;}
        vector<UString> path_dirs = UString (tmp).split (":");
        path_dirs.insert (path_dirs.begin (), ("."));
        vector<UString>::const_iterator it;
        string file_path;
        for (it = path_dirs.begin (); it != path_dirs.end (); ++it) {
            file_path = Glib::build_filename (Glib::locale_from_utf8 (*it),
                                              Glib::locale_from_utf8 (a_prog));
            if (Glib::file_test (file_path,
                                 Glib::FILE_TEST_IS_REGULAR)) {
                a_prog_path = Glib::locale_to_utf8 (file_path);
                return true;
            }
        }
        return false;
    }

    bool launch_gdb (const UString &working_dir,
                     const vector<UString> &a_source_search_dirs,
                     const vector<UString> &a_gdb_options,
                     const UString a_prog="")
    {
        if (is_gdb_running ()) {
            kill_gdb ();
        }
        argv.clear ();
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
        if (a_prog != "") {
            UString prog_path = a_prog;
            if (!Glib::file_test (Glib::locale_from_utf8 (prog_path),
                                  Glib::FILE_TEST_IS_REGULAR)) {
                if (!find_prog_in_path (prog_path, prog_path)) {
                    LOG_ERROR ("Could not find program '" << prog_path << "'");
                    return false;
                }
            }
            argv.push_back (prog_path);
        }

        source_search_dirs = a_source_search_dirs;
        return launch_gdb_real (argv);
    }

    bool launch_gdb_and_set_args (const UString &working_dir,
                                  const vector<UString> &a_source_search_dirs,
                                  const vector<UString> &a_prog_args,
                                  const vector<UString> a_gdb_options)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        bool result (false);
        result = launch_gdb (working_dir, a_source_search_dirs,
                             a_gdb_options, a_prog_args[0]);
        LOG_DD ("workingdir:" << working_dir
                << "\nsearchpath:" << UString::join (a_source_search_dirs)
                << "\nprogargs:" << UString::join (a_prog_args)
                << "\ngdboptions:" << UString::join (a_gdb_options));

        if (!result) {return false;}

        if (!a_prog_args.empty ()) {
            UString args;
            for (vector<UString>::size_type i=1;
                 i < a_prog_args.size ();
                 ++i) {
                args += a_prog_args[i] + " ";
            }

            if (args != "") {
                return issue_command (Command ("set args " + args));
            }
        }
        return true;
    }

    bool launch_gdb_on_core_file (const UString &a_prog_path,
                                  const UString &a_core_path)
    {
        vector<UString> argv;
        argv.push_back (env::get_gdb_program ());
        argv.push_back ("--interpreter=mi2");
        argv.push_back (a_prog_path);
        argv.push_back (a_core_path);
        return launch_gdb_real (argv);
    }

    bool issue_command (const Command &a_command,
                        bool a_do_record=true)
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
            UString meaningful_buffer;
            while (true) {
                memset (buf, 0, CHUNK_SIZE + 1);
                status = gdb_stdout_channel->read (buf, CHUNK_SIZE, nb_read);
                if (status == Glib::IO_STATUS_NORMAL &&
                    nb_read && (nb_read <= CHUNK_SIZE)) {
                    std::string raw_str(buf, nb_read);
                    UString tmp = Glib::locale_to_utf8 (raw_str);
                    gdb_stdout_buffer.append (tmp);
                } else {
                    break;
                }
                nb_read = 0;
            }
            LOG_DD ("gdb_stdout_buffer: <buf>" << gdb_stdout_buffer << "</buf>");

            UString::size_type i=0;
            while ((i = gdb_stdout_buffer.raw ().find ("\n(gdb)")) !=
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
                meaningful_buffer.chomp ();
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
            if (gdb_stdout_buffer.raw ().find ("[0] cancel") != std::string::npos
                && gdb_stdout_buffer.raw ().find ("> ") != std::string::npos) {
                //this is not a gdbmi ouptut, but rather a plain gdb
                //command line. It is actually a prompt sent by gdb
                //to let the user choose between a list of overloaded functions
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


struct OnStreamRecordHandler: OutputHandler{
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

    OnDetachHandler (GDBEngine *a_engine=0) :
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

struct OnBreakPointHandler: OutputHandler {
    GDBEngine * m_engine;
    vector<UString>m_prompt_choices;

    OnBreakPointHandler (GDBEngine *a_engine=0) :
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
        UString::size_type cur=0;
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
        return parse_overloads_choice_prompt (input, cur, cur, a_prompts);
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

        bool has_breaks=false;
        //if breakpoint where set, put them in cache !
        if (has_breakpoints_set (a_in)) {
            m_engine->append_breakpoints_to_cache
            (a_in.output ().result_record ().breakpoints ());
            has_breaks=true;
        }

        if (a_in.output ().has_result_record ()
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
                map<int, IDebugger::BreakPoint>::iterator iter;
                map<int, IDebugger::BreakPoint> &breaks =
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
        } else if (has_breaks){
            LOG_DD ("firing IDebugger::breakpoint_set_signal()");
            m_engine->breakpoints_set_signal ().emit
                            (a_in.output ().result_record ().breakpoints (),
                             a_in.command ().cookie ());
            m_engine->set_state (IDebugger::READY);
        } else {
            LOG_DD ("finally, no breakpoint was detected as set/deleted");
        }
    }
};//end struct OnBreakPointHandler

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

        for (iter = a_in.output ().out_of_band_records ().begin ();
                iter != a_in.output ().out_of_band_records ().end ();
                ++iter) {
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
        if (a_in.has_command ()) {}

        int thread_id = m_out_of_band_record.thread_id ();

        m_engine->stopped_signal ().emit
                    (m_out_of_band_record.stop_reason_as_str (),
                     m_out_of_band_record.has_frame (),
                     m_out_of_band_record.frame (),
                     thread_id,
                     a_in.command ().cookie ());

        UString reason = m_out_of_band_record.stop_reason_as_str ();

        if (reason == "exited-signalled"
            || reason == "exited-normally"
            || reason == "exited") {
            m_engine->set_state (IDebugger::PROGRAM_EXITED);
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

    OnThreadSelectedHandler (GDBEngine *a_engine) :
        m_engine (a_engine)
    {}

    bool can_handle (CommandAndOutput &a_in)
    {
        THROW_IF_FAIL (m_engine);
        if (a_in.output ().has_result_record ()
            && a_in.output ().result_record ().thread_id_got_selected ()) {
            return true;
        }
        return false;
    }

    void do_handle (CommandAndOutput &a_in)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (m_engine);
        m_engine->thread_selected_signal ().emit
            (a_in.output ().result_record ().thread_id (),
             a_in.output ().result_record ().frame_in_thread (),
             a_in.command ().cookie ());
    }
};//end OnThreadSelectedHandler

struct OnCommandDoneHandler : OutputHandler {

    GDBEngine *m_engine;

    OnCommandDoneHandler (GDBEngine *a_engine) :
        m_engine (a_engine)
    {}

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

        m_engine->command_done_signal ().emit (a_in.command ().name (),
                                               a_in.command ().cookie ());
        if (a_in.command ().name () == "attach-to-program") {
            m_engine->set_attached_to_target (true);
        }
        //TODO: this is not necessarily true. Before setting the state
        //to ready here, one must now which command exactly was fired so
        //that gdb returned the "DONE" status for it.
        m_engine->set_state (IDebugger::READY);
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
        m_engine->frames_listed_signal ().emit
            (a_in.output ().result_record ().call_stack (),
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

        int pid=0; UString exe_path;
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

    bool can_handle (CommandAndOutput &a_in)
    {
        if ((a_in.command ().name () == "print-variable-value"
             || a_in.command ().name () == "get-variable-value"
             || a_in.command ().name () == "print-pointed-variable-value"
             || a_in.command ().name () == "dereference-variable"
             || a_in.command ().name () == "set-register-value"
             || a_in.command ().name () == "set-memory")
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
            name_caption = "*" + name_caption ;
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
                LOG_DD ("checking debugger log: "
                        << it->stream_record ().debugger_log ());
                if (it->has_stream_record ()
                    && !it->stream_record ().debugger_log ().compare
                                                            (0, 6, "ptype ")) {

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
        THROW_IF_FAIL2 (it->has_stream_record ()
                        && !it->stream_record ().debugger_log ().compare
                                               (0, 6, "ptype "),
                        "stream_record: " +
                        it->stream_record ().debugger_log ());
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
            if (it->stop_reason () == Output::OutOfBandRecord::SIGNAL_RECEIVED) {
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

void
GDBEngine::load_program (const UString &a_prog_with_args,
                         const UString &a_working_dir)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);

    vector<UString> args = a_prog_with_args.split (" ");
    vector<UString> search_paths;
    UString tty_path;

    load_program (args, a_working_dir, search_paths, tty_path);
}

void
GDBEngine::load_program (const vector<UString> &a_argv,
                         const UString &working_dir,
                         const vector<UString> &a_source_search_dirs,
                         const UString &a_tty_path)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (!a_argv.empty ());
    vector<UString> argv (a_argv);

    //first, check if the the inferior is a libtool wrapper or not.
    //if yes, bet the real path of the actual binary and use that instead
    UString real_path;
    if (is_libtool_executable_wrapper (argv[0], real_path)
        && !real_path.empty ()) {
        LOG_DD ("handling libtool wrapper script ...");
        string tmp_str = Glib::filename_from_utf8 (real_path);
        string dir_name = Glib::path_get_dirname
                                    (Glib::filename_to_utf8 (argv[0]));
        string path = Glib::build_filename (dir_name, tmp_str);
        argv[0] = Glib::filename_to_utf8 (path);
        LOG_DD ("got path to real binary from libtool wrapper: " << path);
    }

    if (!m_priv->is_gdb_running ()) {
        vector<UString> gdb_opts;
        THROW_IF_FAIL (m_priv->launch_gdb_and_set_args
                                    (working_dir, a_source_search_dirs, 
                                     argv, gdb_opts));

        Command command;

        queue_command (Command ("set breakpoint pending on"));
        //tell gdb not to pass the SIGINT signal to the target.
        queue_command (Command ("handle SIGINT stop print nopass"));
        //tell the linker to do all relocations at program load
        //time so that some "step into" don't take for ever.
        //On GDB, it seems that stepping into a function that is
        //in a share lib takes stepping through GNU ld, so it can take time.
        const char *nmv_dont_ld_bind_now = getenv ("NMV_DONT_LD_BIND_NOW");
        if (!nmv_dont_ld_bind_now || !atoi (nmv_dont_ld_bind_now)) {
            LOG_DD ("setting LD_BIND_NOW=1");
            queue_command (Command ("set env LD_BIND_NOW 1"));
        } else {
            LOG_DD ("not setting LD_BIND_NOW environment variable ");
        }
    } else {
        UString args;
        UString::size_type len (argv.size ());
        for (UString::size_type i = 1; i < len; ++i) {
            args += " " + argv[i];
        }

        Command command ("load-program",
                         UString ("-file-exec-and-symbols ") + argv[0]);
        queue_command (command);

        command.value ("set args " + args);
        queue_command (command);
    }
    if (!a_tty_path.empty ()) {
        queue_command (Command ("set inferior-tty " + a_tty_path));
    }
}

void
GDBEngine::load_core_file (const UString &a_prog_path,
                           const UString &a_core_path)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    if (m_priv->is_gdb_running ()) {
        m_priv->kill_gdb ();
    }

    vector<UString> src_dirs, gdb_opts;
    THROW_IF_FAIL (m_priv->launch_gdb_on_core_file (a_prog_path,
                                                    a_core_path));
}

bool
GDBEngine::attach_to_target (unsigned int a_pid,
                             const UString &a_tty_path)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    vector<UString> args, source_search_dirs;

    if (!m_priv->is_gdb_running ()) {
        vector<UString> gdb_opts;
        THROW_IF_FAIL (m_priv->launch_gdb ("", source_search_dirs, gdb_opts));

        Command command;
        command.value ("set breakpoint pending auto");
        queue_command (command);
        //tell the linker to do all relocations at program load
        //time so that some "step into" don't take for ever.
        //On GDB, it seems that stepping into a function that is
        //in a share lib takes stepping through GNU ld, so it can take time.
        const char *nmv_dont_ld_bind_now = getenv ("NMV_DONT_LD_BIND_NOW");
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
    if (a_tty_path != "") {
        queue_command (Command ("tty " + a_tty_path));
    }
    return true;
}

bool
GDBEngine::attach_to_remote_target (const UString &a_host, int a_port)
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
    THROW_IF_FAIL (m_priv);
    queue_command (Command ("detach-from-target", "-target-detach", a_cookie));
}

bool
GDBEngine::is_attached_to_target () const
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    return m_priv->is_attached;
}

void
GDBEngine::set_attached_to_target (bool a_is_attached)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    m_priv->is_attached = a_is_attached;
}

void
GDBEngine::add_env_variables (const map<UString, UString> &a_vars)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
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
    THROW_IF_FAIL (m_priv);
    return m_priv->env_variables;
}

const UString&
GDBEngine::get_target_path ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    return m_priv->exe_path;
}

IDebugger::State
GDBEngine::get_state () const
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);

    return m_priv->state;
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
                (OutputHandlerSafePtr (new OnBreakPointHandler (this)));
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

sigc::signal<void, const IDebugger::BreakPoint&, int, const UString&>&
GDBEngine::breakpoint_deleted_signal () const
{
    return m_priv->breakpoint_deleted_signal;
}

sigc::signal<void, const map<int, IDebugger::BreakPoint>&, const UString&>&
GDBEngine::breakpoints_set_signal () const
{
    return m_priv->breakpoints_set_signal;
}

sigc::signal<void, const vector<IDebugger::OverloadsChoiceEntry>&, const UString&>&
GDBEngine::got_overloads_choice_signal () const
{
    return m_priv->got_overloads_choice_signal;
}

sigc::signal<void, const UString&, bool, const IDebugger::Frame&, int, const UString&>&
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

sigc::signal<void, int, const IDebugger::Frame&, const UString&>&
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

sigc::signal<void, const UString&, const IDebugger::VariableSafePtr&, const UString&>&
GDBEngine::variable_value_signal () const
{
    return m_priv->variable_value_signal;
}

sigc::signal<void, const IDebugger::VariableSafePtr&, const UString&>&
GDBEngine::variable_value_set_signal () const
{
    return m_priv->variable_value_set_signal;
}

sigc::signal<void, const UString&, const IDebugger::VariableSafePtr&, const UString&>&
GDBEngine::pointed_variable_value_signal () const
{
    return m_priv->pointed_variable_value_signal;
}

sigc::signal<void, const UString&, const UString&, const UString&>&
GDBEngine::variable_type_signal () const
{
    return m_priv->variable_type_signal;
}

sigc::signal<void, const IDebugger::VariableSafePtr&, const UString&>&
GDBEngine::variable_type_set_signal () const
{
    return m_priv->variable_type_set_signal;
}

sigc::signal<void, const IDebugger::VariableSafePtr, const UString&>
GDBEngine::variable_dereferenced_signal () const
{
    return m_priv->variable_dereferenced_signal;
}

sigc::signal<void, const std::map<IDebugger::register_id_t, UString>&, const UString& >&
GDBEngine::register_names_listed_signal () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->register_names_listed_signal;
}

sigc::signal<void, const std::map<IDebugger::register_id_t, UString>&, const UString& >&
GDBEngine::register_values_listed_signal () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->register_values_listed_signal;
}

sigc::signal<void, const UString&, const UString&, const UString& >&
GDBEngine::register_value_changed_signal () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->register_value_changed_signal;
}

sigc::signal<void, const std::list<IDebugger::register_id_t>&, const UString& >&
GDBEngine::changed_registers_listed_signal () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->changed_registers_listed_signal;
}

sigc::signal <void, size_t, const std::vector<uint8_t>&, const UString&>&
GDBEngine::read_memory_signal () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->read_memory_signal;
}

sigc::signal <void, size_t, const std::vector<uint8_t>&, const UString& >&
GDBEngine::set_memory_signal () const
{
    THROW_IF_FAIL (m_priv);
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

sigc::signal<void, IDebugger::State>&
GDBEngine::state_changed_signal () const
{
    return m_priv->state_changed_signal;
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

    THROW_IF_FAIL (m_priv);
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
GDBEngine::on_stopped_signal (const UString &a_reason,
                              bool a_has_frame,
                              const IDebugger::Frame &a_frame,
                              int a_thread_id,
                              const UString &a_cookie)
{
    if (a_has_frame || a_frame.line () || a_thread_id || a_cookie.empty ()) {
        //keep compiler happy
    }

    NEMIVER_TRY

    if (a_reason == "exited-signalled"
        || a_reason == "exited-normally"
        || a_reason == "exited") {
        return;
    }
    THROW_IF_FAIL (m_priv);
    m_priv->is_attached = true;

    NEMIVER_CATCH_NOX
}

void
GDBEngine::on_detached_from_target_signal ()
{
    NEMIVER_TRY

    THROW_IF_FAIL (m_priv);
    m_priv->is_attached = false;

    NEMIVER_CATCH_NOX
}

void
GDBEngine::on_program_finished_signal ()
{
    NEMIVER_TRY

    THROW_IF_FAIL (m_priv);
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

void
GDBEngine::do_init (IConfMgrSafePtr &a_conf_mgr)
{
    THROW_IF_FAIL (m_priv);

    m_priv->conf_mgr = a_conf_mgr;
}

IConfMgr&
GDBEngine::get_conf_mgr ()
{
    THROW_IF_FAIL (m_priv);

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
    THROW_IF_FAIL (m_priv);
    m_priv->run_loop_iterations_real (a_nb_iters);
}

void
GDBEngine::set_state (IDebugger::State a_state)
{
    THROW_IF_FAIL (m_priv);
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
    bool result (false);
    THROW_IF_FAIL (m_priv && m_priv->is_gdb_running ());
    LOG_DD ("queuing command: '" << a_command.value () << "'");
    m_priv->queued_commands.push_back (a_command);
    if (!m_priv->line_busy && m_priv->started_commands.empty ()) {
        result = m_priv->issue_command (*m_priv->queued_commands.begin (),
                                        true);
        m_priv->queued_commands.erase (m_priv->queued_commands.begin ());
    }
    return result;
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
    if (a_name == "")
        return;
    UString param_str = a_name + " " + a_value;
    queue_command (Command ("set-debugger-parameter", "set " + param_str));
}

void
GDBEngine::set_solib_prefix_path (const UString &a_name)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    set_debugger_parameter ("solib-absolute-prefix", a_name);
}

void
GDBEngine::do_continue (const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    queue_command (Command ("do-continue", "-exec-continue", a_cookie));
}

void
GDBEngine::run (const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    queue_command (Command ("run", "-exec-run", a_cookie));
}

void
GDBEngine::get_target_info (const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    queue_command (Command ("get-target-info", "info proc", a_cookie));
}

ILangTraitSafePtr
GDBEngine::create_language_trait ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);

    //TODO: detect the actual language of the target being
    //debugged and create a matching language trait on the fly for it.
    //For now, let's say we just debug only c++
    DynamicModule::Loader* loader = get_dynamic_module ().get_module_loader ();
    THROW_IF_FAIL (loader);
    DynamicModuleManager *mgr = loader->get_dynamic_module_manager ();
    THROW_IF_FAIL (mgr);

    ILangTraitSafePtr trait =
        mgr->load_iface<ILangTrait> ("cpptrait", "ILangTrait");

    return trait;
}

ILangTraitSafePtr
GDBEngine::get_language_trait ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    if (!m_priv->lang_trait) {
        m_priv->lang_trait = create_language_trait ();
    }
    return m_priv->lang_trait;
}

bool
GDBEngine::stop_target ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
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
    THROW_IF_FAIL (m_priv);

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
    THROW_IF_FAIL (m_priv);
    queue_command (Command ("step-in", "-exec-step", a_cookie));
}

void
GDBEngine::step_out (const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    queue_command (Command ("step-out", "-exec-finish", a_cookie));
}

void
GDBEngine::step_over (const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    queue_command (Command ("step-over", "-exec-next", a_cookie));
}

void
GDBEngine::continue_to_position (const UString &a_path,
                                 gint a_line_num,
                                 const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    queue_command (Command ("continue-to-position", "-exec-until "
                            + a_path
                            + ":"
                            + UString::from_int (a_line_num),
                            a_cookie));
}

void
GDBEngine::set_breakpoint (const UString &a_path,
                           gint a_line_num,
                           const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    //here, don't use the gdb/mi format, because only the cmd line
    //format supports the 'set breakpoint pending' option that lets
    //gdb set pending breakpoint when a breakpoint location doesn't exist.
    //read http://sourceware.org/gdb/current/onlinedocs/gdb_6.html#SEC33
    //Also, we don't neet to explicitely 'set breakpoint pending' to have it
    //work. Even worse, setting it doesn't work.
    queue_command (Command ("set-breakpoint", "break "
                    + a_path
                    + ":"
                    + UString::from_int (a_line_num),
                    a_cookie));
    list_breakpoints (a_cookie);
}

void
GDBEngine::list_breakpoints (const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    queue_command (Command ("list-breakpoint", "-break-list", a_cookie));
}

map<int, IDebugger::BreakPoint>&
GDBEngine::get_cached_breakpoints ()
{
    THROW_IF_FAIL (m_priv);
    return m_priv->cached_breakpoints;
}

void
GDBEngine::append_breakpoints_to_cache
                            (const map<int, IDebugger::BreakPoint> &a_breaks)
{
    map<int, IDebugger::BreakPoint>::const_iterator iter;
    for (iter = a_breaks.begin (); iter != a_breaks.end (); ++iter) {
        m_priv->cached_breakpoints[iter->first] = iter->second;
    }
}

void
GDBEngine::set_breakpoint (const UString &a_func_name,
                           const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    queue_command (Command ("set-breakpoint",
                            "break " + a_func_name,
                            a_cookie));
    list_breakpoints (a_cookie);
}

void
GDBEngine::set_catch (const UString &a_event,
					  const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    queue_command (Command ("catch",
                            "catch " + a_event,
                            a_cookie));
    // explicitly request the breakpoints to be listed otherwise the newly added
    // catchpoint won't show up in the breakpoint list
    list_breakpoints(a_cookie);
}


void
GDBEngine::enable_breakpoint (gint a_break_num,
                              const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    queue_command (Command ("enable-breakpoint",
                            "-break-enable " + UString::from_int (a_break_num),
                            a_cookie));
    list_breakpoints(a_cookie);
}

void
GDBEngine::disable_breakpoint (gint a_break_num,
                               const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    queue_command (Command ("disable-breakpoint",
                            "-break-disable " + UString::from_int (a_break_num),
                            a_cookie));
    list_breakpoints(a_cookie);
}

void
GDBEngine::list_threads (const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    queue_command (Command ("list-threads", "-thread-list-ids", a_cookie));
}

void
GDBEngine::select_thread (unsigned int a_thread_id,
                          const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (a_thread_id);
    queue_command (Command ("select-thread", "-thread-select "
                            + UString::from_int (a_thread_id),
                            a_cookie));
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

    for (unsigned int i=0; i < a_nums.size (); ++i) {
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

void
GDBEngine::list_frames (const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    queue_command (Command ("list-frames",
                            "-stack-list-frames",
                            a_cookie));
}

void
GDBEngine::select_frame (int a_frame_id,
                         const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    queue_command (Command ("select-frame",
                            "-stack-select-frame "
                                    + UString::from_int (a_frame_id),
                            a_cookie));
}

void
GDBEngine::list_frames_arguments (int a_low_frame,
                                  int a_high_frame,
                                  const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    if (a_low_frame < 0 || a_high_frame < 0) {
        queue_command (Command ("list-frames-arguments",
                                "-stack-list-arguments 1",
                                a_cookie));
    } else {
        queue_command (Command ("list-frames-arguments",
                                "-stack-list-arguments 1 "
                                    + UString::from_int (a_low_frame)
                                    + " "
                                    + UString::from_int (a_high_frame),
                                    a_cookie));
    }
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
                              const UString &a_cookie="")
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

    ILangTraitSafePtr lang_trait = get_language_trait ();
    THROW_IF_FAIL (lang_trait);

    if (!lang_trait->has_pointers ()) {
        LOG_ERROR ("current language does not support pointers");
        return false;
    }

    if (!a_var->type ().empty () &&
        !lang_trait->is_type_a_pointer (a_var->type ())) {
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
    THROW_IF_FAIL (m_priv);

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
    UString::size_type process_index=0, exe_index=0, index=0;
    list<Output::OutOfBandRecord>::const_iterator record_iter =
                                    a_output.out_of_band_records ().begin ();
    for (; record_iter != a_output.out_of_band_records ().end (); ++record_iter) {
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
    string var_name, type_name, tmp_str;
    SimpleDeclarationPtr simple_decl;
    InitDeclaratorPtr init_decl;
    ParserPtr parser;
    bool found=false;
    unsigned cur=0;
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
    cur = 5;
    file_name = str.substr (5);
    file_name.chomp ();
    file_name.erase (file_name.length ()-1, 1);
    THROW_IF_FAIL (!file_name.empty ());

fetch_variable:
    found = false;
    //we are looking for a string that end's up with a ";\n"
    for (++oobr_it;
         oobr_it != a_output.out_of_band_records ().end ();
         ++oobr_it) {
        if (!oobr_it->has_stream_record ()) {continue;}

        str = oobr_it->stream_record ().debugger_console ();
        if (str.raw ()[str.raw ().length () - 2] != ';'
            || str.raw ()[str.raw ().length () - 1] != '\n') {
            continue;
        }
        found=true;
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
    LOG_DD ("globals: got variable: " << " Name: " << var_name );

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
                            "-data-list-register-names",
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
                            "-data-list-register-values x", // x = hex format
                            a_cookie));
}

void
GDBEngine::list_register_values (std::list<IDebugger::register_id_t> a_registers,
                           const UString &a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    UString regs_str;
    for (std::list<register_id_t>::const_iterator iter = a_registers.begin ();
            iter != a_registers.end (); ++ iter)
    {
        regs_str += UString::from_int (*iter) + " ";
    }
    queue_command (Command ("list-register-values",
                            "-data-list-register-values x " + regs_str, // x = hex format
                            a_cookie));
}

void
GDBEngine::set_register_value (const UString& a_reg_name,
                               const UString& a_value,
                               const UString& a_cookie)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    UString command_str;
    command_str.printf ("-data-evaluate-expression $%s=%s",
            a_reg_name.c_str (),
            a_value.c_str ());
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
    // When we parse the output from the command, we assume that there's only a
    // single row of output -- if this ever changes, the parsing function will
    // need to be updated
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
        cmd_str.printf ("-data-evaluate-expression \"*(unsigned char*)%zu = 0x%X\"",
                a_addr++,
                *iter);
        Command command("set-memory", cmd_str, a_cookie);
        command.tag0 ("set-memory");
        command.tag1 (UString ().printf ("0x%X",a_addr));
        queue_command (command);
    }
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

