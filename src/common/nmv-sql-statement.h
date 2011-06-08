/* -*- Mode: C++; indent-tabs-mode:nil; c-basic-offset:4; -*- */
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
#ifndef __NMV_SQL_STATEMENT_H__
#define __NMV_SQL_STATEMENT_H__

//#pragma GCC visibility push(default)
#include <vector>
//#pragma GCC visibility pop

#include "nmv-ustring.h"

using namespace std;
namespace nemiver {

namespace common {
class UString;
class LogStream;
}

namespace common
{

class Column
{
    common::UString m_name;
    common::UString m_value;
    bool m_auto_increment;

public:

    Column (const common::UString &a_name="empty:empty",
            const common::UString &a_value="empty:empty",
            bool a_auto_increment=false):
            m_name (a_name),
            m_value (a_value),
            m_auto_increment (a_auto_increment)
    {}

    Column (const common::UString &a_name,
            long long a_value,
            bool a_auto_increment=false):
            m_name (a_name),
            m_value (common::UString::from_int (a_value)),
            m_auto_increment (a_auto_increment)
    {}

    const common::UString& get_name ()
    {
        return m_name;
    }
    void set_name (const common::UString &a_name)
    {
        m_name = a_name;
    }
    const common::UString& get_value ()
    {
        return m_value;
    }
    void set_value (const common::UString &a_value)
    {
        m_value = a_value;
    }
    void set_auto_increment (bool a_auto)
    {
        m_auto_increment = a_auto;
    }
    bool get_auto_increment ()
    {
        return m_auto_increment;
    };
};

typedef vector<Column> ColumnList;
struct SQLStatementPriv;

class NEMIVER_API SQLStatement
{
    friend class Connection;
    friend struct SQLStatementPriv;

    SQLStatementPriv *m_priv;

public:

    SQLStatement (const common::UString &a_sql_string="");

    SQLStatement (const SQLStatement &);

    SQLStatement& operator= (const SQLStatement &);

    virtual ~SQLStatement ();
    virtual const common::UString& to_string () const;

    static common::UString escape_string (const common::UString &a_sql_string);

    operator const char* () const;
    friend common::LogStream& operator<< (common::LogStream&, const SQLStatement&);
};//end LogStream

NEMIVER_API common::LogStream & operator<< (common::LogStream &,
                                         const SQLStatement &);

}//end namespace common
}//end namespace nemiver
#endif //__NMV_SQL_STATEMENT_H__
