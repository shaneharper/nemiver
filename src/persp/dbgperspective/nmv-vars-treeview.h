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
#ifndef __NMV_VARS_TREEVIEW_H__
#define __NMV_VARS_TREEVIEW_H__

#include <gtkmm/treeview.h>
#include <gtkmm/treestore.h>
#include "common/nmv-safe-ptr.h"
#include "nmv-ui-utils.h"

using nemiver::common::SafePtr;

NEMIVER_BEGIN_NAMESPACE (nemiver)
class VarsTreeView;

typedef SafePtr<VarsTreeView,
                nemiver::ui_utils::WidgetRef,
                nemiver::ui_utils::WidgetUnref> VarsTreeViewSafePtr;

class NEMIVER_API VarsTreeView : public Gtk::TreeView
{
    public:
        enum ColumIndex {
            VARIABLE_NAME_COLUMN_INDEX = 0,
            VARIABLE_VALUE_COLUMN_INDEX,
            VARIABLE_TYPE_COLUMN_INDEX
        };
        static VarsTreeView* create ();
        Glib::RefPtr<Gtk::TreeStore>& get_tree_store ();

    protected:
        VarsTreeView ();
        VarsTreeView (Glib::RefPtr<Gtk::TreeStore>& model);

    private:
        Glib::RefPtr<Gtk::TreeStore> m_tree_store;
};
NEMIVER_END_NAMESPACE (nemiver)
#endif // __NMV_VARS_TREEVIEW_H__
