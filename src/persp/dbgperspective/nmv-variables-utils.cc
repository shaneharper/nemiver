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

#include "config.h"

#include "nmv-variables-utils.h"
#include "common/nmv-exception.h"
#include "nmv-ui-utils.h"

NEMIVER_BEGIN_NAMESPACE (nemiver)
NEMIVER_BEGIN_NAMESPACE (variables_utils2)

static void update_a_variable_real (const IDebugger::VariableSafePtr a_var,
                                    const Gtk::TreeView &a_tree_view,
                                    Gtk::TreeModel::iterator &a_row_it,
                                    bool a_truncate_type,
                                    bool a_handle_highlight,
                                    bool a_is_new_frame,
                                    bool a_update_members);

VariableColumns&
get_variable_columns ()
{
    static VariableColumns s_cols;
    return s_cols;
}

bool
is_type_a_pointer (const UString &a_type)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    LOG_DD ("type: '" << a_type << "'");

    UString type (a_type);
    type.chomp ();
    if (type[type.size () - 1] == '*') {
        LOG_DD ("type is a pointer");
        return true;
    }
    if (type.size () < 8) {
        LOG_DD ("type is not a pointer");
        return false;
    }
    UString::size_type i = type.size () - 7;
    if (!a_type.compare (i, 7, "* const")) {
        LOG_DD ("type is a pointer");
        return true;
    }
    LOG_DD ("type is not a pointer");
    return false;
}

/// Populate the type information of the graphical node representing
/// a variable.
/// \param a_var_it the iterator to the graphical node representing the
/// variable we want to set the type for
/// \param a_type the type string to set
/// \param a_truncate when true, if the type exceeds a certain length,
/// truncate it. Do not truncate the variable otherwise. In any case, in
/// presence of a multi-line type string the type is _always_ truncated to the
/// first line.
void
set_a_variable_node_type (Gtk::TreeModel::iterator &a_var_it,
                          const UString &a_type,
                          bool a_truncate)
{
    THROW_IF_FAIL (a_var_it);
    a_var_it->set_value (get_variable_columns ().type,
                         (Glib::ustring)a_type);
    int nb_lines = a_type.get_number_of_lines ();
    UString type_caption = a_type;
    if (nb_lines) {--nb_lines;}

    UString::size_type truncation_index = 0;
    static const UString::size_type MAX_TYPE_STRING_LENGTH = 50;
    if (nb_lines) {
        truncation_index = a_type.find ('\n');
    } else if (a_truncate
               && (a_type.size () > MAX_TYPE_STRING_LENGTH)) {
        truncation_index = MAX_TYPE_STRING_LENGTH;
    }
    if (truncation_index) {
        type_caption.erase (truncation_index);
        type_caption += "...";
    }

    a_var_it->set_value (get_variable_columns ().type_caption,
                         (Glib::ustring)type_caption);
    IDebugger::VariableSafePtr variable =
        (IDebugger::VariableSafePtr) a_var_it->get_value
                                        (get_variable_columns ().variable);
    THROW_IF_FAIL (variable);
    variable->type (a_type);
}

// Graphically update the value representation of a variable.
// This updates things like the variable name, value and color (in case we
// have to highlight the variable because it value changed in the current
// function).
// \param a_var the symbolic representation of the variable
// \param a_tree_view the widget containing the graphical representation of
// a_var.
// \param a_iter the iterator pointing to the graphical node representing
// a_var.
// \param a_handle_highlight whether to highlight the variable in case its
// new value is different from the previous one.
// \param a_is_new_frame if true, the variable will not be highlighted.
// otherwise, highligthing will be considered if the new variable value is
// different from the previous one.
void
update_a_variable_node (const IDebugger::VariableSafePtr a_var,
                        const Gtk::TreeView &a_tree_view,
                        Gtk::TreeModel::iterator &a_iter,
                        bool a_truncate_type,
                        bool a_handle_highlight,
                        bool a_is_new_frame)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    if (a_var) {
        LOG_DD ("going to really update variable '"
                << a_var->name ()
                << "'");
    } else {
        LOG_DD ("eek, got null variable");
        return;
    }

    (*a_iter)[get_variable_columns ().variable] = a_var;
    UString var_name = a_var->name_caption ();
    if (var_name.empty ()) {var_name = a_var->name ();}
    var_name.chomp ();
    UString prev_var_name =
            (Glib::ustring)(*a_iter)[get_variable_columns ().name];
    LOG_DD ("Prev variable name: " << prev_var_name);
    LOG_DD ("new variable name: " << var_name);
    LOG_DD ("Didn't update variable name");
    if (prev_var_name.raw () == "") {
        (*a_iter)[get_variable_columns ().name] = var_name;
    }
    (*a_iter) [get_variable_columns ().is_highlighted] = false;
    bool do_highlight = false;
    if (a_handle_highlight && !a_is_new_frame) {
        UString prev_value =
            (UString) (*a_iter)[get_variable_columns ().value];
        if (prev_value != a_var->value ()) {
            do_highlight = true;
        }
    }

    if (do_highlight) {
        LOG_DD ("do highlight variable");
        (*a_iter)[get_variable_columns ().is_highlighted]=true;
        (*a_iter)[get_variable_columns ().fg_color] = Gdk::Color ("red");
    } else {
        LOG_DD ("remove highlight from variable");
        (*a_iter)[get_variable_columns ().is_highlighted]=false;
        (*a_iter)[get_variable_columns ().fg_color]  =
            a_tree_view.get_style ()->get_text (Gtk::STATE_NORMAL);
    }

    (*a_iter)[get_variable_columns ().value] = a_var->value ();
    set_a_variable_node_type (a_iter,  a_var->type (), a_truncate_type);
}


/// Update a graphical variable to make it show the new graphical children
/// nodes representing the new children of a variable.
/// \a_var the variable that got unfolded
/// \a_tree_view the tree view in which a_var is represented
/// \a_tree_store the tree store in which a_var is represented
/// \a_var_it the graphical node of the variable that got unfolded.
/// So what happened is that a_var got unfolded.
/// a_var is bound to the graphical node pointed to by a_var_it.
/// This function then updates a_var_it to make it show new graphical
/// nodes representing the new children of a_variable.
void
update_unfolded_variable (const IDebugger::VariableSafePtr a_var,
                          const Gtk::TreeView &a_tree_view,
                          const Glib::RefPtr<Gtk::TreeStore> &a_tree_store,
                          Gtk::TreeModel::iterator a_var_it,
                          bool a_truncate_type)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    Gtk::TreeModel::iterator result_var_row_it;
    IDebugger::VariableList::const_iterator var_it;
    IDebugger::VariableList::const_iterator member_it;
    for (member_it = a_var->members ().begin ();
         member_it != a_var->members ().end ();
         ++member_it) {
        append_a_variable (*member_it,
                           a_tree_view,
                           a_tree_store,
                           a_var_it,
                           result_var_row_it,
                           a_truncate_type);
    }
}

/// Finds a variable in the tree view of variables.
/// All the members of the variable are considered
/// during the search.
/// \param a_var the variable to find in the tree view
/// \param a_parent_row_it the graphical row where to start
//   the search from. This function actually starts looking
//   at the chilren rows of this parent row.
/// \param a_out_row_it the row of the search. This is set
/// if and only if the function returns true.
/// \return true upon successful completion, false otherwise.
bool
find_a_variable (const IDebugger::VariableSafePtr a_var,
                 const Gtk::TreeModel::iterator &a_parent_row_it,
                 Gtk::TreeModel::iterator &a_out_row_it)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    if (!a_var) {
        LOG_DD ("got null var, returning false");
        return false;
    }

    Gtk::TreeModel::iterator row_it;
    IDebugger::VariableSafePtr var;
    for (row_it = a_parent_row_it->children ().begin ();
         row_it != a_parent_row_it->children ().end ();
         ++row_it) {
        var = row_it->get_value (get_variable_columns ().variable);
        if (variables_match (a_var, row_it)) {
            a_out_row_it = row_it;
            LOG_DD ("found variable");
            return true;
        }
    }
    LOG_DD ("didn't find variable " << a_var->name ());
    return false;
}

/// Generates a path that addresses a variable descendant
/// from its root ancestor.
/// \param a_var the variable descendent to address
/// \param the resulting path. It's actually a list of index.
/// Each index is the sibling index of a given level.
/// e.g. consider the path  [0 1 4].
/// It means a_var is the 5th sibbling child of the 2st sibling child
/// of the 1nd sibling child of the root ancestor variable.
/// \return true if a path was generated, false otherwise.
static bool
generate_path_to_descendent (IDebugger::VariableSafePtr a_var,
                             list<int> &a_path)
{
    if (!a_var)
        return false;
    a_path.push_front (a_var->sibling_index ());
    if (a_var->parent ())
        return generate_path_to_descendent (a_var->parent (), a_path);
    return true;
}

/// Walk a "path to descendent" as the one returned by
/// generate_path_to_descendent.
/// The result of the walk is to find the descendent variable member
/// addressed by the path.
/// \param a_from the graphical row
///  (containing the root variable ancestor) to walk from.
/// \param a_path_start an iterator that points to the beginning of the
///  path
/// \param a_path_end an iterator that points to the end of the path
/// \param a_to the resulting row, if any. This points to the variable
///  addressed by the path. This parameter is set if and only if the
///  function returned true.
/// \return true upon succesful completion, false otherwise.
static bool
walk_path_from_row (const Gtk::TreeModel::iterator &a_from,
                    const list<int>::const_iterator &a_path_start,
                    const list<int>::const_iterator &a_path_end,
                    Gtk::TreeModel::iterator &a_to)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    if (a_path_start == a_path_end) {
        if (a_from->parent ()) {
            a_to = a_from->parent ();
            return true;
        }
        return false;
    }

    Gtk::TreeModel::iterator row = a_from;
    for (int steps = 0;
         steps  < *a_path_start && row;
         ++steps, ++row) {
        // stepping at the current level;
    }

    if (!row)
        // we reached the end of the current level. That means the path
        // was not well suited for this variable tree view. Bail out.
        return false;

    // Dive down one level.
    list<int>::const_iterator from = a_path_start;
    from++;
    return walk_path_from_row (row->children ().begin (),
                               from, a_path_end, a_to);
}

/// Find a member variable that is a descendent of a root ancestor variable
/// contained in a tree view. This function must actually find the row of the
/// root ancestor variable in the tree view, and then find the row of
/// the descendent.
/// \param a_descendent the descendent to search for.
/// \param a_parent_row the graphical row from where to start the search.
/// \param a_out_row the result of the find, if any. This is set if and
/// only if the function returns true.
/// \return true upon successful completion, false otherwise.
bool
find_a_variable_descendent (const IDebugger::VariableSafePtr a_descendent,
                            const Gtk::TreeModel::iterator &a_parent_row,
                            Gtk::TreeModel::iterator &a_out_row)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    if (!a_descendent) {
        LOG_DD ("got null variable, returning false");
        return false;
    }

    // first, find the root variable the descendant belongs to.
    IDebugger::VariableSafePtr root_var = a_descendent->root ();
    THROW_IF_FAIL (root_var);
    Gtk::TreeModel::iterator root_var_row;
    if (!find_a_variable (root_var, a_parent_row, root_var_row)) {
        LOG_DD ("didn't find root variable " << root_var->name ());
        return false;
    }

    // Now that we have the row of the root variable, look for the
    // row of the descendent there.
    // For that, let's get the path to that damn descendent first.
    list<int> path;
    generate_path_to_descendent (a_descendent, path);

    // let's walk the path from the root variable down to the descendent
    // now.
    if (!walk_path_from_row (root_var_row, path.begin (),
                             path.end (), a_out_row)) {
        THROW ("fatal: should not be reached");
    }
    return true;
}

/// Tests if a variable matches the variable present on
/// a tree view node.
/// \param a_var the variable to consider
/// \param a_row_it the tree view to test against.
/// \return true if the a_row_it contains a variable that is
/// "value-equal" to a_var.
bool
variables_match (const IDebugger::VariableSafePtr &a_var,
                 const Gtk::TreeModel::iterator a_row_it)
{
    IDebugger::VariableSafePtr var =
        a_row_it->get_value (get_variable_columns ().variable);
    if (a_var == var)
        return true;
    if (!var || !a_var)
        return false;
    if (a_var->internal_name () == var->internal_name ())
        return true;

    return var->equals_by_value (*a_var);
}

// Update the graphical representation of variable a_var.
// \param a_var is the symbolic representation of the variable.
// \param a_tree_view the widget containing the grahical representation of
// a_var.
// \param a_parent_row_it an iterator to a graphical ancestor of the
// graphical representation of a_var.
// \param a_handle_highlight whether to handle highlighting of the variable
// or not.
// \param a_is_new_frame must be set to true if we just entered the frame
// containing the a_var, false otherwise.
// \param a_update_members whether to recursively update the graphical
// representations of the members a_var.
bool
update_a_variable (const IDebugger::VariableSafePtr a_var,
                   const Gtk::TreeView &a_tree_view,
                   Gtk::TreeModel::iterator &a_parent_row_it,
                   bool a_truncate_type,
                   bool a_handle_highlight,
                   bool a_is_new_frame,
                   bool a_update_members)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (a_parent_row_it);

    Gtk::TreeModel::iterator row_it;
    bool found_variable = find_a_variable_descendent (a_var,
                                                      a_parent_row_it,
                                                      row_it);

    if (!found_variable) {
        LOG_ERROR ("could not find variable in inspector: "
                   + a_var->name ());
        return false;
    }

    update_a_variable_real (a_var, a_tree_view,
                            row_it, a_truncate_type, a_handle_highlight,
                            a_is_new_frame, a_update_members);
    return true;
}

// Subroutine of update_a_variable. See that function comments to learn
// more.
// \param a_var the symbolic representation of the variable we are interested in.
// \param a_tree_view the widget containing the graphical representation of
// a_var.
// \param a_row_it an iterator pointing to the graphical representation of
// a_var
// \param a_handle_highlight whether to highlight the node pointed to by
// a_row_it or not.
// \param a_is_new_frame whether if we just entered the function frame
// containing a_var or not.
// \param a_update_members whether the function should recursively update
// the graphical representation of the members of a_var.
static void
update_a_variable_real (const IDebugger::VariableSafePtr a_var,
                        const Gtk::TreeView &a_tree_view,
                        Gtk::TreeModel::iterator &a_row_it,
                        bool a_truncate_type,
                        bool a_handle_highlight,
                        bool a_is_new_frame,
                        bool a_update_members)
{
    update_a_variable_node (a_var,
                            a_tree_view,
                            a_row_it,
                            a_truncate_type,
                            a_handle_highlight,
                            a_is_new_frame);
    Gtk::TreeModel::iterator row_it;
    list<IDebugger::VariableSafePtr>::const_iterator var_it;
    Gtk::TreeModel::Children rows = a_row_it->children ();
    if (a_update_members) {
        for (row_it = rows.begin (), var_it = a_var->members ().begin ();
             row_it != rows.end () && var_it != a_var->members ().end ();
             ++row_it, ++var_it) {
            update_a_variable_real (*var_it, a_tree_view,
                                    row_it, a_truncate_type,
                                    a_handle_highlight,
                                    a_is_new_frame,
                                    true /* update members */);
        }
    }
}

/// Append a variable to a variable tree view widget.
///
/// \param a_var the variable to add
/// \param a_tree_view the variable tree view widget to consider
/// \param a_tree_store the tree store of the variable tree view widget
/// \param a_parent_row_it an iterator to the graphical parent node the
/// the variable is to be added to. If the iterator is false, then the
/// variable is added as the root node of the tree view widget.
/// \return true if a_var was added, false otherwise.
bool
append_a_variable (const IDebugger::VariableSafePtr a_var,
                   const Gtk::TreeView &a_tree_view,
                   const Glib::RefPtr<Gtk::TreeStore> &a_tree_store,
                   Gtk::TreeModel::iterator &a_parent_row_it,
                   bool a_truncate_type)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    Gtk::TreeModel::iterator row_it;
    return append_a_variable (a_var, a_tree_view, a_tree_store,
                              a_parent_row_it, row_it, a_truncate_type);
}

/// Append a variable to a variable tree view widget.
///
/// \param a_var the variable to add. It can be zero. In that case,
/// a dummy (empty) node is added as a graphical child of a_parent_row_it.
/// \param a_tree_view the variable tree view widget to consider
/// \param a_tree_store the tree store of the variable tree view widget
/// \param a_parent_row_it an iterator to the graphical parent node the
/// the variable is to be added to. If the iterator is false, then the
/// variable is added as the root node of the tree view widget.
/// \param result the resulting graphical node that was created an added
/// to the variable tree view widget. This parameter is set if and only if
/// the function returned true.
/// \return true if a_var was added, false otherwise.
bool
append_a_variable (const IDebugger::VariableSafePtr a_var,
                   const Gtk::TreeView &a_tree_view,
                   const Glib::RefPtr<Gtk::TreeStore> &a_tree_store,
                   Gtk::TreeModel::iterator &a_parent_row_it,
                   Gtk::TreeModel::iterator &a_result,
                   bool a_truncate_type)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (a_tree_store);

    Gtk::TreeModel::iterator row_it;
    if (!a_parent_row_it) {
        row_it = a_tree_store->append ();
    } else {
        if (a_parent_row_it->children ()
            && a_var
            && (*a_parent_row_it)[get_variable_columns ().needs_unfolding]){
            // So a_parent_row_it might have dummy empty nodes as children.
            // Remove those, so that that we can properly add a_var as a
            // child node of a_parent_row_it. Then, don't forget to
            // set get_variable_columns ().needs_unfolding to false.
            Gtk::TreeModel::Children::const_iterator it;
            for (it = a_parent_row_it->children ().begin ();
                 it != a_parent_row_it->children ().end ();) {
                it = a_tree_store->erase (it);
            }
            (*a_parent_row_it)[get_variable_columns ().needs_unfolding]
                                                                        = false;
        }
        row_it = a_tree_store->append (a_parent_row_it->children ());
    }
    if (!a_var) {
        return false;
    }
    if (!set_a_variable (a_var, a_tree_view, a_tree_store,
                         row_it, a_truncate_type))
        return false;
    a_result = row_it;
    return true;
}

/// (Re-)render a variable into the node holding its graphical
/// representation.
///
/// \param a_var the variable to render
///
/// \param a_tree_view the treeview containing the graphical
/// representation to set.
///
/// \param a_tree_store the treestore of the treeview.
///
/// \param a_row_it an iterator to the row of the graphical
/// representation of the variable.
///
/// \param a_truncate_type if set to TRUE, the string representing the
/// type is going to be truncated if it is too long.
///
/// \return TRUE upon successful completion
bool
set_a_variable (const IDebugger::VariableSafePtr a_var,
                const Gtk::TreeView &a_tree_view,
                const Glib::RefPtr<Gtk::TreeStore> &a_tree_store,
                Gtk::TreeModel::iterator a_row_it,
                bool a_truncate_type)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (a_tree_store);

    if (!a_var) {
        return false;
    }

    update_a_variable_node (a_var, a_tree_view, a_row_it,
                            a_truncate_type, true, true);

    list<IDebugger::VariableSafePtr>::const_iterator it;
    if (a_var->needs_unfolding ()) {
        // Mark *row_it as needing unfolding, and add an empty
        // child node to it
        (*a_row_it)[get_variable_columns ().needs_unfolding] = true;
        IDebugger::VariableSafePtr empty_var;
        append_a_variable (empty_var, a_tree_view,
                           a_tree_store, a_row_it,
                           a_truncate_type);
    } else {
        for (it = a_var->members ().begin ();
             it != a_var->members ().end ();
             ++it) {
            append_a_variable (*it, a_tree_view,
                               a_tree_store, a_row_it,
                               a_truncate_type);
        }
    }
    return true;
}

/// Unlink the graphical nodes representing the member variables of
/// the variable pointed to by the given row iterator.
///
/// \param a_row_it the iterator pointing at the graphical node which
/// containing the variable which member variables we want to unlink.
///
/// \param a_store the treestore containing a_row_it.
bool
unlink_member_variable_rows (const Gtk::TreeModel::iterator &a_row_it,
                             const Glib::RefPtr<Gtk::TreeStore> &a_store)
{
    IDebugger::VariableSafePtr var;

    var = a_row_it->get_value (get_variable_columns ().variable);
    if (!var)
        return false;

    vector<Gtk::TreePath> paths;
    for (Gtk::TreeModel::iterator it = a_row_it->children ().begin ();
	 it != a_row_it->children ().end ();
	 ++it) {
	var = it->get_value (get_variable_columns ().variable);
	if (var)
	    paths.push_back (a_store->get_path (it));
    }
    for (int i = paths.size (); i > 0; --i) {
	Gtk::TreeIter it = a_store->get_iter (paths[i - 1]);
        IDebugger::VariableSafePtr empty_var;
        (*it)->get_value(get_variable_columns ().variable).reset ();
	a_store->erase (it);
    }
    return true;
}

/// Re-visualize a given variable.  That is, unlink the graphical
/// nodes of the member variables of the given variable, and
/// re-visualize that same variable into its graphical node.
///
/// \param a_var the variable to re-visualize
/// 
/// \param a_row_it an iterator to the graphical node of the variable
/// to re-visualize
///
/// \param a_tree_view the treeview containing the graphical node
///
/// \param a_store the tree store containing the graphical node
bool
visualize_a_variable (const IDebugger::VariableSafePtr a_var,
		      const Gtk::TreeModel::iterator &a_row_it,
                      const Gtk::TreeView &a_tree_view,
		      const Glib::RefPtr<Gtk::TreeStore> &a_store)
{
    if (!unlink_member_variable_rows (a_row_it, a_store))
        return false;

    return set_a_variable (a_var, a_tree_view,
                           a_store, a_row_it,
                           /*a_truncate_type=*/true);
}

NEMIVER_END_NAMESPACE (variables_utils2)
NEMIVER_END_NAMESPACE (nemiver)

