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
#ifndef __NMV_PROC_UTILS_H__
#define __NMV_PROC_UTILS_H__

#include <vector>
#include <glibmm.h>
#include "nmv-ustring.h"

namespace nemiver {
namespace common {

bool NEMIVER_API launch_program (const std::vector<UString> &a_args,
                                 int &a_pid,
                                 int &a_master_pty_fd,
                                 int &a_stdout_fd,
                                 int &a_stderr_fd,
                                 const std::vector<UString> &a_environments
                                     = std::vector<UString> ());

void NEMIVER_API attach_channel_to_loop_context_as_source
                        (Glib::IOCondition a_cond,
                         const sigc::slot<bool, Glib::IOCondition> &a_slot,
                         const Glib::RefPtr<Glib::IOChannel> &a_chan,
                         const Glib::RefPtr<Glib::MainContext>&a_ctxt);

bool NEMIVER_API is_libtool_executable_wrapper (const UString &a_path);
}//end namspace common
}//end namespace nemiver
#endif //__NMV_PROC_UTILS_H__
