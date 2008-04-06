#include <list>
#include <map>
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

static const char* gv_stopped_async_output0 =
"*stopped,reason=\"breakpoint-hit\",bkptno=\"1\",thread-id=\"1\",frame={addr=\"0x0804afb0\",func=\"main\",args=[{name=\"argc\",value=\"1\"},{name=\"argv\",value=\"0xbfc79ed4\"}],file=\"today-main.c\",fullname=\"/home/dodji/devel/gitstore/omoko.git/applications/openmoko-today/src/today-main.c\",line=\"285\"}\n" ;

static const char* gv_stopped_async_output1 =
"*stopped,reason=\"breakpoint-hit\",bkptno=\"1\",thread-id=\"1\",frame={addr=\"0x08048d38\",func=\"main\",args=[],file=\"fooprog.cc\",fullname=\"/opt/dodji/git/nemiver.git/tests/fooprog.cc\",line=\"80\"}\n";


static const char *gv_output_record0 =
"&\"Failed to read a valid object file image from memory.\\n\"\n"
"~\"[Thread debugging using libthread_db enabled]\\n\"\n"
"~\"[New Thread 0xb7892720 (LWP 20182)]\\n\"\n"
"=thread-created,id=1\n"
"*stopped,reason=\"breakpoint-hit\",bkptno=\"1\",thread-id=\"1\",frame={addr=\"0x08048d38\",func=\"main\",args=[],file=\"fooprog.cc\",fullname=\"/opt/dodji/git/nemiver.git/tests/fooprog.cc\",line=\"80\"}\n"
"(gdb)";

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

static const char *gv_var_with_member2 = "value=\"{filePos = 393216, ptr = 0xbf83c3f4 \\\"q\\\\354p1i\\\\233w\\\\310\\\\270R\\\\376\\\\325-\\\\266\\\\235$Qy\\\\212\\\\371\\\\373;|\\\\271\\\\031\\\\311\\\\355\\\\223]K\\\\031x4\\\\374\\\\217z\\\\272\\\\366t\\\\037\\\\2237'\\\\324S\\\\354\\\\321\\\\306\\\\020\\\\233\\\\202>y\\\\024\\\\365\\\\250\\\\\\\"\\\\271\\\\275(D\\\\267\\\\022\\\\205\\\\330B\\\\200\\\\371\\\\371k/\\\\252S\\\\204[\\\\265\\\\373\\\\036\\\\025\\\\fC\\\\251Y\\\\312\\\\333\\\\225\\\\231\\\\247$\\\\024-\\\\273\\\\035KsZV\\\\217r\\\\320I\\\\031gb\\\\347\\\\0371\\\\347\\\\374\\\\361I\\\\323\\\\204\\\\254\\\\337A\\\\271\\\\250\\\\302O\\\\271c)\\\\004\\\\211\\\\r\\\\303\\\\252\\\\273\\\\377\\\", limit = 0xbf85c3f4 \\\"\\\\310\\\\243\\\\020\\\\b\\\\330\\\\274\\\\021\\\\b\\\\f9\\\\020\\\\b\\\\f9\\\\020\\\\b\\\\344\\\\274\\\\022\\\\b\\\\377\\\\355\\\", len = 131072, data = {113 'q', 236 '\\\\354', 112 'p', 49 '1', 105 'i', 155 '\\\\233', 119 'w', 200 '\\\\310', 184 '\\\\270', 82 'R', 254 '\\\\376', 213 '\\\\325', 45 '-', 182 '\\\\266', 157 '\\\\235', 36 '$', 81 'Q', 121 'y', 138 '\\\\212', 249 '\\\\371', 251 '\\\\373', 59 ';', 124 '|', 185 '\\\\271', 25 '\\\\031', 201 '\\\\311', 237 '\\\\355', 147 '\\\\223', 93 ']', 75 'K', 25 '\\\\031', 120 'x', 52 '4', 252 '\\\\374', 143 '\\\\217', 122 'z', 186 '\\\\272', 246 '\\\\366', 116 't', 31 '\\\\037', 147 '\\\\223', 55 '7', 39 '\\\\'', 212 '\\\\324', 83 'S', 236 '\\\\354', 209 '\\\\321', 198 '\\\\306', 16 '\\\\020', 155 '\\\\233', 130 '\\\\202', 62 '>', 121 'y', 20 '\\\\024', 245 '\\\\365', 168 '\\\\250', 34 '\\\"', 185 '\\\\271', 189 '\\\\275', 40 '(', 68 'D', 183 '\\\\267', 18 '\\\\022', 133 '\\\\205', 216 '\\\\330', 66 'B', 128 '\\\\200', 249 '\\\\371', 249 '\\\\371', 107 'k', 47 '/', 170 '\\\\252', 83 'S', 132 '\\\\204', 91 '[', 181 '\\\\265', 251 '\\\\373', 30 '\\\\036', 21 '\\\\025', 12 '\\\\f', 67 'C', 169 '\\\\251', 89 'Y', 202 '\\\\312', 219 '\\\\333', 149 '\\\\225', 153 '\\\\231', 167 '\\\\247', 36 '$', 20 '\\\\024', 45 '-', 187 '\\\\273', 29 '\\\\035', 75 'K', 115 's', 90 'Z', 86 'V', 143 '\\\\217', 114 'r', 208 '\\\\320', 73 'I', 25 '\\\\031', 103 'g', 98 'b', 231 '\\\\347', 31 '\\\\037', 49 '1', 231 '\\\\347', 252 '\\\\374', 241 '\\\\361', 73 'I', 211 '\\\\323', 132 '\\\\204', 172 '\\\\254', 223 '\\\\337', 65 'A', 185 '\\\\271', 168 '\\\\250', 194 '\\\\302', 79 'O', 185 '\\\\271', 99 'c', 41 ')', 4 '\\\\004', 137 '\\\\211', 13 '\\\\r', 195 '\\\\303', 170 '\\\\252', 187 '\\\\273', 255 '\\\\377', 0 '\\\\0', 171 '\\\\253', 76 'L', 245 '\\\\365', 197 '\\\\305', 75 'K', 102 'f', 52 '4', 219 '\\\\333', 125 '}', 70 'F', 1 '\\\\001', 168 '\\\\250', 151 '\\\\227', 88 'X', 94 '^', 64 '@', 120 'x', 78 'N', 74 'J', 247 '\\\\367', 192 '\\\\300', 239 '\\\\357', 87 'W', 90 'Z', 85 'U', 35 '#', 23 '\\\\027', 202 '\\\\312', 190 '\\\\276', 37 '%', 160 '\\\\240', 158 '\\\\236', 95 '_', 81 'Q', 197 '\\\\305', 74 'J', 221 '\\\\335', 207 '\\\\317', 219 '\\\\333', 191 '\\\\277', 216 '\\\\330', 145 '\\\\221', 188 '\\\\274', 59 ';', 15 '\\\\017', 193 '\\\\301', 223 '\\\\337', 22 '\\\\026', 92 '\\\\\\\\', 248 '\\\\370', 83 'S', 69 'E', 254 '\\\\376', 215 '\\\\327', 191 '\\\\277', 215 '\\\\327', 53 '5', 47 '/', 179 '\\\\263', 177 '\\\\261', 212 '\\\\324', 192 '\\\\300', 138 '\\\\212', 37 '%', 85 'U', 81 'Q', 176 '\\\\260', 243 '\\\\363', 193 '\\\\301'...}}\"";
static const char *gv_var_with_member3 = "value=\"{<Gtk::Window> = {<Gtk::Bin> = {<Gtk::Container> = {<Gtk::Widget> = {<Gtk::Object> = {<Glib::Object> = {<Glib::ObjectBase> = {<sigc::trackable> = {callback_list_ = 0xb73e1ff4}, _vptr.ObjectBase = 0xb73e1ff4, gobject_ = 0x8051643, custom_type_name_ = 0x8050ef0 \\\"U\\\\211\\\\345WVS\\\\350O\\\", cpp_destruction_in_progress_ = 200}, _vptr.Object = 0xb73e1ff4, static object_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}}, static object_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}, referenced_ = 67, gobject_disposed_ = 22}, <Atk::Implementor> = {<Glib::Interface> = {_vptr.Interface = 0x8050ef0}, static implementor_class_ = {<Glib::Interface_Class> = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}, <No data fields>}}, static widget_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}}, static container_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}}, static bin_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}}, static window_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}, accel_group_ = {pCppObject_ = 0xbfd83fc8}}, m_VBox = {<Gtk::Box> = {<Gtk::Container> = {<Gtk::Widget> = {<Gtk::Object> = {<Glib::Object> = {<Glib::ObjectBase> = <invalid address>, _vptr.Object = 0xb7390906, static object_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}}, static object_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}, referenced_ = 67, gobject_disposed_ = 22}, <Atk::Implementor> = {<Glib::Interface> = {_vptr.Interface = 0x805164b}, static implementor_class_ = {<Glib::Interface_Class> = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}, <No data fields>}}, static widget_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}}, static container_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}}, static box_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}, children_proxy_ = {<Glib::HelperList<Gtk::Box_Helpers::Child,const Gtk::Box_Helpers::Element,Glib::List_Iterator<Gtk::Box_Helpers::Child> >> = {_vptr.HelperList = 0xbfd83fff, gparent_ = 0xb73e1f00}, <No data fields>}}, static vbox_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}}, m_MyWidget = {<Gtk::Widget> = {<Gtk::Object> = {<Glib::Object> = {<Glib::ObjectBase> = <invalid address>, _vptr.Object = 0xb71aaf55, static object_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}}, static object_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}, referenced_ = 172, gobject_disposed_ = 54}, <Atk::Implementor> = {<Glib::Interface> = {_vptr.Interface = 0x8056200}, static implementor_class_ = {<Glib::Interface_Class> = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}, <No data fields>}}, static widget_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}}, m_refGdkWindow = {pCppObject_ = 0xb7b3ed64}, m_scale = -1209413632}, m_ButtonBox = {<Gtk::ButtonBox> = {<Gtk::Box> = {<Gtk::Container> = {<Gtk::Widget> = {<Gtk::Object> = {<Glib::Object> = {<Glib::ObjectBase> = <invalid address>, _vptr.Object = 0xb71ab0d0, static object_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}}, static object_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}, referenced_ = 20, gobject_disposed_ = 65}, <Atk::Implementor> = {<Glib::Interface> = {_vptr.Interface = 0xb7fdace0}, static implementor_class_ = {<Glib::Interface_Class> = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}, <No data fields>}}, static widget_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}}, static container_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}}, static box_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}, children_proxy_ = {<Glib::HelperList<Gtk::Box_Helpers::Child,const Gtk::Box_Helpers::Element,Glib::List_Iterator<Gtk::Box_Helpers::Child> >> = {_vptr.HelperList = 0xbfd84028, gparent_ = 0x804da59}, <No data fields>}}, static buttonbox_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}}, static hbuttonbox_class_ = {<Glib::Class> = {gtype_ = 0, class_init_func_ = 0}, <No data fields>}}, m_Button_Quit = {<Gtk::Bin> = {<Gtk::Container> = {<Gtk::Widget> = {<Gtk::Object> = {<Glib::Object> = {<error reading variable>}\"";

static const char *gv_var_with_member4 = "value=\"{ref_count = {ref_count = -1}, status = CAIRO_STATUS_NO_MEMORY, user_data = {size = 0, num_elements = 0, element_size = 0, elements = 0x0, is_snapshot = 0}, gstate = 0x0, gstate_tail = {{op = CAIRO_OPERATOR_CLEAR, tolerance = 0, antialias = CAIRO_ANTIALIAS_DEFAULT, stroke_style = {line_width = 0, line_cap = CAIRO_LINE_CAP_BUTT, line_join = CAIRO_LINE_JOIN_MITER, miter_limit = 0, dash = 0x0, num_dashes = 0, dash_offset = 0}, fill_rule = CAIRO_FILL_RULE_WINDING, font_face = 0x0, scaled_font = 0x0, font_matrix = {xx = 0, yx = 0, xy = 0, yy = 0, x0 = 0, y0 = 0}, font_options = {antialias = CAIRO_ANTIALIAS_DEFAULT, subpixel_order = CAIRO_SUBPIXEL_ORDER_DEFAULT, hint_style = CAIRO_HINT_STYLE_DEFAULT, hint_metrics = CAIRO_HINT_METRICS_DEFAULT}, clip = {mode = CAIRO_CLIP_MODE_PATH, all_clipped = 0, surface = 0x0, surface_rect = {x = 0, y = 0, width = 0, height = 0}, serial = 0, region = {rgn = {extents = {x1 = 0, y1 = 0, x2 = 0, y2 = 0}, data = 0x0}}, has_region = 0, path = 0x0}, target = 0x0, parent_target = 0x0, original_target = 0x0, ctm = {xx = 0, yx = 0, xy = 0, yy = 0, x0 = 0, y0 = 0}, ctm_inverse = {xx = 0, yx = 0, xy = 0, yy = 0, x0 = 0, y0 = 0}, source_ctm_inverse = {xx = 0, yx = 0, xy = 0, yy = 0, x0 = 0, y0 = 0}, source = 0x0, next = 0x0}}, path = {{last_move_point = {x = 0, y = 0}, current_point = {x = 0, y = 0}, has_current_point = 0, has_curve_to = 0, buf_tail = 0x0, buf_head = {base = {next = 0x0, prev = 0x0, buf_size = 0, num_ops = 0, num_points = 0, op = 0x0, points = 0x0}, op = '\\\\0' <repeats 26 times>, points = {{x = 0, y = 0} <repeats 54 times>}}}}}\"";

static const char *gv_var_with_member5 = "value=\"{{bmp = 32, encode = 32 ' '}, {bmp = 33, encode = 33 '!'}, {bmp = 35, encode = 35 '#'}, {bmp = 37, encode = 37 '%'}, {bmp = 38, encode = 38 '&'}, {bmp = 40, encode = 40 '('}, {bmp = 41, encode = 41 ')'}, {bmp = 43, encode = 43 '+'}, {bmp = 44, encode = 44 ','}, {bmp = 46, encode = 46 '.'}, {bmp = 47, encode = 47 '/'}, {bmp = 48, encode = 48 '0'}, {bmp = 49, encode = 49 '1'}, {bmp = 50, encode = 50 '2'}, {bmp = 51, encode = 51 '3'}, {bmp = 52, encode = 52 '4'}, {bmp = 53, encode = 53 '5'}, {bmp = 54, encode = 54 '6'}, {bmp = 55, encode = 55 '7'}, {bmp = 56, encode = 56 '8'}, {bmp = 57, encode = 57 '9'}, {bmp = 58, encode = 58 ':'}, {bmp = 59, encode = 59 ';'}, {bmp = 60, encode = 60 '<'}, {bmp = 61, encode = 61 '='}, {bmp = 62, encode = 62 '>'}, {bmp = 63, encode = 63 '?'}, {bmp = 91, encode = 91 '['}, {bmp = 93, encode = 93 ']'}, {bmp = 95, encode = 95 '_'}, {bmp = 123, encode = 123 '{'}, {bmp = 124, encode = 124 '|'}, {bmp = 125, encode = 125 '}'}, {bmp = 160, encode = 32 ' '}, {bmp = 172, encode = 216 '\\\\330'}, {bmp = 176, encode = 176 '\\\\260'}, {bmp = 177, encode = 177 '\\\\261'}, {bmp = 181, encode = 109 'm'}, {bmp = 215, encode = 180 '\\\\264'}, {bmp = 247, encode = 184 '\\\\270'}, {bmp = 402, encode = 166 '\\\\246'}, {bmp = 913, encode = 65 'A'}, {bmp = 914, encode = 66 'B'}, {bmp = 915, encode = 71 'G'}, {bmp = 916, encode = 68 'D'}, {bmp = 917, encode = 69 'E'}, {bmp = 918, encode = 90 'Z'}, {bmp = 919, encode = 72 'H'}, {bmp = 920, encode = 81 'Q'}, {bmp = 921, encode = 73 'I'}, {bmp = 922, encode = 75 'K'}, {bmp = 923, encode = 76 'L'}, {bmp = 924, encode = 77 'M'}, {bmp = 925, encode = 78 'N'}, {bmp = 926, encode = 88 'X'}, {bmp = 927, encode = 79 'O'}, {bmp = 928, encode = 80 'P'}, {bmp = 929, encode = 82 'R'}, {bmp = 931, encode = 83 'S'}, {bmp = 932, encode = 84 'T'}, {bmp = 933, encode = 85 'U'}, {bmp = 934, encode = 70 'F'}, {bmp = 935, encode = 67 'C'}, {bmp = 936, encode = 89 'Y'}, {bmp = 937, encode = 87 'W'}, {bmp = 945, encode = 97 'a'}, {bmp = 946, encode = 98 'b'}, {bmp = 947, encode = 103 'g'}, {bmp = 948, encode = 100 'd'}, {bmp = 949, encode = 101 'e'}, {bmp = 950, encode = 122 'z'}, {bmp = 951, encode = 104 'h'}, {bmp = 952, encode = 113 'q'}, {bmp = 953, encode = 105 'i'}, {bmp = 954, encode = 107 'k'}, {bmp = 955, encode = 108 'l'}, {bmp = 956, encode = 109 'm'}, {bmp = 957, encode = 110 'n'}, {bmp = 958, encode = 120 'x'}, {bmp = 959, encode = 111 'o'}, {bmp = 960, encode = 112 'p'}, {bmp = 961, encode = 114 'r'}, {bmp = 962, encode = 86 'V'}, {bmp = 963, encode = 115 's'}, {bmp = 964, encode = 116 't'}, {bmp = 965, encode = 117 'u'}, {bmp = 966, encode = 102 'f'}, {bmp = 967, encode = 99 'c'}, {bmp = 968, encode = 121 'y'}, {bmp = 969, encode = 119 'w'}, {bmp = 977, encode = 74 'J'}, {bmp = 978, encode = 161 '\\\\241'}, {bmp = 981, encode = 106 'j'}, {bmp = 982, encode = 118 'v'}, {bmp = 8226, encode = 183 '\\\\267'}, {bmp = 8230, encode = 188 '\\\\274'}, {bmp = 8242, encode = 162 '\\\\242'}, {bmp = 8243, encode = 178 '\\\\262'}, {bmp = 8260, encode = 164 '\\\\244'}, {bmp = 8364, encode = 160 '\\\\240'}, {bmp = 8465, encode = 193 '\\\\301'}, {bmp = 8472, encode = 195 '\\\\303'}, {bmp = 8476, encode = 194 '\\\\302'}, {bmp = 8486, encode = 87 'W'}, {bmp = 8501, encode = 192 '\\\\300'}, {bmp = 8592, encode = 172 '\\\\254'}, {bmp = 8593, encode = 173 '\\\\255'}, {bmp = 8594, encode = 174 '\\\\256'}, {bmp = 8595, encode = 175 '\\\\257'}, {bmp = 8596, encode = 171 '\\\\253'}, {bmp = 8629, encode = 191 '\\\\277'}, {bmp = 8656, encode = 220 '\\\\334'}, {bmp = 8657, encode = 221 '\\\\335'}, {bmp = 8658, encode = 222 '\\\\336'}, {bmp = 8659, encode = 223 '\\\\337'}, {bmp = 8660, encode = 219 '\\\\333'}, {bmp = 8704, encode = 34 '\\\"'}, {bmp = 8706, encode = 182 '\\\\266'}, {bmp = 8707, encode = 36 '$'}, {bmp = 8709, encode = 198 '\\\\306'}, {bmp = 8710, encode = 68 'D'}, {bmp = 8711, encode = 209 '\\\\321'}, {bmp = 8712, encode = 206 '\\\\316'}, {bmp = 8713, encode = 207 '\\\\317'}, {bmp = 8715, encode = 39 '\\\\''}, {bmp = 8719, encode = 213 '\\\\325'}, {bmp = 8721, encode = 229 '\\\\345'}, {bmp = 8722, encode = 45 '-'}, {bmp = 8725, encode = 164 '\\\\244'}, {bmp = 8727, encode = 42 '*'}, {bmp = 8730, encode = 214 '\\\\326'}, {bmp = 8733, encode = 181 '\\\\265'}, {bmp = 8734, encode = 165 '\\\\245'}, {bmp = 8736, encode = 208 '\\\\320'}, {bmp = 8743, encode = 217 '\\\\331'}, {bmp = 8744, encode = 218 '\\\\332'}, {bmp = 8745, encode = 199 '\\\\307'}, {bmp = 8746, encode = 200 '\\\\310'}, {bmp = 8747, encode = 242 '\\\\362'}, {bmp = 8756, encode = 92 '\\\\\\\\'}, {bmp = 8764, encode = 126 '~'}, {bmp = 8773, encode = 64 '@'}, {bmp = 8776, encode = 187 '\\\\273'}, {bmp = 8800, encode = 185 '\\\\271'}, {bmp = 8801, encode = 186 '\\\\272'}, {bmp = 8804, encode = 163 '\\\\243'}, {bmp = 8805, encode = 179 '\\\\263'}, {bmp = 8834, encode = 204 '\\\\314'}, {bmp = 8835, encode = 201 '\\\\311'}, {bmp = 8836, encode = 203 '\\\\313'}, {bmp = 8838, encode = 205 '\\\\315'}, {bmp = 8839, encode = 202 '\\\\312'}, {bmp = 8853, encode = 197 '\\\\305'}, {bmp = 8855, encode = 196 '\\\\304'}, {bmp = 8869, encode = 94 '^'}, {bmp = 8901, encode = 215 '\\\\327'}, {bmp = 8992, encode = 243 '\\\\363'}, {bmp = 8993, encode = 245 '\\\\365'}, {bmp = 9001, encode = 225 '\\\\341'}, {bmp = 9002, encode = 241 '\\\\361'}, {bmp = 9674, encode = 224 '\\\\340'}, {bmp = 9824, encode = 170 '\\\\252'}, {bmp = 9827, encode = 167 '\\\\247'}, {bmp = 9829, encode = 169 '\\\\251'}, {bmp = 9830, encode = 168 '\\\\250'}, {bmp = 63193, encode = 211 '\\\\323'}, {bmp = 63194, encode = 210 '\\\\322'}, {bmp = 63195, encode = 212 '\\\\324'}, {bmp = 63717, encode = 96 '`'}, {bmp = 63718, encode = 189 '\\\\275'}, {bmp = 63719, encode = 190 '\\\\276'}, {bmp = 63720, encode = 226 '\\\\342'}, {bmp = 63721, encode = 227 '\\\\343'}, {bmp = 63722, encode = 228 '\\\\344'}, {bmp = 63723, encode = 230 '\\\\346'}, {bmp = 63724, encode = 231 '\\\\347'}, {bmp = 63725, encode = 232 '\\\\350'}, {bmp = 63726, encode = 233 '\\\\351'}, {bmp = 63727, encode = 234 '\\\\352'}, {bmp = 63728, encode = 235 '\\\\353'}, {bmp = 63729, encode = 236 '\\\\354'}, {bmp = 63730, encode = 237 '\\\\355'}, {bmp = 63731, encode = 238 '\\\\356'}, {bmp = 63732, encode = 239 '\\\\357'}, {bmp = 63733, encode = 244 '\\\\364'}, {bmp = 63734, encode = 246 '\\\\366'}, {bmp = 63735, encode = 247 '\\\\367'}, {bmp = 63736, encode = 248 '\\\\370'}, {bmp = 63737, encode = 249 '\\\\371'}, {bmp = 63738, encode = 250 '\\\\372'}, {bmp = 63739, encode = 251 '\\\\373'}, {bmp = 63740, encode = 252 '\\\\374'}, {bmp = 63741, encode = 253 '\\\\375'}, {bmp = 63742, encode = 254 '\\\\376'}}\"";

static const char *gv_var_with_member6 = "value=\"{{notice_offset = 0 '\\\\0', foundry_offset = 8 '\\\\b'}, {notice_offset = 12 '\\\\f', foundry_offset = 18 '\\\\022'}, {notice_offset = 24 '\\\\030', foundry_offset = 34 '\\\"'}, {notice_offset = 44 ',', foundry_offset = 53 '5'}, {notice_offset = 62 '>', foundry_offset = 71 'G'}, {notice_offset = 80 'P', foundry_offset = 94 '^'}, {notice_offset = 103 'g', foundry_offset = 107 'k'}, {notice_offset = 111 'o', foundry_offset = 115 's'}, {notice_offset = 119 'w', foundry_offset = 154 '\\\\232'}, {notice_offset = 158 '\\\\236', foundry_offset = 173 '\\\\255'}, {notice_offset = 178 '\\\\262', foundry_offset = 186 '\\\\272'}, {notice_offset = 194 '\\\\302', foundry_offset = 204 '\\\\314'}, {notice_offset = 214 '\\\\326', foundry_offset = 220 '\\\\334'}, {notice_offset = 226 '\\\\342', foundry_offset = 233 '\\\\351'}, {notice_offset = 238 '\\\\356', foundry_offset = 253 '\\\\375'}}\"";

static const char *gv_var_with_member7 = "value=\"{{upper = 65, method = 0, count = 26, offset = 32}, {upper = 181, method = 0, count = 1, offset = 775}, {upper = 192, method = 0, count = 23, offset = 32}, {upper = 216, method = 0, count = 7, offset = 32}, {upper = 223, method = 2, count = 2, offset = 0}, {upper = 256, method = 1, count = 47, offset = 1}, {upper = 304, method = 2, count = 3, offset = 2}, {upper = 306, method = 1, count = 5, offset = 1}, {upper = 313, method = 1, count = 15, offset = 1}, {upper = 329, method = 2, count = 3, offset = 5}, {upper = 330, method = 1, count = 45, offset = 1}, {upper = 376, method = 0, count = 1, offset = -121}, {upper = 377, method = 1, count = 5, offset = 1}, {upper = 383, method = 0, count = 1, offset = -268}, {upper = 385, method = 0, count = 1, offset = 210}, {upper = 386, method = 1, count = 3, offset = 1}, {upper = 390, method = 0, count = 1, offset = 206}, {upper = 391, method = 1, count = 1, offset = 1}, {upper = 393, method = 0, count = 2, offset = 205}, {upper = 395, method = 1, count = 1, offset = 1}, {upper = 398, method = 0, count = 1, offset = 79}, {upper = 399, method = 0, count = 1, offset = 202}, {upper = 400, method = 0, count = 1, offset = 203}, {upper = 401, method = 1, count = 1, offset = 1}, {upper = 403, method = 0, count = 1, offset = 205}, {upper = 404, method = 0, count = 1, offset = 207}, {upper = 406, method = 0, count = 1, offset = 211}, {upper = 407, method = 0, count = 1, offset = 209}, {upper = 408, method = 1, count = 1, offset = 1}, {upper = 412, method = 0, count = 1, offset = 211}, {upper = 413, method = 0, count = 1, offset = 213}, {upper = 415, method = 0, count = 1, offset = 214}, {upper = 416, method = 1, count = 5, offset = 1}, {upper = 422, method = 0, count = 1, offset = 218}, {upper = 423, method = 1, count = 1, offset = 1}, {upper = 425, method = 0, count = 1, offset = 218}, {upper = 428, method = 1, count = 1, offset = 1}, {upper = 430, method = 0, count = 1, offset = 218}, {upper = 431, method = 1, count = 1, offset = 1}, {upper = 433, method = 0, count = 2, offset = 217}, {upper = 435, method = 1, count = 3, offset = 1}, {upper = 439, method = 0, count = 1, offset = 219}, {upper = 440, method = 1, count = 1, offset = 1}, {upper = 444, method = 1, count = 1, offset = 1}, {upper = 452, method = 0, count = 1, offset = 2}, {upper = 453, method = 1, count = 1, offset = 1}, {upper = 455, method = 0, count = 1, offset = 2}, {upper = 456, method = 1, count = 1, offset = 1}, {upper = 458, method = 0, count = 1, offset = 2}, {upper = 459, method = 1, count = 17, offset = 1}, {upper = 478, method = 1, count = 17, offset = 1}, {upper = 496, method = 2, count = 3, offset = 8}, {upper = 497, method = 0, count = 1, offset = 2}, {upper = 498, method = 1, count = 3, offset = 1}, {upper = 502, method = 0, count = 1, offset = -97}, {upper = 503, method = 0, count = 1, offset = -56}, {upper = 504, method = 1, count = 39, offset = 1}, {upper = 544, method = 0, count = 1, offset = -130}, {upper = 546, method = 1, count = 17, offset = 1}, {upper = 570, method = 0, count = 1, offset = 10795}, {upper = 571, method = 1, count = 1, offset = 1}, {upper = 573, method = 0, count = 1, offset = -163}, {upper = 574, method = 0, count = 1, offset = 10792}, {upper = 577, method = 1, count = 1, offset = 1}, {upper = 579, method = 0, count = 1, offset = -195}, {upper = 580, method = 0, count = 1, offset = 69}, {upper = 581, method = 0, count = 1, offset = 71}, {upper = 582, method = 1, count = 9, offset = 1}, {upper = 837, method = 0, count = 1, offset = 116}, {upper = 902, method = 0, count = 1, offset = 38}, {upper = 904, method = 0, count = 3, offset = 37}, {upper = 908, method = 0, count = 1, offset = 64}, {upper = 910, method = 0, count = 2, offset = 63}, {upper = 912, method = 2, count = 6, offset = 11}, {upper = 913, method = 0, count = 17, offset = 32}, {upper = 931, method = 0, count = 9, offset = 32}, {upper = 944, method = 2, count = 6, offset = 17}, {upper = 962, method = 1, count = 1, offset = 1}, {upper = 976, method = 0, count = 1, offset = -30}, {upper = 977, method = 0, count = 1, offset = -25}, {upper = 981, method = 0, count = 1, offset = -15}, {upper = 982, method = 0, count = 1, offset = -22}, {upper = 984, method = 1, count = 23, offset = 1}, {upper = 1008, method = 0, count = 1, offset = -54}, {upper = 1009, method = 0, count = 1, offset = -48}, {upper = 1012, method = 0, count = 1, offset = -60}, {upper = 1013, method = 0, count = 1, offset = -64}, {upper = 1015, method = 1, count = 1, offset = 1}, {upper = 1017, method = 0, count = 1, offset = -7}, {upper = 1018, method = 1, count = 1, offset = 1}, {upper = 1021, method = 0, count = 3, offset = -130}, {upper = 1024, method = 0, count = 16, offset = 80}, {upper = 1040, method = 0, count = 32, offset = 32}, {upper = 1120, method = 1, count = 33, offset = 1}, {upper = 1162, method = 1, count = 53, offset = 1}, {upper = 1216, method = 0, count = 1, offset = 15}, {upper = 1217, method = 1, count = 13, offset = 1}, {upper = 1232, method = 1, count = 67, offset = 1}, {upper = 1329, method = 0, count = 38, offset = 48}, {upper = 1415, method = 2, count = 4, offset = 23}, {upper = 4256, method = 0, count = 38, offset = 7264}, {upper = 7680, method = 1, count = 149, offset = 1}, {upper = 7830, method = 2, count = 3, offset = 27}, {upper = 7831, method = 2, count = 3, offset = 30}, {upper = 7832, method = 2, count = 3, offset = 33}, {upper = 7833, method = 2, count = 3, offset = 36}, {upper = 7834, method = 2, count = 3, offset = 39}, {upper = 7835, method = 0, count = 1, offset = -58}, {upper = 7840, method = 1, count = 89, offset = 1}, {upper = 7944, method = 0, count = 8, offset = -8}, {upper = 7960, method = 0, count = 6, offset = -8}, {upper = 7976, method = 0, count = 8, offset = -8}, {upper = 7992, method = 0, count = 8, offset = -8}, {upper = 8008, method = 0, count = 6, offset = -8}, {upper = 8016, method = 2, count = 4, offset = 42}, {upper = 8018, method = 2, count = 6, offset = 46}, {upper = 8020, method = 2, count = 6, offset = 52}, {upper = 8022, method = 2, count = 6, offset = 58}, {upper = 8025, method = 0, count = 1, offset = -8}, {upper = 8027, method = 0, count = 1, offset = -8}, {upper = 8029, method = 0, count = 1, offset = -8}, {upper = 8031, method = 0, count = 1, offset = -8}, {upper = 8040, method = 0, count = 8, offset = -8}, {upper = 8064, method = 2, count = 5, offset = 64}, {upper = 8065, method = 2, count = 5, offset = 69}, {upper = 8066, method = 2, count = 5, offset = 74}, {upper = 8067, method = 2, count = 5, offset = 79}, {upper = 8068, method = 2, count = 5, offset = 84}, {upper = 8069, method = 2, count = 5, offset = 89}, {upper = 8070, method = 2, count = 5, offset = 94}, {upper = 8071, method = 2, count = 5, offset = 99}, {upper = 8072, method = 2, count = 5, offset = 104}, {upper = 8073, method = 2, count = 5, offset = 109}, {upper = 8074, method = 2, count = 5, offset = 114}, {upper = 8075, method = 2, count = 5, offset = 119}, {upper = 8076, method = 2, count = 5, offset = 124}, {upper = 8077, method = 2, count = 5, offset = 129}, {upper = 8078, method = 2, count = 5, offset = 134}, {upper = 8079, method = 2, count = 5, offset = 139}, {upper = 8080, method = 2, count = 5, offset = 144}, {upper = 8081, method = 2, count = 5, offset = 149}, {upper = 8082, method = 2, count = 5, offset = 154}, {upper = 8083, method = 2, count = 5, offset = 159}, {upper = 8084, method = 2, count = 5, offset = 164}, {upper = 8085, method = 2, count = 5, offset = 169}, {upper = 8086, method = 2, count = 5, offset = 174}, {upper = 8087, method = 2, count = 5, offset = 179}, {upper = 8088, method = 2, count = 5, offset = 184}, {upper = 8089, method = 2, count = 5, offset = 189}, {upper = 8090, method = 2, count = 5, offset = 194}, {upper = 8091, method = 2, count = 5, offset = 199}, {upper = 8092, method = 2, count = 5, offset = 204}, {upper = 8093, method = 2, count = 5, offset = 209}, {upper = 8094, method = 2, count = 5, offset = 214}, {upper = 8095, method = 2, count = 5, offset = 219}, {upper = 8096, method = 2, count = 5, offset = 224}, {upper = 8097, method = 2, count = 5, offset = 229}, {upper = 8098, method = 2, count = 5, offset = 234}, {upper = 8099, method = 2, count = 5, offset = 239}, {upper = 8100, method = 2, count = 5, offset = 244}, {upper = 8101, method = 2, count = 5, offset = 249}, {upper = 8102, method = 2, count = 5, offset = 254}, {upper = 8103, method = 2, count = 5, offset = 259}, {upper = 8104, method = 2, count = 5, offset = 264}, {upper = 8105, method = 2, count = 5, offset = 269}, {upper = 8106, method = 2, count = 5, offset = 274}, {upper = 8107, method = 2, count = 5, offset = 279}, {upper = 8108, method = 2, count = 5, offset = 284}, {upper = 8109, method = 2, count = 5, offset = 289}, {upper = 8110, method = 2, count = 5, offset = 294}, {upper = 8111, method = 2, count = 5, offset = 299}, {upper = 8114, method = 2, count = 5, offset = 304}, {upper = 8115, method = 2, count = 4, offset = 309}, {upper = 8116, method = 2, count = 4, offset = 313}, {upper = 8118, method = 2, count = 4, offset = 317}, {upper = 8119, method = 2, count = 6, offset = 321}, {upper = 8120, method = 0, count = 2, offset = -8}, {upper = 8122, method = 0, count = 2, offset = -74}, {upper = 8124, method = 2, count = 4, offset = 327}, {upper = 8126, method = 0, count = 1, offset = -7173}, {upper = 8130, method = 2, count = 5, offset = 331}, {upper = 8131, method = 2, count = 4, offset = 336}, {upper = 8132, method = 2, count = 4, offset = 340}, {upper = 8134, method = 2, count = 4, offset = 344}, {upper = 8135, method = 2, count = 6, offset = 348}, {upper = 8136, method = 0, count = 4, offset = -86}, {upper = 8140, method = 2, count = 4, offset = 354}, {upper = 8146, method = 2, count = 6, offset = 358}, {upper = 8147, method = 2, count = 6, offset = 364}, {upper = 8150, method = 2, count = 4, offset = 370}, {upper = 8151, method = 2, count = 6, offset = 374}, {upper = 8152, method = 0, count = 2, offset = -8}, {upper = 8154, method = 0, count = 2, offset = -100}, {upper = 8162, method = 2, count = 6, offset = 380}, {upper = 8163, method = 2, count = 6, offset = 386}, {upper = 8164, method = 2, count = 4, offset = 392}, {upper = 8166, method = 2, count = 4, offset = 396}, {upper = 8167, method = 2, count = 6, offset = 400}, {upper = 8168, method = 0, count = 2, offset = -8}, {upper = 8170, method = 0, count = 2, offset = -112}...}\"";

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

static const char* gv_register_names=
"register-names=[\"eax\",\"ecx\",\"edx\",\"ebx\",\"esp\",\"ebp\",\"esi\",\"edi\",\"eip\",\"eflags\",\"cs\",\"ss\",\"ds\",\"es\",\"fs\",\"gs\",\"st0\",\"st1\",\"st2\",\"st3\",\"st4\",\"st5\",\"st6\",\"st7\",\"fctrl\",\"fstat\",\"ftag\",\"fiseg\",\"fioff\",\"foseg\",\"fooff\",\"fop\",\"xmm0\",\"xmm1\",\"xmm2\",\"xmm3\",\"xmm4\",\"xmm5\",\"xmm6\",\"xmm7\",\"mxcsr\",\"orig_eax\",\"mm0\",\"mm1\",\"mm2\",\"mm3\",\"mm4\",\"mm5\",\"mm6\",\"mm7\"]";

static const char* gv_changed_registers=
"changed-registers=[\"0\",\"1\",\"2\",\"3\",\"4\",\"5\",\"6\",\"8\",\"9\",\"10\",\"11\",\"12\",\"13\",\"15\",\"24\",\"26\",\"40\",\"41\"]";

static const char* gv_register_values =
"register-values=[{number=\"1\",value=\"0xbfd10a60\"},{number=\"2\",value=\"0x1\"},{number=\"3\",value=\"0xb6f03ff4\"},{number=\"4\",value=\"0xbfd10960\"},{number=\"5\",value=\"0xbfd10a48\"},{number=\"6\",value=\"0xb7ff6ce0\"},{number=\"7\",value=\"0x0\"},{number=\"8\",value=\"0x80bb710\"},{number=\"9\",value=\"0x200286\"},{number=\"10\",value=\"0x73\"},{number=\"36\",value=\"{v4_float = {0x0, 0x0, 0x0, 0x0}, v2_double = {0x0, 0x0}, v16_int8 = {0x0 <repeats 16 times>}, v8_int16 = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, v4_int32 = {0x0, 0x0, 0x0, 0x0}, v2_int64 = {0x0, 0x0}, uint128 = 0x00000000000000000000000000000000}\"}]";

static const char* gv_memory_values =
"addr=\"0x000013a0\",nr-bytes=\"32\",total-bytes=\"32\",next-row=\"0x000013c0\",prev-row=\"0x0000139c\",next-page=\"0x000013c0\",prev-page=\"0x00001380\",memory=[{addr=\"0x000013a0\",data=[\"0x10\",\"0x11\",\"0x12\",\"0x13\"],ascii=\"xxxx\"}]";

static const char* gv_breakpoint_table0 =
"BreakpointTable={nr_rows=\"1\",nr_cols=\"6\",hdr=[{width=\"3\",alignment=\"-1\",col_name=\"number\",colhdr=\"Num\"},{width=\"14\",alignment=\"-1\",col_name=\"type\",colhdr=\"Type\"},{width=\"4\",alignment=\"-1\",col_name=\"disp\",colhdr=\"Disp\"},{width=\"3\",alignment=\"-1\",col_name=\"enabled\",colhdr=\"Enb\"},{width=\"10\",alignment=\"-1\",col_name=\"addr\",colhdr=\"Address\"},{width=\"40\",alignment=\"2\",col_name=\"what\",colhdr=\"What\"}],body=[bkpt={number=\"1\",type=\"breakpoint\",disp=\"keep\",enabled=\"y\",addr=\"0x08081566\",func=\"main\",file=\"main.cc\",fullname=\"/home/jonathon/Projects/agave.git/src/main.cc\",line=\"70\",times=\"0\"}]}";


static const char* gv_breakpoint_table1 =
"BreakpointTable={nr_rows=\"2\",nr_cols=\"6\",hdr=[{width=\"7\",alignment=\"-1\",col_name=\"number\",colhdr=\"Num\"},{width=\"14\",alignment=\"-1\",col_name=\"type\",colhdr=\"Type\"},{width=\"4\",alignment=\"-1\",col_name=\"disp\",colhdr=\"Disp\"},{width=\"3\",alignment=\"-1\",col_name=\"enabled\",colhdr=\"Enb\"},{width=\"10\",alignment=\"-1\",col_name=\"addr\",colhdr=\"Address\"},{width=\"40\",alignment=\"2\",col_name=\"what\",colhdr=\"What\"}],body=[bkpt={number=\"1\",type=\"breakpoint\",disp=\"keep\",enabled=\"y\",addr=\"0x0805e404\",func=\"main\",file=\"/usr/include/boost/test/minimal.hpp\",fullname=\"/usr/include/boost/test/minimal.hpp\",line=\"113\",times=\"1\"},bkpt={number=\"2\",type=\"breakpoint\",disp=\"keep\",enabled=\"y\",addr=\"<PENDING>\",pending=\"nemiver_common_create_dynamic_module_instance\",times=\"0\"}]}";

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
    bool is_ok=false, got_frame=false ;
    UString::size_type to=0 ;
    IDebugger::Frame frame ;
    map<UString, UString> attrs ;

    is_ok = parse_stopped_async_output (gv_stopped_async_output0, 0, to,
                                        got_frame, frame, attrs) ;
    BOOST_REQUIRE (is_ok) ;
    BOOST_REQUIRE (got_frame) ;
    BOOST_REQUIRE (attrs.size ()) ;

    to=0;
    is_ok = parse_stopped_async_output (gv_stopped_async_output1, 0, to,
                                        got_frame, frame, attrs) ;
    BOOST_REQUIRE (is_ok) ;
    BOOST_REQUIRE (got_frame) ;
    BOOST_REQUIRE (attrs.size ()) ;
}

void
test_output_record ()
{
    bool is_ok=false;
    UString::size_type to=0;
    Output output;

    is_ok = parse_output_record (gv_output_record0, 0, to, output);
    BOOST_REQUIRE (is_ok) ;
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

    to = 0;
    var.reset (new IDebugger::Variable);
    BOOST_REQUIRE (parse_variable_value (gv_var_with_member2, 0, to, var)) ;
    BOOST_REQUIRE (var) ;
    BOOST_REQUIRE (!var->members ().empty ()) ;

    to = 0;
    var.reset (new IDebugger::Variable);
    BOOST_REQUIRE (parse_variable_value (gv_var_with_member3, 0, to, var)) ;
    BOOST_REQUIRE (var) ;
    BOOST_REQUIRE (!var->members ().empty ()) ;

    to = 0;
    var.reset (new IDebugger::Variable);
    BOOST_REQUIRE (parse_variable_value (gv_var_with_member4, 0, to, var)) ;
    BOOST_REQUIRE (var) ;
    BOOST_REQUIRE (!var->members ().empty ()) ;

    to = 0;
    var.reset (new IDebugger::Variable);
    BOOST_REQUIRE (parse_variable_value (gv_var_with_member5, 0, to, var)) ;
    BOOST_REQUIRE (var) ;
    BOOST_REQUIRE (!var->members ().empty ()) ;

    to = 0;
    var.reset (new IDebugger::Variable);
    BOOST_REQUIRE (parse_variable_value (gv_var_with_member6, 0, to, var)) ;
    BOOST_REQUIRE (var) ;
    BOOST_REQUIRE (!var->members ().empty ()) ;

    to = 0;
    var.reset (new IDebugger::Variable);
    BOOST_REQUIRE (parse_variable_value (gv_var_with_member7, 0, to, var)) ;
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

void
test_register_names ()
{
    std::map<IDebugger::register_id_t, UString> regs;
    UString::size_type cur = 0;

    BOOST_REQUIRE (parse_register_names (gv_register_names,
                cur, cur, regs)) ;
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

    BOOST_REQUIRE (parse_changed_registers (gv_changed_registers,
                cur, cur, regs)) ;
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

    BOOST_REQUIRE (parse_register_values (gv_register_values,
                cur, cur, reg_values)) ;
    BOOST_REQUIRE_EQUAL (reg_values.size (), 11u);
    std::map<IDebugger::register_id_t, UString>::const_iterator reg_iter = reg_values.begin ();
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

    BOOST_REQUIRE (parse_memory_values (gv_memory_values,
                cur, cur, start_addr, mem_values)) ;
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
test_breakpoint_table ()
{
    std::map<int, IDebugger::BreakPoint> breakpoints;
    UString::size_type cur = 0;

    BOOST_REQUIRE (parse_breakpoint_table (gv_breakpoint_table0,
                                           cur, cur, breakpoints)) ;
    BOOST_REQUIRE_EQUAL (breakpoints.size (), 1u);
    std::map<int, IDebugger::BreakPoint>::const_iterator iter;
    iter = breakpoints.find (1);
    BOOST_REQUIRE (iter != breakpoints.end ());
    BOOST_REQUIRE_EQUAL (iter->second.number (), 1);
    BOOST_REQUIRE (iter->second.enabled ());
    BOOST_REQUIRE_EQUAL (iter->second.address (), "0x08081566");
    BOOST_REQUIRE_EQUAL (iter->second.function (), "main");
    BOOST_REQUIRE_EQUAL (iter->second.file_name (), "main.cc");
    BOOST_REQUIRE_EQUAL (iter->second.file_full_name (),
                         "/home/jonathon/Projects/agave.git/src/main.cc");
    BOOST_REQUIRE_EQUAL (iter->second.line (), 70);

    cur = 0, cur = 0, breakpoints.clear();
    BOOST_REQUIRE (parse_breakpoint_table (gv_breakpoint_table1,
                                           cur, cur, breakpoints)) ;
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
    suite->add (BOOST_TEST_CASE (&test_output_record)) ;
    suite->add (BOOST_TEST_CASE (&test_stack_arguments0)) ;
    suite->add (BOOST_TEST_CASE (&test_stack_arguments1)) ;
    suite->add (BOOST_TEST_CASE (&test_local_vars)) ;
    suite->add (BOOST_TEST_CASE (&test_member_variable)) ;
    suite->add (BOOST_TEST_CASE (&test_var_with_member_variable)) ;
    suite->add (BOOST_TEST_CASE (&test_embedded_string)) ;
    suite->add (BOOST_TEST_CASE (&test_overloads_prompt)) ;
    suite->add (BOOST_TEST_CASE (&test_register_names));
    suite->add (BOOST_TEST_CASE (&test_changed_registers));
    suite->add (BOOST_TEST_CASE (&test_register_values));
    suite->add (BOOST_TEST_CASE (&test_memory_values));
    suite->add (BOOST_TEST_CASE (&test_breakpoint_table));
    return suite ;

    NEMIVER_CATCH_NOX

    return 0 ;
}

