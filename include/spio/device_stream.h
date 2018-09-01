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

#ifndef SPIO_DEVICE_STREAM_H
#define SPIO_DEVICE_STREAM_H

#include "config.h"

#include "memory_device.h"
#include "stdio_device.h"
#include "stream.h"

namespace spio {
SPIO_BEGIN_NAMESPACE

template <typename T, typename Return = void>
class auto_delete {
public:
    using type = T;
    using pointer = typename std::add_pointer<T>::type;
    using deleter_type = std::function<Return(pointer)>;

    SPIO_CONSTEXPR auto_delete() = default;
    SPIO_CONSTEXPR14 auto_delete(pointer ptr, const deleter_type& d) noexcept
        : m_ptr(ptr), m_deleter(d)
    {
    }
    SPIO_CONSTEXPR14 auto_delete(pointer ptr, deleter_type&& d) noexcept
        : m_ptr(ptr), m_deleter(std::move(d))
    {
    }

    auto_delete(const auto_delete&) = delete;
    auto_delete& operator=(const auto_delete&) = delete;
    auto_delete(auto_delete&&) noexcept = default;
    auto_delete& operator=(auto_delete&&) noexcept = default;

    ~auto_delete()
    {
        Expects(m_ptr);
        Expects(m_deleter);
        m_deleter(m_ptr);
    }

    void reset(const deleter_type& d)
    {
        if (m_ptr && m_deleter) {
            m_deleter(m_ptr);
        }
        m_deleter = d;
    }
    void reset(deleter_type&& d)
    {
        if (m_ptr && m_deleter) {
            m_deleter(m_ptr);
        }
        m_deleter = std::move(d);
    }

    SPIO_CONSTEXPR14 deleter_type& get_deleter() & noexcept
    {
        return m_deleter;
    }
    SPIO_CONSTEXPR const deleter_type& get_deleter() const& noexcept
    {
        return m_deleter;
    }
    SPIO_CONSTEXPR14 deleter_type&& get_deleter() && noexcept
    {
        return m_deleter;
    }

    SPIO_CONSTEXPR14 pointer& get_pointer() & noexcept
    {
        return m_ptr;
    }
    SPIO_CONSTEXPR pointer get_pointer() const& noexcept
    {
        return m_ptr;
    }
    SPIO_CONSTEXPR14 pointer get_pointer() && noexcept
    {
        return m_ptr;
    }

private:
    pointer m_ptr;
    deleter_type m_deleter;
};

template <typename... StaticFilters>
struct stdio_iostream_chain
    : public virtual sink_filter_chain<StaticFilters...>,
      public virtual byte_source_filter_chain<StaticFilters...> {
};

template <typename Char>
class basic_stdio_handle_outstream
    : public stream<stdio_sink, Char, sink_filter_chain> {
    using base = stream<stdio_sink, Char, sink_filter_chain>;

public:
    basic_stdio_handle_outstream(std::FILE* f, buffer_mode bufmode)
        : base(stdio_sink(f), typename base::chain_type{})
    {
        base::output_base::sink_storage() =
            typename base::output_base::sink_type(base::device(), bufmode);
    }
};
template <typename Char>
class basic_stdio_handle_instream
    : public stream<stdio_source, Char, byte_source_filter_chain> {
    using base = stream<stdio_source, Char, byte_source_filter_chain>;

public:
    basic_stdio_handle_instream(std::FILE* f)
        : base(stdio_source(f), typename base::chain_type{})
    {
    }
};
template <typename Char>
class basic_stdio_handle_iostream
    : public stream<stdio_device, Char, stdio_iostream_chain> {
    using base = stream<stdio_device, Char, stdio_iostream_chain>;

public:
    basic_stdio_handle_iostream(std::FILE* f, buffer_mode bufmode)
        : base(stdio_device(f), typename base::chain_type{})
    {
        base::output_base::sink_storage() =
            typename base::output_base::sink_type(base::device(), bufmode);
    }
};

template <typename Char>
class basic_stdio_outstream : public basic_stdio_handle_outstream<Char> {
    using base = basic_stdio_handle_outstream<Char>;
    using deleter_return = nonstd::expected<void, failure>;
    using deleter = auto_delete<std::FILE, deleter_return>;

public:
    basic_stdio_outstream() : base(nullptr, buffer_mode::none) {}

    nonstd::expected<std::FILE*, failure> open(const char* n,
                                               const char* f,
                                               buffer_mode b)
    {
        auto file = std::fopen(n, f);
        if (!file) {
            return nonstd::make_unexpected(SPIO_MAKE_ERRNO);
        }
        base::device() = stdio_sink(file);
        base::output_base::sink_storage() =
            typename base::output_base::sink_type(base::device(), b);
        m_del = deleter(std::addressof(base::device().handle()),
                        [](std::FILE* h) -> deleter_return {
                            Expects(h);
                            if (std::fclose(h) != 0) {
                                return nonstd::make_unexpected(SPIO_MAKE_ERRNO);
                            }
                            return {};
                        });
        return base::device().handle();
    }

    nonstd::expected<void, failure> close() override
    {
        Expects(base::is_open());
        return std::move(m_del)(base::device());
    }

private:
    deleter m_del;
};
template <typename Char>
class basic_stdio_instream : public basic_stdio_handle_instream<Char> {
    using base = basic_stdio_handle_instream<Char>;
    using deleter_return = nonstd::expected<void, failure>;
    using deleter = auto_delete<std::FILE, deleter_return>;

public:
    basic_stdio_instream() : base(nullptr) {}

    nonstd::expected<std::FILE*, failure> open(const char* n, const char* f)
    {
        auto file = std::fopen(n, f);
        if (!file) {
            return nonstd::make_unexpected(SPIO_MAKE_ERRNO);
        }
        base::device() = stdio_sink(file);
        m_del = deleter(std::addressof(base::device().handle()),
                        [](std::FILE* h) -> deleter_return {
                            Expects(h);
                            if (std::fclose(h) != 0) {
                                return nonstd::make_unexpected(SPIO_MAKE_ERRNO);
                            }
                            return {};
                        });
        return base::device().handle();
    }

    nonstd::expected<void, failure> close() override
    {
        Expects(base::is_open());
        return std::move(m_del)(base::device());
    }

private:
    deleter m_del;
};
template <typename Char>
class basic_stdio_iostream : public basic_stdio_handle_iostream<Char> {
    using base = basic_stdio_handle_iostream<Char>;
    using deleter_return = nonstd::expected<void, failure>;
    using deleter = auto_delete<std::FILE, deleter_return>;

public:
    basic_stdio_iostream() : base(nullptr, buffer_mode::none) {}

    nonstd::expected<std::FILE*, failure> open(const char* n,
                                               const char* f,
                                               buffer_mode b)
    {
        auto file = std::fopen(n, f);
        if (!file) {
            return nonstd::make_unexpected(SPIO_MAKE_ERRNO);
        }
        base::device() = stdio_sink(file);
        base::output_base::sink_storage() =
            typename base::output_base::sink_type(base::device(), b);
        m_del = deleter(std::addressof(base::device().handle()),
                        [](std::FILE* h) -> deleter_return {
                            Expects(h);
                            if (std::fclose(h) != 0) {
                                return nonstd::make_unexpected(SPIO_MAKE_ERRNO);
                            }
                            return {};
                        });
        return base::device().handle();
    }

    nonstd::expected<void, failure> close() override
    {
        Expects(base::is_open());
        return std::move(m_del)(base::device());
    }

private:
    deleter m_del;
};

using stdio_handle_instream = basic_stdio_handle_instream<character<char>>;
using stdio_handle_outstream = basic_stdio_handle_outstream<character<char>>;
using stdio_handle_iostream = basic_stdio_handle_iostream<character<char>>;

using stdio_instream = basic_stdio_instream<character<char>>;
using stdio_outstream = basic_stdio_outstream<character<char>>;
using stdio_iostream = basic_stdio_iostream<character<char>>;

template <typename... StaticFilters>
struct memory_iostream_chain
    : public virtual sink_filter_chain<StaticFilters...>,
      public virtual source_filter_chain<StaticFilters...> {
};

template <typename Char>
class basic_memory_outstream
    : public stream<memory_sink, Char, sink_filter_chain> {
    using base = stream<memory_sink, Char, sink_filter_chain>;

public:
    basic_memory_outstream() : base(memory_sink(), typename base::chain_type{})
    {
    }
    basic_memory_outstream(memory_sink::span_type s)
        : base(memory_sink(s), typename base::chain_type{})
    {
    }

    memory_sink& open(memory_sink::span_type s)
    {
        base::device() = memory_sink(s);
        return base::device();
    }
};
template <typename Char>
class basic_memory_instream
    : public stream<memory_source, Char, source_filter_chain> {
    using base = stream<memory_source, Char, source_filter_chain>;

public:
    basic_memory_instream() : base(memory_source(), typename base::chain_type{})
    {
    }
    basic_memory_instream(memory_source::span_type s)
        : base(memory_source(s), typename base::chain_type{})
    {
    }

    memory_source& open(memory_source::span_type s)
    {
        base::device() = memory_source(s);
        return base::device();
    }
};
template <typename Char>
class basic_memory_iostream
    : public stream<memory_device, Char, memory_iostream_chain> {
    using base = stream<memory_device, Char, memory_iostream_chain>;

public:
    basic_memory_iostream() : base(memory_device(), typename base::chain_type{})
    {
    }
    basic_memory_iostream(memory_sink::span_type s)
        : base(memory_source(s), typename base::chain_type{})
    {
    }

    memory_source& open(memory_device::span_type s)
    {
        base::device() = memory_device(s);
        return base::device();
    }
};

using memory_outstream = basic_memory_outstream<character<char>>;
using memory_instream = basic_memory_instream<character<char>>;
using memory_iostream = basic_memory_iostream<character<char>>;

SPIO_END_NAMESPACE
}  // namespace spio

#endif  // SPIO_DEVICE_STREAM_H
