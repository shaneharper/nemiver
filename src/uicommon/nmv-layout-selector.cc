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
#include "nmv-layout-selector.h"
#include "nmv-layout-manager.h"
#include "nmv-layout.h"
#include "nmv-ui-utils.h"
#include "common/nmv-exception.h"
#include "persp/nmv-i-perspective.h"
#include "confmgr/nmv-i-conf-mgr.h"
#include "confmgr/nmv-conf-keys.h"

#include <gtkmm/treemodel.h>
#include <gtkmm/treeview.h>
#include <gtkmm/liststore.h>

NEMIVER_BEGIN_NAMESPACE (nemiver)

struct LayoutModelColumns : public Gtk::TreeModel::ColumnRecord {
    LayoutModelColumns ()
    {
        add (is_selected);
        add (name);
        add (description);
        add (identifier);
    }

    Gtk::TreeModelColumn<bool> is_selected;
    Gtk::TreeModelColumn<Glib::ustring> name;
    Gtk::TreeModelColumn<Glib::ustring> description;
    Gtk::TreeModelColumn<Glib::ustring> identifier;
};

/// \brief Widget to change the layout of a perspective
struct LayoutSelector::Priv {
    IPerspective &perspective;
    Gtk::TreeView treeview;
    LayoutModelColumns model;
    LayoutManager &layout_manager;

    void
    on_layout_toggled (const Glib::ustring &a_str)
    {
        Glib::RefPtr<Gtk::TreeModel> tree_model = treeview.get_model ();
        THROW_IF_FAIL (tree_model);

        Gtk::TreePath path (a_str);
        Gtk::TreeModel::iterator iter = tree_model->get_iter (path);
        THROW_IF_FAIL (iter);

        (*iter)[model.is_selected] = true;

        // de-select every other layout but the one the user just selected.
        for (Gtk::TreeModel::iterator i = tree_model->children ().begin ();
             i != tree_model->children ().end ();
             ++i) {
            if (*i == *iter)
                continue;

            (*i)[model.is_selected] = false;
        }

        UString identifier = (Glib::ustring) (*iter)[model.identifier];
        layout_manager.load_layout (identifier, perspective);

        NEMIVER_TRY
        IConfMgrSafePtr conf_mgr = perspective.get_workbench ()
            .get_configuration_manager ();
        THROW_IF_FAIL (conf_mgr);
        conf_mgr->set_key_value (CONF_KEY_DBG_PERSPECTIVE_LAYOUT, identifier);
        NEMIVER_CATCH
    }

    void
    on_cell_rendering (Gtk::CellRenderer *a_renderer,
                      const Gtk::TreeModel::iterator &a_iter)
    {
        THROW_IF_FAIL (a_renderer);
        THROW_IF_FAIL (a_iter);

        Gtk::CellRendererText *text_renderer =
            dynamic_cast<Gtk::CellRendererText*> (a_renderer);
        THROW_IF_FAIL (text_renderer);

        text_renderer->property_markup () =
            Glib::ustring::compose (
                        "<b>%1</b>\n%2",
                        Glib::Markup::escape_text ((*a_iter)[model.name]),
                        Glib::Markup::escape_text ((*a_iter)[model.description]));
    }

    Priv (LayoutManager &a_layout_manager, IPerspective &a_perspective) :
        perspective (a_perspective),
        layout_manager (a_layout_manager)
    {
        init ();
    }

    void
    init ()
    {
        treeview.set_headers_visible (false);

        Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create (model);
        treeview.set_model (store);

        treeview.append_column_editable ("", model.is_selected);
        treeview.append_column ("", model.name);

        Gtk::CellRendererToggle *toggle_renderer =
            dynamic_cast<Gtk::CellRendererToggle*>
                (treeview.get_column_cell_renderer (0));
        THROW_IF_FAIL (toggle_renderer);
        toggle_renderer->set_radio (true);
        toggle_renderer->signal_toggled ().connect
            (sigc::mem_fun (*this, &LayoutSelector::Priv::on_layout_toggled));

        Gtk::CellRenderer *renderer = dynamic_cast<Gtk::CellRendererText*>
            (treeview.get_column_cell_renderer (1));
        THROW_IF_FAIL (renderer);

        treeview.get_column (1)->set_cell_data_func (*renderer, sigc::mem_fun
            (*this, &LayoutSelector::Priv::on_cell_rendering));

        fill_tree_view (store);
    }

    void
    fill_tree_view (const Glib::RefPtr<Gtk::ListStore> &a_store)
    {
        Layout* current_layout = layout_manager.layout ();
        std::vector<Layout*> layouts = layout_manager.layouts ();
        for (std::vector<Layout*>::iterator i = layouts.begin ();
             i != layouts.end ();
             ++i) {
            THROW_IF_FAIL (*i);

            Gtk::TreeModel::Row row = *a_store->append ();
            row[model.is_selected] = false;
            row[model.name] = (*i)->name ();
            row[model.description] = (*i)->description ();
            row[model.identifier] = (*i)->identifier ();

            if (current_layout
                && (*i)->identifier () == current_layout->identifier ()) {
                row[model.is_selected] = true;
            }
        }
    }
};//end struct LayoutSelector::Priv

LayoutSelector::LayoutSelector (LayoutManager &a_layout_manager,
                                IPerspective &a_perspective) :
    m_priv (new Priv (a_layout_manager, a_perspective))
{
}

/// \brief gets the widget
/// \return the widget
Gtk::Widget&
LayoutSelector::widget () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->treeview;
}

LayoutSelector::~LayoutSelector ()
{
    LOG_D ("deleted", "destructor-domain");
}

NEMIVER_END_NAMESPACE (nemiver)
