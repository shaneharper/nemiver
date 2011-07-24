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
#include "nmv-dbg-perspective.h"
#include "nmv-dbg-perspective-default-layout.h"
#include "nmv-ui-utils.h"
#include "nmv-conf-keys.h"

#include <gtkmm/notebook.h>
#include <glib/gi18n.h>

using namespace std;
using namespace nemiver::ui_utils;

NEMIVER_BEGIN_NAMESPACE (nemiver)

/// \brief Default Nemiver's Layout
///
/// This is the default nemiver's layout.
/// Every debugging panels are inside a single notebook which is displayed
/// below the sourceview notebook.
struct DBGPerspectiveDefaultLayout::Priv {
    SafePtr<Gtk::Paned> body_main_paned;
    SafePtr<Gtk::Notebook> statuses_notebook;
    map<int, Gtk::Widget&> views;
    IDBGPerspective &dbg_perspective;

    Priv (IDBGPerspective &a_dbg_perspective) :
        dbg_perspective (a_dbg_perspective)
    {
    }
}; //end struct DBGPerspectiveDefaultLayout::Priv

Gtk::Widget*
DBGPerspectiveDefaultLayout::widget () const
{
    return m_priv->body_main_paned.get ();
}

DBGPerspectiveDefaultLayout::DBGPerspectiveDefaultLayout ()
{
}

DBGPerspectiveDefaultLayout::~DBGPerspectiveDefaultLayout ()
{
    LOG_D ("deleted", "destructor-domain");
}

void
DBGPerspectiveDefaultLayout::do_lay_out (IPerspective &a_perspective)
{
    m_priv.reset (new Priv (dynamic_cast<IDBGPerspective&> (a_perspective)));
    THROW_IF_FAIL (m_priv);

    m_priv->body_main_paned.reset (new Gtk::VPaned);
    m_priv->body_main_paned->set_position (350);

    // set the position of the status pane to the last saved position
    IConfMgr &conf_mgr = m_priv->dbg_perspective.get_conf_mgr ();
    int pane_location = -1; // don't specifically set a location
                            // if we can't read the last location from gconf
    NEMIVER_TRY
    conf_mgr.get_key_value (CONF_KEY_DEFAULT_LAYOUT_STATUS_PANE_LOCATION,
                            pane_location);
    NEMIVER_CATCH

    if (pane_location >= 0) {
        m_priv->body_main_paned->set_position (pane_location);
    }

    m_priv->statuses_notebook.reset (new Gtk::Notebook);
    m_priv->statuses_notebook->set_tab_pos (Gtk::POS_BOTTOM);

    m_priv->body_main_paned->pack2 (*m_priv->statuses_notebook);
    m_priv->body_main_paned->pack1
        (m_priv->dbg_perspective.get_source_view_widget (), true, true);

    int width=100, height=70;

    NEMIVER_TRY
    conf_mgr.get_key_value (CONF_KEY_STATUS_WIDGET_MINIMUM_WIDTH, width);
    conf_mgr.get_key_value (CONF_KEY_STATUS_WIDGET_MINIMUM_HEIGHT, height);
    NEMIVER_CATCH

    LOG_DD ("setting status widget min size: width: "
            << width
            << ", height: "
            << height);
    m_priv->statuses_notebook->set_size_request (width, height);
    m_priv->body_main_paned->show_all ();
}

void
DBGPerspectiveDefaultLayout::do_init ()
{
}

void
DBGPerspectiveDefaultLayout::do_cleanup_layout ()
{
    m_priv.reset ();
}

const UString&
DBGPerspectiveDefaultLayout::identifier () const
{
    static const UString s_id = "default-layout";
    return s_id;
}

const UString&
DBGPerspectiveDefaultLayout::name () const
{
    static const UString s_name = _("Default Layout");
    return s_name;
}

const UString&
DBGPerspectiveDefaultLayout::description () const
{
    static const UString s_description = _("Nemiver's default layout");
    return s_description;
}

void
DBGPerspectiveDefaultLayout::activate_view (int a_view)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->statuses_notebook);

    m_priv->statuses_notebook->set_current_page (a_view);
}

void
DBGPerspectiveDefaultLayout::save_configuration ()
{
    THROW_IF_FAIL (m_priv && m_priv->body_main_paned);

    // save the location of the status pane so
    // that it'll open in the same place
    // next time.
    IConfMgr &conf_mgr = m_priv->dbg_perspective.get_conf_mgr ();
    int pane_location = m_priv->body_main_paned->get_position ();

    NEMIVER_TRY
    conf_mgr.set_key_value (CONF_KEY_DEFAULT_LAYOUT_STATUS_PANE_LOCATION,
                            pane_location);
    NEMIVER_CATCH
}

void
DBGPerspectiveDefaultLayout::add_view (Gtk::Widget &a_widget,
                                       const UString &a_title,
                                       int a_index)
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->statuses_notebook);

    if (m_priv->views.count (a_index) || a_widget.get_parent ()) {
        return;
    }

    m_priv->views.insert (std::make_pair<int, Gtk::Widget&> (a_index, a_widget));
    a_widget.show_all ();
    int page_num = m_priv->statuses_notebook->insert_page (a_widget,
                                                           a_title,
                                                           a_index);
    m_priv->statuses_notebook->set_current_page (page_num);
}

void
DBGPerspectiveDefaultLayout::remove_view (int a_index)
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->statuses_notebook);

    if (!m_priv->views.count (a_index)) {
        return;
    }

    m_priv->statuses_notebook->remove_page (m_priv->views.at (a_index));
    m_priv->views.erase (a_index);
}

NEMIVER_END_NAMESPACE (nemiver)
