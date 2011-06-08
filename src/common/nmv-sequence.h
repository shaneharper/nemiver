/* -*- Mode: C++; indent-tabs-mode:nil; c-basic-offset:4;-*- */

//Authors: Dodji Seketeli
//Copyright 2006 Dodji Seketeli
/*
 *This file is part of the nemiver Project.
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
#ifndef __NMV_SEQUENCE_H__
#define __NMV_SEQUENCE_H__

#include "nmv-api-macros.h"
#include "nmv-object.h"
#include "nmv-exception.h"

namespace nemiver {
namespace common {

class NEMIVER_API Sequence : public Object {
    struct Priv;
    friend struct Priv;
    SafePtr<Priv> m_priv;

    //non copyable
    Sequence (const Sequence &);
    Sequence& operator= (const Sequence &);

public:

    class NEMIVER_EXCEPTION_API OverflowException :
                                    public nemiver::common::Exception {
        OverflowException ();

    public:
        OverflowException (const UString &a_message) :
            Exception (a_message)
        {}
        virtual ~OverflowException () throw () {};
    };//end class OverflowException

    Sequence ();
    virtual ~Sequence ();
    long long create_next_integer ();//throws OverflowException
    long long get_current_integer () const;
};//end class Sequence

}//end namespace common
}//end namespace nemiver
#endif // __NMV_SEQUENCE_H__

