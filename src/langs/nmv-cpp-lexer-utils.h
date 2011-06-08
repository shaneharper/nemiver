/*
 *This file is part of the Nemiver Project.
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
#ifndef __NMV_CPP_LEXER_UTILS_H__
#define __NMV_CPP_LEXER_UTILS_H__

#include <iosfwd>
#include "nmv-cpp-lexer.h"

NEMIVER_BEGIN_NAMESPACE (nemiver)
NEMIVER_BEGIN_NAMESPACE (cpp)

bool token_type_as_string (const Token&, std::string&);
bool token_as_string (const Token&, std::string&);
std::ostream& operator<< (std::ostream &, const Token&);


NEMIVER_END_NAMESPACE (cpp)
NEMIVER_END_NAMESPACE (nemiver)

#endif //__NMV_CPP_LEXER_UTILS_H__

