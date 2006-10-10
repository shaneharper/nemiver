//Author: Jonathon Jongsma
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
#ifndef __NMV_BREAKPOINTS_VIEW_H__
#define __NMV_BREAKPOINTS_VIEW_H__

#include <list>
#include <gtkmm/widget.h>
#include "nmv-object.h"
#include "nmv-i-debugger.h"
#include "nmv-safe-ptr-utils.h"

namespace nemiver {

class NEMIVER_API BreakpointsView : public nemiver::common::Object {
    //non copyable
    BreakpointsView (const BreakpointsView&) ;
    BreakpointsView& operator= (const BreakpointsView&) ;

    struct Priv ;
    SafePtr<Priv> m_priv ;

public:

    BreakpointsView () ;
    virtual ~BreakpointsView () ;
    Gtk::Widget& widget () const ;
    void set_breakpoints (const std::map<int, IDebugger::BreakPoint> &a_breakpoints) ;

};//end BreakpointsView

}//end namespace nemiver

#endif //__NMV_BREAKPOINTS_VIEW_H__


