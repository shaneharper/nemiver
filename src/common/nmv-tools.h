/* -*- Mode: C++; indent-tabs-mode:nil; c-basic-offset:4 -*- */

/*
 *This file is part of the Nemiver Project.
 *
 *Dodji is free software; you can redistribute
 *it and/or modify it under the terms of
 *the GNU General Public License as published by the
 *Free Software Foundation; either version 2,
 *or (at your option) any later version.
 *
 *Dodji is distributed in the hope that it will
 *be useful, but WITHOUT ANY WARRANTY;
 *without even the implied warranty of
 *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *See the GNU General Public License for more details.
 *
 *You should have received a copy of the
 *GNU General Public License along with Dodji;
 *see the file COPYING.
 *If not, write to the Free Software Foundation,
 *Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *See COPYRIGHT file copyright information.
 */

#include <iosfwd>
#include "nmv-exception.h"
#include "nmv-transaction.h"

#ifndef __NMV_TOOLS_H__
#define __NMV_TOOLS_H__

using namespace std;

namespace nemiver {
namespace common {
namespace tools {

bool NEMIVER_API execute_sql_command_file (const common::UString &a_sql_cmd_file,
                                           Transaction &a_trans,
                                           ostream &a_ostream,
                                           bool stop_at_first_error=false);

bool NEMIVER_API execute_sql_commands_from_istream (istream &a_istream,
                                                    Transaction &a_trans,
                                                    ostream &a_ostream,
                                                    bool stop_at_first_err=false);

bool NEMIVER_API execute_one_statement (const common::UString &a_sql_string,
                                        Transaction &a_trans,
                                        ostream &a_ostream);
}//end namespace tools
}//end namespace common 
}//end namespace verissimus

#endif //__NMV_TOOL_H__

