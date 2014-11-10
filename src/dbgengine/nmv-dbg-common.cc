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
#include "common/nmv-exception.h"
#include "nmv-dbg-common.h"

NEMIVER_BEGIN_NAMESPACE (nemiver)

struct OutputHandlerList::Priv {
    list<OutputHandlerSafePtr> output_handlers;
};

OutputHandlerList::OutputHandlerList ()
{
    m_priv.reset (new OutputHandlerList::Priv);
}

OutputHandlerList::~OutputHandlerList ()
{
}

void
OutputHandlerList::add (const OutputHandlerSafePtr &a_handler)
{
    THROW_IF_FAIL (m_priv);
    m_priv->output_handlers.push_back (a_handler);
}

void
OutputHandlerList::submit_command_and_output (CommandAndOutput &a_cao)
{
    list<OutputHandlerSafePtr>::iterator iter;
    for (iter = m_priv->output_handlers.begin ();
            iter != m_priv->output_handlers.end ();
            ++iter)
    {
        if ((*iter)->can_handle (a_cao)) {
            NEMIVER_TRY;
            (*iter)->do_handle (a_cao);
            NEMIVER_CATCH_NOX;
        }
    }
}

/// Private stuff of the VarChange type.
struct VarChange::Priv {
    /// The variable this change is to be applied to.
    IDebugger::VariableSafePtr variable;
    int new_num_children;
    list<IDebugger::VariableSafePtr> new_children;

    Priv ()
        : new_num_children (-1)
    {
    }

    Priv (IDebugger::VariableSafePtr a_var,
          int a_new_num_children,
          list<IDebugger::VariableSafePtr> &a_new_children)
        : variable (a_var),
          new_num_children (a_new_num_children),
          new_children (a_new_children)
    {
    }
};

VarChange::VarChange ()
{
    m_priv.reset (new Priv);
}

VarChange::VarChange (IDebugger::VariableSafePtr a_var,
                      int new_num_children,
                      list<IDebugger::VariableSafePtr> &a_changed_children)
{
    m_priv.reset (new Priv (a_var, new_num_children,
                            a_changed_children));
}

/// Returns the variable this change is to be applied to.  The
/// returned variable contains the new value(s) as a result member
/// value change.
const IDebugger::VariableSafePtr
VarChange::variable () const
{
    return m_priv->variable;
}

/// Setter of the variable the current change is to be applied to.
void
VarChange::variable (const IDebugger::VariableSafePtr a_var)
{
    m_priv->variable = a_var;
}

/// If the change encompasses new children sub variables, then this
/// accessor returns a positive value.  If the change is about
/// removing all the children sub variables then this accessor returns
/// zero.  Otherwise, if no children got added or removed the this
/// returns -1.
int
VarChange::new_num_children () const
{
    return m_priv->new_num_children;
}

void
VarChange::new_num_children (int a)
{
    m_priv->new_num_children = a;
}

/// If new_num_children() returned a positive number then this
/// accessor returns the list of new children variables.
const list<IDebugger::VariableSafePtr>&
VarChange::new_children () const
{
    return m_priv->new_children;
}

list<IDebugger::VariableSafePtr>&
VarChange::new_children ()
{
    return m_priv->new_children;
}

void
VarChange::new_children (const list<IDebugger::VariableSafePtr>& a_vars)
{
    m_priv->new_children = a_vars;
}

/// Apply current change to a given variable.
void
VarChange::apply_to_variable (IDebugger::VariableSafePtr a_var,
                              list<IDebugger::VariableSafePtr> &a_changed_vars)
{
    IDebugger::VariableSafePtr applied_to;

    IDebugger::VariableSafePtr v;
    if (*a_var == *variable ()) {
        applied_to = a_var;
    } else {
        // variable must be a descendant of a_var.
        v = a_var->get_descendant (variable ()->internal_name ());
        THROW_IF_FAIL (v);
        applied_to = v;
    }
    update_debugger_variable (*applied_to, *variable ());

    a_changed_vars.push_back (applied_to);

    if (new_num_children () > (int) a_var->members ().size ()) {
        // There are new children
        THROW_IF_FAIL (new_children ().size () > 0);
        list<IDebugger::VariableSafePtr>::const_iterator i;
        for (i = new_children ().begin ();
             i != new_children ().end ();
             ++i) {
            v = a_var->get_descendant ((*i)->internal_name ());
            THROW_IF_FAIL (!v);
            // *i is a new child of variable.  Append it now.
            applied_to->append (*i);
            a_changed_vars.push_back (*i);
        }
    } else if (new_num_children  () > -1
               && (unsigned) new_num_children () < a_var->members ().size ()) {
        // Some children got removed from applied_to.  We assume these
        // got removed from the end of the children list, b/c
        // conceptually, if a variable get removedfrom the middle of
        // the children list, it must be reported as two pieces of
        // change:
        // 
        //    1/ a set of changes to the values of all the variables
        //      that have indexes greater of equal to the variable
        //      that got removed.
        //
        //    2/ The former last child element is now going to be
        //       reported as a child that got removed from the end of
        //       the children list.
        // So we assume we are handling the event 2/ now.
        list<IDebugger::VariableSafePtr>::iterator last =
            applied_to->members ().end ();
        int num_to_erase = applied_to->members ().size () - new_num_children ();
        for (--last; num_to_erase; --num_to_erase, --last) {
            applied_to->members ().erase (last);
            last = a_var->members ().end ();
        }
    } else {
        THROW_IF_FAIL (new_children ().empty ());
    }
}

/// Update variable a_to with new bits from a_from.  Note that only
/// things that can reasonably change are updated here.
void
update_debugger_variable (IDebugger::Variable &a_to,
                          IDebugger::Variable &a_from)
{
    if (!a_from.value ().empty ())
        a_to.value (a_from.value ());
    if (!a_from.type ().empty ())
        a_to.value (a_to.type ());
    a_to.has_more_children (a_from.has_more_children ());
    a_to.in_scope (a_from.in_scope ());
    a_to.is_dynamic (a_from.is_dynamic ());
    // If a_from is the result of -var-update --all-values, then it
    // won't have the user visible name of the variable set (it will
    // only have the varobj name aka internal_name set).  In that
    // case, update the user visible name of a_from, if we have it.
    if (a_from.name ().empty () && !a_to.name ().empty ())
        a_from.name (a_to.name ());
}



NEMIVER_END_NAMESPACE (nemiver)

