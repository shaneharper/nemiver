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
#include <glib/gi18n.h>
#include <sstream>
#include "common/nmv-loc.h"
#include "common/nmv-str-utils.h"
#include "nmv-set-jump-to-dialog.h"
#include "nmv-ui-utils.h"


using namespace nemiver::common;
using namespace nemiver::ui_utils;

NEMIVER_BEGIN_NAMESPACE (nemiver)

class SetJumpToDialog::Priv
{
public:
    Gtk::Entry *entry_function;
    Gtk::Entry *entry_filename;
    Gtk::Entry *entry_line;
    Gtk::Entry *entry_address;
    Gtk::RadioButton *radio_function_name;
    Gtk::RadioButton *radio_source_location;
    Gtk::RadioButton *radio_binary_location;
    Gtk::CheckButton *check_break_at_destination;
    Gtk::Button *okbutton;
    UString current_file_name;

    enum Mode
    {
        MODE_UNDEFINED = 0,
        MODE_FUNCTION_NAME_LOCATION,
        MODE_SOURCE_LOCATION,
        MODE_BINARY_LOCATION
    };

    Priv (Gtk::Dialog &a_dialog,
          const Glib::RefPtr<Gtk::Builder> &a_builder)
        : entry_function (0),
          entry_filename (0),
          entry_line (0),
          entry_address (0),
          radio_function_name (0),
          radio_source_location (0),
          radio_binary_location (0),
          check_break_at_destination (0),
          okbutton (0)
    {
        a_dialog.set_default_response (Gtk::RESPONSE_OK);

        entry_function =
            get_widget_from_gtkbuilder<Gtk::Entry> (a_builder,
                                                    "functionentry");
        THROW_IF_FAIL (entry_function);
        entry_function->signal_changed ().connect
            (sigc::mem_fun (*this, &Priv::on_text_changed_signal));

        entry_filename =
            get_widget_from_gtkbuilder<Gtk::Entry> (a_builder,
                                                    "filenameentry");
        THROW_IF_FAIL (entry_filename);
        entry_filename->signal_changed ().connect
            (sigc::mem_fun (*this, &Priv::on_text_changed_signal));

        entry_line =
            get_widget_from_gtkbuilder<Gtk::Entry> (a_builder,
                                                    "lineentry");
        THROW_IF_FAIL (entry_line);
        entry_line->signal_changed ().connect
            (sigc::mem_fun (*this, &Priv::on_text_changed_signal));

        entry_address =
            get_widget_from_gtkbuilder<Gtk::Entry> (a_builder,
                                                    "addressentry");
        THROW_IF_FAIL (entry_address);
        entry_address->signal_changed ().connect
            (sigc::mem_fun (*this, &Priv::on_text_changed_signal));

        radio_function_name =
            get_widget_from_gtkbuilder<Gtk::RadioButton>
            (a_builder, "functionnameradio");
        THROW_IF_FAIL (radio_function_name);
        radio_function_name->signal_clicked ().connect
            (sigc::mem_fun (*this, &Priv::on_radiobutton_changed));

        radio_source_location =
            get_widget_from_gtkbuilder<Gtk::RadioButton>
            (a_builder, "sourcelocationradio");
        THROW_IF_FAIL (radio_source_location);
        radio_source_location->signal_clicked ().connect
            (sigc::mem_fun (*this, &Priv::on_radiobutton_changed));

        radio_binary_location =
            get_widget_from_gtkbuilder<Gtk::RadioButton>
            (a_builder, "binarylocationradio");
        THROW_IF_FAIL (radio_binary_location);
        radio_binary_location->signal_clicked ().connect
            (sigc::mem_fun (*this, &Priv::on_radiobutton_changed));

        check_break_at_destination =
            get_widget_from_gtkbuilder<Gtk::CheckButton>
            (a_builder, "breakatdestinationcheck");
        THROW_IF_FAIL (check_break_at_destination);

        okbutton =
            get_widget_from_gtkbuilder<Gtk::Button>
            (a_builder, "okbutton");
        THROW_IF_FAIL (okbutton);
    }

    /// Return the 'location setting mode' of the dialog.  I.e, it can
    /// set either function name, source or binary address locations,
    /// depending on the radio button that got selected by the user.
    ///
    /// \return the 'location setting mode' radio button selected by
    /// the user.
    Mode
    mode () const
    {
        if (radio_function_name->get_active ())
            return MODE_FUNCTION_NAME_LOCATION;
        else if (radio_source_location->get_active ())
            return MODE_SOURCE_LOCATION;
        else if (radio_binary_location->get_active ())
            return MODE_BINARY_LOCATION;
        else
            return MODE_UNDEFINED;
    }

    /// Set the 'location setting mode' of the dialog.
    ///
    /// \param a_mode the new mode to set.
    void
    mode (Mode a_mode)
    {
        switch (a_mode) {
        case MODE_UNDEFINED:
            THROW ("Unreachable code reached");
            break;
        case MODE_FUNCTION_NAME_LOCATION:
            radio_function_name->set_active ();
            break;
        case MODE_SOURCE_LOCATION:
            radio_source_location->set_active ();
            break;
        case MODE_BINARY_LOCATION:
            radio_binary_location->set_active ();
            break;
        }
    }

    /// Return the location selected by the user using this dialog.
    /// This is a pointer that needs to deleted by the caller.
    ///
    /// \return a pointer to the location selected by the user of the
    /// dialog.  Must be deleted by the caller using the delete
    /// operator.
    Loc*
    get_location () const
    {
        const Mode m = mode ();
        switch (m) {
        case MODE_UNDEFINED:
            THROW ("Unreachable code reached");
            break;
        case MODE_FUNCTION_NAME_LOCATION: {
            FunctionLoc *loc =
                new FunctionLoc (entry_function->get_text ());
            return loc;
        }
        case MODE_SOURCE_LOCATION: {
            std::string path, line;
            if (get_file_path_and_line_num (path, line)) {
                SourceLoc *loc =
                    new SourceLoc (path, atoi (line.c_str ()));
                return loc;
            }
        }
            break;
        case MODE_BINARY_LOCATION: {
            Address a (entry_address->get_text ());
            AddressLoc *loc = new AddressLoc (a);
            return loc;
        }
        } // end case

        return 0;
    }

    /// Set a location in the dialog.  Depending on the type of
    /// location, this function will Do The Right Thing, e.g,
    /// selecting the right radio button etc
    void
    set_location (const Loc &a_loc)
    {
        switch (a_loc.kind ()) {
        case Loc::UNDEFINED_LOC_KIND:
            break;
        case Loc::SOURCE_LOC_KIND: {
            const SourceLoc &loc = static_cast<const SourceLoc&> (a_loc);
            mode (MODE_SOURCE_LOCATION);
            entry_filename->set_text (loc.file_path ());
            std::ostringstream o;
            o << loc.line_number ();
            entry_line->set_text (o.str ());
        }
            break;
        case Loc::FUNCTION_LOC_KIND: {
            const FunctionLoc &loc = static_cast<const FunctionLoc&> (a_loc);
            mode (MODE_FUNCTION_NAME_LOCATION);
            entry_function->set_text (loc.function_name ());
        }
            break;
        case Loc::ADDRESS_LOC_KIND: {
            const AddressLoc &loc = static_cast<const AddressLoc&> (a_loc);
            mode (MODE_BINARY_LOCATION);
            std::ostringstream o;
            o << loc.address ();
            entry_address->set_text (o.str ());
        }
            break;
        }
    }

    ///  Set the checkbox which semantic is to set a breakpoint at the
    ///  jump location selected by the user.
    ///
    ///  \param a boolean meaning whether to select the "break at
    ///  location" checkbox or not.
    void
    set_break_at_location (bool a)
    {
        check_break_at_destination->set_active (a);
    }

    /// Query whether the "break at location" checkbox is selected or
    /// not.
    ///
    /// \return true if the "break at location" checkbox is selected,
    /// false otherwise.
    bool
    get_break_at_location () const
    {
        return check_break_at_destination->get_active ();
    }

    /// Return the path/line number combo that the user entered in the
    /// dialog, iff the dialog is in the "source location" mode.
    ///
    /// \param a_path the path of the source location.  This is set
    /// iff the function returns true.
    ///
    /// \param a_line_num a string representing the line number of the
    /// source location.  This is set iff the function returns true.
    ///
    /// \return true if the dialog is in source location mode and if
    /// the user entered a file path or line number.
    bool
    get_file_path_and_line_num (std::string &a_path,
                                std::string &a_line_num) const
    {
        if (entry_line->get_text ().empty ()) {
            // Try to see if entry_filename contains a location string
            // of the form "filename:number".
            return str_utils::extract_path_and_line_num_from_location
                (entry_filename->get_text ().raw (), a_path, a_line_num);
        } else {
            // Try to just get the file path from the entry_filename
            // and line from entry_line.
            //
            // If entry_filename is empty then assume the user is
            // referring to the file name being currently visited at
            // the moment the dialog was being used.  That file name
            // must have been set with
            // SetJumpToDialog::set_current_file_name.
            UString file_name;
            if (entry_filename->get_text ().empty ())
                file_name = current_file_name;
            else
                file_name = entry_filename->get_text ();

            if (!file_name.empty ()
                && atoi (entry_line->get_text ().c_str ())) {
                a_path = file_name;
                a_line_num = entry_line->get_text ().raw ();
                return true;
            }
        }
        return false;
    }

    /// Update the sensitivity of the OK button, depending on the
    /// entries the user modified and what she typed in.
    void
    update_ok_button_sensitivity ()
    {
        THROW_IF_FAIL (entry_filename);
        THROW_IF_FAIL (entry_line);
        THROW_IF_FAIL (entry_function);
        THROW_IF_FAIL (entry_address);
        THROW_IF_FAIL (okbutton);

        Mode m = mode ();
        switch (m) {
        case MODE_UNDEFINED:
            break;
        case MODE_SOURCE_LOCATION: {
            string filename, line;
            if (get_file_path_and_line_num (filename, line)
                || atoi (entry_line->get_text ().c_str ()))
                okbutton->set_sensitive (true);
            else
                okbutton->set_sensitive (false);
        }
        break;
        case MODE_FUNCTION_NAME_LOCATION: {
            if (!entry_function->get_text ().empty ())
                okbutton->set_sensitive (true);
            else
                okbutton->set_sensitive (false);
        }
            break;
        case MODE_BINARY_LOCATION: {
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

    /// Set the name of the file being currently visited by the
    /// editor.  This can be useful because it can save the user from
    /// having to type in the name of the current file, and leave the
    /// file name entry blank.
    ///
    /// \param a the name of the file name currently being visited
    void
    set_current_file_name (const UString &a)
    {
        current_file_name = a;
    }

    void
    on_text_changed_signal ()
    {
        NEMIVER_TRY;

        update_ok_button_sensitivity ();

        NEMIVER_CATCH;
    }

    void
    on_radiobutton_changed ()
    {
        NEMIVER_TRY;

        Mode m = mode ();

        entry_function->set_sensitive (m == MODE_FUNCTION_NAME_LOCATION);

        entry_filename->set_sensitive (m == MODE_SOURCE_LOCATION);
        entry_line->set_sensitive (m == MODE_SOURCE_LOCATION);

        entry_address->set_sensitive (m == MODE_BINARY_LOCATION);

        update_ok_button_sensitivity ();

        NEMIVER_CATCH;
    }
};// end class SetJumpToDialog::Priv

SetJumpToDialog::SetJumpToDialog (const UString &a_root_path)
    : Dialog (a_root_path, "setjumptodialog.ui", "setjumptodialog")
{
    m_priv.reset (new Priv (widget (), gtkbuilder ()));
}

SetJumpToDialog::~SetJumpToDialog ()
{
}

/// \return the location set by the user via the dialog
const Loc*
SetJumpToDialog::get_location () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->get_location ();
}

/// Set the location proprosed by the dialog.
///
/// \param a_loc the new location to set
void
SetJumpToDialog::set_location (const Loc &a_loc)
{
    THROW_IF_FAIL (m_priv);
    
    m_priv->set_location (a_loc);
}

/// Set or unset the "set break at location" check button
///
/// \param a if true, check the "set break at location" check button.
/// Uncheck it otherwise.
void
SetJumpToDialog::set_break_at_location (bool a)
{
    THROW_IF_FAIL (m_priv);
    m_priv->set_break_at_location (a);
}

/// Get the value of the "set break at location" check button.
///
/// \return true if the "set break at location" check button is
/// checked, false otherwise.
bool
SetJumpToDialog::get_break_at_location () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->get_break_at_location ();
}

/// Set the name of the file being currently visited by the editor.
/// This can be useful because it can save the user from having to
/// type in the name of the current file, and leave the file name
/// entry blank.
///
/// \param a the name of the file name currently being visited
void
SetJumpToDialog::set_current_file_name (const UString &a)
{
    THROW_IF_FAIL (m_priv);
    m_priv->set_current_file_name (a);
}
NEMIVER_END_NAMESPACE (nemiver)
