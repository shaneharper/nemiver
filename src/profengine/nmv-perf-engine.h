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
#ifndef __NMV_PERF_ENGINE_H__
#define __NMV_PERF_ENGINE_H__

#include "nmv-i-profiler.h"

NEMIVER_BEGIN_NAMESPACE (nemiver)

class PerfEngine : public IProfiler {

    PerfEngine (const PerfEngine &);
    PerfEngine& operator= (const PerfEngine &);

    struct Priv;
    SafePtr<Priv> m_priv;

public:

    PerfEngine (DynamicModule *a_dynmod);
    virtual ~PerfEngine ();

    void report (const UString &a_data_file);

    void record (const UString &a_program_path,
                 const std::vector<UString> &a_argv,
                 bool a_scale_counter_values,
                 bool a_do_callgraph,
                 bool a_child_inherit_counters);

    void annotate_symbol (const UString &a_symbol_name);

    sigc::signal<void, CallGraphSafePtr> report_done_signal () const;
    sigc::signal<void> program_exited_signal () const;
    sigc::signal<void, const UString&> record_done_signal () const;
    sigc::signal<void, const UString&, const UString&>
        symbol_annotated_signal () const;
}; // end namespace PerfEngine

NEMIVER_END_NAMESPACE (nemiver)

#endif /* __NMV_PERF_ENGINE_H__ */


