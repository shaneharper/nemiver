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
#ifndef __NMV_CONNECTION_H__
#define __NMV_CONNECTION_H__

#include "nmv-safe-ptr-utils.h"
#include "nmv-object.h"
#include "nmv-i-connection-driver.h"

namespace nemiver {

namespace common {
class UString;
class Buffer;
}

namespace common {
class SQLStatement;
}

namespace common {

class ResultSetDataReader;
struct ConnectionPriv;

class NEMIVER_API Connection : public common::Object
{

    friend struct ConnectionPriv;
    friend class ConnectionManager;
    ConnectionPriv *m_priv;

    void set_connection_driver (const common::IConnectionDriverSafePtr &a_driver);
    void initialize ();
    void deinitialize ();

public:
    Connection ();

    Connection (const Connection &a_con);

    Connection& operator= (const Connection &a_con);

    bool is_initialized () const;

    virtual ~Connection ();

    const char* get_last_error () const;

    bool start_transaction ();

    bool commit_transaction ();

    bool rollback_transaction ();

    bool execute_statement (const common::SQLStatement &a_statement);

    bool should_have_data () const;

    bool read_next_row ();

    unsigned long get_number_of_columns ();

    bool get_column_type (unsigned long a_offset,
                          enum common::ColumnType &);

    bool get_column_name (unsigned long a_offset, common::Buffer &a_name);

    bool get_column_content (unsigned long a_offset,
                             common::Buffer &a_field_content);

    bool get_column_content (gulong a_offset,
                             gint64 &a_column_content);

    bool get_column_content (gulong a_offset,
                             double& a_column_content);

    bool get_column_content (gulong a_offset,
                             common::UString& a_column_content);

    void close ();
};//end Connection

typedef common::SafePtr<Connection,
                        common::ObjectRef,
                        common::ObjectUnref> ConnectionSafePtr;

}//end namespace common
}//end nemiver
#endif //end __NMV_CONNECTION_H__

