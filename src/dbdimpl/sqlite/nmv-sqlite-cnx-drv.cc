/* -*- Mode: C++; indent-tabs-mode:nil; c-basic-offset:4 -*- */

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
#include <cstring>
#include "config.h"

#include <sqlite3.h>
// Needed for sleep()
#include <unistd.h>
#include "common/nmv-log-stream-utils.h"
#include "common/nmv-exception.h"
#include "common/nmv-buffer.h"
#include "common/nmv-sql-statement.h"
#include "nmv-sqlite-cnx-drv.h"

using namespace nemiver::common;

namespace nemiver {
namespace common {
namespace sqlite {

struct Sqlite3Ref {
    void operator () (sqlite3 *a_ptr) {if (a_ptr) {}}
};//end struct Sqlite3Ref

struct Sqlite3Unref {
    void operator () (sqlite3 *a_ptr)
    {
        sqlite3_close (a_ptr);
    }
};//end struct Sqlite3Unref

typedef common::SafePtr<sqlite3, Sqlite3Ref, Sqlite3Unref> Sqlite3SafePtr;
struct SqliteCnxDrv::Priv {
    //sqlite3 database connection handle.
    //Must be de-allocated by a call to
    //sqlite3_close ();
    Sqlite3SafePtr sqlite;

    //the current prepared sqlite statement.
    //It must be deallocated after the result set
    //has been read, before we can close the db,
    //or before another statement is prepared.
    sqlite3_stmt *cur_stmt;

    //the result of the last sqlite3_step() function, or -333
    int last_execution_result;

    Priv ():
        sqlite (0),
        cur_stmt (0),
        last_execution_result (-333)
     {
     }

    bool step_cur_statement ();

    bool check_offset (gulong a_offset);
};

bool
SqliteCnxDrv::Priv::step_cur_statement ()
{
    RETURN_VAL_IF_FAIL (cur_stmt, false);
    last_execution_result = sqlite3_step (cur_stmt);
    bool result (false);

decide:
    switch (last_execution_result) {
        case SQLITE_BUSY:
            //db file is locked. Let's try again a couple of times.
            for (int i=0;i<2;++i) {
                sleep (1);
                last_execution_result = sqlite3_step (cur_stmt);
                if (last_execution_result != SQLITE_BUSY)
                    goto decide;
            }
            result = false;
            break;
        case SQLITE_DONE:
            //the statement was successfuly executed and
            //there is no more data to fecth.
            //go advertise the good news
            result = true;
            break;
        case SQLITE_ROW:
            //the statement was successfuly executed and
            //there is some rows waiting to be fetched.
            //go advertise the good news.
            result = true;
            break;
        case SQLITE_ERROR:
            LOG_ERROR ("sqlite3_step() encountered a runtime error:"
                 << sqlite3_errmsg (sqlite.get ()));
            if (cur_stmt) {
                sqlite3_finalize (cur_stmt);
                cur_stmt = 0;
            }
            result = false;
            break;
        case SQLITE_MISUSE:
            LOG_ERROR ("seems like sqlite3_step() has been called too much ...");
            if (cur_stmt) {
                sqlite3_finalize (cur_stmt);
                cur_stmt = 0;
            }
            result = false;
            break;
        default:
            LOG_ERROR ("got an unknown error code from sqlite3_step");
            if (cur_stmt) {
                sqlite3_finalize (cur_stmt);
                cur_stmt = 0;
            }
            result = false;
            break;
    }
    return result;
}

bool
SqliteCnxDrv::Priv::check_offset (gulong a_offset)
{
    if (!cur_stmt
        || (static_cast<glong> (a_offset) >= sqlite3_column_count (cur_stmt)))
        return false;
    return true;
}

SqliteCnxDrv::SqliteCnxDrv (sqlite3 *a_sqlite_handle)
{
    THROW_IF_FAIL (a_sqlite_handle);
    m_priv.reset (new Priv);
    m_priv->sqlite.reset (a_sqlite_handle);
}

SqliteCnxDrv::~SqliteCnxDrv ()
{
    LOG_D ("delete", "destructor-domain");
    close ();
}

const char*
SqliteCnxDrv::get_last_error () const
{
    if (m_priv && m_priv->sqlite) {
        return sqlite3_errmsg (m_priv->sqlite.get ());
    }
    return 0;
}

bool
SqliteCnxDrv::start_transaction ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv && m_priv->sqlite);
    return execute_statement (SQLStatement ("begin transaction"));
}

bool
SqliteCnxDrv::commit_transaction ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv && m_priv->sqlite);
    return execute_statement (SQLStatement ("commit"));
}

bool
SqliteCnxDrv::rollback_transaction ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv && m_priv->sqlite);
    return execute_statement (SQLStatement ("rollback"));
}

bool
SqliteCnxDrv::execute_statement (const SQLStatement &a_statement)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv && m_priv->sqlite);
    LOG_VERBOSE ("sql string: " << a_statement);

    //if the previous statement
    //(also contains the resulting context of a query
    //execution) hasn't been deleted, delete it before
    //we go forward.
    if (m_priv->cur_stmt) {
        sqlite3_finalize (m_priv->cur_stmt);
        m_priv->cur_stmt = 0;
        m_priv->last_execution_result = SQLITE_OK;
    }

    if (a_statement.to_string().bytes () == 0)
        return false;

    int status = sqlite3_prepare (m_priv->sqlite.get (),
                                  a_statement.to_string ().c_str (),
                                  a_statement.to_string ().bytes (),
                                  &m_priv->cur_stmt,
                                  0);
    if (status != SQLITE_OK) {
        LOG_ERROR ("sqlite3_prepare() failed, returning: "
             << status << ":" << get_last_error ()
             << ": sql was: '" << a_statement.to_string () + "'");
        return false;
    }

    THROW_IF_FAIL (m_priv->cur_stmt);
    if (!should_have_data ()) {
        return m_priv->step_cur_statement ();
    }
    return true;
}

bool
SqliteCnxDrv::should_have_data () const
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);

    if (get_number_of_columns () > 0)
        return true;
    return false;
}

bool
SqliteCnxDrv::read_next_row ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    if (m_priv->cur_stmt) {
        if (m_priv->last_execution_result == SQLITE_DONE) {
            return false;
        } else {
            bool res = m_priv->step_cur_statement ();
            if (res == true) {
                if (m_priv->last_execution_result == SQLITE_DONE) {
                    //there is no more data to fetch.
                    return false;
                } else {
                    return true;
                }
            }
        }
    }
    return false;
}

unsigned int
SqliteCnxDrv::get_number_of_columns () const
{
    THROW_IF_FAIL (m_priv);
    if (!m_priv->cur_stmt)
        return 0;
    return sqlite3_column_count (m_priv->cur_stmt);
}

bool
SqliteCnxDrv::get_column_content (unsigned long a_offset,
                                  Buffer &a_column_content) const
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);

    RETURN_VAL_IF_FAIL (m_priv->check_offset (a_offset), false);
    a_column_content.set
    (static_cast<const char*>(sqlite3_column_blob (m_priv->cur_stmt, a_offset)),
     sqlite3_column_bytes (m_priv->cur_stmt, a_offset));
    return true;
}

bool
SqliteCnxDrv::get_column_content (gulong a_offset,
                                  gint64 &a_column_content) const
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);

    RETURN_VAL_IF_FAIL (m_priv->check_offset (a_offset), false);
    int type = sqlite3_column_type (m_priv->cur_stmt, a_offset);
    if ((type != SQLITE_INTEGER) && (type != SQLITE_NULL)) {
        LOG_ERROR ("column number "<< static_cast<int> (a_column_content)
                                   << " is not of integer type");
        return false;
    }
    a_column_content = sqlite3_column_int64 (m_priv->cur_stmt, a_offset);
    return true;
}


bool
SqliteCnxDrv::get_column_content (gulong a_offset,
                                  double& a_column_content) const
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    RETURN_VAL_IF_FAIL (m_priv->check_offset (a_offset), false);
    int type = sqlite3_column_type (m_priv->cur_stmt, a_offset);
    if ((type != SQLITE_FLOAT) && (type != SQLITE_NULL)) {
        LOG_ERROR ("column number " << (int) a_offset
                   << " is not of type float");
        return false;
    }
    a_column_content = sqlite3_column_double (m_priv->cur_stmt, a_offset);
    return true;
}

bool
SqliteCnxDrv::get_column_content (gulong a_offset,
                                  UString& a_column_content) const
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);

    RETURN_VAL_IF_FAIL (m_priv->check_offset (a_offset), false);
    int type = sqlite3_column_type (m_priv->cur_stmt, a_offset);
    if (type == SQLITE_BLOB) {
        LOG_ERROR ("column number " << (int) a_offset << " is of type blob");
        return false;
    }
    a_column_content =
    reinterpret_cast<const char*>(sqlite3_column_text(m_priv->cur_stmt,a_offset));
    return true;
}

bool
SqliteCnxDrv::get_column_type (unsigned long a_offset,
                               enum ColumnType &a_type) const
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    RETURN_VAL_IF_FAIL (m_priv->check_offset (a_offset), false);
    int type = sqlite3_column_type (m_priv->cur_stmt, a_offset);

    switch (type) {
        case SQLITE_INTEGER:
            a_type = COLUMN_TYPE_INT;
            break;
        case SQLITE_FLOAT:
            a_type = COLUMN_TYPE_DOUBLE;
            break;
        case SQLITE_TEXT:
            a_type = COLUMN_TYPE_STRING;
            break;
        case SQLITE_BLOB:
            a_type = COLUMN_TYPE_BLOB;
            break;

        case SQLITE_NULL:
            a_type = COLUMN_TYPE_BLOB;
            break;
        default:
            a_type = COLUMN_TYPE_UNKNOWN;
            break;
    }
    return true;
}

bool
SqliteCnxDrv::get_column_name (unsigned long a_offset, Buffer &a_name) const
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    RETURN_VAL_IF_FAIL (m_priv->check_offset (a_offset), false);
    const char* name = sqlite3_column_name (m_priv->cur_stmt, a_offset);
    if (!name)
        return false;
    a_name.set (name, strlen (name));
    return true;
}

void
SqliteCnxDrv::close ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);

    if (m_priv->sqlite) {
        if (m_priv->cur_stmt) {
            sqlite3_finalize (m_priv->cur_stmt);
            m_priv->cur_stmt = 0;
        }
    }
}

}//end namespace sqlite
}//end namespace dbd
}//end namespace nemiver

