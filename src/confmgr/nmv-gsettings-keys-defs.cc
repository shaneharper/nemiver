// Author: Fabien Parent
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
#include "nmv-conf-keys.h"

NEMIVER_BEGIN_NAMESPACE (nemiver)

const char* CONF_NAMESPACE_NEMIVER = "org.nemiver";

const char* CONF_NAMESPACE_DESKTOP_INTERFACE = "org.gnome.desktop.interface";

const char* CONF_KEY_SYSTEM_FONT_NAME = "monospace-font-name";

/* dbgperspective */
const char* CONF_KEY_NEMIVER_SOURCE_DIRS = "source-search-dirs";
const char* CONF_KEY_SHOW_DBG_ERROR_DIALOGS = "show-dbg-error-dialogs";
const char* CONF_KEY_SHOW_SOURCE_LINE_NUMBERS = "show-source-line-numbers";
const char* CONF_KEY_CONFIRM_BEFORE_RELOAD_SOURCE =
                "confirm-before-reload-source";
const char* CONF_KEY_ALLOW_AUTO_RELOAD_SOURCE = "allow-auto-reload-source";
const char* CONF_KEY_HIGHLIGHT_SOURCE_CODE = "highlight-source-code";
const char* CONF_KEY_SOURCE_FILE_ENCODING_LIST = "source-file-encoding-list";
const char* CONF_KEY_USE_SYSTEM_FONT = "use-system-font";
const char* CONF_KEY_CUSTOM_FONT_NAME = "custom-font-name";
const char* CONF_KEY_USE_LAUNCH_TERMINAL = "use-launch-terminal";
const char* CONF_KEY_STATUS_WIDGET_MINIMUM_WIDTH =
                "status-widget-minimum-width";
const char* CONF_KEY_STATUS_WIDGET_MINIMUM_HEIGHT =
                "status-widget-minimum-height";
const char* CONF_KEY_STATUS_PANE_LOCATION = "status-pane-location";
const char* CONF_KEY_DEBUGGER_ENGINE_DYNMOD_NAME = "debugger-engine-dynmod";
const char* CONF_KEY_EDITOR_STYLE_SCHEME = "editor-style-scheme";
const char* CONF_KEY_ASM_STYLE_PURE = "asm-style-pure";
const char* CONF_KEY_DEFAULT_NUM_ASM_INSTRS = "default-num-asm-instrs";
const char* CONF_KEY_GDB_BINARY = "gdb-binary";
const char* CONF_KEY_FOLLOW_FORK_MODE = "follow-fork-mode";
const char* CONF_KEY_CONTEXT_PANE_LOCATION = "context-pane-location";
const char* CONF_KEY_NEMIVER_CALLSTACK_EXPANSION_CHUNK =
                "callstack-expansion-chunk";


/* Workbench */
const char* CONF_KEY_NEMIVER_WINDOW_WIDTH = "window-width";
const char* CONF_KEY_NEMIVER_WINDOW_HEIGHT = "window-height";
const char* CONF_KEY_NEMIVER_WINDOW_POSITION_X = "window-position-x";
const char* CONF_KEY_NEMIVER_WINDOW_POSITION_Y = "window-position-y";
const char* CONF_KEY_NEMIVER_WINDOW_MAXIMIZED = "window-maximized";
const char* CONF_KEY_NEMIVER_WINDOW_MINIMUM_WIDTH = "window-minimum-width";
const char* CONF_KEY_NEMIVER_WINDOW_MINIMUM_HEIGHT = "window-minimum-height";

NEMIVER_END_NAMESPACE (nemiver)
