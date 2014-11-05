/* -*- Mode: C++; indent-tabs-mode:nil; c-basic-offset:4; -*- */

/*Copyright (c) 2005-2006 Dodji Seketeli
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies
 * or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS",
 * WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#include "config.h"
#include <glibmm.h>
#include "nmv-exception.h"
#include "nmv-ustring.h"
#include "nmv-scope-logger.h"


namespace nemiver {
namespace common {

static const UString DELETE ("delete");

struct ScopeLoggerPriv
{
    Glib::Timer timer;
    LogStream *out;
    bool can_free;
    UString name;
    UString domain;

    ScopeLoggerPriv (const char*a_scope_name,
                     enum LogStream::LogLevel a_level,
                     const UString &a_log_domain,
                     bool a_use_default_log_stream) :
        out (0), can_free (false)
    {
        if (!a_use_default_log_stream) {
            out = new LogStream (a_level);
            can_free = true;
        } else {
            out = &(LogStream::default_log_stream ());
            can_free = false;
        }
        name = a_scope_name;
        domain = a_log_domain;

        out->push_domain (a_log_domain);
        *out  << "|{|" << name << ":{" << common::endl;
        out->pop_domain ();

        timer.start ();
    }

    ~ScopeLoggerPriv ()
    {
        timer.stop ();

        if (!out) {return;}

        out->push_domain (domain);
        *out << "|}|" << name <<":}elapsed: "
             << timer.elapsed () << "secs" << common::endl;
        out->pop_domain ();
        if (can_free) {
            if (out) {
                delete out;
            }
        }
        out = 0;
    }
};


ScopeLogger::ScopeLogger (const char*a_scope_name,
                          enum LogStream::LogLevel a_level,
                          const UString &a_log_domain,
                          bool a_use_default_log_stream) :
    m_priv (new ScopeLoggerPriv (a_scope_name, a_level,
                                 a_log_domain, a_use_default_log_stream))
{
}

ScopeLogger::~ScopeLogger ()
{
    //commented this out for performance reasons.
    //yeah, real reasons that got highlighted by profiling!
    //LOG_DD (DELETE);
}

}//end namespace common
}//end namespace nemiver

