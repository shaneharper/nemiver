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
#ifndef __NMV_HEX_DOCUMENT_H__
#define __NMV_HEX_DOCUMENT_H__

#include "common/nmv-object.h"
#include "common/nmv-safe-ptr-utils.h"
#include "common/nmv-ustring.h"
#include <gtkhex/hex-document.h>
#include <sigc++/signal.h>

namespace Gtk {
    class Widget;
}
using nemiver::common::Object;
using nemiver::common::SafePtr;
using nemiver::common::ObjectRef;
using nemiver::common::ObjectUnref;
using nemiver::common::UString;

NEMIVER_BEGIN_NAMESPACE (nemiver)
NEMIVER_BEGIN_NAMESPACE (Hex)

class Document;
typedef SafePtr<Document, ObjectRef, ObjectUnref> DocumentSafePtr;

class Document : public Object {
    class Priv;
    SafePtr<Priv> m_priv;

protected:
    Document ();
    Document (const std::string& filename);

public:
    virtual ~Document ();
    static DocumentSafePtr create ();
    static DocumentSafePtr create (const std::string& filename);
    ::HexDocument* cobj();
    void set_data (guint offset,
                   guint len,
                   guint rep_len,
                   const guchar *data,
                   bool undoable=false);
    guchar *get_data (guint offset, guint len);
    void delete_data (guint offset, guint len, bool undoable=false);
    void clear (bool undoable=false);
    sigc::signal<void, HexChangeData*>& signal_document_changed () const;
};//end class Spinner

NEMIVER_END_NAMESPACE (Hex)
NEMIVER_END_NAMESPACE (nemiver)
#endif //__NMV_HEX_DOCUMENT_H__

