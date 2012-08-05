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
#ifndef __NMV_CONFMGR_RECORD_OPTIONS_H__
#define __NMV_CONFMGR_RECORD_OPTIONS_H__

#include "nmv-record-options.h"
#include "common/nmv-safe-ptr.h"

NEMIVER_BEGIN_NAMESPACE (nemiver)

class IConfMgr;

using common::SafePtr;

class ConfMgrRecordOptions : public RecordOptions {
    ConfMgrRecordOptions (const RecordOptions&);
    ConfMgrRecordOptions& operator= (const RecordOptions&);

    struct Priv;
    SafePtr<Priv> m_priv;

public:
    ConfMgrRecordOptions (IConfMgr&);
    virtual ~ConfMgrRecordOptions ();

    bool do_callgraph_recording () const;
    bool do_collect_without_buffering () const;
    bool do_collect_raw_sample_records () const;
    bool do_system_wide_collection () const;
    bool do_sample_addresses () const;
    bool do_sample_timestamps () const;
}; // end namespace ConfMgrRecordOptions

NEMIVER_END_NAMESPACE (nemiver)

#endif /* __NMV_CONFMGR_RECORD_OPTIONS_H__ */

