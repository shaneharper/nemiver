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
#include "config.h"
#include <vector>
#include <glib/gi18n.h>
#include <gtkmm/dialog.h>
#include <gtkmm/filechooserbutton.h>
#include <gtkmm/stock.h>
#include "common/nmv-exception.h"
#include "common/nmv-env.h"
#include "common/nmv-ustring.h"
#include "nmv-load-report-dialog.h"
#include "nmv-ui-utils.h"

using namespace std;
using namespace nemiver::common;

NEMIVER_BEGIN_NAMESPACE (nemiver)

struct LoadReportDialog::Priv {
    Gtk::Button *okbutton;
    Gtk::FileChooserButton *fcbutton_report_file;

    Priv (const Glib::RefPtr<Gtk::Builder> &a_gtkbuilder) :
        okbutton (0),
        fcbutton_report_file (0)
    {
        okbutton = ui_utils::get_widget_from_gtkbuilder<Gtk::Button>
            (a_gtkbuilder, "okbutton");
        THROW_IF_FAIL (okbutton);
        okbutton->set_sensitive (false);

        fcbutton_report_file =
            ui_utils::get_widget_from_gtkbuilder<Gtk::FileChooserButton>
                (a_gtkbuilder, "filechooserbutton_reportfile");
        fcbutton_report_file->signal_selection_changed ().connect
            (sigc::mem_fun
                (*this, &Priv::on_file_selection_changed_signal));
        fcbutton_report_file->set_current_folder (Glib::get_current_dir ());
    }

    void on_file_selection_changed_signal ()
    {
        NEMIVER_TRY

        THROW_IF_FAIL (fcbutton_report_file);
        okbutton->set_sensitive (Glib::file_test
            (fcbutton_report_file->get_filename (),
             Glib::FILE_TEST_IS_REGULAR));

        NEMIVER_CATCH
    }
};//end class LoadReportDialog::Priv

LoadReportDialog::LoadReportDialog (const UString &a_root_path) :
    Dialog (a_root_path, "loadreportdialog.ui", "loadreportdialog")
{
    m_priv.reset (new Priv (gtkbuilder ()));
}

LoadReportDialog::~LoadReportDialog ()
{
}

UString
LoadReportDialog::report_file () const
{
    NEMIVER_TRY

    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->fcbutton_report_file);

    NEMIVER_CATCH
    return m_priv->fcbutton_report_file->get_filename ();
}

NEMIVER_END_NAMESPACE (nemiver)

