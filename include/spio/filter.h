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

#ifndef SPIO_FILTER_H
#define SPIO_FILTER_H

#include "config.h"

#include "nonstd/expected.hpp"
#include "result.h"
#include "third_party/gsl.h"

SPIO_BEGIN_NAMESPACE

// TODO
// i mean look at dis

struct filter_base {
    using size_type = std::ptrdiff_t;
    using buffer_type = std::vector<gsl::byte>;

    virtual void clear(size_type n)
    {
        SPIO_UNUSED(n);
    }

    virtual ~filter_base() noexcept = default;
};

struct sink_filter : filter_base {
    using sink_type = std::function<result(buffer_type&, size_type)>;

    virtual result write(buffer_type& data, size_type n, sink_type& sink) = 0;
};
struct byte_sink_filter : filter_base {
    using sink_type = std::function<result(gsl::byte)>;

    virtual result put(gsl::byte data, sink_type& sink) = 0;
};

struct null_sink_filter : sink_filter {
    result write(buffer_type& data, size_type n, sink_type& sink) override
    {
        return sink(data, n);
    }
};
struct null_byte_sink_filter : byte_sink_filter {
    result put(gsl::byte data, sink_type& sink) override
    {
        return sink(data);
    }
};

struct source_filter : filter_base {
    using source_type = std::function<result(buffer_type&, size_type, bool&)>;

    virtual result read(buffer_type& data,
                        size_type n,
                        source_type& source,
                        bool& eof) = 0;
};
struct byte_source_filter : filter_base {
    using source_type = std::function<result(gsl::byte&)>;

    virtual result get(gsl::byte& data, source_type& source) = 0;
};

struct null_source_filter : source_filter {
    result read(buffer_type& data,
                size_type n,
                source_type& source,
                bool& eof) override
    {
        return source(data, n, eof);
    }
};
struct null_byte_source_fitler : byte_source_filter {
    result get(gsl::byte& data, source_type& source) override
    {
        return source(data);
    }
};

template <typename Base>
class basic_chain_base {
public:
    using filter_type = Base;
    using filter_ptr = std::unique_ptr<Base>;
    using filter_list = std::vector<filter_ptr>;
    using size_type = std::ptrdiff_t;
    using buffer_type = typename Base::buffer_type;

    basic_chain_base() = default;

    filter_list& filters() &
    {
        return m_list;
    }
    const filter_list& filters() const&
    {
        return m_list;
    }
    filter_list&& filters() &&
    {
        return std::move(m_list);
    }

    template <typename Filter, typename... Args>
    Filter& push(Args&&... a)
    {
        m_list.emplace_back(make_unique<Filter>(std::forward<Args>(a)...));
        return static_cast<Filter&>(*m_list.back());
    }
    template <typename Filter>
    Filter& push(Filter&& f)
    {
        m_list.emplace_back(make_unique<Filter>(std::forward<Filter>(f)));
        return static_cast<Filter&>(*m_list.back());
    }
    filter_ptr pop()
    {
        auto last = std::move(m_list.back());
        m_list.pop_back();
        return last;
    }

    size_type size() const
    {
        return m_list.size();
    }
    bool empty() const
    {
        return m_list.empty();
    }

private:
    filter_list m_list;
};

class sink_filter_chain : public basic_chain_base<sink_filter> {
public:
    using sink_type = sink_filter::sink_type;

    result write(buffer_type& buf, size_type n, sink_type& sink)
    {
        auto it = filters().begin();
        const auto end = filters().end();

        struct fn {
            filter_list::iterator& it;
            filter_list::const_iterator end;
            sink_type& sink;

            result operator()(buffer_type& b, size_type m)
            {
                if (it == end) {
                    return sink(b, m);
                }
                ++it;
                auto fn = sink_type(*this);
                return (*(it - 1))->write(b, m, fn);
            }
        };
        return fn{it, end, sink}(buf, n);
#if 0
        auto fn = sink_filter::sink_type([](buffer_type& b, size_type m) {
            SPIO_UNUSED(b);
            return m;
        });
        for (auto it = filters().begin(); it != filters().end(); ++it) {
            auto ret = (*it)->write(buf, n, fn);
            if (ret.has_error()) {
                auto end = it;
                for (it = filters().begin(); it != end; ++it) {
                    (*it)->clear(n);
                }
                return make_result(0, ret.error());
            }
            if (ret.value() != n) {
                auto end = it;
                for (it = filters().begin(); it != end; ++it) {
                    (*it)->clear(n - ret.value());
                }
                return ret;
            }
        }
        return n;
#endif
    }
};

class byte_sink_filter_chain : public basic_chain_base<byte_sink_filter> {
public:
    using sink_type = byte_sink_filter::sink_type;

    result put(gsl::byte b, sink_type& sink)
    {
        auto it = filters().begin();
        const auto end = filters().end();

        struct fn {
            filter_list::iterator& it;
            filter_list::const_iterator end;
            sink_type& sink;

            result operator()(gsl::byte b_)
            {
                if (it == end) {
                    return sink(b_);
                }
                ++it;
                auto fn = sink_type(*this);
                return (*(it - 1))->put(b_, fn);
            }
        };
        return fn{it, end, sink}(b);
    }
};

class source_filter_chain : public basic_chain_base<source_filter> {
public:
    using source_type = source_filter::source_type;

    result read(buffer_type& buf, size_type n, source_type& source)
    {
        struct filter_source {
            buffer_type& buffer;
            source_type& source;
            bool eof{false};

            result operator()(buffer_type& buf, size_type n, bool& e)
            {
                auto un = static_cast<size_t>(n);
                if (buf.size() < un) {
                    buf.reserve(un);
                }
                if (buffer.size() >= un) {
                    buf = buffer;
                    return n;
                }
                if (eof) {
                    std::copy_n(buffer.begin(), n, buf.begin());
                    return n;
                }
                auto sdiff = n - static_cast<size_type>(buffer.size());
                auto diff = static_cast<size_t>(sdiff);
                std::vector<gsl::byte> tmp(diff);
                auto r = source(tmp, sdiff, e);
                if (e) {
                    eof = true;
                }
                if (r.value() != sdiff) {
                    return make_result(
                        static_cast<size_type>(buffer.size()) + r.value(),
                        r.error());
                }
                std::copy(tmp.begin(), tmp.end(),
                          buf.begin() + static_cast<size_type>(buffer.size()));
                return n;
            }
        };

        bool eof = false;
        auto ret = source(buf, n, eof);
        if (ret.has_error()) {
            return ret;
        }
        n = ret.value();
        auto s = source_type(filter_source{buf, source});
        for (auto& f : filters()) {
            auto r = f->read(buf, n, s, eof);
            if (r.has_error() || r.value() < n) {
                return r;
            }
        }
        return n;
    }
};

#if 0
struct sink_filter {
    using size_type = std::ptrdiff_t;

    virtual ~sink_filter() = default;

    virtual nonstd::expected<size_type, failure> write_space(
        gsl::span<const gsl::byte> data) const
    {
        return data.size();
    }
    virtual result write(gsl::span<const gsl::byte> input,
                         gsl::span<gsl::byte> output) = 0;
};

struct null_sink_filter : sink_filter {
    using size_type = sink_filter::size_type;

    virtual result write(gsl::span<const gsl::byte> input,
                         gsl::span<gsl::byte> output) override
    {
        std::copy(input.begin(), input.end(), output.begin());
        return input.size();
    }
};

struct source_filter {
    using size_type = std::ptrdiff_t;

    virtual ~source_filter() = default;

    virtual nonstd::expected<size_type, failure> read_space(
        gsl::span<const gsl::byte> data)
    {
        return data.size();
    }
    virtual result read(gsl::span<const gsl::byte> input,
                        gsl::span<gsl::byte> output) = 0;
};

class sink_filter_chain {
public:
    using filter_ptr = std::unique_ptr<sink_filter>;
    using filter_list = std::vector<filter_ptr>;

    sink_filter_chain() = default;

    filter_list& filters() &
    {
        return m_filters;
    }
    const filter_list& filters() const&
    {
        return m_filters;
    }
    filter_list&& filters() &&
    {
        return std::move(m_filters);
    }

    template <typename BufferType>
    class basic_iterator {
        using list_iterator = filter_list::iterator;

        class proxy {
        public:
            proxy(sink_filter& f) : m_filter(f) {}
            proxy& operator=(BufferType& buf)
            {
                auto size = buf.size();
                auto space_required = m_filter.write_space(buf);
                if (!space_required) {
                    m_result =
                        make_result(0, std::move(space_required.error()));
                    return *this;
                }

                auto s = static_cast<size_t>(*space_required);
                if (s > buf.size()) {
                    buf.reserve(buf.size() + (s - buf.size()) * 2);
                }

                m_result = m_filter.write(
                    gsl::make_span(buf.data(),
                                   static_cast<std::ptrdiff_t>(size)),
                    buf);
                return *this;
            }

            result& get_result() &
            {
                return m_result;
            }
            const result& get_result() const&
            {
                return m_result;
            }
            result&& get_result() &&
            {
                return std::move(m_result);
            }

        private:
            result m_result{0};
            sink_filter& m_filter;
        };

    public:
        basic_iterator(list_iterator begin) : m_it(begin) {}

        proxy operator*()
        {
            return proxy(*(m_it->get()));
        }

        basic_iterator& operator++()
        {
            ++m_it;
            return *this;
        }
        basic_iterator operator++(int)
        {
            basic_iterator tmp(*this);
            operator++();
            return tmp;
        }

        bool operator==(const basic_iterator& it) const
        {
            return m_it == it.m_it;
        }
        bool operator!=(const basic_iterator& it) const
        {
            return !(operator==(it));
        }

    private:
        list_iterator m_it;
    };

    template <typename BufferType>
    class iterable_type {
    public:
        using iterator = basic_iterator<BufferType>;

        iterable_type(filter_list& l) : m_filters(l) {}

        iterator begin()
        {
            return iterator(m_filters.begin());
        }
        iterator end()
        {
            return iterator(m_filters.end());
        }

    private:
        filter_list& m_filters;
    };
    template <typename BufferType>
    iterable_type<BufferType> iterable()
    {
        return iterable_type<BufferType>(m_filters);
    }

private:
    filter_list m_filters;
};

class source_filter_chain {
public:
    using filter_ptr = std::unique_ptr<source_filter>;
    using filter_list = std::vector<filter_ptr>;

    source_filter_chain() = default;

    filter_list& filters() &
    {
        return m_filters;
    }
    const filter_list& filters() const&
    {
        return m_filters;
    }
    filter_list&& filters() &&
    {
        return std::move(m_filters);
    }

    template <typename BufferType>
    class basic_iterator {
        using list_iterator = filter_list::iterator;

        class proxy {
        public:
            proxy(source_filter& f) : m_filter(f) {}
            proxy& operator=(BufferType& buf)
            {
                auto size = buf.size();
                auto space_required = m_filter.write_space(buf);
                if (!space_required) {
                    m_result =
                        make_result(0, std::move(space_required.error()));
                    return *this;
                }

                auto s = static_cast<size_t>(*space_required);
                if (s > buf.size()) {
                    buf.reserve(buf.size() + (s - buf.size()) * 2);
                }

                m_result = m_filter.write(
                    gsl::make_span(buf.data(),
                                   static_cast<std::ptrdiff_t>(size)),
                    buf);
                return *this;
            }

            result& get_result() &
            {
                return m_result;
            }
            const result& get_result() const&
            {
                return m_result;
            }
            result&& get_result() &&
            {
                return std::move(m_result);
            }

        private:
            result m_result{0};
            source_filter& m_filter;
        };

    public:
        basic_iterator(list_iterator begin) : m_it(begin) {}

        proxy operator*()
        {
            return proxy(*(m_it->get()));
        }

        basic_iterator& operator++()
        {
            ++m_it;
            return *this;
        }
        basic_iterator operator++(int)
        {
            basic_iterator tmp(*this);
            operator++();
            return tmp;
        }

        bool operator==(const basic_iterator& it) const
        {
            return m_it == it.m_it;
        }
        bool operator!=(const basic_iterator& it) const
        {
            return !(operator==(it));
        }

    private:
        list_iterator m_it;
    };

    template <typename BufferType>
    class iterable_type {
    public:
        using iterator = basic_iterator<BufferType>;

        iterable_type(filter_list& l) : m_filters(l) {}

        iterator begin()
        {
            return iterator(m_filters.begin());
        }
        iterator end()
        {
            return iterator(m_filters.end());
        }

    private:
        filter_list& m_filters;
    };
    template <typename BufferType>
    iterable_type<BufferType> iterable()
    {
        return iterable_type<BufferType>(m_filters);
    }

private:
    filter_list m_filters;
};

#endif

SPIO_END_NAMESPACE

#endif  // SPIO_STREAM_BASE_H
