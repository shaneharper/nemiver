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


const char* CONF_NAMESPACE_NEMIVER = "/apps/nemiver";

const char* CONF_NAMESPACE_DESKTOP_INTERFACE = "/desktop/gnome/interface";

const char* CONF_KEY_SYSTEM_FONT_NAME =
                "/desktop/gnome/interface/monospace_font_name";

/* dbgperspective */
const char* CONF_KEY_NEMIVER_SOURCE_DIRS =
                "/apps/nemiver/dbgperspective/source-search-dirs";
const char* CONF_KEY_SHOW_DBG_ERROR_DIALOGS =
                "/apps/nemiver/dbgperspective/show-dbg-error-dialogs";
const char* CONF_KEY_SHOW_SOURCE_LINE_NUMBERS =
                "/apps/nemiver/dbgperspective/show-source-line-numbers";
const char* CONF_KEY_CONFIRM_BEFORE_RELOAD_SOURCE =
                "/apps/nemiver/dbgperspective/confirm-before-reload-source";
const char* CONF_KEY_ALLOW_AUTO_RELOAD_SOURCE =
                "/apps/nemiver/dbgperspective/allow-auto-reload-source";
const char* CONF_KEY_HIGHLIGHT_SOURCE_CODE =
                "/apps/nemiver/dbgperspective/highlight-source-code";
const char* CONF_KEY_SOURCE_FILE_ENCODING_LIST =
                "/apps/nemiver/dbgperspective/source-file-encoding-list";
const char* CONF_KEY_USE_SYSTEM_FONT =
                "/apps/nemiver/dbgperspective/use-system-font";
const char* CONF_KEY_CUSTOM_FONT_NAME =
                "/apps/nemiver/dbgperspective/custom-font-name";
const char* CONF_KEY_USE_LAUNCH_TERMINAL =
                "/apps/nemiver/dbgperspective/use-launch-terminal";
const char* CONF_KEY_STATUS_WIDGET_MINIMUM_WIDTH =
                "/apps/nemiver/dbgperspective/status-widget-minimum-width";
const char* CONF_KEY_STATUS_WIDGET_MINIMUM_HEIGHT =
                "/apps/nemiver/dbgperspective/status-widget-minimum-height";
const char* CONF_KEY_DEFAULT_LAYOUT_STATUS_PANE_LOCATION =
                "/apps/nemiver/dbgperspective/default-layout-status-pane-location";
const char* CONF_KEY_WIDE_LAYOUT_STATUS_PANE_LOCATION =
                "/apps/nemiver/dbgperspective/wide-layout-status-pane-location";
const char* CONF_KEY_TWO_PANE_LAYOUT_STATUS_VPANE_LOCATION =
                "/apps/nemiver/dbgperspective/two-pane-layout-vpane-location";
const char* CONF_KEY_TWO_PANE_LAYOUT_STATUS_HPANE_LOCATION =
                "/apps/nemiver/dbgperspective/two-pane-layout-hpane-location";
const char* CONF_KEY_DEBUGGER_ENGINE_DYNMOD_NAME =
                "/apps/nemiver/dbgperspective/debugger-engine-dynmod";
const char* CONF_KEY_EDITOR_STYLE_SCHEME =
                "/apps/nemiver/dbgperspective/editor-style-scheme";
const char* CONF_KEY_ASM_STYLE_PURE =
                "/apps/nemiver/dbgperspective/asm-style-pure";
const char* CONF_KEY_DEFAULT_NUM_ASM_INSTRS =
                "/apps/nemiver/dbgperspective/default-num-asm-instrs";
const char* CONF_KEY_GDB_BINARY =
                "/apps/nemiver/dbgperspective/gdb-binary";
const char* CONF_KEY_FOLLOW_FORK_MODE =
                "/apps/nemiver/dbgperspective/follow-fork-mode";
const char* CONF_KEY_DISASSEMBLY_FLAVOR =
                "/apps/nemiver/dbgperspective/disassembly-flavor";
const char* CONF_KEY_PRETTY_PRINTING =
    "/apps/nemiver/dbgperspective/pretty-printing";

const char* CONF_KEY_CONTEXT_PANE_LOCATION =
                "/apps/nemiver/dbgperspective/context-pane-location";
const char* CONF_KEY_NEMIVER_CALLSTACK_EXPANSION_CHUNK =
                "/apps/nemiver/dbgperspective/callstack-expansion-chunk";
const char* CONF_KEY_DBG_PERSPECTIVE_LAYOUT =
                "/apps/nemiver/dbgperspective/layout";

/* profperspective */
const char* CONF_KEY_DO_CALLGRAPH_RECORDING =
                "/apps/nemiver/profperspective/do-callgraph-recording";
const char* CONF_KEY_COLLECT_WITHOUT_BUFFERING =
                "/apps/nemiver/profperspective/collect-without-buffering";
const char* CONF_KEY_COLLECT_RAW_SAMPLE_RECORDS =
                "/apps/nemiver/profperspective/collect-raw-sample-records";
const char* CONF_KEY_SAMPLE_ADDRESSES =
                "/apps/nemiver/profperspective/sample-addresses";
const char* CONF_KEY_SAMPLE_TIMESTAMPS =
                "/apps/nemiver/profperspective/sample-timestamps";

/* Workbench */
const char* CONF_KEY_NEMIVER_WINDOW_WIDTH =
                "/apps/nemiver/workbench/window-width";
const char* CONF_KEY_NEMIVER_WINDOW_HEIGHT =
                "/apps/nemiver/workbench/window-height";
const char* CONF_KEY_NEMIVER_WINDOW_POSITION_X =
                "/apps/nemiver/workbench/window-position-x";
const char* CONF_KEY_NEMIVER_WINDOW_POSITION_Y =
                "/apps/nemiver/workbench/window-position-y";
const char* CONF_KEY_NEMIVER_WINDOW_MAXIMIZED =
                "/apps/nemiver/workbench/window-maximized";
const char* CONF_KEY_NEMIVER_WINDOW_MINIMUM_WIDTH =
                "/apps/nemiver/workbench/window-minimum-width";
const char* CONF_KEY_NEMIVER_WINDOW_MINIMUM_HEIGHT =
                "/apps/nemiver/workbench/window-minimum-height";

NEMIVER_END_NAMESPACE (nemiver)
