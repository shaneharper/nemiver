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
#ifndef __NMV_SPINNER_H__
#define __NMV_SPINNER_H__

#include "common/nmv-object.h"
#include "common/nmv-safe-ptr-utils.h"
#include "common/nmv-ustring.h"
namespace Gtk {
    class Widget;
}
using nemiver::common::Object;
using nemiver::common::SafePtr;
using nemiver::common::ObjectRef;
using nemiver::common::ObjectUnref;
using nemiver::common::UString;

NEMIVER_BEGIN_NAMESPACE (nemiver)
class Spinner;
typedef SafePtr<Spinner, ObjectRef, ObjectUnref> SpinnerSafePtr;

class Spinner : public Object {
    class Priv;
    SafePtr<Priv> m_priv;

protected:
    Spinner ();
    Spinner (const UString &a_root_path);

public:
    virtual ~Spinner () ;
    static SpinnerSafePtr create ();
    virtual void start ();
    virtual bool is_started () const;
    virtual void stop ();
    virtual void toggle_state ();
    virtual Gtk::Widget& get_widget () const;
};//end class Spinner

NEMIVER_END_NAMESPACE (nemiver)
#endif //__NMV_SPINNER_H__

