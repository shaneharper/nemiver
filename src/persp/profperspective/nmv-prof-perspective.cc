//Author: Fabien Parent
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

#include "nmv-prof-perspective.h"
#include "nmv-ui-utils.h"
#include "nmv-load-report-dialog.h"
#include "nmv-call-list.h"
#include "nmv-spinner-tool-item.h"
#include "nmv-record-dialog.h"
#include "nmv-i-profiler.h"
#include "common/nmv-safe-ptr-utils.h"
#include "common/nmv-str-utils.h"
#include "uicommon/nmv-source-editor.h"

#include <list>
#include <glib/gi18n.h>
#include <gtkmm/widget.h>

NEMIVER_BEGIN_NAMESPACE (nemiver)

using common::DynamicModuleManager;
using common::GCharSafePtr;

static gchar *gv_report_path = 0;

static const GOptionEntry entries[] =
{
    {
        "load-report",
        0,
        0,
        G_OPTION_ARG_STRING,
        &gv_report_path,
        _("Load a report file"),
        "</path/to/report/file>"
    },
    {0, 0, 0, (GOptionArg) 0, 0, 0, 0}
};

class ProfPerspective : public IProfPerspective {
    //non copyable
    ProfPerspective (const IProfPerspective&);
    ProfPerspective& operator= (const IProfPerspective&);

    IProfilerSafePtr prof;
    SafePtr<CallList> call_list;
    SafePtr<SpinnerToolItem> throbber;
    SafePtr<Gtk::HBox> toolbar;

    std::map<UString, int> symbol_to_pagenum_map;
    Glib::RefPtr<Gtk::ActionGroup> default_action_group;
    Gtk::Notebook body;
    IWorkbench *workbench;
    GOptionGroup *opt_group;

    sigc::signal<void, bool> signal_activated;
    sigc::signal<void> signal_layout_changed;

public:

    ProfPerspective (DynamicModule *a_dynmod);
    GOptionGroup* option_group () const;
    bool process_options (GOptionContext *a_context,
                          int a_argc,
                          char **a_argv);
    bool process_gui_options (int a_argc, char **a_argv);
    const UString& name () const;
    void do_init (IWorkbench *a_workbench);
    const UString& get_perspective_identifier ();
    void get_toolbars (std::list<Gtk::Widget*> &a_tbs);
    Gtk::Widget* get_body ();
    IWorkbench& get_workbench ();
    std::list<Gtk::UIManager::ui_merge_id> edit_workbench_menu ();
    Gtk::Widget* load_menu (const UString &a_filename,
                            const UString &a_widget_name);
    bool agree_to_shutdown ();
    void init_actions ();
    void init_signals ();
    void init_toolbar ();
    void init_body ();
    void load_report_file ();
    void load_report_file (const UString &a_report_file);
    void run_executable ();
    void run_executable (const UString &a_program_name,
                         const UString &a_arguments,
                         bool a_scale_counter_values,
                         bool a_do_callgraph,
                         bool a_child_inherit_counters);
    void annotate_symbol (const UString &a_symbol_name);
    void close_symbol_annotation (UString a_symbol_name);
    void load_toolbar ();

    void stop_recording ();
    void on_stop_recording_action ();
    void on_run_executable_action ();
    void on_load_report_file_action ();
    void on_report_done_signal (CallGraphSafePtr a_call_graph);
    void on_record_done_signal (const UString &a_report_file);
    void on_symbol_annotated_signal (const UString &a_symbol_name,
                                     const UString &a_annotation);

    IProfilerSafePtr& profiler ();

    sigc::signal<void, bool>& activated_signal ();
    sigc::signal<void>& layout_changed_signal ();
}; // end class ProfPerspective

ProfPerspective::ProfPerspective (DynamicModule *a_dynmod) :
    IProfPerspective (a_dynmod),
    workbench (0),
    opt_group (0)
{
    opt_group = g_option_group_new
        ("profiler", _("Profiler"), _("Show profiler options"), 0, 0);
    g_option_group_add_entries (opt_group, entries);
}

GOptionGroup*
ProfPerspective::option_group () const
{
    return opt_group;
}

bool
ProfPerspective::process_options (GOptionContext *a_context,
                                  int a_argc,
                                  char **/*a_argv*/)
{
    if (a_argc && gv_report_path) {
        std::cerr << _("You cannot provide a report file and a binary at "
                       "the same time.\n");
        GCharSafePtr help_message;
        help_message.reset (g_option_context_get_help
            (a_context, true, opt_group));
        std::cerr << help_message.get () << std::endl;
        return false;
    }

    return true;
}

bool
ProfPerspective::process_gui_options (int /*a_argc*/, char **/*a_argv*/)
{
    NEMIVER_TRY

    if (gv_report_path) {
        load_report_file (gv_report_path);
    }

    NEMIVER_CATCH

    return true;
}

IProfilerSafePtr&
ProfPerspective::profiler ()
{
    if (!prof) {
        DynamicModule::Loader *loader =
            get_workbench ().get_dynamic_module ().get_module_loader ();
        THROW_IF_FAIL (loader);

        DynamicModuleManager *module_manager =
                            loader->get_dynamic_module_manager ();
        THROW_IF_FAIL (module_manager);

        UString debugger_dynmod_name;

        if (debugger_dynmod_name == "") {
            debugger_dynmod_name = "perfengine";
        }
        LOG_DD ("using debugger_dynmod_name: '"
                << debugger_dynmod_name << "'");
        prof = module_manager->load_iface<IProfiler>
            (debugger_dynmod_name, "IProfiler");
    }

    THROW_IF_FAIL (prof);
    return prof;
}

const UString&
ProfPerspective::name () const
{
    static const UString &s_name = "Profiler";
    return s_name;
}

void
ProfPerspective::do_init (IWorkbench *a_workbench)
{
    workbench = a_workbench;
    init_actions ();
    load_toolbar ();
    init_signals ();
    init_toolbar ();
    init_body ();
}

const UString&
ProfPerspective::get_perspective_identifier ()
{
    static UString s_id = "org.nemiver.ProfilerPerspective";
    return s_id;
}

void
ProfPerspective::get_toolbars (std::list<Gtk::Widget*> &a_tbs)
{
    a_tbs.push_back (toolbar.get ());
}

Gtk::Widget*
ProfPerspective::get_body ()
{
    return &body;
}

IWorkbench&
ProfPerspective::get_workbench ()
{
    THROW_IF_FAIL (workbench);
    return *workbench;
}

void
ProfPerspective::load_toolbar ()
{
    THROW_IF_FAIL (workbench);

    std::string relative_path = Glib::build_filename ("menus",
                                                      "toolbar.xml");
    string absolute_path;
    THROW_IF_FAIL (build_absolute_resource_path
        (Glib::filename_to_utf8 (relative_path), absolute_path));

    workbench->get_ui_manager ()->add_ui_from_file
        (Glib::filename_to_utf8 (absolute_path));
}


void
ProfPerspective::init_toolbar ()
{
    throbber.reset (new SpinnerToolItem);
    toolbar.reset ((new Gtk::HBox));
    THROW_IF_FAIL (toolbar);

    Gtk::Toolbar *glade_toolbar = dynamic_cast<Gtk::Toolbar*>
        (workbench->get_ui_manager ()->get_widget ("/ProfToolBar"));
    THROW_IF_FAIL (glade_toolbar);

    Glib::RefPtr<Gtk::StyleContext> style_context =
        glade_toolbar->get_style_context ();
    if (style_context) {
        style_context->add_class (GTK_STYLE_CLASS_PRIMARY_TOOLBAR);
    }

    Gtk::SeparatorToolItem *sep = Gtk::manage (new Gtk::SeparatorToolItem);
    sep->set_draw (false);
    sep->set_expand (true);
    glade_toolbar->insert (*sep, -1);
    glade_toolbar->insert (*throbber, -1);
    toolbar->pack_start (*glade_toolbar);
    toolbar->show ();
}

void
ProfPerspective::init_signals ()
{
    THROW_IF_FAIL (profiler ());
    profiler ()->report_done_signal ().connect (sigc::mem_fun
        (*this, &ProfPerspective::on_report_done_signal));

    profiler ()->record_done_signal ().connect (sigc::mem_fun
        (*this, &ProfPerspective::on_record_done_signal));

    profiler ()->symbol_annotated_signal ().connect (sigc::mem_fun
        (*this, &ProfPerspective::on_symbol_annotated_signal));
}

void
ProfPerspective::init_body ()
{
    call_list.reset (new CallList (*this));
    call_list->widget ().show ();
    body.append_page (call_list->widget (), _("Report"));
    body.show_all ();
}

void
ProfPerspective::close_symbol_annotation (UString a_symbol_name)
{
    NEMIVER_TRY;

    THROW_IF_FAIL (symbol_to_pagenum_map.count (a_symbol_name));

    int pagenum = symbol_to_pagenum_map[a_symbol_name];
    body.remove_page (pagenum);
    symbol_to_pagenum_map.erase (a_symbol_name);

    NEMIVER_CATCH;
}

void
ProfPerspective::on_symbol_annotated_signal (const UString &a_symbol_name,
                                             const UString &a_annotation)
{
    NEMIVER_TRY;

    Gtk::Label *label = Gtk::manage (new Gtk::Label (a_symbol_name));
    THROW_IF_FAIL (label);
    label->set_ellipsize (Pango::ELLIPSIZE_MIDDLE);
    label->set_width_chars (a_symbol_name.length ());
    label->set_max_width_chars (25);
    label->set_justify (Gtk::JUSTIFY_LEFT);

    Gtk::Image *close_icon = Gtk::manage (new Gtk::Image
        (Gtk::StockID (Gtk::Stock::CLOSE), Gtk::ICON_SIZE_MENU));

    static const std::string button_style =
        "* {\n"
          "-GtkButton-default-border : 0;\n"
          "-GtkButton-default-outside-border : 0;\n"
          "-GtkButton-inner-border: 0;\n"
          "-GtkWidget-focus-line-width : 0;\n"
          "-GtkWidget-focus-padding : 0;\n"
          "padding: 0;\n"
        "}";
    Glib::RefPtr<Gtk::CssProvider> css = Gtk::CssProvider::create ();
    css->load_from_data (button_style);

    int w = 0;
    int h = 0;
    Gtk::IconSize::lookup (Gtk::ICON_SIZE_MENU, w, h);

    Gtk::Button *close_button = Gtk::manage (new Gtk::Button ());
    close_button->set_size_request (w + 2, h + 2);
    close_button->set_relief (Gtk::RELIEF_NONE);
    close_button->set_focus_on_click (false);
    close_button->add (*close_icon);
    close_button->signal_clicked ().connect (sigc::bind<UString>
        (sigc::mem_fun (*this, &ProfPerspective::close_symbol_annotation),
         a_symbol_name));

    UString message;
    message.printf (_("Close %s"), a_symbol_name.c_str ());
    close_button->set_tooltip_text (message);

    Glib::RefPtr<Gtk::StyleContext> context =
        close_button->get_style_context ();
    context->add_provider (css, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    Gtk::HBox *hbox = Gtk::manage (new Gtk::HBox ());
    // add a bit of space between the label and the close button
    hbox->set_spacing (4);
    hbox->pack_start (*label);
    hbox->pack_start (*close_button, Gtk::PACK_SHRINK);
    hbox->show_all_children ();

    Glib::RefPtr<Gsv::Buffer> buffer = SourceEditor::create_source_buffer ();
    THROW_IF_FAIL (buffer);
    buffer->set_text (a_annotation);

    SourceEditor *editor = new SourceEditor (".", buffer);
    THROW_IF_FAIL (editor);

    int page = body.insert_page (*Gtk::manage (editor), *hbox, -1);
    body.set_current_page (page);
    body.show_all ();

    symbol_to_pagenum_map[a_symbol_name] = page;

    THROW_IF_FAIL (throbber);
    throbber->stop ();

    NEMIVER_CATCH;
}

void
ProfPerspective::on_report_done_signal (CallGraphSafePtr a_call_graph)
{
    NEMIVER_TRY

    call_list->load_call_graph (a_call_graph);

    THROW_IF_FAIL (throbber);
    throbber->stop ();

    NEMIVER_CATCH
}

void
ProfPerspective::on_record_done_signal (const UString &a_report_path)
{
    NEMIVER_TRY

    THROW_IF_FAIL (profiler ());
    profiler ()->report (a_report_path);

    NEMIVER_CATCH
}

void
ProfPerspective::init_actions ()
{
    Gtk::StockID nil_stock_id ("");
    sigc::slot<void> nil_slot;

    static ui_utils::ActionEntry s_default_action_entries [] = {
        {
            "RunExecutableAction",
            nil_stock_id,
            _("Run Executable..."),
            _("Execute a program under the profiler"),
            sigc::mem_fun (*this, &ProfPerspective::on_run_executable_action),
            ui_utils::ActionEntry::DEFAULT,
            "",
            false
        },
        {
            "LoadReportMenuItemAction",
            nil_stock_id,
            _("_Load Report File..."),
            _("Load a report file from disk"),
            sigc::mem_fun (*this, &ProfPerspective::on_load_report_file_action),
            ui_utils::ActionEntry::DEFAULT,
            "",
            false
        },
        {
            "StopProfilingMenuItemAction",
            Gtk::Stock::STOP,
            _("_Stop the profiling"),
            _("Stop the profiling"),
            sigc::mem_fun (*this, &ProfPerspective::on_stop_recording_action),
            ui_utils::ActionEntry::DEFAULT,
            "",
            false
        }
    };

    default_action_group =
        Gtk::ActionGroup::create ("profiler-default-action-group");
    default_action_group->set_sensitive (true);

    int num_actions =
        sizeof (s_default_action_entries) / sizeof (ui_utils::ActionEntry);

    ui_utils::add_action_entries_to_action_group
        (s_default_action_entries, num_actions, default_action_group);

    Glib::RefPtr<Gtk::UIManager> uimanager = get_workbench ().get_ui_manager ();
    THROW_IF_FAIL (uimanager);
    uimanager->insert_action_group (default_action_group);
}

std::list<Gtk::UIManager::ui_merge_id>
ProfPerspective::edit_workbench_menu ()
{
    std::string relative_path = Glib::build_filename ("menus", "menus.xml");
    std::string absolute_path;
    THROW_IF_FAIL (build_absolute_resource_path
        (Glib::filename_to_utf8 (relative_path), absolute_path));

    Glib::RefPtr<Gtk::UIManager> uimanager = get_workbench ().get_ui_manager ();
    THROW_IF_FAIL (uimanager);
    Gtk::UIManager::ui_merge_id menubar_merge_id =
        uimanager->add_ui_from_file (Glib::filename_to_utf8 (absolute_path));

    std::list<Gtk::UIManager::ui_merge_id> merge_ids;
    merge_ids.push_back (menubar_merge_id);
    return merge_ids;
}

void
ProfPerspective::run_executable ()
{
    RecordDialog dialog (plugin_path ());

    int result = dialog.run ();
    if (result != Gtk::RESPONSE_OK) {
        return;
    }

    run_executable (dialog.program_name (), dialog.arguments (),
                    dialog.scale_counter_values (), dialog.callgraph (),
                    dialog.child_inherit_counters ());
}

void
ProfPerspective::run_executable (const UString &a_program_name,
                                 const UString &a_arguments,
                                 bool a_scale_counter_values,
                                 bool a_do_callgraph,
                                 bool a_child_inherit_counters)
{
    std::vector<UString> argv = str_utils::split (a_arguments, " ");

    THROW_IF_FAIL (!a_program_name.empty ());
    THROW_IF_FAIL (profiler ());

    profiler ()->record (a_program_name, argv, a_scale_counter_values,
                         a_do_callgraph, a_child_inherit_counters);

    THROW_IF_FAIL (throbber);
    throbber->start ();
}

void
ProfPerspective::load_report_file (const UString &a_report_file)
{
    THROW_IF_FAIL (!a_report_file.empty ());
    THROW_IF_FAIL (profiler ());
    profiler ()->report (a_report_file);

    THROW_IF_FAIL (throbber);
    throbber->start ();
}

void
ProfPerspective::stop_recording ()
{
    THROW_IF_FAIL (profiler ());
    profiler ()->stop_recording ();
}

void
ProfPerspective::load_report_file ()
{
    LoadReportDialog dialog (plugin_path ());

    int result = dialog.run ();
    if (result != Gtk::RESPONSE_OK) {
        return;
    }

    UString report_file = dialog.report_file ();
    THROW_IF_FAIL (!report_file.empty ());
    load_report_file (report_file);
}

void
ProfPerspective::annotate_symbol (const UString &a_symbol_name)
{
    if (symbol_to_pagenum_map.count (a_symbol_name))
    {
        body.set_current_page (symbol_to_pagenum_map[a_symbol_name]);
    }
    else
    {
        THROW_IF_FAIL (profiler ());
        profiler ()->annotate_symbol (a_symbol_name);

        THROW_IF_FAIL (throbber);
        throbber->start ();
    }
}

void
ProfPerspective::on_stop_recording_action ()
{
    NEMIVER_TRY

    stop_recording ();

    NEMIVER_CATCH
}

void
ProfPerspective::on_load_report_file_action ()
{
    NEMIVER_TRY

    load_report_file ();

    NEMIVER_CATCH
}

void
ProfPerspective::on_run_executable_action ()
{
    NEMIVER_TRY

    run_executable ();

    NEMIVER_CATCH
}

Gtk::Widget*
ProfPerspective::load_menu (const UString &/*a_filename*/, const UString &/*a_widget_name*/)
{
    return 0;
}

bool
ProfPerspective::agree_to_shutdown ()
{
    return true;
}

sigc::signal<void, bool>&
ProfPerspective::activated_signal ()
{
    return signal_activated;
}

sigc::signal<void>&
ProfPerspective::layout_changed_signal ()
{
    return signal_layout_changed;
}

class ProfPerspectiveModule : DynamicModule {

public:

    void get_info (Info &a_info) const
    {
        static Info s_info ("Profiler perspective plugin",
                            "The profiler perspective of Nemiver",
                            "1.0");
        a_info = s_info;
    }

    void do_init ()
    {
    }

    bool lookup_interface (const std::string &a_iface_name,
                           DynModIfaceSafePtr &a_iface)
    {
        LOG_DD ("looking up interface: " + a_iface_name);
        if (a_iface_name == "IPerspective") {
            a_iface.reset (new ProfPerspective (this));
        } else if (a_iface_name == "IProfPerspective") {
            a_iface.reset (new ProfPerspective (this));
        } else {
            return false;
        }
        LOG_DD ("interface " + a_iface_name + " found");
        return true;
    }
};// end class ProfPerspective

NEMIVER_END_NAMESPACE (nemiver)

//the dynmod initial factory.
extern "C" {
NEMIVER_API bool
nemiver_common_create_dynamic_module_instance (void **a_new_instance)
{
    *a_new_instance = new nemiver::ProfPerspectiveModule ();
    return (*a_new_instance != 0);
}

}//end extern C

