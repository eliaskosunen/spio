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

#include "stdio_device.h"
#include "stream.h"

SPIO_BEGIN_NAMESPACE

template <typename T, typename Return = void>
class auto_delete {
public:
    using type = T;
    using pointer = typename std::add_pointer<T>::type;
    using deleter_type = std::function<Return(pointer)>;

    auto_delete() = default;
    auto_delete(pointer ptr, const deleter_type& d) : m_ptr(ptr), m_deleter(d)
    {
    }
    auto_delete(pointer ptr, deleter_type&& d)
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
        m_deleter.reset(d);
    }
    void reset(deleter_type&& d)
    {
        if (m_ptr && m_deleter) {
            m_deleter(m_ptr);
        }
        m_deleter.reset(d);
    }

    deleter_type& get_deleter() &
    {
        return m_deleter;
    }
    const deleter_type& get_deleter() const&
    {
        return m_deleter;
    }
    deleter_type&& get_deleter() &&
    {
        return m_deleter;
    }

    pointer& get_pointer() &
    {
        return m_ptr;
    }
    pointer get_pointer() const&
    {
        return m_ptr;
    }
    pointer get_pointer() &&
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

SPIO_END_NAMESPACE

#endif  // SPIO_DEVICE_STREAM_H
