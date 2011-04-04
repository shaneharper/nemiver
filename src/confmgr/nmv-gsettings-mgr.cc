// Author: Fabien Parent
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
#include <giomm/settings.h>
#include "nmv-i-conf-mgr.h"
#include "nmv-conf-keys.h"

NEMIVER_BEGIN_NAMESPACE (nemiver)

class GSettingsMgr : public IConfMgr {
    GSettingsMgr (const GSettingsMgr &);
    GSettingsMgr& operator= (const GSettingsMgr &);

    typedef std::map<const Glib::ustring, Glib::RefPtr<Gio::Settings> > Settings;

    Settings m_settings;
    sigc::signal<void, const UString&, const UString&> m_value_changed_signal;

public:

    GSettingsMgr (DynamicModule *a_dynmod);
    virtual ~GSettingsMgr ();

    const UString& get_default_namespace () const;

    void register_namespace (const UString &a_name);

    bool get_key_value (const UString &a_key,
                        UString &a_value,
                        const UString &a_schema);
    void set_key_value (const UString &a_key,
                        const UString &a_value,
                        const UString &a_schema);

    bool get_key_value (const UString &a_key,
                        bool &a_value,
                        const UString &a_schema);
    void set_key_value (const UString &a_key,
                        bool a_value,
                        const UString &a_schema);

    bool get_key_value (const UString &a_key,
                        int &a_value,
                        const UString &a_schema);
    void set_key_value (const UString &a_key,
                        int a_value,
                        const UString &a_schema);

    bool get_key_value (const UString &a_key,
                        double &a_value,
                        const UString &a_schema);
    void set_key_value (const UString &a_key,
                        double a_value,
                        const UString &a_schema);

    bool get_key_value (const UString &a_key,
                        std::list<UString> &a_value,
                        const UString &a_schema);
    void set_key_value (const UString &a_key,
                        const std::list<UString> &a_value,
                        const UString &a_schema);

    sigc::signal<void,
                 const UString&,
                 const UString&>& value_changed_signal ();

};//end class GSettingsMgr

GSettingsMgr::GSettingsMgr (DynamicModule *a_dynmod) :
    IConfMgr (a_dynmod)
{
}

GSettingsMgr::~GSettingsMgr ()
{
    LOG_D ("delete", "destructor-domain");
}


const UString&
GSettingsMgr::get_default_namespace () const
{
    static UString s_default_schema_name = CONF_NAMESPACE_NEMIVER;
    return s_default_schema_name;
}

void
GSettingsMgr::register_namespace (const UString &a_name)
{
    UString name = a_name;
    if (name.empty ())
        name = get_default_namespace ();

    if (name.empty ())
        return;

    if (m_settings.find (name) != m_settings.end ())
        // We already have this schema
        return;

    Glib::RefPtr<Gio::Settings> settings =
        Gio::Settings::create (name);
    THROW_IF_FAIL (settings);

    settings->signal_changed ().connect (sigc::bind<const UString> (
        sigc::mem_fun (m_value_changed_signal,
                       &sigc::signal<void,
                       const UString&,
                       const UString&>::emit), name));

    THROW_IF_FAIL (m_settings.count (name) == 0);
    m_settings[name] = settings;
}

#define ENSURE_NAMESPACE_NOT_EMPTY(ns) \
    if (ns.empty ()) \
        ns = get_default_namespace (); \
    THROW_IF_FAIL (!ns.empty ());


bool
GSettingsMgr::get_key_value (const UString &a_key,
                             UString &a_value,
                             const UString &a_namespace)
{
    UString ns = a_namespace;
    ENSURE_NAMESPACE_NOT_EMPTY (ns);

    Glib::RefPtr<Gio::Settings> settings = m_settings[ns];
    THROW_IF_FAIL (settings);

    a_value = settings->get_string (a_key);
    return true;
}

void
GSettingsMgr::set_key_value (const UString &a_key,
                             const UString &a_value,
                             const UString &a_namespace)
{
    UString ns = a_namespace;
    ENSURE_NAMESPACE_NOT_EMPTY (ns);

    Glib::RefPtr<Gio::Settings> settings = m_settings[ns];
    THROW_IF_FAIL (settings);

    settings->set_string (a_key, a_value);
}

bool
GSettingsMgr::get_key_value (const UString &a_key,
                             bool &a_value,
                             const UString &a_namespace)
{
    UString ns = a_namespace;
    ENSURE_NAMESPACE_NOT_EMPTY (ns);

    Glib::RefPtr<Gio::Settings> settings = m_settings[ns];
    THROW_IF_FAIL (settings);

    a_value = settings->get_boolean (a_key);
    return true;
}

void
GSettingsMgr::set_key_value (const UString &a_key,
                             bool a_value,
                             const UString &a_namespace)
{
    UString ns = a_namespace;
    ENSURE_NAMESPACE_NOT_EMPTY (ns);

    Glib::RefPtr<Gio::Settings> settings = m_settings[ns];
    THROW_IF_FAIL (settings);

    settings->set_boolean (a_key, a_value);
}

bool
GSettingsMgr::get_key_value (const UString &a_key,
                             int &a_value,
                             const UString &a_namespace)
{
    UString ns = a_namespace;
    ENSURE_NAMESPACE_NOT_EMPTY (ns);

    Glib::RefPtr<Gio::Settings> settings = m_settings[ns];
    THROW_IF_FAIL (settings);

    a_value = settings->get_int (a_key);
    return true;
}

void
GSettingsMgr::set_key_value (const UString &a_key,
                             int a_value,
                             const UString &a_namespace)
{
    UString ns = a_namespace;
    ENSURE_NAMESPACE_NOT_EMPTY (ns);

    Glib::RefPtr<Gio::Settings> settings = m_settings[ns];
    THROW_IF_FAIL (settings);

    settings->set_int (a_key, a_value);
}

bool
GSettingsMgr::get_key_value (const UString &a_key,
                             double &a_value,
                             const UString &a_namespace)
{
    UString ns = a_namespace;
    ENSURE_NAMESPACE_NOT_EMPTY (ns);

    Glib::RefPtr<Gio::Settings> settings = m_settings[ns];
    THROW_IF_FAIL (settings);

    a_value = settings->get_double (a_key);
    return true;
}

void
GSettingsMgr::set_key_value (const UString &a_key,
                             double a_value,
                             const UString &a_namespace)
{
    UString ns = a_namespace;
    ENSURE_NAMESPACE_NOT_EMPTY (ns);

    Glib::RefPtr<Gio::Settings> settings = m_settings[ns];
    THROW_IF_FAIL (settings);

    settings->set_double (a_key, a_value);
}

bool
GSettingsMgr::get_key_value (const UString &a_key,
                             std::list<UString> &a_value,
                             const UString &a_namespace)
{
    UString ns = a_namespace;
    ENSURE_NAMESPACE_NOT_EMPTY (ns);

    Glib::RefPtr<Gio::Settings> settings = m_settings[ns];
    THROW_IF_FAIL (settings);

    a_value = settings->get_string_array (a_key);
    return true;
}

void
GSettingsMgr::set_key_value (const UString &a_key,
                             const std::list<UString> &a_value,
                             const UString &a_namespace)
{
    UString ns = a_namespace;
    ENSURE_NAMESPACE_NOT_EMPTY (ns);

    if (a_value.empty ())
        return;

    Glib::RefPtr<Gio::Settings> settings = m_settings[ns];
    THROW_IF_FAIL (settings);

    settings->set_string_array (a_key, a_value);
}

sigc::signal<void, const UString&, const UString&>&
GSettingsMgr::value_changed_signal ()
{
    return m_value_changed_signal;
}

using nemiver::common::DynModIfaceSafePtr;
class GSettingsMgrModule : public DynamicModule {

public:
    void get_info (Info &a_info) const
    {
        a_info.module_name = "GSettingsMgr";
        a_info.module_description =
            "A GSettings implementation of the IConfMgr interface";
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
            a_iface.reset (new GSettingsMgr (this));
        } else {
            return false;
        }
        return true;
    }
};//end class GSettingsMgrModule

NEMIVER_END_NAMESPACE (nemiver)

extern "C" {
bool
NEMIVER_API nemiver_common_create_dynamic_module_instance (void **a_new_instance)
{
    *a_new_instance = new nemiver::GSettingsMgrModule ();
    return (*a_new_instance != 0);
}

}//end extern C
