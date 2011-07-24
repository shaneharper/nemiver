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
#ifndef __NMV_LAYOUT_H__
#define __NMV_LAYOUT_H__

#include "common/nmv-ustring.h"
#include "common/nmv-object.h"
#include "common/nmv-safe-ptr-utils.h"

namespace Gtk {
    class Widget;
}//end namespace Gtk

NEMIVER_BEGIN_NAMESPACE (nemiver)

using namespace common;

class IPerspective;
class Layout;
typedef SafePtr<Layout, ObjectRef, ObjectUnref> LayoutSafePtr;

/// \brief The base class for Layouts
///
/// Perspectives can use the LayoutManager to manage different Layouts.
/// The LayoutManager handle object that inherit from the class Layout.
class Layout : public Object {
    //non copyable
    Layout (const Layout&);
    Layout& operator= (const Layout&);

protected:
    Layout () {}

public:
    /// \brief initialize the layout
    /// \param a_perspective The associated perspective.
    virtual void do_lay_out (IPerspective &a_perspective) = 0;

    /// \brief clean-up the layout
    ///
    /// Must be called before initializing a new layout for a perspective.
    virtual void do_cleanup_layout () = 0;

    /// \brief gets the layout unique identifier
    /// \return layout unique identifier
    virtual const UString& identifier () const = 0;

    /// \brief gets the layout name
    /// \return layout name
    virtual const UString& name () const = 0;

    /// \brief gets the layout description
    /// \return layout description
    virtual const UString& description () const = 0;

    /// \brief gets the layout container widget
    /// \return layout container widget
    virtual Gtk::Widget* widget () const = 0;

    /// \brief Initialize the layout
    virtual void do_init () = 0;

    /// \brief save the configuration of the layout
    virtual void save_configuration () = 0;

    /// \brief activate a view
    /// \param a_view_identifier The view to activate
    virtual void activate_view (int a_view_identifier) = 0;

    /// \brief add a view to the layout
    /// \param a_widget Widget of the view to add to the layout
    /// \param a_title Title of the view (will appears in notebook label, ...)
    /// \param a_index Unique identifier of the view to add to the layout
    virtual void add_view (Gtk::Widget &a_widget,
                           const UString &a_title,
                           int a_index) = 0;

    /// \brief remove a view from the layout
    /// \param a_index Unique identifier of the view to remove from the layout
    virtual void remove_view (int a_index) = 0;

    virtual ~Layout () {}
};

NEMIVER_END_NAMESPACE (nemiver)

#endif //__NMV_LAYOUT_H__
