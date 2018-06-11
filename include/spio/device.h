// Copyright 2017-2018 Elias Kosunen
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef SPIO_DEVICE_H
#define SPIO_DEVICE_H

#include "config.h"
#include "third_party/gsl.h"
#include "util.h"

SPIO_BEGIN_NAMESPACE

using streamsize = std::ptrdiff_t;
using streamoff = std::ptrdiff_t;

class streampos {
public:
    SPIO_CONSTEXPR_STRICT streampos(int n) SPIO_NOEXCEPT
        : streampos(static_cast<streamoff>(n))
    {
    }
    SPIO_CONSTEXPR_STRICT streampos(streamoff n) SPIO_NOEXCEPT : m_pos(n) {}

    SPIO_CONSTEXPR_STRICT operator streamoff() const SPIO_NOEXCEPT
    {
        return m_pos;
    }

    bool operator==(const streampos& p) const
    {
        return m_pos == p.m_pos;
    }
    bool operator!=(const streampos& p) const
    {
        return !(*this == p);
    }

    streampos& operator+=(streamoff n)
    {
        m_pos += n;
        return *this;
    }
    streampos& operator-=(streamoff n)
    {
        return (*this += -n);
    }

    streamoff operator-(const streampos& p) const
    {
        return m_pos - p.m_pos;
    }

private:
    streamoff m_pos;
};

inline streampos operator+(streampos l, streampos r)
{
    l += r;
    return l;
}
inline streampos operator-(streampos l, streampos r)
{
    l -= r;
    return l;
}

enum class seekdir { beg, end, cur };

using inout = int;
constexpr inout in = 1;
constexpr inout out = 2;

template <typename Device>
using writable_op = decltype(
    std::declval<Device>().write(std::declval<gsl::span<const gsl::byte>>()));
template <typename Device>
using is_writable = is_detected<writable_op, Device>;

template <typename Device>
using vector_writable_op = decltype(std::declval<Device>().vwrite(
    std::declval<gsl::span<gsl::span<const gsl::byte>>>(),
    std::declval<streampos>()));
template <typename Device>
using is_vector_writable = is_detected<vector_writable_op, Device>;

template <typename Device>
using direct_writable_op = decltype(std::declval<Device>().output());
template <typename Device>
using is_direct_writable = is_detected<direct_writable_op, Device>;

template <typename Device>
using is_sink = disjunction<is_writable<Device>,
                            is_vector_writable<Device>,
                            is_direct_writable<Device>>;

template <typename Device>
using readable_op =
    decltype(std::declval<Device>().read(std::declval<gsl::span<gsl::byte>>(),
                                         std::declval<bool&>()));
template <typename Device>
using is_readable = is_detected<readable_op, Device>;

template <typename Device>
using vector_readable_op = decltype(std::declval<Device>().vread(
    std::declval<gsl::span<gsl::span<gsl::byte>>>(),
    std::declval<streampos>()));
template <typename Device>
using is_vector_readable = is_detected<vector_readable_op, Device>;

template <typename Device>
using direct_readable_op = decltype(std::declval<Device>().input());
template <typename Device>
using is_direct_readable = is_detected<direct_readable_op, Device>;

template <typename Device>
using is_source = disjunction<is_readable<Device>,
                              is_vector_readable<Device>,
                              is_direct_readable<Device>>;

template <typename Device>
struct is_device : disjunction<is_sink<Device>, is_source<Device>>::type {
};

SPIO_END_NAMESPACE

#endif  // SPIO_DEVICE_H
