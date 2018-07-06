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

struct filter_base {
    using size_type = std::ptrdiff_t;
    using buffer_type = std::vector<gsl::byte>;

    filter_base(const filter_base&) = default;
    filter_base& operator=(const filter_base&) = default;
    filter_base(filter_base&&) noexcept = default;
    filter_base& operator=(filter_base&&) noexcept = default;

    virtual ~filter_base() noexcept = default;

protected:
    filter_base() = default;
};

struct output_filter : filter_base {
    virtual result write(buffer_type& data) = 0;
};
struct byte_output_filter : filter_base {
    using sink_type = std::function<result(gsl::byte)>;

    virtual result put(gsl::byte data) = 0;
};

struct null_output_filter : output_filter {
    result write(buffer_type& data) override
    {
        return static_cast<size_type>(data.size());
    }
};
struct null_byte_output_filter : byte_output_filter {
    result put(gsl::byte data) override
    {
        SPIO_UNUSED(data);
        return 1;
    }
};

struct source_filter : filter_base {
    virtual result read(gsl::span<gsl::byte> data, bool& eof) = 0;
};
struct input_filter : filter_base {
    virtual result read(buffer_type& data,
                        source_filter& source,
                        bool& eof) = 0;
};
struct byte_input_filter : filter_base {
    virtual result get(gsl::byte& data) = 0;
};

struct null_input_filter : input_filter {
    result read(buffer_type& data, source_filter& source, bool& eof) override
    {
        SPIO_UNUSED(source);
        SPIO_UNUSED(eof);
        return static_cast<size_type>(data.size());
    }
};
struct null_byte_input_filter : byte_input_filter {
    result get(gsl::byte& data) override
    {
        SPIO_UNUSED(data);
        return 1;
    }
};

template <typename Readable>
class readable_source_filter : public source_filter {
public:
    using readable_type = Readable;

    readable_source_filter(readable_type& r) : source_filter(), m_readable(r) {}

    result read(gsl::span<gsl::byte> data, bool& eof) override
    {
        return m_readable.read(data, eof);
    }

private:
    readable_type& m_readable;
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
        return static_cast<size_type>(m_list.size());
    }
    bool empty() const
    {
        return m_list.empty();
    }

private:
    filter_list m_list;
};

class sink_filter_chain : public basic_chain_base<output_filter> {
public:
    result write(buffer_type& buf)
    {
        for (auto& f : filters()) {
            auto r = f->write(buf);
            if (r.value() < static_cast<size_type>(buf.size()) ||
                r.has_error()) {
                return r;
            }
        }
        return static_cast<size_type>(buf.size());
    }
};
class byte_sink_filter_chain : public basic_chain_base<byte_output_filter> {
public:
    result put(gsl::byte b)
    {
        for (auto& f : filters()) {
            auto r = f->put(b);
            if (r.value() == 0 || r.has_error()) {
                return r;
            }
        }
        return 1;
    }
};

class source_filter_chain : public basic_chain_base<input_filter> {
public:
    result read(buffer_type& buf, source_filter& source, bool& eof)
    {
        for (auto& f : filters()) {
            auto r = f->read(buf, source, eof);
            if (r.value() < static_cast<size_type>(buf.size()) ||
                r.has_error()) {
                return r;
            }
        }
        return static_cast<size_type>(buf.size());
    }
};
class byte_source_filter_chain : public basic_chain_base<byte_input_filter> {
public:
    result get(gsl::byte& b)
    {
        for (auto& f : filters()) {
            auto r = f->get(b);
            if (r.value() == 0 || r.has_error()) {
                return r;
            }
        }
        return 1;
    }
};

SPIO_END_NAMESPACE

#endif  // SPIO_STREAM_BASE_H
