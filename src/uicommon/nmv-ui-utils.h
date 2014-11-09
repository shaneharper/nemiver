/* -*- Mode: C++; indent-tabs-mode:nil; c-basic-offset:4; -*- */

// Author: Dodji Seketeli

/*
 *This file is part of the Nemiver Project.
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
#ifndef __NMV_UI_UTILS_H__
#define __NMV_UI_UTILS_H__

#include "common/nmv-api-macros.h"
#include <gtkmm.h>
#include "common/nmv-env.h"
#include "common/nmv-ustring.h"
#include "common/nmv-safe-ptr-utils.h"

#ifndef NEMIVER_CATCH
#define NEMIVER_CATCH \
} catch (Glib::Exception &e) { \
    LOG_ERROR (std::string ("caught exception: '") + e.what () + "'"); \
    nemiver::ui_utils::display_error_not_transient (e.what ()); \
} catch (std::exception &e) { \
    LOG_ERROR (std::string ("caught exception: '") + e.what () + "'"); \
    nemiver::ui_utils::display_error_not_transient (e.what ()); \
} catch (...) { \
    LOG_ERROR ("caught unknown exception"); \
    nemiver::ui_utils::display_error_not_transient ("An unknown error occurred"); \
}
#endif

#ifndef NEMIVER_CATCH_AND_RETURN
#define NEMIVER_CATCH_AND_RETURN(a_value) \
} catch (Glib::Exception &e) { \
    LOG_ERROR (std::string ("caught exception: '") + e.what () + "'"); \
    nemiver::ui_utils::display_error_not_transient (e.what ()); \
    return a_value; \
} catch (std::exception &e) { \
    LOG_ERROR (std::string ("caught exception: '") + e.what () + "'"); \
    nemiver::ui_utils::display_error_not_transient (e.what ()); \
    return a_value; \
} catch (...) { \
    LOG_ERROR ("Caught unknown exception"); \
    nemiver::ui_utils::display_error_not_transient ("An unknown error occurred"); \
    return a_value; \
}
#endif

using nemiver::common::UString;

namespace nemiver {
namespace ui_utils {

class ActionEntry {

public:
    enum Type {
        DEFAULT=0,
        TOGGLE
    };

    common::UString m_name;
    Gtk::StockID m_stock_id;
    common::UString m_label;
    common::UString m_tooltip;
    sigc::slot<void> m_activate_slot;
    Type m_type;
    common::UString m_accel;
    bool m_is_important;

    Glib::RefPtr<Gtk::Action> to_action () const
    {
        Glib::RefPtr<Gtk::Action> result;
        switch (m_type) {
            case DEFAULT:
                if (m_stock_id.get_string () != "") {
                    result =
                        Gtk::Action::create (m_name, m_stock_id,
                                             m_label, m_tooltip);
                } else {
                    result =
                        Gtk::Action::create (m_name, m_label, m_tooltip);
                }
                break;
            case TOGGLE:
                if (m_stock_id.get_string () != "") {
                    result =
                        Gtk::ToggleAction::create (m_name, m_stock_id,
                                                   m_label, m_tooltip);
                } else {
                    result =
                        Gtk::ToggleAction::create (m_name,
                                                   m_label,
                                                   m_tooltip);
                }
                break;

            default:
                THROW ("should never reach this point");
        }

        if (result)
            result->set_is_important (m_is_important);

        return result;
    }
};//end class ActionEntry

NEMIVER_API void add_action_entries_to_action_group
                                (const ActionEntry a_tab[],
                                 int a_num_entries,
                                 Glib::RefPtr<Gtk::ActionGroup> &a_group);

NEMIVER_API int display_info (Gtk::Window &a_parent_window,
                              const common::UString &a_message);

NEMIVER_API int display_warning (Gtk::Window &a_parent_window,
                                 const common::UString &a_message);

NEMIVER_API int display_error (Gtk::Window &a_parent_window,
                               const common::UString &a_message);

NEMIVER_API int display_error_not_transient (const UString &a_message);

NEMIVER_API int ask_yes_no_question (Gtk::Window &a_parent_window,
                                     const common::UString &a_message);

NEMIVER_API int ask_yes_no_question (Gtk::Window &a_parent_window,
                                     const common::UString &a_message,
                                     bool a_propose_dont_ask_question,
                                     bool &a_dont_ask_this_again);

NEMIVER_API int ask_yes_no_cancel_question (Gtk::Window &a_parent_window,
                                            const common::UString &a_message);

NEMIVER_API bool ask_user_to_select_file (Gtk::Window &a_parent,
                                          const UString &a_file_name,
                                          const UString &a_default_dir,
                                          UString &a_selected_file_path);

NEMIVER_API bool find_file_or_ask_user (Gtk::Window &a_parent_window,
                                        const UString& a_file_path,
                                        const list<UString> &a_where_to_look,
                                        list<UString> &a_session_dirs,
                                        map<UString, bool> &a_ignore_paths,
                                        bool a_ignore_if_not_found,
                                        UString& a_absolute_path);


bool find_file_and_read_line (Gtk::Window &a_parent_window,
                              const UString &a_file_path,
                              const list<UString> &a_where_to_look,
                              list<UString> &a_sess_dirs,
                              map<UString, bool> &a_ignore_paths,
                              int a_line_number,
                              string &a_line);

template <class T>
T*
get_widget_from_gtkbuilder (const Glib::RefPtr<Gtk::Builder> &a_gtkbuilder,
                            const UString &a_widget_name)
{
    T *widget;
    a_gtkbuilder->get_widget (a_widget_name, widget);
    if (!widget) {
        THROW ("couldn't find widget '"
               + a_widget_name);
    }
    return widget;
}


struct WidgetRef {
    void operator () (Gtk::Widget *a_widget)
    {
        if (a_widget) {
            a_widget->reference ();
        }
    }
};//end struct WidgetRef

struct WidgetUnref {
    void operator () (Gtk::Widget *a_widget)
    {
        if (a_widget) {
            a_widget->unreference ();
        }
    }
};//end struct WidgetUnref
}//end namespace ui_utils
}//end namespace nemiver

#endif// __NMV_UI_UTILS_H__

