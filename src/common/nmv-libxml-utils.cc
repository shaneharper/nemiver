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
#include "nmv-libxml-utils.h"
#include "nmv-exception.h"

namespace nemiver {
namespace common {
namespace libxmlutils {

int
reader_io_read_callback (ReaderIOContext *a_read_context,
                         char * a_buf,
                         int a_len)
{
    THROW_IF_FAIL (a_read_context);
    int len = a_len;
    int result (-1);
    IInputStream::Status status = a_read_context->m_istream.read (a_buf, len);
    switch (status) {
    case IInputStream::OK:
        result = len;
        break;
    case IInputStream::EOF_ERROR:
        result = 0;
        break;
    case IInputStream::NOT_OPEN_ERROR:
    case IInputStream::ERROR:
    default:
        result = -1;
    }
    return result;
}

int
reader_io_close_callback (ReaderIOContext *a_read_context)
{
    if (a_read_context) {}
    return 0;
}

bool
goto_next_element_node (XMLTextReaderSafePtr &a_reader)
{
    int result = xmlTextReaderRead (a_reader.get ());
    if (result == 0) {
        return false;
    } else if (result < 0) {
        THROW ("parsing error");
    }
    xmlReaderTypes node_type =
        static_cast<xmlReaderTypes> (xmlTextReaderNodeType (a_reader.get ()));
    while (node_type != XML_READER_TYPE_ELEMENT) {
        result = xmlTextReaderRead (a_reader.get ());
        if (result == 0) {
            return false;
        } else if (result < 0) {
            THROW ("parsing error");
        }
        node_type = static_cast<xmlReaderTypes>
                    (xmlTextReaderNodeType (a_reader.get ()));
    }
    return true;
}

bool
goto_next_element_node_and_check (XMLTextReaderSafePtr &a_reader,
                                  const char* a_element_name)
{
    if (!goto_next_element_node (a_reader)) {
        return false;
    }
    UString name = reinterpret_cast<const char*>
                        (xmlTextReaderConstName (a_reader.get ()));
    if (name != a_element_name) {
        return false;
    }
    return true;
}


bool
search_next_element_node (XMLTextReaderSafePtr &a_reader,
                          const char* a_element_name)
{
    THROW_IF_FAIL (a_element_name);

    bool result (false);
    int status (0);
    xmlReaderTypes node_type;
    for (;;) {
        status = xmlTextReaderRead (a_reader.get ());
        if (status == 0) {
            return false;
        } else if (result < 0) {
            THROW ("parsing error");
        }
        node_type = static_cast<xmlReaderTypes>
                    (xmlTextReaderNodeType (a_reader.get ()));
        UString name =
            reinterpret_cast<const char*>
                (XMLCharSafePtr (xmlTextReaderLocalName (a_reader.get ())).get ());
        if (node_type == XML_READER_TYPE_ELEMENT
            && name == a_element_name) {
            result=true;
            break;
        }
    }
    return result;
}

bool
goto_next_text_node (XMLTextReaderSafePtr &a_reader)
{
    int result = xmlTextReaderRead (a_reader.get ());
    if (result == 0) {
        return false;
    } else if (result < 0) {
        THROW ("parsing error");
    }
    xmlReaderTypes node_type =
        static_cast<xmlReaderTypes> (xmlTextReaderNodeType (a_reader.get ()));
    while (node_type != XML_READER_TYPE_TEXT) {
        result = xmlTextReaderRead (a_reader.get ());
        if (result == 0) {
            return false;
        } else if (result < 0) {
            THROW ("parsing error");
        }
        node_type = static_cast<xmlReaderTypes>
                    (xmlTextReaderNodeType (a_reader.get ()));
    }
    return true;
}

bool
read_next_and_check_node (XMLTextReaderSafePtr &a_reader,
                          xmlReaderTypes a_node_type_to_be)
{
    int result = xmlTextReaderRead (a_reader.get ());
    if (result == 0) {
        return false;
    } else if (result < 0) {
        THROW ("parsing error");
    }
    xmlReaderTypes node_type =
        static_cast<xmlReaderTypes>
        (xmlTextReaderNodeType (a_reader.get ()));
    if (node_type == a_node_type_to_be) {
        return true;
    }
    return false;
}

bool
is_empty_element (XMLTextReaderSafePtr &a_reader)
{
    THROW_IF_FAIL (a_reader);
    int status = xmlTextReaderIsEmptyElement (a_reader.get ());
    if (status == 1) {
        return true;
    } else if (status == 0) {
        return false;
    } else if (status < 0) {
        THROW ("an error occurred while calling "
               "xmlTextReaderIsEmptyElement()");
    } else {
        THROW ("unknown return value for "
               "xmlTextReaderIsEmptyElement()");
    }
}

}//end namespace libxmlutils
}//end namespace common
}//end namespace nemiver

