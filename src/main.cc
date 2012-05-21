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

static gchar *gv_log_domains=0;
static bool gv_show_version = false;
static gchar *gv_tool = (gchar*) "dbgperspective";

static GOptionEntry entries[] =
{
    {
        "tool",
        0,
        0,
        G_OPTION_ARG_STRING,
        &gv_tool,
        _("Use the nemiver tool named <name>"),
        "<name>"
    },
    { "log-domains",
      0,
      0,
      G_OPTION_ARG_STRING,
      &gv_log_domains,
      _("Enable logging domains DOMAINS"),
      "<DOMAINS>"
    },
    { 
        "version",
        0,
        0,
        G_OPTION_ARG_NONE,
        &gv_show_version,
        _("Show the version number of Nemiver"),
        0
    },
    {0, 0, 0, (GOptionArg) 0, 0, 0, 0}
};

struct GOptionContextUnref {
    void operator () (GOptionContext *a_opt)
    {
        if (a_opt) {
            g_option_context_free (a_opt);
        }
    }
};//end struct GOptionContextUnref

struct GOptionGroupUnref {
    void operator () (GOptionGroup *a_group)
    {
        if (a_group) {
            g_option_group_free (a_group);
        }
    }
};//end struct GOptionGroupUnref

struct GOptionContextRef {
    void operator () (GOptionContext *a_opt)
    {
        if (a_opt) {}
    }
};//end struct GOptionContextRef

struct GOptionGroupRef {
    void operator () (GOptionGroup *a_group)
    {
        if (a_group) {}
    }
};//end struct GOptionGroupRef

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
typedef SafePtr<GOptionContext,
                GOptionContextRef,
                GOptionContextUnref> GOptionContextSafePtr;

typedef SafePtr<GOptionGroup,
                GOptionGroupRef,
                GOptionGroupUnref> GOptionGroupSafePtr;

static GOptionContext*
init_option_context ()
{
    GOptionContextSafePtr context;
    context.reset (g_option_context_new
                                (_(" [<prog-to-debug> [prog-args]]")));
#if GLIB_CHECK_VERSION (2, 12, 0)
    g_option_context_set_summary (context.get (),
                                  _("A C/C++ debugger for GNOME"));
#endif
    g_option_context_set_help_enabled (context.get (), true);
    g_option_context_add_main_entries (context.get (),
                                       entries,
                                       GETTEXT_PACKAGE);
    g_option_context_set_ignore_unknown_options (context.get (), false);
    GOptionGroupSafePtr gtk_option_group (gtk_get_option_group (FALSE));
    THROW_IF_FAIL (gtk_option_group);
    g_option_context_add_group (context.get (),
                                gtk_option_group.release ());

    std::list<IPerspectiveSafePtr>::iterator perspective;
    std::list<IPerspectiveSafePtr> perspectives = s_workbench->perspectives ();
    for (perspective = perspectives.begin ();
         perspective != perspectives.end ();
         ++perspective) {
        if (*perspective && (*perspective)->option_group ()) {
            g_option_context_add_group
                (context.get (), (*perspective)->option_group ());
        }
    }

    return context.release ();
}

/// Parse the command line and edits it
/// to make it contain the command line of the inferior program.
/// If an error happens (e.g, the user provided the wrong command
/// lines) then display an usage help message and return
/// false. Otherwise, return true.
/// \param a_arg the argument count. This is the length of a_argv.
///  This is going to be edited. After edit, only the number of
///  arguments of the inferior will be put in this variable.
/// \param a_argv the string of arguments passed to Nemiver. This is
/// going to be edited so that only the arguments passed to the
/// inferior will be left in this.
/// \return true upon successful completion, false otherwise. If the
static bool
parse_command_line (int& a_argc,
                    char** a_argv)
{
    GOptionContextSafePtr context (init_option_context ());
    THROW_IF_FAIL (context);

    if (a_argc == 1) {
        // We have no inferior program so edit the command like accordingly.
        a_argc = 0;
        a_argv[0] = 0;
        return true;
    }

    // Split the command line in two parts. One part is made of the
    // options for Nemiver itself, and the other part is the options
    // relevant to the inferior.
    int i;
    std::vector<UString> args;
    for (i = 1; i < a_argc; ++i)
        if (a_argv[i][0] != '-')
            break;

    // Now parse only the part of the command line that is related
    // to Nemiver and not to the inferior.
    // Once parsed, make a_argv and a_argv contain the command line
    // of the inferior.
    char **nmv_argv, **inf_argv;
    int nmv_argc = a_argc;
    int inf_argc = 0;

    if (i < a_argc) {
        nmv_argc = i;
        inf_argc = a_argc - i;
    }
    nmv_argv = a_argv;
    inf_argv = a_argv + i;
    GError *error = 0;
    if (g_option_context_parse (context.get (),
                                &nmv_argc,
                                &nmv_argv,
                                &error) != TRUE) {
        NEMIVER_TRY;
        if (error)
            cerr << "error: "<< error->message << std::endl;
        NEMIVER_CATCH;
        g_error_free (error);

        GCharSafePtr help_message;
        help_message.reset (g_option_context_get_help (context.get (),
                                                       true, 0));
        cerr << help_message.get () << std::endl;
        return false;
    }

    IPerspectiveSafePtr perspective = s_workbench->get_perspective (gv_tool);
    if (!perspective) {
        std::cerr << "Invalid tool name: " << gv_tool << std::endl;
        return false;
    }

    if (!perspective->process_options  (context.get (), inf_argc, inf_argv)) {
        return false;
    }

    if (a_argv != inf_argv) {
        memmove (a_argv, inf_argv, inf_argc * sizeof (char*));
        a_argc = inf_argc;
    }

    return true;
}

// Return true if Nemiver should keep going after the non gui options
// have been processed.
static bool
process_non_gui_options ()
{
    if (gv_show_version) {
        cout << PACKAGE_VERSION << endl;
        return false;
    }

    if (gv_log_domains) {
        UString log_domains (gv_log_domains);
        vector<UString> domains = log_domains.split (" ");
        for (vector<UString>::const_iterator iter = domains.begin ();
             iter != domains.end ();
             ++iter) {
            LOG_STREAM.enable_domain (*iter);
        }
    }

    return true;
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
    s_workbench->load_perspectives ();

    if (!parse_command_line (a_argc, a_argv))
        return -1;

    if (!process_non_gui_options ()) {
        return -1;
    }

    s_workbench->do_init (gtk_kit);
    LOG_D ("workbench refcount: " <<  (int) s_workbench->get_refcount (),
           "refcount-domain");

    IPerspectiveSafePtr perspective = s_workbench->get_perspective (gv_tool);
    if (!perspective) {
        std::cerr << "Invalid tool name: " << gv_tool << std::endl;
        return -1;
    }

    if (!perspective->process_gui_options  (a_argc, a_argv)) {
        return -1;
    }

    s_workbench->select_perspective (perspective);

    //intercept ctrl-c/SIGINT
    signal (SIGINT, sigint_handler);

    gtk_kit.run (s_workbench->get_root_window ());

    NEMIVER_CATCH_NOX
    s_workbench = 0;

    return 0;
}

