/* -*- Mode: C++; indent-tabs-mode:nil; c-basic-offset: 4; -*- */

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
#ifndef __NMV_PARSING_UTILS_H__
#define __NMV_PARSING_UTILS_H__

#include <glibmm.h>
#include "nmv-ustring.h"

namespace nemiver {
namespace common {

class UString;
namespace parsing_utils
{

NEMIVER_API bool is_digit (gunichar a_char);
NEMIVER_API bool is_alphabet_char (gunichar a_char);
NEMIVER_API bool is_alnum (gunichar a_char);
NEMIVER_API bool is_host_name_char (gunichar a_char);

NEMIVER_API bool remove_white_spaces_at_begining (const UString &a_str,
                                                  UString &a_res);

NEMIVER_API bool remove_white_spaces_at_end (const UString &a_str,
                                             UString &a_res);

NEMIVER_API bool is_white_string (const UString &a_str);

NEMIVER_API UString date_to_string (const Glib::Date &a_date);

NEMIVER_API int month_to_int (Glib::Date::Month a_month);

NEMIVER_API Glib::Date::Month month_from_int (int a_in);

NEMIVER_API bool string_to_date (const UString &a_str, Glib::Date &a_date);

}//end namespace parsing_utils
}//end namespace common
}//end namespace nemiver

#endif //__NMV_PARSING_UTILS_H__

