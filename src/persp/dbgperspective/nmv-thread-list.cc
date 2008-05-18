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

#include <glib/gi18n.h>
#include <gtkmm/treeview.h>
#include <gtkmm/treestore.h>
#include "common/nmv-exception.h"
#include "nmv-thread-list.h"
#include "nmv-i-debugger.h"
#include "nmv-ui-utils.h"

NEMIVER_BEGIN_NAMESPACE (nemiver)

struct ThreadListColumns : public Gtk::TreeModelColumnRecord {
    Gtk::TreeModelColumn<int> thread_id ;

    ThreadListColumns ()
    {
        add (thread_id) ;
    }
};//end class ThreadListColumns

static ThreadListColumns&
thread_list_columns ()
{
    static ThreadListColumns s_thread_list_columns ;
    return s_thread_list_columns ;
}

struct ThreadList::Priv {
    IDebuggerSafePtr debugger ;
    std::list<int> thread_ids ;
    int current_thread ;
    SafePtr<Gtk::TreeView> tree_view ;
    Glib::RefPtr<Gtk::ListStore> list_store ;
    sigc::signal<void, int> thread_selected_signal ;
    int current_thread_id;
    sigc::connection tree_view_selection_changed_connection ;

    Priv (IDebuggerSafePtr &a_debugger) :
        debugger (a_debugger),
        current_thread (0),
        current_thread_id (0)
    {
        build_widget () ;
        connect_to_debugger_signals () ;
        connect_to_widget_signals () ;
    }

    void on_debugger_stopped_signal (IDebugger::StopReason a_reason,
                                     bool a_has_frame,
                                     const IDebugger::Frame &a_frame,
                                     int a_thread_id,
                                     const UString &a_cookie)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD ;
        NEMIVER_TRY
        if (a_has_frame || a_frame.level () || a_cookie.empty ()) {}

        if (a_reason == IDebugger::EXITED_SIGNALLED
            || a_reason == IDebugger::EXITED_NORMALLY) {
            return ;
        }
        current_thread_id = a_thread_id ;
        debugger->list_threads () ;
        NEMIVER_CATCH
    }

    void on_debugger_threads_listed_signal (const std::list<int> &a_threads,
                                            const UString &a_cookie)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD ;

        if (a_cookie.empty ()) {}

        NEMIVER_TRY

        clear_threads () ;
        set_thread_id_list (a_threads) ;
        select_thread_id (current_thread_id, false) ;

        NEMIVER_CATCH
    }

    void on_debugger_thread_selected_signal (int a_tid,
                                             const IDebugger::Frame &a_frame,
                                             const UString &a_cookie)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD ;

        if (a_frame.level () || a_cookie.empty ()) {}

        NEMIVER_TRY

        select_thread_id (a_tid, false) ;
        thread_selected_signal.emit (a_tid) ;

        NEMIVER_CATCH
    }

    void on_tree_view_selection_changed_signal ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD ;
        NEMIVER_TRY

        if (!tree_view) {return;}
        if (!tree_view->get_selection ()) {return;}

        Gtk::TreeModel::iterator it =
                                tree_view->get_selection ()->get_selected () ;
        if (!it) {return;}

        int thread_id = (int) it->get_value (thread_list_columns ().thread_id);
        if (thread_id <= 0) {return;}

        THROW_IF_FAIL (debugger) ;

        debugger->select_thread (thread_id) ;

        NEMIVER_CATCH
    }

    void build_widget ()
    {
        list_store = Gtk::ListStore::create (thread_list_columns ()) ;
        tree_view.reset (new Gtk::TreeView ()) ;
        tree_view->set_model (list_store) ;
        tree_view->get_selection ()->set_mode (Gtk::SELECTION_SINGLE) ;
        tree_view->append_column (_("Thread ID"),
                                  thread_list_columns ().thread_id) ;
        Gtk::TreeViewColumn *column = tree_view->get_column (0) ;
        THROW_IF_FAIL (column) ;
        column->set_clickable (false) ;
        column->set_reorderable (false) ;
    }

    void connect_to_debugger_signals ()
    {
        THROW_IF_FAIL (debugger) ;

        debugger->stopped_signal ().connect (sigc::mem_fun
            (*this, &Priv::on_debugger_stopped_signal)) ;

        debugger->threads_listed_signal ().connect (sigc::mem_fun
            (*this, &Priv::on_debugger_threads_listed_signal)) ;

        debugger->thread_selected_signal ().connect (sigc::mem_fun
            (*this, &Priv::on_debugger_thread_selected_signal)) ;
    }

    void connect_to_widget_signals ()
    {
        THROW_IF_FAIL (debugger) ;
        THROW_IF_FAIL (tree_view && tree_view->get_selection ()) ;
        tree_view_selection_changed_connection =
            tree_view->get_selection ()->signal_changed ().connect (sigc::mem_fun
                (*this, &Priv::on_tree_view_selection_changed_signal)) ;
    }

    void set_a_thread_id (int a_id)
    {
        THROW_IF_FAIL (list_store) ;
        Gtk::TreeModel::iterator iter = list_store->append () ;
        iter->set_value (thread_list_columns ().thread_id, a_id) ;
    }

    void set_thread_id_list (const std::list<int> &a_list)
    {
        std::list<int>::const_iterator it ;
        for (it = a_list.begin () ; it != a_list.end () ; ++it) {
            set_a_thread_id (*it) ;
        }
    }

    void clear_threads ()
    {
        THROW_IF_FAIL (list_store) ;
        list_store->clear () ;
    }

    void select_thread_id (int a_tid, bool a_emit_signal)
    {
        THROW_IF_FAIL (list_store) ;

        Gtk::TreeModel::iterator it  ;
        for (it = list_store->children ().begin () ;
             it != list_store->children ().end () ;
             ++it) {
            LOG_DD ("testing list row") ;
            if ((int)(*it)->get_value (thread_list_columns ().thread_id) == a_tid) {
                if (!a_emit_signal) {
                    tree_view_selection_changed_connection.block (true) ;
                }
                tree_view->get_selection ()->select (it) ;
                tree_view_selection_changed_connection.block (false) ;
            }
            LOG_DD ("tested list row") ;
        }
        current_thread_id = a_tid ;
    }
};//end ThreadList::Priv

ThreadList::ThreadList (IDebuggerSafePtr &a_debugger)
{
    m_priv.reset (new ThreadList::Priv (a_debugger));
}

ThreadList::~ThreadList ()
{
}

const list<int>&
ThreadList::thread_ids () const
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;

    THROW_IF_FAIL (m_priv) ;
    return m_priv->thread_ids ;
}

int
ThreadList::current_thread_id () const
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;

    THROW_IF_FAIL (m_priv) ;
    return m_priv->current_thread ;
}

Gtk::Widget&
ThreadList::widget () const
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;

    THROW_IF_FAIL (m_priv) ;
    return *m_priv->tree_view ;
}

void
ThreadList::clear ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;

    THROW_IF_FAIL (m_priv) ;
    if (m_priv->list_store) {
        m_priv->list_store->clear () ;
    }
    m_priv->current_thread_id = -1 ;
}

sigc::signal<void, int>&
ThreadList::thread_selected_signal () const
{
    THROW_IF_FAIL (m_priv) ;
    return m_priv->thread_selected_signal ;
}

NEMIVER_END_NAMESPACE (nemiver)

