/* -*- Mode: C++; indent-tabs-mode:nil; c-basic-offset:4; -*- */

/*
 *This file is part of the Nemiver Project.
 *
 *Nemiver is free software; you can redistribute
 *it and/or modify it under the terms of
 *the GNU General Public License as published by the
 *Free Software Foundation; either version 2,
 *or (at your option) any later version.
 *
 *Nemiver is distributed in the hope that it will
 *be useful, but WITHOUT ANY WARRANTY;
 *without even the implied warranty of
 *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *See the GNU General Public License for more details.
 *
 *You should have received a copy of the
 *GNU General Public License along with Nemiver;
 *see the file COPYING.
 *If not, write to the Free Software Foundation,
 *Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *See COPYRIGHT file copyright information.
 */
#ifndef __NMV_CONF_MANAGER_H__
#define __NMV_CONF_MANAGER_H__

#include "nmv-object.h"
#include "nmv-safe-ptr-utils.h"


namespace nemiver {
namespace common {

class ConfManager;
class Config;
class UString;
struct ConfigPriv;

typedef SafePtr<Config, ObjectRef, ObjectUnref> ConfigSafePtr;

class NEMIVER_API Config : public Object
{

    friend class ConfManager;
    friend class ConfigPriv;

    ConfigPriv *m_priv;

    Config ();
    Config (const Config &);
    Config& operator= (const Config &);
    virtual ~Config ();

public:

    bool get_property (const UString &a_name, UString &a_value);
    void set_property (const UString a_name, const UString a_value);

}
;//end class Config

class NEMIVER_API ConfManager
{
    //forbid instantiation/copy/assignation
    ConfManager ();
    ConfManager (const ConfManager &);
    ConfManager& operator= (const ConfManager &);

    static void set_config (const Config &a_conf);

public:

    static Config& parse_config_file (const UString &a_path);

    static Config& parse_user_config_file (bool a_create_if_not_exist=true);

    static bool user_config_dir_exists ();

    static const std::string& get_user_config_dir_path ();

    static void create_default_config_file (const UString a_path);

    static void create_default_config_file (std::ostream &a_ostream);

    static void init ();

    static Config& get_config ();
};//end class ConfManager

}//end namespace common
}//end namespace nemiver

#endif //__NMV_CONF_MANAGER_H__

