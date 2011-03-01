//Author: Dodji Seketeli
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
#ifndef __NMV_VARIABLES_UTILS_H__
#define __NMV_VARIABLES_UTILS_H__

#include <list>
#include <gtkmm/treeview.h>
#include <gtkmm/treestore.h>
#include <gtkmm/treemodelcolumn.h>
#include <gdkmm/color.h>
#include "common/nmv-ustring.h"
#include "nmv-i-debugger.h"

NEMIVER_BEGIN_NAMESPACE (nemiver)
NEMIVER_BEGIN_NAMESPACE (variables_utils2)

struct VariableColumns : public Gtk::TreeModelColumnRecord {
    enum Offset {
        NAME_OFFSET=0,
        VALUE_OFFSET,
        TYPE_OFFSET,
        TYPE_CAPTION_OFFSET,
        VARIABLE_OFFSET,
        IS_HIGHLIGHTED_OFFSET,
        NEEDS_UNFOLDING,
        FG_COLOR_OFFSET,
        VARIABLE_VALUE_EDITABLE_OFFSET
    };

    Gtk::TreeModelColumn<Glib::ustring> name;
    Gtk::TreeModelColumn<Glib::ustring> value;
    Gtk::TreeModelColumn<Glib::ustring> type;
    Gtk::TreeModelColumn<Glib::ustring> type_caption;
    Gtk::TreeModelColumn<IDebugger::VariableSafePtr> variable;
    Gtk::TreeModelColumn<bool> is_highlighted;
    Gtk::TreeModelColumn<bool> needs_unfolding;
    Gtk::TreeModelColumn<Gdk::Color> fg_color;
    Gtk::TreeModelColumn<bool> variable_value_editable;

    VariableColumns ()
    {
        add (name);
        add (value);
        add (type);
        add (type_caption);
        add (variable);
        add (is_highlighted);
        add (needs_unfolding);
        add (fg_color);
        add (variable_value_editable);
    }
};//end VariableColumns

VariableColumns& get_variable_columns ();

bool is_type_a_pointer (const UString &a_type);

void set_a_variable_node_type (Gtk::TreeModel::iterator &a_var_it,
                               const UString &a_type,
                               bool a_truncate);

void update_a_variable_node (const IDebugger::VariableSafePtr a_var,
                             const Gtk::TreeView &a_tree_view,
                             Gtk::TreeModel::iterator &a_iter,
                             bool a_truncate_type,
                             bool a_handle_highlight,
                             bool a_is_new_frame);

void update_unfolded_variable (const IDebugger::VariableSafePtr a_var,
                               const Gtk::TreeView &a_tree_view,
                               const Glib::RefPtr<Gtk::TreeStore> &a_tree_store,
                               Gtk::TreeModel::iterator a_var_it,
                               bool a_truncate_type);

bool find_a_variable (const IDebugger::VariableSafePtr a_var,
                      const Gtk::TreeModel::iterator &a_parent_row_it,
                      Gtk::TreeModel::iterator &a_out_row_it);

bool variables_match (const IDebugger::VariableSafePtr &a_var,
                      const Gtk::TreeModel::iterator a_row_it);

bool find_a_variable_descendent (const IDebugger::VariableSafePtr a_var,
                                 const Gtk::TreeModel::iterator &a_parent_row_it,
                                 Gtk::TreeModel::iterator &a_out_row_it);

bool update_a_variable (const IDebugger::VariableSafePtr a_var,
                        const Gtk::TreeView &a_tree_view,
                        Gtk::TreeModel::iterator &a_parent_row_it,
                        bool a_truncate_type,
                        bool a_handle_highlight,
                        bool a_is_new_frame,
                        bool a_update_members = false);

bool append_a_variable (const IDebugger::VariableSafePtr a_var,
                        const Gtk::TreeView &a_tree_view,
                        const Glib::RefPtr<Gtk::TreeStore> &a_tree_store,
                        Gtk::TreeModel::iterator &a_parent_row_it,
                        bool a_truncate_type);

bool append_a_variable (const IDebugger::VariableSafePtr a_var,
                        const Gtk::TreeView &a_tree_view,
                        const Glib::RefPtr<Gtk::TreeStore> &a_tree_store,
                        Gtk::TreeModel::iterator &a_parent_row_it,
                        Gtk::TreeModel::iterator &a_result,
                        bool a_truncate_type);

bool set_a_variable (const IDebugger::VariableSafePtr a_var,
		     const Gtk::TreeView &a_tree_view,
		     const Glib::RefPtr<Gtk::TreeStore> &a_tree_store,
		     Gtk::TreeModel::iterator a_row_it,
		     bool a_truncate_type);

bool unlink_member_variable_rows (const Gtk::TreeModel::iterator &a_row_it,
				  const Glib::RefPtr<Gtk::TreeStore> &a_store);

bool visualize_a_variable (const IDebugger::VariableSafePtr a_var,
			   const Gtk::TreeModel::iterator &a_var_row_it,
			   const Gtk::TreeView &a_tree_view,
			   const Glib::RefPtr<Gtk::TreeStore> &a_store);

NEMIVER_END_NAMESPACE (variables_utils2)
NEMIVER_END_NAMESPACE (nemiver)

#endif //__NMV_VARIABLES_UTILS_H__

