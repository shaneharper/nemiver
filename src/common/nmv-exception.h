/* -*- Mode: C++; indent-tabs-mode:nil; c-basic-offset: 4-*- */

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
#ifndef __NMV_EXCEPTION_H__
#define __NMV_EXCEPTION_H__

#include <cstdlib>
#include <stdexcept>
#include "nmv-ustring.h"
#include "nmv-log-stream-utils.h"

namespace nemiver {
namespace common {

class NEMIVER_EXCEPTION_API Exception: public std::runtime_error
{
public:
    Exception (const char* a_reason);
    Exception (const UString &a_reason);
    Exception (const Exception &a_other);
    Exception (const std::exception &);
    Exception& operator= (const Exception &a_other);
    virtual ~Exception () throw ();
    const char* what () const throw ();
};//class Exception

#define _THROW(EXPR) \
if (std::getenv ("nmv_abort_on_throw")) \
    abort (); \
else \
    throw (EXPR);

#define _THROW_EMPTY \
if (std::getenv ("nmv_abort_on_throw")) \
    abort (); \
else \
    throw;

#define THROW_IF_FAIL(a_cond) \
if (!(a_cond)) { \
LOG_EXCEPTION ("condition (" << #a_cond << ") failed; raising exception\n" );\
_THROW (nemiver::common::Exception \
    (nemiver::common::UString ("Assertion failed: ") + #a_cond)) ;\
}

#define THROW_IF_FAIL2(a_cond, a_reason) \
if (!(a_cond)) { \
LOG_EXCEPTION ("condition (" << #a_cond << ") failed; raising exception " << a_reason <<"\n");\
_THROW (nemiver::common::Exception (a_reason)) ;\
}

#define THROW_IF_FAIL3(a_cond, type, a_reason) \
if (!(a_cond)) { \
LOG_EXCEPTION ("condition (" << #a_cond << ") failed; raising exception " << #type << \
<< ":  " << a_reason << "\n" ); _THROW (type (a_reason))  ;\
}

#define ABORT_IF_FAIL(a_cond, a_reason) \
if (!(a_cond)) { \
LOG_EXCEPTION ("condition (" << #a_cond << ") failed; raising exception " << a_reason <<"\n"); abort();\
}

#define THROW(a_reason) \
LOG_EXCEPTION ("raised exception: "<< (nemiver::common::UString (a_reason)) << "\n"); \
_THROW (nemiver::common::Exception (nemiver::common::UString (a_reason))) ;

#define THROW_EMPTY \
LOG_EXCEPTION ("raised empty exception " << endl); \
_THROW_EMPTY

#define THROW_EXCEPTION(type, message) \
LOG_EXCEPTION ("raised " << #type << ": "<< message<< "\n"); \
_THROW (type (message));

#define TRACE_EXCEPTION(exception) \
LOG_EXCEPTION ("catched exception: " << exception.what () << "\n")

#define RETHROW_EXCEPTION(exception) \
LOG_EXCEPTION ("catched and rethrowing exception: " << exception.what() << "\n")

#define RETURN_VAL_IF_FAIL(expression, value) \
if (!(expression)) { \
LOG_ERROR ("assertion " << #expression << " failed. Returning " << #value << "\n"); \
return value; \
}

#define RETURN_IF_FAIL(expression) \
if (!(expression)) { \
LOG_ERROR ("assertion " << #expression << " failed. Returning.\n"); \
return; \
}

#ifndef NEMIVER_TRY
#define NEMIVER_TRY try {
#endif

#ifndef NEMIVER_CATCH_NOX
#define NEMIVER_CATCH_NOX \
} catch (Glib::Exception &e) { \
    LOG_ERROR (e.what ()); \
} catch (std::exception &e) { \
    LOG_ERROR (e.what ()); \
} catch (...) { \
    LOG_ERROR ("An unknown error occured"); \
}
#endif

#ifndef NEMIVER_CATCH_AND_RETURN_NOX
#define NEMIVER_CATCH_AND_RETURN_NOX(a_value) \
} catch (Glib::Exception &e) { \
    LOG_ERROR (e.what ()); \
    return a_value; \
} catch (std::exception &e) { \
    LOG_ERROR (e.what ()); \
    return a_value; \
} catch (...) { \
    LOG_ERROR ("An unknown error occured"); \
    return a_value; \
}
#endif

}//end namespace common
}//end namespace nemiver 

#endif //__NMV_EXCEPTION_H__
