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
#ifndef __NMV_CPP_AST_H__
#define __NMV_CPP_AST_H__

#include <string>
#include <list>
#if __GNUC__ >= 4
#include <tr1/memory>
#else
#include <boost/tr1/memory.hpp>
#endif
#include "common/nmv-namespace.h"
#include "common/nmv-api-macros.h"

NEMIVER_BEGIN_NAMESPACE (nemiver)
NEMIVER_BEGIN_NAMESPACE (cpp)

using std::tr1::shared_ptr;
using std::list;
using std::string;

class NEMIVER_API Token {
public:
    enum Kind {
        UNDEFINED=0,
        IDENTIFIER,
        KEYWORD,
        //LITERAL
        INTEGER_LITERAL,
        CHARACTER_LITERAL,
        FLOATING_LITERAL,
        STRING_LITERAL,
        BOOLEAN_LITERAL,
        //OPERATOR
        OPERATOR_NEW /*new*/,
        OPERATOR_DELETE /*delete*/,
        OPERATOR_NEW_VECT /*new[]*/,
        OPERATOR_DELETE_VECT /*delete[]*/,
        OPERATOR_PLUS /*+*/,
        OPERATOR_MINUS /*-*/,
        OPERATOR_MULT /* * */,
        OPERATOR_DIV /* / */,
        OPERATOR_MOD /*%*/,
        OPERATOR_BIT_XOR /*^*/,
        OPERATOR_BIT_AND /*&*/,
        OPERATOR_BIT_OR /*|*/,
        OPERATOR_BIT_COMPLEMENT /*~*/,
        OPERATOR_NOT /*!*/,
        OPERATOR_ASSIGN /*=*/,
        OPERATOR_LT /*<*/,
        OPERATOR_GT /*<*/,
        OPERATOR_PLUS_EQ /*+=*/,
        OPERATOR_MINUS_EQ /*-=*/,
        OPERATOR_MULT_EQ /* *= */,
        OPERATOR_DIV_EQ /* /= */,
        OPERATOR_MOD_EQ /* %= */,
        OPERATOR_BIT_XOR_EQ /* ^= */,
        OPERATOR_BIT_AND_EQ /* &= */,
        OPERATOR_BIT_OR_EQ /* |= */,
        OPERATOR_BIT_LEFT_SHIFT /*<<*/,
        OPERATOR_BIT_RIGHT_SHIFT /*<<*/,
        OPERATOR_BIT_LEFT_SHIFT_EQ /*<<=*/,
        OPERATOR_BIT_RIGHT_SHIFT_EQ /*>>=*/,
        OPERATOR_EQUALS /*==*/,
        OPERATOR_NOT_EQUAL /*!=*/,
        OPERATOR_LT_EQ /*=*/,
        OPERATOR_GT_EQ /*=*/,
        OPERATOR_AND /*&&*/,
        OPERATOR_OR /*||*/,
        OPERATOR_PLUS_PLUS /*++*/,
        OPERATOR_MINUS_MINUS /*--*/,
        OPERATOR_SEQ_EVAL /*,*/,
        OPERATOR_ARROW_STAR /*->**/,
        OPERATOR_DEREF /*->*/,
        OPERATOR_GROUP /*()*/,
        OPERATOR_ARRAY_ACCESS /*[]*/,
        OPERATOR_SCOPE_RESOL /*::*/,
        OPERATOR_DOT /*.*/,
        OPERATOR_DOT_STAR /* .* */,
        //punctuators
        PUNCTUATOR_COLON /*:*/,
        PUNCTUATOR_SEMI_COLON /*;*/,
        PUNCTUATOR_CURLY_BRACKET_OPEN /*{*/,
        PUNCTUATOR_CURLY_BRACKET_CLOSE /*}*/,
        PUNCTUATOR_BRACKET_OPEN /*[*/,
        PUNCTUATOR_BRACKET_CLOSE /*]*/,
        PUNCTUATOR_PARENTHESIS_OPEN /*(*/,
        PUNCTUATOR_PARENTHESIS_CLOSE /*(*/,
        PUNCTUATOR_QUESTION_MARK /*?*/
    };

private:
    Kind m_kind;
    string m_str_value;
    string m_str_value2;
    int m_int_value;

public:
    Token ();
    Token (Kind a_kind, const string& a_value);
    Token (Kind a_kind,
           const string& a_value,
           const string& a_value2);
    Token (Kind a_kind, int a_value);
    Token (const Token &);
    Token& operator= (const Token&);
    ~Token ();
    const string& get_str_value () const;
    const string& get_str_value2 () const;
    int get_int_value () const;
    Kind get_kind () const;
    void set (Kind a_kind);
    void set (Kind a_kind, const string &a_val);
    void set (Kind a_kind,
              const string &a_val,
              const string &a_val2);
    void set (Kind a_kind, int a_val);
    void clear ();
    bool is_literal () const;
    bool is_operator () const;
};//end class Token

//***********************
//forward declarations
//***********************
class QName;
typedef shared_ptr<QName> QNamePtr;

class IDExpr;
typedef shared_ptr<IDExpr> IDExprPtr;

class UnqualifiedIDExpr;
typedef shared_ptr<UnqualifiedIDExpr> UnqualifiedIDExprPtr;

class UnqualifiedID;
typedef shared_ptr<UnqualifiedID> UnqualifiedIDPtr;

class TypeSpecifier;
typedef shared_ptr<TypeSpecifier> TypeSpecifierPtr;

class TemplateID;
typedef shared_ptr<TemplateID> TemplateIDPtr;

class UnqualifiedTemplateID;
typedef shared_ptr<UnqualifiedTemplateID> UnqualifiedTemplateIDPtr;

class TypeID;
typedef shared_ptr<TypeID> TypeIDPtr;

bool to_string (const TypeIDPtr, string &);
bool to_string (const UnqualifiedIDExprPtr, string &);
UnqualifiedIDPtr create_unqualified_id (const string &);

/// \brief Qualified Name.
///
/// can contain the result of the parsing of
/// a nested name specifier.
class NEMIVER_API QName {

public:
    class ClassOrNSName {
        UnqualifiedIDExprPtr m_name;
        bool m_prefixed_with_template;//true if is prefixed by the 'template' keyword.

    public:
        ClassOrNSName (const string &n, bool p=false) :
            m_name (create_unqualified_id (n)),
            m_prefixed_with_template (p)
        {}
        ClassOrNSName () :
            m_prefixed_with_template (false)
        {}
        ClassOrNSName (const UnqualifiedIDExprPtr n, bool p=false) :
            m_name (n),
            m_prefixed_with_template (p)
        {}
        const UnqualifiedIDExprPtr get_name () const {return m_name;}
        const string get_name_as_string () const
        {
            string str;
            if (m_name) {
                nemiver::cpp::to_string (m_name, str);
            }
            return str;
        }
        void set_name (const UnqualifiedIDExprPtr n) {m_name = n;}
        void set_name (const string &a_n)
        {
            UnqualifiedIDExprPtr n (create_unqualified_id (a_n));
            m_name = n;
        }
        bool is_prefixed_with_template () const {return m_prefixed_with_template;}
        void set_is_prefixed_with_template (bool a) {m_prefixed_with_template=a;}
    };//end ClassOrNSName

private:
    list<ClassOrNSName>m_names;

public:
    QName () {}
    QName (const QName &a_other):
        m_names (a_other.m_names)
    {}

    /// an empty name means the QName is: '::'
    const list<ClassOrNSName>& get_names () const {return m_names;}
    void append (const string &a_name, bool a_prefixed_with_template=false)
    {
        ClassOrNSName c (a_name, a_prefixed_with_template);
        m_names.push_back (c);
    }
    void append (const UnqualifiedIDExprPtr a_name, bool a_prefixed_with_template=false)
    {
        ClassOrNSName c (a_name, a_prefixed_with_template);
        m_names.push_back (c);
    }
    void append (const QNamePtr &a_q, bool a_prefixed_with_template=false)
    {
        if (!a_q) {return;}
        list<ClassOrNSName>::const_iterator it=a_q->get_names ().begin ();
        for (it=a_q->get_names ().begin (); it != a_q->get_names ().end (); ++it) {
            if (it == a_q->get_names ().begin ()) {
                ClassOrNSName n (it->get_name (), a_prefixed_with_template);
                m_names.push_back (n);
            } else {
                m_names.push_back (*it);
            }
        }
    }
    bool to_string (string &) const;
};//end class QName

class NEMIVER_API CVQualifier {
    CVQualifier ();
    CVQualifier (const CVQualifier&);
    CVQualifier& operator= (const CVQualifier&);

public:
    enum Kind {
        UNDEFINED,
        CONST,
        VOLATILE
    };
private:
    Kind m_kind;

public:
    CVQualifier (Kind k) :
        m_kind (k)
    {}
    virtual ~CVQualifier () {}
    Kind get_kind () const {return m_kind;}
    void set_kind (Kind k) {m_kind=k;}
    virtual bool to_string (string &) const=0;
};
typedef shared_ptr<CVQualifier> CVQualifierPtr;

class NEMIVER_API ConstQualifier : public CVQualifier {
    ConstQualifier (const ConstQualifier&);
    ConstQualifier& operator= (const ConstQualifier&);
public:
    ConstQualifier () :
        CVQualifier (CONST)
    {}
    ~ConstQualifier () {}
    bool to_string (string &a_str) const {a_str="const";return true;}
};
typedef shared_ptr<ConstQualifier> ConstQualifierPtr;

class NEMIVER_API VolatileQualifier : public CVQualifier {
    VolatileQualifier (const VolatileQualifier &);
    VolatileQualifier& operator= (const VolatileQualifier &);
public:
    VolatileQualifier ():
        CVQualifier (VOLATILE)
    {}
    ~VolatileQualifier () {}
    bool to_string (string &a_str) const {a_str="volatile";return true;}
};
typedef shared_ptr<VolatileQualifier> VolatileQualifierPtr;


class NEMIVER_API PtrOperator {
    PtrOperator (const PtrOperator&);
    PtrOperator& operator= (const PtrOperator&);

public:
    class Elem {
        Elem ();
    public:
        enum Kind {
            UNDEFINED,
            STAR,
            AND,
            CONST,
            VOLATILE,
        };
    private:
        Kind m_kind;

    public:
        Elem (Kind a_k) : m_kind (a_k)
        {}
        Kind get_kind () const {return m_kind;}
        void set_kind (Kind a_kind) {m_kind=a_kind;}
        virtual ~Elem () {}
        virtual bool to_string (string &) const=0;
    };
    typedef shared_ptr<Elem> ElemPtr;
    class StarElem : public Elem {
    public:
        StarElem () :
            Elem (STAR)
        {}
        ~StarElem () {}
        bool to_string (string &a_str) const {a_str="*";return true;}
    };//end class StarElem
    class AndElem : public Elem {
    public:
        AndElem () :
           Elem (AND)
        {}
        ~AndElem () {}
        bool to_string (string &a_str) const {a_str="&";return true;}
    };//end class AndElem
    class ConstElem : public Elem {
    public:
        ConstElem ():
            Elem (CONST)
        {}
        bool to_string (string &a_str) const {a_str="const";return true;}
    };//end class ConstElem
    class VolatileElem : public Elem {
    public:
        VolatileElem () :
            Elem (VOLATILE)
        {}
        bool to_string (string &a_str) const {a_str="volatile";return true;}
    };//end class ConstElem

private:
    QNamePtr m_scope;
    list<ElemPtr> m_elems;

public:
    PtrOperator () {}
    ~PtrOperator () {}

    void set_scope (QNamePtr a_scope) {m_scope = a_scope;}
    const QNamePtr get_scope () const {return m_scope;}
    const list<ElemPtr>& get_elems () const {return m_elems;}
    void append (const ElemPtr &a_elem) {m_elems.push_back (a_elem);}
    bool to_string (string &) const;
};//end class PtrOperator
typedef shared_ptr<PtrOperator> PtrOperatorPtr;

/// \brief the base class of declarations
class NEMIVER_API Declaration {
    Declaration ();
    Declaration (const Declaration&);
    Declaration& operator= (const Declaration&);

public:
    enum Kind {
        UNDEFINED=0,
        SIMPLE_DECLARATION
    };

private:
    Kind m_kind;

protected:
    void set_kind (Kind a_kind);

public:
    Kind get_kind () const;

    Declaration (Kind a_kind);
};//end class Declaration

class DeclSpecifier;
class InitDeclarator;
typedef shared_ptr<DeclSpecifier> DeclSpecifierPtr;
typedef shared_ptr<InitDeclarator> InitDeclaratorPtr;
/// \brief a simple-declaration
class NEMIVER_API SimpleDeclaration : public Declaration {
    SimpleDeclaration (const SimpleDeclaration&);
    SimpleDeclaration& operator= (SimpleDeclaration&);

    list<DeclSpecifierPtr> m_decl_specs;
    list<InitDeclaratorPtr> m_init_decls;
public:

    SimpleDeclaration () :
        Declaration (SIMPLE_DECLARATION)
    {}
    SimpleDeclaration (list<DeclSpecifierPtr> &a, list<InitDeclaratorPtr> &b) :
        Declaration (SIMPLE_DECLARATION),
        m_decl_specs (a),
        m_init_decls (b)
    {}
    ~SimpleDeclaration () {}
    void add_decl_specifier (DeclSpecifierPtr a_spec) {m_decl_specs.push_back (a_spec);}
    const list<DeclSpecifierPtr>& get_decl_specifiers () const {return m_decl_specs;}

    void add_init_declarator (InitDeclaratorPtr a_decl) {m_init_decls.push_back (a_decl);}
    const list<InitDeclaratorPtr>& get_init_declarators () const {return m_init_decls;}
    bool to_string (string &) const;
};//end SimpleDeclaration
typedef shared_ptr<SimpleDeclaration> SimpleDeclarationPtr;

/// the base class of all expressions
class NEMIVER_API ExprBase {
    ExprBase (const ExprBase&);
    ExprBase& operator= (const ExprBase&);
    ExprBase ();

public:
    enum Kind {
        UNDEFINED=0,
        PRIMARY_EXPRESSION,
        CONDITIONAL_EXPRESSION,
        ASSIGNMENT_EXPRESION,
        THROW_EXPRESSION,
        UNARY_EXPRESSION,
        CAST_EXPRESSION,
        PM_EXPRESSION,
        MULT_EXPR,
        ADD_EXPR,
        SHIFT_EXPR,
        RELATIONAL_EXPR,
        EQUALITY_EXPR,
        AND_EXPR,
        XOR_EXPR,
        INCL_OR_EXPR,
        LOGICAL_AND_EXPR,
        LOGICAL_OR_EXPR,
        COND_EXPR,
        ASSIGNMENT_EXPR,
        ASSIGNMENT_LIST
    };

    enum Operator {
        OP_UNDEFINED,
        MULT,
        DIV,
        MOD,
        PLUS,
        MINUS,
        LT,
        GT,
        LT_OR_EQ,
        GT_OR_EQ,
        LEFT_SHIFT,
        RIGHT_SHIFT,
        ASSIGN,
        MULT_EQ,
        DIV_EQ,
        MOD_EQ,
        PLUS_EQ,
        MINUS_EQ,
        RIGHT_SHIFT_EQ,
        LEFT_SHIFT_EQ,
        AND_EQ,
        XOR_EQ,
        OR_EQ,
        EQUALS,
        NOT_EQUALS,
        BIT_AND,
        LOG_AND,
        LOG_OR
    };

private:
    Kind m_kind;

public:
    ExprBase (Kind a_kind) :
        m_kind (a_kind)
    {
    }
    virtual ~ExprBase () {}
    Kind get_kind () {return m_kind;}
    virtual bool to_string (string &) const=0;
    static const string& operator_to_string (Operator);
};//end class ExprBase
typedef shared_ptr<ExprBase> ExprBasePtr;

class Expr;
typedef shared_ptr<Expr> ExprPtr;

class NEMIVER_API PrimaryExpr : public ExprBase {
    PrimaryExpr (const PrimaryExpr&);
    PrimaryExpr& operator= (const PrimaryExpr&);

public:
    enum Kind {
        UNDEFINED,
        LITERAL,
        THIS,
        PARENTHESIZED,
        ID_EXPR,
    };

private:
    Kind m_kind;
    Token m_token;
    IDExprPtr m_id_expr;
    ExprBasePtr m_parenthesized;

public:
    PrimaryExpr () :
        ExprBase (PRIMARY_EXPRESSION), m_kind (UNDEFINED)
    {}
    PrimaryExpr (Kind k) :
        ExprBase (PRIMARY_EXPRESSION),
        m_kind (k)
    {}
    ~PrimaryExpr () {}
    Kind get_kind () const {return m_kind;}
    void set_kind (Kind kind) {m_kind=kind;}
    void set_token (Kind kind, const Token &token) {m_kind=kind, m_token=token;}
    const Token& get_token () const {return m_token;}
    void set_id_expr (IDExprPtr id_expr) {m_kind=ID_EXPR, m_id_expr=id_expr;}
    const IDExprPtr get_id_expr () const {return m_id_expr;}
    void set_parenthesized (ExprBasePtr expr)
    {
        m_kind=PARENTHESIZED, m_parenthesized=expr;
    }
    const ExprBasePtr get_parenthesized () const {return m_parenthesized;}
};//end class PrimaryExpr
typedef shared_ptr<PrimaryExpr> PrimaryExprPtr;

class NEMIVER_API LiteralPrimaryExpr : public PrimaryExpr {
    LiteralPrimaryExpr (const LiteralPrimaryExpr&);
    LiteralPrimaryExpr& operator= (const LiteralPrimaryExpr&);

    Token m_literal;

public:
    LiteralPrimaryExpr () :
        PrimaryExpr (LITERAL)
    {}
    LiteralPrimaryExpr (const Token &t) :
        PrimaryExpr (LITERAL),
        m_literal (t)
    {}
    ~LiteralPrimaryExpr () {}
    const Token& get_token () const {return m_literal;}
    void set_token (Token &t) {m_literal=t;}
    bool to_string (string &a_str) const {a_str=m_literal.get_str_value ();return true;}
};//end class PrimaryExpr
typedef shared_ptr<LiteralPrimaryExpr> LiteralPrimaryExprPtr;

class NEMIVER_API ThisPrimaryExpr : public PrimaryExpr {
    ThisPrimaryExpr (const ThisPrimaryExpr&);
    ThisPrimaryExpr& operator= (const ThisPrimaryExpr&);

public:
    ThisPrimaryExpr () :
        PrimaryExpr (THIS)
    {}
    ~ThisPrimaryExpr () {}
    bool to_string (string &a_str) const {a_str="this"; return true;}
};//end class ThisPrimaryExpr
typedef shared_ptr<ThisPrimaryExpr> ThisPrimaryExprPtr;

class NEMIVER_API ParenthesisPrimaryExpr : public PrimaryExpr {
    ParenthesisPrimaryExpr (const ParenthesisPrimaryExpr&);
    ParenthesisPrimaryExpr& operator= (const ParenthesisPrimaryExpr&);

    ExprPtr m_expr;

public:
    ParenthesisPrimaryExpr () :
        PrimaryExpr (PARENTHESIZED)
    {}
    ParenthesisPrimaryExpr (const ExprPtr e) :
        PrimaryExpr (PARENTHESIZED),
        m_expr (e)
    {}
    ~ParenthesisPrimaryExpr () {}
    bool to_string (string &a_str) const
    {
        a_str="(";
        if (m_expr) {
            string str;
            ((ExprBase*)m_expr.get ())->to_string (str);
            a_str += str;
        }
        a_str += ")";
        return true;
    }
};//end ParenthesisPrimaryExpr

/// base type of template-argument
class NEMIVER_API TemplateArg {
    TemplateArg ();
    TemplateArg (const TemplateArg&);
    TemplateArg& operator= (const TemplateArg&);

public:
    enum Kind {
        UNDEFINED,
        ASSIGNMENT_EXPR_ARG,
        TYPE_ID_ARG,
        ID_EXPR_ARG
    };

private:
    Kind m_kind;

public:
    TemplateArg (Kind k) :
        m_kind (k)
    {}
    virtual ~TemplateArg () {}
    Kind get_kind () const {return m_kind;}
    void set_kind (Kind k) {m_kind = k;}
    virtual bool to_string (string &) const =0;
};//end class TemplateArg

typedef shared_ptr<TemplateArg> TemplateArgPtr;
class NEMIVER_API TemplateID {
    TemplateID (const TemplateID&);
    TemplateID& operator= (const TemplateID&);
    string m_name;
    list<TemplateArgPtr> m_args;

public:
    TemplateID ()
    {}
    TemplateID (const string &name,
                const list<TemplateArgPtr> &args) :
        m_name (name),
        m_args (args)
    {}
    virtual ~TemplateID () {}
    const string& get_name () const {return m_name;}
    void set_name (const string &n) {m_name = n;}
    const list<TemplateArgPtr>& get_arguments () const {return m_args;}
    void set_arguments (const list<TemplateArgPtr> &args) {m_args = args;}
    virtual bool to_string (string &a_str)
    {
        if (m_name.empty ()) {
            return false;
        }
        a_str = m_name + "<";
        string str;
        list<TemplateArgPtr>::const_iterator it;
        for (it = m_args.begin (); it != m_args.end (); ++it) {
            if (!*it) {continue;}
            (*it)->to_string (str);
            if (it != m_args.begin ()) {
                a_str += ", ";
            }
            a_str += str;
        }
        if (a_str[a_str.size ()-1] == '>') {
            a_str += ' ';
        }
        a_str += ">";
        return true;
    }
};//end class TemplateID

class NEMIVER_API IDExpr : public PrimaryExpr {
    IDExpr (const IDExpr&);
    IDExpr& operator= (const IDExpr&);
    IDExpr ();

public:
    enum Kind {
        UNDEFINED,
        QUALIFIED,
        UNQUALIFIED
    };

private:
    Kind m_kind;

public:
    IDExpr (Kind kind) :
        PrimaryExpr (ID_EXPR),
        m_kind (kind)
    {
    }
    ~IDExpr ();
    Kind get_kind () const {return m_kind;}
};//end class ExprBase

class NEMIVER_API UnqualifiedIDExpr : public IDExpr {
    UnqualifiedIDExpr (const UnqualifiedIDExpr&);
    UnqualifiedIDExpr& operator= (const UnqualifiedIDExpr&);

public:
    enum Kind {
        UNDEFINED,
        IDENTIFIER,
        OP_FUNC_ID,
        CONV_FUNC_ID, //TODO:not supported yet
        DESTRUCTOR_ID,
        TEMPLATE_ID
    };

private:
    Kind m_kind;

public:
    UnqualifiedIDExpr () :
        IDExpr (UNQUALIFIED),
        m_kind (UNDEFINED)
    {
    }

    UnqualifiedIDExpr (Kind kind) :
        IDExpr (UNQUALIFIED),
        m_kind (kind)

    {
    }
    Kind get_kind () const {return m_kind;}
    void set_kind (Kind kind) {m_kind=kind;}
    virtual ~UnqualifiedIDExpr () {}
    virtual bool to_string (string &) const=0;
};//end class UnqualifiedIDExpr

class NEMIVER_API UnqualifiedID : public UnqualifiedIDExpr {
    string m_name;

public:
    UnqualifiedID ():
        UnqualifiedIDExpr (UnqualifiedIDExpr::IDENTIFIER)
    {}
    UnqualifiedID (const string &a_s):
        UnqualifiedIDExpr (UnqualifiedIDExpr::IDENTIFIER),
        m_name (a_s)
    {}
    ~UnqualifiedID ()
    {}
    const string& get_name () const {return m_name;}
    void set_name (const string &a_n) {m_name=a_n;}
    bool to_string (string &a_s) const;
};

class UnqualifiedOpFuncID : public UnqualifiedIDExpr {
    UnqualifiedOpFuncID (const UnqualifiedOpFuncID&);
    UnqualifiedOpFuncID& operator= (const UnqualifiedOpFuncID&);
    Token m_op_token;

public:
    UnqualifiedOpFuncID ():
        UnqualifiedIDExpr (UnqualifiedIDExpr::OP_FUNC_ID)
    {}
    UnqualifiedOpFuncID (const Token a_t):
        UnqualifiedIDExpr (UnqualifiedIDExpr::OP_FUNC_ID),
        m_op_token (a_t)
    {}
    ~UnqualifiedOpFuncID ()
    {}
    void set_token (Token &a_t) {m_op_token = a_t;}
    Token::Kind get_kind () const {return m_op_token.get_kind ();}
    bool to_string (string &a_s) const;
};
typedef shared_ptr<UnqualifiedOpFuncID>  UnqualifiedOpFuncIDPtr;

class NEMIVER_API DestructorID : public UnqualifiedIDExpr {
    DestructorID (const DestructorID&);
    DestructorID& operator= (const DestructorID&);
    UnqualifiedIDExprPtr m_name;

public:
    DestructorID ():
        UnqualifiedIDExpr (UnqualifiedIDExpr::DESTRUCTOR_ID)
    {}
    DestructorID (const string &a_n) :
        UnqualifiedIDExpr (UnqualifiedIDExpr::DESTRUCTOR_ID),
        m_name (new UnqualifiedID (a_n))
    {}
    DestructorID (const UnqualifiedIDExprPtr &a_n) :
        UnqualifiedIDExpr (UnqualifiedIDExpr::DESTRUCTOR_ID),
        m_name (a_n)
    {}
    ~DestructorID ()
    {}
    void set_name (const UnqualifiedIDExprPtr &a_n) {m_name=a_n;}
    const UnqualifiedIDExprPtr get_name () const {return m_name;}
    bool to_string (string &a_s) const;
};

class NEMIVER_API UnqualifiedTemplateID : public UnqualifiedIDExpr {
    UnqualifiedTemplateID (const UnqualifiedTemplateID&);
    UnqualifiedTemplateID& operator= (const UnqualifiedTemplateID&);
    TemplateIDPtr m_id;

public:
    UnqualifiedTemplateID () :
        UnqualifiedIDExpr (TEMPLATE_ID)
    {}
    UnqualifiedTemplateID (const TemplateIDPtr t) :
        UnqualifiedIDExpr (TEMPLATE_ID),
        m_id (t)
    {}
    ~UnqualifiedTemplateID () {}
    const TemplateIDPtr get_template_id () const {return m_id;}
    void set_template_id (const TemplateIDPtr t) {m_id = t;}
    bool to_string (string &a_str) const
    {
        if (m_id) {
            m_id->to_string (a_str);
            return true;
        }
        return false;
    }
};//end class UnqualifiedTemplateID

class QualifiedIDExpr;
typedef shared_ptr<QualifiedIDExpr> QualifiedIDExprPtr;
class NEMIVER_API QualifiedIDExpr : public IDExpr {
    QualifiedIDExpr (const QualifiedIDExpr &);
    QualifiedIDExpr& operator= (const QualifiedIDExpr &);

    QNamePtr m_scope;
    UnqualifiedIDExprPtr m_id;

public:
    QualifiedIDExpr ():
        IDExpr (QUALIFIED)
    {}
    QualifiedIDExpr (const QNamePtr a_scope, const UnqualifiedIDExprPtr a_id):
        IDExpr (QUALIFIED),
        m_scope (a_scope),
        m_id (a_id)
    {}
    QualifiedIDExpr (const QName &a_scope, const UnqualifiedIDExprPtr a_id):
        IDExpr (QUALIFIED),
        m_scope (new QName (a_scope)),
        m_id (a_id)
    {}

    const QNamePtr get_scope () const {return m_scope;}
    void set_scope (const QNamePtr a_scope) {m_scope=a_scope;}
    const UnqualifiedIDExprPtr get_unqualified_id () const {return m_id;}
    void set_unqualified_id (const UnqualifiedIDExprPtr a_id) {m_id = a_id;}
    bool to_string (string &) const;
    //TODO: support template-id
};//end QualifiedIDExpr


/// contain the result of the parsing of an postfix-expression
class NEMIVER_API PostfixExpr {
    PostfixExpr ();
    PostfixExpr (const PostfixExpr&);
    PostfixExpr& operator= (const PostfixExpr&);

public:
    enum Kind {
        UNDEFINED,
        //primary-expression
        PRIMARY,
        //postfix-expression [ expression ]
        ARRAY,
        //postfix-expression ( expression-listopt )
        //simple-type-specifier ( expression-listopt )
        //typename ::(opt) nested-name-specifier identifier ( expression-listopt )
        //typename ::(opt) nested-name-specifier template(opt) template-id ...
        //(expression-listopt )
        FUNC_CALL,
        //postfix-expression . template(opt) id-expression
        MEMBER,
        // postfix-expression -> template(opt) id-expression
        POINTER_MEMBER,
        // postfix-expression . pseudo-destructor-name
        SCOPED_DESTRUCTOR,
        //postfix-expression -> pseudo-destructor-name
        SCOPED_POINTER_DESTRUCTOR,
        //postfix-expression ++
        INCREMENT,
        //postfix-expression --
        DECREMENT,
        //dynamic_cast < type-id > ( expression )
        DYNAMIC_CAST,
        //static_cast < type-id > ( expression )
        STATIC_CAST,
        //reinterpret_cast < type-id > ( expression )
        REINTERPRET_CAST,
        // const_cast < type-id > ( expression )
        CONST_CAST,
        //typeid ( expression )
        //typeid ( type-id )
        FUNC_STYLE_CAST
    };
private:
    Kind m_kind;

public:
    PostfixExpr (Kind k) :
        m_kind (k)
    {}
    virtual ~PostfixExpr () {}
    Kind get_kind () const {return m_kind;}
    virtual bool to_string (string &) const=0;
};//end class PostfixExpr
typedef shared_ptr<PostfixExpr> PostfixExprPtr;

/// contains the result of the parsing of a postfix-expression of the form:
/// primary-expression
class NEMIVER_API PrimaryPFE : public PostfixExpr {
    PrimaryPFE (const PrimaryPFE&);
    PrimaryPFE& operator= (const PrimaryPFE&);

    PrimaryExprPtr m_primary;

public:
    PrimaryPFE () :
        PostfixExpr (PRIMARY)
    {}
    PrimaryPFE (const PrimaryExprPtr e) :
        PostfixExpr (PRIMARY),
        m_primary (e)
    {}
    ~PrimaryPFE () {}
    const PrimaryExprPtr get_primary_expr () const {return m_primary;}
    void set_primary_expr (const PrimaryExprPtr &e) {m_primary=e;}
    bool to_string (string &a_str) const
    {
        if (m_primary) {
            m_primary->to_string (a_str);
        }
        return true;
    }
};//end class PrimaryPFE
typedef shared_ptr<PrimaryPFE> PrimaryPFEPtr;

/// contains a postfix expr of the type:
/// postfix-expression [ expression ]
class NEMIVER_API ArrayPFE : public PostfixExpr {
    ArrayPFE (const ArrayPFE &);
    ArrayPFE& operator= (const ArrayPFE &);

    PostfixExprPtr m_postfix_expr;
    ExprPtr m_subscript_expr;
public:
    ArrayPFE () :
        PostfixExpr (ARRAY)
    {}
    ArrayPFE (const PostfixExprPtr p,
             const ExprPtr s) :
        PostfixExpr (ARRAY),
        m_postfix_expr (p),
        m_subscript_expr (s)
    {}
    ~ArrayPFE () {}
    const PostfixExprPtr get_postfix_expr () const {return  m_postfix_expr;}
    void set_postfix_expr (const PostfixExprPtr e) {m_postfix_expr=e;}
    const ExprPtr get_subscript_expr () const {return m_subscript_expr;}
    void set_subscript_expr (const ExprPtr e) {m_subscript_expr=e;}
    bool to_string (string &a_str) const
    {
        if (!m_postfix_expr) {return true;}
        m_postfix_expr->to_string (a_str);
        string str;
        if (m_subscript_expr) {
            ((ExprBase*)m_subscript_expr.get ())->to_string (str);
        }
        a_str += "[" + str + "]";
        return true;
    }
};//end class ArrayPFE
typedef shared_ptr<ArrayPFE> ArrayPFEPtr;

/// contains the result of an
/// unary-expression production.
class NEMIVER_API UnaryExpr : public ExprBase {
    UnaryExpr ();
    UnaryExpr (const UnaryExpr&);
    UnaryExpr& operator= (const UnaryExpr&);
public:
    enum Kind {
        UNDEFINED,
        //postfix-expression
        PFE
    };
private:
    Kind m_kind;
public:
    UnaryExpr (Kind k) :
        ExprBase (UNARY_EXPRESSION),
        m_kind (k)
    {}
    ~UnaryExpr () {}
    virtual bool to_string (string &) const=0;
};//end class UnaryExpr
typedef shared_ptr<UnaryExpr> UnaryExprPtr;

/// contains the result of the parsing of a
/// postfix-expression production
class NEMIVER_API PFEUnaryExpr : public UnaryExpr {
    PFEUnaryExpr (const PFEUnaryExpr &);
    PFEUnaryExpr& operator= (const PFEUnaryExpr &);

    PostfixExprPtr m_pfe;
public:
    PFEUnaryExpr () :
        UnaryExpr (PFE)
    {}
    PFEUnaryExpr (PostfixExprPtr pfe) :
        UnaryExpr (PFE),
        m_pfe (pfe)
    {}
    ~PFEUnaryExpr () {}
    const PostfixExprPtr get_postfix_expr () const {return m_pfe;}
    void set_postfix_expr (const PostfixExprPtr pfe) {m_pfe=pfe;}
    bool to_string (string &a_str) const
    {
        if (m_pfe) {
            m_pfe->to_string (a_str);
        }
        return true;
    }
};//end class PFEUnaryExpr
typedef shared_ptr<PFEUnaryExpr> PFEUnaryExprPtr;

class NEMIVER_API CastExpr : public ExprBase {
    CastExpr (const CastExpr &);
    CastExpr& operator= (const CastExpr &);
public:
    enum Kind {
        UNDEFINED,
        UNARY,
        C_STYLE
    };
private:
    Kind m_kind;

public:
    CastExpr () :
        ExprBase (CAST_EXPRESSION),
        m_kind (UNDEFINED)
    {}
    CastExpr (Kind k) :
        ExprBase (CAST_EXPRESSION),
        m_kind (k)
    {}
    ~CastExpr () {}
    virtual bool to_string (string &) const=0;
};
typedef shared_ptr<CastExpr> CastExprPtr;

class NEMIVER_API UnaryCastExpr : public CastExpr {
    UnaryCastExpr (const UnaryCastExpr&);
    UnaryCastExpr& operator= (const UnaryCastExpr&);

    UnaryExprPtr m_expr;

public:
    UnaryCastExpr () :
        CastExpr (UNARY)
    {}
    UnaryCastExpr (const UnaryExprPtr e) :
        CastExpr (UNARY),
        m_expr (e)
    {}
    ~UnaryCastExpr () {}
    bool to_string (string &a_str) const
    {
        if (m_expr) {
            m_expr->to_string (a_str);
        }
        return true;
    }
};//end UnaryCastExpr
typedef shared_ptr<UnaryCastExpr> UnaryCastExprPtr;

class NEMIVER_API CStyleCastExpr : public CastExpr {
    CStyleCastExpr (const CStyleCastExpr&);
    CStyleCastExpr& operator= (const CStyleCastExpr&);

    TypeIDPtr m_type_id;
    CastExprPtr m_right_expr;

public:
    CStyleCastExpr () :
        CastExpr (C_STYLE)
    {}
    CStyleCastExpr (const CastExprPtr e) :
        CastExpr (C_STYLE),
        m_right_expr (e)
    {}
    CStyleCastExpr (const TypeIDPtr t, CastExprPtr r) :
        CastExpr (C_STYLE),
        m_type_id (t),
        m_right_expr (r)
    {}
    ~CStyleCastExpr () {}
    const TypeIDPtr get_type_id () const {return m_type_id;}
    void set_type_id (const TypeIDPtr t) {m_type_id=t;}
    const CastExprPtr get_right_expr () const {return m_right_expr;}
    void set_right_expr (const CastExprPtr r) {m_right_expr=r;}
    bool to_string (string &a_str) const
    {
        string str;
        if (m_type_id) {
            nemiver::cpp::to_string (m_type_id, str);
            str = "(" + str + ")";
        }
        a_str = str;
        if (m_right_expr) {
            m_right_expr->to_string (str);
            a_str += str;
        }
        return true;
    }
};//CStyleCastExpr
typedef shared_ptr<CStyleCastExpr> CStyleCastExprPtr;

class NEMIVER_API PMExpr : public ExprBase {
    PMExpr ();
    PMExpr (const PMExpr&);
    PMExpr& operator= (const PMExpr&);

public:
    enum Kind {
        UNDEFINED,
        CAST,
        DOT_STAR_PM,
        ARROW_STAR_PM
    };
private:
    Kind m_kind;

public:
    PMExpr (Kind k) :
        ExprBase (PM_EXPRESSION),
        m_kind (k)
    {}
    ~PMExpr () {}
};//PMExpr
typedef shared_ptr<PMExpr> PMExprPtr;

class NEMIVER_API CastPMExpr : public PMExpr {
    CastPMExpr (const CastPMExpr &);
    CastPMExpr& operator= (const CastPMExpr &);

    CastExprPtr m_expr;

public:
    CastPMExpr () :
        PMExpr (CAST)
    {}
    CastPMExpr (CastExprPtr e) :
        PMExpr (CAST),
        m_expr (e)
    {}
    ~CastPMExpr () {}
    bool to_string (string &a_str) const
    {
        if (m_expr) {
            m_expr->to_string (a_str);
        }
        return true;
    }
    const CastExprPtr get_expr () const {return m_expr;}
    void set_expr (const CastExprPtr e) {m_expr=e;}
};//CastPMExpr

class NEMIVER_API DotStarPMExpr : public PMExpr {
    DotStarPMExpr (const DotStarPMExpr&);
    DotStarPMExpr& operator= (const DotStarPMExpr&);
    PMExprPtr m_lhs;
    CastExprPtr m_rhs;

public:
    DotStarPMExpr () :
        PMExpr (DOT_STAR_PM)
    {}
    DotStarPMExpr (PMExprPtr lhs, CastExprPtr rhs) :
        PMExpr (DOT_STAR_PM),
        m_lhs (lhs),
        m_rhs (rhs)
    {}
    ~DotStarPMExpr () {}
    const PMExprPtr get_lhs () const {return m_lhs;}
    void set_lhs (PMExprPtr lhs) {m_lhs=lhs;}
    const CastExprPtr get_rhs () const {return m_rhs;}
    void set_rhs (CastExprPtr rhs) {m_rhs=rhs;}
    bool to_string (string &a_str) const
    {
        string str;
        if (m_lhs) {
            m_lhs->to_string (str);
        }
        if (m_rhs) {
            string str2;
            str += ".*";
            m_rhs->to_string (str2);
            str += str2;
        }
        a_str=str;
        return true;
    }
};//DotStarPMExpr
typedef shared_ptr<DotStarPMExpr> DotStarPMExprPtr;


class NEMIVER_API ArrowStarPMExpr : public PMExpr {
    ArrowStarPMExpr (const ArrowStarPMExpr&);
    ArrowStarPMExpr& operator= (const ArrowStarPMExpr&);
    PMExprPtr m_lhs;
    CastExprPtr m_rhs;

public:
    ArrowStarPMExpr () :
        PMExpr (ARROW_STAR_PM)
    {}
    ArrowStarPMExpr (PMExprPtr lhs, CastExprPtr rhs) :
        PMExpr (ARROW_STAR_PM),
        m_lhs (lhs),
        m_rhs (rhs)
    {}
    ~ArrowStarPMExpr () {}
    const PMExprPtr get_lhs () const {return m_lhs;}
    void set_lhs (PMExprPtr lhs) {m_lhs=lhs;}
    const CastExprPtr get_rhs () const {return m_rhs;}
    void set_rhs (CastExprPtr rhs) {m_rhs=rhs;}
    bool to_string (string &a_str) const
    {
        string str;
        if (m_lhs) {
            m_lhs->to_string (str);
        }
        if (m_rhs) {
            string str2;
            str += "->*";
            m_rhs->to_string (str2);
            str += str2;
        }
        a_str=str;
        return true;
    }
};//ArrowStarPMExpr
typedef shared_ptr<ArrowStarPMExpr> ArrowStarPMExprPtr;

class MultExpr;
typedef shared_ptr<MultExpr> MultExprPtr;
class NEMIVER_API MultExpr : public ExprBase {
    MultExpr (const MultExpr&);
    MultExpr& operator= (const MultExpr&);

    Operator m_op;
    MultExprPtr m_lhs;
    PMExprPtr m_rhs;
public:
    MultExpr () :
        ExprBase (MULT_EXPR),
        m_op (OP_UNDEFINED)
    {}
    MultExpr (PMExprPtr a_rhs) :
        ExprBase (MULT_EXPR),
        m_op (OP_UNDEFINED),
        m_rhs (a_rhs)
    {}
    MultExpr (MultExprPtr a_lhs,
              Operator a_op,
              PMExprPtr a_rhs) :
        ExprBase (MULT_EXPR),
        m_op (a_op),
        m_lhs (a_lhs),
        m_rhs (a_rhs)
    {}
    ~MultExpr () {}
    const MultExprPtr get_lhs () const {return m_lhs;}
    void set_lhs (const MultExprPtr l) {m_lhs=l;}
    const PMExprPtr get_rhs () const {return m_rhs;}
    void set_rhs (const PMExprPtr r) {m_rhs=r;}
    Operator get_operator () const {return m_op;}
    void set_operator (Operator o) {m_op=o;}
    bool to_string (string &a_str) const
    {
        string str;
        if (m_lhs) {
            m_lhs->to_string (str);
            str += operator_to_string (m_op);
        }
        a_str = str;
        m_rhs->to_string (str);
        a_str += str;
        return true;
    }
};//end class MultExpr

class AddExpr;
typedef shared_ptr<AddExpr> AddExprPtr;
class NEMIVER_API AddExpr : public ExprBase {
    AddExpr (const AddExpr&);
    AddExpr& operator= (const AddExpr&);
    AddExprPtr m_lhs;
    Operator m_op;
    MultExprPtr m_rhs;

public:
    AddExpr () :
        ExprBase (ADD_EXPR),
        m_op (OP_UNDEFINED)
    {}
    AddExpr (const MultExprPtr rhs) :
        ExprBase (ADD_EXPR),
        m_op (OP_UNDEFINED),
        m_rhs (rhs)
    {}
    AddExpr (const AddExprPtr lhs,
             Operator op,
             const MultExprPtr rhs) :
        ExprBase (ADD_EXPR),
        m_lhs (lhs),
        m_op (op),
        m_rhs (rhs)
    {}
    ~AddExpr () {}
    const AddExprPtr get_lhs () const {return m_lhs;}
    void set_lhs (const AddExprPtr lhs) {m_lhs = lhs;}
    const MultExprPtr get_rhs () const {return m_rhs;}
    void set_rhs (const MultExprPtr rhs) {m_rhs = rhs;}
    Operator get_operator () const {return m_op;}
    void set_operator (Operator op) {m_op = op;}
    bool to_string (string &a_str) const
    {
        string str;
        if (m_lhs) {
            m_lhs->to_string (str);
            str += operator_to_string (m_op);
        }
        if (m_rhs) {
            a_str = str;
            m_rhs->to_string (str);
            a_str += str;
        }
        return true;
    }
};//end class AddExpr

class ShiftExpr;
typedef shared_ptr<ShiftExpr> ShiftExprPtr;
class NEMIVER_API ShiftExpr : public ExprBase {
    ShiftExpr (const ShiftExpr&);
    ShiftExpr& operator= (const ShiftExpr&);
    ShiftExprPtr m_lhs;
    Operator m_op;
    AddExprPtr m_rhs;

public:
    ShiftExpr () :
        ExprBase  (SHIFT_EXPR),
        m_op (OP_UNDEFINED)
    {}
    ShiftExpr (const AddExprPtr rhs) :
        ExprBase  (SHIFT_EXPR),
        m_op (OP_UNDEFINED),
        m_rhs (rhs)
    {}
    ShiftExpr (const ShiftExprPtr lhs,
               Operator op,
               const AddExprPtr rhs) :
        ExprBase  (SHIFT_EXPR),
        m_lhs (lhs),
        m_op (op),
        m_rhs (rhs)
    {}
    ~ShiftExpr () {}
    const ShiftExprPtr get_lhs () const {return m_lhs;}
    void set_lhs (ShiftExprPtr lhs) {m_lhs = lhs;}
    Operator get_operator () const {return m_op;}
    void set_operator (Operator op) {m_op = op;}
    const AddExprPtr get_rhs () const {return m_rhs;}
    void set_rhs (AddExprPtr rhs) {m_rhs = rhs;}
    bool to_string (string &a_str) const
    {
        string str;
        if (m_lhs) {
            m_lhs->to_string (str);
            str += operator_to_string (m_op);
        }
        if (m_rhs) {
            a_str = str;
            m_rhs->to_string (str);
            a_str += str;
        }
        return true;
    }
};//end class ShiftExpr

class RelExpr;
typedef shared_ptr<RelExpr> RelExprPtr;
class NEMIVER_API RelExpr : public ExprBase {
    RelExpr (const RelExpr&);
    RelExpr& operator= (const RelExpr&);
    RelExprPtr m_lhs;
    Operator m_op;
    ShiftExprPtr m_rhs;

public:
    RelExpr () :
        ExprBase (RELATIONAL_EXPR),
        m_op (OP_UNDEFINED)
    {}
    RelExpr (ShiftExprPtr rhs) :
        ExprBase (RELATIONAL_EXPR),
        m_op (OP_UNDEFINED),
        m_rhs (rhs)
    {}
    RelExpr (RelExprPtr lhs, Operator op, ShiftExprPtr rhs) :
        ExprBase (RELATIONAL_EXPR),
        m_lhs (lhs),
        m_op (op),
        m_rhs (rhs)
    {}
    ~RelExpr () {}
    const RelExprPtr get_lhs () const {return m_lhs;}
    void set_lhs (RelExprPtr lhs) {m_lhs = lhs;}
    Operator get_operator () const {return m_op;}
    void set_operator (Operator op) {m_op = op;}
    const ShiftExprPtr get_rhs () const {return m_rhs;}
    void set_rhs (ShiftExprPtr rhs) {m_rhs = rhs;}
    bool to_string (string &a_str) const
    {
        string str;
        if (m_lhs) {
            m_lhs->to_string (str);
            str += operator_to_string (m_op);
        }
        if (m_rhs) {
            a_str = str;
            m_rhs->to_string (str);
            a_str += str;
        }
        return true;
    }
};//end class RelExpr

class EqExpr;
typedef shared_ptr<EqExpr> EqExprPtr;
/// contains an equality-expression
class NEMIVER_API EqExpr : public ExprBase {
    EqExpr (const EqExpr&);
    EqExpr& operator= (const EqExpr&);
    EqExprPtr m_lhs;
    Operator m_op;
    RelExprPtr m_rhs;

public:
    EqExpr () :
        ExprBase (EQUALITY_EXPR),
        m_op (OP_UNDEFINED)
    {}
    EqExpr (RelExprPtr rhs) :
        ExprBase (EQUALITY_EXPR),
        m_op (OP_UNDEFINED),
        m_rhs (rhs)
    {}
    EqExpr (EqExprPtr lhs, Operator op, RelExprPtr rhs) :
        ExprBase (EQUALITY_EXPR),
        m_lhs (lhs),
        m_op (op),
        m_rhs (rhs)
    {}
    ~EqExpr () {}
    const EqExprPtr get_lhs () const {return m_lhs;}
    void set_lhs (const EqExprPtr lhs) {m_lhs = lhs;}
    Operator get_operator () const {return m_op;}
    void set_operator (Operator op) {m_op = op;}
    const RelExprPtr get_rhs () const {return m_rhs;}
    void set_rhs (const RelExprPtr rhs) {m_rhs = rhs;}
    bool to_string (string &a_str) const
    {
        string str;
        if (m_lhs) {
            m_lhs->to_string (str);
            str += operator_to_string (m_op);
        }
        if (m_rhs) {
            a_str = str;
            m_rhs->to_string (str);
            a_str += str;
        }
        return true;
    }
};//end EqExprPtr

class AndExpr;
typedef shared_ptr<AndExpr> AndExprPtr;
/// contains an and-expression
class NEMIVER_API AndExpr : public ExprBase {
    AndExpr (const AndExpr&);
    AndExpr& operator= (const AndExpr&);
    AndExprPtr m_lhs;
    EqExprPtr m_rhs;

public:
    AndExpr () :
        ExprBase (AND_EXPR)
    {}
    AndExpr (const EqExprPtr rhs) :
        ExprBase (AND_EXPR),
        m_rhs (rhs)
    {}
    AndExpr (const AndExprPtr lhs,
             const EqExprPtr rhs) :
        ExprBase (AND_EXPR),
        m_lhs (lhs),
        m_rhs (rhs)
    {}
    ~AndExpr () {}
    const AndExprPtr get_lhs () const {return m_lhs;}
    void set_lhs (const AndExprPtr lhs) {m_lhs = lhs;}
    const EqExprPtr get_rhs () const {return m_rhs;}
    void set_rhs (const EqExprPtr rhs) {m_rhs = rhs;}
    bool to_string (string &a_str) const
    {
        string str;
        if (m_lhs) {
            m_lhs->to_string (str);
            str += "&";
        }
        if (m_rhs) {
            a_str = str;
            m_rhs->to_string (str);
            a_str += str;
        }
        return true;
    }
};//end class AndExpr

class XORExpr;
typedef shared_ptr<XORExpr> XORExprPtr;
class NEMIVER_API XORExpr : public ExprBase {
    XORExpr (const XORExpr&);
    XORExpr& operator= (const XORExpr&);
    XORExprPtr m_lhs;
    AndExprPtr m_rhs;

public:
    XORExpr () :
        ExprBase (XOR_EXPR)
    {}
    XORExpr (const AndExprPtr rhs) :
        ExprBase (XOR_EXPR),
        m_rhs (rhs)
    {}
    XORExpr (const XORExprPtr lhs,
                const AndExprPtr rhs) :
        ExprBase (XOR_EXPR),
        m_lhs (lhs),
        m_rhs (rhs)
    {}
    ~XORExpr () {}
    const XORExprPtr get_lhs () const {return m_lhs;}
    void set_lhs (const XORExprPtr lhs) {m_lhs = lhs;}
    const AndExprPtr get_rhs () const {return m_rhs;}
    void set_rhs (const AndExprPtr rhs) {m_rhs = rhs;}
    bool to_string (string &a_str) const
    {
        string str;
        if (m_lhs) {
            m_lhs->to_string (str);
            str += "^";
        }
        if (m_rhs) {
            a_str = str;
            m_rhs->to_string (str);
            a_str += str;
        }
        return true;
    }
};//end class XORExpr

class ORExpr;
typedef shared_ptr<ORExpr> ORExprPtr;
/// contains an inclusive-or-expression production
class NEMIVER_API ORExpr : public ExprBase {
    ORExpr (const ORExpr&);
    ORExpr& operator= (const ORExpr&);
    ORExprPtr m_lhs;
    XORExprPtr m_rhs;

public:
    ORExpr ():
        ExprBase (INCL_OR_EXPR)
    {}
    ORExpr (const XORExprPtr rhs):
        ExprBase (INCL_OR_EXPR),
        m_rhs (rhs)
    {}
    ORExpr (const ORExprPtr lhs, const XORExprPtr rhs):
        ExprBase (INCL_OR_EXPR),
        m_lhs (lhs),
        m_rhs (rhs)
    {}
    ~ORExpr () {}
    const ORExprPtr get_lhs () const {return m_lhs;}
    void set_lhs (const ORExprPtr lhs) {m_lhs = lhs;}
    const XORExprPtr get_rhs () const {return m_rhs;}
    void set_rhs (const XORExprPtr rhs) {m_rhs = rhs;}
    bool to_string (string &a_str) const
    {
        string str;
        if (m_lhs) {
            m_lhs->to_string (str);
            str += "|";
        }
        if (m_rhs) {
            a_str = str;
            m_rhs->to_string (str);
            a_str += str;
        }
        return true;
    }
};//end class ORExpr

class LogAndExpr;
typedef shared_ptr<LogAndExpr> LogAndExprPtr;
/// contains a logical-and-expression production
class NEMIVER_API LogAndExpr : public ExprBase {
    LogAndExpr (const LogAndExpr&);
    LogAndExpr& operator= (const LogAndExpr&);
    LogAndExprPtr m_lhs;
    ORExprPtr m_rhs;

public:
    LogAndExpr () :
        ExprBase (LOGICAL_AND_EXPR)
    {}
    LogAndExpr (const ORExprPtr rhs) :
        ExprBase (LOGICAL_AND_EXPR),
        m_rhs (rhs)
    {}
    LogAndExpr (const LogAndExprPtr lhs, const ORExprPtr rhs) :
        ExprBase (LOGICAL_AND_EXPR),
        m_lhs (lhs),
        m_rhs (rhs)
    {}
    ~LogAndExpr () {}
    const LogAndExprPtr get_lhs () const {return m_lhs;}
    void set_lhs (const LogAndExprPtr lhs) {m_lhs = lhs;}
    const ORExprPtr get_rhs () const {return m_rhs;}
    void set_rhs (const ORExprPtr rhs) {m_rhs = rhs;}
    bool to_string (string &a_str) const
    {
        string str;
        if (m_lhs) {
            m_lhs->to_string (str);
            str += "&&";
        }
        if (m_rhs) {
            a_str = str;
            m_rhs->to_string (str);
            a_str += str;
        }
        return true;
    }
};//end class LogAndExpr

class LogOrExpr;
typedef shared_ptr<LogOrExpr> LogOrExprPtr;
/// contains a logical-or-expression
class NEMIVER_API LogOrExpr : public ExprBase {
    LogOrExpr (const LogOrExpr&);
    LogOrExpr& operator= (const LogOrExpr&);
    LogOrExprPtr m_lhs;
    LogAndExprPtr m_rhs;

public:
    LogOrExpr () :
        ExprBase (LOGICAL_OR_EXPR)
    {}
    LogOrExpr (const LogAndExprPtr rhs) :
        ExprBase (LOGICAL_OR_EXPR),
        m_rhs (rhs)
    {}
    LogOrExpr (const LogOrExprPtr lhs, const LogAndExprPtr rhs) :
        ExprBase (LOGICAL_OR_EXPR),
        m_lhs (lhs),
        m_rhs (rhs)
    {}
    ~LogOrExpr () {}
    const LogOrExprPtr get_lhs () const {return m_lhs;}
    void set_lhs (const LogOrExprPtr lhs) {m_lhs = lhs;}
    const LogAndExprPtr get_rhs () const {return m_rhs;}
    void set_rhs (const LogAndExprPtr rhs) {m_rhs = rhs;}
    bool to_string (string &a_str) const
    {
        string str;
        if (m_lhs) {
            m_lhs->to_string (str);
            str += "||";
        }
        if (m_rhs) {
            a_str = str;
            m_rhs->to_string (str);
            a_str += str;
        }
        return true;
    }
};//end class LogOrExpr

class AssignExpr;
typedef shared_ptr<AssignExpr> AssignExprPtr;

class CondExpr;
typedef shared_ptr<CondExpr> CondExprPtr;
/// contains a conditional-expression
class NEMIVER_API CondExpr : public ExprBase {
    CondExpr (const CondExprPtr &);
    CondExpr& operator= (const CondExprPtr &);
    LogOrExprPtr m_cond;
    ExprBasePtr m_then_branch;
    AssignExprPtr m_else_branch;

public:
    CondExpr ():
        ExprBase (COND_EXPR)
    {}
    CondExpr (LogOrExprPtr cond):
        ExprBase (COND_EXPR),
        m_cond (cond)
    {}
    CondExpr (LogOrExprPtr cond,
              ExprBasePtr then_branch,
              AssignExprPtr else_branch):
        ExprBase (COND_EXPR),
        m_cond (cond),
        m_then_branch (then_branch),
        m_else_branch (else_branch)
    {}
    ~CondExpr () {}
    const LogOrExprPtr get_condition () const {return m_cond;}
    void set_condition (const LogOrExprPtr cond) {m_cond = cond;}
    const ExprBasePtr get_then_branch () const {return m_then_branch;}
    void set_then_branch (const ExprBasePtr tb) {m_then_branch = tb;}
    const AssignExprPtr get_else_branch () const {return m_else_branch;}
    void set_else_branch (const AssignExprPtr eb) {m_else_branch = eb;}
    bool to_string (string &a_str) const
    {
        string str;
        if (m_cond) {
            m_cond->to_string (a_str);
        }
        if (m_then_branch) {
            a_str += "?";
            m_then_branch->to_string (str);
            a_str += str;
        }
        if (m_else_branch) {
            a_str += ":";
            ((ExprBase*)m_else_branch.get ())->to_string (str);
            a_str += str;
        }
        return true;
    }
};//end class CondExpr

/// contains an assignment-expression
class NEMIVER_API AssignExpr : public ExprBase {
    AssignExpr ();
    AssignExpr (const AssignExpr&);
    AssignExpr& operator= (const AssignExpr&);

public:
    enum Kind {
        UNDEFINED,
        CONDITIONAL,
        FULL,
        THROW_EXPRESSION
    };

private:
    Kind m_kind;

public:
    AssignExpr (Kind kind) :
        ExprBase (ASSIGNMENT_EXPR),
        m_kind (kind)
    {}
    ~AssignExpr () {}
};//end class AssignExpr

class NEMIVER_API CondAssignExpr : public AssignExpr {
    CondAssignExpr (const CondAssignExpr &);
    CondAssignExpr& operator= (const CondAssignExpr &);
    CondExprPtr m_cond;

public:
    CondAssignExpr () :
        AssignExpr (AssignExpr::CONDITIONAL)
    {}
    CondAssignExpr (const CondExprPtr cond) :
        AssignExpr (AssignExpr::CONDITIONAL),
        m_cond (cond)
    {}
    ~CondAssignExpr () {}
    const CondExprPtr get_condition () const {return m_cond;}
    void set_condition (const CondExprPtr cond) {m_cond = cond;}
    bool to_string (string &a_str) const
    {
        if (m_cond) {
            m_cond->to_string (a_str);
        } else {
            return false;
        }
        return true;
    }
};//end class CondAssignExpr
typedef shared_ptr<CondAssignExpr> CondAssignExprPtr;

class NEMIVER_API FullAssignExpr : public AssignExpr {
    FullAssignExpr (const FullAssignExpr&);
    FullAssignExpr& operator= (const FullAssignExpr&);
    LogOrExprPtr m_lhs;
    Operator m_op;
    AssignExprPtr m_rhs;

public:
    FullAssignExpr () :
        AssignExpr (AssignExpr::FULL)
    {}
    FullAssignExpr (const LogOrExprPtr lhs,
                    Operator op,
                    const AssignExprPtr rhs) :
        AssignExpr (AssignExpr::FULL),
        m_lhs (lhs),
        m_op (op),
        m_rhs (rhs)
    {}
    ~FullAssignExpr () {}
    const LogOrExprPtr get_lhs () const {return m_lhs;}
    void set_lhs (const LogOrExprPtr lhs) {m_lhs = lhs;}
    Operator get_operator () const {return m_op;}
    void set_operator (Operator op) {m_op = op;}
    AssignExprPtr get_rhs () const {return m_rhs;}
    void set_rhs (const AssignExprPtr rhs) {m_rhs = rhs;}
    bool to_string (string &a_str) const
    {
        string str, str2;
        if (m_lhs) {
            m_lhs->to_string (str2);
            str += str2;
        }
        if (m_rhs) {
            str += operator_to_string (m_op);
            m_rhs->to_string (str2);
            str += str2;
        }
        a_str = str;
        return true;
    }
};

/// contains an expression production
class NEMIVER_API Expr : public ExprBase {
    Expr (const Expr&);
    Expr& operator= (const Expr&);
    list<AssignExprPtr> m_assignments;

public:
    Expr () :
        ExprBase (ASSIGNMENT_LIST)
    {}
    Expr (const list<AssignExprPtr> &a) :
        ExprBase (ASSIGNMENT_LIST),
        m_assignments (a)
    {}
    ~Expr () {}
    const list<AssignExprPtr>& get_assignements () const {return m_assignments;}
    void set_assignments (const list<AssignExprPtr> &l) {m_assignments = l;}
    void append_assignment (const AssignExprPtr a) {m_assignments.push_back (a);}
    bool to_string (string &a_str) const
    {
        string str;
        list<AssignExprPtr>::const_iterator it;
        for (it = m_assignments.begin (); it != m_assignments.end (); ++it) {
            if (!*it) {continue;}
            (*it)->to_string (str);
            if (it == m_assignments.begin ()) {
                a_str = str;
            } else {
                a_str += ", " + str;
            }
        }
        return true;
    }
};

/// contains a constant-expression production
class NEMIVER_API ConstExpr : public ExprBase {
    ConstExpr (const ConstExpr &);
    ConstExpr& operator= (const ConstExpr &);
    CondExprPtr m_cond;

public:
    ConstExpr () :
        ExprBase (COND_EXPR)
    {}
    ConstExpr (const CondExprPtr cond) :
        ExprBase (COND_EXPR),
        m_cond (cond)
    {}
    ~ConstExpr () {}
    const CondExprPtr get_condition () const {return m_cond;}
    void set_condition (const CondExprPtr cond) {m_cond = cond;}
    bool to_string (string &a_str) const
    {
        if (m_cond) {
            m_cond->to_string (a_str);
            return true;
        }
        return false;
    }
};//end class ConstExpr
typedef shared_ptr<ConstExpr> ConstExprPtr;

/// the assignment-expression form of template-argument
class NEMIVER_API AssignExprTemplArg : public TemplateArg {
    AssignExprTemplArg (const AssignExprTemplArg&);
    AssignExprTemplArg& operator= (const AssignExprTemplArg&);
    AssignExprPtr m_expr;

public:
    AssignExprTemplArg () :
        TemplateArg (ASSIGNMENT_EXPR_ARG)
    {}
    AssignExprTemplArg (const AssignExprPtr e) :
        TemplateArg (ASSIGNMENT_EXPR_ARG),
        m_expr (e)
    {}
    ~AssignExprTemplArg () {}
    const AssignExprPtr get_expression () const {return m_expr;}
    void set_expression (const AssignExprPtr e) {m_expr = e;}
    bool to_string (string &a_str) const
    {
        if (m_expr) {
            m_expr->to_string (a_str);
            return true;
        }
        return false;
    }
};//end class AssignExprTemplArg
typedef shared_ptr<AssignExprTemplArg> AssignExprTemplArgPtr;

class NEMIVER_API TypeIDTemplArg : public TemplateArg {
    TypeIDTemplArg (const TypeIDTemplArg&);
    TypeIDTemplArg& operator= (const TypeIDTemplArg&);
    TypeIDPtr m_id;

public:
    TypeIDTemplArg () :
        TemplateArg (TYPE_ID_ARG)
    {}
    TypeIDTemplArg (const TypeIDPtr id) :
        TemplateArg (TYPE_ID_ARG),
        m_id (id)
    {}
    ~TypeIDTemplArg () {}
    const TypeIDPtr get_type_id () const {return m_id;}
    void set_type_id (const TypeIDPtr id) {m_id = id;}
    bool to_string (string &a_str) const
    {
        if (m_id) {
            nemiver::cpp::to_string (m_id, a_str);
            return true;
        }
        return false;
    }
};//end class TypeIDTemplArg
typedef shared_ptr<TypeIDTemplArg> TypeIDTemplArgPtr;

class NEMIVER_API IDExprTemplArg : public TemplateArg {
    IDExprTemplArg (const IDExprTemplArg&);
    IDExprTemplArg& operator= (const IDExprTemplArg&);
    IDExprPtr m_expr;

public:
    IDExprTemplArg () :
        TemplateArg (ID_EXPR_ARG)
    {}
    IDExprTemplArg (const IDExprPtr id) :
        TemplateArg (ID_EXPR_ARG),
        m_expr (id)
    {}
    ~IDExprTemplArg () {}
    const IDExprPtr get_id_expr () const {return m_expr;}
    void set_id_expr (const IDExprPtr e) {m_expr = e;}
    bool to_string (string &a_str) const
    {
        if (m_expr) {
            m_expr->to_string (a_str);
            return true;
        }
        return false;
    }
};//end class
typedef shared_ptr<IDExprTemplArg> IDExprTemplArgPtr;

class NEMIVER_API DeclSpecifier {
    DeclSpecifier (const DeclSpecifier&);
    DeclSpecifier& operator= (const DeclSpecifier&);

public:
    enum Kind {
        UNDEFINED,
        //storage class speficiers
        AUTO,
        REGISTER,
        STATIC,
        EXTERN,
        MUTABLE,
        //type specifier
        TYPE,
        //function specifiers
        INLINE,
        VIRTUAL,
        EXPLICIT,
        //friend specifier
        FRIEND,
        //typedef specifier
        TYPEDEF
    };

private:
    Kind m_kind;

    DeclSpecifier ();
public:

    DeclSpecifier (Kind k):
        m_kind (k)
    {
    }

    virtual ~DeclSpecifier ()
    {
    }

    Kind get_kind () const {return m_kind;}
    void set_kind (Kind a_kind) {m_kind=a_kind;}

    bool is_storage_class ()
    {
        return (m_kind == AUTO || m_kind == REGISTER
                || m_kind == STATIC || m_kind == EXTERN
                || m_kind == MUTABLE);
    }
    virtual bool to_string (string &a_str) const=0;
    static bool list_to_string (const list<DeclSpecifierPtr> &a_decls, string &a_str);
};//end class DeclSpecifier

class NEMIVER_API AutoSpecifier : public DeclSpecifier {
    AutoSpecifier (const AutoSpecifier&);
    AutoSpecifier& operator= (const AutoSpecifier&);
public:
    AutoSpecifier () :
        DeclSpecifier (AUTO)
    {}
    ~AutoSpecifier () {}
    bool to_string (string &a_str) const {a_str="auto";return true;}
};//end class AutoSpecifier

class NEMIVER_API RegisterSpecifier : public DeclSpecifier {
    RegisterSpecifier (const RegisterSpecifier&);
    RegisterSpecifier& operator= (const RegisterSpecifier&);
public:
    RegisterSpecifier () :
        DeclSpecifier (REGISTER)
    {}
    ~RegisterSpecifier () {}
    bool to_string (string &a_str) const {a_str="register";return true;}
};
typedef shared_ptr<RegisterSpecifier> RegisterSpecifierPtr;

class NEMIVER_API StaticSpecifier : public DeclSpecifier {
    StaticSpecifier (const StaticSpecifier&);
    StaticSpecifier& operator= (const StaticSpecifier&);
public:
    StaticSpecifier () :
        DeclSpecifier (STATIC)
    {}
    ~StaticSpecifier () {}
    bool to_string (string &a_str) const {a_str="static";return true;}
};
typedef shared_ptr<StaticSpecifier> StaticSpecifierPtr;

class NEMIVER_API ExternSpecifier : public DeclSpecifier {
    ExternSpecifier (const ExternSpecifier&);
    ExternSpecifier& operator= (const ExternSpecifier&);
public:
    ExternSpecifier () :
        DeclSpecifier (EXTERN)
    {}
    ~ExternSpecifier () {}
    bool to_string (string &a_str) const {a_str="extern";return true;}
};


class NEMIVER_API MutableSpecifier : public DeclSpecifier {
    MutableSpecifier (const MutableSpecifier&);
    MutableSpecifier& operator= (const MutableSpecifier&);
public:
    MutableSpecifier () :
        DeclSpecifier (MUTABLE)
    {}
    ~MutableSpecifier () {}
    bool to_string (string &a_str) const {a_str="mutable";return true;}
};
class NEMIVER_API TypeSpecifier : public DeclSpecifier {
    TypeSpecifier ();
    TypeSpecifier (const TypeSpecifier&);
    TypeSpecifier& operator= (const TypeSpecifier&);

public:
    enum Kind {
        UNDEFINED,
        SIMPLE,
        CLASS,
        ENUM,
        ELABORATED,
        CONST,
        VOLATILE
    };

private:
    Kind m_kind;

public:

    TypeSpecifier (Kind k):
        DeclSpecifier (TYPE),
        m_kind (k)
    {
    }
    virtual ~TypeSpecifier () {}

    Kind get_kind () const {return m_kind;}
    void set_kind (Kind a_kind) {m_kind = a_kind;}
    bool is_cv_qualifier () const {return (m_kind == CONST || m_kind == VOLATILE);}
    virtual bool to_string (string &a_str) const=0;
    static bool list_to_string (const list<TypeSpecifierPtr> &, string &);
};//end class TypeSpecifier
typedef shared_ptr<TypeSpecifier> TypeSpecifierPtr;

class NEMIVER_API SimpleTypeSpec : public TypeSpecifier {
    SimpleTypeSpec (const SimpleTypeSpec&);
    SimpleTypeSpec& operator= (const SimpleTypeSpec&);

    QNamePtr m_scope;
    UnqualifiedIDExprPtr m_name;

public:
    SimpleTypeSpec () :
        TypeSpecifier (SIMPLE)
    {}
    SimpleTypeSpec (const QNamePtr a_scope, const string &a_name) :
        TypeSpecifier (SIMPLE),
        m_scope (a_scope),
        m_name (new UnqualifiedID (a_name))
    {}
    SimpleTypeSpec (const QNamePtr a_scope, const UnqualifiedIDExprPtr a_name) :
        TypeSpecifier (SIMPLE),
        m_scope (a_scope),
        m_name (a_name)
    {}
    ~SimpleTypeSpec () {}
    void set_scope (const QNamePtr &a_scope) {m_scope = a_scope;}
    const QNamePtr get_scope () const {return m_scope;}
    void set_name (const UnqualifiedIDExprPtr a_name) {m_name = a_name;}
    const UnqualifiedIDExprPtr get_name () const {return m_name;}
    string get_name_as_string () const
    {
        string str;
        if (!m_name)
            return str;
        m_name->to_string (str);
        return str;
    }
    bool to_string (string &a_str) const
    {
        string str;
        if (m_scope) {
            m_scope->to_string (str);
            str += "::";
        }
        if (m_name) {
            string tmp;
            m_name->to_string (tmp);
            str += tmp;
        }
        a_str = str;
        return true;
    }
};//end SimpleTypeSpec
typedef shared_ptr<SimpleTypeSpec> SimpleTypeSpecPtr;

class NEMIVER_API ClassTypeSpec : public TypeSpecifier {
    ClassTypeSpec (const ClassTypeSpec&);
    ClassTypeSpec& operator= (const ClassTypeSpec&);

public:
    ClassTypeSpec () :
        TypeSpecifier (CLASS)
    {}
    ~ClassTypeSpec () {}
    bool to_string (string &a_str) const {a_str="class";return true;}
};//end ClassTypeSpec
typedef shared_ptr<SimpleTypeSpec> SimpleTypeSpecPtr;

class NEMIVER_API EnumTypeSpec : public TypeSpecifier {
    EnumTypeSpec (const EnumTypeSpec&);
    EnumTypeSpec& operator= (const EnumTypeSpec&);

public:
    EnumTypeSpec () :
        TypeSpecifier (ENUM)
    {}
    ~EnumTypeSpec () {}
    bool to_string (string &a_str) const {a_str="enum";return true;}
};//end EnumTypeSpec
typedef shared_ptr<EnumTypeSpec> EnumTypeSpecPtr;

class NEMIVER_API ElaboratedTypeSpec : public TypeSpecifier {
    ElaboratedTypeSpec (const ElaboratedTypeSpec&);
    ElaboratedTypeSpec& operator= (const ElaboratedTypeSpec&);

public:
    class Elem {
        Elem ();
    public:
        enum Kind {
            UNDEFINED,
            CLASS,
            STRUCT,
            UNION,
            ENUM,
            TYPENAME,
            SCOPE,
            IDENTIFIER,
            TEMPLATE,
            TEMPLATE_ID
        };
    private:
        Kind m_kind;
    public:
        Elem (Kind k) :
            m_kind (k)
        {}
        virtual ~Elem () {}
        virtual bool to_string (string &) const=0;
    };
    typedef shared_ptr<Elem> ElemPtr;

    class ClassElem : public Elem {
    public:
        ClassElem ():
            Elem (CLASS)
        {}
        ~ClassElem () {}
        bool to_string (string &a_str) const {a_str="class";return true;}
    };
    typedef shared_ptr<ClassElem> ClassElemPtr;

    class StructElem : public Elem {
    public:
        StructElem () :
            Elem (STRUCT)
        {}
        ~StructElem () {}
        bool to_string (string &a_str) const {a_str="struct";return true;}
    };
    typedef shared_ptr<StructElem> StructElemPtr;

    class UnionElem : public Elem {
    public:
        UnionElem ():
            Elem (UNION)
        {}
        ~UnionElem () {}
        bool to_string (string &a_str) const {a_str="union";return true;}
    };
    typedef shared_ptr<UnionElem> UnionElemPtr;

    class EnumElem : public Elem {
    public:
        EnumElem ():
            Elem (ENUM)
        {}
        ~EnumElem () {}
        bool to_string (string &a_str) const {a_str="enum";return true;}
    };
    typedef shared_ptr<EnumElem> EnumElemPtr;

    class TypenameElem : public Elem {
    public:
        TypenameElem () :
            Elem (TYPENAME)
        {}
        ~TypenameElem () {}
        bool to_string (string &a_str) const {a_str="typename";return true;}
    };
    typedef shared_ptr<TypenameElem> TypenameElemPtr;

    class ScopeElem : public Elem {
        QNamePtr m_scope;
    public:
        ScopeElem () :
            Elem (SCOPE)
        {}
        ScopeElem (const QNamePtr s) :
            Elem (SCOPE),
            m_scope (s)
        {}
        ~ScopeElem () {}
        void set_scope (const QNamePtr s) {m_scope=s;}
        const QNamePtr& get_qname () const {return m_scope;}
        bool to_string (string &a_str) const
        {
            if (m_scope) {
                m_scope->to_string (a_str);
            }
            return true;
        }
    };
    typedef shared_ptr<ScopeElem> ScopeElemPtr;

    class IdentifierElem : public Elem {
        string m_name;
    public:
        IdentifierElem ():
            Elem (IDENTIFIER)
        {}
        IdentifierElem (const string &a_id):
            Elem (IDENTIFIER),
            m_name (a_id)
        {}
        ~IdentifierElem () {}
        const string& get_name () const {return m_name;}
        void set_name (const string &a_name) {m_name=a_name;}
        bool to_string (string &a_str) const {a_str=m_name;return true;}
    };
    typedef shared_ptr<IdentifierElem> IdentifierElemPtr;

    class TemplateElem : public Elem {
    public:
        TemplateElem ():
            Elem (TEMPLATE)
        {}
        ~TemplateElem () {}
        bool to_string (string &a_str) const {a_str="template";return true;}
    };
    typedef shared_ptr<TemplateElem> TemplateElemPtr;

private:
    list<ElemPtr> m_elems;

public:
    ElaboratedTypeSpec () :
        TypeSpecifier (ELABORATED)
    {}
    ElaboratedTypeSpec (const list<ElaboratedTypeSpec::ElemPtr> &elems) :
        TypeSpecifier (ELABORATED),
        m_elems (elems)
    {}
    ~ElaboratedTypeSpec () {}
    void append (const ElemPtr a_elem) {m_elems.push_back (a_elem);}
    void append (const list<ElemPtr> &a_elems)
    {
        list<ElemPtr>::const_iterator it;
        for (it=a_elems.begin (); it != a_elems.end (); ++it) {
            m_elems.push_back (*it);
        }
    }
    const list<ElemPtr>& get_elems () const {return m_elems;}
    bool to_string (string &a_str) const
    {
        string str;
        list<ElemPtr>::const_iterator it;
        for (it = m_elems.begin (); it != m_elems.end (); ++it) {
            if (it == m_elems.begin () && *it) {
                (*it)->to_string (a_str);
            } else if (*it) {
                (*it)->to_string (str);
                a_str += " " + str;
            }
        }
        return true;
    }
};//end ElaboratedTypeSpec
typedef shared_ptr<ElaboratedTypeSpec> ElaboratedTypeSpecPtr;

class NEMIVER_API ConstTypeSpec : public TypeSpecifier {
    ConstTypeSpec (const ConstTypeSpec&);
    ConstTypeSpec& operator= (const ConstTypeSpec&);

public:
    ConstTypeSpec () :
        TypeSpecifier (CONST)
    {}
    ~ConstTypeSpec () {}
    bool to_string (string &a_str) const {a_str="const";return true;}
};//end ConstTypeSpec
typedef shared_ptr<ConstTypeSpec> ConstTypeSpecPtr;

class NEMIVER_API VolatileTypeSpec : public TypeSpecifier {
    VolatileTypeSpec (const VolatileTypeSpec&);
    VolatileTypeSpec& operator= (const VolatileTypeSpec&);

public:
    VolatileTypeSpec () :
        TypeSpecifier (VOLATILE)
    {}
    ~VolatileTypeSpec () {}
    bool to_string (string &a_str) const {a_str="volatile";return true;}
};//end VolatileTypeSpec
typedef shared_ptr<VolatileTypeSpec> VolatileTypeSpecPtr;

class NEMIVER_API InlineSpecifier : public DeclSpecifier {
    InlineSpecifier (const InlineSpecifier&);
    InlineSpecifier& operator= (const InlineSpecifier&);
public:
    InlineSpecifier () :
        DeclSpecifier (INLINE)
    {}
    ~InlineSpecifier () {}
    bool to_string (string &a_str) const {a_str="inline";return true;}
};
typedef shared_ptr<InlineSpecifier> InlineSpecifierPtr;

class NEMIVER_API VirtualSpecifier : public DeclSpecifier {
    VirtualSpecifier (const VirtualSpecifier&);
    VirtualSpecifier& operator= (const VirtualSpecifier&);
public:
    VirtualSpecifier () :
        DeclSpecifier (VIRTUAL)
    {}
    ~VirtualSpecifier () {}
    bool to_string (string &a_str) const {a_str="virtual";return true;}
};
typedef shared_ptr<VirtualSpecifier> VirtualSpecifierPtr;

class NEMIVER_API ExplicitSpecifier : public DeclSpecifier {
    ExplicitSpecifier (const ExplicitSpecifier&);
    ExplicitSpecifier& operator= (const ExplicitSpecifier&);
public:
    ExplicitSpecifier () :
        DeclSpecifier (EXPLICIT)
    {}
    ~ExplicitSpecifier () {}
    bool to_string (string &a_str) const {a_str="explicit";return true;}
};
typedef shared_ptr<ExplicitSpecifier> ExplicitSpecifierPtr;

class NEMIVER_API FriendSpecifier : public DeclSpecifier {
    FriendSpecifier (const FriendSpecifier&);
    FriendSpecifier& operator= (const FriendSpecifier&);

public:
    FriendSpecifier () :
        DeclSpecifier (FRIEND)
    {}
    ~FriendSpecifier () {}
    bool to_string (string &a_str) const {a_str="friend";return true;}
};
typedef shared_ptr<FriendSpecifier> FriendSpecifierPtr;

class NEMIVER_API TypedefSpecifier : public DeclSpecifier {
    TypedefSpecifier (const TypedefSpecifier&);
    TypedefSpecifier& operator= (const TypedefSpecifier&);
public:
    TypedefSpecifier () :
        DeclSpecifier (TYPEDEF)
    {}
    ~TypedefSpecifier () {}
    bool to_string (string &a_str) const {a_str="typedef";return true;}
};
typedef shared_ptr<TypedefSpecifier> TypedefSpecifierPtr;

class NEMIVER_API TypeID {
    TypeID (const TypeID&);
    TypeID& operator= (const TypeID&);

    list<TypeSpecifierPtr> m_type_specs;
    //TODO: add abstract-declarator

public:
    TypeID () {}
    TypeID (const list<TypeSpecifierPtr> &a_type_specs) :
        m_type_specs (a_type_specs)
    {}
    ~TypeID () {}
    const list<TypeSpecifierPtr>& get_type_specifiers () const {return m_type_specs;}
    void set_type_specifiers (const list<TypeSpecifierPtr> &t) {m_type_specs=t;}
    bool to_string (string &a_str) const
    {
        list<TypeSpecifierPtr>::const_iterator it;
        for (it=m_type_specs.begin (); it != m_type_specs.end (); ++it) {
            if (!*it) {continue;}
            if (it == m_type_specs.begin ()) {
                (*it)->to_string (a_str);
            } else {
                string str;
                (*it)->to_string (str);
                a_str += " " + str;
            }
        }
        return true;
    }
};//end class TypeID

class Declarator;
typedef shared_ptr<Declarator> DeclaratorPtr;
/// \brief the declarator class
/// the result of the direct-declarator production is stored
/// in this type, as well as the declarator production.
class NEMIVER_API Declarator {
    Declarator ();
    Declarator (const Declarator&);
    Declarator& operator= (const Declarator&);

public:
    enum Kind {
        UNDEFINED,
        ID_DECLARATOR,
        FUNCTION_DECLARATOR,
        ARRAY_DECLARATOR,
        FUNCTION_POINTER_DECLARATOR
    };

private:
    Kind m_kind;
    PtrOperatorPtr m_ptr_node;
    DeclaratorPtr m_decl_node;

public:

    Declarator (Kind k) :
        m_kind (k)
    {}
    Declarator (PtrOperatorPtr a_ptr_node,
                DeclaratorPtr a_decl_node) :
        m_kind (UNDEFINED),
        m_ptr_node (a_ptr_node),
        m_decl_node (a_decl_node)
    {}
    Declarator (DeclaratorPtr a_decl_node) :
        m_kind (UNDEFINED),
        m_decl_node (a_decl_node)
    {}
    virtual ~Declarator () {}

    Kind get_kind () const {return m_kind;}

    void set_kind (Kind k) {m_kind=k;}

    const PtrOperatorPtr& get_ptr_op_node () const {return m_ptr_node;}

    void set_ptr_op_node (const PtrOperatorPtr a_ptr) {m_ptr_node=a_ptr;}

    const DeclaratorPtr& get_decl_node () const {return m_decl_node;}

    void set_decl_node (const DeclaratorPtr a_decl) {m_decl_node = a_decl;}

    virtual bool to_string (string &a_str) const
    {
        if (get_ptr_op_node ()) {
            get_ptr_op_node ()->to_string (a_str);
        }
        if (get_decl_node ()) {
            string str;
            get_decl_node ()->to_string (str);
            if (a_str.size ()
                && (*a_str.rbegin () != '*' && *a_str.rbegin () != ' ')) {
                a_str += ' ';
            }
            a_str += str;
        }
        return true;
    }
};//class Declarator

class NEMIVER_API IDDeclarator : public Declarator {
    IDDeclarator (const IDDeclarator&);
    IDDeclarator& operator= (const IDDeclarator&);

    IDExprPtr m_id;

public:
    IDDeclarator () :
        Declarator (Declarator::ID_DECLARATOR)
    {}
    IDDeclarator (const IDExprPtr a_id) :
        Declarator (ID_DECLARATOR),
        m_id (a_id)
    {}
    const IDExprPtr get_id () const {return m_id;}
    bool to_string (string &a_str) const
    {
        if (!m_id) {return false;}
        string str, tmp;
        if (get_ptr_op_node ()) {
            get_ptr_op_node ()->to_string (str);
            str += " ";
        }
        m_id->to_string (tmp);
        str += tmp;
        a_str = str;
        return true;
    }
};//class IDDeclarator
typedef shared_ptr<IDDeclarator> IDDeclaratorPtr;

class NEMIVER_API FuncDeclarator : public Declarator {
    FuncDeclarator (const FuncDeclarator&);
    FuncDeclarator& operator= (const FuncDeclarator&);

    DeclaratorPtr m_declarator;
    //TODO: put parameters here
    list<CVQualifierPtr> m_cvs;
    //TODO: put exception spec here
public:
    FuncDeclarator () :
        Declarator (FUNCTION_DECLARATOR)
    {}
    ~FuncDeclarator () {}
    void set_declarator (const DeclaratorPtr a_decl) {m_declarator = a_decl;}
    DeclaratorPtr get_declarator () const {return m_declarator;}
    void append_cv_qualifier (const CVQualifierPtr a_cv) {m_cvs.push_back (a_cv);}
    const list<CVQualifierPtr>& get_cv_qualifiers () const {return m_cvs;}
    bool to_string (string &a_str) {a_str="<FuncDecl not supported>";return true;}
};//end FuncDeclarator
typedef shared_ptr<FuncDeclarator> FuncDeclaratorPtr;

class NEMIVER_API ArrayDeclarator : public Declarator {
    ArrayDeclarator (const ArrayDeclarator&);
    ArrayDeclarator& operator= (const ArrayDeclarator&);

    DeclaratorPtr m_declarator;
    ConstExprPtr m_const_expr;

public:
    ArrayDeclarator () :
        Declarator (Declarator::ARRAY_DECLARATOR)
    {}
    ArrayDeclarator (const DeclaratorPtr decl) :
        Declarator (Declarator::ARRAY_DECLARATOR),
        m_declarator (decl)
    {}
    ArrayDeclarator (const DeclaratorPtr decl,
                     const ConstExprPtr const_expr) :
        Declarator (Declarator::ARRAY_DECLARATOR),
        m_declarator (decl),
        m_const_expr (const_expr)
    {}
    ~ArrayDeclarator () {}
    void set_declarator (const DeclaratorPtr a_decl) {m_declarator=a_decl;}
    const DeclaratorPtr get_declarator () const {return m_declarator;}
    const ConstExprPtr get_constant_expression () const {return m_const_expr;}
    void set_constant_expression (const ConstExprPtr expr) {m_const_expr = expr;}
    bool to_string (string &a_str) const
    {
        string str;
        if (m_declarator) {
            m_declarator->to_string (str);
            a_str = str;
        }
        a_str += '[';
        if (m_const_expr) {
            m_const_expr->to_string (str);
            a_str += str;
        }
        a_str += ']';
        return true;
    }
};//class ArrayDeclarator
typedef shared_ptr<ArrayDeclarator> ArrayDeclaratorPtr;

class NEMIVER_API FuncPointerDeclarator : public Declarator {
    FuncPointerDeclarator (const FuncPointerDeclarator&);
    FuncPointerDeclarator& operator= (const FuncPointerDeclarator&);

    DeclaratorPtr m_declarator;

public:
    FuncPointerDeclarator () :
        Declarator (Declarator::FUNCTION_POINTER_DECLARATOR)
    {}
    ~FuncPointerDeclarator () {}
    void set_declarator (const DeclaratorPtr a_decl) {m_declarator=a_decl;}
    const DeclaratorPtr get_declarator () const {return m_declarator;}
    bool to_string (string &a_str)
    {
        if (!m_declarator) {return false;}
        if (get_ptr_op_node ()) {a_str = "*";}
        string str;
        m_declarator->to_string (str);
        a_str += "(" + str + ")";
        return true;
    }
};//class FuncPointerDeclarator


class NEMIVER_API InitDeclarator {
    InitDeclarator (const InitDeclarator&);
    InitDeclarator& operator= (const InitDeclarator&);

    DeclaratorPtr m_declarator;

public:
    InitDeclarator () {}
    InitDeclarator (const DeclaratorPtr &d) :
        m_declarator (d)
    {}
    ~InitDeclarator () {}
    const DeclaratorPtr get_declarator () const {return m_declarator;}
    void set_declarator (const DeclaratorPtr &a_decl) {m_declarator=a_decl;}
    bool to_string (string &a_str) const
    {
        if (!get_declarator ()) {return false;}
        return m_declarator->to_string (a_str);
    }
    static bool list_to_string (const list<InitDeclaratorPtr> &, string &);
};

NEMIVER_END_NAMESPACE (cpp)
NEMIVER_END_NAMESPACE (nemiver)
#endif /*__NMV_CPP_AST_H__*/

