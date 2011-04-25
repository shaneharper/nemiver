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
#ifndef __NMV_CONF_MGR_H__
#define __NMV_CONF_MGR_H__

#include "config.h"
#include <boost/variant.hpp>
#include <list>
#include "common/nmv-dynamic-module.h"
#include "common/nmv-env.h"

using nemiver::common::SafePtr;
using nemiver::common::DynamicModule;
using nemiver::common::DynamicModuleSafePtr;
using nemiver::common::DynModIface;
using nemiver::common::ObjectRef;
using nemiver::common::ObjectUnref;
using nemiver::common::UString;

NEMIVER_BEGIN_NAMESPACE (nemiver)

class IConfMgr;
typedef SafePtr<IConfMgr, ObjectRef, ObjectUnref> IConfMgrSafePtr;

class NEMIVER_API IConfMgr : public DynModIface {
    //non copyable
    IConfMgr (const IConfMgr &);
    IConfMgr& operator= (const IConfMgr &);

protected:

    IConfMgr (DynamicModule *a_dynmod) : DynModIface (a_dynmod)
    {
    }

public:

    virtual ~IConfMgr () {}

    virtual const UString& get_default_namespace () const = 0;

    virtual void register_namespace
        (const UString &a_namespace = /*default namespace*/"") = 0;

    virtual bool get_key_value (const UString &a_key,
                                UString &a_value,
                                const UString &a_namespace = "") = 0;
    virtual void set_key_value (const UString &a_key,
                                const UString &a_value,
                                const UString &a_namespace = "") = 0;

    virtual bool get_key_value (const UString &a_key,
                                bool &a_value,
                                const UString &a_namespace = "") = 0;
    virtual void set_key_value (const UString &a_key,
                                bool a_value,
                                const UString &a_namespace = "") = 0;

    virtual bool get_key_value (const UString &a_key,
                                int &a_value,
                                const UString &a_namespace = "") = 0;
    virtual void set_key_value (const UString &a_key,
                                int a_value,
                                const UString &a_namespace = "") = 0;

    virtual bool get_key_value (const UString &a_key,
                                double &a_value,
                                const UString &a_namespace = "") = 0;
    virtual void set_key_value (const UString &a_key,
                                double a_value,
                                const UString &a_namespace = "") = 0;

    virtual bool get_key_value (const UString &a_key,
                                std::list<UString> &a_value,
                                const UString &a_namespace = "") = 0;
    virtual void set_key_value (const UString &a_key,
                                const std::list<UString> &a_value,
                                const UString &a_namespace = "") = 0;

    virtual sigc::signal<void,
                         const UString&,
                         const UString&>& value_changed_signal () = 0;

};//end class IConfMgr

/// Load a dynamic module of a given name, query it for an interface
/// and return it.  But before that, load the proper configuration
/// manager dynamic module and query its interface.
///
/// \param a_dynmod_name the name of dynamic module to load
///
/// \param a_iface_name the name of the interface to query from the 
/// loaded dynamic module
///
/// \param a_confmgr an output argument set to a pointer to the
/// interface of configuration manager that was loaded.
template<class T>
SafePtr<T, ObjectRef, ObjectUnref>
load_iface_and_confmgr (const UString &a_dynmod_name,
                        const UString &a_iface_name,
                        IConfMgrSafePtr &a_confmgr)
{
    typedef SafePtr<T, ObjectRef, ObjectUnref> TSafePtr;

    // Load the confmgr interface
    a_confmgr =
      common::DynamicModuleManager::load_iface_with_default_manager<IConfMgr>
      (CONFIG_MGR_MODULE_NAME, "IConfMgr");

    //Load the IDebugger iterface
    TSafePtr iface =
        common::DynamicModuleManager::load_iface_with_default_manager<T>
        (a_dynmod_name, a_iface_name);
    THROW_IF_FAIL (iface);
    return iface;
}

/// Load a dynamic module of a given name, query it for an interface
/// and return it.  But before that, load the proper configuration
/// manager dynamic module and query its interface.  Initialize the
/// former interface with the interface of the dynamic module.  This
/// function template requires that T has an
/// T::do_init(IConfMgrSafePtr) method.
///
/// \param a_dynmod_name the name of dynamic module to load
///
/// \param a_iface_name the name of the interface to query from the 
/// loaded dynamic module
///
/// \return a pointer (wrapped in a SafePtr) to the interface which
/// name is a_iface_name, initialized with the proper interface of the
/// configuration manager.
template<class T>
SafePtr<T, ObjectRef, ObjectUnref>
load_iface_and_confmgr (const UString &a_dynmod_name,
                        const UString &a_iface_name)
{
    IConfMgrSafePtr m;
    SafePtr<T, ObjectRef, ObjectUnref> result;
    result = load_iface_and_confmgr<T> (a_dynmod_name, a_iface_name, m);
    result->do_init (m);
    return result;
}


NEMIVER_END_NAMESPACE(nemiver)

#endif //__NMV_CONF_MGR_H__
