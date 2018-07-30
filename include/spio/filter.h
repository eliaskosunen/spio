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
    using buffer_type = std::vector<gsl::byte>;

    virtual result write(buffer_type& data) = 0;
};
struct byte_output_filter : filter_base {
    using buffer_type = void;

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

struct input_filter : filter_base {
    using buffer_type = gsl::span<gsl::byte>;

    virtual result read(buffer_type& data) = 0;
};
struct byte_input_filter : filter_base {
    using buffer_type = void;

    virtual result get(gsl::byte& data) = 0;
};

struct null_input_filter : input_filter {
    result read(buffer_type& data) override
    {
        return data.size();
    }
};
struct null_byte_input_filter : byte_input_filter {
    result get(gsl::byte& data) override
    {
        SPIO_UNUSED(data);
        return 1;
    }
};

namespace detail {
    template <int... Is>
    struct seq {
    };

    template <int N, int... Is>
    struct gen_seq : gen_seq<N - 1, N - 1, Is...> {
    };
    template <int... Is>
    struct gen_seq<0, Is...> : seq<Is...> {
    };

    template <typename T, typename F, int... Is>
    auto for_each(T& t, F f, seq<Is...>)
        -> std::initializer_list<typename std::result_of<F(T&)>::type>
    {
        SPIO_UNUSED(f);
        return {f(std::get<Is>(t))...};
    }
}  // namespace detail

template <typename... Filters>
class basic_static_chain {
public:
    using filter_list = std::tuple<Filters...>;
    using size_type = std::ptrdiff_t;

    basic_static_chain() = default;
    basic_static_chain(filter_list l) : m_list(std::move(l)) {}

    template <typename... A>
    basic_static_chain(A&&... a) : m_list(std::forward<A>(a)...)
    {
    }

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

    SPIO_CONSTEXPR size_type size() const noexcept
    {
        return std::tuple_size<filter_list>::value;
    }
    SPIO_CONSTEXPR bool empty() const noexcept
    {
        return size() != 0;
    }

    template <typename F>
    auto for_each(F&& f) noexcept(
        noexcept(std::declval<F>()(std::declval<filter_list&>())))
        -> std::initializer_list<typename std::result_of<F(filter_list&)>::type>
    {
        return detail::for_each(
            m_list, std::forward<F>(f),
            detail::gen_seq<std::tuple_size<filter_list>::value>());
    }

private:
    filter_list m_list;
};

template <typename Base>
class basic_dynamic_chain {
public:
    using filter_type = Base;
    using filter_ptr = std::unique_ptr<Base>;
    using filter_list = std::vector<filter_ptr>;
    using size_type = std::ptrdiff_t;

    basic_dynamic_chain() = default;

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

template <typename... StaticFilters>
class chain_base {
public:
    using static_type = basic_static_chain<StaticFilters...>;
    using size_type = std::ptrdiff_t;

    chain_base() = default;
    chain_base(static_type s) noexcept : m_static(std::move(s)) {}

    SPIO_CONSTEXPR14 static_type& get_static() & noexcept
    {
        return m_static;
    }
    SPIO_CONSTEXPR const static_type& get_static() const& noexcept
    {
        return m_static;
    }
    SPIO_CONSTEXPR14 static_type&& get_static() && noexcept
    {
        return std::move(m_static);
    }

private:
    static_type m_static;
};

template <typename Base, typename... StaticFilters>
class basic_chain_base : public chain_base<StaticFilters...> {
public:
    using static_type = basic_static_chain<StaticFilters...>;
    using dynamic_type = basic_dynamic_chain<Base>;
    using size_type = std::ptrdiff_t;
    using buffer_type = typename Base::buffer_type;

    basic_chain_base() = default;
    basic_chain_base(static_type s, dynamic_type d) noexcept
        : chain_base<StaticFilters...>(std::move(s)), m_dynamic(std::move(d))
    {
    }

    SPIO_CONSTEXPR14 dynamic_type& get_dynamic() & noexcept
    {
        return m_dynamic;
    }
    SPIO_CONSTEXPR const dynamic_type& get_dynamic() const& noexcept
    {
        return m_dynamic;
    }
    SPIO_CONSTEXPR14 dynamic_type&& get_dynamic() && noexcept
    {
        return std::move(m_dynamic);
    }

    size_type size() const noexcept
    {
        return chain_base<StaticFilters...>::get_static().size() +
               m_dynamic.size();
    }
    bool empty() const noexcept
    {
        return chain_base<StaticFilters...>::get_static().empty() &&
               m_dynamic.empty();
    }

private:
    dynamic_type m_dynamic;
};

template <typename... StaticFilters>
class sink_filter_chain
    : public virtual basic_chain_base<output_filter, StaticFilters...> {
    using base = basic_chain_base<output_filter, StaticFilters...>;

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
        if (base::get_static().size() > 0) {
            auto res =
                result{static_cast<typename base::size_type>(buf.size())};
            auto l =
                base::get_static().for_each(static_filter_callback{buf, res});
            Ensures(static_cast<typename base::size_type>(l.size()) ==
                    base::get_static().size());
            for (auto& r : l) {
                if (r.value() <
                        static_cast<typename base::size_type>(buf.size()) ||
                    r.has_error()) {
                    return r;
                }
            }
        }
        for (auto& f : base::get_dynamic().filters()) {
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
template <typename... StaticFilters>
class byte_sink_filter_chain
    : public virtual basic_chain_base<byte_output_filter, StaticFilters...> {
    using base = basic_chain_base<byte_output_filter, StaticFilters...>;

    struct static_filter_callback {
        template <typename T>
        result operator()(T& a)
        {
            if (r.value() != 0 && !r.has_error()) {
                r = a->put(b);
            }
            return r;
        }

        gsl::byte b;
        result& r;
    };

public:
    result put(gsl::byte b)
    {
        if (base::get_static().size() > 0) {
            auto res = result{1};
            auto l =
                base::get_static().for_each(static_filter_callback{b, res});
            Ensures(static_cast<typename base::size_type>(l.size()) ==
                    base::get_static().size());
            for (auto& r : l) {
                if (r.value() != 1 || r.has_error()) {
                    return r;
                }
            }
        }
        for (auto& f : base::get_dynamic().filters()) {
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

template <typename... StaticFilters>
class source_filter_chain
    : public virtual basic_chain_base<input_filter, StaticFilters...> {
    using base = basic_chain_base<input_filter, StaticFilters...>;

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
        if (base::get_static().size() > 0) {
            auto res =
                result{static_cast<typename base::size_type>(buf.size())};
            auto l =
                base::get_static().for_each(static_filter_callback{buf, res});
            Ensures(static_cast<typename base::size_type>(l.size()) ==
                    base::get_static().size());
            for (auto& r : l) {
                if (r.value() <=
                        static_cast<typename base::size_type>(buf.size()) ||
                    r.has_error()) {
                    return r;
                }
            }
        }
        for (auto& f : base::get_dynamic().filters()) {
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
template <typename... StaticFilters>
class byte_source_filter_chain
    : public virtual basic_chain_base<byte_input_filter, StaticFilters...> {
    using base = basic_chain_base<byte_input_filter, StaticFilters...>;

    struct static_filter_callback {
        template <typename T>
        result operator()(T& a)
        {
            if (r.value() != 0 && !r.has_error()) {
                r = a->get(b);
            }
            return r;
        }

        gsl::byte& b;
        result& r;
    };

public:
    result get(gsl::byte& b)
    {
        if (base::get_static().size() > 0) {
            auto res = result{1};
            auto l =
                base::get_static().for_each(static_filter_callback{b, res});
            Ensures(static_cast<typename base::size_type>(l.size()) ==
                    base::get_static().size());
            for (auto& r : l) {
                if (r.value() != 1 || r.has_error()) {
                    return r;
                }
            }
        }
        for (auto& f : base::get_dynamic().filters()) {
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
