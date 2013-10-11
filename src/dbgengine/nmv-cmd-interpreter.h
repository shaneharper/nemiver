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


#ifndef __NMV_CMD_INTERPRETER_H__
#define __NMV_CMD_INTERPRETER_H__

#include "common/nmv-safe-ptr.h"
#include "common/nmv-namespace.h"
#include "common/nmv-ustring.h"
#include "common/nmv-safe-ptr-utils.h"
#include <sigc++/signal.h>
#include <ostream>
#include <vector>

NEMIVER_BEGIN_NAMESPACE(nemiver)

using common::UString;
using common::SafePtr;
using common::Object;
using common::ObjectRef;
using common::ObjectUnref;

class IDebugger;

/// Command Interpreter used as interface to nemiver and specifically
/// to IDebugger.
///
/// The command interpreter is able to execute command which has the form of
/// the following string: "command-name option1 option2".
/// One can add additional commands through \a register_command.
///
/// The Command Interpreter primary goal is to create a console to IDebugger,
/// and thus provides features useful for terminals like completion, help,
/// and aliases.
class CmdInterpreter {
    //non copyable
    CmdInterpreter (const CmdInterpreter&);
    CmdInterpreter& operator= (const CmdInterpreter&);

    struct Priv;
    SafePtr<Priv> m_priv;

public:
    /// Base class to write commands which can be registered into the \a
    /// CmdInterpreter.
    class Command : public Object {
        sigc::signal<void> m_done_signal;

    protected:
        virtual bool execute (const std::vector<UString> &a_argv,
                              std::ostream &a_output) = 0;

    public:
        /// This signal must be emited at the end of the command execution.
        sigc::signal<void>& done_signal ()
        {
            return m_done_signal;
        }

        /// Get the name of the command.
        virtual const std::string& name () const = 0;

        /// Get a vector of all the aliases available for the command.
        virtual const std::vector<UString>& aliases () const
        {
            static std::vector<UString> s_aliases;
            return s_aliases;
        }

        /// Provide the possible completions for the options of a command.
        ///
        /// \param a_argv Vector of the options used for the command.
        /// A command must provide the correct completion following the options
        /// the user has already written on the command line.
        ///
        /// \param a_completion_vector Vector of all the possible completion.
        virtual void completions (const std::vector<UString> &/*a_argv*/,
                                  std::vector<UString> &/*a_completion_vector*/)
                                  const
        {
        }

        /// Display the help message for the command.
        ///
        /// \param a_argv Vector of the options used for the command.
        /// A command must provide the correct usage following the options
        /// the user has already written on the command line.
        ///
        /// \param a_stream Stream to used to display the usage of the command.
        virtual void display_usage (const std::vector<UString> &/*a_argv*/,
                                    std::ostream &/*a_stream*/) const
        {
        }

        /// Execute the command and emit the \a done_signal () if the command
        /// has finish executing.
        /// \param a_argv Options given to the command.
        /// \param a_output Stream to used to display the command output.
        void operator () (const std::vector<UString> &a_argv,
                          std::ostream &a_output)
        {
            if (execute (a_argv, a_output)) {
                done_signal ().emit ();
            }
        }

        virtual ~Command ()
        {
        }
    };

    typedef SafePtr<Command, ObjectRef, ObjectUnref> CommandSafePtr;

    CmdInterpreter (IDebugger &a_debugger, std::ostream &a_output_stream);
    ~CmdInterpreter ();

    /// Register a new command into the command interpreter
    /// \param a_command Command to register
    void register_command (Command &a_command);

    /// Execute a command
    /// \param a_command Command to execute
    void execute_command (const UString &a_command);

    /// Get a vector of all the commands registered into
    /// the command interpreter.
    const std::vector<Command*>& commands () const;

    /// Set the path of the source code on which the command interpreter is
    /// acting.
    /// \param a_file_path
    void current_file_path (const UString &a_file_path);

    /// Get the path of the source code on which the command interpreter is
    /// acting.
    /// It is usually the path of the source code of the current frame except
    /// if the user change the path by calling \a current_file_path ().
    const UString& current_file_path () const;

    /// Get whether the command interpreter is ready to execute new commands.
    bool ready () const;

    /// \name signals
    /// @{

    /// This signal is emited when the command interpreter becomes ready to
    /// execute new commands.
    sigc::signal<void>& ready_signal () const;

    /// @}
};

NEMIVER_END_NAMESPACE(nemiver)

#endif /* __NMV_CMD_INTERPRETER_H__ */

