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
#include "nmv-prof-preferences-dialog.h"
#include "nmv-ui-utils.h"
#include "nmv-i-conf-mgr.h"
#include "nmv-i-perspective.h"
#include "nmv-conf-keys.h"
#include "common/nmv-env.h"
#include <gtkmm/checkbutton.h>

NEMIVER_BEGIN_NAMESPACE(nemiver)

class ProfPreferencesDialog::Priv {
    Priv ();

    IPerspective &perspective;
    Glib::RefPtr<Gtk::Builder> gtkbuilder;
    Gtk::CheckButton *callgraph_checkbutton;
    Gtk::CheckButton *no_buffering_checkbutton;
    Gtk::CheckButton *raw_sample_checkbutton;
    Gtk::CheckButton *system_wide_collection_checkbutton;
    Gtk::CheckButton *sample_addresses_checkbutton;
    Gtk::CheckButton *sample_timestamps_checkbutton;

public:

    Priv (const Glib::RefPtr<Gtk::Builder> &a_gtkbuilder,
          IPerspective &a_perspective) :
        perspective (a_perspective),
        gtkbuilder (a_gtkbuilder),
        callgraph_checkbutton (0),
        no_buffering_checkbutton (0),
        raw_sample_checkbutton (0),
        system_wide_collection_checkbutton (0),
        sample_addresses_checkbutton (0),
        sample_timestamps_checkbutton (0)
    {
        init ();
    }

    void
    on_callgraph_toggled_signal ()
    {
        update_callgraph_key ();
    }

    void
    on_no_buffering_toggled_signal ()
    {
        update_no_buffering_key ();
    }

    void
    on_raw_sample_toggled_signal ()
    {
        update_raw_sample_key ();
    }

    void
    on_system_wide_collection_toggled_signal ()
    {
        update_system_wide_collection_key ();
    }

    void
    on_sample_addresses_toggled_signal ()
    {
        update_sample_addresses_key ();
    }

    void
    on_sample_timestamps_toggled_signal ()
    {
        update_sample_timestamps_key ();
    }

    void
    init ()
    {
        init_recording_tab ();
    }

    void
    init_recording_tab ()
    {
        callgraph_checkbutton =
            ui_utils::get_widget_from_gtkbuilder<Gtk::CheckButton>
                (gtkbuilder, "callgraphcheckbutton");
        THROW_IF_FAIL (callgraph_checkbutton);
        callgraph_checkbutton->signal_toggled ().connect (sigc::mem_fun
            (*this, &ProfPreferencesDialog::Priv::on_callgraph_toggled_signal));

        no_buffering_checkbutton =
            ui_utils::get_widget_from_gtkbuilder<Gtk::CheckButton>
                (gtkbuilder, "nobufferingcheckbutton");
        THROW_IF_FAIL (no_buffering_checkbutton);
        no_buffering_checkbutton->signal_toggled ().connect (sigc::mem_fun
            (*this,
             &ProfPreferencesDialog::Priv::on_no_buffering_toggled_signal));

        raw_sample_checkbutton =
            ui_utils::get_widget_from_gtkbuilder<Gtk::CheckButton>
                (gtkbuilder, "rawsamplecheckbutton");
        THROW_IF_FAIL (raw_sample_checkbutton);
        raw_sample_checkbutton->signal_toggled ().connect (sigc::mem_fun
            (*this,
             &ProfPreferencesDialog::Priv::on_raw_sample_toggled_signal));

        system_wide_collection_checkbutton =
            ui_utils::get_widget_from_gtkbuilder<Gtk::CheckButton>
                (gtkbuilder, "systemcollctioncheckbutton");
        THROW_IF_FAIL (system_wide_collection_checkbutton);
        system_wide_collection_checkbutton->signal_toggled ().connect
            (sigc::mem_fun (*this, &ProfPreferencesDialog::Priv
                                ::on_system_wide_collection_toggled_signal));

        sample_timestamps_checkbutton =
            ui_utils::get_widget_from_gtkbuilder<Gtk::CheckButton>
                (gtkbuilder, "timestampsamplecheckbutton");
        THROW_IF_FAIL (sample_timestamps_checkbutton);
        sample_timestamps_checkbutton->signal_toggled ().connect (sigc::mem_fun
            (*this, &ProfPreferencesDialog::Priv
                        ::on_sample_timestamps_toggled_signal));

        sample_addresses_checkbutton =
            ui_utils::get_widget_from_gtkbuilder<Gtk::CheckButton>
                (gtkbuilder, "addresssamplecheckbutton");
        THROW_IF_FAIL (sample_addresses_checkbutton);
        sample_addresses_checkbutton->signal_toggled ().connect (sigc::mem_fun
            (*this,
             &ProfPreferencesDialog::Priv::on_sample_addresses_toggled_signal));
    }

    IConfMgr&
    conf_manager () const
    {
        IConfMgrSafePtr conf_mgr =
            perspective.get_workbench ().get_configuration_manager ();
        THROW_IF_FAIL (conf_mgr);
        return *conf_mgr;
    }

    void
    update_callgraph_key ()
    {
        THROW_IF_FAIL (callgraph_checkbutton);
        bool is_on = callgraph_checkbutton->get_active ();
        conf_manager ().set_key_value (CONF_KEY_DO_CALLGRAPH_RECORDING, is_on);
    }

    void
    update_no_buffering_key ()
    {
        THROW_IF_FAIL (no_buffering_checkbutton);
        bool is_on = no_buffering_checkbutton->get_active ();
        conf_manager ().set_key_value
            (CONF_KEY_COLLECT_WITHOUT_BUFFERING, is_on);
    }

    void
    update_raw_sample_key ()
    {
        THROW_IF_FAIL (raw_sample_checkbutton);
        bool is_on = raw_sample_checkbutton->get_active ();
        conf_manager ().set_key_value
            (CONF_KEY_COLLECT_RAW_SAMPLE_RECORDS, is_on);
    }

    void
    update_system_wide_collection_key ()
    {
        THROW_IF_FAIL (system_wide_collection_checkbutton);
        bool is_on = system_wide_collection_checkbutton->get_active ();
        conf_manager ().set_key_value (CONF_KEY_SYSTEM_WIDE_COLLECTION, is_on);
    }

    void
    update_sample_addresses_key ()
    {
        THROW_IF_FAIL (sample_addresses_checkbutton);
        bool is_on = sample_addresses_checkbutton->get_active ();
        conf_manager ().set_key_value (CONF_KEY_SAMPLE_ADDRESSES, is_on);
    }

    void
    update_sample_timestamps_key ()
    {
        THROW_IF_FAIL (sample_timestamps_checkbutton);
        bool is_on = sample_timestamps_checkbutton->get_active ();
        conf_manager ().set_key_value (CONF_KEY_SAMPLE_TIMESTAMPS, is_on);
    }

    void
    update_widget_from_recorder_keys ()
    {
        THROW_IF_FAIL (callgraph_checkbutton);
        THROW_IF_FAIL (no_buffering_checkbutton);
        THROW_IF_FAIL (raw_sample_checkbutton);
        THROW_IF_FAIL (system_wide_collection_checkbutton);
        THROW_IF_FAIL (sample_addresses_checkbutton);
        THROW_IF_FAIL (sample_timestamps_checkbutton);

        bool do_callgraph_recording = true;
        if (!conf_manager ().get_key_value (CONF_KEY_DO_CALLGRAPH_RECORDING,
                                            do_callgraph_recording)) {
            LOG_ERROR ("failed to get gconf key "
                       << CONF_KEY_DO_CALLGRAPH_RECORDING);
        }
        callgraph_checkbutton->set_active (do_callgraph_recording);

        bool do_collect_without_buffering = false;
        if (!conf_manager ().get_key_value (CONF_KEY_COLLECT_WITHOUT_BUFFERING,
                                            do_collect_without_buffering)) {
            LOG_ERROR ("failed to get gconf key "
                       << CONF_KEY_COLLECT_WITHOUT_BUFFERING);
        }
        no_buffering_checkbutton->set_active (do_collect_without_buffering);

        bool do_collect_raw_sample_records = false;
        if (!conf_manager ().get_key_value (CONF_KEY_COLLECT_RAW_SAMPLE_RECORDS,
                                            do_collect_raw_sample_records)) {
            LOG_ERROR ("failed to get gconf key "
                       << CONF_KEY_COLLECT_RAW_SAMPLE_RECORDS);
        }
        raw_sample_checkbutton->set_active (do_collect_raw_sample_records);

        bool do_system_wide_collection = false;
        if (!conf_manager ().get_key_value (CONF_KEY_SYSTEM_WIDE_COLLECTION,
                                            do_system_wide_collection)) {
            LOG_ERROR ("failed to get gconf key "
                       << CONF_KEY_SYSTEM_WIDE_COLLECTION);
        }
        system_wide_collection_checkbutton->set_active
            (do_system_wide_collection);

        bool do_sample_addresses = false;
        if (!conf_manager ().get_key_value (CONF_KEY_SAMPLE_ADDRESSES,
                                            do_sample_addresses)) {
            LOG_ERROR ("failed to get gconf key "
                       << CONF_KEY_SAMPLE_ADDRESSES);
        }
        sample_addresses_checkbutton->set_active (do_sample_addresses);

        bool do_sample_timestamps = false;
        if (!conf_manager ().get_key_value (CONF_KEY_SAMPLE_TIMESTAMPS,
                                            do_sample_timestamps)) {
            LOG_ERROR ("failed to get gconf key "
                       << CONF_KEY_SAMPLE_TIMESTAMPS);
        }
        sample_timestamps_checkbutton->set_active (do_sample_timestamps);
    }

    void
    update_widget_from_conf ()
    {
        update_widget_from_recorder_keys ();
    }
};//end ProfPreferencesDialog

ProfPreferencesDialog::ProfPreferencesDialog (IPerspective &a_perspective,
                                              const UString &a_root_path) :
    Dialog (a_root_path,
            "preferencesdialog.ui",
            "preferencesdialog")
{
    m_priv.reset (new Priv (gtkbuilder (), a_perspective));
    m_priv->update_widget_from_conf ();
}

ProfPreferencesDialog::~ProfPreferencesDialog ()
{
    LOG_D ("delete", "destructor-domain");
    THROW_IF_FAIL (m_priv);
}

NEMIVER_END_NAMESPACE (nemiver)
