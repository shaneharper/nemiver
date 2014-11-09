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
#include <glib/gi18n.h>
#include <gtkmm/treeview.h>
#include <gtkmm/liststore.h>
#include <gtkmm/dialog.h>
#include <gtkmm/scrolledwindow.h>
#include "nmv-choose-overloads-dialog.h"
#include "nmv-ui-utils.h"

NEMIVER_BEGIN_NAMESPACE (nemiver)
using namespace std;

struct Cols : public Gtk::TreeModel::ColumnRecord {
    Gtk::TreeModelColumn<Glib::ustring> function_name;
    Gtk::TreeModelColumn<Glib::ustring> function_location;
    Gtk::TreeModelColumn<IDebugger::OverloadsChoiceEntry> overload;

    Cols ()
    {
        add (function_name);
        add (function_location);
        add (overload);
    }
};//end Cols

static Cols&
columns ()
{
    static Cols s_choice_cols;
    return s_choice_cols;
}

struct ChooseOverloadsDialog::Priv {
    friend class ChooseOverloadsDialog;
    Gtk::Dialog &dialog;
    Glib::RefPtr<Gtk::Builder> gtkbuilder;
    Gtk::TreeView *tree_view;
    Glib::RefPtr<Gtk::ListStore> list_store;
    vector<IDebugger::OverloadsChoiceEntry> current_overloads;

    Priv ();

    Priv (Gtk::Dialog &a_dialog,
          const Glib::RefPtr<Gtk::Builder> &a_gtkbuilder) :
        dialog (a_dialog),
        gtkbuilder (a_gtkbuilder)
    {
        init_members ();
        init_tree_view ();
        pack_tree_view_into_gtkbuilder ();
        Gtk::Widget *button =
            ui_utils::get_widget_from_gtkbuilder<Gtk::Widget> (gtkbuilder, "okbutton");
        button->set_sensitive (false);
        a_dialog.set_default_response (Gtk::RESPONSE_OK);
    }

    void init_members ()
    {
        tree_view = 0;
    }

    void pack_tree_view_into_gtkbuilder ()
    {
        Gtk::ScrolledWindow *sw =
            ui_utils::get_widget_from_gtkbuilder<Gtk::ScrolledWindow>
                                            (gtkbuilder, "treeviewscrolledwindow");
        sw->add (*tree_view);
    }

    void clear ()
    {
        if (list_store) {
            list_store->clear ();
        }
    }

    void init_tree_view ()
    {
        if (tree_view)
            return;

        if (!list_store) {
            list_store = Gtk::ListStore::create (columns ());
        }
        tree_view = Gtk::manage (new Gtk::TreeView (list_store));
        tree_view->append_column (_("Function Name"),
                                  columns ().function_name);
        tree_view->append_column (_("Location"),
                                  columns ().function_location);

        tree_view->get_selection ()->set_mode (Gtk::SELECTION_MULTIPLE);
        tree_view->get_selection ()->signal_changed ().connect
                        (sigc::mem_fun (*this, &Priv::on_selection_changed));
        tree_view->show_all ();
    }

    void add_choice_entry (const IDebugger::OverloadsChoiceEntry &a_entry)
    {
        Gtk::TreeModel::iterator tree_it = list_store->append ();
        THROW_IF_FAIL (tree_it);

        tree_it->set_value (columns ().overload, a_entry);
        tree_it->set_value (columns ().function_name,
                            (Glib::ustring)a_entry.function_name ());
        UString location (a_entry.file_name ()
                          + ":" + UString::from_int (a_entry.line_number ()));
        tree_it->set_value (columns ().function_location,
                            (Glib::ustring)location);
    }

    void on_selection_changed ()
    {
        NEMIVER_TRY

        THROW_IF_FAIL (tree_view);
        THROW_IF_FAIL (list_store);

        vector<Gtk::TreeModel::Path> iters =
                            tree_view->get_selection ()->get_selected_rows ();

        current_overloads.clear ();
        vector<Gtk::TreeModel::Path>::const_iterator it;
        for (it = iters.begin (); it != iters.end (); ++it) {
            current_overloads.push_back
                (list_store->get_iter (*it)->get_value (columns ().overload));
        }

        Gtk::Widget *ok_button =
            ui_utils::get_widget_from_gtkbuilder<Gtk::Button>
                                                    (gtkbuilder, "okbutton");
        if (current_overloads.empty ()) {
            ok_button->set_sensitive (false);
        } else {
            ok_button->set_sensitive (true);
        }

        NEMIVER_CATCH
    }

};//end ChooseOverloadsDialog

/// Constructor of the ChooseOverloadsDialog type.
///
/// \param a_parent the parent window of the dialog.x
///
/// \param a_root_path the path to the root directory of the
/// resources of the dialog.
ChooseOverloadsDialog::ChooseOverloadsDialog
(Gtk::Window &a_parent,
 const UString &a_root_path,
 const vector<IDebugger::OverloadsChoiceEntry> &a_entries) :
    Dialog (a_root_path,
            "chooseoverloadsdialog.ui",
            "chooseoverloadsdialog",
            a_parent)
{
    m_priv.reset (new Priv (widget (), gtkbuilder ()));
    THROW_IF_FAIL (m_priv);
    set_overloads_choice_entries (a_entries);
}

ChooseOverloadsDialog::~ChooseOverloadsDialog ()
{
    LOG_D ("destroyed", "destructor-domain");
}

const vector<IDebugger::OverloadsChoiceEntry>&
ChooseOverloadsDialog::overloaded_functions () const
{
    THROW_IF_FAIL (m_priv);

    return m_priv->current_overloads;
}

void
ChooseOverloadsDialog::overloaded_function (int a_in) const
{
    THROW_IF_FAIL (m_priv);

    Gtk::TreeModel::iterator it;
    for (it = m_priv->list_store->children ().begin ();
         it != m_priv->list_store->children ().end () && it;
         ++it) {
        if (it->get_value (columns ().overload).index () == a_in) {
            m_priv->tree_view->get_selection ()->select (it);
        }
    }
}

void
ChooseOverloadsDialog::set_overloads_choice_entries
                        (const vector<IDebugger::OverloadsChoiceEntry> &a_in)
{
    THROW_IF_FAIL (m_priv);

    vector<IDebugger::OverloadsChoiceEntry>::const_iterator it;
    for (it = a_in.begin (); it != a_in.end (); ++it) {
        if (it->kind () ==
            nemiver::IDebugger::OverloadsChoiceEntry::LOCATION) {
            m_priv->add_choice_entry (*it);
        }
    }
}

void
ChooseOverloadsDialog::clear ()
{
    THROW_IF_FAIL (m_priv);
    m_priv->clear ();
}

NEMIVER_END_NAMESPACE (nemiver)
