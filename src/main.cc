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
#include <signal.h>
#include <iostream>
#include <gtkmm/window.h>
#include <libglademm.h>
#include <glib/gi18n.h>
#include <libgnome/gnome-init.h>
#include "nmv-exception.h"
#include "nmv-initializer.h"
#include "nmv-i-workbench.h"
#include "nmv-ui-utils.h"
#include "nmv-proc-mgr.h"
#include "nmv-env.h"
#include "nmv-dbg-perspective.h"
#include "config.h"

using namespace std;
using nemiver::common::DynamicModuleManager;
using nemiver::common::Initializer;
using nemiver::IWorkbench;
using nemiver::IWorkbenchSafePtr;
using nemiver::IDBGPerspective;
using nemiver::common::UString;
using nemiver::ISessMgr;

static const UString DBGPERSPECTIVE_PLUGIN_NAME="dbgperspective";
static gchar *gv_env_vars=0;
static gchar *gv_process_to_attach_to=0;
static bool gv_list_sessions=false;
static bool gv_purge_sessions=false;
static int gv_execute_session=0;
static bool gv_last_session=false;
static gchar *gv_log_domains=0;
static bool gv_log_debugger_output=false;

static GOptionEntry entries[] =
{
    {
      "env",
      0,
      0,
      G_OPTION_ARG_STRING,
      &gv_env_vars,
      _("Set the environment of the program to debug"),
      "\"var0=val0 var1=val1 var2=val2 ...\""
    },
    {
      "attach",
      0,
      0,
      G_OPTION_ARG_STRING,
      &gv_process_to_attach_to,
      _("Attach to a process"),
      "<pid|process name>"
    },
    { "listsessions",
      0,
      0,
      G_OPTION_ARG_NONE,
      &gv_list_sessions,
      _("List the saved debugging sessions"),
      NULL
    },
    { "purgesessions",
      0,
      0,
      G_OPTION_ARG_NONE,
      &gv_purge_sessions,
      _("Erase the saved debugging sessions"),
      NULL
    },
    { "session",
      0,
      0,
      G_OPTION_ARG_INT,
      &gv_execute_session,
      _("Debug the program that was of session number N"),
      "N"
    },
    { "last",
      0,
      0,
      G_OPTION_ARG_NONE,
      &gv_last_session,
      _("Execute the last session"),
      NULL
    },
    { "log-domains",
      0,
      0,
      G_OPTION_ARG_STRING,
      &gv_log_domains,
      _("Enable logging domains DOMAINS"),
      "DOMAINS"
    },
    { "logdebuggeroutput",
      0,
      0,
      G_OPTION_ARG_NONE,
      &gv_log_debugger_output,
      _("Log the debugger output"),
      NULL
    },
    {0,0,0,(GOptionArg)0,0,0,0}
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
    g_option_context_set_ignore_unknown_options (context.get (), true);
    GOptionGroupSafePtr gtk_option_group (gtk_get_option_group (FALSE));
    THROW_IF_FAIL (gtk_option_group);
    g_option_context_add_group (context.get (), gtk_option_group.release ());
    return context.release ();
}

static void
parse_command_line (int& a_argc,
                    char** a_argv)
{
    GOptionContextSafePtr context (init_option_context ());
    THROW_IF_FAIL (context);
    g_option_context_parse (context.get (), &a_argc, &a_argv, 0);
}

static bool
process_command_line (int& a_argc, char** a_argv, int &a_return)
{
    if (gv_log_debugger_output) {
        LOG_STREAM.enable_domain ("gdbmi-output-domain");
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

    if (gv_process_to_attach_to) {
        using nemiver::common::IProcMgrSafePtr;
        using nemiver::common::IProcMgr;

        IDBGPerspective *debug_persp =
                dynamic_cast<IDBGPerspective*> (s_workbench->get_perspective
                                                (DBGPERSPECTIVE_PLUGIN_NAME));
        if (!debug_persp) {
            cerr << "Could not get the debugging perspective" << endl;
            a_return = -1;
            return false;
        }
        int pid = atoi (gv_process_to_attach_to);
        if (!pid) {
            IProcMgrSafePtr proc_mgr = IProcMgr::create ();
            if (!proc_mgr) {
                cerr << "Could not create proc mgr" << endl;
                a_return = -1;
                return false;
            }
            IProcMgr::Process process;
            if (!proc_mgr->get_process_from_name (gv_process_to_attach_to,
                                                  process, true)) {
                cerr << "Could not find any process named '"
                << gv_process_to_attach_to
                << "'"
                << endl
               ;
                a_return = -1;
                return false;
            }
            pid = process.pid ();
        }
        if (!pid) {
            cerr << "Could not find any process '"
                 << gv_process_to_attach_to
                 << "'"
                 << endl
                ;
            a_return = -1;
            return false;
        } else {
            debug_persp->attach_to_program (pid);
        }
    }

    if (gv_list_sessions) {
        IDBGPerspective *debug_persp =
            dynamic_cast<IDBGPerspective*> (s_workbench->get_perspective
                                                (DBGPERSPECTIVE_PLUGIN_NAME));
        if (debug_persp) {
            debug_persp->session_manager ().load_sessions ();
            list<ISessMgr::Session>::iterator session_iter;
            list<ISessMgr::Session>& sessions =
                            debug_persp->session_manager ().sessions ();
            for (session_iter = sessions.begin ();
                 session_iter != sessions.end ();
                 ++session_iter) {
                cout << session_iter->session_id ()
                     << " "
                     << session_iter->properties ()["sessionname"]
                     << "\n"
                    ;
            }
            a_return = 0;
            return false;
        } else {
            cerr << "Could not find the debugger perpective plugin";
            a_return = -1;
            return false;
        }
    }

    if (gv_purge_sessions) {
        IDBGPerspective *debug_persp =
            dynamic_cast<IDBGPerspective*> (s_workbench->get_perspective
                                                (DBGPERSPECTIVE_PLUGIN_NAME));
        if (debug_persp) {
            debug_persp->session_manager ().delete_sessions ();
        }
        a_return = 0;
        return false;
    }

    if (gv_execute_session) {
        IDBGPerspective *debug_persp =
            dynamic_cast<IDBGPerspective*> (s_workbench->get_perspective
                                                (DBGPERSPECTIVE_PLUGIN_NAME));
        if (debug_persp) {
            debug_persp->session_manager ().load_sessions ();
            list<ISessMgr::Session>::iterator session_iter;
            list<ISessMgr::Session>& sessions =
                            debug_persp->session_manager ().sessions ();
            bool found_session=false;
            for (session_iter = sessions.begin ();
                 session_iter != sessions.end ();
                 ++session_iter) {
                if (session_iter->session_id () == gv_execute_session) {
                    debug_persp->execute_session (*session_iter);
                    found_session = true;
                    break;
                }
            }

            if (!found_session) {
                cerr << "Could not find session of number "
                     << gv_execute_session
                     << "\n";
                a_return = -1;
                return false;
            }
            return true;
        }
    }

    //execute the last session if one exists
    if (gv_last_session) {
        IDBGPerspective *debug_persp =
            dynamic_cast<IDBGPerspective*> (s_workbench->get_perspective
                                                (DBGPERSPECTIVE_PLUGIN_NAME));
        if (debug_persp) {
            debug_persp->session_manager ().load_sessions ();
            list<ISessMgr::Session>& sessions =
                            debug_persp->session_manager ().sessions ();
            if (!sessions.empty ()) {
                list<ISessMgr::Session>::iterator session_iter,
                    latest_session_iter;
                glong time_val = 0;
                for (session_iter = sessions.begin ();
                        session_iter != sessions.end ();
                        ++session_iter) {
                    std::map<UString, UString>::const_iterator map_iter = session_iter->properties ().find ("lastruntime");
                    if (map_iter != session_iter->properties ().end ()) {
                        glong new_time = atoi (map_iter->second.c_str ());
                        if (new_time > time_val) {
                            time_val = new_time;
                            latest_session_iter = session_iter;
                        }
                    }
                }
                debug_persp->execute_session (*latest_session_iter);
            } else {
                cerr << "Could not find any sessions"
                     << "\n";
                a_return = -1;
                return false;
            }
            return true;
        }
    }

    if (a_argc > 1) {
        if (a_argv[1][0] == '-') {
            std::cerr << "unknown option " << a_argv[1] << "\n";
            a_return = 0;
            return false;
        }
        UString prog_args;
        for (int i=1; i < a_argc;++i) {
            prog_args +=  Glib::locale_to_utf8 (a_argv[i]) + " ";
        }
        IDBGPerspective *debug_persp =
            dynamic_cast<IDBGPerspective*> (s_workbench->get_perspective
                                                (DBGPERSPECTIVE_PLUGIN_NAME));
        if (debug_persp) {
            LOG_D ("going to debug program: '"
                   << prog_args << "'\n",
                   NMV_DEFAULT_DOMAIN);
            map<UString, UString> env;
            if (gv_env_vars) {
                vector<UString> env_vars = UString (gv_env_vars).split (" ");
                for (vector<UString>::const_iterator it = env_vars.begin ();
                     it != env_vars.end ();
                     ++it) {
                    vector<UString> env_var = it->split ("=");
                    if (env_var.size () != 2) {
                        continue;
                    }
                    UString name = env_var[0];
                    name.chomp ();
                    UString value = env_var[1];
                    value.chomp ();
                    LOG_DD ("got env var: " << name << "=" << value);
                    env[name] = value;
                }
            }
            debug_persp->execute_program (prog_args, env);
        } else {
            cerr << "Could not find the debugger perspective plugin\n";
            a_return = -1;
            return false;
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

    parse_command_line (a_argc, a_argv);

    //intialize gnome libraries
    gnome_program_init (PACKAGE, PACKAGE_VERSION,
                        LIBGNOME_MODULE, a_argc, a_argv,
                        GNOME_PROGRAM_STANDARD_PROPERTIES, NULL);


    //**********************************
    //load the workbench dynamic module
    //**********************************
    DynamicModuleManager module_manager;
    IWorkbenchSafePtr workbench =
        module_manager.load_iface<IWorkbench> ("workbench", "IWorkbench");
    s_workbench = workbench.get ();
    LOG_D ("workbench refcount: " <<  (int) s_workbench->get_refcount (),
            "refcount-domain");
    s_workbench->do_init (gtk_kit);
    LOG_D ("workbench refcount: " <<  (int) s_workbench->get_refcount (),
           "refcount-domain");

    int retval;
    if (process_command_line (a_argc, a_argv, retval) != true) {
        return retval;
    }

    //intercept ctrl-c/SIGINT
    signal (SIGINT, sigint_handler);
    gtk_kit.run (s_workbench->get_root_window ());

    NEMIVER_CATCH_NOX
    s_workbench = 0;

    return 0;
}

