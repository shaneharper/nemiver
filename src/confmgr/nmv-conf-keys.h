//Author: Jonathon Jongsma, Dodji Seketeli
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

#ifndef __NMV_CONF_KEYS_H__
#define __NMV_CONF_KEYS_H__

#include "common/nmv-namespace.h"

NEMIVER_BEGIN_NAMESPACE (nemiver)

extern const char* CONF_NAMESPACE_NEMIVER;

extern const char* CONF_NAMESPACE_DESKTOP_INTERFACE;

extern const char* CONF_KEY_SYSTEM_FONT_NAME;

extern const char* CONF_KEY_NEMIVER_SOURCE_DIRS;
extern const char* CONF_KEY_SHOW_DBG_ERROR_DIALOGS;
extern const char* CONF_KEY_SHOW_SOURCE_LINE_NUMBERS;
extern const char* CONF_KEY_CONFIRM_BEFORE_RELOAD_SOURCE;
extern const char* CONF_KEY_ALLOW_AUTO_RELOAD_SOURCE;
extern const char* CONF_KEY_HIGHLIGHT_SOURCE_CODE;
extern const char* CONF_KEY_SOURCE_FILE_ENCODING_LIST;
extern const char* CONF_KEY_USE_SYSTEM_FONT;
extern const char* CONF_KEY_CUSTOM_FONT_NAME;
extern const char* CONF_KEY_USE_LAUNCH_TERMINAL;
extern const char* CONF_KEY_STATUS_WIDGET_MINIMUM_WIDTH;
extern const char* CONF_KEY_STATUS_WIDGET_MINIMUM_HEIGHT;
extern const char* CONF_KEY_DEFAULT_LAYOUT_STATUS_PANE_LOCATION;
extern const char* CONF_KEY_WIDE_LAYOUT_STATUS_PANE_LOCATION;
extern const char* CONF_KEY_TWO_PANE_LAYOUT_STATUS_HPANE_LOCATION;
extern const char* CONF_KEY_TWO_PANE_LAYOUT_STATUS_VPANE_LOCATION;
extern const char* CONF_KEY_DEBUGGER_ENGINE_DYNMOD_NAME;
extern const char* CONF_KEY_EDITOR_STYLE_SCHEME;
extern const char* CONF_KEY_ASM_STYLE_PURE;
extern const char* CONF_KEY_GDB_BINARY;
extern const char* CONF_KEY_DEFAULT_NUM_ASM_INSTRS;
extern const char* CONF_KEY_FOLLOW_FORK_MODE;
extern const char* CONF_KEY_DISASSEMBLY_FLAVOR;
extern const char* CONF_KEY_PRETTY_PRINTING;
extern const char* CONF_KEY_CONTEXT_PANE_LOCATION;
extern const char* CONF_KEY_NEMIVER_CALLSTACK_EXPANSION_CHUNK;
extern const char* CONF_KEY_DBG_PERSPECTIVE_LAYOUT;

extern const char* CONF_KEY_DO_CALLGRAPH_RECORDING;
extern const char* CONF_KEY_COLLECT_WITHOUT_BUFFERING;
extern const char* CONF_KEY_COLLECT_RAW_SAMPLE_RECORDS;
extern const char* CONF_KEY_SAMPLE_ADDRESSES;
extern const char* CONF_KEY_SAMPLE_TIMESTAMPS;

extern const char* CONF_KEY_NEMIVER_WINDOW_WIDTH;
extern const char* CONF_KEY_NEMIVER_WINDOW_HEIGHT;
extern const char* CONF_KEY_NEMIVER_WINDOW_POSITION_X;
extern const char* CONF_KEY_NEMIVER_WINDOW_POSITION_Y;
extern const char* CONF_KEY_NEMIVER_WINDOW_MAXIMIZED;
extern const char* CONF_KEY_NEMIVER_WINDOW_MINIMUM_WIDTH;
extern const char* CONF_KEY_NEMIVER_WINDOW_MINIMUM_HEIGHT;

NEMIVER_END_NAMESPACE (nemiver)

#endif // __NMV_CONF_KEYS_H__
