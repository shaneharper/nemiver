/* -*- Mode: C++; indent-tabs-mode:nil; c-basic-offset:4; -*- */

/*
 *This file is part of the Dodji Common Library Project.
 *
 *Dodji is free software; you can redistribute
 *it and/or modify it under the terms of
 *the GNU General Public License as published by the
 *Free Software Foundation; either version 2,
 *or (at your option) any later version.
 *
 *Dodji is distributed in the hope that it will
 *be useful, but WITHOUT ANY WARRANTY;
 *without even the implied warranty of
 *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *See the GNU General Public License for more details.
 *
 *You should have received a copy of the
 *GNU General Public License along with Dodji;
 *see the file COPYING.
 *If not, write to the Free Software Foundation,
 *Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *See COPYRIGHT file copyright information.
 */
#ifndef __NMV_SQLITE_CNX_DRV_H__
#define __NMV_SQLITE_CNX_DRV_H__

#include "common/nmv-i-connection-driver.h"

namespace nemiver {

namespace common {
class SQLStatement;
}

namespace common {
namespace sqlite {

class SqliteCnxDrv: public common::IConnectionDriver {
    struct Priv;
    friend class SqliteCnxMgrDrv;
    SafePtr<Priv> m_priv;

    //forbid copy
    SqliteCnxDrv (const SqliteCnxDrv &);
    SqliteCnxDrv& operator= (const SqliteCnxDrv &);

    SqliteCnxDrv (sqlite3 *a_sqlite_handle);
    virtual ~SqliteCnxDrv ();

public:

    const char* get_last_error () const;

    bool start_transaction ();

    bool commit_transaction ();

    bool rollback_transaction ();

    bool execute_statement (const common::SQLStatement &a_statement);

    bool should_have_data () const;

    bool read_next_row ();

    unsigned int get_number_of_columns () const;

    bool get_column_type (unsigned long a_offset,
                          enum common::ColumnType &a_type) const;

    bool get_column_name (unsigned long a_offset, common::Buffer &a_name) const;

    bool get_column_content (unsigned long a_offset,
                             common::Buffer &a_column_content) const;

    bool get_column_content (gulong a_offset,
                             gint64 &a_column_content) const;

    bool get_column_content (gulong a_offset,
                             double& a_column_content) const;

    bool get_column_content (gulong a_offset,
                             common::UString& a_column_content) const;

    void close ();
};//end IConnectionDriver

}//end namespace sqlite
}//end namspace common
}//end namespace nemiver

#endif //__NMV_SQLITE_CNX_DRV_H__

