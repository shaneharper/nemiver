//Author: Fabien Parent
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
#ifndef __NMV_PROF_PERSPECTIVE_H__
#define __NMV_PROF_PERSPECTIVE_H__

#include "nmv-i-perspective.h"

NEMIVER_BEGIN_NAMESPACE (nemiver)

class NEMIVER_API IProfPerspective : public IPerspective {
    //non copyable
    IProfPerspective (const IProfPerspective&);
    IProfPerspective& operator= (const IProfPerspective&);

public:

    IProfPerspective (DynamicModule *a_dynmod) :
        IPerspective (a_dynmod)
    {
    }

    virtual void annotate_symbol (const UString &a_symbol_name) = 0;

    virtual ~IProfPerspective () {};
};//end class IProfPerspective

NEMIVER_END_NAMESPACE (nemiver)
#endif //__NMV_PROF_PERSPECTIVE_H__

