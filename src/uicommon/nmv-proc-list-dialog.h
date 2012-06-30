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
#ifndef __NMV_PROC_LIST_DIALOG_H__
#define __NMV_PROC_LIST_DIALOG_H__

#include "common/nmv-safe-ptr-utils.h"
#include "common/nmv-proc-mgr.h"
#include "nmv-dialog.h"

namespace common {
    class UString;
}

NEMIVER_BEGIN_NAMESPACE (nemiver)

using nemiver::common::IProcMgr;

class ProcListDialog : public Dialog {
    class Priv;
    SafePtr<Priv> m_priv;

public:

    ProcListDialog (const UString &a_root_path,
                    IProcMgr &a_proc_mgr);
    virtual ~ProcListDialog ();
    virtual gint run ();

    bool has_selected_process ();
    bool get_selected_process (IProcMgr::Process &a_proc/*out param*/);
};//end class ProcListDialog
NEMIVER_END_NAMESPACE (nemiver)

#endif //__NMV_PROC_LIST_DIALOG_H__

