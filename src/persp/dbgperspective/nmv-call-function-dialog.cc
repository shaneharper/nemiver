// Dodji Seketeli
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
#include "nmv-call-function-dialog.h"
#include <glib/gi18n.h>
#include "common/nmv-exception.h"
#include "common/nmv-env.h"
#include "common/nmv-ustring.h"
#include "nmv-ui-utils.h"

NEMIVER_BEGIN_NAMESPACE (nemiver)

struct CallExprHistoryCols : public Gtk::TreeModel::ColumnRecord {
    Gtk::TreeModelColumn<Glib::ustring> expr;
    CallExprHistoryCols () { add (expr); }
};

static CallExprHistoryCols&
get_call_expr_history_cols ()
{
    static CallExprHistoryCols cols;
    return cols;
}

struct CallFunctionDialog::Priv {
    Gtk::ComboBox *call_expr_entry;
    Glib::RefPtr<Gtk::ListStore> call_expr_history;
    Gtk::Button *ok_button;
    Priv (Gtk::Dialog &a_dialog,
          const Glib::RefPtr<Gtk::Builder> &a_gtkbuilder) :
        call_expr_entry (0),
        ok_button (0)
    {
        a_dialog.set_default_response (Gtk::RESPONSE_OK);

        ok_button =
            ui_utils::get_widget_from_gtkbuilder<Gtk::Button> (a_gtkbuilder,
                                                          "okbutton");
        THROW_IF_FAIL (ok_button);
        ok_button->set_sensitive (false);
	ok_button->signal_clicked ().connect
            (sigc::mem_fun (*this, &Priv::on_ok_button_clicked_signal));

        call_expr_entry =
            ui_utils::get_widget_from_gtkbuilder<Gtk::ComboBox>
                                            (a_gtkbuilder, "callexpressionentry");
        THROW_IF_FAIL (call_expr_entry);
        call_expr_history=
            Gtk::ListStore::create (get_call_expr_history_cols ());
        call_expr_entry->set_model (call_expr_history);
        call_expr_entry->set_entry_text_column (get_call_expr_history_cols ().expr);

        call_expr_entry->signal_changed ().connect
            (sigc::mem_fun (*this, &Priv::on_call_expr_entry_changed_signal));
        call_expr_entry->get_entry ()->set_activates_default ();
    }

    void on_ok_button_clicked_signal ()
    {
        THROW_IF_FAIL (call_expr_entry);

        add_to_history (call_expr_entry->get_entry ()->get_text (),
                        /*prepend=*/false,
                        /*allow_dups=*/false);
    }

    void on_call_expr_entry_changed_signal ()
    {
        NEMIVER_TRY
        update_ok_button_sensitivity ();
        NEMIVER_CATCH
    }

    void update_ok_button_sensitivity ()
    {
        THROW_IF_FAIL (call_expr_entry);
        THROW_IF_FAIL (ok_button);

        if (call_expr_entry->get_entry ()->get_text ().empty ()) {
            ok_button->set_sensitive (false);
        } else {
            ok_button->set_sensitive (true);
        }
    }

    /// Tests whether if the expression exists in history.
    /// 
    /// \param a_expr the expression to look for.
    /// 
    /// \param a_iter if the function returned true and if this
    /// pointer is non-null, then *a_iter is filled with an iterator
    /// pointing at the expression found in history.
    ///
    /// \return true if the expression exists in history, false
    /// otherwise.
    bool exists_in_history (const UString &a_expr,
                            Gtk::TreeModel::iterator *a_iter = 0) const
    {
        THROW_IF_FAIL (call_expr_history);
        Gtk::TreeModel::iterator it;
        for (it = call_expr_history->children ().begin ();
             it != call_expr_history->children ().end ();
             ++it) {
            if ((*it)[get_call_expr_history_cols ().expr] == a_expr) {
                if (a_iter != 0)
                    *a_iter = it;
                return true;
            }
        }
        return false;
    }

    /// Erases an expression from expression history.
    ///
    /// \param a_iter the iterator pointing to the expression to erase
    /// from history.
    void erase_expression_from_history (Gtk::TreeModel::iterator &a_iter)
    {
        THROW_IF_FAIL (call_expr_history);
        call_expr_history->erase (a_iter);
    }

    void clear_history ()
    {
        call_expr_history->clear ();
    }

    /// Add an expression to expression history.  If the expression
    /// already exists in history, it can either be duplicated or be
    /// not.  Also, the new expression can be either appended or
    /// prepended to history.
    ///
    /// \param a_expr the expression to add to history.
    ///
    /// \param a_prepend if true, prepend the expression to history.
    ///
    /// \param allow_dups if true, allow expressions to be present in
    /// more than copy in history.
    void add_to_history (const UString &a_expr,
                         bool a_prepend = false,
                         bool allow_dups = false)
    {
        // don't append empty expressions
        if (a_expr.empty ())
            return;
        
        // If the expression already exists in history, remove it, so
        // that it can be added again, as to be the latest added item
        // to history.
        Gtk::TreeModel::iterator it;
        if (!allow_dups
            && exists_in_history (a_expr, &it))
            erase_expression_from_history (it);

        THROW_IF_FAIL (call_expr_history);
        if (a_prepend)
            it = call_expr_history->insert
                                    (call_expr_history->children ().begin ());
        else
            it = call_expr_history->append ();
        (*it)[get_call_expr_history_cols ().expr] = a_expr;
    }

    void get_history (std::list<UString> &a_hist) const
    {
        Gtk::TreeModel::iterator it;
        a_hist.clear ();
        for (it = call_expr_history->children ().begin ();
             it != call_expr_history->children ().end ();
             ++it) {
            Glib::ustring elem = (*it)[get_call_expr_history_cols ().expr];
            a_hist.push_back (elem);
        }
    }
};//end struct CallFunctionDialog::Priv

/// Constructor of the CallFunctionDialog type.
///
/// \param a_parent the parent window of the dialog.
///
/// \param a_root_path the path to the root directory of the
/// resources of the dialog.
CallFunctionDialog::CallFunctionDialog (Gtk::Window &a_parent,
                                        const UString &a_root_path):
    Dialog (a_root_path, "callfunctiondialog.ui",
            "callfunctiondialog", a_parent)
{
    m_priv.reset (new Priv (widget (), gtkbuilder ()));
}

CallFunctionDialog::~CallFunctionDialog ()
{
}

UString
CallFunctionDialog::call_expression () const
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->call_expr_entry);

    return m_priv->call_expr_entry->get_entry ()->get_text ();
}

void
CallFunctionDialog::call_expression (const UString &a_expr)
{
    if (a_expr.empty ())
        return;

    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->call_expr_entry);

    m_priv->call_expr_entry->get_entry ()->set_text (a_expr);
    add_to_history (a_expr);
}

void
CallFunctionDialog::set_history (const std::list<UString> &a_hist)
{
    THROW_IF_FAIL (m_priv);

    m_priv->clear_history ();

    list<UString>::const_iterator it;
    for (it = a_hist.begin (); it != a_hist.end (); ++it) {
        m_priv->add_to_history (*it, false /*append*/);
    }
}

void
CallFunctionDialog::get_history (std::list<UString> &a_hist) const
{
    THROW_IF_FAIL (m_priv);
    m_priv->get_history (a_hist);
}

void
CallFunctionDialog::add_to_history (const UString &a_expr)
{
    THROW_IF_FAIL (m_priv);
    m_priv->add_to_history (a_expr,
                            false /* append */,
                            false /* disallow duplicates */);
}
NEMIVER_END_NAMESPACE (nemiver)

