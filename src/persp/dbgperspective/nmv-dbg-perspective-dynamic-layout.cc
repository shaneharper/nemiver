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
#include "common/nmv-safe-ptr.h"
#include "common/nmv-safe-ptr-utils.h"
#include "nmv-dbg-perspective.h"
#include "nmv-dbg-perspective-dynamic-layout.h"
#include "nmv-ui-utils.h"
#include "nmv-conf-keys.h"

#include <gdlmm.h>
#include <gtkmm.h>
#include <glib/gi18n.h>

using namespace std;
using namespace nemiver::ui_utils;

NEMIVER_BEGIN_NAMESPACE (nemiver)

struct GObjectMMRef {

    void operator () (Glib::Object *a_obj)
    {
        if (a_obj) {
            a_obj->reference ();
        }
    }
};//end GlibObjectRef

struct GObjectMMUnref {
    void operator () (Glib::Object *a_obj)
    {
        if (a_obj) {
            a_obj->unreference ();
        }
    }
};//end GlibObjectRef

typedef SafePtr<Gdl::DockItem, GObjectMMRef, GObjectMMUnref> DockItemPtr;

/// \brief Dynamic Layout
///
/// This layout is based on the external library gdlmm.
/// The user can directly change the layout, but by default every debugging
/// panels are inside a single notebook below the sourceview notebook.
struct DBGPerspectiveDynamicLayout::Priv {
    SafePtr<Gtk::Box> main_box;

    SafePtr<Gdl::Dock> dock;
    SafePtr<Gdl::DockBar> dock_bar;
    Glib::RefPtr<Gdl::DockLayout> dock_layout;
    SafePtr<Gdl::DockItem> source_item;

    map<int, DockItemPtr> views;

    IDBGPerspective &dbg_perspective;

    Priv (IDBGPerspective &a_dbg_perspective) :
        dbg_perspective (a_dbg_perspective)
    {
    }

    void
    iconify_item_if_detached (Gdl::DockItem& a_item)
    {
        THROW_IF_FAIL (dock);

        if (!a_item.get_parent_object ()) {
            dock->add_item (a_item, Gdl::DOCK_NONE);
            a_item.iconify_item ();
        }
    }

    const UString&
    dynamic_layout_configuration_filepath () const
    {
        static UString file = Glib::build_filename (Glib::get_home_dir (),
                                                    ".nemiver",
                                                    "config",
                                                    "dynamic-layout.xml");
        return file;
    }
}; //end struct DBGPerspectiveDynamicLayout::Priv

Gtk::Widget*
DBGPerspectiveDynamicLayout::widget () const
{
    return m_priv->main_box.get ();
}

DBGPerspectiveDynamicLayout::DBGPerspectiveDynamicLayout ()
{
    Gdl::init ();
}

DBGPerspectiveDynamicLayout::~DBGPerspectiveDynamicLayout ()
{
    LOG_D ("deleted", "destructor-domain");
}

void
DBGPerspectiveDynamicLayout::do_lay_out (IPerspective &a_perspective)
{
    m_priv.reset (new Priv (dynamic_cast<IDBGPerspective&> (a_perspective)));
    THROW_IF_FAIL (m_priv);

    m_priv->source_item.reset
            (new Gdl::DockItem ("source",
                                _("Source Code"),
                                Gdl::DOCK_ITEM_BEH_CANT_CLOSE |
                                Gdl::DOCK_ITEM_BEH_LOCKED |
                                Gdl::DOCK_ITEM_BEH_CANT_ICONIFY |
                                Gdl::DOCK_ITEM_BEH_NO_GRIP));
    m_priv->source_item->add
        (m_priv->dbg_perspective.get_source_view_widget ());

    m_priv->dock.reset (new Gdl::Dock);
    Glib::RefPtr<Gdl::DockMaster> master = m_priv->dock->get_master ();
    if (master) {
        master->property_switcher_style () = Gdl::SWITCHER_STYLE_TABS;
    }

    m_priv->dock->add_item (*m_priv->source_item, Gdl::DOCK_TOP);

    m_priv->dock_bar.reset (new Gdl::DockBar (*m_priv->dock));
    m_priv->dock_bar->set_style (Gdl::DOCK_BAR_TEXT);

    m_priv->main_box.reset (new Gtk::HBox);
    m_priv->main_box->pack_start (*m_priv->dock_bar, false, false);
    m_priv->main_box->pack_end (*m_priv->dock);
    m_priv->main_box->show_all ();

    m_priv->dock_layout = Gdl::DockLayout::create (*m_priv->dock);
}

void
DBGPerspectiveDynamicLayout::do_init ()
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->dock_layout);

    if (Glib::file_test (m_priv->dynamic_layout_configuration_filepath (),
                         Glib::FILE_TEST_EXISTS | Glib::FILE_TEST_IS_REGULAR)) {
        m_priv->dock_layout->load_from_file
            (m_priv->dynamic_layout_configuration_filepath ());
        m_priv->dock_layout->load_layout (identifier ());
    }

    for (std::map<int, DockItemPtr>::iterator item = m_priv->views.begin ();
         item != m_priv->views.end ();
         ++item) {
        m_priv->iconify_item_if_detached (*item->second);
    }
}

void
DBGPerspectiveDynamicLayout::do_cleanup_layout ()
{
    m_priv.reset ();
}

const UString&
DBGPerspectiveDynamicLayout::identifier () const
{
    static const UString s_id = "dynamic-layout";
    return s_id;
}

const UString&
DBGPerspectiveDynamicLayout::name () const
{
    static const UString s_name = _("Dynamic Layout");
    return s_name;
}

const UString&
DBGPerspectiveDynamicLayout::description () const
{
    static const UString s_description = _("A layout which can be modified");
    return s_description;
}

void
DBGPerspectiveDynamicLayout::activate_view (int a_view)
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->views.count (a_view));

    DockItemPtr dock_item = m_priv->views[a_view];
    if (!dock_item) {
        LOG_ERROR ("Trying to activate a widget with a null pointer");
        return;
    }

    if (!dock_item->get_parent_object ()) {
        dock_item->show_item ();
    } else {
        dock_item->present (*dock_item->get_parent_object ());
    }
}

void
DBGPerspectiveDynamicLayout::save_configuration ()
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->dock_layout);

    if (m_priv->dock_layout->is_dirty ()) {
        m_priv->dock_layout->save_layout (identifier ());
        m_priv->dock_layout->save_to_file
            (m_priv->dynamic_layout_configuration_filepath ());
    }
}

void
DBGPerspectiveDynamicLayout::add_view (Gtk::Widget &a_widget,
                                       const UString &a_title,
                                       int a_index)
{
    THROW_IF_FAIL (m_priv);
    if (m_priv->views.count (a_index) || a_widget.get_parent ()) {
        return;
    }

    // Otherwise the widget takes all the horizontal|vertical place
    // and cannot be reduced.
#ifdef WITH_MEMORYVIEW
    if (a_index == TARGET_TERMINAL_VIEW_INDEX || a_index == MEMORY_VIEW_INDEX) {
#else
    if (a_index == TARGET_TERMINAL_VIEW_INDEX) {
#endif // WITH_MEMORYVIEW
        IConfMgr &conf_mgr = m_priv->dbg_perspective.get_conf_mgr ();
        int width = 100;
        int height = 70;

        NEMIVER_TRY
        conf_mgr.get_key_value (CONF_KEY_STATUS_WIDGET_MINIMUM_WIDTH, width);
        conf_mgr.get_key_value (CONF_KEY_STATUS_WIDGET_MINIMUM_HEIGHT, height);
        NEMIVER_CATCH

        a_widget.set_size_request (width, height);
    }

    DockItemPtr item (Gtk::manage (new Gdl::DockItem
            (a_title, a_title, Gdl::DOCK_ITEM_BEH_CANT_CLOSE)));
    THROW_IF_FAIL (item);
    item->reference ();
    m_priv->dock->add_item (*item, Gdl::DOCK_BOTTOM);

    // Attach those widgets to the terminal_item.
    // Gdl::DOCK_CENTER means that the widget will form a notebook with the
    // widget they are docked to.
    if (m_priv->views.size ()) {
        item->dock_to (*m_priv->views.begin ()->second, Gdl::DOCK_CENTER);
    }

    m_priv->views[a_index] = item;
    item->add (a_widget);
    item->show_all ();
}

void
DBGPerspectiveDynamicLayout::remove_view (int a_index)
{
    THROW_IF_FAIL (m_priv);
    if (!m_priv->views.count (a_index)) {
        return;
    }

    m_priv->dock->remove (*m_priv->views[a_index]);
    m_priv->views.erase (a_index);
}

NEMIVER_END_NAMESPACE (nemiver)
