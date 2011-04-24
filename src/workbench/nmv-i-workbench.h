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
#ifndef __NMV_I_WORKBENCH_H__
#define __NMV_I_WORKBENCH_H__

#include "common/nmv-api-macros.h"
#include "common/nmv-dynamic-module.h"
#include "nmv-i-conf-mgr.h"

//*******************
//some Gtk forward decls
//******************
namespace Gtk {
    class Widget;
    class Notebook;
    class Window;
    class ActionGroup;
    class UIManager;
    class Main;
}//end namespace Gtk

namespace Glib {
    class MainContext;
}

namespace nemiver {
namespace common {
    class UString;
}
}

NEMIVER_BEGIN_NAMESPACE (nemiver)

//*******************
//some forward decls
//******************
class IPerspective;
class IConfMgr;
using nemiver::common::SafePtr;
using nemiver::common::DynamicModule;
using nemiver::common::DynamicModuleSafePtr;
using nemiver::common::DynModIface;
using nemiver::common::DynModIfaceSafePtr;
using nemiver::common::ObjectRef;
using nemiver::common::ObjectUnref;
using nemiver::common::UString;

class IWorkbench;
typedef SafePtr<IWorkbench, ObjectRef, ObjectUnref> IWorkbenchSafePtr;

/// \brief the interface of the Workbench.
/// The workbench is what you see graphically when you use
/// Nemiver. It is made of a menu bar, toolbars and a set of widgets
/// that let you actually debug your code.
/// The set of widgets that actually makes the debugger experience is called
/// a perspective. The Workbench must be seen a container of perspectives.
/// A perspective can be active or not.
///
/// An implication of a workbench being a container of
/// perspectives is the way it manages toolbars.
/// There can be several toolbars hosted by the workbench.
/// The expected behaviour is that when the user switch from one perspective
/// to another, (for example from the debugger perspective to the valgring memory
/// checker, if one appears some day) the toolbars of the right perspective
/// are displayed. Oh, and there can be only one perspective active at a time.
///
/// When a new perspective is added to the workbench, the later queries the former
/// for the toolbars to display when the perspective becomes active. As a result,
/// The workbench appends the toolbars in a Gtk::Notebook and shows the appropriate
/// toolbars when the perspective becomes active.
class NEMIVER_API IWorkbench : public DynModIface {

    //non copyable
    IWorkbench (const IWorkbench&);
    IWorkbench& operator= (const IWorkbench&);

protected:
    // must be created by the dynamic modules factory
    IWorkbench (DynamicModule *a_dynmod) : DynModIface (a_dynmod)
    {
    }

public:

    virtual ~IWorkbench () {}

    /// \brief initialization function
    /// \param a_main the main loop created by the application.
    virtual void do_init (Gtk::Main &a_main) = 0;

    virtual void shut_down () = 0;

    /// \brief signal


    /// \name various getters

    /// @{

    /// \return the action group that is always activated
    virtual Glib::RefPtr<Gtk::ActionGroup> get_default_action_group () = 0;

    virtual Gtk::Widget& get_menubar () = 0;

    /// \returns gets the container of the toolbars.
    virtual Gtk::Notebook& get_toolbar_container () = 0;

    /// \return the Workbench root window
    virtual Gtk::Window& get_root_window () = 0;

    /// Set state-related information to be appended to the window title
    virtual void set_title_extension (const UString &a_str) = 0;

    /// \return the Gtk::UIManager of the workbench
    virtual Glib::RefPtr<Gtk::UIManager>& get_ui_manager () = 0;

    /// \return the perspective that which name matches a_name
    virtual IPerspective* get_perspective (const UString &a_name) = 0;

    /// set the configuration manager used by this interface
    virtual void do_init (IConfMgrSafePtr &) = 0;

    /// \return the interface of the configuration manager
    virtual IConfMgrSafePtr get_configuration_manager () = 0;
    ///@}

    virtual Glib::RefPtr<Glib::MainContext> get_main_context () = 0;
    /// \name signals

    /// @{

    /// \brief emitted just before the workbench shuts down
    virtual sigc::signal<void>& shutting_down_signal () = 0;

    /// @}
};//end class IWorkbench

NEMIVER_END_NAMESPACE (nemiver)

#endif //__NMV_I_WORKBENCH

