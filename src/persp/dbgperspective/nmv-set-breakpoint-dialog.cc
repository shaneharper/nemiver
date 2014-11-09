// -*- c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4; -*-'
//Authors: Jonathon Jongsma
//         Hubert Figuiere
//         Dodji Seketeli
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
#include "config.h"
#include <vector>
#include <glib/gi18n.h>
#include <gtkmm/dialog.h>
#include "common/nmv-exception.h"
#include "common/nmv-env.h"
#include "common/nmv-str-utils.h"
#include "nmv-set-breakpoint-dialog.h"
#include "nmv-ui-utils.h"

using namespace std;
using namespace nemiver::common;

NEMIVER_BEGIN_NAMESPACE (nemiver)

class EventComboModelColumns
    : public Gtk::TreeModel::ColumnRecord
{
public:
    EventComboModelColumns()
        {
            add (m_label);
            add (m_command);
        }
    Gtk::TreeModelColumn<Glib::ustring> m_label;
    Gtk::TreeModelColumn<UString> m_command;
};

class SetBreakpointDialog::Priv {
public:
    Gtk::ComboBox *combo_event;
    EventComboModelColumns combo_event_cols;
    Glib::RefPtr<Gtk::ListStore> combo_event_model;
    Gtk::CellRendererText combo_event_cell_renderer;
    Gtk::Entry *entry_filename;
    Gtk::Entry *entry_line;
    Gtk::Entry *entry_function;
    Gtk::Entry *entry_address;
    Gtk::Entry *entry_condition;
    Gtk::RadioButton *radio_source_location;
    Gtk::RadioButton *radio_function_name;
    Gtk::RadioButton *radio_binary_location;
    Gtk::RadioButton *radio_event;
    Gtk::CheckButton *check_countpoint;
    Gtk::Button *okbutton;

public:
    Priv (Gtk::Dialog &a_dialog,
          const Glib::RefPtr<Gtk::Builder> &a_gtkbuilder) :
        combo_event (0),
        entry_filename (0),
        entry_line (0),
        entry_function (0),
        entry_address (0),
        radio_source_location (0),
        radio_function_name (0),
        radio_binary_location (0),
        radio_event (0),
        check_countpoint (0),
        okbutton (0)
    {
        a_dialog.set_default_response (Gtk::RESPONSE_OK);

        okbutton =
            ui_utils::get_widget_from_gtkbuilder<Gtk::Button> (a_gtkbuilder,
                                                               "okbutton");
        THROW_IF_FAIL (okbutton);
        okbutton->set_sensitive (false);

        combo_event =
            ui_utils::get_widget_from_gtkbuilder<Gtk::ComboBox>
            (a_gtkbuilder, "combo_event");
        combo_event_model = Gtk::ListStore::create (combo_event_cols);
        combo_event->set_model (combo_event_model);

        // Clear the cell renderer that might have been
        // associated to the combo box before.
        combo_event->clear ();

        // Then make sure the combo box uses a text cell renderer
        combo_event->pack_start (combo_event_cell_renderer);

        // And display the content of the m_label column of
        // combo_even_cols in the combo box.
        combo_event->add_attribute (combo_event_cell_renderer.property_text (),
                                    combo_event_cols.m_label);

        Gtk::TreeModel::Row row;
        row = *(combo_event_model->append ());
        row[combo_event_cols.m_label] = _("Throw Exception");
        row[combo_event_cols.m_command] = "throw";

        row = *(combo_event_model->append ());
        row[combo_event_cols.m_label] = _("Catch Exception");
        row[combo_event_cols.m_command] = "catch";

        row = *(combo_event_model->append ());
        row[combo_event_cols.m_label] = _("Fork System Call");
        row[combo_event_cols.m_command] = "fork";
        
        row = *(combo_event_model->append ());
        row[combo_event_cols.m_label] = _("Vfork System Call");
        row[combo_event_cols.m_command] = "vfork";

        row = *(combo_event_model->append ());
        row[combo_event_cols.m_label] = _("Exec System Call");
        row[combo_event_cols.m_command] = "exec";

        combo_event->set_active (false);

        entry_filename =
            ui_utils::get_widget_from_gtkbuilder<Gtk::Entry>
                (a_gtkbuilder, "filenameentry");
        entry_filename->signal_changed ().connect (sigc::mem_fun
                (*this, &Priv::on_text_changed_signal));

        entry_line =
            ui_utils::get_widget_from_gtkbuilder<Gtk::Entry>
                (a_gtkbuilder, "lineentry");
        entry_line->signal_changed ().connect (sigc::mem_fun
                (*this, &Priv::on_text_changed_signal));
        entry_line->set_activates_default ();

        entry_function =
            ui_utils::get_widget_from_gtkbuilder<Gtk::Entry>
                (a_gtkbuilder, "functionentry");
        entry_function->signal_changed ().connect (sigc::mem_fun
                (*this, &Priv::on_text_changed_signal));
        entry_function->set_activates_default ();

        entry_address =
            ui_utils::get_widget_from_gtkbuilder<Gtk::Entry>
                (a_gtkbuilder, "addressentry");
        entry_address->signal_changed ().connect (sigc::mem_fun
              (*this, &Priv::on_text_changed_signal));
        entry_address->set_activates_default ();

        entry_condition = ui_utils::get_widget_from_gtkbuilder<Gtk::Entry>
                (a_gtkbuilder, "conditionentry");
        entry_condition->signal_changed ().connect (sigc::mem_fun
                (*this, &Priv::on_text_changed_signal));
        entry_condition->set_activates_default ();

        radio_source_location =
            ui_utils::get_widget_from_gtkbuilder<Gtk::RadioButton>
                (a_gtkbuilder, "sourcelocationradio");
        radio_source_location->signal_clicked ().connect (sigc::mem_fun
                (*this, &Priv::on_radiobutton_changed));

        radio_function_name =
            ui_utils::get_widget_from_gtkbuilder<Gtk::RadioButton>
                (a_gtkbuilder, "functionnameradio");
        radio_function_name->signal_clicked ().connect (sigc::mem_fun
                (*this, &Priv::on_radiobutton_changed));

        radio_binary_location =
            ui_utils::get_widget_from_gtkbuilder<Gtk::RadioButton>
                (a_gtkbuilder, "binarylocationradio");
        radio_binary_location->signal_clicked ().connect (sigc::mem_fun
              (*this, &Priv::on_radiobutton_changed));

        radio_event =
            ui_utils::get_widget_from_gtkbuilder<Gtk::RadioButton>
            (a_gtkbuilder, "eventradio");
        radio_event->signal_clicked ().connect (sigc::mem_fun
                (*this, &Priv::on_radiobutton_changed));

        check_countpoint =
            ui_utils::get_widget_from_gtkbuilder<Gtk::CheckButton>
            (a_gtkbuilder, "countpointcheck");

        // set the 'function name' mode active by default
        mode (MODE_FUNCTION_NAME);
        // hack to ensure that the correct text entry fields
        // get insensitive at startup since if the gtkbuilder file
        // initializes MODE_FUNCTION_NAME to
        // active, the 'changed' signal won't be emitted here
        // (is there a better way to do this?)
        on_radiobutton_changed ();
    }

    /// Return the file path and the line numbers as set by the
    /// user. This function supports locations filename:linenumber in
    /// the entry_filename text entry field.
    /// \param a_path output parameter. The file path entered by the
    /// user
    /// \param a_line_num output parameter. The line number entered by
    /// the user. The function makes sure the line number is a valid
    /// number.
    /// \return true iff the file path *and* the line number have been
    /// set.
    bool get_file_path_and_line_num (std::string &a_path,
                                     std::string &a_line_num)
    {
        if (entry_line->get_text ().empty ()) {
            // Try to see if entry_filename contains a location string
            // of the form "filename:number".
            return str_utils::extract_path_and_line_num_from_location
                (entry_filename->get_text ().raw (), a_path, a_line_num);
        } else {
            // Try to just get the file path from the entry_filename
            // and line from entry_line.
            if (!entry_filename->get_text ().empty ()
                && atoi (entry_line->get_text ().c_str ())) {
                a_path = entry_filename->get_text ();
                a_line_num = entry_line->get_text ().raw ();
                return true;
            }
        }
        return false;
    }

    void update_ok_button_sensitivity ()
    {
        THROW_IF_FAIL (entry_filename);
        THROW_IF_FAIL (entry_line);

        SetBreakpointDialog::Mode a_mode = mode ();

        switch (a_mode) {
        case MODE_SOURCE_LOCATION: {
            // make sure there's something in the line number entry,
            // at least, and that something is a valid number.
            // Or, make sure the user typed a location of the form
            // filename:linenumber in the file name entry.
            string filename, line;
            if (get_file_path_and_line_num (filename, line)
                || atoi (entry_line->get_text ().c_str ()))
                okbutton->set_sensitive (true);
            else
                okbutton->set_sensitive (false);
            break;
        }
        case MODE_FUNCTION_NAME:
            if (!entry_function->get_text ().empty ()) {
                okbutton->set_sensitive (true);
            } else {
                okbutton->set_sensitive (false);
            }
            break;
        case MODE_BINARY_ADDRESS: {
            bool address_is_valid = false;
            UString address = entry_address->get_text ();
            // Validate the address
            if (str_utils::string_is_number (address))
                address_is_valid = true;
            okbutton->set_sensitive (address_is_valid);
        }
            break;
        default:
            okbutton->set_sensitive (true);
            break;
        }
    }

    void on_text_changed_signal ()
    {
        NEMIVER_TRY
        update_ok_button_sensitivity ();
        NEMIVER_CATCH
    }

    void on_radiobutton_changed ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        NEMIVER_TRY

        THROW_IF_FAIL (entry_filename);
        THROW_IF_FAIL (entry_line);
        THROW_IF_FAIL (entry_function);
        THROW_IF_FAIL (entry_address);

        SetBreakpointDialog::Mode a_mode = mode ();

        entry_function->set_sensitive (a_mode == MODE_FUNCTION_NAME);
        entry_filename->set_sensitive (a_mode == MODE_SOURCE_LOCATION);
        entry_line->set_sensitive (a_mode == MODE_SOURCE_LOCATION);
        entry_address->set_sensitive (a_mode == MODE_BINARY_ADDRESS);
        combo_event->set_sensitive (a_mode == MODE_EVENT);
        entry_condition->set_sensitive (a_mode != MODE_EVENT);
        check_countpoint->set_sensitive (a_mode != MODE_EVENT);
        update_ok_button_sensitivity ();
        NEMIVER_CATCH
    }

    void mode (SetBreakpointDialog::Mode a_mode)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (radio_source_location);
        THROW_IF_FAIL (radio_function_name);
        THROW_IF_FAIL (entry_line);
        THROW_IF_FAIL (entry_filename);
        THROW_IF_FAIL (entry_function);
        THROW_IF_FAIL (entry_address);

        switch (a_mode) {
            case MODE_SOURCE_LOCATION:
                LOG_DD ("Changing Mode to SOURCE_LOCATION");
                radio_source_location->set_active ();
                entry_filename->grab_focus ();
                break;
            case MODE_FUNCTION_NAME:
                LOG_DD ("Changing Mode to FUNCTION_NAME");
                radio_function_name->set_active ();
                entry_function->grab_focus ();
                break;
            case MODE_BINARY_ADDRESS:
                LOG_DD ("Changing Mode to BINARY_ADDRESS");
                radio_binary_location->set_active ();
                entry_address->grab_focus ();
                break;
            case MODE_EVENT:
                LOG_DD ("Changing Mode to EVENT");
                radio_event->set_active ();
                combo_event->grab_focus ();
                break;
            default:
                THROW ("Should not be reached");
        }
    }

    UString get_active_event () const
    {
        Gtk::TreeModel::iterator iter = combo_event->get_active ();
        return (*iter)[combo_event_cols.m_command];
    }

    void set_active_event (const UString &) const
    {
        //TODO
    }

    SetBreakpointDialog::Mode mode () const
    {
        THROW_IF_FAIL (radio_source_location);
        THROW_IF_FAIL (radio_function_name);


        if (radio_source_location->get_active ()) {
            return MODE_SOURCE_LOCATION;
        } else if (radio_event->get_active ()) {
            return MODE_EVENT;
        } else if (radio_function_name->get_active ()) {
            return MODE_FUNCTION_NAME;
        } else if (radio_binary_location->get_active ()) {
            return MODE_BINARY_ADDRESS;
        } else {
            THROW ("Unreachable code reached");
        }
    }
};//end class SetBreakpointDialog::Priv

/// Constructor of the SetBreakpointDialog type.
///
/// \param a_parent the parent window of the dialog.
///
/// \param a_root_path the path to the root directory of the
/// resources of the dialog.
SetBreakpointDialog::SetBreakpointDialog (Gtk::Window &a_parent,
                                          const UString &a_root_path) :
    Dialog (a_root_path, "setbreakpointdialog.ui",
            "setbreakpointdialog",
            a_parent)
{
    m_priv.reset (new Priv (widget (), gtkbuilder ()));
}

SetBreakpointDialog::~SetBreakpointDialog ()
{
}

UString
SetBreakpointDialog::file_name () const
{
    THROW_IF_FAIL (m_priv);

    string file_path, line_num;
    if (m_priv->get_file_path_and_line_num (file_path, line_num))
        return file_path;

    THROW_IF_FAIL (m_priv->entry_filename);
    return m_priv->entry_filename->get_text ();
}

void
SetBreakpointDialog::file_name (const UString &a_name)
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->entry_filename);
    m_priv->entry_filename->set_text (a_name);
}

int
SetBreakpointDialog::line_number () const
{
    THROW_IF_FAIL (m_priv);

    string file_path, line_num;
    if (m_priv->get_file_path_and_line_num (file_path, line_num))
        return atoi (line_num.c_str ());

    THROW_IF_FAIL (m_priv->entry_line);
    return atoi (m_priv->entry_line->get_text ().c_str ());
}

void
SetBreakpointDialog::line_number (int a_line)
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->entry_line);
    m_priv->entry_line->set_text (UString::from_int(a_line));
}

UString
SetBreakpointDialog::function () const
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->entry_function);

    return m_priv->entry_function->get_text ();
}

void
SetBreakpointDialog::function (const UString &a_name)
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->entry_function);
    m_priv->entry_function->set_text (a_name);
}

Address
SetBreakpointDialog::address () const
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->entry_address);

    Address address;

    UString str = m_priv->entry_address->get_text ();
    if (str_utils::string_is_number (str))
        address = str;
    return address;
}

void
SetBreakpointDialog::address (const Address &a)
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->entry_address);
    m_priv->entry_address->set_text (a.to_string ());
}

UString
SetBreakpointDialog::event () const
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->combo_event);
    return m_priv->get_active_event ();
}

void SetBreakpointDialog::event (const UString &a_event) 
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->combo_event);
    m_priv->set_active_event (a_event);

}

UString
SetBreakpointDialog::condition () const
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->entry_condition);
    return m_priv->entry_condition->get_text ();
}

void
SetBreakpointDialog::condition (const UString &a_cond)
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->entry_condition);
    m_priv->entry_condition->set_text (a_cond);
}

bool
SetBreakpointDialog::count_point () const
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->check_countpoint);
    return m_priv->check_countpoint->get_active ();
}

void
SetBreakpointDialog::count_point (bool a_flag)
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->check_countpoint);
    m_priv->check_countpoint->set_active (a_flag);
}

SetBreakpointDialog::Mode
SetBreakpointDialog::mode () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->mode ();
}

void
SetBreakpointDialog::mode (Mode a_mode)
{
    THROW_IF_FAIL (m_priv);
    m_priv->mode (a_mode);
}

NEMIVER_END_NAMESPACE (nemiver);
