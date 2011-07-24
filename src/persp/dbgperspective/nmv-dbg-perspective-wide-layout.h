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
#ifndef __NMV_DBG_PERSPECTIVE_WIDE_LAYOUT_H__
#define __NMV_DBG_PERSPECTIVE_WIDE_LAYOUT_H__

#include <gtkmm/widget.h>
#include "common/nmv-safe-ptr-utils.h"
#include "uicommon/nmv-layout.h"

NEMIVER_BEGIN_NAMESPACE (nemiver)

class NEMIVER_API DBGPerspectiveWideLayout : public Layout {
    //non copyable
    DBGPerspectiveWideLayout (const Layout&);
    DBGPerspectiveWideLayout& operator= (const Layout&);

    struct Priv;
    SafePtr<Priv> m_priv;

public:
    DBGPerspectiveWideLayout ();

    void activate_view (int);

    Gtk::Widget* widget () const;

    void do_lay_out (IPerspective&);

    void do_init ();

    void do_cleanup_layout ();

    const UString& identifier () const;

    const UString& name () const;

    const UString& description () const;

    void save_configuration ();

    void add_view (Gtk::Widget&, const UString&, int);

    void remove_view (int);

    virtual ~DBGPerspectiveWideLayout ();
};//end class DBGPerspectiveDefaultLayout

NEMIVER_END_NAMESPACE (nemiver)
#endif //__NMV_DBG_PERSPECTIVE_WIDE_LAYOUT_H__
