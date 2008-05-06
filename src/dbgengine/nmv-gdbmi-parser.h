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
#ifndef __NMV_GDBMI_PARSER_H
#define __NMV_GDBMI_PARSER_H
#include <boost/variant.hpp>
#include <iostream>
#include "nmv-i-debugger.h"
#include "nmv-dbg-common.h"

NEMIVER_BEGIN_NAMESPACE (nemiver)

class GDBMITuple ;
class GDBMIResult ;
class GDBMIValue ;
class GDBMIList ;
typedef SafePtr<GDBMIResult, ObjectRef, ObjectUnref> GDBMIResultSafePtr ;
typedef SafePtr<GDBMITuple, ObjectRef, ObjectUnref> GDBMITupleSafePtr ;
typedef SafePtr<GDBMIValue, ObjectRef, ObjectUnref> GDBMIValueSafePtr;
typedef SafePtr<GDBMIList, ObjectRef, ObjectUnref> GDBMIListSafePtr ;

class GDBMITuple : public Object {
    GDBMITuple (const GDBMITuple&) ;
    GDBMITuple& operator= (const GDBMITuple&) ;

    list<GDBMIResultSafePtr> m_content ;

public:

    GDBMITuple () {}
    virtual ~GDBMITuple () {}
    const list<GDBMIResultSafePtr>& content () const {return m_content;}
    void content (const list<GDBMIResultSafePtr> &a_in) {m_content = a_in;}
    void append (const GDBMIResultSafePtr &a_result)
    {
        m_content.push_back (a_result);
    }
    void clear () {m_content.clear ();}
};//end class GDBMITuple

/// A GDB/MI Value.
/// the syntax of a GDB/MI value is:
/// VALUE ==> CONST | TUPLE | LIST
/// In our case, CONST is a UString class, TUPLE is a GDBMITuple class and
/// LIST is a GDBMIList class.
/// please, read the GDB/MI output syntax documentation for more.
class GDBMIValue : public Object {
    GDBMIValue (const GDBMIValue&) ;
    GDBMIValue& operator= (const GDBMIValue&) ;
    typedef boost::variant<bool,
                           UString,
                           GDBMIListSafePtr,
                           GDBMITupleSafePtr> ContentType ;
    ContentType m_content ;
    friend class GDBMIResult ;


public:
    enum Type {
        EMPTY_TYPE=0,
        STRING_TYPE,
        LIST_TYPE,
        TUPLE_TYPE,
    };

    GDBMIValue () {m_content = false;}

    GDBMIValue (const UString &a_str) {m_content = a_str ;}

    GDBMIValue (const GDBMIListSafePtr &a_list)
    {
        m_content = a_list ;
    }

    GDBMIValue (const GDBMITupleSafePtr &a_tuple)
    {
        m_content = a_tuple ;
    }

    Type content_type () const {return (Type) m_content.which ();}

    const UString& get_string_content ()
    {
        THROW_IF_FAIL (content_type () == STRING_TYPE) ;
        return boost::get<UString> (m_content) ;
    }

    const GDBMIListSafePtr get_list_content () const
    {
        THROW_IF_FAIL (content_type () == LIST_TYPE) ;
        return boost::get<GDBMIListSafePtr> (m_content) ;
    }
    GDBMIListSafePtr get_list_content ()
    {
        THROW_IF_FAIL (content_type () == LIST_TYPE) ;
        return boost::get<GDBMIListSafePtr> (m_content) ;
    }

    const GDBMITupleSafePtr get_tuple_content () const
    {
        THROW_IF_FAIL (content_type () == TUPLE_TYPE) ;
        THROW_IF_FAIL (boost::get<GDBMITupleSafePtr> (&m_content)) ;
        return boost::get<GDBMITupleSafePtr> (m_content) ;
    }
    GDBMITupleSafePtr get_tuple_content ()
    {
        THROW_IF_FAIL (content_type () == TUPLE_TYPE) ;
        THROW_IF_FAIL (boost::get<GDBMITupleSafePtr> (&m_content)) ;
        return boost::get<GDBMITupleSafePtr> (m_content) ;
    }


    const ContentType& content () const
    {
        return m_content;
    }
    void content (const ContentType &a_in)
    {
        m_content = a_in;
    }
};//end class value

/// A GDB/MI Result . This is the
/// It syntax looks like VARIABLE=VALUE,
/// where VALUE is a complex type.
class GDBMIResult : public Object {
    GDBMIResult (const GDBMIResult&) ;
    GDBMIResult& operator= (const GDBMIResult&) ;

    UString m_variable ;
    GDBMIValueSafePtr m_value ;

public:

    GDBMIResult () {}
    GDBMIResult (const UString &a_variable,
                 const GDBMIValueSafePtr &a_value) :
        m_variable (a_variable),
        m_value (a_value)
    {}
    virtual ~GDBMIResult () {}
    const UString& variable () const {return m_variable;}
    void variable (const UString& a_in) {m_variable = a_in;}
    const GDBMIValueSafePtr& value () const {return m_value;}
    void value (const GDBMIValueSafePtr &a_in) {m_value = a_in;}
};//end class GDBMIResult

/// A GDB/MI LIST. It can be a list of either GDB/MI Result or GDB/MI Value.
class GDBMIList : public Object {
    GDBMIList (const GDBMIList &) ;
    GDBMIList& operator= (const GDBMIList &) ;

    //boost::variant<list<GDBMIResultSafePtr>, list<GDBMIValueSafePtr> > m_content ;
    list<boost::variant<GDBMIResultSafePtr, GDBMIValueSafePtr> >  m_content ;
    bool m_empty ;

public:
    enum ContentType {
        RESULT_TYPE=0,
        VALUE_TYPE,
        UNDEFINED_TYPE
    };

    GDBMIList () :
        m_empty (true)
    {}

    GDBMIList (const GDBMITupleSafePtr &a_tuple) :
        m_empty (false)
    {
        GDBMIValueSafePtr value (new GDBMIValue (a_tuple)) ;
        //list<GDBMIValueSafePtr> value_list ; value_list.push_back (value) ;
        //list<GDBMIValueSafePtr> value_list ; value_list.push_back (value) ;
        //m_content = value_list ;
        m_content.push_back (value) ;
    }

    GDBMIList (const UString &a_str) :
        m_empty (false)
    {
        GDBMIValueSafePtr value (new GDBMIValue (a_str)) ;
        //list<GDBMIValueSafePtr> list ;
        //list.push_back (value) ;
        //m_content = list ;
        m_content.push_back (value) ;
    }

    GDBMIList (const GDBMIResultSafePtr &a_result) :
        m_empty (false)
    {
        //list<GDBMIResultSafePtr> list ;
        //list.push_back (a_result) ;
        //m_content = list ;
        m_content.push_back (a_result) ;
    }

    GDBMIList (const GDBMIValueSafePtr &a_value) :
        m_empty (false)
    {
        //list<GDBMIValueSafePtr> list ;
        //list.push_back (a_value) ;
        //m_content = list ;
        m_content.push_back (a_value) ;
    }

    virtual ~GDBMIList () {}
    ContentType content_type () const
    {
        if (m_content.empty ()) {
            return UNDEFINED_TYPE ;
        }
        return (ContentType) m_content.front ().which ();
    }

    bool empty () const {return m_empty;}

    void append (const GDBMIResultSafePtr &a_result)
    {
        THROW_IF_FAIL (a_result) ;
        if (!m_content.empty ()) {
            THROW_IF_FAIL (m_content.front ().which () == RESULT_TYPE) ;
        }
        m_content.push_back (a_result) ;
        m_empty = false ;
    }
    void append (const GDBMIValueSafePtr &a_value)
    {
        THROW_IF_FAIL (a_value) ;
        if (!m_content.empty ()) {
            THROW_IF_FAIL (m_content.front ().which () == VALUE_TYPE) ;
        }
        m_content.push_back (a_value) ;
        m_empty = false ;
    }

    void get_result_content (list<GDBMIResultSafePtr> &a_list) const
    {
        if (empty ()) {return;}
        THROW_IF_FAIL (content_type () == RESULT_TYPE) ;
        list<boost::variant<GDBMIResultSafePtr,GDBMIValueSafePtr> >::const_iterator it;
        for (it= m_content.begin () ; it!= m_content.end () ; ++it) {
            a_list.push_back (boost::get<GDBMIResultSafePtr> (*it)) ;
        }
    }

    void get_value_content (list<GDBMIValueSafePtr> &a_list) const
    {
        if (empty ()) {return;}
        THROW_IF_FAIL (content_type () == VALUE_TYPE) ;
        list<boost::variant<GDBMIResultSafePtr,GDBMIValueSafePtr> >::const_iterator it;
        for (it= m_content.begin () ; it!= m_content.end () ; ++it) {
            a_list.push_back (boost::get<GDBMIValueSafePtr> (*it)) ;
        }
    }
};//end class GDBMIList

//******************************************
//<gdbmi datastructure streaming operators>
//******************************************

ostream& operator<< (ostream &a_out, const GDBMIValueSafePtr &a_val) ;

ostream& operator<< (ostream &a_out, const GDBMIResultSafePtr &a_result) ;

ostream& operator<< (ostream &a_out, const GDBMITupleSafePtr &a_tuple) ;

ostream&
operator<< (ostream &a_out, const GDBMIListSafePtr a_list) ;

ostream&
operator<< (ostream &a_out, const GDBMIValueSafePtr &a_val) ;

std::ostream&
operator<< (std::ostream &a_out, const IDebugger::Variable &a_var) ;

//******************************************
//</gdbmi datastructure streaming operators>
//******************************************


//**************************
//GDBMI parsing functions
//**************************

/// parse a GDB/MI Result data structure.
/// A result basically has the form:
/// variable=value.
/// Beware value is more complicated than what it looks like :-)
/// Look at the GDB/MI spec for more.
bool parse_gdbmi_result (const UString &a_input,
                         UString::size_type a_from,
                         UString::size_type &a_to,
                         GDBMIResultSafePtr &a_value) ;

/// parse a GDB/MI Tuple is a actualy a set of name=value constructs,
/// where 'value' can be quite complicated.
/// Look at the GDB/MI output syntax for more.
bool parse_gdbmi_tuple (const UString &a_input,
                        UString::size_type a_from,
                        UString::size_type &a_to,
                        GDBMITupleSafePtr &a_tuple) ;

/// Parse a GDB/MI LIST. A list is either a list of GDB/MI Results
/// or a list of GDB/MI Values.
/// Look at the GDB/MI output syntax documentation for more.
bool parse_gdbmi_list (const UString &a_input,
                       UString::size_type a_from,
                       UString::size_type &a_to,
                       GDBMIListSafePtr &a_list) ;

/// parse a GDB/MI Result data structure.
/// A result basically has the form:
/// variable=value.
/// Beware value is more complicated than what it looks like :-)
/// Look at the GDB/MI spec for more.
bool parse_gdbmi_result (const UString &a_input,
                         UString::size_type a_from,
                         UString::size_type &a_to,
                         GDBMIResultSafePtr &a_value) ;

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
bool parse_gdbmi_value (const UString &a_input,
                        UString::size_type a_from,
                        UString::size_type &a_to,
                        GDBMIValueSafePtr &a_value) ;

/// parse a GDB/MI output record
bool parse_output_record (const UString &a_input,
                          UString::size_type a_from,
                          UString::size_type &a_to,
                          Output &a_output) ;

bool parse_stream_record (const UString &a_input,
                          UString::size_type a_from,
                          UString::size_type &a_to,
                          Output::StreamRecord &a_record) ;

/// parse GDBMI async output that says that the debugger has
/// stopped.
/// the string looks like:
/// *stopped,reason="foo",var0="foo0",var1="foo1",frame={<a-frame>}
bool parse_stopped_async_output (const UString &a_input,
                                 UString::size_type a_from,
                                 UString::size_type &a_to,
                                 bool &a_got_frame,
                                 IDebugger::Frame &a_frame,
                                 map<UString, UString> &a_attrs);

bool parse_out_of_band_record (const UString &a_input,
                               UString::size_type a_from,
                               UString::size_type &a_to,
                               Output::OutOfBandRecord &a_record);

/// parse a GDB/MI output record
bool parse_output_record (const UString &a_input,
                          UString::size_type a_from,
                          UString::size_type &a_to,
                          Output &a_output);

bool parse_attribute (const UString &a_input,
                      UString::size_type a_from,
                      UString::size_type &a_to,
                      UString &a_name,
                      UString &a_value) ;

/// \brief parses an attribute list
///
/// An attribute list has the form:
/// attr0="val0",attr1="bal1",attr2="val2"
bool parse_attributes (const UString &a_input,
                       UString::size_type a_from,
                       UString::size_type &a_to,
                       map<UString, UString> &a_attrs);

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
bool parse_breakpoint (const UString &a_input,
                       Glib::ustring::size_type a_from,
                       Glib::ustring::size_type &a_to,
                       IDebugger::BreakPoint &a_bkpt) ;

bool parse_breakpoint_table (const UString &a_input,
                             UString::size_type a_from,
                             UString::size_type &a_to,
                             map<int, IDebugger::BreakPoint> &a_breakpoints) ;

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
bool parse_frame (const UString &a_input,
                  UString::size_type a_from,
                  UString::size_type &a_to,
                  IDebugger::Frame &a_frame) ;

/// parse a callstack as returned by the gdb/mi command:
/// -stack-list-frames
bool parse_call_stack (const UString &a_input,
                       const UString::size_type a_from,
                       UString::size_type &a_to,
                       vector<IDebugger::Frame> &a_stack) ;

/// Parse the arguments of the call stack.
/// The call stack arguments is the result of the
/// GDB/MI command -stack-list-arguments 1.
/// It is basically the arguments of the functions of the call stack.
/// See the GDB/MI documentation for more.
bool parse_stack_arguments
                    (const UString &a_input,
                     UString::size_type a_from,
                     UString::size_type &a_to,
                     map<int, list<IDebugger::VariableSafePtr> > &a_params) ;

/// parse a list of local variables as returned by
/// the GDBMI command -stack-list-locals 2
bool parse_local_var_list
                    (const UString &a_input,
                     UString::size_type a_from,
                     UString::size_type &a_to,
                     list<IDebugger::VariableSafePtr> &a_vars) ;

/// parse the result of -data-evaluate-expression <var-name>
/// the result is a gdbmi result of the form:
/// value={attrname0=val0, attrname1=val1,...} where val0 and val1
/// can be simili tuples representing complex types as well.
bool parse_variable_value (const UString &a_input,
                           const UString::size_type a_from,
                           UString::size_type &a_to,
                           IDebugger::VariableSafePtr &a_var) ;


bool parse_member_variable (const UString &a_input,
                            const UString::size_type a_from,
                            UString::size_type &a_to,
                            IDebugger::VariableSafePtr &a_var,
                            bool a_in_unnamed_var=false) ;

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
bool parse_function_args (const UString &a_input,
                          UString::size_type a_from,
                          UString::size_type &a_to,
                          map<UString, UString> a_args) ;

/// parses the result of the gdbmi command
/// "-thread-list-ids".
bool parse_threads_list (const UString &a_input,
                         UString::size_type a_from,
                         UString::size_type &a_to,
                         std::list<int> &a_thread_ids) ;

/// parses the result of the gdbmi command
/// "-file-list-exec-source-files".
bool parse_file_list (const UString &a_input,
                      UString::size_type a_from,
                      UString::size_type &a_to,
                      std::vector<UString> &a_files) ;

/// parses the result of the gdbmi command
/// "-thread-select"
/// \param a_input the input string to parse
/// \param a_from the offset from where to start the parsing
/// \param a_to. out parameter. The next offset after the end of what
/// got parsed.
/// \param a_thread_id out parameter. The id of the selected thread.
/// \param a_frame out parameter. The current frame in the selected thread.
/// \param a_level out parameter. the level
bool parse_new_thread_id (const UString &a_input,
                          UString::size_type a_from,
                          UString::size_type &a_to,
                          int &a_thread_id,
                          IDebugger::Frame &a_frame);

/// parses a string that has the form:
/// \"blah\"
bool parse_c_string (const UString &a_input,
                     UString::size_type a_from,
                     UString::size_type &a_to,
                     UString &a_c_string);


/// parses a string that has the form:
/// blah
bool parse_string (const UString &a_input,
                   UString::size_type a_from,
                   UString::size_type &a_to,
                   UString &a_string);

bool parse_embedded_c_string (const UString &a_input,
                              UString::size_type a_from,
                              UString::size_type &a_to,
                              UString &a_string) ;


bool parse_overloads_choice_prompt
                        (const UString &a_input,
                         UString::size_type a_from,
                         UString::size_type &a_to,
                         vector<IDebugger::OverloadsChoiceEntry> &a_prompts) ;

bool parse_register_names (const UString &a_input,
                           UString::size_type a_from,
                           UString::size_type &a_to,
                           std::map<IDebugger::register_id_t, UString> &a_registers);

bool parse_changed_registers (const UString &a_input,
                              UString::size_type a_from,
                              UString::size_type &a_to,
                              std::list<IDebugger::register_id_t> &a_registers);

bool parse_register_values (const UString &a_input,
                            UString::size_type a_from,
                            UString::size_type &a_to,
                            std::map<IDebugger::register_id_t, UString> &a_values);

bool parse_memory_values (const UString &a_input,
                          UString::size_type a_from,
                          UString::size_type &a_to,
                          size_t& a_start_addr,
                          std::vector<uint8_t> &a_values);

NEMIVER_END_NAMESPACE (nemiver)
#endif //__NMV_GDBMI_PARSER_H

