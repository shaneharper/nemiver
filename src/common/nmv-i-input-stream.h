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
#ifndef __NMV_I_INPUT_STREAM_H__
#define __NMV_I_INPUT_STREAM_H__

#include "nmv-object.h"

namespace nemiver {
namespace common {

class IInputStream : public Object {
    //forbid copy
    IInputStream (const IInputStream &);
    IInputStream& operator= (const IInputStream &);

public:
    enum Status {
        OK=0,
        EOF_ERROR,
        NOT_OPEN_ERROR,
        ERROR
    };//end Status

    IInputStream ()
    {}

    virtual ~IInputStream ()
    {}

    virtual bool open (const char *a_path) = 0;

    virtual Status read (char *a_buf, int &a_len) =0;

    virtual void close () =0;
};//end class InputStream
}//end namespace common
}//end namespace nemiver
#endif //__NMV_INPUT_STREAM_H__

