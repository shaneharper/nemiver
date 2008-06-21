// Author: Dodji Seketeli
/*
 *This file is part of the Nemiver project
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
#include <gconf/gconf-client.h>
#include "nmv-i-conf-mgr.h"
#include "common/nmv-ustring.h"
#include "common/nmv-exception.h"
#include "common/nmv-safe-ptr-utils.h"

NEMIVER_BEGIN_NAMESPACE (nemiver)

using nemiver::common::GCharSafePtr;

class GConfMgr : public IConfMgr {
    GConfMgr (const GConfMgr &);
    GConfMgr& operator= (const GConfMgr &);

    GConfClient *m_gconf_client;
    sigc::signal<void, const UString&, IConfMgr::Value&> m_value_changed_signal;

public:

    GConfMgr (DynamicModule *a_dynmod);
    virtual ~GConfMgr ();

    void set_key_dir_to_notify (const UString &a_key_dir);
    void add_key_to_notify (const UString &a_key);

    bool get_key_value (const UString &a_key, UString &a_value);
    void set_key_value (const UString &a_key, const UString &a_value);

    bool get_key_value (const UString &a_key, bool &a_value);
    void set_key_value (const UString &a_key, bool a_value);

    bool get_key_value (const UString &a_key, int &a_value);
    void set_key_value (const UString &a_key, int a_value);

    bool get_key_value (const UString &a_key, double &a_value) ;
    void set_key_value (const UString &a_key, double a_value) ;

    sigc::signal<void, const UString&, IConfMgr::Value&>& value_changed_signal ();

};//end class GCongMgr

//static const char * NEMIVER_KEY_DIR = "/app/nemiver";

struct GErrorRef {
    void operator () (GError *a_error) {if (a_error) {}}
};

struct GErrorUnref {
    void operator () (GError *a_error) {if (a_error) {g_error_free (a_error);}}
};

typedef SafePtr<GError, GErrorRef, GErrorUnref> GErrorSafePtr;

void
client_notify_func (GConfClient *a_client,
                    const char* a_key,
                    GConfValue *a_value,
                    GConfMgr *a_conf_mgr)
{
    NEMIVER_TRY

    THROW_IF_FAIL (a_client);
    THROW_IF_FAIL (a_key);
    THROW_IF_FAIL (a_value);
    THROW_IF_FAIL (a_conf_mgr);


    LOG_DD ("key changed: '" << a_key << "'");

    IConfMgr::Value value;
    switch (a_value->type) {
        case GCONF_VALUE_STRING:
            value = UString (gconf_value_get_string (a_value));
            LOG_DD ("key value is: '" << boost::get<UString> (value) << "'");
            break;
        case GCONF_VALUE_INT:
            value = gconf_value_get_int (a_value);
            LOG_DD ("key value is: '" << boost::get<int> (value) << "'");
            break;
        case GCONF_VALUE_FLOAT:
            value = gconf_value_get_float (a_value);
            LOG_DD ("key value is: '" << boost::get<double> (value) << "'");
            break;
        case GCONF_VALUE_BOOL:
            value = (bool) gconf_value_get_bool (a_value);
            LOG_DD ("key value is: '" << boost::get<bool> (value) << "'");
            break;
        default:
            LOG_ERROR ("unsupported key type '"
                       << (int)a_value->type
                       << "'");
            return;
    }
    a_conf_mgr->value_changed_signal ().emit (a_key, value);

    NEMIVER_CATCH_NOX
}

void
client_notify_add_func (GConfClient *a_client,
                        guint a_cnxn_id,
                        GConfEntry *a_entry,
                        GConfMgr *a_conf_mgr)
{
    THROW_IF_FAIL (a_client);
    THROW_IF_FAIL (a_entry);
    THROW_IF_FAIL (a_conf_mgr);
    if (a_cnxn_id) {}

    client_notify_func (a_client,
                        a_entry->key,
                        a_entry->value,
                        a_conf_mgr);
}

void
GConfMgr::set_key_dir_to_notify (const UString &a_key_dir)
{
    THROW_IF_FAIL (m_gconf_client);
    GError *err=0;
    gconf_client_add_dir (m_gconf_client,
                          a_key_dir.c_str (),
                          GCONF_CLIENT_PRELOAD_NONE,
                          &err);
    GErrorSafePtr error (err);
    THROW_IF_FAIL2 (!error, error->message);
    LOG_DD ("watching key for notification: '" << a_key_dir << "'");
}

void
GConfMgr::add_key_to_notify (const UString &a_key)
{
    THROW_IF_FAIL (m_gconf_client);
    GError *err=0;
    gconf_client_notify_add (m_gconf_client,
                             a_key.c_str (),
                             (GConfClientNotifyFunc) client_notify_add_func,
                             this,
                             NULL,
                             &err);
    GErrorSafePtr error (err);
    THROW_IF_FAIL2 (!error, error->message);
    LOG_DD ("watching key for notification: '" << a_key << "'");
}

GConfMgr::GConfMgr (DynamicModule *a_dynmod) :
    IConfMgr (a_dynmod),
    m_gconf_client (0)
{
    m_gconf_client = gconf_client_get_default ();
    THROW_IF_FAIL (m_gconf_client);
    g_signal_connect (G_OBJECT (m_gconf_client),
                      "value-changed",
                      G_CALLBACK (client_notify_func),
                      this);
}

GConfMgr::~GConfMgr ()
{
    LOG_D ("delete", "destructor-domain");
}

bool
GConfMgr::get_key_value (const UString &a_key, UString &a_value)
{
    THROW_IF_FAIL (m_gconf_client);

    GError *err=NULL;
    GCharSafePtr value (gconf_client_get_string (m_gconf_client,
                                                 a_key.c_str (),
                                                 &err));
    GErrorSafePtr error (err);
    if (error) {
        LOG_ERROR (error->message);
        return false;
    }
    a_value = value.get ();
    return true;
}

void
GConfMgr::set_key_value (const UString &a_key, const UString &a_value)
{
    THROW_IF_FAIL (m_gconf_client);
    GError *err=NULL;

    gconf_client_set_string (m_gconf_client,
                             a_key.c_str (),
                             a_value.c_str (),
                             &err);
    GErrorSafePtr error (err);
    if (error) {
        THROW (error->message);
    }
}

bool
GConfMgr::get_key_value (const UString &a_key, bool &a_value)
{
    THROW_IF_FAIL (m_gconf_client);

    GError *err=NULL;
    a_value = gconf_client_get_bool (m_gconf_client,
                                     a_key.c_str (),
                                     &err);
    GErrorSafePtr error (err);
    if (error) {
        LOG_ERROR (error->message);
        return false;
    }
    return true;
}

void
GConfMgr::set_key_value (const UString &a_key, bool a_value)
{
    THROW_IF_FAIL (m_gconf_client);
    GError *err=NULL;

    gconf_client_set_bool (m_gconf_client,
                           a_key.c_str (),
                           a_value,
                           &err);
    GErrorSafePtr error (err);
    if (error) {
        THROW (error->message);
    }
}

bool
GConfMgr::get_key_value (const UString &a_key, int &a_value)
{
    THROW_IF_FAIL (m_gconf_client);

    GError *err=NULL;
    a_value = gconf_client_get_int (m_gconf_client,
                                    a_key.c_str (),
                                    &err);
    GErrorSafePtr error (err);
    if (error) {
        LOG_ERROR (error->message);
        return false;
    }
    return true;
}

void
GConfMgr::set_key_value (const UString &a_key, int a_value)
{
    THROW_IF_FAIL (m_gconf_client);
    GError *err=NULL;

    gconf_client_set_int (m_gconf_client,
                          a_key.c_str (),
                          a_value,
                          &err);
    GErrorSafePtr error (err);
    if (error) {
        THROW (error->message);
    }
}

bool
GConfMgr::get_key_value (const UString &a_key, double &a_value)
{
    THROW_IF_FAIL (m_gconf_client);

    GError *err=NULL;
    a_value = gconf_client_get_float (m_gconf_client,
                                      a_key.c_str (),
                                      &err);
    GErrorSafePtr error (err);
    if (error) {
        LOG_ERROR (error->message);
        return false;
    }
    return true;
}

void
GConfMgr::set_key_value (const UString &a_key, double a_value)
{
    THROW_IF_FAIL (m_gconf_client);
    GError *err=NULL;

    gconf_client_set_float (m_gconf_client,
                            a_key.c_str (),
                            a_value,
                            &err);
    GErrorSafePtr error (err);
    if (error) {
        THROW (error->message) ;
    }
}

sigc::signal<void, const UString&, IConfMgr::Value&>&
GConfMgr::value_changed_signal ()
{
    return m_value_changed_signal;
}


using nemiver::common::DynModIfaceSafePtr;
class GConfMgrModule : public DynamicModule {

public:
    void get_info (Info &a_info) const
    {
        a_info.module_name = "GConfMgr";
        a_info.module_description =
            "A GConf implementation of the IConfMgr interface";
        a_info.module_version = "1.0";
    }

    /// \brief module init routinr
    void do_init ()
    {
    }

    bool lookup_interface (const std::string &a_iface_name,
                           DynModIfaceSafePtr &a_iface)
    {
        if (a_iface_name == "IConfMgr") {
            a_iface.reset (new GConfMgr (this));
        } else {
            return false;
        }
        return true;
    }
};//end class GConfMgrModule

NEMIVER_END_NAMESPACE (nemiver)

extern "C" {
bool
NEMIVER_API nemiver_common_create_dynamic_module_instance (void **a_new_instance)
{
    *a_new_instance = new nemiver::GConfMgrModule ();
    return (*a_new_instance != 0);
}

}//end extern C
