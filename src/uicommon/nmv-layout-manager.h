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
#ifndef __NMV_LAYOUT_MANAGER_H__
#define __NMV_LAYOUT_MANAGER_H__

#include "common/nmv-safe-ptr-utils.h"
#include "common/nmv-ustring.h"
#include "nmv-layout.h"
#include <vector>

NEMIVER_BEGIN_NAMESPACE (nemiver)

using nemiver::common::SafePtr;
using nemiver::common::UString;

class Layout;
class IPerspective;

class LayoutManager {
    //non copyable
    LayoutManager (const LayoutManager&);
    LayoutManager& operator= (const LayoutManager&);

    struct Priv;
    SafePtr<Priv> m_priv;

public:
    LayoutManager ();

    void register_layout (const LayoutSafePtr &a_layout);

    void load_layout (const UString &a_layout, IPerspective &a_perspective);

    Layout* layout () const;

    std::vector<Layout*> layouts () const;

    bool is_layout_registered (const UString &a_layout_identifier) const;

    /// \name signals
    /// @{

    sigc::signal<void>& layout_changed_signal () const;

    /// @}

    virtual ~LayoutManager ();
};

NEMIVER_END_NAMESPACE (nemiver)

#endif //__NMV_LAYOUT_MANAGER_H__
