/*******************************************************************************
 *  PROJECT: Nemiver
 *
 *  AUTHOR: Jonathon Jongsma
 *  See COPYRIGHT file copyright information.
 *
 *  License:
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the
 *    Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 *    Boston, MA  02111-1307  USA
 *
 *******************************************************************************/
#include <sstream>
#include <bitset>
#include <iomanip>
#include <gtkmm/entry.h>
#include <gtkmm/label.h>
#include <gtkmm/box.h>
#include <glib/gi18n.h>
#include <gtkmm/scrolledwindow.h>
#include "nmv-ui-utils.h"
#include "nmv-memory-view.h"
#include "nmv-i-debugger.h"
#include "uicommon/nmv-hex-editor.h"

namespace nemiver {

class GroupingComboBox : public Gtk::ComboBox
{
    public:
        GroupingComboBox ()
        {
            m_model = Gtk::ListStore::create (m_cols);
            THROW_IF_FAIL (m_model);
            Gtk::TreeModel::iterator iter = m_model->append ();
            (*iter)[m_cols.name] = _("Byte");
            (*iter)[m_cols.group_type] = GROUP_BYTE;
            iter = m_model->append ();
            (*iter)[m_cols.name] = _("Word");
            (*iter)[m_cols.group_type] = GROUP_WORD;
            iter = m_model->append ();
            (*iter)[m_cols.name] = _("Long Word");
            (*iter)[m_cols.group_type] = GROUP_LONG;
            set_model (m_model);
            pack_start (m_cols.name);
            set_active (0);
        }

        guint get_group_type () const
        {
            Gtk::TreeModel::iterator iter = get_active ();
            if (iter)
            {
                return (*iter)[m_cols.group_type];
            }
            else return GROUP_BYTE;
        }

    private:
        Glib::RefPtr<Gtk::ListStore> m_model;
        struct GroupModelColumns : public Gtk::TreeModelColumnRecord
        {
            GroupModelColumns () {add (name); add (group_type);}
            Gtk::TreeModelColumn<Glib::ustring> name;
            Gtk::TreeModelColumn<guint> group_type;
        } m_cols;

};

struct MemoryView::Priv {
public:
    SafePtr<Gtk::Label> m_address_label;
    SafePtr<Gtk::Entry> m_address_entry;
    SafePtr<Gtk::Button> m_jump_button;
    SafePtr<Gtk::HBox> m_hbox;
    SafePtr<Gtk::VBox> m_container;
    SafePtr<Gtk::Label> m_group_label;
    GroupingComboBox m_grouping_combo;
    SafePtr<Gtk::ScrolledWindow> m_scrolledwindow;
    Hex::DocumentSafePtr m_document;
    Hex::EditorSafePtr m_editor;
    IDebuggerSafePtr m_debugger;
    sigc::connection signal_document_changed_connection;

    Priv (IDebuggerSafePtr& a_debugger) :
        m_address_label (new Gtk::Label (_("Address:"))),
        m_address_entry (new Gtk::Entry ()),
        m_jump_button (new Gtk::Button (_("Show"))),
        m_hbox (new Gtk::HBox ()),
        m_container (new Gtk::VBox ()),
        m_group_label (new Gtk::Label (_("Group By:"))),
        m_scrolledwindow (new Gtk::ScrolledWindow ()),
        m_document (Hex::Document::create ()),
        m_editor (Hex::Editor::create (m_document)),
        m_debugger (a_debugger)
    {
        m_editor->set_geometry (20 /*characters per line*/, 6 /*lines*/);
        m_editor->show_offsets ();
        m_scrolledwindow->set_policy (Gtk::POLICY_NEVER, Gtk::POLICY_NEVER);
        m_editor->get_widget ().set_border_width (0);
        m_scrolledwindow->add (m_editor->get_widget ());
        m_scrolledwindow->set_shadow_type (Gtk::SHADOW_IN);

        m_hbox->set_spacing (6);
        m_hbox->set_border_width (3);
        m_hbox->pack_start (*m_address_label, Gtk::PACK_SHRINK);
        m_hbox->pack_start (*m_address_entry, Gtk::PACK_SHRINK);
        m_hbox->pack_start (*m_group_label, Gtk::PACK_SHRINK);
        m_hbox->pack_start (m_grouping_combo, Gtk::PACK_SHRINK);
        m_hbox->pack_start (*m_jump_button, Gtk::PACK_SHRINK);
        m_container->pack_start (*m_hbox, Gtk::PACK_SHRINK);
        m_container->pack_start (*m_scrolledwindow);

        connect_signals ();
    }

    void connect_signals ()
    {
        THROW_IF_FAIL (m_debugger);
        m_debugger->state_changed_signal ().connect (sigc::mem_fun (this,
                    &Priv::on_debugger_state_changed));
        m_debugger->stopped_signal ().connect (sigc::mem_fun
                (this, &Priv::on_debugger_stopped));
        m_debugger->read_memory_signal ().connect (sigc::mem_fun (this,
                    &Priv::on_memory_read_response));
        THROW_IF_FAIL (m_jump_button);
        m_jump_button->signal_clicked ().connect (sigc::mem_fun (this,
                    &Priv::do_memory_read));
        m_grouping_combo.signal_changed ().connect (sigc::mem_fun (this,
                    &Priv::on_group_changed));
        THROW_IF_FAIL (m_address_entry);
        m_address_entry->signal_activate ().connect (sigc::mem_fun (this,
                    &Priv::do_memory_read));
        THROW_IF_FAIL (m_document);
        signal_document_changed_connection =
            m_document->signal_document_changed ().connect (sigc::mem_fun (this,
                        &Priv::on_document_changed));
    }

    void on_debugger_state_changed (IDebugger::State a_state)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        NEMIVER_TRY
        THROW_IF_FAIL (m_address_entry);
        switch (a_state)
        {
            case IDebugger::READY:
                set_widgets_sensitive (true);
                break;
            default:
                set_widgets_sensitive (false);
        }
        NEMIVER_CATCH
    }

    void do_memory_read ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        NEMIVER_TRY
        THROW_IF_FAIL (m_debugger);
        THROW_IF_FAIL (m_editor);
        int editor_cpl, editor_lines;
        m_editor->get_geometry (editor_cpl, editor_lines);
        size_t addr = get_address ();
        if (validate_address (addr))
        {
            LOG_DD ("Fetching " << editor_cpl * editor_lines << " bytes");
            // read as much memory as will fill the hex editor widget
            m_debugger->read_memory (addr, editor_cpl * editor_lines);
        }
        NEMIVER_CATCH
    }

    void on_group_changed ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        NEMIVER_TRY
        THROW_IF_FAIL (m_editor);
        m_editor->set_group_type (m_grouping_combo.get_group_type ());
        NEMIVER_CATCH
    }

    void on_debugger_stopped (IDebugger::StopReason a_reason/*a_reason*/,
                              bool /*a_has_frame*/,
                              const IDebugger::Frame& /*a_frame*/,
                              int /*a_thread_id*/,
                              const UString& /*a_cookie*/)
    {
        NEMIVER_TRY

        if (a_reason == IDebugger::EXITED_SIGNALLED
            || a_reason == IDebugger::EXITED_NORMALLY
            || a_reason == IDebugger::EXITED) {
            return ;
        }
        do_memory_read ();

        NEMIVER_CATCH
    }

    size_t get_address ()
    {
        THROW_IF_FAIL (m_address_entry);
        std::istringstream istream (m_address_entry->get_text ());
        size_t addr=0;
        istream >> std::hex >> addr;
        return addr;
    }

    bool validate_address (size_t a_addr)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        // FIXME: implement validation
        if (a_addr)
        {
            return true;
        }
        return false;
    }

    void set_widgets_sensitive (bool a_enable = true)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        THROW_IF_FAIL (m_address_entry && m_jump_button);
        m_address_entry->set_sensitive (a_enable);
        m_jump_button->set_sensitive (a_enable);
        m_editor->get_widget ().set_sensitive (a_enable);
    }

    void on_memory_read_response (size_t a_addr,
                                  const std::vector<uint8_t> &a_values,
                                  const UString& /*a_cookie*/)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        NEMIVER_TRY
        THROW_IF_FAIL (m_address_entry);
        ostringstream addr;
        addr << std::showbase << std::hex << a_addr;
        m_address_entry->set_text (addr.str ());
        set_data (a_addr, a_values);
        NEMIVER_CATCH
    }

    void set_data (size_t a_start_addr, const std::vector<uint8_t> &a_data)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        THROW_IF_FAIL (m_document);
        // don't want to set memory in gdb in response to data read from gdb
        signal_document_changed_connection.block ();
        m_document->clear ();
        m_editor->set_starting_offset (a_start_addr);
        m_document->set_data (0 /*offset*/,
                              a_data.size (),
                              0 /*rep_len*/,
                              &a_data[0]);
        signal_document_changed_connection.unblock ();
    }

    void on_document_changed (HexChangeData* a_change_data)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
        size_t length = a_change_data->end - a_change_data->start + 1;
        guchar* new_data = m_document->get_data (a_change_data->start, length);
        if (new_data)
        {
            std::vector<uint8_t> data(new_data, new_data + length);
            // set data in the debugger
            m_debugger->set_memory(static_cast<size_t> (get_address () + a_change_data->start),
                    data);
        }
    }

};

MemoryView::MemoryView (IDebuggerSafePtr& a_debugger) :
    m_priv (new Priv(a_debugger))
{
}

MemoryView::~MemoryView ()
{}

Gtk::Widget&
MemoryView::widget () const
{
    THROW_IF_FAIL (m_priv && m_priv->m_container);
    return *m_priv->m_container;
}

void
MemoryView::clear ()
{
    THROW_IF_FAIL (m_priv && m_priv->m_document && m_priv->m_address_entry);
    m_priv->m_document->set_data (0, 0, 0, 0, false);
    m_priv->m_address_entry->set_text ("");
}

void
MemoryView::modify_font (const Pango::FontDescription& a_font_desc)
{
    THROW_IF_FAIL (m_priv && m_priv->m_editor);
    m_priv->m_editor->set_font (a_font_desc);
}

} // namespace nemiver
