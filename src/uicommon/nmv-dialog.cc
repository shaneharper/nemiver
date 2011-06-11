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

#include <vector>
#include <gtkmm/dialog.h>
#include "common/nmv-exception.h"
#include "common/nmv-env.h"
#include "common/nmv-ustring.h"
#include "nmv-ui-utils.h"
#include "nmv-dialog.h"

using namespace std;
using namespace nemiver::common;

namespace nemiver {
class Dialog::Priv {
    Priv ();
public:

    SafePtr<Gtk::Dialog> dialog;
    Glib::RefPtr<Gtk::Builder> gtkbuilder;

    Priv (const UString &a_resource_root_path,
          const UString &a_gtkbuilder_filename,
          const UString &a_widget_name)
    {
        string gtkbuilder_path;
        if (!a_resource_root_path.empty ()) {
            // So the glade file is shipped within a plugin. Build the
            // path to it accordingly.
            vector<string> path_elems;
            path_elems.push_back (Glib::locale_from_utf8 (a_resource_root_path));
            path_elems.push_back ("ui");
            path_elems.push_back (a_gtkbuilder_filename);
            gtkbuilder_path = Glib::build_filename (path_elems);
        } else {
            // THe glade file is shipped into the global nemiver glade
            // directories.
            gtkbuilder_path = 
	      env::build_path_to_gtkbuilder_file (a_gtkbuilder_filename);
        }

        if (!Glib::file_test (gtkbuilder_path, Glib::FILE_TEST_IS_REGULAR)) {
            THROW (UString ("could not find file ") + gtkbuilder_path);
        }

        gtkbuilder = Gtk::Builder::create_from_file (gtkbuilder_path);
        THROW_IF_FAIL (gtkbuilder);
        dialog.reset
            (ui_utils::get_widget_from_gtkbuilder<Gtk::Dialog> (gtkbuilder,
                                                                a_widget_name));
        THROW_IF_FAIL (dialog);
        dialog->hide ();
    }
};//end struct Dialog::Priv

Dialog::Dialog (const UString &a_resource_root_path,
                const UString &a_gtkbuilder_filename,
                const UString &a_widget_name)
{
    m_priv.reset (new Priv (a_resource_root_path,
                            a_gtkbuilder_filename,
                            a_widget_name));
}

Gtk::Dialog&
Dialog::widget () const
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->dialog);
    return *m_priv->dialog;
}

const Glib::RefPtr<Gtk::Builder>
Dialog::gtkbuilder () const
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->gtkbuilder);
    return m_priv->gtkbuilder;
}

Dialog::~Dialog ()
{
}

int
Dialog::run ()
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->dialog);
    return m_priv->dialog->run ();
}

void
Dialog::show ()
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->dialog);
    return m_priv->dialog->show ();
}

void
Dialog::hide ()
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->dialog);
    return m_priv->dialog->hide ();
}

Glib::SignalProxy1<void, int>
Dialog::signal_response ()
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->dialog);
    return m_priv->dialog->signal_response ();
}

}//end namespace nemiver

