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

#ifndef SPIO_MEMORY_DEVICE_H
#define SPIO_MEMORY_DEVICE_H

#include "config.h"

#include "device.h"
#include "error.h"
#include "result.h"
#include "third_party/expected.h"
#include "third_party/gsl.h"

SPIO_BEGIN_NAMESPACE

namespace detail {
    template <bool C>
    struct memory_device_span;
    template <>
    struct memory_device_span<true> {
        using type = gsl::span<const gsl::byte>;
    };
    template <>
    struct memory_device_span<false> {
        using type = gsl::span<gsl::byte>;
    };

    template <bool IsConst>
    class memory_device_impl {
    public:
        using span_type = typename memory_device_span<IsConst>::type;
        using byte_type = typename span_type::value_type;

        SPIO_CONSTEXPR_STRICT memory_device_impl() = default;
        SPIO_CONSTEXPR_STRICT memory_device_impl(span_type s) : m_buf(s) {}

        bool is_open() const
        {
            return m_buf.data() != nullptr;
        }
        nonstd::expected<void, failure> close()
        {
            m_buf = span_type{};
            return {};
        }

        template <bool C = IsConst>
        auto output() -> typename std::enable_if<!C, gsl::span<gsl::byte>>::type
        {
            Expects(is_open());
            return gsl::as_writeable_bytes(m_buf);
        }
        template <bool C = IsConst>
        auto write_at(gsl::span<const gsl::byte> s, streampos pos) ->
            typename std::enable_if<!C, result>::type
        {
            Expects(is_open());
            auto ext = extent();
            auto n = std::min(static_cast<streamoff>(ext.value() - pos),
                              static_cast<streamoff>(s.size()));
            std::copy(s.begin(), s.begin() + n, m_buf.begin() + streamoff(pos));
            return n;
        }

        gsl::span<const gsl::byte> input() const
        {
            Expects(is_open());
            return gsl::as_bytes(m_buf);
        }
        result read_at(gsl::span<gsl::byte> s, streampos pos)
        {
            Expects(is_open());
            auto ext = extent();
            auto n = std::min(static_cast<streamoff>(ext.value() - pos),
                              static_cast<streamoff>(s.size()));
            std::copy(m_buf.begin() + pos, m_buf.begin() + pos + n, s.begin());
            return n;
        }

        nonstd::expected<streamsize, failure> extent() const
        {
            return m_buf.size();
        }

    private:
        span_type m_buf{};
    };
}  // namespace detail

class memory_device : private detail::memory_device_impl<false> {
    using base = detail::memory_device_impl<false>;

public:
    using base::base;
    using base::close;
    using base::extent;
    using base::input;
    using base::is_open;
    using base::output;
    using base::read_at;
    using base::write_at;
};

class memory_sink : private detail::memory_device_impl<false> {
    using base = detail::memory_device_impl<false>;

public:
    using base::base;
    using base::close;
    using base::extent;
    using base::is_open;
    using base::output;
    using base::write_at;
};

class memory_source : private detail::memory_device_impl<true> {
    using base = detail::memory_device_impl<true>;

public:
    using base::base;
    using base::close;
    using base::extent;
    using base::input;
    using base::is_open;
    using base::read_at;
};

SPIO_END_NAMESPACE

#endif  // SPIO_MEMORY_DEVICE_H
