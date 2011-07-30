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
#include "nmv-ustring.h"
#include "nmv-exception.h"
#include "nmv-delete-statement.h"

using namespace nemiver::common;

namespace nemiver {
namespace common {

struct DeleteStatementPriv
{
    UString table_name;
    ColumnList where_cols;
    UString string_repr;

    DeleteStatementPriv (const UString &a_table_name,
                         ColumnList &a_where_cols):
            table_name (a_table_name),
            where_cols (a_where_cols)
    {}


}
;//end struct DeleteStatementPriv

DeleteStatement::DeleteStatement (const UString &a_table_name,
                                  ColumnList &a_where_columns)
{
    m_priv = new DeleteStatementPriv (a_table_name, a_where_columns);
}

DeleteStatement::~DeleteStatement ()
{
    if (m_priv) {
        delete m_priv;
        m_priv = 0;
    }
}

const UString&
DeleteStatement::to_string () const
{
    THROW_IF_FAIL (m_priv);

    RETURN_VAL_IF_FAIL (m_priv->table_name != "",
                        m_priv->string_repr);

    UString str, where_list;
    if (m_priv->string_repr == "") {
        for (ColumnList::iterator it = m_priv->where_cols.begin ();
                it != m_priv->where_cols.end ();
                ++it) {
            if (where_list.size ()) {
                where_list += ", ";
            }
            where_list += it->get_name () + "='" + it->get_value () + "'";
        }
        str = "delete from " + m_priv->table_name;
        if (where_list != "") {
            str += " where " + where_list;
        }
        m_priv->string_repr  = str;
    }
    return m_priv->string_repr;
}

const ColumnList&
DeleteStatement::get_where_columns () const
{
    return m_priv->where_cols;
}

}//end namespace common
}//end namespace nemiver

