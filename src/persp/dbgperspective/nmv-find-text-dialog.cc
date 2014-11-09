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
#include <gtksourceviewmm.h>
#include <gtkmm/liststore.h>
#include "common/nmv-exception.h"
#include "nmv-find-text-dialog.h"
#include "nmv-ui-utils.h"
#include "nmv-source-editor.h"

NEMIVER_BEGIN_NAMESPACE (nemiver)

struct SearchTermCols : public Gtk::TreeModel::ColumnRecord {
    Gtk::TreeModelColumn<Glib::ustring> term;

    SearchTermCols ()
    {
        add (term);
    }
};

static SearchTermCols&
columns ()
{
    static SearchTermCols s_columns;
    return s_columns;
}

using namespace Gsv;

class FindTextDialog::Priv {
    friend class FindTextDialog;
    Gtk::Dialog &dialog;
    Glib::RefPtr<Gtk::Builder> gtkbuilder;
    Glib::RefPtr<Gtk::ListStore> searchterm_store;
    Gtk::TextIter match_start;
    Gtk::TextIter match_end;
    bool clear_selection_before_search;

    Priv ();

public:

    Priv (Gtk::Dialog &a_dialog,
          const Glib::RefPtr<Gtk::Builder> &a_gtkbuilder) :
        dialog (a_dialog),
        gtkbuilder (a_gtkbuilder),
        clear_selection_before_search (false)
    {
        a_dialog.set_default_response (Gtk::RESPONSE_OK);
        connect_dialog_signals ();
        searchterm_store = Gtk::ListStore::create (columns ());
        get_search_text_combo ()->set_model (searchterm_store);
        get_search_text_combo ()->set_entry_text_column (columns ().term);
    }

    void on_search_entry_activated_signal ()
    {
        NEMIVER_TRY
        get_search_button ()->clicked ();
        NEMIVER_CATCH
    }

    void on_dialog_show ()
    {
        NEMIVER_TRY
        // return the focus to the search text entry and highlight the search
        // term so that it can be overwritten by simply typing a new term
        get_search_text_combo ()->get_entry ()->grab_focus ();
        UString search_text =
            get_search_text_combo ()->get_entry ()->get_text ();
        if (search_text.size ()) {
            get_search_text_combo ()->get_entry ()->select_region
                                                    (0, search_text.size ());
        }
        NEMIVER_CATCH
    }

    Gtk::Button* get_close_button ()
    {
        Gtk::Button *button =
            ui_utils::get_widget_from_gtkbuilder<Gtk::Button> (gtkbuilder,
                                                          "closebutton1");
        return button;
    }

    Gtk::Button* get_search_button ()
    {
        Gtk::Button *button =
            ui_utils::get_widget_from_gtkbuilder<Gtk::Button> (gtkbuilder,
                                                          "searchbutton");
        return button;
    }

    Gtk::ComboBox* get_search_text_combo () const
    {
        Gtk::ComboBox *combo =
            ui_utils::get_widget_from_gtkbuilder<Gtk::ComboBox>
                                                (gtkbuilder, "searchtextcombo");
        return combo;
    }

    Gtk::CheckButton* get_match_case_check_button () const
    {
        Gtk::CheckButton *button =
            ui_utils::get_widget_from_gtkbuilder<Gtk::CheckButton>
                                            (gtkbuilder, "matchcasecheckbutton");
        return button;
    }

    Gtk::CheckButton* get_match_entire_word_check_button () const
    {
        Gtk::CheckButton *button =
            ui_utils::get_widget_from_gtkbuilder<Gtk::CheckButton>
                                    (gtkbuilder, "matchentirewordcheckbutton");
        return button;
    }

    Gtk::CheckButton* get_wrap_around_check_button () const
    {
        Gtk::CheckButton *button =
            ui_utils::get_widget_from_gtkbuilder<Gtk::CheckButton>
                                        (gtkbuilder, "wraparoundcheckbutton");
        return button;
    }

    Gtk::CheckButton* get_search_backwards_check_button () const
    {
        Gtk::CheckButton *button =
            ui_utils::get_widget_from_gtkbuilder<Gtk::CheckButton>
                                    (gtkbuilder, "searchbackwardscheckbutton");
        return button;
    }

    void connect_dialog_signals ()
    {
        Gtk::Button *search_button = get_search_button ();
        THROW_IF_FAIL (search_button);
        get_search_text_combo ()->get_entry ()->signal_activate ().connect
            (sigc::mem_fun (*this, &Priv::on_search_entry_activated_signal));
        dialog.signal_show ().connect (sigc::mem_fun
                                            (*this, &Priv::on_dialog_show));
        search_button->signal_clicked ().connect (sigc::mem_fun
                                (*this, &Priv::on_search_button_clicked));
    }

    //*******************
    //<signal handlers>
    //*******************
    void on_search_button_clicked ()
    {
        NEMIVER_TRY
        UString new_term =
                    get_search_text_combo ()->get_entry ()->get_text ();
        bool found = false;
        // first check if this term is already in the list
        Gtk::TreeModel::iterator tree_iter;
        for (tree_iter = searchterm_store->children ().begin ();
             tree_iter != searchterm_store->children ().end ();
             ++tree_iter) {
            if (new_term == (*tree_iter)[columns ().term]) {
                found = true;
                break;
            }
        }
        if (!found) {
            // if it's not already in the list, add it.
            Gtk::TreeModel::iterator new_iter = searchterm_store->append ();
            (*new_iter)[columns ().term] = new_term;
        }
        NEMIVER_CATCH
    }

    //*******************
    //</signal handlers>
    //*******************

};//end FindTextDialog

/// Constructor of the FindTextDialog type.
///
/// \param a_parent the parent window of the dialog.
///
/// \param a_root_path the path to the root directory of the
/// resources of the dialog.
FindTextDialog::FindTextDialog (Gtk::Window &a_parent,
                                const UString &a_root_path) :
    Dialog (a_root_path, "findtextdialog.ui",
            "findtextdialog", a_parent)
{
    m_priv.reset (new Priv (widget (), gtkbuilder ()));
    THROW_IF_FAIL (m_priv);
}

FindTextDialog::~FindTextDialog ()
{
    LOG_D ("destroyed", "destructor-domain");
}

Gtk::TextIter&
FindTextDialog::get_search_match_start () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->match_start;
}

Gtk::TextIter&
FindTextDialog::get_search_match_end () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->match_end;
}

void
FindTextDialog::get_search_string (UString &a_search_str) const
{
    THROW_IF_FAIL (m_priv);
    a_search_str = m_priv->get_search_text_combo ()->get_entry ()->get_text ();
}

void
FindTextDialog::set_search_string (const UString &a_search_str)
{
    THROW_IF_FAIL (m_priv);
    m_priv->get_search_text_combo ()->get_entry ()->set_text (a_search_str);
}

bool
FindTextDialog::get_match_case () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->get_match_case_check_button ()->get_active ();
}

void
FindTextDialog::set_match_case (bool a_flag)
{
    THROW_IF_FAIL (m_priv);
    m_priv->get_match_case_check_button ()->set_active (a_flag);
}

bool
FindTextDialog::get_match_entire_word () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->get_match_entire_word_check_button ()->get_active ();
}

void
FindTextDialog::set_match_entire_word (bool a_flag)
{
    THROW_IF_FAIL (m_priv);
    m_priv->get_match_entire_word_check_button ()->set_active (a_flag);
}

bool
FindTextDialog::get_wrap_around () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->get_wrap_around_check_button ()->get_active ();
}

void
FindTextDialog::set_wrap_around (bool a_flag)
{
    THROW_IF_FAIL (m_priv);
    m_priv->get_wrap_around_check_button ()->set_active (a_flag);
}

bool
FindTextDialog::get_search_backward () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->get_search_backwards_check_button ()->get_active ();
}

void
FindTextDialog::set_search_backward (bool a_flag)
{
    THROW_IF_FAIL (m_priv);
    m_priv->get_search_backwards_check_button ()->set_active (a_flag);
}

bool
FindTextDialog::clear_selection_before_search () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->clear_selection_before_search;
}

void
FindTextDialog::clear_selection_before_search (bool a)
{
    THROW_IF_FAIL (m_priv);
    m_priv->clear_selection_before_search = a;
}

NEMIVER_END_NAMESPACE (nemiver)

