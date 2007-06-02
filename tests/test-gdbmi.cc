#include <list>
#include <map>
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include "dbgengine/nmv-gdbmi-parser.h"
#include "common/nmv-exception.h"
#include "common/nmv-initializer.h"

using namespace std ;
using namespace nemiver ;

static const char* gv_str0 = "\"abracadabra\"" ;
static const char* gv_str1 = "\"/home/dodji/misc/no\\303\\253l-\\303\\251-\\303\\240/test.c\"" ;
static const char* gv_str2 = "\"No symbol \\\"events_ecal\\\" in current context.\\n\"" ;

static const char* gv_attrs0 = "msg=\"No symbol \\\"g_return_if_fail\\\" in current context.\"" ;

static const char* gv_stopped_async_output =
"*stopped,reason=\"breakpoint-hit\",bkptno=\"1\",thread-id=\"1\",frame={addr=\"0x0804afb0\",func=\"main\",args=[{name=\"argc\",value=\"1\"},{name=\"argv\",value=\"0xbfc79ed4\"}],file=\"today-main.c\",fullname=\"/home/dodji/devel/gitstore/omoko.git/applications/openmoko-today/src/today-main.c\",line=\"285\"}\n" ;

//the partial result of a gdbmi command: -stack-list-argument 1 command
//this command is used to implement IDebugger::list_frames_arguments()
static const char* gv_stack_arguments0 =
"stack-args=[frame={level=\"0\",args=[{name=\"a_param\",value=\"(Person &) @0xbf88fad4: {m_first_name = {static npos = 4294967295, _M_dataplus = {<std::allocator<char>> = {<__gnu_cxx::new_allocator<char>> = {<No data fields>}, <No data fields>}, _M_p = 0x804b144 \\\"Ali\\\"}}, m_family_name = {static npos = 4294967295, _M_dataplus = {<std::allocator<char>> = {<__gnu_cxx::new_allocator<char>> = {<No data fields>}, <No data fields>}, _M_p = 0x804b12c \\\"BABA\\\"}}, m_age = 15}\"}]},frame={level=\"1\",args=[]}]" ;

static const char* gv_local_vars =
"locals=[{name=\"person\",type=\"Person\"}]";

static const char* gv_emb_str =
"\\\"\\\\311\\\\303\\\\220U\\\\211\\\\345S\\\\203\\\\354\\\\024\\\\213E\\\\b\\\\211\\\\004$\\\\350\\\\202\\\\373\\\\377\\\\377\\\\213E\\\\b\\\\203\\\\300\\\\004\\\\211\\\\004$\\\\350t\\\\373\\\\377\\\\377\\\\213U\\\\b\\\\213E\\\\f\\\\211D$\\\\004\\\\211\\\\024$\\\\350\\\\002\\\\373\\\\377\\\\377\\\\213U\\\\b\\\\203\\\\302\\\\004\\\\213E\\\\020\\\\211D$\\\\004\\\\211\\\\024$\\\\350\\\\355\\\\372\\\\377\\\\377\\\\213U\\\\b\\\\213E\\\\024\\\\211B\\\\b\\\\3538\\\\211E\\\\370\\\\213]\\\\370\\\\213E\\\\b\\\\203\\\\300\\\\004\\\\211\\\\004$\\\\350\\\\276\\\\372\\\\377\\\\377\\\\211]\\\\370\\\\353\\\\003\\\\211E\\\\370\\\\213]\\\\370\\\\213E\\\\b\\\\211\\\\004$\\\\350\\\\250\\\\372\\\\377\\\\377\\\\211]\\\\370\\\\213E\\\\370\\\\211\\\\004$\\\\350\\\\032\\\\373\\\\377\\\\377\\\\203\\\\304\\\\024[]\\\\303U\\\\211\\\\345S\\\\203\\\\354$\\\\215E\\\\372\\\\211\\\\004$\\\\350\\\\\\\"\\\\373\\\\377\\\\377\\\\215E\\\\372\\\\211D$\\\\b\\\\307D$\\\\004\\\\370\\\\217\\\\004\\\\b\\\\215E\\\\364\\\\211\\\\004$\\\\350\\\\230\\\\372\\\\377\\\\377\\\\215E\\\\372\\\\211\\\\004$\\\\350}\\\\372\\\""

;
static const char* gv_member_var = 
"{static npos = 4294967295, _M_dataplus = {<std::allocator<char>> = {<__gnu_cxx::new_allocator<char>> = {<No data fields>}, <No data fields>}, _M_p = 0x8048ce1 \"\\311\\303\\220U\\211\\345S\\203\\354\\024\\213E\\b\\211\\004$\\350\\202\\373\\377\\377\\213E\\b\\203\\300\\004\\211\\004$\\350t\\373\\377\\377\\213U\\b\\213E\\f\\211D$\\004\\211\\024$\\350\\002\\373\\377\\377\\213U\\b\\203\\302\\004\\213E\\020\\211D$\\004\\211\\024$\\350\\355\\372\\377\\377\\213U\\b\\213E\\024\\211B\\b\\3538\\211E\\370\\213]\\370\\213E\\b\\203\\300\\004\\211\\004$\\350\\276\\372\\377\\377\\211]\\370\\353\\003\\211E\\370\\213]\\370\\213E\\b\\211\\004$\\350\\250\\372\\377\\377\\211]\\370\\213E\\370\\211\\004$\\350\\032\\373\\377\\377\\203\\304\\024[]\\303U\\211\\345S\\203\\354$\\215E\\372\\211\\004$\\350\\\"\\373\\377\\377\\215E\\372\\211D$\\b\\307D$\\004\\370\\217\\004\\b\\215E\\364\\211\\004$\\350\\230\\372\\377\\377\\215E\\372\\211\\004$\\350}\\372\"...}}" ;

static const char* gv_member_var2 = "{<com::sun::star::uno::BaseReference> = {_pInterface = 0x86a4834}, <No data fields>}" ;

static const char* gv_var_with_member = "value=\"{static npos = 4294967295, _M_dataplus = {<std::allocator<char>> = {<__gnu_cxx::new_allocator<char>> = {<No data fields>}, <No data fields>}, _M_p = 0x8048ce1 \"\\311\\303\\220U\\211\\345S\\203\\354\\024\\213E\\b\\211\\004$\\350\\202\\373\\377\\377\\213E\\b\\203\\300\\004\\211\\004$\\350t\\373\\377\\377\\213U\\b\\213E\\f\\211D$\\004\\211\\024$\\350\\002\\373\\377\\377\\213U\\b\\203\\302\\004\\213E\\020\\211D$\\004\\211\\024$\\350\\355\\372\\377\\377\\213U\\b\\213E\\024\\211B\\b\\3538\\211E\\370\\213]\\370\\213E\\b\\203\\300\\004\\211\\004$\\350\\276\\372\\377\\377\\211]\\370\\353\\003\\211E\\370\\213]\\370\\213E\\b\\211\\004$\\350\\250\\372\\377\\377\\211]\\370\\213E\\370\\211\\004$\\350\\032\\373\\377\\377\\203\\304\\024[]\\303U\\211\\345S\\203\\354$\\215E\\372\\211\\004$\\350\\\"\\373\\377\\377\\215E\\372\\211D$\\b\\307D$\\004\\370\\217\\004\\b\\215E\\364\\211\\004$\\350\\230\\372\\377\\377\\215E\\372\\211\\004$\\350}\\372\"...}}\"" ;

static const char* gv_stack_arguments1 =
"stack-args=[frame={level=\"0\",args=[{name=\"a_comp\",value=\"(icalcomponent *) 0x80596f8\"},{name=\"a_entry\",value=\"(MokoJEntry **) 0xbfe02178\"}]},frame={level=\"1\",args=[{name=\"a_view\",value=\"(ECalView *) 0x804ba60\"},{name=\"a_entries\",value=\"(GList *) 0x8054930\"},{name=\"a_journal\",value=\"(MokoJournal *) 0x8050580\"}]},frame={level=\"2\",args=[{name=\"closure\",value=\"(GClosure *) 0x805a010\"},{name=\"return_value\",value=\"(GValue *) 0x0\"},{name=\"n_param_values\",value=\"2\"},{name=\"param_values\",value=\"(const GValue *) 0xbfe023cc\"},{name=\"invocation_hint\",value=\"(gpointer) 0xbfe022dc\"},{name=\"marshal_data\",value=\"(gpointer) 0xb7f9a146\"}]},frame={level=\"3\",args=[{name=\"closure\",value=\"(GClosure *) 0x805a010\"},{name=\"return_value\",value=\"(GValue *) 0x0\"},{name=\"n_param_values\",value=\"2\"},{name=\"param_values\",value=\"(const GValue *) 0xbfe023cc\"},{name=\"invocation_hint\",value=\"(gpointer) 0xbfe022dc\"}]},frame={level=\"4\",args=[{name=\"node\",value=\"(SignalNode *) 0x80599c8\"},{name=\"detail\",value=\"0\"},{name=\"instance\",value=\"(gpointer) 0x804ba60\"},{name=\"emission_return\",value=\"(GValue *) 0x0\"},{name=\"instance_and_params\",value=\"(const GValue *) 0xbfe023cc\"}]},frame={level=\"5\",args=[{name=\"instance\",value=\"(gpointer) 0x804ba60\"},{name=\"signal_id\",value=\"18\"},{name=\"detail\",value=\"0\"},{name=\"var_args\",value=\"0xbfe02610 \\\"\\\\300\\\\365\\\\004\\\\b\\\\020,\\\\340\\\\277\\\\370\\\\024[\\\\001\\\\360\\\\226i\\\\267\\\\320`\\\\234\\\\267\\\\200\\\\237\\\\005\\\\bX&\\\\340\\\\277\\\\333cg\\\\267\\\\200{\\\\005\\\\b0I\\\\005\\\\b`\\\\272\\\\004\\\\b\\\\002\\\"\"}]},frame={level=\"6\",args=[{name=\"instance\",value=\"(gpointer) 0x804ba60\"},{name=\"signal_id\",value=\"18\"},{name=\"detail\",value=\"0\"}]},frame={level=\"7\",args=[{name=\"listener\",value=\"(ECalViewListener *) 0x8057b80\"},{name=\"objects\",value=\"(GList *) 0x8054930\"},{name=\"data\",value=\"(gpointer) 0x804ba60\"}]},frame={level=\"8\",args=[{name=\"closure\",value=\"(GClosure *) 0x8059f80\"},{name=\"return_value\",value=\"(GValue *) 0x0\"},{name=\"n_param_values\",value=\"2\"},{name=\"param_values\",value=\"(const GValue *) 0xbfe0286c\"},{name=\"invocation_hint\",value=\"(gpointer) 0xbfe0277c\"},{name=\"marshal_data\",value=\"(gpointer) 0xb79c60d0\"}]},frame={level=\"9\",args=[{name=\"closure\",value=\"(GClosure *) 0x8059f80\"},{name=\"return_value\",value=\"(GValue *) 0x0\"},{name=\"n_param_values\",value=\"2\"},{name=\"param_values\",value=\"(const GValue *) 0xbfe0286c\"},{name=\"invocation_hint\",value=\"(gpointer) 0xbfe0277c\"}]},frame={level=\"10\",args=[{name=\"node\",value=\"(SignalNode *) 0x8057a08\"},{name=\"detail\",value=\"0\"},{name=\"instance\",value=\"(gpointer) 0x8057b80\"},{name=\"emission_return\",value=\"(GValue *) 0x0\"},{name=\"instance_and_params\",value=\"(const GValue *) 0xbfe0286c\"}]},frame={level=\"11\",args=[{name=\"instance\",value=\"(gpointer) 0x8057b80\"},{name=\"signal_id\",value=\"12\"},{name=\"detail\",value=\"0\"},{name=\"var_args\",value=\"0xbfe02ab0 \\\"\\\\314,\\\\340\\\\277\\\\300m\\\\006\\\\b\\\\233d\\\\234\\\\267\\\\020\\\\347\\\\240\\\\267\\\\220d\\\\234\\\\267\\\\230m\\\\006\\\\b\\\\370*\\\\340\\\\277\\\\317i\\\\234\\\\267\\\\200{\\\\005\\\\b@n\\\\006\\\\b\\\\200*\\\\005\\\\b\\\"\"}]},frame={level=\"12\",args=[{name=\"instance\",value=\"(gpointer) 0x8057b80\"},{name=\"signal_id\",value=\"12\"},{name=\"detail\",value=\"0\"}]},frame={level=\"13\",args=[{name=\"ql\",value=\"(ECalViewListener *) 0x8057b80\"},{name=\"objects\",value=\"(char **) 0x8066e40\"},{name=\"context\",value=\"(DBusGMethodInvocation *) 0x8052a80\"}]},frame={level=\"14\",args=[{name=\"closure\",value=\"(GClosure *) 0xbfe02d1c\"},{name=\"return_value\",value=\"(GValue *) 0x0\"},{name=\"n_param_values\",value=\"3\"},{name=\"param_values\",value=\"(const GValue *) 0x8066d98\"},{name=\"invocation_hint\",value=\"(gpointer) 0x0\"},{name=\"marshal_data\",value=\"(gpointer) 0xb79c6490\"}]},frame={level=\"15\",args=[]},frame={level=\"16\",args=[]},frame={level=\"17\",args=[]}]" ;


static const char*  gv_overloads_prompt0=
"[0] cancel\\n"
"[1] all\\n"
"[2] nemiver::GDBEngine::set_breakpoint(nemiver::common::UString const&, nemiver::common::UString const&) at nmv-gdb-engine.cc:2507\\n"
"[3] nemiver::GDBEngine::set_breakpoint(nemiver::common::UString const&, int, nemiver::common::UString const&) at nmv-gdb-engine.cc:2462\\n"
;

static const char* gv_overloads_prompt1=
"[0] cancel\n[1] all\n"
"[2] Person::overload(int) at fooprog.cc:65\n"
"[3] Person::overload() at fooprog.cc:59"
;

void
test_str0 ()
{
    bool is_ok =false ;

    UString res ;
    UString::size_type to=0 ;
    is_ok = parse_c_string (gv_str0, 0, to, res) ;
    BOOST_REQUIRE (is_ok) ;
    BOOST_REQUIRE (res == "abracadabra") ;
}

void
test_str1 ()
{
    bool is_ok =false ;

    UString res ;
    UString::size_type to=0 ;
    is_ok = parse_c_string (gv_str1, 0, to, res) ;
    BOOST_REQUIRE (is_ok) ;
    MESSAGE ("got string: '" << Glib::locale_from_utf8 (res) << "'") ;
    BOOST_REQUIRE_MESSAGE (res.size () == 32, "res size was: " << res.size ());
}

void
test_str2 ()
{
    bool is_ok =false ;

    UString res ;
    UString::size_type to=0 ;
    is_ok = parse_c_string (gv_str2, 0, to, res) ;
    BOOST_REQUIRE (is_ok) ;
    MESSAGE ("got string: '" << Glib::locale_from_utf8 (res) << "'") ;
    BOOST_REQUIRE_MESSAGE (res.size (), "res size was: " << res.size ());
}

void
test_attr0 ()
{
    bool is_ok =false ;

    UString name,value ;
    UString::size_type to=0 ;
    is_ok = parse_attribute (gv_attrs0, 0, to, name, value) ;
    BOOST_REQUIRE (is_ok) ;
    BOOST_REQUIRE_MESSAGE (name == "msg", "got name: " << name) ;
    BOOST_REQUIRE_MESSAGE (value.size(), "got empty value") ;
}

void
test_stoppped_async_output ()
{
    bool is_ok=false,got_frame=false ;
    UString::size_type to=0 ;
    IDebugger::Frame frame ;
    map<UString, UString> attrs ;

    is_ok = parse_stopped_async_output (gv_stopped_async_output, 0, to,
                                        got_frame, frame, attrs) ;
    BOOST_REQUIRE (is_ok) ;
    BOOST_REQUIRE (got_frame) ;
    BOOST_REQUIRE (attrs.size ()) ;
}


void
test_stack_arguments0 ()
{
    bool is_ok=false ;
    UString::size_type to ;
    map<int, list<IDebugger::VariableSafePtr> >params ;
    is_ok = nemiver::parse_stack_arguments (gv_stack_arguments0,
                                            0, to, params) ;
    BOOST_REQUIRE (is_ok) ;
    BOOST_REQUIRE (params.size () == 2) ;
    map<int, list<IDebugger::VariableSafePtr> >::iterator param_iter ;
    param_iter = params.find (0) ;
    BOOST_REQUIRE (param_iter != params.end ()) ;
    IDebugger::VariableSafePtr variable = *(param_iter->second.begin ()) ;
    BOOST_REQUIRE (variable) ;
    BOOST_REQUIRE (variable->name () == "a_param") ;
    BOOST_REQUIRE (!variable->members ().empty ()) ;
}

void
test_stack_arguments1 ()
{
    bool is_ok=false ;
    UString::size_type to ;
    map<int, list<IDebugger::VariableSafePtr> >params ;
    is_ok = nemiver::parse_stack_arguments (gv_stack_arguments1,
                                            0, to, params) ;
    BOOST_REQUIRE (is_ok) ;
    BOOST_REQUIRE_MESSAGE (params.size () == 18, "got nb params "
                           << params.size ()) ;
    map<int, list<IDebugger::VariableSafePtr> >::iterator param_iter ;
    param_iter = params.find (0) ;
    BOOST_REQUIRE (param_iter != params.end ()) ;
    IDebugger::VariableSafePtr variable = *(param_iter->second.begin ()) ;
    BOOST_REQUIRE (variable) ;
    BOOST_REQUIRE (variable->name () == "a_comp") ;
    BOOST_REQUIRE (variable->members ().empty ()) ;
}

void
test_local_vars ()
{
    bool is_ok=false ;
    UString::size_type to=0 ;
    list<IDebugger::VariableSafePtr> vars ;
    is_ok = parse_local_var_list (gv_local_vars, 0, to, vars) ;
    BOOST_REQUIRE (is_ok) ;
    BOOST_REQUIRE (vars.size () == 1) ;
    IDebugger::VariableSafePtr var = *(vars.begin ()) ;
    BOOST_REQUIRE (var) ;
    BOOST_REQUIRE (var->name () == "person") ;
    BOOST_REQUIRE (var->type () == "Person") ;
}

void
test_member_variable ()
{
    {
        UString::size_type to = 0;
        IDebugger::VariableSafePtr var (new IDebugger::Variable) ;
        BOOST_REQUIRE (parse_member_variable (gv_member_var, 0, to, var)) ;
        BOOST_REQUIRE (var) ;
        BOOST_REQUIRE (!var->members ().empty ()) ;
    }

    {
        UString::size_type to = 0;
        IDebugger::VariableSafePtr var (new IDebugger::Variable) ;
        BOOST_REQUIRE (parse_member_variable (gv_member_var2, 0, to, var)) ;
        BOOST_REQUIRE (var) ;
        BOOST_REQUIRE (!var->members ().empty ()) ;
    }
}

void
test_var_with_member_variable ()
{
    UString::size_type to = 0;
    IDebugger::VariableSafePtr var (new IDebugger::Variable) ;
    BOOST_REQUIRE (parse_variable_value (gv_var_with_member, 0, to, var)) ;
    BOOST_REQUIRE (var) ;
    BOOST_REQUIRE (!var->members ().empty ()) ;
}

void
test_embedded_string ()
{
    UString::size_type to = 0 ;
    UString str ;
    BOOST_REQUIRE (parse_embedded_c_string (gv_emb_str, 0, to, str)) ;
}

void
test_overloads_prompt ()
{
    vector<IDebugger::OverloadsChoiceEntry> prompts ;
    UString::size_type cur = 0 ;
    BOOST_REQUIRE (parse_overloads_choice_prompt (gv_overloads_prompt0,
                                                  cur, cur, prompts)) ;
    BOOST_REQUIRE_MESSAGE (prompts.size () == 4,
                           "actually got " << prompts.size ()) ;

    cur=0 ;
    prompts.clear () ;
    BOOST_REQUIRE (parse_overloads_choice_prompt (gv_overloads_prompt1,
                                                  cur, cur, prompts)) ;
    BOOST_REQUIRE_MESSAGE (prompts.size () == 4,
                           "actually got " << prompts.size ()) ;
}

using boost::unit_test::test_suite ;

NEMIVER_API test_suite*
init_unit_test_suite (int argc, char **argv)
{
    if (argc || argv) {/*keep compiler happy*/}

    NEMIVER_TRY

    nemiver::common::Initializer::do_init () ;

    test_suite *suite = BOOST_TEST_SUITE ("GDBMI tests") ;
    suite->add (BOOST_TEST_CASE (&test_str0)) ;
    suite->add (BOOST_TEST_CASE (&test_str1)) ;
    suite->add (BOOST_TEST_CASE (&test_str2)) ;
    suite->add (BOOST_TEST_CASE (&test_attr0)) ;
    suite->add (BOOST_TEST_CASE (&test_stoppped_async_output)) ;
    suite->add (BOOST_TEST_CASE (&test_stack_arguments0)) ;
    suite->add (BOOST_TEST_CASE (&test_stack_arguments1)) ;
    suite->add (BOOST_TEST_CASE (&test_local_vars)) ;
    suite->add (BOOST_TEST_CASE (&test_member_variable)) ;
    suite->add (BOOST_TEST_CASE (&test_var_with_member_variable)) ;
    suite->add (BOOST_TEST_CASE (&test_embedded_string)) ;
    suite->add (BOOST_TEST_CASE (&test_overloads_prompt)) ;
    return suite ;

    NEMIVER_CATCH_NOX

    return 0 ;
}

