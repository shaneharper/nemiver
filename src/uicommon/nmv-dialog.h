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
#ifndef __NMV_DIALOG_H__
#define __NMV_DIALOG_H__

#include <gtkmm/builder.h>
#include "common/nmv-object.h"

namespace Gtk {
    class Dialog;
}

namespace Gnome {
    namespace Glade {
        class Xml;
    }
}

NEMIVER_BEGIN_NAMESPACE (nemiver)

namespace common {
class UString;
}

using nemiver::common::UString;
using nemiver::common::SafePtr;

class Dialog : public common::Object {
    class Priv;
    friend class Priv;
    SafePtr<Priv> m_priv;
    //non copyable
    Dialog (const Dialog&);
    Dialog& operator= (const Dialog&);

    Dialog ();

protected:
    //the actual underlying Gtk::Dialog widget
    Gtk::Dialog& widget () const;

    //the actual gtkbuilder object loaded by this dialog.
    const Glib::RefPtr<Gtk::Builder> gtkbuilder ()  const;

public:

    Dialog (const UString &a_resource_root_path,
            const UString &a_gtkbuilder_filename,
            const UString &a_widget_name);

    virtual ~Dialog ();

    virtual gint run ();

    virtual void show ();

    virtual void hide ();

    Glib::SignalProxy1<void, int> signal_response ();

};//end class nemiver

NEMIVER_END_NAMESPACE (nemiver)

#endif //__NMV_DIALOG_H__


