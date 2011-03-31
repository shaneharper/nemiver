// Author: Dodji Seketeli
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

#include <iostream>
#include <iomanip>
#include <boost/test/minimal.hpp>
#include <glibmm.h>
#include "common/nmv-initializer.h"
#include "common/nmv-safe-ptr-utils.h"
#include "common/nmv-asm-utils.h"
#include "nmv-i-debugger.h"
#include "nmv-dbg-common.h"
#include "nmv-debugger-utils.h"

using namespace nemiver;
using namespace nemiver::common;
using namespace std;

Glib::RefPtr<Glib::MainLoop> loop =
   Glib::MainLoop::create (Glib::MainContext::get_default ()) ;
IDebuggerSafePtr debugger;

static const char *PROG_TO_DEBUG = "./fooprog";
static char counter1 = 0;
static char counter2 = 0;

void
on_engine_died_signal ()
{
    loop->quit ();
    BOOST_FAIL ("engine died");
}

void
on_program_finished_signal ()
{
    MESSAGE ("program finished");
    loop->quit ();
    BOOST_REQUIRE (counter1 == 2);
    BOOST_REQUIRE (counter2 == 3);
}

typedef list<common::Asm> AsmInstrs;

void
on_instructions_disassembled_signal0 (const common::DisassembleInfo &a_info,
                                      const AsmInstrs &a_instrs,
                                      const UString &/*a_cookie*/);
void
on_instructions_disassembled_signal1 (const common::DisassembleInfo &,
                                      const AsmInstrs &a_instrs);

void
on_instructions_disassembled_signal0 (const common::DisassembleInfo &a_info,
                                      const AsmInstrs &a_instrs,
                                      const UString &/*a_cookie*/)
{
    on_instructions_disassembled_signal1 (a_info, a_instrs);
    ++counter1;
}

void
on_instructions_disassembled_signal1 (const common::DisassembleInfo &,
                                      const AsmInstrs &a_instrs)
{
    cout << "<AssemblyInstructionList nb='" << a_instrs.size ()
         << "'>" << endl;
    for (AsmInstrs::const_iterator it = a_instrs.begin ();
         it != a_instrs.end ();
         ++it) {
        cout << " <instruction>" << endl;
        cout << "  @" << setbase (16) << it->instr ().address ()
             << setbase (10) << endl;
        cout << "  func: " << it->instr ().function () << endl;
        cout << "  offset: " << it->instr ().offset () << endl;
        cout << "  instr: " << it->instr ().instruction () << endl;
        cout << " </instruction>\n";
    }
    cout << "</AssemblyInstructionList>" << endl;
    ++counter2;
}


void
on_stopped_signal (IDebugger::StopReason a_reason,
                   bool a_has_frame,
                   const IDebugger::Frame &a_frame,
                   int /*a_thread_id*/,
                   int /*a_bp_num*/,
                   const UString &/*a_cookie*/)
{
    MESSAGE ("stopped, reason is: "  << a_reason);
    if (a_reason != IDebugger::BREAKPOINT_HIT)
        return;
    BOOST_REQUIRE (a_has_frame);

    MESSAGE ("current frame: '" << a_frame.function_name () << "'");

    if (a_frame.function_name () != "main")
        return;

    BOOST_REQUIRE (debugger);

    debugger->disassemble (0, true, 20, true);
    debugger->disassemble_lines (a_frame.file_name (), a_frame.line (),
                                 20, &on_instructions_disassembled_signal1,
                                 true);
    debugger->do_continue ();
}

NEMIVER_API int
test_main (int, char**)
{
    NEMIVER_TRY

    Initializer::do_init ();

    THROW_IF_FAIL (loop);

    debugger = debugger_utils::load_debugger_iface_with_confmgr ();

    debugger->set_event_loop_context (loop->get_context ());

    //*****************************
    //<connect to IDebugger events>
    //*****************************

    debugger->engine_died_signal ().connect (&on_engine_died_signal);
    debugger->program_finished_signal ().connect
                                    (&on_program_finished_signal);
    debugger->stopped_signal ().connect (&on_stopped_signal);
    debugger->instructions_disassembled_signal ().connect
                                    (&on_instructions_disassembled_signal0);

    //*****************************
    //</connect to IDebugger events>
    //*****************************

    vector<UString> args, source_search_dir;
    source_search_dir.push_back (".");

    // load the program to debug.
    debugger->load_program (PROG_TO_DEBUG, args, ".",
                            source_search_dir, "", false);

    // set a breakpoint in the main function.
    debugger->set_breakpoint ("main");

    // run the debugger. This will not take action until we run the
    // even loop.
    debugger->run ();

    // run the even loop. This runs for ever, until someone from within
    // the loop calls loop->quit (); In which case the call returns.
    loop->run ();

    NEMIVER_CATCH_AND_RETURN_NOX (-1);

    _exit (0);
}


