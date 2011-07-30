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
#include "nmv-exception.h"
#include "nmv-log-stream-utils.h"
#include "nmv-sql-statement.h"

using namespace std;
using namespace nemiver::common;

namespace nemiver {
namespace common {

struct SQLStatementPriv
{
    UString sql_string;
};

const UString&
SQLStatement::to_string () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->sql_string;
}

/// Escape a string by making sure all the lone '\'' in the string are
/// escaped properly by a string "''".
/// If given a string that is already escaped, this function should do the
/// right thing.
/// \param a_sql_string the string to escape
/// \return the escaped string.
common::UString
SQLStatement::escape_string (const common::UString &a_sql_string)
{
    UString out_string;
    unsigned i = 0;
    char c;
    while (i < a_sql_string.raw ().length ()) {
        c = a_sql_string.raw ()[i];
        if (c == '\'') {
            if (i + 1 < a_sql_string.raw ().length ()
                && a_sql_string.raw ()[i + 1] == '\'') {
                // This character '\'' precedes another '\'' character.
                // It means the next '\'' is escaped. We don't need to
                // escape anything, just insert the string "''".
                i += 2;
            } else {
                // This '\'' character is not followed by a '\''. So we
                // must escape this by "''". Yes in sql, a '\'' is escaped
                // by the string "''".
                ++i;
            }
            out_string.append ("''");
            continue;
        } else {
            out_string.append (1, c);
        }
        ++i;
    }
    return out_string;
}

SQLStatement::SQLStatement (const UString &a_sql_string)
{
    m_priv = new SQLStatementPriv;
    m_priv->sql_string = a_sql_string;
}

SQLStatement::SQLStatement (const SQLStatement &a_statement)
{
    m_priv = new SQLStatementPriv;
    m_priv->sql_string = a_statement.m_priv->sql_string;
}

SQLStatement&
SQLStatement::operator= (const SQLStatement &a_statement)
{
    if (this == &a_statement) {
        return *this;
    }
    m_priv->sql_string = a_statement.m_priv->sql_string;
    return *this;
}

SQLStatement::~SQLStatement ()
{
    if (!m_priv)
        return;
    delete m_priv;
    m_priv = 0;
}

SQLStatement::operator const char* () const
{
    return to_string ().c_str ();
}

LogStream&
operator<< (LogStream &a_os, const SQLStatement &a_s)
{
    a_os << a_s.to_string ();
    return a_os;
}

}//end common
}//end nemiver

