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

#include "nmv-debugger-utils.h"
#include "common/nmv-exception.h"

NEMIVER_BEGIN_NAMESPACE (nemiver)
NEMIVER_BEGIN_NAMESPACE (debugger_utils)

/// \name Definitions of no-op callback slots that can used by client
/// code.
/// 
/// @{
void
null_const_variable_slot (const IDebugger::VariableSafePtr &)
{
}

void
null_const_variable_list_slot (const IDebugger::VariableList &)
{
}

void
null_const_ustring_slot (const UString &)
{
}

void
null_frame_vector_slot (const vector<IDebugger::Frame> &)
{
}

void
null_frame_args_slot (const map<int, IDebugger::VariableList> &)
{
}

void
null_default_slot ()
{
}

void
null_disass_slot (const common::DisassembleInfo &,
                  const std::list<common::Asm> &)
{
}

void
null_breakpoints_slot (const map<int, IDebugger::Breakpoint>&)
{
}
///@}

/// Generate a string of of white spaces.
/// \param a_nb_ws the number of white spaces to generate
/// \param a_ws_str the output string the white spaces are
///  generated into.
void
gen_white_spaces (int a_nb_ws,
                  std::string &a_ws_str)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    for (int i = 0; i < a_nb_ws; i++) {
        a_ws_str += ' ';
    }
}

/// Serialize a variable and its value into an output stream.
/// \param a_var the variable to serialize.
/// \param a_indent_num the number of spaces to indent to before
/// serializing the variable.
/// \param a_os the output stream to serialize into.
/// \param a_print_var_name if true, serialize the variable name too.
void
dump_variable_value (IDebugger::VariableSafePtr a_var,
                     int a_indent_num,
                     std::ostringstream &a_os,
                     bool a_print_var_name)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    THROW_IF_FAIL (a_var);

    std::string ws_string;

    if (a_indent_num)
        gen_white_spaces (a_indent_num, ws_string);

    if (a_print_var_name)
        a_os << ws_string << a_var->name ();

    if (!a_var->members ().empty ()) {
        a_os << "\n"  << ws_string << "{";
        IDebugger::VariableList::const_iterator it;
        for (it = a_var->members ().begin ();
             it != a_var->members ().end ();
             ++it) {
            a_os << "\n";
            dump_variable_value (*it, a_indent_num + 2, a_os, true);
        }
        a_os << "\n" << ws_string <<  "}";
    } else {
        if (a_print_var_name)
            a_os << " = ";
        a_os << a_var->value ();
    }
}

/// Serialize a variable and its value into a string.
/// \param a_var the variable to serialize.
/// \param a_indent_num the number of spaces to indent to before
/// serializing the variable.
/// \param a_out_str the string to serialize the variable into.
void
dump_variable_value (IDebugger::VariableSafePtr a_var,
                     int a_indent_num,
                     std::string &a_out_str)
{
    std::ostringstream os;
    dump_variable_value (a_var, a_indent_num, os);
    a_out_str = os.str ();
}

/// Load the debugger interface using the default
/// DynamicModuleManager, and initialize it with the configuration
/// manager.
/// 
/// \return the IDebuggerSafePtr
IDebuggerSafePtr
load_debugger_iface_with_confmgr ()
{
    IConfMgrSafePtr confmgr;

    IDebuggerSafePtr debugger = 
        load_iface_and_confmgr<IDebugger> ("gdbengine",
                                           "IDebugger",
                                           confmgr);
    confmgr->register_namespace (/* default nemiver namespace */);
    debugger->do_init (confmgr);
    return debugger;
}

NEMIVER_END_NAMESPACE (debugger_utils)
NEMIVER_END_NAMESPACE (nemiver)
