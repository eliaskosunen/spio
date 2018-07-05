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

#ifndef SPIO_AFIO_DEVICE_H
#define SPIO_AFIO_DEVICE_H

#include "config.h"

#include <chrono>
#include "device.h"
#include "error.h"
#include "third_party/afio.h"
#include "third_party/expected.h"
#include "third_party/gsl.h"
#include "third_party/variant.h"

SPIO_BEGIN_NAMESPACE

struct deadline {
    bool steady;
    union {
        std::chrono::nanoseconds nsecs;
    };
};

namespace detail {
template <typename Handle>
class afio_device {
public:
    using afio_buffer_type = typename Handle::buffer_type;
    using const_afio_buffer_type = typename Handle::const_buffer_type;

    using afio_request_type =
        typename Handle::template io_request<afio::span<afio_buffer_type>>;
    using const_afio_request_type = typename Handle::template io_request<
        afio::span<const_afio_buffer_type>>;

    using buffer_type = gsl::span<gsl::byte>;
    using const_buffer_type = gsl::span<const gsl::byte>;

    constexpr afio_device() = default;
    afio_device(Handle& h) : m_handle(std::addressof(h)) {}

    Handle* handle() noexcept
    {
        return m_handle;
    }
    const Handle* handle() const noexcept
    {
        return m_handle;
    }

    bool is_open() const
    {
        return m_handle != nullptr;
    }
    void close()
    {
        Expects(m_handle);
        m_handle->close();
    }

    nonstd::expected<afio::span<afio_buffer_type>, failure> afio_read(
        afio_request_type reqs)
    {
        Expects(m_handle);
        Expects(!reqs.buffers.empty());

        auto ret = afio::read(*handle(), reqs);
        if (ret.has_error()) {
            return nonstd::make_unexpected(make_error_code(ret.error()));
        }
        return ret.value();
    }
    nonstd::expected<gsl::span<buffer_type>, failure> vread(
        gsl::span<buffer_type> bufs,
        streampos pos = 0)
    {
        Expects(!bufs.empty());

        static afio_buffer_type dummy;
        afio::span<afio_buffer_type> list(&dummy, bufs.size());
        std::transform(
            bufs.begin(), bufs.end(), list.begin(),
            [](buffer_type& b) -> afio_buffer_type {
                return {reinterpret_cast<afio::byte*>(b.data()), b.size()};
            });

        auto ret =
            afio_read({list, static_cast<typename Handle::extent_type>(pos)});
        if (!ret) {
            return nonstd::make_unexpected(ret.error());
        }

        std::transform(ret->begin(), ret->end(), bufs.begin(),
                       [](afio_buffer_type& b) -> buffer_type {
                           return {reinterpret_cast<gsl::byte*>(b.data), b.len};
                       });
        return bufs;
    }

    nonstd::expected<afio::span<const_afio_buffer_type>, failure> afio_write(
        typename Handle::template io_request<afio::span<const_afio_buffer_type>>
            reqs)
    {
        Expects(m_handle);
        Expects(!reqs.buffers.empty());

        auto ret = afio::write(*handle(), reqs);
        if (ret.has_error()) {
            return nonstd::make_unexpected(make_error_code(ret.error()));
        }
        return ret.value();
    }
    nonstd::expected<gsl::span<const_buffer_type>, failure> vwrite(
        gsl::span<const_buffer_type> bufs,
        streampos pos = 0)
    {
        Expects(!bufs.empty());

        static const_afio_buffer_type dummy;
        afio::span<const_afio_buffer_type> list(&dummy, bufs.size());
        std::transform(bufs.begin(), bufs.end(), list.begin(),
                       [](const_buffer_type& b) -> const_afio_buffer_type {
                           return {
                               reinterpret_cast<const afio::byte*>(b.data()),
                               b.size()};
                       });

        auto ret =
            afio_write({list, static_cast<typename Handle::extent_type>(pos)});
        if (!ret) {
            return nonstd::make_unexpected(ret.error());
        }

        std::transform(
            ret->begin(), ret->end(), bufs.begin(),
            [](const_afio_buffer_type& b) -> const_buffer_type {
                return {reinterpret_cast<const gsl::byte*>(b.data), b.len};
            });
        return bufs;
    }

private:
    Handle* m_handle{nullptr};
};
}  // namespace detail

class afio_io_device : public detail::afio_device<afio::io_handle> {
    using base = detail::afio_device<afio::io_handle>;

public:
    using base::base;
};

class afio_file_device : public detail::afio_device<afio::file_handle> {
    using base = detail::afio_device<afio::file_handle>;

public:
    using base::base;

    nonstd::expected<streamsize, failure> truncate(streamsize newsize)
    {
        Expects(is_open());

        auto ret = handle()->truncate(
            static_cast<afio::file_handle::extent_type>(newsize));
        if (ret.has_error()) {
            return nonstd::make_unexpected(make_error_code(ret.error()));
        }
        return static_cast<streamsize>(ret.value());
    }

    nonstd::expected<streamsize, failure> extent() const
    {
        auto ret = handle()->maximum_extent();
        if (ret.has_error()) {
            return nonstd::make_unexpected(make_error_code(ret.error()));
        }
        return static_cast<streamsize>(ret.value());
    }
};

SPIO_END_NAMESPACE

#endif  // SPIO_AFIO_DEVICE_H
