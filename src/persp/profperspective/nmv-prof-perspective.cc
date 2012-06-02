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

#include <list>
#include <glib/gi18n.h>
#include <gtkmm/widget.h>

NEMIVER_BEGIN_NAMESPACE (nemiver)

class ProfPerspective : public IProfPerspective {
    //non copyable
    ProfPerspective (const IProfPerspective&);
    ProfPerspective& operator= (const IProfPerspective&);

    Glib::RefPtr<Gtk::ActionGroup> default_action_group;
    Gtk::HPaned body;
    IWorkbench *workbench;

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
    void load_report_file ();
    void on_load_report_file_action ();

    sigc::signal<void, bool>& activated_signal ();
    sigc::signal<void>& layout_changed_signal ();
}; // end class ProfPerspective

ProfPerspective::ProfPerspective (DynamicModule *a_dynmod) :
    IProfPerspective (a_dynmod)
{
}

GOptionGroup*
ProfPerspective::option_group () const
{
    return 0;
}

bool
ProfPerspective::process_options (GOptionContext */*a_context*/,
                 int /*a_argc*/,
                 char **/*a_argv*/)
{
    return true;
}

bool
ProfPerspective::process_gui_options (int /*a_argc*/, char **/*a_argv*/)
{
    return true;
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
}

const UString&
ProfPerspective::get_perspective_identifier ()
{
    static UString s_id = "org.nemiver.ProfilerPerspective";
    return s_id;
}

void
ProfPerspective::get_toolbars (std::list<Gtk::Widget*> &/*a_tbs*/)
{

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
ProfPerspective::init_actions ()
{
    Gtk::StockID nil_stock_id ("");
    sigc::slot<void> nil_slot;
    Glib::RefPtr<Gtk::UIManager> uimanager = get_workbench ().get_ui_manager ();
    THROW_IF_FAIL (uimanager);

    static ui_utils::ActionEntry s_default_action_entries [] = {
        {
            "LoadReportMenuItemAction",
            nil_stock_id,
            _("_Load Report File..."),
            _("Load a report file from disk"),
            sigc::mem_fun (*this, &ProfPerspective::on_load_report_file_action),
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

    uimanager->insert_action_group (default_action_group);

    get_workbench ().get_root_window ().add_accel_group
        (uimanager->get_accel_group ());
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
ProfPerspective::load_report_file ()
{
    LoadReportDialog dialog (plugin_path ());

    int result = dialog.run ();
    if (result != Gtk::RESPONSE_OK) {
        return;
    }
}

void
ProfPerspective::on_load_report_file_action ()
{
    NEMIVER_TRY

    load_report_file ();

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

