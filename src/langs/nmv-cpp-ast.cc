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
#include "nmv-cpp-ast.h"

NEMIVER_BEGIN_NAMESPACE (nemiver)
NEMIVER_BEGIN_NAMESPACE (cpp)

//**************
// class <Token>
//**************

Token::Token () :
    m_kind (UNDEFINED),
    m_int_value (-1)
{
}

Token::Token (Kind a_kind, const string& a_value) :
    m_kind (a_kind), m_str_value (a_value), m_int_value (-1)
{
}

Token::Token (Kind a_kind,
              const string& a_value,
              const string& a_value2) :
    m_kind (a_kind),
    m_str_value (a_value),
    m_str_value2 (a_value2),
    m_int_value (-1)
{
}

Token::Token (Kind a_kind, int a_value) :
    m_kind (a_kind), m_int_value (a_value)
{
}

Token::Token (const Token &a_t)
{
    m_kind = a_t.get_kind ();
    m_str_value = a_t.get_str_value ();
    m_int_value = a_t.get_int_value ();
}

Token&
Token::operator= (const Token &a_t)
{
    m_kind = a_t.get_kind ();
    m_str_value = a_t.get_str_value ();
    m_str_value2 = a_t.get_str_value2 ();
    m_int_value = a_t.get_int_value ();
    return *this;
}

Token::~Token ()
{
}

const string&
Token::get_str_value () const
{
    return m_str_value;
}

const string&
Token::get_str_value2 () const
{
    return m_str_value2;
}

int
Token::get_int_value () const
{
    return m_int_value;
}

Token::Kind
Token::get_kind () const
{
    return m_kind;
}

void
Token::set (Kind a_kind)
{
    m_kind = a_kind;
}

void
Token::set (Kind a_kind, const string &a_val)
{
    m_kind = a_kind;
    m_str_value = a_val;
}

void
Token::set (Kind a_kind, const string &a_val, const string &a_val2)
{
    m_kind = a_kind;
    m_str_value = a_val;
    m_str_value2 = a_val2;
}

void
Token::set (Kind a_kind, int a_val)
{
    m_kind = a_kind;
    m_int_value = a_val;
}

void
Token::clear ()
{
    m_kind = Token::UNDEFINED;
    m_str_value.clear ();
    m_str_value2.clear ();
    m_int_value=0;
}

bool
Token::is_literal () const
{
    switch (get_kind ()) {
        case INTEGER_LITERAL:
        case CHARACTER_LITERAL:
        case FLOATING_LITERAL:
        case STRING_LITERAL:
        case BOOLEAN_LITERAL:
            return true;
        default:
            break;
    }
    return false;
}

bool
Token::is_operator () const
{
    switch (get_kind ()) {
        case OPERATOR_NEW:
        case OPERATOR_DELETE:
        case OPERATOR_NEW_VECT:
        case OPERATOR_DELETE_VECT:
        case OPERATOR_PLUS:
        case OPERATOR_MINUS:
        case OPERATOR_MULT:
        case OPERATOR_DIV:
        case OPERATOR_MOD:
        case OPERATOR_BIT_XOR:
        case OPERATOR_BIT_AND:
        case OPERATOR_BIT_OR:
        case OPERATOR_BIT_COMPLEMENT:
        case OPERATOR_NOT:
        case OPERATOR_ASSIGN:
        case OPERATOR_LT:
        case OPERATOR_GT:
        case OPERATOR_PLUS_EQ:
        case OPERATOR_MINUS_EQ:
        case OPERATOR_MULT_EQ:
        case OPERATOR_DIV_EQ:
        case OPERATOR_MOD_EQ:
        case OPERATOR_BIT_XOR_EQ:
        case OPERATOR_BIT_AND_EQ:
        case OPERATOR_BIT_OR_EQ:
        case OPERATOR_BIT_LEFT_SHIFT:
        case OPERATOR_BIT_RIGHT_SHIFT:
        case OPERATOR_BIT_LEFT_SHIFT_EQ:
        case OPERATOR_BIT_RIGHT_SHIFT_EQ:
        case OPERATOR_EQUALS:
        case OPERATOR_NOT_EQUAL:
        case OPERATOR_LT_EQ:
        case OPERATOR_GT_EQ:
        case OPERATOR_AND:
        case OPERATOR_OR:
        case OPERATOR_PLUS_PLUS:
        case OPERATOR_MINUS_MINUS:
        case OPERATOR_SEQ_EVAL:
        case OPERATOR_ARROW_STAR:
        case OPERATOR_DEREF:
        case OPERATOR_GROUP:
        case OPERATOR_ARRAY_ACCESS:
        case OPERATOR_SCOPE_RESOL:
        case OPERATOR_DOT:
        case OPERATOR_DOT_STAR:
            return true;
        default:
            break;
    }
    return false;
}

//**************
// class </Token>
//**************


const string&
ExprBase::operator_to_string (Operator a_op)
{
    static const string OP_UNDEFINED ("<undefined>");
    static const string OP_MULT ("*");
    static const string OP_DIV ("/");
    static const string OP_MOD ("%");
    static const string OP_PLUS ("+");
    static const string OP_MINUS ("-");
    static const string OP_LT ("<");
    static const string OP_GT (">");
    static const string OP_LT_OR_EQ ("<=");
    static const string OP_GT_OR_EQ (">=");
    static const string OP_LEFT_SHIFT ("<<");
    static const string OP_RIGHT_SHIFT (">>");
    static const string OP_ASSIGN ("=");
    static const string OP_EQUALS ("==");
    static const string OP_NOT_EQUALS ("!=");
    static const string OP_BIT_AND ("&");
    static const string OP_LOG_AND ("&&");
    static const string OP_LOG_OR ("||");
    static const string OP_MULT_EQ ("*=");
    static const string OP_DIV_EQ ("/=");
    static const string OP_MOD_EQ ("%=");
    static const string OP_PLUS_EQ ("+=");
    static const string OP_MINUS_EQ ("-=");
    static const string OP_RIGHT_SHIFT_EQ (">>=");
    static const string OP_LEFT_SHIFT_EQ ("<<=");
    static const string OP_AND_EQ ("&=");
    static const string OP_XOR_EQ ("^=");
    static const string OP_OR_EQ ("|=");
    switch (a_op) {
        case ExprBase::OP_UNDEFINED:
            return OP_UNDEFINED;
        case MULT:
            return OP_MULT;
        case DIV:
            return OP_DIV;
        case MOD:
            return OP_MOD;
        case PLUS:
            return OP_PLUS;
        case MINUS:
            return OP_MINUS;
        case LT:
            return OP_LT;
        case GT:
            return OP_GT;
        case LT_OR_EQ:
            return OP_LT_OR_EQ;
        case GT_OR_EQ:
            return OP_GT_OR_EQ;
        case LEFT_SHIFT:
            return OP_LEFT_SHIFT;
        case RIGHT_SHIFT:
            return OP_RIGHT_SHIFT;
        case EQUALS:
            return OP_EQUALS;
        case NOT_EQUALS:
            return OP_NOT_EQUALS;
        case ASSIGN:
            return OP_ASSIGN;
        case BIT_AND:
            return OP_BIT_AND;
        case LOG_AND:
            return OP_LOG_AND;
        case LOG_OR:
            return OP_LOG_OR;
        case MULT_EQ:
            return OP_MULT_EQ;
        case DIV_EQ:
            return OP_DIV_EQ;
        case MOD_EQ:
            return OP_MOD_EQ;
        case PLUS_EQ:
            return OP_PLUS_EQ;
        case MINUS_EQ:
            return OP_MINUS_EQ;
        case RIGHT_SHIFT_EQ:
            return OP_RIGHT_SHIFT_EQ;
        case LEFT_SHIFT_EQ:
            return OP_LEFT_SHIFT_EQ;
        case AND_EQ:
            return OP_AND_EQ;
        case XOR_EQ:
            return OP_XOR_EQ;
        case OR_EQ:
            return OP_OR_EQ;
    }
    return OP_UNDEFINED;
}

Declaration::Declaration (Declaration::Kind a_kind) :
    m_kind (a_kind)
{
}

Declaration::Kind
Declaration::get_kind () const
{
    return m_kind;
}

bool
QName::to_string (string &a_result) const
{
    list<ClassOrNSName>::const_iterator it=get_names ().begin ();
    if (!it->get_name ())
        return false;
    string result;
    for (it=get_names ().begin (); it != get_names ().end (); ++it) {
        if (it == get_names ().begin ()) {
            result = it->get_name_as_string ();
        } else {
            result += "::";
            if (it->is_prefixed_with_template ()) {
                a_result += "template ";
            }
            result += it->get_name_as_string ();
        }
    }
    a_result = result;
    return true;
}

bool
PtrOperator::to_string (string &a_result) const
{
    if (m_elems.empty ()) {return false;}
    string result, str;
    list<PtrOperator::ElemPtr>::const_iterator prev_it;
    list<PtrOperator::ElemPtr>::const_iterator it=m_elems.begin ();
    if (!*it) {return false;}
    (*it)->to_string (result);
    prev_it = it;
    for (++it; it != m_elems.end (); ++it) {
        if (!*it) {continue;}
        (*it)->to_string (str);
        if ((*prev_it)->get_kind () != Elem::STAR) {
            result += ' ';
        }
        result += str;
        prev_it = it;
    }
    a_result = result;
    return true;
}

void
Declaration::set_kind (Declaration::Kind a_kind)
{
    m_kind = a_kind;
}

bool
SimpleDeclaration::to_string (string &a_result) const
{
    string str, result;

    DeclSpecifier::list_to_string (get_decl_specifiers (), result);
    InitDeclarator::list_to_string (get_init_declarators (), str);

    a_result = result + ' ' + str;
    return true;
}

bool
TypeSpecifier::list_to_string (const list<TypeSpecifierPtr> &a_type_specs,
                               string &a_str)
{
    string str;
    list<TypeSpecifierPtr>::const_iterator it;
    for (it = a_type_specs.begin (); it != a_type_specs.end (); ++it) {
        if (it == a_type_specs.begin ()) {
            if (*it) {
                (*it)->to_string (a_str);
            } else {
                continue;
            }
        } else {
            (*it)->to_string (str);
            a_str += " " + str;
        }
    }
    return true;
}

bool
DeclSpecifier::list_to_string (const list<DeclSpecifierPtr> &a_decls,
                               string &a_str)
{
    list<shared_ptr<DeclSpecifier> >::const_iterator it;
    string str;

    for (it = a_decls.begin (); it != a_decls.end (); ++it) {
        (*it)->to_string (str);
        if (it == a_decls.begin ()) {
            a_str = str;
        } else {
            a_str += " " + str;
        }
    }
    return true;
}

IDExpr::~IDExpr ()
{
}


bool
UnqualifiedID::to_string (string &a_result) const
{
    a_result = m_name;
    return true;
}

bool
UnqualifiedOpFuncID::to_string (string &a_result) const
{
    switch (get_kind ()) {
        case Token::OPERATOR_NEW:
            a_result = "operator new";
            break;
        case Token::OPERATOR_DELETE:
            a_result = "operator delete";
            break;
        case Token::OPERATOR_NEW_VECT:
            a_result = "operator new []";
            break;
        case Token::OPERATOR_DELETE_VECT:
            a_result = "operator delete";
            break;
        case Token::OPERATOR_PLUS:
            a_result = "operator +";
            break;
        case Token::OPERATOR_MINUS:
            a_result = "operator -";
            break;
        case Token::OPERATOR_MULT:
            a_result = "operator *";
            break;
        case Token::OPERATOR_DIV:
            a_result = "operator /";
            break;
        case Token::OPERATOR_MOD:
            a_result = "operator %";
            break;
        case Token::OPERATOR_BIT_XOR:
            a_result = "operator ^";
            break;
        case Token::OPERATOR_BIT_AND:
            a_result = "operator &";
            break;
        case Token::OPERATOR_BIT_OR:
            a_result = "operator |";
            break;
        case Token::OPERATOR_BIT_COMPLEMENT:
            a_result = "operator ~";
            break;
        case Token::OPERATOR_NOT:
            a_result = "operator !";
            break;
        case Token::OPERATOR_ASSIGN:
            a_result = "operator =";
            break;
        case Token::OPERATOR_LT:
            a_result = "operator <";
            break;
        case Token::OPERATOR_GT:
            a_result = "operator >";
            break;
        case Token::OPERATOR_PLUS_EQ:
            a_result = "operator +=";
            break;
        case Token::OPERATOR_MINUS_EQ:
            a_result = "operator -=";
            break;
        case Token::OPERATOR_MULT_EQ:
            a_result = "operator *=";
            break;
        case Token::OPERATOR_DIV_EQ:
            a_result = "operator /+";
            break;
        case Token::OPERATOR_MOD_EQ:
            a_result = "operator %=";
            break;
        case Token::OPERATOR_BIT_XOR_EQ:
            a_result = "operator ^=";
            break;
        case Token::OPERATOR_BIT_AND_EQ:
            a_result = "operator &=";
            break;
        case Token::OPERATOR_BIT_OR_EQ:
            a_result = "operator |=";
            break;
        case Token::OPERATOR_BIT_LEFT_SHIFT:
            a_result = "operator <<";
            break;
        case Token::OPERATOR_BIT_RIGHT_SHIFT:
            a_result = "operator >>";
            break;
        case Token::OPERATOR_BIT_LEFT_SHIFT_EQ:
            a_result = "operator <<=";
            break;
        case Token::OPERATOR_BIT_RIGHT_SHIFT_EQ:
            a_result = "operator >>=";
            break;
        case Token::OPERATOR_EQUALS:
            a_result = "operator ==";
            break;
        case Token::OPERATOR_NOT_EQUAL:
            a_result = "operator !=";
            break;
        case Token::OPERATOR_LT_EQ:
            a_result = "operator <=";
            break;
        case Token::OPERATOR_GT_EQ:
            a_result = "operator >=";
            break;
        case Token::OPERATOR_AND:
            a_result = "operator &&";
            break;
        case Token::OPERATOR_OR:
            a_result = "operator ||";
            break;
        case Token::OPERATOR_PLUS_PLUS:
            a_result = "operator ++";
            break;
        case Token::OPERATOR_MINUS_MINUS:
            a_result = "operator --";
            break;
        case Token::OPERATOR_SEQ_EVAL:
            a_result = "operator ,";
            break;
        case Token::OPERATOR_ARROW_STAR:
            a_result = "operator ->*";
            break;
        case Token::OPERATOR_DEREF:
            a_result = "operator ->";
            break;
        case Token::OPERATOR_GROUP:
            a_result = "operator ()";
            break;
        case Token::OPERATOR_ARRAY_ACCESS:
            a_result = "operator []";
            break;
        case Token::OPERATOR_SCOPE_RESOL:
            a_result = "operator ::";
            break;
        case Token::OPERATOR_DOT:
            a_result = "operator .";
            break;
        case Token::OPERATOR_DOT_STAR:
            a_result = "operator .*";
            break;
        default:
            return false;
            break;
    }
    return true;
}

bool
DestructorID::to_string (string &a_result) const
{
    if (!get_name ())
        return false;
    string str;
    get_name ()->to_string (str);
    a_result = "~" + str;
    return true;
}

bool
QualifiedIDExpr::to_string (string &a_result) const
{
    string result;
    if (get_scope ()) {
        get_scope ()->to_string (result);
    }
    if (get_unqualified_id ()) {
        string tmp;
        get_unqualified_id ()->to_string (tmp);
        result += "::" + tmp;
    }
    a_result = result;
    return true;
}

bool
InitDeclarator::list_to_string (const list<InitDeclaratorPtr> &a_decls,
                                string &a_str)
{
    string str, str2;
    list<InitDeclaratorPtr>::const_iterator it=a_decls.begin ();
    if (it == a_decls.end () || !(*it)) {
        return false;
    }
    (*it)->to_string (str2);
    for (++it; it != a_decls.end (); ++it) {
        if (!*it) {continue;}
        (*it)->to_string (str);
        str2 += " " + str;
    }
    a_str = str2;
    return true;
}

bool
to_string (const TypeIDPtr a_id, string &a_str)
{
    if (!a_id)
        return false;
    a_id->to_string (a_str);
    return true;
}

UnqualifiedIDPtr
create_unqualified_id (const string &a_str)
{
    UnqualifiedIDPtr result (new UnqualifiedID (a_str));
    return result;
}

bool
to_string (const UnqualifiedIDExprPtr a_expr, string &a_str)
{
    if (!a_expr)
        return false;
    a_expr->to_string (a_str);
    return true;
}

NEMIVER_END_NAMESPACE (cpp)
NEMIVER_END_NAMESPACE (nemiver)
