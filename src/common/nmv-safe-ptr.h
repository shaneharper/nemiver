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
#ifndef __NMV_SAFE_PTR_H__
#define __NMV_SAFE_PTR_H__

#include <cstdlib>

namespace nemiver {
namespace common {

struct DefaultRef
{
    void
    operator () (const void*) {}
}
; //end struct DefaultReference

struct FreeUnref
{
    void
    operator () (const void* a_ptr)
    {
        if (a_ptr)
            free (const_cast<void *> (a_ptr));
    }
}
;//end struct DefaultUnreference

template <class PointerType>
struct DeleteFunctor
{
    void
    operator () (const PointerType* a_ptr)
    {
        if (a_ptr)
            delete (a_ptr);
    }
};

template<class PointerType,
         class ReferenceFunctor = DefaultRef,
         class UnreferenceFunctor = DeleteFunctor<PointerType>
        >
class SafePtr
{
protected:
    mutable PointerType *m_pointer;


public:
    explicit SafePtr (const PointerType *a_pointer, bool a_do_ref=false) :
        m_pointer (const_cast<PointerType*> (a_pointer))
    {
        if (a_do_ref) {
            reference ();
        }
    }

    SafePtr () : m_pointer (0)
    {
    }

    SafePtr (const SafePtr<PointerType,
                           ReferenceFunctor,
                           UnreferenceFunctor> &a_safe_ptr) :
        m_pointer (a_safe_ptr.m_pointer)
    {
        reference ();
    }

    ~SafePtr ()
    {
        unreference ();
        m_pointer = 0;
    }

    SafePtr<PointerType, ReferenceFunctor, UnreferenceFunctor>&
    operator= (const SafePtr<PointerType,
                             ReferenceFunctor,
                             UnreferenceFunctor> &a_safe_ptr)
    {
        SafePtr<PointerType,
                ReferenceFunctor,
                UnreferenceFunctor> temp (a_safe_ptr);
        swap (temp);
        return *this;
    }

    /*
    SafePtr<PointerType, ReferenceFunctor, UnreferenceFunctor>&
    operator= (const PointerType *a_pointer)
    {
        reset (a_pointer);
        return *this;
    }
    */

    PointerType&
    operator* () const
    {
        return  *(m_pointer);
    }

    PointerType*
    operator-> () const
    {
        return m_pointer;
    }


    bool operator== (const SafePtr<PointerType,
                     ReferenceFunctor,
                     UnreferenceFunctor> &a_safe_ptr) const
    {
        return m_pointer == a_safe_ptr.m_pointer;
    }

    bool operator== (const PointerType *a_ptr) const
    {
        return m_pointer == a_ptr;
    }

    bool operator! () const
    {
        if (m_pointer)
            return false;
        return true;
    }

    operator bool () const
    {
        if (!m_pointer)
            return false;
        return true;
    }


    bool operator!= (const PointerType *a_pointer)
    {
        return !this->operator== (a_pointer);
    }

    bool operator!= (const SafePtr<PointerType,
                     ReferenceFunctor,
                     UnreferenceFunctor> &a_safe_ptr)
    {
        return !this->operator== (a_safe_ptr);
    }

    void
    reset ()
    {
        reset (0);
    }

    void
    reset (const PointerType *a_pointer, bool a_do_ref=false)
    {
        if (a_pointer != m_pointer) {
            unreference ();
            m_pointer = const_cast<PointerType*> (a_pointer);
            if (a_do_ref) {
                reference ();
            }
        }
    }

    PointerType*
    get () const
    {
        return m_pointer;
    }

    PointerType*
    ref_and_get () const
    {
        const_cast<SafePtr<PointerType,
                           ReferenceFunctor,
                           UnreferenceFunctor>* > (this)->reference ();
        return m_pointer;
    }

    template <class T>
    SafePtr<T, ReferenceFunctor, UnreferenceFunctor>
    do_dynamic_cast ()
    {
        T *pointer = dynamic_cast<T*> (m_pointer);
        SafePtr<T, ReferenceFunctor, UnreferenceFunctor> result (pointer);
        if (result) {
            result.reference ();
        }
        return result;
    }

    PointerType*
    release ()
    {
        PointerType* pointer = m_pointer;
        m_pointer = 0;
        return  pointer;
    }

    void
    swap (SafePtr<PointerType,
                  ReferenceFunctor,
                  UnreferenceFunctor> &a_safe_ptr)
    {
        PointerType *const tmp (m_pointer);
        m_pointer = a_safe_ptr.m_pointer;
        a_safe_ptr.m_pointer = tmp;
    }

    void
    reference ()
    {
        if (m_pointer) {
            ReferenceFunctor do_ref;
            do_ref (m_pointer);
        }
    }

    void
    unreference ()
    {
        if (m_pointer) {
            UnreferenceFunctor do_unref;
            do_unref (m_pointer);
        }
    }
};//end class SafePtr

}//end namespace common
}//end namespace nemiver

#endif //__NMV_SAFR_PTR_H__

