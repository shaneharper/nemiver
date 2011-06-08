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
#ifndef __NMV_I_CONNECTION_MANAGER_DRIVER_H__
#define __NMV_I_CONNECTION_MANAGER_DRIVER_H__

#include "nmv-ustring.h"
#include "nmv-dynamic-module.h"
#include "nmv-safe-ptr-utils.h"
#include "nmv-i-connection-driver.h"

namespace nemiver {
namespace common {

class IConnectionManagerDriver;

class DBDesc {
    common::UString m_type;
    common::UString m_host;
    unsigned long m_port;
    common::UString m_name;

public:
    DBDesc ()
    {}
    DBDesc (const common::UString &a_host,
            const unsigned long &a_port,
            const common::UString &a_db_name);

    const common::UString host () const
    {
        return m_host;
    };
    unsigned long port () const
    {
        return m_port;
    };
    const common::UString name () const
    {
        return m_name;
    };
    const common::UString type () const
    {
        return m_type;
    };

    void set_host (const common::UString &a_host)
    {
        m_host = a_host;
    };
    void set_port (const unsigned long &a_port)
    {
        m_port = a_port;
    };
    void set_name (const common::UString &a_name)
    {
        m_name = a_name;
    };
    void set_type (const common::UString &a_type)
    {
        m_type = a_type;
    };
};//end class DBDesc

class NEMIVER_PURE_IFACE IConnectionManagerDriver :
        public common::DynModIface {
public:
    IConnectionManagerDriver (DynamicModule *a_dynmod) :
        common::DynModIface (a_dynmod)
    {
    }

    virtual ~IConnectionManagerDriver () {};
    virtual IConnectionDriverSafePtr connect_to_db
                                        (const DBDesc &a_db_desc,
                                         const common::UString &a_user,
                                         const common::UString &a_pass) = 0;
};//end class IConnectionManagerDriver

typedef common::SafePtr<IConnectionManagerDriver,
                        common::ObjectRef,
                        common::ObjectUnref> IConnectionManagerDriverSafePtr;
};//end namespace common
};//end namespace nemiver
#endif //__NMV_I_CONNECTION_MANAGER_DRIVER_H__

