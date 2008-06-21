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
#include <locale.h>
#include <glibmm.h>
#include <libxml/parser.h>
#include "nmv-initializer.h"
#include "nmv-conf-manager.h"

namespace nemiver {
namespace common {

Initializer::Initializer ()
{
    setlocale (LC_ALL, "") ;
    Glib::init ();
    Glib::thread_init () ;
    ConfManager::init () ;
}

Initializer::~Initializer ()
{
    xmlCleanupParser () ;
}

void
Initializer::do_init ()
{
    static Initializer s_init ;
}

}//end namespace common
}//end namespace nemiver

