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
#ifndef __NMV_CALL_GRAPH_NODE_H__
#define __NMV_CALL_GRAPH_NODE_H__

#include "common/nmv-safe-ptr-utils.h"
#include "common/nmv-namespace.h"
#include "common/nmv-ustring.h"
#include "common/nmv-object.h"
#include <list>

using nemiver::common::SafePtr;
using nemiver::common::UString;
using nemiver::common::Object;
using nemiver::common::ObjectRef;
using nemiver::common::ObjectUnref;

NEMIVER_BEGIN_NAMESPACE (nemiver)

class CallGraphNode;
typedef SafePtr<CallGraphNode, ObjectRef, ObjectUnref> CallGraphNodeSafePtr;
typedef CallGraphNode CallGraph;
typedef SafePtr<CallGraph, ObjectRef, ObjectUnref> CallGraphSafePtr;

class CallGraphNode : public Object {
    CallGraphNode (const CallGraphNode&);
    CallGraphNode& operator= (const CallGraphNode&);

    struct Priv;
    SafePtr<Priv> m_priv;

public:
    CallGraphNode ();
    CallGraphNode (CallGraphNodeSafePtr&);
    virtual ~CallGraphNode ();

    bool root () const;
    const CallGraphNode& parent () const;
    const std::list<CallGraphNodeSafePtr>& children () const;
    void add_child (const CallGraphNodeSafePtr&);

    const float& overhead () const;
    void overhead (const float&);

    const UString& command () const;
    void command (const UString&);

    const UString& shared_object () const;
    void shared_object (const UString&);

    const UString& symbol () const;
    void symbol (const UString&);
}; // end namespace CallGraphNode

NEMIVER_END_NAMESPACE (nemiver)

#endif /* __NMV_CALL_GRAPH_NODE_H__ */

