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

const char *const NEMIVER_CMD_INTERPRETER_COOKIE = "nemiver-cmd-interpreter";
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

struct ContinueCommand : public CmdInterpreter::Command {
    IDebugger &debugger;

    ContinueCommand (IDebugger &a_debugger) :
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

    bool
    execute (const std::vector<UString>&, std::ostream&)
    {
        debugger.do_continue (NEMIVER_CMD_INTERPRETER_COOKIE);
        return true;
    }
};

struct NextCommand : public CmdInterpreter::Command {
    IDebugger &debugger;

    NextCommand (IDebugger &a_debugger) :
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

    bool
    execute (const std::vector<UString>&, std::ostream&)
    {
        debugger.step_over (NEMIVER_CMD_INTERPRETER_COOKIE);
        return true;
    }
};

struct StepCommand : public CmdInterpreter::Command {
    IDebugger &debugger;

    StepCommand (IDebugger &a_debugger) :
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

    bool
    execute (const std::vector<UString>&, std::ostream&)
    {
        debugger.step_in (NEMIVER_CMD_INTERPRETER_COOKIE);
        return true;
    }
};

struct NextiCommand : public CmdInterpreter::Command {
    IDebugger &debugger;

    NextiCommand (IDebugger &a_debugger) :
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

    bool
    execute (const std::vector<UString>&, std::ostream&)
    {
        debugger.step_over_asm (NEMIVER_CMD_INTERPRETER_COOKIE);
        return true;
    }
};

struct StepiCommand : public CmdInterpreter::Command {
    IDebugger &debugger;

    StepiCommand (IDebugger &a_debugger) :
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

    bool
    execute (const std::vector<UString>&, std::ostream&)
    {
        debugger.step_in_asm (NEMIVER_CMD_INTERPRETER_COOKIE);
        return true;
    }
};

struct StopCommand : public CmdInterpreter::Command {
    IDebugger &debugger;

    StopCommand (IDebugger &a_debugger) :
        debugger (a_debugger)
    {
    }

    const std::string&
    name () const
    {
        static const std::string &s_name = "stop";
        return s_name;
    }

    bool
    execute (const std::vector<UString>&, std::ostream&)
    {
        debugger.stop_target ();
        return true;
    }
};

struct FinishCommand : public CmdInterpreter::Command {
    IDebugger &debugger;

    FinishCommand (IDebugger &a_debugger) :
        debugger (a_debugger)
    {
    }

    const std::string&
    name () const
    {
        static const std::string &s_name = "finish";
        return s_name;
    }

    bool
    execute (const std::vector<UString>&, std::ostream&)
    {
        debugger.step_out (NEMIVER_CMD_INTERPRETER_COOKIE);
        return true;
    }
};

struct CallCommand : public CmdInterpreter::Command {
    IDebugger &debugger;
    std::string cmd;

    CallCommand (IDebugger &a_debugger) :
        debugger (a_debugger)
    {
    }

    const std::string&
    name () const
    {
        static const std::string &s_name = "call";
        return s_name;
    }

    bool
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

        return true;
    }
};

struct ThreadCommand : public CmdInterpreter::Command {
    IDebugger &debugger;

    ThreadCommand (IDebugger &a_debugger) :
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
        NEMIVER_TRY;

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

        NEMIVER_CATCH_NOX;
    }

    bool
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
                    (*this, &ThreadCommand::threads_listed_signal),
                 a_stream));
            debugger.list_threads (NEMIVER_CMD_INTERPRETER_COOKIE);
            return false;
        } else {
            a_stream << "Invalid argument: " << a_argv[0] << ".\n";
        }

        return true;
    }
};

struct BreakCommand : public CmdInterpreter::Command {
    DebuggingData &dbg_data;

    BreakCommand (DebuggingData &a_dbg_data) :
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

        if (frame.file_full_name ().empty ()) {
            a_stream << "Cannot set a breakpoint at this position.\n";
        } else {
            debugger.set_breakpoint (frame.file_full_name (), frame.line ());
        }
    }

    void
    break_at_line (const std::vector<UString> &a_argv,
                   std::ostream &a_stream)
    {
        IDebugger &debugger = dbg_data.debugger;

        if (dbg_data.current_file_path.empty ()) {
            a_stream << "Cannot set a breakpoint at this position.\n";
        } else {
            THROW_IF_FAIL (a_argv.size());
            debugger.set_breakpoint (dbg_data.current_file_path,
                                     str_utils::from_string<int> (a_argv[0]));
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

        THROW_IF_FAIL (a_argv.size());
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

        THROW_IF_FAIL (a_argv.size());
        std::string addr (a_argv[0].substr (1));
        if (!str_utils::string_is_hexa_number (addr)) {
            a_stream << "Invalid address: " << addr << ".\n";
        } else {
            debugger.set_breakpoint
                (Address (addr), NEMIVER_CMD_INTERPRETER_COOKIE);
        }
    }

    bool
    execute (const std::vector<UString> &a_argv, std::ostream &a_stream)
    {
        IDebugger &debugger = dbg_data.debugger;

        if (a_argv.size () > 1) {
            a_stream << "Too much parameters.\n";
            return true;
        }

        if (a_argv.size () == 0) {
            break_at_current_line (a_stream);
            return true;
        }

        const char first_param_char = a_argv[0][0];
        if (str_utils::string_is_number (a_argv[0])) {
            break_at_line (a_argv, a_stream);
        } else if ((first_param_char >= 'a' && first_param_char <= 'z')
                   || first_param_char == '_') {
            debugger.set_breakpoint
                (a_argv[0], "", 0, NEMIVER_CMD_INTERPRETER_COOKIE);
        } else if (first_param_char == '*') {
            break_at_address (a_argv, a_stream);
        } else if (first_param_char == '+' || first_param_char == '-') {
            break_at_offset (a_argv, a_stream);
        } else {
            a_stream << "Invalid argument: " << a_argv[0] << ".\n";
        }

        return true;
    }
};

struct PrintCommand : public CmdInterpreter::Command {
    IDebugger &debugger;
    std::string expression;

    PrintCommand (IDebugger &a_debugger) :
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
        NEMIVER_TRY;

        THROW_IF_FAIL (a_var);
        a_stream << a_var->name () << " = " << a_var->value () << "\n";
        debugger.delete_variable (a_var);
        done_signal ().emit ();

        NEMIVER_CATCH_NOX;
    }

    bool
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
            return true;
        }

        debugger.create_variable
            (expression, sigc::bind<std::ostream&>
                (sigc::mem_fun
                    (*this, &PrintCommand::on_variable_created_signal),
                 a_stream));
        return false;
    }
};

struct LoadExecCommand : public CmdInterpreter::Command {
    IDebugger &debugger;

    LoadExecCommand (IDebugger &a_debugger) :
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

    bool
    execute (const std::vector<UString> &a_argv, std::ostream &a_stream)
    {
        std::vector<UString> argv;
        if (!a_argv.size ()) {
            display_usage (argv, a_stream);
            return true;
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

        return true;
    }
};

struct RunCommand : public CmdInterpreter::Command {
    IDebugger &debugger;

    RunCommand (IDebugger &a_debugger) :
        debugger (a_debugger)
    {
    }

    const std::string&
    name () const
    {
        static const std::string &s_name = "run";
        return s_name;
    }

    bool
    execute (const std::vector<UString>&, std::ostream&)
    {
        debugger.run (NEMIVER_CMD_INTERPRETER_COOKIE);
        return true;
    }
};

struct CmdInterpreter::Priv {
    std::vector<CommandSafePtr> commands;
    std::vector<CmdInterpreter::Command*> command_vector;
    std::map<std::string, CmdInterpreter::Command&> command_map;
    std::queue<UString> command_queue;
    std::ostream &output_stream;
    sigc::signal<void> ready_signal;

    sigc::connection cmd_execution_done_connection;
    sigc::connection cmd_execution_timeout_connection;
    bool has_a_command_running;

    DebuggingData data;

    Priv (IDebugger &a_debugger, std::ostream &a_output_stream) :
        output_stream (a_output_stream),
        has_a_command_running (false),
        data (a_debugger)
    {
        init_commands ();
        init_signals ();
    }

    void
    init_commands ()
    {
        commands.push_back (CommandSafePtr (new NextCommand (data.debugger)));
        commands.push_back (CommandSafePtr (new StepCommand (data.debugger)));
        commands.push_back (CommandSafePtr (new BreakCommand (data)));
        commands.push_back (CommandSafePtr (new PrintCommand (data.debugger)));
        commands.push_back (CommandSafePtr (new CallCommand (data.debugger)));
        commands.push_back (CommandSafePtr (new FinishCommand (data.debugger)));
        commands.push_back (CommandSafePtr (new ThreadCommand (data.debugger)));
        commands.push_back (CommandSafePtr (new StopCommand (data.debugger)));
        commands.push_back (CommandSafePtr (new NextiCommand (data.debugger)));
        commands.push_back (CommandSafePtr (new StepiCommand (data.debugger)));
        commands.push_back (CommandSafePtr (new RunCommand (data.debugger)));
        commands.push_back
            (CommandSafePtr (new LoadExecCommand (data.debugger)));
        commands.push_back
            (CommandSafePtr (new ContinueCommand (data.debugger)));
    }

    void
    init_signals ()
    {
        data.debugger.stopped_signal ().connect
            (sigc::mem_fun (*this, &CmdInterpreter::Priv::on_stopped_signal));
        data.debugger.files_listed_signal ().connect (sigc::mem_fun
            (*this, &CmdInterpreter::Priv::on_files_listed_signal));
        data.debugger.state_changed_signal ().connect (sigc::mem_fun
            (*this, &CmdInterpreter::Priv::on_state_changed_signal));
    }

    void
    on_state_changed_signal (IDebugger::State a_state)
    {
        if (a_state == IDebugger::READY) {
            process_command_queue ();
        }
    }

    void
    on_stopped_signal (IDebugger::StopReason,
                       bool,
                       const IDebugger::Frame &a_frame,
                       int,
                       const string&,
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
        THROW_IF_FAIL (!has_a_command_running);

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
            return has_a_command_running;
        }

        if (!command_map.count (command_name)) {
            output_stream << "Undefined command: " << command_name << ".\n";
            ready_signal.emit ();
            return has_a_command_running;
        }

        Command &command = command_map.at (command_name);
        has_a_command_running = true;
        cmd_execution_done_connection = command.done_signal ().connect
            (sigc::mem_fun (*this, &CmdInterpreter::Priv::on_done_signal));
        command (cmd_argv, output_stream);
        cmd_execution_timeout_connection =
            Glib::signal_timeout().connect_seconds (sigc::mem_fun
                (*this, &CmdInterpreter::Priv::on_cmd_execution_timeout_signal),
            COMMAND_EXECUTION_TIMEOUT_IN_SECONDS);

        return has_a_command_running;
    }

    bool
    on_cmd_execution_timeout_signal ()
    {
        NEMIVER_TRY;
        on_done_signal ();
        NEMIVER_CATCH_NOX;

        return true;
    }

    void
    on_done_signal ()
    {
        NEMIVER_TRY;

        if (!has_a_command_running) {
            return;
        }

        has_a_command_running = false;
        cmd_execution_done_connection.disconnect ();
        cmd_execution_timeout_connection.disconnect ();

        if (command_queue.size ()) {
            process_command_queue ();
        } else {
            ready_signal.emit ();
        }

        NEMIVER_CATCH_NOX;
    }

    void
    process_command_queue ()
    {
        bool is_busy = has_a_command_running;
        while (!is_busy && command_queue.size ()
               && data.debugger.get_state () != IDebugger::RUNNING) {
            NEMIVER_TRY;
            UString command = command_queue.front ();
            command_queue.pop ();
            is_busy = execute_command (command);
            NEMIVER_CATCH_NOX;
        }
    }

    void
    queue_command (const UString &a_command)
    {
        NEMIVER_TRY;

        if (a_command.empty ()) {
            ready_signal.emit ();
            return;
        }

        command_queue.push (a_command);
        process_command_queue ();

        NEMIVER_CATCH_NOX;
    }

    void
    register_command_alias (const std::string& a_alias,
                            CmdInterpreter::Command &a_command)
    {
        if (command_map.count (a_alias)) {
            LOG ("Command '" << a_alias << "' is already registered in"
                 " the command interpreter. The previous command will be"
                 " overwritten");
        }

        command_map.insert (std::make_pair<std::string, Command&>
            (a_alias, a_command));
    }
};

CmdInterpreter::CmdInterpreter (IDebugger &a_debugger,
                                std::ostream &a_output_stream) :
    m_priv (new Priv (a_debugger, a_output_stream))
{
    THROW_IF_FAIL (m_priv);

    for (std::vector<CommandSafePtr >::iterator iter =
            m_priv->commands.begin ();
         iter != m_priv->commands.end ();
         ++iter) {
        THROW_IF_FAIL (*iter);
        register_command (**iter);
    }
}

CmdInterpreter::~CmdInterpreter ()
{
}

void
CmdInterpreter::register_command (CmdInterpreter::Command &a_command)
{
    THROW_IF_FAIL (m_priv);

    m_priv->command_vector.push_back (&a_command);
    m_priv->register_command_alias (a_command.name (), a_command);

    const std::vector<UString> &aliases = a_command.aliases ();
    for (std::vector<UString>::const_iterator iter = aliases.begin ();
         iter != aliases.end ();
         ++iter) {
        m_priv->register_command_alias (*iter, a_command);
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
    return !m_priv->has_a_command_running && !m_priv->command_queue.size ();
}

NEMIVER_END_NAMESPACE(nemiver)

