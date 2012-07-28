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

#include "common/nmv-namespace.h"
#include "common/nmv-safe-ptr.h"
#include "common/nmv-exception.h"
#include "common/nmv-ustring.h"
#include "common/nmv-proc-utils.h"

#include <glibmm.h>
#include <giomm.h>
#include <glib/gi18n.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <iostream>

using nemiver::common::SafePtr;
using nemiver::common::FreeUnref;
using nemiver::common::DefaultRef;
using nemiver::common::UString;

NEMIVER_BEGIN_NAMESPACE (nemiver)

static const char *const NMV_BUS_NAME = "org.gnome.nemiver.profiler";
static const char *const NMV_DBUS_PROFILER_SERVER_INTROSPECTION_DATA =
    "<node>"
    "  <interface name='org.gnome.nemiver.profiler'>"
    "    <method name='AttachToPID'>"
    "        <arg type='i' name='pid' direction='in' />"
    "        <arg type='i' name='uid' direction='in' />"
    "        <arg type='i' name='gid' direction='in' />"
//    "        <arg type='u' name='cookie' direction='in' />"
    "        <arg type='as' name='arguments' direction='in' />"
    "        <arg type='s' name='data_filepath' direction='out' />"
    "    </method>"
    "    <method name='DetachFromProcess'>"
    "    </method>"
    "  </interface>"
    "</node>";

class PerfServer {

    PerfServer (const PerfServer &);
    PerfServer& operator= (const PerfServer &);

    struct Priv;
    SafePtr<Priv> m_priv;

public:

    PerfServer ();
    ~PerfServer ();
}; // end namespace PerfServer

struct PerfServer::Priv {
//    PolkitSubject *subject;
    unsigned int bus_id;
    unsigned int registration_id;
    Glib::RefPtr<Gio::DBus::NodeInfo> introspection_data;
    Gio::DBus::InterfaceVTable interface_vtable;

    int perf_pid;
    int master_pty_fd;
    int perf_stdout_fd;
    int perf_stderr_fd;

    Priv () :
//        subject (polkit_system_bus_name_new ("org.gnome.Nemiver")),
        bus_id (0),
        registration_id (0),
        introspection_data (0),
        interface_vtable
            (sigc::mem_fun (*this, &PerfServer::Priv::on_new_request)),
        perf_pid (0),
        master_pty_fd (0),
        perf_stdout_fd (0),
        perf_stderr_fd (0)
    {
        init ();
    }

    bool
    on_wait_for_record_to_exit (int a_uid, int a_gid,
                                Glib::RefPtr<Gio::DBus::MethodInvocation>
                                    a_invocation,
                                Glib::ustring a_report_filepath)
    {
        int status = 0;
        pid_t pid = waitpid (perf_pid, &status, WNOHANG);
        bool is_terminated = WIFEXITED (status) || WIFSIGNALED (status);

        if (pid == perf_pid && is_terminated) {
            g_spawn_close_pid (perf_pid);
            perf_pid = 0;
            master_pty_fd = 0;
            perf_stdout_fd = 0;
            perf_stderr_fd = 0;

            NEMIVER_TRY;

            Glib::Variant<Glib::ustring> perf_data =
                Glib::Variant<Glib::ustring>::create (a_report_filepath);

            Glib::VariantContainerBase response =
                Glib::VariantContainerBase::create_tuple (perf_data);

            chown (a_report_filepath.c_str (), a_uid, a_gid);

            std::cout << "Saving report to " << a_report_filepath << std::endl;

            a_invocation->return_value (response);

            NEMIVER_CATCH_NOX;

            return false;
        }

        return true;
    }

    void
    on_new_request (const Glib::RefPtr<Gio::DBus::Connection> &a_connection,
                    const Glib::ustring&,
                    const Glib::ustring&,
                    const Glib::ustring&,
                    const Glib::ustring &a_request_name,
                    const Glib::VariantContainerBase &a_parameters,
                    const Glib::RefPtr<Gio::DBus::MethodInvocation>
                        &a_invocation)
    {
        NEMIVER_TRY;

        THROW_IF_FAIL (a_connection);
        THROW_IF_FAIL (a_invocation);

        if(a_request_name == "AttachToPID") {
            Glib::Variant<int> pid_param;
            Glib::Variant<int> uid_param;
            Glib::Variant<int> gid_param;
            Glib::Variant<std::vector<Glib::ustring> > options_param;
            a_parameters.get_child (pid_param);
            a_parameters.get_child (uid_param, 1);
            a_parameters.get_child (gid_param, 2);
            a_parameters.get_child (options_param, 3);

            int pid = pid_param.get ();
            int uid = uid_param.get ();
            int gid = gid_param.get ();
            std::vector<Glib::ustring> options (options_param.get ());
            for (std::vector<Glib::ustring>::iterator iter = options.begin ();
                 iter != options.end ();
                 ++iter) {
                if (*iter != "--output") {
                    continue;
                }

                Gio::DBus::Error error
                    (Gio::DBus::Error::INVALID_ARGS,
                     _("--output parameter is "
                       "forbidden for security reasons"));
                a_invocation->return_error (error);
            }

            SafePtr<char, DefaultRef, FreeUnref> filepath (tempnam(0, 0));
            THROW_IF_FAIL (filepath);

            std::vector<UString> argv;
            argv.push_back ("perf");
            argv.push_back ("record");
            argv.push_back ("--pid");
            argv.push_back (UString::compose ("%1", pid));
            argv.push_back ("--output");
            argv.push_back (filepath.get ());
            argv.insert (argv.end (), options.begin (), options.end ());

            std::cout << "Launching perf with pid: " << pid << std::endl;

            bool is_launched =
                common::launch_program (argv,
                                        perf_pid,
                                        master_pty_fd,
                                        perf_stdout_fd,
                                        perf_stderr_fd);

            THROW_IF_FAIL (is_launched);

            std::cout << "Perf started" << std::endl;

            Glib::RefPtr<Glib::MainContext> context =
                Glib::MainContext::get_default ();
            context->signal_idle ().connect
                (sigc::bind<int, int,
                            Glib::RefPtr<Gio::DBus::MethodInvocation>,
                            Glib::ustring>
                        (sigc::mem_fun (*this,
                         &PerfServer::Priv::on_wait_for_record_to_exit),
                 uid, gid,
                 a_invocation,
                 filepath.get ()));
        }
        else if (a_request_name == "DetachFromProcess") {
            kill (perf_pid, SIGINT);

            Glib::VariantContainerBase response;
            a_invocation->return_value (response);
        }
        else
        {
            Gio::DBus::Error error
                (Gio::DBus::Error::UNKNOWN_METHOD, _("Invalid request"));
            a_invocation->return_error (error);
        }

        NEMIVER_CATCH_NOX
    }

    void
    init ()
    {
        introspection_data = Gio::DBus::NodeInfo::create_for_xml
            (NMV_DBUS_PROFILER_SERVER_INTROSPECTION_DATA);

        bus_id = Gio::DBus::own_name
            (Gio::DBus::BUS_TYPE_SYSTEM,
             NMV_BUS_NAME,
             sigc::mem_fun (*this, &PerfServer::Priv::on_bus_acquired),
             sigc::mem_fun (*this, &PerfServer::Priv::on_name_acquired),
             sigc::mem_fun (*this, &PerfServer::Priv::on_name_lost));
    }

    void
    on_bus_acquired (const Glib::RefPtr<Gio::DBus::Connection> &a_connection,
                    const Glib::ustring&)
    {
        NEMIVER_TRY;

        THROW_IF_FAIL (a_connection);
        registration_id = a_connection->register_object
            ("/org/gnome/nemiver/profiler",
             introspection_data->lookup_interface (),
             interface_vtable);

        NEMIVER_CATCH_NOX;
    }

    void
    on_name_acquired (const Glib::RefPtr<Gio::DBus::Connection>&,
                      const Glib::ustring&)
    {
    }

    void
    on_name_lost (const Glib::RefPtr<Gio::DBus::Connection> &a_connection,
                  const Glib::ustring&)
    {
        NEMIVER_TRY;

        THROW_IF_FAIL (a_connection);
        a_connection->unregister_object (registration_id);

        NEMIVER_CATCH_NOX;
    }
};

PerfServer::PerfServer () :
    m_priv (new Priv)
{
}

PerfServer::~PerfServer ()
{
}

NEMIVER_END_NAMESPACE (nemiver)

int main(int, char**)
{
    NEMIVER_TRY;

    Glib::init ();
    Gio::init ();

    nemiver::PerfServer server;

    Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create ();
    THROW_IF_FAIL (loop);
    loop->run ();

    NEMIVER_CATCH_NOX;

    return 0;
}

