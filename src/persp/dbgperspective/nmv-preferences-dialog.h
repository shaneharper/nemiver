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
#ifndef __NMV_PREFERENCES_DIALOG_H__
#define __NMV_PREFERENCES_DIALOG_H__

#include <vector>
#include "common/nmv-ustring.h"
#include "nmv-dialog.h"

using nemiver::common::UString;

NEMIVER_BEGIN_NAMESPACE (nemiver)

class IWorkbench;
class PreferencesDialog : public Dialog {

    class Priv;
    SafePtr<Priv> m_priv;

    PreferencesDialog ();

public:
    PreferencesDialog (IWorkbench &a_workbench,
                       const UString &a_root_path);
    virtual ~PreferencesDialog ();
    const std::vector<UString>& source_directories () const;
    void source_directories (const std::vector<UString> &a_dirs);
};//end class PreferencesDialog

NEMIVER_END_NAMESPACE (nemiver)

#endif //__NMV_PREFERENCES_DIALOG
