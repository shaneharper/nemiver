//Author: Jonathon Jongsma
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
#include <glib/gi18n.h>
#include <gtkmm/filechooserwidget.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/button.h>
#include <gtkmm/box.h>
#include "common/nmv-exception.h"
#include "common/nmv-ustring.h"
#include "nmv-open-file-dialog.h"
#include "nmv-ui-utils.h"
#include "nmv-file-list.h"

using namespace std;
using namespace nemiver::common;

namespace nemiver {
class OpenFileDialog::Priv {
    public:
    Gtk::Box* vbox_file_list;
    Gtk::RadioButton *radio_button_file_list, *radio_button_chooser;
    Gtk::FileChooserWidget file_chooser;
    FileList file_list;
    Gtk::Button *okbutton;
    IDebugger *debugger; //a poor man weak ref. I don't want to ref this here.

public:

    Priv (const Glib::RefPtr<Gtk::Builder> &a_gtkbuilder,
          IDebuggerSafePtr &a_debugger, const UString &a_working_dir) :
        vbox_file_list (0),
        radio_button_file_list (0),
        radio_button_chooser (0),
        file_chooser(Gtk::FILE_CHOOSER_ACTION_OPEN),
        file_list(a_debugger, a_working_dir),
        okbutton (0),
        debugger (a_debugger.get ())
    {

        file_chooser.set_select_multiple (true);
        okbutton =
            ui_utils::get_widget_from_gtkbuilder<Gtk::Button> (a_gtkbuilder,
                                                          "okbutton");
        THROW_IF_FAIL (okbutton);
        vbox_file_list =
            ui_utils::get_widget_from_gtkbuilder<Gtk::Box> (a_gtkbuilder,
                                                        "vbox_file_list");
        THROW_IF_FAIL (vbox_file_list);
        radio_button_file_list =
            ui_utils::get_widget_from_gtkbuilder<Gtk::RadioButton>
                                                (a_gtkbuilder,
                                                 "radiobutton_target");
        THROW_IF_FAIL (radio_button_file_list);
        radio_button_file_list->signal_toggled ().connect (sigc::mem_fun
                    (*this, &Priv::on_radio_button_toggled));
        radio_button_chooser =
            ui_utils::get_widget_from_gtkbuilder<Gtk::RadioButton>
                                                (a_gtkbuilder,
                                                 "radiobutton_other");
        THROW_IF_FAIL (radio_button_chooser);
        radio_button_chooser->signal_toggled ().connect
                    (sigc::mem_fun (*this, &Priv::on_radio_button_toggled));

        file_list.file_activated_signal ().connect
                (sigc::mem_fun (*this, &Priv::on_file_activated_signal));
        file_list.files_selected_signal ().connect
                (sigc::mem_fun (*this, &Priv::on_files_selected_signal));

        file_chooser.signal_selection_changed ().connect
            (sigc::mem_fun (*this,
                            &Priv::on_chooser_selection_changed_signal));

        update_from_debugger_state ();
    }

    void on_radio_button_toggled()
    {
        NEMIVER_TRY

        THROW_IF_FAIL (vbox_file_list);
        THROW_IF_FAIL (radio_button_file_list);
        THROW_IF_FAIL (radio_button_chooser);

        if (radio_button_file_list->get_active ()) {
            LOG_DD("Target file list is active");
            // remove existing children of vbox_file_list
            vbox_file_list->foreach (sigc::mem_fun (vbox_file_list,
                                                    &Gtk::Box::remove));
            vbox_file_list->pack_start (file_list.widget ());
            file_list.widget ().show ();
        } else if (radio_button_chooser->get_active ()) {
            LOG_DD("file chooser is active");
            // remove existing children of vbox_file_list
            vbox_file_list->foreach (sigc::mem_fun (vbox_file_list,
                                                    &Gtk::Box::remove));
            vbox_file_list->pack_start (file_chooser);
            file_chooser.show ();
        }
        NEMIVER_CATCH
    }

    void update_from_debugger_state ()
    {
        if (debugger) {
            LOG_DD ("debugger state: " << (int) debugger->get_state ());
        } else {
            LOG_DD ("have null debugger");
        }

        if (debugger && debugger->get_state () == IDebugger::READY) {
            LOG_DD ("debugger ready detected");
            file_list.update_content ();
            radio_button_file_list->set_active (true);
            radio_button_file_list->set_sensitive (true);
        } else {
            LOG_DD ("debugger not ready detected");
            radio_button_chooser->set_active (true);
            radio_button_file_list->set_sensitive (false);
        }
        //it seems that the radiobutton widgets doesn't
        //emit this signal when you toggle them programatically ???
        on_radio_button_toggled ();
    }

    bool validate_source_files(const vector<string> &files)
    {
        if (files.empty()) {
            return false;
        }

        for (vector<string>::const_iterator iter = files.begin ();
             iter != files.end ();
             ++iter) {
            if (!validate_source_file (*iter)) {
                return false;
            }
        }
        return true;
    }

    bool validate_source_file (const UString &a_file)
    {
        if (!Glib::file_test (a_file, Glib::FILE_TEST_IS_REGULAR)) {
            return false;
        }
        return true;
    }

    void on_file_activated_signal (const UString &a_file)
    {
        NEMIVER_TRY

        THROW_IF_FAIL(okbutton);

        if (validate_source_file (a_file)) {
            okbutton->clicked ();
        } else {
            okbutton->set_sensitive (false);
        }
        NEMIVER_CATCH
    }

    void on_files_selected_signal ()
    {
        NEMIVER_TRY

        THROW_IF_FAIL (okbutton);

        vector<string> filenames;
        file_list.get_filenames (filenames);
        if (validate_source_files (filenames)){
            okbutton->set_sensitive (true);
        } else {
            okbutton->set_sensitive (false);
        }

        NEMIVER_CATCH
    }

    void on_chooser_selection_changed_signal ()
    {
        NEMIVER_TRY

        THROW_IF_FAIL (okbutton);

        if (validate_source_files (file_chooser.get_filenames ())) {
            okbutton->set_sensitive (true);
        } else {
            okbutton->set_sensitive (false);
        }
        NEMIVER_CATCH
    }

    void get_filenames (vector<string> &a_files)
    {
        THROW_IF_FAIL(radio_button_file_list);
        THROW_IF_FAIL(radio_button_chooser);

        if (radio_button_file_list->get_active ()) {
            file_list.get_filenames (a_files);
        } else if (radio_button_chooser->get_active ()) {
            a_files = file_chooser.get_filenames ();
        }
    }
};//end class OpenFileDialog::Priv

/// Constructor of the OpenFileDialog type.
///
/// \param a_parent the parent window of the dialog.
///
/// \param a_root_path the path to the root directory of the
/// ressources of the dialog.
///
/// \param a_debugger the IDebugger interface to use.
///
/// \param a_working_dir the directory to consider as the current
/// working directory.
OpenFileDialog::OpenFileDialog (Gtk::Window &a_parent,
                                const UString &a_root_path,
                                IDebuggerSafePtr &a_debugger,
                                const UString  &a_working_dir) :
    Dialog (a_root_path, "openfiledialog.ui",
            "dialog_open_source_file", a_parent)
{
    m_priv.reset (new Priv (gtkbuilder (), a_debugger, a_working_dir));
}

OpenFileDialog::~OpenFileDialog ()
{
    LOG_D ("deleted", "destructor-domain");
}

void
OpenFileDialog::get_filenames (vector<string> &a_files) const
{
    NEMIVER_TRY

    THROW_IF_FAIL (m_priv);
    m_priv->get_filenames (a_files);

    NEMIVER_CATCH
}

}//end namespace nemiver

