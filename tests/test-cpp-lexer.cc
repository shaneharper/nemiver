#include "config.h"
#include <iostream>
#include <string>
#include <boost/test/unit_test.hpp>
#include <glibmm.h>
#include "common/nmv-ustring.h"
#include "common/nmv-initializer.h"
#include "common/nmv-exception.h"
#include "langs/nmv-cpp-lexer.h"
#include "langs/nmv-cpp-lexer-utils.h"

const char *prog = "if (foo ()) {printf (\"bar\");}";
const char *prog2 = "std::list<int> > toto;";

using nemiver::cpp::Lexer;
using nemiver::cpp::Token;
using nemiver::common::Initializer;
namespace cpp=nemiver::cpp;

void
test_lexer ()
{
    Lexer lexer (prog);
    Token token;
    int i=0;
    std::cout << "going to tokenize string: \"" << prog << "\""<< std::endl;
    while (lexer.consume_next_token (token)) {
        ++i;
        std::cout << "got token : " << i << ":" << token << std::endl;
        token.clear ();
    }
    BOOST_REQUIRE_MESSAGE (i == 12, "got " << i << " number of tokens");
    std::cout << "tokenization done okay" << std::endl;
}

void
test_lexer2 ()
{
    Lexer lexer (prog2);
    Token token;
    int i=0;
    std::cout << "going to tokenize string: \"" << prog2 << "\""<< std::endl;
    while (lexer.consume_next_token (token)) {
        ++i;
        std::cout << "got token : " << i << ":" << token << std::endl;
        token.clear ();
    }
    BOOST_REQUIRE_MESSAGE (i == 9, "got " << i << " number of tokens");
    std::cout << "tokenization done okay" << std::endl;
}

using boost::unit_test::test_suite;

test_suite*
init_unit_test_suite (int /*argc*/, char** /*argv*/)
{
    NEMIVER_TRY

    Initializer::do_init ();

    test_suite *suite = BOOST_TEST_SUITE ("c++ lexer tests");
    suite->add (BOOST_TEST_CASE (&test_lexer));
    suite->add (BOOST_TEST_CASE (&test_lexer2));
    return suite;

    NEMIVER_CATCH_NOX

    return 0;
}
