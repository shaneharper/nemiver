/* -*- Mode: C++; indent-tabs-mode:nil; c-basic-offset:4; -*- */

//Author: Dodji Seketeli
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
#include <cstring>
#include <glib.h>
#include <glib/gstdio.h>
#include <fstream>
#include <map>
#include <glibmm.h>
#include "nmv-libxml-utils.h"
#include "nmv-ustring.h"
#include "nmv-exception.h"
#include "nmv-conf-manager.h"
#include "nmv-env.h"

static const char *NEMIVER_CONFIG_TOP_DIR_NAME = ".nemiver";
static const char *NEMIVER_CONFIG_DIR_NAME = "config";
static const char *NEMIVER_CONFIG_FILE_NAME = "nemiver.conf";

using namespace std;
using namespace Glib;

namespace nemiver {
namespace common {

typedef map<UString, UString> StringToStringMap;

struct ConfigPriv
{
    Glib::RecMutex mutex;
    StringToStringMap props;
}
;//end ConfigPriv

static Glib::RecMutex&
config_mutex ()
{
    static Glib::RecMutex s_config_mutex;
    return s_config_mutex;
}


Config::Config ()
{
    m_priv = new ConfigPriv;
}

Config::Config (const Config &a_conf) :
    Object (a_conf)
{
    m_priv = new ConfigPriv ();
    m_priv->props = a_conf.m_priv->props;
}

Config&
Config::operator= (const Config &a_conf)
{
    if (this == &a_conf) {
        return *this;
    }
    m_priv->props = a_conf.m_priv->props;
    return *this;
}

Config::~Config ()
{
    if (m_priv) {
        delete m_priv;
        m_priv = 0;
    }
}

bool
Config::get_property (const UString &a_name, UString &a_value)
{
    StringToStringMap::iterator it = m_priv->props.find (a_name);
    if (it == m_priv->props.end ()) {
        return false;
    }
    a_value = it->second;
    return true;
}

void
Config::set_property (const UString a_name, const UString a_value)
{
    if (a_name == "")
        return;

    {
        Glib::RecMutex::Lock lock (m_priv->mutex)
           ;
        m_priv->props.insert
        (StringToStringMap::value_type (a_name, a_value));
    }
}

Config&
ConfManager::get_config ()
{

    static Config s_config;
    return s_config;
}

void
ConfManager::set_config (const Config &a_conf)
{
    Glib::RecMutex::Lock lock (config_mutex ());
    get_config () = a_conf;
}

Config&
ConfManager::parse_config_file (const UString &a_path)
{
    Config conf;

    if (a_path == "") {
        THROW ("Got path \"\" to config file");
    }

    using libxmlutils::XMLTextReaderSafePtr;
    using libxmlutils::XMLCharSafePtr ;
    XMLTextReaderSafePtr reader;

    reader.reset (xmlNewTextReaderFilename (a_path.c_str ()));
    if (!reader) {
        THROW ("could not create xml reader");
    }

    xmlReaderTypes type;
    int type_tmp (0);
    int res (0);
    for (res = xmlTextReaderRead (reader.get ());
         res > 0;
         res = xmlTextReaderRead (reader.get ())) {

        type_tmp = xmlTextReaderNodeType (reader.get ());
        THROW_IF_FAIL2 (type_tmp >= 0, "got an error while parsing conf file");
        type = static_cast<xmlReaderTypes> (type_tmp);

        switch (type) {
        case XML_READER_TYPE_ELEMENT: {
                XMLCharSafePtr str (xmlTextReaderName (reader.get ()));
                UString name = reinterpret_cast<const char*> (str.get ());
                if (name == "database") {
                    xmlNode* node = xmlTextReaderExpand (reader.get ());
                    THROW_IF_FAIL (node);
                    for (xmlNode *cur_node=node->children;
                         cur_node;
                         cur_node = cur_node->next) {
                        if (cur_node->type == XML_ELEMENT_NODE
                            && cur_node->name) {
                            if (!strncmp ("connection",
                                        (const char*)cur_node->name, 10)) {
                                THROW_IF_FAIL (cur_node->children
                                        && xmlNodeIsText
                                        (cur_node->children));
                                XMLCharSafePtr con
                                    (xmlNodeGetContent (cur_node->children));
                                conf.set_property ("database.connection",
                                        (char*)con.get ());
                            } else if (!strncmp ("username",
                                        (const char*)cur_node->name, 8)) {
                                THROW_IF_FAIL (cur_node->children
                                        && xmlNodeIsText
                                        (cur_node->children));
                                XMLCharSafePtr user
                                    (xmlNodeGetContent (cur_node->children));
                                conf.set_property ("database.username",
                                        (const char*) user.get ());
                            } else if (!strncmp ("password",
                                        (const char*)cur_node->name, 8)) {
                                THROW_IF_FAIL (cur_node->children
                                        && xmlNodeIsText
                                        (cur_node->children));
                                XMLCharSafePtr user
                                    (xmlNodeGetContent (cur_node->children));
                                conf.set_property ("database.password",
                                        (const char*)user.get ());
                            }
                        }
                    }
                } else if (name == "logging") {
                    xmlNode* node = xmlTextReaderExpand (reader.get ());
                    THROW_IF_FAIL (node);
                    for (xmlNode *cur_node=node->children;
                         cur_node;
                         cur_node = cur_node->next) {
                         if (cur_node->type == XML_ELEMENT_NODE
                            && cur_node->name ) {
                            if (!strncmp ("enabled",
                                         (const char*)cur_node->name, 7)) {
                                XMLCharSafePtr value
                                    (xmlGetProp (cur_node, (xmlChar*) "value"));
                                conf.set_property ("logging.enabled",
                                        (const char*)value.get ());
                            } else if (!strncmp ("level",
                                                 (const char*)cur_node->name, 5)){
                                XMLCharSafePtr value
                                ((xmlChar*)xmlGetProp (cur_node,
                                                      (const xmlChar*)"value"));
                                conf.set_property ("logging.level",
                                        (const char*)value.get ());
                            } else if (!strncmp ("logstreamtype",
                                                 (const char*)cur_node->name, 13)) {
                                libxmlutils::XMLCharSafePtr value
                                ((xmlChar*) xmlGetProp (cur_node,
                                                        (const xmlChar*)"value"));
                                conf.set_property ("logging.logstreamtype",
                                        (const char*)value.get ());
                            } else if (!strncmp ("logfile",
                                                 (const char*)cur_node->name, 7)){
                                libxmlutils::XMLCharSafePtr content
                                ((xmlChar*)xmlNodeGetContent(cur_node->children));
                                conf.set_property ("logging.logfile",
                                        (const char*)content.get ());
                            }
                        }
                    }
                } else if (name == "config") {
                    XMLCharSafePtr value (xmlTextReaderGetAttribute
                        (reader.get (), (const xmlChar*)"version"));
                    conf.set_property ("config.version",
                                       (const char*)value.get ());
                }
            }
            break;
        default:
            break;
        }
    }
    THROW_IF_FAIL2 (res == 0, "got an error while parsing config file");
    set_config (conf);

    return get_config ();
}

Config&
ConfManager::parse_user_config_file (bool a_create_if_not_exists)
{
    string home_dir = get_home_dir ();
    vector<string> path_elems;
    path_elems.push_back (home_dir);
    path_elems.push_back (NEMIVER_CONFIG_TOP_DIR_NAME);
    path_elems.push_back (NEMIVER_CONFIG_DIR_NAME);
    string user_config_path = build_filename (path_elems);

    if (!file_test (user_config_path, FILE_TEST_IS_DIR))
        THROW_IF_FAIL (g_mkdir_with_parents
                       (user_config_path.c_str (), S_IRWXU) == 0);

    string user_config_file = build_filename
                              (user_config_path, NEMIVER_CONFIG_FILE_NAME);

    if (!file_test (user_config_file, FILE_TEST_EXISTS)
        && a_create_if_not_exists) {
        create_default_config_file (user_config_file.c_str ());
    }

    parse_config_file (user_config_file.c_str ());
    return get_config ();
}

bool
ConfManager::user_config_dir_exists ()
{
    if (file_test (get_user_config_dir_path (), FILE_TEST_EXISTS)) {
        return true;
    }
    return false;
}

const string&
ConfManager::get_user_config_dir_path ()
{
    static string user_config_dir;
    if (user_config_dir.empty ()) {
        vector<string> path_elems;
        path_elems.push_back (get_home_dir ());
        path_elems.push_back (NEMIVER_CONFIG_TOP_DIR_NAME);
        user_config_dir = build_filename (path_elems);
    }
    LOG_DD ("user_config_dir: " << user_config_dir);
    return user_config_dir;
}

void
ConfManager::create_default_config_file (const UString a_path)
{

    ofstream of;
    try {
        string filepath (Glib::filename_from_utf8 (a_path));
        of.open (filepath.c_str ());
        THROW_IF_FAIL (of.good ());
        create_default_config_file (of);
        of.flush ();
    } catch (nemiver::common::Exception &e) {
        of.close ();
        RETHROW_EXCEPTION (e);
    } catch (Glib::Exception &e) {
        of.close ();
        RETHROW_EXCEPTION (e);
    } catch (std::exception &e) {
        of.close ();
        RETHROW_EXCEPTION (e);
    }
    of.close ();
}

void
ConfManager::create_default_config_file (std::ostream &a_ostream)
{
    a_ostream <<

    "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n"
    "<config version=\"0.0.1\">\n"
    "    <database>\n"
    "        <connection>vdbc:sqlite://localhost/nemivercommon.db</connection>\n"
    "        <username>nemiver</username>\n"
    "        <password>pass</password>\n"
    "    </database>\n"
    "\n"
    "    <logging>\n"
    "        <enabled value=\"true\"/>\n"
    "        <!--<level value=\"verbose\"/>-->\n"
    "        <level value=\"normal\"/>\n"
    "        <!--<logstreamtype value=\"file\"/>-->\n"
    "        <!--<logstreamtype value=\"stderr\"/>-->\n"
    "        <logstreamtype value=\"stdout\"/>\n"
    "        <logfile>/tmp/nemiver.log</logfile>\n"
    "    </logging>\n"
    "</config>\n";
    THROW_IF_FAIL (a_ostream.good ());
}

void
ConfManager::init ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    static bool initialized=false;

    if (initialized)
        return;

    const char* configfile (g_getenv ("nemiverconfigfile"));
    if (configfile) {
        parse_config_file (configfile);
    } else if (file_test ("nemiver.conf", FILE_TEST_IS_REGULAR)) {
        parse_config_file ("nemiver.conf");
    } else {
        parse_user_config_file ();
    }

    initialized = true;
}

}//end namespace common
}//end namespace nemiver

