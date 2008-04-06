//Author: Dodji Seketeli
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
#include <cstring>
#include <sstream>
#include "nmv-gdbmi-parser.h"

using nemiver::common::UString;

static const UString GDBMI_PARSING_DOMAIN = "gdbmi-parsing-domain";
static const UString GDBMI_OUTPUT_DOMAIN = "gdbmi-output-domain";

#define LOG_PARSING_ERROR(a_buf, a_from) \
{ \
Glib::ustring str_01 (a_buf, (a_from), a_buf.size () - (a_from));\
LOG_ERROR ("parsing failed for buf: >>>" \
             << a_buf << "<<<" \
             << " cur index was: " << (int)(a_from)); \
}

#define CHECK_END(a_input, a_current, a_end) \
if ((a_current) >= (a_end)) {\
LOG_ERROR ("hit end index " << (int) a_end); \
return false;\
}

#define CHECK_END_BREAK(a_input, a_current, a_end) \
if ((a_current) >= (a_end)) {\
LOG_ERROR ("hit end index " << (int) a_end); \
G_BREAKPOINT();\
}

#define SKIP_WS(a_input, a_from, a_to) \
while (a_from < a_input.bytes () && isspace (a_input.c_str ()[a_from])) { \
    CHECK_END (a_input, a_from, end);++a_from; \
} \
a_to = a_from;

#define SKIP_BLANK(a_input, a_from, a_to) \
while (a_from < a_input.bytes () && isblank (a_input.c_str ()[a_from])) { \
    CHECK_END (a_input, a_from, end);++a_from; \
} \
a_to = a_from;

using namespace std;
using namespace nemiver::common;

NEMIVER_BEGIN_NAMESPACE (nemiver)

// prefixes of command output records.
const char* PREFIX_DONE = "^done";
const char* PREFIX_RUNNING = "^running";
const char* PREFIX_EXIT = "^exit";
const char* PREFIX_CONNECTED = "^connected";
const char* PREFIX_ERROR = "^error";
const char* PREFIX_BKPT = "bkpt={";
const char* PREFIX_BREAKPOINT_TABLE = "BreakpointTable={";
const char* PREFIX_THREAD_IDS = "thread-ids={";
const char* PREFIX_NEW_THREAD_ID = "new-thread-id=\"";
const char* PREFIX_FILES = "files=[";
const char* PREFIX_STACK = "stack=[";
const char* PREFIX_FRAME = "frame={";
const char* PREFIX_DEPTH = "depth=\"";
const char* PREFIX_STACK_ARGS = "stack-args=[";
const char* PREFIX_LOCALS = "locals=[";
const char* PREFIX_VALUE = "value=\"";
const char* PREFIX_REGISTER_NAMES = "register-names=";
const char* PREFIX_CHANGED_REGISTERS = "changed-registers=";
const char* PREFIX_REGISTER_VALUES = "register-values=";
const char* PREFIX_MEMORY_VALUES = "addr=";

bool parse_c_string_body (const UString &a_input,
                          UString::size_type a_from,
                          UString::size_type &a_to,
                          UString &a_string);

static bool is_string_start (gunichar a_c);

bool
parse_attribute (const UString &a_input,
                 UString::size_type a_from,
                 UString::size_type &a_to,
                 UString &a_name,
                 UString &a_value)
{
    UString::size_type cur = a_from, end = a_input.size ();
    if (cur >= end || !is_string_start (a_input.c_str ()[cur])) {return false;}

    GDBMIResultSafePtr result;
    if (!parse_gdbmi_result (a_input, cur, a_to, result)
        || !result
        || !result->value ()
        || result->value ()->content_type () != GDBMIValue::STRING_TYPE) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    a_name = result->variable ();
    a_value = result->value ()->get_string_content ();
    return true;
}

/// \brief parses an attribute list
///
/// An attribute list has the form:
/// attr0="val0",attr1="bal1",attr2="val2"
///TODO: this is way too hackish. Rewrite this
/// using the direct GDBMI datastructures
bool
parse_attributes (const UString &a_input,
                  UString::size_type a_from,
                  UString::size_type &a_to,
                  map<UString, UString> &a_attrs)
{
    UString::size_type cur = a_from, end = a_input.size ();

    if (cur == end) {return false;}
    UString name, value;
    map<UString, UString> attrs;

    while (true) {
        if (!parse_attribute (a_input, cur, cur, name, value)) {break;}
        if (!name.empty () && !value.empty ()) {
            attrs[name] = value;
            name.clear (); value.clear ();
        }

        while (isspace (a_input[cur])) {++cur;}
        if (cur >= end || a_input[cur] != ',') {break;}
        if (++cur >= end) {break;}
    }
    a_attrs = attrs;
    a_to = cur;
    return true;
}

/// \brief parses a breakpoint definition as returned by gdb.
///
///breakpoint definition string looks like this:
///bkpt={number="3",type="breakpoint",disp="keep",enabled="y",
///addr="0x0804860e",func="func2()",file="fooprog.cc",line="13",times="0"}
///
///\param a_input the input string to parse.
///\param a_from where to start the parsing from
///\param a_to out parameter. A past the end iterator that
/// point the the end of the parsed text. This is set if and only
/// if the function completes successfuly
/// \param a_output the output datatructure filled upon parsing.
/// \return true in case of successful parsing, false otherwise.
bool
parse_breakpoint (const UString &a_input,
                  Glib::ustring::size_type a_from,
                  Glib::ustring::size_type &a_to,
                  IDebugger::BreakPoint &a_bkpt)
{
    LOG_FUNCTION_SCOPE_NORMAL_D (GDBMI_PARSING_DOMAIN);

    Glib::ustring::size_type cur = a_from, end = a_input.size ();

    if (a_input.compare (cur, strlen (PREFIX_BKPT), PREFIX_BKPT)) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    cur += 6;
    if (cur >= end) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    map<UString, UString> attrs;
    bool is_ok = parse_attributes (a_input, cur, cur, attrs);
    if (!is_ok) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    if (a_input.c_str ()[cur] != '}') {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    ++cur;


    if (attrs["addr"] == "<PENDING>") {
        UString pending = attrs["pending"];
        if (pending == "") {
            LOG_PARSING_ERROR (a_input, cur);
            return false;
        }
        LOG_D ("got pending breakpoint: '" << pending << "'",
               GDBMI_OUTPUT_DOMAIN);
        vector<UString> str_tab = pending.split (":");
        LOG_D ("filepath: '" << str_tab[0] << "'", GDBMI_OUTPUT_DOMAIN);
        LOG_D ("linenum: '" << str_tab[1] << "'", GDBMI_OUTPUT_DOMAIN);
        if (str_tab.size () != 2) {
            LOG_PARSING_ERROR (a_input, cur);
            return false;
        }
        string path = Glib::locale_from_utf8 (str_tab[0]);
        if (Glib::path_is_absolute (path)) {
            attrs["file"] = Glib::locale_to_utf8
                                    (Glib::path_get_basename (path));
            attrs["fullname"] = Glib::locale_to_utf8 (path);
        } else {
            attrs["file"] = Glib::locale_to_utf8 (path);;
        }
        attrs["line"] = str_tab[1];
    }

    map<UString, UString>::iterator iter, null_iter = attrs.end ();
    //we use to require that the "fullname" property be present as
    //well, but it seems that a lot debug info set got shipped without
    //that property. Client code should do what they can with the
    //file property only.
    if (   (iter = attrs.find ("number"))  == null_iter
            || (iter = attrs.find ("type"))    == null_iter
            || (iter = attrs.find ("disp"))    == null_iter
            || (iter = attrs.find ("enabled")) == null_iter
            || (iter = attrs.find ("addr"))    == null_iter
            || (iter = attrs.find ("times"))   == null_iter
       ) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    a_bkpt.number (atoi (attrs["number"].c_str ()));
    if (attrs["enabled"] == "y") {
        a_bkpt.enabled (true);
    } else {
        a_bkpt.enabled (false);
    }
    a_bkpt.address (attrs["addr"]);
    if (!attrs["func"].empty ()) {
        a_bkpt.function (attrs["func"]);
    } else if (!attrs["what"].empty ()) {
        // catchpoints don't have a 'func' field, but they have a 'what' field
        // that says something like "Exception throw"
        a_bkpt.function (attrs["what"]);
    }
    a_bkpt.file_name (attrs["file"]); //may be nil
    a_bkpt.file_full_name (attrs["fullname"]); //may be nil
    a_bkpt.line (atoi (attrs["line"].c_str ())); //may be nil
    //TODO: get the 'at' attribute that is present on targets that
    //are not compiled with -g.
    a_to = cur;
    return true;
}

bool
parse_breakpoint_table (const UString &a_input,
                        UString::size_type a_from,
                        UString::size_type &a_to,
                        map<int, IDebugger::BreakPoint> &a_breakpoints)
{
    LOG_FUNCTION_SCOPE_NORMAL_D (GDBMI_PARSING_DOMAIN);
    UString::size_type cur=a_from, end=a_input.bytes ();

    if (a_input.compare (cur, strlen (PREFIX_BREAKPOINT_TABLE), PREFIX_BREAKPOINT_TABLE)) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    cur += strlen (PREFIX_BREAKPOINT_TABLE);
    if (cur >= end) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    //skip table headers and go to table body.
    cur = a_input.find ("body=[", 0) ;
    if (!cur) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    cur += 6;
    if (cur >= end) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    map<int, IDebugger::BreakPoint> breakpoint_table;
    if (a_input.c_str ()[cur] == ']') {
        //there are zero breakpoints ...
    } else if (!a_input.compare (cur, strlen (PREFIX_BKPT), PREFIX_BKPT)){
        //there are some breakpoints
        IDebugger::BreakPoint breakpoint;
        while (true) {
            if (a_input.compare (cur, strlen (PREFIX_BKPT), PREFIX_BKPT)) {break;}
            if (!parse_breakpoint (a_input, cur, cur, breakpoint)) {
                LOG_PARSING_ERROR (a_input, cur);
                return false;
            }
            breakpoint_table[breakpoint.number ()] = breakpoint;
            if (a_input[cur] == ',') {
                ++cur;
                if (cur >= end) {
                    LOG_PARSING_ERROR (a_input, cur);
                    return false;
                }
            }
            breakpoint.clear ();
        }
        if (breakpoint_table.empty ()) {
            LOG_PARSING_ERROR (a_input, cur);
            return false;
        }
    } else {
        //weird things are happening, get out.
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    if (a_input.c_str ()[cur] != ']') {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    ++cur;
    if (cur >= end) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    if (a_input.c_str ()[cur] != '}') {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    ++cur;

    a_to = cur;
    a_breakpoints = breakpoint_table;
    return true;
}

/// parse a GDB/MI Tuple is a actualy a set of name=value constructs,
/// where 'value' can be quite complicated.
/// Look at the GDB/MI output syntax for more.
bool
parse_gdbmi_tuple (const UString &a_input,
                   UString::size_type a_from,
                   UString::size_type &a_to,
                   GDBMITupleSafePtr &a_tuple)
{
    LOG_FUNCTION_SCOPE_NORMAL_D (GDBMI_PARSING_DOMAIN);
    UString::size_type cur = a_from, end = a_input.bytes ();
    CHECK_END (a_input, cur, end);

    if (a_input.c_str ()[cur] != '{') {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    ++cur;
    CHECK_END (a_input, cur, end);

    if (a_input.c_str ()[cur] == '}') {
        ++cur;
        a_to = cur;
        return true;
    }

    GDBMITupleSafePtr tuple;
    GDBMIResultSafePtr result;

    for (;;) {
        if (parse_gdbmi_result (a_input, cur, cur, result)) {
            THROW_IF_FAIL (result);
            SKIP_BLANK (a_input, cur, cur);
            CHECK_END (a_input, cur, end);
            if (!tuple) {
                tuple = GDBMITupleSafePtr (new GDBMITuple);
                THROW_IF_FAIL (tuple);
            }
            tuple->append (result);
            if (a_input.c_str ()[cur] == ',') {
                ++cur;
                CHECK_END (a_input, cur, end);
                SKIP_BLANK (a_input, cur, cur);
                continue;
            }
            if (a_input.c_str ()[cur] == '}') {
                ++cur;
            }
        } else {
            LOG_PARSING_ERROR (a_input, cur);
            return false;
        }
        LOG_D ("getting out at char '"
               << (char)a_input.c_str ()[cur]
               << "', at offset '"
               << (int)cur
               << "' for text >>>"
               << a_input
               << "<<<",
               GDBMI_PARSING_DOMAIN);
        break;
    }

    SKIP_BLANK (a_input, cur, cur);
    a_to = cur;
    a_tuple = tuple;
    return true;
}

/// Parse a GDB/MI LIST. A list is either a list of GDB/MI Results
/// or a list of GDB/MI Values.
/// Look at the GDB/MI output syntax documentation for more.
bool
parse_gdbmi_list (const UString &a_input,
                  UString::size_type a_from,
                  UString::size_type &a_to,
                  GDBMIListSafePtr &a_list)
{
    LOG_FUNCTION_SCOPE_NORMAL_D (GDBMI_PARSING_DOMAIN);
    UString::size_type cur = a_from, end = a_input.bytes ();
    CHECK_END (a_input, cur, end);

    GDBMIListSafePtr return_list;
    if (a_input.c_str ()[cur] != '[') {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    CHECK_END (a_input, cur + 1, end);
    if (a_input.c_str ()[cur + 1] == ']') {
        a_list = GDBMIListSafePtr (new GDBMIList);
        cur += 2;
        a_to = cur;
        return true;
    }

    ++cur;
    CHECK_END (a_input, cur, end);
    SKIP_BLANK (a_input, cur, cur);

    GDBMIValueSafePtr value;
    GDBMIResultSafePtr result;
    if ((isalpha (a_input.c_str ()[cur]) || a_input.c_str ()[cur] == '_')
         && parse_gdbmi_result (a_input, cur, cur, result)) {
        CHECK_END (a_input, cur, end);
        THROW_IF_FAIL (result);
        return_list = GDBMIListSafePtr (new GDBMIList (result));
        for (;;) {
            if (a_input.c_str ()[cur] == ',') {
                ++cur;
                CHECK_END (a_input, cur, end);
                result.reset ();
                if (parse_gdbmi_result (a_input, cur, cur, result)) {
                    THROW_IF_FAIL (result);
                    return_list->append (result);
                    continue;
                }
            }
            break;
        }
    } else if (parse_gdbmi_value (a_input, cur, cur, value)) {
        CHECK_END (a_input, cur, end);
        THROW_IF_FAIL (value);
        return_list = GDBMIListSafePtr (new GDBMIList (value));
        for (;;) {
            if (a_input.c_str ()[cur] == ',') {
                ++cur;
                CHECK_END (a_input, cur, end);
                value.reset ();
                if (parse_gdbmi_value (a_input, cur, cur, value)) {
                    THROW_IF_FAIL (value);
                    return_list->append (value);
                    continue;
                }
            }
            break;
        }
    } else {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    if (a_input.c_str ()[cur] != ']') {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    ++cur;

    a_to = cur;
    a_list = return_list;
    return true;
}

/// parse a GDB/MI Result data structure.
/// A result basically has the form:
/// variable=value.
/// Beware value is more complicated than what it looks like :-)
/// Look at the GDB/MI spec for more.
bool
parse_gdbmi_result (const UString &a_input,
                    UString::size_type a_from,
                    UString::size_type &a_to,
                    GDBMIResultSafePtr &a_value)
{
    LOG_FUNCTION_SCOPE_NORMAL_D (GDBMI_PARSING_DOMAIN);
    UString::size_type cur = a_from, end = a_input.bytes ();
    CHECK_END (a_input, cur, end);

    UString variable;
    if (!parse_string (a_input, cur, cur, variable)) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    CHECK_END (a_input, cur, end);
    SKIP_BLANK (a_input, cur, cur);
    if (a_input.c_str ()[cur] != '=') {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    LOG_D ("got gdbmi variable: " << variable, GDBMI_PARSING_DOMAIN);
    ++cur;
    CHECK_END (a_input, cur, end);

    GDBMIValueSafePtr value;
    if (!parse_gdbmi_value (a_input, cur, cur, value)) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    THROW_IF_FAIL (value);

    GDBMIResultSafePtr result (new GDBMIResult (variable, value));
    THROW_IF_FAIL (result);
    a_to = cur;
    a_value = result;
    return true;
}

/// \brief parse a GDB/MI value.
/// GDB/MI value type is defined as:
/// VALUE ==> CONST | TUPLE | LIST
/// CONSTis a string, basically. Look at parse_string() for more.
/// TUPLE is a GDB/MI tuple. Look at parse_tuple() for more.
/// LIST is a GDB/MI list. It is either  a list of GDB/MI Result or
/// a list of GDB/MI value. Yeah, that can be recursive ...
/// To parse a GDB/MI list, we use parse_list() defined above.
/// You can look at the GDB/MI output syntax for more.
/// \param a_input the input string to parse
/// \param a_from where to start parsing from
/// \param a_to (out parameter) a pointer to the current char,
/// after the parsing.
//// \param a_value the result of the parsing.
bool
parse_gdbmi_value (const UString &a_input,
                   UString::size_type a_from,
                   UString::size_type &a_to,
                   GDBMIValueSafePtr &a_value)
{
    LOG_FUNCTION_SCOPE_NORMAL_D (GDBMI_PARSING_DOMAIN);
    UString::size_type cur = a_from, end = a_input.bytes ();
    CHECK_END (a_input, cur, end);

    GDBMIValueSafePtr value;
    if (a_input.c_str ()[cur] == '"') {
        UString const_string;
        if (parse_c_string (a_input, cur, cur, const_string)) {
            value = GDBMIValueSafePtr (new GDBMIValue (const_string));
            LOG_D ("got str gdbmi value: '"
                    << const_string
                    << "'",
                   GDBMI_PARSING_DOMAIN);
        }
    } else if (a_input.c_str ()[cur] == '{') {
        GDBMITupleSafePtr tuple;
        if (parse_gdbmi_tuple (a_input, cur, cur, tuple)) {
            if (!tuple) {
                value = GDBMIValueSafePtr (new GDBMIValue ());
            } else {
                value = GDBMIValueSafePtr (new GDBMIValue (tuple));
            }
        }
    } else if (a_input.c_str ()[cur] == '[') {
        GDBMIListSafePtr list;
        if (parse_gdbmi_list (a_input, cur, cur, list)) {
            THROW_IF_FAIL (list);
            value = GDBMIValueSafePtr (new GDBMIValue (list));
        }
    } else {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    if (!value) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    a_value = value;
    a_to = cur;
    return true;
}

ostream&
operator<< (ostream &a_out, const GDBMIResultSafePtr &a_result)
{
    if (!a_result) {
        a_out << "<result nilpointer/>";
        return a_out;
    }
    a_out << "<result variable='";
    a_out << Glib::locale_from_utf8 (a_result->variable ()) << "'>";
    a_out << a_result->value ();
    a_out << "</result>";
    return a_out;
}

ostream&
operator<< (ostream &a_out, const GDBMITupleSafePtr &a_tuple)
{
    if (!a_tuple) {
        a_out << "<tuple nilpointer/>";
        return a_out;
    }
    list<GDBMIResultSafePtr>::const_iterator iter;
    a_out << "<tuple>" ;
    for (iter=a_tuple->content ().begin (); iter!=a_tuple->content ().end(); ++iter) {
        a_out  << *iter;
    }
    a_out  << "</tuple>";
    return a_out;
}

ostream&
operator<< (ostream &a_out, const GDBMIListSafePtr a_list)
{
    if (!a_list) {
        a_out << "<list nilpointer/>";
        return a_out;
    }
    if (a_list->content_type () == GDBMIList::RESULT_TYPE) {
        a_out << "<list type='result'>";
        list<GDBMIResultSafePtr>::const_iterator iter;
        list<GDBMIResultSafePtr> result_list;
        a_list->get_result_content (result_list);
        for (iter = result_list.begin ();
             iter != result_list.end ();
             ++iter) {
            a_out << *iter;
        }
        a_out << "</list>";
    } else if (a_list->content_type () == GDBMIList::VALUE_TYPE) {
        a_out << "<list type='value'>";
        list<GDBMIValueSafePtr>::const_iterator iter;
        list<GDBMIValueSafePtr> value_list;
        a_list->get_value_content (value_list);
        for (iter = value_list.begin ();
             iter != value_list.end ();
             ++iter) {
            a_out << *iter;
        }
        a_out << "</list>";
    } else {
        THROW_IF_FAIL ("assert not reached");
    }
    return a_out;
}

ostream&
operator<< (ostream &a_out, const GDBMIValueSafePtr &a_val)
{
    if (!a_val) {
        a_out << "<value nilpointer/>";
        return a_out;
    }

    switch (a_val->content_type ()) {
        case GDBMIValue::EMPTY_TYPE:
            a_out << "<value type='empty'/>";
            break;
        case GDBMIValue::TUPLE_TYPE :
            a_out << "<value type='tuple'>"
                  << a_val->get_tuple_content ()
                  << "</value>";
            break;
        case GDBMIValue::LIST_TYPE :
            a_out << "<value type='list'>\n"
                  << a_val->get_list_content ()<< "</value>";
            break;
        case GDBMIValue::STRING_TYPE:
            a_out << "<value type='string'>"
                  << Glib::locale_from_utf8 (a_val->get_string_content ())
                  << "</value>";
            break;
    }
    return a_out;
}

std::ostream&
operator<< (std::ostream &a_out, const IDebugger::Variable &a_var)
{
    a_out << "<variable>"
          << "<name>"<< a_var.name () << "</name>"
          << "<type>"<< a_var.type () << "</type>"
          << "<members>";

    if (!a_var.members ().empty ()) {
        list<IDebugger::VariableSafePtr>::const_iterator it;
        for (it = a_var.members ().begin ();
             it != a_var.members ().end ();
             ++it) {
            a_out << *(*it);
        }
    }
    a_out << "</members></variable>";
    return a_out;
}


/// \brief parses a function frame
///
/// function frames have the form:
/// frame={addr="0x080485fa",func="func1",args=[{name="foo", value="bar"}],
/// file="fooprog.cc",fullname="/foo/fooprog.cc",line="6"}
///
/// \param a_input the input string to parse
/// \param a_from where to parse from.
/// \param a_to out parameter. Where the parser went after the parsing.
/// \param a_frame the parsed frame. It is set if and only if the function
///  returns true.
/// \return true upon successful parsing, false otherwise.
bool
parse_frame (const UString &a_input,
             UString::size_type a_from,
             UString::size_type &a_to,
             IDebugger::Frame &a_frame)
{
    LOG_FUNCTION_SCOPE_NORMAL_D (GDBMI_PARSING_DOMAIN);
    UString::size_type cur = a_from, end = a_input.size ();
    CHECK_END (a_input, cur, end);

    if (a_input.compare (a_from, strlen (PREFIX_FRAME), PREFIX_FRAME)) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    GDBMIResultSafePtr result;
    if (!parse_gdbmi_result (a_input, cur, cur, result)) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    THROW_IF_FAIL (result);
    //CHECK_END (a_input, cur, end);

    if (result->variable () != "frame") {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    if (!result->value ()
        ||result->value ()->content_type ()
                != GDBMIValue::TUPLE_TYPE) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    GDBMITupleSafePtr result_value_tuple =
                                result->value ()->get_tuple_content ();
    if (!result_value_tuple) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    list<GDBMIResultSafePtr>::const_iterator res_it;
    GDBMIResultSafePtr tmp_res;
    IDebugger::Frame frame;
    UString name, value;
    for (res_it = result_value_tuple->content ().begin ();
         res_it != result_value_tuple->content ().end ();
         ++res_it) {
        if (!(*res_it)) {continue;}
        tmp_res = *res_it;
        if (!tmp_res->value ()
            ||tmp_res->value ()->content_type () != GDBMIValue::STRING_TYPE) {
            continue;
        }
        name = tmp_res->variable ();
        value = tmp_res->value ()->get_string_content ();
        if (name == "level") {
            frame.level (atoi (value.c_str ()));
        } else if (name == "addr") {
            frame.address (value);
        } else if (name == "func") {
            frame.function_name (value);
        } else if (name == "file") {
            frame.file_name (value);
        } else if (name == "fullname") {
            frame.file_full_name (value);
        } else if (name == "line") {
            frame.line (atoi (value.c_str ()));
        }
    }
    a_frame = frame;
    a_to = cur;
    return true;
}

/// parse a callstack as returned by the gdb/mi command:
/// -stack-list-frames
bool
parse_call_stack (const UString &a_input,
                  const UString::size_type a_from,
                  UString::size_type &a_to,
                  vector<IDebugger::Frame> &a_stack)
{
    LOG_FUNCTION_SCOPE_NORMAL_D (GDBMI_PARSING_DOMAIN);
    UString::size_type cur = a_from, end = a_input.size ();
    CHECK_END (a_input, cur, end);

    GDBMIResultSafePtr result;
    if (!parse_gdbmi_result (a_input, cur, cur, result)) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    THROW_IF_FAIL (result);
    CHECK_END (a_input, cur, end);

    if (result->variable () != "stack") {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    if (!result->value ()
        ||result->value ()->content_type ()
                != GDBMIValue::LIST_TYPE) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    GDBMIListSafePtr result_value_list =
        result->value ()->get_list_content ();
    if (!result_value_list) {
        a_to = cur;
        a_stack.clear ();
        return true;
    }

    if (result_value_list->content_type () != GDBMIList::RESULT_TYPE) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    list<GDBMIResultSafePtr> result_list ;
    result_value_list->get_result_content (result_list);

    GDBMITupleSafePtr frame_tuple;
    vector<IDebugger::Frame> stack;
    list<GDBMIResultSafePtr>::const_iterator iter, frame_part_iter;
    UString value;
    for (iter = result_list.begin (); iter != result_list.end (); ++iter) {
        if (!(*iter)) {continue;}
        THROW_IF_FAIL ((*iter)->value ()
                       && (*iter)->value ()->content_type ()
                       == GDBMIValue::TUPLE_TYPE);

        frame_tuple = (*iter)->value ()->get_tuple_content ();
        THROW_IF_FAIL (frame_tuple);
        IDebugger::Frame frame;
        for (frame_part_iter = frame_tuple->content ().begin ();
             frame_part_iter != frame_tuple->content ().end ();
             ++frame_part_iter) {
            THROW_IF_FAIL ((*frame_part_iter)->value ());
            value = (*frame_part_iter)->value ()->get_string_content ();
            if ((*frame_part_iter)->variable () == "addr") {
                frame.address (value);
            } else if ((*frame_part_iter)->variable () == "func") {
                frame.function_name (value);
            } else if ((*frame_part_iter)->variable () == "file") {
                frame.file_name (value);
            } else if ((*frame_part_iter)->variable () == "fullname") {
                frame.file_full_name (value);
            } else if ((*frame_part_iter)->variable () == "line") {
                frame.line (atol (value.c_str ()));
            }
        }
        THROW_IF_FAIL (frame.address () != "");
        stack.push_back (frame);
        frame.clear ();
    }
    a_stack = stack;
    a_to = cur;
    return true;
}

/// Parse the arguments of the call stack.
/// The call stack arguments is the result of the
/// GDB/MI command -stack-list-arguments 1.
/// It is basically the arguments of the functions of the call stack.
/// See the GDB/MI documentation for more.
bool
parse_stack_arguments (const UString &a_input,
                       UString::size_type a_from,
                       UString::size_type &a_to,
                       map<int, list<IDebugger::VariableSafePtr> > &a_params)
{
    LOG_FUNCTION_SCOPE_NORMAL_D (GDBMI_PARSING_DOMAIN);
    UString::size_type cur = a_from, end = a_input.size ();
    CHECK_END (a_input, cur, end);

    if (a_input.compare (cur, strlen (PREFIX_STACK_ARGS), PREFIX_STACK_ARGS)) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    GDBMIResultSafePtr gdbmi_result;
    if (!parse_gdbmi_result (a_input, cur, cur, gdbmi_result)) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    THROW_IF_FAIL (gdbmi_result
                   && gdbmi_result->variable () == "stack-args");

    if (!gdbmi_result->value ()
        || gdbmi_result->value ()->content_type ()
            != GDBMIValue::LIST_TYPE) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    GDBMIListSafePtr gdbmi_list =
        gdbmi_result->value ()->get_list_content ();
    if (!gdbmi_list) {
        a_to = cur;
        a_params.clear ();
        return true;
    }

    if (gdbmi_list->content_type () != GDBMIList::RESULT_TYPE) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    list<GDBMIResultSafePtr> frames_params_list;
    gdbmi_list->get_result_content (frames_params_list);
    LOG_D ("number of frames: " << (int) frames_params_list.size (),
           GDBMI_PARSING_DOMAIN);

    list<GDBMIResultSafePtr>::const_iterator frames_iter,
                                        params_records_iter,
                                        params_iter;
    map<int, list<IDebugger::VariableSafePtr> > all_frames_args;
    //walk through the list of frames
    //each frame is a tuple of the form:
    //{level="2", args=[list-of-arguments]}
    for (frames_iter = frames_params_list.begin ();
         frames_iter != frames_params_list.end ();
         ++frames_iter) {
        if (!(*frames_iter)) {
            LOG_D ("Got a null frmae, skipping", GDBMI_PARSING_DOMAIN);
            continue;
        }
        THROW_IF_FAIL ((*frames_iter)->variable () != "stack");
        THROW_IF_FAIL ((*frames_iter)->value ()
                        && (*frames_iter)->value ()->content_type ()
                        == GDBMIValue::TUPLE_TYPE)

        //params_record is a tuple that has the form:
        //{level="2", args=[list-of-arguments]}
        GDBMITupleSafePtr params_record;
        params_record = (*frames_iter)->value ()->get_tuple_content ();
        THROW_IF_FAIL (params_record);

        //walk through the tuple {level="2", args=[list-of-arguments]}
        int cur_frame_level=-1;
        for (params_records_iter = params_record->content ().begin ();
             params_records_iter != params_record->content ().end ();
             ++params_records_iter) {
            THROW_IF_FAIL ((*params_records_iter)->value ());

            if ((*params_records_iter)->variable () == "level") {
                THROW_IF_FAIL
                ((*params_records_iter)->value ()
                 && (*params_records_iter)->value ()->content_type ()
                 == GDBMIValue::STRING_TYPE);
                cur_frame_level = atoi
                    ((*params_records_iter)->value
                         ()->get_string_content ().c_str ());
                LOG_D ("frame level '" << (int) cur_frame_level << "'",
                       GDBMI_PARSING_DOMAIN);
            } else if ((*params_records_iter)->variable () == "args") {
                //this gdbmi result is of the form:
                //args=[{name="foo0", value="bar0"},
                //      {name="foo1", bar="bar1"}]

                THROW_IF_FAIL
                ((*params_records_iter)->value ()
                 && (*params_records_iter)->value ()->get_list_content ());

                GDBMIListSafePtr arg_list =
                    (*params_records_iter)->value ()->get_list_content ();
                list<GDBMIValueSafePtr>::const_iterator args_as_value_iter;
                list<IDebugger::VariableSafePtr> cur_frame_args;
                if (arg_list && !(arg_list->empty ())) {
                    LOG_D ("arg list is *not* empty for frame level '"
                           << (int)cur_frame_level,
                           GDBMI_PARSING_DOMAIN);
                    //walk each parameter.
                    //Each parameter is a tuple (in a value)
                    list<GDBMIValueSafePtr> arg_as_value_list;
                    arg_list->get_value_content (arg_as_value_list);
                    LOG_D ("arg list size: "
                           << (int)arg_as_value_list.size (),
                           GDBMI_PARSING_DOMAIN);
                    for (args_as_value_iter=arg_as_value_list.begin();
                         args_as_value_iter!=arg_as_value_list.end();
                         ++args_as_value_iter) {
                        if (!*args_as_value_iter) {
                            LOG_D ("got NULL arg, skipping",
                                   GDBMI_PARSING_DOMAIN);
                            continue;
                        }
                        GDBMITupleSafePtr args =
                            (*args_as_value_iter)->get_tuple_content ();
                        list<GDBMIResultSafePtr>::const_iterator arg_iter;
                        IDebugger::VariableSafePtr parameter
                                                (new IDebugger::Variable);
                        THROW_IF_FAIL (parameter);
                        THROW_IF_FAIL (args);
                        //walk the name and value of the parameter
                        for (arg_iter = args->content ().begin ();
                             arg_iter != args->content ().end ();
                             ++arg_iter) {
                            THROW_IF_FAIL (*arg_iter);
                            if ((*arg_iter)->variable () == "name") {
                                THROW_IF_FAIL ((*arg_iter)->value ());
                                parameter->name
                                ((*arg_iter)->value()->get_string_content ());
                            } else if ((*arg_iter)->variable () == "value") {
                                THROW_IF_FAIL ((*arg_iter)->value ());
                                parameter->value
                                    ((*arg_iter)->value
                                                 ()->get_string_content());
                                UString::size_type pos;
                                pos = parameter->value ().find ("{");
                                if (pos != Glib::ustring::npos) {
                                    //in certain cases
                                    //(gdbmi is not strict enough to be sure)
                                    //having a '{' in the parameter value
                                    //means the parameter has an anonymous
                                    //member variable. So let's try to
                                    //parse an anonymous member variable
                                    //and in case of success,
                                    //fill parameter->members()
                                    //with the structured member
                                    //embedded in parameter->value()
                                    //and set parameter->value() to nothing
                                    //This is shity performancewise
                                    //(and is ugly) but that's the way
                                    //of gdbmi
                                    if (parse_member_variable
                                            (parameter->value (),
                                             pos, pos, parameter, true)) {
                                        parameter->value ("");
                                    } else {
                                        LOG_ERROR ("Oops, '{' was not for "
                                                   "for an embedded variable");
                                    }
                                }
                            } else {
                                THROW ("should not reach this line");
                            }
                        }
                        LOG_D ("pushing arg '"
                               <<parameter->name()
                               <<"'='"<<parameter->value() <<"'"
                               <<" for frame level='"
                               <<(int)cur_frame_level
                               <<"'",
                               GDBMI_PARSING_DOMAIN);
                        cur_frame_args.push_back (parameter);
                    }
                } else {
                    LOG_D ("arg list is empty for frame level '"
                           << (int)cur_frame_level,
                           GDBMI_PARSING_DOMAIN);
                }
                THROW_IF_FAIL (cur_frame_level >= 0);
                LOG_D ("cur_frame_level: '"
                       << (int) cur_frame_level
                       << "', NB Params: "
                       << (int) cur_frame_args.size (),
                       GDBMI_PARSING_DOMAIN);
                all_frames_args[cur_frame_level] = cur_frame_args;
            } else {
                LOG_PARSING_ERROR (a_input, cur);
                return false;
            }
        }

    }

    a_to = cur;
    a_params = all_frames_args;
    LOG_D ("number of frames parsed: " << (int)a_params.size (),
           GDBMI_PARSING_DOMAIN);
    return true;
}

/// parse a list of local variables as returned by
/// the GDBMI command -stack-list-locals 2
bool
parse_local_var_list (const UString &a_input,
                     UString::size_type a_from,
                     UString::size_type &a_to,
                     list<IDebugger::VariableSafePtr> &a_vars)
{
    LOG_FUNCTION_SCOPE_NORMAL_D (GDBMI_PARSING_DOMAIN);
    UString::size_type cur = a_from, end = a_input.size ();
    CHECK_END (a_input, cur, end);

    if (a_input.compare (cur, strlen (PREFIX_LOCALS), PREFIX_LOCALS)) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    GDBMIResultSafePtr gdbmi_result;
    if (!parse_gdbmi_result (a_input, cur, cur, gdbmi_result)) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    THROW_IF_FAIL (gdbmi_result
                   && gdbmi_result->variable () == "locals");

    if (!gdbmi_result->value ()
        || gdbmi_result->value ()->content_type ()
            != GDBMIValue::LIST_TYPE) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    GDBMIListSafePtr gdbmi_list =
        gdbmi_result->value ()->get_list_content ();
    if (!gdbmi_list
        || gdbmi_list->content_type () == GDBMIList::UNDEFINED_TYPE) {
        a_to = cur;
        a_vars.clear ();
        return true;
    }
    RETURN_VAL_IF_FAIL (gdbmi_list->content_type () == GDBMIList::VALUE_TYPE,
                        false);

    std::list<GDBMIValueSafePtr> gdbmi_value_list;
    gdbmi_list->get_value_content (gdbmi_value_list);
    RETURN_VAL_IF_FAIL (!gdbmi_value_list.empty (), false);

    std::list<IDebugger::VariableSafePtr> variables;
    std::list<GDBMIValueSafePtr>::const_iterator value_iter;
    std::list<GDBMIResultSafePtr> tuple_content;
    std::list<GDBMIResultSafePtr>::const_iterator tuple_iter;
    for (value_iter = gdbmi_value_list.begin ();
         value_iter != gdbmi_value_list.end ();
         ++value_iter) {
        if (!(*value_iter)) {continue;}
        if ((*value_iter)->content_type () != GDBMIValue::TUPLE_TYPE) {
            LOG_ERROR_D ("list of tuple should contain only tuples",
                         GDBMI_PARSING_DOMAIN);
            continue;
        }
        GDBMITupleSafePtr gdbmi_tuple = (*value_iter)->get_tuple_content ();
        RETURN_VAL_IF_FAIL (gdbmi_tuple, false);
        RETURN_VAL_IF_FAIL (!gdbmi_tuple->content ().empty (), false);

        tuple_content.clear ();
        tuple_content = gdbmi_tuple->content ();
        RETURN_VAL_IF_FAIL (!tuple_content.empty (), false);
        IDebugger::VariableSafePtr variable (new IDebugger::Variable);
        for (tuple_iter = tuple_content.begin ();
             tuple_iter != tuple_content.end ();
             ++tuple_iter) {
            if (!(*tuple_iter)) {
                LOG_ERROR_D ("got and empty tuple member",
                             GDBMI_PARSING_DOMAIN);
                continue;
            }

            if (!(*tuple_iter)->value ()
                ||(*tuple_iter)->value ()->content_type ()
                    != GDBMIValue::STRING_TYPE) {
                LOG_ERROR_D ("Got a tuple member which value is not a string",
                             GDBMI_PARSING_DOMAIN);
                continue;
            }

            UString variable_str = (*tuple_iter)->variable ();
            UString value_str =
                        (*tuple_iter)->value ()->get_string_content ();
            value_str.chomp ();
            if (variable_str == "name") {
                variable->name (value_str);
            } else if (variable_str == "type") {
                variable->type (value_str);
            } else if (variable_str == "value") {
                variable->value (value_str);
            } else {
                LOG_ERROR_D ("got an unknown tuple member with name: '"
                             << variable_str << "'",
                             GDBMI_PARSING_DOMAIN)
                continue;
            }

        }
        variables.push_back (variable);
    }

    LOG_D ("got '" << (int)variables.size () << "' variables",
           GDBMI_PARSING_DOMAIN);

    a_vars = variables;
    a_to = cur;
    return true;
}

/// parse the result of -data-evaluate-expression <var-name>
/// the result is a gdbmi result of the form:
/// value={attrname0=val0, attrname1=val1,...} where val0 and val1
/// can be simili tuples representing complex types as well.
bool
parse_variable_value (const UString &a_input,
                      const UString::size_type a_from,
                      UString::size_type &a_to,
                      IDebugger::VariableSafePtr &a_var)
{
    LOG_FUNCTION_SCOPE_NORMAL_D (GDBMI_PARSING_DOMAIN);
    UString::size_type cur = a_from, end = a_input.size ();
    CHECK_END (a_input, cur, end);

    if (a_input.compare (cur, strlen (PREFIX_VALUE), PREFIX_VALUE)) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    cur += 6;
    CHECK_END (a_input, cur, end);
    CHECK_END (a_input, cur+1, end);

    a_var = IDebugger::VariableSafePtr (new IDebugger::Variable);
    if (a_input[cur+1] == '{') {
        ++cur;
        if (!parse_member_variable (a_input, cur, cur, a_var)) {
            LOG_PARSING_ERROR (a_input, cur);
            return false;
        }
        SKIP_BLANK (a_input, cur, end);
        if (a_input[cur] != '"') {
            UString value ;
            if (!parse_c_string_body (a_input, cur, end, value)) {
                LOG_PARSING_ERROR (a_input, cur);
                return false;
            }
            value = a_var->value () + " " + value;
            a_var->value (value);
        }
    } else {
        UString value;
        if (!parse_c_string (a_input, cur, cur, value)) {
            LOG_PARSING_ERROR (a_input, cur);
            return false;
        }
        a_var->value (value);
    }

    ++cur;
    a_to = cur;
    return true;
}

bool
parse_member_variable (const UString &a_input,
                       const UString::size_type a_from,
                       UString::size_type &a_to,
                       IDebugger::VariableSafePtr &a_var,
                       bool a_in_unnamed_var)
{
    LOG_FUNCTION_SCOPE_NORMAL_D (GDBMI_PARSING_DOMAIN);
    LOG_D ("in_unnamed_var = " <<(int)a_in_unnamed_var, GDBMI_PARSING_DOMAIN);
    THROW_IF_FAIL (a_var);

    UString::size_type cur = a_from, end = a_input.bytes ();
    CHECK_END (a_input, cur, end);

    if (a_input.c_str ()[cur] != '{') {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    ++cur;
    CHECK_END (a_input, cur, end);

    UString name, value;
    UString::size_type name_start=0, name_end=0, value_start=0, value_end=0;

    while (true /*fetch name*/) {
        name_start=0, name_end=0, value_start=0, value_end=0;
        name = "#unnamed#" , value = "";

        SKIP_BLANK (a_input, cur, cur);
        LOG_D ("fetching name ...", GDBMI_PARSING_DOMAIN);
        //we should be at the begining of A = B. lets try to parse A
        if (a_input.c_str ()[cur] != '{') {
            SKIP_BLANK (a_input, cur, cur);
            name_start = cur;
            while (true) {
                if (cur < a_input.bytes ()
                    && a_input.c_str ()[cur] != '='
                    && a_input.c_str ()[cur] != '}') {
                    ++cur;
                } else {
                    //if we found an '}' character, make sure
                    //it is not enclosed in sigle quotes. If it is in
                    //single quotes then ignore it.
                    if (cur > 0
                        && cur+1 < a_input.bytes ()
                        && a_input.c_str ()[cur-1] == '\''
                        && a_input.c_str ()[cur+1] == '\'') {
                        ++cur;
                        continue;
                    }
                    break;
                }
            }
            //we should be at the end of A (as in A = B)
            name_end = cur - 1;
            name.assign (a_input, name_start, name_end - name_start + 1);
            LOG_D ("got name '" << name << "'", GDBMI_PARSING_DOMAIN);
        }

        IDebugger::VariableSafePtr cur_var (new IDebugger::Variable);
        name.chomp ();
        cur_var->name (name);

        if (a_input.c_str ()[cur] == '}') {
            ++cur;
            cur_var->value ("");
            a_var->append (cur_var);
            LOG_D ("reached '}' after '"
                   << name << "'",
                   GDBMI_PARSING_DOMAIN);
            break;
        }

        SKIP_BLANK (a_input, cur, cur);
        if (a_input.c_str ()[cur] != '{') {
            ++cur;
            CHECK_END (a_input, cur, end);
            SKIP_BLANK (a_input, cur, cur);
        }

        //if we are at a '{', (like in A = {B}),
        //the B is said to be a member variable of A
        //you can also call it a member attribute of A
        //In this case, let's try to parse {B}
        if (a_input.c_str ()[cur] == '{') {
            bool in_unnamed = true;
            if (name == "#unnamed#") {
                in_unnamed = false;
            }
            if (!parse_member_variable (a_input,
                                        cur,
                                        cur,
                                        cur_var,
                                        in_unnamed)) {
                LOG_PARSING_ERROR (a_input, cur);
                return false;
            }
        } else {
            //else, we are at a B, (like in A = B),
            //so let's try to parse B
            SKIP_BLANK (a_input, cur, cur);
            value_start = cur;
            while (true) {
                UString str;
                if (a_input.c_str ()[cur] == '"'
                    && a_input.c_str ()[cur -1] != '\\') {
                    if (!parse_c_string (a_input, cur, cur, str)) {
                        LOG_PARSING_ERROR (a_input, cur);
                        return false;
                    }
                } else if (cur+2 < end
                           && a_input.c_str ()[cur] == '\\'
                           && a_input.c_str ()[cur+1] == '"'
                           && a_input.c_str ()[cur+2] != '\''
                           && a_input.c_str ()[cur-1] != '\'') {
                    if (!parse_embedded_c_string (a_input, cur, cur, str)){
                        LOG_PARSING_ERROR (a_input, cur);
                        return false;
                    }
                } else if ((a_input.c_str ()[cur] != ','
                               || ((cur+1 < end) && a_input[cur+1] != ' '))
                           && a_input.c_str ()[cur] != '}') {
                    ++cur;
                    CHECK_END (a_input, cur, end);
                } else {
                    //if we found an '}' character, make sure
                    //it is not enclosed in sigle quotes. If it is in
                    //single quotes then ignore it.
                    if (cur > 0
                        && cur+1 < a_input.bytes ()
                        && a_input.c_str ()[cur-1] == '\''
                        && a_input.c_str ()[cur+1] == '\'') {
                        ++cur;
                        continue;
                    }
                    //otherwise, getting out condition is either
                    //", " or "}".  check out the the 'if' condition.
                    break;
                }
            }
            if (cur != value_start) {
                value_end = cur - 1;
                value.assign (a_input, value_start,
                              value_end - value_start + 1);
                LOG_D ("got value: '"
                       << value << "'",
                       GDBMI_PARSING_DOMAIN);
            } else {
                value = "";
                LOG_D ("got empty value", GDBMI_PARSING_DOMAIN);
            }
            cur_var->value (value);
        }
        a_var->append (cur_var);

        LOG_D ("cur char: " << (char) a_input.c_str()[cur],
               GDBMI_PARSING_DOMAIN);

end_of_block:
        SKIP_BLANK (a_input, cur, cur);

        LOG_D ("cur char: " << (char) a_input.c_str()[cur],
                GDBMI_PARSING_DOMAIN);

        if (cur == end || a_input.c_str ()[cur] == '"') {
            break;
        } else if (a_input.c_str ()[cur] == '}') {
            ++cur;
            break;
        } else if (a_input.c_str ()[cur] == ',') {
            ++cur;
            CHECK_END (a_input, cur, end);
            LOG_D ("got ',' , going to fetch name",
                   GDBMI_PARSING_DOMAIN);
            continue /*goto fetch name*/;
        } else if (!a_input.raw ().compare (cur, 9, "<repeats ")) {
            //TODO: we need to handle the <repeat N> format at some point.
            //just skipping it for now.
            bool skipped_repeat=false;
            while (true) {
                ++cur;
                if (cur == end) {break;}
                if (a_input.c_str ()[cur] == '>') {
                    ++cur;
                    skipped_repeat = true;
                    break;
                }
            }
            if (skipped_repeat) {
                LOG_D ("skipped repeat construct", GDBMI_PARSING_DOMAIN);
                goto end_of_block;
                break;
            } else {
                LOG_ERROR ("failed to skip repeat construct");
                LOG_PARSING_ERROR (a_input, cur);
                return false;
            }
        } else if (!a_input.raw ().compare (cur, 3, "...")) {
            //hugh ? wtf does this '...' mean ? Anyway, skip it for now.
            cur+= 3;
            goto end_of_block;
        }
        LOG_PARSING_ERROR (a_input, cur);
        THROW ("should not be reached");
    }//end while

    a_to = cur;
    return true;
}


/// \brief parse function arguments list
///
/// function args list have the form:
/// args=[{name="name0",value="0"},{name="name1",value="1"}]
///
/// This function parses only started from (including) the first '{'
/// \param a_input the input string to parse
/// \param a_from where to parse from.
/// \param out parameter. End of the parsed string.
/// This is a past the end  offset.
/// \param a_args the map of the parsed attributes.
/// This is set if and
/// only if the function returned true.
/// \return true upon successful parsing, false otherwise.
bool
parse_function_args (const UString &a_input,
                     UString::size_type a_from,
                     UString::size_type &a_to,
                     map<UString, UString> a_args)
{
    LOG_FUNCTION_SCOPE_NORMAL_D (GDBMI_PARSING_DOMAIN);
    UString::size_type cur = a_from, end = a_input.size ();
    CHECK_END (a_input, cur, end);

    if (a_input.compare (cur, 1, "{")) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    cur ++ ;
    if (cur >= end) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    UString::size_type name_start (0),
    name_end (0),
    value_start (0),
    value_end (0);

    Glib::ustring name, value;
    map<UString, UString> args;

    while (true) {
        if (a_input.compare (cur, 6, "name=\"")) {break;}
        cur += 6;
        if (cur >= end) {
            LOG_PARSING_ERROR (a_input, cur);
            return false;
        }
        name_start = cur;
        for (;
             cur < end && (a_input[cur] != '"' || a_input[cur -1] == '\\');
             ++cur) {
        }
        if (a_input[cur] != '"') {
            LOG_PARSING_ERROR (a_input, cur);
            return false;
        }
        name_end = cur - 1;
        if (++cur >= end) {
            LOG_PARSING_ERROR (a_input, cur);
            return false;
        }
        if (a_input.compare (cur, 8, ",value=\"")) {
            LOG_PARSING_ERROR (a_input, cur);
            return false;
        }
        cur += 8; if (cur >= end) {
            LOG_PARSING_ERROR (a_input, cur);
            return false;
        }
        value_start = cur;
        for (;
             cur < end && (a_input[cur] != '"' || a_input[cur-1] == '\\');
             ++cur) {
        }
        if (a_input[cur] != '"') {
            LOG_PARSING_ERROR (a_input, cur);
            return false;
        }
        value_end = cur - 1;
        name.clear (), value.clear ();
        name.assign (a_input, name_start, name_end - name_start + 1);
        value.assign (a_input, value_start, value_end - value_start + 1);
        args[name] = value;
        if (++cur >= end) {
            LOG_PARSING_ERROR (a_input, cur);
            return false;
        }
        if (a_input[cur] != '}') {
            LOG_PARSING_ERROR (a_input, cur);
            return false;
        }
        if (++cur >= end) {break;}

        if (!a_input.compare(cur, 2,",{") ){
            cur += 2;
            continue;
        } else {
            break;
        }
    }

    a_args = args;
    a_to = cur;
    return true;
}

/// parses the result of the gdbmi command
/// "-thread-list-ids".
bool
parse_threads_list (const UString &a_input,
                    UString::size_type a_from,
                    UString::size_type &a_to,
                    std::list<int> &a_thread_ids)
{
    LOG_FUNCTION_SCOPE_NORMAL_D (GDBMI_PARSING_DOMAIN);
    UString::size_type cur = a_from, end = a_input.size ();

    if (a_input.compare (cur, strlen (PREFIX_THREAD_IDS), PREFIX_THREAD_IDS)) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    GDBMIResultSafePtr gdbmi_result;
    if (!parse_gdbmi_result (a_input, cur, cur, gdbmi_result)) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    if (a_input[cur] != ',') {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    ++cur;
    CHECK_END (a_input, cur, end);

    if (gdbmi_result->variable () != "thread-ids") {
        LOG_ERROR ("expected gdbmi variable 'thread-ids', got: '"
                   << gdbmi_result->variable () << "\'");
        return false;
    }
    THROW_IF_FAIL (gdbmi_result->value ());
    THROW_IF_FAIL ((gdbmi_result->value ()->content_type ()
                       == GDBMIValue::TUPLE_TYPE)
                   ||
                   (gdbmi_result->value ()->content_type ()
                        == GDBMIValue::EMPTY_TYPE));

    GDBMITupleSafePtr gdbmi_tuple;
    if (gdbmi_result->value ()->content_type ()
         != GDBMIValue::EMPTY_TYPE) {
        gdbmi_tuple = gdbmi_result->value ()->get_tuple_content ();
        THROW_IF_FAIL (gdbmi_tuple);
    }

    list<GDBMIResultSafePtr> result_list;
    if (gdbmi_tuple) {
        result_list = gdbmi_tuple->content ();
    }
    list<GDBMIResultSafePtr>::const_iterator it;
    int thread_id=0;
    std::list<int> thread_ids;
    for (it = result_list.begin (); it != result_list.end (); ++it) {
        THROW_IF_FAIL (*it);
        if ((*it)->variable () != "thread-id") {
            LOG_ERROR ("expected a gdbmi value with variable name 'thread-id'"
                       ". Got '" << (*it)->variable () << "'");
            return false;
        }
        THROW_IF_FAIL ((*it)->value ()
                       && ((*it)->value ()->content_type ()
                           == GDBMIValue::STRING_TYPE));
        thread_id = atoi ((*it)->value ()->get_string_content ().c_str ());
        THROW_IF_FAIL (thread_id);
        thread_ids.push_back (thread_id);
    }

    if (!parse_gdbmi_result (a_input, cur, cur, gdbmi_result)) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    if (gdbmi_result->variable () != "number-of-threads") {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    THROW_IF_FAIL (gdbmi_result->value ()
                   && gdbmi_result->value ()->content_type ()
                       == GDBMIValue::STRING_TYPE);
    unsigned int num_threads =
        atoi (gdbmi_result->value ()->get_string_content ().c_str ());

    if (num_threads != thread_ids.size ()) {
        LOG_ERROR ("num_threads: '"
                   << (int) num_threads
                   << "', thread_ids.size(): '"
                   << (int) thread_ids.size ()
                   << "'");
        return false;
    }

    a_thread_ids = thread_ids;
    a_to = cur;
    return true;
}

struct QuickUStringLess : public std::binary_function<const UString,
                                                      const UString,
                                                      bool> {
    bool operator() (const UString &a_lhs,
                     const UString &a_rhs)
    {
        if (!a_lhs.c_str ()) {return true;}
        if (!a_rhs.c_str ()) {return false;}
        //this is false for non ascii characters
        //but is way faster than UString::compare().
        int res = strncmp (a_lhs.c_str (), a_rhs.c_str (), a_lhs.bytes ());
        if (res < 0) {return true;}
        return false;
    }
};//end struct QuickUstringLess

/// parses the result of the gdbmi command
/// "-file-list-exec-source-files".
bool
parse_file_list (const UString &a_input,
                 UString::size_type a_from,
                 UString::size_type &a_to,
                 std::vector<UString> &a_files)
{
    LOG_FUNCTION_SCOPE_NORMAL_D (GDBMI_PARSING_DOMAIN);
    UString::size_type cur = a_from, end = a_input.bytes ();

    if (a_input.compare (cur, strlen (PREFIX_FILES), PREFIX_FILES)) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    cur += 7;

    std::vector<GDBMITupleSafePtr> tuples;
    while (cur <= end) {
        GDBMITupleSafePtr tuple;
        if (!parse_gdbmi_tuple(a_input, cur, cur, tuple)) {
            LOG_PARSING_ERROR (a_input, cur);
            return false;
        }
        tuples.push_back(tuple);
        if (a_input.c_str ()[cur] == ',') {
            ++cur;
        } else if (a_input.c_str ()[cur] == ']') {
            //at the end of the list, just get out
            break;
        } else {
            //unexpected data
            LOG_PARSING_ERROR (a_input, cur);
        }
    }

    std::vector<UString> files;
    vector<GDBMITupleSafePtr>::const_iterator file_iter;
    for (file_iter = tuples.begin();
         file_iter != tuples.end();
         ++file_iter)
    {
        UString filename;
        list<GDBMIResultSafePtr>::const_iterator attr_it;
        for (attr_it = (*file_iter)->content ().begin ();
             attr_it != (*file_iter)->content ().end (); ++attr_it) {
             THROW_IF_FAIL ((*attr_it)->value ()
                           && ((*attr_it)->value ()->content_type ()
                               == GDBMIValue::STRING_TYPE));
            if ((*attr_it)->variable () == "file") {
                //only use the 'file' attribute if the
                //fullname isn't already set
                //FIXME: do we even want to list these at all?
                if (filename.empty ()) {
                    filename = (*attr_it)->value ()->get_string_content ();
                }
            } else if ((*attr_it)->variable () == "fullname") {
                //use the fullname attribute,
                //overwriting the 'file' attribute
                //if necessary
                filename = (*attr_it)->value ()->get_string_content ();
            } else {
                LOG_ERROR ("expected a gdbmi value with "
                            "variable name 'file' or 'fullname'"
                            ". Got '" << (*attr_it)->variable () << "'");
                return false;
            }
        }
        THROW_IF_FAIL (!filename.empty());
        files.push_back (filename);
    }

    std::sort(files.begin(), files.end(), QuickUStringLess());
    std::vector<UString>::iterator last_unique =
        std::unique (files.begin (), files.end ());
    a_files = std::vector<UString>(files.begin (), last_unique);
    a_to = cur;
    return true;
}

bool
parse_register_names (const UString &a_input,
                      UString::size_type a_from,
                      UString::size_type &a_to,
                      std::map<IDebugger::register_id_t, UString> &a_registers)
{
    LOG_FUNCTION_SCOPE_NORMAL_D (GDBMI_PARSING_DOMAIN);
    UString::size_type cur = a_from;

    if (a_input.compare (cur, strlen (PREFIX_REGISTER_NAMES),
                PREFIX_REGISTER_NAMES)) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    cur += strlen (PREFIX_REGISTER_NAMES);

    GDBMIListSafePtr reg_list;
    if (!parse_gdbmi_list (a_input, cur, cur, reg_list)) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    if (a_input.c_str ()[cur-1] != ']') {
        //unexpected data
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    std::map<IDebugger::register_id_t, UString> regs;
    if (reg_list->content_type () != GDBMIList::VALUE_TYPE) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    std::list<GDBMIValueSafePtr> value_list;
    reg_list->get_value_content (value_list);
    IDebugger::register_id_t id = 0;
    std::list<GDBMIValueSafePtr>::const_iterator val_iter;
    for (val_iter = value_list.begin();
         val_iter != value_list.end();
         ++val_iter, ++id) {
        UString regname = (*val_iter)->get_string_content ();
        regs[id] = regname;
    }

    a_registers = regs;
    a_to = cur;
    return true;
}

bool
parse_changed_registers (const UString &a_input,
                         UString::size_type a_from,
                         UString::size_type &a_to,
                         std::list<IDebugger::register_id_t> &a_registers)
{
    LOG_FUNCTION_SCOPE_NORMAL_D (GDBMI_PARSING_DOMAIN);
    UString::size_type cur = a_from;

    if (a_input.compare (cur, strlen (PREFIX_CHANGED_REGISTERS),
                PREFIX_CHANGED_REGISTERS)) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    cur += strlen (PREFIX_CHANGED_REGISTERS);

    GDBMIListSafePtr reg_list;
    if (!parse_gdbmi_list (a_input, cur, cur, reg_list)) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    if (a_input.c_str ()[cur-1] != ']') {
        // unexpected data
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    std::list<IDebugger::register_id_t> regs;
    if (!reg_list->empty () &&
            reg_list->content_type () != GDBMIList::VALUE_TYPE) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    std::list<GDBMIValueSafePtr> value_list;
    reg_list->get_value_content (value_list);
    for (std::list<GDBMIValueSafePtr>::const_iterator val_iter =
            value_list.begin();
            val_iter != value_list.end();
            ++val_iter)
    {
        UString regname = (*val_iter)->get_string_content ();
        regs.push_back (atoi (regname.c_str ()));
    }

    a_registers = regs;
    a_to = cur;
    return true;
}

bool
parse_register_values (const UString &a_input,
                       UString::size_type a_from,
                       UString::size_type &a_to,
                       std::map<IDebugger::register_id_t, UString> &a_values)
{
    LOG_FUNCTION_SCOPE_NORMAL_D (GDBMI_PARSING_DOMAIN);
    UString::size_type cur = a_from;

    if (a_input.compare (cur, strlen (PREFIX_REGISTER_VALUES),
                PREFIX_REGISTER_VALUES)) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    cur += strlen (PREFIX_REGISTER_VALUES);

    GDBMIListSafePtr gdbmi_list;
    if (!parse_gdbmi_list (a_input, cur, cur, gdbmi_list)) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    if (a_input.c_str ()[cur-1] != ']') {
        // unexpected data
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    std::map<IDebugger::register_id_t, UString> vals;
    if (gdbmi_list->content_type () != GDBMIList::VALUE_TYPE) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    std::list<GDBMIValueSafePtr> val_list;
    gdbmi_list->get_value_content (val_list);
    std::list<GDBMIValueSafePtr>::const_iterator val_iter;
    for (val_iter = val_list.begin ();
         val_iter != val_list.end ();
         ++val_iter) {
        UString value_str;
        if ((*val_iter)->content_type ()
            != GDBMIValue::TUPLE_TYPE) {
            LOG_PARSING_ERROR (a_input, cur);
            return false;
        }
        GDBMITupleSafePtr tuple = (*val_iter)->get_tuple_content ();
        std::list<GDBMIResultSafePtr> result_list = tuple->content ();
        if (result_list.size () != 2) {
            // each tuple should have a 'number' and 'value' field
            LOG_PARSING_ERROR (a_input, cur);
            return false;
        }
        std::list<GDBMIResultSafePtr>::const_iterator res_iter =
                                                result_list.begin ();
        // get register number
        GDBMIValueSafePtr reg_number_val = (*res_iter)->value ();
        if ((*res_iter)->variable () != "number"
            || reg_number_val->content_type () != GDBMIValue::STRING_TYPE) {
            LOG_PARSING_ERROR (a_input, cur);
            return false;
        }
        IDebugger::register_id_t id =
                atoi (reg_number_val->get_string_content ().c_str ());

        // get the new value of the register
        ++res_iter;
        GDBMIValueSafePtr reg_value_val = (*res_iter)->value ();
        if ((*res_iter)->variable () != "value"
            || reg_value_val->content_type () != GDBMIValue::STRING_TYPE) {
            LOG_PARSING_ERROR (a_input, cur);
            return false;
        } else {
            value_str = reg_value_val->get_string_content ();
        }
        vals[id] = value_str;
    }

    a_values = vals;
    a_to = cur;
    return true;
}

bool
parse_memory_values (const UString &a_input,
                     UString::size_type a_from,
                     UString::size_type &a_to,
                     size_t& a_start_addr,
                     std::vector<uint8_t> &a_values)
{
    LOG_FUNCTION_SCOPE_NORMAL_D (GDBMI_PARSING_DOMAIN);
    UString::size_type cur = a_from;

    if (a_input.compare (cur,
                         strlen (PREFIX_MEMORY_VALUES),
                                 PREFIX_MEMORY_VALUES)) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    //skip to the actual list of memory values
    const char* prefix_memory = "memory=";
    cur = a_input.find (prefix_memory);
    if (!cur) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    cur += strlen (prefix_memory);

    GDBMIListSafePtr mem_gdbmi_list;
    if (!parse_gdbmi_list (a_input, cur, cur, mem_gdbmi_list)) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    if (a_input.c_str ()[cur-1] != ']') {
        //unexpected data
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    if (mem_gdbmi_list->content_type ()
        != GDBMIList::VALUE_TYPE) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    std::list<GDBMIValueSafePtr> mem_value_list;
    mem_gdbmi_list->get_value_content (mem_value_list);

    //there should only be one 'row'
    if (mem_value_list.size () != 1) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    std::list<GDBMIValueSafePtr>::const_iterator mem_tuple_iter =
                                                    mem_value_list.begin ();
    if ((*mem_tuple_iter)->content_type ()
        != GDBMIValue::TUPLE_TYPE) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    const GDBMITupleSafePtr gdbmi_tuple =
                            (*mem_tuple_iter)->get_tuple_content ();

    std::list<GDBMIResultSafePtr> result_list;
    result_list = gdbmi_tuple->content ();
    if (result_list.size () < 2) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    std::vector<uint8_t> memory_values;
    bool seen_addr = false, seen_data = false;
    for (std::list<GDBMIResultSafePtr>::const_iterator result_iter =
            result_list.begin();
            result_iter != result_list.end();
            ++result_iter) {
        if ((*result_iter)->variable () == "addr") {
            seen_addr = true;
            if ((*result_iter)->value ()->content_type ()
                != GDBMIValue::STRING_TYPE) {
                LOG_PARSING_ERROR (a_input, cur);
                return false;
            }
            std::istringstream istream((*result_iter)->value ()->
                                                    get_string_content ());
            istream >> std::hex >> a_start_addr;
        } else if ((*result_iter)->variable () == "data") {
            seen_data = true;
            if ((*result_iter)->value ()->content_type ()
                != GDBMIValue::LIST_TYPE) {
                LOG_PARSING_ERROR (a_input, cur);
                return false;
            }
            GDBMIListSafePtr gdbmi_list =
                            (*result_iter)->value ()->get_list_content ();
            if (gdbmi_list->content_type () != GDBMIList::VALUE_TYPE) {
                LOG_PARSING_ERROR (a_input, cur);
                return false;
            }
            std::list<GDBMIValueSafePtr> gdbmi_values;
            gdbmi_list->get_value_content (gdbmi_values);
            std::list<GDBMIValueSafePtr>::const_iterator val_iter;
            for (val_iter = gdbmi_values.begin ();
                 val_iter != gdbmi_values.end ();
                 ++val_iter) {
                if ((*val_iter)->content_type ()
                    != GDBMIValue::STRING_TYPE) {
                    LOG_PARSING_ERROR (a_input, cur);
                    return false;
                }
                std::istringstream istream
                                        ((*val_iter)->get_string_content ());
                //if I use a uint8_t type here, it doesn't seem to work, so
                //using a 16-bit value that will be cast down to 8 bits
                uint16_t byte_val;
                istream >> std::hex >> byte_val;
                memory_values.push_back (byte_val);
            }
        }
        //else ignore it
    }

    if (!seen_addr || !seen_data)
    {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    a_values = memory_values;
    a_to = cur;
    return true;
}

/// parses the result of the gdbmi command
/// "-thread-select"
/// \param a_input the input string to parse
/// \param a_from the offset from where to start the parsing
/// \param a_to. out parameter. The next offset after the end of what
/// got parsed.
/// \param a_thread_id out parameter. The id of the selected thread.
/// \param a_frame out parameter. The current frame in the selected thread.
/// \param a_level out parameter. the level
bool
parse_new_thread_id (const UString &a_input,
                     UString::size_type a_from,
                     UString::size_type &a_to,
                     int &a_thread_id,
                     IDebugger::Frame &a_frame)
{
    LOG_FUNCTION_SCOPE_NORMAL_D (GDBMI_PARSING_DOMAIN);
    UString::size_type cur = a_from, end = a_input.size ();

    if (a_input.compare (cur, strlen (PREFIX_NEW_THREAD_ID),
                PREFIX_NEW_THREAD_ID)) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    GDBMIResultSafePtr gdbmi_result;
    if (!parse_gdbmi_result (a_input, cur, cur, gdbmi_result)
        || !gdbmi_result) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    if (gdbmi_result->variable () != "new-thread-id") {
        LOG_ERROR ("expected 'new-thread-id', got '"
                   << gdbmi_result->variable () << "'");
        return false;
    }
    THROW_IF_FAIL (gdbmi_result->value ());
    THROW_IF_FAIL (gdbmi_result->value ()->content_type ()
                   == GDBMIValue::STRING_TYPE);
    CHECK_END (a_input, cur, end);

    int thread_id =
        atoi (gdbmi_result->value ()->get_string_content ().c_str ());
    if (!thread_id) {
        LOG_ERROR ("got null thread id");
        return false;
    }

    SKIP_BLANK (a_input, cur, cur);

    if (a_input[cur] != ',') {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    ++cur;
    CHECK_END (a_input, cur, end);

    IDebugger::Frame frame;
    if (!parse_frame (a_input, cur, cur, frame)) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    a_to = cur;
    a_thread_id = thread_id;
    a_frame = frame;
    return true;
}

/// parses a string that has the form:
/// \"blah\"
bool
parse_c_string (const UString &a_input,
                UString::size_type a_from,
                UString::size_type &a_to,
                UString &a_c_string)
{
    //LOG_FUNCTION_SCOPE_NORMAL_D (GDBMI_PARSING_DOMAIN);

    UString::size_type cur=a_from, end = a_input.bytes ();
    CHECK_END (a_input, cur, end);

    if (a_input.c_str ()[cur] != '"') {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    ++cur;
    CHECK_END (a_input, cur, end);

    UString str;
    if (!parse_c_string_body (a_input, cur, cur, str)) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    if (a_input.c_str ()[cur] != '"') {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    ++cur;
    a_c_string = str;
    a_to = cur;
    return true;
}

/// parse an octal escape.
/// it as the form: "\xxx"
/// where x is an character between
/// '0' and '9', so that isdigit(x) returns true.
/// the result of the parsing is the character code
///of the actual charactere being escaped
/// \param a_input the input string to parse from
/// \param a_from  the byte index to start parsing from
/// \param a_to out parameter. Index pointing after the parsed octal escape
/// \param a_byte_value the resulting byte value parsed
//
static bool
parse_octal_escape (const UString &a_input,
                    UString::size_type a_from,
                    UString::size_type &a_to,
                    unsigned char &a_byte_value)
{
    LOG_FUNCTION_SCOPE_NORMAL_D (GDBMI_PARSING_DOMAIN);
    UString::size_type cur=a_from, end = a_input.bytes ();

    if (cur+3 >= end)
        return false;

    if (a_input.c_str ()[cur] != '\\'
        || !isdigit (a_input.c_str ()[cur+1])
        || !isdigit (a_input.c_str ()[cur+2])
        || !isdigit (a_input.c_str ()[cur+3])) {
        return false;
    }

    a_byte_value = (a_input.c_str ()[cur+1] - '0') * 64 +
                   (a_input.c_str ()[cur+2] - '0') * 8 +
                   (a_input.c_str ()[cur+3] - '0');

    a_to = cur + 4;
    return true;
}

/// parse a sequence of bytes expressed in octal escapes.
/// The result is a string, (in utf8).
/// \param a_input the string (containing the octal escapes.) to parse from
/// \param a_from in index to start parsing from
/// \param a_to out param. The index pointing after what got parsed.
/// \param a_result out param. The resulting parsed string
static bool
parse_octal_escape_sequence (const UString &a_input,
                             UString::size_type a_from,
                             UString::size_type &a_to,
                             UString &a_result)
{
    LOG_FUNCTION_SCOPE_NORMAL_D (GDBMI_PARSING_DOMAIN);
    UString::size_type cur=a_from, end = a_input.bytes ();

    if (cur+3 >= end)
        return false;

    CHECK_END (a_input, cur, end);
    CHECK_END (a_input, cur+1, end);

    unsigned char b=0;
    string raw;
    while (a_input.c_str ()[cur] == '\\') {
        if (parse_octal_escape (a_input, cur, cur, b)) {
            raw += b;
        } else {
            break;
        }
    }
    if (raw.empty ()) return false;
    try {
        a_result = Glib::locale_to_utf8 (raw);
    } catch (...) {
        a_result.assign (raw.size (), '?');
    }
    a_to = cur;
    return true;
}

bool
parse_c_string_body (const UString &a_input,
                     UString::size_type a_from,
                     UString::size_type &a_to,
                     UString &a_string)
{
    //LOG_FUNCTION_SCOPE_NORMAL_D (GDBMI_PARSING_DOMAIN);

    UString::size_type cur=a_from, end = a_input.bytes ();
    CHECK_END (a_input, cur, end);

    UString::value_type ch = a_input.c_str()[cur], prev_ch;
    if (ch == '"') {
        a_string = "";
        a_to = cur;
        return true;
    }

    if (!isascii (ch) && ch != '\\') {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    UString result;
    if (ch != '\\') {
        result += ch;
        ++cur;
    } else {
        UString seq;
        if ((cur+3 < end)
            && isdigit (a_input.c_str ()[cur +1])
            && isdigit (a_input.c_str ()[cur +2])
            && isdigit (a_input.c_str ()[cur +3])
            && parse_octal_escape_sequence (a_input, cur, cur, seq)) {
            result += seq;
        } else {
            result += ch;
            ++cur;
        }
    }
    CHECK_END (a_input, cur, end);

    for (;;) {
        prev_ch = ch;
        ch = a_input.c_str()[cur];
        if (isascii (ch)) {
            if (ch == '"' && prev_ch != '\\') {
                break;
            }
            if (ch == '\\') {
                UString seq;
                if ((cur+3 < end)
                    && isdigit (a_input.c_str ()[cur +1])
                    && isdigit (a_input.c_str ()[cur +2])
                    && isdigit (a_input.c_str ()[cur +3])
                    && parse_octal_escape_sequence (a_input, cur, cur, seq)) {
                    ch = seq[seq.size ()-1];
                    result += seq;
                } else {
                    result += ch;
                    ++cur;
                }
            } else {
                result += ch;
                ++cur;
            }
            CHECK_END (a_input, cur, end);
            continue;
        }
        break;
    }

    if (ch != '"') {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    a_string = result;
    a_to = cur;
    return true;
}

bool
parse_embedded_c_string_body (const UString &a_input,
                              UString::size_type a_from,
                              UString::size_type &a_to,
                              UString &a_string)
{
    LOG_FUNCTION_SCOPE_NORMAL_D (GDBMI_PARSING_DOMAIN);

    UString::size_type cur=a_from, end = a_input.bytes ();
    CHECK_END (a_input, cur, end);
    CHECK_END (a_input, cur+1, end);

    if (a_input.c_str ()[cur] != '\\' || a_input.c_str ()[cur+1] != '"') {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    cur += 2;
    CHECK_END (a_input, cur, end);
    UString escaped_str;
    escaped_str += '"';

    //first walk the string, and unescape everything we find escaped
    UString::value_type ch=0, prev_ch=0;
    bool escaping = false, found_end=false;
    for (; cur != end; ++cur) {
        ch = a_input[cur];
        if (ch == '\\') {
            if (escaping) {
                prev_ch = ch;
                escaped_str += ch;
                escaping = false;
            } else {
                escaping = true;
            }
        } else if (ch == '"') {
            if (escaping) {
                if (prev_ch != '\\') {
                    //found the end of string
                    found_end = true;
                }
                prev_ch = ch;
                escaped_str += ch;
                escaping = false;
                if (found_end) {
                    break;
                }
            } else {
                LOG_PARSING_ERROR (a_input, cur);
                return false;
            }
        } else {
            escaped_str += ch;
            prev_ch = ch;
            escaping = false;
        }
    }
    if (!found_end) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    //TODO:debug this.
    a_string = escaped_str;
    a_to = cur;
    return true;
}

bool
parse_embedded_c_string (const UString &a_input,
                         UString::size_type a_from,
                         UString::size_type &a_to,
                         UString &a_string)
{
    LOG_FUNCTION_SCOPE_NORMAL_D (GDBMI_PARSING_DOMAIN);

    UString::size_type cur=a_from, end = a_input.bytes ();
    CHECK_END (a_input, cur, end);

    if (a_input.c_str ()[cur] != '\\' || a_input.c_str ()[cur+1] != '"') {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    if (!parse_embedded_c_string_body (a_input, cur, cur, a_string)) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    a_to = ++cur;
    return true;
}

static bool
is_string_start (gunichar a_c)
{
    if (!isalpha (a_c) && a_c != '_' && a_c != '<' && a_c != '>') {
        return false;
    }
    return true;
}

/// parses a string that has the form:
/// blah
bool
parse_string (const UString &a_input,
              UString::size_type a_from,
              UString::size_type &a_to,
              UString &a_string)
{
    LOG_FUNCTION_SCOPE_NORMAL_D (GDBMI_PARSING_DOMAIN);
    UString::size_type cur=a_from, end = a_input.bytes ();
    CHECK_END (a_input, cur, end);

    UString::value_type ch = a_input.c_str ()[cur];
    if (!is_string_start (ch)) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    UString::size_type str_start (cur), str_end (0);
    ++cur;
    CHECK_END (a_input, cur, end);

    for (;;) {
        ch = a_input.c_str ()[cur];
        if (isalnum (ch)
            || ch == '_'
            || ch == '-'
            || ch == '>'
            || ch == '<') {
            ++cur;
            CHECK_END (a_input, cur, end);
            continue;
        }
        str_end = cur - 1;
        break;
    }
    Glib::ustring str (a_input.c_str () +str_start,
                       str_end - str_start + 1);
    a_string = str;
    a_to = cur;
    return true;
}

/// remove the trailing chars "\\n" at the end of a string
/// these chars are found at the end gdb stream records.
void
remove_stream_record_trailing_chars (UString &a_str)
{
    if (a_str.size () < 2) {return;}
    UString::size_type i = a_str.size () - 1;
    LOG_D ("stream record: '" << a_str << "' size=" << (int) a_str.size (),
           GDBMI_PARSING_DOMAIN);
    if (a_str[i] == 'n' && a_str[i-1] == '\\') {
        i = i-1;
        a_str.erase (i, 2);
        a_str.append (1, '\n');
    }
}

bool
parse_stream_record (const UString &a_input,
                     UString::size_type a_from,
                     UString::size_type &a_to,
                     Output::StreamRecord &a_record)
{
    UString::size_type cur=a_from, end = a_input.bytes ();

    if (cur >= end) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    UString console, target, log;

    if (a_input.raw ()[cur] == '~') {
        //console stream output
        ++cur;
        if (cur >= end) {
            LOG_PARSING_ERROR (a_input, cur);
            return false;
        }
        if (!parse_c_string (a_input, cur, cur, console)) {
            LOG_PARSING_ERROR (a_input, cur);
            return false;
        }
        SKIP_WS (a_input, cur, cur);
        if (cur + 1 < end
            && a_input.raw ()[cur] == '>'
            && isspace (a_input.raw ()[cur+1])) {
            cur += 2;
        }
        SKIP_BLANK (a_input, cur, cur);
    } else if (a_input.raw ()[cur] == '@') {
        //target stream output
        ++cur;
        if (cur >= end) {
            LOG_PARSING_ERROR (a_input, cur);
            return false;
        }
        if (!parse_c_string (a_input, cur, cur, target)) {
            LOG_PARSING_ERROR (a_input, cur);
            return false;
        }
    } else if (a_input.raw ()[cur] == '&') {
        //log stream output
        ++cur;
        if (cur >= end) {
            LOG_PARSING_ERROR (a_input, cur);
            return false;
        }
        if (!parse_c_string (a_input, cur, cur, log)) {
            LOG_PARSING_ERROR (a_input, cur);
            return false;
        }
    } else {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    for (; cur < end && isspace (a_input.raw ()[cur]); ++cur) {}
    bool found (false);
    if (!console.empty ()) {
        found = true;
        remove_stream_record_trailing_chars (console);
        a_record.debugger_console (console);
    }
    if (!target.empty ()) {
        found = true;
        remove_stream_record_trailing_chars (target);
        a_record.target_output (target);
    }
    if (!log.empty ()) {
        found = true;
        remove_stream_record_trailing_chars (log);
        a_record.debugger_log (log);
    }

    if (!found) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    a_to = cur;
    return true;
}

/// parse GDBMI async output that says that the debugger has
/// stopped.
/// the string looks like:
/// *stopped,reason="foo",var0="foo0",var1="foo1",frame={<a-frame>}
bool
parse_stopped_async_output (const UString &a_input,
                            UString::size_type a_from,
                            UString::size_type &a_to,
                            bool &a_got_frame,
                            IDebugger::Frame &a_frame,
                            map<UString, UString> &a_attrs)
{
    LOG_FUNCTION_SCOPE_NORMAL_D (GDBMI_PARSING_DOMAIN);

    UString::size_type cur=a_from, end=a_input.size ();

    if (cur >= end) {return false;}

    if (a_input.compare (cur, 9,"*stopped,")) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    cur += 9; if (cur >= end) {return false;}

    map<UString, UString> attrs;
    UString name, value;
    bool got_frame (false);
    IDebugger::Frame frame;
    while (true) {
        if (!a_input.compare (cur, strlen (PREFIX_FRAME), PREFIX_FRAME)) {
            if (!parse_frame (a_input, cur, cur, frame)) {
                LOG_PARSING_ERROR (a_input, cur);
                return false;
            }
            got_frame = true;
        } else {
            if (!parse_attribute (a_input, cur, cur, name, value)) {break;}
            attrs[name] = value;
            name.clear (); value.clear ();
        }

        if (cur >= end) {break;}
        if (a_input[cur] == ',') {++cur;}
        if (cur >= end) {break;}
    }

    for (; cur < end && a_input[cur] != '\n'; ++cur) {}

    if (a_input[cur] != '\n') {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }
    ++cur;

    a_got_frame = got_frame;
    if (a_got_frame) {
        a_frame = frame;
    }
    a_to = cur;
    a_attrs = attrs;
    return true;
}

Output::OutOfBandRecord::StopReason
str_to_stopped_reason (const UString &a_str)
{
    if (a_str == "breakpoint-hit") {
        return Output::OutOfBandRecord::BREAKPOINT_HIT;
    } else if (a_str == "watchpoint-trigger") {
        return Output::OutOfBandRecord::WATCHPOINT_TRIGGER;
    } else if (a_str == "read-watchpoint-trigger") {
        return Output::OutOfBandRecord::READ_WATCHPOINT_TRIGGER;
    } else if (a_str == "function-finished") {
        return Output::OutOfBandRecord::FUNCTION_FINISHED;
    } else if (a_str == "location-reached") {
        return Output::OutOfBandRecord::LOCATION_REACHED;
    } else if (a_str == "watchpoint-scope") {
        return Output::OutOfBandRecord::WATCHPOINT_SCOPE;
    } else if (a_str == "end-stepping-range") {
        return Output::OutOfBandRecord::END_STEPPING_RANGE;
    } else if (a_str == "exited-signalled") {
        return Output::OutOfBandRecord::EXITED_SIGNALLED;
    } else if (a_str == "exited") {
        return Output::OutOfBandRecord::EXITED;
    } else if (a_str == "exited-normally") {
        return Output::OutOfBandRecord::EXITED_NORMALLY;
    } else if (a_str == "signal-received") {
        return Output::OutOfBandRecord::SIGNAL_RECEIVED;
    } else {
        return Output::OutOfBandRecord::UNDEFINED;
    }
}

bool
parse_out_of_band_record (const UString &a_input,
                          UString::size_type a_from,
                          UString::size_type &a_to,
                          Output::OutOfBandRecord &a_record)
{
    LOG_FUNCTION_SCOPE_NORMAL_D (GDBMI_PARSING_DOMAIN);

    UString::size_type cur=a_from, end = a_input.bytes ();
    if (cur >= end) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    Output::OutOfBandRecord record;
    if (a_input.raw ()[cur] == '~' ||
        a_input.raw ()[cur] == '@' ||
        a_input.raw ()[cur] == '&') {
        Output::StreamRecord stream_record;
        if (!parse_stream_record (a_input, cur, cur, stream_record)) {
            LOG_PARSING_ERROR (a_input, cur);
            return false;
        }
        record.has_stream_record (true);
        record.stream_record (stream_record);

        while (cur < end && isspace (a_input.raw ()[cur])) {++cur;}
    } else if (a_input.raw ()[cur] == '=') {
        //this is a notification sent by gdb. For now, the only one
        //I have seen like this is of the form:
        //'=thread-created,id=1',
        //and the notification ends with a '\n' character.
        //Of course it is not documented
        //Let's ignore this by now
        while (a_input.raw ()[cur] != '\n') {++cur;}
        ++cur;//consume the '\n' character
    }

    if (!a_input.raw ().compare (cur, 9,"*stopped,")) {
        map<UString, UString> attrs;
        bool got_frame (false);
        IDebugger::Frame frame;
        if (!parse_stopped_async_output (a_input, cur, cur,
                                         got_frame, frame, attrs)) {
            return false;
        }
        record.is_stopped (true);
        record.stop_reason (str_to_stopped_reason (attrs["reason"]));
        if (got_frame) {
            record.frame (frame);
            record.has_frame (true);
        }

        if (attrs.find ("bkptno") != attrs.end ()) {
            record.breakpoint_number (atoi (attrs["bkptno"].c_str ()));
        }
        record.thread_id (atoi (attrs["thread-id"].c_str ()));
        record.signal_type (attrs["signal-name"]);
        record.signal_meaning (attrs["signal-meaning"]);
    }

    while (cur < end && isspace (a_input.raw ()[cur])) {++cur;}
    a_to = cur;
    a_record = record;
    return true;
}

/// parse a GDB/MI result record.
/// a result record is the result of the command that has been issued right
/// before.
bool
parse_result_record (const UString &a_input,
                     UString::size_type a_from,
                     UString::size_type &a_to,
                     Output::ResultRecord &a_record)
{
    LOG_FUNCTION_SCOPE_NORMAL_D (GDBMI_PARSING_DOMAIN);

    UString::size_type cur=a_from, end=a_input.bytes ();
    if (cur == end) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    UString name, value;
    Output::ResultRecord result_record;
    if (!a_input.raw ().compare (cur, strlen (PREFIX_DONE), PREFIX_DONE)) {
        cur += 5;
        result_record.kind (Output::ResultRecord::DONE);


        if (cur < end && a_input.raw ()[cur] == ',') {

fetch_gdbmi_result:
            cur++;
            if (cur >= end) {
                LOG_PARSING_ERROR (a_input, cur);
                return false;
            }

            if (!a_input.raw ().compare (cur,
                                         strlen (PREFIX_BKPT),
                                                 PREFIX_BKPT)) {
                IDebugger::BreakPoint breakpoint;
                if (parse_breakpoint (a_input, cur, cur, breakpoint)) {
                    result_record.breakpoints ()[breakpoint.number ()] =
                    breakpoint;
                }
            } else if (!a_input.compare (cur,
                                         strlen (PREFIX_BREAKPOINT_TABLE),
                                         PREFIX_BREAKPOINT_TABLE)) {
                map<int, IDebugger::BreakPoint> breaks;
                if (parse_breakpoint_table (a_input, cur, cur, breaks)) {
                    result_record.breakpoints () = breaks;
                }
            } else if (!a_input.compare (cur, strlen (PREFIX_THREAD_IDS),
                        PREFIX_THREAD_IDS)) {
                std::list<int> thread_ids;
                if (parse_threads_list (a_input, cur, cur, thread_ids)) {
                    result_record.thread_list (thread_ids);
                }
            } else if (!a_input.raw ().compare
                                            (cur,
                                             strlen (PREFIX_NEW_THREAD_ID),
                                             PREFIX_NEW_THREAD_ID)) {
                IDebugger::Frame frame;
                int thread_id=0;
                if (parse_new_thread_id (a_input, cur, cur,
                                         thread_id, frame)) {
                    //finish this !
                    result_record.thread_id_selected_info (thread_id, frame);
                }
            } else if (!a_input.compare (cur, strlen (PREFIX_FILES),
                        PREFIX_FILES)) {
                vector<UString> files;
                if (!parse_file_list (a_input, cur, cur, files)) {
                    LOG_PARSING_ERROR (a_input, cur);
                    return false;
                }
                result_record.file_list (files);
                LOG_D ("parsed a list of files: "
                       << (int) files.size (),
                       GDBMI_PARSING_DOMAIN);
            } else if (!a_input.raw ().compare (cur, strlen (PREFIX_STACK),
                        PREFIX_STACK)) {
                vector<IDebugger::Frame> call_stack;
                if (!parse_call_stack (a_input, cur, cur, call_stack)) {
                    LOG_PARSING_ERROR (a_input, cur);
                    return false;
                }
                result_record.call_stack (call_stack);
                LOG_D ("parsed a call stack of depth: "
                       << (int) call_stack.size (),
                       GDBMI_PARSING_DOMAIN);
                vector<IDebugger::Frame>::iterator frame_iter;
                for (frame_iter = call_stack.begin ();
                     frame_iter != call_stack.end ();
                     ++frame_iter) {
                    LOG_D ("function-name: " << frame_iter->function_name (),
                           GDBMI_PARSING_DOMAIN);
                }
            } else if (!a_input.raw ().compare (cur,
                                                strlen (PREFIX_FRAME),
                                                        PREFIX_FRAME)) {
                IDebugger::Frame frame;
                if (!parse_frame (a_input, cur, cur, frame)) {
                    LOG_PARSING_ERROR (a_input, cur);
                } else {
                    LOG_D ("parsed result", GDBMI_PARSING_DOMAIN);
                    result_record.current_frame_in_core_stack_trace (frame);
                    //current_frame_signal.emit (frame, "");
                }
            } else if (!a_input.raw ().compare (cur,
                                                strlen (PREFIX_DEPTH),
                                                        PREFIX_DEPTH)) {
                GDBMIResultSafePtr result;
                parse_gdbmi_result (a_input, cur, cur, result);
                THROW_IF_FAIL (result);
                LOG_D ("parsed result", GDBMI_PARSING_DOMAIN);
            } else if (!a_input.raw ().compare (cur,
                                                strlen (PREFIX_STACK_ARGS),
                                                        PREFIX_STACK_ARGS)) {
                map<int, list<IDebugger::VariableSafePtr> > frames_args;
                if (!parse_stack_arguments (a_input, cur,
                                            cur, frames_args)) {
                    LOG_PARSING_ERROR (a_input, cur);
                } else {
                    LOG_D ("parsed stack args", GDBMI_PARSING_DOMAIN);
                }
                result_record.frames_parameters (frames_args) ;
            } else if (!a_input.raw ().compare (cur,
                                                strlen (PREFIX_LOCALS),
                                                        PREFIX_LOCALS)) {
                list<IDebugger::VariableSafePtr> vars;
                if (!parse_local_var_list (a_input, cur, cur, vars)) {
                    LOG_PARSING_ERROR (a_input, cur);
                } else {
                    LOG_D ("parsed local vars", GDBMI_PARSING_DOMAIN);
                    result_record.local_variables (vars);
                }
            } else if (!a_input.raw ().compare (cur,
                                                strlen (PREFIX_VALUE),
                                                PREFIX_VALUE)) {
                // FIXME: this case will parse any response from
                // -data-evaluate-expression, including the response from
                // setting the value of a register or any other expression
                // evaluation.  Currently all of these cases can be parsed as a
                // variable, but there's no guarantee that this is the case.
                // Perhaps this needs to be reworked somehow
                IDebugger::VariableSafePtr var;
                if (!parse_variable_value (a_input, cur, cur, var)) {
                    LOG_PARSING_ERROR (a_input, cur);
                } else {
                    LOG_D ("parsed var value", GDBMI_PARSING_DOMAIN);
                    THROW_IF_FAIL (var);
                    result_record.variable_value (var);
                }
            } else if (!a_input.raw ().compare (cur,
                                                strlen (PREFIX_REGISTER_NAMES),
                                                PREFIX_REGISTER_NAMES)) {
                std::map<IDebugger::register_id_t, UString> regs;
                if (!parse_register_names (a_input, cur, cur, regs)) {
                    LOG_PARSING_ERROR (a_input, cur);
                } else {
                    LOG_D ("parsed register names", GDBMI_PARSING_DOMAIN);
                    result_record.register_names (regs);
                }
            } else if (!a_input.raw ().compare (cur,
                                                strlen (PREFIX_CHANGED_REGISTERS),
                                                PREFIX_CHANGED_REGISTERS)) {
                std::list<IDebugger::register_id_t> regs;
                if (!parse_changed_registers (a_input, cur, cur, regs)) {
                    LOG_PARSING_ERROR (a_input, cur);
                } else {
                    LOG_D ("parsed changed register", GDBMI_PARSING_DOMAIN);
                    result_record.changed_registers (regs);
                }
            } else if (!a_input.raw ().compare (cur,
                                                strlen (PREFIX_REGISTER_VALUES),
                                                PREFIX_REGISTER_VALUES)) {
                std::map<IDebugger::register_id_t, UString>  values;
                if (!parse_register_values (a_input, cur, cur,  values)) {
                    LOG_PARSING_ERROR (a_input, cur);
                } else {
                    LOG_D ("parsed register values", GDBMI_PARSING_DOMAIN);
                    result_record.register_values ( values);
                }
            } else if (!a_input.compare (cur, strlen (PREFIX_MEMORY_VALUES),
                        PREFIX_MEMORY_VALUES)) {
                size_t addr;
                std::vector<uint8_t>  values;
                if (!parse_memory_values (a_input, cur, cur,  addr, values)) {
                    LOG_PARSING_ERROR (a_input, cur);
                } else {
                    LOG_D ("parsed memory values", GDBMI_PARSING_DOMAIN);
                    result_record.memory_values (addr, values);
                }
            } else {
                GDBMIResultSafePtr result;
                if (!parse_gdbmi_result (a_input, cur, cur, result)
                    || !result) {
                    LOG_PARSING_ERROR (a_input, cur);
                } else {
                    LOG_D ("parsed unknown gdbmi result",
                           GDBMI_PARSING_DOMAIN);
                }
            }

            if (a_input.raw ()[cur] == ',') {
                goto fetch_gdbmi_result;
            }

            //skip the remaining things we couldn't parse, until the
            //'end of line' character.
            for (;cur < end && a_input.raw ()[cur] != '\n';++cur) {}
        }
    } else if (!a_input.raw ().compare (cur,
                                        strlen (PREFIX_RUNNING),
                                        PREFIX_RUNNING)) {
        result_record.kind (Output::ResultRecord::RUNNING);
        cur += 8;
        for (;cur < end && a_input.raw ()[cur] != '\n';++cur) {}
    } else if (!a_input.raw ().compare (cur,
                                        strlen (PREFIX_EXIT),
                                        PREFIX_EXIT)) {
        result_record.kind (Output::ResultRecord::EXIT);
        cur += 5;
        for (;cur < end && a_input.raw ()[cur] != '\n';++cur) {}
    } else if (!a_input.raw ().compare (cur,
                                        strlen (PREFIX_CONNECTED),
                                        PREFIX_CONNECTED)) {
        result_record.kind (Output::ResultRecord::CONNECTED);
        cur += 10;
        for (;cur < end && a_input.raw ()[cur] != '\n';++cur) {}
    } else if (!a_input.raw ().compare (cur,
                                        strlen (PREFIX_ERROR),
                                        PREFIX_ERROR)) {
        result_record.kind (Output::ResultRecord::ERROR);
        cur += 6;
        CHECK_END (a_input, cur, end);
        if (cur < end && a_input.raw ()[cur] == ',') {++cur;}
        CHECK_END (a_input, cur, end);
        if (!parse_attribute (a_input, cur, cur, name, value)) {
            LOG_PARSING_ERROR (a_input, cur);
            return false;
        }
        if (name != "") {
            LOG_DD ("got error with attribute: '"
                    << name << "':'" << value << "'");
            result_record.attrs ()[name] = value;
        } else {
            LOG_ERROR ("weird, got error with no attribute. continuing.");
        }
        for (;cur < end && a_input.raw ()[cur] != '\n';++cur) {}
    } else {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    if (a_input.raw ()[cur] != '\n') {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    a_record = result_record;
    a_to = cur;
    return true;
}

/// parse a GDB/MI output record
bool
parse_output_record (const UString &a_input,
                     UString::size_type a_from,
                     UString::size_type &a_to,
                     Output &a_output)
{
    LOG_FUNCTION_SCOPE_NORMAL_D (GDBMI_PARSING_DOMAIN);

    UString::size_type cur=a_from, end=a_input.bytes ();

    if (cur >= end) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    Output output;

fetch_out_of_band_record:
    if (a_input.raw () [cur] == '*'
         || a_input.raw () [cur] == '~'
         || a_input.raw ()[cur] == '@'
         || a_input.raw ()[cur] == '&'
         || a_input.raw ()[cur] == '+'
         || a_input.raw ()[cur] == '=') {
        Output::OutOfBandRecord oo_record;
        if (!parse_out_of_band_record (a_input, cur, cur, oo_record)) {
            LOG_PARSING_ERROR (a_input, cur);
            return false;
        }
        output.has_out_of_band_record (true);
        output.out_of_band_records ().push_back (oo_record);
        goto fetch_out_of_band_record;
    }

    if (cur > end) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    if (a_input.raw ()[cur] == '^') {
        Output::ResultRecord result_record;
        if (parse_result_record (a_input, cur, cur, result_record)) {
            output.has_result_record (true);
            output.result_record (result_record);
        }
        if (cur >= end) {
            LOG_PARSING_ERROR (a_input, cur);
            return false;
        }
    }

    while (cur < end && isspace (a_input.raw ()[cur])) {++cur;}

    if (!a_input.raw ().compare (cur, 5, "(gdb)")) {
        cur += 5;
    }

    if (cur == a_from) {
        //we didn't parse anything
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    while (cur < end && isspace (a_input.raw ()[cur])) {++cur;}

    a_output = output;
    a_to = cur;
    return true;
}

bool
parse_overloads_choice_prompt
                    (const UString &a_input,
                     UString::size_type a_from,
                     UString::size_type &a_to,
                     vector<IDebugger::OverloadsChoiceEntry> &a_prompts)
{
    UString::size_type cur=a_from, end=a_input.size ();
    gunichar c = 0;

    if (cur >= end) {
        LOG_PARSING_ERROR (a_input, cur);
        return false;
    }

    if (a_input[cur] != '[') {
        LOG_PARSING_ERROR (a_input, cur);
    }
    c = a_input.raw ()[cur];

    string index_str;
    vector<IDebugger::OverloadsChoiceEntry> prompts;
    while (c == '[') {
        ++cur;
        CHECK_END (a_input, cur, end);
        index_str.clear ();
        c = a_input.raw ()[cur];
        //look for the numerical index of the current prompt entry
        for (;;) {
            CHECK_END (a_input, cur, end);
            c = a_input.raw ()[cur];
            if (isdigit (c)) {
                index_str += a_input[cur];
            } else if (c == ']') {
                break;
            } else {
                LOG_PARSING_ERROR (a_input, cur);
                return false;
            }
            ++cur;
            if (cur >= end)
                break;
            c = a_input.raw ()[cur];
        }
        //we should have the numerical index of the current prompt entry
        //by now.
        if (index_str.empty ()) {
            LOG_ERROR ("Could not parse prompt index");
            return false;
        }
        LOG_DD ("prompt index: " << index_str);
        ++cur;
        CHECK_END (a_input, cur, end);
        SKIP_WS (a_input, cur, cur);
        c = a_input.raw ()[cur];

        //now parse the prompt value.
        //it is either  "all", "cancel", or a function name/file location.
        IDebugger::OverloadsChoiceEntry entry;
        entry.index (atoi (index_str.c_str ()));
        if (end - cur >= 3
            && !a_input.raw ().compare (cur, 3, "all")) {
            entry.kind (IDebugger::OverloadsChoiceEntry::ALL);
            cur += 3;
            SKIP_WS (a_input, cur, cur);
            c = a_input.raw ()[cur];
            LOG_DD ("pushing entry: " << (int) entry.index ()
                    << ", all" );
            prompts.push_back (entry);
            if (cur+1 < end && c == '\\' && a_input.raw ()[cur+1] == 'n') {
                cur += 2;
                c = a_input.raw ()[cur];
            }
        } else if  (end - cur >= 6
                    && !a_input.compare (cur, 6, "cancel")) {
            entry.kind (IDebugger::OverloadsChoiceEntry::CANCEL);
            cur += 6;
            SKIP_WS (a_input, cur, cur);
            c = a_input.raw ()[cur];
            LOG_DD ("pushing entry: " << (int) entry.index ()
                    << ", cancel");
            prompts.push_back (entry);
            if (cur+1 < end && c == '\\' && a_input.raw ()[cur+1] == 'n') {
                cur += 2;
                c = a_input.raw ()[cur];
            }
        } else {
            //try to parse a breakpoint location
            UString function_name, file_name, line_num;
            UString::size_type b=0, e=0;
            b = cur;
            while (cur < end && a_input.raw ().compare (cur, 4, " at ")) {
                ++cur;
            }
            c = a_input.raw ()[cur];
            if (cur < end && !a_input.raw ().compare (cur, 4, " at ")) {
                e = cur;
            } else {
                LOG_PARSING_ERROR (a_input, cur);
                return false;
            }
            function_name.assign (a_input, b, e-b);

            cur += 4;
            SKIP_WS (a_input, cur, cur);
            c = a_input.raw ()[cur];
            b=cur; e=0;
            while (c != ':') {
                ++cur;
                if (cur >= end)
                    break;
                c = a_input.raw ()[cur];
            }
            if (cur < end && c == ':') {
                e = cur;
            } else {
                LOG_PARSING_ERROR (a_input, cur);
                return false;
            }
            file_name.assign (a_input, b, e-b);
            ++cur;
            SKIP_WS (a_input, cur, cur);
            c = a_input.raw ()[cur];

            while (isdigit (c)) {
                line_num += c;
                ++cur;
                if (cur >= end)
                    break;
                c = a_input.raw ()[cur];
            }
            entry.kind (IDebugger::OverloadsChoiceEntry::LOCATION);
            entry.function_name (function_name);
            entry.file_name (file_name);
            entry.line_number (atoi (line_num.c_str ()));
            LOG_DD ("pushing entry: " << (int) entry.index ()
                    << ", " << entry.function_name ());
            prompts.push_back (entry);
            if (cur >= end) {
                LOG_DD ("reached end, getting out");
                break;
            }
            SKIP_WS (a_input, cur, cur);
            c = a_input.raw ()[cur];
            if (cur+1 < end
                && c == '\\' && a_input.raw ()[cur+1] == 'n') {
                cur += 2;
                c = a_input.raw ()[cur];
            }
            SKIP_WS (a_input, cur, cur);
            c = a_input.raw ()[cur];
            if (cur >= end)
                break;
        }
    }//end while//parse a prompt entry
    if (prompts.empty ())
        return false;
    a_prompts = prompts;
    a_to = cur;
    return true;
}

NEMIVER_END_NAMESPACE (nemiver)


