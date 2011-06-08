// Author: Dodji Seketeli
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
#ifndef __NMV_ASM_INSTR_H__
#define __NMV_ASM_INSTR_H__

#include <list>
#include <boost/variant.hpp>
#include "nmv-namespace.h"
#include "nmv-api-macros.h"
#include "nmv-exception.h"

NEMIVER_BEGIN_NAMESPACE (nemiver)
NEMIVER_BEGIN_NAMESPACE (common)

/// Assembly instruction type
/// It carries the address of the instruction,
/// the function the instruction is from, offset of the instruction
/// starting from the beginning of the function, and the instruction
/// itself, represented by a string.
class AsmInstr {
  string m_address;
  string m_func;
  string m_offset;
  string m_instr;

 public:
  explicit AsmInstr ()
  {
  }

  AsmInstr (const string &a_address,
	    const string &a_func,
	    const string &a_offset,
	    const string &a_instr):
  m_address (a_address),
    m_func (a_func),
    m_offset (a_offset),
    m_instr (a_instr)
    {
    }

  virtual ~AsmInstr ()
    {
    }

  const string& address () const {return m_address;}
  void address (string &a) {m_address = a;}

  const string& function () const {return m_func;}
  void function (const string &a_str) {m_func = a_str;}

  const string& offset () const {return m_offset;}
  void offset (string &a_o) {m_offset = a_o;}

  const string& instruction () const {return m_instr;}
  void instruction (const string &a_instr) {m_instr = a_instr;}
};//end class AsmInstr

class MixedAsmInstr {
  // No need of copy constructor or assignment operator yet.
  UString m_file_path;
  int m_line_number;
  list<AsmInstr> m_instrs;

 public:

  MixedAsmInstr () :
  m_line_number (-1)
    {
    }

  MixedAsmInstr (const UString &a_path,
		 int a_line_num) :
  m_file_path (a_path),
    m_line_number (a_line_num)
    {
    }

  MixedAsmInstr (const UString &a_path,
		 int a_line_num,
		 list<AsmInstr> &a_instrs) :
  m_file_path (a_path),
    m_line_number (a_line_num),
    m_instrs (a_instrs)
    {
    }

  const UString& file_path () const {return m_file_path;}
  void file_path (const UString &a) {m_file_path = a;}

  int line_number () const {return m_line_number;}
  void line_number (int a) {m_line_number = a;}

  const list<AsmInstr>& instrs () const {return m_instrs;}
  list<AsmInstr>& instrs () {return m_instrs;}
  void instrs (const list<AsmInstr> &a) {m_instrs = a;}
};

// Okay, so the result of a call to IDebugger::disassemble returns a
// list of Asm. An Asm is a variant type that is either an AsmInstr
// (a pure asm instruction) or a mixed asm/source that is basically
// a source code locus associated to the list of AsmInstr it
// generated. Asm::which will return an ASM_TYPE_PURE if it's an
// AsmInstr or ASM_TYPE_MIXED if it's an MixedAsmInstr.
class Asm {
  boost::variant<AsmInstr, MixedAsmInstr> m_asm;

 public:
  enum Type {
    TYPE_PURE = 0,
    TYPE_MIXED
  };

  Asm (const AsmInstr &a) :
  m_asm (a)
  {
  }

  Asm (const MixedAsmInstr &a) :
  m_asm (a)
  {
  }

  Type which () const
  {
    return static_cast<Type> (m_asm.which ());
  }

  bool empty () const
  {
    switch (which ()) {
    case TYPE_PURE: {
      const AsmInstr instr = boost::get<AsmInstr> (m_asm);
      return instr.address ().empty ();
    }
      break;
    case TYPE_MIXED: {
      const MixedAsmInstr &mixed =
	boost::get<MixedAsmInstr> (m_asm);
      return mixed.instrs ().empty ();
    }
    default:
      THROW ("unknown asm type");
    }
    return false;
  }

  const AsmInstr& instr () const
  {
    switch (which ()) {
    case TYPE_PURE:
      return boost::get<AsmInstr> (m_asm);
    case TYPE_MIXED: {
      const MixedAsmInstr &mixed =
	boost::get<MixedAsmInstr> (m_asm);
      if (mixed.instrs ().empty ()) {
	stringstream msg;
	msg << "mixed asm has empty instrs at "
	    << mixed.file_path ()
	    << ":"
	    << mixed.line_number ();
	THROW (msg.str ());
      }
      return mixed.instrs ().front ();
    }
    default:
      break;
    }
    THROW ("reached unreachable");
  }

  const MixedAsmInstr& mixed_instr () const
  {
    THROW_IF_FAIL (which () == TYPE_MIXED);
    return boost::get<MixedAsmInstr> (m_asm);
  }
};

class DisassembleInfo {
  // no need of copy constructor yet,
  // as we don't have any pointer member.
  UString m_function_name;
  UString m_file_name;
  string m_start_address;
  string m_end_address;

 public:
  DisassembleInfo ()
    {
    }
  ~DisassembleInfo ()
    {
    }

  const UString& function_name () const {return m_function_name;}
  void function_name (const UString &a_name) {m_function_name = a_name;}

  const UString& file_name () const {return m_file_name;}
  void file_name (const UString &a_name) {m_file_name = a_name;}

  const std::string& start_address () const {return m_start_address;}
  void start_address (const std::string &a) {m_start_address = a;}

  const std::string& end_address () const {return m_end_address;}
  void end_address (const std::string &a) {m_end_address = a;}
};// end class DisassembleInfo

NEMIVER_END_NAMESPACE (common)
NEMIVER_END_NAMESPACE (nemiver)

#endif // __NMV_ASM_INSTR_H__
