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

#include <glib/gi18n.h>
#include "nmv-vars-treeview.h"
#include "nmv-variables-utils.h"

namespace vutil = nemiver::variables_utils2;

NEMIVER_BEGIN_NAMESPACE (nemiver)

VarsTreeView*
VarsTreeView::create ()
{
    Glib::RefPtr<Gtk::TreeStore> model =
        Gtk::TreeStore::create (vutil::get_variable_columns ());
    THROW_IF_FAIL (model);
    return new VarsTreeView (model);
}

VarsTreeView::VarsTreeView (Glib::RefPtr<Gtk::TreeStore>& model) :
    Gtk::TreeView (model),
    m_tree_store (model)
{
    set_headers_clickable (true);
    get_selection ()->set_mode (Gtk::SELECTION_SINGLE);
    //create the columns of the tree view
    append_column (_("Variable"),
                   vutil::get_variable_columns ().name);
    Gtk::TreeViewColumn * col = get_column (VARIABLE_NAME_COLUMN_INDEX);
    THROW_IF_FAIL (col);
    col->set_resizable (true);
    col->add_attribute (*col->get_first_cell_renderer (),
                        "foreground-gdk",
                        vutil::VariableColumns::FG_COLOR_OFFSET);

    append_column (_("Value"), vutil::get_variable_columns ().value);
    col = get_column (VARIABLE_VALUE_COLUMN_INDEX);
    THROW_IF_FAIL (col);
    col->set_resizable (true);
    col->add_attribute (*col->get_first_cell_renderer (),
                        "foreground-gdk",
                        vutil::VariableColumns::FG_COLOR_OFFSET);
    col->add_attribute (*col->get_first_cell_renderer (),
                        "editable",
                        vutil::VariableColumns::VARIABLE_VALUE_EDITABLE_OFFSET);

    append_column (_("Type"),
                   vutil::get_variable_columns ().type_caption);
    col = get_column (VARIABLE_TYPE_COLUMN_INDEX);
    THROW_IF_FAIL (col);
    col->set_resizable (true);
}

Glib::RefPtr<Gtk::TreeStore>&
VarsTreeView::get_tree_store ()
{
    return m_tree_store;
}

NEMIVER_END_NAMESPACE (nemiver)

