//Author: Dodji Seketeli
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
#ifndef __NMV_LOCAL_VARS_INSPECTOR_H__
#define __NMV_LOCAL_VARS_INSPECTOR_H__

#include <list>
#include "common/nmv-object.h"
#include "common/nmv-safe-ptr-utils.h"
#include "nmv-i-perspective.h"
#include "nmv-i-debugger.h"

namespace Gtk {
    class Widget;
}

NEMIVER_BEGIN_NAMESPACE (nemiver)

class IWorkbench;

class NEMIVER_API LocalVarsInspector : public nemiver::common::Object {
    //non copyable
    LocalVarsInspector (const LocalVarsInspector&);
    LocalVarsInspector& operator= (const LocalVarsInspector&);

    struct Priv;
    SafePtr<Priv> m_priv;

protected:
    LocalVarsInspector ();

public:

    LocalVarsInspector (IDebuggerSafePtr &a_dbg,
                         IWorkbench &a_wb,
                         IPerspective &a_perspective);
    virtual ~LocalVarsInspector ();
    Gtk::Widget& widget () const;
    void set_local_variables
      (const std::list<IDebugger::VariableSafePtr> &a_vars);
    void show_local_variables_of_current_function
      (const IDebugger::Frame &a_frame);
    void visualize_local_variables_of_current_function ();
    void re_init_widget ();
};//end LocalVarsInspector

NEMIVER_END_NAMESPACE (nemiver)

#endif //__NMV_LOCAL_VARS_INSPECTOR_H__


