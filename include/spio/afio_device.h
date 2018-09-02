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

namespace spio {
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

        using buffer_type = span<byte>;
        using const_buffer_type = span<const byte>;

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

        expected<afio::span<afio_buffer_type>, failure> afio_read(
            afio_request_type reqs)
        {
            Expects(m_handle);
            Expects(!reqs.buffers.empty());

            auto ret = afio::read(*handle(), reqs);
            if (ret.has_error()) {
                return make_unexpected(make_error_code(ret.error()));
            }
            return ret.value();
        }
        expected<span<buffer_type>, failure> vread(
            span<buffer_type> bufs,
            streampos pos = 0)
        {
            Expects(!bufs.empty());

            afio_buffer_type dummy;
            afio::span<afio_buffer_type> list(&dummy, bufs.size());
            std::transform(
                bufs.begin(), bufs.end(), list.begin(),
                [](buffer_type& b) -> afio_buffer_type {
                    return {reinterpret_cast<afio::byte*>(b.data()), b.size()};
                });

            auto ret = afio_read(
                {list, static_cast<typename Handle::extent_type>(pos)});
            if (!ret) {
                return make_unexpected(ret.error());
            }

            std::transform(ret->begin(), ret->end(), bufs.begin(),
                           [](afio_buffer_type& b) -> buffer_type {
                               return {reinterpret_cast<byte*>(b.data), b.len};
                           });
            return bufs;
        }

        expected<afio::span<const_afio_buffer_type>, failure>
        afio_write(typename Handle::template io_request<
                   afio::span<const_afio_buffer_type>> reqs)
        {
            Expects(m_handle);
            Expects(!reqs.buffers.empty());

            auto ret = afio::write(*handle(), reqs);
            if (ret.has_error()) {
                return make_unexpected(make_error_code(ret.error()));
            }
            return ret.value();
        }
        expected<span<const_buffer_type>, failure> vwrite(
            span<const_buffer_type> bufs,
            streampos pos = 0)
        {
            Expects(!bufs.empty());

            const_afio_buffer_type dummy;
            afio::span<const_afio_buffer_type> list(&dummy, bufs.size());
            std::transform(
                bufs.begin(), bufs.end(), list.begin(),
                [](const_buffer_type& b) -> const_afio_buffer_type {
                    return {reinterpret_cast<const afio::byte*>(b.data()),
                            b.size()};
                });

            auto ret = afio_write(
                {list, static_cast<typename Handle::extent_type>(pos)});
            if (!ret) {
                return make_unexpected(ret.error());
            }

            std::transform(
                ret->begin(), ret->end(), bufs.begin(),
                [](const_afio_buffer_type& b) -> const_buffer_type {
                    return {reinterpret_cast<const byte*>(b.data), b.len};
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

    expected<streamsize, failure> truncate(streamsize newsize)
    {
        Expects(is_open());

        auto ret = handle()->truncate(
            static_cast<afio::file_handle::extent_type>(newsize));
        if (ret.has_error()) {
            return make_unexpected(make_error_code(ret.error()));
        }
        return static_cast<streamsize>(ret.value());
    }

    expected<streamsize, failure> extent() const
    {
        auto ret = handle()->maximum_extent();
        if (ret.has_error()) {
            return make_unexpected(make_error_code(ret.error()));
        }
        return static_cast<streamsize>(ret.value());
    }
};

SPIO_END_NAMESPACE
}  // namespace spio

#endif  // SPIO_AFIO_DEVICE_H
