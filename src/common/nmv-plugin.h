/* -*- Mode: C++; indent-tabs-mode:nil; c-basic-offset:4;-*- */

/*Copyright (c) 2005-2006 Dodji Seketeli
 *
 * Permission is hereby granted, free of charge,
 * to any person obtaining a copy of this
 * software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission
 * notice shall be included in all copies
 * or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS",
 * WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#ifndef __NMV_PLUGIN_H__
#define __NMV_PLUGIN_H__

#include "nmv-api-macros.h"
#include <vector>
#include <string>
#include "nmv-safe-ptr-utils.h"
#include "nmv-dynamic-module.h"

using namespace std;

namespace nemiver {
namespace common {

class PluginManager;
typedef SafePtr<PluginManager, ObjectRef, ObjectUnref> PluginManagerSafePtr;

class Plugin;
typedef SafePtr<Plugin, ObjectRef, ObjectUnref> PluginSafePtr;

class NEMIVER_API Plugin : public Object {

public:

    class Descriptor;
    typedef SafePtr<Descriptor, ObjectRef, ObjectUnref> DescriptorSafePtr;

    class EntryPoint;
    typedef SafePtr<EntryPoint, ObjectRef, ObjectUnref> EntryPointSafePtr;

private:

    friend class PluginManager;

    struct Priv;
    SafePtr<Priv> m_priv;

    //non copyable
    Plugin (const Plugin &);
    Plugin& operator= (const Plugin &);
    //forbid default constructor
    Plugin ();

private:
    Plugin (DescriptorSafePtr &a_descriptor,
            DynamicModuleManager &a_bootstrap_module_manager);

    void load_entry_point ();

public:

    class Descriptor : public Object {
        bool m_auto_activate;
        bool m_can_deactivate;
        UString m_name;
        UString m_version;
        UString m_plugin_path;
        UString m_entry_point_module_name;
        UString m_entry_point_interface_name;
        //map of deps, made of plugin/versions
        std::map<UString, UString> m_dependencies;

    public:

        Descriptor () :
            m_auto_activate (false),
            m_can_deactivate (true)
        {}

        void auto_activate (bool a_in) {m_auto_activate = a_in;}
        bool auto_activate () {return m_auto_activate;}

        void can_deactivate (bool a_in) {m_can_deactivate = a_in;}
        bool can_deactivate () {return m_can_deactivate;}

        const UString& name () const {return m_name;}
        void name (const UString &a_in) {m_name = a_in;}

        const UString& entry_point_module_name () const
        {
            return m_entry_point_module_name;
        }
        void entry_point_module_name (const UString &a_in)
        {
            m_entry_point_module_name = a_in;
        }

        const UString& entry_point_interface_name () const
        {
            return m_entry_point_interface_name;
        }
        void entry_point_interface_name (const UString &a_in)
        {
            m_entry_point_interface_name = a_in;
        }

        std::map<UString, UString>& dependencies () {return m_dependencies;}
        const std::map<UString, UString>& dependencies () const
        {
            return m_dependencies;
        }
        void dependencies (const std::map<UString, UString> &a_in)
        {
            m_dependencies = a_in;
        }

        const UString& plugin_path () const {return m_plugin_path;}
        void plugin_path (const UString &a_in) {m_plugin_path = a_in;}

        const UString& version () const {return m_version;}
        void version (const UString &a_in) {m_version = a_in;}
    };//end class Descriptor

public:

    class NEMIVER_API EntryPoint : public DynModIface {
        friend class Plugin;
    public:
        class Loader;
        typedef SafePtr<Loader, ObjectRef, ObjectUnref> LoaderSafePtr;

    private:
        friend class PluginManager;
        class Priv;
        SafePtr<Priv> m_priv;

        //non copyable
        EntryPoint (const EntryPoint &);
        EntryPoint& operator= (const EntryPoint &);
        EntryPoint ();


    protected:

        Plugin::EntryPoint::LoaderSafePtr plugin_entry_point_loader ();
        void plugin_entry_point_loader (Plugin::EntryPoint::LoaderSafePtr &);

        //must be created by a factory
        EntryPoint (DynamicModuleSafePtr &a_module);
        EntryPoint (DynamicModule *a_module);
        virtual void activate (bool a_activate,
                               ObjectSafePtr &a_activation_context);
        virtual bool is_activated ();

        const UString& plugin_path ();

        void descriptor (DescriptorSafePtr &a_desc);

    public:

        bool build_absolute_resource_path (const UString &a_relative_path,
                                           std::string &a_absolute_path);

        class NEMIVER_API Loader : public DynamicModule::Loader {
            struct Priv;
            SafePtr<Priv> m_priv;

            Loader ();
        public:
            Loader (const UString &a_plugin_path);
            virtual ~Loader ();
            const UString& plugin_path ();
        };//end Loader

        virtual ~EntryPoint ();
        DescriptorSafePtr descriptor ();
    };//end class EntryPoint

    virtual ~Plugin ();
    DescriptorSafePtr descriptor ();
    void descriptor (const DescriptorSafePtr &a_desc);
    EntryPoint& entry_point ();
    EntryPointSafePtr entry_point_ptr ();
    void activate (bool a_activate, ObjectSafePtr &a_activation_context);
    bool is_activated ();
};//end class Plugin

class NEMIVER_API PluginManager : public Object {
    struct Priv;
    SafePtr<Priv> m_priv;

    UString find_plugin_path_from_name (const UString &a_name);
    bool parse_descriptor (const UString &a_path,
                           Plugin::DescriptorSafePtr &a_out);
    static const UString& descriptor_name ();
    bool load_descriptor_from_plugin_path (const UString &a_plugin_path,
                                           Plugin::DescriptorSafePtr &a_out);
    bool load_descriptor_from_plugin_name (const UString &a_name,
                                           Plugin::DescriptorSafePtr &a_out);
    bool load_dependant_descriptors
                            (const Plugin::Descriptor &a_desc,
                             std::vector<Plugin::DescriptorSafePtr> &a_descs);
    bool load_dependant_descriptors_recursive
                            (const Plugin::Descriptor &a_desc,
                             std::vector<Plugin::DescriptorSafePtr> &);
public:

    PluginManager (DynamicModuleManager &a_module_manager);

    virtual ~PluginManager ();
    PluginSafePtr load_plugin_from_path
                                    (const UString &a_plugin_path,
                                     std::vector<PluginSafePtr> &a_deps);
    PluginSafePtr load_plugin_from_name (const UString &a_name,
                                         std::vector<PluginSafePtr> &a_deps);
    bool load_plugins ();
    std::vector<UString>& plugins_search_path ();
    void entry_point_loader (Plugin::EntryPoint::LoaderSafePtr &a_loader);
    Plugin::EntryPoint::LoaderSafePtr entry_point_loader ();
    std::map<UString, PluginSafePtr>& plugins_map ();
};//end class PluginManager

}//end namespace common
};//end namespace nemiver

#endif //__NMV_PLUGIN_H__

