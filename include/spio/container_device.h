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

#ifndef SPIO_CONTAINER_DEVICE_H
#define SPIO_CONTAINER_DEVICE_H

#include "config.h"

#include "device.h"
#include "error.h"
#include "result.h"
#include "third_party/expected.h"
#include "third_party/gsl.h"
#include "util.h"

namespace spio {
SPIO_BEGIN_NAMESPACE

namespace detail {
    template <template <typename...> class Container>
    class basic_container_device_impl {
    public:
        using container_type = Container<byte>;
        using iterator = typename container_type::iterator;
        using const_iterator = typename container_type::const_iterator;

        basic_container_device_impl() = default;
        basic_container_device_impl(container_type& c)
            : m_buf(std::addressof(c)), m_it(m_buf->begin())
        {
        }

        SPIO_CONSTEXPR14 container_type* container() noexcept
        {
            return m_buf;
        }
        SPIO_CONSTEXPR const container_type* container() const noexcept
        {
            return m_buf;
        }

        SPIO_CONSTEXPR bool is_open() const noexcept
        {
            return m_buf != nullptr;
        }
        expected<void, failure> close() noexcept
        {
            Expects(m_buf != nullptr);
            m_buf = nullptr;
            return {};
        }

        result read(span<byte> s, bool& eof) noexcept
        {
            Expects(is_open());

            if (SPIO_UNLIKELY(m_it == m_buf->end() && s.size() != 0)) {
                return make_result(0, end_of_file);
            }
            auto dist = std::distance(m_it, m_buf->end());
            auto n = std::min(dist, static_cast<decltype(dist)>(s.size()));
            std::copy_n(m_it, n, s.begin());
            m_it += n;
            if (m_it == m_buf->end()) {
                eof = true;
            }
            return n;
        }
        result read_at(span<byte> s, streampos pos) noexcept
        {
            Expects(is_open());

            if (pos >= m_buf->size()) {
                return make_result(0, out_of_range);
            }
            auto dist = m_buf->size() - pos;
            auto n = std::min(dist, static_cast<decltype(dist)>(s.size()));
            std::copy_n(m_buf->begin() + pos, n, s.begin());
            if (n < s.size()) {
                return make_result(n, out_of_range);
            }
            return n;
        }

        result write(span<const byte> s)
        {
            Expects(is_open());

            if (SPIO_LIKELY(m_it == m_buf->end())) {
                append(s.begin(), s.end());
                m_it = m_buf->end();
            }
            else {
                auto dist = std::distance(m_it, m_buf->end());
                auto n = std::min(dist, s.size());
                m_it = std::copy_n(s.begin(), n, m_it);
                s = s.subspan(n);
                m_it = m_buf->insert(m_it, s.begin(), s.end());
            }
            return s.size();
        }
        result write_at(span<const byte> s, streampos pos)
        {
            Expects(is_open());

            if (pos >= m_buf->size()) {
                return make_result(0, out_of_range);
            }
            auto dist = m_buf->size() - pos;
            auto n = std::min(dist, s.size());
            const auto size = s.size();
            std::copy_n(s.begin(), n, m_buf->begin() + pos);
            pos += n;
            s = s.subspan(n);
            m_buf->insert(m_buf->begin() + pos, s.begin(), s.end());
            return size;
        }

        expected<streampos, failure> seek(streampos pos,
                                          inout which = in | out) noexcept
        {
            SPIO_UNUSED(which);
            return seek(pos, seekdir::beg);
        }
        expected<streampos, failure> seek(streamoff off,
                                          seekdir dir,
                                          inout which = in | out) noexcept
        {
            SPIO_UNUSED(which);
            Expects(is_open());

            if (dir == seekdir::beg) {
                if (m_buf->size() < off || off < 0) {
                    return make_unexpected(
                        failure{make_error_code(std::errc::invalid_argument),
                                "offset out of range"});
                }
                m_it = m_buf->begin() + off;
                return off;
            }
            if (dir == seekdir::cur) {
                if (off == 0) {
                    return std::distance(m_buf->begin(), m_it);
                }
                if (off < 0) {
                    auto dist = std::distance(m_buf->begin(), m_it);
                    if (dist < -off) {
                        return make_unexpected(failure{
                            make_error_code(std::errc::invalid_argument),
                            "offset out of range"});
                    }
                    m_it -= off;
                    return std::distance(m_buf->begin(), m_it);
                }
                auto dist = std::distance(m_it, m_buf->end());
                if (dist < off) {
                    return make_unexpected(
                        failure{make_error_code(std::errc::invalid_argument),
                                "offset out of range"});
                }
                m_it += off;
                return std::distance(m_buf->begin(), m_it);
            }

            if (m_buf->size() < -off || off > 0) {
                return make_unexpected(
                    failure{make_error_code(std::errc::invalid_argument),
                            "offset out of range"});
            }
            m_it = m_buf->end() + off;
            return std::distance(m_buf->begin(), m_it);
        }

        expected<streamsize, failure> extent() const noexcept
        {
            Expects(is_open());
            return static_cast<streamsize>(m_buf->size());
        }
        expected<streamsize, failure> truncate(streamsize newsize)
        {
            Expects(is_open());
            m_buf->resize(newsize);
            return extent();
        }

    private:
        template <typename Iterator,
                  typename C = container_type,
                  typename = void>
        struct has_append : std::false_type {
        };
        template <typename Iterator, typename C>
        struct has_append<Iterator,
                          C,
                          void_t<decltype(std::declval<C>().append(
                              std::declval<Iterator>(),
                              std::declval<Iterator>()))>> : std::true_type {
        };

        template <typename Iterator, typename C = container_type>
        auto append(Iterator b, Iterator e) ->
            typename std::enable_if<has_append<Iterator, C>::value>::type
        {
            m_buf->append(b, e);
        }
        template <typename Iterator, typename C = container_type>
        auto append(Iterator b, Iterator e) ->
            typename std::enable_if<!has_append<Iterator, C>::value>::type
        {
            m_buf->insert(m_buf->end(), b, e);
        }

        container_type* m_buf{nullptr};
        iterator m_it;
    };
}  // namespace detail

template <template <typename...> class Container>
class basic_container_device
    : private detail::basic_container_device_impl<Container> {
    using base = detail::basic_container_device_impl<Container>;

public:
    using base::base;
    using base::close;
    using base::container;
    using base::is_open;
    using base::read;
    using base::write;
};

template <template <typename...> class Container>
class basic_container_sink
    : private detail::basic_container_device_impl<Container> {
    using base = detail::basic_container_device_impl<Container>;

public:
    using base::base;
    using base::close;
    using base::container;
    using base::is_open;
    using base::write;
};

template <template <typename...> class Container>
class basic_container_source
    : private detail::basic_container_device_impl<Container> {
    using base = detail::basic_container_device_impl<Container>;

public:
    using base::base;
    using base::close;
    using base::container;
    using base::is_open;
    using base::read;
};

template <template <typename...> class Container>
class basic_random_access_container_device
    : private detail::basic_container_device_impl<Container> {
    using base = detail::basic_container_device_impl<Container>;

public:
    using base::base;
    using base::close;
    using base::container;
    using base::is_open;
    using base::read_at;
    using base::write_at;
};

template <template <typename...> class Container>
class basic_random_access_container_sink
    : private detail::basic_container_device_impl<Container> {
    using base = detail::basic_container_device_impl<Container>;

public:
    using base::base;
    using base::close;
    using base::container;
    using base::is_open;
    using base::write_at;
};

template <template <typename...> class Container>
class basic_random_access_container_source
    : private detail::basic_container_device_impl<Container> {
    using base = detail::basic_container_device_impl<Container>;

public:
    using base::base;
    using base::close;
    using base::container;
    using base::is_open;
    using base::read_at;
};

using vector_device = basic_container_device<std::vector>;
using vector_sink = basic_container_sink<std::vector>;
using vector_source = basic_container_source<std::vector>;

SPIO_END_NAMESPACE
}  // namespace spio

#endif  // SPIO_CONTAINER_DEVICE_H
