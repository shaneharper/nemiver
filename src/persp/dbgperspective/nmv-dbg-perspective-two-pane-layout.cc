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
#include "nmv-dbg-perspective-two-pane-layout.h"
#include "nmv-ui-utils.h"
#include "nmv-conf-keys.h"

#include <gtkmm/notebook.h>
#include <glib/gi18n.h>

using namespace std;
using namespace nemiver::ui_utils;

NEMIVER_BEGIN_NAMESPACE (nemiver)

/// \brief Two-Pane Layout
///
/// This layout is composed of 2 statuses notebook, the first one on the right
/// of the sourceview notebook, and the second one is below the sourceview
/// notebook.
///
/// The composition of these two notebook is the following:
/// Right notebook (called vertical pane in the code): memory, register,
/// and target terminal status view.
/// Bottom notebook (called horizontal pane in the code): context,
/// and breakpoints status view.
struct DBGPerspectiveTwoPaneLayout::Priv {
    SafePtr<Gtk::Paned> vertical_paned;
    SafePtr<Gtk::Paned> horizontal_paned;
    SafePtr<Gtk::Notebook> horizontal_statuses_notebook;
    SafePtr<Gtk::Notebook> vertical_statuses_notebook;
    map<int, Gtk::Widget&> views;
    IDBGPerspective &dbg_perspective;

    Priv (IDBGPerspective &a_dbg_perspective) :
        dbg_perspective (a_dbg_perspective)
    {
    }

    Gtk::Notebook&
    statuses_notebook (int a_view)
    {
        THROW_IF_FAIL (vertical_statuses_notebook);
        THROW_IF_FAIL (horizontal_statuses_notebook);

        switch ((ViewsIndex) a_view) {
            case TARGET_TERMINAL_VIEW_INDEX:
            case REGISTERS_VIEW_INDEX:
#ifdef WITH_MEMORYVIEW
            case MEMORY_VIEW_INDEX:
#endif // WITH_MEMORYVIEW
                return *vertical_statuses_notebook;

            default:
                return *horizontal_statuses_notebook;
        }
    }
}; //end struct DBGPerspectiveDefaultLayout::Priv

Gtk::Widget*
DBGPerspectiveTwoPaneLayout::widget () const
{
    return m_priv->vertical_paned.get ();
}

DBGPerspectiveTwoPaneLayout::DBGPerspectiveTwoPaneLayout ()
{
}

DBGPerspectiveTwoPaneLayout::~DBGPerspectiveTwoPaneLayout ()
{
    LOG_D ("deleted", "destructor-domain");
}

void
DBGPerspectiveTwoPaneLayout::do_lay_out (IPerspective &a_perspective)
{
    m_priv.reset (new Priv (dynamic_cast<IDBGPerspective&> (a_perspective)));
    THROW_IF_FAIL (m_priv);

    m_priv->vertical_paned.reset (new Gtk::VPaned);
    m_priv->horizontal_paned.reset (new Gtk::HPaned);
    m_priv->vertical_paned->set_position (350);
    m_priv->horizontal_paned->set_position (350);

    // set the position of the status pane to the last saved position
    IConfMgr &conf_mgr = m_priv->dbg_perspective.get_conf_mgr ();
    int vpane_location = -1;
    int hpane_location = -1;
    NEMIVER_TRY
    conf_mgr.get_key_value (CONF_KEY_TWO_PANE_LAYOUT_STATUS_VPANE_LOCATION,
                            vpane_location);
    conf_mgr.get_key_value (CONF_KEY_TWO_PANE_LAYOUT_STATUS_HPANE_LOCATION,
                            hpane_location);
    NEMIVER_CATCH

    if (vpane_location >= 0) {
        m_priv->vertical_paned->set_position (vpane_location);
    }

    if (hpane_location >= 0) {
        m_priv->horizontal_paned->set_position (hpane_location);
    }

    m_priv->horizontal_statuses_notebook.reset (new Gtk::Notebook);
    m_priv->horizontal_statuses_notebook->set_tab_pos (Gtk::POS_BOTTOM);

    m_priv->vertical_statuses_notebook.reset (new Gtk::Notebook);

    m_priv->vertical_paned->pack1 (*m_priv->horizontal_paned);
    m_priv->vertical_paned->pack2 (*m_priv->horizontal_statuses_notebook);

    m_priv->horizontal_paned->pack1
        (m_priv->dbg_perspective.get_source_view_widget (), true, true);
    m_priv->horizontal_paned->pack2 (*m_priv->vertical_statuses_notebook);

    int width=100, height=70;

    NEMIVER_TRY
    conf_mgr.get_key_value (CONF_KEY_STATUS_WIDGET_MINIMUM_WIDTH, width);
    conf_mgr.get_key_value (CONF_KEY_STATUS_WIDGET_MINIMUM_HEIGHT, height);
    NEMIVER_CATCH

    LOG_DD ("setting status widget min size: width: "
            << width
            << ", height: "
            << height);
    m_priv->horizontal_statuses_notebook->set_size_request (width, height);
    m_priv->vertical_statuses_notebook->set_size_request (height, width);
    m_priv->vertical_paned->show_all ();
}

void
DBGPerspectiveTwoPaneLayout::do_init ()
{
}

void
DBGPerspectiveTwoPaneLayout::do_cleanup_layout ()
{
    m_priv.reset ();
}

const UString&
DBGPerspectiveTwoPaneLayout::identifier () const
{
    static const UString s_id = "two-pane-layout";
    return s_id;
}

const UString&
DBGPerspectiveTwoPaneLayout::name () const
{
    static const UString s_name = _("Two Status Pane");
    return s_name;
}

const UString&
DBGPerspectiveTwoPaneLayout::description () const
{
    static const UString s_description = _("A layout with 2 status pane");
    return s_description;
}

void
DBGPerspectiveTwoPaneLayout::activate_view (int a_view)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (!m_priv->views.count (a_view));

    Gtk::Notebook &status_notebook = m_priv->statuses_notebook (a_view);
    int page_num = status_notebook.page_num (m_priv->views.at (a_view));
    THROW_IF_FAIL (page_num >= 0);
    status_notebook.set_current_page (page_num);
}

void
DBGPerspectiveTwoPaneLayout::save_configuration ()
{
    THROW_IF_FAIL (m_priv && m_priv->vertical_paned && m_priv->horizontal_paned);

    // save the location of the status pane so
    // that it'll open in the same place
    // next time.
    IConfMgr &conf_mgr = m_priv->dbg_perspective.get_conf_mgr ();
    int vpane_location = m_priv->vertical_paned->get_position ();
    int hpane_location = m_priv->horizontal_paned->get_position ();

    NEMIVER_TRY
    conf_mgr.set_key_value (CONF_KEY_TWO_PANE_LAYOUT_STATUS_VPANE_LOCATION,
                            vpane_location);
    conf_mgr.set_key_value (CONF_KEY_TWO_PANE_LAYOUT_STATUS_HPANE_LOCATION,
                            hpane_location);
    NEMIVER_CATCH
}

void
DBGPerspectiveTwoPaneLayout::add_view (Gtk::Widget &a_widget,
                                       const UString &a_title,
                                       int a_index)
{
    THROW_IF_FAIL (m_priv);
    if (m_priv->views.count (a_index) || a_widget.get_parent ()) {
        return;
    }

    m_priv->views.insert (std::make_pair<int, Gtk::Widget&> (a_index, a_widget));
    a_widget.show_all ();
    Gtk::Notebook &statuses_notebook = m_priv->statuses_notebook (a_index);
    int page_num = statuses_notebook.insert_page (a_widget, a_title, a_index);
    statuses_notebook.set_current_page (page_num);
}

void
DBGPerspectiveTwoPaneLayout::remove_view (int a_index)
{
    THROW_IF_FAIL (m_priv);
    if (!m_priv->views.count (a_index)) {
        return;
    }

    m_priv->statuses_notebook (a_index).remove_page (m_priv->views.at (a_index));
    m_priv->views.erase (a_index);
}

NEMIVER_END_NAMESPACE (nemiver)
