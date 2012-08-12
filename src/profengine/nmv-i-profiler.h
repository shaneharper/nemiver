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
#ifndef __NMV_I_PROFILER_H__
#define __NMV_I_PROFILER_H__

#include "common/nmv-api-macros.h"
#include "common/nmv-ustring.h"
#include "common/nmv-dynamic-module.h"
#include "common/nmv-safe-ptr-utils.h"
#include "nmv-i-conf-mgr.h"
#include "nmv-i-profiler.h"
#include "nmv-call-graph-node.h"
#include "nmv-record-options.h"

using nemiver::common::SafePtr;
using nemiver::common::DynamicModule;
using nemiver::common::DynamicModuleSafePtr;
using nemiver::common::DynModIface;
using nemiver::common::ObjectRef;
using nemiver::common::ObjectUnref;
using nemiver::common::UString;
using nemiver::common::Object;

NEMIVER_BEGIN_NAMESPACE (nemiver)

class IProfiler;
typedef SafePtr<IProfiler, ObjectRef, ObjectUnref> IProfilerSafePtr;

class NEMIVER_API IProfiler : public DynModIface {

    IProfiler (const IProfiler&);
    IProfiler& operator= (const IProfiler&);

protected:

    IProfiler (DynamicModule *a_dynmod) : DynModIface (a_dynmod)
    {
    }

public:

    virtual ~IProfiler () {}

    /// \name events you can connect to.

    /// @{
    virtual sigc::signal<void, CallGraphSafePtr>
        report_done_signal () const = 0;

    virtual sigc::signal<void, const UString&> record_done_signal () const = 0;

    virtual sigc::signal<void, const UString&, const UString&>
        symbol_annotated_signal () const = 0;
    /// @}

    virtual void attach_to_pid (int a_pid) = 0;

    virtual void report (const UString &a_data_file) = 0;

    virtual void system_wide_record (const RecordOptions &a_options) = 0;

    virtual void record (const UString &a_program_path,
                         const std::vector<UString> &a_argv,
                         const RecordOptions &a_options) = 0;

    virtual void stop_recording () = 0;

    virtual void annotate_symbol (const UString &a_symbol_name) = 0;

    virtual void do_init (IConfMgrSafePtr a_conf_mgr) = 0;

//    virtual void attach_to_pid () = 0;


};//end IProfiler

NEMIVER_END_NAMESPACE (nemiver)

#endif //__NMV_I_PROFILER_H__
