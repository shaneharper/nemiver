//Author: Dodji Seketeli, Jonathon Jongsma
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
#include <glib/gi18n.h>
#include <gtkmm/treeview.h>
#include <gtkmm/liststore.h>
#include "common/nmv-env.h"
#include "nmv-preferences-dialog.h"
#include "nmv-ui-utils.h"
#include "nmv-i-conf-mgr.h"
#include "nmv-i-perspective.h"
#include "nmv-conf-keys.h"
#include "nmv-layout-selector.h"
#include <gtksourceviewmm/styleschememanager.h>

using nemiver::common::DynamicModuleManager;
static const std::string DEFAULT_GDB_BINARY = "default-gdb-binary";
NEMIVER_BEGIN_NAMESPACE(nemiver)

struct SourceDirsCols : public Gtk::TreeModelColumnRecord {
    Gtk::TreeModelColumn<Glib::ustring> dir;

    SourceDirsCols ()
    {
        add (dir);
    }
};// end SourceDirsColumns

static SourceDirsCols&
source_dirs_cols ()
{
    static SourceDirsCols s_cols;
    return s_cols;
}

class PreferencesDialog::Priv {
    Priv ();

     class StyleModelColumns : public Gtk::TreeModel::ColumnRecord {
     public:
        Gtk::TreeModelColumn<Glib::ustring> scheme_id;
        Gtk::TreeModelColumn<Glib::ustring> name;
        StyleModelColumns () { add (scheme_id); add (name); }
    };

public:
    IPerspective &perspective;
    LayoutManager &layout_manager;
    //source directories property widgets
    vector<UString> source_dirs;
    Glib::RefPtr<Gtk::ListStore> list_store;
    Gtk::TreeView *tree_view;
    Gtk::TreeModel::iterator cur_dir_iter;
    Gtk::Button *remove_dir_button;
    //source editor properties widgets
    Gtk::CheckButton *system_font_check_button;
    Gtk::FontButton *custom_font_button;
    Gtk::ComboBox *editor_style_combo;
    Gtk::ComboBoxText *asm_flavor_combo;
    Glib::RefPtr<Gtk::ListStore> m_editor_style_model;
    StyleModelColumns m_style_columns;
    Gtk::CellRendererText m_style_name_renderer;
    Gtk::HBox *custom_font_box;
    Gtk::Box *layout_box;
    Gtk::CheckButton *show_lines_check_button;
    Gtk::CheckButton *launch_terminal_check_button;
    Gtk::CheckButton *highlight_source_check_button;
    Gtk::RadioButton *always_reload_radio_button;
    Gtk::RadioButton *never_reload_radio_button;
    Gtk::RadioButton *confirm_reload_radio_button;
    Gtk::RadioButton *pure_asm_radio_button;
    Gtk::RadioButton *mixed_asm_radio_button;
    Gtk::RadioButton *follow_parent_radio_button;
    Gtk::RadioButton *follow_child_radio_button;
    Gtk::SpinButton  *default_num_asm_instrs_spin_button;
    Gtk::FileChooserButton *gdb_binary_path_chooser_button;
    Gtk::CheckButton *pretty_printing_check_button;
    Glib::RefPtr<Gtk::Builder> gtkbuilder;
    SafePtr<LayoutSelector> layout_selector;

    Priv (const Glib::RefPtr<Gtk::Builder> &a_gtkbuilder,
          IPerspective &a_perspective,
          LayoutManager &a_layout_manager) :
        perspective (a_perspective),
        layout_manager (a_layout_manager),
        tree_view (0),
        remove_dir_button (0),
        system_font_check_button (0),
        custom_font_button (0),
        custom_font_box (0),
        layout_box (0),
        show_lines_check_button (0),
        launch_terminal_check_button (0),
        highlight_source_check_button (0),
        always_reload_radio_button (0),
        never_reload_radio_button (0),
        confirm_reload_radio_button (0),
        pure_asm_radio_button (0),
        mixed_asm_radio_button (0),
        follow_parent_radio_button (0),
        follow_child_radio_button (0),
        default_num_asm_instrs_spin_button (0),
        gdb_binary_path_chooser_button (0),
        pretty_printing_check_button (0),
        gtkbuilder (a_gtkbuilder)
    {
        init ();
    }

    void on_tree_view_selection_changed ()
    {
        THROW_IF_FAIL (tree_view);
        Glib::RefPtr<Gtk::TreeSelection> sel = tree_view->get_selection ();
        THROW_IF_FAIL (sel);
        cur_dir_iter = sel->get_selected ();
        if (cur_dir_iter) {
            remove_dir_button->set_sensitive (true);
        } else {
            remove_dir_button->set_sensitive (false);
        }
    }

    void on_add_dir_button_clicked ()
    {
        NEMIVER_TRY
        Gtk::FileChooserDialog file_chooser
                                (_("Choose directory"),
                                 Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);

        file_chooser.add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
        file_chooser.add_button (Gtk::Stock::OK, Gtk::RESPONSE_OK);
        file_chooser.set_select_multiple (false);

        int result = file_chooser.run ();

        if (result != Gtk::RESPONSE_OK) {LOG_DD ("cancelled"); return;}

        UString path = file_chooser.get_filename ();

        if (path == "") {LOG_DD ("Got null dir");return;}

        Gtk::TreeModel::iterator iter = list_store->append ();
        (*iter)[source_dirs_cols ().dir] = path;

        update_source_dirs_key ();

        NEMIVER_CATCH
    }

    void on_remove_dir_button_clicked ()
    {
        NEMIVER_TRY
        if (!cur_dir_iter) {return;}
        list_store->erase (cur_dir_iter);
        update_source_dirs_key ();
        NEMIVER_CATCH
    }

    void on_show_lines_toggled_signal ()
    {
        update_show_source_line_numbers_key ();
    }

    void on_launch_terminal_toggled_signal ()
    {
        update_use_launch_terminal_key ();
    }

    void on_highlight_source_toggled_signal ()
    {
        update_highlight_source_keys ();
    }

    void on_system_font_toggled_signal ()
    {
        update_system_font_key ();
        custom_font_box->set_sensitive
                            (!system_font_check_button->get_active ());
    }

    void on_custom_font_set_signal ()
    {
        update_custom_font_key ();
    }

    void on_editor_style_changed_signal ()
    {
        update_editor_style_key ();
    }

    void on_asm_flavor_changed_signal ()
    {
        update_asm_flavor_key ();
    }

    void on_reload_files_toggled_signal ()
    {
        update_reload_files_keys ();
    }

    void on_asm_style_toggled_signal ()
    {
        update_asm_style_keys ();
    }

    void on_num_asms_value_changed_signal ()
    {
        update_default_num_asm_instrs_key ();
    }

    void on_gdb_binary_file_set_signal ()
    {
        update_gdb_binary_key ();
    }

    void on_follow_fork_mode_toggle_signal ()
    {
        update_follow_fork_mode_key ();
    }

    void on_pretty_printing_toggled_signal ()
    {
        update_pretty_printing_key ();
    }

    void init ()
    {

        // ********************************************
        // Handle the sources directory preferences tab
        // ********************************************

        list_store = Gtk::ListStore::create (source_dirs_cols ());
        tree_view =
            ui_utils::get_widget_from_gtkbuilder<Gtk::TreeView> (gtkbuilder,
                                                            "dirstreeview");
        tree_view->append_column (_("Source directories"),
                                  source_dirs_cols ().dir);

        tree_view->set_headers_visible (false);

        tree_view->set_model (list_store);

        tree_view->get_selection ()->set_mode (Gtk::SELECTION_SINGLE);

        tree_view->get_selection ()->signal_changed ().connect
        (sigc::mem_fun
         (this, &PreferencesDialog::Priv::on_tree_view_selection_changed));

        Gtk::Button *button =
            ui_utils::get_widget_from_gtkbuilder<Gtk::Button> (gtkbuilder,
                                                          "adddirbutton");
        button->signal_clicked ().connect (sigc::mem_fun
                (this, &PreferencesDialog::Priv::on_add_dir_button_clicked));

        remove_dir_button =
        ui_utils::get_widget_from_gtkbuilder<Gtk::Button> (gtkbuilder,
                                                      "suppressdirbutton");
        remove_dir_button->signal_clicked ().connect
        (sigc::mem_fun
            (this, &PreferencesDialog::Priv::on_remove_dir_button_clicked));
        remove_dir_button->set_sensitive (false);

        // *************************************
        // Handle the "Editor" preference tab
        // *************************************

        show_lines_check_button  =
            ui_utils::get_widget_from_gtkbuilder<Gtk::CheckButton>
                                            (gtkbuilder, "showlinescheckbutton");
        THROW_IF_FAIL (show_lines_check_button);
        show_lines_check_button->signal_toggled ().connect
        (sigc::mem_fun
            (*this,
             &PreferencesDialog::Priv::on_show_lines_toggled_signal));

        launch_terminal_check_button  =
            ui_utils::get_widget_from_gtkbuilder<Gtk::CheckButton>
                                            (gtkbuilder, "launchterminalcheckbutton");
        THROW_IF_FAIL (launch_terminal_check_button);
        launch_terminal_check_button->signal_toggled ().connect
        (sigc::mem_fun
            (*this,
             &PreferencesDialog::Priv::on_launch_terminal_toggled_signal));

        highlight_source_check_button  =
            ui_utils::get_widget_from_gtkbuilder<Gtk::CheckButton>
                                    (gtkbuilder, "highlightsourcecheckbutton");
        THROW_IF_FAIL (highlight_source_check_button);
        highlight_source_check_button->signal_toggled ().connect
        (sigc::mem_fun
            (*this,
             &PreferencesDialog::Priv::on_highlight_source_toggled_signal));

        system_font_check_button =
            ui_utils::get_widget_from_gtkbuilder<Gtk::CheckButton>
            (gtkbuilder, "systemfontcheckbutton");
        THROW_IF_FAIL (system_font_check_button);
        system_font_check_button->signal_toggled ().connect
        (sigc::mem_fun
            (*this,
             &PreferencesDialog::Priv::on_system_font_toggled_signal));

        custom_font_button =
            ui_utils::get_widget_from_gtkbuilder<Gtk::FontButton>
            (gtkbuilder, "customfontfontbutton");
        THROW_IF_FAIL (custom_font_button);
        custom_font_button->signal_font_set ().connect
        (sigc::mem_fun
            (*this,
             &PreferencesDialog::Priv::on_custom_font_set_signal));

        custom_font_box =
            ui_utils::get_widget_from_gtkbuilder<Gtk::HBox>
            (gtkbuilder, "customfonthbox");
        THROW_IF_FAIL (custom_font_box);

        editor_style_combo =
            ui_utils::get_widget_from_gtkbuilder<Gtk::ComboBox>
                                                        (gtkbuilder,
                                                         "editorstylecombobox");
        THROW_IF_FAIL (editor_style_combo);
        m_editor_style_model = Gtk::ListStore::create (m_style_columns);

        Glib::RefPtr<Gsv::StyleSchemeManager> mgr =
            Gsv::StyleSchemeManager::get_default ();
        std::vector<std::string> schemes = mgr->get_scheme_ids ();
        for (std::vector<std::string>::const_iterator it = schemes.begin ();
             it != schemes.end(); ++it) {
            Gtk::TreeModel::iterator treeiter = m_editor_style_model->append ();
            (*treeiter)[m_style_columns.scheme_id] = *it;
            Glib::RefPtr<Gsv::StyleScheme> scheme =
                mgr->get_scheme (*it);
            (*treeiter)[m_style_columns.name] =
                Glib::ustring::compose ("%1 - <span size=\"smaller\" "
                                        "font_style=\"italic\">%2</span>",
                                        scheme->get_name(),
                                        scheme->get_description ());
        }
        editor_style_combo->set_model (m_editor_style_model);
        editor_style_combo->pack_start (m_style_name_renderer);
        editor_style_combo->add_attribute (m_style_name_renderer.property_markup(),
                                           m_style_columns.name);
        m_style_name_renderer.property_ellipsize () = Pango::ELLIPSIZE_END;
        editor_style_combo->signal_changed ().connect (sigc::mem_fun
                    (*this,
                     &PreferencesDialog::Priv::on_editor_style_changed_signal));

        always_reload_radio_button =
            ui_utils::get_widget_from_gtkbuilder<Gtk::RadioButton>
                                                        (gtkbuilder,
                                                         "reloadradiobutton");
        THROW_IF_FAIL (always_reload_radio_button);
        always_reload_radio_button->signal_toggled ().connect (sigc::mem_fun
                    (*this,
                     &PreferencesDialog::Priv::on_reload_files_toggled_signal));

        never_reload_radio_button =
            ui_utils::get_widget_from_gtkbuilder<Gtk::RadioButton>
                                                (gtkbuilder,
                                                 "neverreloadradiobutton");
        THROW_IF_FAIL (never_reload_radio_button);
        never_reload_radio_button->signal_toggled ().connect (sigc::mem_fun
            (*this,
             &PreferencesDialog::Priv::on_reload_files_toggled_signal));

        confirm_reload_radio_button =
            ui_utils::get_widget_from_gtkbuilder<Gtk::RadioButton>
            (gtkbuilder, "confirmreloadradiobutton");
        THROW_IF_FAIL (confirm_reload_radio_button);
        confirm_reload_radio_button->signal_toggled ().connect (sigc::mem_fun
            (*this,
             &PreferencesDialog::Priv::on_reload_files_toggled_signal));

        // *************************************
        // Handle the "Debugger" preferences tab
        // *************************************

        pure_asm_radio_button =
            ui_utils::get_widget_from_gtkbuilder<Gtk::RadioButton>
                (gtkbuilder, "pureasmradio");
        THROW_IF_FAIL (pure_asm_radio_button);
        pure_asm_radio_button->signal_toggled ().connect
            (sigc::mem_fun
                 (*this,
                  &PreferencesDialog::Priv::on_asm_style_toggled_signal));

        asm_flavor_combo =
            ui_utils::get_widget_from_gtkbuilder<Gtk::ComboBoxText>
                (gtkbuilder, "asmflavorcombobox");
        THROW_IF_FAIL (asm_flavor_combo);
        asm_flavor_combo->signal_changed ().connect (sigc::mem_fun
                    (*this,
                     &PreferencesDialog::Priv::on_asm_flavor_changed_signal));
        asm_flavor_combo->set_entry_text_column (0);

        mixed_asm_radio_button =
            ui_utils::get_widget_from_gtkbuilder<Gtk::RadioButton>
                (gtkbuilder, "mixedasmradio");
        THROW_IF_FAIL (mixed_asm_radio_button);
        mixed_asm_radio_button->signal_toggled ().connect
            (sigc::mem_fun
                 (*this,
                  &PreferencesDialog::Priv::on_asm_style_toggled_signal));

        default_num_asm_instrs_spin_button =
            ui_utils::get_widget_from_gtkbuilder<Gtk::SpinButton>
                (gtkbuilder, "defaultnumasmspin");
        THROW_IF_FAIL (default_num_asm_instrs_spin_button);
        default_num_asm_instrs_spin_button->signal_value_changed ().connect
            (sigc::mem_fun
                 (*this,
                  &PreferencesDialog::Priv::on_num_asms_value_changed_signal));

        gdb_binary_path_chooser_button =
        ui_utils::get_widget_from_gtkbuilder<Gtk::FileChooserButton>
                (gtkbuilder, "pathtogdbfilechooser");
        THROW_IF_FAIL (gdb_binary_path_chooser_button);
        gdb_binary_path_chooser_button->signal_file_set ().connect
            (sigc::mem_fun
                 (*this,
                  &PreferencesDialog::Priv::on_gdb_binary_file_set_signal));

        follow_parent_radio_button =
        ui_utils::get_widget_from_gtkbuilder<Gtk::RadioButton> (gtkbuilder,
                                                           "followparentradio");
        THROW_IF_FAIL (follow_parent_radio_button);
        follow_parent_radio_button->signal_toggled ().connect
            (sigc::mem_fun
                 (*this,
                  &PreferencesDialog::Priv::on_follow_fork_mode_toggle_signal));

        follow_child_radio_button =
        ui_utils::get_widget_from_gtkbuilder<Gtk::RadioButton> (gtkbuilder,
                                                           "followchildradio");
        THROW_IF_FAIL (follow_child_radio_button);
        follow_child_radio_button->signal_toggled ().connect
            (sigc::mem_fun
                 (*this,
                  &PreferencesDialog::Priv::on_follow_fork_mode_toggle_signal));

        pretty_printing_check_button =
            ui_utils::get_widget_from_gtkbuilder<Gtk::CheckButton>
            (gtkbuilder,
             "prettyprintingcheckbutton");
        THROW_IF_FAIL (pretty_printing_check_button);
        pretty_printing_check_button->signal_toggled ().connect
            (sigc::mem_fun
             (*this,
              &PreferencesDialog::Priv::on_pretty_printing_toggled_signal));


        // *************************************
        // Handle the "Layout" preferences tab
        // *************************************

        layout_box = ui_utils::get_widget_from_gtkbuilder<Gtk::Box>
            (gtkbuilder, "layoutbox");
        THROW_IF_FAIL (layout_box);

        layout_selector.reset (new LayoutSelector (layout_manager, perspective));
        layout_box->pack_start (layout_selector->widget ());
        layout_box->show_all_children ();                                    
    }

    void collect_source_dirs ()
    {
        source_dirs.clear ();
        Gtk::TreeModel::iterator iter;

        for (iter = list_store->children ().begin ();
             iter != list_store->children ().end ();
             ++iter) {
            source_dirs.push_back (UString ((*iter)[source_dirs_cols ().dir]));
        }

    }

    void set_source_dirs (const vector<UString> &a_dirs)
    {
        Gtk::TreeModel::iterator row_it;
        vector<UString>::const_iterator dir_it;
        for (dir_it = a_dirs.begin (); dir_it != a_dirs.end (); ++dir_it) {
            row_it = list_store->append ();
            (*row_it)[source_dirs_cols ().dir] = *dir_it;
        }
    }

    IConfMgr& conf_manager () const
    {
        IConfMgrSafePtr conf_mgr = perspective.get_workbench ()
            .get_configuration_manager ();
        THROW_IF_FAIL (conf_mgr);
        return *conf_mgr;
    }

    void update_source_dirs_key ()
    {
        collect_source_dirs ();
        UString source_dirs_str;
        for (vector<UString>::const_iterator it = source_dirs.begin ();
             it != source_dirs.end ();
             ++it) {
            if (source_dirs_str == "") {
                source_dirs_str = *it;
            } else {
                source_dirs_str += ":"  + *it;
            }
        }
        conf_manager ().set_key_value
            (CONF_KEY_NEMIVER_SOURCE_DIRS, source_dirs_str);
    }

    void update_show_source_line_numbers_key ()
    {
        THROW_IF_FAIL (show_lines_check_button);
        bool is_on = show_lines_check_button->get_active ();
        conf_manager ().set_key_value
                    (CONF_KEY_SHOW_SOURCE_LINE_NUMBERS, is_on);
    }

    void update_use_launch_terminal_key ()
    {
        THROW_IF_FAIL (launch_terminal_check_button);
        bool is_on = launch_terminal_check_button->get_active ();
        conf_manager ().set_key_value
                    (CONF_KEY_USE_LAUNCH_TERMINAL, is_on);
    }

    void update_highlight_source_keys ()
    {
        THROW_IF_FAIL (highlight_source_check_button);
        bool is_on = highlight_source_check_button->get_active ();
        conf_manager ().set_key_value
                    (CONF_KEY_HIGHLIGHT_SOURCE_CODE,
                     is_on);
    }

    void update_widget_from_source_dirs_key ()
    {
        UString paths_str;
        if (!conf_manager ().get_key_value
                        (CONF_KEY_NEMIVER_SOURCE_DIRS, paths_str)) {
            return;
        }
        if (paths_str == "") {return;}
        std::vector<UString> paths = paths_str.split (":");
        set_source_dirs (paths);
    }

    void update_system_font_key ()
    {
        THROW_IF_FAIL (system_font_check_button);
        bool is_on = system_font_check_button->get_active ();
        conf_manager ().set_key_value
                    (CONF_KEY_USE_SYSTEM_FONT, is_on);
    }

    void update_custom_font_key ()
    {
        THROW_IF_FAIL (custom_font_button);
        UString font_name = custom_font_button->get_font_name ();
        conf_manager ().set_key_value
                    (CONF_KEY_CUSTOM_FONT_NAME, font_name);
    }

    void update_editor_style_key ()
    {
        THROW_IF_FAIL (editor_style_combo);
        Gtk::TreeModel::iterator treeiter = editor_style_combo->get_active();
        UString scheme = treeiter->get_value(m_style_columns.scheme_id);
        conf_manager ().set_key_value
                    (CONF_KEY_EDITOR_STYLE_SCHEME, scheme);
    }

    void update_asm_flavor_key ()
    {
        THROW_IF_FAIL (asm_flavor_combo);
        UString asm_flavor = asm_flavor_combo->get_active_text ();
        if (asm_flavor == "Intel") {
            conf_manager ().set_key_value
                    (CONF_KEY_DISASSEMBLY_FLAVOR, Glib::ustring ("intel"));
        } else {
            conf_manager ().set_key_value
                    (CONF_KEY_DISASSEMBLY_FLAVOR, Glib::ustring ("att"));
        }
    }

    void update_reload_files_keys ()
    {
        THROW_IF_FAIL (always_reload_radio_button);
        THROW_IF_FAIL (never_reload_radio_button);
        THROW_IF_FAIL (confirm_reload_radio_button);

        if (always_reload_radio_button->get_active ()) {
            conf_manager ().set_key_value
                    (CONF_KEY_CONFIRM_BEFORE_RELOAD_SOURCE, false);
            conf_manager ().set_key_value
                    (CONF_KEY_ALLOW_AUTO_RELOAD_SOURCE, true);
        } else if (never_reload_radio_button->get_active ()) {
            conf_manager ().set_key_value
                    (CONF_KEY_CONFIRM_BEFORE_RELOAD_SOURCE, false);
            conf_manager ().set_key_value
                    (CONF_KEY_ALLOW_AUTO_RELOAD_SOURCE, false);
        } else {
            conf_manager ().set_key_value
                    (CONF_KEY_CONFIRM_BEFORE_RELOAD_SOURCE, true);
        }
    }

    void update_asm_style_keys ()
    {
        THROW_IF_FAIL (pure_asm_radio_button);
        THROW_IF_FAIL (mixed_asm_radio_button);

        if (pure_asm_radio_button->get_active ()) {
            conf_manager ().set_key_value (CONF_KEY_ASM_STYLE_PURE, true);
        } else if (mixed_asm_radio_button->get_active ()) {
            conf_manager ().set_key_value (CONF_KEY_ASM_STYLE_PURE, false);
        }
    }

    void update_default_num_asm_instrs_key ()
    {
        THROW_IF_FAIL (default_num_asm_instrs_spin_button);
        int num = default_num_asm_instrs_spin_button->get_value_as_int ();
        conf_manager ().set_key_value (CONF_KEY_DEFAULT_NUM_ASM_INSTRS,
                                       num);
    }

    void update_gdb_binary_key ()
    {
        THROW_IF_FAIL (gdb_binary_path_chooser_button);
        UString path = gdb_binary_path_chooser_button->get_filename ();
        if (path.empty ())
            return;
        if (path == DEFAULT_GDB_BINARY)
            path = common::env::get_gdb_program ();
        conf_manager ().set_key_value (CONF_KEY_GDB_BINARY,
                                       Glib::filename_from_utf8 (path));
    }

    void update_follow_fork_mode_key ()
    {
        THROW_IF_FAIL (follow_parent_radio_button);
        THROW_IF_FAIL (follow_child_radio_button);

        UString mode = "parent";
        if (follow_parent_radio_button->get_active ())
            /*mode = "parent"*/;
        else if (follow_child_radio_button->get_active ())
            mode = "child";

        conf_manager ().set_key_value (CONF_KEY_FOLLOW_FORK_MODE, mode);
    }

    void update_pretty_printing_key ()
    {
        THROW_IF_FAIL (pretty_printing_check_button);
        
        bool is_on = pretty_printing_check_button->get_active ();
        conf_manager ().set_key_value (CONF_KEY_PRETTY_PRINTING, is_on);
    }

    void update_widget_from_editor_keys ()
    {
        THROW_IF_FAIL (show_lines_check_button);
        THROW_IF_FAIL (launch_terminal_check_button);
        THROW_IF_FAIL (highlight_source_check_button);
        THROW_IF_FAIL (system_font_check_button);
        THROW_IF_FAIL (custom_font_button);
        THROW_IF_FAIL (custom_font_box);
        THROW_IF_FAIL (editor_style_combo);
        THROW_IF_FAIL (asm_flavor_combo);

        bool is_on = true;
        if (!conf_manager ().get_key_value
                (CONF_KEY_SHOW_SOURCE_LINE_NUMBERS, is_on)) {
            LOG_ERROR ("failed to get gconf key");
        } else {
            show_lines_check_button->set_active (is_on);
        }

        is_on = false;
        if (!conf_manager ().get_key_value
                (CONF_KEY_USE_LAUNCH_TERMINAL, is_on)) {
            LOG_ERROR ("failed to get gconf key");
        } else {
            launch_terminal_check_button->set_active (is_on);
        }

        is_on = true;
        if (!conf_manager ().get_key_value
                (CONF_KEY_HIGHLIGHT_SOURCE_CODE, is_on)) {
            LOG_ERROR ("failed to get gconf key");
        } else {
            highlight_source_check_button->set_active (is_on);
        }

        is_on = true;
        if (!conf_manager ().get_key_value
                (CONF_KEY_USE_SYSTEM_FONT, is_on)) {
            LOG_ERROR ("failed to get gconf key");
        } else {
            system_font_check_button->set_active (is_on);
            custom_font_box->set_sensitive (!is_on);
        }
        UString font_name;
        if (!conf_manager ().get_key_value
                (CONF_KEY_CUSTOM_FONT_NAME, font_name)) {
            LOG_ERROR ("failed to get gconf key");
        } else {
            custom_font_button->set_font_name (font_name);
        }

        UString style_scheme;
        if (!conf_manager ().get_key_value
                (CONF_KEY_EDITOR_STYLE_SCHEME, style_scheme)) {
            LOG_ERROR ("failed to get gconf key");
        } else {
            Gtk::TreeModel::iterator treeiter;
            for (treeiter = m_editor_style_model->children().begin();
                 treeiter != m_editor_style_model->children().end();
                 ++treeiter) {
                if ((*treeiter)[m_style_columns.scheme_id] == style_scheme) {
                    editor_style_combo->set_active(treeiter);
                }
            }
        }

        UString asm_flavor;
        if (!conf_manager ().get_key_value
                (CONF_KEY_DISASSEMBLY_FLAVOR, asm_flavor)) {
            LOG_ERROR ("failed to get gconf key");
        } else {
            if (asm_flavor == "intel") {
                asm_flavor_combo->set_active_text ("Intel");
            } else {
                asm_flavor_combo->set_active_text ("AT&T");
            }
        }

        bool confirm_reload = true, always_reload = false;
        if (!conf_manager ().get_key_value
                (CONF_KEY_CONFIRM_BEFORE_RELOAD_SOURCE, confirm_reload)) {
            LOG_ERROR ("failed to get gconf key");
        }
        if (!conf_manager ().get_key_value
                (CONF_KEY_ALLOW_AUTO_RELOAD_SOURCE, always_reload)) {
            LOG_ERROR ("failed to get gconf key");
        }
        if (confirm_reload) {
            confirm_reload_radio_button->set_active ();
        } else if (always_reload) {
            always_reload_radio_button->set_active ();
        } else {
            never_reload_radio_button->set_active ();
        }
    }

    void update_widget_from_debugger_keys ()
    {
        THROW_IF_FAIL (pure_asm_radio_button);
        THROW_IF_FAIL (mixed_asm_radio_button);
        THROW_IF_FAIL (default_num_asm_instrs_spin_button);
        THROW_IF_FAIL (gdb_binary_path_chooser_button);

        bool pure_asm = false;
        if (!conf_manager ().get_key_value (CONF_KEY_ASM_STYLE_PURE,
                                            pure_asm)) {
            LOG_ERROR ("failed to get gconf key " << CONF_KEY_ASM_STYLE_PURE);
        }
        if (pure_asm)
            pure_asm_radio_button->set_active ();
        else
            mixed_asm_radio_button->set_active ();

        int num_asms = 25;
        if (!conf_manager ().get_key_value (CONF_KEY_DEFAULT_NUM_ASM_INSTRS,
                                            num_asms)) {
            LOG_ERROR ("failed to get gconf key "
                       << CONF_KEY_DEFAULT_NUM_ASM_INSTRS);
        }
        default_num_asm_instrs_spin_button->set_value (num_asms);

        UString gdb_binary = common::env::get_gdb_program ();
        if (!conf_manager ().get_key_value (CONF_KEY_GDB_BINARY, gdb_binary)) {
            LOG_ERROR ("failed to get gconf key " << CONF_KEY_GDB_BINARY);
        }
        gdb_binary_path_chooser_button->set_filename
            (Glib::filename_to_utf8 (gdb_binary));

        UString follow_fork_mode = "parent";
        if (!conf_manager ().get_key_value (CONF_KEY_FOLLOW_FORK_MODE,
                                           follow_fork_mode)) {
            LOG_ERROR ("failed to get gconf key "
                       << CONF_KEY_FOLLOW_FORK_MODE);
        }
        if (follow_fork_mode == "parent") {
            follow_parent_radio_button->set_active (true);
        } else if (follow_fork_mode == "child") {
            follow_child_radio_button->set_active (true);
        }

        bool is_on = true;
        if (!conf_manager ().get_key_value (CONF_KEY_PRETTY_PRINTING,
                                            is_on)) {
            LOG_ERROR ("failed to get gconf key "
                       << CONF_KEY_PRETTY_PRINTING);
        }
        pretty_printing_check_button->set_active (is_on);
    }

    void update_widget_from_conf ()
    {
        update_widget_from_source_dirs_key ();
        update_widget_from_editor_keys ();
        update_widget_from_debugger_keys ();
    }
};//end PreferencesDialog

PreferencesDialog::PreferencesDialog (IPerspective &a_perspective,
                                      LayoutManager &a_layout_manager,
                                      const UString &a_root_path) :
    Dialog (a_root_path,
            "preferencesdialog.ui",
            "preferencesdialog")
{
    m_priv.reset (new Priv (gtkbuilder (), a_perspective, a_layout_manager));
    m_priv->update_widget_from_conf ();
}

PreferencesDialog::~PreferencesDialog ()
{
    LOG_D ("delete", "destructor-domain");
    THROW_IF_FAIL (m_priv);
}

const std::vector<UString>&
PreferencesDialog::source_directories () const
{
    THROW_IF_FAIL (m_priv);
    m_priv->collect_source_dirs ();
    return m_priv->source_dirs;
}

void
PreferencesDialog::source_directories (const std::vector<UString> &a_dirs)
{
    THROW_IF_FAIL (m_priv);
    m_priv->source_dirs = a_dirs;
    m_priv->set_source_dirs (m_priv->source_dirs);
}

NEMIVER_END_NAMESPACE (nemiver)
