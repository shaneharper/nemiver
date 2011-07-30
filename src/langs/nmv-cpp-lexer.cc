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
#include <cstring>
#include <deque>
#include "nmv-cpp-lexer.h"

using std::deque;

NEMIVER_BEGIN_NAMESPACE (nemiver)
NEMIVER_BEGIN_NAMESPACE (cpp)

//********************
//<class Token implem>
//********************
//********************
//</class Token implem>
//********************

//********************
//<class Lexer implem>
//********************
struct Lexer::Priv {
    string input;
    //points to the current character in the char stream
    string::size_type cursor;
    deque<string::size_type> recorded_positions;
    deque<Token> tokens_queue;
    //points to the next token in the token stream;
    deque<Token>::size_type token_cursor;

    Priv (const string &a_in) :
        input (a_in),
        cursor (0),
        token_cursor (0)
    {
    }

    Priv () :
        cursor (0),
        token_cursor (0)
    {
    }
};//end struct Lexer::Priv

#define CUR m_priv->cursor

#define INPUT m_priv->input

#define CUR_CHAR INPUT[CUR]

#define IN_BOUNDS(cursor) \
((cursor) < m_priv->input.size ())

#define CURSOR_IN_BOUNDS (CUR < INPUT.size ())

#define CHECK_CURSOR_BOUNDS \
if (!IN_BOUNDS (CUR)) {return false;}

#define CHECK_CURSOR_BOUNDS2 \
if (!IN_BOUNDS (CUR)) {return;}

#define SCAN_KEYWORD(len,key) \
((!(INPUT.compare (CUR, len, key)) and (key_length=len)))

#define MOVE_FORWARD(nb) CUR += nb

#define MOVE_FORWARD_AND_CHECK(nb) {CUR+=nb;if (!IN_BOUNDS(CUR)) {goto error;}}

#define CONSUME_CHAR MOVE_FORWARD(1)

#define CONSUME_CHAR_AND_CHECK {MOVE_FORWARD(1);if (!CURSOR_IN_BOUNDS) {goto error;}}

Lexer::Lexer (const string &a_in)
{
    m_priv = new Lexer::Priv (a_in);
}

Lexer::~Lexer ()
{
    if (m_priv) {
        delete m_priv;
        m_priv = 0;
    }
}

bool
Lexer::next_is (const char *a_char_seq) const
{
    CHECK_CURSOR_BOUNDS

    if (!a_char_seq)
        return false;

    int len = strlen (a_char_seq);
    if (!len)
        return false;
    if (IN_BOUNDS (CUR + len - 1) && !INPUT.compare (CUR, len, a_char_seq)) {
        return true;
    }
    return false;
}

void
Lexer::skip_blanks ()
{
    CHECK_CURSOR_BOUNDS2;

    while (CURSOR_IN_BOUNDS && isblank (CUR_CHAR)) {
        MOVE_FORWARD (1);
    }
}

bool
Lexer::is_nondigit (const char a_in) const
{
    if (a_in == '_' ||
        (a_in >= 'A' && a_in <= 'Z') ||
        (a_in >= 'a' && a_in <= 'z')) {
        return true;
    }
    return false;
}

bool
Lexer::is_digit (const char a_in) const
{
    if (a_in >= '0' and a_in <= '9')
        return true;
    return false;
}

bool
Lexer::is_nonzero_digit (const char a_in) const
{
    if (a_in >= '1' and a_in <= '9')
        return true;
    return false;
}

bool
Lexer::is_octal_digit (const char a_in) const
{
    if (a_in >= '0' && a_in <= '7')
        return true;
    return false;
}

bool
Lexer::is_hexadecimal_digit (const char a_in) const
{
    if (is_digit (a_in) ||
        (a_in >= 'a' && a_in <= 'f') ||
        (a_in >= 'A' && a_in <= 'F')) {
        return true;
    }
    return false;
}

int
Lexer::hexadigit_to_decimal (const char a_hexa) const
{
    if (a_hexa >= '0' && a_hexa <= '9') {
        return a_hexa - '0';
    } else if (a_hexa >= 'a' && a_hexa <= 'f') {
        return 10 + a_hexa - 'a';
    } else if (a_hexa >= 'A' && a_hexa <= 'F') {
        return 10 + a_hexa - 'A';
    }
    return -1;
}

bool
Lexer::scan_decimal_literal (string &a_result)
{
    CHECK_CURSOR_BOUNDS
    record_ci_position ();

    string result;
    if (is_nonzero_digit (CUR_CHAR)) {
        result += CUR_CHAR;
        CONSUME_CHAR;
    } else {
        goto error;
    }
    while (CURSOR_IN_BOUNDS && is_digit (CUR_CHAR)) {
        result += CUR_CHAR;
        CONSUME_CHAR;
    }
    a_result = result;
    pop_recorded_ci_position ();
    return true;

error:
    restore_ci_position ();
    return false;
}

bool
Lexer::scan_octal_literal (string &a_result)
{
    CHECK_CURSOR_BOUNDS;
    record_ci_position ();

    string result;
    if (CUR_CHAR != '0')
        goto error;

    result += CUR_CHAR;
    CONSUME_CHAR;

    while (CURSOR_IN_BOUNDS && is_octal_digit (CUR_CHAR)) {
        result += CUR_CHAR;
        CONSUME_CHAR;
    }
    a_result = result;
    pop_recorded_ci_position ();
    return true;

error:
    restore_ci_position ();
    return false;
}

bool
Lexer::scan_hexadecimal_literal (string &a_result)
{
    CHECK_CURSOR_BOUNDS
    record_ci_position ();

    string result;
    if (IN_BOUNDS (CUR+1)
        && INPUT[CUR] == '0'
        && (INPUT[CUR+1] == 'x' || INPUT[CUR+1] == 'X')) {
        MOVE_FORWARD_AND_CHECK (2);
    }
    while (CURSOR_IN_BOUNDS && is_hexadecimal_digit (CUR_CHAR)) {
        result +=  CUR_CHAR;
        CONSUME_CHAR;
    }
    if (result.empty ()) {
        goto error;
    }
    a_result = result;
    pop_recorded_ci_position ();
    return true;

error:
    restore_ci_position ();
    return false;
}

bool
Lexer::scan_escape_sequence (int &a_result)
{
    CHECK_CURSOR_BOUNDS

    if (scan_simple_escape_sequence (a_result) ||
        scan_octal_escape_sequence (a_result)  ||
        scan_hexadecimal_escape_sequence (a_result)) {
        return true;
    }
    return false;
}

bool
Lexer::scan_universal_character_name (int &a_result)
{
    CHECK_CURSOR_BOUNDS
    record_ci_position ();

    if (!IN_BOUNDS (CUR+5) ||
        INPUT[CUR] != '\\' ||
        (INPUT[CUR+1] != 'U' && INPUT[CUR+1] != 'u')) {
        return false;
    }
    MOVE_FORWARD_AND_CHECK (2);
    if (!scan_hexquad (a_result)) {
        goto error;
    }
    pop_recorded_ci_position ();
    return true;

error:
    restore_ci_position ();
    return false;
}

bool
Lexer::scan_c_char (int &a_result)
{
    CHECK_CURSOR_BOUNDS

    if (CUR_CHAR != '\\' && CUR_CHAR != '\'' && CUR_CHAR != '\n') {
        a_result = CUR_CHAR;
        CONSUME_CHAR;
        return true;
    } else {
        if (scan_escape_sequence (a_result) ||
            scan_universal_character_name (a_result)) {
            return true;
        }
    }
    return false;
}

bool
Lexer::scan_c_char_sequence (string &a_result)
{
    CHECK_CURSOR_BOUNDS
    int c=0;

    if (!scan_c_char (c)) {
        return false;
    }
    a_result = c;
    while (CURSOR_IN_BOUNDS && scan_c_char (c)) {
        a_result += c;
    }
    return true;
}

bool
Lexer::scan_s_char (int &a_result)
{
    CHECK_CURSOR_BOUNDS

    if (CUR_CHAR != '\\' && CUR_CHAR != '"' && CUR_CHAR != '\n') {
        a_result = CUR_CHAR;
        CONSUME_CHAR;
        return true;
    } else {
        if (scan_escape_sequence (a_result) ||
            scan_universal_character_name (a_result)) {
            return true;
        }
    }
    return false;
}

bool
Lexer::scan_s_char_sequence (string &a_result)
{
    CHECK_CURSOR_BOUNDS
    int c=0;

    if (!scan_s_char (c)) {
        return false;
    }
    a_result = c;
    while (CURSOR_IN_BOUNDS && scan_s_char (c)) {
        a_result += c;
    }
    return true;
}

bool
Lexer::scan_character_literal (string &a_result)
{
    CHECK_CURSOR_BOUNDS
    record_ci_position ();

    string result;
    if (CUR_CHAR == 'L') {
        CONSUME_CHAR_AND_CHECK;
    }
    if (CUR_CHAR == '\'') {
        CONSUME_CHAR_AND_CHECK;
    } else {
        goto error;
    }
    if (!scan_c_char_sequence (result)) {
        goto error;
    }
    if (CUR_CHAR == '\'') {
        CONSUME_CHAR;
    } else {
        goto error;
    }
    a_result = result;
    pop_recorded_ci_position ();
    return true;

error:
    restore_ci_position ();
    return false;
}

bool
Lexer::scan_digit_sequence (string &a_result)
{
    CHECK_CURSOR_BOUNDS
    record_ci_position ();

    string result;
    while (CURSOR_IN_BOUNDS && is_digit (CUR_CHAR)) {
        result += CUR_CHAR;
        CONSUME_CHAR;
    }
    if (result.empty ()) {
        goto error;
    }

    a_result = result;
    pop_recorded_ci_position ();
    return true;

error:
    restore_ci_position ();
    return false;
}

bool
Lexer::scan_fractional_constant (string &a_result)
{
    CHECK_CURSOR_BOUNDS
    record_ci_position ();

    string left, right;

    scan_digit_sequence (left);
    if (CUR_CHAR != '.') {
        goto error;
    }
    CONSUME_CHAR_AND_CHECK;
    if (!scan_digit_sequence (right) && left.empty ()) {
        goto error;
    }
    a_result = left + "." + right;
    pop_recorded_ci_position ();
    return true;

error:
    restore_ci_position ();
    return false;
}

bool
Lexer::scan_exponent_part (string &a_result)
{
    CHECK_CURSOR_BOUNDS
    record_ci_position ();

    string result, sign;
    if (CUR_CHAR != 'e' && CUR_CHAR != 'E') {
        goto error;
    }
    CONSUME_CHAR_AND_CHECK;
    if (INPUT[CUR] == '-' || INPUT[CUR] == '+') {
        sign = INPUT[CUR];
        CONSUME_CHAR_AND_CHECK;
    }
    if (!scan_digit_sequence (result)) {
        goto error;
    }
    a_result = sign + result;
    pop_recorded_ci_position ();
    return true;

error:
    restore_ci_position ();
    return false;
}

bool
Lexer::scan_floating_literal (string &a_result,
                                 string &a_exponent)
{
    CHECK_CURSOR_BOUNDS
    record_ci_position ();

    string fract, exp;

    if (scan_fractional_constant (fract)) {
        scan_exponent_part (exp);
        if (CUR_CHAR == 'f' || CUR_CHAR == 'F'
            || CUR_CHAR == 'L' || CUR_CHAR == 'l') {
            CONSUME_CHAR_AND_CHECK;
        }
    } else if (scan_digit_sequence (fract)) {
        if (!scan_exponent_part (exp)) {
            goto error;
        }
        if (CUR_CHAR == 'f' || CUR_CHAR == 'F'
            || CUR_CHAR == 'L' || CUR_CHAR == 'l') {
            CONSUME_CHAR;
        }
    } else {
        goto error;
    }

    a_result = fract;
    a_exponent = exp;
    pop_recorded_ci_position ();
    return true;

error:
    restore_ci_position ();
    return false;

}

bool
Lexer::scan_string_literal (string &a_result)
{
    CHECK_CURSOR_BOUNDS
    record_ci_position ();

    string result;
    if (CUR_CHAR == 'L') {
        CONSUME_CHAR_AND_CHECK;
    }
    if (CUR_CHAR == '"') {
        CONSUME_CHAR_AND_CHECK;
    } else {
        goto error;
    }
    if (!scan_s_char_sequence (result)) {
        goto error;
    }
    if (CUR_CHAR == '"') {
        CONSUME_CHAR;
    } else {
        goto error;
    }
    a_result = result;
    pop_recorded_ci_position ();
    return true;

error:
    restore_ci_position ();
    return false;
}

bool
Lexer::scan_boolean_literal (bool &a_result)
{
    CHECK_CURSOR_BOUNDS

    if (IN_BOUNDS (CUR+4)
        && INPUT[CUR] == 'f'
        && INPUT[CUR+1] == 'a'
        && INPUT[CUR+2] == 'l'
        && INPUT[CUR+3] == 's'
        && INPUT[CUR+4] == 'e') {
        MOVE_FORWARD (4);
        a_result = false;
        return true;
    } else if (IN_BOUNDS (CUR+3)
               && INPUT[CUR] == 't'
               && INPUT[CUR+1] == 'r'
               && INPUT[CUR+2] == 'u'
               && INPUT[CUR+3] == 'e') {
        MOVE_FORWARD (3);
        a_result = true;
        return true;
    }
    return false;
}

bool
Lexer::scan_integer_suffix (string &a_result)
{
    CHECK_CURSOR_BOUNDS
    record_ci_position ();

    string result;
    if (CUR_CHAR == 'u' or CUR_CHAR == 'U') {
        result += CUR_CHAR;
        CONSUME_CHAR_AND_CHECK;
        if (CUR_CHAR == 'l' or CUR_CHAR == 'L') {
            result += CUR_CHAR;
            CONSUME_CHAR;
        }
    } else if (CUR_CHAR == 'L' or CUR_CHAR == 'L') {
        result += CUR_CHAR;
        CONSUME_CHAR_AND_CHECK;
        if (CUR_CHAR == 'u' or CUR_CHAR == 'U') {
            result += CUR_CHAR;
            CONSUME_CHAR;
        }
    } else {
        goto error;
    }
    if (result.empty ()) {
        goto error;
    }
    a_result = result;
    pop_recorded_ci_position ();
    return true;

error:
    restore_ci_position ();
    return false;
}

bool
Lexer::scan_integer_literal (string &a_result)
{
    CHECK_CURSOR_BOUNDS

    string literal, tmp;
    if (is_nonzero_digit (CUR_CHAR)) {
        if (!scan_decimal_literal (literal)) {
            return false;
        }
        if (CUR_CHAR == 'l' or
            CUR_CHAR == 'L' or
            CUR_CHAR == 'u' or
            CUR_CHAR == 'U') {
            if (scan_integer_suffix (tmp)) {
                literal += tmp;
            }
        }
    } else if (IN_BOUNDS (CUR+1) and CUR_CHAR == '0' and
               (INPUT[CUR+1] == 'x' or INPUT[CUR+1] == 'X')) {
        if (!scan_hexadecimal_literal (literal)) {
            return false;
        }
    } else if (CUR_CHAR == '0') {
        if (!scan_octal_literal (literal)) {
            return false;
        }
    } else {
        return false;
    }
    a_result = literal;
    return true;
}

bool
Lexer::scan_simple_escape_sequence (int &a_result)
{
    CHECK_CURSOR_BOUNDS;
    record_ci_position ();

    if (CUR_CHAR != '\\')
        return false;

    CONSUME_CHAR_AND_CHECK;

    switch (CUR_CHAR) {
        case '\'':
            a_result = '\\';
            break;
        case '"' :
            a_result = '"';
            break;
        case '?':
            a_result = '?';
            break;
        case '\\':
            a_result = '\\';
            break;
        case 'a' :
            a_result = '\a';
            break;
        case 'b' :
            a_result = '\b';
            break;
        case 'f' :
            a_result = '\f';
            break;
        case 'n' :
            a_result = '\n';
            break;
        case 'r' :
            a_result = '\r';
            break;
        case 't' :
            a_result = '\t';
            break;
        case 'v':
            a_result = '\v';
            break;
        default:
            goto error;
    }
    CONSUME_CHAR;
    pop_recorded_ci_position ();
    return true;

error:
    restore_ci_position ();
    return false;
}

bool
Lexer::scan_octal_escape_sequence (int &a_result)
{
    CHECK_CURSOR_BOUNDS;
    unsigned cur=CUR;
    int result=0;

    if (!IN_BOUNDS (cur+1) || INPUT[cur] != '\\' || !is_octal_digit (INPUT[cur+1])) {
        return false;
    }
    ++cur;
    result = INPUT[CUR] - '0';
    ++cur;
    if (IN_BOUNDS (cur) && is_octal_digit (INPUT[cur])) {
        result = 8*result + (INPUT[cur] - '0');

        ++cur;
        if (IN_BOUNDS (cur) && is_octal_digit (INPUT[cur])) {
            result = 8*result + (INPUT[cur] - '0');
            ++cur;
        }
    }

    CUR = cur;
    a_result = result;
    return true;
}

bool
Lexer::scan_hexadecimal_escape_sequence (int &a_result)
{
    CHECK_CURSOR_BOUNDS;
    unsigned cur=CUR;

    if (!IN_BOUNDS (cur+1) || INPUT[cur] != '\\' ||
        !is_hexadecimal_digit (INPUT[cur+1])) {
        return false;
    }
    ++cur;
    a_result = INPUT[cur];
    ++cur;
    while (IN_BOUNDS (cur) && is_hexadecimal_digit (INPUT[cur])) {
        a_result = 16*a_result + hexadigit_to_decimal (INPUT[cur]);
        ++cur;
    }
    CUR = cur;
    return true;
}

bool
Lexer::scan_hexquad (int &a_result)
{
    CHECK_CURSOR_BOUNDS;
    unsigned cur=CUR;

    if (!IN_BOUNDS (cur+3)) {
        return false;
    }
    if (is_hexadecimal_digit (cur) &&
        is_hexadecimal_digit (cur+1) &&
        is_hexadecimal_digit (cur+2) &&
        is_hexadecimal_digit (cur+3)) {
        a_result = INPUT[cur];
        a_result = 16*a_result + hexadigit_to_decimal (INPUT[cur+1]);
        a_result = 16*a_result + hexadigit_to_decimal (INPUT[cur+2]);
        a_result = 16*a_result + hexadigit_to_decimal (INPUT[cur+3]);
        CUR = cur+4;
        return true;
    }
    return false;
}

bool
Lexer::scan_identifier (Token &a_token)
{
    CHECK_CURSOR_BOUNDS;
    string identifier;

    record_ci_position ();

    if (!is_nondigit (CUR_CHAR)) {
        goto error;
    }
    identifier += CUR_CHAR;
    CONSUME_CHAR;
    while (CURSOR_IN_BOUNDS && (is_nondigit (CUR_CHAR) || is_digit (CUR_CHAR))) {
        identifier += CUR_CHAR;
        CONSUME_CHAR;
    }
    if (identifier.empty ()) {
        goto error;
    }
    a_token.set (Token::IDENTIFIER, identifier);
    pop_recorded_ci_position ();
    return true;

error:
    restore_ci_position ();
    return false;
}

bool
Lexer::scan_keyword (Token &a_token)
{
    CHECK_CURSOR_BOUNDS
    int key_length=0;

    if (SCAN_KEYWORD (3, "and") or
        SCAN_KEYWORD (6, "and_eq") or
        SCAN_KEYWORD (3, "asm")  or
        SCAN_KEYWORD (4, "auto")  or
        SCAN_KEYWORD (6, "bitand") or
        SCAN_KEYWORD (5, "bitor") or
        SCAN_KEYWORD (4, "bool") or
        SCAN_KEYWORD (5, "break") or
        SCAN_KEYWORD (4, "case") or
        SCAN_KEYWORD (5, "catch") or
        SCAN_KEYWORD (4, "char") or
        SCAN_KEYWORD (5, "class") or
        SCAN_KEYWORD (5, "compl") or
        SCAN_KEYWORD (5, "const") or
        SCAN_KEYWORD (10, "const_cast") or
        SCAN_KEYWORD (8, "continue") or
        SCAN_KEYWORD (7, "default") or
        SCAN_KEYWORD (6, "delete") or
        SCAN_KEYWORD (2, "do") or
        SCAN_KEYWORD (6, "double") or
        SCAN_KEYWORD (12, "dynamic_cast") or
        SCAN_KEYWORD (4, "else") or
        SCAN_KEYWORD (4, "enum") or
        SCAN_KEYWORD (8, "explicit") or
        SCAN_KEYWORD (6, "export") or
        SCAN_KEYWORD (6, "extern") or
        SCAN_KEYWORD (5, "false") or
        SCAN_KEYWORD (5, "float") or
        SCAN_KEYWORD (3, "for") or
        SCAN_KEYWORD (6, "friend") or
        SCAN_KEYWORD (4, "goto") or
        SCAN_KEYWORD (2, "if") or
        SCAN_KEYWORD (6, "inline") or
        SCAN_KEYWORD (3, "int") or
        SCAN_KEYWORD (4, "long") or
        SCAN_KEYWORD (7, "mutable") or
        SCAN_KEYWORD (9, "namespace") or
        SCAN_KEYWORD (3, "new") or
        SCAN_KEYWORD (3, "not") or
        SCAN_KEYWORD (6, "not_eq") or
        SCAN_KEYWORD (8, "operator") or
        SCAN_KEYWORD (2, "or") or
        SCAN_KEYWORD (5, "or_eq") or
        SCAN_KEYWORD (7, "private") or
        SCAN_KEYWORD (9, "protected") or
        SCAN_KEYWORD (6, "public") or
        SCAN_KEYWORD (8, "register") or
        SCAN_KEYWORD (16, "reinterpret_cast") or
        SCAN_KEYWORD (6, "return") or
        SCAN_KEYWORD (5, "short") or
        SCAN_KEYWORD (6, "signed") or
        SCAN_KEYWORD (6, "sizeof") or
        SCAN_KEYWORD (6, "static") or
        SCAN_KEYWORD (11, "static_cast") or
        SCAN_KEYWORD (6, "struct") or
        SCAN_KEYWORD (6, "switch") or
        SCAN_KEYWORD (8, "template") or
        SCAN_KEYWORD (4, "this") or
        SCAN_KEYWORD (5, "throw") or
        SCAN_KEYWORD (4, "true") or
        SCAN_KEYWORD (3, "try") or
        SCAN_KEYWORD (7, "typedef") or
        SCAN_KEYWORD (6, "typeid") or
        SCAN_KEYWORD (8, "typename") or
        SCAN_KEYWORD (5, "union") or
        SCAN_KEYWORD (8, "unsigned") or
        SCAN_KEYWORD (5, "using") or
        SCAN_KEYWORD (7, "virtual") or
        SCAN_KEYWORD (4, "void") or
        SCAN_KEYWORD (8, "volatile") or
        SCAN_KEYWORD (7, "wchar_t") or
        SCAN_KEYWORD (5, "while") or
        SCAN_KEYWORD (3, "xor") or
        SCAN_KEYWORD (6, "xor_eq")) {
            if (IN_BOUNDS (CUR + key_length + 1)
                && (is_nondigit (INPUT[CUR+key_length])
                    || is_digit (INPUT[CUR+key_length]))) {
                //keyword + char following the keyword
                //could be an identifier
                return false;
            }
            string value = INPUT.substr (CUR, key_length);
            a_token.set (Token::KEYWORD, value);
            CUR += key_length;
            return true;
    }
    return false;
}

bool
Lexer::scan_literal (Token &a_token)
{
    CHECK_CURSOR_BOUNDS
    string lit1, lit2;
    bool lit3=0;
    if (scan_character_literal (lit1)) {
        a_token.set (Token::CHARACTER_LITERAL, lit1);
    } else if (scan_integer_literal (lit1)) {
        a_token.set (Token::INTEGER_LITERAL, lit1);
    } else if (scan_floating_literal (lit1, lit2)) {
        a_token.set (Token::FLOATING_LITERAL, lit1, lit2);
    } else if (scan_string_literal (lit1)) {
        a_token.set (Token::STRING_LITERAL, lit1);
    } else if (scan_boolean_literal (lit3)) {
        a_token.set (Token::BOOLEAN_LITERAL, lit3);
    } else {
        return false;
    }
    return true;
}

bool
Lexer::scan_operator (Token &a_token)
{
    CHECK_CURSOR_BOUNDS;
    record_ci_position ();

    if (next_is ("new")) {
        MOVE_FORWARD (sizeof ("new"));
        skip_blanks ();
        if (next_is ("[]")) {
            MOVE_FORWARD (sizeof ("[]"));
            a_token.set (Token::OPERATOR_NEW_VECT);
        } else {
            a_token.set (Token::OPERATOR_NEW);
        }
    } else if (next_is ("delete")) {
        MOVE_FORWARD (sizeof ("delete"));
        skip_blanks ();
        if (next_is ("[]")) {
            MOVE_FORWARD (sizeof ("[]"));
            a_token.set (Token::OPERATOR_DELETE_VECT);
        } else {
            a_token.set (Token::OPERATOR_DELETE);
        }
    } else if (CUR_CHAR == '+') {
        CONSUME_CHAR;
        if (CUR_CHAR == '=') {
            CONSUME_CHAR;
            a_token.set (Token::OPERATOR_PLUS_EQ);
        } else if (CUR_CHAR == '+') {
            CONSUME_CHAR;
            a_token.set (Token::OPERATOR_PLUS_PLUS);
        } else {
            a_token.set (Token::OPERATOR_PLUS);
        }
    } else if (CUR_CHAR == '-') {
        CONSUME_CHAR;
        if (CUR_CHAR == '=') {
            CONSUME_CHAR;
            a_token.set (Token::OPERATOR_MINUS_EQ);
        } else if (CUR_CHAR == '-') {
            CONSUME_CHAR;
            a_token.set (Token::OPERATOR_MINUS_MINUS);
        } else if (CUR_CHAR == '>') {
            CONSUME_CHAR;
            if (CUR_CHAR == '*') {
                CONSUME_CHAR;
                a_token.set (Token::OPERATOR_ARROW_STAR);
            } else {
                a_token.set (Token::OPERATOR_DEREF);
            }
        } else {
            a_token.set (Token::OPERATOR_MINUS);
        }
    } else if (CUR_CHAR == '*') {
        CONSUME_CHAR;
        if (CUR_CHAR == '=') {
            CONSUME_CHAR;
            a_token.set (Token::OPERATOR_MULT_EQ);
        } else {
            a_token.set (Token::OPERATOR_MULT);
        }
    } else if (CUR_CHAR ==  '/') {
        CONSUME_CHAR;
        if (CUR_CHAR == '=') {
            CONSUME_CHAR;
            a_token.set (Token::OPERATOR_DIV_EQ);
        } else {
            a_token.set (Token::OPERATOR_DIV);
        }
    } else if (CUR_CHAR == '%') {
        CONSUME_CHAR;
        if (CUR_CHAR == '=') {
            CONSUME_CHAR;
            a_token.set (Token::OPERATOR_MOD_EQ);
        } else {
            a_token.set (Token::OPERATOR_MOD);
        }
    } else if (CUR_CHAR == '^') {
        CONSUME_CHAR;
        if (CUR_CHAR == '=') {
            CONSUME_CHAR;
            a_token.set (Token::OPERATOR_BIT_XOR_EQ);
        } else {
            a_token.set (Token::OPERATOR_BIT_XOR);
        }
    } else if (CUR_CHAR == '&') {
        CONSUME_CHAR;
        if (CUR_CHAR == '=') {
            CONSUME_CHAR;
            a_token.set (Token::OPERATOR_BIT_AND_EQ);
        } else if (CUR_CHAR == '&') {
            CONSUME_CHAR;
            a_token.set (Token::OPERATOR_AND);
        } else {
            a_token.set (Token::OPERATOR_BIT_AND);
        }
    } else if (CUR_CHAR == '|') {
        CONSUME_CHAR;
        if (CUR_CHAR == '=') {
            CONSUME_CHAR;
            a_token.set (Token::OPERATOR_BIT_OR_EQ);
        } else if (CUR_CHAR == '|') {
            CONSUME_CHAR;
            a_token.set (Token::OPERATOR_OR);
        } else {
            a_token.set (Token::OPERATOR_BIT_OR);
        }
    } else if (CUR_CHAR == '~') {
        CONSUME_CHAR;
        a_token.set (Token::OPERATOR_BIT_COMPLEMENT);
    } else if (CUR_CHAR == '!') {
        CONSUME_CHAR;
        if (CUR_CHAR == '=') {
            CONSUME_CHAR;
            a_token.set (Token::OPERATOR_NOT_EQUAL);
        } else {
            a_token.set (Token::OPERATOR_NOT);
        }
    } else if (CUR_CHAR == '=') {
        CONSUME_CHAR;
        if (CUR_CHAR == '=') {
            CONSUME_CHAR;
            a_token.set (Token::OPERATOR_EQUALS);
        } else {
            a_token.set (Token::OPERATOR_ASSIGN);
        }
    } else if (CUR_CHAR == '<') {
        CONSUME_CHAR;
        if (CUR_CHAR == '=') {
            CONSUME_CHAR;
            a_token.set (Token::OPERATOR_LT_EQ);
        } else if (CUR_CHAR == '<') {
            CONSUME_CHAR;
            if (CUR_CHAR == '=') {
                CONSUME_CHAR;
                a_token.set (Token::OPERATOR_BIT_LEFT_SHIFT_EQ);
            } else {
                a_token.set (Token::OPERATOR_BIT_LEFT_SHIFT);
            }
        } else {
            a_token.set (Token::OPERATOR_LT);
        }
    } else if (CUR_CHAR == '>') {
        CONSUME_CHAR;
        if (CUR_CHAR == '=') {
            CONSUME_CHAR;
            a_token.set (Token::OPERATOR_GT_EQ);
        } else if (CUR_CHAR == '>') {
            CONSUME_CHAR;
            if (CUR_CHAR == '=') {
                CONSUME_CHAR;
                a_token.set (Token::OPERATOR_BIT_RIGHT_SHIFT_EQ);
            } else {
                a_token.set (Token::OPERATOR_BIT_RIGHT_SHIFT);
            }
        } else {
            a_token.set (Token::OPERATOR_GT);
        }
    } else if (CUR_CHAR == ',') {
        CONSUME_CHAR;
        a_token.set (Token::OPERATOR_SEQ_EVAL);
    } else if (CUR_CHAR == '(') {
        CONSUME_CHAR;
        if (CUR_CHAR == ')') {
            CONSUME_CHAR;
            a_token.set (Token::OPERATOR_GROUP);
        } else {
            goto error;
        }
    } else if (CUR_CHAR == '[') {
        CONSUME_CHAR;
        if (CUR_CHAR == ']') {
            CONSUME_CHAR;
            a_token.set (Token::OPERATOR_ARRAY_ACCESS);
        } else {
            goto error;
        }
    } else if (CUR_CHAR == '.') {
        CONSUME_CHAR;
        if (CUR_CHAR == '*') {
            CONSUME_CHAR;
            a_token.set (Token::OPERATOR_DOT_STAR);
        } else {
            a_token.set (Token::OPERATOR_DOT);
        }
    } else if (CUR_CHAR == ':') {
        CONSUME_CHAR;
        if (CUR_CHAR == ':') {
            CONSUME_CHAR;
            a_token.set (Token::OPERATOR_SCOPE_RESOL);
        } else {
            goto error;
        }
    } else {
        goto error;
    }
    pop_recorded_ci_position ();
    return true;

error:
    restore_ci_position ();
    return false;
}

bool
Lexer::scan_punctuator (Token &a_token)
{
    CHECK_CURSOR_BOUNDS;
    record_ci_position ();

    switch (CUR_CHAR) {
        case ':':
            CONSUME_CHAR;
            if (CUR_CHAR == ':') {
                goto error;
            } else {
                a_token.set (Token::PUNCTUATOR_COLON);
                goto okay;
            }
            break;
        case ';':
            a_token.set (Token::PUNCTUATOR_SEMI_COLON);
            break;
        case '{':
            a_token.set (Token::PUNCTUATOR_CURLY_BRACKET_OPEN);
            break;
        case '}':
            a_token.set (Token::PUNCTUATOR_CURLY_BRACKET_CLOSE);
            break;
        case '[':
            a_token.set (Token::PUNCTUATOR_BRACKET_OPEN);
            break;
        case ']':
            a_token.set (Token::PUNCTUATOR_BRACKET_CLOSE);
            break;
        case '(':
            a_token.set (Token::PUNCTUATOR_PARENTHESIS_OPEN);
            break;
        case ')':
            a_token.set (Token::PUNCTUATOR_PARENTHESIS_CLOSE);
            break;
        case '?':
            a_token.set (Token::PUNCTUATOR_QUESTION_MARK);
            break;
        default:
            goto error;
    }
    CONSUME_CHAR;
okay:
    pop_recorded_ci_position ();
    return true;
error:
    restore_ci_position ();
    return false;
}

/// scan the character input
/// \param a_token the retrieved token
/// \return true upon successful completion, false otherwise.
bool
Lexer::scan_next_token (Token &a_token)
{
    CHECK_CURSOR_BOUNDS;
    bool is_ok=false;

    record_ci_position ();

    skip_blanks ();

    //try operators
    switch (CUR_CHAR) {
        case 'n':
        case 'd':
        case '+':
        case '-':
        case '*':
        case '/':
        case '%':
        case '^':
        case '&':
        case '|':
        case '~':
        case '=':
        case '<':
        case '>':
        case ',':
        case '(':
        case '[':
        case '.':
        case ':':
            is_ok = scan_operator (a_token);
            if (is_ok) {
                goto okay;
            }
            break;
        default:
            break;
    }
    //then try punctuators
    switch (CUR_CHAR) {
        case ':':
        case ';':
        case '{':
        case '}':
        case '[':
        case ']':
        case '(':
        case ')':
        case '?':
            is_ok = scan_punctuator (a_token);
            if (is_ok) {
                goto okay;
            }
            break;
        default:
            break;
    }
    //then try literals
    switch (CUR_CHAR) {
        case '\\':
        case '\'':
        case '"':
        case 'L':
        case 'l':
        case 'u':
        case 'U':
        case '.':
        case 't':
        case 'f':
            is_ok = scan_literal (a_token);
            break;
        default :
            if (is_digit (CUR_CHAR)) {
                is_ok = scan_literal (a_token);
            }
            break;
    }
    if (is_ok) {
        goto okay;
    }
    //then try keywords
    if (is_nondigit (CUR_CHAR)) {
        is_ok = scan_keyword (a_token);
        if (is_ok) {
            goto okay;
        }
    }
    //then try identifiers
    if (is_nondigit (CUR_CHAR)) {
        is_ok = scan_identifier (a_token);
        if (is_ok) {
            goto okay;
        }
    }

    restore_ci_position ();
    return false;

okay:
    pop_recorded_ci_position ();
    return true;
}

/// peek a token from the token queue
/// \return true upon successful completion, false otherwise.
bool
Lexer::peek_next_token (Token &a_token)
{
    if (m_priv->token_cursor >= m_priv->tokens_queue.size ()) {
        //token queue empty. Go scan the character input
        //to get one token and push it onto the token queue
        Token token;
        if (scan_next_token (token)) {
            m_priv->tokens_queue.push_back (token);
        }
    }
    if (m_priv->token_cursor >= m_priv->tokens_queue.size ()) {
        return false;
    }
    a_token = m_priv->tokens_queue[m_priv->token_cursor];
    return true;
}

/// peek the nth token coming next, from the token queue.
////
/// \param a_nth the nth coming token. The first token to come has index 0.
///  so Lexer::peek_nth_token (0, token) is equivalent to
///  Lexer::peek_next_token ()
/// \param a_token the resulting token.
/// \return true upon successful completion, false otherwise.
bool
Lexer::peek_nth_token (unsigned a_nth, Token &a_token)
{
    if (a_nth + m_priv->token_cursor >= m_priv->tokens_queue.size ()) {
        //we don't have enough tokens in the queue.
        //so go scan the input for tokens and push them
        //onto the queue
        Token token;
        unsigned nb_tokens_2_scan =
            a_nth + m_priv->token_cursor - m_priv->tokens_queue.size ();
        while (nb_tokens_2_scan--) {
            if (!scan_next_token (token)) {
                return false;
            }
            m_priv->tokens_queue.push_back (token);
        }
    }
    if (a_nth + m_priv->token_cursor >= m_priv->tokens_queue.size ()) {
        //we should never reach this condition.
        return false;
    }
    a_token = m_priv->tokens_queue[a_nth];
    return true;
}

/// Consume the first token present in the token queue.
/// The token to be consumed can be retrieved using Lexer::peek_next_token().
/// \return true upon successful completion, false otherwise.
bool
Lexer::consume_next_token ()
{
    Token token;
    return consume_next_token (token);
}

/// Consume the first token present in the token queue.
///
/// \param a_token the consumed token.
/// \return true upon successful completion, false otherwise.
bool
Lexer::consume_next_token (Token &a_token)
{
    if (!peek_next_token (a_token)) {return false;}
    ++m_priv->token_cursor;
    return true;
}

/// tests if the lexer has reached the end of character input stream
bool
Lexer::reached_eof () const
{
    return (CUR >= INPUT.size ());
}

void
Lexer::record_ci_position ()
{
    m_priv->recorded_positions.push_front (CUR);
}

void
Lexer::restore_ci_position ()
{
    if (m_priv->recorded_positions.empty ())
        return;
    CUR = m_priv->recorded_positions.front ();
    m_priv->recorded_positions.pop_front ();
    //m_priv->tokens_queue.clear ();
}

void
Lexer::pop_recorded_ci_position ()
{
    if (m_priv->recorded_positions.empty ())
        return;
    m_priv->recorded_positions.pop_front ();
}

unsigned
Lexer::get_token_stream_mark () const
{
    return m_priv->token_cursor;
}

void
Lexer::rewind_to_mark (unsigned a_mark)
{
    m_priv->token_cursor = a_mark;
}

//********************
//</class Lexer implem>
//********************
NEMIVER_END_NAMESPACE (cpp)
NEMIVER_END_NAMESPACE (nemiver)

