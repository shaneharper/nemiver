/* -*- Mode: C++; indent-tabs-mode:nil; c-basic-offset:4; -*- */

// Author: Dodji Seketeli
/*
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
#ifndef __NMV_ENV_H__
#define __NMV_ENV_H__

#include "nmv-api-macros.h"
#include "nmv-ustring.h"
#include "nmv-exception.h"

using nemiver::common::UString;

NEMIVER_BEGIN_NAMESPACE (nemiver)
NEMIVER_BEGIN_NAMESPACE (common)
NEMIVER_BEGIN_NAMESPACE (env)

void do_init ();

NEMIVER_API const UString& get_system_config_dir ();

NEMIVER_API const UString& get_system_config_file ();

NEMIVER_API const UString& get_data_dir ();

NEMIVER_API const UString& get_system_lib_dir ();

NEMIVER_API const UString& get_gtkbuilder_files_dir ();

NEMIVER_API const UString& get_menu_files_dir ();

NEMIVER_API const UString& get_image_files_dir ();

NEMIVER_API const UString& get_install_prefix ();

NEMIVER_API const UString& get_system_modules_config_file ();

NEMIVER_API const UString& get_system_modules_dir ();

NEMIVER_API const UString& get_system_modules_config_file ();

NEMIVER_API const UString& get_system_plugins_dir ();

NEMIVER_API const UString& get_user_db_dir ();

NEMIVER_API const UString& get_gdb_program ();

NEMIVER_API bool create_user_db_dir ();

NEMIVER_API UString build_path_to_gtkbuilder_file (const UString &a_gtkbuilder_file_name);

NEMIVER_API UString build_path_to_menu_file (const UString &a_ui_file_name);

NEMIVER_API UString build_path_to_image_file (const UString &a_image_file_name);

NEMIVER_API UString build_path_to_help_file (const UString &a_file_name);

NEMIVER_API bool build_path_to_executable (const UString &a_exe_name,
                                           UString &a_exe_path);

NEMIVER_API bool find_file (const UString &a_file_name,
                            const list<UString> &a_where_to_look,
                            UString &a_aboslute_file_path);

NEMIVER_API bool read_file_line (const UString &a_file_path,
                                 int a_line_number,
                                 string &a_line);

NEMIVER_END_NAMESPACE (env)
NEMIVER_END_NAMESPACE (common)
NEMIVER_END_NAMESPACE (nemiver)

#endif //__NMV_ENV_H__

