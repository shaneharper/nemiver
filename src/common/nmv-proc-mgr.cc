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

#include <algorithm>
extern "C" {
#include "glibtop.h"
#include <glibtop/proclist.h>
#include <glibtop/procargs.h>
#include <glibtop/procuid.h>
}

#include "nmv-proc-mgr.h"
#include "nmv-exception.h"

NEMIVER_BEGIN_NAMESPACE (nemiver)
NEMIVER_BEGIN_NAMESPACE (common)

class ProcMgr : public IProcMgr {
    //non copyable
    ProcMgr (const ProcMgr &);
    ProcMgr& operator= (const ProcMgr &);

    mutable list<Process> m_process_list;
    friend class IProcMgr;

protected:
    ProcMgr ();

public:
    virtual ~ProcMgr ();
    const list<Process>& get_all_process_list () const ;
    bool get_process_from_pid (pid_t a_pid,
                               Process &a_process) const;
    bool get_process_from_name (const UString &a_pname,
                                Process &a_process,
                                bool a_fuzzy_search) const;
};//end class ProcMgr

struct LibgtopInit {
    LibgtopInit ()
    {
        glibtop_init ();
    }

    ~LibgtopInit ()
    {
        glibtop_close ();
    }
};//end struct LibgtopInit

ProcMgr::ProcMgr ()
{
    //init libgtop.
    static  LibgtopInit s_init;
}

ProcMgr::~ProcMgr ()
{
}

const list<ProcMgr::Process>&
ProcMgr::get_all_process_list () const
{
    glibtop_proclist buf_desc;
    memset (&buf_desc, 0, sizeof (buf_desc));
    pid_t *pids=0;

    m_process_list.clear ();

    try {
        //get the list of pids
        //this is an ugly cast, but I am quite obliged
        //since I have to support one version of glibtop_get_proclist()
        //that returns an int* and one that returns pid_t*
        pids = (pid_t*) glibtop_get_proclist (&buf_desc,
                                               GLIBTOP_KERN_PROC_ALL, 0) ;

        //get a couple of info about each pocess
        for (unsigned i=0; i < buf_desc.number ; ++i) {
            Process process;
            bool got_process = get_process_from_pid (pids[i], process);
            THROW_IF_FAIL (got_process);
            m_process_list.push_back (process);
        }

    } catch (...) {
    }
    if (pids) {
        g_free (pids);
        pids=0;
    }
    return m_process_list;
}

IProcMgrSafePtr
IProcMgr::create ()
{
    IProcMgrSafePtr result (new ProcMgr);
    return result;
}

bool
ProcMgr::get_process_from_pid (pid_t a_pid,
                               IProcMgr::Process &a_process) const
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    LOG_DD ("a_pid: " << (int) a_pid);
    Process process (a_pid);

    //get the process arguments
    glibtop_proc_args process_args_desc;
    memset (&process_args_desc, 0, sizeof (process_args_desc));
    char **argv = glibtop_get_proc_argv (&process_args_desc, a_pid, 1024);
    char **cur_arg = argv;
    while (cur_arg && *cur_arg) {
        process.args ().push_back
                        (UString (Glib::locale_to_utf8 (*cur_arg)));
        ++cur_arg;
    }
    if (argv) {
        g_strfreev (argv);
        argv=0;
    } else {
        LOG_DD ("got null process args, "
                "it means there is no process with pid: '"
                << (int) a_pid << "'. Bailing out.");
        return false;
    }

    //the the process ppid and uid, euid and user_name.
    glibtop_proc_uid proc_info;
    memset (&proc_info, 0, sizeof (proc_info));
    glibtop_get_proc_uid (&proc_info, process.pid ());
    process.ppid (proc_info.ppid);
    process.uid (proc_info.uid);
    process.euid (proc_info.uid);
    struct passwd *passwd_info=0;
    passwd_info = getpwuid (process.uid ());
    if (passwd_info) {
        process.user_name (passwd_info->pw_name);
    }

    //no need to free(passwd_info).
    a_process = process;
    LOG_DD ("got process with pid '" << (int) a_pid << "' okay.");
    return true;
}

class HasSameName {
    UString m_name;
    bool m_fuzzy;

    HasSameName ();
public:

    HasSameName (const UString &a_name,
                 bool a_fuzzy_search=false) :
        m_name (a_name.lowercase ()),
        m_fuzzy (a_fuzzy_search)
    {
    }

    bool operator() (const IProcMgr::Process &a_process)
    {
        if (a_process.args ().empty ()) {return false;}
        UString pname = *(a_process.args ().begin ());
        if (m_fuzzy) {
            if (pname.lowercase ().find (m_name) != UString::npos) {return true;}
        } else {
            if (pname.lowercase () == m_name) {return true;}
        }
        return false;
    }
};//end struct HasSameName

bool
ProcMgr::get_process_from_name (const UString &a_pname,
                                IProcMgr::Process &a_process,
                                bool a_fuzzy_search) const
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    LOG_DD ("a_pname: '" << a_pname << "'");
    if (a_pname == "") {return false;}
    const list<Process>& processes = get_all_process_list ();

    list<Process>::const_iterator it;
    if (a_fuzzy_search) {
        it = std::find_if (processes.begin (),
                           processes.end (),
                           HasSameName (a_pname, true));
    } else {
        it = std::find_if (processes.begin (),
                           processes.end (),
                           HasSameName (a_pname));
    }
    if (it == processes.end ()) {
        LOG_DD ("didn't find any process with name: '" << a_pname << "'");
        return false;
    }
    a_process = *it;
    LOG_DD ("found process with name: '"
            << a_pname
            << "', with pid: '"
            << (int) a_process.pid ());
    return true;
}

NEMIVER_END_NAMESPACE (common)
NEMIVER_END_NAMESPACE (nemiver)
