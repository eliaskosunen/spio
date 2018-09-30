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

#ifndef SPIO_FILTER_H
#define SPIO_FILTER_H

#include "config.h"

#include "nonstd/expected.hpp"
#include "result.h"
#include "third_party/gsl.h"

namespace spio {
SPIO_BEGIN_NAMESPACE

struct filter_base {
    using size_type = std::ptrdiff_t;

    filter_base(const filter_base&) = default;
    filter_base& operator=(const filter_base&) = default;
    filter_base(filter_base&&) noexcept = default;
    filter_base& operator=(filter_base&&) noexcept = default;

    virtual ~filter_base() noexcept = default;

protected:
    filter_base() = default;
};

struct output_filter : filter_base {
    using buffer_type = std::vector<byte>;

    virtual result write(buffer_type& data) = 0;
};
struct byte_output_filter : filter_base {
    using buffer_type = void;

    virtual result put(byte data) = 0;
};

struct null_output_filter : output_filter {
    result write(buffer_type& data) override
    {
        return static_cast<size_type>(data.size());
    }
};
struct null_byte_output_filter : byte_output_filter {
    result put(byte data) override
    {
        SPIO_UNUSED(data);
        return 1;
    }
};

struct input_filter : filter_base {
    using buffer_type = span<byte>;

    virtual result read(buffer_type& data) = 0;
};
struct byte_input_filter : filter_base {
    using buffer_type = void;

    virtual result get(byte& data) = 0;
};

struct null_input_filter : input_filter {
    result read(buffer_type& data) override
    {
        return data.size();
    }
};
struct null_byte_input_filter : byte_input_filter {
    result get(byte& data) override
    {
        SPIO_UNUSED(data);
        return 1;
    }
};

template <typename Base>
class basic_chain {
public:
    using filter_type = Base;
    using filter_ptr = std::unique_ptr<Base>;
    using filter_list = std::vector<filter_ptr>;
    using size_type = std::ptrdiff_t;
    using buffer_type = typename Base::buffer_type;

    basic_chain() = default;

    SPIO_CONSTEXPR14 filter_list& filters() & noexcept
    {
        return m_list;
    }
    SPIO_CONSTEXPR const filter_list& filters() const& noexcept
    {
        return m_list;
    }
    SPIO_CONSTEXPR14 filter_list&& filters() && noexcept
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

    size_type size() const noexcept
    {
        return static_cast<size_type>(m_list.size());
    }
    bool empty() const noexcept
    {
        return m_list.empty();
    }

private:
    filter_list m_list;
};

class sink_filter_chain : public virtual basic_chain<output_filter> {
    using base = basic_chain<output_filter>;

    struct static_filter_callback {
        template <typename T>
        result operator()(T& a)
        {
            if (r.value() >=
                    static_cast<typename base::size_type>(buf.size()) &&
                !r.has_error()) {
                r = a->write(buf);
            }
            return r;
        }

        typename base::buffer_type& buf;
        result& r;
    };

public:
    result write(typename base::buffer_type& buf)
    {
        for (auto& f : base::filters()) {
            auto r = f->write(buf);
            if (r.value() < static_cast<typename base::size_type>(buf.size()) ||
                r.has_error()) {
                return r;
            }
        }
        return static_cast<typename base::size_type>(buf.size());
    }

    typename base::size_type output_size() const noexcept
    {
        return base::size();
    }
    bool output_empty() const noexcept
    {
        return base::empty();
    }
};
class byte_sink_filter_chain : public virtual basic_chain<byte_output_filter> {
    using base = basic_chain<byte_output_filter>;

    struct static_filter_callback {
        template <typename T>
        result operator()(T& a)
        {
            if (r.value() != 0 && !r.has_error()) {
                r = a->put(b);
            }
            return r;
        }

        byte b;
        result& r;
    };

public:
    result put(byte b)
    {
        for (auto& f : base::filters()) {
            auto r = f->put(b);
            if (r.value() == 0 || r.has_error()) {
                return r;
            }
        }
        return 1;
    }

    typename base::size_type output_size() const noexcept
    {
        return base::size();
    }
    bool output_empty() const noexcept
    {
        return base::empty();
    }
};

class source_filter_chain : public virtual basic_chain<input_filter> {
    using base = basic_chain<input_filter>;

    struct static_filter_callback {
        template <typename T>
        result operator()(T& a)
        {
            if (r.value() >=
                    static_cast<typename base::size_type>(buf.size()) &&
                !r.has_error()) {
                r = a->read(buf);
            }
            return r;
        }

        typename base::buffer_type& buf;
        result& r;
    };

public:
    result read(typename base::buffer_type buf)
    {
        for (auto& f : base::filters()) {
            auto r = f->read(buf);
            if (r.value() < static_cast<typename base::size_type>(buf.size()) ||
                r.has_error()) {
                return r;
            }
        }
        return static_cast<typename base::size_type>(buf.size());
    }

    typename base::size_type input_size() const noexcept
    {
        return base::size();
    }
    bool input_empty() const noexcept
    {
        return base::empty();
    }
};
class byte_source_filter_chain : public virtual basic_chain<byte_input_filter> {
    using base = basic_chain<byte_input_filter>;

    struct static_filter_callback {
        template <typename T>
        result operator()(T& a)
        {
            if (r.value() != 0 && !r.has_error()) {
                r = a->get(b);
            }
            return r;
        }

        byte& b;
        result& r;
    };

public:
    result get(byte& b)
    {
        for (auto& f : base::filters()) {
            auto r = f->get(b);
            if (r.value() == 0 || r.has_error()) {
                return r;
            }
        }
        return 1;
    }

    typename base::size_type input_size() const noexcept
    {
        return base::size();
    }
    bool input_empty() const noexcept
    {
        return base::empty();
    }
};

SPIO_END_NAMESPACE
}  // namespace spio

#endif  // SPIO_STREAM_BASE_H
