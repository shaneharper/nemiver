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

#define USE_VARARGS
#define PREFER_STDARG

#include "nmv-console.h"
#include "common/nmv-str-utils.h"
#include "uicommon/nmv-terminal.h"
#include "dbgengine/nmv-cmd-interpreter.h"
#include "dbgengine/nmv-i-debugger.h"
#include <vector>
#include <cstring>
#include <fstream>
#include <cctype>
#include <readline/readline.h>
#include <readline/history.h>

NEMIVER_BEGIN_NAMESPACE(nemiver)

const char *const CONSOLE_PROMPT = "> ";

struct Console::Priv {
    Terminal terminal;
    Console::Stream stream;
    CmdInterpreter cmd_interpreter;
    Glib::RefPtr<Glib::IOSource> io_source;
    IDebugger &debugger;

    struct readline_state console_state;
    struct readline_state saved_state;

    Priv (IDebugger &a_debugger,
          const std::string &a_menu_file_path,
          const Glib::RefPtr<Gtk::UIManager> &a_ui_manager) :
        terminal (a_menu_file_path, a_ui_manager),
        stream (terminal.slave_fd ()),
        cmd_interpreter (a_debugger, stream),
        io_source (Glib::IOSource::create (terminal.slave_fd (), Glib::IO_IN)),
        debugger (a_debugger)
    {
        init ();
    }

    void
    init ()
    {
        int fd = terminal.slave_fd ();
        THROW_IF_FAIL (fd);
        if (consoles ().count (fd)) {
            THROW ("Cannot create two consoles from the same file descriptor.");
        }
        consoles ()[fd] = this;

        cmd_interpreter.ready_signal ().connect
            (sigc::mem_fun (*this, &Console::Priv::on_ready_signal));

        io_source->connect (sigc::mem_fun (*this, &Console::Priv::read_char));
        io_source->attach ();

        rl_save_state (&saved_state);
        rl_instream = fdopen (fd, "r");
        rl_outstream = fdopen (fd, "w");
        rl_bind_key ('\t', &Console::Priv::on_tab_key_pressed);
        rl_callback_handler_install (CONSOLE_PROMPT,
                                     &Console::Priv::process_command);
        rl_already_prompted = true;
        rl_save_state (&console_state);
        rl_restore_state (&saved_state);
    }

    void
    on_ready_signal ()
    {
        stream << CONSOLE_PROMPT;
    }

    bool
    read_char (Glib::IOCondition)
    {
        NEMIVER_TRY

        if (!cmd_interpreter.ready ())
            return false;

        rl_restore_state (&console_state);
        rl_callback_read_char ();
        rl_save_state (&console_state);
        rl_restore_state (&saved_state);

        NEMIVER_CATCH_NOX

        return true;
    }

    void
    do_completion (const std::string &a_completion)
    {
        size_t buffer_length = std::strlen (rl_line_buffer);
        rl_extend_line_buffer (buffer_length + a_completion.size ());
        std::memcpy (rl_line_buffer + rl_point + a_completion.size (),
                     rl_line_buffer + rl_point,
                     a_completion.size ());
        for (size_t i = 0; i < a_completion.size(); i++) {
            rl_line_buffer[rl_point + i] = a_completion[i];
        }
        rl_end += a_completion.size ();
        rl_point += a_completion.size ();
    }

    void
    display_message (const std::string &a_msg)
    {
        rl_save_prompt ();
        rl_message ("%s%s\n%s\n%s",
                    rl_display_prompt,
                    std::string (rl_line_buffer, rl_end).c_str (),
                    a_msg.c_str (),
                    rl_display_prompt);
        rl_clear_message ();
        rl_restore_prompt ();
    }

    void
    do_command_completion (const std::string &a_line)
    {
        const std::vector<CmdInterpreter::Command*> &commands =
            cmd_interpreter.commands ();

        std::vector<CmdInterpreter::Command*> matches;
        for (std::vector<CmdInterpreter::Command*>::const_iterator iter =
                commands.begin ();
             iter != commands.end ();
             ++iter) {
            if (*iter && !(*iter)->name ().find (a_line)) {
                matches.push_back (*iter);
            }
        }

        if (!matches.size ()) {
            return;
        }

        std::string completion = matches[0]->name ().substr (a_line.size ());
        if (matches.size () > 1) {
            std::string msg;
            for (size_t i = 0; i < matches.size (); i++) {
                size_t j = a_line.size ();
                for (;
                     j < matches[i]->name ().size ()
                        && j < completion.size ()
                        && matches[i]->name ()[j] == completion[j];
                     j++) {
                }
                completion = completion.substr (0, j);
                msg += matches[i]->name () + "\t";
            }

            display_message (msg);
        }
        do_completion (completion);
    }

    void
    do_param_completion (std::vector<UString> &a_tokens)
    {
        if (!a_tokens.size ()) {
            return;
        }

        const std::vector<CmdInterpreter::Command*> &commands =
            cmd_interpreter.commands ();

        CmdInterpreter::Command* command = 0;
        for (std::vector<CmdInterpreter::Command*>::const_iterator iter =
                commands.begin ();
             iter != commands.end ();
             ++iter) {
            if (*iter && (*iter)->name () == a_tokens[0]) {
                command = *iter;
                break;
            }
        }

        if (!command) {
            return;
        }

        a_tokens.erase (a_tokens.begin ());
        std::string line (rl_line_buffer, rl_point);
        if (std::isspace (line[line.size () - 1])) {
            stream << "\n";
            command->display_usage (a_tokens, stream);
            rl_forced_update_display ();
        } else {
            UString token = a_tokens.back ();
            a_tokens.pop_back ();

            std::vector<UString> completions;
            std::vector<UString> matches;
            command->completions (a_tokens, completions);
            for (std::vector<UString>::iterator iter = completions.begin ();
                 iter != completions.end ();
                 ++iter) {
                if (!iter->find (token)) {
                    matches.push_back (*iter);
                }
            }

            if (matches.size () == 1) {
                std::string completion = matches[0].substr (token.size ());
                do_completion (completion);
            } else if (matches.size () > 1) {
                std::string msg;
                UString completion = matches[0].substr (token.size ());
                for (size_t i = 0; i < matches.size (); i++) {
                    size_t j = token.size ();
                    for (; j < matches[i].size ()
                                && j < completion.size ()
                                && matches[i][j] == completion[j];
                         j++) {
                    }
                    completion = completion.substr (0, j);
                    msg += matches[i] + "\t";
                }

                display_message (msg);
                do_completion (completion);
            } else {
                rl_complete (0, '\t');
            }
        }
    }

    static int
    on_tab_key_pressed (int, int)
    {
        NEMIVER_TRY

        std::string line (rl_line_buffer, rl_point);
        std::vector<UString> tokens_raw = str_utils::split (line, " ");
        std::vector<UString> tokens;
        for (size_t i = 0; i < tokens_raw.size (); i++) {
            if (tokens_raw[i].size ()) {
                tokens.push_back (tokens_raw[i]);
            }
        }

        if (std::isspace (line[line.size () - 1]) || tokens.size () > 1) {
            self ().do_param_completion (tokens);
        } else {
            self ().do_command_completion (line);
        }

        NEMIVER_CATCH_NOX

        return 0;
    }

    static std::map<int, Console::Priv*>& consoles ()
    {
        static std::map<int, Console::Priv*> s_consoles;
        return s_consoles;
    }

    static Console::Priv& self ()
    {
        int fd = fileno (rl_instream);
        THROW_IF_FAIL (fd);
        THROW_IF_FAIL (consoles ().count (fd));
        Console::Priv *self = consoles ()[fd];
        THROW_IF_FAIL (self);
        return *self;
    }

    static void
    process_command (char *a_command)
    {
        NEMIVER_TRY

        THROW_IF_FAIL (a_command);
        add_history (a_command);
        self ().cmd_interpreter.execute_command (a_command);

        NEMIVER_CATCH_NOX

        free (a_command);
    }
};

Console::Console (IDebugger &a_debugger,
                  const std::string &a_menu_file_path,
                  const Glib::RefPtr<Gtk::UIManager> &a_ui_manager) :
    m_priv (new Priv (a_debugger, a_menu_file_path, a_ui_manager))
{
}

Console::~Console ()
{
}

void
Console::execute_command_file (const UString &a_command_file)
{
    std::ifstream file (a_command_file.c_str ());
    while (file.good ()) {
        std::string command;
        std::getline (file, command);
        execute_command (command);
    }
    file.close ();
}

void
Console::execute_command (const UString &a_command)
{
    THROW_IF_FAIL (m_priv);

    rl_restore_state (&m_priv->console_state);

    add_history (a_command.c_str ());
    m_priv->stream << a_command << "\n";

    rl_save_state (&m_priv->console_state);
    rl_restore_state (&m_priv->saved_state);

    m_priv->cmd_interpreter.execute_command (a_command);
}

CmdInterpreter&
Console::command_interpreter() const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->cmd_interpreter;
}

Terminal&
Console::terminal () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->terminal;
}

NEMIVER_END_NAMESPACE(nemiver)

