#include <boost/test/unit_test.hpp>
#include "common/nmv-initializer.h"
#include "common/nmv-exception.h"
#include "nmv-i-lang-trait.h"
#include "nmv-i-debugger.h"
#include "nmv-debugger-utils.h"

using namespace std;
using nemiver::common::Initializer;
using nemiver::common::DynamicModuleManager;
using nemiver::ILangTraitSafePtr;
using nemiver::ILangTrait;

void test_load_dynmod ()
{
    ILangTraitSafePtr trait =
        DynamicModuleManager::load_iface_with_default_manager<ILangTrait>
                                                        ("cpptrait",
                                                         "ILangTrait");
    //BOOST_REQUIRE (trait);
}

void test_name ()
{
    ILangTraitSafePtr trait =
        DynamicModuleManager::load_iface_with_default_manager<ILangTrait>
                                                        ("cpptrait",
                                                         "ILangTrait");
    BOOST_REQUIRE (trait->get_name () == "cpptrait");
}

void test_is_pointer ()
{
    ILangTraitSafePtr trait =
        DynamicModuleManager::load_iface_with_default_manager<ILangTrait>
                                                        ("cpptrait",
                                                         "ILangTrait");
    BOOST_REQUIRE (trait->has_pointers () == true);

    BOOST_REQUIRE (trait->is_type_a_pointer ("char *"));
    BOOST_REQUIRE (trait->is_type_a_pointer ("AType * const"));
}

void test_debugger ()
{
    using nemiver::IDebugger;
    using nemiver::IDebuggerSafePtr;
    IDebuggerSafePtr debugger =
        nemiver::debugger_utils::load_debugger_iface_with_confmgr ();
    BOOST_REQUIRE (debugger);
    ILangTrait &trait = debugger->get_language_trait ();
    BOOST_REQUIRE (trait.get_name () == "cpptrait");
}

using boost::unit_test::test_suite;

NEMIVER_API test_suite*
init_unit_test_suite (int, char **)
{
    NEMIVER_TRY

    Initializer::do_init ();

    test_suite *suite = BOOST_TEST_SUITE ("cpp language trait tests");
    suite->add (BOOST_TEST_CASE (&test_load_dynmod));
    suite->add (BOOST_TEST_CASE (&test_name));
    suite->add (BOOST_TEST_CASE (&test_is_pointer));
    suite->add (BOOST_TEST_CASE (&test_debugger));
    return suite;

    NEMIVER_CATCH_NOX

    return 0;
}
