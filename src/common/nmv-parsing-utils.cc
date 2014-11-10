/* -*- Mode: C++; indent-tabs-mode:nil; c-basic-offset: 4;  -*- */

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
#include "config.h"
#include <ctype.h>
#include <vector>
#include "nmv-exception.h"
#include "nmv-ustring.h"
#include "nmv-parsing-utils.h"

using namespace std;

namespace nemiver {
namespace common {
namespace parsing_utils {

bool
is_digit (gunichar a_char)
{
    return (a_char >= '0' && a_char <= '9');
}

bool
is_alphabet_char (gunichar a_char)
{
    return (a_char >= 'a' && a_char <= 'z')
            || (a_char >= 'A' && a_char <= 'Z');
}

bool
is_alnum (gunichar a_char)
{
    return is_alphabet_char (a_char)
            || is_digit (a_char)
            || a_char == '_'
            || a_char == '-';
}

bool
is_host_name_char (gunichar a_char)
{
    return is_digit (a_char)
            || is_alnum (a_char)
            || a_char == '.'
            || a_char == '_'
            || a_char == '-';
}

bool
remove_white_spaces_at_begining (const UString &a_str, UString &a_res)
{
    if (a_str == "")
        return false;

    a_res = "";
    UString::const_iterator it = a_str.begin ();
    for (;;) {
        if (isspace (*it)) {
            ++it;
        } else if (it == a_str.end ()) {
            return true;
        } else {
            break;
        }
    }
    if (it == a_str.end ())
        return true;
    for (;;) {
        a_res += *it;
        ++it;
        if (it == a_str.end ())
            return true;
    }
    return true;
}

bool
remove_white_spaces_at_end (const UString &a_str, UString &a_res)
{
    if (a_str == "") {
        return false;
    }
    a_res = "";
    unsigned int i = a_str.size () - 1;
    if (i==0)
        return false;

    while (isspace (a_str[i])) {
        --i;
        if (i==0)
            return true;
    }

    if (i==0)
        return true;

    for (;;) {
        a_res.insert (a_res.begin(), a_str[i]);
       ;
        if (i == 0)
            return true;
        --i;
    }
    return true;
}

bool
is_white_string (const UString &a_str)
{
    for (UString::const_iterator it=a_str.begin ();
            it != a_str.end ();
            ++it) {
        if (!isspace (*it))
            return false;
    }
    return true;
}

Glib::Date::Month
month_from_int (int a_in)
{
    switch (a_in) {
        case 1:
            return Glib::Date::JANUARY;
        case 2:
            return Glib::Date::FEBRUARY;
        case 3:
            return Glib::Date::MARCH;
        case 4:
            return Glib::Date::APRIL;
        case 5:
            return Glib::Date::MAY;
        case 6:
            return Glib::Date::JUNE;
        case 7:
            return Glib::Date::JULY;
        case 8:
            return Glib::Date::AUGUST;
        case 9:
            return Glib::Date::SEPTEMBER;
        case 10:
            return Glib::Date::OCTOBER;
        case 11:
            return Glib::Date::NOVEMBER;
        case 12:
            return Glib::Date::DECEMBER;
        default:
            return Glib::Date::BAD_MONTH;
    }
}

int
month_to_int (Glib::Date::Month a_month)
{
    switch (a_month) {
        case Glib::Date::JANUARY:
            return 1;
        case Glib::Date::FEBRUARY:
            return 2;
        case Glib::Date::MARCH:
            return 3;
        case Glib::Date::APRIL:
            return 4;
        case Glib::Date::MAY:
            return 5;
        case Glib::Date::JUNE:
            return 6;
        case Glib::Date::JULY:
            return 7;
        case Glib::Date::AUGUST:
            return 8;
        case Glib::Date::SEPTEMBER:
            return 9;
        case Glib::Date::OCTOBER:
            return 10;
        case Glib::Date::NOVEMBER:
            return 11;
        case Glib::Date::DECEMBER:
            return 12;
        default:
            THROW ("unawaited month value: "
                   + UString::from_int (a_month));
    }
    THROW ("we shouldn't reach this point.");
    return -1;
}

UString
date_to_string (const Glib::Date &a_date)
{
    UString result (UString::from_int (a_date.get_year ()));
    result += '-';
    UString month (UString::from_int (month_to_int (a_date.get_month ())));
    if (month.size () == 1) {
        month.insert (month.begin (), '0');
    }
    result += month + '-';
    UString day (UString::from_int (a_date.get_day ()));
    if (day.size () == 1) {
        day.insert (day.begin () , '0');
    }
    result += day;
    return result;
}

bool
string_to_date (const UString &a_str, Glib::Date &a_date)
{
    UString::size_type cur (0), start (0), end (0);
    vector<int> date_parts;

    start = cur;
    for (;;) {
        if (date_parts.size () == 3) break;
        if (a_str[cur] == '-' || a_str[cur] == ' ' || cur >= a_str.size ()) {
            end = cur;
            date_parts.push_back
                (atoi (a_str.substr (start, end - start).c_str ()));
            start = cur + 1;
        }
        cur++;
    }
    if (date_parts.size () != 3) return false;
    a_date.set_year (date_parts[0]);
    a_date.set_month (month_from_int (date_parts[1]));
    a_date.set_day (date_parts[2]);
    return true;
}


}//end namespace parsing_utils
}//end namespace common
}//end namespace nemiver

