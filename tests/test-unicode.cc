#include "config.h"
#include <iostream>
#include <string>
#include <boost/test/unit_test.hpp>
#include <glibmm.h>
#include "common/nmv-ustring.h"
#include "common/nmv-initializer.h"
#include "common/nmv-exception.h"

using namespace std;
using namespace nemiver;
using nemiver::common::Initializer;
using nemiver::common::UString;
using nemiver::common::WString;
using nemiver::common::ustring_to_wstring;
using nemiver::common::wstring_to_ustring;

gunichar s_wstr[] = {230, 231, 232, 233, 234, 0};

void test_wstring_to_ustring ()
{
    UString utf8_str;
    BOOST_REQUIRE (wstring_to_ustring (s_wstr, utf8_str));
    unsigned int s_wstr_len = (sizeof (s_wstr)/sizeof (gunichar))-1;
    BOOST_REQUIRE (s_wstr_len == 5);
    BOOST_REQUIRE (s_wstr_len == utf8_str.size ());
    for (unsigned int i=0; i < utf8_str.size () ; ++i) {
        BOOST_REQUIRE (s_wstr[i] == utf8_str[i]);
    }
}

void test_ustring_to_wstring ()
{
    UString utf8_str;
    BOOST_REQUIRE (wstring_to_ustring (s_wstr, utf8_str));
    WString wstr;
    BOOST_REQUIRE (ustring_to_wstring (utf8_str, wstr));
    BOOST_REQUIRE (wstr.size () == utf8_str.size ());
    BOOST_REQUIRE (wstr.size () == 5);
    BOOST_REQUIRE (!wstr.compare (0, wstr.size (), s_wstr));
}

using boost::unit_test::test_suite;

NEMIVER_API test_suite*
init_unit_test_suite (int /*argc*/, char** /*argv*/)
{
    NEMIVER_TRY

    Initializer::do_init ();

    test_suite *suite = BOOST_TEST_SUITE ("Unicode tests");
    suite->add (BOOST_TEST_CASE (&test_wstring_to_ustring));
    suite->add (BOOST_TEST_CASE (&test_ustring_to_wstring));
    return suite;

    NEMIVER_CATCH_NOX

    return 0;
}
