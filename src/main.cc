//Author: Dodji Seketeli
/*
 *This file is part of the Nemiver project
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
#include "config.h"
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <gtkmm/window.h>
#include <glib/gi18n.h>
#include "nmv-str-utils.h"
#include "nmv-exception.h"
#include "nmv-initializer.h"
#include "nmv-i-workbench.h"
#include "nmv-ui-utils.h"
#include "nmv-proc-mgr.h"
#include "nmv-env.h"
#include "nmv-i-perspective.h"
#include "nmv-i-conf-mgr.h"

using namespace std;
using nemiver::IConfMgr;
using nemiver::common::DynamicModuleManager;
using nemiver::common::Initializer;
using nemiver::IWorkbench;
using nemiver::IWorkbenchSafePtr;
using nemiver::IPerspective;
using nemiver::IPerspectiveSafePtr;
using nemiver::common::UString;
using nemiver::common::GCharSafePtr;

static IWorkbench *s_workbench=0;

void
sigint_handler (int a_signum)
{
    if (a_signum != SIGINT) {
        return;
    }
    static bool s_got_down = false;
    if (!s_got_down && s_workbench) {
        s_workbench->shut_down ();
        s_workbench = 0;
        s_got_down = true;
    }
}

int
main (int a_argc, char *a_argv[])
{
    bindtextdomain (GETTEXT_PACKAGE, NEMIVERLOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    NEMIVER_TRY

    Initializer::do_init ();
    Gtk::Main gtk_kit (a_argc, a_argv);

    //********************************************
    //load and init the workbench dynamic module
    //********************************************
    IWorkbenchSafePtr workbench =
        nemiver::load_iface_and_confmgr<IWorkbench> ("workbench",
                                                     "IWorkbench");
    s_workbench = workbench.get ();
    THROW_IF_FAIL (s_workbench);
    LOG_D ("workbench refcount: " <<  (int) s_workbench->get_refcount (),
           "refcount-domain");

    if (!s_workbench->do_init (gtk_kit, a_argc, a_argv)) {
        return -1;
    }
    LOG_D ("workbench refcount: " <<  (int) s_workbench->get_refcount (),
           "refcount-domain");

    //intercept ctrl-c/SIGINT
    signal (SIGINT, sigint_handler);

    gtk_kit.run (s_workbench->get_root_window ());

    NEMIVER_CATCH_NOX
    s_workbench = 0;

    return 0;
}

