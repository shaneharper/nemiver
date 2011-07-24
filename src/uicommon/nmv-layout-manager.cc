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

#include <gtkmm/widget.h>
#include "nmv-layout-manager.h"
#include "nmv-layout.h"
#include "persp/nmv-i-perspective.h"
#include "common/nmv-exception.h"

using namespace std;
using namespace nemiver::common;

NEMIVER_BEGIN_NAMESPACE (nemiver)

/// \brief Manages Layouts
///
/// The LayoutManager is used to load/unload layout inside a IPerspective.
struct LayoutManager::Priv {
    map<UString, LayoutSafePtr> layouts_map;
    Layout *layout;
    sigc::signal<void> layout_changed_signal;

    Priv () :
        layout (0)
    {
    }
};

LayoutManager::LayoutManager () :
    m_priv (new Priv)
{
}

LayoutManager::~LayoutManager ()
{
    LOG_D ("deleted", "destructor-domain");
}

/// \brief Register a layout inside the LayoutManager
///
/// A Layout registered can be loaded and unloaded by the layout manager.
/// This layout will also be displayed in the Layout Selector to let the user
/// use it.
///
/// \param a_layout Layout to register
void
LayoutManager::register_layout (const LayoutSafePtr &a_layout)
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (a_layout);

    UString layout_identifier = a_layout->identifier ();
    THROW_IF_FAIL (!m_priv->layouts_map.count (layout_identifier));

    m_priv->layouts_map[layout_identifier] =  a_layout;
}

/// \brief Load a layout.
///
/// Unload the previous layout and load a new layout inside
/// the perspective.
///
/// \param a_layout Identifier of the layout to load
/// \param a_perspective Perspective loading the layout
void
LayoutManager::load_layout (const UString &a_identifier,
                            IPerspective &a_perspective)
{
    THROW_IF_FAIL (m_priv);

    if (!is_layout_registered (a_identifier)) {
        LOG_ERROR ("Trying to load a unregistered layout with the identifier: "
                   << a_identifier);
        return;
    }

    if (m_priv->layout) {
        m_priv->layout->save_configuration ();
        m_priv->layout->do_cleanup_layout ();
    }

    m_priv->layout = m_priv->layouts_map[a_identifier].get ();
    THROW_IF_FAIL (m_priv->layout);

    m_priv->layout->do_lay_out (a_perspective);
    m_priv->layout_changed_signal.emit ();
}

/// \brief emited when the layout is changed
sigc::signal<void>&
LayoutManager::layout_changed_signal () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->layout_changed_signal;
}

/// \brief check if the given layout had been registered in the
/// layout manager.
///
/// \param a_layout_identifier Identifier of the layout
/// \return true is the layout is registered in the LayoutManager
bool
LayoutManager::is_layout_registered (const UString &a_layout_identifier) const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->layouts_map.count (a_layout_identifier);
}

/// \brief gets the loaded layout
/// \return loaded layout
Layout*
LayoutManager::layout () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->layout;
}

/// \brief gets the layouts registered
/// \return layouts registered
std::vector<Layout*>
LayoutManager::layouts () const
{
    THROW_IF_FAIL (m_priv);

    std::vector<Layout*> layouts;
    for (map<UString, LayoutSafePtr>::const_iterator i = m_priv->layouts_map.begin ();
         i != m_priv->layouts_map.end ();
         ++i) {
        layouts.push_back (i->second.get ());
    }

    return layouts;
}

NEMIVER_END_NAMESPACE (nemiver)
