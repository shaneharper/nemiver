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
#ifndef __NMV_DBG_COMMON_H_H__
#define __NMV_DBG_COMMON_H_H__
#include "nmv-i-debugger.h"

NEMIVER_BEGIN_NAMESPACE (nemiver)

/// \brief A container of the textual command sent to the debugger
class Command {
    UString m_cookie;
    UString m_name;
    UString m_value;
    UString m_tag0;
    UString m_tag1;
    int m_tag2;
    UString m_tag3;
    UString m_tag4;
    IDebugger::VariableSafePtr m_var;
    sigc::slot_base m_slot;

public:

    Command ()  :
        m_tag2 (0),
        m_slot (0)
    {
        clear ();
    }

    /// \param a_value a textual command to send to the debugger.
    Command (const UString &a_value) :
        m_value (a_value),
        m_tag2 (0),
        m_slot (0)
    {
    }

    Command (const UString &a_name, const UString &a_value) :
        m_name (a_name),
        m_value (a_value),
        m_tag2 (0),
        m_slot (0)
    {
    }

    Command (const UString &a_name,
             const UString &a_value,
             const UString &a_cookie) :
        m_cookie (a_cookie),
        m_name (a_name),
        m_value (a_value),
        m_tag2 (0),
        m_slot (0)
    {
    }

    /// \name accesors

    /// @{

    const UString& name () const {return m_name;}
    void name (const UString &a_in) {m_name = a_in;}

    const UString& value () const {return m_value;}
    void value (const UString &a_in) {m_value = a_in;}

    const UString& cookie () const {return m_cookie;}
    void cookie (const UString &a_in) {m_cookie = a_in;}

    const UString& tag0 () const {return m_tag0;}
    void tag0 (const UString &a_in) {m_tag0 = a_in;}

    const UString& tag1 () const {return m_tag1;}
    void tag1 (const UString &a_in) {m_tag1 = a_in;}

    int tag2 () const {return m_tag2;}
    void tag2 (int a_in) {m_tag2 = a_in;}

    const UString& tag3 () const {return m_tag3;}
    void tag3 (const UString &a_in) {m_tag3 = a_in;}

    const UString& tag4 () const {return m_tag4;}
    void tag4 (const UString &a_in) {m_tag4 = a_in;}

    void variable (const IDebugger::VariableSafePtr a_in) {m_var = a_in;}
    IDebugger::VariableSafePtr variable () const {return m_var;}

    bool has_slot () const
    {
        return m_slot;
    }

    template<class T>
    void set_slot (T &a_slot)
    {
        m_slot = a_slot;
    }

    template<class T>
    const T& get_slot () const
    {
        return reinterpret_cast<const T&> (m_slot);
    }

    /// @}

    void clear ()
    {
        m_name.clear ();
        m_value.clear ();
        m_tag0.clear ();
        m_tag1.clear ();
        m_tag2 = 0;
        m_tag3.clear ();
        m_tag4.clear ();
    }

};//end class Command

/// \brief the output received from the debugger.
///
/// This is tightly modeled after the interface exposed
/// by gdb, but I hope it is generic enough to serve for several
/// debugging engines.
/// See the documentation of GDB/MI for more.
class Output {
public:

    /// \brief debugger stream record.
    ///
    ///either the output stream of the
    ///debugger console (to be displayed by the CLI),
    ///or the target output stream or the debugger log stream.
    class StreamRecord {
        UString m_debugger_console;
        UString m_target_output;
        UString m_debugger_log;

    public:

        StreamRecord () {clear ();}
        StreamRecord (const UString &a_debugger_console,
                      const UString &a_target_output,
                      const UString &a_debugger_log) :
            m_debugger_console (a_debugger_console),
            m_target_output (a_target_output),
            m_debugger_log (a_debugger_log)
        {}

        /// \name accessors

        /// @{
        const UString& debugger_console () const {return m_debugger_console;}
        UString& debugger_console () {return m_debugger_console;}
        void debugger_console (const UString &a_in)
        {
            m_debugger_console = a_in;
        }

        const UString& target_output () const {return m_target_output;}
        void target_output (const UString &a_in) {m_target_output = a_in;}

        const UString& debugger_log () const {return m_debugger_log;}
        void debugger_log (const UString &a_in) {m_debugger_log = a_in;}
        /// @}

        void clear ()
        {
            m_debugger_console = "";
            m_target_output = "";
            m_debugger_log = "";
        }
    };//end class StreamRecord


    /// \brief the out of band record we got from GDB.
    ///
    ///Out of band record is either
    ///a set of messages sent by gdb
    ///to tell us about the reason why the target has stopped,
    ///or, a stream record.
    class OutOfBandRecord {
    public:


    private:
        bool m_has_stream_record;
        StreamRecord m_stream_record;
        bool m_is_stopped;
	bool m_is_running;
        IDebugger::StopReason m_stop_reason;
        bool m_has_frame;
        IDebugger::Frame m_frame;
        long m_breakpoint_number;
        long m_thread_id;
        UString m_signal_type;
        UString m_signal_meaning;

    public:

        OutOfBandRecord () {clear ();}

        UString stop_reason_to_string (IDebugger::StopReason a_reason) const
        {
            UString result ("undefined");

            switch (a_reason) {
                case IDebugger::UNDEFINED_REASON:
                    return "undefined";
                    break;
                case IDebugger::BREAKPOINT_HIT:
                    return "breakpoint-hit";
                    break;
                case IDebugger::WATCHPOINT_TRIGGER:
                    return "watchpoint-trigger";
                    break;
                case IDebugger::READ_WATCHPOINT_TRIGGER:
                    return "read-watchpoint-trigger";
                    break;
                case IDebugger::ACCESS_WATCHPOINT_TRIGGER:
                    return "access-watchpoint-trigger";
                    break;
                case IDebugger::FUNCTION_FINISHED:
                    return "function-finished";
                    break;
                case IDebugger::LOCATION_REACHED:
                    return "location-reached";
                    break;
                case IDebugger::WATCHPOINT_SCOPE:
                    return "watchpoint-scope";
                    break;
                case IDebugger::END_STEPPING_RANGE:
                    return "end-stepping-range";
                    break;
                case IDebugger::EXITED_SIGNALLED:
                    return "exited-signaled";
                    break;
                case IDebugger::EXITED:
                    return "exited";
                    break;
                case IDebugger::EXITED_NORMALLY:
                    return "exited-normally";
                    break;
                case IDebugger::SIGNAL_RECEIVED:
                    return "signal-received";
                    break;
            }
            return result;
        }

        /// \accessors


        /// @{
        bool has_stream_record () const {return m_has_stream_record;}
        void has_stream_record (bool a_in) {m_has_stream_record = a_in;}

        const StreamRecord& stream_record () const {return m_stream_record;}
        StreamRecord& stream_record () {return m_stream_record;}
        void stream_record (const StreamRecord &a_in) {m_stream_record=a_in;}

        bool is_stopped () const {return m_is_stopped;}
        void is_stopped (bool a_in) {m_is_stopped = a_in;}

	bool is_running () const {return m_is_running;}
        void is_running (bool a) {m_is_running = a;}

        IDebugger::StopReason stop_reason () const {return m_stop_reason;}
        UString stop_reason_as_str () const
        {
            return stop_reason_to_string (m_stop_reason);
        }
        void stop_reason (IDebugger::StopReason a_in) {m_stop_reason = a_in;}

        bool has_frame () const {return m_has_frame;}
        void has_frame (bool a_in) {m_has_frame = a_in;}

        const IDebugger::Frame& frame () const {return m_frame;}
        IDebugger::Frame& frame () {return m_frame;}
        void frame (const IDebugger::Frame &a_in) {m_frame = a_in;}

        long breakpoint_number () const {return m_breakpoint_number;}
        void breakpoint_number (long a_in) {m_breakpoint_number = a_in;}

        long thread_id () const {return m_thread_id;}
        void thread_id (long a_in) {m_thread_id = a_in;}

        const UString& signal_type () const {return m_signal_type;}
        void signal_type (const UString &a_in) {m_signal_type = a_in;}

        const UString& signal_meaning () const {return m_signal_meaning;}
        void signal_meaning (const UString &a_in) {m_signal_meaning = a_in;}

        bool has_signal () const {return m_signal_type != "";}

        /// @}

        void clear ()
        {
            m_has_stream_record = false;
            m_stream_record.clear ();
            m_is_stopped = false;
            m_is_running = false;
            m_stop_reason = IDebugger::UNDEFINED_REASON;
            m_has_frame = false;
            m_frame.clear ();
            m_breakpoint_number = 0;
            m_thread_id = -1;
            m_signal_type.clear ();
        }
    };//end class OutOfBandRecord

    /// \debugger result record
    ///
    /// this is a notification of gdb that about the last operation
    /// he was asked to perform. This basically can be either
    /// "running" --> the operation was successfully started, the target
    /// is running.
    /// "done" --> the synchronous operation was successful. In this case,
    /// gdb also returns the result of the operation.
    /// "error" the operation failed. In this case, gdb returns the
    /// corresponding error message.
    class ResultRecord {
    public:
        enum Kind {
            UNDEFINED=0,
            DONE,
            RUNNING,
            CONNECTED,
            ERROR,
            EXIT
        };//end enum Kind

    private:
        Kind m_kind;
        map<int, IDebugger::Breakpoint> m_breakpoints;
        map<UString, UString> m_attrs;

        //call stack listed members
        vector<IDebugger::Frame> m_call_stack;
        bool m_has_call_stack;

        //frame parameters listed members
        map<int, list<IDebugger::VariableSafePtr> > m_frames_parameters;
        bool m_has_frames_parameters;

        //local variable listed members
        list<IDebugger::VariableSafePtr> m_local_variables;
        bool m_has_local_variables;

        //variable value evaluated members
        IDebugger::VariableSafePtr m_variable_value;
        bool m_has_variable_value;

        //threads listed members
        std::list<int> m_thread_list;
        bool m_has_thread_list;

        //files listed members
        std::vector<UString> m_file_list;
        bool m_has_file_list;

        //new thread id selected members
        int m_thread_id;
        IDebugger::Frame m_frame_in_thread;
        bool m_thread_id_got_selected;
        //TODO: finish (re)initialisation of thread id selected members

        //current frame, in the context of a core file stack trace.
        IDebugger::Frame m_current_frame_in_core_stack_trace;
        bool m_has_current_frame_in_core_stack_trace;

        // register names
        std::map<IDebugger::register_id_t, UString> m_register_names;
        bool m_has_register_names;

        // register values
        std::map<IDebugger::register_id_t, UString> m_register_values;
        bool m_has_register_values;

        // changed registers
        std::list<IDebugger::register_id_t> m_changed_registers;
        bool m_has_changed_registers;

        // memory values
        std::vector<uint8_t> m_memory_values;
        size_t m_memory_address;
        bool m_has_memory_values;

        // asm instruction list
        std::list<common::Asm> m_asm_instrs;
        bool m_has_asm_instrs;

        // Variable Object
        IDebugger::VariableSafePtr m_variable;
        bool m_has_variable;

        // Variable Object deletion information
        int m_nb_variable_deleted;

        // Children variables of a given variable.
        vector<IDebugger::VariableSafePtr> m_variable_children;
        bool m_has_variable_children;

        // Updated sub variables (direct/indirect children)
        // of a given variable
        list<IDebugger::VariableSafePtr> m_changed_var_list;
        bool m_has_changed_var_list;

        // The path expression of a variable object.
        UString m_path_expression;
        bool m_has_path_expression;

        // The variable format of a variable object
        IDebugger::Variable::Format m_variable_format;
        bool m_has_variable_format;

    public:
        ResultRecord () {clear ();}

        /// blank out everything
        void clear ()
        {
            m_kind = UNDEFINED;
            m_breakpoints.clear ();
            m_attrs.clear ();
            m_call_stack.clear ();
            m_has_call_stack = false;
            m_frames_parameters.clear ();
            m_has_frames_parameters = false;
            m_local_variables.clear ();
            m_has_local_variables = false;
            m_variable_value.reset ();
            m_has_variable_value = false;
            m_thread_list.clear ();
            m_has_thread_list = false;
            m_thread_id = 0;
            m_frame_in_thread.clear ();
            m_thread_id_got_selected = false;
            m_file_list.clear ();
            m_has_file_list = false;
            m_has_current_frame_in_core_stack_trace = false;
            m_has_changed_registers = false;
            m_changed_registers.clear ();
            m_has_register_values = false;
            m_register_values.clear ();
            m_has_register_names = false;
            m_register_names.clear ();
            m_memory_values.clear ();
            m_memory_address = 0;
            m_has_memory_values = false;
            m_asm_instrs.clear ();
            m_has_asm_instrs = false;
            m_has_variable = false;
            m_nb_variable_deleted = 0;
            m_has_variable_children = false;
            m_has_changed_var_list = false;
            m_path_expression.clear ();
            m_has_path_expression = false;
            m_variable_format = IDebugger::Variable::UNDEFINED_FORMAT;
            m_has_variable_format = false;
        }

        /// \name accessors

        /// @{
        Kind kind () const {return m_kind;}
        void kind (Kind a_in) {m_kind = a_in;}

        const map<int, IDebugger::Breakpoint>& breakpoints () const
        {
            return m_breakpoints;
        }
        map<int, IDebugger::Breakpoint>& breakpoints () {return m_breakpoints;}

        map<UString, UString>& attrs () {return m_attrs;}
        const map<UString, UString>& attrs () const {return m_attrs;}

        bool has_call_stack () const {return m_has_call_stack;}
        void has_call_stack (bool a_flag) {m_has_call_stack = a_flag;}

        const vector<IDebugger::Frame>& call_stack () const {return m_call_stack;}
        vector<IDebugger::Frame>& call_stack () {return m_call_stack;}
        void call_stack (const vector<IDebugger::Frame> &a_in)
        {
            m_call_stack = a_in;
            has_call_stack (true);
        }
        bool has_register_names () const { return m_has_register_names; }
        void has_register_names (bool a_flag) { m_has_register_names = a_flag; }
        const std::map<IDebugger::register_id_t, UString>& register_names () const
        {
            return m_register_names;
        }

        void register_names
            (const std::map<IDebugger::register_id_t, UString>& a_names)
        {
            m_register_names = a_names;
            has_register_names (true);
        }

        bool has_changed_registers () const { return m_has_changed_registers; }
        void has_changed_registers (bool a_flag)
        {
            m_has_changed_registers = a_flag;
        }
        const std::list<IDebugger::register_id_t>& changed_registers () const
        {
            return m_changed_registers;
        }
        void changed_registers (const std::list<IDebugger::register_id_t>& a_regs)
        {
            m_changed_registers = a_regs;
            has_changed_registers (true);
        }

        bool has_register_values () const { return m_has_register_values; }
        void has_register_values (bool a_flag) { m_has_register_values = a_flag; }
        const std::map<IDebugger::register_id_t, UString>& register_values () const
        {
            return m_register_values;
        }
        void register_values
            (const std::map<IDebugger::register_id_t, UString>& a_regs)
        {
            m_register_values = a_regs;
            has_register_values (true);
        }

        bool has_memory_values () const { return m_has_memory_values; }
        void has_memory_values (bool a_flag) { m_has_memory_values = a_flag; }
        const std::vector<uint8_t>& memory_values () const
        {
            return m_memory_values;
        }
        size_t memory_address () const { return m_memory_address; }
        void memory_values (size_t a_address, const std::vector<uint8_t>& a_values)
        {
            m_memory_address = a_address;
            m_memory_values = a_values;
            has_memory_values (true);
        }

        bool has_asm_instruction_list () const {return m_has_asm_instrs;}
        void has_asm_instruction_list (bool a) {m_has_asm_instrs = a;}

        const std::list<common::Asm>& asm_instruction_list () const
        {
            return m_asm_instrs;
        }
        void asm_instruction_list
                            (const std::list<common::Asm> &a_asms)
        {
            m_asm_instrs = a_asms;
            m_has_asm_instrs = true;
        }

        const map<int, list<IDebugger::VariableSafePtr> >&
                                                    frames_parameters () const
        {
            return m_frames_parameters;
        }
        void frames_parameters
                    (const map<int, list<IDebugger::VariableSafePtr> > &a_in)
        {
            m_frames_parameters = a_in;
            has_frames_parameters (true);
        }

        bool has_frames_parameters () const {return m_has_frames_parameters;}
        void has_frames_parameters (bool a_yes) {m_has_frames_parameters = a_yes;}

        const list<IDebugger::VariableSafePtr>& local_variables () const
        {
            return m_local_variables;
        }
        void local_variables (const list<IDebugger::VariableSafePtr> &a_in)
        {
            m_local_variables = a_in;
            has_local_variables (true);
        }

        bool has_local_variables () const {return m_has_local_variables;}
        void has_local_variables (bool a_in) {m_has_local_variables = a_in;}

        const IDebugger::VariableSafePtr& variable_value () const
        {
            return m_variable_value;
        }
        void variable_value (const IDebugger::VariableSafePtr &a_in)
        {
            m_variable_value = a_in;
            has_variable_value (true);
        }

        bool has_variable_value () const {return m_has_variable_value;}
        void has_variable_value (bool a_in) {m_has_variable_value = a_in;}

        bool has_thread_list () const {return m_has_thread_list;}
        void has_thread_list (bool a_in) {m_has_thread_list = a_in;}

        const std::list<int>& thread_list () const {return m_thread_list;}
        void thread_list (const std::list<int> &a_in)
        {
            m_thread_list = a_in;
            has_thread_list (true);
        }

        bool thread_id_got_selected () const {return m_thread_id_got_selected;}
        void thread_id_got_selected (bool a_in) {m_thread_id_got_selected = a_in;}

        int thread_id () const {return m_thread_id;}
        const IDebugger::Frame& frame_in_thread () const
        {
            return m_frame_in_thread;
        }
        void thread_id_selected_info (int a_thread_id,
                                      const IDebugger::Frame &a_frame_in_thread)
        {
            m_thread_id = a_thread_id;
            m_frame_in_thread = a_frame_in_thread;
            thread_id_got_selected (true);
        }

        bool has_file_list () const {return m_has_file_list;}
        void has_file_list (bool a_in) {m_has_file_list = a_in;}

        const std::vector<UString>& file_list () const {return m_file_list;}
        void file_list (const std::vector<UString> &a_in)
        {
            m_file_list = a_in;
            has_file_list (true);
        }

        const IDebugger::Frame& current_frame_in_core_stack_trace ()
        {
            return m_current_frame_in_core_stack_trace;
        }
        void current_frame_in_core_stack_trace (const IDebugger::Frame& a_in)
        {
            m_current_frame_in_core_stack_trace = a_in;
            has_current_frame_in_core_stack_trace (true);
        }

        bool has_current_frame_in_core_stack_trace ()
        {
            return m_has_current_frame_in_core_stack_trace;
        }
        void has_current_frame_in_core_stack_trace (bool a_in)
        {
            m_has_current_frame_in_core_stack_trace = a_in;
        }

        bool has_variable () {return m_has_variable; }
        void has_variable (bool a_in) {m_has_variable = a_in;}

        IDebugger::VariableSafePtr variable () const
        {
            return m_variable;
        }
        void variable (const IDebugger::VariableSafePtr a_var)
        {
            m_variable = a_var;
            has_variable (true);
        }

        int number_of_variables_deleted () const
        {
            return m_nb_variable_deleted;
        }

        void number_of_variables_deleted (int a_nb)
        {
            m_nb_variable_deleted = a_nb;
        }

        bool has_variable_children () const
        {
            return m_has_variable_children;
        }
        void has_variable_children (bool a_in)
        {
            m_has_variable_children = a_in;
        }

        const vector<IDebugger::VariableSafePtr>&
        variable_children () const
        {
            return m_variable_children;
        }
        void
        variable_children (const vector<IDebugger::VariableSafePtr> &a_vars)
        {
            m_variable_children = a_vars;
            has_variable_children (true);
        }

        bool has_changed_var_list () const
        {
            return m_has_changed_var_list;
        }
        void has_changed_var_list (bool a_in)
        {
            m_has_changed_var_list = a_in;
        }
        const list<IDebugger::VariableSafePtr>& changed_var_list () const
        {
            return m_changed_var_list;
        }
        void changed_var_list (const list<IDebugger::VariableSafePtr> &a_in)
        {
            m_changed_var_list = a_in;
            has_changed_var_list (true);
        }

        const UString& path_expression () const
        {
            return m_path_expression;
        }
        void path_expression (const UString &a_str)
        {
            m_path_expression = a_str;
            if (!a_str.empty ())
                has_path_expression (true);
        }

        bool has_path_expression () const
        {
            return m_has_path_expression;
        }
        void has_path_expression (bool a)
        {
            m_has_path_expression = a;
        }

        IDebugger::Variable::Format variable_format () const
        {
            return m_variable_format;
        }
        void variable_format (IDebugger::Variable::Format a_format)
        {
            m_variable_format = a_format;
            has_variable_format (true);
        }

        bool has_variable_format () const
        {
            return m_has_variable_format;
        }
        void has_variable_format (bool a_flag)
        {
            m_has_variable_format = a_flag;
        }

        /// @}

    };//end class ResultRecord

private:
    UString m_value;
    bool m_parsing_succeeded;
    bool m_has_out_of_band_record;
    list<OutOfBandRecord> m_out_of_band_records;
    bool m_has_result_record;
    ResultRecord m_result_record;

public:

    Output () {clear ();}

    Output (const UString &a_value) {clear ();if (a_value == "") {}}

    /// \name accessors

    /// @{
    const UString& raw_value () const {return m_value;}
    void raw_value (const UString &a_in) {m_value = a_in;}

    bool parsing_succeeded () const {return m_parsing_succeeded;}
    void parsing_succeeded (bool a_in) {m_parsing_succeeded = a_in;}

    bool has_out_of_band_record () const {return m_has_out_of_band_record;}
    void has_out_of_band_record (bool a_in) {m_has_out_of_band_record = a_in;}

    const list<OutOfBandRecord>& out_of_band_records () const
    {
        return m_out_of_band_records;
    }

    list<OutOfBandRecord>& out_of_band_records ()
    {
        return m_out_of_band_records;
    }

    bool has_result_record () const {return m_has_result_record;}
    void has_result_record (bool a_in) {m_has_result_record = a_in;}

    const ResultRecord& result_record () const {return m_result_record;}
    ResultRecord& result_record () {return m_result_record;}
    void result_record (const ResultRecord &a_in) {m_result_record = a_in;}
    /// @}

    void clear ()
    {
        m_value = "";
        m_parsing_succeeded = false;
        m_has_out_of_band_record = false;
        m_out_of_band_records.clear ();
        m_has_result_record = false;
        m_result_record.clear ();
    }
};//end class Output

/// A container of the Command sent to the debugger
/// and the output it sent back.
class CommandAndOutput {
    bool m_has_command;
    Command m_command;
    Output m_output;

public:

    CommandAndOutput (const Command &a_command,
                      const Output &a_output) :
        m_has_command (true),
        m_command (a_command),
        m_output (a_output)
    {
    }

    CommandAndOutput () :
        m_has_command (false)
    {}

    /// \name accessors

    /// @{

    bool has_command () const {return m_has_command;}
    void has_command (bool a_in) {m_has_command = a_in;}

    const Command& command () const {return m_command;}
    Command& command () {return m_command;}
    void command (const Command &a_in)
    {
        m_command = a_in; has_command (true);
    }

    const Output& output () const {return m_output;}
    Output& output () {return m_output;}
    void output (const Output &a_in) {m_output = a_in;}
    /// @}
};//end CommandAndOutput

/// After the output of the debugger
/// engine got parsed, it is send to
/// a list of OutputHandlers. In that list,
/// each output handler looks for some particular
/// properties of the output (e.g did the debugger stopped ?, or
/// is it running ? etc ...) and fires events according to the properties
/// found in the output. Output handlers are the place from where IDebugger
/// implementations fire their signals from.
struct OutputHandler : Object {

    //a method supposed to return
    //true if the current handler knows
    //how to handle a given debugger output
    virtual bool can_handle (CommandAndOutput &){return false;}

    //a method supposed to return
    //true if the current handler knows
    //how to handle a given debugger output
    virtual void do_handle (CommandAndOutput &) {}
};//end struct OutputHandler
typedef SafePtr<OutputHandler, ObjectRef, ObjectUnref> OutputHandlerSafePtr;

/// A list of OutputHandlers
/// Instances of CommandAndOutput can be submitted
/// to this list or OutputHandlers.
/// Upon submission of a CommandAndOutput, each OutputHandler of the list
/// is queried (by a call on OutputHandler::can_handle())
/// to see if it wants to 'handle' the submitted CommandAndOutput.
/// If it wants to handle it, then it is called on OutputHandler::do_handle()
/// so that it has a chance to handle the output.
/// This is the mechanism that must be used to send signals about the state
/// of implementations of IDebugger.
class OutputHandlerList : Object {
    struct Priv;
    SafePtr<Priv> m_priv;

    //non copyable
    OutputHandlerList (const OutputHandlerList&);
    OutputHandlerList& operator= (const OutputHandlerList&);

public:
    OutputHandlerList ();
    ~OutputHandlerList ();
    void add (const OutputHandlerSafePtr &a_handler);
    void submit_command_and_output (CommandAndOutput &a_cao);
};//end class OutputHandlerList


NEMIVER_END_NAMESPACE (nemiver)

#endif //__NMV_DBG_COMMON_H_H__

