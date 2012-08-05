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
#ifndef __NMV_RECORD_OPTIONS_H__
#define __NMV_RECORD_OPTIONS_H__

#include "common/nmv-namespace.h"

NEMIVER_BEGIN_NAMESPACE (nemiver)

class RecordOptions {
    RecordOptions (const RecordOptions&);
    RecordOptions& operator= (const RecordOptions&);

protected:
    RecordOptions ()
    {
    }

public:
    virtual ~RecordOptions ()
    {
    }

    virtual bool do_callgraph_recording () const = 0;
    virtual bool do_collect_without_buffering () const = 0;
    virtual bool do_collect_raw_sample_records () const = 0;
    virtual bool do_system_wide_collection () const = 0;
    virtual bool do_sample_addresses () const = 0;
    virtual bool do_sample_timestamps () const = 0;
}; // end namespace RecordOptions

NEMIVER_END_NAMESPACE (nemiver)

#endif /* __NMV_RECORD_OPTIONS_H__ */

