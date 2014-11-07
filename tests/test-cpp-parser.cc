#include "config.h"
#include <iostream>
#include <string>
#include <boost/test/unit_test.hpp>
#include <glibmm.h>
#include "common/nmv-ustring.h"
#include "common/nmv-initializer.h"
#include "common/nmv-exception.h"
#include "langs/nmv-cpp-parser.h"
#include "langs/nmv-cpp-ast-utils.h"

#define TAB_LEN(tab) (sizeof (tab)/sizeof (tab[0]))
const char *prog0 = "foo bar";
const char *prog1 = "foo *bar";
const char *prog1_1 = "foo bar[10]";
const char *prog2 = "const foo *bar";

const char* test3_inputs[] = {
    "const char *std::__num_base::_S_atoms_in",
    "const char *const *const std::locale::_S_categories",
    "const std::locale::id *const *std::locale::_Impl::_S_facet_categories[0]",
    "static const foo *bar",
    "static const bar<baz> maman",
    "const size_t std::allocator<wchar_t>::foo",
    "const size_t std::basic_string<wchar_t, std::char_traits<wchar_t> >::foo",
    "const size_t std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> >::_Rep::_S_max_size",
    "static long unsigned int __stl_prime_list[28]",
};

const char* test4_inputs[] = {
    "static const unsigned int",
    "foo<bar>"
};

const char* test5_inputs[] = {
    "(int)5",
    "5",
    "foo.*bar",
    "foo->*bar",
    "foo*bar",
    "foo/bar",
    "foo%bar",
    "foo+bar-baz",
    "foo<<bar",
    "foo>>bar",
    "foo<bar",
    "foo>bar",
    "foo>=bar",
    "foo<=bar",
    "(1+3)*5<=(10-6+32)-bar",
    "foo==bar",
    "foo<=bar",
    "foo&bar",
    "foo^bar",
    "foo|bar",
    "foo&&bar",
    "foo||bar",
    "(foo<bar)?coin=pouf:paf=pim"
};

const char* test6_inputs[] = {
    "foo<t1, t2, t3>",
    "foo<(t1>t2)>",
    "Y<X<1> >",
    "Y<X<t1>, X<t2> >"
};

struct test7_record {
    const char *input;
    const char* variable_name;
};

const test7_record test7_inputs[] = {
    {"void *__dso_handle", "__dso_handle"},
    {"const char *std::__num_base::_S_atoms_in", "std::__num_base::_S_atoms_in"},
    {"const std::locale::id *const *std::locale::_Impl::_S_facet_categories[0]", "std::locale::_Impl::_S_facet_categories"},
    {"const std::locale::id *std::locale::_Impl::_S_id_ctype[0]", "std::locale::_Impl::_S_id_ctype"},
    {"const std::locale::id *std::locale::_Impl::_S_id_monetary[0]", "std::locale::_Impl::_S_id_monetary"},
};

using std::cout;
using std::endl;
using nemiver::cpp::Parser;
using nemiver::cpp::ParserPtr;
using nemiver::cpp::SimpleDeclarationPtr;
using nemiver::cpp::DeclSpecifier;
using nemiver::cpp::DeclSpecifierPtr;
using nemiver::cpp::InitDeclaratorPtr;
using nemiver::cpp::InitDeclarator;
using nemiver::cpp::Expr;
using nemiver::cpp::CastExprPtr;
using nemiver::cpp::CastExpr;
using nemiver::cpp::PMExprPtr;
using nemiver::cpp::MultExprPtr;
using nemiver::cpp::AddExprPtr;
using nemiver::cpp::ShiftExprPtr;
using nemiver::cpp::RelExprPtr;
using nemiver::cpp::EqExprPtr;
using nemiver::cpp::AndExprPtr;
using nemiver::cpp::XORExprPtr;
using nemiver::cpp::ORExprPtr;
using nemiver::cpp::LogAndExprPtr;
using nemiver::cpp::LogOrExprPtr;
using nemiver::cpp::CondExprPtr;
using nemiver::cpp::ExprPtr;
using nemiver::cpp::TemplateIDPtr;
using nemiver::common::Initializer;
namespace cpp=nemiver::cpp;

void
test_parser0 ()
{
    Parser parser (prog0);
    Parser parser2 (prog0);
    string str;
    SimpleDeclarationPtr simple_decl;
    list<DeclSpecifierPtr> decls;

    if (!parser.parse_decl_specifier_seq (decls)) {
        BOOST_FAIL ("parsing failed");
    }
    if (decls.size () != 1) {
        BOOST_FAIL ("found " << decls.size () << "decl specifiers instead of 1");
    }
    DeclSpecifier::list_to_string (decls, str);
    cout << "decl specifier was: '" << str <<  "'." << endl;

    if (!parser2.parse_simple_declaration (simple_decl)) {
        BOOST_FAIL ("parsing failed");
    }
    if (!simple_decl) {
        BOOST_FAIL ("got empty simple declaration");
        return;
    }
    simple_decl->to_string (str);
    cout << "simple declaration was: '" << str <<  "'." << endl;

    DeclSpecifier::list_to_string (simple_decl->get_decl_specifiers (), str);
    cout << "Decl specifier was: " << str << endl;
    InitDeclarator::list_to_string (simple_decl->get_init_declarators (), str);
    cout << "Init declarators were: " << str << endl;
}

void
test_parser1 ()
{
    string str, prog_str=prog1;
    Parser parser (prog_str);
    SimpleDeclarationPtr simple_decl;

    if (!parser.parse_simple_declaration (simple_decl)) {
        BOOST_FAIL ("parsing failed");
    }
    if (!simple_decl) {
        cout << "got empty simple declaration" << endl;
        return;
    }
    simple_decl->to_string (str);
    if (prog_str != str) {
        BOOST_FAIL ("parsed '" <<prog_str << "' into '" << str << "'");
    }

    prog_str = prog1_1;
    Parser parser1 (prog_str);
    if (!parser1.parse_simple_declaration (simple_decl)) {
        BOOST_FAIL ("parsing failed");
    }
    simple_decl->to_string (str);
    if (prog_str != str) {
        BOOST_FAIL ("parsed '" <<prog_str << "' into '" << str << "'");
    }
}

void
test_parser2 ()
{
    Parser parser (prog2);
    string str;
    SimpleDeclarationPtr simple_decl;

    if (!parser.parse_simple_declaration (simple_decl)) {
        BOOST_FAIL ("parsing failed");
    }
    if (!simple_decl) {
        cout << "got empty simple declaration" << endl;
        return;
    }
    simple_decl->to_string (str);
    if (str != prog2) {
        BOOST_FAIL ("parsed '" <<prog2 << "' into '" << str << "'");
    }
}

void
test_parser3 ()
{
    SimpleDeclarationPtr simple_decl;
    ParserPtr parser;
    string str;

    for (unsigned i=0; i < TAB_LEN (test3_inputs); i++) {
        parser.reset (new Parser (test3_inputs[i]));
        if (!parser->parse_simple_declaration (simple_decl)) {
            BOOST_FAIL ("parsing of '" << test3_inputs[i] << "' failed");
        }
        if (!simple_decl) {
            cout << "got empty simple declaration" << endl;
            return;
        }
        simple_decl->to_string (str);
        if (test3_inputs[i] != str) {
            BOOST_FAIL ("parsed string No " << i << ": '"
                        << test3_inputs[i]
                        << "' into '" << str << "'");
        }
    }
}

void
test_parser4 ()
{
    ParserPtr parser;
    list<DeclSpecifierPtr> decls;
    string str;

    for (unsigned i=0; i < TAB_LEN (test4_inputs); ++i) {
        parser.reset (new Parser (test4_inputs[i]));
        if (!parser->parse_decl_specifier_seq (decls)) {
            BOOST_FAIL ("parsing of '" << test4_inputs[i] << "' failed");
        }
        if (decls.empty ()) {
            cout << "got empty decl specifier sequence" << endl;
            return;
        }
        DeclSpecifier::list_to_string (decls, str);
        if (str != test4_inputs[i]) {
            BOOST_FAIL ("parsed '" << test4_inputs[i] << "' into '" << str << "'");
        }
    }
}

void
test_parser5 ()
{
    string str;
    ExprPtr expr;
    ParserPtr parser;

    for (unsigned i=0; i < TAB_LEN (test5_inputs); ++i) {
        parser.reset (new Parser (test5_inputs[i]));
        if (!parser->parse_expr (expr) || !expr) {
            BOOST_FAIL ("failed to parse '" << test5_inputs[i] << "'");
        }
        expr->to_string (str);
        if (str != test5_inputs[i]) {
            BOOST_FAIL ("tried parsing '"
                        << test5_inputs[i]
                        << "' and got: '"
                        << str << "'");
        }
    }
}

void
test_parser6 ()
{
    string str, prog_str;
    TemplateIDPtr template_id;
    ParserPtr parser;

    unsigned i=0;
    for (i=0; i < TAB_LEN (test6_inputs); ++i) {
        parser.reset (new Parser (test6_inputs[i]));
        if (!parser->parse_template_id (template_id)) {
            BOOST_FAIL ("failed to parse '" << test6_inputs[i] << "'");
        }
        template_id->to_string (str);
        if (str != test6_inputs[i]) {
            BOOST_FAIL ("parsed expr '"
                        << test6_inputs[i]
                        << "into '"
                        << str
                        <<  "'");
        }
    }
}

void
test_parser7 ()
{
    SimpleDeclarationPtr simple_decl;
    InitDeclaratorPtr init_decl;
    ParserPtr parser;
    string str;

    for (unsigned i=0; i < TAB_LEN (test7_inputs); i++) {
        parser.reset (new Parser (test7_inputs[i].input));
        if (!parser->parse_simple_declaration (simple_decl)) {
            BOOST_FAIL ("parsing of '" << test7_inputs[i].input << "' failed");
        }
        if (!simple_decl) {
            cout << "got empty simple declaration" << endl;
            return;
        }
        init_decl = *simple_decl->get_init_declarators ().begin ();
        if (!get_declarator_id_as_string (init_decl, str)) {
            BOOST_FAIL ("failed to get decl id of input: " << test7_inputs[i].input);
        }
        if (str != test7_inputs[i].variable_name) {
            BOOST_FAIL ("for input " << test7_inputs[i].input
                        << ", found variable name: "
                        << str
                        << "instead of "
                        << test7_inputs[i].variable_name);
        }
    }
}

using boost::unit_test::test_suite;

test_suite*
init_unit_test_suite (int /*argc*/, char** /*argv*/)
{
    NEMIVER_TRY

    Initializer::do_init ();

    test_suite *suite = BOOST_TEST_SUITE ("c++ lexer tests");
    suite->add (BOOST_TEST_CASE (&test_parser0));
    suite->add (BOOST_TEST_CASE (&test_parser1));
    suite->add (BOOST_TEST_CASE (&test_parser2));
    suite->add (BOOST_TEST_CASE (&test_parser3));
    suite->add (BOOST_TEST_CASE (&test_parser4));
    suite->add (BOOST_TEST_CASE (&test_parser5));
    suite->add (BOOST_TEST_CASE (&test_parser6));
    suite->add (BOOST_TEST_CASE (&test_parser7));
    return suite;

    NEMIVER_CATCH_NOX

    return 0;
}
