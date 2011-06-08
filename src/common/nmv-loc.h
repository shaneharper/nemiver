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

#ifndef __NMV_LOC_H__
#define __NMV_LOC_H__

#include "nmv-ustring.h"
#include "nmv-address.h"
#include "nmv-api-macros.h"

/// \file
/// The declaration of source, function and addresses related location
/// types.  For now, it's only the debugging engine backend that knows
/// how to serialize a location into a location string.

NEMIVER_BEGIN_NAMESPACE (nemiver)
NEMIVER_BEGIN_NAMESPACE (common)

///  The base type presenting a location.
class NEMIVER_API Loc
{

 public:
    /// The different possible kinds of locations.
    enum Kind
    {
        UNDEFINED_LOC_KIND,
        SOURCE_LOC_KIND,
        FUNCTION_LOC_KIND,
        ADDRESS_LOC_KIND
    };

 protected:
    Kind m_kind;

    Loc ()
        : m_kind (UNDEFINED_LOC_KIND)
    {
    }

    Loc (const Loc &a)
      : m_kind (a.m_kind)
    {
    }

 public:

    Kind kind () const {return m_kind;}
    virtual ~Loc () {};
}; // end class Loc

/// A source location.
/// 
/// It encapsulates a file name and a line number
/// in that file.
class NEMIVER_API SourceLoc : public Loc
{
    UString m_file_path;
    int m_line_number;

 public:

    SourceLoc (const UString &a_file_path,
               int a_line_number)
        : m_file_path (a_file_path),
        m_line_number (a_line_number)
    {
        m_kind = SOURCE_LOC_KIND;
    }

    SourceLoc (const SourceLoc &a)
      : Loc (a)
    {
        m_file_path = a.m_file_path;
        m_line_number = a.m_line_number;
    }

    const UString& file_path () const {return m_file_path;}
    void file_path (const UString &a) {m_file_path = a;}
    int line_number () const {return m_line_number;}
    void line_number (int a) {m_line_number = a;}
};// end class SourceLoc;

/// A function location.
///
/// It encapsulates the location of a function.
class NEMIVER_API FunctionLoc : public Loc
{
    UString m_function_name;

 public:

    FunctionLoc (const UString &a_function_name)
        : m_function_name (a_function_name)
    {
        m_kind = FUNCTION_LOC_KIND;
    }

    FunctionLoc (const FunctionLoc &a)
      : Loc (a),
      m_function_name (a.m_function_name)
    {
    }

    const UString& function_name () const {return m_function_name;}
    void function_name (const UString &a) {m_function_name = a;}
};

/// An address location.
///
/// It encapslates an address.
class NEMIVER_API AddressLoc : public Loc
{
    Address m_address;

 public:

    AddressLoc (const Address &a)      
      : m_address (a)
    {
        m_kind = ADDRESS_LOC_KIND;
    }

    AddressLoc (const AddressLoc &a)
      : Loc (a),
      m_address (a.m_address)
    {
    }

    const Address& address () const {return m_address;}
    void address (const Address &a) {m_address = a;}
};

NEMIVER_END_NAMESPACE (common)
NEMIVER_END_NAMESPACE (nemiver)

#endif //__NMV_LOC_H__
