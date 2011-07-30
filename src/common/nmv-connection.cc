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
#include "nmv-connection.h"
#include "nmv-i-connection-driver.h"
#include "nmv-exception.h"
#include "nmv-buffer.h"

using namespace nemiver::common;

namespace nemiver {
namespace common {

struct ConnectionPriv {
    SafePtr <common::IConnectionDriver, ObjectRef, ObjectUnref> driver_iface;
    bool initialized;
    Glib::Mutex mutex;

    ConnectionPriv () :
            driver_iface (0), initialized (false)
    {}

    common::IConnectionDriver&
    get_driver ()
    {
        if (!initialized) {
            THROW ("Connection Driver not initialized");
        }
        return *driver_iface;
    }
};

void
Connection::set_connection_driver (const common::IConnectionDriverSafePtr &a_driver)
{
    THROW_IF_FAIL (m_priv);
    m_priv->driver_iface = a_driver;
}

void
Connection::initialize ()
{
    m_priv->initialized = true;
}

void
Connection::deinitialize ()
{
    m_priv->initialized = false;
}

Connection::Connection ()
{
    m_priv = new ConnectionPriv ();
}

Connection::Connection (const Connection &a_con) :
    Object (a_con)
{
    m_priv = new ConnectionPriv ();
    m_priv->driver_iface = a_con.m_priv->driver_iface;
    m_priv->initialized = a_con.m_priv->initialized;
}

Connection&
Connection::operator= (const Connection &a_con)
{
    if (this == &a_con) {
        return *this;
    }
    m_priv->driver_iface = a_con.m_priv->driver_iface;
    m_priv->initialized = a_con.m_priv->initialized;
    return *this;
}

bool
Connection::is_initialized () const
{
    return m_priv->initialized;
}

Connection::~Connection ()
{
    if (!m_priv) {
        return;
    }
    close ();
    delete m_priv;
    m_priv = 0;
}

const char*
Connection::get_last_error () const
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    Glib::Mutex::Lock lock (m_priv->mutex);
    return m_priv->get_driver ().get_last_error ();
}

bool
Connection::start_transaction ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    return m_priv->get_driver ().start_transaction ();
}

bool
Connection::commit_transaction ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    Glib::Mutex::Lock lock (m_priv->mutex);
    return m_priv->get_driver ().commit_transaction ();
}

bool
Connection::rollback_transaction ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    Glib::Mutex::Lock lock (m_priv->mutex);
    return m_priv->get_driver ().rollback_transaction ();
}

bool
Connection::execute_statement (const common::SQLStatement &a_statement)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    Glib::Mutex::Lock lock (m_priv->mutex);
    return m_priv->get_driver ().execute_statement (a_statement);
}

bool
Connection::should_have_data () const
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    Glib::Mutex::Lock lock (m_priv->mutex);
    return m_priv->get_driver ().should_have_data ();
}

bool
Connection::read_next_row ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    if (!should_have_data ()) {
        return false;
    }
    Glib::Mutex::Lock lock (m_priv->mutex);
    return m_priv->get_driver ().read_next_row ();
}

unsigned long
Connection::get_number_of_columns ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    Glib::Mutex::Lock lock (m_priv->mutex);
    return m_priv->get_driver ().get_number_of_columns ();
}


bool
Connection::get_column_type (unsigned long a_offset,
                             enum common::ColumnType &a_type)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    Glib::Mutex::Lock lock (m_priv->mutex);
    return m_priv->get_driver ().get_column_type (a_offset, a_type);
}

bool
Connection::get_column_name (unsigned long a_offset,
                             common::Buffer &a_name)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    Glib::Mutex::Lock lock (m_priv->mutex);
    return m_priv->get_driver ().get_column_name (a_offset, a_name);
}

bool
Connection::get_column_content (unsigned long a_offset,
                                common::Buffer &a_column_content)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    Glib::Mutex::Lock lock (m_priv->mutex);
    return m_priv->get_driver ().get_column_content
           (a_offset, a_column_content);
}

bool
Connection::get_column_content (gulong a_offset,
                                gint64 &a_column_content)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    Glib::Mutex::Lock lock (m_priv->mutex);
    return m_priv->get_driver ().get_column_content
           (a_offset, a_column_content);
}

bool
Connection::get_column_content (gulong a_offset,
                                double& a_column_content)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    Glib::Mutex::Lock lock (m_priv->mutex);
    return m_priv->get_driver ().get_column_content
           (a_offset, a_column_content);
}

bool
Connection::get_column_content (gulong a_offset,
                                UString& a_column_content)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);
    Glib::Mutex::Lock lock (m_priv->mutex);
    return m_priv->get_driver ().get_column_content
           (a_offset, a_column_content);
}

void
Connection::close ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    THROW_IF_FAIL (m_priv);

    //successive calls to this method
    //should never "fail".
    Glib::Mutex::Lock lock (m_priv->mutex);
    if (m_priv->driver_iface) {
        m_priv->driver_iface->close ();
    }
    deinitialize ();
    LOG_D ("delete", "destructor-domain");
}

}//end namespace common
}//end namespace nemiver

