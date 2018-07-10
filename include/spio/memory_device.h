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
