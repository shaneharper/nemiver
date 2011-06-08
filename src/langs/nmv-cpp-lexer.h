//Author: Dodji Seketeli <dodji@gnome.org>
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
#ifndef __NMV_CPP_LEXER_H__
#define __NMV_CPP_LEXER_H__

#include <string>
#include "nmv-cpp-ast.h"
#include "common/nmv-namespace.h"
#include "common/nmv-api-macros.h"

using std::string;
using nemiver::cpp::Token;

NEMIVER_BEGIN_NAMESPACE (nemiver)
NEMIVER_BEGIN_NAMESPACE (cpp)


class NEMIVER_API Lexer {
    Lexer (const Lexer &);
    Lexer& operator=(const Lexer&);
    struct Priv;
    Priv *m_priv;

private:
    /// \name token scanning helpers
    /// @{

    bool next_is (const char *a_char_seq) const;
    void skip_blanks ();
    bool is_nondigit (const char a_in) const;
    bool is_digit (const char a_in) const;
    bool is_nonzero_digit (const char a_in) const;
    bool is_octal_digit (const char a_in) const;
    bool is_hexadecimal_digit (const char) const;
    int  hexadigit_to_decimal (const char) const;

    bool scan_decimal_literal (string &a_result);
    bool scan_octal_literal (string &a_result);
    bool scan_hexadecimal_literal (string &a_result);
    bool scan_hexquad (string &a_result);
    bool scan_integer_suffix (string &a_result);
    bool scan_integer_literal (string &a_result);
    bool scan_simple_escape_sequence (int &a_result);
    bool scan_octal_escape_sequence (int &a_result);
    bool scan_hexadecimal_escape_sequence (int &a_result);
    bool scan_escape_sequence (int &a_result);
    bool scan_hexquad (int &a_result);
    bool scan_universal_character_name (int &a_result);
    bool scan_c_char (int &a_result);
    bool scan_c_char_sequence (string &a_result);
    bool scan_s_char (int &a_result);
    bool scan_s_char_sequence (string &a_result);
    bool scan_character_literal (string &a_result);
    bool scan_digit_sequence (string &a_result);
    bool scan_fractional_constant (string &a_result);
    bool scan_exponent_part (string &a_result);
    bool scan_floating_literal (string &a_result,
                                string &a_exponent);
    bool scan_string_literal (string &a_result);
    bool scan_boolean_literal (bool &a_result);
    /// @}

    /// \name scanning methods
    /// \@{
    bool scan_identifier (Token &a_token);
    bool scan_keyword (Token &a_token);
    bool scan_literal (Token &a_token);
    bool scan_operator (Token &a_token);
    bool scan_punctuator (Token &a_token);
    bool scan_next_token (Token &a_token);
    /// \@}

    /// \name recording/restoring position in the  char input stream
    void record_ci_position ();
    void restore_ci_position ();
    void pop_recorded_ci_position ();

public:
    Lexer (const string &a_in);
    ~Lexer ();


    /// \name peeking/consuming tokens
    /// @{
    bool peek_next_token (Token &a_token);
    bool peek_nth_token (unsigned nth, Token &a_token);
    bool consume_next_token ();
    bool consume_next_token (Token &a_token);
    bool reached_eof () const;
    /// @}

    unsigned get_token_stream_mark () const;
    void rewind_to_mark (unsigned a_mark);
};//end class Lexer

NEMIVER_END_NAMESPACE (cpp)
NEMIVER_END_NAMESPACE (nemiver)

#endif /*__NMV_CPP_LEXER_H__*/

