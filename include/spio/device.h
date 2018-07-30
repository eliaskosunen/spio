// Copyright 2017-2018 Elias Kosunen
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// This file is a part of spio:
//     https://github.com/eliaskosunen/spio

#ifndef SPIO_DEVICE_H
#define SPIO_DEVICE_H

#include "config.h"
#include "third_party/gsl.h"
#include "util.h"

namespace spio {
SPIO_BEGIN_NAMESPACE

using streamsize = std::ptrdiff_t;
using streamoff = std::ptrdiff_t;

class streampos {
public:
#if !SPIO_PTRDIFF_IS_INT
    SPIO_CONSTEXPR streampos(int n) noexcept
        : streampos(static_cast<streamoff>(n))
    {
    }
#endif
    SPIO_CONSTEXPR streampos(streamoff n) noexcept : m_pos(n) {}

    SPIO_CONSTEXPR explicit operator streamoff() const noexcept
    {
        return m_pos;
    }

    SPIO_CONSTEXPR bool operator==(const streampos& p) const noexcept
    {
        return m_pos == p.m_pos;
    }
    SPIO_CONSTEXPR bool operator!=(const streampos& p) const noexcept
    {
        return !(*this == p);
    }

    SPIO_CONSTEXPR14 streampos& operator+=(streamoff n) noexcept
    {
        m_pos += n;
        return *this;
    }
    SPIO_CONSTEXPR14 streampos& operator-=(streamoff n) noexcept
    {
        return (*this += -n);
    }

    SPIO_CONSTEXPR streamoff operator-(const streampos& p) const noexcept
    {
        return m_pos - p.m_pos;
    }

private:
    streamoff m_pos;
};

SPIO_CONSTEXPR14 streampos operator+(streampos l, streampos r) noexcept
{
    l += streamoff(r);
    return l;
}
SPIO_CONSTEXPR14 streampos operator-(streampos l, streampos r) noexcept
{
    l -= streamoff(r);
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
using ra_writable_op = decltype(
    std::declval<Device>().write_at(std::declval<gsl::span<const gsl::byte>>(),
                                    std::declval<streampos>()));
template <typename Device>
using is_random_access_writable = is_detected<ra_writable_op, Device>;

template <typename Device>
using byte_writable_op =
    decltype(std::declval<Device>().put(std::declval<gsl::byte>()));
template <typename Device>
using is_byte_writable = is_detected<byte_writable_op, Device>;

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
                            is_random_access_writable<Device>,
                            is_byte_writable<Device>,
                            is_vector_writable<Device>,
                            is_direct_writable<Device>>;

template <typename Device>
using readable_op =
    decltype(std::declval<Device>().read(std::declval<gsl::span<gsl::byte>>(),
                                         std::declval<bool&>()));
template <typename Device>
using is_readable = is_detected<readable_op, Device>;

template <typename Device>
using ra_readable_op =
    decltype(std::declval<Device>().read(std::declval<gsl::span<gsl::byte>>(),
                                         std::declval<streampos>(),
                                         std::declval<bool&>()));
template <typename Device>
using is_random_access_readable = is_detected<ra_readable_op, Device>;

template <typename Device>
using byte_readable_op =
    decltype(std::declval<Device>().get(std::declval<gsl::byte&>(),
                                        std::declval<bool&>()));
template <typename Device>
using is_byte_readable = is_detected<byte_readable_op, Device>;

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
                              is_random_access_readable<Device>,
                              is_byte_readable<Device>,
                              is_vector_readable<Device>,
                              is_direct_readable<Device>>;

template <typename Device>
using syncable_op = decltype(std::declval<Device>().sync());
template <typename Device>
using is_syncable = is_detected<syncable_op, Device>;

template <typename Device>
using relative_seekable_op =
    decltype(std::declval<Device>().seek(std::declval<streampos>(),
                                         std::declval<inout>()));
template <typename Device>
using absolute_seekable_op =
    decltype(std::declval<Device>().seek(std::declval<streampos>(),
                                         std::declval<seekdir>(),
                                         std::declval<inout>()));
template <typename Device>
using is_seekable = conjunction<is_detected<relative_seekable_op, Device>,
                                is_detected<absolute_seekable_op, Device>>;

template <typename Device>
using sized_op = decltype(std::declval<Device>().extent());
template <typename Device>
using is_sized = is_detected<sized_op, Device>;

template <typename Device>
using truncatable_op =
    decltype(std::declval<Device>().truncate(std::declval<streampos>()));
template <typename Device>
using is_truncatable = is_detected<truncatable_op, Device>;

template <typename Device>
using closable_op = decltype(std::declval<Device>().close());
template <typename Device>
using is_open_op = decltype(std::declval<Device>().is_open());
template <typename Device>
using is_device = conjunction<is_detected<closable_op, Device>,
                              is_detected<is_open_op, Device>,
                              disjunction<is_sink<Device>, is_source<Device>>>;
SPIO_END_NAMESPACE
}  // namespace spio

#endif  // SPIO_DEVICE_H
