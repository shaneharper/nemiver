/* -*- Mode: C++; indent-tabs-mode:nil; c-basic-offset:4; -*- */

/*Copyright (c) 2005-2006 Dodji Seketeli
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies
 * or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS",
 * WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#include <cstring>
#include "nmv-ustring.h"
#include "nmv-date-utils.h"

namespace nemiver {
namespace common {
namespace dateutils {

time_t
get_current_datetime ()
{
    struct timeval tv;
    memset (&tv, 0, sizeof (tv));
    gettimeofday (&tv, 0);
    return tv.tv_sec;
}

void
get_current_datetime (struct tm &a_tm)
{
    time_t now = get_current_datetime ();
    gmtime_r (&now, &a_tm);
}

void
get_current_datetime (UString &a_datetime)
{
    struct tm now;
    memset (&now, 0, sizeof (now));

    get_current_datetime (now);
    char now_str[21] = {0};
    strftime (now_str, 20, "%Y-%m-%d %H:%M:%S", &now);
    a_datetime = now_str;
}

}//end namespace dateutils
}//end namespace common 
}//end namespace nemiver
