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

#ifndef __NMV_DEBUGGER_UTILS_H__
#define __NMV_DEBUGGER_UTILS_H__

#include <sstream>
#include "nmv-i-debugger.h"

NEMIVER_BEGIN_NAMESPACE (nemiver)
NEMIVER_BEGIN_NAMESPACE (debugger_utils)

void null_const_variable_slot (const IDebugger::VariableSafePtr &);

void null_const_variable_list_slot (const IDebugger::VariableList &);

void null_const_ustring_slot (const UString &);

void null_frame_vector_slot (const vector<IDebugger::Frame> &);

void null_frame_args_slot (const map<int, IDebugger::VariableList> &);

void dump_variable_value (IDebugger::VariableSafePtr a_var,
                          int a_indent_num,
                          std::ostringstream &a_os,
                          bool a_print_var_name = false);

void dump_variable_value (IDebugger::VariableSafePtr a_var,
                          int a_indent_num,
                          std::string &a_out_str);

IDebuggerSafePtr load_debugger_iface_with_confmgr ();

NEMIVER_END_NAMESPACE (debugger_utils)
NEMIVER_END_NAMESPACE (nemiver)

#endif // __NMV_DEBUGGER_UTILS_H__

