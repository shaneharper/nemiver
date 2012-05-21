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
#ifndef __NMV_I_PERSPECTIVE_H__
#define __NMV_I_PERSPECTIVE_H__

#include <gtkmm.h>
#include <list>
#include "common/nmv-api-macros.h"
#include "common/nmv-plugin.h"
#include "nmv-i-workbench.h"

NEMIVER_BEGIN_NAMESPACE (nemiver)

using nemiver::common::Plugin;
using std::list;
using nemiver::common::UString;
using nemiver::common::ObjectRef;
using nemiver::common::ObjectUnref;
using nemiver::common::SafePtr;
using nemiver::IWorkbenchSafePtr;

class IPerspective;

typedef SafePtr<IPerspective, ObjectRef, ObjectUnref> IPerspectiveSafePtr;

/// an abstraction of a consistent user interface dedicated
/// at doing a certain task. Nemiver is a collection of perspectives
/// even though only on perspective is coded at the moment:
/// 'the debugger  perspective'
/// Perspective are also plugins. It is the dutty of the Workbench to load
/// all the perspective it finds, at launch time.
class NEMIVER_API IPerspective : public Plugin::EntryPoint {
    //non copyable
    IPerspective (const IPerspective&);
    IPerspective& operator= (const IPerspective&);
    IPerspective ();

protected:
    IPerspective (DynamicModule *a_dynmod) :
        Plugin::EntryPoint (a_dynmod)
    {
    }

public:

    virtual bool process_gui_options (int a_argc, char **a_argv) = 0;

    virtual bool process_options (GOptionContext *a_context,
                                  int a_argc,
                                  char **a_argv) = 0;

    virtual GOptionGroup* option_group () const = 0;

    virtual const UString& name () const = 0;

    /// initialize the perspective within the context of
    /// of the workbench that loads it.
    /// \param a_workbench, the workbench that loaded the
    /// current perspective.
    virtual void do_init (IWorkbench *a_workbench) = 0;

    /// Get a unique identifier of the perspective.
    /// It is a good practice that this remains legible.
    /// \return the unique identifier of the of the perspective.
    virtual const UString& get_perspective_identifier () = 0;

    /// this method is called by the Workbench when
    /// the perspective is first set in it.
    /// \params a_tbs the list of toolbars. The implementation of this method
    /// must fill this parameter with the list of toolbars it wants the workbench
    /// to display when this perspective becomes active.
    virtual void get_toolbars (list<Gtk::Widget*> &a_tbs) = 0;

    /// \returns the body of the perspective.
    virtual Gtk::Widget* get_body () = 0;

    /// \returns the workbench associated to this perspective
    virtual IWorkbench& get_workbench () = 0;

    /// This method is only called once, during the
    /// perspective's initialisation time,
    /// by the workbench.
    virtual std::list<Gtk::UIManager::ui_merge_id> edit_workbench_menu () = 0;

    /// \brief load a menu file
    /// \param a_filename the file name of the menu file.
    ///  It's relative to the "menus" subdirectory of the perspective
    ///  \param a_widget_name the name of the widget to return as the root
    ///  of the menu.
    virtual Gtk::Widget* load_menu (const UString &a_filename,
                                    const UString &a_widget_name) = 0;

    /// \brief Should return true to allow shutdown.
    /// This Method will be called for each perspective before 
    /// workbench initiates a shutdown (). This is a chance given to
    /// the perspective to veto the shutdown (). Each perspective has
    /// to implement this function wherein it can decide for itself
    /// whether it wants to veto the shutdown or not. Returning 'true'
    /// from here means that the perspective is ok with the shutdown
    /// returning 'false' vetoes the shutdown and nemiver does not go
    /// down.
    virtual bool agree_to_shutdown () = 0;

    /// \name signals

    /// @{

    /// This signal is emited to notify the perspective
    /// about its activation state (whether it is activated or not).
    virtual sigc::signal<void, bool>& activated_signal () = 0;

    /// This signal is emited to notify the workbench when the
    /// layout of the perspective changes.
    virtual sigc::signal<void>& layout_changed_signal () = 0;

    /// @}

};//end class IPerspective

NEMIVER_END_NAMESPACE (nemiver)

#endif //__NMV_I_PERSPECTIVE_H__

