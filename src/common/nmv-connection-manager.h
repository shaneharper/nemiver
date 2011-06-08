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
#ifndef __NMV_CONNECTION_MANAGER_H__
#define __NMV_CONNECTION_MANAGER_H__

#include "nmv-connection.h"

/// \file
///declaration of the #Connection manager class
namespace nemiver {

namespace common {
    class UString;
}

namespace common {

/// \brief declaration of the entry point
/// of the db abstraction layer.
/// this class has the capacity to set the type
/// of the db driver we want to use, connect to a database
/// and return a #Connection object.
class NEMIVER_API ConnectionManager
{

    //forbid instanciation
    ConnectionManager ();
    ~ConnectionManager ();

public:

    /// \brief connect to a db specified by a uri
    /// note that this function throws an #Exception if fails.
    /// \param a_connection_string a uri specifing the db we want to connect to.
    /// the uri ressembles:
    /// vdbc:dbtypename://[host:[port]]/[nameofthedbinstance]
    /// \param a_user the username to access the db
    /// \param a_pass the password to access the db
    /// \param a_connection out parameter. The connection initialized
    /// by the function upon successful completion.
    static void create_db_connection (const common::UString &a_connection_string,
                                      const common::UString &a_user,
                                      const common::UString &a_pass,
                                      Connection &a_connection);

    /// \brief create a connection to the default database.
    /// The default database is the one configured by the user
    /// by filling the file nemiver.conf.
    /// \return a smart pointer to a useable db connection. Throws
    /// an Exception if it fails.
    static ConnectionSafePtr create_db_connection ();

    /// \brief get the type of database we are using.
    /// \return a null terminated string that may be "mysql", "sqlite" etc ...
    static const char* get_db_type ();

protected:

};//end class ConnectionManager

}//end namespace common
}//end namespace nemiver

#endif //__NMV_CONNECTION_MANAGER_H__

