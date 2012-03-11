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

class CmdInterpreter {
    //non copyable
    CmdInterpreter (const CmdInterpreter&);
    CmdInterpreter& operator= (const CmdInterpreter&);

    struct Priv;
    SafePtr<Priv> m_priv;

public:
    class Command : public Object {
        sigc::signal<void> m_done_signal;

    public:
        sigc::signal<void>& done_signal ()
        {
            return m_done_signal;
        }

        virtual const std::string& name () const = 0;

        virtual const std::vector<UString>& aliases () const
        {
            static std::vector<UString> s_aliases;
            return s_aliases;
        }

        virtual void completions (const std::vector<UString>&,
                                  std::vector<UString>&) const
        {
        }

        virtual void display_usage (const std::vector<UString>&,
                                    std::ostream&) const
        {
        }

        virtual void execute (const std::vector<UString> &a_argv,
                              std::ostream &a_output) = 0;

        virtual void operator() (const std::vector<UString> &a_argv,
                                 std::ostream &a_output)
        {
            execute (a_argv, a_output);
        }

        virtual ~Command ()
        {
        }
    };

    typedef SafePtr<Command, ObjectRef, ObjectUnref> CommandSafePtr;
    typedef Command AsynchronousCommand;
    struct SynchronousCommand : public Command{
        virtual void operator() (const std::vector<UString> &a_argv,
                                 std::ostream &a_output)
        {
            execute (a_argv, a_output);
            done_signal ().emit ();
        }
    };

    CmdInterpreter (IDebugger &a_debugger, std::ostream &a_output_stream);
    ~CmdInterpreter ();
    void register_command (Command &a_command);
    void execute_command (const UString &a_command);
    const std::vector<Command*>& commands() const;

    void current_file_path (const UString &a_file_path);
    const UString& current_file_path () const;

    bool ready () const;

    sigc::signal<void, UString>& file_opened_signal () const;
    sigc::signal<void>& ready_signal () const;
};

NEMIVER_END_NAMESPACE(nemiver)

#endif /* __NMV_CMD_INTERPRETER_H__ */

