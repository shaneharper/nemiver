/* -*- Mode: C++; indent-tabs-mode:nil; c-basic-offset: 4-*- */

/*Copyright (c) 2005-2009 Dodji Seketeli
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
#ifndef __NMV_USTRING_H__
#define __NMV_USTRING_H__

#include <vector>
#include <string>
#include <glibmm.h>
#include "nmv-namespace.h"
#include "nmv-api-macros.h"

using namespace std;

NEMIVER_BEGIN_NAMESPACE (nemiver)
NEMIVER_BEGIN_NAMESPACE (common)

class WString;
class NEMIVER_API UString: public Glib::ustring {

public:
    UString ();
    UString (const char *a_cstr, long a_len=-1);
    UString (const unsigned char *a_cstr, long a_len=-1);
    UString (const Glib::ustring &an_other_string);
    UString (const string &an_other_string);
    UString (UString const &an_other_string);
    virtual ~UString ();
    UString& set (const gchar* a_buf, gulong a_len);
    static UString from_int (long long an_int);
    static size_t hexa_to_int (const string &a_hexa_str);
    bool is_integer () const;
    UString& append_int (long long an_int);
    UString& assign_int (long long);
    UString& operator= (const char *a_cstr);
    UString& operator= (const unsigned char *a_cstr);
    UString& operator= (UString const &a_cstr);
    bool operator! () const;
    vector<UString> split (const UString &a_delim) const;
    list<UString> split_to_list (const UString &a_delim) const;
    vector<UString> split_set (const UString &a_delim_set) const;
    static UString join (const vector<UString> &a_elements,
                         const UString &a_delim=" ");
    static UString join (vector<UString>::const_iterator &a_from,
                         vector<UString>::const_iterator &a_to,
                         const UString &a_delim=" ");
    void chomp ();

    UString::size_type get_number_of_lines () const;

    UString::size_type get_number_of_words () const;

    UString& printf (const UString &a_format, ...);

    UString& vprintf (const UString &a_format, va_list a_args);
};//end class UString

class NEMIVER_API WString : public basic_string<gunichar> {

typedef basic_string<gunichar> super_type;

public:

    WString ();

    ~WString ();

    WString (const super_type &a_str);

    WString (const char* a_str, unsigned int a_len=-1);

    WString (const super_type::allocator_type &a);

    WString (const WString &str);

    WString (const WString &str,
             size_type position,
             size_type n = npos);

    WString (const WString &str, size_type position,
             size_type n,
             const super_type::allocator_type &a);

    WString (const gunichar *s,
             super_type::size_type n,
             const super_type::allocator_type &a=super_type::allocator_type());

    WString (const gunichar *s,
             const super_type::allocator_type &a=super_type::allocator_type());

    WString (size_type n,
             gunichar c,
             const super_type::allocator_type &a=super_type::allocator_type());

    WString& assign (const char *a_str, long a_len=-1);

    WString&  assign (const WString &a_str);

    WString&  assign (const WString &a_str, size_type a_position,
                       super_type::size_type a_n);

    WString&  assign (const gunichar *a_str,
                       super_type::size_type a_n);

    WString&  assign (const gunichar *a_str);

    WString&  assign (super_type::size_type a_n, gunichar a_c);
};//end WString

bool NEMIVER_API wstring_to_ustring (const WString &a_wstr,
                                     UString &a_ustr);
bool NEMIVER_API ustring_to_wstring (const UString &a_ustr,
                                     WString &a_wstr);

NEMIVER_END_NAMESPACE (common)
NEMIVER_END_NAMESPACE (nemiver)

#endif

