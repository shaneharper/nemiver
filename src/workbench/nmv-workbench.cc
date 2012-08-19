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
#include <vector>
#include <iostream>
#include <glib/gi18n.h>
#include <giomm/init.h>
#include <gtkmm/aboutdialog.h>
#include <gtkmm/icontheme.h>
#include "common/nmv-exception.h"
#include "common/nmv-plugin.h"
#include "common/nmv-log-stream-utils.h"
#include "nmv-ui-utils.h"
#include "nmv-i-workbench.h"
#include "nmv-i-perspective.h"
#include "nmv-i-conf-mgr.h"
#include "nmv-conf-keys.h"

using namespace std;
using namespace nemiver;
using namespace nemiver::common;

#ifndef PACKAGE_URL
// This is not define with autoconf < 2.64
#define PACKAGE_URL "http://projects.gnome.org/nemiver"
#endif

NEMIVER_BEGIN_NAMESPACE (nemiver)

static gchar *gv_log_domains = 0;
static bool gv_show_version = false;
static gchar *gv_tool = (gchar*) "debugger";
static bool gv_show_tool_list = false;

static GOptionEntry entries[] =
{
    {
        "tool",
        0,
        0,
        G_OPTION_ARG_STRING,
        &gv_tool,
        _("Use the nemiver tool named <name>"),
        "<name>; use --list-tools to get the possibilities"
    },
    {
        "list-tools",
        0,
        0,
        G_OPTION_ARG_NONE,
        &gv_show_tool_list,
        _("Show the list of tool available"),
        0
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

typedef SafePtr<GOptionContext,
                GOptionContextRef,
                GOptionContextUnref> GOptionContextSafePtr;

typedef SafePtr<GOptionGroup,
                GOptionGroupRef,
                GOptionGroupUnref> GOptionGroupSafePtr;

class WorkbenchStaticInit {
    WorkbenchStaticInit ()
    {
        Gio::init();
    }

    ~WorkbenchStaticInit ()
    {
    }

public:
    static void do_init ()
    {
        static WorkbenchStaticInit s_wb_init;
    }

};//end class WorkbenchStaticInit

class Workbench : public IWorkbench {
    struct Priv;
    SafePtr<Priv> m_priv;

    Workbench (const Workbench&);
    Workbench& operator= (const Workbench&);


private:

    //************************
    //<slots (signal callbacks)>
    //************************
    void on_quit_menu_item_action ();
    void on_about_menu_item_action ();
    void on_contents_menu_item_action ();
    void on_shutting_down_signal ();
    void on_perspective_layout_changed_signal (IPerspectiveSafePtr);
    void on_perspective_changed ();
    //************************
    //</slots (signal callbacks)>
    //************************

    void init_builder ();
    void init_window ();
    void init_actions ();
    void init_menubar ();
    void init_toolbar ();
    void init_body ();
    void add_perspective_toolbars (IPerspectiveSafePtr &a_perspective,
                                   list<Gtk::Widget*> &a_tbs);
    void add_perspective_body (IPerspectiveSafePtr &a_perspective,
                               Gtk::Widget *a_body);
    void add_perspective_to_perspective_selector
        (IPerspectiveSafePtr &a_perspective);
    bool remove_perspective_body (IPerspectiveSafePtr &a_perspective);
    void remove_all_perspective_bodies ();
    void disconnect_all_perspective_signals ();
    bool process_non_gui_options ();
    void save_window_geometry ();
    bool parse_command_line (int &a_argc, char** a_argv);
    GOptionContext* init_option_context ();

    void do_init ()
    {
        WorkbenchStaticInit::do_init ();
    }
    bool on_delete_event (GdkEventAny* event);
    bool query_for_shutdown ();

public:
    Workbench (DynamicModule *a_dynmod);
    virtual ~Workbench ();
    void load_perspectives ();
    bool do_init (Gtk::Main &a_main, int a_argc, char **a_argv);
    void do_init (IConfMgrSafePtr &);
    void select_perspective (IPerspectiveSafePtr &a_perspective);
    void shut_down ();
    Glib::RefPtr<Gtk::ActionGroup> get_default_action_group ();
    Glib::RefPtr<Gtk::ActionGroup> get_debugger_ready_action_group ();
    Gtk::Widget& get_menubar ();
    Gtk::Notebook& get_toolbar_container ();
    Gtk::Window& get_root_window ();
    void set_title_extension (const UString &a_str);
    Glib::RefPtr<Gtk::UIManager>& get_ui_manager () ;
    IPerspectiveSafePtr get_perspective (const UString &a_name);
    const std::list<IPerspectiveSafePtr>& perspectives () const;
    void set_configuration_manager (IConfMgrSafePtr &);
    IConfMgrSafePtr get_configuration_manager () ;
    Glib::RefPtr<Glib::MainContext> get_main_context () ;
    sigc::signal<void>& shutting_down_signal ();
};//end class Workbench

struct Workbench::Priv {
    bool initialized;
    Gtk::Main *main;
    Glib::RefPtr<Gtk::ActionGroup> default_action_group;
    Glib::RefPtr<Gtk::UIManager> ui_manager;
    Glib::RefPtr<Gtk::Builder> builder;
    SafePtr <Gtk::Window> root_window;
    Gtk::Widget *menubar;
    Gtk::Notebook *toolbar_container;
    Gtk::Notebook *bodies_container;
    Gtk::ComboBoxText *persp_selector_combobox;
    PluginManagerSafePtr plugin_manager;
    list<IPerspectiveSafePtr> perspectives;
    map<IPerspective*, int> toolbars_index_map;
    map<IPerspective*, int> bodies_index_map;
    map<UString, UString> properties;
    IConfMgrSafePtr conf_mgr;
    sigc::signal<void> shutting_down_signal;
    UString base_title;
    std::list<Gtk::UIManager::ui_merge_id> perspective_menubar_merge_ids;

    Priv () :
        initialized (false),
        main (0),
        root_window (0),
        menubar (0),
        toolbar_container (0),
        bodies_container (0)
    {
    }

};//end Workbench::Priv

#ifndef CHECK_WB_INIT
#define CHECK_WB_INIT THROW_IF_FAIL(m_priv->initialized);
#endif

//****************
//private methods
//****************
bool
Workbench::query_for_shutdown ()
{
    bool retval = true;
    list<IPerspectiveSafePtr>::const_iterator iter;
    for (iter = m_priv->perspectives.begin ();
         iter != m_priv->perspectives.end ();
         ++iter) {
         if ((*iter)->agree_to_shutdown () == false) {
             retval = false;
             break;
         }
    }
    return retval;
}


//*********************
//signal slots methods
//*********************
void
Workbench::on_perspective_changed ()
{
    NEMIVER_TRY;

    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->persp_selector_combobox);

    UString name = m_priv->persp_selector_combobox->get_active_text ();
    std::list<IPerspectiveSafePtr>::iterator iter;
    for (iter = m_priv->perspectives.begin ();
         iter != m_priv->perspectives.end ();
         ++iter) {
        THROW_IF_FAIL ((*iter)->descriptor ());
        if ((*iter)->descriptor ()->name () == name) {
            select_perspective (*iter);
            return;
        }
    }

    NEMIVER_CATCH;
}

bool
Workbench::on_delete_event (GdkEventAny* a_event)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    bool retval = true;

    NEMIVER_TRY
    // use event so that compilation doesn't fail with -Werror :(
    if (a_event) {}

    // clicking the window manager's X and shutting down the with Quit menu item
    // should do the same thing
    if (query_for_shutdown () == true) {
        shut_down ();
        retval = false;
    }

    NEMIVER_CATCH

    //Will propagate if retval = false, else not
    return retval;
}

void
Workbench::on_quit_menu_item_action ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    NEMIVER_TRY

    if (query_for_shutdown () == true) {
        shut_down ();
    }

    NEMIVER_CATCH
}

void
Workbench::on_contents_menu_item_action ()
{
    NEMIVER_TRY

    UString help_url = "ghelp:nemiver";
    LOG_DD ("launching help url: " << help_url);
    UString path_to_help =
        nemiver::common::env::build_path_to_help_file ("nemiver.xml");
    THROW_IF_FAIL (!path_to_help.empty ());
    UString cmd_line ("yelp " + path_to_help);
    LOG_DD ("going to spawn: " << cmd_line);
    bool is_ok = g_spawn_command_line_async (Glib::locale_from_utf8
                                             (cmd_line).c_str (), 0);
    if (!is_ok) {
        LOG_ERROR ("failed to spawn " << is_ok);
    }
    NEMIVER_CATCH
}

void
Workbench::on_about_menu_item_action ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    NEMIVER_TRY

    Gtk::AboutDialog dialog;
    dialog.set_name (PACKAGE_NAME);
    dialog.set_version (PACKAGE_VERSION);
    dialog.set_comments(_("A C/C++ debugger for GNOME"));

    vector<Glib::ustring> authors;
    authors.push_back ("Dodji Seketeli <dodji@gnome.org>");
    authors.push_back ("Jonathon Jongsma <jjongsma@gnome.org>");
    dialog.set_authors (authors);

    vector<Glib::ustring> documenters;
    documenters.push_back ("Jonathon Jongsma <jjongsma@gnome.org>");
    dialog.set_documenters (documenters);

    dialog.set_website (PACKAGE_URL);
    dialog.set_website_label (_("Project Website"));

    Glib::ustring license =
        "This program is free software; you can redistribute it and/or modify\n"
        "it under the terms of the GNU General Public License as published by\n"
        "the Free Software Foundation; either version 2 of the License, or\n"
        "(at your option) any later version.\n\n"

        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "GNU General Public License for more details.\n\n"

        "You should have received a copy of the GNU General Public License\n"
        "along with this program; if not, write to the \n"
        "Free Software Foundation, Inc., 59 Temple Place, Suite 330, \n"
        "Boston, MA  02111-1307  USA\n";
    dialog.set_license (license);

    // Translators: change this to your name, separate multiple names with \n
    dialog.set_translator_credits (_("translator-credits"));

    Glib::RefPtr<Gtk::IconTheme> theme = Gtk::IconTheme::get_default ();
    if (theme->has_icon ("nemiver")) {
        Glib::RefPtr<Gdk::Pixbuf> icon =
        theme->load_icon ("nemiver", 128,
                          Gtk::ICON_LOOKUP_USE_BUILTIN);
        dialog.set_logo (icon);
    }

    vector<Glib::ustring> artists;
    artists.push_back ("Steven Brown <swjb@interchange.ubc.ca>");
    artists.push_back ("Andreas Nilsson <andreas@andreasn.se>");
    dialog.set_artists (artists);

    dialog.run ();

    NEMIVER_CATCH
}

void
Workbench::on_shutting_down_signal ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    NEMIVER_TRY
    save_window_geometry ();
    NEMIVER_CATCH
}

Workbench::Workbench (DynamicModule *a_dynmod) :
    IWorkbench (a_dynmod)
{
    m_priv.reset (new Priv ());
}

Workbench::~Workbench ()
{
    remove_all_perspective_bodies ();
    disconnect_all_perspective_signals ();
    LOG_D ("delete", "destructor-domain");
}

/// Initialize the workbench by setting the configuration manager it
/// is going to use.  This function is usually called by the helper
/// function template load_iface_and_confmgr.
///
/// \param a_conf_mgr the configuration manager to set.
void
Workbench::do_init (IConfMgrSafePtr &a_conf_mgr)
{
    set_configuration_manager (a_conf_mgr);
}

void
Workbench::load_perspectives ()
{
    THROW_IF_FAIL (m_priv);

    DynamicModule::Loader *loader =
        get_dynamic_module ().get_module_loader ();
    THROW_IF_FAIL (loader);

    DynamicModuleManager *dynmod_manager =
        loader->get_dynamic_module_manager ();
    THROW_IF_FAIL (dynmod_manager);

    m_priv->plugin_manager =
        PluginManagerSafePtr (new PluginManager (*dynmod_manager));
    THROW_IF_FAIL (m_priv->plugin_manager);

    NEMIVER_TRY;

    std::map<UString, PluginSafePtr>::const_iterator plugin_iter;
    Plugin::EntryPointSafePtr entry_point;
    IPerspectiveSafePtr perspective;

    m_priv->plugin_manager->load_plugins ();

    //**************************************************************
    //store the list of perspectives we may have loaded as plugins,
    //and init each of them.
    //**************************************************************
    for (plugin_iter = m_priv->plugin_manager->plugins_map ().begin ();
         plugin_iter != m_priv->plugin_manager->plugins_map ().end ();
         ++plugin_iter) {
         LOG_D ("plugin '"
                << plugin_iter->second->descriptor ()->name ()
                << "' refcount: "
                << (int) plugin_iter->second->get_refcount (),
                "refcount-domain");
        if (plugin_iter->second && plugin_iter->second->entry_point_ptr ()) {
            entry_point = plugin_iter->second->entry_point_ptr ();
            perspective = entry_point.do_dynamic_cast<IPerspective> ();
            if (perspective) {
                m_priv->perspectives.push_front (perspective);
                LOG_D ("perspective '"
                       << perspective->get_perspective_identifier ()
                       << "' refcount: "
                       << (int) perspective->get_refcount (),
                       "refcount-domain");
            }
        }
    }

    NEMIVER_CATCH;
}

GOptionContext*
Workbench::init_option_context ()
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

    std::list<IPerspectiveSafePtr>::const_iterator perspective;
    const std::list<IPerspectiveSafePtr> &perps = perspectives ();
    for (perspective = perps.begin ();
         perspective != perps.end ();
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
bool
Workbench::parse_command_line (int &a_argc, char** a_argv)
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

    IPerspectiveSafePtr perspective = get_perspective (gv_tool);
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
bool
Workbench::process_non_gui_options ()
{
    if (gv_show_version) {
        std::cout << PACKAGE_VERSION << endl;
        return false;
    }

    if (gv_show_tool_list) {
        const std::list<IPerspectiveSafePtr> &persp = perspectives ();
        std::list<IPerspectiveSafePtr>::const_iterator perspective;
        std::cout << "Tools available: \n";

        for (perspective = persp.begin ();
             perspective != persp.end ();
             ++perspective) {
            THROW_IF_FAIL (*perspective);
            THROW_IF_FAIL ((*perspective)->descriptor ());
            std::cout << "\t- " << (*perspective)->descriptor ()->name ()
                      << std::endl;
        }

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

/// Initialize the workbench by doing all the graphical plumbling
/// needed to setup the perspectives held by this workbench.  Calling
/// this function is mandatory prior to using the workbench.
///
/// \param a_main the Gtk main object the workbench is going to use.
bool
Workbench::do_init (Gtk::Main &a_main, int a_argc, char **a_argv)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    load_perspectives ();

    if (!parse_command_line (a_argc, a_argv))
        return false;

    if (!process_non_gui_options ()) {
        return false;
    }

    m_priv->main = &a_main;

    // set the icon that will be used by all workbench windows that don't
    // explicitly call set_icon() (must be set before the root window is
    // constructed)
    Glib::RefPtr<Gtk::IconTheme> theme = Gtk::IconTheme::get_default();
    if (theme->has_icon("nemiver")) {
        Glib::RefPtr<Gdk::Pixbuf> icon16 = theme->load_icon("nemiver", 16,
                Gtk::ICON_LOOKUP_USE_BUILTIN);
        Glib::RefPtr<Gdk::Pixbuf> icon32 = theme->load_icon("nemiver", 32,
                Gtk::ICON_LOOKUP_USE_BUILTIN);
        Glib::RefPtr<Gdk::Pixbuf> icon48 = theme->load_icon("nemiver", 48,
                Gtk::ICON_LOOKUP_USE_BUILTIN);

        std::vector<Glib::RefPtr<Gdk::Pixbuf> > icon_list;
        icon_list.push_back(icon16);
        icon_list.push_back(icon32);
        icon_list.push_back(icon48);
        m_priv->root_window->set_default_icon_list(icon_list);
    }

    init_builder ();
    init_window ();
    init_actions ();
    init_menubar ();
    init_toolbar ();
    init_body ();

    m_priv->initialized = true;

    NEMIVER_TRY
        IPerspectiveSafePtr perspective;
        list<Gtk::Widget*> toolbars;

        //**************************************************************
        //store the list of perspectives we may have loaded as plugins,
        //and init each of them.
        //**************************************************************
        IWorkbench *workbench = dynamic_cast<IWorkbench*> (this);
        std::list<IPerspectiveSafePtr>::iterator iter;
        for (iter = m_priv->perspectives.begin ();
             iter != m_priv->perspectives.end ();
             ++iter) {
            perspective = *iter;
            if (perspective) {
                perspective->do_init (workbench);
                perspective->layout_changed_signal ().connect
                    (sigc::bind<IPerspectiveSafePtr> (sigc::mem_fun
                        (*this,
                        &Workbench::on_perspective_layout_changed_signal),
                    perspective));
                toolbars.clear ();
                perspective->get_toolbars (toolbars);
                add_perspective_to_perspective_selector (perspective);
                add_perspective_toolbars (perspective, toolbars);
                add_perspective_body (perspective, perspective->get_body ());
            }
        }

        if (!m_priv->perspectives.empty ()) {
            perspective = *m_priv->perspectives.begin ();
            THROW_IF_FAIL (perspective);
            THROW_IF_FAIL (perspective->descriptor ());
            m_priv->persp_selector_combobox->set_active_text
                (perspective->descriptor ()->name ());
        }

        perspective = get_perspective (gv_tool);
        if (!perspective) {
            std::cerr << "Invalid tool name: " << gv_tool << std::endl;
            return false;
        }

        if (!perspective->process_gui_options  (a_argc, a_argv)) {
            return false;
        }

        select_perspective (perspective);
    NEMIVER_CATCH

    return true;
}

void
Workbench::shut_down ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    shutting_down_signal ().emit ();
    m_priv->main->quit ();
}

Glib::RefPtr<Gtk::ActionGroup>
Workbench::get_default_action_group ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    CHECK_WB_INIT;
    THROW_IF_FAIL (m_priv);
    return m_priv->default_action_group;
}

Gtk::Widget&
Workbench::get_menubar ()
{
    CHECK_WB_INIT;
    THROW_IF_FAIL (m_priv && m_priv->menubar);
    return *m_priv->menubar;
}

Gtk::Notebook&
Workbench::get_toolbar_container ()
{
    CHECK_WB_INIT;
    THROW_IF_FAIL (m_priv && m_priv->toolbar_container);
    return *m_priv->toolbar_container;
}

Gtk::Window&
Workbench::get_root_window ()
{
    CHECK_WB_INIT;
    THROW_IF_FAIL (m_priv && m_priv->root_window);

    return *m_priv->root_window;
}

void
Workbench::set_title_extension (const UString &a_str)
{
    if (a_str.empty ()) {
        get_root_window ().set_title (m_priv->base_title);
    } else {
        get_root_window ().set_title (a_str + " - " + m_priv->base_title);
    }
}

Glib::RefPtr<Gtk::UIManager>&
Workbench::get_ui_manager ()
{
    THROW_IF_FAIL (m_priv);
    if (!m_priv->ui_manager) {
        m_priv->ui_manager = Gtk::UIManager::create ();
        THROW_IF_FAIL (m_priv->ui_manager);
    }
    return m_priv->ui_manager;
}

const std::list<IPerspectiveSafePtr>&
Workbench::perspectives () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->perspectives;
}

IPerspectiveSafePtr
Workbench::get_perspective (const UString &a_name)
{
    list<IPerspectiveSafePtr>::const_iterator iter;
    for (iter = m_priv->perspectives.begin ();
         iter != m_priv->perspectives.end ();
         ++iter) {
        if ((*iter)->descriptor ()->name () == a_name) {
            return *iter;
        }
    }

    return IPerspectiveSafePtr ();
}

/// Set the configuration manager
void
Workbench::set_configuration_manager (IConfMgrSafePtr &a_conf_mgr)
{
    m_priv->conf_mgr = a_conf_mgr;

    NEMIVER_TRY;

    m_priv->conf_mgr->register_namespace
        (/*default nemiver namespace*/);
    m_priv->conf_mgr->register_namespace
        (CONF_NAMESPACE_DESKTOP_INTERFACE);

    NEMIVER_CATCH;
}

IConfMgrSafePtr
Workbench::get_configuration_manager ()
{
    THROW_IF_FAIL (m_priv);
    if (!m_priv->conf_mgr) {
        IConfMgrSafePtr new_conf_mgr = 
            DynamicModuleManager::load_iface_with_default_manager<IConfMgr>
            (CONFIG_MGR_MODULE_NAME, "IConfMgr");
        set_configuration_manager (new_conf_mgr);
    }
    THROW_IF_FAIL (m_priv->conf_mgr);
    return m_priv->conf_mgr;
}

Glib::RefPtr<Glib::MainContext>
Workbench::get_main_context ()
{
    THROW_IF_FAIL (m_priv);
    return Glib::MainContext::get_default ();
}

sigc::signal<void>&
Workbench::shutting_down_signal ()
{
    THROW_IF_FAIL (m_priv);

    return m_priv->shutting_down_signal;
}

void
Workbench::init_builder ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    THROW_IF_FAIL (m_priv);

    UString file_path = env::build_path_to_gtkbuilder_file ("workbench.ui");
    m_priv->builder = Gtk::Builder::create_from_file (file_path);
    THROW_IF_FAIL (m_priv->builder);

    Gtk::Widget *w =
        ui_utils::get_widget_from_gtkbuilder<Gtk::Window> (m_priv->builder,
                                                      "workbench");
    THROW_IF_FAIL (w);
    m_priv->root_window.reset (dynamic_cast<Gtk::Window*>
                                                (w->get_toplevel ()));
    THROW_IF_FAIL (m_priv->root_window);
    //get the title of the toplevel window as specified in the
    //gtkbuilder file and save
    //it so that later we can add state-specific
    //extensions to this base title
    //if needed
    m_priv->base_title = m_priv->root_window->get_title ();
}
void

Workbench::init_window ()
{
   LOG_FUNCTION_SCOPE_NORMAL_DD;

    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->root_window);
    IConfMgrSafePtr conf_mgr = get_configuration_manager ();
    THROW_IF_FAIL (conf_mgr);

    int width = 700, height = 500, pos_x = 0, pos_y = 0;
    bool maximized=false;

    LOG_DD ("getting windows geometry from confmgr ...");

    NEMIVER_TRY
    conf_mgr->get_key_value (CONF_KEY_NEMIVER_WINDOW_WIDTH, width);
    conf_mgr->get_key_value (CONF_KEY_NEMIVER_WINDOW_HEIGHT, height);
    conf_mgr->get_key_value (CONF_KEY_NEMIVER_WINDOW_POSITION_X, pos_x);
    conf_mgr->get_key_value (CONF_KEY_NEMIVER_WINDOW_POSITION_Y, pos_y);
    conf_mgr->get_key_value (CONF_KEY_NEMIVER_WINDOW_MAXIMIZED, maximized);
    LOG_DD ("got windows geometry from confmgr.");
    NEMIVER_CATCH_NOX

    if (width) {
        LOG_DD ("restoring windows geometry from confmgr ...");
        m_priv->root_window->resize (width, height);
        m_priv->root_window->move (pos_x, pos_y);
        if (maximized) {
            m_priv->root_window->maximize ();
        }
        LOG_DD ("restored windows geometry from confmgr");
    } else {
        LOG_DD ("null window geometry from confmgr.");
    }

    // Allow the user to set the minimum width/height of nemiver.
    width = 0, height = 0;

    NEMIVER_TRY
    conf_mgr->get_key_value (CONF_KEY_NEMIVER_WINDOW_MINIMUM_WIDTH, width);
    conf_mgr->get_key_value (CONF_KEY_NEMIVER_WINDOW_MINIMUM_HEIGHT, height);
    NEMIVER_CATCH

    m_priv->root_window->set_size_request (width, height);
    LOG_DD ("set windows min size to ("
            << (int) width
            << ","
            << (int) height
            << ")");

    shutting_down_signal ().connect (sigc::mem_fun
                        (*this, &Workbench::on_shutting_down_signal));
    m_priv->root_window->signal_delete_event ().connect (
            sigc::mem_fun (*this, &Workbench::on_delete_event));
}

void
Workbench::init_actions ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    Gtk::StockID nil_stock_id ("");
    sigc::slot<void> nil_slot;
    using ui_utils::ActionEntry;

    static ActionEntry s_default_action_entries [] = {
        {
            "FileMenuAction",
            nil_stock_id,
            _("_File"),
            "",
            nil_slot,
            ActionEntry::DEFAULT,
            "",
            false
        }
        ,
        {
            "QuitMenuItemAction",
            Gtk::Stock::QUIT,
            _("_Quit"),
            _("Quit the application"),
            sigc::mem_fun (*this, &Workbench::on_quit_menu_item_action),
            ActionEntry::DEFAULT,
            "",
            false
        }
        ,
        {
            "EditMenuAction",
            nil_stock_id,
            _("_Edit"),
            "",
            nil_slot,
            ActionEntry::DEFAULT,
            "",
            false
        }
        ,
        {
            "HelpMenuAction",
            nil_stock_id,
            _("_Help"),
            "",
            nil_slot,
            ActionEntry::DEFAULT,
            "",
            false
        }
        ,
        {
            "AboutMenuItemAction",
            Gtk::Stock::ABOUT,
            _("_About"),
            _("Display information about this application"),
            sigc::mem_fun (*this, &Workbench::on_about_menu_item_action),
            ActionEntry::DEFAULT,
            "",
            false
        }
        ,
        {
            "ContentsMenuItemAction",
            Gtk::Stock::HELP,
            _("_Contents"),
            _("Display the user manual for this application"),
            sigc::mem_fun (*this, &Workbench::on_contents_menu_item_action),
            ActionEntry::DEFAULT,
            "F1",
            false
        }
    };

    m_priv->default_action_group =
        Gtk::ActionGroup::create ("workbench-default-action-group");
    int num_default_actions =
         sizeof (s_default_action_entries)/sizeof (ui_utils::ActionEntry);
    ui_utils::add_action_entries_to_action_group (s_default_action_entries,
                                                  num_default_actions,
                                                  m_priv->default_action_group);
    get_ui_manager ()->insert_action_group (m_priv->default_action_group);
}

void
Workbench::init_menubar ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    THROW_IF_FAIL (m_priv && m_priv->default_action_group);


    UString file_path = env::build_path_to_menu_file ("menubar.xml");
    m_priv->ui_manager->add_ui_from_file (file_path);

    m_priv->menubar = m_priv->ui_manager->get_widget ("/MenuBar");
    THROW_IF_FAIL (m_priv->menubar);

    Gtk::Box *menu_container =
        ui_utils::get_widget_from_gtkbuilder<Gtk::Box> (m_priv->builder, "menucontainer");
    menu_container->pack_start (*m_priv->menubar);
    menu_container->show_all ();
}

void
Workbench::init_toolbar ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    m_priv->toolbar_container =
        ui_utils::get_widget_from_gtkbuilder<Gtk::Notebook> (m_priv->builder,
                                                        "toolbarcontainer");

    m_priv->persp_selector_combobox =
        ui_utils::get_widget_from_gtkbuilder<Gtk::ComboBoxText>
            (m_priv->builder, "perspectiveselector");
    m_priv->persp_selector_combobox->set_entry_text_column (0);
    m_priv->persp_selector_combobox->signal_changed ().connect (sigc::mem_fun
        (*this, &Workbench::on_perspective_changed));
}

void
Workbench::init_body ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    m_priv->bodies_container =
        ui_utils::get_widget_from_gtkbuilder<Gtk::Notebook> (m_priv->builder,
                                                        "bodynotebook");
}

void
Workbench::add_perspective_toolbars (IPerspectiveSafePtr &a_perspective,
                                     list<Gtk::Widget*> &a_tbs)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    if (a_tbs.empty ()) {return;}

    SafePtr<Gtk::Box> box (Gtk::manage (new Gtk::VBox));
    list<Gtk::Widget*>::const_iterator iter;

    for (iter = a_tbs.begin (); iter != a_tbs.end (); ++iter) {
        box->pack_start (**iter);
    }

    box->show_all ();
    m_priv->toolbars_index_map [a_perspective.get ()] =
                    m_priv->toolbar_container->insert_page (*box, -1);

    box.release ();
}

void
Workbench::add_perspective_to_perspective_selector
    (IPerspectiveSafePtr &a_perspective)
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (a_perspective);
    THROW_IF_FAIL (m_priv->persp_selector_combobox);
    THROW_IF_FAIL (a_perspective->descriptor ());

    m_priv->persp_selector_combobox->append
        (a_perspective->descriptor ()->name ());
}

void
Workbench::add_perspective_body (IPerspectiveSafePtr &a_perspective,
                                 Gtk::Widget *a_body)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    if (!a_body || !a_perspective) {return;}

    m_priv->bodies_index_map[a_perspective.get ()] =
        m_priv->bodies_container->insert_page (*a_body, -1);
}

void
Workbench::on_perspective_layout_changed_signal
        (IPerspectiveSafePtr a_perspective)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->bodies_container);

    if (!a_perspective) {return;}

    int page = m_priv->bodies_index_map[a_perspective.get ()];

    m_priv->bodies_container->remove_page (page);
    m_priv->bodies_container->insert_page (*a_perspective->get_body (), page);

    select_perspective (a_perspective);
}

bool
Workbench::remove_perspective_body (IPerspectiveSafePtr &a_perspective)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->bodies_container);

    if (!a_perspective) {return false;}

    map<IPerspective*, int>::iterator it;
    it = m_priv->bodies_index_map.find (a_perspective.get ());
    if (it == m_priv->bodies_index_map.end ()) {
        return false;
    }
    m_priv->bodies_container->remove_page (it->second) ;
    m_priv->bodies_index_map.erase (it);
    return true;
}

void
Workbench::save_window_geometry () 
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->root_window);
    IConfMgrSafePtr conf_mgr = get_configuration_manager ();
    THROW_IF_FAIL (conf_mgr);

    int width=0, height=0, pos_x=0, pos_y=0;
    m_priv->root_window->get_size (width, height);
    m_priv->root_window->get_position (pos_x, pos_y);
    bool maximized = (m_priv->root_window->get_window()->get_state()
                      & Gdk::WINDOW_STATE_MAXIMIZED);

    NEMIVER_TRY
    conf_mgr->set_key_value (CONF_KEY_NEMIVER_WINDOW_MAXIMIZED, maximized);

    if (!maximized) {
        LOG_DD ("storing windows geometry to confmgr...");
        conf_mgr->set_key_value (CONF_KEY_NEMIVER_WINDOW_WIDTH, width);
        conf_mgr->set_key_value (CONF_KEY_NEMIVER_WINDOW_HEIGHT, height);
        conf_mgr->set_key_value (CONF_KEY_NEMIVER_WINDOW_POSITION_X, pos_x);
        conf_mgr->set_key_value (CONF_KEY_NEMIVER_WINDOW_POSITION_Y, pos_y);
        LOG_DD ("windows geometry stored to confmgr");
    } else {
        LOG_DD ("windows was maximized, didn't store its geometry");
    }
    NEMIVER_CATCH_NOX
}

void
Workbench::remove_all_perspective_bodies ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    map<IPerspective*, int>::iterator it;
    for (it = m_priv->bodies_index_map.begin ();
         it != m_priv->bodies_index_map.end ();
         ++it) {
        m_priv->bodies_container->remove_page (it->second);
    }
    m_priv->bodies_index_map.clear ();
}

void
Workbench::disconnect_all_perspective_signals ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    list<IPerspectiveSafePtr>::iterator it;
    for (it = m_priv->perspectives.begin ();
         it != m_priv->perspectives.end ();
         ++it) {
        (*it)->layout_changed_signal ().clear ();
    }
}

void
Workbench::select_perspective (IPerspectiveSafePtr &a_perspective)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;

    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->toolbar_container);
    THROW_IF_FAIL (m_priv->bodies_container);
    THROW_IF_FAIL (a_perspective);
    THROW_IF_FAIL (a_perspective->descriptor ());

    map<IPerspective*, int>::const_iterator iter, nil;
    int toolbar_index=0, body_index=0;

    nil = m_priv->toolbars_index_map.end ();
    iter = m_priv->toolbars_index_map.find (a_perspective.get ());
    if (iter != nil) {
        toolbar_index = iter->second;
    }

    nil = m_priv->bodies_index_map.end ();
    iter = m_priv->bodies_index_map.find (a_perspective.get ());
    if (iter != nil) {
        body_index = iter->second;
    }

    std::list<Gtk::UIManager::ui_merge_id>::iterator merge_id;
    for (merge_id = m_priv->perspective_menubar_merge_ids.begin ();
         merge_id != m_priv->perspective_menubar_merge_ids.end ();
         ++merge_id) {
        m_priv->ui_manager->remove_ui (*merge_id);
    }
    m_priv->perspective_menubar_merge_ids =
        a_perspective->edit_workbench_menu ();

    m_priv->toolbar_container->set_current_page (toolbar_index);

    m_priv->bodies_container->set_current_page (body_index);

    UString name = m_priv->persp_selector_combobox->get_active_text ();
    if (a_perspective->descriptor ()->name () != name) {
        m_priv->persp_selector_combobox->set_active_text
            (a_perspective->descriptor ()->name ());
    }
}

class WorkbenchModule : public DynamicModule {
public:

    void do_init ()
    {
        WorkbenchStaticInit::do_init ();
    }

    void get_info (Info &a_info) const
    {
        static Info s_info ("workbench",
                            "The workbench of Nemiver",
                            "1.0");
        a_info = s_info;
    }

    bool lookup_interface (const std::string &a_iface_name,
                           DynModIfaceSafePtr &a_iface)
    {
        if (a_iface_name == "IWorkbench") {
            a_iface.reset (new Workbench (this));
        } else {
            return false;
        }
        return true;
    }
};//end class WorkbenchModule

NEMIVER_END_NAMESPACE (nemiver)

//the dynmod initial factory.
extern "C" {
bool
NEMIVER_API nemiver_common_create_dynamic_module_instance (void **a_new_instance)
{
    *a_new_instance = new nemiver::WorkbenchModule ();
    return (*a_new_instance != 0);
}

}//end extern C


