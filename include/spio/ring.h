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

SPIO_BEGIN_NAMESPACE

namespace detail {
#if SPIO_POSIX && SPIO_RING_USE_MMAP
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

class ring_base_posix {
public:
    using value_type = gsl::byte;
    using size_type = std::ptrdiff_t;

    ring_base_posix() = default;

    ring_base_posix(const ring_base_posix&) = delete;
    ring_base_posix& operator=(const ring_base_posix&) = delete;
    ring_base_posix(ring_base_posix&&) SPIO_NOEXCEPT = default;
    ring_base_posix& operator=(ring_base_posix&&) SPIO_NOEXCEPT = default;

    ~ring_base_posix() SPIO_NOEXCEPT
    {
        ::munmap(m_ptr - m_size, static_cast<std::size_t>(m_size * 3));
    }

    nonstd::expected<void, failure> init(size_type size)
    {
        auto rounded_size = round_up_power_of_two(size);
        auto page_size = ::sysconf(_SC_PAGESIZE);
        m_size = (rounded_size + page_size - 1) & ~(page_size - 1);

        char path[] = "/tmp/spio-ring-buffer-mirror-XXXXXX";
        int fd = ::mkstemp(path);
        if (fd < 0) {
            return nonstd::make_unexpected(SPIO_MAKE_ERRNO);
        }

        if (::unlink(path)) {
            return nonstd::make_unexpected(SPIO_MAKE_ERRNO);
        }
        if (::ftruncate(fd, m_size)) {
            return nonstd::make_unexpected(SPIO_MAKE_ERRNO);
        }

        m_ptr = static_cast<gsl::byte*>(
            ::mmap(nullptr, static_cast<std::size_t>(m_size * 3), PROT_NONE,
                   MAP_ANONYMOUS | MAP_PRIVATE, -1, 0));
        if (m_ptr == MAP_FAILED) {
            return nonstd::make_unexpected(SPIO_MAKE_ERRNO);
        }

        auto addr =
            ::mmap(m_ptr, static_cast<std::size_t>(m_size),
                   PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, fd, 0);
        if (addr != m_ptr) {
            ::munmap(m_ptr, static_cast<std::size_t>(m_size * 2));
            return nonstd::make_unexpected(SPIO_MAKE_ERRNO);
        }

        auto addr2 =
            ::mmap(m_ptr + m_size, static_cast<std::size_t>(m_size),
                   PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, fd, 0);
        if (addr2 != m_ptr + m_size) {
            ::munmap(m_ptr, static_cast<std::size_t>(m_size * 2));
            /* ::munmap(addr, real_size); */
            return nonstd::make_unexpected(SPIO_MAKE_ERRNO);
        }

        auto addr3 =
            ::mmap(m_ptr + m_size * 2, static_cast<std::size_t>(m_size),
                   PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, fd, 0);
        if (addr3 != m_ptr + m_size * 2) {
            ::munmap(m_ptr, static_cast<std::size_t>(m_size * 2));
            /* ::munmap(addr, real_size); */
            return nonstd::make_unexpected(SPIO_MAKE_ERRNO);
        }

        if (::close(fd)) {
            ::munmap(m_ptr, static_cast<std::size_t>(m_size * 2));
            /* ::munmap(addr, real_size); */
            /* ::munmap(addr2, real_size); */
            return nonstd::make_unexpected(SPIO_MAKE_ERRNO);
        }
        m_ptr += m_size;

        return {};
    }

    size_type write(gsl::span<const gsl::byte> data)
    {
        auto written =
            std::min(static_cast<size_type>(data.size()), free_space());
        std::copy(data.begin(), data.begin() + written, m_ptr + m_head);
        m_head += written;
        if (m_size < m_head) {
            m_head &= (m_size - 1);
        }
        if (data.size() != 0) {
            m_empty = false;
        }
        return written;
    }
    size_type write_tail(gsl::span<const gsl::byte> data)
    {
        auto written =
            std::min(static_cast<size_type>(data.size()), free_space());
        std::reverse_copy(data.rbegin(), data.rbegin() + written,
                          m_ptr + m_tail - written);
        m_tail -= written;
        if (m_tail < 0) {
            m_tail &= (m_size - 1);
        }
        if (data.size() != 0) {
            m_empty = true;
        }
        return written;
    }
    size_type read(gsl::span<gsl::byte> data)
    {
        if (empty()) {
            return 0;
        }

        auto n = std::min(static_cast<size_type>(data.size()), in_use());
        std::copy(m_ptr + m_tail, m_ptr + m_tail + n, data.begin());
        m_tail += n;
        if (m_size < m_tail) {
            m_tail &= (m_size - 1);
        }
        if (m_head == m_tail) {
            m_empty = true;
        }
        return n;
    }

    gsl::span<const value_type> peek(size_type n) const
    {
        Expects(size() >= n);
        return gsl::make_span(m_ptr + m_tail - n, n);
    }

    void clear()
    {
        m_head = m_tail;
        m_empty = true;
    }

    size_type in_use() const
    {
        if (m_tail <= m_head) {
            return m_head - m_tail;
        }
        return m_size - (m_tail - m_head);
    }
    size_type free_space() const
    {
        return m_size - in_use();
    }

    size_type size() const
    {
        return m_size;
    }
    bool empty() const
    {
        return m_head == m_tail && m_empty;
    }

    value_type* data()
    {
        return m_ptr;
    }
    const value_type* data() const
    {
        return m_ptr;
    }
    size_type head() const
    {
        return m_head;
    }
    size_type tail() const
    {
        return m_tail;
    }

    class direct_read_t {
    public:
        direct_read_t(ring_base_posix& r, size_type n, size_type i = 0)
            : m_ring(r), m_n(n), m_i(i)
        {
        }

        direct_read_t begin()
        {
            return *this;
        }
        direct_read_t end() const
        {
            return {m_ring, m_n, 1};
        }

        gsl::span<const gsl::byte> operator*()
        {
            return gsl::make_span(m_ring.data() + m_ring.tail(), m_n);
        }

        direct_read_t& operator++()
        {
            m_ring.move_tail(m_n);
            ++m_i;
            return *this;
        }
        direct_read_t operator++(int)
        {
            direct_read_t tmp(*this);
            operator++();
            return tmp;
        }

        bool operator==(const direct_read_t& r) const
        {
            return m_i == r.m_i;
        }
        bool operator!=(const direct_read_t& r) const
        {
            return !(*this == r);
        }

    private:
        ring_base_posix& m_ring;
        size_type m_n;
        size_type m_i;
    };

    direct_read_t direct_read(size_type n)
    {
        Expects(n <= in_use());
        return direct_read_t(*this, n);
    }

    class direct_write_t {
    public:
        direct_write_t(ring_base_posix& r, size_type n, size_type i = 0)
            : m_ring(r), m_n(n), m_i(i)
        {
        }

        direct_write_t begin()
        {
            return *this;
        }
        direct_write_t end() const
        {
            return {m_ring, m_n, 1};
        }

        gsl::span<gsl::byte> operator*()
        {
            return gsl::make_span(m_ring.data() + m_ring.head(), m_n);
        }

        direct_write_t& operator++()
        {
            m_ring.move_head(m_n);
            ++m_i;
            return *this;
        }
        direct_write_t operator++(int)
        {
            direct_write_t tmp(*this);
            operator++();
            return tmp;
        }

        bool operator==(const direct_write_t& r) const
        {
            return m_i == r.m_i;
        }
        bool operator!=(const direct_write_t& r) const
        {
            return !(*this == r);
        }

    private:
        ring_base_posix& m_ring;
        size_type m_n;
        size_type m_i;
    };

    direct_write_t direct_write(size_type n)
    {
        Expects(n <= free_space());
        return direct_write_t(*this, n);
    }

    void move_head(size_type off)
    {
        m_head += off;
        m_head &= (m_size - 1);
        m_empty = false;
    }
    void move_tail(size_type off)
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
    using value_type = gsl::byte;
    using storage_type = std::unique_ptr<value_type[]>;
    using size_type = std::ptrdiff_t;

    ring_base_std() = default;

    nonstd::expected<void, failure> init(size_type size)
    {
        m_buf = storage_type(new value_type[static_cast<std::size_t>(size)]);
        m_size = size;
        return {};
    }

    size_type write(gsl::span<const gsl::byte> data)
    {
        if (m_head < m_tail) {
            auto n =
                std::min(static_cast<size_type>(data.size()), m_tail - m_head);
            std::copy(data.begin(), data.begin() + n, m_buf.get() + m_head);
            m_head += n;
            if (m_head == m_size) {
                m_head = 0;
            }
            return n;
        }
        auto space_end = m_size - m_head;
        auto n = std::min(space_end, static_cast<size_type>(data.size()));
        std::copy(data.begin(), data.begin() + n, m_buf.get() + m_head);
        data = data.subspan(n);
        m_head += n;
        if (m_head == m_size) {
            m_head = 0;
        }
        if (data.size() == 0) {
            return n;
        }

        return write(data);
    }
    size_type write_tail(gsl::span<const gsl::byte> data)
    {
        auto written = std::min(static_cast<size_type>(data.size()), m_tail);
        std::reverse_copy(data.rbegin(), data.rbegin() + written,
                          m_buf.get() + m_tail - written);
        m_tail -= written;
        data = data.first(data.size() - written);
        if (data.size() == 0) {
            return written;
        }

        auto space =
            std::min(static_cast<size_type>(data.size()), m_size - m_head - 1);
        std::reverse_copy(data.rbegin(), data.rbegin() + space,
                          m_buf.get() + m_size - space);
        m_tail = m_size - space;
        return written + space;
    }
    size_type read(gsl::span<gsl::byte> data)
    {
        if (empty()) {
            return 0;
        }

        if (m_tail < m_head) {
            auto n =
                std::min(static_cast<size_type>(data.size()), m_head - m_tail);
            std::copy(m_buf.get() + m_tail, m_buf.get() + m_tail + n,
                      data.begin());
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
        auto n = std::min(space_end, static_cast<size_type>(data.size()));
        std::copy(m_buf.get() + m_tail, m_buf.get() + m_tail + n, data.begin());
        data = data.subspan(n);
        m_tail += n;
        if (m_tail == m_size) {
            m_tail = 0;
        }
        if (data.size() == 0) {
            return n;
        }

        return read(data) + n;
    }

    gsl::span<const value_type> peek(size_type n) const
    {
        Expects(size() >= n);
        return gsl::make_span(m_buf.get() + m_tail - n, n);
    }

    void clear()
    {
        m_head = m_tail;
        m_empty = true;
    }

    size_type size() const
    {
        return m_size;
    }
    bool empty() const
    {
        return m_head == m_tail && m_empty;
    }

    size_type in_use() const
    {
        if (m_tail <= m_head) {
            return m_head - m_tail;
        }
        return m_size - (m_tail - m_head);
    }
    size_type free_space() const
    {
        return m_size - in_use();
    }

    value_type* data()
    {
        return m_buf.get();
    }
    const value_type* data() const
    {
        return m_buf.get();
    }
    size_type head() const
    {
        return m_head;
    }
    size_type tail() const
    {
        return m_tail;
    }

    class direct_read_t {
    public:
        direct_read_t(ring_base_std& r, size_type n, size_type i = 0)
            : m_ring(r), m_n(n), m_i(i)
        {
        }

        direct_read_t begin()
        {
            return *this;
        }
        direct_read_t end() const
        {
            return {m_ring, m_n, two_ranges() ? 2 : 1};
        }

        gsl::span<const gsl::byte> operator*()
        {
            if (two_ranges()) {
                if (m_i == 0) {
                    return gsl::make_span(m_ring.data() + m_ring.tail(),
                                          m_ring.size() - m_ring.tail());
                }
                return gsl::make_span(m_ring.data(),
                                      m_n - (m_ring.size() - m_ring.tail()));
            }
            return gsl::make_span(m_ring.data() + m_ring.tail(), m_n);
        }

        direct_read_t& operator++()
        {
            m_ring.move_tail(m_n);
            ++m_i;
            return *this;
        }
        direct_read_t operator++(int)
        {
            direct_read_t tmp(*this);
            operator++();
            return tmp;
        }

        bool operator==(const direct_read_t& r) const
        {
            return m_i == r.m_i;
        }
        bool operator!=(const direct_read_t& r) const
        {
            return !(*this == r);
        }

    private:
        bool two_ranges() const
        {
            return m_ring.head() <= m_ring.tail() &&
                   m_n < m_ring.size() - m_ring.tail();
        }

        ring_base_std& m_ring;
        size_type m_n;
        size_type m_i;
    };

    direct_read_t direct_read(size_type n)
    {
        Expects(n <= in_use());
        return direct_read_t(*this, n);
    }

    class direct_write_t {
    public:
        direct_write_t(ring_base_std& r, size_type n, size_type i = 0)
            : m_ring(r), m_n(n), m_i(i)
        {
        }

        direct_write_t begin()
        {
            return *this;
        }
        direct_write_t end() const
        {
            return {m_ring, m_n, two_ranges() ? 2 : 1};
        }

        gsl::span<gsl::byte> operator*()
        {
            if (two_ranges()) {
                if (m_i == 0) {
                    return gsl::make_span(m_ring.data() + m_ring.head(),
                                          m_ring.size() - m_ring.head());
                }
                return gsl::make_span(m_ring.data(),
                                      m_n - (m_ring.size() - m_ring.head()));
            }
            return gsl::make_span(m_ring.data() + m_ring.head(), m_n);
        }

        direct_write_t& operator++()
        {
            m_ring.move_head(m_n);
            ++m_i;
            return *this;
        }
        direct_write_t operator++(int)
        {
            direct_write_t tmp(*this);
            operator++();
            return tmp;
        }

        bool operator==(const direct_write_t& r) const
        {
            return m_i == r.m_i;
        }
        bool operator!=(const direct_write_t& r) const
        {
            return !(*this == r);
        }

    private:
        bool two_ranges() const
        {
            return m_ring.tail() <= m_ring.head() &&
                   m_n > m_ring.size() - m_ring.head();
        }

        ring_base_std& m_ring;
        size_type m_n;
        size_type m_i;
    };

    direct_write_t direct_write(size_type n)
    {
        Expects(n <= free_space());
        return direct_write_t(*this, n);
    }

    void move_head(size_type off)
    {
        m_head += off;
        m_head &= (m_size - 1);
        m_empty = false;
        Ensures(m_head > m_tail);
    }
    void move_tail(size_type off)
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

    size_type write(gsl::span<const value_type> data)
    {
        return m_buf.write(gsl::as_bytes(data)) /
               static_cast<std::ptrdiff_t>(sizeof(value_type));
    }
    size_type write_tail(gsl::span<const value_type> data)
    {
        return m_buf.write_tail(gsl::as_bytes(data)) /
               static_cast<std::ptrdiff_t>(sizeof(value_type));
    }
    size_type read(gsl::span<value_type> data)
    {
        return m_buf.read(gsl::as_writeable_bytes(data)) /
               static_cast<std::ptrdiff_t>(sizeof(value_type));
    }

    gsl::span<const value_type> peek(size_type n) const
    {
        auto s =
            m_buf.peek(n * static_cast<std::ptrdiff_t>(sizeof(value_type)));
        return gsl::make_span(
            reinterpret_cast<const value_type*>(s.data()),
            s.size() / static_cast<std::ptrdiff_t>(sizeof(value_type)));
    }

    void clear()
    {
        m_buf.clear();
    }

    size_type size() const
    {
        return m_buf.size() / static_cast<size_type>(sizeof(value_type));
    }
    bool empty() const
    {
        return m_buf.empty();
    }

    size_type in_use() const
    {
        return m_buf.in_use() / static_cast<size_type>(sizeof(value_type));
    }
    size_type free_space() const
    {
        return m_buf.free_space() / static_cast<size_type>(sizeof(value_type));
    }

    gsl::span<value_type> span()
    {
        return gsl::make_span(reinterpret_cast<value_type*>(m_buf.data()),
                              size());
    }
    gsl::span<const value_type> span() const
    {
        return gsl::make_span(reinterpret_cast<const value_type*>(m_buf.data()),
                              size());
    }

private:
    detail::ring_base m_buf;
};
template <>
class basic_ring<gsl::byte> : public detail::ring_base {
    using base = detail::ring_base;

public:
    using value_type = gsl::byte;
    using size_type = std::ptrdiff_t;

    basic_ring(size_type n) : base{}
    {
        auto r = base::init(n * static_cast<size_type>(sizeof(value_type)));
        if (!r) {
            throw r.error();
        }
    }

    gsl::span<value_type> span()
    {
        return gsl::make_span(data(), size());
    }
    gsl::span<const value_type> span() const
    {
        return gsl::make_span(data(), size());
    }
};

using ring = basic_ring<gsl::byte>;

SPIO_END_NAMESPACE

#endif  // SPIO_RING_H
