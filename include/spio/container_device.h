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

#ifndef SPIO_CONTAINER_DEVICE_H
#define SPIO_CONTAINER_DEVICE_H

#include "config.h"

#include "device.h"
#include "error.h"
#include "result.h"
#include "third_party/expected.h"
#include "third_party/gsl.h"
#include "util.h"

SPIO_BEGIN_NAMESPACE

namespace detail {
template <template <typename...> class Container, bool IsConst>
class basic_container_device_impl {
    using _container_type = Container<gsl::byte>;

public:
    using container_type = typename std::conditional<
        IsConst,
        typename std::add_const<_container_type>::type,
        _container_type>::type;
    using const_iterator = typename container_type::const_iterator;
    using iterator =
        typename std::conditional<IsConst,
                                  const_iterator,
                                  typename container_type::iterator>::type;

    basic_container_device_impl() = default;
    basic_container_device_impl(container_type& c)
        : m_buf(std::addressof(c)), m_it(m_buf->begin())
    {
    }

    container_type* container()
    {
        return m_buf;
    }
    typename std::add_const<container_type*>::type container() const
    {
        return m_buf;
    }

    bool is_open() const
    {
        return m_buf != nullptr;
    }
    void close()
    {
        Expects(m_buf != nullptr);
        m_buf = nullptr;
    }

    result read(gsl::span<gsl::byte> s, bool& eof)
    {
        Expects(is_open());

        if (m_it == m_buf->end() && s.size() != 0) {
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
    result read_at(gsl::span<gsl::byte> s, streampos pos)
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

    template <bool C = IsConst>
    auto write(gsl::span<const gsl::byte> s) ->
        typename std::enable_if<!C, result>::type
    {
        Expects(is_open());

        if (m_it == m_buf->end()) {
            m_buf->reserve(m_buf->size() + static_cast<std::size_t>(s.size()));
            append(s.begin(), s.end());
            m_it = m_buf->end();
        }
        else {
            auto dist = std::distance(m_buf->begin(), m_it);
            if (static_cast<std::size_t>(s.size()) > m_buf->size()) {
                m_buf->reserve(m_buf->size() +
                               static_cast<std::size_t>(s.size()));
                m_buf->insert(m_buf->begin() + dist, s.begin(), s.end());
            }
            else {
                m_buf->insert(m_it, s.begin(), s.end());
            }
            m_it = m_buf->begin() + dist + s.size();
        }
        return s.size();
    }
    template <bool C = IsConst>
    auto write_at(gsl::span<const gsl::byte> s, streampos pos)
    {
        Expects(is_open());

        if (pos >= m_buf->size()) {
            return make_result(0, out_of_range);
        }
        m_buf->insert(m_buf->begin() + pos, s.begin(), s.end());
        return s.size();
    }

    nonstd::expected<streampos, failure> seek(streampos pos,
                                              inout which = in | out)
    {
        SPIO_UNUSED(which);
        return seek(pos, seekdir::beg);
    }
    nonstd::expected<streampos, failure> seek(streamoff off,
                                              seekdir dir,
                                              inout which = in | out)
    {
        SPIO_UNUSED(which);
        Expects(is_open());

        if (dir == seekdir::beg) {
            if (m_buf->size() < off || off < 0) {
                return nonstd::make_unexpected(
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
                    return nonstd::make_unexpected(
                        failure{make_error_code(std::errc::invalid_argument),
                                "offset out of range"});
                }
                m_it -= off;
                return std::distance(m_buf->begin(), m_it);
            }
            auto dist = std::distance(m_it, m_buf->end());
            if (dist < off) {
                return nonstd::make_unexpected(
                    failure{make_error_code(std::errc::invalid_argument),
                            "offset out of range"});
            }
            m_it += off;
            return std::distance(m_buf->begin(), m_it);
        }

        if (m_buf->size() < -off || off > 0) {
            return nonstd::make_unexpected(
                failure{make_error_code(std::errc::invalid_argument),
                        "offset out of range"});
        }
        m_it = m_buf->end() + off;
        return std::distance(m_buf->begin(), m_it);
    }

    nonstd::expected<streamsize, failure> extent() const
    {
        Expects(is_open());
        return static_cast<streamsize>(m_buf->size());
    }
    nonstd::expected<streamsize, failure> truncate(streamsize newsize)
    {
        Expects(is_open());
        m_buf->truncate(newsize);
        return extent();
    }

private:
    template <typename Iterator, typename C = container_type, typename = void>
    struct has_append : std::false_type {
    };
    template <typename Iterator, typename C>
    struct has_append<
        Iterator,
        C,
        void_t<decltype(std::declval<C>().append(std::declval<Iterator>(),
                                                 std::declval<Iterator>()))>>
        : std::true_type {
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
    : private detail::basic_container_device_impl<Container, false> {
    using base = detail::basic_container_device_impl<Container, false>;

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
    : private detail::basic_container_device_impl<Container, false> {
    using base = detail::basic_container_device_impl<Container, false>;

public:
    using base::base;
    using base::close;
    using base::container;
    using base::is_open;
    using base::write;
};

template <template <typename...> class Container>
class basic_container_source
    : private detail::basic_container_device_impl<Container, true> {
    using base = detail::basic_container_device_impl<Container, true>;

public:
    using base::base;
    using base::close;
    using base::container;
    using base::is_open;
    using base::read;
};

using vector_device = basic_container_device<std::vector>;
using vector_sink = basic_container_sink<std::vector>;
using vector_source = basic_container_source<std::vector>;

SPIO_END_NAMESPACE

#endif  // SPIO_CONTAINER_DEVICE_H
