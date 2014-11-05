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
#include "nmv-conf-keys.h"

NEMIVER_BEGIN_NAMESPACE (nemiver)

using nemiver::common::GCharSafePtr;
using nemiver::common::GErrorSafePtr;

class GConfMgr : public IConfMgr {
    GConfMgr (const GConfMgr &);
    GConfMgr& operator= (const GConfMgr &);

    GConfClient *m_gconf_client;
    sigc::signal<void, const UString&, const UString&> m_value_changed_signal;

public:

    GConfMgr (DynamicModule *a_dynmod);
    virtual ~GConfMgr ();

    const UString& get_default_namespace () const;

    void register_namespace (const UString &a_name);

    bool get_key_value (const UString &a_key,
                        UString &a_value,
                        const UString &a_namespace);
    void set_key_value (const UString &a_key,
                        const UString &a_value,
                        const UString &a_namespace);

    bool get_key_value (const UString &a_key,
                        bool &a_value,
                        const UString &a_namespace);
    void set_key_value (const UString &a_key,
                        bool a_value,
                        const UString &a_namespace);

    bool get_key_value (const UString &a_key,
                        int &a_value,
                        const UString &a_namespace);
    void set_key_value (const UString &a_key,
                        int a_value,
                        const UString &a_namespace);

    bool get_key_value (const UString &a_key,
                        double &a_value,
                        const UString &a_namespace);
    void set_key_value (const UString &a_key,
                        double a_value,
                        const UString &a_namespace);

    bool get_key_value (const UString &a_key,
                        std::list<UString> &a_value,
                        const UString &a_namespace);
    void set_key_value (const UString &a_key,
                        const std::list<UString> &a_value,
                        const UString &a_namespace);

    sigc::signal<void,
                 const UString&,
                 const UString&>& value_changed_signal ();

};//end class GCongMgr

//static const char * NEMIVER_KEY_DIR = "/app/nemiver";

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
    a_conf_mgr->value_changed_signal ().emit (a_key, "");

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

const UString&
GConfMgr::get_default_namespace () const
{
    static UString s_default_ns = CONF_NAMESPACE_NEMIVER;
    return s_default_ns;
}

void
GConfMgr::register_namespace (const UString &a_name)
{
    THROW_IF_FAIL (m_gconf_client);
    GError *err=0;

    UString name = a_name;
    if (name.empty ())
        name = get_default_namespace ();

    if (name.empty ())
        return;

    gconf_client_add_dir (m_gconf_client,
                          name.c_str (),
                          GCONF_CLIENT_PRELOAD_NONE,
                          &err);
    GErrorSafePtr error (err);
    THROW_IF_FAIL2 (!error, error->message);

    gconf_client_notify_add (m_gconf_client,
                             name.c_str (),
                             (GConfClientNotifyFunc) client_notify_add_func,
                             this,
                             0,
                             &err);
    error.reset (err);
    THROW_IF_FAIL2 (!error, error->message);

    LOG_DD ("watching key for notification: '" << name << "'");
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
GConfMgr::get_key_value (const UString &a_key,
                         UString &a_value,
                         const UString &)
{
    THROW_IF_FAIL (m_gconf_client);

    GError *err=0;
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
GConfMgr::set_key_value (const UString &a_key,
                         const UString &a_value,
                         const UString &)
{
    THROW_IF_FAIL (m_gconf_client);
    GError *err=0;

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
GConfMgr::get_key_value (const UString &a_key,
                         bool &a_value,
                         const UString &)
{
    THROW_IF_FAIL (m_gconf_client);

    GError *err=0;
    bool value = gconf_client_get_bool (m_gconf_client,
                                        a_key.c_str (),
                                        &err);
    GErrorSafePtr error (err);
    if (error) {
        LOG_ERROR (error->message);
        return false;
    }
    a_value = value;
    return true;
}

void
GConfMgr::set_key_value (const UString &a_key,
                         bool a_value,
                         const UString &)
{
    THROW_IF_FAIL (m_gconf_client);
    GError *err=0;

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
GConfMgr::get_key_value (const UString &a_key,
                         int &a_value,
                         const UString &)
{
    THROW_IF_FAIL (m_gconf_client);

    GError *err=0;
    int value = gconf_client_get_int (m_gconf_client,
                                      a_key.c_str (),
                                      &err);
    GErrorSafePtr error (err);
    if (error) {
        LOG_ERROR (error->message);
        return false;
    }
    a_value = value;
    return true;
}

void
GConfMgr::set_key_value (const UString &a_key,
                         int a_value,
                         const UString &)
{
    THROW_IF_FAIL (m_gconf_client);
    GError *err=0;

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
GConfMgr::get_key_value (const UString &a_key,
                         double &a_value,
                         const UString &)
{
    THROW_IF_FAIL (m_gconf_client);

    GError *err=0;
    double value = gconf_client_get_float (m_gconf_client,
                                           a_key.c_str (),
                                           &err);
    GErrorSafePtr error (err);
    if (error) {
        LOG_ERROR (error->message);
        return false;
    }
    a_value = value;
    return true;
}

void
GConfMgr::set_key_value (const UString &a_key,
                         double a_value,
                         const UString &)
{
    THROW_IF_FAIL (m_gconf_client);
    GError *err=0;

    gconf_client_set_float (m_gconf_client,
                            a_key.c_str (),
                            a_value,
                            &err);
    GErrorSafePtr error (err);
    if (error) {
        THROW (error->message);
    }
}

bool
GConfMgr::get_key_value (const UString &a_key,
                         std::list<UString> &a_value,
                         const UString &)
{
    bool result=false;
    THROW_IF_FAIL (m_gconf_client);

    GError *err=0;
    GSList *list=0;
    list = gconf_client_get_list (m_gconf_client,
                                  a_key.c_str (),
                                  GCONF_VALUE_STRING,
                                  &err);
    GErrorSafePtr error (err);
    if (error) {
        LOG_ERROR (error->message);
        result = false;
        goto out;
    }
    for (GSList *cur = list; cur; cur = cur->next) {
        a_value.push_back ((char*)cur->data);
    }
    result = true;
out:
    if (list) {
        for (GSList *cur = list; cur; cur = cur->next) {
            g_free (cur->data);
        }
        g_slist_free (list);
        list = 0;
    }
    return result;
}

void
GConfMgr::set_key_value (const UString &a_key,
                         const std::list<UString> &a_value,
                         const UString &)
{
    if (a_value.empty ())
        return;
    THROW_IF_FAIL (m_gconf_client);
    GSList *list=0;
    std::list<UString>::const_iterator it;
    for (it = a_value.begin (); it != a_value.end (); ++it) {
        list = g_slist_prepend (list, g_strdup (it->c_str ()));
    }
    THROW_IF_FAIL (list);
    list = g_slist_reverse (list);
    THROW_IF_FAIL (list);

    GError *err=0;
    gconf_client_set_list (m_gconf_client, a_key.c_str (),
                           GCONF_VALUE_STRING,
                           list, &err);
    for (GSList *cur=list; cur; cur = cur->next) {
        g_free (cur->data);
    }
    g_slist_free (list);
    list = 0;
    GErrorSafePtr error (err);
    if (error) {
        THROW (error->message);
    }
}

sigc::signal<void, const UString&, const UString&>&
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
