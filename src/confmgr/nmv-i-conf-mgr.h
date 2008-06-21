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

    typedef boost::variant<UString, bool, int, double> Value;

    virtual ~IConfMgr () {}

    virtual void set_key_dir_to_notify (const UString &a_key_dir) = 0;
    virtual void add_key_to_notify (const UString &a_key) = 0;

    virtual bool get_key_value (const UString &a_key, UString &a_value) = 0;
    virtual void set_key_value (const UString &a_key, const UString &a_value) = 0;

    virtual bool get_key_value (const UString &a_key, bool &a_value) = 0;
    virtual void set_key_value (const UString &a_key, bool a_value) = 0;

    virtual bool get_key_value (const UString &a_key, int &a_value) = 0;
    virtual void set_key_value (const UString &a_key, int a_value) = 0;

    virtual bool get_key_value (const UString &a_key, double &a_value) = 0;
    virtual void set_key_value (const UString &a_key, double a_value) = 0;

    virtual sigc::signal<void, const UString&, IConfMgr::Value&>&
                                                    value_changed_signal () = 0;

};//end class IConfMgr

NEMIVER_END_NAMESPACE(nemiver)

#endif //__NMV_CONF_MGR_H__
