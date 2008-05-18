//Author: Jonathon Jongsma
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
#include "common/nmv-exception.h"
#include "nmv-registers-view.h"
#include "nmv-ui-utils.h"
#include "nmv-i-workbench.h"
#include "nmv-i-perspective.h"

namespace nemiver {

struct RegisterColumns : public Gtk::TreeModelColumnRecord {
    Gtk::TreeModelColumn<IDebugger::register_id_t> id ;
    Gtk::TreeModelColumn<Glib::ustring> name ;
    Gtk::TreeModelColumn<Glib::ustring> value ;
    Gtk::TreeModelColumn<Gdk::Color> fg_color;

    RegisterColumns ()
    {
        add (id) ;
        add (name) ;
        add (value) ;
        add (fg_color);
    }
};//end Cols

static RegisterColumns&
get_columns ()
{
    static RegisterColumns s_cols ;
    return s_cols ;
}

struct RegistersView::Priv {
public:
    SafePtr<Gtk::TreeView> tree_view ;
    Glib::RefPtr<Gtk::ListStore> list_store ;
    IDebuggerSafePtr& debugger;

    Priv (IDebuggerSafePtr& a_debugger) :
        debugger(a_debugger)
    {
        build_tree_view () ;

        // update breakpoint list when debugger indicates that the list of
        // breakpoints has changed.
        debugger->register_names_listed_signal ().connect (sigc::mem_fun
                (*this, &Priv::on_debugger_registers_listed)) ;
        debugger->changed_registers_listed_signal ().connect (sigc::mem_fun
                (*this, &Priv::on_debugger_changed_registers_listed)) ;
        debugger->register_values_listed_signal ().connect (sigc::mem_fun
                (*this, &Priv::on_debugger_register_values_listed)) ;
        debugger->register_value_changed_signal ().connect (sigc::mem_fun
                (*this, &Priv::on_debugger_register_value_changed)) ;

        debugger->stopped_signal ().connect (sigc::mem_fun
                (*this, &Priv::on_debugger_stopped));
    }

    void build_tree_view ()
    {
        if (tree_view) {return;}
        //create a default tree store and a tree view
        list_store = Gtk::ListStore::create (get_columns ()) ;
        tree_view.reset (new Gtk::TreeView (list_store)) ;

        //create the columns of the tree view
        tree_view->append_column (_("ID"), get_columns ().id) ;
        tree_view->append_column (_("Name"), get_columns ().name) ;
        tree_view->append_column_editable (_("Value"), get_columns ().value) ;
        Gtk::TreeViewColumn * col = tree_view->get_column (2) ;
        col->add_attribute (*col->get_first_cell_renderer (),
                            "foreground-gdk",
                            get_columns ().fg_color) ;
        Gtk::CellRendererText* renderer = dynamic_cast<Gtk::CellRendererText*>
            (col->get_first_cell_renderer ());
        THROW_IF_FAIL (renderer);
        renderer->signal_edited ().connect (sigc::mem_fun (this,
                    &Priv::on_register_value_edited));
    }

    void on_debugger_stopped (IDebugger::StopReason a_reason,
                              bool,
                              const IDebugger::Frame &,
                              int,
                              const UString&)
    {
        if (a_reason == IDebugger::EXITED_SIGNALLED
            || a_reason == IDebugger::EXITED_NORMALLY
            || a_reason == IDebugger::EXITED) {
            return ;
        }
        static bool first_run = true;
        if (first_run) {
            first_run = false;
            debugger->list_register_names ();
        }
        else {
            debugger->list_changed_registers ();
        }
    }

    void on_debugger_registers_listed
                            (const map<IDebugger::register_id_t, UString> &a_regs,
                             const UString &a_cookie)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        NEMIVER_TRY
        THROW_IF_FAIL (list_store);
        if (a_cookie.empty ()) {}
        list_store->clear ();
        LOG_DD ("got num registers: " << (int)a_regs.size ());
        for (std::map<IDebugger::register_id_t, UString>::const_iterator reg_iter = a_regs.begin ();
                reg_iter != a_regs.end (); ++reg_iter) {
            Gtk::TreeModel::iterator tree_iter = list_store->append ();
            (*tree_iter)[get_columns ().id] = reg_iter->first;
            (*tree_iter)[get_columns ().name] = reg_iter->second;
            LOG_DD ("got register: " << reg_iter->second);
        }
        debugger->list_register_values ("first-time");
        NEMIVER_CATCH
    }

    void on_debugger_changed_registers_listed
            (std::list<IDebugger::register_id_t> a_regs,
             const UString &a_cookie)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        NEMIVER_TRY
        if (a_cookie.empty()) {}
        if (!a_regs.empty ()) {
            debugger->list_register_values (a_regs);
        }

        NEMIVER_CATCH
    }

    void on_debugger_register_values_listed
                            (const map<IDebugger::register_id_t, UString> &a_reg_values,
                             const UString &a_cookie)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        NEMIVER_TRY
        for (Gtk::TreeModel::iterator tree_iter = list_store->children ().begin ();
                tree_iter != list_store->children ().end (); ++tree_iter) {
            IDebugger::register_id_t id = (*tree_iter)[get_columns ().id];
            std::map<IDebugger::register_id_t, UString>::const_iterator value_iter = a_reg_values.find (id);
            if (value_iter != a_reg_values.end ())
            {
                (*tree_iter)[get_columns ().value] = value_iter->second;
                if (a_cookie != "first-time") {
                    set_changed (tree_iter);
                } else {
                    set_changed (tree_iter, false);
                }
            }
            else
            {
                set_changed (tree_iter, false);
            }
        }
        NEMIVER_CATCH
    }

    void on_register_value_edited (const Glib::ustring& a_path,
                                   const Glib::ustring& a_new_val)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        Gtk::TreeModel::iterator tree_iter = list_store->get_iter (a_path);
        Glib::ustring reg_name = (*tree_iter)[get_columns ().name];
        LOG_DD ("setting register " << reg_name << " to " << a_new_val);
        debugger->set_register_value (reg_name, a_new_val);

        // FIXME: this is an ugly hack to keep the treeview in sync with the
        // current value of the register.  At present, we can't easily tell if
        // setting of the register has failed. We don't want to just leave the
        // user-entered value in the treeview or the user will falsely assume
        // that she has successfully modified the register value even on
        // failures.  So until we have proper error detection and can present a
        // message to the user indicating that setting the register has failed,
        // we must read back the register value immediately after setting it.
        std::list<IDebugger::register_id_t> regs;
        regs.push_back ((tree_iter->get_value (get_columns ().id)));
        debugger->list_register_values (regs);
    }

    void on_debugger_register_value_changed (const Glib::ustring& a_register_name,
                                    const Glib::ustring& a_new_value,
                                    const Glib::ustring& a_cookie)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        if (a_cookie.empty ()) {}
        for (Gtk::TreeModel::iterator tree_iter = list_store->children ().begin ();
                tree_iter != list_store->children ().end (); ++tree_iter) {
            if ((*tree_iter)[get_columns ().name] == a_register_name)
            {
                // no need to update if the value is the same as last time
                if ((*tree_iter)[get_columns ().value] == a_new_value) {
                    (*tree_iter)[get_columns ().value] = a_new_value;
                    set_changed (tree_iter);
                }
                break;
            }
        }
    }

    // helper function which highlights a row in red or returns the text to
    // normal color to indicate whether it has changed since last update
    void set_changed (Gtk::TreeModel::iterator& iter, bool changed = true)
    {
        if (changed)
        {
            (*iter)[get_columns ().fg_color]  = Gdk::Color ("red");
        }
        else
        {
            (*iter)[get_columns ().fg_color] =
                tree_view->get_style ()->get_text (Gtk::STATE_NORMAL);
        }
    }

};//end class RegistersView::Priv

RegistersView::RegistersView (IDebuggerSafePtr& a_debugger)
{
    m_priv.reset (new Priv (a_debugger));
}

RegistersView::~RegistersView ()
{
    LOG_D ("deleted", "destructor-domain") ;
}

Gtk::Widget&
RegistersView::widget () const
{
    THROW_IF_FAIL (m_priv) ;
    THROW_IF_FAIL (m_priv->tree_view) ;
    THROW_IF_FAIL (m_priv->list_store) ;
    return *m_priv->tree_view ;
}

void
RegistersView::clear ()
{
    THROW_IF_FAIL (m_priv && m_priv->list_store) ;
    m_priv->list_store->clear ();
}

}//end namespace nemiver

