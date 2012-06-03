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

#include "nmv-call-list.h"
#include "common/nmv-exception.h"
#include "uicommon/nmv-ui-utils.h"
#include <gtkmm/treeview.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/treestore.h>
#include <gtkmm/scrolledwindow.h>
#include <glib/gi18n.h>

NEMIVER_BEGIN_NAMESPACE (nemiver)

struct CallListColumns : public Gtk::TreeModel::ColumnRecord
{
    CallListColumns ()
    {
        add (symbol);
        add (overhead);
        add (call_node);
        add (command);
        add (dso);
    }

    Gtk::TreeModelColumn<CallGraphNodeSafePtr> call_node;
    Gtk::TreeModelColumn<Glib::ustring> symbol;
    Gtk::TreeModelColumn<float> overhead;
    Gtk::TreeModelColumn<Glib::ustring> command;
    Gtk::TreeModelColumn<Glib::ustring> dso;
};

struct CallList::Priv {
    Gtk::TreeView treeview;
    Gtk::ScrolledWindow body;
    CallListColumns columns;
    Glib::RefPtr<Gtk::TreeStore> store;
    IProfilerSafePtr profiler;

    Priv (const IProfilerSafePtr &a_profiler) :
        profiler (a_profiler)
    {
        body.add (treeview);
        body.set_policy (Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
        body.show_all ();

        store = Gtk::TreeStore::create (columns);

        Gtk::CellRendererProgress *renderer =
            Gtk::manage (new Gtk::CellRendererProgress);
        THROW_IF_FAIL (renderer);

        treeview.append_column (_("Symbol"), columns.symbol);
        treeview.append_column (_("Command"), columns.command);
        treeview.append_column (_("Shared Object"), columns.dso);
        int usage_col_id = treeview.append_column (_("Overhead"), *renderer);
        treeview.set_model (store);

        Gtk::TreeViewColumn *column = treeview.get_column (usage_col_id - 1);
        if (column) {
            column->add_attribute
                (renderer->property_value(), columns.overhead);
        }
    }

    void
    add_node (CallGraphNodeSafePtr a_call_graph_node,
              Gtk::TreeModel::iterator a_parent = Gtk::TreeModel::iterator ())
    {
        const std::list<CallGraphNodeSafePtr>& children =
            a_call_graph_node->children ();
        for (std::list<CallGraphNodeSafePtr>::const_iterator iter =
                 children.begin ();
             iter != children.end ();
             ++iter) {

            Gtk::TreeModel::iterator row;
            if (a_parent) {
                row = store->append (a_parent->children ());
            } else {
                row = store->append ();
            }

            if (row) {
                (*row)[columns.symbol] = (*iter)->symbol ();
                (*row)[columns.overhead] = (*iter)->overhead ();
                (*row)[columns.command] = (*iter)->command ();
                (*row)[columns.dso] = (*iter)->shared_object ();
                (*row)[columns.call_node] = *iter;
            }

            add_node (*iter, row);
        }
    }
};

Gtk::Widget& CallList::widget () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->body;
}

void
CallList::load_call_graph (CallGraphSafePtr a_call_graph)
{
    THROW_IF_FAIL (m_priv);
    m_priv->add_node(a_call_graph);
}

CallList::CallList (const IProfilerSafePtr &a_profiler) :
    m_priv (new Priv (a_profiler))
{
}

CallList::~CallList ()
{
}

NEMIVER_END_NAMESPACE (nemiver)

