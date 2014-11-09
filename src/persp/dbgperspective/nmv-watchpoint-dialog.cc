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
#include "nmv-watchpoint-dialog.h"
#include <gtkmm/dialog.h>
#include "common/nmv-exception.h"
#include "common/nmv-env.h"
#include "common/nmv-ustring.h"
#include "nmv-ui-utils.h"
#include "nmv-expr-inspector.h"

NEMIVER_BEGIN_NAMESPACE (nemiver)

struct WatchpointDialog::Priv {

    Gtk::Dialog &dialog;
    Glib::RefPtr<Gtk::Builder> gtkbuilder;
    Gtk::Entry *expression_entry;
    Gtk::Button *inspect_button;
    Gtk::CheckButton *read_check_button;
    Gtk::CheckButton *write_check_button;
    Gtk::Button *ok_button;
    Gtk::Button *cancel_button;
    SafePtr<ExprInspector> var_inspector;
    IDebugger &debugger;
    IPerspective &perspective;

    Priv (Gtk::Dialog &a_dialog,
          const Glib::RefPtr<Gtk::Builder> &a_gtkbuilder,
          IDebugger &a_debugger,
          IPerspective &a_perspective) :
        dialog (a_dialog),
        gtkbuilder (a_gtkbuilder),
        expression_entry (0),
        inspect_button (0),
        read_check_button (0),
        write_check_button (0),
        debugger (a_debugger),
        perspective (a_perspective)
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        build_dialog ();
        connect_to_widget_signals ();
        connect_to_debugger_signals ();
    }

    void
    build_dialog ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        dialog.set_default_response (Gtk::RESPONSE_OK);

        expression_entry =
            ui_utils::get_widget_from_gtkbuilder<Gtk::Entry>
                                            (gtkbuilder, "expressionentry");
        THROW_IF_FAIL (expression_entry);
        expression_entry->set_activates_default ();

        inspect_button =
           ui_utils::get_widget_from_gtkbuilder<Gtk::Button> (gtkbuilder, "inspectbutton");
        THROW_IF_FAIL (inspect_button);
        inspect_button->set_sensitive (false);

        read_check_button =
            ui_utils::get_widget_from_gtkbuilder<Gtk::CheckButton>
                                            (gtkbuilder, "readcheckbutton");
        THROW_IF_FAIL (read_check_button);

        write_check_button =
            ui_utils::get_widget_from_gtkbuilder<Gtk::CheckButton>
                                            (gtkbuilder, "writecheckbutton");
        THROW_IF_FAIL (write_check_button);

        ok_button =
            ui_utils::get_widget_from_gtkbuilder<Gtk::Button>
                                            (gtkbuilder, "okbutton");
        THROW_IF_FAIL (ok_button);
        ok_button->set_sensitive (false);

        cancel_button =
            ui_utils::get_widget_from_gtkbuilder<Gtk::Button>
                                            (gtkbuilder, "cancelbutton");
        THROW_IF_FAIL (cancel_button);
        cancel_button->set_sensitive (true);

        Gtk::Box *box =
            ui_utils::get_widget_from_gtkbuilder<Gtk::Box> (gtkbuilder,
                                                       "varinspectorbox");
        THROW_IF_FAIL (box);

        var_inspector.reset (new ExprInspector (debugger, perspective));
        THROW_IF_FAIL (var_inspector);

        Gtk::ScrolledWindow *scr = Gtk::manage (new Gtk::ScrolledWindow);
        scr->set_policy (Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
        scr->set_shadow_type (Gtk::SHADOW_IN);
        scr->add (var_inspector->widget ());

        box->pack_start (*scr, true, true);

        ensure_either_read_or_write_mode ();
        dialog.show_all ();
    }

    void
    connect_to_widget_signals ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        THROW_IF_FAIL (inspect_button);
        THROW_IF_FAIL (expression_entry);

        inspect_button->signal_clicked ().connect (sigc::mem_fun
               (*this, &Priv::on_inspect_button_clicked));
        expression_entry->signal_changed ().connect (sigc::mem_fun
               (*this, &Priv::on_expression_entry_changed_signal));
    }

    void
    connect_to_debugger_signals ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;
    }

    void
    ensure_either_read_or_write_mode ()
    {
        THROW_IF_FAIL (read_check_button);
        THROW_IF_FAIL (write_check_button);

        if (!read_check_button->get_active ()
            && !read_check_button->get_active ())
            write_check_button->set_active (true);
    }

    void
    on_inspect_button_clicked ()
    {
        NEMIVER_TRY

        THROW_IF_FAIL (expression_entry);
        THROW_IF_FAIL (var_inspector);

        UString expression = expression_entry->get_text ();
        if (expression == "")
            return;
        var_inspector->inspect_expression (expression);

        NEMIVER_CATCH
    }

    void
    on_expression_entry_changed_signal ()
    {
        LOG_FUNCTION_SCOPE_NORMAL_DD;

        NEMIVER_TRY

        THROW_IF_FAIL (expression_entry);
        THROW_IF_FAIL (inspect_button);

        UString expression = expression_entry->get_text ();
        if (expression == "") {
            inspect_button->set_sensitive (false);
            ok_button->set_sensitive (false);
        } else {
            inspect_button->set_sensitive (true);
            ok_button->set_sensitive (true);
        }

        NEMIVER_CATCH
    }

}; // end struct WatchpointDialog

/// Constructor of the WatchpointDialog type.
///
/// \param a_parent the parent window of the dialog.
///
/// \param a_root_path the path to the root directory of the
///
/// \param a_debugger the IDebugger interface to use.
///
/// \param a_perspective the IPerspective interface to use.
/// resources of the dialog.
WatchpointDialog::WatchpointDialog (Gtk::Window &a_parent,
                                    const UString &a_root_path,
                                    IDebugger &a_debugger,
                                    IPerspective &a_perspective) :
    Dialog (a_root_path,
            "watchpointdialog.ui",
            "watchpointdialog",
            a_parent)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD;
    m_priv.reset (new WatchpointDialog::Priv (widget (),
                                              gtkbuilder (),
                                              a_debugger,
                                              a_perspective));
}

WatchpointDialog::~WatchpointDialog ()
{
}

const UString
WatchpointDialog::expression () const
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->expression_entry);
    return m_priv->expression_entry->get_text ();
}

void
WatchpointDialog::expression (const UString &a_text)
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->expression_entry);
    return m_priv->expression_entry->set_text (a_text);
}

WatchpointDialog::Mode
WatchpointDialog::mode () const
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->read_check_button);
    THROW_IF_FAIL (m_priv->write_check_button);

    Mode mode = UNDEFINED_MODE;

    if (m_priv->write_check_button->get_active ())
        mode |= WRITE_MODE;

    if (m_priv->read_check_button->get_active ())
        mode |= READ_MODE;

    return mode;
}

void
WatchpointDialog::mode (Mode a_mode)
{
    THROW_IF_FAIL (m_priv);
    THROW_IF_FAIL (m_priv->read_check_button);
    THROW_IF_FAIL (m_priv->write_check_button);

    if ((a_mode & WRITE_MODE) == WRITE_MODE)
        m_priv->write_check_button->set_active (true);
    else
        m_priv->write_check_button->set_active (false);

    if ((a_mode & READ_MODE) == READ_MODE)
        m_priv->read_check_button->set_active (true);
    else
        m_priv->read_check_button->set_active (false);

    m_priv->ensure_either_read_or_write_mode ();
}

WatchpointDialog::Mode
operator| (WatchpointDialog::Mode a_l,
           WatchpointDialog::Mode a_r)
{
    return (a_l |= a_r);
}

WatchpointDialog::Mode
operator& (WatchpointDialog::Mode a_l,
           WatchpointDialog::Mode a_r)
{
    return (a_l &= a_r);
}

WatchpointDialog::Mode
operator~ (WatchpointDialog::Mode a_m)
{
    return static_cast<WatchpointDialog::Mode> (~a_m);
}

WatchpointDialog::Mode&
operator|= (WatchpointDialog::Mode &a_l,
            WatchpointDialog::Mode a_r)
{
    a_l = static_cast<WatchpointDialog::Mode>
        (static_cast<unsigned> (a_l) | static_cast<unsigned> (a_r));
    return a_l;
}

WatchpointDialog::Mode&
operator&= (WatchpointDialog::Mode &a_l,
            WatchpointDialog::Mode a_r)
{
    a_l = static_cast<WatchpointDialog::Mode>
            (static_cast<unsigned> (a_l) & static_cast<unsigned> (a_r));
    return a_l;
}

NEMIVER_END_NAMESPACE (nemiver)

