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

SPIO_END_NAMESPACE

#endif  // SPIO_STREAM_BASE_H
