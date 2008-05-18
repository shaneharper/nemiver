#include <iostream>
#include <string>
#include <boost/test/unit_test.hpp>
#include <glibmm.h>
#include "common/nmv-ustring.h"
#include "common/nmv-initializer.h"
#include "common/nmv-exception.h"
#include "common/nmv-env.h"

using namespace std;
using nemiver::common::UString;
using nemiver::common::Initializer;
using boost::unit_test::test_suite;
namespace env=nemiver::common::env;
void
test_build_path_to_help_file ()
{
    string nemiver_file ("nemiver.xml");
    UString path (env::build_path_to_help_file ("nemiver.xml"));
    BOOST_REQUIRE_MESSAGE (!path.empty (),
                           "failed build path to " + nemiver_file);
}

test_suite*
init_unit_test_suite (int argc, char** argv)
{
    if (argc || argv) {/*keep compiler happy*/}

    NEMIVER_TRY

    Initializer::do_init () ;

    test_suite *suite = BOOST_TEST_SUITE ("nemiver env tests") ;
    suite->add (BOOST_TEST_CASE (&test_build_path_to_help_file)) ;
    return suite;

    NEMIVER_CATCH_NOX

    return 0;
}
