/* -*- Mode: C++; indent-tabs-mode:nil; c-basic-offset:4 -*- */

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
#include "config.h"
#include <sqlite3.h>
#include "common/nmv-ustring.h"
#include "common/nmv-exception.h"
#include "common/nmv-safe-ptr-utils.h"
#include "common/nmv-env.h"
#include "nmv-sqlite-cnx-drv.h"
#include "nmv-sqlite-cnx-mgr-drv.h"

using namespace nemiver::common;

NEMIVER_BEGIN_NAMESPACE (nemiver)
NEMIVER_BEGIN_NAMESPACE (common)
NEMIVER_BEGIN_NAMESPACE (sqlite)

struct SqliteCnxMgrDrvPriv {
};//end SqliteCnxMgrDrvPriv

SqliteCnxMgrDrv::SqliteCnxMgrDrv (DynamicModule *a_dynmod) :
    IConnectionManagerDriver (a_dynmod)
{
    m_priv = new SqliteCnxMgrDrvPriv ();

    //this is a singleton.
    enable_refcount (false);
}

SqliteCnxMgrDrv::~SqliteCnxMgrDrv ()
{
    if (!m_priv) {
        return;
    }

    delete m_priv;
    m_priv = 0;
}


IConnectionDriverSafePtr
SqliteCnxMgrDrv::connect_to_db (const DBDesc &a_db_desc,
                                const UString &a_user,
                                const UString &a_pass)
{

    if (a_user == "") {}
    if (a_pass == "") {}
    sqlite3 *sqlite (0);

    //HACK. As we are using sqlite, make sure to use a db file
    //that is in $HOME/.nemiver/db/sqlite
    UString db_name (a_db_desc.name ());
    if (!Glib::path_is_absolute (db_name)) {
        if (!Glib::file_test (env::get_user_db_dir (),
                              Glib::FILE_TEST_IS_DIR)) {
            env::create_user_db_dir ();
        }
        db_name = Glib::build_filename (env::get_user_db_dir (),
                                        db_name).c_str ();
    }

    int result = sqlite3_open (db_name.c_str (), &sqlite);
    if (result != SQLITE_OK) {
        THROW ("could not connect to sqlite database: "
               + UString (sqlite3_errmsg(sqlite)));
        sqlite3_close (sqlite);
        exit(1);
    }
    common::IConnectionDriverSafePtr connection_driver
                                            (new SqliteCnxDrv (sqlite));
    return connection_driver;
}

class SqliteCnxMgrModule : public DynamicModule {
    void get_info (Info &a_info) const
    {
        a_info.module_name = "org.nemiver.db.sqlitedriver.default";
        a_info.module_description = "The nemiver database driver for sqlite."
                                    " Implements the IConnectionManagerDriver "
                                    "iface";
        a_info.module_version = "0.0.1";
    }

    void do_init ()
    {
    }

    bool lookup_interface (const std::string &a_iface_name,
                           DynModIfaceSafePtr &a_iface)
    {
        if (a_iface_name == "IConnectionManagerDriver") {
            static SqliteCnxMgrDrv s_driver (this);
            s_driver.enable_refcount (false);
            a_iface.reset (&s_driver, true);
        } else {
            return false;
        }
        return true;
    }
};//end class SqliteCnxMgrModule

NEMIVER_END_NAMESPACE (sqlite)
NEMIVER_END_NAMESPACE (common)
NEMIVER_END_NAMESPACE (nemiver)

extern "C" {

bool
NEMIVER_API
nemiver_common_create_dynamic_module_instance (void **a_new_instance)
{
    RETURN_VAL_IF_FAIL (a_new_instance, false);

    try {
        nemiver::common::sqlite::SqliteCnxMgrModule *module =
            new nemiver::common::sqlite::SqliteCnxMgrModule;
        *a_new_instance = module;
    } catch (std::exception &e) {
        TRACE_EXCEPTION (e);
        return false;
    } catch (Glib::Exception &e) {
        TRACE_EXCEPTION (e);
        return false;
    } catch (...) {
        LOG ("Got an unknown exception");
        return false;
    }
    return true;
}
}//end extern C
