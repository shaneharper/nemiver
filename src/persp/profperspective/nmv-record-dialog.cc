//Authors: Dodji Seketeli, Fabien Parent
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
#include <gtkmm/dialog.h>
#include <gtkmm/entry.h>
#include <gtkmm/filechooserbutton.h>
#include <gtkmm/stock.h>
#include <gtkmm/treeview.h>
#include <gtkmm/liststore.h>
#include "common/nmv-exception.h"
#include "common/nmv-env.h"
#include "common/nmv-ustring.h"
#include "nmv-record-dialog.h"
#include "nmv-ui-utils.h"

using namespace std;
using namespace nemiver::common;

namespace nemiver {

struct EnvVarModelColumns : public Gtk::TreeModel::ColumnRecord
{
    // I tried using UString here, but it didn't want to compile... jmj
    Gtk::TreeModelColumn<Glib::ustring> varname;
    Gtk::TreeModelColumn<Glib::ustring> value;
    EnvVarModelColumns() { add (varname); add (value); }
};

struct RecordDialog::Priv {
private:
    Priv ();

public:
    Gtk::FileChooserButton *fcbutton;
    Gtk::Button *okbutton;
    Gtk::TreeView* treeview_environment;
    Gtk::Button* remove_button;
    Gtk::Button* add_button;
    EnvVarModelColumns env_columns;
    Glib::RefPtr<Gtk::ListStore> model;
    Gtk::Dialog &dialog;
    Glib::RefPtr<Gtk::Builder> gtkbuilder;
    Gtk::CheckButton *scalecounter_checkbutton;
    Gtk::CheckButton *callgraph_checkbutton;
    Gtk::CheckButton *inherit_checkbutton;

    Priv (Gtk::Dialog &a_dialog,
        const Glib::RefPtr<Gtk::Builder> &a_gtkbuilder) :
        fcbutton (0),
        okbutton (0),
        treeview_environment (0),
        remove_button (0),
        add_button (0),
        model (Gtk::ListStore::create (env_columns)),
        dialog (a_dialog),
        gtkbuilder (a_gtkbuilder)
    {
        init ();
    }

    void init ()
    {
        okbutton =
            ui_utils::get_widget_from_gtkbuilder<Gtk::Button>
                (gtkbuilder, "executebutton");
        THROW_IF_FAIL (okbutton);
        okbutton->set_sensitive (false);

        treeview_environment =
            ui_utils::get_widget_from_gtkbuilder<Gtk::TreeView>
                                            (gtkbuilder, "treeview_environment");

        treeview_environment->set_model (model);

        treeview_environment->append_column_editable
                                        (_("Name"), env_columns.varname);

        treeview_environment->append_column_editable
                                            (_("Value"), env_columns.value);

        add_button =
            ui_utils::get_widget_from_gtkbuilder<Gtk::Button>
                                                    (gtkbuilder, "button_add_var");
        THROW_IF_FAIL (add_button);

        add_button->signal_clicked().connect(sigc::mem_fun(*this,
                    &RecordDialog::Priv::on_add_new_variable));

        remove_button = ui_utils::get_widget_from_gtkbuilder<Gtk::Button>
                                                (gtkbuilder, "button_remove_var");
        THROW_IF_FAIL (remove_button);
        remove_button->signal_clicked().connect(sigc::mem_fun(*this,
                    &RecordDialog::Priv::on_remove_variable));

        // we need to disable / enable sensitivity of the
        // "Remove variable" button based on whether a
        // variable is selected in the treeview.
        treeview_environment->get_selection ()->signal_changed ().connect
            (sigc::mem_fun
                 (*this,
                  &RecordDialog::Priv::on_variable_selection_changed));

        fcbutton =
            ui_utils::get_widget_from_gtkbuilder<Gtk::FileChooserButton>
                                                (gtkbuilder, "filechooserbutton");
        THROW_IF_FAIL (fcbutton);
        fcbutton->set_show_hidden (true);
        fcbutton->signal_selection_changed ().connect (sigc::mem_fun
                (*this, &Priv::on_file_selection_changed));

        // activate the default action (execute) when pressing enter in the
        // arguments text box
        ui_utils::get_widget_from_gtkbuilder<Gtk::Entry>
                        (gtkbuilder, "argumentsentry")->set_activates_default ();

        scalecounter_checkbutton =
            ui_utils::get_widget_from_gtkbuilder<Gtk::CheckButton>
                (gtkbuilder, "scalecheckbutton");
        THROW_IF_FAIL (scalecounter_checkbutton);

        callgraph_checkbutton =
            ui_utils::get_widget_from_gtkbuilder<Gtk::CheckButton>
                (gtkbuilder, "callgraphcheckbutton");
        THROW_IF_FAIL (callgraph_checkbutton);

        inherit_checkbutton =
            ui_utils::get_widget_from_gtkbuilder<Gtk::CheckButton>
                (gtkbuilder, "inheritcheckbutton");
        THROW_IF_FAIL (inherit_checkbutton);
    }

    void on_add_new_variable ()
    {
        THROW_IF_FAIL(model);
        THROW_IF_FAIL(treeview_environment);
        Gtk::TreeModel::iterator treeiter = model->append ();
        Gtk::TreeModel::Path path = model->get_path (treeiter);
        // activate the first cell of the newly
        // added row so that the user can start
        // typing in the name and value of the variable
        treeview_environment->set_cursor (path,
                *treeview_environment->get_column (0), true);
    }

    void on_remove_variable ()
    {
        THROW_IF_FAIL(treeview_environment);
        Gtk::TreeModel::iterator treeiter =
            treeview_environment->get_selection ()->get_selected ();
        if (treeiter)
        {
            model->erase(treeiter);
        }
    }

    void on_variable_selection_changed ()
    {
        THROW_IF_FAIL (remove_button);
        if (treeview_environment->get_selection ()->count_selected_rows ()) {
            remove_button->set_sensitive();
        } else {
            remove_button->set_sensitive(false);
        }
    }

    void on_activate_textentry ()
    {
        dialog.activate_default ();
    }

    void on_file_selection_changed ()
    {
        if (okbutton && fcbutton) {
            if (Glib::file_test
                    (Glib::locale_from_utf8 (fcbutton->get_filename ()),
                                             Glib::FILE_TEST_IS_EXECUTABLE)) {
                okbutton->set_sensitive (true);
            }
        }
    }
};//end struct RecordDialog::Priv

RecordDialog::RecordDialog (const UString &a_root_path) :
    Dialog (a_root_path, "recorddialog.ui", "recorddialog")
{
    m_priv.reset (new Priv (widget (), gtkbuilder ()));
    THROW_IF_FAIL (m_priv);

    working_directory (Glib::filename_to_utf8 (Glib::get_current_dir ()));
}

RecordDialog::~RecordDialog ()
{
    LOG_D ("destroyed", "destructor-domain");
}

UString
RecordDialog::program_name () const
{
    Gtk::FileChooserButton *chooser =
        ui_utils::get_widget_from_gtkbuilder<Gtk::FileChooserButton>
                                        (gtkbuilder (), "filechooserbutton");
    return chooser->get_filename ();
}

void
RecordDialog::program_name (const UString &a_name)
{
    THROW_IF_FAIL (m_priv);

    Gtk::FileChooserButton *chooser =
        ui_utils::get_widget_from_gtkbuilder<Gtk::FileChooserButton>
                                    (gtkbuilder (), "filechooserbutton");
    THROW_IF_FAIL (chooser);
    chooser->set_filename (a_name);
}

UString
RecordDialog::arguments () const
{
    Gtk::Entry *entry =
        ui_utils::get_widget_from_gtkbuilder<Gtk::Entry> (gtkbuilder (),
                                                     "argumentsentry");
    THROW_IF_FAIL (entry);
    return entry->get_text ();
}

void
RecordDialog::arguments (const UString &a_args)
{
    Gtk::Entry *entry =
        ui_utils::get_widget_from_gtkbuilder<Gtk::Entry> (gtkbuilder (),
                                                     "argumentsentry");
    THROW_IF_FAIL (entry);
    entry->set_text (a_args);
}

UString
RecordDialog::working_directory () const
{
    Gtk::FileChooserButton *chooser =
        ui_utils::get_widget_from_gtkbuilder<Gtk::FileChooserButton>
                                (gtkbuilder (), "filechooserbutton_workingdir");
    return chooser->get_filename ();
}

void
RecordDialog::working_directory (const UString &a_dir)
{
    Gtk::FileChooserButton *chooser =
        ui_utils::get_widget_from_gtkbuilder<Gtk::FileChooserButton>
            (gtkbuilder (), "filechooserbutton_workingdir");
    if (a_dir == "" || a_dir == ".") {
        chooser->set_filename
                    (Glib::locale_to_utf8 (Glib::get_current_dir ()));
    } else {
        chooser->set_filename (a_dir);
    }
}

map<UString, UString>
RecordDialog::environment_variables () const
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->model);
    map<UString, UString> env_vars;
    for (Gtk::TreeModel::iterator iter = m_priv->model->children().begin ();
         iter != m_priv->model->children().end();
         ++iter) {
        // for some reason I have to explicitly convert from Glib::ustring to
        // UString here or it won't compile
        env_vars[UString((*iter)[m_priv->env_columns.varname])] =
            UString((*iter)[m_priv->env_columns.value]);
    }
    return env_vars;
}

void
RecordDialog::environment_variables (const map<UString, UString> &vars)
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->model);
    // clear out the old data so we can set the new data
    m_priv->model->clear();
    for (map<UString, UString>::const_iterator iter = vars.begin();
         iter != vars.end();
         ++iter) {
        Gtk::TreeModel::iterator treeiter = m_priv->model->append();
        (*treeiter)[m_priv->env_columns.varname] = iter->first;
        (*treeiter)[m_priv->env_columns.value] = iter->second;
    }
}

bool
RecordDialog::callgraph () const
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->callgraph_checkbutton);
    return m_priv->callgraph_checkbutton->get_active ();
}

bool
RecordDialog::scale_counter_values () const
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->scalecounter_checkbutton);
    return m_priv->scalecounter_checkbutton->get_active ();
}

bool
RecordDialog::child_inherit_counters () const
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->inherit_checkbutton);
    return m_priv->inherit_checkbutton->get_active ();
}

}//end namespace nemiver

