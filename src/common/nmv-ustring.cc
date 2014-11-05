/* -*- Mode: C++; indent-tabs-mode:nil; c-basic-offset: 4-*- */

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
#include "config.h"
#include <cstring>
#include <sstream>
#include "nmv-ustring.h"
#include "nmv-safe-ptr-utils.h"
#include "nmv-log-stream-utils.h"

using namespace std;

NEMIVER_BEGIN_NAMESPACE (nemiver)
NEMIVER_BEGIN_NAMESPACE (common)

#if defined(__FreeBSD__) || defined(__OpenBSD__)
int
strnlen (const gchar *string, gulong a_len)
{
  gchar *pos = (gchar*) memchr ((void*)string, '\0', a_len);
  if (pos)
    return (pos - string);
  else
    return (a_len);
}
#endif

UString
UString::from_int (long long an_int)
{
    UString str;
    ostringstream os;
    os << an_int;
    str = os.str ().c_str ();
    return str;
}

size_t
UString::hexa_to_int (const string &a_hexa_str)
{
    return strtoll (a_hexa_str.c_str (), 0, 16);
}

UString::UString ()
{
}

UString::UString (const char *a_cstr, long a_len)
{
    if (!a_cstr) {
        Glib::ustring::operator= ("");
    } else {
        if (a_len < 0)
            Glib::ustring::operator= (a_cstr);
        else
            Glib::ustring::assign (a_cstr, a_len);
    }
}

UString::UString (const unsigned char *a_cstr, long a_len)
{
    const char* cstr = reinterpret_cast<const char*> (a_cstr);
    if (!cstr) {
        Glib::ustring::operator= ("");
    } else {
        if (a_len < 0)
            Glib::ustring::operator= (cstr);
        else
            Glib::ustring::assign (cstr, a_len);
    }
}

UString::UString (UString const &an_other_string): Glib::ustring (an_other_string)
{}

UString::UString (const Glib::ustring &an_other_string) :
        Glib::ustring (an_other_string)
{}

UString::UString (const string &an_other_string) :
    Glib::ustring (an_other_string)
{}

UString::~UString ()
{}

UString&
UString::set (const gchar* a_buf, gulong a_len)
{
    if (!a_buf) {
        return *this;
    }
    //truncate the length of string if it contains
    //zeros, otherwise, Glib::ustring throws an exception.
    gulong length_to_first_zero = strnlen (a_buf, a_len);
    gulong len = a_len;
    if (length_to_first_zero != a_len) {
        len = length_to_first_zero;
    }
    Glib::ustring::assign (a_buf, len);
    return *this;
}

bool
UString::is_integer () const
{
    UString::value_type c (0);
    if (*this == "")
        return false;

    for (UString::size_type i = 0; i < size (); ++i) {
        c = (*this)[i];
        if (c < '0' || c > '9') {
            return false;
        }
    }
    return true;
}

UString&
UString::append_int (long long an_int)
{
    this->operator+= (from_int (an_int));
    return *this;
}

UString&
UString::assign_int (long long an_int)
{
    this->operator= (from_int (an_int));
    return *this;
}

UString&
UString::operator= (const char *a_cstr)
{
    if (!a_cstr) {
        Glib::ustring::operator= ("");
    } else {
        Glib::ustring::operator= (a_cstr);
    }
    return *this;
}

UString&
UString::operator= (const unsigned char *a_cstr)
{
    return operator= (reinterpret_cast<const char*> (a_cstr));
}

UString&
UString::operator= (UString const &a_cstr)
{
    if (this == &a_cstr)
        return *this;
    Glib::ustring::operator= (a_cstr);
    return *this;
}

bool
UString::operator! () const
{
    if (*this == ("") || this->empty ()) {
        return true;
    }
    return false;
}

template<class string_container>
string_container
split_base (const UString &a_string, const UString &a_delim)
{
    string_container result;
    if (a_string.size () == Glib::ustring::size_type (0)) {return result;}

    gint len = a_string.bytes () + 1;
    CharSafePtr buf (new gchar[len]);
    memset (buf.get (), 0, len);
    memcpy (buf.get (), a_string.c_str (), a_string.bytes ());

    gchar **splited = g_strsplit (buf.get (), a_delim.c_str (), -1);
    try {
        for (gchar **cur = splited; cur && *cur; ++cur) {
            result.push_back (UString (*cur));
        }
    } catch (...) {
    }

    if (splited) {
        g_strfreev (splited);
    }
    return result;
}

vector<UString>
UString::split (const UString &a_delim) const
{
    return split_base<vector<UString> > (*this, a_delim);
}

list<UString>
UString::split_to_list (const UString &a_delim) const
{
    return split_base<list<UString> > (*this, a_delim);
}

vector<UString>
UString::split_set (const UString &a_delim_set) const
{
    vector<UString> result;
    if (size () == Glib::ustring::size_type (0)) {return result;}

    gint len = bytes () + 1;
    CharSafePtr buf (new gchar[len]);
    memset (buf.get (), 0, len);
    memcpy (buf.get (), c_str (), bytes ());

    gchar **splited = g_strsplit_set (buf.get (), a_delim_set.c_str (), -1);
    try {
        for (gchar **cur = splited; cur && *cur; ++cur) {
            result.push_back (UString (*cur));
        }
    } catch (...) {
    }

    if (splited) {
        g_strfreev (splited);
    }
    return result;
}

UString
UString::join (const vector<UString> &a_elements,
               const UString &a_delim)
{
    if (!a_elements.size ()) {
        return UString ("");
    }
    vector<UString>::const_iterator from = a_elements.begin ();
    vector<UString>::const_iterator to = a_elements.end ();
    return join (from, to, a_delim);
}

UString
UString::join (vector<UString>::const_iterator &a_from,
               vector<UString>::const_iterator &a_to,
               const UString &a_delim)
{
    if (a_from == a_to) {return UString ("");}

    vector<UString>::const_iterator iter = a_from;
    UString result = *iter;
    for (; ++iter != a_to; ) {
        result += a_delim + *iter;
    }
    return result;
}

void
UString::chomp ()
{
    if (!size ()) {return;}

    Glib::ustring::size_type i = 0;

    //remove the ws from the beginning of the string.
    while (!empty () && isspace (at (0))) {
        erase (0, 1);
    }

    //remove the ws from the end of the string.
    i = size ();
    if (!i) {return;}
    --i;
    while (i > 0 && isspace (at (i))) {
        erase (i, 1);
        i = size ();
        if (!i) {return;}
        --i;
    }
    if (i == 0 && isspace (at (i))) {erase (0, 1);}
}

UString::size_type
UString::get_number_of_lines () const
{
    UString::size_type res = 0;
    for (UString::const_iterator it = begin (); it != end () ; ++it) {
        if (*it == '\n') {++res;}
    }
    return res;
}

UString::size_type
UString::get_number_of_words () const
{
    UString::size_type i=0, num_words=0;

skip_blanks:
    for (;i < raw ().size (); ++i) {
        if (!isblank (raw ()[i]))
            goto eat_word;
    }
    goto out;

eat_word:
    num_words++;
    for (; i < raw ().size (); ++i) {
        if (isblank (raw ()[i]))
            goto skip_blanks;
    }

out:
    return num_words;
}

UString&
UString::vprintf (const UString &a_format, va_list a_args)
{
    GCharSafePtr str (g_strdup_vprintf (a_format.c_str (), a_args));
    assign (str.get ());
    return *this;
}

UString&
UString::printf (const char *a_format, ...)
{
    va_list args;
    va_start (args, a_format);
    this->vprintf (a_format, args);
    va_end (args);
    return *this;
}

WString::~WString ()
{
}

WString::WString () : super_type ()
{
}

WString::WString (const super_type &a_str) : super_type (a_str)
{
}

WString::WString (const char* a_str, unsigned int a_len)
{
    if (!a_str) {
        assign ("");
    } else {
        assign (a_str, a_len);
    }
}

WString::WString (const super_type::allocator_type &a) : super_type (a)
{
}

WString::WString (const WString &str) : super_type (str)
{
}

WString::WString (const WString &str,
                  size_type position,
                  size_type n) :
    super_type (str, position, n)
{
}

WString::WString (const WString &str, size_type position,
                  size_type n,
                  const super_type::allocator_type &a) :
    super_type (str, position, n, a)
{
}

WString::WString (const gunichar *s,
                  super_type::size_type n,
                  const super_type::allocator_type &a) :
    super_type (s, n, a)

{
}

WString::WString (const gunichar *s,
                  const super_type::allocator_type &a) :
    super_type (s, a)
{
}

WString::WString (size_type n,
                  gunichar c,
                  const super_type::allocator_type &a) :
    super_type (n, c, a)
{
}

WString&
WString::assign (const char *a_str, long a_len)
{
    if (!a_str) {
        static gunichar s_empty_str[]={0};
        super_type::assign (s_empty_str);
    } else {
        if (a_len <0) {
            a_len = strlen (a_str);
        }
        if (a_len) {
            if ((long)capacity () < a_len) {resize (a_len);}
            for (long i=0; i < a_len; ++i) {
                at (i) = a_str[i];
            }
        }
    }
    return *this;
}

WString&
WString::assign (const WString &a_str)
{
    super_type::assign (a_str);
    return *this;
}

WString&
WString::assign (const WString &a_str, size_type a_position,
                 super_type::size_type a_n)
{
    super_type::assign ((super_type)a_str, a_position, a_n);
    return *this;
}

WString&
WString::assign (const gunichar *a_str,
                 super_type::size_type a_n)
{
    super_type::assign (a_str, a_n);
    return *this;
}

WString&
WString::assign (const gunichar *a_str)
{
    super_type::assign (a_str);
    return *this;
}

WString&
WString::assign (super_type::size_type a_n, gunichar a_c)
{
    super_type::assign (a_n, a_c);
    return *this;
}

bool
wstring_to_ustring (const WString &a_wstr,
                    UString &a_ustr)
{
    glong wstr_len=0, utf8_bytes_len=0;
    GCharSafePtr utf8_buf;
    GError *err=0;
    utf8_buf.reset (g_ucs4_to_utf8 (a_wstr.c_str (),
                                    a_wstr.size (), &wstr_len,
                                    &utf8_bytes_len, &err));
    GErrorSafePtr error;
    error.reset (err);
    if (error) {
        LOG_ERROR ("got error conversion error: '" << error->message << "'");
        return false;
    }

    if (!utf8_bytes_len && a_wstr.size ()) {
        LOG_ERROR ("Conversion from ucs4 str to utf8 str failed.");
        return false;
    }
    a_ustr.assign (utf8_buf.get (), wstr_len);
    return true;
}

bool
ustring_to_wstring (const UString &a_ustr,
                    WString &a_wstr)
{
    glong wstr_len=0, utf8_bytes_len=0;
    SafePtr<gunichar, DefaultRef, FreeUnref> wbuf;
    GError *err=0;
    wbuf.reset (g_utf8_to_ucs4 (a_ustr.c_str (),
                                a_ustr.bytes (),
                                &utf8_bytes_len,
                                &wstr_len,
                                &err));
    GErrorSafePtr error;
    error.reset (err);
    if (error) {
        LOG_ERROR ("got error conversion error: '" << error->message << "'");
        return false;
    }
    if (!wstr_len && a_ustr.bytes ()) {
        LOG_ERROR ("Conversion from utf8 str to ucs4 str failed");
        return false;
    }

    if ((gulong)wstr_len != a_ustr.size ()) {
        LOG_ERROR ("Conversion from utf8 str to ucs4 str failed");
    }
    a_wstr.assign (wbuf.get (), wstr_len);
    return true;
}
NEMIVER_END_NAMESPACE (nemiver)
NEMIVER_END_NAMESPACE (common)

