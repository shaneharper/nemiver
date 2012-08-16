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
#include "nmv-record-options.h"
#include "nmv-perf-engine.h"

#include <polkit/polkit.h>
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
using nemiver::common::Exception;

NEMIVER_BEGIN_NAMESPACE (nemiver)

static const char *const NMV_BUS_NAME = "org.gnome.nemiver.profiler";
static const char *const NMV_DBUS_PROFILER_SERVER_INTROSPECTION_DATA =
    "<node>"
    "  <interface name='org.gnome.nemiver.profiler'>"
    "    <method name='ProfileSystem'>"
    "        <arg type='b' name='callgraph_recording' direction='in' />"
    "        <arg type='b' name='collect_without_buffering' direction='in' />"
    "        <arg type='b' name='collect_raw_sample_records' direction='in' />"
    "        <arg type='b' name='sample_addresses' direction='in' />"
    "        <arg type='b' name='sample_timestamps' direction='in' />"
    "        <arg type='i' name='uid' direction='in' />"
    "        <arg type='i' name='gid' direction='in' />"
    "        <arg type='u' name='request_id' direction='out' />"
    "    </method>"
    "    <method name='AttachToPID'>"
    "        <arg type='b' name='callgraph_recording' direction='in' />"
    "        <arg type='b' name='collect_without_buffering' direction='in' />"
    "        <arg type='b' name='collect_raw_sample_records' direction='in' />"
    "        <arg type='b' name='sample_addresses' direction='in' />"
    "        <arg type='b' name='sample_timestamps' direction='in' />"
    "        <arg type='i' name='pid' direction='in' />"
    "        <arg type='i' name='uid' direction='in' />"
    "        <arg type='i' name='gid' direction='in' />"
    "        <arg type='u' name='request_id' direction='out' />"
    "    </method>"
    "    <method name='DetachFromProcess'>"
    "        <arg type='u' name='request_id' direction='in' />"
    "    </method>"
    "    <method name='RecordDoneSignal'>"
    "        <arg type='u' name='request_id' direction='in' />"
    "        <arg type='s' name='report_filepath' direction='out' />"
    "    </method>"
    "  </interface>"
    "</node>";

class PerfRecordOptions : public RecordOptions {
    bool callgraph_recording;
    bool collect_without_buffering;
    bool collect_raw_sample_records;
    bool sample_addresses;
    bool sample_timestamps;

public:
    PerfRecordOptions (const Glib::VariantContainerBase &a_parameters) :
        callgraph_recording (true),
        collect_without_buffering (false),
        collect_raw_sample_records (false),
        sample_addresses (false),
        sample_timestamps (false)
    {
        init (a_parameters);
    }

    void
    init (const Glib::VariantContainerBase &a_parameters)
    {
        Glib::Variant<bool> callgraph_recording_param;
        Glib::Variant<bool> collect_without_buffering_param;
        Glib::Variant<bool> collect_raw_sample_records_param;
        Glib::Variant<bool> sample_addresses_param;
        Glib::Variant<bool> sample_timestamps_param;

        a_parameters.get_child (callgraph_recording_param, 0);
        a_parameters.get_child (collect_without_buffering_param, 1);
        a_parameters.get_child (collect_raw_sample_records_param, 2);
        a_parameters.get_child (sample_addresses_param, 3);
        a_parameters.get_child (sample_timestamps_param, 4);
    }

    bool
    do_callgraph_recording () const
    {
        return callgraph_recording;
    }

    bool
    do_collect_without_buffering () const
    {
        return collect_without_buffering;
    }

    bool
    do_collect_raw_sample_records () const
    {
        return collect_raw_sample_records;
    }

    bool
    do_sample_addresses () const
    {
        return sample_addresses;
    }

    bool
    do_sample_timestamps () const
    {
        return sample_timestamps;
    }
}; // end namespace PerfRecordOptions

class PerfServer {

    PerfServer (const PerfServer &);
    PerfServer& operator= (const PerfServer &);

    struct Priv;
    SafePtr<Priv> m_priv;

public:

    PerfServer ();
    ~PerfServer ();
}; // end namespace PerfServer

struct RequestInfo {
    int gid;
    int uid;
    SafePtr<PerfEngine, ObjectRef, ObjectUnref> profiler;
    Glib::RefPtr<Gio::DBus::MethodInvocation> invocation;

    RequestInfo () :
        profiler (nemiver::load_iface_and_confmgr<PerfEngine> ("perfengine",
                                                               "IProfiler")),
        invocation (0)
    {
    }
};

struct PerfServer::Priv {
//    PolkitSubject *subject;
    unsigned bus_id;
    unsigned registration_id;
    Glib::RefPtr<Gio::DBus::NodeInfo> introspection_data;
    Gio::DBus::InterfaceVTable interface_vtable;
    std::map<int, RequestInfo> request_map;
    unsigned next_request_id;

    Priv () :
//        subject (polkit_system_bus_name_new ("org.gnome.Nemiver")),
        bus_id (0),
        registration_id (0),
        introspection_data (0),
        interface_vtable
            (sigc::mem_fun (*this, &PerfServer::Priv::on_new_request)),
        next_request_id (0)
    {
        init ();
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
        try {

            THROW_IF_FAIL (a_connection);
            THROW_IF_FAIL (a_invocation);

            Glib::ustring bus_name = a_invocation->get_sender ();
            PolkitAuthorizationResult *result = 0;

            if(a_request_name == "ProfileSystem") {
                Glib::Variant<int> uid_param;
                Glib::Variant<int> gid_param;
                a_parameters.get_child (uid_param, 5);
                a_parameters.get_child (gid_param, 6);

                result = polkit_authority_check_authorization_sync
                    (polkit_authority_get_sync (0, 0),
                     polkit_system_bus_name_new (bus_name.c_str ()),
                     "org.gnome.nemiver.profiler.profile-system",
                     0,
                     POLKIT_CHECK_AUTHORIZATION_FLAGS_ALLOW_USER_INTERACTION,
                     0,
                     0);
                THROW_IF_FAIL (result);
                THROW_IF_FAIL
                    (polkit_authorization_result_get_is_authorized (result));

                RequestInfo request;
                request.uid = uid_param.get ();
                request.gid = gid_param.get ();

                THROW_IF_FAIL (!request_map.count (next_request_id));
                request_map[next_request_id] = request;

                THROW_IF_FAIL (request.profiler);

                std::vector<UString> argv;
                argv.push_back ("--all-cpus");

                PerfRecordOptions options (a_parameters);
                request.profiler->record (argv, options);

                Glib::Variant<unsigned> perf_data =
                    Glib::Variant<unsigned>::create (next_request_id++);

                Glib::VariantContainerBase response;
                response = Glib::VariantContainerBase::create_tuple (perf_data);
                a_invocation->return_value (response);
            } else if(a_request_name == "AttachToPID") {
                Glib::Variant<int> pid_param;
                Glib::Variant<int> uid_param;
                Glib::Variant<int> gid_param;
                a_parameters.get_child (pid_param, 5);
                a_parameters.get_child (uid_param, 6);
                a_parameters.get_child (gid_param, 7);

                result = polkit_authority_check_authorization_sync
                    (polkit_authority_get_sync (0, 0),
                     polkit_system_bus_name_new (bus_name.c_str ()),
                     "org.gnome.nemiver.profiler.profile-running-process",
                     0,
                     POLKIT_CHECK_AUTHORIZATION_FLAGS_ALLOW_USER_INTERACTION,
                     0,
                     0);
                THROW_IF_FAIL (result);
                THROW_IF_FAIL
                    (polkit_authorization_result_get_is_authorized (result));

                RequestInfo request;

                int pid = pid_param.get ();
                request.uid = uid_param.get ();
                request.gid = gid_param.get ();

                THROW_IF_FAIL (!request_map.count (next_request_id));

                std::vector<UString> argv;
                argv.push_back ("--pid");
                argv.push_back (UString::compose ("%1", pid));

                request_map[next_request_id] = request;
                PerfRecordOptions options (a_parameters);

                THROW_IF_FAIL (request.profiler);
                request.profiler->record (argv, options);

                Glib::Variant<unsigned> perf_data =
                    Glib::Variant<unsigned>::create (next_request_id++);

                Glib::VariantContainerBase response;
                response = Glib::VariantContainerBase::create_tuple (perf_data);
                a_invocation->return_value (response);
            }
            else if (a_request_name == "RecordDoneSignal") {
                Glib::Variant<unsigned> request_param;
                a_parameters.get_child (request_param);

                unsigned request_id = request_param.get ();
                THROW_IF_FAIL (request_map.count (request_id));
                request_map[request_id].invocation = a_invocation;
                request_map[request_id].profiler->record_done_signal ().connect
                    (sigc::bind<unsigned> (sigc::mem_fun
                        (*this, &PerfServer::Priv::on_record_done_signal),
                         request_id));
            }
            else if (a_request_name == "DetachFromProcess") {
                Glib::Variant<unsigned> request_param;
                a_parameters.get_child (request_param);

                unsigned request_id = request_param.get ();
                THROW_IF_FAIL (request_map.count (request_id));

                THROW_IF_FAIL (request_map[request_id].profiler);
                request_map[request_id].profiler->stop_recording ();

                Glib::VariantContainerBase response;
                a_invocation->return_value (response);
            }
            else
            {
                THROW (_("Invalid request"));
            }
        } catch (const Exception &ex) {
            Gio::DBus::Error error (Gio::DBus::Error::FAILED, ex.what ());
            a_invocation->return_error (error);
        } catch (const Glib::Exception &ex) {
            Gio::DBus::Error error
                (Gio::DBus::Error::FAILED, ex.what ());
            a_invocation->return_error (error);
        } catch (...) {
            Gio::DBus::Error error
                (Gio::DBus::Error::FAILED,
                 _("An unknown error occured while processing your request"));
            a_invocation->return_error (error);
        }
    }

    void
    on_record_done_signal (const UString &a_report, unsigned a_request_id)
    {
        NEMIVER_TRY;

        THROW_IF_FAIL (request_map.count (a_request_id));

        Glib::Variant<Glib::ustring> perf_data =
            Glib::Variant<Glib::ustring>::create (a_report);

        Glib::VariantContainerBase response =
            Glib::VariantContainerBase::create_tuple (perf_data);

        chown (a_report.c_str (),
               request_map[a_request_id].uid,
               request_map[a_request_id].gid);

        request_map[a_request_id].invocation->return_value (response);

        NEMIVER_CATCH_NOX;
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

