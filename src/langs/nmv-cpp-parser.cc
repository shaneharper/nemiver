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
#include "nmv-cpp-parser.h"
#include "nmv-cpp-lexer.h"
#include "common/nmv-exception.h"

NEMIVER_BEGIN_NAMESPACE (nemiver)
NEMIVER_BEGIN_NAMESPACE (cpp)

struct Parser::Priv {
    Lexer lexer;
    //this indicates whether we are parsing
    //template arguments.
    //this is necessary because the c++ spec says at [14.2.3]:
    //~=~
    //After name lookup (3.4) finds that a name is a template-name, if this name
    //is followed by a <, the < is always taken as the beginning of
    //a template-argument-list and never as a name followed by the less-than
    //operator. When parsing a template-id, the first non-nested > is taken
    //as the end of the template.
    //~=~
    int parsing_template_argument;
    int in_lt_nesting_context;

    Priv () :
        lexer (""),
        parsing_template_argument (0),
        in_lt_nesting_context (0)
    {
    }

    Priv (const string &a_in):
        lexer (a_in),
        parsing_template_argument (0),
        in_lt_nesting_context (0)
    {
    }
};

#define LEXER m_priv->lexer

Parser::Parser (const string &a_in) :
    m_priv (new Priv (a_in))
{
}

Parser::~Parser ()
{
}

/// parse a primary-expression production.
///primary-expression:
///           literal
///           this
///           ( expression )
///           id-expression
bool
Parser::parse_primary_expr (PrimaryExprPtr &a_expr)
{
    bool status=false;
    Token token;
    PrimaryExprPtr result;
    unsigned mark=LEXER.get_token_stream_mark ();

    if (!LEXER.peek_next_token (token)) {
        return false;
    }
    switch (token.get_kind ()) {
        case Token::KEYWORD:
            if (token.get_str_value () == "this") {
                result.reset (new ThisPrimaryExpr);
                LEXER.consume_next_token ();
            } else {
                IDExprPtr expr;
                if (!parse_id_expr (expr)) {goto error;}
                result = expr;
                goto okay;
            }
            break;
        case Token::PUNCTUATOR_PARENTHESIS_OPEN:
            {
                //consume the open parenthesis
                LEXER.consume_next_token ();
                ++m_priv->in_lt_nesting_context;
                ExprPtr expr;
                if (!parse_expr (expr)) {
                    --m_priv->in_lt_nesting_context;
                    goto error;
                }
                result.reset (new ParenthesisPrimaryExpr (expr));
                LEXER.consume_next_token (token);
                if (token.get_kind () != Token::PUNCTUATOR_PARENTHESIS_CLOSE) {
                    --m_priv->in_lt_nesting_context;
                    goto error;
                }
                --m_priv->in_lt_nesting_context;
                goto okay;
            }
            break;
        default:
            if (token.is_literal ()) {
                result.reset (new LiteralPrimaryExpr (token));
                LEXER.consume_next_token ();
                goto okay;
            } else {
                IDExprPtr expr;
                if (!parse_id_expr (expr)) {goto error;}
                result = expr;
                goto okay;
            }
            break;
    }

error:
    status=false;
    LEXER.rewind_to_mark (mark);
    goto out;
okay:
    a_expr = result;
    status=true;
out:
    return status;
}

/// parses a postfix-expression production:
///  postfix-expression:
///            primary-expression
///            postfix-expression [ expression ]
///            postfix-expression ( expression-list(opt) )
///            simple-type-specifier ( expression-list(opt))
///            typename ::(opt) nested-name-specifier identifier ( expression-list(opt) )
///            typename ::(opt) nested-name-specifier template(opt) template-id ...
///                ( expression-listopt )
///            postfix-expression . template(opt) id-expression
///            postfix-expression -> template(opt) id-expression
///            postfix-expression . pseudo-destructor-name
///            postfix-expression -> pseudo-destructor-name
///            postfix-expression ++
///            postfix-expression --
///            dynamic_cast < type-id > ( expression )
///            static_cast < type-id > ( expression )
///            reinterpret_cast < type-id > ( expression )
///            const_cast < type-id > ( expression )
///            typeid ( expression )
///            typeid ( type-id )
///
///TODO: this only parse the first two forms of the postfix-expression. So need
//to fill the gap in here.
bool
Parser::parse_postfix_expr (PostfixExprPtr &a_result)
{
    bool status=false;
    PostfixExprPtr result;
    PostfixExprPtr pfe;
    unsigned mark = LEXER.get_token_stream_mark ();

    PrimaryExprPtr primary_expr;
    if (parse_primary_expr (primary_expr)) {
        result.reset (new PrimaryPFE (primary_expr));
        goto okay;
    }
    if (parse_postfix_expr (pfe)) {
        Token token;
        if (LEXER.peek_next_token (token)
            && token.get_kind () == Token::PUNCTUATOR_BRACKET_OPEN) {
            LEXER.consume_next_token ();
            //try to parse postfix-expression [ expression ]
            ExprPtr expr;
            if (!parse_expr (expr)) {
                goto error;
            }
            if (!LEXER.consume_next_token (token)
                || token.get_kind () != Token::PUNCTUATOR_BRACKET_CLOSE) {
                goto error;
            }
            result.reset (new ArrayPFE (pfe, expr));
            goto okay;
        } else {
            //TODO: handle other types of postfix-expression
        }
    }
    //TODO: handle other types of postfix-expression

error:
    LEXER.rewind_to_mark (mark);
    status=false;
    goto out;
okay:
    a_result=result;
    status = true;
out:
    return status;
}


/// parse a cast-expression production.
///
/// cast-expression:
///            unary-expression
///            ( type-id ) cast-expression
bool
Parser::parse_cast_expr (CastExprPtr &a_result)
{
    bool status=false;
    CastExprPtr result;
    Token token;
    UnaryExprPtr unary_expr;
    unsigned mark=LEXER.get_token_stream_mark ();

    if (!LEXER.peek_next_token (token)) {goto error;}
    if (token.get_kind () == Token::PUNCTUATOR_PARENTHESIS_OPEN) {
        if (!LEXER.consume_next_token ()) {goto error;}
        TypeIDPtr type_id;
        if (parse_type_id (type_id)) {
            if (LEXER.consume_next_token (token)
                && token.get_kind () == Token::PUNCTUATOR_PARENTHESIS_CLOSE) {
                CastExprPtr right_expr;
                if (parse_cast_expr (right_expr)) {
                    result.reset (new CStyleCastExpr (type_id, right_expr));
                    goto okay;
                } else {
                    LEXER.rewind_to_mark (mark);
                }
            } else {
                LEXER.rewind_to_mark (mark);
            }
        } else {
            LEXER.rewind_to_mark (mark);
        }
    }
    if (parse_unary_expr (unary_expr)) {
        result.reset (new UnaryCastExpr (unary_expr));
        goto okay;
    }

error:
    status=false;
    LEXER.rewind_to_mark (mark);
    goto out;
okay:
    status=true;
    a_result=result;
out:
    return status;
}

/// parse a pm-expression production.
///
/// pm-expression:
///           cast-expression
///           pm-expression .* cast-expression
///           pm-expression ->* cast-expression
bool
Parser::parse_pm_expr (PMExprPtr &a_result)
{
    bool status=false;
    CastExprPtr cast_expr, rhs;
    PMExprPtr lhs, result;
    unsigned mark = LEXER.get_token_stream_mark ();
    Token token;

    if (!parse_cast_expr (cast_expr)) {goto error;}
    lhs.reset (new CastPMExpr (cast_expr));

    while (true) {
        if (!LEXER.peek_next_token (token)) {
            result = lhs;
            goto okay;
        }
        if (token.get_kind () == Token::OPERATOR_DOT_STAR
            || token.get_kind () == Token::OPERATOR_ARROW_STAR) {
            LEXER.consume_next_token ();
        } else {
            result = lhs;
            goto okay;
        }
        if (!parse_cast_expr (rhs)) {goto error;}
        if (token.get_kind () == Token::OPERATOR_DOT_STAR) {
            lhs.reset (new DotStarPMExpr (lhs, rhs));
        } else {
            lhs.reset (new ArrowStarPMExpr (lhs, rhs));
        }
    }

error:
    status = false;
    LEXER.rewind_to_mark (mark);
    goto out;
okay:
    status = true;
    a_result = result;
out:
    return status;
}

/// parse a multiplicative-expression production
///
/// multiplicative-expression:
///            pm-expression
///            multiplicative-expression * pm-expression
///            multiplicative-expression / pm-expression
///            multiplicative-expression % pm-expression
bool
Parser::parse_mult_expr (MultExprPtr &a_result)
{
    bool status=false;
    MultExprPtr lhs, result;
    PMExprPtr pm_expr, rhs;
    ExprBase::Operator op=ExprBase::OP_UNDEFINED;
    Token token;
    unsigned mark=LEXER.get_token_stream_mark ();

    if (!parse_pm_expr (pm_expr)) {goto error;}
    lhs.reset (new MultExpr (pm_expr));

    while (true) {
        if (!LEXER.peek_next_token (token)) {
            result = lhs;
            goto okay;
        }
        if (token.get_kind () == Token::OPERATOR_MULT) {
            op = ExprBase::MULT;
        } else if (token.get_kind () == Token::OPERATOR_DIV) {
            op = ExprBase::DIV;
        } else if (token.get_kind () == Token::OPERATOR_MOD) {
            op = ExprBase::MOD;
        } else {
            result = lhs;
            goto okay;
        }
        LEXER.consume_next_token ();//consume the operator token
        if (!parse_pm_expr (rhs)) {goto error;}
        lhs.reset (new MultExpr (lhs, op, rhs));
    }

okay:
    status=true;
    a_result=result;
    goto out;
error:
    status=false;
    LEXER.rewind_to_mark (mark);
out:
    return status;
}

/// parses an additive-expression production
///
/// additive-expression:
///            multiplicative-expression
///            additive-expression + multiplicative-expression
///            additive-expression - multiplicative-expression
bool
Parser::parse_add_expr (AddExprPtr &a_result)
{
    bool status=false;
    AddExprPtr lhs, result;
    MultExprPtr mult_expr, rhs;
    ExprBase::Operator op=ExprBase::OP_UNDEFINED;
    Token token;
    unsigned mark=LEXER.get_token_stream_mark ();

    if (!parse_mult_expr (mult_expr)) {goto error;}
    lhs.reset (new AddExpr (mult_expr));

    while (true) {
        if (!LEXER.peek_next_token (token)) {
            result = lhs;
            goto okay;
        }
        if (token.get_kind () == Token::OPERATOR_PLUS) {
            op = ExprBase::PLUS;
        } else if (token.get_kind () == Token::OPERATOR_MINUS) {
            op = ExprBase::MINUS;
        } else {
            result = lhs;
            goto okay;
        }
        LEXER.consume_next_token ();//consume the operator token
        if (!parse_mult_expr (rhs)) {goto error;}
        lhs.reset (new AddExpr (lhs, op, rhs));
    }

okay:
    status=true;
    a_result=result;
    goto out;
error:
    status=false;
    LEXER.rewind_to_mark (mark);
out:
    return status;
}

/// parse a shift-expression production
///
/// shift-expression:
///            additive-expression
///            shift-expression << additive-expression
///            shift-expression >> additive-expression
bool
Parser::parse_shift_expr (ShiftExprPtr &a_result)
{
    bool status=false;
    ShiftExprPtr lhs, result;
    AddExprPtr add_expr, rhs;
    ExprBase::Operator op=ExprBase::OP_UNDEFINED;
    Token token;
    unsigned mark=LEXER.get_token_stream_mark ();

    if (!parse_add_expr (add_expr)) {goto error;}
    lhs.reset (new ShiftExpr (add_expr));

    while (true) {
        if (!LEXER.peek_next_token (token)) {
            result = lhs;
            goto okay;
        }
        if (token.get_kind () == Token::OPERATOR_BIT_LEFT_SHIFT) {
            op = ExprBase::LEFT_SHIFT;
        } else if (token.get_kind () == Token::OPERATOR_BIT_RIGHT_SHIFT) {
            op = ExprBase::RIGHT_SHIFT;
        } else {
            result = lhs;
            goto okay;
        }
        LEXER.consume_next_token ();//consume the operator token
        if (!parse_add_expr (rhs)) {goto error;}
        lhs.reset (new ShiftExpr (lhs, op, rhs));
    }

okay:
    status=true;
    a_result=result;
    goto out;
error:
    status=false;
    LEXER.rewind_to_mark (mark);
out:
    return status;
}

/// parse a relational-expression production
///
/// relational-expression:
///            shift-expression
///            relational-expression < shift-expression
///            relational-expression > shift-expression
///            relational-expression <= shift-expression
///           relational-expression >= shift-expression
bool
Parser::parse_rel_expr (RelExprPtr &a_result)
{
    bool status=false;
    RelExprPtr lhs, result;
    ShiftExprPtr shift_expr, rhs;
    ExprBase::Operator op=ExprBase::OP_UNDEFINED;
    Token token;
    unsigned mark=LEXER.get_token_stream_mark ();

    if (!parse_shift_expr (shift_expr)) {goto error;}
    lhs.reset (new RelExpr (shift_expr));

    while (true) {
        if (!LEXER.peek_next_token (token)) {
            result = lhs;
            goto okay;
        }
        if (token.get_kind () == Token::OPERATOR_LT) {
            op = ExprBase::LT;
        } else if (token.get_kind () == Token::OPERATOR_GT) {
            if (m_priv->parsing_template_argument)  {
                if (!m_priv->in_lt_nesting_context) {
                    //we did encounter a '>' while parsing a template argument
                    //and without being inside parenthesis.
                    //So we must consider that the '>' is the ending of the
                    //template parameter list. That means we will consider that
                    //parsing of this relational expression ends here and we
                    //actually parsed the shift-expression form of it.
                    //example:
                    //X< 1>2 > x1; --> error;
                    //X<(1>2)> x2; ---> okay.
                    //
                    result = lhs;
                    goto okay;
                }
            }
            op = ExprBase::GT;
        } else if (token.get_kind () == Token::OPERATOR_LT_EQ) {
            op = ExprBase::LT_OR_EQ;
        } else if (token.get_kind () == Token::OPERATOR_GT_EQ) {
            if (m_priv->parsing_template_argument)  {
                if (!m_priv->in_lt_nesting_context) {
                    //we did encounter a '>' while parsing a template argument
                    //and without being inside an lt-nesting context.
                    //So we must consider that the '>' is the ending of the
                    //template parameter list. That means we will consider that
                    //parsing of this relational expression ends here and we
                    //actually parsed the shift-expression form of it.
                    //example:
                    //X< 1>2 > x1; --> error;
                    //X<(1>2)> x2; ---> okay.
                    result = lhs;
                    goto okay;
                }
            }
            op = ExprBase::GT_OR_EQ;
        } else {
            result = lhs;
            goto okay;
        }
        LEXER.consume_next_token ();//consume the operator token
        if (!parse_shift_expr (rhs)) {goto error;}
        lhs.reset (new RelExpr (lhs, op, rhs));
    }

okay:
    status=true;
    a_result=result;
    goto out;
error:
    status=false;
    LEXER.rewind_to_mark (mark);
out:
    return status;
}

/// parse an equality-expression
///
/// equality-expression:
///            relational-expression
///            equality-expression == relational-expression
///            equality-expression != relational-expression
bool
Parser::parse_eq_expr (EqExprPtr &a_result)
{
    bool status=false;
    EqExprPtr lhs, result;
    RelExprPtr rel_expr, rhs;
    ExprBase::Operator op=ExprBase::OP_UNDEFINED;
    Token token;
    unsigned mark=LEXER.get_token_stream_mark ();

    if (!parse_rel_expr (rel_expr)) {goto error;}
    lhs.reset (new EqExpr (rel_expr));

    while (true) {
        if (!LEXER.peek_next_token (token)) {
            result = lhs;
            goto okay;
        }
        if (token.get_kind () == Token::OPERATOR_EQUALS) {
            op = ExprBase::EQUALS;
        } else if (token.get_kind () == Token::OPERATOR_NOT_EQUAL) {
            op = ExprBase::NOT_EQUALS;
        } else {
            result = lhs;
            goto okay;
        }
        LEXER.consume_next_token ();//consume the operator token
        if (!parse_rel_expr (rhs)) {goto error;}
        lhs.reset (new EqExpr (lhs, op, rhs));
    }

okay:
    status=true;
    a_result=result;
    goto out;
error:
    status=false;
    LEXER.rewind_to_mark (mark);
out:
    return status;
}

/// parse an and-expression
///
/// and-expression:
///           equality-expression
///           and-expression & equality-expression
bool
Parser::parse_and_expr (AndExprPtr &a_result)
{
    bool status=false;
    AndExprPtr lhs, result;
    EqExprPtr eq_expr, rhs;
    Token token;
    unsigned mark=LEXER.get_token_stream_mark ();

    if (!parse_eq_expr (eq_expr)) {goto error;}
    lhs.reset (new AndExpr (eq_expr));

    while (true) {
        if (!LEXER.peek_next_token (token)) {
            result = lhs;
            goto okay;
        }
        if (token.get_kind () != Token::OPERATOR_BIT_AND) {
            result = lhs;
            goto okay;
        }
        LEXER.consume_next_token ();//consume the operator token
        if (!parse_eq_expr (rhs)) {goto error;}
        lhs.reset (new AndExpr (lhs, rhs));
    }

okay:
    status=true;
    a_result=result;
    goto out;
error:
    status=false;
    LEXER.rewind_to_mark (mark);
out:
    return status;
}

/// parse exclusive-op-expression production.
///
/// exclusive-or-expression:
///            and-expression
///            exclusive-or-expression ^ and-expression
bool
Parser::parse_xor_expr (XORExprPtr &a_result)
{
    bool status=false;
    XORExprPtr lhs, result;
    AndExprPtr and_expr, rhs;
    Token token;
    unsigned mark=LEXER.get_token_stream_mark ();

    if (!parse_and_expr (and_expr)) {goto error;}
    lhs.reset (new XORExpr (and_expr));

    while (true) {
        if (!LEXER.peek_next_token (token)) {
            result = lhs;
            goto okay;
        }
        if (token.get_kind () != Token::OPERATOR_BIT_XOR) {
            result = lhs;
            goto okay;
        }
        LEXER.consume_next_token ();//consume the operator token
        if (!parse_and_expr (rhs)) {goto error;}
        lhs.reset (new XORExpr (lhs, rhs));
    }

okay:
    status=true;
    a_result=result;
    goto out;
error:
    status=false;
    LEXER.rewind_to_mark (mark);
out:
    return status;
}

/// parse an inclusive-or-expression
///
/// inclusive-or-expression:
///            exclusive-or-expression
///            inclusive-or-expression | exclusive-or-expression
bool
Parser::parse_or_expr (ORExprPtr &a_result)
{
    bool status=false;
    ORExprPtr lhs, result;
    XORExprPtr xor_expr, rhs;
    Token token;
    unsigned mark=LEXER.get_token_stream_mark ();

    if (!parse_xor_expr (xor_expr)) {goto error;}
    lhs.reset (new ORExpr (xor_expr));

    while (true) {
        if (!LEXER.peek_next_token (token)) {
            result = lhs;
            goto okay;
        }
        if (token.get_kind () != Token::OPERATOR_BIT_OR) {
            result = lhs;
            goto okay;
        }
        LEXER.consume_next_token ();//consume the operator token
        if (!parse_xor_expr (rhs)) {goto error;}
        lhs.reset (new ORExpr (lhs, rhs));
    }

okay:
    status=true;
    a_result=result;
    goto out;
error:
    status=false;
    LEXER.rewind_to_mark (mark);
out:
    return status;
}

/// parse a logical-and-expression production
///
/// logical-and-expression:
///            inclusive-or-expression
///            logical-and-expression && inclusive-or-expression
bool
Parser::parse_log_and_expr (LogAndExprPtr &a_result)
{
    bool status=false;
    LogAndExprPtr lhs, result;
    ORExprPtr or_expr, rhs;
    Token token;
    unsigned mark=LEXER.get_token_stream_mark ();

    if (!parse_or_expr (or_expr)) {goto error;}
    lhs.reset (new LogAndExpr (or_expr));

    while (true) {
        if (!LEXER.peek_next_token (token)) {
            result = lhs;
            goto okay;
        }
        if (token.get_kind () != Token::OPERATOR_AND) {
            result = lhs;
            goto okay;
        }
        LEXER.consume_next_token ();//consume the operator token
        if (!parse_or_expr (rhs)) {goto error;}
        lhs.reset (new LogAndExpr (lhs, rhs));
    }

okay:
    status=true;
    a_result=result;
    goto out;
error:
    status=false;
    LEXER.rewind_to_mark (mark);
out:
    return status;
}

/// parse a logical-or-expression
///
/// logical-or-expression:
///            logical-and-expression
///            logical-or-expression || logical-and-expression
bool
Parser::parse_log_or_expr (LogOrExprPtr &a_result)
{
    bool status=false;
    LogOrExprPtr lhs, result;
    LogAndExprPtr log_and_expr, rhs;
    Token token;
    unsigned mark=LEXER.get_token_stream_mark ();

    if (!parse_log_and_expr (log_and_expr)) {goto error;}
    lhs.reset (new LogOrExpr (log_and_expr));

    while (true) {
        if (!LEXER.peek_next_token (token)) {
            result = lhs;
            goto okay;
        }
        if (token.get_kind () != Token::OPERATOR_OR) {
            result = lhs;
            goto okay;
        }
        LEXER.consume_next_token ();//consume the operator token
        if (!parse_log_and_expr (rhs)) {goto error;}
        lhs.reset (new LogOrExpr (lhs, rhs));
    }

okay:
    status=true;
    a_result=result;
    goto out;
error:
    status=false;
    LEXER.rewind_to_mark (mark);
out:
    return status;
}

/// parse a conditional-expression
///
/// conditional-expression:
///            logical-or-expression
///            logical-or-expression ? expression : assignment-expression
///
bool
Parser::parse_cond_expr (CondExprPtr &a_result)
{
    bool status=false;
    Token token;
    CondExprPtr result;
    LogOrExprPtr cond;
    ExprPtr then_branch;
    AssignExprPtr else_branch;
    unsigned mark=LEXER.get_token_stream_mark ();

    if (!parse_log_or_expr (cond)) {goto error;}
    if (!LEXER.peek_next_token (token)
        || token.get_kind () != Token::PUNCTUATOR_QUESTION_MARK) {
        result.reset (new CondExpr (cond));
        goto okay;
    }
    LEXER.consume_next_token ();//consume the '?'
    if (!parse_expr (then_branch)) {goto error;}
    if (!LEXER.consume_next_token (token)
        || token.get_kind () != Token::PUNCTUATOR_COLON) {
        goto error;
    }
    if (!parse_assign_expr (else_branch) || !else_branch) {goto error;}
    result.reset (new CondExpr (cond, then_branch, else_branch));

okay:
    status=true;
    a_result=result;
    goto out;
error:
    status=false;
    LEXER.rewind_to_mark (mark);
out:
    return status;
}

/// parse an assignment-expression production
///
/// assignment-expression:
///           conditional-expression
///           logical-or-expression assignment-operator assignment-expression
///           throw-expression
/// TODO: parse the throw-expression form
bool
Parser::parse_assign_expr (AssignExprPtr &a_result)
{
    bool status=false;
    Token token;
    AssignExprPtr result, rhs;
    CondExprPtr cond_expr;
    LogOrExprPtr lhs;
    ExprBase::Operator op=ExprBase::OP_UNDEFINED;
    unsigned mark=LEXER.get_token_stream_mark ();

    //TODO: parse throw-expression here.
    if (parse_log_or_expr (lhs) && lhs) {
        if (LEXER.consume_next_token (token)) {
            switch (token.get_kind ()) {
                case Token::OPERATOR_ASSIGN:
                    op = ExprBase::ASSIGN;
                    break;
                case Token::OPERATOR_MULT_EQ:
                    op = ExprBase::MULT_EQ;
                    break;
                case Token::OPERATOR_DIV_EQ:
                    op = ExprBase::DIV_EQ;
                    break;
                case Token::OPERATOR_MOD_EQ:
                    op = ExprBase::MOD_EQ;
                    break;
                case Token::OPERATOR_PLUS_EQ:
                    op = ExprBase::PLUS_EQ;
                    break;
                case Token::OPERATOR_MINUS_EQ:
                    op = ExprBase::MINUS_EQ;
                    break;
                case Token::OPERATOR_BIT_RIGHT_SHIFT:
                    op = ExprBase::RIGHT_SHIFT_EQ;
                    break;
                case Token::OPERATOR_BIT_LEFT_SHIFT:
                    op = ExprBase::LEFT_SHIFT_EQ;
                    break;
                case Token::OPERATOR_BIT_AND_EQ:
                    op = ExprBase::AND_EQ;
                    break;
                case Token::OPERATOR_BIT_XOR_EQ:
                    op = ExprBase::XOR_EQ;
                    break;
                case Token::OPERATOR_BIT_OR_EQ:
                    op = ExprBase::OR_EQ;
                    break;
                default:
                    LEXER.rewind_to_mark (mark);
                    goto condexpr;
                    break;
            }
            if (parse_assign_expr (rhs)) {
                result.reset (new FullAssignExpr (lhs, op, rhs));
                goto okay;
            } else {
                LEXER.rewind_to_mark (mark);
            }
        } else {
            LEXER.rewind_to_mark (mark);
        }
    }
condexpr:
    if (parse_cond_expr (cond_expr) && cond_expr) {
        result.reset (new CondAssignExpr (cond_expr));
        goto okay;
    }
    goto error;

okay:
    status=true;
    a_result=result;
    goto out;
error:
    status=false;
    LEXER.rewind_to_mark (mark);
out:
    return status;
}

/// parse an expression production.
///
/// expression:
///            assignment-expression
///            expression , assignment-expression
bool
Parser::parse_expr (ExprPtr &a_result)
{
    bool status=false;
    Token token;
    list<AssignExprPtr> assignments;
    ExprPtr result;
    AssignExprPtr assign_expr;
    unsigned mark=LEXER.get_token_stream_mark ();

    if (!parse_assign_expr (assign_expr)) {goto error;}
    assignments.push_back (assign_expr);

    while (true) {
        if (!LEXER.peek_next_token (token)
            || token.get_kind () != Token::OPERATOR_SEQ_EVAL) {
            result.reset (new Expr (assignments));
            goto okay;
        }
        LEXER.consume_next_token ();//consume the ','
        if (!parse_assign_expr (assign_expr)) {goto error;}
        assignments.push_back (assign_expr);
    }

okay:
    status=true;
    a_result=result;
    goto out;
error:
    status=false;
    LEXER.rewind_to_mark (mark);
out:
    return status;
}

/// parse a constant-expression
///
///constant-expression:
///           conditional-expression
bool
Parser::parse_const_expr (ConstExprPtr &a_result)
{
    CondExprPtr cond_expr;

    if (parse_cond_expr (cond_expr)) {
        a_result.reset (new ConstExpr (cond_expr));
        return true;
    }
    return false;
}

/// parses a unary-expression production.
///
/// unary-expression:
///          postfix-expression
///           ++ cast-expression
///           -- cast-expression
///           unary-operator cast-expression
///           sizeof unary-expression
///           sizeof ( type-id )
///           new-expression
///           delete-expression
///TODO: this only support the postfix-expression form parsing.
///      need to support the other forms
bool
Parser::parse_unary_expr (UnaryExprPtr &a_result)
{
    PostfixExprPtr pfe;

    if (parse_postfix_expr (pfe)) {
        a_result.reset (new PFEUnaryExpr (pfe));
        return true;
    }
    return false;
}


/// parses a class-or-namespace-name production.
///
/// class-or-namespace-name:
///           class-name
///           namespace-name
/// with,
/// class-name:
///           identifier
///           template-id
///  and,
//
/// namespace-name:
///           original-namespace-name
///           namespace-alias
/// and,
//
/// original-namespace-name:
///           identifier
///
/// namespace-alias:
///           identifier
bool
Parser::parse_class_or_namespace_name (UnqualifiedIDExprPtr &a_result)
{
    Token token;
    if (!LEXER.peek_next_token (token)) {return false;}

    if (token.get_kind () == Token::IDENTIFIER) {
        TemplateIDPtr template_id;
        if (parse_template_id (template_id)) {
            a_result.reset (new UnqualifiedTemplateID (template_id));
        } else {
            a_result.reset (new UnqualifiedID (token.get_str_value ()));
            LEXER.consume_next_token ();
        }
    } else {
        return false;
    }
    return true;
}

/// parse template-id production
///
/// template-id:
///           template-name < template-argument-listopt >
bool
Parser::parse_template_id (TemplateIDPtr &a_result)
{
    Token token;
    TemplateIDPtr result;
    string name;
    list<TemplateArgPtr> args;
    unsigned mark = LEXER.get_token_stream_mark ();

    if (!LEXER.peek_next_token (token)
        || token.get_kind () != Token::IDENTIFIER) {
        goto error;
    }
    LEXER.consume_next_token ();
    name = token.get_str_value ();
    if (!LEXER.consume_next_token (token)
        || token.get_kind () != Token::OPERATOR_LT) {
        goto error;
    }
    if (!parse_template_argument_list (args)) {
        goto error;
    }
    if (!LEXER.consume_next_token (token)
        || token.get_kind () != Token::OPERATOR_GT) {
        goto error;
    }
    a_result.reset (new TemplateID (name, args));
    return true;

error:
    LEXER.rewind_to_mark (mark);
    return false;
}

/// parse a type-name production.
///
/// type-name:
///           class-name
///           enum-name
///           typedef-name
/// with:
///
/// class-name:
///           identifier
///           template-id
///
/// enum-name:
///      identifier
///
/// typedef-name:
///      identifier
bool
Parser::parse_type_name (UnqualifiedIDExprPtr &a_result)
{
    UnqualifiedIDExprPtr result;
    Token token;

    if (!LEXER.peek_next_token (token)) {return false;}

    if (token.get_kind () == Token::IDENTIFIER) {
        TemplateIDPtr template_id;
        if (parse_template_id (template_id)) {
            a_result.reset (new UnqualifiedTemplateID (template_id));
            return true;
        }
        if (!LEXER.consume_next_token ()) {return false;}
        a_result.reset (new UnqualifiedID (token.get_str_value ()));
        return true;
    }
    return false;
}

/// parse nested-name-specifier production.
///
/// nested-name-specifier:
///          class-or-namespace-name :: nested-name-specifier(opt)
///          class-or-namespace-name :: 'template' nested-name-specifier
bool
Parser::parse_nested_name_specifier (QNamePtr &a_result)
{
    bool result=false;
    string specifier, specifier2;
    QNamePtr qname, qname2;
    Token token;
    UnqualifiedIDExprPtr id;
    unsigned mark = LEXER.get_token_stream_mark ();

    if (!parse_class_or_namespace_name (id)) {goto error;}
    qname.reset (new QName);
    qname->append (id);
    if (!LEXER.consume_next_token (token)) {goto error;}
    if (token.get_kind () != Token::OPERATOR_SCOPE_RESOL) {goto error;}
    if (parse_nested_name_specifier (qname2)) {
        qname->append (qname2);
    } else {
        if (LEXER.peek_next_token (token)
            && token.get_kind () == Token::KEYWORD
            && token.get_str_value () == "template") {
            if (!LEXER.consume_next_token (token)) {goto error;}
            if (parse_nested_name_specifier (qname2)) {
                qname->append (qname2, true);
            } else {
                goto error;
            }
        }
    }
    a_result = qname;
    result=true;
    goto out;

error:
    LEXER.rewind_to_mark (mark);
out:
    return result;
}

/// parse an unqualified-id production.
///
/// unqualified-id:
///   identifier
///   operator-function-id
///   conversion-function-id
///   ~class-name
///   template-id
///
/// TODO: support conversion-function-id cases.
bool
Parser::parse_unqualified_id (UnqualifiedIDExprPtr &a_result)
{
    bool status = false;
    UnqualifiedIDExprPtr result;
    unsigned mark = LEXER.get_token_stream_mark ();
    Token token;
    if (!LEXER.peek_next_token (token)) {goto error;}

    switch (token.get_kind ()) {
        case Token::IDENTIFIER: {
            TemplateIDPtr template_id;
            if (parse_template_id (template_id)) {
                result.reset (new UnqualifiedTemplateID (template_id));
                goto okay;
            }
            if (!LEXER.consume_next_token ()) {goto error;}
            result.reset (new UnqualifiedID (token.get_str_value ()));
            goto okay;
        }
            break;//this is useless, but I keep it.
        case Token::KEYWORD:
            //operator-function-id:
            // 'operator' operator
            if (!LEXER.consume_next_token ()) {goto error;}
            if (token.get_str_value () == "operator") {
                if (!LEXER.peek_next_token (token)) {goto error;}
                if (!token.is_operator ()) {goto error;}
                if (!LEXER.consume_next_token ()) {goto error;}
                result.reset (new UnqualifiedOpFuncID (token));
                goto okay;
            } else {
                result.reset (new UnqualifiedID (token.get_str_value ()));
                goto okay;
            }
            break;
        case Token::OPERATOR_BIT_COMPLEMENT:
            {
                // ~class-name, with
                // class-name:
                //   identifier
                //   template-id
                if (!LEXER.consume_next_token ()) {goto error;}
                UnqualifiedIDExprPtr class_name;
                if (parse_type_name (class_name)) {
                    result.reset (new DestructorID (class_name));
                    goto okay;
                } else {
                    goto error;
                }
            }
            break;
        default:
            //TODO: support conversion-function-id
            break;
    }
    goto error;

okay:
    status = true;
    a_result = result;
    goto out;
error:
    LEXER.rewind_to_mark (mark);
    status = false;
out:
    return status;
}

/// parse a qualified-id production.
///
/// qualified-id:
///   ::(opt) nested-name-specifier 'template'(opt) unqualified-id
///   :: identifier
///   :: operator-function-id
///   :: template-id
bool
Parser::parse_qualified_id (QualifiedIDExprPtr &a_expr)
{
    bool result=false, has_template=false;
    UnqualifiedIDExprPtr id;
    Token token;
    QNamePtr scope;
    unsigned mark = LEXER.get_token_stream_mark ();

    if (!LEXER.peek_next_token (token)) {return false;}
    QualifiedIDExprPtr expr;

    if (token.get_kind () == Token::OPERATOR_SCOPE_RESOL) {
        LEXER.consume_next_token ();
    }

    if (parse_nested_name_specifier (scope)) {
        if (LEXER.peek_next_token (token)
            && token.get_kind () == Token::KEYWORD
            && token.get_str_value () == "template") {
            LEXER.consume_next_token ();
            has_template=true;
        }
        if (!parse_unqualified_id (id)) {goto error;}
        expr.reset (new QualifiedIDExpr (scope, id));
        goto okay;
    }
    if (token.get_kind () != Token::OPERATOR_SCOPE_RESOL) {
        goto error;
    }
    if (!LEXER.consume_next_token (token)) {goto error;}

    if (parse_unqualified_id (id)) {
        expr.reset (new QualifiedIDExpr (scope, id));
    } else {
        goto error;
    }

okay:
    a_expr=expr;
    result = true;
    goto out;
error:
    LEXER.rewind_to_mark (mark);
out:
    return result;
}

/// parse an id-expression production.
///
/// id-expression:
///            unqualified-id
///            qualified-id
bool
Parser::parse_id_expr (IDExprPtr &a_expr)
{
    bool is_okay=false;
    Token token;

    if (!LEXER.peek_next_token (token)) {return false;}
    switch (token.get_kind ()) {
        case Token::KEYWORD:
        case Token::OPERATOR_BIT_COMPLEMENT: {
            UnqualifiedIDExprPtr unq_expr;
            is_okay = parse_unqualified_id (unq_expr);
            if (is_okay) {
                a_expr = unq_expr;
            }
            return is_okay;
        }
        case Token::OPERATOR_SCOPE_RESOL: {
            QualifiedIDExprPtr q_expr;
            is_okay = parse_qualified_id (q_expr);
            if (is_okay) {
                a_expr = q_expr;
            }
            return is_okay;
        }
        case Token::IDENTIFIER: {
            UnqualifiedIDExprPtr unq_expr;
            QualifiedIDExprPtr q_expr;
            if (parse_qualified_id (q_expr)) {
                a_expr = q_expr;
                is_okay = true;
            } else {
                if (parse_unqualified_id (unq_expr)) {
                    a_expr = unq_expr;
                    is_okay = true;
                }
            }
            return is_okay;
        }
        default:
            break;
    }
    return false;
}

/// parse the elaborated-type-specifier production.
///
/// elaborated-type-specifier:
///           class-key ::(opt) nested-name-specifier(opt) identifier
///           enum ::(opt) nested-name-specifier(opt) identifier
///           typename ::(opt) nested-name-specifier identifier
///           typename ::(opt) nested-name-specifier template(opt) template-id
/// TODO: handle the template-id form.
bool
Parser::parse_elaborated_type_specifier (ElaboratedTypeSpecPtr &a_result)
{
    bool status=false;
    ElaboratedTypeSpecPtr result;
    Token token;
    list<ElaboratedTypeSpec::ElemPtr> elems;
    ElaboratedTypeSpec::ElemPtr elem;
    unsigned mark = LEXER.get_token_stream_mark ();

    if (!LEXER.consume_next_token (token)) {goto error;}

    if (token.get_kind () == Token::KEYWORD) {
        if (token.get_str_value () == "class") {
            elem.reset (new ElaboratedTypeSpec::ClassElem);
            elems.push_back (elem);
        } else if (token.get_str_value () == "struct") {
            elem.reset (new ElaboratedTypeSpec::StructElem);
            elems.push_back (elem);
        } else if (token.get_str_value () == "union") {
            elem.reset (new ElaboratedTypeSpec::UnionElem);
            elems.push_back (elem);
        } else {
            goto error;
        }
        if (!LEXER.peek_next_token (token)) {goto error;}
        if (token.get_kind () == Token::OPERATOR_SCOPE_RESOL) {
            //TODO: add empty scope here.
            if (!LEXER.consume_next_token ()) {goto error;}
        }
        QNamePtr scope;
        parse_nested_name_specifier (scope);
        if (scope) {
            elem.reset (new ElaboratedTypeSpec::ScopeElem (scope));
            elems.push_back (elem);
        }
        if (!LEXER.consume_next_token (token)) {goto error;}
        if (token.get_kind () != Token::IDENTIFIER) {goto error;}
        elem.reset (new ElaboratedTypeSpec::IdentifierElem (token.get_str_value ()));
        elems.push_back (elem);
        result.reset (new ElaboratedTypeSpec (elems));
        goto okay;
    } else if (token.get_kind () == Token::KEYWORD
               && token.get_str_value () == "enum") {
        elem.reset (new ElaboratedTypeSpec::EnumElem);
        elems.push_back (elem);
        if (!LEXER.peek_next_token (token)) {goto error;}
        if (token.get_kind () == Token::OPERATOR_SCOPE_RESOL) {
            //TODO: add empty scope here.
            LEXER.consume_next_token ();
        }
        QNamePtr scope;
        if (parse_nested_name_specifier (scope)) {
            elem.reset (new ElaboratedTypeSpec::ScopeElem (scope));
            elems.push_back (elem);
        }
        if (!LEXER.consume_next_token (token)) {goto error;}
        if (token.get_kind () != Token::IDENTIFIER) {goto error;}
        elem.reset (new ElaboratedTypeSpec::IdentifierElem (token.get_str_value ()));
        elems.push_back (elem);
        result.reset (new ElaboratedTypeSpec (elems));
        goto okay;
    } else if (token.get_kind () == Token::KEYWORD
               && token.get_str_value () == "typename") {
        elem.reset (new ElaboratedTypeSpec::TypenameElem);
        elems.push_back (elem);
        if (!LEXER.peek_next_token (token)) {goto error;}
        if (token.get_kind () == Token::OPERATOR_SCOPE_RESOL) {
            LEXER.consume_next_token ();
            //TODO: add empty scope here.
        }
        QNamePtr scope;
        if (!parse_nested_name_specifier (scope) || !scope) {goto error;}
        elem.reset (new ElaboratedTypeSpec::ScopeElem (scope));
        elems.push_back (elem);
        if (!LEXER.peek_next_token (token)) {goto error;}
        if (token.get_kind () == Token::IDENTIFIER) {
            LEXER.consume_next_token ();
            elem.reset (new ElaboratedTypeSpec::IdentifierElem (token.get_str_value ()));
            elems.push_back (elem);
            result.reset (new ElaboratedTypeSpec (elems));
            goto okay;
        }
        if (token.get_kind () == Token::KEYWORD
            && token.get_str_value () == "template"){
            LEXER.consume_next_token ();
            elem.reset (new ElaboratedTypeSpec::ScopeElem (scope));
            elems.push_back (elem);
        }
        //TODO: handle template id
        //string template_id;
        //if (!parse_template_id (template_id)) {goto error;}
        //str += template_id;
        //goto okay;
        goto error;
    } else {
        goto error;
    }

okay:
    status = true;
    a_result = result;
    goto out;
error:
    LEXER.rewind_to_mark (mark);
out:
    return status;

}

/// parse a simple-type-specifier production
///
/// simple-type-specifier:
///             ::(opt) nested-name-specifier(opt) type-name
///             ::(opt) nested-name-specifier template template-id
///             char
///             wchar_t
///             bool
///             short
///             int
///             long
///             signed
///             unsigned
///             float
///             double
///             void
/// TODO: support template-id part
bool
Parser::parse_simple_type_specifier (SimpleTypeSpecPtr &a_str)
{
    bool status=false;
    string str;
    SimpleTypeSpecPtr result;
    Token token;
    QNamePtr scope;
    UnqualifiedIDExprPtr type_name;

    unsigned mark = LEXER.get_token_stream_mark ();

    if (!LEXER.peek_next_token (token)) {goto error;}

    if ((token.get_kind () == Token::KEYWORD)
        && (token.get_str_value () == "char"
            || token.get_str_value () == "wchar_t"
            || token.get_str_value () == "bool"
            || token.get_str_value () == "short"
            || token.get_str_value () == "int"
            || token.get_str_value () == "long"
            || token.get_str_value () == "signed"
            || token.get_str_value () == "unsigned"
            || token.get_str_value () == "float"
            || token.get_str_value () == "double"
            || token.get_str_value () == "void")) {
        LEXER.consume_next_token ();
        result.reset (new SimpleTypeSpec (scope, token.get_str_value ()));
        goto okay;
    }

    if (token.get_kind () == Token::OPERATOR_SCOPE_RESOL) {
        LEXER.consume_next_token ();
        //TODO: add nil scope;
    }
    if (parse_nested_name_specifier (scope) && scope) {
        if (LEXER.peek_next_token (token)
            && token.get_kind () == Token::KEYWORD
            && token.get_str_value () == "template") {
            if (!LEXER.consume_next_token ()) {goto error;}
            TemplateIDPtr template_id;
            if (!parse_template_id (template_id)) {goto error;}
            UnqualifiedIDExprPtr id (new UnqualifiedTemplateID (template_id));
            result.reset (new SimpleTypeSpec (scope,id));
            goto okay;
        }
    }
    if (!parse_type_name (type_name) || !type_name) {goto error;}
    type_name->to_string (str);
    result.reset (new SimpleTypeSpec (scope, str));

okay:
    status = true;
    a_str = result;
    goto out;
error:
    LEXER.rewind_to_mark (mark);
out:
    return status;
}

/// parse a type-specifier production
///
/// type-specifier:
///            simple-type-specifier
///            class-specifier
///            enum-specifier
///            elaborated-type-specifier
///            cv-qualifier
/// TODO: handle class-specifier and enum specifier
bool
Parser::parse_type_specifier (TypeSpecifierPtr &a_result)
{
    string str;
    TypeSpecifierPtr result ;
    SimpleTypeSpecPtr type_spec;
    ElaboratedTypeSpecPtr type_spec2;
    Token token;
    bool is_ok=false;
    unsigned mark = LEXER.get_token_stream_mark ();

    if (parse_simple_type_specifier (type_spec)) {
        result = type_spec;
        goto okay;
    }
    if (parse_elaborated_type_specifier (type_spec2)) {
        result = type_spec2;
        goto okay;
    }
    if (LEXER.consume_next_token (token)
        && token.get_kind () == Token::KEYWORD) {
        if (token.get_str_value () == "const") {
            result.reset (new ConstTypeSpec);
            goto okay;
        } else if (token.get_str_value () == "volatile") {
            result.reset (new VolatileTypeSpec);
            goto okay;
        } else {
            goto error;
        }
    }
    // TODO: handle class-specifier and enum specifier

error:
    LEXER.rewind_to_mark (mark);
    is_ok = false;
    goto out;
okay:
    a_result = result;
    is_ok = true;
out:
    return is_ok;
}

/// parse a type-specifier-seq production.
///
/// type-specifier-seq:
///            type-specifier type-specifier-seq(opt)
///
bool
Parser::parse_type_specifier_seq (list<TypeSpecifierPtr> &a_result)
{
    TypeSpecifierPtr type_spec;

    if (!parse_type_specifier (type_spec)) {return false;}
    a_result.push_back (type_spec);
    while (parse_type_specifier (type_spec)) {
        a_result.push_back (type_spec);
    }
    return true;
}

/// parse a template-argument production
///
/// template-argument:
///           assignment-expression
///           type-id
///           id-expression
bool
Parser::parse_template_argument (TemplateArgPtr &a_result)
{

    bool is_okay = false;
    m_priv->parsing_template_argument++;
    AssignExprPtr assign_expr;
    IDExprPtr id_expr;
    TypeIDPtr type_id_expr;
    if (parse_assign_expr (assign_expr)) {
        a_result.reset (new AssignExprTemplArg (assign_expr));
        is_okay = true;
        goto out;
    }
    if (parse_type_id (type_id_expr)) {
        a_result.reset (new TypeIDTemplArg (type_id_expr));
        is_okay = true;
        goto out;
    }
    if (parse_id_expr (id_expr)) {
        a_result.reset (new IDExprTemplArg (id_expr));
        is_okay = true;
        goto out;
    }

out:
    m_priv->parsing_template_argument--;
    return is_okay;
}

/// parse a template-argument-list production
///
/// template-argument-list:
///           template-argument
///           template-argument-list , template-argument
bool
Parser::parse_template_argument_list (list<TemplateArgPtr> &a_result)
{
    Token token;
    bool status = false;
    TemplateArgPtr arg;
    list<TemplateArgPtr> result;
    unsigned mark = LEXER.get_token_stream_mark ();

    if (parse_template_argument (arg)) {
        result.push_back (arg);
    } else {
        goto error;
    }
    while (true) {
        if (!LEXER.peek_next_token (token)) {break;}
        if (token.get_kind () != Token::OPERATOR_SEQ_EVAL) {break;}
        if (!LEXER.consume_next_token ()) {break;}
        if (!parse_template_argument (arg)) {goto error;}
        result.push_back (arg);
    }

    status = true;
    a_result = result;
    goto out;
error:
    status = false;
    LEXER.rewind_to_mark (mark);
out:
    return status;
}

/// parse a type-id production
///
/// type-id:
///         type-specifier-seq abstract-declarator(opt)
///TODO: handle abstract-declarator
bool
Parser::parse_type_id (TypeIDPtr &a_result)
{
    list<TypeSpecifierPtr> type_specs;

    if (!parse_type_specifier_seq (type_specs)) {return false;}
    a_result.reset (new TypeID (type_specs));
    //TODO:handle abstract-declarator here.
    return true;
}

/// parse a decl-specifier production
///
///decl-specifier:
///            storage-class-specifier
///            type-specifier
///            function-specifier
///            friend
///            typedef
///
/// TODO: handle function-specifier
bool
Parser::parse_decl_specifier (DeclSpecifierPtr &a_result)
{
    bool status=false;
    Token token;
    TypeSpecifierPtr type_spec;
    DeclSpecifierPtr result;

    unsigned mark = LEXER.get_token_stream_mark ();

    if (!LEXER.peek_next_token (token)) {goto error;}

    if (token.get_kind () == Token::KEYWORD) {
        if (token.get_str_value () == "auto") {
            result.reset (new AutoSpecifier);
        } else if (token.get_str_value () == "register") {
            result.reset (new RegisterSpecifier);
        } else if (token.get_str_value () == "static") {
            result.reset (new StaticSpecifier);
            result->set_kind (DeclSpecifier::STATIC);
        } else if (token.get_str_value () == "extern") {
            result.reset (new ExternSpecifier);
        } else if (token.get_str_value () == "mutable") {
            result.reset (new MutableSpecifier);
        } else if (token.get_str_value () == "friend") {
            result.reset (new FriendSpecifier);
        } else if (token.get_str_value () == "typedef") {
            result.reset (new TypedefSpecifier);
        } else {
            if (!parse_type_specifier (type_spec)) {
                goto error;
            }
            result = type_spec;
            goto okay;
        }
        LEXER.consume_next_token ();
        if (!result) {goto error;}
        goto okay;
    }
    if (parse_type_specifier (type_spec)) {
        result = type_spec;
        goto okay;
    }
    //TODO: handle function-specifier

error:
    status=false;
    goto out;
    LEXER.rewind_to_mark (mark);

okay:
    status = true;
    a_result=result;

out:
    return status;
}

/// parse a decl-specifier-seq production.
///
/// decl-specifier-seq:
///            decl-specifier-seq(opt) decl-specifier
///
/// [dcl.type]
///
/// As a general rule, at most one type-specifier is
///  allowed in the complete decl-specifier-seq of a declaration.
/// The only exceptions to this rule are the following:
/// — const or volatile can be combined with any other type-specifier.
///   However, redundant cv- qualifiers are prohibited except
///   when introduced through the use of typedefs (7.1.3) or template type
///   arguments (14.3), in which case the redundant cv-qualifiers are ignored.
/// — signed or unsigned can be combined with char, long, short, or int.
/// — short or long can be combined with int.
/// — long can be combined with double.
/// TODO: finish this.
bool
Parser::parse_decl_specifier_seq (list<DeclSpecifierPtr> &a_result)
{
    string str;
    list<DeclSpecifierPtr> result;
    DeclSpecifierPtr decl, decl2;
    bool is_ok=false;
    unsigned mark = LEXER.get_token_stream_mark ();
    Token token;
    int nb_type_specifiers=0;

    //TODO:REMIND: make sure only one type-specifier is parsed at most, except
    //for the special cases.

    unsigned mark2=0;
    while (true ) {
        mark2=LEXER.get_token_stream_mark ();
        if (!parse_decl_specifier (decl) || !decl) {
            break;
        }
        //type specifier special cases
        if (decl->get_kind () == DeclSpecifier::TYPE) {
            if (nb_type_specifiers) {
                LEXER.rewind_to_mark (mark2);
                break;
            }
            TypeSpecifierPtr type_specifier =
                std::tr1::static_pointer_cast<TypeSpecifier> (decl);
            //— const or volatile can be combined with any other type-specifier.
            //  However, redundant cv-qualifiers are prohibited except
            if (type_specifier->is_cv_qualifier ()) {
                result.push_back (type_specifier);
                if (!LEXER.peek_next_token (token)) {break;}
                if (type_specifier->get_kind () == TypeSpecifier::CONST
                    && token.get_kind () == Token::KEYWORD
                    && token.get_str_value () == "const") {
                    goto error;
                } else if (type_specifier->get_kind () == TypeSpecifier::VOLATILE
                           && token.get_kind () == Token::KEYWORD
                           && token.get_str_value () == "volatile") {
                    goto error;
                }
            } else if (type_specifier->get_kind () == TypeSpecifier::SIMPLE) {
                //— signed or unsigned can be combined with char, long, short, or int.
                //— short or long can be combined with int.
                //— long can be combined with double.
                //TODO: handle this
                SimpleTypeSpecPtr simple_type_spec =
                    std::tr1::static_pointer_cast<SimpleTypeSpec> (type_specifier);

                result.push_back (simple_type_spec);
                string type_name_str = simple_type_spec->get_name_as_string ();
                if (type_name_str == "signed"
                    || type_name_str == "unsigned") {
                    if (!LEXER.peek_next_token (token)) {break;}
                    if (token.get_kind () == Token::KEYWORD
                        && (token.get_str_value () ==    "char"
                            || token.get_str_value () == "long"
                            || token.get_str_value () == "short"
                            || token.get_str_value () == "int")) {
                        if (!parse_decl_specifier (decl) || !decl) {goto error;}
                        result.push_back (decl);
                        nb_type_specifiers++;
                        continue;
                    }
                } else if (type_name_str == "short"
                           || type_name_str == "long") {
                    if (!LEXER.peek_next_token (token)) {break;}
                    if (token.get_kind () == Token::KEYWORD
                        && (token.get_str_value () == "int"
                            || token.get_str_value () == "unsigned"
                            || token.get_str_value () == "signed")) {
                        if (!parse_decl_specifier (decl) || !decl) {goto error;}
                        result.push_back (decl);
                        if (token.get_str_value () != "unsigned"
                            && token.get_str_value () != "signed") {
                            nb_type_specifiers++;
                        }
                    } else if (type_name_str == "long"
                               && token.get_kind () == Token::KEYWORD
                               && token.get_str_value () == "double") {
                        if (!parse_decl_specifier (decl) || !decl) {goto error;}
                        result.push_back (decl);
                        nb_type_specifiers++;
                    } else {
                        nb_type_specifiers++;
                    }
                } else {
                    nb_type_specifiers++;
                }
            } else {
                result.push_back (decl);
                nb_type_specifiers++;
            }
        } else {
            result.push_back (decl);
        }
    }
    if (result.empty ())
        goto error;

    a_result = result;
    is_ok=true;
    goto out;
error:
    LEXER.rewind_to_mark (mark);
    is_ok =false;
out:
    return is_ok;
}

///
///
/// declarator-id:
///            id-expression
///            ::(opt) nested-name-specifier(opt) type-name
bool
Parser::parse_declarator_id (IDDeclaratorPtr &a_result)
{
    bool status=false;
    string str;
    IDDeclaratorPtr result;
    IDExprPtr id_expr;
    UnqualifiedIDExprPtr type_name;
    Token token;
    QNamePtr scope;
    unsigned mark = LEXER.get_token_stream_mark ();

    if (parse_id_expr (id_expr)) {
        result.reset (new IDDeclarator (id_expr));
        if (!result) {
            goto error;
        } else {
            goto okay;
        }
    }
    if (!LEXER.peek_next_token (token)) {goto error;}
    if (token.get_kind () == Token::OPERATOR_SCOPE_RESOL) {
        LEXER.consume_next_token ();
    }
    parse_nested_name_specifier (scope) ;
    if (parse_type_name (type_name)) {
        IDExprPtr id_expr (new QualifiedIDExpr (scope, type_name));
        result.reset (new IDDeclarator (id_expr));
        goto okay;
    }

error:
    LEXER.rewind_to_mark (mark);
    status = false;
    goto out;
okay:
    a_result = result;
    status = true;
out:
    return status;
}

/// parse a direct-declarator production.
///
///direct-declarator:
///            declarator-id
///            direct-declarator ( parameter-declaration-clause ) cv-qualifier-seq(opt)...
///               exception-specification(opt)
///            direct-declarator [ constant-expression(opt) ]
///            ( declarator )
/// TODO: handle function style direct-declarator
/// TODO: handle subscript style direct-declarator
/// TODO: handle function pointer style direct declarator
bool
Parser::parse_direct_declarator (DeclaratorPtr &a_result)
{
    bool status=false;
    DeclaratorPtr result;
    IDDeclaratorPtr id;
    Token token;
    unsigned mark = LEXER.get_token_stream_mark ();

    if (parse_declarator_id (id)) {
        if (LEXER.peek_next_token (token)
            && token.get_kind () == Token::PUNCTUATOR_BRACKET_OPEN) {
            LEXER.consume_next_token ();
            if (LEXER.peek_next_token (token)
                && token.get_kind () == Token::PUNCTUATOR_BRACKET_CLOSE) {
                LEXER.consume_next_token ();
                result.reset (new ArrayDeclarator (id));
                goto okay;
            }
            ConstExprPtr const_expr;
            if (parse_const_expr (const_expr)) {
                if (LEXER.consume_next_token (token)
                    && token.get_kind () == Token::PUNCTUATOR_BRACKET_CLOSE) {
                    result.reset (new ArrayDeclarator (id, const_expr));
                    goto okay;
                } else {
                    goto error;
                }
            } else {
                goto error;
            }
        }//TODO: handle other forms of direct-declarators
        result = id;
        goto okay;
    }

    //TODO: handle function style direct-declarator
    //TODO: handle subscript style direct-declarator
    //TODO: handle function pointer style direct declarator

error:
    LEXER.rewind_to_mark (mark);
    status = false;
    goto out;

okay:
    status = true;
    a_result = result;

out:
    return status;
}

bool
Parser::parse_cv_qualifier (CVQualifierPtr &a_result)
{
    bool status=false;
    Token token;
    CVQualifierPtr result;

    if (!LEXER.peek_next_token (token)) {goto error;}

    if (token.get_kind () == Token::KEYWORD) {
        if (token.get_str_value () == "const") {
            result.reset (new ConstQualifier);
        } else if (token.get_str_value () == "volatile") {
            result.reset (new VolatileQualifier);
        } else {
            goto error;
        }
        if (!LEXER.consume_next_token ()) {goto error;}
        goto okay;
    }

error:
    status = false;
    goto out;
okay:
    status = true;
    a_result = result;
out:
    return status;
}

/// parses the cv-qualifier-seq production
///
/// cv-qualifier-seq:
///          cv-qualifier cv-qualifier-seq(opt)
///
bool
Parser::parse_cv_qualifier_seq (list<CVQualifierPtr> &a_result)

{
    bool status=false;
    list<CVQualifierPtr> result;
    CVQualifierPtr qualifier;
    unsigned mark = LEXER.get_token_stream_mark ();

    while (parse_cv_qualifier (qualifier) && qualifier) {
            result.push_back (qualifier);
    }
    if (result.empty ()) {goto error;}
    goto okay;

error:
    LEXER.rewind_to_mark (mark);
    status = false;
    goto out;

okay:
    status = true;
    a_result = result;

out:
    return status;
}

///
///
/// ptr-operator:
///           * cv-qualifier-seq(opt)
///           &
///           ::(opt) nested-name-specifier * cv-qualifier-seq(opt)
bool
Parser::parse_ptr_operator (PtrOperatorPtr &a_result)
{
    bool status=false;
    string  str;
    PtrOperatorPtr result;
    Token token;
    QNamePtr scope;
    PtrOperator::ElemPtr elem;
    list<CVQualifierPtr> qualifiers;
    unsigned mark = LEXER.get_token_stream_mark ();

start:
    if (!LEXER.consume_next_token (token)) {goto error;}
    result.reset (new PtrOperator);

    if (token.get_kind () == Token::OPERATOR_BIT_AND) {
        elem.reset (new PtrOperator::AndElem);
        result->append (elem);
        goto okay;
    } else if (token.get_kind () == Token::OPERATOR_MULT) {
        elem.reset (new PtrOperator::StarElem);
        result->append (elem);
        if (parse_cv_qualifier_seq (qualifiers)) {
            for (list<CVQualifierPtr>::const_iterator it=qualifiers.begin ();
                 it != qualifiers.end ();
                 ++it) {
                if ((*it)->get_kind () == CVQualifier::CONST) {
                    elem.reset (new PtrOperator::ConstElem);
                } else if ((*it)->get_kind () == CVQualifier::VOLATILE) {
                    elem.reset (new PtrOperator::VolatileElem);
                } else {
                    continue;
                }
                result->append (elem);
            }
        }
        goto okay;
    } else if (token.get_kind () == Token::OPERATOR_SCOPE_RESOL) {
        //TODO: insert nil scope here.
    }
    if (parse_nested_name_specifier (scope)) {
        result->set_scope (scope);
        goto start;
    }
    goto error;

error:
    status = false;
    LEXER.rewind_to_mark (mark);
    goto out;

okay:
    status = true;
    a_result = result;

out:
    return status;
}

/// parse a declarator production.
///
/// declarator:
///            direct-declarator
///            ptr-operator declarator
bool
Parser::parse_declarator (DeclaratorPtr &a_result)
{
    DeclaratorPtr result;
    PtrOperatorPtr ptr;
    unsigned mark = LEXER.get_token_stream_mark ();

    if (parse_direct_declarator (result)) {
        a_result.reset (new Declarator (result));
        return true;
    }
    if (parse_ptr_operator (ptr)) {
        DeclaratorPtr decl;
        if (!parse_declarator (decl)) {
            LEXER.rewind_to_mark (mark);
            return false;
        }
        result.reset (new Declarator (ptr, decl));
        a_result = result;
        return true;
    }
    LEXER.rewind_to_mark (mark);
    return false;
}

/// parse an init-declarator production
///
/// init-declarator:
///            declarator initializer(opt)
///
/// TODO: support initializer
bool
Parser::parse_init_declarator (InitDeclaratorPtr &a_decl)
{
    DeclaratorPtr decl;
    if (parse_declarator (decl)) {
        a_decl.reset (new InitDeclarator (decl));
        return true;
    }
    //TODO: support initializer
    return false;
}

/// parse an init-declarator-list production.
///
/// init-declarator-list:
///           init-declarator
///           init-declarator-list , init-declarator
bool
Parser::parse_init_declarator_list (list<InitDeclaratorPtr> &a_result)
{
    bool status=false;
    Token token;
    InitDeclaratorPtr decl;
    list<InitDeclaratorPtr> result;

    unsigned mark = LEXER.get_token_stream_mark ();

    if (parse_init_declarator (decl)) {
        result.push_back (decl);
    } else {
        goto error;
    }

loop:
    if (LEXER.peek_next_token (token)
        && token.get_kind () == Token::OPERATOR_SEQ_EVAL) {
        if (parse_init_declarator (decl)) {
            result.push_back (decl);
        } else {
            goto okay;
        }
    } else {
        goto okay;
    }
    goto loop;

error:
    LEXER.rewind_to_mark (mark);
    status = false;
    goto out;

okay:
    status = true;
    a_result = result;

out:
    return status;
}

/// parse a simple-declaration production
///
/// simple-declaration:
///           decl-specifier-seq(opt) init-declarator-list(opt) ;
///
bool
Parser::parse_simple_declaration (SimpleDeclarationPtr &a_result)
{
    list<DeclSpecifierPtr>  decl_specs;
    list<InitDeclaratorPtr> init_decls;

    if (!parse_decl_specifier_seq (decl_specs))
        return true;
    parse_init_declarator_list (init_decls) ;
    a_result.reset (new SimpleDeclaration (decl_specs, init_decls));
    return true;
}

NEMIVER_END_NAMESPACE (nemiver)
NEMIVER_END_NAMESPACE (cpp)
