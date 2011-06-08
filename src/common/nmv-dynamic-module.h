/* -*- Mode: C++; indent-tabs-mode:nil; c-basic-offset:4;-*- */

/*Copyright (c) 2005-2006 Dodji Seketeli
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies
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
#ifndef __NMV_DYNAMIC_MODULE_H__
#define __NMV_DYNAMIC_MODULE_H__

#include <vector>
#include <list>
#include <map>
#include <string>
#include <gmodule.h>
#include "nmv-api-macros.h"
#include "nmv-object.h"
#include "nmv-ustring.h"
#include "nmv-safe-ptr-utils.h"
#include "nmv-exception.h"

/// \file
/// the declaration of the DynamicModule class.
extern "C" {

/// \brief the factory of the DynamicModule.
///
/// Implementors of the loadable module must implement this function.
/// This function must then instanciate a DynamicModule,
/// set \em a_new_inst to that new DynamicModule, and return true in case
/// of success. Otherwise, it must return false.
bool nemiver_common_create_dynamic_module_instance (void **a_new_instance);

}

NEMIVER_BEGIN_NAMESPACE (nemiver)
NEMIVER_BEGIN_NAMESPACE (common)

class DynamicModule;
typedef SafePtr<DynamicModule, ObjectRef, ObjectUnref> DynamicModuleSafePtr;

class DynModIface;
typedef SafePtr<DynModIface, ObjectRef, ObjectUnref> DynModIfaceSafePtr;

class DynamicModuleManager;
typedef SafePtr<DynamicModuleManager,
                ObjectRef,
                ObjectUnref> DynamicModuleManagerSafePtr;

/// \brief The base class for loadable modules
///
/// A loadable module is a dynamic library that contains an instance
/// of DynamicModule.
class NEMIVER_API DynamicModule : public Object {

public:

    struct Info {
        UString module_name;
        UString module_description;
        UString module_version;

        Info (const UString &a_name,
              const UString &a_desc,
              const UString &a_version) :
            module_name (a_name),
            module_description (a_desc),
            module_version (a_version)
        {}

    };//end struct info

    struct Config : public Object {
        std::vector<UString> custom_library_search_paths;
        UString library_name;
        virtual ~Config () {};
    };//end stuct ModuleConfig

    typedef SafePtr<Config, ObjectRef, ObjectUnref> ConfigSafePtr;

    class NEMIVER_API Loader : public Object {
        struct Priv;
        SafePtr<Priv> m_priv;
        friend class DynamicModuleManager;

        //non copyable
        Loader (const Loader &);
        Loader& operator= (const Loader &);

        void set_dynamic_module_manager (DynamicModuleManager *a_mgr);


    public:
        Loader ();
        virtual ~Loader ();
        std::vector<UString>& config_search_paths ();
        virtual DynamicModule::ConfigSafePtr module_config
                                                    (const std::string &a_name);
        virtual UString build_library_path (const UString &a_module_name,
                                            const UString &a_lib_name);
        virtual ConfigSafePtr parse_module_config_file (const UString &a_path);
        virtual UString module_library_path (const UString &a_name);
        virtual GModule* load_library_from_path (const UString &a_path);
        virtual GModule* load_library_from_module_name (const UString &a_name);
        virtual DynamicModuleSafePtr create_dynamic_module_instance
                                                            (GModule *a_module);

        virtual DynamicModuleSafePtr load (const UString &a_name);
        virtual DynamicModuleSafePtr load_from_path
                                            (const UString &a_lib_path);
        DynamicModuleManager* get_dynamic_module_manager ();

    };//end class Loader
    typedef SafePtr<Loader, ObjectRef, ObjectUnref> LoaderSafePtr;

protected:
    //must be instanciated by a factory
    DynamicModule ();

private:
    struct Priv;
    SafePtr<Priv> m_priv;
    friend bool gpil_common_create_loadable_module_instance (void **a_new_inst);
    friend class Loader;
    friend class DynamicModuleManager;

    //non copyable
    DynamicModule (const DynamicModule &);
    DynamicModule& operator= (const DynamicModule &);

    void set_real_library_path (const UString &a_path);

    void set_name (const UString &a_name);

    void set_module_loader (Loader *a_loader);


public:

    /// \brief get the module loader class
    /// \return the module loader
    Loader* get_module_loader ();

    /// \brief gets the path of the library this module has been instanciated
    /// from.
    /// \return the path of the library the module has been instanciated from.
    const UString& get_real_library_path () const;

    /// \brief gets the (short) name of this dynmod
    const UString& get_name () const;

    /// \brief destructor
    virtual ~DynamicModule ();

    /// \brief get module info
    virtual void get_info (Info &a_info) const = 0;

    /// \brief module init routinr
    ///
    /// This abstract method must be implemented by type that
    /// extend DynamicModule
    virtual void do_init () = 0;

    /// \brief lookup the interface of a service object
    ///
    /// Creates a service object and returns a pointer to its interface
    /// This abstract method must be implemented by types that extend
    /// DynamicModule
    /// \param a_iface_name the name of the service object interface to return.
    /// \param a_iface out parameter. The returned service object interface, 
    /// if and only if this function returns true. Otherwise, this parameter
    /// is not set by the function.
    /// \return true if a service object interface with the name a_iface_name
    /// has been found, false otherwise.
    virtual bool lookup_interface (const std::string &a_iface_name,
                                   DynModIfaceSafePtr &a_iface) = 0;

    template <class T> bool lookup_interface
                                (const std::string &a_iface_name,
                                 SafePtr<T, ObjectRef, ObjectUnref> &a_iface)
    {
        DynModIfaceSafePtr iface;
        if (!lookup_interface (a_iface_name, iface)) {
            return false;
        }
        typedef SafePtr<T, ObjectRef, ObjectUnref> TSafePtr;
        // Here, the 'template' keyword is useless, stricto sensu.
        // But we need it to keep gcc 3.3.5 happy.
        TSafePtr res = iface.template do_dynamic_cast<T> ();
        if (!res) {
            return false;
        }
        a_iface = res;
        return true;
    }
};//end class DynamicModule

class NEMIVER_API DynModIface : public Object {
    DynamicModuleSafePtr m_dynamic_module;
    //subclassers _MUST_ not use the default constructor,
    //but rather use the DynModIface (DynamicModule &a_dynmod) one.
    DynModIface ();
    DynModIface (const DynModIface &);
    DynModIface& operator= (const DynModIface &);

public:

    DynModIface (DynamicModuleSafePtr &a_dynmod) :
        m_dynamic_module (a_dynmod)
    {
        THROW_IF_FAIL (m_dynamic_module);
    }

    DynModIface (DynamicModule *a_dynmod) :
        m_dynamic_module (DynamicModuleSafePtr (a_dynmod, true))
    {
        THROW_IF_FAIL (m_dynamic_module);
    }

    DynamicModule& get_dynamic_module () const
    {
        THROW_IF_FAIL (m_dynamic_module);
        return *m_dynamic_module;
    }
};//end class DynModIface

class NEMIVER_API ModuleRegistry : public Object {
    struct Priv;
    SafePtr<Priv> m_priv;
    //non coyable;
    ModuleRegistry (const ModuleRegistry &);
    ModuleRegistry& operator= (const ModuleRegistry &);

public:
    ModuleRegistry ();
    virtual ~ModuleRegistry ();
    GModule* get_library_from_cache (const UString a_name);
    void put_library_into_cache (const UString a_name,
                                 GModule *a_module);
};//end class ModuleRegistry


class NEMIVER_API DynamicModuleManager : public Object {
    struct Priv;
    SafePtr<Priv> m_priv;

public:

    DynamicModuleManager ();
    virtual ~DynamicModuleManager ();

    DynamicModuleSafePtr load_module_from_path (const UString &a_library_path,
                                                DynamicModule::Loader &);

    DynamicModuleSafePtr load_module_from_path (const UString &a_library_path);

    DynamicModuleSafePtr load_module (const UString &a_name,
                                      DynamicModule::Loader &);

    DynamicModuleSafePtr load_module (const UString &a_name);

    static DynamicModuleManager& get_default_manager ();

    static DynamicModuleSafePtr load_module_with_default_manager
                                                    (const UString &a_mod_name,
                                                     DynamicModule::Loader &);

    static DynamicModuleSafePtr load_module_with_default_manager
                                                    (const UString &a_mod_name);


    template <class T> SafePtr<T, ObjectRef, ObjectUnref>
                                    load_iface (const UString &a_module_name,
                                                const UString &a_iface_name,
                                                DynamicModule::Loader & a_loader,
                                                DynamicModuleSafePtr &a_dynmod);

    template <class T> SafePtr<T, ObjectRef, ObjectUnref>
                                    load_iface (const UString &a_module_name,
                                                const UString &a_iface_name,
                                                DynamicModule::Loader & a_loader);

    template <class T> SafePtr<T, ObjectRef, ObjectUnref>
                                    load_iface (const UString &a_module_name,
                                                const UString &a_iface_name,
                                                DynamicModuleSafePtr &a_dynmod);

    template <class T> SafePtr<T, ObjectRef, ObjectUnref>
                                    load_iface (const UString &a_module_name,
                                                const UString &a_iface_name);
    template <class T> static SafePtr<T, ObjectRef, ObjectUnref>
                 load_iface_with_default_manager (const UString &a_module_name,
                                                  const UString &a_iface_name);


    ModuleRegistry& module_registry ();
    DynamicModule::LoaderSafePtr& module_loader ();
    void module_loader (DynamicModule::LoaderSafePtr &a_loader);
};//end class DynamicModuleManager

template <class T>
SafePtr<T, ObjectRef, ObjectUnref>
DynamicModuleManager::load_iface (const UString &a_module_name,
                                  const UString &a_iface_name,
                                  DynamicModule::Loader &a_loader,
                                  DynamicModuleSafePtr &a_dynmod)
{
    DynamicModuleSafePtr module (load_module (a_module_name, a_loader));
    typedef SafePtr<T, ObjectRef, ObjectUnref> TSafePtr;
    if (!module) {
        THROW (UString ("failed to load module '") + a_module_name);
    }
    module->do_init ();
    LOG_REF_COUNT (module, a_module_name);
    DynModIfaceSafePtr tmp_iface;
    if (!module->lookup_interface (a_iface_name, tmp_iface)) {
        THROW (UString ("module does not have interface: ") + a_iface_name);
    }
    THROW_IF_FAIL (tmp_iface);
    LOG_REF_COUNT (module, a_module_name);
    TSafePtr result;
    // Here, the 'template' keyword is useless, stricto sensu.
    // But we need it to keep gcc 3.3.5 happy.
    result = tmp_iface.template do_dynamic_cast<T> ();
    LOG_REF_COUNT (module, a_module_name);
    if (!result) {
        THROW (UString ("interface named ")
               + a_iface_name
               + " is not of the expected type'");
    }
    a_dynmod = module;
    return result;
}

template <class T>
SafePtr<T, ObjectRef, ObjectUnref>
DynamicModuleManager::load_iface (const UString &a_module_name,
                                  const UString &a_iface_name,
                                  DynamicModule::Loader &a_loader)
{
    DynamicModuleSafePtr dynmod;

    return load_iface<T> (a_module_name, a_iface_name,
                          a_loader, dynmod);
}

template <class T>
SafePtr<T, ObjectRef, ObjectUnref>
DynamicModuleManager::load_iface (const UString &a_module_name,
                                  const UString &a_iface_name,
                                  DynamicModuleSafePtr &a_dynmod)
{
    return load_iface<T> (a_module_name, a_iface_name,
                          *module_loader (), a_dynmod);
}

template <class T>
SafePtr<T, ObjectRef, ObjectUnref>
DynamicModuleManager::load_iface (const UString &a_module_name,
                                  const UString &a_iface_name)
{
    DynamicModuleSafePtr dynmod;
    return load_iface<T> (a_module_name, a_iface_name,
                          *module_loader (), dynmod);
}

template <class T>
SafePtr<T, ObjectRef, ObjectUnref>
DynamicModuleManager::load_iface_with_default_manager
                                            (const UString &a_mod_name,
                                             const UString &a_iface_name)
{
    return get_default_manager ().load_iface<T> (a_mod_name, a_iface_name);
}

/// This is a shortcut method to load an interface
/// using the same module_manager that was used to load
/// a_iface.
template<class T>
SafePtr<T, ObjectRef, ObjectUnref>
load_iface_using_context (DynModIface &a_iface,
                          const UString &a_iface_name)
{
    DynamicModuleManager *manager =
        a_iface.get_dynamic_module ().get_module_loader ()
                                    ->get_dynamic_module_manager ();
    THROW_IF_FAIL (manager);
    SafePtr<T, ObjectRef, ObjectUnref> iface =
        manager->load_iface<T> (a_iface.get_dynamic_module ().get_name (),
                                a_iface_name);
    THROW_IF_FAIL (iface);
    return iface;
}

template<class T>
SafePtr<T, ObjectRef, ObjectUnref>
load_iface_using_context (DynamicModule &a_dynmod,
                          const UString &a_iface_name)
{
    DynamicModuleManager *manager =
        a_dynmod.get_module_loader () ->get_dynamic_module_manager ();

    THROW_IF_FAIL (manager);
    SafePtr<T, ObjectRef, ObjectUnref> iface =
        manager->load_iface<T> (a_dynmod.get_name (), a_iface_name);
    THROW_IF_FAIL (iface);
    return iface;
}

NEMIVER_END_NAMESPACE (common)
NEMIVER_END_NAMESPACE (nemiver)

#endif// __NMV_DYNAMIC_MODULE_H__

