//Author: Fabien Parent
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


#include "nmv-cmd-interpreter.h"
#include "nmv-i-debugger.h"
#include "common/nmv-str-utils.h"
#include <map>
#include <queue>

NEMIVER_BEGIN_NAMESPACE(nemiver)

const char *const NEMIVER_CMD_INTERPRETER_COOKIE = "nemiver-dbg-console";
const unsigned int COMMAND_EXECUTION_TIMEOUT_IN_SECONDS = 10;

struct DebuggingData {
    IDebugger &debugger;
    IDebugger::Frame current_frame;
    UString current_file_path;
    std::vector<UString> source_files;

    DebuggingData (IDebugger &a_debugger) :
        debugger (a_debugger)
    {
    }
};

struct CommandContinue : public CmdInterpreter::SynchronousCommand {
    IDebugger &debugger;

    CommandContinue (IDebugger &a_debugger) :
        debugger (a_debugger)
    {
    }

    const std::string&
    name () const
    {
        static const std::string &s_name = "continue";
        return s_name;
    }

    const std::vector<UString>&
    aliases () const
    {
        static std::vector<UString> s_aliases;
        if (!s_aliases.size ()) {
            s_aliases.push_back ("c");
        }
        return s_aliases;
    }

    void
    execute (const std::vector<UString>&, std::ostream&)
    {
        debugger.do_continue (NEMIVER_CMD_INTERPRETER_COOKIE);
    }
};

struct CommandNext : public CmdInterpreter::SynchronousCommand {
    IDebugger &debugger;

    CommandNext (IDebugger &a_debugger) :
        debugger (a_debugger)
    {
    }

    const std::string&
    name () const
    {
        static const std::string &s_name = "next";
        return s_name;
    }

    const std::vector<UString>&
    aliases () const
    {
        static std::vector<UString> s_aliases;
        if (!s_aliases.size ()) {
            s_aliases.push_back ("n");
        }
        return s_aliases;
    }

    void
    execute (const std::vector<UString>&, std::ostream&)
    {
        debugger.step_over (NEMIVER_CMD_INTERPRETER_COOKIE);
    }
};

struct CommandStep : public CmdInterpreter::SynchronousCommand {
    IDebugger &debugger;

    CommandStep (IDebugger &a_debugger) :
        debugger (a_debugger)
    {
    }

    const std::string&
    name () const
    {
        static const std::string &s_name = "step";
        return s_name;
    }

    const std::vector<UString>&
    aliases () const
    {
        static std::vector<UString> s_aliases;
        if (!s_aliases.size ()) {
            s_aliases.push_back ("s");
        }
        return s_aliases;
    }

    void
    execute (const std::vector<UString>&, std::ostream&)
    {
        debugger.step_in (NEMIVER_CMD_INTERPRETER_COOKIE);
    }
};

struct CommandNexti : public CmdInterpreter::SynchronousCommand {
    IDebugger &debugger;

    CommandNexti (IDebugger &a_debugger) :
        debugger (a_debugger)
    {
    }

    const std::string&
    name () const
    {
        static const std::string &s_name = "nexti";
        return s_name;
    }

    const std::vector<UString>&
    aliases () const
    {
        static std::vector<UString> s_aliases;
        if (!s_aliases.size ()) {
            s_aliases.push_back ("ni");
        }
        return s_aliases;
    }

    void
    execute (const std::vector<UString>&, std::ostream&)
    {
        debugger.step_over_asm (NEMIVER_CMD_INTERPRETER_COOKIE);
    }
};

struct CommandStepi : public CmdInterpreter::SynchronousCommand {
    IDebugger &debugger;

    CommandStepi (IDebugger &a_debugger) :
        debugger (a_debugger)
    {
    }

    const std::string&
    name () const
    {
        static const std::string &s_name = "stepi";
        return s_name;
    }

    const std::vector<UString>&
    aliases () const
    {
        static std::vector<UString> s_aliases;
        if (!s_aliases.size ()) {
            s_aliases.push_back ("si");
        }
        return s_aliases;
    }

    void
    execute (const std::vector<UString>&, std::ostream&)
    {
        debugger.step_in_asm (NEMIVER_CMD_INTERPRETER_COOKIE);
    }
};

struct CommandStop : public CmdInterpreter::SynchronousCommand {
    IDebugger &debugger;

    CommandStop (IDebugger &a_debugger) :
        debugger (a_debugger)
    {
    }

    const std::string&
    name () const
    {
        static const std::string &s_name = "stop";
        return s_name;
    }

    void
    execute (const std::vector<UString>&, std::ostream&)
    {
        debugger.stop_target ();
    }
};

struct CommandFinish : public CmdInterpreter::SynchronousCommand {
    IDebugger &debugger;

    CommandFinish (IDebugger &a_debugger) :
        debugger (a_debugger)
    {
    }

    const std::string&
    name () const
    {
        static const std::string &s_name = "finish";
        return s_name;
    }

    void
    execute (const std::vector<UString>&, std::ostream&)
    {
        debugger.step_out (NEMIVER_CMD_INTERPRETER_COOKIE);
    }
};

struct CommandCall : public CmdInterpreter::SynchronousCommand {
    IDebugger &debugger;
    std::string cmd;

    CommandCall (IDebugger &a_debugger) :
        debugger (a_debugger)
    {
    }

    const std::string&
    name () const
    {
        static const std::string &s_name = "call";
        return s_name;
    }

    void
    execute (const std::vector<UString> &a_argv, std::ostream &a_stream)
    {
        if (a_argv.size ()) {
            cmd = str_utils::join (a_argv);
        }

        if (cmd.empty ()) {
            a_stream << "The history is empty.\n";
        } else {
            debugger.call_function (cmd, NEMIVER_CMD_INTERPRETER_COOKIE);
        }
    }
};

struct CommandThread : public CmdInterpreter::AsynchronousCommand {
    IDebugger &debugger;

    CommandThread (IDebugger &a_debugger) :
        debugger (a_debugger)
    {
    }

    const std::string&
    name () const
    {
        static const std::string &s_name = "thread";
        return s_name;
    }

    void
    completions (const std::vector<UString> &a_argv,
                 std::vector<UString> &a_completion_vector) const
    {
        if (a_argv.size () == 0) {
            a_completion_vector.push_back ("list");
        }
    }

    void
    display_usage (const std::vector<UString> &a_argv,
                   std::ostream &a_stream) const
    {
        if (a_argv.size ()) {
            return;
        }

        a_stream << "Usage:\n"
                 << "\tthread\n"
                 << "\tthread [THREAD ID]\n"
                 << "\tthread list\n";
    }

    void
    threads_listed_signal (const std::list<int> a_thread_ids,
                           const UString &a_cookie,
                           std::ostream &a_stream)
    {
        NEMIVER_TRY

        if (a_cookie != NEMIVER_CMD_INTERPRETER_COOKIE) {
            return;
        }

        a_stream << "Threads:\n";
        for (std::list<int>::const_iterator iter = a_thread_ids.begin ();
             iter != a_thread_ids.end ();
             ++iter) {
            a_stream << *iter << "\n";
        }
        done_signal ().emit ();

        NEMIVER_CATCH_NOX
    }

    void
    execute (const std::vector<UString> &a_argv, std::ostream &a_stream)
    {
        if (a_argv.size () > 1) {
            a_stream << "Too much parameters.\n";
        } else if (!a_argv.size ()) {
            a_stream << "Current thread ID: " << debugger.get_current_thread ()
                     << ".\n";
        } else if (str_utils::string_is_number (a_argv[0])) {
            debugger.select_thread
                (str_utils::from_string<unsigned int> (a_argv[0]));
        } else if (a_argv[0] == "list") {
            debugger.threads_listed_signal ().connect
                (sigc::bind<std::ostream&> (sigc::mem_fun
                    (*this, &CommandThread::threads_listed_signal),
                 a_stream));
            debugger.list_threads (NEMIVER_CMD_INTERPRETER_COOKIE);
            return;
        } else {
            a_stream << "Invalid argument: " << a_argv[0] << ".\n";
        }

        done_signal ().emit ();
    }
};

struct CommandBreak : public CmdInterpreter::SynchronousCommand {
    DebuggingData &dbg_data;

    CommandBreak (DebuggingData &a_dbg_data) :
        dbg_data (a_dbg_data)
    {
    }

    const std::string&
    name () const
    {
        static const std::string &s_name = "break";
        return s_name;
    }

    const std::vector<UString>&
    aliases () const
    {
        static std::vector<UString> s_aliases;
        if (!s_aliases.size ()) {
            s_aliases.push_back ("b");
        }
        return s_aliases;
    }

    void
    display_usage (const std::vector<UString>&, std::ostream &a_stream) const
    {
        a_stream << "Usage:\n"
                 << "\tbreak\n"
                 << "\tbreak [LINE]\n"
                 << "\tbreak [FUNCTION]\n"
                 << "\tbreak *[ADDRESS]\n"
                 << "\tbreak +[OFFSET]\n"
                 << "\tbreak -[OFFSET]\n";
    }

    void
    break_at_current_line (std::ostream &a_stream)
    {
        IDebugger::Frame &frame = dbg_data.current_frame;
        IDebugger &debugger = dbg_data.debugger;

        if (!frame.file_full_name ().empty ()) {
            debugger.set_breakpoint (frame.file_full_name (), frame.line ());
        } else {
            a_stream << "Cannot set a breakpoint at this position.\n";
        }
    }

    void
    break_at_line (const std::vector<UString> &a_argv,
                   std::ostream &a_stream)
    {
        IDebugger &debugger = dbg_data.debugger;

        if (!dbg_data.current_file_path.empty ()) {
            debugger.set_breakpoint (dbg_data.current_file_path,
                                     str_utils::from_string<int> (a_argv[0]));
        } else {
            a_stream << "Cannot set a breakpoint at this position.\n";
        }
    }

    void
    break_at_offset (const std::vector<UString> &a_argv,
                     std::ostream &a_stream)
    {
        IDebugger::Frame &frame = dbg_data.current_frame;
        IDebugger &debugger = dbg_data.debugger;

        if (frame.file_full_name ().empty ()) {
            a_stream << "Cannot set a breakpoint at this position.\n";
            return;
        }

        std::string offset (a_argv[0].substr (1));
        if (!str_utils::string_is_decimal_number (offset)) {
            a_stream << "Invalid offset: " << offset << ".\n";
            return;
        }

        int line = frame.line ();
        if (a_argv[0][0] == '+') {
            line += str_utils::from_string<int> (offset);
        } else {
            line -= str_utils::from_string<int> (offset);
        }

        debugger.set_breakpoint
            (frame.file_full_name (), line, NEMIVER_CMD_INTERPRETER_COOKIE);
    }

    void
    break_at_address (const std::vector<UString> &a_argv,
                      std::ostream &a_stream)
    {
        IDebugger &debugger = dbg_data.debugger;

        std::string addr (a_argv[0].substr (1));
        if (str_utils::string_is_hexa_number (addr)) {
            Address address (addr);
            debugger.set_breakpoint (address, NEMIVER_CMD_INTERPRETER_COOKIE);
        } else {
            a_stream << "Invalid address: " << addr << ".\n";
        }
    }

    void
    execute (const std::vector<UString> &a_argv, std::ostream &a_stream)
    {
        IDebugger &debugger = dbg_data.debugger;

        if (a_argv.size () > 1) {
            a_stream << "Too much parameters.\n";
            return;
        }

        if (a_argv.size () == 0) {
            break_at_current_line (a_stream);
            return;
        }

        const char first_param_char = a_argv[0][0];
        if (str_utils::string_is_number (a_argv[0])) {
            break_at_line (a_argv, a_stream);
        } else if ((first_param_char >= 'a' && first_param_char <= 'z')
                   || first_param_char == '_') {
            debugger.set_breakpoint (a_argv[0], NEMIVER_CMD_INTERPRETER_COOKIE);
        } else if (first_param_char == '*') {
            break_at_address (a_argv, a_stream);
        } else if (first_param_char == '+' || first_param_char == '-') {
            break_at_offset (a_argv, a_stream);
        } else {
            a_stream << "Invalid argument: " << a_argv[0] << ".\n";
        }
    }
};

struct CommandPrint : public CmdInterpreter::AsynchronousCommand {
    IDebugger &debugger;
    std::string expression;

    CommandPrint (IDebugger &a_debugger) :
        debugger (a_debugger)
    {
    }

    const std::string&
    name () const
    {
        static const std::string &s_name = "print";
        return s_name;
    }

    void
    on_variable_created_signal (const IDebugger::VariableSafePtr a_var,
                                std::ostream &a_stream)
    {
        NEMIVER_TRY

        THROW_IF_FAIL (a_var);
        a_stream << a_var->name () << " = " << a_var->value () << "\n";
        debugger.delete_variable (a_var);
        done_signal ().emit ();

        NEMIVER_CATCH_NOX
    }

    void
    execute (const std::vector<UString> &a_argv, std::ostream &a_stream)
    {
        if (a_argv.size ()) {
            expression.clear ();
        }

        for (std::vector<UString>::const_iterator iter = a_argv.begin ();
             iter != a_argv.end ();
             ++iter) {
            expression += *iter;
        }

        if (expression.empty ()) {
            a_stream << "No history\n";
            done_signal ().emit ();
            return;
        }

        debugger.create_variable
            (expression, sigc::bind<std::ostream&>
                (sigc::mem_fun
                    (*this, &CommandPrint::on_variable_created_signal),
                 a_stream));
    }
};

struct CommandOpen : public CmdInterpreter::SynchronousCommand {
    DebuggingData &dbg_data;
    sigc::signal<void, UString> file_opened_signal;

    CommandOpen (DebuggingData &a_dbg_data) :
        dbg_data (a_dbg_data)
    {
    }

    const std::string&
    name () const
    {
        static const std::string &s_name = "open";
        return s_name;
    }

    const std::vector<UString>&
    aliases () const
    {
        static std::vector<UString> s_aliases;
        if (!s_aliases.size ()) {
            s_aliases.push_back ("o");
        }
        return s_aliases;
    }

    void
    completions (const std::vector<UString>&,
                 std::vector<UString> &a_completion_vector) const
    {
        a_completion_vector.insert (a_completion_vector.begin (),
                                    dbg_data.source_files.begin (),
                                    dbg_data.source_files.end ());
    }

    void
    execute (const std::vector<UString> &a_argv, std::ostream&)
    {
        for (std::vector<UString>::const_iterator iter = a_argv.begin ();
             iter != a_argv.end ();
             ++iter) {
            UString path = *iter;
            if (path.size () && path[0] == '~') {
                path = path.replace (0, 1, Glib::get_home_dir ());
            }
            file_opened_signal.emit (path);
        }
    }
};

struct CommandLoadExec : public CmdInterpreter::SynchronousCommand {
    IDebugger &debugger;

    CommandLoadExec (IDebugger &a_debugger) :
        debugger (a_debugger)
    {
    }

    const std::string&
    name () const
    {
        static const std::string &s_name = "load-exec";
        return s_name;
    }

    void
    display_usage (const std::vector<UString>&, std::ostream &a_stream) const
    {
        a_stream << "Usage:\n"
                 << "\tload-exec PROGRAM_NAME [ARG1 ARG2 ...]\n";
    }

    void
    execute (const std::vector<UString> &a_argv, std::ostream &a_stream)
    {
        std::vector<UString> argv;
        if (!a_argv.size ()) {
            display_usage (argv, a_stream);
            return;
        }

        for (std::vector<UString>::const_iterator iter = a_argv.begin ();
             iter != a_argv.end ();
             ++iter) {
            UString path = *iter;
            if (path.size () && path[0] == '~') {
                path = path.replace (0, 1, Glib::get_home_dir ());
            }
            argv.push_back (path);
        }

        UString prog = argv[0];
        argv.erase (argv.begin ());

        if (!debugger.load_program (prog, argv)) {
            a_stream << "Could not load program '" << prog << "'.\n";
        }
    }
};

struct CommandRun : public CmdInterpreter::SynchronousCommand {
    IDebugger &debugger;

    CommandRun (IDebugger &a_debugger) :
        debugger (a_debugger)
    {
    }

    const std::string&
    name () const
    {
        static const std::string &s_name = "run";
        return s_name;
    }

    void
    execute (const std::vector<UString>&, std::ostream&)
    {
        debugger.run (NEMIVER_CMD_INTERPRETER_COOKIE);
    }
};

struct CmdInterpreter::Priv {
    std::vector<CmdInterpreter::Command*> command_vector;
    std::map<std::string, CmdInterpreter::Command&> commands;
    std::queue<UString> command_queue;
    std::ostream &output_stream;
    sigc::signal<void> ready_signal;

    sigc::connection cmd_execution_done_connection;
    sigc::connection cmd_execution_timeout_connection;
    bool done_signal_received;

    DebuggingData data;

    CommandContinue cmd_continue;
    CommandNext cmd_next;
    CommandStep cmd_step;
    CommandBreak cmd_break;
    CommandPrint cmd_print;
    CommandCall cmd_call;
    CommandFinish cmd_finish;
    CommandThread cmd_thread;
    CommandStop cmd_stop;
    CommandNexti cmd_nexti;
    CommandStepi cmd_stepi;
    CommandOpen cmd_open;
    CommandLoadExec cmd_load_exec;
    CommandRun cmd_run;

    Priv (IDebugger &a_debugger, std::ostream &a_output_stream) :
        output_stream (a_output_stream),
        done_signal_received (true),
        data (a_debugger),
        cmd_continue (a_debugger),
        cmd_next (a_debugger),
        cmd_step (a_debugger),
        cmd_break (data),
        cmd_print (a_debugger),
        cmd_call (a_debugger),
        cmd_finish (a_debugger),
        cmd_thread (a_debugger),
        cmd_stop (a_debugger),
        cmd_nexti (a_debugger),
        cmd_stepi (a_debugger),
        cmd_open (data),
        cmd_load_exec (a_debugger),
        cmd_run (a_debugger)
    {
        init_signals ();
    }

    void
    init_signals ()
    {
        data.debugger.stopped_signal ().connect
            (sigc::mem_fun (*this, &CmdInterpreter::Priv::on_stopped_signal));
        data.debugger.files_listed_signal ().connect (sigc::mem_fun
            (*this, &CmdInterpreter::Priv::on_files_listed_signal));
    }

    void
    on_stopped_signal (IDebugger::StopReason,
                       bool,
                       const IDebugger::Frame &a_frame,
                       int,
                       int,
                       const UString&)
    {
        data.current_frame = a_frame;
        data.current_file_path = a_frame.file_full_name ();
    }

    void
    on_files_listed_signal (const std::vector<UString> &a_files, const UString&)
    {
        data.source_files = a_files;
    }

    bool
    execute_command (const UString &a_buffer)
    {
        std::string command_name;
        std::vector<UString> cmd_argv;

        std::istringstream is (a_buffer);
        is >> command_name;

        while (is.good ()) {
            std::string arg;
            is >> arg;
            cmd_argv.push_back (arg);
        }

        if (command_name.empty ()) {
            ready_signal.emit ();
            return false;
        }

        if (!commands.count (command_name)) {
            output_stream << "Undefined command: " << command_name << ".\n";
            ready_signal.emit ();
            return false;
        }

        Command &command = commands.at (command_name);
        done_signal_received = false;
        cmd_execution_done_connection = command.done_signal ().connect
            (sigc::mem_fun (*this, &CmdInterpreter::Priv::on_done_signal));
        commands.at (command_name) (cmd_argv, output_stream);
        cmd_execution_timeout_connection =
            Glib::signal_timeout().connect_seconds (sigc::mem_fun
                (*this, &CmdInterpreter::Priv::on_cmd_execution_timeout_signal),
            COMMAND_EXECUTION_TIMEOUT_IN_SECONDS);

        return true;
    }

    bool
    on_cmd_execution_timeout_signal ()
    {
        NEMIVER_TRY
        on_done_signal ();
        NEMIVER_CATCH_NOX

        return true;
    }

    void
    on_done_signal ()
    {
        NEMIVER_TRY

        if (done_signal_received) {
            return;
        }

        done_signal_received = true;
        cmd_execution_done_connection.disconnect ();
        cmd_execution_timeout_connection.disconnect ();

        if (command_queue.size ()) {
            process_command_queue ();
        } else {
            ready_signal.emit ();
        }

        NEMIVER_CATCH_NOX
    }

    void
    process_command_queue ()
    {
        bool is_command_launched_successfully = false;
        while (!is_command_launched_successfully && command_queue.size ()) {
            NEMIVER_TRY
            UString command = command_queue.front ();
            command_queue.pop ();
            is_command_launched_successfully = execute_command (command);
            NEMIVER_CATCH_NOX
        }
    }

    void
    queue_command (const UString &a_command)
    {
        NEMIVER_TRY

        if (a_command.empty ()) {
            ready_signal.emit ();
            return;
        }

        command_queue.push (a_command);
        if (command_queue.size () == 1 && done_signal_received) {
            process_command_queue ();
        }

        NEMIVER_CATCH_NOX
    }
};

CmdInterpreter::CmdInterpreter (IDebugger &a_debugger,
                                std::ostream &a_output_stream) :
    m_priv (new Priv (a_debugger, a_output_stream))
{
    register_command (m_priv->cmd_continue);
    register_command (m_priv->cmd_next);
    register_command (m_priv->cmd_step);
    register_command (m_priv->cmd_break);
    register_command (m_priv->cmd_print);
    register_command (m_priv->cmd_call);
    register_command (m_priv->cmd_finish);
    register_command (m_priv->cmd_thread);
    register_command (m_priv->cmd_stop);
    register_command (m_priv->cmd_nexti);
    register_command (m_priv->cmd_stepi);
    register_command (m_priv->cmd_open);
    register_command (m_priv->cmd_load_exec);
    register_command (m_priv->cmd_run);
}

CmdInterpreter::~CmdInterpreter ()
{
}

void
CmdInterpreter::register_command (CmdInterpreter::Command &a_command)
{
    THROW_IF_FAIL (m_priv);

    if (m_priv->commands.count (a_command.name ())) {
        LOG ("Command '" << a_command.name () << "' is already registered in"
             " the console. The previous command will be overwritten");
    }

    m_priv->commands.insert (std::make_pair<std::string, Command&>
        (a_command.name (), a_command));
    m_priv->command_vector.push_back (&a_command);

    const std::vector<UString> &aliases = a_command.aliases ();
    for (std::vector<UString>::const_iterator iter = aliases.begin ();
         iter != aliases.end ();
         ++iter) {
        if (m_priv->commands.count (*iter)) {
            LOG ("Command '" << *iter << "' is already registered in"
                 " the console. The previous command will be overwritten");
        }
        m_priv->commands.insert (std::make_pair<std::string, Command&>
            (*iter, a_command));
    }
}

void
CmdInterpreter::current_file_path (const UString &a_file_path)
{
    THROW_IF_FAIL (m_priv);
    m_priv->data.current_file_path = a_file_path;
}

const UString&
CmdInterpreter::current_file_path () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->data.current_file_path;
}

sigc::signal<void, UString>&
CmdInterpreter::file_opened_signal () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->cmd_open.file_opened_signal;
}

sigc::signal<void>&
CmdInterpreter::ready_signal () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->ready_signal;
}

const std::vector<CmdInterpreter::Command*>&
CmdInterpreter::commands() const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->command_vector;
}

void
CmdInterpreter::execute_command (const UString &a_command)
{
    THROW_IF_FAIL (m_priv);
    m_priv->queue_command (a_command);
}

bool
CmdInterpreter::ready () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->done_signal_received && !m_priv->command_queue.size ();
}

NEMIVER_END_NAMESPACE(nemiver)

