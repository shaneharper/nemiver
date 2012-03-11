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
#ifndef __NMV_CONSOLE_H__
#define __NMV_CONSOLE_H__

#include "common/nmv-safe-ptr.h"
#include "common/nmv-namespace.h"
#include "common/nmv-exception.h"
#include <gtkmm/uimanager.h>
#include <string>
#include <vector>
#include <ostream>

NEMIVER_BEGIN_NAMESPACE(nemiver)

class IDebugger;
class CmdInterpreter;
class Terminal;

using nemiver::common::SafePtr;
using nemiver::common::UString;

class Console {
    //non copyable
    Console (const Console&);
    Console& operator= (const Console&);

    struct Priv;
    SafePtr<Priv> m_priv;

    struct StreamBuf : public std::streambuf {
        int fd;

        StreamBuf () :
            fd (0)
        {
        }

        StreamBuf (int a_fd) :
            fd (a_fd)
        {
        }

        virtual std::streamsize
        xsputn (const char *a_string, std::streamsize a_size)
        {
            THROW_IF_FAIL (fd);
            return ::write (fd, a_string, a_size);
        }
    };

public:
    class Stream : public std::ostream {
        StreamBuf streambuf;
    public:
        explicit Stream (int a_fd) :
            std::ostream (&streambuf),
            streambuf (a_fd)
        {
        }
    };

    Console (IDebugger &a_debugger,
             const std::string &a_menu_file_path,
             const Glib::RefPtr<Gtk::UIManager> &a_ui_manager);
    ~Console ();
    void execute_command_file (const UString &a_command_file);
    void execute_command (const UString &a_command);
    CmdInterpreter& command_interpreter() const;
    Terminal& terminal () const;
};

NEMIVER_END_NAMESPACE(nemiver)

#endif /* __NMV_CONSOLE_H__ */

