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
#ifndef __NMV_TRANSACTION_H__
#define __NMV_TRANSACTION_H__

/// \file
/// the declaration of the application level transaction class

#include "nmv-ustring.h"
#include "nmv-object.h"
#include "nmv-connection.h"
#include "nmv-exception.h"

namespace nemiver {
namespace common {

struct TransactionPriv;

/// \brief the application level persistence transaction class.
/// abstracts a transaction several persistent objects can be part of.
/// This class is used to model the fact that there the persistence
/// of several instances can be part of the same transaction.
/// Either they all succeed in  persisting their data,
/// or they all fail.
/// Read docs/nmv-persistence.txt to learn more.
class NEMIVER_API Transaction: public common::Object
{
    friend struct TransactionPriv;
    TransactionPriv *m_priv;
    Transaction ();

public:

    Transaction (Connection &a_con);
    Transaction (const Transaction &);
    Transaction& operator= (const Transaction &);
    virtual ~Transaction ();
    Connection& get_connection ();
    bool begin (const common::UString &a_subtransaction_name="");
    bool commit (const common::UString &a_subtransaction_name="");
    bool is_commited ();
    bool rollback ();
    long long get_id ();
    Glib::Mutex& get_mutex () const;
};//end class Transaction

typedef common::SafePtr<Transaction,
                        common::ObjectRef,
                        common::ObjectUnref> TransactionSafePtr;

//this class starts a transaction
//upon instanciation, and reverts
//the transaction if the transaction
//hasn't been commited by hand before
//the desctructor is called.
//This is a simple pattern to have exception safe
//based transaction code.
struct TransactionAutoHelper
{
    Transaction &m_trans;
    bool m_is_started;
    bool m_ignore;

    TransactionAutoHelper (common::Transaction &a_trans,
                           const common::UString &a_name ="generic-transaction",
                           bool a_ignore=false) :
            m_trans (a_trans),
            m_ignore (a_ignore)
    {
        if (m_ignore) {
            return;
        }
        THROW_IF_FAIL (m_trans.begin (a_name));
        m_is_started = true;
    }

    void end (const common::UString& a_name="generic-transaction")
    {
        if (m_ignore) {
            return;
        }
        THROW_IF_FAIL (m_trans.commit (a_name));
        m_is_started = false;
    }

    operator common::Transaction& ()
    {
        return m_trans;
    }

    common::Transaction& get ()
    {
        return m_trans;
    }

    ~TransactionAutoHelper ()
    {
        if (m_ignore) {
            return;
        }
        if (m_is_started) {
            THROW_IF_FAIL (m_trans.rollback ());
            m_is_started = false;
        }
    }
};//end TransactionAutoHelper
}//end namespace common
}//end namespace nemiver

#endif //__NMV_TRANSACTION_H__

