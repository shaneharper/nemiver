/* -*- Mode: C++; indent-tabs-mode:nil; c-basic-offset:4; -*- */

/*Copyright (c) 2005-2006 Dodji Seketeli
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies
 * or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS",
 * WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <vector>
#include <glibmm.h>
#include "nmv-env.h"
#include "nmv-ustring.h"
#include "nmv-dynamic-module.h"

using namespace std ;

NEMIVER_BEGIN_NAMESPACE (nemiver)
NEMIVER_BEGIN_NAMESPACE (common)
NEMIVER_BEGIN_NAMESPACE (env)

class Initializer
{

public:
    Initializer ()
    {
        Glib::thread_init () ;
    }

    ~Initializer ()
    {
    }
};

void
do_init ()
{
    static Initializer init ;
}

const UString&
get_system_config_file ()
{
    static UString path ;
    if (path.size () == 0) {
        vector<string> path_elements ;
        path_elements.push_back (get_system_config_dir ()) ;
        path_elements.push_back ("nemiver.conf") ;
        path = Glib::build_filename (path_elements).c_str () ;
    }
    return path ;
}

const UString&
get_data_dir ()
{
#ifndef DATADIR
#error The macro DATADIR must be set at compile time !
#endif
    static UString s_data_dir (DATADIR);
    return s_data_dir;
}
const UString&
get_system_lib_dir ()
{
#ifndef SYSTEM_LIBDIR
#error The macro SYSTEM_LIBDIR must be set at compile time !
#endif
    static UString s_system_lib_dir (SYSTEM_LIBDIR) ;
    return s_system_lib_dir ;
}

const UString&
get_glade_files_dir ()
{
    static UString s_glade_files_dir ;
    if (s_glade_files_dir == "") {
        vector<string> path_elems ;
        path_elems.push_back (get_data_dir ());
        path_elems.push_back ("nemiver") ;
        path_elems.push_back ("glade") ;
        s_glade_files_dir = Glib::build_filename (path_elems).c_str ();
    }
    return s_glade_files_dir;
}

const UString&
get_menu_files_dir ()
{
    static UString s_menu_files_dir ;
    if (s_menu_files_dir == "") {
        vector<string> path_elems ;
        path_elems.push_back (get_data_dir ());
        path_elems.push_back ("nemiver") ;
        path_elems.push_back ("menus") ;
        s_menu_files_dir = Glib::build_filename (path_elems).c_str ();
    }
    return s_menu_files_dir;
}

const UString&
get_image_files_dir ()
{
    static UString s_menu_files_dir ;
    if (s_menu_files_dir == "") {
        vector<string> path_elems ;
        path_elems.push_back (get_data_dir ());
        path_elems.push_back ("nemiver") ;
        path_elems.push_back ("images") ;
        s_menu_files_dir = Glib::build_filename (path_elems).c_str ();
    }
    return s_menu_files_dir;
}

const UString&
get_install_prefix ()
{
#ifndef NEMIVER_INSTALL_PREFIX
#error The macro NEMIVER_INSTALL_PREFIX must be set at compile time !
#endif
    //this macro must be computed at compile time.
    //maybe we'll need to change the way we compute
    //this to allow the choice of install prefix
    //at install time.
    static UString s_path (NEMIVER_INSTALL_PREFIX) ;
    return s_path ;
}

const UString&
get_system_config_dir ()
{
#ifndef NEMIVER_SYSTEM_CONFIG_DIR
#error The macro NEMIVER_SYSTEM_CONFIG_DIR must be set at compile time !
#endif
    //this macro must be computed at compile time.
    static UString s_path (NEMIVER_SYSTEM_CONFIG_DIR) ;
    return s_path ;
}

const UString&
get_system_modules_dir ()
{
#ifndef NEMIVER_MODULES_DIR
#error The macro NEMIVER_MODULES_DIR must be set at compile time !
#endif
    //this macro must be set at compile time.
    static UString s_path (NEMIVER_MODULES_DIR) ;
    return s_path ;
}

const UString&
get_system_modules_config_file ()
{
    static UString path ;
    if (path.size () == 0) {
        vector<string> path_elements ;
        path_elements.push_back (get_system_config_dir ()) ;
        path_elements.push_back ("nemivermodules.conf") ;
        path = Glib::build_filename (path_elements).c_str () ;
    }
    return path ;
}

const UString&
get_system_plugins_dir ()
{

#ifndef NEMIVER_PLUGINS_DIR
#error The macro NEMIVER_PLUGINS_DIR must be set at compile time !
#endif

    static UString s_path (NEMIVER_PLUGINS_DIR);
    return s_path ;
}

const UString&
get_user_db_dir ()
{
    static UString s_path ;
    if (s_path.size () == 0) {
        vector<string> path_elements ;
        string home = Glib::get_home_dir () ;
        path_elements.push_back (home) ;
        path_elements.push_back (".nemiver") ;
        s_path = Glib::build_filename (path_elements).c_str ();
    }
    return s_path ;
}

const UString&
get_gdb_program ()
{

#ifndef GDB_PROG
#error the macro GDB_PROG must be set at compile time !
#endif

    static const UString s_gdb_prog (GDB_PROG);
    return s_gdb_prog ;
}

bool
create_user_db_dir ()
{
    gint error =
        g_mkdir_with_parents (get_user_db_dir ().c_str (), S_IRWXU) ;
    if (!error)
        return true ;
    return false ;
}

UString
build_path_to_glade_file (const UString &a_glade_file_name)
{
    UString dir (get_glade_files_dir ()) ;
    vector<string> path_elems ;
    path_elems.push_back (dir.c_str ()) ;
    path_elems.push_back (a_glade_file_name) ;
    UString path_to_glade = Glib::build_filename (path_elems).c_str () ;
    if (!Glib::file_test (path_to_glade.c_str (),
                         Glib::FILE_TEST_IS_REGULAR)) {
        THROW ("couldn't find file " + path_to_glade) ;
    }
    return path_to_glade ;
}

UString
build_path_to_menu_file (const UString &a_menu_file_name)
{
    UString dir (get_menu_files_dir ()), path_to_menu_file ;
    vector<string> path_elems ;
    path_elems.push_back (dir.c_str ()) ;
    path_elems.push_back (a_menu_file_name) ;
    path_to_menu_file = Glib::build_filename (path_elems).c_str () ;
    if (!Glib::file_test (path_to_menu_file.c_str (),
                          Glib::FILE_TEST_IS_REGULAR)) {
        THROW ("couldn't find file " + path_to_menu_file) ;
    }
    return path_to_menu_file ;
}

UString
build_path_to_image_file (const UString &a_image_file_name)
{
    UString dir (get_image_files_dir ()) ;
    vector<string> path_elems ;
    path_elems.push_back (dir.c_str ()) ;
    path_elems.push_back (a_image_file_name) ;
    UString result = Glib::build_filename (path_elems).c_str () ;
    if (!Glib::file_test (result.c_str (),
                          Glib::FILE_TEST_IS_REGULAR)) {
        THROW ("couldn't find file " + result) ;
    }
    return result ;
}

UString
build_path_to_help_file (const UString &a_file_name)
{
    //So, before all, don't forget that a locale name has the general form:
    //language[_territory][.codeset][@modifier]
    //
    //Once that is said, let's not forget either that
    //the help file is located under the directory:
    //$prefix/share/gnome/help/nemiver/language[_territory]
    //So the goal of this function is to go look in that directory
    //to see if a_file_name exists there. If so, return the path to it.
    //Otherwise, return an empty string.
    UString result;
    UString prefix (get_install_prefix ());
    vector<string> path_elems;
    path_elems.push_back (prefix.c_str ());
    path_elems.push_back ("share");
    path_elems.push_back ("gnome");
    path_elems.push_back ("help");
    path_elems.push_back ("nemiver");

    string help_dir = Glib::build_filename (path_elems);

    if (!Glib::file_test (help_dir, Glib::FILE_TEST_IS_DIR)) {
        LOG_ERROR ("help dir " << help_dir << " does not exist");
        return "";
    }

    locale loc ("");
    UString loc_name (loc.name ());
    LOG_DD ("locale name: " << loc_name);
    string lang_dir = Glib::build_filename (help_dir, loc_name.c_str ());
    if (!Glib::file_test (lang_dir, Glib::FILE_TEST_IS_DIR)) {
        LOG_DD ("lang dir '" << lang_dir << "' does not exist");
        //let's try now to extract the <language>_<territory> part of the
        //locale, that can be of the more general form:
        //language[_territory][.codeset][@modifier]
        vector<UString> tmp;
        tmp = loc_name.split (".");
        if (tmp.empty ()) {
            goto locale_language;
        }
        loc_name = tmp[0];
        lang_dir = Glib::build_filename (help_dir, loc_name.c_str ());
        LOG_DD ("trying locale name: " << lang_dir);
        if (!Glib::file_test (lang_dir, Glib::FILE_TEST_IS_DIR)) {
            LOG_DD ("lang dir '" << lang_dir << "' does not exist");
            //let's try to extract the <language> part of the locale now.
locale_language:
            tmp.clear ();
            tmp = loc_name.split ("_");
            if (tmp.empty ()) {
                goto c_locale;
            }
            loc_name = tmp[0];
            lang_dir = Glib::build_filename (help_dir, loc_name.c_str ());
            LOG_DD ("trying locale name: " << lang_dir);
            if (!Glib::file_test (lang_dir, Glib::FILE_TEST_IS_DIR)) {
                LOG_DD ("lang dir '" << lang_dir << "' does not exist");
                //okay so let's fall back to the C locale then.
c_locale:
                loc_name = "C";
                lang_dir = Glib::build_filename (help_dir, loc_name.c_str ());
                LOG_DD ("trying locale name: " << lang_dir);
                if (!Glib::file_test (lang_dir, Glib::FILE_TEST_IS_DIR)) {
                    LOG_ERROR ("could not find a proper help dir");
                    return result;
                }
            }
        }
    }
    result = Glib::build_filename (lang_dir, a_file_name);
    if (!Glib::file_test (result, Glib::FILE_TEST_IS_REGULAR)) {
        LOG_ERROR ("file " << result << " does not exist!");
        result.clear ();
    }
    return result;
}

NEMIVER_END_NAMESPACE (env)
NEMIVER_END_NAMESPACE (common)
NEMIVER_END_NAMESPACE (nemiver)

