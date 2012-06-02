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
    unsigned cost;
    UString filepath;
    UString function;
    float percentage;

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

float
CallGraphNode::percentage () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->percentage;
}

void
CallGraphNode::percentage (const float &a_percentage)
{
    THROW_IF_FAIL (m_priv);
    m_priv->percentage = a_percentage;
}

unsigned
CallGraphNode::cost () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->cost;
}

void
CallGraphNode::cost (unsigned a_cost)
{
    THROW_IF_FAIL (m_priv);
    m_priv->cost = a_cost;
}

UString&
CallGraphNode::filepath () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->filepath;
}

void
CallGraphNode::filepath (const UString &a_filepath)
{
    THROW_IF_FAIL (m_priv);
    m_priv->filepath = a_filepath;
}

UString&
CallGraphNode::function () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->function;
}

void
CallGraphNode::function (const UString &a_function)
{
    THROW_IF_FAIL (m_priv);
    m_priv->function = a_function;
}

NEMIVER_END_NAMESPACE (nemiver)

