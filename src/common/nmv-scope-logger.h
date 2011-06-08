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
#ifndef __NMV_SCOPE_LOGGER_H__
#define __NMV_SCOPE_LOGGER_H__

#include "nmv-api-macros.h"
#include "nmv-log-stream-utils.h"
#include "nmv-safe-ptr-utils.h"

namespace nemiver {
namespace common {

struct ScopeLoggerPriv;
class NEMIVER_API ScopeLogger
{
    friend struct ScopeLoggerPriv;

    SafePtr<ScopeLoggerPriv> m_priv;
    //forbid copy/assignation
    ScopeLogger (ScopeLogger const &);
    ScopeLogger& operator= (ScopeLogger const &);
    ScopeLogger ();

public:

    ScopeLogger (const char*a_scope_name,
                 enum LogStream::LogLevel a_level=LogStream::LOG_LEVEL_NORMAL,
                 const UString &a_log_domain=NMV_GENERAL_DOMAIN,
                 bool a_use_default_log_stream=true);

    virtual ~ScopeLogger ();

};//class ScopeLogger

}//end namespace common
}//end namespace nemiver

#endif

