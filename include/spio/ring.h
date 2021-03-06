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

#ifndef SPIO_RING_H
#define SPIO_RING_H

#include "config.h"

#include "error.h"
#include "third_party/expected.h"
#include "third_party/gsl.h"
#include "third_party/optional.h"
#include "util.h"

#if SPIO_POSIX
#include <sys/mman.h>
#include <unistd.h>
#endif

#ifndef SPIO_RING_USE_MMAP
#define SPIO_RING_USE_MMAP SPIO_POSIX
#endif

namespace spio {
SPIO_BEGIN_NAMESPACE

namespace detail {
#if SPIO_POSIX && SPIO_RING_USE_MMAP
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

    class ring_base_posix {
    public:
        using value_type = byte;
        using size_type = std::ptrdiff_t;

        SPIO_CONSTEXPR ring_base_posix() = default;

        ring_base_posix(const ring_base_posix&) = delete;
        ring_base_posix& operator=(const ring_base_posix&) = delete;
        ring_base_posix(ring_base_posix&&) noexcept = default;
        ring_base_posix& operator=(ring_base_posix&&) noexcept = default;

        ~ring_base_posix() noexcept
        {
            ::munmap(m_ptr - m_size, static_cast<std::size_t>(m_size * 3));
        }

        expected<void, failure> init(size_type s) noexcept
        {
            auto rounded_size = round_up_power_of_two(s);
            auto page_size = ::sysconf(_SC_PAGESIZE);
            m_size = (rounded_size + page_size - 1) & ~(page_size - 1);

            char path[] = "/tmp/spio-ring-buffer-mirror-XXXXXX";
            int fd = ::mkstemp(path);
            if (fd < 0) {
                return make_unexpected(SPIO_MAKE_ERRNO);
            }

            if (::unlink(path)) {
                return make_unexpected(SPIO_MAKE_ERRNO);
            }
            if (::ftruncate(fd, m_size)) {
                return make_unexpected(SPIO_MAKE_ERRNO);
            }

            m_ptr = static_cast<byte*>(
                ::mmap(nullptr, static_cast<std::size_t>(m_size * 3), PROT_NONE,
                       MAP_ANONYMOUS | MAP_PRIVATE, -1, 0));
            if (m_ptr == MAP_FAILED) {
                return make_unexpected(SPIO_MAKE_ERRNO);
            }

            auto addr =
                ::mmap(m_ptr, static_cast<std::size_t>(m_size),
                       PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, fd, 0);
            if (addr != m_ptr) {
                ::munmap(m_ptr, static_cast<std::size_t>(m_size * 2));
                return make_unexpected(SPIO_MAKE_ERRNO);
            }

            auto addr2 =
                ::mmap(m_ptr + m_size, static_cast<std::size_t>(m_size),
                       PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, fd, 0);
            if (addr2 != m_ptr + m_size) {
                ::munmap(m_ptr, static_cast<std::size_t>(m_size * 2));
                /* ::munmap(addr, real_size); */
                return make_unexpected(SPIO_MAKE_ERRNO);
            }

            auto addr3 =
                ::mmap(m_ptr + m_size * 2, static_cast<std::size_t>(m_size),
                       PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, fd, 0);
            if (addr3 != m_ptr + m_size * 2) {
                ::munmap(m_ptr, static_cast<std::size_t>(m_size * 2));
                /* ::munmap(addr, real_size); */
                return make_unexpected(SPIO_MAKE_ERRNO);
            }

            if (::close(fd)) {
                ::munmap(m_ptr, static_cast<std::size_t>(m_size * 2));
                /* ::munmap(addr, real_size); */
                /* ::munmap(addr2, real_size); */
                return make_unexpected(SPIO_MAKE_ERRNO);
            }
            m_ptr += m_size;

            return {};
        }

        size_type write(span<const byte> s)
        {
            auto written =
                std::min(static_cast<size_type>(s.size()), free_space());
            std::copy(s.begin(), s.begin() + written, m_ptr + m_head);
            m_head += written;
            if (m_size < m_head) {
                m_head &= (m_size - 1);
            }
            if (s.size() != 0) {
                m_empty = false;
            }
            return written;
        }
        size_type write_tail(span<const byte> s)
        {
            auto written =
                std::min(static_cast<size_type>(s.size()), free_space());
            std::reverse_copy(s.rbegin(), s.rbegin() + written,
                              m_ptr + m_tail - written);
            m_tail -= written;
            if (m_tail < 0) {
                m_tail &= (m_size - 1);
            }
            if (s.size() != 0) {
                m_empty = true;
            }
            return written;
        }
        size_type read(span<byte> s)
        {
            if (empty()) {
                return 0;
            }

            auto n = std::min(static_cast<size_type>(s.size()), in_use());
            std::copy(m_ptr + m_tail, m_ptr + m_tail + n, s.begin());
            m_tail += n;
            if (m_size < m_tail) {
                m_tail &= (m_size - 1);
            }
            if (m_head == m_tail) {
                m_empty = true;
            }
            return n;
        }

        span<const value_type> peek(size_type n) const noexcept
        {
            Expects(size() >= n);
            return make_span(m_ptr + m_tail - n, n);
        }

        void clear() noexcept
        {
            m_head = m_tail;
            m_empty = true;
        }

        size_type in_use() const noexcept
        {
            return m_tail <= m_head ? m_head - m_tail
                                    : m_size - (m_tail - m_head);
        }
        size_type free_space() const noexcept
        {
            return m_size - in_use();
        }

        size_type size() const noexcept
        {
            return m_size;
        }
        bool empty() const noexcept
        {
            return m_head == m_tail && m_empty;
        }

        value_type* data() noexcept
        {
            return m_ptr;
        }
        const value_type* data() const noexcept
        {
            return m_ptr;
        }

        size_type head() const noexcept
        {
            return m_head;
        }
        size_type tail() const noexcept
        {
            return m_tail;
        }

        class direct_read_t {
        public:
            direct_read_t(ring_base_posix& r,
                                         size_type n,
                                         size_type i = 0) noexcept
                : m_ring(r), m_n(n), m_i(i)
            {
            }

            direct_read_t begin() noexcept
            {
                return *this;
            }
            direct_read_t end() const noexcept
            {
                return {m_ring, m_n, 1};
            }

            span<const byte> operator*() noexcept
            {
                return make_span(m_ring.data() + m_ring.tail(), m_n);
            }

            direct_read_t& operator++() noexcept
            {
                m_ring.move_tail(m_n);
                ++m_i;
                return *this;
            }
            direct_read_t operator++(int) noexcept
            {
                direct_read_t tmp(*this);
                operator++();
                return tmp;
            }

            bool operator==(const direct_read_t& r) const
                noexcept
            {
                return m_i == r.m_i;
            }
            bool operator!=(const direct_read_t& r) const
                noexcept
            {
                return !(*this == r);
            }

        private:
            ring_base_posix& m_ring;
            size_type m_n;
            size_type m_i;
        };

        direct_read_t direct_read(size_type n) noexcept
        {
            Expects(n <= in_use());
            return direct_read_t(*this, n);
        }

        class direct_write_t {
        public:
            direct_write_t(ring_base_posix& r,
                                          size_type n,
                                          size_type i = 0) noexcept
                : m_ring(r), m_n(n), m_i(i)
            {
            }

            direct_write_t begin() noexcept
            {
                return *this;
            }
            direct_write_t end() const noexcept
            {
                return {m_ring, m_n, 1};
            }

            span<byte> operator*() noexcept
            {
                return make_span(m_ring.data() + m_ring.head(), m_n);
            }

            direct_write_t& operator++() noexcept
            {
                m_ring.move_head(m_n);
                ++m_i;
                return *this;
            }
            direct_write_t operator++(int) noexcept
            {
                direct_write_t tmp(*this);
                operator++();
                return tmp;
            }

            bool operator==(const direct_write_t& r) const
                noexcept
            {
                return m_i == r.m_i;
            }
            bool operator!=(const direct_write_t& r) const
                noexcept
            {
                return !(*this == r);
            }

        private:
            ring_base_posix& m_ring;
            size_type m_n;
            size_type m_i;
        };

        direct_write_t direct_write(size_type n) noexcept
        {
            Expects(n <= free_space());
            return direct_write_t(*this, n);
        }

        void move_head(size_type off) noexcept
        {
            m_head += off;
            m_head &= (m_size - 1);
            m_empty = false;
        }
        void move_tail(size_type off) noexcept
        {
            m_tail += off;
            m_tail &= (m_size - 1);
            if (m_head == m_tail)
                m_empty = true;
        }

    private:
        value_type* m_ptr{};
        size_type m_size{};
        size_type m_head{0};
        size_type m_tail{0};
        bool m_empty{true};
    };

    using ring_base = ring_base_posix;
#else
    class ring_base_std {
    public:
        using value_type = byte;
        using storage_type = std::unique_ptr<value_type[]>;
        using size_type = std::ptrdiff_t;

        SPIO_CONSTEXPR ring_base_std() = default;

        expected<void, failure> init(size_type s)
        {
            m_buf = storage_type(new value_type[static_cast<std::size_t>(s)]);
            m_size = s;
            return {};
        }

        size_type write(span<const byte> s)
        {
            if (m_head < m_tail) {
                auto n =
                    std::min(static_cast<size_type>(s.size()), m_tail - m_head);
                std::copy(s.begin(), s.begin() + n, m_buf.get() + m_head);
                m_head += n;
                if (m_head == m_size) {
                    m_head = 0;
                }
                return n;
            }
            auto space_end = m_size - m_head;
            auto n = std::min(space_end, static_cast<size_type>(s.size()));
            std::copy(s.begin(), s.begin() + n, m_buf.get() + m_head);
            s = s.subspan(n);
            m_head += n;
            if (m_head == m_size) {
                m_head = 0;
            }
            if (s.size() == 0) {
                return n;
            }

            return write(s);
        }
        size_type write_tail(span<const byte> s)
        {
            auto written = std::min(static_cast<size_type>(s.size()), m_tail);
            std::reverse_copy(s.rbegin(), s.rbegin() + written,
                              m_buf.get() + m_tail - written);
            m_tail -= written;
            s = s.first(s.size() - written);
            if (s.size() == 0) {
                return written;
            }

            auto space =
                std::min(static_cast<size_type>(s.size()), m_size - m_head - 1);
            std::reverse_copy(s.rbegin(), s.rbegin() + space,
                              m_buf.get() + m_size - space);
            m_tail = m_size - space;
            return written + space;
        }
        size_type read(span<byte> s)
        {
            if (empty()) {
                return 0;
            }

            if (m_tail < m_head) {
                auto n =
                    std::min(static_cast<size_type>(s.size()), m_head - m_tail);
                std::copy(m_buf.get() + m_tail, m_buf.get() + m_tail + n,
                          s.begin());
                m_tail += n;
                if (m_tail == m_size) {
                    m_tail = 0;
                }
                if (m_head == m_tail) {
                    m_empty = true;
                }
                return n;
            }

            auto space_end = m_size - m_tail;
            auto n = std::min(space_end, static_cast<size_type>(s.size()));
            std::copy(m_buf.get() + m_tail, m_buf.get() + m_tail + n,
                      s.begin());
            s = s.subspan(n);
            m_tail += n;
            if (m_tail == m_size) {
                m_tail = 0;
            }
            if (s.size() == 0) {
                return n;
            }

            return read(s) + n;
        }

        span<const value_type> peek(size_type n) const noexcept
        {
            Expects(size() >= n);
            return make_span(m_buf.get() + m_tail - n, n);
        }

        void clear() noexcept
        {
            m_head = m_tail;
            m_empty = true;
        }

        size_type size() const noexcept
        {
            return m_size;
        }
        bool empty() const noexcept
        {
            return m_head == m_tail && m_empty;
        }

        size_type in_use() const noexcept
        {
            return m_tail <= m_head ? m_head - m_tail
                                    : m_size - (m_tail - m_head);
        }
        size_type free_space() const noexcept
        {
            return m_size - in_use();
        }

        value_type* data() noexcept
        {
            return m_buf.get();
        }
        const value_type* data() const noexcept
        {
            return m_buf.get();
        }

        size_type head() const noexcept
        {
            return m_head;
        }
        size_type tail() const noexcept
        {
            return m_tail;
        }

        class direct_read_t {
        public:
            SPIO_CONSTEXPR direct_read_t(ring_base_std& r,
                                         size_type n,
                                         size_type i = 0) noexcept
                : m_ring(r), m_n(n), m_i(i)
            {
            }

            direct_read_t begin() noexcept
            {
                return *this;
            }
            direct_read_t end() const noexcept
            {
                return {m_ring, m_n, two_ranges() ? 2 : 1};
            }

            span<const byte> operator*() noexcept
            {
                if (two_ranges()) {
                    if (m_i == 0) {
                        return make_span(m_ring.data() + m_ring.tail(),
                                         m_ring.size() - m_ring.tail());
                    }
                    return make_span(m_ring.data(),
                                     m_n - (m_ring.size() - m_ring.tail()));
                }
                return make_span(m_ring.data() + m_ring.tail(), m_n);
            }

            direct_read_t& operator++() noexcept
            {
                m_ring.move_tail(m_n);
                ++m_i;
                return *this;
            }
            direct_read_t operator++(int) noexcept
            {
                direct_read_t tmp(*this);
                operator++();
                return tmp;
            }

            bool operator==(const direct_read_t& r) const noexcept
            {
                return m_i == r.m_i;
            }
            bool operator!=(const direct_read_t& r) const noexcept
            {
                return !(*this == r);
            }

        private:
            bool two_ranges() const noexcept
            {
                return m_ring.head() <= m_ring.tail() &&
                       m_n < m_ring.size() - m_ring.tail();
            }

            ring_base_std& m_ring;
            size_type m_n;
            size_type m_i;
        };

        direct_read_t direct_read(size_type n) noexcept
        {
            Expects(n <= in_use());
            return direct_read_t(*this, n);
        }

        class direct_write_t {
        public:
            direct_write_t(ring_base_std& r,
                           size_type n,
                           size_type i = 0) noexcept
                : m_ring(r), m_n(n), m_i(i)
            {
            }

            direct_write_t begin() noexcept
            {
                return *this;
            }
            direct_write_t end() const noexcept
            {
                return {m_ring, m_n, two_ranges() ? 2 : 1};
            }

            span<byte> operator*() noexcept
            {
                if (two_ranges()) {
                    if (m_i == 0) {
                        return make_span(m_ring.data() + m_ring.head(),
                                         m_ring.size() - m_ring.head());
                    }
                    return make_span(m_ring.data(),
                                     m_n - (m_ring.size() - m_ring.head()));
                }
                return make_span(m_ring.data() + m_ring.head(), m_n);
            }

            direct_write_t& operator++() noexcept
            {
                m_ring.move_head(m_n);
                ++m_i;
                return *this;
            }
            direct_write_t operator++(int) noexcept
            {
                direct_write_t tmp(*this);
                operator++();
                return tmp;
            }

            bool operator==(const direct_write_t& r) const noexcept
            {
                return m_i == r.m_i;
            }
            bool operator!=(const direct_write_t& r) const noexcept
            {
                return !(*this == r);
            }

        private:
            bool two_ranges() const noexcept
            {
                return m_ring.tail() <= m_ring.head() &&
                       m_n > m_ring.size() - m_ring.head();
            }

            ring_base_std& m_ring;
            size_type m_n;
            size_type m_i;
        };

        direct_write_t direct_write(size_type n) noexcept
        {
            Expects(n <= free_space());
            return direct_write_t(*this, n);
        }

        void move_head(size_type off) noexcept
        {
            m_head += off;
            m_head &= (m_size - 1);
            m_empty = false;
            Ensures(m_head > m_tail);
        }
        void move_tail(size_type off) noexcept
        {
            Expects(m_tail + off <= m_head);
            m_tail += off;
            m_tail &= (m_size - 1);
            if (m_head == m_tail)
                m_empty = true;
        }

    private:
        storage_type m_buf{};
        size_type m_size{};
        size_type m_head{0};
        size_type m_tail{0};
        bool m_empty{true};
    };

    using ring_base = ring_base_std;
#endif
}  // namespace detail

template <typename T>
class basic_ring {
public:
    using value_type = T;
    using size_type = std::ptrdiff_t;

    basic_ring(size_type n) : m_buf{}
    {
        auto r =
            m_buf.init(n * static_cast<std::ptrdiff_t>(sizeof(value_type)));
        if (!r) {
            throw r.error();
        }
    }

    size_type write(span<const value_type> data)
    {
        return m_buf.write(as_bytes(data)) /
               static_cast<std::ptrdiff_t>(sizeof(value_type));
    }
    size_type write_tail(span<const value_type> data)
    {
        return m_buf.write_tail(as_bytes(data)) /
               static_cast<std::ptrdiff_t>(sizeof(value_type));
    }
    size_type read(span<value_type> data)
    {
        return m_buf.read(as_writeable_bytes(data)) /
               static_cast<std::ptrdiff_t>(sizeof(value_type));
    }

    span<const value_type> peek(size_type n) const noexcept
    {
        auto s =
            m_buf.peek(n * static_cast<std::ptrdiff_t>(sizeof(value_type)));
        return make_span(
            reinterpret_cast<const value_type*>(s.data()),
            s.size() / static_cast<std::ptrdiff_t>(sizeof(value_type)));
    }

    void clear() noexcept
    {
        m_buf.clear();
    }

    size_type size() const noexcept
    {
        return m_buf.size() / static_cast<size_type>(sizeof(value_type));
    }
    bool empty() const noexcept
    {
        return m_buf.empty();
    }

    size_type in_use() const noexcept
    {
        return m_buf.in_use() / static_cast<size_type>(sizeof(value_type));
    }
    size_type free_space() const noexcept
    {
        return m_buf.free_space() / static_cast<size_type>(sizeof(value_type));
    }

    span<value_type> get_span() noexcept
    {
        return make_span(reinterpret_cast<value_type*>(m_buf.data()), size());
    }
    span<const value_type> get_span() const noexcept
    {
        return make_span(reinterpret_cast<const value_type*>(m_buf.data()),
                         size());
    }

private:
    detail::ring_base m_buf;
};
template <>
class basic_ring<byte> : public detail::ring_base {
    using base = detail::ring_base;

public:
    using value_type = byte;
    using size_type = std::ptrdiff_t;

    basic_ring(size_type n) : base{}
    {
        auto r = base::init(n * static_cast<size_type>(sizeof(value_type)));
        if (!r) {
            throw r.error();
        }
    }

    span<value_type> get_span() noexcept
    {
        return make_span(data(), size());
    }
    span<const value_type> get_span() const noexcept
    {
        return make_span(data(), size());
    }
};

using ring = basic_ring<byte>;

SPIO_END_NAMESPACE
}  // namespace spio

#endif  // SPIO_RING_H
