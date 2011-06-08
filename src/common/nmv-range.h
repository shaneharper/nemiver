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
#ifndef __NMV_RANGE_H__
#define __NMV_RANGE_H__

#include "nmv-namespace.h"
#include "nmv-api-macros.h"

NEMIVER_BEGIN_NAMESPACE (nemiver)
NEMIVER_BEGIN_NAMESPACE (common)

/// This abstracts a range of integer values.
class Range {
    size_t m_min;
    size_t m_max;

public:

    /// An enum representing the result of the searching of a range
    /// around a value. E.g, suppose we have an ordered set of integer
    /// values vi {v1, v2, v3 ...} that we name V.
    /// Suppose we also have a particular integer N. And we want to
    /// make the query expressed as: "What is the smallest range
    /// R{vi,vi+p} that encloses N?".
    /// If we had a function that would execute that query and return
    /// an answer, this enum would help us know more about the how N
    /// relates to the returned range.
    /// VALUE_SEARCH_RESULT_EXACT means
    /// that the N equals vi and equals vi+p. It means the range is
    /// actually a single value, which is N.
    /// VALUE_SEARCH_RESULT_WITHIN means that N is greater than vi
    /// and less than vi+p. I means N is within the range.
    /// VALUE_SEARCH_RESULT_BEFORE means that N is less then vi, and
    /// so less than vi+p too. It means N is before the range.
    /// VALUE_SEARCH_RESULT_AFTER means N is greater than vi+p, and so
    /// greater than vi too. It means N is after the range.
    enum ValueSearchResult {
        VALUE_SEARCH_RESULT_EXACT = 0,
        VALUE_SEARCH_RESULT_WITHIN,
        VALUE_SEARCH_RESULT_BEFORE,
        VALUE_SEARCH_RESULT_AFTER,
        VALUE_SEARCH_RESULT_NONE // <-- must always be last.
    };

    Range (size_t a_min = 0, size_t a_max = 0) :
        m_min (a_min),
        m_max (a_max)
    {
    }

    /// Accessors of the lower bound of the range.
    size_t min () const {return m_min;}
    void min (size_t a) {m_min = a;}

    /// Accessors of the upper bound of the range.
    size_t max () const {return m_max;}
    void max (size_t a) {m_max = a;}

    /// Returns true if a_value is within the range.
    bool contains (size_t a_value) const
    {
        return (a_value >= m_min && a_value <= m_max);
    }

    /// Extends the range. If a_value is greater than the upper bound
    /// of the range, then the range becomes [lower-bound, a_value].
    /// If a_value is less than the lower bound of the range, then the
    /// range becomes [a_value, upper-bound]. If a_value is within the
    /// range, nothing happens.
    void extend (size_t a_value)
    {
        if (!contains (a_value)) {
            if (a_value < m_min)
                m_min = a_value;
            else
                m_max = a_value;
        }
    }
};

NEMIVER_END_NAMESPACE (common)
NEMIVER_END_NAMESPACE (nemiver)

#endif // __NMV_RANGE_H__

