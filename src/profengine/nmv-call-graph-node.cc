// Author: Fabien Parent
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

#include "nmv-call-graph-node.h"
#include "common/nmv-exception.h"

NEMIVER_BEGIN_NAMESPACE (nemiver)

struct CallGraphNode::Priv {
    std::list<CallGraphNodeSafePtr> children;
    CallGraphNodeSafePtr parent;
    UString command;
    UString shared_object;
    UString symbol;
    float overhead;

    Priv () :
        parent (0)
    {
    }

    Priv (CallGraphNodeSafePtr &a_parent) :
        parent (a_parent)
    {
    }
};

CallGraphNode::CallGraphNode () :
    m_priv (new Priv)
{
}

CallGraphNode::CallGraphNode (CallGraphNodeSafePtr &a_parent) :
    m_priv (new Priv (a_parent))
{
}

CallGraphNode::~CallGraphNode ()
{
}

bool
CallGraphNode::root () const
{
    THROW_IF_FAIL (m_priv);
    return !m_priv->parent;
}

const CallGraphNode&
CallGraphNode::parent () const
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->parent);
    return *m_priv->parent;
}

const std::list<CallGraphNodeSafePtr>&
CallGraphNode::children () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->children;
}

void
CallGraphNode::add_child (const CallGraphNodeSafePtr &a_node)
{
    THROW_IF_FAIL (m_priv);
    m_priv->children.push_back (a_node);
}

const float&
CallGraphNode::overhead () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->overhead;
}

void
CallGraphNode::overhead (const float &a_overhead)
{
    THROW_IF_FAIL (m_priv);
    m_priv->overhead = a_overhead;
}

const UString&
CallGraphNode::symbol () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->symbol;
}

void
CallGraphNode::symbol (const UString &a_symbol)
{
    THROW_IF_FAIL (m_priv);
    m_priv->symbol = a_symbol;
}

const UString&
CallGraphNode::command () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->command;
}

void
CallGraphNode::command (const UString &a_command)
{
    THROW_IF_FAIL (m_priv);
    m_priv->command = a_command;
}

const UString&
CallGraphNode::shared_object () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->shared_object;
}

void
CallGraphNode::shared_object (const UString &a_shared_object)
{
    THROW_IF_FAIL (m_priv);
    m_priv->shared_object = a_shared_object;
}

NEMIVER_END_NAMESPACE (nemiver)

