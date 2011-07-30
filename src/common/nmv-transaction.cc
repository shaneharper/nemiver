/* -*- Mode: C++; indent-tabs-mode:nil; c-basic-offset: 4;  -*- */

/*
 *This file is part of the Nemiver Project.
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
#include <stack>
#include "nmv-exception.h"
#include "nmv-transaction.h"

using namespace std;
using namespace nemiver::common;

namespace nemiver {
namespace common {

struct TransactionPriv
{
    bool is_started;
    bool is_commited;
    stack<UString> transaction_stack;
    Connection *connection;
    long long id;
    mutable Glib::Mutex mutex;

    TransactionPriv (Connection &a_con):
            is_started (false),
            is_commited (false),
            connection (&a_con),
            id (0)
    {
        id = generate_id ();
    }

    long long generate_id ()
    {
        static long long s_id_sequence (0);
        static Glib::RecMutex s_mutex;
        Glib::RecMutex::Lock lock (s_mutex);
        return ++s_id_sequence;
    }
};//end TransactionPriv

Transaction::Transaction (Connection &a_con)
{
    m_priv = new TransactionPriv (a_con);
}

Transaction::Transaction (const Transaction &a_trans) :
    Object (a_trans)
{
    m_priv = new TransactionPriv (*a_trans.m_priv->connection);
    m_priv->is_started = a_trans.m_priv->is_started;
    m_priv->is_commited = a_trans.m_priv->is_commited;
    m_priv->transaction_stack = a_trans.m_priv->transaction_stack;
}

Glib::Mutex&
Transaction::get_mutex () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->mutex;
}

Transaction&
Transaction::operator= (const Transaction &a_trans)
{
    if (this == &a_trans)
        return *this;

    m_priv->is_started = a_trans.m_priv->is_started;
    m_priv->is_commited = a_trans.m_priv->is_commited;
    m_priv->transaction_stack = a_trans.m_priv->transaction_stack;
    m_priv->connection = a_trans.m_priv->connection;
    return *this;
}

Transaction::~Transaction ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    rollback ();
    if (m_priv) {
        delete m_priv;
        m_priv = 0;
    }
}

Connection&
Transaction::get_connection ()
{
    return *m_priv->connection;
}

bool
Transaction::begin (const UString &a_subtransaction_name)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    THROW_IF_FAIL (m_priv);
    m_priv->transaction_stack.push (a_subtransaction_name);
    if (m_priv->transaction_stack.size () == 1) {
        //This is the higher level subtransaction,
        //let's start a table level transaction then.
        m_priv->connection->start_transaction ();
        m_priv->is_started = true;
    }
    LOG_VERBOSE ("sub transaction " << a_subtransaction_name
                 <<"started");
    return true;
}

bool
Transaction::commit (const UString &a_subtrans_name)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    THROW_IF_FAIL (m_priv);

    if (m_priv->transaction_stack.empty ()) {
        LOG_ERROR ("There is no sub transaction named '"
                    << a_subtrans_name << "' to close");
        return false;
    }
    UString opened_subtrans  = m_priv->transaction_stack.top ();
    if (opened_subtrans != a_subtrans_name) {
        LOG_ERROR ("trying to close sub transaction '"
                   <<a_subtrans_name
                   <<"' while sub transaction '"
                   << opened_subtrans
                   << "' remains opened");
        return false;
    }
    m_priv->transaction_stack.pop ();
    if (m_priv->transaction_stack.empty ()) {
        if (m_priv->is_started) {
            if (!m_priv->connection->commit_transaction ()) {
                LOG_ERROR ("error during commit: "
                           << m_priv->connection->get_last_error ());
                return false;
            }
            m_priv->is_started = false;
            m_priv->is_commited = true;
            LOG_VERBOSE ("table level commit done");
        }
    }
    return true;
}

bool
Transaction::is_commited ()
{
    THROW_IF_FAIL (m_priv);
    return m_priv->is_commited;
}

bool
Transaction::rollback ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    THROW_IF_FAIL (m_priv);
    while (m_priv->transaction_stack.size ()) {
        m_priv->transaction_stack.pop ();
    }
    if (m_priv->is_started) {
        RETURN_VAL_IF_FAIL
        (m_priv->connection->rollback_transaction (), false);
    }
    m_priv->is_started = false;
    m_priv->is_commited = false;
    return true;
}

long long
Transaction::get_id ()
{
    return m_priv->id;
}

}//end namespace common
}//end namespace nemiver

