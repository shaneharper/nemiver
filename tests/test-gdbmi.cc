#include <iostream>
#include <list>
#include <map>
#include <boost/test/unit_test.hpp>
#include "dbgengine/nmv-gdbmi-parser.h"
#include "common/nmv-exception.h"
#include "common/nmv-initializer.h"
#include "common/nmv-asm-utils.h"

using namespace std;
using namespace nemiver;

static const char* gv_str0 = "\"abracadabra\"";
static const char* gv_str1 = "\"/home/dodji/misc/no\\303\\253l-\\303\\251-\\303\\240/test.c\"";
static const char* gv_str2 = "\"No symbol \\\"events_ecal\\\" in current context.\\n\"";
static const char* gv_str3 = "\"Reading symbols from /home/dodji/devel/tests/éçà/test...\"";
static const char* gv_str4 = "\"\\\"Eins\\\"\"";
static const char* gv_attrs0 = "msg=\"No symbol \\\"g_return_if_fail\\\" in current context.\"";
static const char* gv_attrs1 = "script=[\"silent\",\"return\"]";

static const char* gv_stopped_async_output0 =
"*stopped,reason=\"breakpoint-hit\",bkptno=\"1\",thread-id=\"1\",frame={addr=\"0x0804afb0\",func=\"main\",args=[{name=\"argc\",value=\"1\"},{name=\"argv\",value=\"0xbfc79ed4\"}],file=\"today-main.c\",fullname=\"/home/dodji/devel/gitstore/omoko.git/applications/openmoko-today/src/today-main.c\",line=\"285\"}\n";

static const char* gv_stopped_async_output1 =
"*stopped,reason=\"breakpoint-hit\",bkptno=\"1\",thread-id=\"1\",frame={addr=\"0x08048d38\",func=\"main\",args=[],file=\"fooprog.cc\",fullname=\"/opt/dodji/git/nemiver.git/tests/fooprog.cc\",line=\"80\"}\n";

static const char *gv_running_async_output0 =
"*running,thread-id=\"all\"\n";

static const char *gv_running_async_output1 =
"*running,thread-id=\"1\"\n";

static const char *gv_var_list_children0="numchild=\"2\",displayhint=\"string\",children=[child={name=\"var1.public.m_first_name.public\",exp=\"public\",numchild=\"1\",value=\"\",thread-id=\"1\"},child={name=\"var1.public.m_first_name.private\",exp=\"private\",numchild=\"1\",value=\"\",thread-id=\"1\"}]";

static const char *gv_output_record0 =
"&\"Failed to read a valid object file image from memory.\\n\"\n"
"~\"[Thread debugging using libthread_db enabled]\\n\"\n"
"~\"[New Thread 0xb7892720 (LWP 20182)]\\n\"\n"
"=thread-created,id=1\n"
"*stopped,reason=\"breakpoint-hit\",bkptno=\"1\",thread-id=\"1\",frame={addr=\"0x08048d38\",func=\"main\",args=[],file=\"fooprog.cc\",fullname=\"/opt/dodji/git/nemiver.git/tests/fooprog.cc\",line=\"80\"}\n"
"(gdb)";

static const char *gv_output_record1 =
"&\"Failed to read a valid object file image from memory.\\n\"\n"
"~\"[Thread debugging using libthread_db enabled]\\n\"\n"
"~\"[New Thread 0xb7892720 (LWP 20182)]\\n\"\n"
"=thread-created,id=1\n"
"*running,thread-id=\"1\"n"
"(gdb)";

static const char *gv_output_record2=
"^done,value=\"{tree (int, int, int, tree, tree)} 0x483c2f <build_template_parm_index>\"\n"
"(gdb)";

static const char *gv_output_record3="^done,thread-ids={thread-id=\"1\"},current-thread-id=\"1\",number-of-threads=\"1\"\n";
static const char *gv_output_record4="^done,name=\"var1\",numchild=\"1\",value=\"{...}\",type=\"Person\"\n";

static const char *gv_output_record5="^done,ndeleted=\"2\"\n";

static const char *gv_output_record6="^done,numchild=\"3\",children=[child={name=\"var2.public.m_first_name\",exp=\"m_first_name\",numchild=\"2\",type=\"string\"},child={name=\"var2.public.m_family_name\",exp=\"m_family_name\",numchild=\"2\",type=\"string\"},child={name=\"var2.public.m_age\",exp=\"m_age\",numchild=\"0\",type=\"unsigned int\"}]\n";

static const char *gv_output_record7="^done,changelist=[{name=\"var1.public.m_first_name.public.npos\",value=\"1\",in_scope=\"true\",type_changed=\"false\"}]\n";

static const char *gv_output_record8="^done,changelist=[]\n";

static const char *gv_output_record9="^done,changelist=[{name=\"var1\",value=\"{...}\",in_scope=\"true\",type_changed=\"false\",new_num_children=\"2\",displayhint=\"array\",dynamic=\"1\",has_more=\"0\",new_children=[{name=\"var1.[1]\",exp=\"[1]\",numchild=\"0\",value=\" \\\"fila\\\"\",type=\"std::basic_string<char, std::char_traits<char>, std::allocator<char> >\",thread-id=\"1\",displayhint=\"string\",dynamic=\"1\"}]},{name=\"var1.[0]\",value=\"\\\"k\\303\\251l\\303\\251\\\"\",in_scope=\"true\",type_changed=\"false\",displayhint=\"array\",dynamic=\"1\",has_more=\"0\"}]\n";

static const char *gv_stack0 =
"stack=[frame={level=\"0\",addr=\"0x000000330f832f05\",func=\"raise\",file=\"../nptl/sysdeps/unix/sysv/linux/raise.c\",fullname=\"/usr/src/debug/glibc-20081113T2206/nptl/sysdeps/unix/sysv/linux/raise.c\",line=\"64\"},frame={level=\"1\",addr=\"0x000000330f834a73\",func=\"abort\",file=\"abort.c\",fullname=\"/usr/src/debug/glibc-20081113T2206/stdlib/abort.c\",line=\"88\"},frame={level=\"2\",addr=\"0x0000000000400872\",func=\"overflow_after_n_recursions\",file=\"do-stack-overflow.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/tests/do-stack-overflow.cc\",line=\"7\"},frame={level=\"3\",addr=\"0x000000000040087e\",func=\"overflow_after_n_recursions\",file=\"do-stack-overflow.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/tests/do-stack-overflow.cc\",line=\"8\"},frame={level=\"4\",addr=\"0x000000000040087e\",func=\"overflow_after_n_recursions\",file=\"do-stack-overflow.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/tests/do-stack-overflow.cc\",line=\"8\"},frame={level=\"5\",addr=\"0x000000000040087e\",func=\"overflow_after_n_recursions\",file=\"do-stack-overflow.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/tests/do-stack-overflow.cc\",line=\"8\"},frame={level=\"6\",addr=\"0x000000000040087e\",func=\"overflow_after_n_recursions\",file=\"do-stack-overflow.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/tests/do-stack-overflow.cc\",line=\"8\"},frame={level=\"7\",addr=\"0x000000000040087e\",func=\"overflow_after_n_recursions\",file=\"do-stack-overflow.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/tests/do-stack-overflow.cc\",line=\"8\"},frame={level=\"8\",addr=\"0x000000000040087e\",func=\"overflow_after_n_recursions\",file=\"do-stack-overflow.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/tests/do-stack-overflow.cc\",line=\"8\"},frame={level=\"9\",addr=\"0x000000000040087e\",func=\"overflow_after_n_recursions\",file=\"do-stack-overflow.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/tests/do-stack-overflow.cc\",line=\"8\"},frame={level=\"10\",addr=\"0x000000000040087e\",func=\"overflow_after_n_recursions\",file=\"do-stack-overflow.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/tests/do-stack-overflow.cc\",line=\"8\"},frame={level=\"11\",addr=\"0x000000000040087e\",func=\"overflow_after_n_recursions\",file=\"do-stack-overflow.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/tests/do-stack-overflow.cc\",line=\"8\"},frame={level=\"12\",addr=\"0x000000000040087e\",func=\"overflow_after_n_recursions\",file=\"do-stack-overflow.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/tests/do-stack-overflow.cc\",line=\"8\"},frame={level=\"13\",addr=\"0x000000000040087e\",func=\"overflow_after_n_recursions\",file=\"do-stack-overflow.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/tests/do-stack-overflow.cc\",line=\"8\"},frame={level=\"14\",addr=\"0x000000000040087e\",func=\"overflow_after_n_recursions\",file=\"do-stack-overflow.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/tests/do-stack-overflow.cc\",line=\"8\"},frame={level=\"15\",addr=\"0x000000000040087e\",func=\"overflow_after_n_recursions\",file=\"do-stack-overflow.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/tests/do-stack-overflow.cc\",line=\"8\"},frame={level=\"16\",addr=\"0x000000000040087e\",func=\"overflow_after_n_recursions\",file=\"do-stack-overflow.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/tests/do-stack-overflow.cc\",line=\"8\"},frame={level=\"17\",addr=\"0x000000000040087e\",func=\"overflow_after_n_recursions\",file=\"do-stack-overflow.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/tests/do-stack-overflow.cc\",line=\"8\"},frame={level=\"18\",addr=\"0x000000000040087e\",func=\"overflow_after_n_recursions\",file=\"do-stack-overflow.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/tests/do-stack-overflow.cc\",line=\"8\"},frame={level=\"19\",addr=\"0x000000000040087e\",func=\"overflow_after_n_recursions\",file=\"do-stack-overflow.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/tests/do-stack-overflow.cc\",line=\"8\"},frame={level=\"20\",addr=\"0x000000000040087e\",func=\"overflow_after_n_recursions\",file=\"do-stack-overflow.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/tests/do-stack-overflow.cc\",line=\"8\"},frame={level=\"21\",addr=\"0x000000000040087e\",func=\"overflow_after_n_recursions\",file=\"do-stack-overflow.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/tests/do-stack-overflow.cc\",line=\"8\"},frame={level=\"22\",addr=\"0x000000000040087e\",func=\"overflow_after_n_recursions\",file=\"do-stack-overflow.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/tests/do-stack-overflow.cc\",line=\"8\"},frame={level=\"23\",addr=\"0x000000000040087e\",func=\"overflow_after_n_recursions\",file=\"do-stack-overflow.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/tests/do-stack-overflow.cc\",line=\"8\"},frame={level=\"24\",addr=\"0x000000000040087e\",func=\"overflow_after_n_recursions\",file=\"do-stack-overflow.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/tests/do-stack-overflow.cc\",line=\"8\"},frame={level=\"25\",addr=\"0x000000000040087e\",func=\"overflow_after_n_recursions\",file=\"do-stack-overflow.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/tests/do-stack-overflow.cc\",line=\"8\"}]";

// the partial result of a gdbmi command: -stack-list-argument 1 command
// this command is used to implement IDebugger::list_frames_arguments()
static const char* gv_stack_arguments0 =
"stack-args=[frame={level=\"0\",args=[{name=\"a_param\",value=\"(Person &) @0xbf88fad4: {m_first_name = {static npos = 4294967295, _M_dataplus = {<std::allocator<char>> = {<__gnu_cxx::new_allocator<char>> = {<No data fields>}, <No data fields>}, _M_p = 0x804b144 \\\"Ali\\\"}}, m_family_name = {static npos = 4294967295, _M_dataplus = {<std::allocator<char>> = {<__gnu_cxx::new_allocator<char>> = {<No data fields>}, <No data fields>}, _M_p = 0x804b12c \\\"BABA\\\"}}, m_age = 15}\"}]},frame={level=\"1\",args=[]}]";

static const char* gv_local_vars =
"locals=[{name=\"person\",type=\"Person\"}]";

static const char* gv_emb_str =
"\\\"\\\\311\\\\303\\\\220U\\\\211\\\\345S\\\\203\\\\354\\\\024\\\\213E\\\\b\\\\211\\\\004$\\\\350\\\\202\\\\373\\\\377\\\\377\\\\213E\\\\b\\\\203\\\\300\\\\004\\\\211\\\\004$\\\\350t\\\\373\\\\377\\\\377\\\\213U\\\\b\\\\213E\\\\f\\\\211D$\\\\004\\\\211\\\\024$\\\\350\\\\002\\\\373\\\\377\\\\377\\\\213U\\\\b\\\\203\\\\302\\\\004\\\\213E\\\\020\\\\211D$\\\\004\\\\211\\\\024$\\\\350\\\\355\\\\372\\\\377\\\\377\\\\213U\\\\b\\\\213E\\\\024\\\\211B\\\\b\\\\3538\\\\211E\\\\370\\\\213]\\\\370\\\\213E\\\\b\\\\203\\\\300\\\\004\\\\211\\\\004$\\\\350\\\\276\\\\372\\\\377\\\\377\\\\211]\\\\370\\\\353\\\\003\\\\211E\\\\370\\\\213]\\\\370\\\\213E\\\\b\\\\211\\\\004$\\\\350\\\\250\\\\372\\\\377\\\\377\\\\211]\\\\370\\\\213E\\\\370\\\\211\\\\004$\\\\350\\\\032\\\\373\\\\377\\\\377\\\\203\\\\304\\\\024[]\\\\303U\\\\211\\\\345S\\\\203\\\\354$\\\\215E\\\\372\\\\211\\\\004$\\\\350\\\\\\\"\\\\373\\\\377\\\\377\\\\215E\\\\372\\\\211D$\\\\b\\\\307D$\\\\004\\\\370\\\\217\\\\004\\\\b\\\\215E\\\\364\\\\211\\\\004$\\\\350\\\\230\\\\372\\\\377\\\\377\\\\215E\\\\372\\\\211\\\\004$\\\\350}\\\\372\\\""

;
static const char* gv_member_var = 
"{static npos = 4294967295, _M_dataplus = {<std::allocator<char>> = {<__gnu_cxx::new_allocator<char>> = {<No data fields>}, <No data fields>}, _M_p = 0x8048ce1 \"\\311\\303\\220U\\211\\345S\\203\\354\\024\\213E\\b\\211\\004$\\350\\202\\373\\377\\377\\213E\\b\\203\\300\\004\\211\\004$\\350t\\373\\377\\377\\213U\\b\\213E\\f\\211D$\\004\\211\\024$\\350\\002\\373\\377\\377\\213U\\b\\203\\302\\004\\213E\\020\\211D$\\004\\211\\024$\\350\\355\\372\\377\\377\\213U\\b\\213E\\024\\211B\\b\\3538\\211E\\370\\213]\\370\\213E\\b\\203\\300\\004\\211\\004$\\350\\276\\372\\377\\377\\211]\\370\\353\\003\\211E\\370\\213]\\370\\213E\\b\\211\\004$\\350\\250\\372\\377\\377\\211]\\370\\213E\\370\\211\\004$\\350\\032\\373\\377\\377\\203\\304\\024[]\\303U\\211\\345S\\203\\354$\\215E\\372\\211\\004$\\350\\\"\\373\\377\\377\\215E\\372\\211D$\\b\\307D$\\004\\370\\217\\004\\b\\215E\\364\\211\\004$\\350\\230\\372\\377\\377\\215E\\372\\211\\004$\\350}\\372\"...}}";

static const char* gv_member_var2 = "{<com::sun::star::uno::BaseReference> = {_pInterface = 0x86a4834}, <No data fields>}";


static const char* gv_var_with_member = "value=\"{static npos = 4294967295, _M_dataplus = {<std::allocator<char>> = {<__gnu_cxx::new_allocator<char>> = {<No data fields>}, <No data fields>}, _M_p = 0x8048ce1 \"\\311\\303\\220U\\211\\345S\\203\\354\\024\\213E\\b\\211\\004$\\350\\202\\373\\377\\377\\213E\\b\\203\\300\\004\\211\\004$\\350t\\373\\377\\377\\213U\\b\\213E\\f\\211D$\\004\\211\\024$\\350\\002\\373\\377\\377\\213U\\b\\203\\302\\004\\213E\\020\\211D$\\004\\211\\024$\\350\\355\\372\\377\\377\\213U\\b\\213E\\024\\211B\\b\\3538\\211E\\370\\213]\\370\\213E\\b\\203\\300\\004\\211\\004$\\350\\276\\372\\377\\377\\211]\\370\\353\\003\\211E\\370\\213]\\370\\213E\\b\\211\\004$\\350\\250\\372\\377\\377\\211]\\370\\213E\\370\\211\\004$\\350\\032\\373\\377\\377\\203\\304\\024[]\\303U\\211\\345S\\203\\354$\\215E\\372\\211\\004$\\350\\\"\\373\\377\\377\\215E\\372\\211D$\\b\\307D$\\004\\370\\217\\004\\b\\215E\\364\\211\\004$\\350\\230\\372\\377\\377\\215E\\372\\211\\004$\\350}\\372\"...}}\"";

static const char *gv_var_with_member2 = "value=\"{filePos = 393216, ptr = 0xbf83c3f4 \\\"q\\\\354p1i\\\\233w\\\\310\\\\270R\\\\376\\\\325-\\\\266\\\\235$Qy\\\\212\\\\371\\\\373;|\\\\271\\\\031\\\\311\\\\355\\\\223]K\\\\031x4\\\\374\\\\217z\\\\272\\\\366t\\\\037\\\\2237'\\\\324S\\\\354\\\\321\\\\306\\\\020\\\\233\\\\202>y\\\\024\\\\365\\\\250\\\\\\\"\\\\271\\\\275(D\\\\267\\\\022\\\\205\\\\330B\\\\200\\\\371\\\\371k/\\\\252S\\\\204[\\\\265\\\\373\\\\036\\\\025\\\\fC\\\\251Y\\\\312\\\\333\\\\225\\\\231\\\\247$\\\\024-\\\\273\\\\035KsZV\\\\217r\\\\320I\\\\031gb\\\\347\\\\0371\\\\347\\\\374\\\\361I\\\\323\\\\204\\\\254\\\\337A\\\\271\\\\250\\\\302O\\\\271c)\\\\004\\\\211\\\\r\\\\303\\\\252\\\\273\\\\377\\\", limit = 0xbf85c3f4 \\\"\\\\310\\\\243\\\\020\\\\b\\\\330\\\\274\\\\021\\\\b\\\\f9\\\\020\\\\b\\\\f9\\\\020\\\\b\\\\344\\\\274\\\\022\\\\b\\\\377\\\\355\\\", len = 131072, data = {113 'q', 236 '\\\\354', 112 'p', 49 '1', 105 'i', 155 '\\\\233', 119 'w', 200 '\\\\310', 184 '\\\\270', 82 'R', 254 '\\\\376', 213 '\\\\325', 45 '-', 182 '\\\\266', 157 '\\\\235', 36 '$', 81 'Q', 121 'y', 138 '\\\\212', 249 '\\\\371', 251 '\\\\373', 59 ';', 124 '|', 185 '\\\\271', 25 '\\\\031', 201 '\\\\311', 237 '\\\\355', 147 '\\\\223', 93 ']', 75 'K', 25 '\\\\031', 120 'x', 52 '4', 252 '\\\\374', 143 '\\\\217', 122 'z', 186 '\\\\272', 246 '\\\\366', 116 't', 31 '\\\\037', 147 '\\\\223', 55 '7', 39 '\\\\'', 212 '\\\\324', 83 'S', 236 '\\\\354', 209 '\\\\321', 198 '\\\\306', 16 '\\\\020', 155 '\\\\233', 130 '\\\\202', 62 '>', 121 'y', 20 '\\\\024', 245 '\\\\365', 168 '\\\\250', 34 '\\\"', 185 '\\\\271', 189 '\\\\275', 40 '(', 68 'D', 183 '\\\\267', 18 '\\\\022', 133 '\\\\205', 216 '\\\\330', 66 'B', 128 '\\\\200', 249 '\\\\371', 249 '\\\\371', 107 'k', 47 '/', 170 '\\\\252', 83 'S', 132 '\\\\204', 91 '[', 181 '\\\\265', 251 '\\\\373', 30 '\\\\036', 21 '\\\\025', 12 '\\\\f', 67 'C', 169 '\\\\251', 89 'Y', 202 '\\\\312', 219 '\\\\333', 149 '\\\\225', 153 '\\\\231', 167 '\\\\247', 36 '$', 20 '\\\\024', 45 '-', 187 '\\\\273', 29 '\\\\035', 75 'K', 115 's', 90 'Z', 86 'V', 143 '\\\\217', 114 'r', 208 '\\\\320', 73 'I', 25 '\\\\031', 103 'g', 98 'b', 231 '\\\\347', 31 '\\\\037', 49 '1', 231 '\\\\347', 252 '\\\\374', 241 '\\\\361', 73 'I', 211 '\\\\323', 132 '\\\\204', 172 '\\\\254', 223 '\\\\337', 65 'A', 185 '\\\\271', 168 '\\\\250', 194 '\\\\302', 79 'O', 185 '\\\\271', 99 'c', 41 ')', 4 '\\\\004', 137 '\\\\211', 13 '\\\\r', 195 '\\\\303', 170 '\\\\252', 187 '\\\\273', 255 '\\\\377', 0 '\\\\0', 171 '\\\\253', 76 'L', 245 '\\\\365', 197 '\\\\305', 75 'K', 102 'f', 52 '4', 219 '\\\\333', 125 '}', 70 'F', 1 '\\\\001', 168 '\\\\250', 151 '\\\\227', 88 'X', 94 '^', 64 '@', 120 'x', 78 'N', 74 'J', 247 '\\\\367', 192 '\\\\300', 239 '\\\\357', 87 'W', 90 'Z', 85 'U', 35 '#', 23 '\\\\027', 202 '\\\\312', 190 '\\\\276', 37 '%', 160 '\\\\240', 158 '\\\\236', 95 '_', 81 'Q', 197 '\\\\305', 74 'J', 221 '\\\\335', 207 '\\\\317', 219 '\\\\333', 191 '\\\\277', 216 '\\\\330', 145 '\\\\221', 188 '\\\\274', 59 ';', 15 '\\\\017', 193 '\\\\301', 223 '\\\\337', 22 '\\\\026', 92 '\\\\\\\\', 248 '\\\\370', 83 'S', 69 'E', 254 '\\\\376', 215 '\\\\327', 191 '\\\\277', 215 '\\\\327', 53 '5', 47 '/', 179 '\\\\263', 177 '\\\\261', 212 '\\\\324', 192 '\\\\300', 138 '\\\\212', 37 '%', 85 'U', 81 'Q', 176 '\\\\260', 243 '\\\\363', 193 '\\\\301'...}}\"";
static const char *gv_var_with_member3 = "value=\"{<Gtk::Window> = {<Gtk::Bin> = {<Gtk::Container> = {<Gtk::Widget> = {<Gtk::Object> = {<Glib::Object> = {<Glib::ObjectBase> = {<sigc::trackable> = {callback_list_ = 0xb73e1ff4}, _vptr.ObjectBase = 0xb73e1ff4, gobject_ = 0x8051643, custom_type_name_ = 0x8050ef0 \\\"U\\\\211\\\\345WVS\\\\350O\\\", cpp_destruction_in_progress_ = 200}, _vptr.Object = 0xb73e1ff4, static object_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}}, static object_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}, referenced_ = 67, gobject_disposed_ = 22}, <Atk::Implementor> = {<Glib::Interface> = {_vptr.Interface = 0x8050ef0}, static implementor_class_ = {<Glib::Interface_Class> = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}, <No data fields>}}, static widget_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}}, static container_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}}, static bin_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}}, static window_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}, accel_group_ = {pCppObject_ = 0xbfd83fc8}}, m_VBox = {<Gtk::Box> = {<Gtk::Container> = {<Gtk::Widget> = {<Gtk::Object> = {<Glib::Object> = {<Glib::ObjectBase> = <invalid address>, _vptr.Object = 0xb7390906, static object_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}}, static object_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}, referenced_ = 67, gobject_disposed_ = 22}, <Atk::Implementor> = {<Glib::Interface> = {_vptr.Interface = 0x805164b}, static implementor_class_ = {<Glib::Interface_Class> = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}, <No data fields>}}, static widget_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}}, static container_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}}, static box_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}, children_proxy_ = {<Glib::HelperList<Gtk::Box_Helpers::Child,const Gtk::Box_Helpers::Element,Glib::List_Iterator<Gtk::Box_Helpers::Child> >> = {_vptr.HelperList = 0xbfd83fff, gparent_ = 0xb73e1f00}, <No data fields>}}, static vbox_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}}, m_MyWidget = {<Gtk::Widget> = {<Gtk::Object> = {<Glib::Object> = {<Glib::ObjectBase> = <invalid address>, _vptr.Object = 0xb71aaf55, static object_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}}, static object_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}, referenced_ = 172, gobject_disposed_ = 54}, <Atk::Implementor> = {<Glib::Interface> = {_vptr.Interface = 0x8056200}, static implementor_class_ = {<Glib::Interface_Class> = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}, <No data fields>}}, static widget_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}}, m_refGdkWindow = {pCppObject_ = 0xb7b3ed64}, m_scale = -1209413632}, m_ButtonBox = {<Gtk::ButtonBox> = {<Gtk::Box> = {<Gtk::Container> = {<Gtk::Widget> = {<Gtk::Object> = {<Glib::Object> = {<Glib::ObjectBase> = <invalid address>, _vptr.Object = 0xb71ab0d0, static object_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}}, static object_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}, referenced_ = 20, gobject_disposed_ = 65}, <Atk::Implementor> = {<Glib::Interface> = {_vptr.Interface = 0xb7fdace0}, static implementor_class_ = {<Glib::Interface_Class> = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}, <No data fields>}}, static widget_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}}, static container_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}}, static box_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}, children_proxy_ = {<Glib::HelperList<Gtk::Box_Helpers::Child,const Gtk::Box_Helpers::Element,Glib::List_Iterator<Gtk::Box_Helpers::Child> >> = {_vptr.HelperList = 0xbfd84028, gparent_ = 0x804da59}, <No data fields>}}, static buttonbox_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}}, static hbuttonbox_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}}, m_Button_Quit = {<Gtk::Bin> = {<Gtk::Container> = {<Gtk::Widget> = {<Gtk::Object> = {<Glib::Object> = {<error reading variable>}\"";

static const char *gv_var_with_member4 = "value=\"{ref_count = {ref_count = -1}, status = CAIRO_STATUS_NO_MEMORY, user_data = {size = 0, num_elements = 0, element_size = 0, elements = 0x0, is_snapshot = 0}, gstate = 0x0, gstate_tail = {{op = CAIRO_OPERATOR_CLEAR, tolerance = 0, antialias = CAIRO_ANTIALIAS_DEFAULT, stroke_style = {line_width = 0, line_cap = CAIRO_LINE_CAP_BUTT, line_join = CAIRO_LINE_JOIN_MITER, miter_limit = 0, dash = 0x0, num_dashes = 0, dash_offset = 0}, fill_rule = CAIRO_FILL_RULE_WINDING, font_face = 0x0, scaled_font = 0x0, font_matrix = {xx = 0, yx = 0, xy = 0, yy = 0, x0 = 0, y0 = 0}, font_options = {antialias = CAIRO_ANTIALIAS_DEFAULT, subpixel_order = CAIRO_SUBPIXEL_ORDER_DEFAULT, hint_style = CAIRO_HINT_STYLE_DEFAULT, hint_metrics = CAIRO_HINT_METRICS_DEFAULT}, clip = {mode = CAIRO_CLIP_MODE_PATH, all_clipped = 0, surface = 0x0, surface_rect = {x = 0, y = 0, width = 0, height = 0}, serial = 0, region = {rgn = {extents = {x1 = 0, y1 = 0, x2 = 0, y2 = 0}, data = 0x0}}, has_region = 0, path = 0x0}, target = 0x0, parent_target = 0x0, original_target = 0x0, ctm = {xx = 0, yx = 0, xy = 0, yy = 0, x0 = 0, y0 = 0}, ctm_inverse = {xx = 0, yx = 0, xy = 0, yy = 0, x0 = 0, y0 = 0}, source_ctm_inverse = {xx = 0, yx = 0, xy = 0, yy = 0, x0 = 0, y0 = 0}, source = 0x0, next = 0x0}}, path = {{last_move_point = {x = 0, y = 0}, current_point = {x = 0, y = 0}, has_current_point = 0, has_curve_to = 0, buf_tail = 0x0, buf_head = {base = {next = 0x0, prev = 0x0, buf_size = 0, num_ops = 0, num_points = 0, op = 0x0, points = 0x0}, op = '\\\\0' <repeats 26 times>, points = {{x = 0, y = 0} <repeats 54 times>}}}}}\"";

static const char *gv_var_with_member5 = "value=\"{{bmp = 32, encode = 32 ' '}, {bmp = 33, encode = 33 '!'}, {bmp = 35, encode = 35 '#'}, {bmp = 37, encode = 37 '%'}, {bmp = 38, encode = 38 '&'}, {bmp = 40, encode = 40 '('}, {bmp = 41, encode = 41 ')'}, {bmp = 43, encode = 43 '+'}, {bmp = 44, encode = 44 ','}, {bmp = 46, encode = 46 '.'}, {bmp = 47, encode = 47 '/'}, {bmp = 48, encode = 48 '0'}, {bmp = 49, encode = 49 '1'}, {bmp = 50, encode = 50 '2'}, {bmp = 51, encode = 51 '3'}, {bmp = 52, encode = 52 '4'}, {bmp = 53, encode = 53 '5'}, {bmp = 54, encode = 54 '6'}, {bmp = 55, encode = 55 '7'}, {bmp = 56, encode = 56 '8'}, {bmp = 57, encode = 57 '9'}, {bmp = 58, encode = 58 ':'}, {bmp = 59, encode = 59 ';'}, {bmp = 60, encode = 60 '<'}, {bmp = 61, encode = 61 '='}, {bmp = 62, encode = 62 '>'}, {bmp = 63, encode = 63 '?'}, {bmp = 91, encode = 91 '['}, {bmp = 93, encode = 93 ']'}, {bmp = 95, encode = 95 '_'}, {bmp = 123, encode = 123 '{'}, {bmp = 124, encode = 124 '|'}, {bmp = 125, encode = 125 '}'}, {bmp = 160, encode = 32 ' '}, {bmp = 172, encode = 216 '\\\\330'}, {bmp = 176, encode = 176 '\\\\260'}, {bmp = 177, encode = 177 '\\\\261'}, {bmp = 181, encode = 109 'm'}, {bmp = 215, encode = 180 '\\\\264'}, {bmp = 247, encode = 184 '\\\\270'}, {bmp = 402, encode = 166 '\\\\246'}, {bmp = 913, encode = 65 'A'}, {bmp = 914, encode = 66 'B'}, {bmp = 915, encode = 71 'G'}, {bmp = 916, encode = 68 'D'}, {bmp = 917, encode = 69 'E'}, {bmp = 918, encode = 90 'Z'}, {bmp = 919, encode = 72 'H'}, {bmp = 920, encode = 81 'Q'}, {bmp = 921, encode = 73 'I'}, {bmp = 922, encode = 75 'K'}, {bmp = 923, encode = 76 'L'}, {bmp = 924, encode = 77 'M'}, {bmp = 925, encode = 78 'N'}, {bmp = 926, encode = 88 'X'}, {bmp = 927, encode = 79 'O'}, {bmp = 928, encode = 80 'P'}, {bmp = 929, encode = 82 'R'}, {bmp = 931, encode = 83 'S'}, {bmp = 932, encode = 84 'T'}, {bmp = 933, encode = 85 'U'}, {bmp = 934, encode = 70 'F'}, {bmp = 935, encode = 67 'C'}, {bmp = 936, encode = 89 'Y'}, {bmp = 937, encode = 87 'W'}, {bmp = 945, encode = 97 'a'}, {bmp = 946, encode = 98 'b'}, {bmp = 947, encode = 103 'g'}, {bmp = 948, encode = 100 'd'}, {bmp = 949, encode = 101 'e'}, {bmp = 950, encode = 122 'z'}, {bmp = 951, encode = 104 'h'}, {bmp = 952, encode = 113 'q'}, {bmp = 953, encode = 105 'i'}, {bmp = 954, encode = 107 'k'}, {bmp = 955, encode = 108 'l'}, {bmp = 956, encode = 109 'm'}, {bmp = 957, encode = 110 'n'}, {bmp = 958, encode = 120 'x'}, {bmp = 959, encode = 111 'o'}, {bmp = 960, encode = 112 'p'}, {bmp = 961, encode = 114 'r'}, {bmp = 962, encode = 86 'V'}, {bmp = 963, encode = 115 's'}, {bmp = 964, encode = 116 't'}, {bmp = 965, encode = 117 'u'}, {bmp = 966, encode = 102 'f'}, {bmp = 967, encode = 99 'c'}, {bmp = 968, encode = 121 'y'}, {bmp = 969, encode = 119 'w'}, {bmp = 977, encode = 74 'J'}, {bmp = 978, encode = 161 '\\\\241'}, {bmp = 981, encode = 106 'j'}, {bmp = 982, encode = 118 'v'}, {bmp = 8226, encode = 183 '\\\\267'}, {bmp = 8230, encode = 188 '\\\\274'}, {bmp = 8242, encode = 162 '\\\\242'}, {bmp = 8243, encode = 178 '\\\\262'}, {bmp = 8260, encode = 164 '\\\\244'}, {bmp = 8364, encode = 160 '\\\\240'}, {bmp = 8465, encode = 193 '\\\\301'}, {bmp = 8472, encode = 195 '\\\\303'}, {bmp = 8476, encode = 194 '\\\\302'}, {bmp = 8486, encode = 87 'W'}, {bmp = 8501, encode = 192 '\\\\300'}, {bmp = 8592, encode = 172 '\\\\254'}, {bmp = 8593, encode = 173 '\\\\255'}, {bmp = 8594, encode = 174 '\\\\256'}, {bmp = 8595, encode = 175 '\\\\257'}, {bmp = 8596, encode = 171 '\\\\253'}, {bmp = 8629, encode = 191 '\\\\277'}, {bmp = 8656, encode = 220 '\\\\334'}, {bmp = 8657, encode = 221 '\\\\335'}, {bmp = 8658, encode = 222 '\\\\336'}, {bmp = 8659, encode = 223 '\\\\337'}, {bmp = 8660, encode = 219 '\\\\333'}, {bmp = 8704, encode = 34 '\\\"'}, {bmp = 8706, encode = 182 '\\\\266'}, {bmp = 8707, encode = 36 '$'}, {bmp = 8709, encode = 198 '\\\\306'}, {bmp = 8710, encode = 68 'D'}, {bmp = 8711, encode = 209 '\\\\321'}, {bmp = 8712, encode = 206 '\\\\316'}, {bmp = 8713, encode = 207 '\\\\317'}, {bmp = 8715, encode = 39 '\\\\''}, {bmp = 8719, encode = 213 '\\\\325'}, {bmp = 8721, encode = 229 '\\\\345'}, {bmp = 8722, encode = 45 '-'}, {bmp = 8725, encode = 164 '\\\\244'}, {bmp = 8727, encode = 42 '*'}, {bmp = 8730, encode = 214 '\\\\326'}, {bmp = 8733, encode = 181 '\\\\265'}, {bmp = 8734, encode = 165 '\\\\245'}, {bmp = 8736, encode = 208 '\\\\320'}, {bmp = 8743, encode = 217 '\\\\331'}, {bmp = 8744, encode = 218 '\\\\332'}, {bmp = 8745, encode = 199 '\\\\307'}, {bmp = 8746, encode = 200 '\\\\310'}, {bmp = 8747, encode = 242 '\\\\362'}, {bmp = 8756, encode = 92 '\\\\\\\\'}, {bmp = 8764, encode = 126 '~'}, {bmp = 8773, encode = 64 '@'}, {bmp = 8776, encode = 187 '\\\\273'}, {bmp = 8800, encode = 185 '\\\\271'}, {bmp = 8801, encode = 186 '\\\\272'}, {bmp = 8804, encode = 163 '\\\\243'}, {bmp = 8805, encode = 179 '\\\\263'}, {bmp = 8834, encode = 204 '\\\\314'}, {bmp = 8835, encode = 201 '\\\\311'}, {bmp = 8836, encode = 203 '\\\\313'}, {bmp = 8838, encode = 205 '\\\\315'}, {bmp = 8839, encode = 202 '\\\\312'}, {bmp = 8853, encode = 197 '\\\\305'}, {bmp = 8855, encode = 196 '\\\\304'}, {bmp = 8869, encode = 94 '^'}, {bmp = 8901, encode = 215 '\\\\327'}, {bmp = 8992, encode = 243 '\\\\363'}, {bmp = 8993, encode = 245 '\\\\365'}, {bmp = 9001, encode = 225 '\\\\341'}, {bmp = 9002, encode = 241 '\\\\361'}, {bmp = 9674, encode = 224 '\\\\340'}, {bmp = 9824, encode = 170 '\\\\252'}, {bmp = 9827, encode = 167 '\\\\247'}, {bmp = 9829, encode = 169 '\\\\251'}, {bmp = 9830, encode = 168 '\\\\250'}, {bmp = 63193, encode = 211 '\\\\323'}, {bmp = 63194, encode = 210 '\\\\322'}, {bmp = 63195, encode = 212 '\\\\324'}, {bmp = 63717, encode = 96 '`'}, {bmp = 63718, encode = 189 '\\\\275'}, {bmp = 63719, encode = 190 '\\\\276'}, {bmp = 63720, encode = 226 '\\\\342'}, {bmp = 63721, encode = 227 '\\\\343'}, {bmp = 63722, encode = 228 '\\\\344'}, {bmp = 63723, encode = 230 '\\\\346'}, {bmp = 63724, encode = 231 '\\\\347'}, {bmp = 63725, encode = 232 '\\\\350'}, {bmp = 63726, encode = 233 '\\\\351'}, {bmp = 63727, encode = 234 '\\\\352'}, {bmp = 63728, encode = 235 '\\\\353'}, {bmp = 63729, encode = 236 '\\\\354'}, {bmp = 63730, encode = 237 '\\\\355'}, {bmp = 63731, encode = 238 '\\\\356'}, {bmp = 63732, encode = 239 '\\\\357'}, {bmp = 63733, encode = 244 '\\\\364'}, {bmp = 63734, encode = 246 '\\\\366'}, {bmp = 63735, encode = 247 '\\\\367'}, {bmp = 63736, encode = 248 '\\\\370'}, {bmp = 63737, encode = 249 '\\\\371'}, {bmp = 63738, encode = 250 '\\\\372'}, {bmp = 63739, encode = 251 '\\\\373'}, {bmp = 63740, encode = 252 '\\\\374'}, {bmp = 63741, encode = 253 '\\\\375'}, {bmp = 63742, encode = 254 '\\\\376'}}\"";

static const char *gv_var_with_member6 = "value=\"{{notice_offset = 0 '\\\\0', foundry_offset = 8 '\\\\b'}, {notice_offset = 12 '\\\\f', foundry_offset = 18 '\\\\022'}, {notice_offset = 24 '\\\\030', foundry_offset = 34 '\\\"'}, {notice_offset = 44 ',', foundry_offset = 53 '5'}, {notice_offset = 62 '>', foundry_offset = 71 'G'}, {notice_offset = 80 'P', foundry_offset = 94 '^'}, {notice_offset = 103 'g', foundry_offset = 107 'k'}, {notice_offset = 111 'o', foundry_offset = 115 's'}, {notice_offset = 119 'w', foundry_offset = 154 '\\\\232'}, {notice_offset = 158 '\\\\236', foundry_offset = 173 '\\\\255'}, {notice_offset = 178 '\\\\262', foundry_offset = 186 '\\\\272'}, {notice_offset = 194 '\\\\302', foundry_offset = 204 '\\\\314'}, {notice_offset = 214 '\\\\326', foundry_offset = 220 '\\\\334'}, {notice_offset = 226 '\\\\342', foundry_offset = 233 '\\\\351'}, {notice_offset = 238 '\\\\356', foundry_offset = 253 '\\\\375'}}\"";

static const char *gv_var_with_member7 = "value=\"{{upper = 65, method = 0, count = 26, offset = 32}, {upper = 181, method = 0, count = 1, offset = 775}, {upper = 192, method = 0, count = 23, offset = 32}, {upper = 216, method = 0, count = 7, offset = 32}, {upper = 223, method = 2, count = 2, offset = 0}, {upper = 256, method = 1, count = 47, offset = 1}, {upper = 304, method = 2, count = 3, offset = 2}, {upper = 306, method = 1, count = 5, offset = 1}, {upper = 313, method = 1, count = 15, offset = 1}, {upper = 329, method = 2, count = 3, offset = 5}, {upper = 330, method = 1, count = 45, offset = 1}, {upper = 376, method = 0, count = 1, offset = -121}, {upper = 377, method = 1, count = 5, offset = 1}, {upper = 383, method = 0, count = 1, offset = -268}, {upper = 385, method = 0, count = 1, offset = 210}, {upper = 386, method = 1, count = 3, offset = 1}, {upper = 390, method = 0, count = 1, offset = 206}, {upper = 391, method = 1, count = 1, offset = 1}, {upper = 393, method = 0, count = 2, offset = 205}, {upper = 395, method = 1, count = 1, offset = 1}, {upper = 398, method = 0, count = 1, offset = 79}, {upper = 399, method = 0, count = 1, offset = 202}, {upper = 400, method = 0, count = 1, offset = 203}, {upper = 401, method = 1, count = 1, offset = 1}, {upper = 403, method = 0, count = 1, offset = 205}, {upper = 404, method = 0, count = 1, offset = 207}, {upper = 406, method = 0, count = 1, offset = 211}, {upper = 407, method = 0, count = 1, offset = 209}, {upper = 408, method = 1, count = 1, offset = 1}, {upper = 412, method = 0, count = 1, offset = 211}, {upper = 413, method = 0, count = 1, offset = 213}, {upper = 415, method = 0, count = 1, offset = 214}, {upper = 416, method = 1, count = 5, offset = 1}, {upper = 422, method = 0, count = 1, offset = 218}, {upper = 423, method = 1, count = 1, offset = 1}, {upper = 425, method = 0, count = 1, offset = 218}, {upper = 428, method = 1, count = 1, offset = 1}, {upper = 430, method = 0, count = 1, offset = 218}, {upper = 431, method = 1, count = 1, offset = 1}, {upper = 433, method = 0, count = 2, offset = 217}, {upper = 435, method = 1, count = 3, offset = 1}, {upper = 439, method = 0, count = 1, offset = 219}, {upper = 440, method = 1, count = 1, offset = 1}, {upper = 444, method = 1, count = 1, offset = 1}, {upper = 452, method = 0, count = 1, offset = 2}, {upper = 453, method = 1, count = 1, offset = 1}, {upper = 455, method = 0, count = 1, offset = 2}, {upper = 456, method = 1, count = 1, offset = 1}, {upper = 458, method = 0, count = 1, offset = 2}, {upper = 459, method = 1, count = 17, offset = 1}, {upper = 478, method = 1, count = 17, offset = 1}, {upper = 496, method = 2, count = 3, offset = 8}, {upper = 497, method = 0, count = 1, offset = 2}, {upper = 498, method = 1, count = 3, offset = 1}, {upper = 502, method = 0, count = 1, offset = -97}, {upper = 503, method = 0, count = 1, offset = -56}, {upper = 504, method = 1, count = 39, offset = 1}, {upper = 544, method = 0, count = 1, offset = -130}, {upper = 546, method = 1, count = 17, offset = 1}, {upper = 570, method = 0, count = 1, offset = 10795}, {upper = 571, method = 1, count = 1, offset = 1}, {upper = 573, method = 0, count = 1, offset = -163}, {upper = 574, method = 0, count = 1, offset = 10792}, {upper = 577, method = 1, count = 1, offset = 1}, {upper = 579, method = 0, count = 1, offset = -195}, {upper = 580, method = 0, count = 1, offset = 69}, {upper = 581, method = 0, count = 1, offset = 71}, {upper = 582, method = 1, count = 9, offset = 1}, {upper = 837, method = 0, count = 1, offset = 116}, {upper = 902, method = 0, count = 1, offset = 38}, {upper = 904, method = 0, count = 3, offset = 37}, {upper = 908, method = 0, count = 1, offset = 64}, {upper = 910, method = 0, count = 2, offset = 63}, {upper = 912, method = 2, count = 6, offset = 11}, {upper = 913, method = 0, count = 17, offset = 32}, {upper = 931, method = 0, count = 9, offset = 32}, {upper = 944, method = 2, count = 6, offset = 17}, {upper = 962, method = 1, count = 1, offset = 1}, {upper = 976, method = 0, count = 1, offset = -30}, {upper = 977, method = 0, count = 1, offset = -25}, {upper = 981, method = 0, count = 1, offset = -15}, {upper = 982, method = 0, count = 1, offset = -22}, {upper = 984, method = 1, count = 23, offset = 1}, {upper = 1008, method = 0, count = 1, offset = -54}, {upper = 1009, method = 0, count = 1, offset = -48}, {upper = 1012, method = 0, count = 1, offset = -60}, {upper = 1013, method = 0, count = 1, offset = -64}, {upper = 1015, method = 1, count = 1, offset = 1}, {upper = 1017, method = 0, count = 1, offset = -7}, {upper = 1018, method = 1, count = 1, offset = 1}, {upper = 1021, method = 0, count = 3, offset = -130}, {upper = 1024, method = 0, count = 16, offset = 80}, {upper = 1040, method = 0, count = 32, offset = 32}, {upper = 1120, method = 1, count = 33, offset = 1}, {upper = 1162, method = 1, count = 53, offset = 1}, {upper = 1216, method = 0, count = 1, offset = 15}, {upper = 1217, method = 1, count = 13, offset = 1}, {upper = 1232, method = 1, count = 67, offset = 1}, {upper = 1329, method = 0, count = 38, offset = 48}, {upper = 1415, method = 2, count = 4, offset = 23}, {upper = 4256, method = 0, count = 38, offset = 7264}, {upper = 7680, method = 1, count = 149, offset = 1}, {upper = 7830, method = 2, count = 3, offset = 27}, {upper = 7831, method = 2, count = 3, offset = 30}, {upper = 7832, method = 2, count = 3, offset = 33}, {upper = 7833, method = 2, count = 3, offset = 36}, {upper = 7834, method = 2, count = 3, offset = 39}, {upper = 7835, method = 0, count = 1, offset = -58}, {upper = 7840, method = 1, count = 89, offset = 1}, {upper = 7944, method = 0, count = 8, offset = -8}, {upper = 7960, method = 0, count = 6, offset = -8}, {upper = 7976, method = 0, count = 8, offset = -8}, {upper = 7992, method = 0, count = 8, offset = -8}, {upper = 8008, method = 0, count = 6, offset = -8}, {upper = 8016, method = 2, count = 4, offset = 42}, {upper = 8018, method = 2, count = 6, offset = 46}, {upper = 8020, method = 2, count = 6, offset = 52}, {upper = 8022, method = 2, count = 6, offset = 58}, {upper = 8025, method = 0, count = 1, offset = -8}, {upper = 8027, method = 0, count = 1, offset = -8}, {upper = 8029, method = 0, count = 1, offset = -8}, {upper = 8031, method = 0, count = 1, offset = -8}, {upper = 8040, method = 0, count = 8, offset = -8}, {upper = 8064, method = 2, count = 5, offset = 64}, {upper = 8065, method = 2, count = 5, offset = 69}, {upper = 8066, method = 2, count = 5, offset = 74}, {upper = 8067, method = 2, count = 5, offset = 79}, {upper = 8068, method = 2, count = 5, offset = 84}, {upper = 8069, method = 2, count = 5, offset = 89}, {upper = 8070, method = 2, count = 5, offset = 94}, {upper = 8071, method = 2, count = 5, offset = 99}, {upper = 8072, method = 2, count = 5, offset = 104}, {upper = 8073, method = 2, count = 5, offset = 109}, {upper = 8074, method = 2, count = 5, offset = 114}, {upper = 8075, method = 2, count = 5, offset = 119}, {upper = 8076, method = 2, count = 5, offset = 124}, {upper = 8077, method = 2, count = 5, offset = 129}, {upper = 8078, method = 2, count = 5, offset = 134}, {upper = 8079, method = 2, count = 5, offset = 139}, {upper = 8080, method = 2, count = 5, offset = 144}, {upper = 8081, method = 2, count = 5, offset = 149}, {upper = 8082, method = 2, count = 5, offset = 154}, {upper = 8083, method = 2, count = 5, offset = 159}, {upper = 8084, method = 2, count = 5, offset = 164}, {upper = 8085, method = 2, count = 5, offset = 169}, {upper = 8086, method = 2, count = 5, offset = 174}, {upper = 8087, method = 2, count = 5, offset = 179}, {upper = 8088, method = 2, count = 5, offset = 184}, {upper = 8089, method = 2, count = 5, offset = 189}, {upper = 8090, method = 2, count = 5, offset = 194}, {upper = 8091, method = 2, count = 5, offset = 199}, {upper = 8092, method = 2, count = 5, offset = 204}, {upper = 8093, method = 2, count = 5, offset = 209}, {upper = 8094, method = 2, count = 5, offset = 214}, {upper = 8095, method = 2, count = 5, offset = 219}, {upper = 8096, method = 2, count = 5, offset = 224}, {upper = 8097, method = 2, count = 5, offset = 229}, {upper = 8098, method = 2, count = 5, offset = 234}, {upper = 8099, method = 2, count = 5, offset = 239}, {upper = 8100, method = 2, count = 5, offset = 244}, {upper = 8101, method = 2, count = 5, offset = 249}, {upper = 8102, method = 2, count = 5, offset = 254}, {upper = 8103, method = 2, count = 5, offset = 259}, {upper = 8104, method = 2, count = 5, offset = 264}, {upper = 8105, method = 2, count = 5, offset = 269}, {upper = 8106, method = 2, count = 5, offset = 274}, {upper = 8107, method = 2, count = 5, offset = 279}, {upper = 8108, method = 2, count = 5, offset = 284}, {upper = 8109, method = 2, count = 5, offset = 289}, {upper = 8110, method = 2, count = 5, offset = 294}, {upper = 8111, method = 2, count = 5, offset = 299}, {upper = 8114, method = 2, count = 5, offset = 304}, {upper = 8115, method = 2, count = 4, offset = 309}, {upper = 8116, method = 2, count = 4, offset = 313}, {upper = 8118, method = 2, count = 4, offset = 317}, {upper = 8119, method = 2, count = 6, offset = 321}, {upper = 8120, method = 0, count = 2, offset = -8}, {upper = 8122, method = 0, count = 2, offset = -74}, {upper = 8124, method = 2, count = 4, offset = 327}, {upper = 8126, method = 0, count = 1, offset = -7173}, {upper = 8130, method = 2, count = 5, offset = 331}, {upper = 8131, method = 2, count = 4, offset = 336}, {upper = 8132, method = 2, count = 4, offset = 340}, {upper = 8134, method = 2, count = 4, offset = 344}, {upper = 8135, method = 2, count = 6, offset = 348}, {upper = 8136, method = 0, count = 4, offset = -86}, {upper = 8140, method = 2, count = 4, offset = 354}, {upper = 8146, method = 2, count = 6, offset = 358}, {upper = 8147, method = 2, count = 6, offset = 364}, {upper = 8150, method = 2, count = 4, offset = 370}, {upper = 8151, method = 2, count = 6, offset = 374}, {upper = 8152, method = 0, count = 2, offset = -8}, {upper = 8154, method = 0, count = 2, offset = -100}, {upper = 8162, method = 2, count = 6, offset = 380}, {upper = 8163, method = 2, count = 6, offset = 386}, {upper = 8164, method = 2, count = 4, offset = 392}, {upper = 8166, method = 2, count = 4, offset = 396}, {upper = 8167, method = 2, count = 6, offset = 400}, {upper = 8168, method = 0, count = 2, offset = -8}, {upper = 8170, method = 0, count = 2, offset = -112}...}\"";

// make sure we don't treat the comma within the <> as a separator for a new
// member variable
static const char *gv_var_with_member8 = "value=\"{member = 0x40085e <my_func(void*, void*)>}\"";
static const char *gv_var_with_comma = "value=\"0x40085e <my_func(void*, void*)>\"";

static const char* gv_stack_arguments1 =
"stack-args=[frame={level=\"0\",args=[{name=\"a_comp\",value=\"(icalcomponent *) 0x80596f8\"},{name=\"a_entry\",value=\"(MokoJEntry **) 0xbfe02178\"}]},frame={level=\"1\",args=[{name=\"a_view\",value=\"(ECalView *) 0x804ba60\"},{name=\"a_entries\",value=\"(GList *) 0x8054930\"},{name=\"a_journal\",value=\"(MokoJournal *) 0x8050580\"}]},frame={level=\"2\",args=[{name=\"closure\",value=\"(GClosure *) 0x805a010\"},{name=\"return_value\",value=\"(GValue *) 0x0\"},{name=\"n_param_values\",value=\"2\"},{name=\"param_values\",value=\"(const GValue *) 0xbfe023cc\"},{name=\"invocation_hint\",value=\"(gpointer) 0xbfe022dc\"},{name=\"marshal_data\",value=\"(gpointer) 0xb7f9a146\"}]},frame={level=\"3\",args=[{name=\"closure\",value=\"(GClosure *) 0x805a010\"},{name=\"return_value\",value=\"(GValue *) 0x0\"},{name=\"n_param_values\",value=\"2\"},{name=\"param_values\",value=\"(const GValue *) 0xbfe023cc\"},{name=\"invocation_hint\",value=\"(gpointer) 0xbfe022dc\"}]},frame={level=\"4\",args=[{name=\"node\",value=\"(SignalNode *) 0x80599c8\"},{name=\"detail\",value=\"0\"},{name=\"instance\",value=\"(gpointer) 0x804ba60\"},{name=\"emission_return\",value=\"(GValue *) 0x0\"},{name=\"instance_and_params\",value=\"(const GValue *) 0xbfe023cc\"}]},frame={level=\"5\",args=[{name=\"instance\",value=\"(gpointer) 0x804ba60\"},{name=\"signal_id\",value=\"18\"},{name=\"detail\",value=\"0\"},{name=\"var_args\",value=\"0xbfe02610 \\\"\\\\300\\\\365\\\\004\\\\b\\\\020,\\\\340\\\\277\\\\370\\\\024[\\\\001\\\\360\\\\226i\\\\267\\\\320`\\\\234\\\\267\\\\200\\\\237\\\\005\\\\bX&\\\\340\\\\277\\\\333cg\\\\267\\\\200{\\\\005\\\\b0I\\\\005\\\\b`\\\\272\\\\004\\\\b\\\\002\\\"\"}]},frame={level=\"6\",args=[{name=\"instance\",value=\"(gpointer) 0x804ba60\"},{name=\"signal_id\",value=\"18\"},{name=\"detail\",value=\"0\"}]},frame={level=\"7\",args=[{name=\"listener\",value=\"(ECalViewListener *) 0x8057b80\"},{name=\"objects\",value=\"(GList *) 0x8054930\"},{name=\"data\",value=\"(gpointer) 0x804ba60\"}]},frame={level=\"8\",args=[{name=\"closure\",value=\"(GClosure *) 0x8059f80\"},{name=\"return_value\",value=\"(GValue *) 0x0\"},{name=\"n_param_values\",value=\"2\"},{name=\"param_values\",value=\"(const GValue *) 0xbfe0286c\"},{name=\"invocation_hint\",value=\"(gpointer) 0xbfe0277c\"},{name=\"marshal_data\",value=\"(gpointer) 0xb79c60d0\"}]},frame={level=\"9\",args=[{name=\"closure\",value=\"(GClosure *) 0x8059f80\"},{name=\"return_value\",value=\"(GValue *) 0x0\"},{name=\"n_param_values\",value=\"2\"},{name=\"param_values\",value=\"(const GValue *) 0xbfe0286c\"},{name=\"invocation_hint\",value=\"(gpointer) 0xbfe0277c\"}]},frame={level=\"10\",args=[{name=\"node\",value=\"(SignalNode *) 0x8057a08\"},{name=\"detail\",value=\"0\"},{name=\"instance\",value=\"(gpointer) 0x8057b80\"},{name=\"emission_return\",value=\"(GValue *) 0x0\"},{name=\"instance_and_params\",value=\"(const GValue *) 0xbfe0286c\"}]},frame={level=\"11\",args=[{name=\"instance\",value=\"(gpointer) 0x8057b80\"},{name=\"signal_id\",value=\"12\"},{name=\"detail\",value=\"0\"},{name=\"var_args\",value=\"0xbfe02ab0 \\\"\\\\314,\\\\340\\\\277\\\\300m\\\\006\\\\b\\\\233d\\\\234\\\\267\\\\020\\\\347\\\\240\\\\267\\\\220d\\\\234\\\\267\\\\230m\\\\006\\\\b\\\\370*\\\\340\\\\277\\\\317i\\\\234\\\\267\\\\200{\\\\005\\\\b@n\\\\006\\\\b\\\\200*\\\\005\\\\b\\\"\"}]},frame={level=\"12\",args=[{name=\"instance\",value=\"(gpointer) 0x8057b80\"},{name=\"signal_id\",value=\"12\"},{name=\"detail\",value=\"0\"}]},frame={level=\"13\",args=[{name=\"ql\",value=\"(ECalViewListener *) 0x8057b80\"},{name=\"objects\",value=\"(char **) 0x8066e40\"},{name=\"context\",value=\"(DBusGMethodInvocation *) 0x8052a80\"}]},frame={level=\"14\",args=[{name=\"closure\",value=\"(GClosure *) 0xbfe02d1c\"},{name=\"return_value\",value=\"(GValue *) 0x0\"},{name=\"n_param_values\",value=\"3\"},{name=\"param_values\",value=\"(const GValue *) 0x8066d98\"},{name=\"invocation_hint\",value=\"(gpointer) 0x0\"},{name=\"marshal_data\",value=\"(gpointer) 0xb79c6490\"}]},frame={level=\"15\",args=[]},frame={level=\"16\",args=[]},frame={level=\"17\",args=[]}]";


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

static const char* gv_register_names=
"register-names=[\"eax\",\"ecx\",\"edx\",\"ebx\",\"esp\",\"ebp\",\"esi\",\"edi\",\"eip\",\"eflags\",\"cs\",\"ss\",\"ds\",\"es\",\"fs\",\"gs\",\"st0\",\"st1\",\"st2\",\"st3\",\"st4\",\"st5\",\"st6\",\"st7\",\"fctrl\",\"fstat\",\"ftag\",\"fiseg\",\"fioff\",\"foseg\",\"fooff\",\"fop\",\"xmm0\",\"xmm1\",\"xmm2\",\"xmm3\",\"xmm4\",\"xmm5\",\"xmm6\",\"xmm7\",\"mxcsr\",\"orig_eax\",\"mm0\",\"mm1\",\"mm2\",\"mm3\",\"mm4\",\"mm5\",\"mm6\",\"mm7\"]";

static const char* gv_changed_registers=
"changed-registers=[\"0\",\"1\",\"2\",\"3\",\"4\",\"5\",\"6\",\"8\",\"9\",\"10\",\"11\",\"12\",\"13\",\"15\",\"24\",\"26\",\"40\",\"41\"]";

static const char* gv_register_values =
"register-values=[{number=\"1\",value=\"0xbfd10a60\"},{number=\"2\",value=\"0x1\"},{number=\"3\",value=\"0xb6f03ff4\"},{number=\"4\",value=\"0xbfd10960\"},{number=\"5\",value=\"0xbfd10a48\"},{number=\"6\",value=\"0xb7ff6ce0\"},{number=\"7\",value=\"0x0\"},{number=\"8\",value=\"0x80bb710\"},{number=\"9\",value=\"0x200286\"},{number=\"10\",value=\"0x73\"},{number=\"36\",value=\"{v4_float = {0x0, 0x0, 0x0, 0x0}, v2_double = {0x0, 0x0}, v16_int8 = {0x0 <repeats 16 times>}, v8_int16 = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, v4_int32 = {0x0, 0x0, 0x0, 0x0}, v2_int64 = {0x0, 0x0}, uint128 = 0x00000000000000000000000000000000}\"}]";

static const char* gv_memory_values =
"addr=\"0x000013a0\",nr-bytes=\"32\",total-bytes=\"32\",next-row=\"0x000013c0\",prev-row=\"0x0000139c\",next-page=\"0x000013c0\",prev-page=\"0x00001380\",memory=[{addr=\"0x000013a0\",data=[\"0x10\",\"0x11\",\"0x12\",\"0x13\"],ascii=\"xxxx\"}]";

static const char* gv_gdbmi_result0 = "variable=[\"foo\", \"bar\"]";
static const char* gv_gdbmi_result1 = "variable";
static const char* gv_gdbmi_result2 = "\"variable\"";

static const char* gv_breakpoint_table0 =
"BreakpointTable={nr_rows=\"1\",nr_cols=\"6\",hdr=[{width=\"3\",alignment=\"-1\",col_name=\"number\",colhdr=\"Num\"},{width=\"14\",alignment=\"-1\",col_name=\"type\",colhdr=\"Type\"},{width=\"4\",alignment=\"-1\",col_name=\"disp\",colhdr=\"Disp\"},{width=\"3\",alignment=\"-1\",col_name=\"enabled\",colhdr=\"Enb\"},{width=\"10\",alignment=\"-1\",col_name=\"addr\",colhdr=\"Address\"},{width=\"40\",alignment=\"2\",col_name=\"what\",colhdr=\"What\"}],body=[bkpt={number=\"1\",type=\"breakpoint\",disp=\"keep\",enabled=\"y\",addr=\"0x08081566\",func=\"main\",file=\"main.cc\",fullname=\"/home/jonathon/Projects/agave.git/src/main.cc\",line=\"70\",times=\"0\"}]}";


static const char* gv_breakpoint_table1 =
"BreakpointTable={nr_rows=\"2\",nr_cols=\"6\",hdr=[{width=\"7\",alignment=\"-1\",col_name=\"number\",colhdr=\"Num\"},{width=\"14\",alignment=\"-1\",col_name=\"type\",colhdr=\"Type\"},{width=\"4\",alignment=\"-1\",col_name=\"disp\",colhdr=\"Disp\"},{width=\"3\",alignment=\"-1\",col_name=\"enabled\",colhdr=\"Enb\"},{width=\"10\",alignment=\"-1\",col_name=\"addr\",colhdr=\"Address\"},{width=\"40\",alignment=\"2\",col_name=\"what\",colhdr=\"What\"}],body=[bkpt={number=\"1\",type=\"breakpoint\",disp=\"keep\",enabled=\"y\",addr=\"0x0805e404\",func=\"main\",file=\"/usr/include/boost/test/minimal.hpp\",fullname=\"/usr/include/boost/test/minimal.hpp\",line=\"113\",times=\"1\"},bkpt={number=\"2\",type=\"breakpoint\",disp=\"keep\",enabled=\"y\",addr=\"<PENDING>\",pending=\"nemiver_common_create_dynamic_module_instance\",times=\"0\"}]}";

static const char* gv_breakpoint_table2 =
"BreakpointTable={nr_rows=\"5\",nr_cols=\"6\",hdr=[{width=\"3\",alignment=\"-1\",col_name=\"number\",colhdr=\"Num\"},{width=\"14\",alignment=\"-1\",col_name=\"type\",colhdr=\"Type\"},{width=\"4\",alignment=\"-1\",col_name=\"disp\",colhdr=\"Disp\"},{width=\"3\",alignment=\"-1\",col_name=\"enabled\",colhdr=\"Enb\"},{width=\"10\",alignment=\"-1\",col_name=\"addr\",colhdr=\"Address\"},{width=\"40\",alignment=\"2\",col_name=\"what\",colhdr=\"What\"}],body=[bkpt={number=\"1\",type=\"breakpoint\",disp=\"keep\",enabled=\"y\",addr=\"0x0806f8f8\",func=\"main\",file=\"main.cc\",fullname=\"/home/jonathon/Projects/agave.git/src/main.cc\",line=\"73\",times=\"1\"},bkpt={number=\"2\",type=\"breakpoint\",disp=\"keep\",enabled=\"y\",addr=\"<PENDING>\",pending=\"gcs::MainWindow::MainWindow()\",times=\"0\"},bkpt={number=\"3\",type=\"breakpoint\",disp=\"keep\",enabled=\"y\",addr=\"0x0805f91e\",func=\"gcs::MainWindow::Instance()\",file=\"gcs-mainwindow.cc\",fullname=\"/home/jonathon/Projects/agave.git/src/gcs-mainwindow.cc\",line=\"70\",times=\"1\"},bkpt={number=\"4\",type=\"breakpoint\",disp=\"keep\",enabled=\"y\",addr=\"0x0805f91e\",func=\"gcs::MainWindow::Instance()\",file=\"gcs-mainwindow.cc\",fullname=\"/home/jonathon/Projects/agave.git/src/gcs-mainwindow.cc\",line=\"70\",times=\"1\"},bkpt={number=\"5\",type=\"breakpoint\",disp=\"keep\",enabled=\"y\",addr=\"0x0805db44\",func=\"MainWindow\",file=\"gcs-mainwindow.cc\",fullname=\"/home/jonathon/Projects/agave.git/src/gcs-mainwindow.cc\",line=\"95\",times=\"0\"}]}";

static const char* gv_breakpoint_table3 =
"BreakpointTable={nr_rows=\"3\",nr_cols=\"6\",hdr=[{width=\"7\",alignment=\"-1\",col_name=\"number\",colhdr=\"Num\"},{width=\"14\",alignment=\"-1\",col_name=\"type\",colhdr=\"Type\"},{width=\"4\",alignment=\"-1\",col_name=\"disp\",colhdr=\"Disp\"},{width=\"3\",alignment=\"-1\",col_name=\"enabled\",colhdr=\"Enb\"},{width=\"18\",alignment=\"-1\",col_name=\"addr\",colhdr=\"Address\"},{width=\"40\",alignment=\"2\",col_name=\"what\",colhdr=\"What\"}],body=[bkpt={number=\"1\",type=\"breakpoint\",disp=\"keep\",enabled=\"y\",addr=\"0x000000000044f7d0\",func=\"internal_error\",file=\"utils.c\",fullname=\"/home/dodji/devel/git/archer.git/gdb/utils.c\",line=\"964\",times=\"0\"},bkpt={number=\"2\",type=\"breakpoint\",disp=\"keep\",enabled=\"y\",addr=\"0x000000000048ec10\",func=\"info_command\",file=\".././gdb/cli/cli-cmds.c\",fullname=\"/home/dodji/devel/git/archer.git/gdb/cli/cli-cmds.c\",line=\"198\",times=\"0\",script={\"silent\",\"return\"}},bkpt={number=\"3\",type=\"breakpoint\",disp=\"keep\",enabled=\"y\",addr=\"0x0000000000446ec0\",func=\"main\",file=\"gdb.c\",fullname=\"/home/dodji/devel/git/archer.git/gdb/gdb.c\",line=\"26\",times=\"1\"}]}";

static const char* gv_breakpoint_table4 =
"BreakpointTable={nr_rows=\"2\",nr_cols=\"6\",hdr=[{width=\"7\",alignment=\"-1\",col_name=\"number\",colhdr=\"Num\"},{width=\"14\",alignment=\"-1\",col_name=\"type\",colhdr=\"Type\"},{width=\"4\",alignment=\"-1\",col_name=\"disp\",colhdr=\"Disp\"},{width=\"3\",alignment=\"-1\",col_name=\"enabled\",colhdr=\"Enb\"},{width=\"18\",alignment=\"-1\",col_name=\"addr\",colhdr=\"Address\"},{width=\"40\",alignment=\"2\",col_name=\"what\",colhdr=\"What\"}],body=[bkpt={number=\"1\",type=\"breakpoint\",disp=\"keep\",enabled=\"y\",addr=\"0x000000000040aca0\",func=\"main\",file=\"main.cpp\",fullname=\"/home/philip/nemiver:temp/main.cpp\",line=\"77\",times=\"1\",original-location=\"main\"},bkpt={number=\"2\",type=\"breakpoint\",disp=\"keep\",enabled=\"y\",addr=\"<PENDING>\",pending=\"/home/philip/nemiver:temp/main.cpp:78\",times=\"0\",original-location=\"/home/philip/nemiver:temp/main.cpp:78\"}]}";

static const char* gv_breakpoint_table5 =
"BreakpointTable={nr_rows=\"2\",nr_cols=\"6\",hdr=[{width=\"7\",alignment=\"-1\",col_name=\"number\",colhdr=\"Num\"},{width=\"14\",alignment=\"-1\",col_name=\"type\",colhdr=\"Type\"},{width=\"4\",alignment=\"-1\",col_name=\"disp\",colhdr=\"Disp\"},{width=\"3\",alignment=\"-1\",col_name=\"enabled\",colhdr=\"Enb\"},{width=\"18\",alignment=\"-1\",col_name=\"addr\",colhdr=\"Address\"},{width=\"40\",alignment=\"2\",col_name=\"what\",colhdr=\"What\"}],body=[bkpt={number=\"1\",type=\"breakpoint\",disp=\"keep\",enabled=\"y\",addr=\"0x000000000040cb85\",func=\"main(int, char**)\",file=\"main.cc\",fullname=\"/home/dodji/devel/git/nemiver-branches.git/asm-support/src/main.cc\",line=\"489\",times=\"1\",original-location=\"main\"},bkpt={number=\"3\",type=\"breakpoint\",disp=\"keep\",enabled=\"y\",addr=\"<PENDING>\",file=\"nmv-dbg-perspective.cc\",line=\"4143\",times=\"0\",original-location=\"/home/dodji/devel/git/nemiver-branches.git/asm-support/src/persp/dbgperspective/nmv-dbg-perspective.cc:4143\"}]}";

static const char* gv_breakpoint0 =
"bkpt={number=\"2\",type=\"breakpoint\",disp=\"keep\",enabled=\"y\",addr=\"<MULTIPLE>\",times=\"0\",original-location=\"/home/dodji/devel/srcs/test.cc:36\"}";

static const char* gv_breakpoint1 =
"bkpt={number=\"3\",type=\"breakpoint\",disp=\"keep\",enabled=\"y\",addr=\"0x0000000000480210\",at=\"<exit@plt>\",times=\"0\",original-location=\"exit\"}";

static const char* gv_breakpoint2 =
"bkpt={number=\"2\",type=\"breakpoint\",disp=\"keep\",enabled=\"y\",addr=\"<PENDING>\",pending=\"/home/philip/nemiver:temp/main.cpp:78\",times=\"0\",original-location=\"/home/philip/nemiver:temp/main.cpp:78\"}";

static const char* gv_breakpoint3 =
    "bkpt={number=\"2\",type=\"breakpoint\",disp=\"keep\",enabled=\"y\",addr=\"<MULTIPLE>\",times=\"0\",original-location=\"error\"},{number=\"2.1\",enabled=\"y\",addr=\"0x000000000132d2f7\",func=\"error(char const*,...)\",file=\"/home/dodji/git/gcc/PR56782/gcc/diagnostic.c\",fullname=\"/home/dodji/git/gcc/PR56782/gcc/diagnostic.c\",line=\"1038\"},{number=\"2.2\",enabled=\"y\",addr=\"0x00000032026f1490\",at=\"<error>\"}";

static const char* gv_breakpoint_modified_async_output0 =
    "=breakpoint-modified,bkpt={number=\"2\",type=\"breakpoint\",disp=\"keep\",enabled=\"y\",addr=\"<MULTIPLE>\",times=\"0\",original-location=\"/home/dodji/git/libabigail/abi-diff/include/abg-diff-utils.h:1322\"},{number=\"2.1\",enabled=\"y\",addr=\"0x00007ffff7d70922\",func=\"abigail::diff_utils::compute_diff<__gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::base_spec> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::base_spec>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::base_spec> > > > >(__gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::base_spec> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::base_spec>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::base_spec> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::base_spec> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::base_spec>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::base_spec> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::base_spec> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::base_spec>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::base_spec> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::base_spec> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::base_spec>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::base_spec> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::base_spec> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::base_spec>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::base_spec> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::base_spec> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::base_spec>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::base_spec> > > >, std::vector<abigail::diff_utils::point, std::allocator<abigail::diff_utils::point> >&, abigail::diff_utils::edit_script&, int&)\",file=\"/home/dodji/git/libabigail/abi-diff/build/../include/abg-diff-utils.h\",fullname=\"/home/dodji/git/libabigail/abi-diff/include/abg-diff-utils.h\",line=\"1322\"},{number=\"2.2\",enabled=\"y\",addr=\"0x00007ffff7d71536\",func=\"abigail::diff_utils::compute_diff<__gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::member_type> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::member_type>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::member_type> > > > >(__gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::member_type> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::member_type>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::member_type> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::member_type> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::member_type>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::member_type> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::member_type> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::member_type>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::member_type> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::member_type> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::member_type>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::member_type> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::member_type> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::member_type>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::member_type> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::member_type> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::member_type>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::member_type> > > >, std::vector<abigail::diff_utils::point, std::allocator<abigail::diff_utils::point> >&, abigail::diff_utils::edit_script&, int&)\",file=\"/home/dodji/git/libabigail/abi-diff/build/../include/abg-diff-utils.h\",fullname=\"/home/dodji/git/libabigail/abi-diff/include/abg-diff-utils.h\",line=\"1322\"},{number=\"2.3\",enabled=\"y\",addr=\"0x00007ffff7d7214a\",func=\"abigail::diff_utils::compute_diff<__gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::data_member> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::data_member>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::data_member> > > > >(__gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::data_member> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::data_member>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::data_member> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::data_member> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::data_member>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::data_member> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::data_member> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::data_member>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::data_member> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::data_member> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::data_member>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::data_member> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::data_member> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::data_member>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::data_member> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::data_member> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::data_member>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::data_member> > > >, std::vector<abigail::diff_utils::point, std::allocator<abigail::diff_utils::point> >&, abigail::diff_utils::edit_script&, int&)\",file=\"/home/dodji/git/libabigail/abi-diff/build/../include/abg-diff-utils.h\",fullname=\"/home/dodji/git/libabigail/abi-diff/include/abg-diff-utils.h\",line=\"1322\"},{number=\"2.4\",enabled=\"y\",addr=\"0x00007ffff7d72d5e\",func=\"abigail::diff_utils::compute_diff<__gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::member_function> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::member_function>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::member_function> > > > >(__gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::member_function> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::member_function>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::member_function> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::member_function> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::member_function>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::member_function> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::member_function> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::member_function>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::member_function> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::member_function> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::member_function>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::member_function> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::member_function> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::member_function>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::member_function> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::member_function> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::member_function>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::member_function> > > >, std::vector<abigail::diff_utils::point, std::allocator<abigail::diff_utils::point> >&, abigail::diff_utils::edit_script&, int&)\",file=\"/home/dodji/git/libabigail/abi-diff/build/../include/abg-diff-utils.h\",fullname=\"/home/dodji/git/libabigail/abi-diff/include/abg-diff-utils.h\",line=\"1322\"},{number=\"2.5\",enabled=\"y\",addr=\"0x00007ffff7d73972\",func=\"abigail::diff_utils::compute_diff<__gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::member_function_template> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::member_function_template>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::member_function_template> > > > >(__gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::member_function_template> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::member_function_template>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::member_function_template> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::member_function_template> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::member_function_template>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::member_function_template> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::member_function_template> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::member_function_template>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::member_function_template> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::member_function_template> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::member_function_template>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::member_function_template> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::member_function_template> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::member_function_template>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::member_function_template> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::member_function_template> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::member_function_template>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::member_function_template> > > >, std::vector<abigail::diff_utils::point, std::allocator<abigail::diff_utils::point> >&, abigail::diff_utils::edit_script&, int&)\",file=\"/home/dodji/git/libabigail/abi-diff/build/../include/abg-diff-utils.h\",fullname=\"/home/dodji/git/libabigail/abi-diff/include/abg-diff-utils.h\",line=\"1322\"},{number=\"2.6\",enabled=\"y\",addr=\"0x00007ffff7d74586\",func=\"abigail::diff_utils::compute_diff<__gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::member_class_template> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::member_class_template>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::member_class_template> > > > >(__gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::member_class_template> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::member_class_template>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::member_class_template> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::member_class_template> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::member_class_template>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::member_class_template> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::member_class_template> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::member_class_template>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::member_class_template> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::member_class_template> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::member_class_template>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::member_class_template> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::member_class_template> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::member_class_template>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::member_class_template> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::class_decl::member_class_template> const*, std::vector<std::tr1::shared_ptr<abigail::class_decl::member_class_template>, std::allocator<std::tr1::shared_ptr<abigail::class_decl::member_class_template> > > >, std::vector<abigail::diff_utils::point, std::allocator<abigail::diff_utils::point> >&, abigail::diff_utils::edit_script&, int&)\",file=\"/home/dodji/git/libabigail/abi-diff/build/../include/abg-diff-utils.h\",fullname=\"/home/dodji/git/libabigail/abi-diff/include/abg-diff-utils.h\",line=\"1322\"},{number=\"2.7\",enabled=\"y\",addr=\"0x00007ffff7d75928\",func=\"abigail::diff_utils::compute_diff<__gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::decl_base> const*, std::vector<std::tr1::shared_ptr<abigail::decl_base>, std::allocator<std::tr1::shared_ptr<abigail::decl_base> > > > >(__gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::decl_base> const*, std::vector<std::tr1::shared_ptr<abigail::decl_base>, std::allocator<std::tr1::shared_ptr<abigail::decl_base> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::decl_base> const*, std::vector<std::tr1::shared_ptr<abigail::decl_base>, std::allocator<std::tr1::shared_ptr<abigail::decl_base> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::decl_base> const*, std::vector<std::tr1::shared_ptr<abigail::decl_base>, std::allocator<std::tr1::shared_ptr<abigail::decl_base> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::decl_base> const*, std::vector<std::tr1::shared_ptr<abigail::decl_base>, std::allocator<std::tr1::shared_ptr<abigail::decl_base> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::decl_base> const*, std::vector<std::tr1::shared_ptr<abigail::decl_base>, std::allocator<std::tr1::shared_ptr<abigail::decl_base> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::decl_base> const*, std::vector<std::tr1::shared_ptr<abigail::decl_base>, std::allocator<std::tr1::shared_ptr<abigail::decl_base> > > >, std::vector<abigail::diff_utils::point, std::allocator<abigail::diff_utils::point> >&, abigail::diff_utils::edit_script&, int&)\",file=\"/home/dodji/git/libabigail/abi-diff/build/../include/abg-diff-utils.h\",fullname=\"/home/dodji/git/libabigail/abi-diff/include/abg-diff-utils.h\",line=\"1322\"},{number=\"2.8\",enabled=\"y\",addr=\"0x00007ffff7d76f1a\",func=\"abigail::diff_utils::compute_diff<__gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::function_decl::parameter> const*, std::vector<std::tr1::shared_ptr<abigail::function_decl::parameter>, std::allocator<std::tr1::shared_ptr<abigail::function_decl::parameter> > > > >(__gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::function_decl::parameter> const*, std::vector<std::tr1::shared_ptr<abigail::function_decl::parameter>, std::allocator<std::tr1::shared_ptr<abigail::function_decl::parameter> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::function_decl::parameter> const*, std::vector<std::tr1::shared_ptr<abigail::function_decl::parameter>, std::allocator<std::tr1::shared_ptr<abigail::function_decl::parameter> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::function_decl::parameter> const*, std::vector<std::tr1::shared_ptr<abigail::function_decl::parameter>, std::allocator<std::tr1::shared_ptr<abigail::function_decl::parameter> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::function_decl::parameter> const*, std::vector<std::tr1::shared_ptr<abigail::function_decl::parameter>, std::allocator<std::tr1::shared_ptr<abigail::function_decl::parameter> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::function_decl::parameter> const*, std::vector<std::tr1::shared_ptr<abigail::function_decl::parameter>, std::allocator<std::tr1::shared_ptr<abigail::function_decl::parameter> > > >, __gnu_cxx::__normal_iterator<std::tr1::shared_ptr<abigail::function_decl::parameter> const*, std::vector<std::tr1::shared_ptr<abigail::function_decl::parameter>, std::allocator<std::tr1::shared_ptr<abigail::function_decl::parameter> > > >, std::vector<abigail::diff_utils::point, std::allocator<abigail::diff_utils::point> >&, abigail::diff_utils::edit_script&, int&)\",file=\"/home/dodji/git/libabigail/abi-diff/build/../include/abg-diff-utils.h\",fullname=\"/home/dodji/git/libabigail/abi-diff/include/abg-diff-utils.h\",line=\"1322\"},{number=\"2.9\",enabled=\"y\",addr=\"0x00007ffff7d77b2e\",func=\"abigail::diff_utils::compute_diff<__gnu_cxx::__normal_iterator<char*, std::vector<char, std::allocator<char> > > >(__gnu_cxx::__normal_iterator<char*, std::vector<char, std::allocator<char> > >, __gnu_cxx::__normal_iterator<char*, std::vector<char, std::allocator<char> > >, __gnu_cxx::__normal_iterator<char*, std::vector<char, std::allocator<char> > >, __gnu_cxx::__normal_iterator<char*, std::vector<char, std::allocator<char> > >, __gnu_cxx::__normal_iterator<char*, std::vector<char, std::allocator<char> > >, __gnu_cxx::__normal_iterator<char*, std::vector<char, std::allocator<char> > >, std::vector<abigail::diff_utils::point, std::allocator<abigail::diff_utils::point> >&, abigail::diff_utils::edit_script&, int&)\",file=\"/home/dodji/git/libabigail/abi-diff/build/../include/abg-diff-utils.h\",fullname=\"/home/dodji/git/libabigail/abi-diff/include/abg-diff-utils.h\",line=\"1322\"},{number=\"2.10\",enabled=\"y\",addr=\"0x00007ffff7d573c8\",func=\"abigail::diff_utils::compute_diff<char const*>(char const*, char const*, char const*, char const*, char const*, char const*, std::vector<abigail::diff_utils::point, std::allocator<abigail::diff_utils::point> >&, abigail::diff_utils::edit_script&, int&)\",file=\"/home/dodji/git/libabigail/abi-diff/build/../include/abg-diff-utils.h\",fullname=\"/home/dodji/git/libabigail/abi-diff/include/abg-diff-utils.h\",line=\"1322\"}";

 const char *gv_disassemble0 =
 "asm_insns=[{address=\"0x08048dc3\",func-name=\"main\",offset=\"0\",inst=\"lea    0x4(%esp),%ecx\"},{address=\"0x08048dc7\",func-name=\"main\",offset=\"4\",inst=\"and    $0xfffffff0,%esp\"},{address=\"0x08048dca\",func-name=\"main\",offset=\"7\",inst=\"pushl  -0x4(%ecx)\"},{address=\"0x08048dcd\",func-name=\"main\",offset=\"10\",inst=\"push   %ebp\"},{address=\"0x08048dce\",func-name=\"main\",offset=\"11\",inst=\"mov    %esp,%ebp\"},{address=\"0x08048dd0\",func-name=\"main\",offset=\"13\",inst=\"push   %esi\"},{address=\"0x08048dd1\",func-name=\"main\",offset=\"14\",inst=\"push   %ebx\"},{address=\"0x08048dd2\",func-name=\"main\",offset=\"15\",inst=\"push   %ecx\"},{address=\"0x08048dd3\",func-name=\"main\",offset=\"16\",inst=\"sub    $0x5c,%esp\"},{address=\"0x08048dd6\",func-name=\"main\",offset=\"19\",inst=\"lea    -0x25(%ebp),%eax\"},{address=\"0x08048dd9\",func-name=\"main\",offset=\"22\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048ddc\",func-name=\"main\",offset=\"25\",inst=\"call   0x8048be4 <_ZNSaIcEC1Ev@plt>\"},{address=\"0x08048de1\",func-name=\"main\",offset=\"30\",inst=\"lea    -0x25(%ebp),%eax\"},{address=\"0x08048de4\",func-name=\"main\",offset=\"33\",inst=\"mov    %eax,0x8(%esp)\"},{address=\"0x08048de8\",func-name=\"main\",offset=\"37\",inst=\"movl   $0x8049485,0x4(%esp)\"},{address=\"0x08048df0\",func-name=\"main\",offset=\"45\",inst=\"lea    -0x2c(%ebp),%eax\"},{address=\"0x08048df3\",func-name=\"main\",offset=\"48\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048df6\",func-name=\"main\",offset=\"51\",inst=\"call   0x8048b84 <_ZNSsC1EPKcRKSaIcE@plt>\"},{address=\"0x08048dfb\",func-name=\"main\",offset=\"56\",inst=\"lea    -0x1d(%ebp),%eax\"},{address=\"0x08048dfe\",func-name=\"main\",offset=\"59\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048e01\",func-name=\"main\",offset=\"62\",inst=\"call   0x8048be4 <_ZNSaIcEC1Ev@plt>\"},{address=\"0x08048e06\",func-name=\"main\",offset=\"67\",inst=\"lea    -0x1d(%ebp),%eax\"},{address=\"0x08048e09\",func-name=\"main\",offset=\"70\",inst=\"mov    %eax,0x8(%esp)\"},{address=\"0x08048e0d\",func-name=\"main\",offset=\"74\",inst=\"movl   $0x804948c,0x4(%esp)\"},{address=\"0x08048e15\",func-name=\"main\",offset=\"82\",inst=\"lea    -0x24(%ebp),%eax\"},{address=\"0x08048e18\",func-name=\"main\",offset=\"85\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048e1b\",func-name=\"main\",offset=\"88\",inst=\"call   0x8048b84 <_ZNSsC1EPKcRKSaIcE@plt>\"},{address=\"0x08048e20\",func-name=\"main\",offset=\"93\",inst=\"movl   $0xf,0xc(%esp)\"},{address=\"0x08048e28\",func-name=\"main\",offset=\"101\",inst=\"lea    -0x2c(%ebp),%eax\"},{address=\"0x08048e2b\",func-name=\"main\",offset=\"104\",inst=\"mov    %eax,0x8(%esp)\"},{address=\"0x08048e2f\",func-name=\"main\",offset=\"108\",inst=\"lea    -0x24(%ebp),%eax\"},{address=\"0x08048e32\",func-name=\"main\",offset=\"111\",inst=\"mov    %eax,0x4(%esp)\"},{address=\"0x08048e36\",func-name=\"main\",offset=\"115\",inst=\"lea    -0x38(%ebp),%eax\"},{address=\"0x08048e39\",func-name=\"main\",offset=\"118\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048e3c\",func-name=\"main\",offset=\"121\",inst=\"call   0x8049178 <Person>\"},{address=\"0x08048e41\",func-name=\"main\",offset=\"126\",inst=\"lea    -0x24(%ebp),%eax\"},{address=\"0x08048e44\",func-name=\"main\",offset=\"129\",inst=\"mov    %eax,-0x48(%ebp)\"},{address=\"0x08048e47\",func-name=\"main\",offset=\"132\",inst=\"mov    -0x48(%ebp),%eax\"},{address=\"0x08048e4a\",func-name=\"main\",offset=\"135\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048e4d\",func-name=\"main\",offset=\"138\",inst=\"call   0x8048bb4 <_ZNSsD1Ev@plt>\"},{address=\"0x08048e52\",func-name=\"main\",offset=\"143\",inst=\"jmp    0x8048e79 <main+182>\"},{address=\"0x08048e54\",func-name=\"main\",offset=\"145\",inst=\"mov    %eax,-0x54(%ebp)\"},{address=\"0x08048e57\",func-name=\"main\",offset=\"148\",inst=\"mov    %edx,-0x50(%ebp)\"},{address=\"0x08048e5a\",func-name=\"main\",offset=\"151\",inst=\"mov    -0x50(%ebp),%esi\"},{address=\"0x08048e5d\",func-name=\"main\",offset=\"154\",inst=\"mov    -0x54(%ebp),%ebx\"},{address=\"0x08048e60\",func-name=\"main\",offset=\"157\",inst=\"lea    -0x24(%ebp),%eax\"},{address=\"0x08048e63\",func-name=\"main\",offset=\"160\",inst=\"mov    %eax,-0x48(%ebp)\"},{address=\"0x08048e66\",func-name=\"main\",offset=\"163\",inst=\"mov    -0x48(%ebp),%eax\"},{address=\"0x08048e69\",func-name=\"main\",offset=\"166\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048e6c\",func-name=\"main\",offset=\"169\",inst=\"call   0x8048bb4 <_ZNSsD1Ev@plt>\"},{address=\"0x08048e71\",func-name=\"main\",offset=\"174\",inst=\"mov    %ebx,-0x54(%ebp)\"},{address=\"0x08048e74\",func-name=\"main\",offset=\"177\",inst=\"mov    %esi,-0x50(%ebp)\"},{address=\"0x08048e77\",func-name=\"main\",offset=\"180\",inst=\"jmp    0x8048ebc <main+249>\"},{address=\"0x08048e79\",func-name=\"main\",offset=\"182\",inst=\"lea    -0x1d(%ebp),%eax\"},{address=\"0x08048e7c\",func-name=\"main\",offset=\"185\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048e7f\",func-name=\"main\",offset=\"188\",inst=\"call   0x8048b74 <_ZNSaIcED1Ev@plt>\"},{address=\"0x08048e84\",func-name=\"main\",offset=\"193\",inst=\"lea    -0x2c(%ebp),%eax\"},{address=\"0x08048e87\",func-name=\"main\",offset=\"196\",inst=\"mov    %eax,-0x4c(%ebp)\"},{address=\"0x08048e8a\",func-name=\"main\",offset=\"199\",inst=\"mov    -0x4c(%ebp),%eax\"},{address=\"0x08048e8d\",func-name=\"main\",offset=\"202\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048e90\",func-name=\"main\",offset=\"205\",inst=\"call   0x8048bb4 <_ZNSsD1Ev@plt>\"},{address=\"0x08048e95\",func-name=\"main\",offset=\"210\",inst=\"jmp    0x8048ef2 <main+303>\"},{address=\"0x08048e97\",func-name=\"main\",offset=\"212\",inst=\"mov    %eax,-0x54(%ebp)\"},{address=\"0x08048e9a\",func-name=\"main\",offset=\"215\",inst=\"mov    %edx,-0x50(%ebp)\"},{address=\"0x08048e9d\",func-name=\"main\",offset=\"218\",inst=\"mov    -0x50(%ebp),%esi\"},{address=\"0x08048ea0\",func-name=\"main\",offset=\"221\",inst=\"mov    -0x54(%ebp),%ebx\"},{address=\"0x08048ea3\",func-name=\"main\",offset=\"224\",inst=\"lea    -0x38(%ebp),%eax\"},{address=\"0x08048ea6\",func-name=\"main\",offset=\"227\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048ea9\",func-name=\"main\",offset=\"230\",inst=\"call   0x804921a <~Person>\"},{address=\"0x08048eae\",func-name=\"main\",offset=\"235\",inst=\"mov    %ebx,-0x54(%ebp)\"},{address=\"0x08048eb1\",func-name=\"main\",offset=\"238\",inst=\"mov    %esi,-0x50(%ebp)\"},{address=\"0x08048eb4\",func-name=\"main\",offset=\"241\",inst=\"jmp    0x8048ebc <main+249>\"},{address=\"0x08048eb6\",func-name=\"main\",offset=\"243\",inst=\"mov    %eax,-0x54(%ebp)\"},{address=\"0x08048eb9\",func-name=\"main\",offset=\"246\",inst=\"mov    %edx,-0x50(%ebp)\"},{address=\"0x08048ebc\",func-name=\"main\",offset=\"249\",inst=\"mov    -0x50(%ebp),%esi\"},{address=\"0x08048ebf\",func-name=\"main\",offset=\"252\",inst=\"mov    -0x54(%ebp),%ebx\"},{address=\"0x08048ec2\",func-name=\"main\",offset=\"255\",inst=\"lea    -0x1d(%ebp),%eax\"},{address=\"0x08048ec5\",func-name=\"main\",offset=\"258\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048ec8\",func-name=\"main\",offset=\"261\",inst=\"call   0x8048b74 <_ZNSaIcED1Ev@plt>\"},{address=\"0x08048ecd\",func-name=\"main\",offset=\"266\",inst=\"mov    %ebx,-0x54(%ebp)\"},{address=\"0x08048ed0\",func-name=\"main\",offset=\"269\",inst=\"mov    %esi,-0x50(%ebp)\"},{address=\"0x08048ed3\",func-name=\"main\",offset=\"272\",inst=\"mov    -0x50(%ebp),%esi\"},{address=\"0x08048ed6\",func-name=\"main\",offset=\"275\",inst=\"mov    -0x54(%ebp),%ebx\"},{address=\"0x08048ed9\",func-name=\"main\",offset=\"278\",inst=\"lea    -0x2c(%ebp),%eax\"},{address=\"0x08048edc\",func-name=\"main\",offset=\"281\",inst=\"mov    %eax,-0x4c(%ebp)\"},{address=\"0x08048edf\",func-name=\"main\",offset=\"284\",inst=\"mov    -0x4c(%ebp),%eax\"},{address=\"0x08048ee2\",func-name=\"main\",offset=\"287\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048ee5\",func-name=\"main\",offset=\"290\",inst=\"call   0x8048bb4 <_ZNSsD1Ev@plt>\"},{address=\"0x08048eea\",func-name=\"main\",offset=\"295\",inst=\"mov    %ebx,-0x54(%ebp)\"},{address=\"0x08048eed\",func-name=\"main\",offset=\"298\",inst=\"mov    %esi,-0x50(%ebp)\"},{address=\"0x08048ef0\",func-name=\"main\",offset=\"301\",inst=\"jmp    0x8048f62 <main+415>\"},{address=\"0x08048ef2\",func-name=\"main\",offset=\"303\",inst=\"lea    -0x25(%ebp),%eax\"},{address=\"0x08048ef5\",func-name=\"main\",offset=\"306\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048ef8\",func-name=\"main\",offset=\"309\",inst=\"call   0x8048b74 <_ZNSaIcED1Ev@plt>\"},{address=\"0x08048efd\",func-name=\"main\",offset=\"314\",inst=\"call   0x8048cd4 <_Z5func1v>\"},{address=\"0x08048f02\",func-name=\"main\",offset=\"319\",inst=\"movl   $0x2,0x4(%esp)\"},{address=\"0x08048f0a\",func-name=\"main\",offset=\"327\",inst=\"movl   $0x1,(%esp)\"},{address=\"0x08048f11\",func-name=\"main\",offset=\"334\",inst=\"call   0x8048ce7 <_Z5func2ii>\"},{address=\"0x08048f16\",func-name=\"main\",offset=\"339\",inst=\"lea    -0x15(%ebp),%eax\"},{address=\"0x08048f19\",func-name=\"main\",offset=\"342\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048f1c\",func-name=\"main\",offset=\"345\",inst=\"call   0x8048be4 <_ZNSaIcEC1Ev@plt>\"},{address=\"0x08048f21\",func-name=\"main\",offset=\"350\",inst=\"lea    -0x15(%ebp),%eax\"},{address=\"0x08048f24\",func-name=\"main\",offset=\"353\",inst=\"mov    %eax,0x8(%esp)\"},{address=\"0x08048f28\",func-name=\"main\",offset=\"357\",inst=\"movl   $0x8049490,0x4(%esp)\"},{address=\"0x08048f30\",func-name=\"main\",offset=\"365\",inst=\"lea    -0x1c(%ebp),%eax\"},{address=\"0x08048f33\",func-name=\"main\",offset=\"368\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048f36\",func-name=\"main\",offset=\"371\",inst=\"call   0x8048b84 <_ZNSsC1EPKcRKSaIcE@plt>\"},{address=\"0x08048f3b\",func-name=\"main\",offset=\"376\",inst=\"jmp    0x8048f84 <main+449>\"},{address=\"0x08048f3d\",func-name=\"main\",offset=\"378\",inst=\"mov    %eax,-0x54(%ebp)\"},{address=\"0x08048f40\",func-name=\"main\",offset=\"381\",inst=\"mov    %edx,-0x50(%ebp)\"},{address=\"0x08048f43\",func-name=\"main\",offset=\"384\",inst=\"mov    -0x50(%ebp),%esi\"},{address=\"0x08048f46\",func-name=\"main\",offset=\"387\",inst=\"mov    -0x54(%ebp),%ebx\"},{address=\"0x08048f49\",func-name=\"main\",offset=\"390\",inst=\"lea    -0x38(%ebp),%eax\"},{address=\"0x08048f4c\",func-name=\"main\",offset=\"393\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048f4f\",func-name=\"main\",offset=\"396\",inst=\"call   0x804921a <~Person>\"},{address=\"0x08048f54\",func-name=\"main\",offset=\"401\",inst=\"mov    %ebx,-0x54(%ebp)\"},{address=\"0x08048f57\",func-name=\"main\",offset=\"404\",inst=\"mov    %esi,-0x50(%ebp)\"},{address=\"0x08048f5a\",func-name=\"main\",offset=\"407\",inst=\"jmp    0x8048f62 <main+415>\"},{address=\"0x08048f5c\",func-name=\"main\",offset=\"409\",inst=\"mov    %eax,-0x54(%ebp)\"},{address=\"0x08048f5f\",func-name=\"main\",offset=\"412\",inst=\"mov    %edx,-0x50(%ebp)\"},{address=\"0x08048f62\",func-name=\"main\",offset=\"415\",inst=\"mov    -0x50(%ebp),%esi\"},{address=\"0x08048f65\",func-name=\"main\",offset=\"418\",inst=\"mov    -0x54(%ebp),%ebx\"},{address=\"0x08048f68\",func-name=\"main\",offset=\"421\",inst=\"lea    -0x25(%ebp),%eax\"},{address=\"0x08048f6b\",func-name=\"main\",offset=\"424\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048f6e\",func-name=\"main\",offset=\"427\",inst=\"call   0x8048b74 <_ZNSaIcED1Ev@plt>\"},{address=\"0x08048f73\",func-name=\"main\",offset=\"432\",inst=\"mov    %ebx,-0x54(%ebp)\"},{address=\"0x08048f76\",func-name=\"main\",offset=\"435\",inst=\"mov    %esi,-0x50(%ebp)\"},{address=\"0x08048f79\",func-name=\"main\",offset=\"438\",inst=\"mov    -0x54(%ebp),%eax\"},{address=\"0x08048f7c\",func-name=\"main\",offset=\"441\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048f7f\",func-name=\"main\",offset=\"444\",inst=\"call   0x8048bd4 <_Unwind_Resume@plt>\"},{address=\"0x08048f84\",func-name=\"main\",offset=\"449\",inst=\"lea    -0x1c(%ebp),%eax\"},{address=\"0x08048f87\",func-name=\"main\",offset=\"452\",inst=\"mov    %eax,0x4(%esp)\"},{address=\"0x08048f8b\",func-name=\"main\",offset=\"456\",inst=\"lea    -0x38(%ebp),%eax\"},{address=\"0x08048f8e\",func-name=\"main\",offset=\"459\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048f91\",func-name=\"main\",offset=\"462\",inst=\"call   0x8049140 <_ZN6Person14set_first_nameERKSs>\"},{address=\"0x08048f96\",func-name=\"main\",offset=\"467\",inst=\"lea    -0x1c(%ebp),%eax\"},{address=\"0x08048f99\",func-name=\"main\",offset=\"470\",inst=\"mov    %eax,-0x44(%ebp)\"},{address=\"0x08048f9c\",func-name=\"main\",offset=\"473\",inst=\"mov    -0x44(%ebp),%eax\"},{address=\"0x08048f9f\",func-name=\"main\",offset=\"476\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048fa2\",func-name=\"main\",offset=\"479\",inst=\"call   0x8048bb4 <_ZNSsD1Ev@plt>\"},{address=\"0x08048fa7\",func-name=\"main\",offset=\"484\",inst=\"jmp    0x8048fce <main+523>\"},{address=\"0x08048fa9\",func-name=\"main\",offset=\"486\",inst=\"mov    %eax,-0x54(%ebp)\"},{address=\"0x08048fac\",func-name=\"main\",offset=\"489\",inst=\"mov    %edx,-0x50(%ebp)\"},{address=\"0x08048faf\",func-name=\"main\",offset=\"492\",inst=\"mov    -0x50(%ebp),%esi\"},{address=\"0x08048fb2\",func-name=\"main\",offset=\"495\",inst=\"mov    -0x54(%ebp),%ebx\"},{address=\"0x08048fb5\",func-name=\"main\",offset=\"498\",inst=\"lea    -0x1c(%ebp),%eax\"},{address=\"0x08048fb8\",func-name=\"main\",offset=\"501\",inst=\"mov    %eax,-0x44(%ebp)\"},{address=\"0x08048fbb\",func-name=\"main\",offset=\"504\",inst=\"mov    -0x44(%ebp),%eax\"},{address=\"0x08048fbe\",func-name=\"main\",offset=\"507\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048fc1\",func-name=\"main\",offset=\"510\",inst=\"call   0x8048bb4 <_ZNSsD1Ev@plt>\"},{address=\"0x08048fc6\",func-name=\"main\",offset=\"515\",inst=\"mov    %ebx,-0x54(%ebp)\"},{address=\"0x08048fc9\",func-name=\"main\",offset=\"518\",inst=\"mov    %esi,-0x50(%ebp)\"},{address=\"0x08048fcc\",func-name=\"main\",offset=\"521\",inst=\"jmp    0x8049006 <main+579>\"},{address=\"0x08048fce\",func-name=\"main\",offset=\"523\",inst=\"lea    -0x15(%ebp),%eax\"},{address=\"0x08048fd1\",func-name=\"main\",offset=\"526\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048fd4\",func-name=\"main\",offset=\"529\",inst=\"call   0x8048b74 <_ZNSaIcED1Ev@plt>\"},{address=\"0x08048fd9\",func-name=\"main\",offset=\"534\",inst=\"lea    -0xd(%ebp),%eax\"},{address=\"0x08048fdc\",func-name=\"main\",offset=\"537\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048fdf\",func-name=\"main\",offset=\"540\",inst=\"call   0x8048be4 <_ZNSaIcEC1Ev@plt>\"},{address=\"0x08048fe4\",func-name=\"main\",offset=\"545\",inst=\"lea    -0xd(%ebp),%eax\"},{address=\"0x08048fe7\",func-name=\"main\",offset=\"548\",inst=\"mov    %eax,0x8(%esp)\"},{address=\"0x08048feb\",func-name=\"main\",offset=\"552\",inst=\"movl   $0x8049494,0x4(%esp)\"},{address=\"0x08048ff3\",func-name=\"main\",offset=\"560\",inst=\"lea    -0x14(%ebp),%eax\"},{address=\"0x08048ff6\",func-name=\"main\",offset=\"563\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048ff9\",func-name=\"main\",offset=\"566\",inst=\"call   0x8048b84 <_ZNSsC1EPKcRKSaIcE@plt>\"},{address=\"0x08048ffe\",func-name=\"main\",offset=\"571\",inst=\"jmp    0x8049022 <main+607>\"},{address=\"0x08049000\",func-name=\"main\",offset=\"573\",inst=\"mov    %eax,-0x54(%ebp)\"},{address=\"0x08049003\",func-name=\"main\",offset=\"576\",inst=\"mov    %edx,-0x50(%ebp)\"},{address=\"0x08049006\",func-name=\"main\",offset=\"579\",inst=\"mov    -0x50(%ebp),%esi\"},{address=\"0x08049009\",func-name=\"main\",offset=\"582\",inst=\"mov    -0x54(%ebp),%ebx\"},{address=\"0x0804900c\",func-name=\"main\",offset=\"585\",inst=\"lea    -0x15(%ebp),%eax\"},{address=\"0x0804900f\",func-name=\"main\",offset=\"588\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08049012\",func-name=\"main\",offset=\"591\",inst=\"call   0x8048b74 <_ZNSaIcED1Ev@plt>\"},{address=\"0x08049017\",func-name=\"main\",offset=\"596\",inst=\"mov    %ebx,-0x54(%ebp)\"},{address=\"0x0804901a\",func-name=\"main\",offset=\"599\",inst=\"mov    %esi,-0x50(%ebp)\"},{address=\"0x0804901d\",func-name=\"main\",offset=\"602\",inst=\"jmp    0x80490fa <main+823>\"},{address=\"0x08049022\",func-name=\"main\",offset=\"607\",inst=\"lea    -0x14(%ebp),%eax\"},{address=\"0x08049025\",func-name=\"main\",offset=\"610\",inst=\"mov    %eax,0x4(%esp)\"},{address=\"0x08049029\",func-name=\"main\",offset=\"614\",inst=\"lea    -0x38(%ebp),%eax\"},{address=\"0x0804902c\",func-name=\"main\",offset=\"617\",inst=\"mov    %eax,(%esp)\"},{address=\"0x0804902f\",func-name=\"main\",offset=\"620\",inst=\"call   0x804915a <_ZN6Person15set_family_nameERKSs>\"},{address=\"0x08049034\",func-name=\"main\",offset=\"625\",inst=\"lea    -0x14(%ebp),%eax\"},{address=\"0x08049037\",func-name=\"main\",offset=\"628\",inst=\"mov    %eax,-0x40(%ebp)\"},{address=\"0x0804903a\",func-name=\"main\",offset=\"631\",inst=\"mov    -0x40(%ebp),%eax\"},{address=\"0x0804903d\",func-name=\"main\",offset=\"634\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08049040\",func-name=\"main\",offset=\"637\",inst=\"call   0x8048bb4 <_ZNSsD1Ev@plt>\"},{address=\"0x08049045\",func-name=\"main\",offset=\"642\",inst=\"jmp    0x804906c <main+681>\"},{address=\"0x08049047\",func-name=\"main\",offset=\"644\",inst=\"mov    %eax,-0x54(%ebp)\"},{address=\"0x0804904a\",func-name=\"main\",offset=\"647\",inst=\"mov    %edx,-0x50(%ebp)\"},{address=\"0x0804904d\",func-name=\"main\",offset=\"650\",inst=\"mov    -0x50(%ebp),%esi\"},{address=\"0x08049050\",func-name=\"main\",offset=\"653\",inst=\"mov    -0x54(%ebp),%ebx\"},{address=\"0x08049053\",func-name=\"main\",offset=\"656\",inst=\"lea    -0x14(%ebp),%eax\"},{address=\"0x08049056\",func-name=\"main\",offset=\"659\",inst=\"mov    %eax,-0x40(%ebp)\"},{address=\"0x08049059\",func-name=\"main\",offset=\"662\",inst=\"mov    -0x40(%ebp),%eax\"},{address=\"0x0804905c\",func-name=\"main\",offset=\"665\",inst=\"mov    %eax,(%esp)\"},{address=\"0x0804905f\",func-name=\"main\",offset=\"668\",inst=\"call   0x8048bb4 <_ZNSsD1Ev@plt>\"},{address=\"0x08049064\",func-name=\"main\",offset=\"673\",inst=\"mov    %ebx,-0x54(%ebp)\"},{address=\"0x08049067\",func-name=\"main\",offset=\"676\",inst=\"mov    %esi,-0x50(%ebp)\"},{address=\"0x0804906a\",func-name=\"main\",offset=\"679\",inst=\"jmp    0x804908a <main+711>\"},{address=\"0x0804906c\",func-name=\"main\",offset=\"681\",inst=\"lea    -0xd(%ebp),%eax\"},{address=\"0x0804906f\",func-name=\"main\",offset=\"684\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08049072\",func-name=\"main\",offset=\"687\",inst=\"call   0x8048b74 <_ZNSaIcED1Ev@plt>\"},{address=\"0x08049077\",func-name=\"main\",offset=\"692\",inst=\"lea    -0x38(%ebp),%eax\"},{address=\"0x0804907a\",func-name=\"main\",offset=\"695\",inst=\"mov    %eax,(%esp)\"},{address=\"0x0804907d\",func-name=\"main\",offset=\"698\",inst=\"call   0x8049272 <_ZN6Person7do_thisEv>\"},{address=\"0x08049082\",func-name=\"main\",offset=\"703\",inst=\"jmp    0x80490a3 <main+736>\"},{address=\"0x08049084\",func-name=\"main\",offset=\"705\",inst=\"mov    %eax,-0x54(%ebp)\"},{address=\"0x08049087\",func-name=\"main\",offset=\"708\",inst=\"mov    %edx,-0x50(%ebp)\"},{address=\"0x0804908a\",func-name=\"main\",offset=\"711\",inst=\"mov    -0x50(%ebp),%esi\"},{address=\"0x0804908d\",func-name=\"main\",offset=\"714\",inst=\"mov    -0x54(%ebp),%ebx\"},{address=\"0x08049090\",func-name=\"main\",offset=\"717\",inst=\"lea    -0xd(%ebp),%eax\"},{address=\"0x08049093\",func-name=\"main\",offset=\"720\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08049096\",func-name=\"main\",offset=\"723\",inst=\"call   0x8048b74 <_ZNSaIcED1Ev@plt>\"},{address=\"0x0804909b\",func-name=\"main\",offset=\"728\",inst=\"mov    %ebx,-0x54(%ebp)\"},{address=\"0x0804909e\",func-name=\"main\",offset=\"731\",inst=\"mov    %esi,-0x50(%ebp)\"},{address=\"0x080490a1\",func-name=\"main\",offset=\"734\",inst=\"jmp    0x80490fa <main+823>\"},{address=\"0x080490a3\",func-name=\"main\",offset=\"736\",inst=\"lea    -0x38(%ebp),%eax\"},{address=\"0x080490a6\",func-name=\"main\",offset=\"739\",inst=\"mov    %eax,(%esp)\"},{address=\"0x080490a9\",func-name=\"main\",offset=\"742\",inst=\"call   0x804911c <_ZN6Person8overloadEv>\"},{address=\"0x080490ae\",func-name=\"main\",offset=\"747\",inst=\"movl   $0x0,0x4(%esp)\"},{address=\"0x080490b6\",func-name=\"main\",offset=\"755\",inst=\"lea    -0x38(%ebp),%eax\"},{address=\"0x080490b9\",func-name=\"main\",offset=\"758\",inst=\"mov    %eax,(%esp)\"},{address=\"0x080490bc\",func-name=\"main\",offset=\"761\",inst=\"call   0x8049130 <_ZN6Person8overloadEi>\"},{address=\"0x080490c1\",func-name=\"main\",offset=\"766\",inst=\"lea    -0x38(%ebp),%eax\"},{address=\"0x080490c4\",func-name=\"main\",offset=\"769\",inst=\"mov    %eax,(%esp)\"},{address=\"0x080490c7\",func-name=\"main\",offset=\"772\",inst=\"call   0x8048db0 <_Z5func3R6Person>\"},{address=\"0x080490cc\",func-name=\"main\",offset=\"777\",inst=\"lea    -0x38(%ebp),%eax\"},{address=\"0x080490cf\",func-name=\"main\",offset=\"780\",inst=\"mov    %eax,(%esp)\"},{address=\"0x080490d2\",func-name=\"main\",offset=\"783\",inst=\"call   0x8048cff <_Z5func4R6Person>\"},{address=\"0x080490d7\",func-name=\"main\",offset=\"788\",inst=\"mov    $0x0,%ebx\"},{address=\"0x080490dc\",func-name=\"main\",offset=\"793\",inst=\"lea    -0x38(%ebp),%eax\"},{address=\"0x080490df\",func-name=\"main\",offset=\"796\",inst=\"mov    %eax,(%esp)\"},{address=\"0x080490e2\",func-name=\"main\",offset=\"799\",inst=\"call   0x804921a <~Person>\"},{address=\"0x080490e7\",func-name=\"main\",offset=\"804\",inst=\"mov    %ebx,%eax\"},{address=\"0x080490e9\",func-name=\"main\",offset=\"806\",inst=\"add    $0x5c,%esp\"},{address=\"0x080490ec\",func-name=\"main\",offset=\"809\",inst=\"pop    %ecx\"},{address=\"0x080490ed\",func-name=\"main\",offset=\"810\",inst=\"pop    %ebx\"},{address=\"0x080490ee\",func-name=\"main\",offset=\"811\",inst=\"pop    %esi\"},{address=\"0x080490ef\",func-name=\"main\",offset=\"812\",inst=\"pop    %ebp\"},{address=\"0x080490f0\",func-name=\"main\",offset=\"813\",inst=\"lea    -0x4(%ecx),%esp\"},{address=\"0x080490f3\",func-name=\"main\",offset=\"816\",inst=\"ret    \"},{address=\"0x080490f4\",func-name=\"main\",offset=\"817\",inst=\"mov    %eax,-0x54(%ebp)\"},{address=\"0x080490f7\",func-name=\"main\",offset=\"820\",inst=\"mov    %edx,-0x50(%ebp)\"},{address=\"0x080490fa\",func-name=\"main\",offset=\"823\",inst=\"mov    -0x50(%ebp),%esi\"},{address=\"0x080490fd\",func-name=\"main\",offset=\"826\",inst=\"mov    -0x54(%ebp),%ebx\"},{address=\"0x08049100\",func-name=\"main\",offset=\"829\",inst=\"lea    -0x38(%ebp),%eax\"},{address=\"0x08049103\",func-name=\"main\",offset=\"832\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08049106\",func-name=\"main\",offset=\"835\",inst=\"call   0x804921a <~Person>\"},{address=\"0x0804910b\",func-name=\"main\",offset=\"840\",inst=\"mov    %ebx,-0x54(%ebp)\"},{address=\"0x0804910e\",func-name=\"main\",offset=\"843\",inst=\"mov    %esi,-0x50(%ebp)\"},{address=\"0x08049111\",func-name=\"main\",offset=\"846\",inst=\"mov    -0x54(%ebp),%eax\"},{address=\"0x08049114\",func-name=\"main\",offset=\"849\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08049117\",func-name=\"main\",offset=\"852\",inst=\"call   0x8048bd4 <_Unwind_Resume@plt>\"}]";
  
 const char *gv_disassemble1 =
 "^done,asm_insns=[{address=\"0x08048dc3\",func-name=\"main\",offset=\"0\",inst=\"lea    0x4(%esp),%ecx\"},{address=\"0x08048dc7\",func-name=\"main\",offset=\"4\",inst=\"and    $0xfffffff0,%esp\"},{address=\"0x08048dca\",func-name=\"main\",offset=\"7\",inst=\"pushl  -0x4(%ecx)\"},{address=\"0x08048dcd\",func-name=\"main\",offset=\"10\",inst=\"push   %ebp\"},{address=\"0x08048dce\",func-name=\"main\",offset=\"11\",inst=\"mov    %esp,%ebp\"},{address=\"0x08048dd0\",func-name=\"main\",offset=\"13\",inst=\"push   %esi\"},{address=\"0x08048dd1\",func-name=\"main\",offset=\"14\",inst=\"push   %ebx\"},{address=\"0x08048dd2\",func-name=\"main\",offset=\"15\",inst=\"push   %ecx\"},{address=\"0x08048dd3\",func-name=\"main\",offset=\"16\",inst=\"sub    $0x5c,%esp\"},{address=\"0x08048dd6\",func-name=\"main\",offset=\"19\",inst=\"lea    -0x25(%ebp),%eax\"},{address=\"0x08048dd9\",func-name=\"main\",offset=\"22\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048ddc\",func-name=\"main\",offset=\"25\",inst=\"call   0x8048be4 <_ZNSaIcEC1Ev@plt>\"},{address=\"0x08048de1\",func-name=\"main\",offset=\"30\",inst=\"lea    -0x25(%ebp),%eax\"},{address=\"0x08048de4\",func-name=\"main\",offset=\"33\",inst=\"mov    %eax,0x8(%esp)\"},{address=\"0x08048de8\",func-name=\"main\",offset=\"37\",inst=\"movl   $0x8049485,0x4(%esp)\"},{address=\"0x08048df0\",func-name=\"main\",offset=\"45\",inst=\"lea    -0x2c(%ebp),%eax\"},{address=\"0x08048df3\",func-name=\"main\",offset=\"48\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048df6\",func-name=\"main\",offset=\"51\",inst=\"call   0x8048b84 <_ZNSsC1EPKcRKSaIcE@plt>\"},{address=\"0x08048dfb\",func-name=\"main\",offset=\"56\",inst=\"lea    -0x1d(%ebp),%eax\"},{address=\"0x08048dfe\",func-name=\"main\",offset=\"59\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048e01\",func-name=\"main\",offset=\"62\",inst=\"call   0x8048be4 <_ZNSaIcEC1Ev@plt>\"},{address=\"0x08048e06\",func-name=\"main\",offset=\"67\",inst=\"lea    -0x1d(%ebp),%eax\"},{address=\"0x08048e09\",func-name=\"main\",offset=\"70\",inst=\"mov    %eax,0x8(%esp)\"},{address=\"0x08048e0d\",func-name=\"main\",offset=\"74\",inst=\"movl   $0x804948c,0x4(%esp)\"},{address=\"0x08048e15\",func-name=\"main\",offset=\"82\",inst=\"lea    -0x24(%ebp),%eax\"},{address=\"0x08048e18\",func-name=\"main\",offset=\"85\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048e1b\",func-name=\"main\",offset=\"88\",inst=\"call   0x8048b84 <_ZNSsC1EPKcRKSaIcE@plt>\"},{address=\"0x08048e20\",func-name=\"main\",offset=\"93\",inst=\"movl   $0xf,0xc(%esp)\"},{address=\"0x08048e28\",func-name=\"main\",offset=\"101\",inst=\"lea    -0x2c(%ebp),%eax\"},{address=\"0x08048e2b\",func-name=\"main\",offset=\"104\",inst=\"mov    %eax,0x8(%esp)\"},{address=\"0x08048e2f\",func-name=\"main\",offset=\"108\",inst=\"lea    -0x24(%ebp),%eax\"},{address=\"0x08048e32\",func-name=\"main\",offset=\"111\",inst=\"mov    %eax,0x4(%esp)\"},{address=\"0x08048e36\",func-name=\"main\",offset=\"115\",inst=\"lea    -0x38(%ebp),%eax\"},{address=\"0x08048e39\",func-name=\"main\",offset=\"118\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048e3c\",func-name=\"main\",offset=\"121\",inst=\"call   0x8049178 <Person>\"},{address=\"0x08048e41\",func-name=\"main\",offset=\"126\",inst=\"lea    -0x24(%ebp),%eax\"},{address=\"0x08048e44\",func-name=\"main\",offset=\"129\",inst=\"mov    %eax,-0x48(%ebp)\"},{address=\"0x08048e47\",func-name=\"main\",offset=\"132\",inst=\"mov    -0x48(%ebp),%eax\"},{address=\"0x08048e4a\",func-name=\"main\",offset=\"135\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048e4d\",func-name=\"main\",offset=\"138\",inst=\"call   0x8048bb4 <_ZNSsD1Ev@plt>\"},{address=\"0x08048e52\",func-name=\"main\",offset=\"143\",inst=\"jmp    0x8048e79 <main+182>\"},{address=\"0x08048e54\",func-name=\"main\",offset=\"145\",inst=\"mov    %eax,-0x54(%ebp)\"},{address=\"0x08048e57\",func-name=\"main\",offset=\"148\",inst=\"mov    %edx,-0x50(%ebp)\"},{address=\"0x08048e5a\",func-name=\"main\",offset=\"151\",inst=\"mov    -0x50(%ebp),%esi\"},{address=\"0x08048e5d\",func-name=\"main\",offset=\"154\",inst=\"mov    -0x54(%ebp),%ebx\"},{address=\"0x08048e60\",func-name=\"main\",offset=\"157\",inst=\"lea    -0x24(%ebp),%eax\"},{address=\"0x08048e63\",func-name=\"main\",offset=\"160\",inst=\"mov    %eax,-0x48(%ebp)\"},{address=\"0x08048e66\",func-name=\"main\",offset=\"163\",inst=\"mov    -0x48(%ebp),%eax\"},{address=\"0x08048e69\",func-name=\"main\",offset=\"166\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048e6c\",func-name=\"main\",offset=\"169\",inst=\"call   0x8048bb4 <_ZNSsD1Ev@plt>\"},{address=\"0x08048e71\",func-name=\"main\",offset=\"174\",inst=\"mov    %ebx,-0x54(%ebp)\"},{address=\"0x08048e74\",func-name=\"main\",offset=\"177\",inst=\"mov    %esi,-0x50(%ebp)\"},{address=\"0x08048e77\",func-name=\"main\",offset=\"180\",inst=\"jmp    0x8048ebc <main+249>\"},{address=\"0x08048e79\",func-name=\"main\",offset=\"182\",inst=\"lea    -0x1d(%ebp),%eax\"},{address=\"0x08048e7c\",func-name=\"main\",offset=\"185\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048e7f\",func-name=\"main\",offset=\"188\",inst=\"call   0x8048b74 <_ZNSaIcED1Ev@plt>\"},{address=\"0x08048e84\",func-name=\"main\",offset=\"193\",inst=\"lea    -0x2c(%ebp),%eax\"},{address=\"0x08048e87\",func-name=\"main\",offset=\"196\",inst=\"mov    %eax,-0x4c(%ebp)\"},{address=\"0x08048e8a\",func-name=\"main\",offset=\"199\",inst=\"mov    -0x4c(%ebp),%eax\"},{address=\"0x08048e8d\",func-name=\"main\",offset=\"202\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048e90\",func-name=\"main\",offset=\"205\",inst=\"call   0x8048bb4 <_ZNSsD1Ev@plt>\"},{address=\"0x08048e95\",func-name=\"main\",offset=\"210\",inst=\"jmp    0x8048ef2 <main+303>\"},{address=\"0x08048e97\",func-name=\"main\",offset=\"212\",inst=\"mov    %eax,-0x54(%ebp)\"},{address=\"0x08048e9a\",func-name=\"main\",offset=\"215\",inst=\"mov    %edx,-0x50(%ebp)\"},{address=\"0x08048e9d\",func-name=\"main\",offset=\"218\",inst=\"mov    -0x50(%ebp),%esi\"},{address=\"0x08048ea0\",func-name=\"main\",offset=\"221\",inst=\"mov    -0x54(%ebp),%ebx\"},{address=\"0x08048ea3\",func-name=\"main\",offset=\"224\",inst=\"lea    -0x38(%ebp),%eax\"},{address=\"0x08048ea6\",func-name=\"main\",offset=\"227\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048ea9\",func-name=\"main\",offset=\"230\",inst=\"call   0x804921a <~Person>\"},{address=\"0x08048eae\",func-name=\"main\",offset=\"235\",inst=\"mov    %ebx,-0x54(%ebp)\"},{address=\"0x08048eb1\",func-name=\"main\",offset=\"238\",inst=\"mov    %esi,-0x50(%ebp)\"},{address=\"0x08048eb4\",func-name=\"main\",offset=\"241\",inst=\"jmp    0x8048ebc <main+249>\"},{address=\"0x08048eb6\",func-name=\"main\",offset=\"243\",inst=\"mov    %eax,-0x54(%ebp)\"},{address=\"0x08048eb9\",func-name=\"main\",offset=\"246\",inst=\"mov    %edx,-0x50(%ebp)\"},{address=\"0x08048ebc\",func-name=\"main\",offset=\"249\",inst=\"mov    -0x50(%ebp),%esi\"},{address=\"0x08048ebf\",func-name=\"main\",offset=\"252\",inst=\"mov    -0x54(%ebp),%ebx\"},{address=\"0x08048ec2\",func-name=\"main\",offset=\"255\",inst=\"lea    -0x1d(%ebp),%eax\"},{address=\"0x08048ec5\",func-name=\"main\",offset=\"258\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048ec8\",func-name=\"main\",offset=\"261\",inst=\"call   0x8048b74 <_ZNSaIcED1Ev@plt>\"},{address=\"0x08048ecd\",func-name=\"main\",offset=\"266\",inst=\"mov    %ebx,-0x54(%ebp)\"},{address=\"0x08048ed0\",func-name=\"main\",offset=\"269\",inst=\"mov    %esi,-0x50(%ebp)\"},{address=\"0x08048ed3\",func-name=\"main\",offset=\"272\",inst=\"mov    -0x50(%ebp),%esi\"},{address=\"0x08048ed6\",func-name=\"main\",offset=\"275\",inst=\"mov    -0x54(%ebp),%ebx\"},{address=\"0x08048ed9\",func-name=\"main\",offset=\"278\",inst=\"lea    -0x2c(%ebp),%eax\"},{address=\"0x08048edc\",func-name=\"main\",offset=\"281\",inst=\"mov    %eax,-0x4c(%ebp)\"},{address=\"0x08048edf\",func-name=\"main\",offset=\"284\",inst=\"mov    -0x4c(%ebp),%eax\"},{address=\"0x08048ee2\",func-name=\"main\",offset=\"287\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048ee5\",func-name=\"main\",offset=\"290\",inst=\"call   0x8048bb4 <_ZNSsD1Ev@plt>\"},{address=\"0x08048eea\",func-name=\"main\",offset=\"295\",inst=\"mov    %ebx,-0x54(%ebp)\"},{address=\"0x08048eed\",func-name=\"main\",offset=\"298\",inst=\"mov    %esi,-0x50(%ebp)\"},{address=\"0x08048ef0\",func-name=\"main\",offset=\"301\",inst=\"jmp    0x8048f62 <main+415>\"},{address=\"0x08048ef2\",func-name=\"main\",offset=\"303\",inst=\"lea    -0x25(%ebp),%eax\"},{address=\"0x08048ef5\",func-name=\"main\",offset=\"306\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048ef8\",func-name=\"main\",offset=\"309\",inst=\"call   0x8048b74 <_ZNSaIcED1Ev@plt>\"},{address=\"0x08048efd\",func-name=\"main\",offset=\"314\",inst=\"call   0x8048cd4 <_Z5func1v>\"},{address=\"0x08048f02\",func-name=\"main\",offset=\"319\",inst=\"movl   $0x2,0x4(%esp)\"},{address=\"0x08048f0a\",func-name=\"main\",offset=\"327\",inst=\"movl   $0x1,(%esp)\"},{address=\"0x08048f11\",func-name=\"main\",offset=\"334\",inst=\"call   0x8048ce7 <_Z5func2ii>\"},{address=\"0x08048f16\",func-name=\"main\",offset=\"339\",inst=\"lea    -0x15(%ebp),%eax\"},{address=\"0x08048f19\",func-name=\"main\",offset=\"342\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048f1c\",func-name=\"main\",offset=\"345\",inst=\"call   0x8048be4 <_ZNSaIcEC1Ev@plt>\"},{address=\"0x08048f21\",func-name=\"main\",offset=\"350\",inst=\"lea    -0x15(%ebp),%eax\"},{address=\"0x08048f24\",func-name=\"main\",offset=\"353\",inst=\"mov    %eax,0x8(%esp)\"},{address=\"0x08048f28\",func-name=\"main\",offset=\"357\",inst=\"movl   $0x8049490,0x4(%esp)\"},{address=\"0x08048f30\",func-name=\"main\",offset=\"365\",inst=\"lea    -0x1c(%ebp),%eax\"},{address=\"0x08048f33\",func-name=\"main\",offset=\"368\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048f36\",func-name=\"main\",offset=\"371\",inst=\"call   0x8048b84 <_ZNSsC1EPKcRKSaIcE@plt>\"},{address=\"0x08048f3b\",func-name=\"main\",offset=\"376\",inst=\"jmp    0x8048f84 <main+449>\"},{address=\"0x08048f3d\",func-name=\"main\",offset=\"378\",inst=\"mov    %eax,-0x54(%ebp)\"},{address=\"0x08048f40\",func-name=\"main\",offset=\"381\",inst=\"mov    %edx,-0x50(%ebp)\"},{address=\"0x08048f43\",func-name=\"main\",offset=\"384\",inst=\"mov    -0x50(%ebp),%esi\"},{address=\"0x08048f46\",func-name=\"main\",offset=\"387\",inst=\"mov    -0x54(%ebp),%ebx\"},{address=\"0x08048f49\",func-name=\"main\",offset=\"390\",inst=\"lea    -0x38(%ebp),%eax\"},{address=\"0x08048f4c\",func-name=\"main\",offset=\"393\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048f4f\",func-name=\"main\",offset=\"396\",inst=\"call   0x804921a <~Person>\"},{address=\"0x08048f54\",func-name=\"main\",offset=\"401\",inst=\"mov    %ebx,-0x54(%ebp)\"},{address=\"0x08048f57\",func-name=\"main\",offset=\"404\",inst=\"mov    %esi,-0x50(%ebp)\"},{address=\"0x08048f5a\",func-name=\"main\",offset=\"407\",inst=\"jmp    0x8048f62 <main+415>\"},{address=\"0x08048f5c\",func-name=\"main\",offset=\"409\",inst=\"mov    %eax,-0x54(%ebp)\"},{address=\"0x08048f5f\",func-name=\"main\",offset=\"412\",inst=\"mov    %edx,-0x50(%ebp)\"},{address=\"0x08048f62\",func-name=\"main\",offset=\"415\",inst=\"mov    -0x50(%ebp),%esi\"},{address=\"0x08048f65\",func-name=\"main\",offset=\"418\",inst=\"mov    -0x54(%ebp),%ebx\"},{address=\"0x08048f68\",func-name=\"main\",offset=\"421\",inst=\"lea    -0x25(%ebp),%eax\"},{address=\"0x08048f6b\",func-name=\"main\",offset=\"424\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048f6e\",func-name=\"main\",offset=\"427\",inst=\"call   0x8048b74 <_ZNSaIcED1Ev@plt>\"},{address=\"0x08048f73\",func-name=\"main\",offset=\"432\",inst=\"mov    %ebx,-0x54(%ebp)\"},{address=\"0x08048f76\",func-name=\"main\",offset=\"435\",inst=\"mov    %esi,-0x50(%ebp)\"},{address=\"0x08048f79\",func-name=\"main\",offset=\"438\",inst=\"mov    -0x54(%ebp),%eax\"},{address=\"0x08048f7c\",func-name=\"main\",offset=\"441\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048f7f\",func-name=\"main\",offset=\"444\",inst=\"call   0x8048bd4 <_Unwind_Resume@plt>\"},{address=\"0x08048f84\",func-name=\"main\",offset=\"449\",inst=\"lea    -0x1c(%ebp),%eax\"},{address=\"0x08048f87\",func-name=\"main\",offset=\"452\",inst=\"mov    %eax,0x4(%esp)\"},{address=\"0x08048f8b\",func-name=\"main\",offset=\"456\",inst=\"lea    -0x38(%ebp),%eax\"},{address=\"0x08048f8e\",func-name=\"main\",offset=\"459\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048f91\",func-name=\"main\",offset=\"462\",inst=\"call   0x8049140 <_ZN6Person14set_first_nameERKSs>\"},{address=\"0x08048f96\",func-name=\"main\",offset=\"467\",inst=\"lea    -0x1c(%ebp),%eax\"},{address=\"0x08048f99\",func-name=\"main\",offset=\"470\",inst=\"mov    %eax,-0x44(%ebp)\"},{address=\"0x08048f9c\",func-name=\"main\",offset=\"473\",inst=\"mov    -0x44(%ebp),%eax\"},{address=\"0x08048f9f\",func-name=\"main\",offset=\"476\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048fa2\",func-name=\"main\",offset=\"479\",inst=\"call   0x8048bb4 <_ZNSsD1Ev@plt>\"},{address=\"0x08048fa7\",func-name=\"main\",offset=\"484\",inst=\"jmp    0x8048fce <main+523>\"},{address=\"0x08048fa9\",func-name=\"main\",offset=\"486\",inst=\"mov    %eax,-0x54(%ebp)\"},{address=\"0x08048fac\",func-name=\"main\",offset=\"489\",inst=\"mov    %edx,-0x50(%ebp)\"},{address=\"0x08048faf\",func-name=\"main\",offset=\"492\",inst=\"mov    -0x50(%ebp),%esi\"},{address=\"0x08048fb2\",func-name=\"main\",offset=\"495\",inst=\"mov    -0x54(%ebp),%ebx\"},{address=\"0x08048fb5\",func-name=\"main\",offset=\"498\",inst=\"lea    -0x1c(%ebp),%eax\"},{address=\"0x08048fb8\",func-name=\"main\",offset=\"501\",inst=\"mov    %eax,-0x44(%ebp)\"},{address=\"0x08048fbb\",func-name=\"main\",offset=\"504\",inst=\"mov    -0x44(%ebp),%eax\"},{address=\"0x08048fbe\",func-name=\"main\",offset=\"507\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048fc1\",func-name=\"main\",offset=\"510\",inst=\"call   0x8048bb4 <_ZNSsD1Ev@plt>\"},{address=\"0x08048fc6\",func-name=\"main\",offset=\"515\",inst=\"mov    %ebx,-0x54(%ebp)\"},{address=\"0x08048fc9\",func-name=\"main\",offset=\"518\",inst=\"mov    %esi,-0x50(%ebp)\"},{address=\"0x08048fcc\",func-name=\"main\",offset=\"521\",inst=\"jmp    0x8049006 <main+579>\"},{address=\"0x08048fce\",func-name=\"main\",offset=\"523\",inst=\"lea    -0x15(%ebp),%eax\"},{address=\"0x08048fd1\",func-name=\"main\",offset=\"526\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048fd4\",func-name=\"main\",offset=\"529\",inst=\"call   0x8048b74 <_ZNSaIcED1Ev@plt>\"},{address=\"0x08048fd9\",func-name=\"main\",offset=\"534\",inst=\"lea    -0xd(%ebp),%eax\"},{address=\"0x08048fdc\",func-name=\"main\",offset=\"537\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048fdf\",func-name=\"main\",offset=\"540\",inst=\"call   0x8048be4 <_ZNSaIcEC1Ev@plt>\"},{address=\"0x08048fe4\",func-name=\"main\",offset=\"545\",inst=\"lea    -0xd(%ebp),%eax\"},{address=\"0x08048fe7\",func-name=\"main\",offset=\"548\",inst=\"mov    %eax,0x8(%esp)\"},{address=\"0x08048feb\",func-name=\"main\",offset=\"552\",inst=\"movl   $0x8049494,0x4(%esp)\"},{address=\"0x08048ff3\",func-name=\"main\",offset=\"560\",inst=\"lea    -0x14(%ebp),%eax\"},{address=\"0x08048ff6\",func-name=\"main\",offset=\"563\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08048ff9\",func-name=\"main\",offset=\"566\",inst=\"call   0x8048b84 <_ZNSsC1EPKcRKSaIcE@plt>\"},{address=\"0x08048ffe\",func-name=\"main\",offset=\"571\",inst=\"jmp    0x8049022 <main+607>\"},{address=\"0x08049000\",func-name=\"main\",offset=\"573\",inst=\"mov    %eax,-0x54(%ebp)\"},{address=\"0x08049003\",func-name=\"main\",offset=\"576\",inst=\"mov    %edx,-0x50(%ebp)\"},{address=\"0x08049006\",func-name=\"main\",offset=\"579\",inst=\"mov    -0x50(%ebp),%esi\"},{address=\"0x08049009\",func-name=\"main\",offset=\"582\",inst=\"mov    -0x54(%ebp),%ebx\"},{address=\"0x0804900c\",func-name=\"main\",offset=\"585\",inst=\"lea    -0x15(%ebp),%eax\"},{address=\"0x0804900f\",func-name=\"main\",offset=\"588\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08049012\",func-name=\"main\",offset=\"591\",inst=\"call   0x8048b74 <_ZNSaIcED1Ev@plt>\"},{address=\"0x08049017\",func-name=\"main\",offset=\"596\",inst=\"mov    %ebx,-0x54(%ebp)\"},{address=\"0x0804901a\",func-name=\"main\",offset=\"599\",inst=\"mov    %esi,-0x50(%ebp)\"},{address=\"0x0804901d\",func-name=\"main\",offset=\"602\",inst=\"jmp    0x80490fa <main+823>\"},{address=\"0x08049022\",func-name=\"main\",offset=\"607\",inst=\"lea    -0x14(%ebp),%eax\"},{address=\"0x08049025\",func-name=\"main\",offset=\"610\",inst=\"mov    %eax,0x4(%esp)\"},{address=\"0x08049029\",func-name=\"main\",offset=\"614\",inst=\"lea    -0x38(%ebp),%eax\"},{address=\"0x0804902c\",func-name=\"main\",offset=\"617\",inst=\"mov    %eax,(%esp)\"},{address=\"0x0804902f\",func-name=\"main\",offset=\"620\",inst=\"call   0x804915a <_ZN6Person15set_family_nameERKSs>\"},{address=\"0x08049034\",func-name=\"main\",offset=\"625\",inst=\"lea    -0x14(%ebp),%eax\"},{address=\"0x08049037\",func-name=\"main\",offset=\"628\",inst=\"mov    %eax,-0x40(%ebp)\"},{address=\"0x0804903a\",func-name=\"main\",offset=\"631\",inst=\"mov    -0x40(%ebp),%eax\"},{address=\"0x0804903d\",func-name=\"main\",offset=\"634\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08049040\",func-name=\"main\",offset=\"637\",inst=\"call   0x8048bb4 <_ZNSsD1Ev@plt>\"},{address=\"0x08049045\",func-name=\"main\",offset=\"642\",inst=\"jmp    0x804906c <main+681>\"},{address=\"0x08049047\",func-name=\"main\",offset=\"644\",inst=\"mov    %eax,-0x54(%ebp)\"},{address=\"0x0804904a\",func-name=\"main\",offset=\"647\",inst=\"mov    %edx,-0x50(%ebp)\"},{address=\"0x0804904d\",func-name=\"main\",offset=\"650\",inst=\"mov    -0x50(%ebp),%esi\"},{address=\"0x08049050\",func-name=\"main\",offset=\"653\",inst=\"mov    -0x54(%ebp),%ebx\"},{address=\"0x08049053\",func-name=\"main\",offset=\"656\",inst=\"lea    -0x14(%ebp),%eax\"},{address=\"0x08049056\",func-name=\"main\",offset=\"659\",inst=\"mov    %eax,-0x40(%ebp)\"},{address=\"0x08049059\",func-name=\"main\",offset=\"662\",inst=\"mov    -0x40(%ebp),%eax\"},{address=\"0x0804905c\",func-name=\"main\",offset=\"665\",inst=\"mov    %eax,(%esp)\"},{address=\"0x0804905f\",func-name=\"main\",offset=\"668\",inst=\"call   0x8048bb4 <_ZNSsD1Ev@plt>\"},{address=\"0x08049064\",func-name=\"main\",offset=\"673\",inst=\"mov    %ebx,-0x54(%ebp)\"},{address=\"0x08049067\",func-name=\"main\",offset=\"676\",inst=\"mov    %esi,-0x50(%ebp)\"},{address=\"0x0804906a\",func-name=\"main\",offset=\"679\",inst=\"jmp    0x804908a <main+711>\"},{address=\"0x0804906c\",func-name=\"main\",offset=\"681\",inst=\"lea    -0xd(%ebp),%eax\"},{address=\"0x0804906f\",func-name=\"main\",offset=\"684\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08049072\",func-name=\"main\",offset=\"687\",inst=\"call   0x8048b74 <_ZNSaIcED1Ev@plt>\"},{address=\"0x08049077\",func-name=\"main\",offset=\"692\",inst=\"lea    -0x38(%ebp),%eax\"},{address=\"0x0804907a\",func-name=\"main\",offset=\"695\",inst=\"mov    %eax,(%esp)\"},{address=\"0x0804907d\",func-name=\"main\",offset=\"698\",inst=\"call   0x8049272 <_ZN6Person7do_thisEv>\"},{address=\"0x08049082\",func-name=\"main\",offset=\"703\",inst=\"jmp    0x80490a3 <main+736>\"},{address=\"0x08049084\",func-name=\"main\",offset=\"705\",inst=\"mov    %eax,-0x54(%ebp)\"},{address=\"0x08049087\",func-name=\"main\",offset=\"708\",inst=\"mov    %edx,-0x50(%ebp)\"},{address=\"0x0804908a\",func-name=\"main\",offset=\"711\",inst=\"mov    -0x50(%ebp),%esi\"},{address=\"0x0804908d\",func-name=\"main\",offset=\"714\",inst=\"mov    -0x54(%ebp),%ebx\"},{address=\"0x08049090\",func-name=\"main\",offset=\"717\",inst=\"lea    -0xd(%ebp),%eax\"},{address=\"0x08049093\",func-name=\"main\",offset=\"720\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08049096\",func-name=\"main\",offset=\"723\",inst=\"call   0x8048b74 <_ZNSaIcED1Ev@plt>\"},{address=\"0x0804909b\",func-name=\"main\",offset=\"728\",inst=\"mov    %ebx,-0x54(%ebp)\"},{address=\"0x0804909e\",func-name=\"main\",offset=\"731\",inst=\"mov    %esi,-0x50(%ebp)\"},{address=\"0x080490a1\",func-name=\"main\",offset=\"734\",inst=\"jmp    0x80490fa <main+823>\"},{address=\"0x080490a3\",func-name=\"main\",offset=\"736\",inst=\"lea    -0x38(%ebp),%eax\"},{address=\"0x080490a6\",func-name=\"main\",offset=\"739\",inst=\"mov    %eax,(%esp)\"},{address=\"0x080490a9\",func-name=\"main\",offset=\"742\",inst=\"call   0x804911c <_ZN6Person8overloadEv>\"},{address=\"0x080490ae\",func-name=\"main\",offset=\"747\",inst=\"movl   $0x0,0x4(%esp)\"},{address=\"0x080490b6\",func-name=\"main\",offset=\"755\",inst=\"lea    -0x38(%ebp),%eax\"},{address=\"0x080490b9\",func-name=\"main\",offset=\"758\",inst=\"mov    %eax,(%esp)\"},{address=\"0x080490bc\",func-name=\"main\",offset=\"761\",inst=\"call   0x8049130 <_ZN6Person8overloadEi>\"},{address=\"0x080490c1\",func-name=\"main\",offset=\"766\",inst=\"lea    -0x38(%ebp),%eax\"},{address=\"0x080490c4\",func-name=\"main\",offset=\"769\",inst=\"mov    %eax,(%esp)\"},{address=\"0x080490c7\",func-name=\"main\",offset=\"772\",inst=\"call   0x8048db0 <_Z5func3R6Person>\"},{address=\"0x080490cc\",func-name=\"main\",offset=\"777\",inst=\"lea    -0x38(%ebp),%eax\"},{address=\"0x080490cf\",func-name=\"main\",offset=\"780\",inst=\"mov    %eax,(%esp)\"},{address=\"0x080490d2\",func-name=\"main\",offset=\"783\",inst=\"call   0x8048cff <_Z5func4R6Person>\"},{address=\"0x080490d7\",func-name=\"main\",offset=\"788\",inst=\"mov    $0x0,%ebx\"},{address=\"0x080490dc\",func-name=\"main\",offset=\"793\",inst=\"lea    -0x38(%ebp),%eax\"},{address=\"0x080490df\",func-name=\"main\",offset=\"796\",inst=\"mov    %eax,(%esp)\"},{address=\"0x080490e2\",func-name=\"main\",offset=\"799\",inst=\"call   0x804921a <~Person>\"},{address=\"0x080490e7\",func-name=\"main\",offset=\"804\",inst=\"mov    %ebx,%eax\"},{address=\"0x080490e9\",func-name=\"main\",offset=\"806\",inst=\"add    $0x5c,%esp\"},{address=\"0x080490ec\",func-name=\"main\",offset=\"809\",inst=\"pop    %ecx\"},{address=\"0x080490ed\",func-name=\"main\",offset=\"810\",inst=\"pop    %ebx\"},{address=\"0x080490ee\",func-name=\"main\",offset=\"811\",inst=\"pop    %esi\"},{address=\"0x080490ef\",func-name=\"main\",offset=\"812\",inst=\"pop    %ebp\"},{address=\"0x080490f0\",func-name=\"main\",offset=\"813\",inst=\"lea    -0x4(%ecx),%esp\"},{address=\"0x080490f3\",func-name=\"main\",offset=\"816\",inst=\"ret    \"},{address=\"0x080490f4\",func-name=\"main\",offset=\"817\",inst=\"mov    %eax,-0x54(%ebp)\"},{address=\"0x080490f7\",func-name=\"main\",offset=\"820\",inst=\"mov    %edx,-0x50(%ebp)\"},{address=\"0x080490fa\",func-name=\"main\",offset=\"823\",inst=\"mov    -0x50(%ebp),%esi\"},{address=\"0x080490fd\",func-name=\"main\",offset=\"826\",inst=\"mov    -0x54(%ebp),%ebx\"},{address=\"0x08049100\",func-name=\"main\",offset=\"829\",inst=\"lea    -0x38(%ebp),%eax\"},{address=\"0x08049103\",func-name=\"main\",offset=\"832\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08049106\",func-name=\"main\",offset=\"835\",inst=\"call   0x804921a <~Person>\"},{address=\"0x0804910b\",func-name=\"main\",offset=\"840\",inst=\"mov    %ebx,-0x54(%ebp)\"},{address=\"0x0804910e\",func-name=\"main\",offset=\"843\",inst=\"mov    %esi,-0x50(%ebp)\"},{address=\"0x08049111\",func-name=\"main\",offset=\"846\",inst=\"mov    -0x54(%ebp),%eax\"},{address=\"0x08049114\",func-name=\"main\",offset=\"849\",inst=\"mov    %eax,(%esp)\"},{address=\"0x08049117\",func-name=\"main\",offset=\"852\",inst=\"call   0x8048bd4 <_Unwind_Resume@plt>\"}]\n";

static const char* gv_disassemble2 =
"asm_insns=[src_and_asm_line={line=\"91\",file=\"fooprog.cc\",line_asm_insn=[{address=\"0x0000000000400a52\",func-name=\"main(int, char**)\",offset=\"18\",inst=\"lea    -0x41(%rbp),%rax\"},{address=\"0x0000000000400a56\",func-name=\"main(int, char**)\",offset=\"22\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400a59\",func-name=\"main(int, char**)\",offset=\"25\",inst=\"callq  0x400868 <_ZNSaIcEC1Ev@plt>\"},{address=\"0x0000000000400a5e\",func-name=\"main(int, char**)\",offset=\"30\",inst=\"lea    -0x41(%rbp),%rdx\"},{address=\"0x0000000000400a62\",func-name=\"main(int, char**)\",offset=\"34\",inst=\"lea    -0x50(%rbp),%rax\"},{address=\"0x0000000000400a66\",func-name=\"main(int, char**)\",offset=\"38\",inst=\"mov    $0x40112d,%esi\"},{address=\"0x0000000000400a6b\",func-name=\"main(int, char**)\",offset=\"43\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400a6e\",func-name=\"main(int, char**)\",offset=\"46\",inst=\"callq  0x400848 <_ZNSsC1EPKcRKSaIcE@plt>\"},{address=\"0x0000000000400a73\",func-name=\"main(int, char**)\",offset=\"51\",inst=\"lea    -0x31(%rbp),%rax\"},{address=\"0x0000000000400a77\",func-name=\"main(int, char**)\",offset=\"55\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400a7a\",func-name=\"main(int, char**)\",offset=\"58\",inst=\"callq  0x400868 <_ZNSaIcEC1Ev@plt>\"},{address=\"0x0000000000400a7f\",func-name=\"main(int, char**)\",offset=\"63\",inst=\"lea    -0x31(%rbp),%rdx\"},{address=\"0x0000000000400a83\",func-name=\"main(int, char**)\",offset=\"67\",inst=\"lea    -0x40(%rbp),%rax\"},{address=\"0x0000000000400a87\",func-name=\"main(int, char**)\",offset=\"71\",inst=\"mov    $0x401134,%esi\"},{address=\"0x0000000000400a8c\",func-name=\"main(int, char**)\",offset=\"76\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400a8f\",func-name=\"main(int, char**)\",offset=\"79\",inst=\"callq  0x400848 <_ZNSsC1EPKcRKSaIcE@plt>\"},{address=\"0x0000000000400a94\",func-name=\"main(int, char**)\",offset=\"84\",inst=\"lea    -0x50(%rbp),%rdx\"},{address=\"0x0000000000400a98\",func-name=\"main(int, char**)\",offset=\"88\",inst=\"lea    -0x40(%rbp),%rbx\"},{address=\"0x0000000000400a9c\",func-name=\"main(int, char**)\",offset=\"92\",inst=\"lea    -0x70(%rbp),%rax\"},{address=\"0x0000000000400aa0\",func-name=\"main(int, char**)\",offset=\"96\",inst=\"mov    $0xf,%ecx\"},{address=\"0x0000000000400aa5\",func-name=\"main(int, char**)\",offset=\"101\",inst=\"mov    %rbx,%rsi\"},{address=\"0x0000000000400aa8\",func-name=\"main(int, char**)\",offset=\"104\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400aab\",func-name=\"main(int, char**)\",offset=\"107\",inst=\"callq  0x400d6c <Person::Person(std::string const&, std::string const&, unsigned int)>\"},{address=\"0x0000000000400ab0\",func-name=\"main(int, char**)\",offset=\"112\",inst=\"lea    -0x40(%rbp),%rax\"},{address=\"0x0000000000400ab4\",func-name=\"main(int, char**)\",offset=\"116\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400ab7\",func-name=\"main(int, char**)\",offset=\"119\",inst=\"callq  0x400828 <_ZNSsD1Ev@plt>\"},{address=\"0x0000000000400abc\",func-name=\"main(int, char**)\",offset=\"124\",inst=\"jmp    0x400ad7 <main(int, char**)+151>\"},{address=\"0x0000000000400abe\",func-name=\"main(int, char**)\",offset=\"126\",inst=\"mov    %edx,%ebx\"},{address=\"0x0000000000400ac0\",func-name=\"main(int, char**)\",offset=\"128\",inst=\"mov    %rax,%r12\"},{address=\"0x0000000000400ac3\",func-name=\"main(int, char**)\",offset=\"131\",inst=\"lea    -0x40(%rbp),%rax\"},{address=\"0x0000000000400ac7\",func-name=\"main(int, char**)\",offset=\"135\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400aca\",func-name=\"main(int, char**)\",offset=\"138\",inst=\"callq  0x400828 <_ZNSsD1Ev@plt>\"},{address=\"0x0000000000400acf\",func-name=\"main(int, char**)\",offset=\"143\",inst=\"mov    %r12,%rax\"},{address=\"0x0000000000400ad2\",func-name=\"main(int, char**)\",offset=\"146\",inst=\"movslq %ebx,%rdx\"},{address=\"0x0000000000400ad5\",func-name=\"main(int, char**)\",offset=\"149\",inst=\"jmp    0x400b0a <main(int, char**)+202>\"},{address=\"0x0000000000400ad7\",func-name=\"main(int, char**)\",offset=\"151\",inst=\"lea    -0x31(%rbp),%rax\"},{address=\"0x0000000000400adb\",func-name=\"main(int, char**)\",offset=\"155\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400ade\",func-name=\"main(int, char**)\",offset=\"158\",inst=\"callq  0x400858 <_ZNSaIcED1Ev@plt>\"},{address=\"0x0000000000400ae3\",func-name=\"main(int, char**)\",offset=\"163\",inst=\"lea    -0x50(%rbp),%rax\"},{address=\"0x0000000000400ae7\",func-name=\"main(int, char**)\",offset=\"167\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400aea\",func-name=\"main(int, char**)\",offset=\"170\",inst=\"callq  0x400828 <_ZNSsD1Ev@plt>\"},{address=\"0x0000000000400aef\",func-name=\"main(int, char**)\",offset=\"175\",inst=\"jmp    0x400b3a <main(int, char**)+250>\"},{address=\"0x0000000000400af1\",func-name=\"main(int, char**)\",offset=\"177\",inst=\"mov    %edx,%ebx\"},{address=\"0x0000000000400af3\",func-name=\"main(int, char**)\",offset=\"179\",inst=\"mov    %rax,%r12\"},{address=\"0x0000000000400af6\",func-name=\"main(int, char**)\",offset=\"182\",inst=\"lea    -0x70(%rbp),%rax\"},{address=\"0x0000000000400afa\",func-name=\"main(int, char**)\",offset=\"186\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400afd\",func-name=\"main(int, char**)\",offset=\"189\",inst=\"callq  0x400fbe <Person::~Person()>\"},{address=\"0x0000000000400b02\",func-name=\"main(int, char**)\",offset=\"194\",inst=\"mov    %r12,%rax\"},{address=\"0x0000000000400b05\",func-name=\"main(int, char**)\",offset=\"197\",inst=\"movslq %ebx,%rdx\"},{address=\"0x0000000000400b08\",func-name=\"main(int, char**)\",offset=\"200\",inst=\"jmp    0x400b0a <main(int, char**)+202>\"},{address=\"0x0000000000400b0a\",func-name=\"main(int, char**)\",offset=\"202\",inst=\"mov    %edx,%ebx\"},{address=\"0x0000000000400b0c\",func-name=\"main(int, char**)\",offset=\"204\",inst=\"mov    %rax,%r12\"},{address=\"0x0000000000400b0f\",func-name=\"main(int, char**)\",offset=\"207\",inst=\"lea    -0x31(%rbp),%rax\"},{address=\"0x0000000000400b13\",func-name=\"main(int, char**)\",offset=\"211\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400b16\",func-name=\"main(int, char**)\",offset=\"214\",inst=\"callq  0x400858 <_ZNSaIcED1Ev@plt>\"},{address=\"0x0000000000400b1b\",func-name=\"main(int, char**)\",offset=\"219\",inst=\"mov    %r12,%rax\"},{address=\"0x0000000000400b1e\",func-name=\"main(int, char**)\",offset=\"222\",inst=\"movslq %ebx,%rdx\"},{address=\"0x0000000000400b21\",func-name=\"main(int, char**)\",offset=\"225\",inst=\"mov    %edx,%ebx\"},{address=\"0x0000000000400b23\",func-name=\"main(int, char**)\",offset=\"227\",inst=\"mov    %rax,%r12\"},{address=\"0x0000000000400b26\",func-name=\"main(int, char**)\",offset=\"230\",inst=\"lea    -0x50(%rbp),%rax\"},{address=\"0x0000000000400b2a\",func-name=\"main(int, char**)\",offset=\"234\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400b2d\",func-name=\"main(int, char**)\",offset=\"237\",inst=\"callq  0x400828 <_ZNSsD1Ev@plt>\"},{address=\"0x0000000000400b32\",func-name=\"main(int, char**)\",offset=\"242\",inst=\"mov    %r12,%rax\"},{address=\"0x0000000000400b35\",func-name=\"main(int, char**)\",offset=\"245\",inst=\"movslq %ebx,%rdx\"},{address=\"0x0000000000400b38\",func-name=\"main(int, char**)\",offset=\"248\",inst=\"jmp    0x400b96 <main(int, char**)+342>\"},{address=\"0x0000000000400b3a\",func-name=\"main(int, char**)\",offset=\"250\",inst=\"lea    -0x41(%rbp),%rax\"},{address=\"0x0000000000400b3e\",func-name=\"main(int, char**)\",offset=\"254\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400b41\",func-name=\"main(int, char**)\",offset=\"257\",inst=\"callq  0x400858 <_ZNSaIcED1Ev@plt>\"},{address=\"0x0000000000400b82\",func-name=\"main(int, char**)\",offset=\"322\",inst=\"lea    -0x70(%rbp),%rax\"},{address=\"0x0000000000400b86\",func-name=\"main(int, char**)\",offset=\"326\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400b89\",func-name=\"main(int, char**)\",offset=\"329\",inst=\"callq  0x400fbe <Person::~Person()>\"},{address=\"0x0000000000400b8e\",func-name=\"main(int, char**)\",offset=\"334\",inst=\"mov    %r12,%rax\"},{address=\"0x0000000000400b91\",func-name=\"main(int, char**)\",offset=\"337\",inst=\"movslq %ebx,%rdx\"},{address=\"0x0000000000400b94\",func-name=\"main(int, char**)\",offset=\"340\",inst=\"jmp    0x400b96 <main(int, char**)+342>\"},{address=\"0x0000000000400b96\",func-name=\"main(int, char**)\",offset=\"342\",inst=\"mov    %edx,%ebx\"},{address=\"0x0000000000400b98\",func-name=\"main(int, char**)\",offset=\"344\",inst=\"mov    %rax,%r12\"},{address=\"0x0000000000400b9b\",func-name=\"main(int, char**)\",offset=\"347\",inst=\"lea    -0x41(%rbp),%rax\"},{address=\"0x0000000000400b9f\",func-name=\"main(int, char**)\",offset=\"351\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400ba2\",func-name=\"main(int, char**)\",offset=\"354\",inst=\"callq  0x400858 <_ZNSaIcED1Ev@plt>\"},{address=\"0x0000000000400ba7\",func-name=\"main(int, char**)\",offset=\"359\",inst=\"mov    %r12,%rax\"},{address=\"0x0000000000400baa\",func-name=\"main(int, char**)\",offset=\"362\",inst=\"movslq %ebx,%rdx\"},{address=\"0x0000000000400bad\",func-name=\"main(int, char**)\",offset=\"365\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400bb0\",func-name=\"main(int, char**)\",offset=\"368\",inst=\"callq  0x400888 <_Unwind_Resume@plt>\"}]},src_and_asm_line={line=\"92\",file=\"fooprog.cc\",line_asm_insn=[{address=\"0x0000000000400b46\",func-name=\"main(int, char**)\",offset=\"262\",inst=\"callq  0x400994 <func1()>\"}]},src_and_asm_line={line=\"93\",file=\"fooprog.cc\",line_asm_insn=[]},src_and_asm_line={line=\"94\",file=\"fooprog.cc\",line_asm_insn=[{address=\"0x0000000000400b4b\",func-name=\"main(int, char**)\",offset=\"267\",inst=\"mov    $0x2,%esi\"},{address=\"0x0000000000400b50\",func-name=\"main(int, char**)\",offset=\"272\",inst=\"mov    $0x1,%edi\"},{address=\"0x0000000000400b55\",func-name=\"main(int, char**)\",offset=\"277\",inst=\"callq  0x4009a5 <func2(int, int)>\"}]},src_and_asm_line={line=\"95\",file=\"fooprog.cc\",line_asm_insn=[]},src_and_asm_line={line=\"96\",file=\"fooprog.cc\",line_asm_insn=[{address=\"0x0000000000400b5a\",func-name=\"main(int, char**)\",offset=\"282\",inst=\"lea    -0x21(%rbp),%rax\"},{address=\"0x0000000000400b5e\",func-name=\"main(int, char**)\",offset=\"286\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400b61\",func-name=\"main(int, char**)\",offset=\"289\",inst=\"callq  0x400868 <_ZNSaIcEC1Ev@plt>\"},{address=\"0x0000000000400b66\",func-name=\"main(int, char**)\",offset=\"294\",inst=\"lea    -0x21(%rbp),%rdx\"},{address=\"0x0000000000400b6a\",func-name=\"main(int, char**)\",offset=\"298\",inst=\"lea    -0x30(%rbp),%rax\"},{address=\"0x0000000000400b6e\",func-name=\"main(int, char**)\",offset=\"302\",inst=\"mov    $0x401138,%esi\"},{address=\"0x0000000000400b73\",func-name=\"main(int, char**)\",offset=\"307\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400b76\",func-name=\"main(int, char**)\",offset=\"310\",inst=\"callq  0x400848 <_ZNSsC1EPKcRKSaIcE@plt>\"},{address=\"0x0000000000400b7b\",func-name=\"main(int, char**)\",offset=\"315\",inst=\"jmp    0x400bb5 <main(int, char**)+373>\"},{address=\"0x0000000000400b7d\",func-name=\"main(int, char**)\",offset=\"317\",inst=\"mov    %edx,%ebx\"},{address=\"0x0000000000400b7f\",func-name=\"main(int, char**)\",offset=\"319\",inst=\"mov    %rax,%r12\"},{address=\"0x0000000000400bb5\",func-name=\"main(int, char**)\",offset=\"373\",inst=\"lea    -0x30(%rbp),%rdx\"},{address=\"0x0000000000400bb9\",func-name=\"main(int, char**)\",offset=\"377\",inst=\"lea    -0x70(%rbp),%rax\"},{address=\"0x0000000000400bbd\",func-name=\"main(int, char**)\",offset=\"381\",inst=\"mov    %rdx,%rsi\"},{address=\"0x0000000000400bc0\",func-name=\"main(int, char**)\",offset=\"384\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400bc3\",func-name=\"main(int, char**)\",offset=\"387\",inst=\"callq  0x400e1c <Person::set_first_name(std::string const&)>\"},{address=\"0x0000000000400bc8\",func-name=\"main(int, char**)\",offset=\"392\",inst=\"lea    -0x30(%rbp),%rax\"},{address=\"0x0000000000400bcc\",func-name=\"main(int, char**)\",offset=\"396\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400bcf\",func-name=\"main(int, char**)\",offset=\"399\",inst=\"callq  0x400828 <_ZNSsD1Ev@plt>\"},{address=\"0x0000000000400bd4\",func-name=\"main(int, char**)\",offset=\"404\",inst=\"jmp    0x400bef <main(int, char**)+431>\"},{address=\"0x0000000000400bd6\",func-name=\"main(int, char**)\",offset=\"406\",inst=\"mov    %edx,%ebx\"},{address=\"0x0000000000400bd8\",func-name=\"main(int, char**)\",offset=\"408\",inst=\"mov    %rax,%r12\"},{address=\"0x0000000000400bdb\",func-name=\"main(int, char**)\",offset=\"411\",inst=\"lea    -0x30(%rbp),%rax\"},{address=\"0x0000000000400bdf\",func-name=\"main(int, char**)\",offset=\"415\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400be2\",func-name=\"main(int, char**)\",offset=\"418\",inst=\"callq  0x400828 <_ZNSsD1Ev@plt>\"},{address=\"0x0000000000400be7\",func-name=\"main(int, char**)\",offset=\"423\",inst=\"mov    %r12,%rax\"},{address=\"0x0000000000400bea\",func-name=\"main(int, char**)\",offset=\"426\",inst=\"movslq %ebx,%rdx\"},{address=\"0x0000000000400bed\",func-name=\"main(int, char**)\",offset=\"429\",inst=\"jmp    0x400c1e <main(int, char**)+478>\"},{address=\"0x0000000000400bef\",func-name=\"main(int, char**)\",offset=\"431\",inst=\"lea    -0x21(%rbp),%rax\"},{address=\"0x0000000000400bf3\",func-name=\"main(int, char**)\",offset=\"435\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400bf6\",func-name=\"main(int, char**)\",offset=\"438\",inst=\"callq  0x400858 <_ZNSaIcED1Ev@plt>\"},{address=\"0x0000000000400c23\",func-name=\"main(int, char**)\",offset=\"483\",inst=\"lea    -0x21(%rbp),%rax\"},{address=\"0x0000000000400c27\",func-name=\"main(int, char**)\",offset=\"487\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400c2a\",func-name=\"main(int, char**)\",offset=\"490\",inst=\"callq  0x400858 <_ZNSaIcED1Ev@plt>\"},{address=\"0x0000000000400c2f\",func-name=\"main(int, char**)\",offset=\"495\",inst=\"mov    %r12,%rax\"},{address=\"0x0000000000400c32\",func-name=\"main(int, char**)\",offset=\"498\",inst=\"movslq %ebx,%rdx\"},{address=\"0x0000000000400c35\",func-name=\"main(int, char**)\",offset=\"501\",inst=\"jmpq   0x400cf8 <main(int, char**)+696>\"}]},src_and_asm_line={line=\"97\",file=\"fooprog.cc\",line_asm_insn=[{address=\"0x0000000000400bfb\",func-name=\"main(int, char**)\",offset=\"443\",inst=\"lea    -0x11(%rbp),%rax\"},{address=\"0x0000000000400bff\",func-name=\"main(int, char**)\",offset=\"447\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400c02\",func-name=\"main(int, char**)\",offset=\"450\",inst=\"callq  0x400868 <_ZNSaIcEC1Ev@plt>\"},{address=\"0x0000000000400c07\",func-name=\"main(int, char**)\",offset=\"455\",inst=\"lea    -0x11(%rbp),%rdx\"},{address=\"0x0000000000400c0b\",func-name=\"main(int, char**)\",offset=\"459\",inst=\"lea    -0x20(%rbp),%rax\"},{address=\"0x0000000000400c0f\",func-name=\"main(int, char**)\",offset=\"463\",inst=\"mov    $0x40113c,%esi\"},{address=\"0x0000000000400c14\",func-name=\"main(int, char**)\",offset=\"468\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400c17\",func-name=\"main(int, char**)\",offset=\"471\",inst=\"callq  0x400848 <_ZNSsC1EPKcRKSaIcE@plt>\"},{address=\"0x0000000000400c1c\",func-name=\"main(int, char**)\",offset=\"476\",inst=\"jmp    0x400c3a <main(int, char**)+506>\"},{address=\"0x0000000000400c1e\",func-name=\"main(int, char**)\",offset=\"478\",inst=\"mov    %edx,%ebx\"},{address=\"0x0000000000400c20\",func-name=\"main(int, char**)\",offset=\"480\",inst=\"mov    %rax,%r12\"},{address=\"0x0000000000400c3a\",func-name=\"main(int, char**)\",offset=\"506\",inst=\"lea    -0x20(%rbp),%rdx\"},{address=\"0x0000000000400c3e\",func-name=\"main(int, char**)\",offset=\"510\",inst=\"lea    -0x70(%rbp),%rax\"},{address=\"0x0000000000400c42\",func-name=\"main(int, char**)\",offset=\"514\",inst=\"mov    %rdx,%rsi\"},{address=\"0x0000000000400c45\",func-name=\"main(int, char**)\",offset=\"517\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400c48\",func-name=\"main(int, char**)\",offset=\"520\",inst=\"callq  0x400e42 <Person::set_family_name(std::string const&)>\"},{address=\"0x0000000000400c4d\",func-name=\"main(int, char**)\",offset=\"525\",inst=\"lea    -0x20(%rbp),%rax\"},{address=\"0x0000000000400c51\",func-name=\"main(int, char**)\",offset=\"529\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400c54\",func-name=\"main(int, char**)\",offset=\"532\",inst=\"callq  0x400828 <_ZNSsD1Ev@plt>\"},{address=\"0x0000000000400c59\",func-name=\"main(int, char**)\",offset=\"537\",inst=\"jmp    0x400c74 <main(int, char**)+564>\"},{address=\"0x0000000000400c5b\",func-name=\"main(int, char**)\",offset=\"539\",inst=\"mov    %edx,%ebx\"},{address=\"0x0000000000400c5d\",func-name=\"main(int, char**)\",offset=\"541\",inst=\"mov    %rax,%r12\"},{address=\"0x0000000000400c60\",func-name=\"main(int, char**)\",offset=\"544\",inst=\"lea    -0x20(%rbp),%rax\"},{address=\"0x0000000000400c64\",func-name=\"main(int, char**)\",offset=\"548\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400c67\",func-name=\"main(int, char**)\",offset=\"551\",inst=\"callq  0x400828 <_ZNSsD1Ev@plt>\"},{address=\"0x0000000000400c6c\",func-name=\"main(int, char**)\",offset=\"556\",inst=\"mov    %r12,%rax\"},{address=\"0x0000000000400c6f\",func-name=\"main(int, char**)\",offset=\"559\",inst=\"movslq %ebx,%rdx\"},{address=\"0x0000000000400c72\",func-name=\"main(int, char**)\",offset=\"562\",inst=\"jmp    0x400c8e <main(int, char**)+590>\"},{address=\"0x0000000000400c74\",func-name=\"main(int, char**)\",offset=\"564\",inst=\"lea    -0x11(%rbp),%rax\"},{address=\"0x0000000000400c78\",func-name=\"main(int, char**)\",offset=\"568\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400c7b\",func-name=\"main(int, char**)\",offset=\"571\",inst=\"callq  0x400858 <_ZNSaIcED1Ev@plt>\"},{address=\"0x0000000000400c93\",func-name=\"main(int, char**)\",offset=\"595\",inst=\"lea    -0x11(%rbp),%rax\"},{address=\"0x0000000000400c97\",func-name=\"main(int, char**)\",offset=\"599\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400c9a\",func-name=\"main(int, char**)\",offset=\"602\",inst=\"callq  0x400858 <_ZNSaIcED1Ev@plt>\"},{address=\"0x0000000000400c9f\",func-name=\"main(int, char**)\",offset=\"607\",inst=\"mov    %r12,%rax\"},{address=\"0x0000000000400ca2\",func-name=\"main(int, char**)\",offset=\"610\",inst=\"movslq %ebx,%rdx\"},{address=\"0x0000000000400ca5\",func-name=\"main(int, char**)\",offset=\"613\",inst=\"jmp    0x400cf8 <main(int, char**)+696>\"}]},src_and_asm_line={line=\"98\",file=\"fooprog.cc\",line_asm_insn=[{address=\"0x0000000000400c80\",func-name=\"main(int, char**)\",offset=\"576\",inst=\"lea    -0x70(%rbp),%rax\"},{address=\"0x0000000000400c84\",func-name=\"main(int, char**)\",offset=\"580\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400c87\",func-name=\"main(int, char**)\",offset=\"583\",inst=\"callq  0x400e6c <Person::do_this()>\"},{address=\"0x0000000000400c8c\",func-name=\"main(int, char**)\",offset=\"588\",inst=\"jmp    0x400ca7 <main(int, char**)+615>\"},{address=\"0x0000000000400c8e\",func-name=\"main(int, char**)\",offset=\"590\",inst=\"mov    %edx,%ebx\"},{address=\"0x0000000000400c90\",func-name=\"main(int, char**)\",offset=\"592\",inst=\"mov    %rax,%r12\"}]},src_and_asm_line={line=\"99\",file=\"fooprog.cc\",line_asm_insn=[{address=\"0x0000000000400ca7\",func-name=\"main(int, char**)\",offset=\"615\",inst=\"lea    -0x70(%rbp),%rax\"},{address=\"0x0000000000400cab\",func-name=\"main(int, char**)\",offset=\"619\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400cae\",func-name=\"main(int, char**)\",offset=\"622\",inst=\"callq  0x400f90 <Person::overload()>\"}]},src_and_asm_line={line=\"100\",file=\"fooprog.cc\",line_asm_insn=[{address=\"0x0000000000400cb3\",func-name=\"main(int, char**)\",offset=\"627\",inst=\"lea    -0x70(%rbp),%rax\"},{address=\"0x0000000000400cb7\",func-name=\"main(int, char**)\",offset=\"631\",inst=\"mov    $0x0,%esi\"},{address=\"0x0000000000400cbc\",func-name=\"main(int, char**)\",offset=\"636\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400cbf\",func-name=\"main(int, char**)\",offset=\"639\",inst=\"callq  0x400fa6 <Person::overload(int)>\"}]},src_and_asm_line={line=\"101\",file=\"fooprog.cc\",line_asm_insn=[]},src_and_asm_line={line=\"102\",file=\"fooprog.cc\",line_asm_insn=[{address=\"0x0000000000400cc4\",func-name=\"main(int, char**)\",offset=\"644\",inst=\"lea    -0x70(%rbp),%rax\"},{address=\"0x0000000000400cc8\",func-name=\"main(int, char**)\",offset=\"648\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400ccb\",func-name=\"main(int, char**)\",offset=\"651\",inst=\"callq  0x4009c1 <func3(Person&)>\"}]},src_and_asm_line={line=\"103\",file=\"fooprog.cc\",line_asm_insn=[]},src_and_asm_line={line=\"104\",file=\"fooprog.cc\",line_asm_insn=[{address=\"0x0000000000400cd0\",func-name=\"main(int, char**)\",offset=\"656\",inst=\"lea    -0x70(%rbp),%rax\"},{address=\"0x0000000000400cd4\",func-name=\"main(int, char**)\",offset=\"660\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400cd7\",func-name=\"main(int, char**)\",offset=\"663\",inst=\"callq  0x4009db <func4(Person&)>\"}]},src_and_asm_line={line=\"105\",file=\"fooprog.cc\",line_asm_insn=[]},src_and_asm_line={line=\"106\",file=\"fooprog.cc\",line_asm_insn=[{address=\"0x0000000000400cdc\",func-name=\"main(int, char**)\",offset=\"668\",inst=\"mov    $0x0,%ebx\"},{address=\"0x0000000000400ce1\",func-name=\"main(int, char**)\",offset=\"673\",inst=\"lea    -0x70(%rbp),%rax\"},{address=\"0x0000000000400ce5\",func-name=\"main(int, char**)\",offset=\"677\",inst=\"mov    %rax,%rdi\"},{address=\"0x0000000000400ce8\",func-name=\"main(int, char**)\",offset=\"680\",inst=\"callq  0x400fbe <Person::~Person()>\"},{address=\"0x0000000000400ced\",func-name=\"main(int, char**)\",offset=\"685\",inst=\"mov    %ebx,%eax\"}]}]";

static const char *gv_disassemble3 =
"asm_insns=[{address=\"0x00000000004004f3\",func-name=\"main\",offset=\"15\",inst=\"mov    $0x4005fc,%edi\"},{address=\"0x00000000004004f8\",func-name=\"main\",offset=\"20\",inst=\"callq  0x4003e0 <puts@plt>\"},{address=\"0x00000000004004fd\",func-name=\"main\",offset=\"25\",inst=\"mov    $0x0,%eax\"},{address=\"0x0000000000400502\",func-name=\"main\",offset=\"30\",inst=\"leaveq \"},{address=\"0x0000000000400503\",func-name=\"main\",offset=\"31\",inst=\"retq   \"},{address=\"0x0000000000400504\",inst=\"nop\"},{address=\"0x0000000000400505\",inst=\"nop\"},{address=\"0x0000000000400506\",inst=\"nop\"}]";

static const char *gv_disassemble4 = "asm_insns=[src_and_asm_line={line=\"0\",file=\"pippo.c\",line_asm_insn=[]},src_and_asm_line={line=\"1\",file=\"pippo.c\",line_asm_insn=[]},src_and_asm_line={line=\"2\",file=\"pippo.c\",line_asm_insn=[]}]";

static const char* gv_file_list1 =
"files=[{file=\"fooprog.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/tests/fooprog.cc\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/locale_facets.h\",fullname=\"/usr/include/c++/4.3.2/bits/locale_facets.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/x86_64-redhat-linux/bits/ctype_base.h\",fullname=\"/usr/include/c++/4.3.2/x86_64-redhat-linux/bits/ctype_base.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/ext/numeric_traits.h\",fullname=\"/usr/include/c++/4.3.2/ext/numeric_traits.h\"},{file=\"/usr/include/wctype.h\",fullname=\"/usr/include/wctype.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/locale_classes.h\",fullname=\"/usr/include/c++/4.3.2/bits/locale_classes.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/basic_string.tcc\",fullname=\"/usr/include/c++/4.3.2/bits/basic_string.tcc\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stringfwd.h\",fullname=\"/usr/include/c++/4.3.2/bits/stringfwd.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/x86_64-redhat-linux/bits/atomic_word.h\",fullname=\"/usr/include/c++/4.3.2/x86_64-redhat-linux/bits/atomic_word.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/x86_64-redhat-linux/bits/gthr-default.h\",fullname=\"/usr/include/c++/4.3.2/x86_64-redhat-linux/bits/gthr-default.h\"},{file=\"/usr/include/bits/pthreadtypes.h\",fullname=\"/usr/include/bits/pthreadtypes.h\"},{file=\"/usr/include/locale.h\",fullname=\"/usr/include/locale.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/allocator.h\",fullname=\"/usr/include/c++/4.3.2/bits/allocator.h\"},{file=\"/usr/include/_G_config.h\",fullname=\"/usr/include/_G_config.h\"},{file=\"/usr/include/bits/types.h\",fullname=\"/usr/include/bits/types.h\"},{file=\"/usr/include/time.h\",fullname=\"/usr/include/time.h\"},{file=\"/usr/include/wchar.h\",fullname=\"/usr/include/wchar.h\"},{file=\"/home/dodji/.ccache/fooprog.tmp.tutu.605.ii\"},{file=\"/usr/include/libio.h\",fullname=\"/usr/include/libio.h\"},{file=\"/usr/include/stdio.h\",fullname=\"/usr/include/stdio.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/include/stddef.h\",fullname=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/include/stddef.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/debug/debug.h\",fullname=\"/usr/include/c++/4.3.2/debug/debug.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/cpp_type_traits.h\",fullname=\"/usr/include/c++/4.3.2/bits/cpp_type_traits.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/ext/new_allocator.h\",fullname=\"/usr/include/c++/4.3.2/ext/new_allocator.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/cwctype\",fullname=\"/usr/include/c++/4.3.2/cwctype\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/basic_string.h\",fullname=\"/usr/include/c++/4.3.2/bits/basic_string.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/ios_base.h\",fullname=\"/usr/include/c++/4.3.2/bits/ios_base.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/clocale\",fullname=\"/usr/include/c++/4.3.2/clocale\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/cstdio\",fullname=\"/usr/include/c++/4.3.2/cstdio\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/cwchar\",fullname=\"/usr/include/c++/4.3.2/cwchar\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/cstddef\",fullname=\"/usr/include/c++/4.3.2/cstddef\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/iostream\",fullname=\"/usr/include/c++/4.3.2/iostream\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/iostream\",fullname=\"/usr/include/c++/4.3.2/iostream\"},{file=\"nmv-safe-ptr.h\"},{file=\"nmv-safe-ptr-utils.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/list.tcc\",fullname=\"/usr/include/c++/4.3.2/bits/list.tcc\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_algo.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_algo.h\"},{file=\"/usr/include/glibmm-2.4/glibmm/ustring.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/ustring.h\"},{file=\"nmv-proc-mgr.cc\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/allocator.h\",fullname=\"/usr/include/c++/4.3.2/bits/allocator.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/ext/new_allocator.h\",fullname=\"/usr/include/c++/4.3.2/ext/new_allocator.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_iterator_base_types.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_iterator_base_types.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_list.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_list.h\"},{file=\"nmv-proc-mgr.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/new\",fullname=\"/usr/include/c++/4.3.2/new\"},{file=\"nmv-proc-mgr.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/src/common/nmv-proc-mgr.cc\"},{file=\"/usr/include/glibmm-2.4/glibmm/ustring.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/ustring.h\"},{file=\"nmv-proc-utils.cc\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/basic_string.h\",fullname=\"/usr/include/c++/4.3.2/bits/basic_string.h\"},{file=\"/usr/include/glibmm-2.4/glibmm/refptr.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/refptr.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_vector.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_vector.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/backward/auto_ptr.h\",fullname=\"/usr/include/c++/4.3.2/backward/auto_ptr.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_iterator.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_iterator.h\"},{file=\"nmv-proc-utils.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/src/common/nmv-proc-utils.cc\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_uninitialized.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_uninitialized.h\"},{file=\"/usr/include/glibmm-2.4/glibmm/ustring.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/ustring.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_construct.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_construct.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/allocator.h\",fullname=\"/usr/include/c++/4.3.2/bits/allocator.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/ext/new_allocator.h\",fullname=\"/usr/include/c++/4.3.2/ext/new_allocator.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_vector.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_vector.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_iterator.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_iterator.h\"},{file=\"nmv-delete-statement.cc\"},{file=\"nmv-sql-statement.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/new\",fullname=\"/usr/include/c++/4.3.2/new\"},{file=\"nmv-delete-statement.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/src/common/nmv-delete-statement.cc\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/vector.tcc\",fullname=\"/usr/include/c++/4.3.2/bits/vector.tcc\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_uninitialized.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_uninitialized.h\"},{file=\"/usr/include/glibmm-2.4/glibmm/ustring.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/ustring.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_construct.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_construct.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/allocator.h\",fullname=\"/usr/include/c++/4.3.2/bits/allocator.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/ext/new_allocator.h\",fullname=\"/usr/include/c++/4.3.2/ext/new_allocator.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_algobase.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_algobase.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_iterator.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_iterator.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_vector.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_vector.h\"},{file=\"nmv-insert-statement.cc\"},{file=\"nmv-sql-statement.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/new\",fullname=\"/usr/include/c++/4.3.2/new\"},{file=\"nmv-insert-statement.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/src/common/nmv-insert-statement.cc\"},{file=\"/usr/include/glibmm-2.4/glibmm/ustring.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/ustring.h\"},{file=\"nmv-sql-statement.cc\"},{file=\"nmv-sql-statement.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/src/common/nmv-sql-statement.cc\"},{file=\"/usr/include/glibmm-2.4/glibmm/thread.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/thread.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_uninitialized.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_uninitialized.h\"},{file=\"/usr/include/glibmm-2.4/glibmm/ustring.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/ustring.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_iterator_base_funcs.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_iterator_base_funcs.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_iterator_base_types.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_iterator_base_types.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/deque.tcc\",fullname=\"/usr/include/c++/4.3.2/bits/deque.tcc\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_construct.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_construct.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/allocator.h\",fullname=\"/usr/include/c++/4.3.2/bits/allocator.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/ext/new_allocator.h\",fullname=\"/usr/include/c++/4.3.2/ext/new_allocator.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_stack.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_stack.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_algobase.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_algobase.h\"},{file=\"nmv-transaction.cc\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_deque.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_deque.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/new\",fullname=\"/usr/include/c++/4.3.2/new\"},{file=\"nmv-transaction.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/src/common/nmv-transaction.cc\"},{file=\"nmv-tools.cc\"},{file=\"nmv-transaction.h\"},{file=\"/usr/include/glibmm-2.4/glibmm/ustring.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/ustring.h\"},{file=\"nmv-buffer.h\"},{file=\"nmv-tools.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/src/common/nmv-tools.cc\"},{file=\"/usr/include/glibmm-2.4/glibmm/thread.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/thread.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/vector.tcc\",fullname=\"/usr/include/c++/4.3.2/bits/vector.tcc\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_uninitialized.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_uninitialized.h\"},{file=\"nmv-conf-manager.cc\"},{file=\"/usr/include/glibmm-2.4/glibmm/ustring.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/ustring.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_algobase.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_algobase.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_construct.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_construct.h\"},{file=\"/usr/include/glibmm-2.4/glibmm/arrayhandle.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/arrayhandle.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_iterator.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_iterator.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_pair.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_pair.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_function.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_function.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_map.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_map.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/allocator.h\",fullname=\"/usr/include/c++/4.3.2/bits/allocator.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/ext/new_allocator.h\",fullname=\"/usr/include/c++/4.3.2/ext/new_allocator.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_vector.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_vector.h\"},{file=\"nmv-safe-ptr.h\"},{file=\"/usr/include/glibmm-2.4/glibmm/containerhandle_shared.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/containerhandle_shared.h\"},{file=\"nmv-libxml-utils.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_tree.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_tree.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/ios_base.h\",fullname=\"/usr/include/c++/4.3.2/bits/ios_base.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/new\",fullname=\"/usr/include/c++/4.3.2/new\"},{file=\"nmv-conf-manager.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/src/common/nmv-conf-manager.cc\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/vector.tcc\",fullname=\"/usr/include/c++/4.3.2/bits/vector.tcc\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_uninitialized.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_uninitialized.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_algobase.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_algobase.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/allocator.h\",fullname=\"/usr/include/c++/4.3.2/bits/allocator.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_construct.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_construct.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/ext/new_allocator.h\",fullname=\"/usr/include/c++/4.3.2/ext/new_allocator.h\"},{file=\"/usr/include/glibmm-2.4/glibmm/ustring.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/ustring.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_iterator.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_iterator.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_vector.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_vector.h\"},{file=\"nmv-parsing-utils.cc\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/new\",fullname=\"/usr/include/c++/4.3.2/new\"},{file=\"nmv-parsing-utils.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/src/common/nmv-parsing-utils.cc\"},{file=\"nmv-safe-ptr-utils.h\"},{file=\"/usr/include/glibmm-2.4/glibmm/thread.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/thread.h\"},{file=\"/usr/include/glibmm-2.4/glibmm/ustring.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/ustring.h\"},{file=\"nmv-safe-ptr.h\"},{file=\"nmv-connection.cc\"},{file=\"nmv-connection.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/src/common/nmv-connection.cc\"},{file=\"nmv-dynamic-module.h\"},{file=\"/usr/include/glibmm-2.4/glibmm/ustring.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/ustring.h\"},{file=\"nmv-safe-ptr-utils.h\"},{file=\"nmv-connection-manager.cc\"},{file=\"nmv-safe-ptr.h\"},{file=\"nmv-i-connection-manager-driver.h\"},{file=\"nmv-connection-manager.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/src/common/nmv-connection-manager.cc\"},{file=\"nmv-option-utils.cc\"},{file=\"nmv-option-utils.h\"},{file=\"nmv-option-utils.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/src/common/nmv-option-utils.cc\"},{file=\"nmv-sequence.h\"},{file=\"/usr/include/glibmm-2.4/glibmm/thread.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/thread.h\"},{file=\"nmv-sequence.cc\"},{file=\"nmv-safe-ptr.h\"},{file=\"nmv-sequence.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/src/common/nmv-sequence.cc\"},{file=\"nmv-dynamic-module.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/vector.tcc\",fullname=\"/usr/include/c++/4.3.2/bits/vector.tcc\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_uninitialized.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_uninitialized.h\"},{file=\"nmv-safe-ptr-utils.h\"},{file=\"/usr/include/glibmm-2.4/glibmm/ustring.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/ustring.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_iterator_base_funcs.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_iterator_base_funcs.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_iterator_base_types.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_iterator_base_types.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_pair.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_pair.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_function.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_function.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_construct.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_construct.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_algobase.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_algobase.h\"},{file=\"/usr/include/glibmm-2.4/glibmm/arrayhandle.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/arrayhandle.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_map.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_map.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/allocator.h\",fullname=\"/usr/include/c++/4.3.2/bits/allocator.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/ext/new_allocator.h\",fullname=\"/usr/include/c++/4.3.2/ext/new_allocator.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_vector.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_vector.h\"},{file=\"nmv-plugin.cc\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_tree.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_tree.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_iterator.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_iterator.h\"},{file=\"/usr/include/glibmm-2.4/glibmm/containerhandle_shared.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/containerhandle_shared.h\"},{file=\"nmv-plugin.h\"},{file=\"nmv-libxml-utils.h\"},{file=\"nmv-safe-ptr.h\"},{file=\"/usr/include/glibmm-2.4/glibmm/fileutils.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/fileutils.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/new\",fullname=\"/usr/include/c++/4.3.2/new\"},{file=\"nmv-plugin.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/src/common/nmv-plugin.cc\"},{file=\"/usr/include/glibmm-2.4/glibmm/thread.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/thread.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/vector.tcc\",fullname=\"/usr/include/c++/4.3.2/bits/vector.tcc\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_uninitialized.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_uninitialized.h\"},{file=\"/usr/include/glibmm-2.4/glibmm/ustring.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/ustring.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_construct.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_construct.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/allocator.h\",fullname=\"/usr/include/c++/4.3.2/bits/allocator.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/ext/new_allocator.h\",fullname=\"/usr/include/c++/4.3.2/ext/new_allocator.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_algobase.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_algobase.h\"},{file=\"/usr/include/glibmm-2.4/glibmm/arrayhandle.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/arrayhandle.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_iterator.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_iterator.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_vector.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_vector.h\"},{file=\"/usr/include/glibmm-2.4/glibmm/containerhandle_shared.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/containerhandle_shared.h\"},{file=\"nmv-env.cc\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/new\",fullname=\"/usr/include/c++/4.3.2/new\"},{file=\"nmv-env.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/src/common/nmv-env.cc\"},{file=\"nmv-date-utils.cc\"},{file=\"nmv-date-utils.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/src/common/nmv-date-utils.cc\"},{file=\"/usr/include/glibmm-2.4/glibmm/thread.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/thread.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/vector.tcc\",fullname=\"/usr/include/c++/4.3.2/bits/vector.tcc\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_uninitialized.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_uninitialized.h\"},{file=\"nmv-dynamic-module.h\"},{file=\"/usr/include/glibmm-2.4/glibmm/ustring.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/ustring.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_pair.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_pair.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/basic_string.h\",fullname=\"/usr/include/c++/4.3.2/bits/basic_string.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_algobase.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_algobase.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_function.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_function.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_construct.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_construct.h\"},{file=\"/usr/include/glibmm-2.4/glibmm/arrayhandle.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/arrayhandle.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_map.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_map.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/allocator.h\",fullname=\"/usr/include/c++/4.3.2/bits/allocator.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/ext/new_allocator.h\",fullname=\"/usr/include/c++/4.3.2/ext/new_allocator.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_vector.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_vector.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_iterator.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_iterator.h\"},{file=\"nmv-dynamic-module.cc\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_tree.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_tree.h\"},{file=\"/usr/include/glibmm-2.4/glibmm/containerhandle_shared.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/containerhandle_shared.h\"},{file=\"nmv-libxml-utils.h\"},{file=\"nmv-safe-ptr-utils.h\"},{file=\"nmv-safe-ptr.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/new\",fullname=\"/usr/include/c++/4.3.2/new\"},{file=\"nmv-dynamic-module.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/src/common/nmv-dynamic-module.cc\"},{file=\"/usr/include/glibmm-2.4/glibmm/thread.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/thread.h\"},{file=\"nmv-initializer.cc\"},{file=\"nmv-initializer.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/src/common/nmv-initializer.cc\"},{file=\"/usr/include/glibmm-2.4/glibmm/ustring.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/ustring.h\"},{file=\"nmv-exception.cc\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/stdexcept\",fullname=\"/usr/include/c++/4.3.2/stdexcept\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/exception\",fullname=\"/usr/include/c++/4.3.2/exception\"},{file=\"nmv-exception.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/src/common/nmv-exception.cc\"},{file=\"/usr/include/glibmm-2.4/glibmm/ustring.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/ustring.h\"},{file=\"nmv-scope-logger.cc\"},{file=\"nmv-safe-ptr.h\"},{file=\"nmv-scope-logger.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/src/common/nmv-scope-logger.cc\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/vector.tcc\",fullname=\"/usr/include/c++/4.3.2/bits/vector.tcc\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_uninitialized.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_uninitialized.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/tr1_impl/unordered_map\",fullname=\"/usr/include/c++/4.3.2/tr1_impl/unordered_map\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/list.tcc\",fullname=\"/usr/include/c++/4.3.2/bits/list.tcc\"},{file=\"/usr/include/glibmm-2.4/glibmm/thread.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/thread.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/basic_string.h\",fullname=\"/usr/include/c++/4.3.2/bits/basic_string.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/char_traits.h\",fullname=\"/usr/include/c++/4.3.2/bits/char_traits.h\"},{file=\"/usr/include/glibmm-2.4/glibmm/ustring.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/ustring.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/iostream\",fullname=\"/usr/include/c++/4.3.2/iostream\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_iterator_base_funcs.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_iterator_base_funcs.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_construct.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_construct.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_iterator_base_types.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_iterator_base_types.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/tr1_impl/hashtable\",fullname=\"/usr/include/c++/4.3.2/tr1_impl/hashtable\"},{file=\"/usr/include/glibmm-2.4/glibmm/arrayhandle.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/arrayhandle.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_function.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_function.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_list.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_list.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/allocator.h\",fullname=\"/usr/include/c++/4.3.2/bits/allocator.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/ext/new_allocator.h\",fullname=\"/usr/include/c++/4.3.2/ext/new_allocator.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_vector.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_vector.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_iterator.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_iterator.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_pair.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_pair.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_algobase.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_algobase.h\"},{file=\"/usr/include/glibmm-2.4/glibmm/containerhandle_shared.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/containerhandle_shared.h\"},{file=\"nmv-log-stream.cc\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/tr1_impl/hashtable_policy.h\",fullname=\"/usr/include/c++/4.3.2/tr1_impl/hashtable_policy.h\"},{file=\"nmv-safe-ptr-utils.h\"},{file=\"nmv-safe-ptr.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/ios_base.h\",fullname=\"/usr/include/c++/4.3.2/bits/ios_base.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/new\",fullname=\"/usr/include/c++/4.3.2/new\"},{file=\"nmv-log-stream.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/src/common/nmv-log-stream.cc\"},{file=\"/usr/include/glibmm-2.4/glibmm/ustring.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/ustring.h\"},{file=\"nmv-safe-ptr.h\"},{file=\"nmv-libxml-utils.cc\"},{file=\"nmv-libxml-utils.h\"},{file=\"nmv-libxml-utils.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/src/common/nmv-libxml-utils.cc\"},{file=\"/usr/include/glibmm-2.4/glibmm/ustring.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/ustring.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_pair.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_pair.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_function.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_function.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_map.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_map.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/allocator.h\",fullname=\"/usr/include/c++/4.3.2/bits/allocator.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/ext/new_allocator.h\",fullname=\"/usr/include/c++/4.3.2/ext/new_allocator.h\"},{file=\"nmv-object.cc\"},{file=\"nmv-safe-ptr.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_tree.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_tree.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/new\",fullname=\"/usr/include/c++/4.3.2/new\"},{file=\"nmv-object.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/src/common/nmv-object.cc\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/vector.tcc\",fullname=\"/usr/include/c++/4.3.2/bits/vector.tcc\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_uninitialized.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_uninitialized.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/basic_string.tcc\",fullname=\"/usr/include/c++/4.3.2/bits/basic_string.tcc\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_iterator_base_funcs.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_iterator_base_funcs.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_iterator_base_types.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_iterator_base_types.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/ext/type_traits.h\",fullname=\"/usr/include/c++/4.3.2/ext/type_traits.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_algobase.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_algobase.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_construct.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_construct.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_function.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_function.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/char_traits.h\",fullname=\"/usr/include/c++/4.3.2/bits/char_traits.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/basic_string.h\",fullname=\"/usr/include/c++/4.3.2/bits/basic_string.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/allocator.h\",fullname=\"/usr/include/c++/4.3.2/bits/allocator.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/ext/new_allocator.h\",fullname=\"/usr/include/c++/4.3.2/ext/new_allocator.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_iterator.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_iterator.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/bits/stl_vector.h\",fullname=\"/usr/include/c++/4.3.2/bits/stl_vector.h\"},{file=\"nmv-ustring.cc\"},{file=\"nmv-safe-ptr-utils.h\"},{file=\"nmv-safe-ptr.h\"},{file=\"/usr/include/glibmm-2.4/glibmm/ustring.h\",fullname=\"/usr/include/glibmm-2.4/glibmm/ustring.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/new\",fullname=\"/usr/include/c++/4.3.2/new\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/ext/atomicity.h\",fullname=\"/usr/include/c++/4.3.2/ext/atomicity.h\"},{file=\"/usr/lib/gcc/x86_64-redhat-linux/4.3.2/../../../../include/c++/4.3.2/x86_64-redhat-linux/bits/gthr-default.h\",fullname=\"/usr/include/c++/4.3.2/x86_64-redhat-linux/bits/gthr-default.h\"},{file=\"nmv-ustring.cc\",fullname=\"/home/dodji/devel/git/nemiver.git/src/common/nmv-ustring.cc\"}]";

void
test_str0 ()
{
    bool is_ok =false;

    UString res;
    UString::size_type to=0;

    GDBMIParser parser (gv_str0);
    is_ok = parser.parse_c_string (0, to, res);
    BOOST_REQUIRE (is_ok);
    BOOST_REQUIRE (res == "abracadabra");
}

void
test_str1 ()
{
    bool is_ok =false;

    UString res;
    UString::size_type to=0;

    GDBMIParser parser (gv_str1);
    is_ok = parser.parse_c_string (0, to, res);

    BOOST_REQUIRE (is_ok);
    MESSAGE ("got string: '" << Glib::locale_from_utf8 (res) << "'");
    BOOST_REQUIRE_MESSAGE (res.size () == 32, "res size was: " << res.size ());
}

void
test_str2 ()
{
    bool is_ok =false;

    UString res;
    UString::size_type to=0;

    GDBMIParser parser (gv_str2);
    is_ok = parser.parse_c_string (0, to, res);

    BOOST_REQUIRE (is_ok);
    MESSAGE ("got string: '" << Glib::locale_from_utf8 (res) << "'");
    BOOST_REQUIRE_MESSAGE (res.size (), "res size was: " << res.size ());
}

void
test_str3 ()
{
    bool is_ok =false;

    UString res;
    UString::size_type to=0;

    GDBMIParser parser (gv_str3);
    is_ok = parser.parse_c_string (0, to, res);

    BOOST_REQUIRE (is_ok);
    MESSAGE ("got string: '" << Glib::locale_from_utf8 (res) << "'");
    BOOST_REQUIRE_MESSAGE (res.size (), "res size was: " << res.size ());
}

void
test_str4 ()
{
    bool is_ok =false;

    UString res;
    UString::size_type to=0;

    GDBMIParser parser (gv_str4);
    is_ok = parser.parse_c_string (0, to, res);

    BOOST_REQUIRE (is_ok);
    MESSAGE ("got string: '" << Glib::locale_from_utf8 (res) << "'");
    BOOST_REQUIRE_MESSAGE (res == "\"Eins\"", "res was: " << res);
}


void
test_attr0 ()
{
    bool is_ok =false;

    UString name,value;
    UString::size_type to=0;

    GDBMIParser parser (gv_attrs0);

    name.clear (), value.clear ();
    is_ok = parser.parse_attribute (0, to, name, value);
    BOOST_REQUIRE (is_ok);
    BOOST_REQUIRE_MESSAGE (name == "msg", "got name: " << name);
    BOOST_REQUIRE_MESSAGE (value.size(), "got empty value");

    name.clear (), value.clear ();
    parser.push_input (gv_attrs1);
    is_ok = parser.parse_attribute (0, to, name, value);
    BOOST_REQUIRE (is_ok);
    BOOST_REQUIRE_MESSAGE (name == "script", "got name: " << name);
    BOOST_REQUIRE_MESSAGE (value == "[silent,return]", "got empty value");

}

void
test_stoppped_async_output ()
{
    bool is_ok=false, got_frame=false;
    UString::size_type to=0;
    IDebugger::Frame frame;
    map<UString, UString> attrs;

    GDBMIParser parser (gv_stopped_async_output0);

    is_ok = parser.parse_stopped_async_output (0, to,
                                               got_frame,
                                               frame, attrs);
    BOOST_REQUIRE (is_ok);
    BOOST_REQUIRE (got_frame);
    BOOST_REQUIRE (attrs.size ());

    to=0;
    parser.push_input (gv_stopped_async_output1);
    is_ok = parser.parse_stopped_async_output (0, to, got_frame, frame, attrs);
    BOOST_REQUIRE (is_ok);
    BOOST_REQUIRE (got_frame);
    BOOST_REQUIRE (attrs.size ());
}

void
test_running_async_output ()
{
    bool is_ok=false;
    UString::size_type to=0;
    int thread_id=0;

    GDBMIParser parser (gv_running_async_output0);

    is_ok = parser.parse_running_async_output (0, to, thread_id);
    BOOST_REQUIRE (is_ok);
    BOOST_REQUIRE (thread_id == -1);

    parser.push_input (gv_running_async_output1);

    to=0;
    is_ok = parser.parse_running_async_output (0, to, thread_id);
    BOOST_REQUIRE (is_ok);
    BOOST_REQUIRE (thread_id == 1);
}

void
test_var_list_children ()
{

    GDBMIParser parser (gv_var_list_children0);

    bool is_ok=false;
    UString::size_type to=0;
    std::vector<IDebugger::VariableSafePtr> vars;
    is_ok = parser.parse_var_list_children (0, to, vars);
    BOOST_REQUIRE (is_ok);
    BOOST_REQUIRE (vars.size () == 2);
}

void
test_output_record ()
{
    bool is_ok=false;
    UString::size_type to=0;
    Output output;

    GDBMIParser parser (gv_output_record0);
    is_ok = parser.parse_output_record (0, to, output);
    BOOST_REQUIRE (is_ok);

    parser.push_input (gv_output_record1);
    is_ok = parser.parse_output_record (0, to, output);
    BOOST_REQUIRE (is_ok);

    parser.push_input (gv_output_record2);
    is_ok = parser.parse_output_record (0, to, output);
    BOOST_REQUIRE (is_ok);

    parser.push_input (gv_output_record3);
    is_ok = parser.parse_output_record (0, to, output);
    BOOST_REQUIRE (is_ok);

    // gv_output_record4 should result in a variable.
    parser.push_input (gv_output_record4);
    is_ok = parser.parse_output_record (0, to, output);
    BOOST_REQUIRE (is_ok);
    BOOST_REQUIRE (output.has_result_record ());
    BOOST_REQUIRE (output.result_record ().has_variable ());
    BOOST_REQUIRE (output.result_record ().variable ());
    BOOST_REQUIRE (output.result_record ().variable ()->name ().empty ());
    BOOST_REQUIRE (output.result_record ().variable ()->internal_name ()
                   == "var1");
    BOOST_REQUIRE (output.result_record ().variable ()->type () == "Person");

    // gv_output_record5 should result in 2 deleted variables.
    parser.push_input (gv_output_record5);
    is_ok = parser.parse_output_record (0, to, output);
    BOOST_REQUIRE (is_ok);
    BOOST_REQUIRE (output.result_record ().number_of_variables_deleted () == 2);

    // gv_output_record6 should result in 3 children variables.
    parser.push_input (gv_output_record6);
    is_ok = parser.parse_output_record (0, to, output);
    BOOST_REQUIRE (is_ok);
    BOOST_REQUIRE (output.result_record ().has_variable_children ());
    vector<IDebugger::VariableSafePtr> v =
            output.result_record ().variable_children ();
    BOOST_REQUIRE (v.size () == 3);

    int i = 0;
    BOOST_REQUIRE (v[i]
                   && v[i]->name () == "m_first_name"
                   && v[i]->type () == "string"
                   && v[i]->internal_name () == "var2.public.m_first_name");

    i = 1;
    BOOST_REQUIRE (v[i]
                   && v[i]->name () == "m_family_name"
                   && v[i]->type () == "string"
                   && v[i]->internal_name () == "var2.public.m_family_name");

    i = 2;
    BOOST_REQUIRE (v[i]
                   && v[i]->name () == "m_age"
                   && v[i]->type () == "unsigned int"
                   && v[i]->internal_name () == "var2.public.m_age");

    // gv_output_record7 should result in 1 variable.
    parser.push_input (gv_output_record7);
    is_ok = parser.parse_output_record (0, to, output);
    BOOST_REQUIRE (is_ok);
    BOOST_REQUIRE (output.result_record ().has_var_changes ());

    // gv_output_record8 should result in 0 variable.
    parser.push_input (gv_output_record8);
    is_ok = parser.parse_output_record (0, to, output);
    BOOST_REQUIRE (is_ok);
    BOOST_REQUIRE (output.result_record ().has_var_changes ());

    // gv_output_record9 should contain a list of 2 var_changes.
    parser.push_input (gv_output_record9);
    is_ok = parser.parse_output_record (0, to, output);
    BOOST_REQUIRE (is_ok);
    BOOST_REQUIRE (output.result_record ().has_var_changes ());
    BOOST_REQUIRE (output.result_record ().var_changes ().size () == 2);
    {
        // The first var_change should represent a variable of internal
        // name "var1" that has one new child named var1.[1]
        list<nemiver::VarChangePtr>::const_iterator it =
            output.result_record ().var_changes ().begin ();
        BOOST_REQUIRE ((*it)->variable ()->internal_name () == "var1");
        BOOST_REQUIRE ((*it)->new_children ().size () == 1);
        BOOST_REQUIRE ((*it)->new_children ().front ()-> internal_name ()
                       == "var1.[1]");

        // The second var_change should represent a variable of
        // internal name var1.[0] which value is "kélé".
        ++it;
        BOOST_REQUIRE ((*it)->variable ()->internal_name () == "var1.[0]");
        BOOST_REQUIRE ((*it)->variable ()->value () == "\"kélé\"");
    }
}

void
test_stack0 ()
{
    UString::size_type to = 0;
    GDBMIParser parser (gv_stack0);
    vector<IDebugger::Frame> call_stack;
    bool is_ok = parser.parse_call_stack (0, to, call_stack);
    BOOST_REQUIRE (is_ok);
    for (unsigned i = 0; i < call_stack.size (); ++i) {
        BOOST_REQUIRE (call_stack[i].level () >= 0
                       && call_stack[i].level () == (int) i);
        BOOST_REQUIRE (!call_stack[i].function_name ().empty ());
        BOOST_REQUIRE (!call_stack[i].has_empty_address ());
        BOOST_REQUIRE (!call_stack[i].file_name ().empty ());
        BOOST_REQUIRE (!call_stack[i].file_full_name ().empty ());
        BOOST_REQUIRE (call_stack[i].line ());
    }
}

void
test_stack_arguments0 ()
{
    bool is_ok=false;
    UString::size_type to;
    map<int, list<IDebugger::VariableSafePtr> >params;

    GDBMIParser parser (gv_stack_arguments0);
    is_ok = parser.parse_stack_arguments (0, to, params);
    BOOST_REQUIRE (is_ok);
    BOOST_REQUIRE (params.size () == 2);
    map<int, list<IDebugger::VariableSafePtr> >::iterator param_iter;
    param_iter = params.find (0);
    BOOST_REQUIRE (param_iter != params.end ());
    IDebugger::VariableSafePtr variable = *(param_iter->second.begin ());
    BOOST_REQUIRE (variable);
    BOOST_REQUIRE (variable->name () == "a_param");
    BOOST_REQUIRE (!variable->members ().empty ());
}

void
test_stack_arguments1 ()
{
    bool is_ok=false;
    UString::size_type to;
    map<int, list<IDebugger::VariableSafePtr> >params;

    GDBMIParser parser (gv_stack_arguments1);
    is_ok = parser.parse_stack_arguments (0, to, params);
    BOOST_REQUIRE (is_ok);
    BOOST_REQUIRE_MESSAGE (params.size () == 18, "got nb params "
                           << params.size ());
    map<int, list<IDebugger::VariableSafePtr> >::iterator param_iter;
    param_iter = params.find (0);
    BOOST_REQUIRE (param_iter != params.end ());
    IDebugger::VariableSafePtr variable = *(param_iter->second.begin ());
    BOOST_REQUIRE (variable);
    BOOST_REQUIRE (variable->name () == "a_comp");
    BOOST_REQUIRE (variable->members ().empty ());
}

void
test_local_vars ()
{
    bool is_ok=false;
    UString::size_type to=0;
    list<IDebugger::VariableSafePtr> vars;

    GDBMIParser parser (gv_local_vars);
    is_ok = parser.parse_local_var_list (0, to, vars);
    BOOST_REQUIRE (is_ok);
    BOOST_REQUIRE (vars.size () == 1);
    IDebugger::VariableSafePtr var = *(vars.begin ());
    BOOST_REQUIRE (var);
    BOOST_REQUIRE (var->name () == "person");
    BOOST_REQUIRE (var->type () == "Person");
}

void
test_member_variable ()
{
    {
        UString::size_type to = 0;
        IDebugger::VariableSafePtr var (new IDebugger::Variable);

        GDBMIParser parser (gv_member_var);
        BOOST_REQUIRE (parser.parse_member_variable (0, to, var));
        BOOST_REQUIRE (var);
        BOOST_REQUIRE (!var->members ().empty ());
    }

    {
        UString::size_type to = 0;
        IDebugger::VariableSafePtr var (new IDebugger::Variable);

        GDBMIParser parser (gv_member_var2);
        BOOST_REQUIRE (parser.parse_member_variable (0, to, var));
        BOOST_REQUIRE (var);
        BOOST_REQUIRE (!var->members ().empty ());
    }
}

void
test_var_with_member_variable ()
{
    UString::size_type to = 0;
    IDebugger::VariableSafePtr var (new IDebugger::Variable);

    GDBMIParser parser (gv_var_with_member);
    BOOST_REQUIRE (parser.parse_variable_value (0, to, var));
    BOOST_REQUIRE (var);
    BOOST_REQUIRE (!var->members ().empty ());

    to = 0;
    var.reset (new IDebugger::Variable);
    parser.push_input (gv_var_with_member2);
    BOOST_REQUIRE (parser.parse_variable_value (0, to, var));
    BOOST_REQUIRE (var);
    BOOST_REQUIRE (!var->members ().empty ());

    to = 0;
    var.reset (new IDebugger::Variable);
    parser.push_input (gv_var_with_member3);
    BOOST_REQUIRE (parser.parse_variable_value (0, to, var));
    BOOST_REQUIRE (var);
    BOOST_REQUIRE (!var->members ().empty ());

    to = 0;
    var.reset (new IDebugger::Variable);
    parser.push_input (gv_var_with_member4);
    BOOST_REQUIRE (parser.parse_variable_value (0, to, var));
    BOOST_REQUIRE (var);
    BOOST_REQUIRE (!var->members ().empty ());

    to = 0;
    var.reset (new IDebugger::Variable);
    parser.push_input (gv_var_with_member5);
    BOOST_REQUIRE (parser.parse_variable_value (0, to, var));
    BOOST_REQUIRE (var);
    BOOST_REQUIRE (!var->members ().empty ());

    to = 0;
    var.reset (new IDebugger::Variable);
    parser.push_input (gv_var_with_member6);
    BOOST_REQUIRE (parser.parse_variable_value (0, to, var));
    BOOST_REQUIRE (var);
    BOOST_REQUIRE (!var->members ().empty ());

    to = 0;
    var.reset (new IDebugger::Variable);
    parser.push_input (gv_var_with_member7);
    BOOST_REQUIRE (parser.parse_variable_value (0, to, var));
    BOOST_REQUIRE (var);
    BOOST_REQUIRE (!var->members ().empty ());
}

void
test_var_with_comma ()
{
    // gdb sometimes returns values that don't exactly conform to spec, for
    // instance, for function pointers, it appends the name and signature of the
    // function withing < > after the address.  If the value is a member
    // variable, we have to make sure that we don't treat the commas from the
    // function declaration as a delimiter between member variables.  e.g.
    // {A = 0, b = 1 } is two member variables,
    // {A = 0x1234 <void foo(int, int)>} is a single member variable

    UString::size_type to = 0;
    IDebugger::VariableSafePtr var (new IDebugger::Variable);

    GDBMIParser parser (gv_var_with_member8);

    BOOST_REQUIRE (parser.parse_variable_value (0, to, var));
    BOOST_REQUIRE (var);
    BOOST_REQUIRE (1 == var->members ().size ());
    BOOST_REQUIRE_EQUAL("0x40085e <my_func(void*, void*)>", (*var->members ().begin ())->value ());

    to = 0;
    var.reset (new IDebugger::Variable);
    parser.push_input (gv_var_with_comma);
    BOOST_REQUIRE (parser.parse_variable_value (0, to, var));
    BOOST_REQUIRE (var);
    BOOST_REQUIRE (var->members ().empty ());
    BOOST_REQUIRE_EQUAL("0x40085e <my_func(void*, void*)>", var->value ());
}

void
test_embedded_string ()
{
    UString::size_type to = 0;
    UString str;
    GDBMIParser parser (gv_emb_str);
    BOOST_REQUIRE (parser.parse_embedded_c_string (0, to, str));
}

void
test_overloads_prompt ()
{
    vector<IDebugger::OverloadsChoiceEntry> prompts;
    UString::size_type cur = 0;

    GDBMIParser parser (gv_overloads_prompt0);
    BOOST_REQUIRE (parser.parse_overloads_choice_prompt (cur, cur, prompts));
    BOOST_REQUIRE_MESSAGE (prompts.size () == 4,
                           "actually got " << prompts.size ());

    cur=0;
    prompts.clear ();
    parser.push_input (gv_overloads_prompt1);
    BOOST_REQUIRE (parser.parse_overloads_choice_prompt (cur, cur, prompts));
    BOOST_REQUIRE_MESSAGE (prompts.size () == 4,
                           "actually got " << prompts.size ());
}

void
test_register_names ()
{
    std::map<IDebugger::register_id_t, UString> regs;
    UString::size_type cur = 0;

    GDBMIParser parser (gv_register_names);

    BOOST_REQUIRE (parser.parse_register_names (cur, cur, regs));
    BOOST_REQUIRE_EQUAL (regs.size (), 50u);
    BOOST_REQUIRE_EQUAL (regs[0], "eax");
    BOOST_REQUIRE_EQUAL (regs[1], "ecx");
    BOOST_REQUIRE_EQUAL (regs[2], "edx");
    BOOST_REQUIRE_EQUAL (regs[3], "ebx");
    BOOST_REQUIRE_EQUAL (regs[4], "esp");
    BOOST_REQUIRE_EQUAL (regs[5], "ebp");
    BOOST_REQUIRE_EQUAL (regs[6], "esi");
    BOOST_REQUIRE_EQUAL (regs[7], "edi");
    BOOST_REQUIRE_EQUAL (regs[8], "eip");
    BOOST_REQUIRE_EQUAL (regs[9], "eflags");
    BOOST_REQUIRE_EQUAL (regs[10], "cs");
    BOOST_REQUIRE_EQUAL (regs[11], "ss");
    BOOST_REQUIRE_EQUAL (regs[12], "ds");
    BOOST_REQUIRE_EQUAL (regs[13], "es");
    BOOST_REQUIRE_EQUAL (regs[14], "fs");
    BOOST_REQUIRE_EQUAL (regs[15], "gs");
    BOOST_REQUIRE_EQUAL (regs[16], "st0");
    BOOST_REQUIRE_EQUAL (regs[17], "st1");
    BOOST_REQUIRE_EQUAL (regs[18], "st2");
    BOOST_REQUIRE_EQUAL (regs[19], "st3");
    BOOST_REQUIRE_EQUAL (regs[20], "st4");
    BOOST_REQUIRE_EQUAL (regs[21], "st5");
    BOOST_REQUIRE_EQUAL (regs[22], "st6");
    BOOST_REQUIRE_EQUAL (regs[23], "st7");
    BOOST_REQUIRE_EQUAL (regs[24], "fctrl");
    BOOST_REQUIRE_EQUAL (regs[25], "fstat");
    BOOST_REQUIRE_EQUAL (regs[26], "ftag");
    BOOST_REQUIRE_EQUAL (regs[27], "fiseg");
    BOOST_REQUIRE_EQUAL (regs[28], "fioff");
    BOOST_REQUIRE_EQUAL (regs[29], "foseg");
    BOOST_REQUIRE_EQUAL (regs[30], "fooff");
    BOOST_REQUIRE_EQUAL (regs[31], "fop");
    BOOST_REQUIRE_EQUAL (regs[32], "xmm0");
    BOOST_REQUIRE_EQUAL (regs[33], "xmm1");
    BOOST_REQUIRE_EQUAL (regs[34], "xmm2");
    BOOST_REQUIRE_EQUAL (regs[35], "xmm3");
    BOOST_REQUIRE_EQUAL (regs[36], "xmm4");
    BOOST_REQUIRE_EQUAL (regs[37], "xmm5");
    BOOST_REQUIRE_EQUAL (regs[38], "xmm6");
    BOOST_REQUIRE_EQUAL (regs[39], "xmm7");
    BOOST_REQUIRE_EQUAL (regs[40], "mxcsr");
    BOOST_REQUIRE_EQUAL (regs[41], "orig_eax");
    BOOST_REQUIRE_EQUAL (regs[42], "mm0");
    BOOST_REQUIRE_EQUAL (regs[43], "mm1");
    BOOST_REQUIRE_EQUAL (regs[44], "mm2");
    BOOST_REQUIRE_EQUAL (regs[45], "mm3");
    BOOST_REQUIRE_EQUAL (regs[46], "mm4");
    BOOST_REQUIRE_EQUAL (regs[47], "mm5");
    BOOST_REQUIRE_EQUAL (regs[48], "mm6");
    BOOST_REQUIRE_EQUAL (regs[49], "mm7");
}

void
test_changed_registers ()
{
    std::list<IDebugger::register_id_t> regs;
    UString::size_type cur = 0;

    GDBMIParser parser (gv_changed_registers);
    BOOST_REQUIRE (parser.parse_changed_registers (cur, cur, regs));
    BOOST_REQUIRE_EQUAL (regs.size (), 18u);
    std::list<IDebugger::register_id_t>::const_iterator reg_iter = regs.begin ();
    BOOST_REQUIRE_EQUAL (*reg_iter++, 0u);
    BOOST_REQUIRE_EQUAL (*reg_iter++, 1u);
    BOOST_REQUIRE_EQUAL (*reg_iter++, 2u);
    BOOST_REQUIRE_EQUAL (*reg_iter++, 3u);
    BOOST_REQUIRE_EQUAL (*reg_iter++, 4u);
    BOOST_REQUIRE_EQUAL (*reg_iter++, 5u);
    BOOST_REQUIRE_EQUAL (*reg_iter++, 6u);
    BOOST_REQUIRE_EQUAL (*reg_iter++, 8u);
    BOOST_REQUIRE_EQUAL (*reg_iter++, 9u);
    BOOST_REQUIRE_EQUAL (*reg_iter++, 10u);
    BOOST_REQUIRE_EQUAL (*reg_iter++, 11u);
    BOOST_REQUIRE_EQUAL (*reg_iter++, 12u);
    BOOST_REQUIRE_EQUAL (*reg_iter++, 13u);
    BOOST_REQUIRE_EQUAL (*reg_iter++, 15u);
    BOOST_REQUIRE_EQUAL (*reg_iter++, 24u);
    BOOST_REQUIRE_EQUAL (*reg_iter++, 26u);
    BOOST_REQUIRE_EQUAL (*reg_iter++, 40u);
    BOOST_REQUIRE_EQUAL (*reg_iter++, 41u);
}

void
test_register_values ()
{
    std::map<IDebugger::register_id_t, UString> reg_values;
    UString::size_type cur = 0;

    GDBMIParser parser (gv_register_values);
    BOOST_REQUIRE (parser.parse_register_values (cur, cur, reg_values));
    BOOST_REQUIRE_EQUAL (reg_values.size (), 11u);
    std::map<IDebugger::register_id_t, UString>::const_iterator
                                                reg_iter = reg_values.begin ();
    BOOST_REQUIRE_EQUAL ((reg_iter)->first, 1u);
    BOOST_REQUIRE_EQUAL ((reg_iter)->second, "0xbfd10a60");
    ++reg_iter;
    BOOST_REQUIRE_EQUAL ((reg_iter)->first, 2u);
    BOOST_REQUIRE_EQUAL ((reg_iter)->second, "0x1");
    ++reg_iter;
    BOOST_REQUIRE_EQUAL ((reg_iter)->first, 3u);
    BOOST_REQUIRE_EQUAL ((reg_iter)->second, "0xb6f03ff4");
    ++reg_iter;
    BOOST_REQUIRE_EQUAL ((reg_iter)->first, 4u);
    BOOST_REQUIRE_EQUAL ((reg_iter)->second, "0xbfd10960");
    ++reg_iter;
    BOOST_REQUIRE_EQUAL ((reg_iter)->first, 5u);
    BOOST_REQUIRE_EQUAL ((reg_iter)->second, "0xbfd10a48");
    ++reg_iter;
    BOOST_REQUIRE_EQUAL ((reg_iter)->first, 6u);
    BOOST_REQUIRE_EQUAL ((reg_iter)->second, "0xb7ff6ce0");
    ++reg_iter;
    BOOST_REQUIRE_EQUAL ((reg_iter)->first, 7u);
    BOOST_REQUIRE_EQUAL ((reg_iter)->second, "0x0");
    ++reg_iter;
    BOOST_REQUIRE_EQUAL ((reg_iter)->first, 8u);
    BOOST_REQUIRE_EQUAL ((reg_iter)->second, "0x80bb710");
    ++reg_iter;
    BOOST_REQUIRE_EQUAL ((reg_iter)->first, 9u);
    BOOST_REQUIRE_EQUAL ((reg_iter)->second, "0x200286");
    ++reg_iter;
    BOOST_REQUIRE_EQUAL ((reg_iter)->first, 10u);
    BOOST_REQUIRE_EQUAL ((reg_iter)->second, "0x73");
    ++reg_iter;
    BOOST_REQUIRE_EQUAL ((reg_iter)->first, 36u);
    BOOST_REQUIRE_EQUAL ((reg_iter)->second, "{v4_float = {0x0, 0x0, 0x0, 0x0}, v2_double = {0x0, 0x0}, v16_int8 = {0x0 <repeats 16 times>}, v8_int16 = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, v4_int32 = {0x0, 0x0, 0x0, 0x0}, v2_int64 = {0x0, 0x0}, uint128 = 0x00000000000000000000000000000000}");
}

void
test_memory_values ()
{
    std::vector<uint8_t> mem_values;
    size_t start_addr;
    UString::size_type cur = 0;

    GDBMIParser parser (gv_memory_values);
    BOOST_REQUIRE (parser.parse_memory_values (cur, cur, start_addr, mem_values));
    BOOST_REQUIRE_EQUAL (start_addr, 0x000013a0u);
    BOOST_REQUIRE_EQUAL (mem_values.size (), 4u);
    std::vector<uint8_t>::const_iterator mem_iter = mem_values.begin ();
    BOOST_REQUIRE_EQUAL (*mem_iter, 0x10u);
    ++mem_iter;
    BOOST_REQUIRE_EQUAL (*mem_iter, 0x11u);
    ++mem_iter;
    BOOST_REQUIRE_EQUAL (*mem_iter, 0x12u);
    ++mem_iter;
    BOOST_REQUIRE_EQUAL (*mem_iter, 0x13u);
}

void
test_gdbmi_result ()
{
    GDBMIResultSafePtr result;
    UString::size_type cur = 0;

    GDBMIParser parser (gv_gdbmi_result0);
    bool is_ok = parser.parse_gdbmi_result (cur, cur, result);
    BOOST_REQUIRE (is_ok && result && !result->is_singular ());

    parser.push_input (gv_gdbmi_result1);
    parser.set_mode (GDBMIParser::BROKEN_MODE);
    cur = 0;
    is_ok = parser.parse_gdbmi_result (cur, cur, result);
    BOOST_REQUIRE (is_ok
                   && result
                   && result->is_singular ()
                   && result->variable () == "variable");

    parser.push_input (gv_gdbmi_result2);
    cur = 0;
    is_ok = parser.parse_gdbmi_result (cur, cur, result);
    BOOST_REQUIRE (is_ok
                   && result
                   && result->is_singular ()
                   && result->variable () == "variable");
}

void
test_breakpoint_table ()
{
    std::map<string, IDebugger::Breakpoint> breakpoints;
    UString::size_type cur = 0;

    GDBMIParser parser (gv_breakpoint_table0);
    BOOST_REQUIRE (parser.parse_breakpoint_table (cur, cur, breakpoints));
    BOOST_REQUIRE_EQUAL (breakpoints.size (), 1u);
    std::map<string, IDebugger::Breakpoint>::const_iterator iter;
    iter = breakpoints.find ("1");
    BOOST_REQUIRE (iter != breakpoints.end ());
    BOOST_REQUIRE_EQUAL (iter->second.id (), "1");
    BOOST_REQUIRE (iter->second.enabled ());
    BOOST_REQUIRE_EQUAL (iter->second.address (), "0x08081566");
    BOOST_REQUIRE_EQUAL (iter->second.function (), "main");
    BOOST_REQUIRE_EQUAL (iter->second.file_name (), "main.cc");
    BOOST_REQUIRE_EQUAL (iter->second.file_full_name (),
                         "/home/jonathon/Projects/agave.git/src/main.cc");
    BOOST_REQUIRE_EQUAL (iter->second.line (), 70);

    cur = 0, cur = 0, breakpoints.clear ();
    parser.push_input (gv_breakpoint_table1);
    BOOST_REQUIRE (parser.parse_breakpoint_table (cur, cur, breakpoints));

    cur = 0, cur = 0, breakpoints.clear ();
    parser.push_input (gv_breakpoint_table2);
    BOOST_REQUIRE (parser.parse_breakpoint_table (cur, cur, breakpoints));

    cur = 0, cur = 0, breakpoints.clear ();
    parser.push_input (gv_breakpoint_table3);
    parser.set_mode (GDBMIParser::BROKEN_MODE);
    BOOST_REQUIRE (parser.parse_breakpoint_table (cur, cur, breakpoints));

    cur = 0, cur = 0, breakpoints.clear ();
    parser.push_input (gv_breakpoint_table4);
    parser.set_mode (GDBMIParser::BROKEN_MODE);
    BOOST_REQUIRE (parser.parse_breakpoint_table (cur, cur, breakpoints));
    BOOST_REQUIRE_EQUAL (breakpoints["2"].file_full_name (),
                         "/home/philip/nemiver:temp/main.cpp");
    BOOST_REQUIRE_EQUAL (breakpoints["2"].line (), 78);

    cur = 0, cur = 0, breakpoints.clear ();
    parser.push_input (gv_breakpoint_table5);
    parser.set_mode (GDBMIParser::BROKEN_MODE);
    BOOST_REQUIRE (parser.parse_breakpoint_table (cur, cur, breakpoints));
}

void
test_breakpoint ()
{
    IDebugger::Breakpoint breakpoint;

    GDBMIParser parser (gv_breakpoint0);
    UString::size_type cur = 0;
    bool is_ok = parser.parse_breakpoint (0, cur, breakpoint);
    BOOST_REQUIRE (is_ok);
    BOOST_REQUIRE (!breakpoint.file_full_name ().empty ()
                   && breakpoint.line ());

    parser.push_input (gv_breakpoint1);
    breakpoint.clear ();
    is_ok = parser.parse_breakpoint (0, cur, breakpoint);
    BOOST_REQUIRE (is_ok);
    BOOST_REQUIRE (breakpoint.file_full_name ().empty ()
                   && !breakpoint.line ());

    parser.push_input (gv_breakpoint2);
    breakpoint.clear ();
    is_ok = parser.parse_breakpoint (0, cur, breakpoint);
    BOOST_REQUIRE (is_ok);
    BOOST_REQUIRE_EQUAL (breakpoint.file_full_name (),
                         "/home/philip/nemiver:temp/main.cpp");
    BOOST_REQUIRE_EQUAL (breakpoint.line (), 78);

    parser.push_input (gv_breakpoint3);
    breakpoint.clear ();
    is_ok = parser.parse_breakpoint (0, cur, breakpoint);
    BOOST_REQUIRE (is_ok);
    BOOST_REQUIRE (breakpoint.has_multiple_locations ());
    BOOST_REQUIRE_EQUAL (breakpoint.sub_breakpoints ().size (), 2);
    BOOST_REQUIRE_EQUAL (breakpoint.sub_breakpoints ()[0].id (), "2.1");
    BOOST_REQUIRE_EQUAL (breakpoint.sub_breakpoints ()[1].id (), "2.2");

    parser.push_input (gv_breakpoint_modified_async_output0);
    breakpoint.clear ();
    is_ok = parser.parse_breakpoint_modified_async_output (0, cur, breakpoint);
    BOOST_REQUIRE (is_ok);
    BOOST_REQUIRE (breakpoint.has_multiple_locations ());
    BOOST_REQUIRE_EQUAL (breakpoint.sub_breakpoints ().size (), 10);
    BOOST_REQUIRE_EQUAL (breakpoint.sub_breakpoints ()[0].id (), "2.1");
    BOOST_REQUIRE_EQUAL (breakpoint.sub_breakpoints ()[9].id (), "2.10");
}

void
test_disassemble ()
{
    typedef list<common::Asm> AsmInstrList;
    AsmInstrList instrs;
    UString::size_type cur = 0;
    GDBMIParser parser (gv_disassemble0);
    BOOST_REQUIRE (parser.parse_asm_instruction_list (cur, cur, instrs));
    int nb_instrs = instrs.size ();
    // There should be 253 assembly instructions in gv_disassemble0.
    // Yes, I counted them all.
    BOOST_REQUIRE_MESSAGE (nb_instrs == 253, "nb_instrs was: " << nb_instrs);
    std::cout << "========== asm instructions =============\n";
    for (AsmInstrList::const_iterator it = instrs.begin ();
         it != instrs.end ();
         ++it) {
        std::cout << *it;
    }
    std::cout << "========== end of asm instructions =============\n";

    Output::ResultRecord record;
    cur = 0;
    parser.push_input (gv_disassemble1);
    BOOST_REQUIRE (parser.parse_result_record (cur, cur, record));
    BOOST_REQUIRE (record.has_asm_instruction_list ());
    nb_instrs = record.asm_instruction_list ().size ();
    // There should be 253 assembly instructions in gv_disassemble1.
    BOOST_REQUIRE_MESSAGE (nb_instrs == 253, "nb_instrs was: " << nb_instrs);

    parser.push_input (gv_disassemble2);
    cur = 0;
    instrs.clear ();
    BOOST_REQUIRE (parser.parse_asm_instruction_list (cur, cur, instrs));
    BOOST_REQUIRE_MESSAGE (instrs.size () == 16,
                           "nb instrs was: " << instrs.size ());

    parser.push_input (gv_disassemble3);
    cur = 0;
    instrs.clear ();
    BOOST_REQUIRE (parser.parse_asm_instruction_list (cur, cur, instrs));
    BOOST_REQUIRE (!instrs.empty ());
    BOOST_REQUIRE (!instrs.front ().empty ());

    instrs.clear ();
    parser.push_input (gv_disassemble4);
    cur = 0;
    BOOST_REQUIRE (parser.parse_asm_instruction_list (cur, cur, instrs));
    BOOST_REQUIRE (!instrs.empty ());
    BOOST_REQUIRE (instrs.front ().empty ());
}

void
test_file_list ()
{
    UString::size_type from=0, to=0;
    std::vector<UString> files;
    GDBMIParser parser (gv_file_list1);

    bool is_ok = parser.parse_file_list (from, to, files);
    int num_files = files.size ();

    BOOST_REQUIRE (is_ok);
    BOOST_REQUIRE (num_files == 126);
}

using boost::unit_test::test_suite;

NEMIVER_API test_suite*
init_unit_test_suite (int /*argc*/, char ** /*argv*/)
{
    NEMIVER_TRY

    nemiver::common::Initializer::do_init ();

    test_suite *suite = BOOST_TEST_SUITE ("GDBMI tests");
    suite->add (BOOST_TEST_CASE (&test_str0));
    suite->add (BOOST_TEST_CASE (&test_str1));
    suite->add (BOOST_TEST_CASE (&test_str2));
    suite->add (BOOST_TEST_CASE (&test_str3));
    suite->add (BOOST_TEST_CASE (&test_str4));
    suite->add (BOOST_TEST_CASE (&test_attr0));
    suite->add (BOOST_TEST_CASE (&test_stoppped_async_output));
    suite->add (BOOST_TEST_CASE (&test_running_async_output));
    suite->add (BOOST_TEST_CASE (&test_var_list_children));
    suite->add (BOOST_TEST_CASE (&test_output_record));
    suite->add (BOOST_TEST_CASE (&test_stack0));
    suite->add (BOOST_TEST_CASE (&test_stack_arguments0));
    suite->add (BOOST_TEST_CASE (&test_stack_arguments1));
    suite->add (BOOST_TEST_CASE (&test_local_vars));
    suite->add (BOOST_TEST_CASE (&test_member_variable));
    suite->add (BOOST_TEST_CASE (&test_var_with_member_variable));
    suite->add (BOOST_TEST_CASE (&test_var_with_comma));
    suite->add (BOOST_TEST_CASE (&test_embedded_string));
    suite->add (BOOST_TEST_CASE (&test_overloads_prompt));
    suite->add (BOOST_TEST_CASE (&test_register_names));
    suite->add (BOOST_TEST_CASE (&test_changed_registers));
    suite->add (BOOST_TEST_CASE (&test_register_values));
    suite->add (BOOST_TEST_CASE (&test_memory_values));
    suite->add (BOOST_TEST_CASE (&test_gdbmi_result));
    suite->add (BOOST_TEST_CASE (&test_breakpoint_table));
    suite->add (BOOST_TEST_CASE (&test_breakpoint));
    suite->add (BOOST_TEST_CASE (&test_disassemble));
    suite->add (BOOST_TEST_CASE (&test_file_list));
    return suite;

    NEMIVER_CATCH_NOX

    return 0;
}

