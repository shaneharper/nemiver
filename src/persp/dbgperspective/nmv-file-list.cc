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
#include "config.h"
#include <vector>
#include <glib/gi18n.h>
#include <gtkmm/treeview.h>
#include <gtkmm/treestore.h>
#include <gtkmm/scrolledwindow.h>
#include "common/nmv-exception.h"
#include "nmv-file-list.h"
#include "nmv-ui-utils.h"
#include "nmv-i-debugger.h"

namespace nemiver {

struct FileListColumns : public Gtk::TreeModel::ColumnRecord {
    Gtk::TreeModelColumn<Glib::ustring> display_name;
    Gtk::TreeModelColumn<Glib::ustring> path;
    Gtk::TreeModelColumn<Gtk::StockID> stock_icon;

    FileListColumns ()
    {
        add (display_name);
        add (path);
        add (stock_icon);
    }
};

class FileListView : public Gtk::TreeView {
public:
    FileListView ();
    virtual ~FileListView ();

    void set_files (const std::vector<UString> &a_files);
    void get_selected_filenames (vector<string> &a_filenames) const;
    void expand_to_filename (const UString &a_filename);

    sigc::signal<void,
                 const UString&> file_activated_signal;
    sigc::signal<void> files_selected_signal;

protected:
    typedef std::pair<UString, Gtk::TreeModel::iterator> StringTreeIterPair;
    struct ComparePathMap :
        public std::binary_function<const StringTreeIterPair,
                                    const UString, bool> {
        bool
        operator() (const StringTreeIterPair &a_map,
                    const UString &a_path_component)
        {
            return a_path_component == a_map.first;
        }
    };//end struct ComparePathMap

    Gtk::TreeModel::iterator find_filename_recursive
                                (const Gtk::TreeModel::iterator &a_iter,
                                 const UString &a_filename);

    virtual void on_row_activated (const Gtk::TreeModel::Path& path,
                                   Gtk::TreeViewColumn* column);
    virtual void on_file_list_selection_changed ();
    virtual bool on_button_press_event (GdkEventButton *ev);
    virtual bool on_key_press_event (GdkEventKey *ev);
    virtual void on_menu_popup_expand_clicked ();
    virtual void on_menu_popup_expand_all_clicked ();
    virtual void on_menu_popup_collapse_clicked ();

    void expand_selected (bool recursive, bool collapse_if_expanded);

    FileListColumns m_columns;

    Glib::RefPtr<Gtk::TreeStore> m_tree_model;

    Gtk::Menu m_menu_popup;
}; // end class FileListView

FileListView::FileListView ()
{
    // create the tree model:
    m_tree_model = Gtk::TreeStore::create (m_columns);
    set_model (m_tree_model);

    set_headers_visible (false);

    // create the columns of the tree view
    Gtk::TreeViewColumn* view_column = new Gtk::TreeViewColumn (_("File Name"));
    Gtk::CellRendererPixbuf renderer_pixbuf;
    Gtk::CellRendererText renderer_text;

    view_column->pack_start (renderer_pixbuf, false /* don't expand */);
    view_column->add_attribute (renderer_pixbuf,
                                "stock-id",
                                m_columns.stock_icon);
    view_column->pack_start (renderer_text);
    view_column->add_attribute (renderer_text, "text", m_columns.display_name);
    append_column (*view_column);

    // set and handle selection
    get_selection ()->set_mode (Gtk::SELECTION_MULTIPLE);
    get_selection ()->signal_changed ().connect (
        sigc::mem_fun (*this, &FileListView::on_file_list_selection_changed));

    // fill popup menu:
    Gtk::MenuItem *menu_item;

    menu_item = Gtk::manage (new Gtk::MenuItem (_("Expand _Selected"), true));
    menu_item->signal_activate ().connect (
        sigc::mem_fun (*this, &FileListView::on_menu_popup_expand_clicked));
    m_menu_popup.append (*menu_item);
    menu_item->show ();

    menu_item = Gtk::manage (new Gtk::MenuItem (_("Expand _All"), true));
    menu_item->signal_activate ().connect (
        sigc::mem_fun (*this, &FileListView::on_menu_popup_expand_all_clicked));
    m_menu_popup.append (*menu_item);
    menu_item->show ();

    menu_item = Gtk::manage (new Gtk::SeparatorMenuItem ());
    m_menu_popup.append (*menu_item);
    menu_item->show ();

    menu_item = Gtk::manage (new Gtk::MenuItem (_("_Collapse"), true));
    menu_item->signal_activate ().connect (
        sigc::mem_fun (*this, &FileListView::on_menu_popup_collapse_clicked));
    m_menu_popup.append (*menu_item);
    menu_item->show ();

    m_menu_popup.accelerate (*this);
}

FileListView::~FileListView ()
{
}

void
FileListView::set_files (const std::vector<UString> &a_files)
{
    // NOTE: This assumes a sorted file list.  If the file list is not
    // sorted, this function will not work as expected

    THROW_IF_FAIL (m_tree_model);
    if (!(m_tree_model->children ().empty ())) {
        m_tree_model->clear();
    }

    // a map vector to keep track of what directory levels we have most
    // recently added so that its easier to figure out where new paths need
    // to be added to the tree structure
    typedef std::pair<UString, Gtk::TreeModel::iterator> folder_map_t;
    typedef vector<folder_map_t> folder_map_list_t;
    folder_map_list_t folder_map;

    std::vector<UString>::const_iterator file_iter;
    for (file_iter = a_files.begin ();
         file_iter != a_files.end ();
         ++file_iter) {
        vector<UString> path_components;
        // only add absolute paths to the treeview
        if (Glib::path_is_absolute (*file_iter)) {
            path_components = (*file_iter).split (G_DIR_SEPARATOR_S);
            // find the first point where the last added path and the new
            // path do not match
            pair<folder_map_list_t::iterator, vector<UString>::iterator>
                mismatch_iter =
                std::mismatch(folder_map.begin (),
                              folder_map.end (), path_components.begin (),
                              ComparePathMap());
            // ASSUMPTION: there will always be a mismatch found before we
            // run out of components of the new path since all paths in the
            // list should be unique
            THROW_IF_FAIL(mismatch_iter.second != path_components.end ());
            // truncate the folder_map vector at the point of the mismatch
            folder_map.erase (mismatch_iter.first, folder_map.end ());

            // loop through any remaining new path elements and add them to
            // the tree
            for (vector<UString>::iterator iter =
                     mismatch_iter.second; iter !=
                     path_components.end (); ++iter) {
                Gtk::TreeModel::iterator tree_iter;
                if (folder_map.empty ()) {
                    // add as children of the root level
                    tree_iter = m_tree_model->append ();
                } else {
                    // add as children of the last matched item
                    // in the folder map
                    tree_iter = m_tree_model->append (
                        folder_map.rbegin ()->second->children ());
                }
                // build the full path name up to this element
                Glib::ustring path = "/" + // add back the root element
                    Glib::build_filename
                            (std::vector<UString> (path_components.begin(),
                                                   iter + 1));// we want
                                                              // to include
                                                              // this iter
                                                              // element in
                                                              // the array,
                                                              // so + 1
                (*tree_iter)[m_columns.path] = path;
                Glib::ustring display_name =
                    Glib::filename_display_name
                                            (Glib::locale_to_utf8 (*iter));
                (*tree_iter)[m_columns.display_name] =
                    display_name.empty () ? "/" : display_name;
                if (Glib::file_test(path, Glib::FILE_TEST_IS_REGULAR)) {
                    (*tree_iter)[m_columns.stock_icon] = Gtk::Stock::FILE;
                } else if (Glib::file_test(path, Glib::FILE_TEST_IS_DIR)) {
                    (*tree_iter)[m_columns.stock_icon] =
                                                        Gtk::Stock::DIRECTORY;
                }
                // store the element in the folder
                // map for use next time around
                folder_map.push_back (folder_map_t(*iter, tree_iter));
            }
        }
    }
}

void
FileListView::get_selected_filenames (vector<string> &a_filenames) const
{
    Glib::RefPtr<const Gtk::TreeSelection> selection = get_selection ();
    THROW_IF_FAIL (selection);
    vector<Gtk::TreeModel::Path> paths = selection->get_selected_rows ();

    for (vector<Gtk::TreeModel::Path>::iterator path_iter = paths.begin ();
         path_iter != paths.end ();
         ++path_iter) {
        Gtk::TreeModel::iterator tree_iter =
            (m_tree_model->get_iter(*path_iter));
        a_filenames.push_back (UString((*tree_iter)[m_columns.path]));
    }
}

void
FileListView::on_row_activated (const Gtk::TreeModel::Path &a_path,
                                Gtk::TreeViewColumn *a_col)
{
    NEMIVER_TRY

    if (!a_col) {return;}
    Gtk::TreeIter it = m_tree_model->get_iter (a_path);

    if (!it) {return;}
    Glib::ustring path = (*it)[m_columns.path];

    file_activated_signal.emit (path);

    NEMIVER_CATCH
}

void
FileListView::on_file_list_selection_changed ()
{
    NEMIVER_TRY

    if (!get_selection ()->count_selected_rows ()) {
        return;
    }

    files_selected_signal.emit ();

    NEMIVER_CATCH
}

bool
FileListView::on_button_press_event (GdkEventButton* event)
{
    // first call base class handler, to allow normal events
    bool return_value = TreeView::on_button_press_event (event);

    // then do our custom stuff
    if ((event->type == GDK_BUTTON_PRESS) && (event->button == 3)) {
        m_menu_popup.popup (event->button, event->time);
    } else if ((event->type == GDK_2BUTTON_PRESS) && (event->button == 1)) {
        // double click
        bool recursive = false;
        bool collapse_if_expanded = true;
        expand_selected (recursive, collapse_if_expanded);
    }

    return return_value;
}

bool
FileListView::on_key_press_event (GdkEventKey* event)
{
    bool return_value = TreeView::on_key_press_event (event);

    if ((event->type == GDK_KEY_PRESS) && (event->keyval == GDK_KEY_Return)) {
        bool recursive = (event->state & GDK_CONTROL_MASK);
        bool collapse_if_expanded = true;
        expand_selected (recursive, collapse_if_expanded);
    }

    return return_value;
}

void
FileListView::on_menu_popup_expand_clicked ()
{
    bool recursive = true;
    bool dont_collapse = false;
    expand_selected (recursive, dont_collapse);
}
void
FileListView::on_menu_popup_expand_all_clicked ()
{
    expand_all();
}

void
FileListView::on_menu_popup_collapse_clicked ()
{
    collapse_all();
}

void
FileListView::expand_selected (bool recursive, bool collapse_if_expanded)
{
    Glib::RefPtr<Gtk::TreeView::Selection> selection = get_selection ();

    if (selection) {
        vector<Gtk::TreeModel::Path> paths = selection->get_selected_rows ();

        for (vector<Gtk::TreeModel::Path>::iterator path_iter = paths.begin ();
             path_iter != paths.end ();
             ++path_iter) {
            Gtk::TreeModel::iterator tree_iter =
                (m_tree_model->get_iter (*path_iter));

            if (Glib::file_test (UString ((*tree_iter)[m_columns.path]),
                                 Glib::FILE_TEST_IS_DIR)) {
                if ((row_expanded(*path_iter)) && collapse_if_expanded) {
                    collapse_row(*path_iter);
                } else {
                    expand_row (*path_iter, recursive);
                }
            }
        }
    }
}

void
FileListView::expand_to_filename (const UString &a_filename)
{
    Gtk::TreeModel::iterator tree_iter;
    for (tree_iter = m_tree_model->children ().begin ();
         tree_iter != m_tree_model->children ().end ();
         ++tree_iter) {
        Gtk::TreeModel::iterator iter =
                    find_filename_recursive (tree_iter, a_filename);
        if (iter) {
            Gtk::TreeModel::Path path(iter);
            expand_to_path (path);
            // Scroll to the directory that contains the file
            path.up ();
            scroll_to_row (path);
            break;
        }
    }
}

Gtk::TreeModel::iterator
FileListView::find_filename_recursive (const Gtk::TreeModel::iterator &a_iter,
                                       const UString &a_filename)
{
    Gtk::TreeModel::iterator tree_iter;
    // first check the iter we were passed
    if ((*a_iter)[m_columns.path] == a_filename) {
        return a_iter;
    }
    // then check all of its children
    if (!a_iter->children ().empty ()) {
        for (tree_iter = a_iter->children ().begin ();
             tree_iter != a_iter->children ().end ();
             ++tree_iter) {
            Gtk::TreeModel::iterator child_iter =
                        find_filename_recursive (tree_iter, a_filename);
            if (child_iter) {
                return child_iter;
            }
            // else continue on to the next child
        }
    }
    // if we get to this point without having found it, return an invalid iter
    return Gtk::TreeModel::iterator();
}

struct FileList::Priv : public sigc::trackable {
public:
    SafePtr<Gtk::VBox> vbox;
    SafePtr<Gtk::ScrolledWindow> scrolled_window;
    SafePtr<Gtk::Label> loading_indicator;
    SafePtr<FileListView> tree_view;

    Glib::RefPtr<Gtk::ActionGroup> file_list_action_group;
    IDebuggerSafePtr debugger;
    UString start_path;

    Priv (IDebuggerSafePtr &a_debugger, const UString &a_starting_path) :
        vbox (new Gtk::VBox()),
        scrolled_window (new Gtk::ScrolledWindow ()),
        loading_indicator (new Gtk::Label (_("Loading files from target executable..."))),
        debugger (a_debugger),
        start_path (a_starting_path)
    {
        build_tree_view ();
        vbox->pack_start (*loading_indicator, Gtk::PACK_SHRINK, 3 /*padding*/);
        vbox->pack_start (*scrolled_window);
        scrolled_window->set_policy (Gtk::POLICY_AUTOMATIC,
                                     Gtk::POLICY_AUTOMATIC);
        scrolled_window->set_shadow_type (Gtk::SHADOW_IN);
        scrolled_window->add (*tree_view);
        scrolled_window->show ();
        vbox->show ();
        debugger->files_listed_signal ().connect(
            sigc::mem_fun(*this, &FileList::Priv::on_files_listed_signal));
    }

    void build_tree_view ()
    {
        if (tree_view) {return;}
        tree_view.reset (new FileListView ());
        tree_view->show ();
    }

    void show_loading_indicator ()
    {
        loading_indicator->show ();
    }

    void stop_loading_indicator ()
    {
        loading_indicator->hide ();
    }

    void on_files_listed_signal (const vector<UString> &a_files,
                                 const UString &/*a_cookie*/)
    {
        NEMIVER_TRY

        THROW_IF_FAIL (tree_view);

        stop_loading_indicator ();
        tree_view->set_files (a_files);
        // this signal should only be called once per dialog
        // -- the first time
        // it loads up the list of files from the debugger.
        // So it should be OK
        // to expand to the 'starting path' in this handler
        tree_view->expand_to_filename (start_path);

        NEMIVER_CATCH
    }

};//end class FileList::Priv

FileList::FileList (IDebuggerSafePtr &a_debugger,
                    const UString &a_starting_path)
{
    m_priv.reset (new Priv (a_debugger, a_starting_path));
}

FileList::~FileList ()
{
    LOG_D ("deleted", "destructor-domain");
}

Gtk::Widget&
FileList::widget () const
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->tree_view);
    return *m_priv->vbox;
}

void
FileList::update_content ()
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->debugger);
    // set some placeholder text to indicate that we're loading files
    m_priv->show_loading_indicator ();
    m_priv->debugger->list_files ();
}

sigc::signal<void, const UString&>&
FileList::file_activated_signal () const
{
    THROW_IF_FAIL(m_priv);
    THROW_IF_FAIL (m_priv->tree_view);
    return m_priv->tree_view->file_activated_signal;
}

sigc::signal<void>&
FileList::files_selected_signal () const
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->tree_view);
    return m_priv->tree_view->files_selected_signal;
}

void
FileList::get_filenames (vector<string> &a_filenames) const
{
    THROW_IF_FAIL (m_priv);
    m_priv->tree_view->get_selected_filenames (a_filenames);
}

void
FileList::expand_to_filename (const UString &a_filename)
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->tree_view);
    m_priv->tree_view->expand_to_filename (a_filename);
}

}//end namespace nemiver
