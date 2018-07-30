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

#ifndef SPIO_STDIO_DEVICE_H
#define SPIO_STDIO_DEVICE_H

#include "config.h"

#include <cstdio>
#include "device.h"
#include "error.h"
#include "result.h"
#include "third_party/expected.h"
#include "third_party/gsl.h"
#include "util.h"

namespace spio {
SPIO_BEGIN_NAMESPACE

class stdio_device {
public:
    SPIO_CONSTEXPR stdio_device() = default;
    SPIO_CONSTEXPR stdio_device(std::FILE* h) : m_handle(h) {}

    SPIO_CONSTEXPR bool is_open() const noexcept
    {
        return m_handle != nullptr;
    }
    nonstd::expected<void, failure> close() noexcept
    {
        Expects(is_open());
        m_handle = nullptr;
        return {};
    }

    SPIO_CONSTEXPR14 std::FILE* handle() noexcept
    {
        return m_handle;
    }
    SPIO_CONSTEXPR const std::FILE* handle() const noexcept
    {
        return m_handle;
    }

    result get(gsl::byte& r, bool& eof)
    {
        Expects(is_open());

        if (std::feof(m_handle) != 0) {
            return make_result(0, end_of_file);
        }

        auto n = std::fread(std::addressof(r), 1, 1, m_handle);
        if (SPIO_UNLIKELY(n == 0)) {
            if (SPIO_UNLIKELY(std::ferror(m_handle) != 0)) {
                return make_result(0, SPIO_MAKE_ERRNO);
            }
        }
        if (std::feof(m_handle) != 0) {
            eof = true;
        }
        return static_cast<std::ptrdiff_t>(n);
    }

    bool putback(gsl::byte b)
    {
        Expects(is_open());

        return std::ungetc(gsl::to_uchar(b), m_handle) != EOF;
    }

    result write(gsl::span<const gsl::byte> s)
    {
        Expects(is_open());

        auto b = static_cast<std::ptrdiff_t>(std::fwrite(
            s.data(), 1, static_cast<std::size_t>(s.size_bytes()), m_handle));
        if (SPIO_UNLIKELY(b < s.size_bytes())) {
            if (SPIO_UNLIKELY(std::ferror(m_handle) != 0)) {
                return make_result(b, SPIO_MAKE_ERRNO);
            }
        }
        return b;
    }

    nonstd::expected<void, failure> sync()
    {
        Expects(is_open());

        if (std::fflush(m_handle) != 0) {
            return nonstd::make_unexpected(SPIO_MAKE_ERRNO);
        }
        return {};
    }

    nonstd::expected<streampos, failure> seek(streampos pos,
                                              inout which = in | out)
    {
        SPIO_UNUSED(which);
        return seek(streamoff(pos), seekdir::beg);
    }
    nonstd::expected<streampos, failure> seek(streamoff off,
                                              seekdir dir,
                                              inout which = in | out)
    {
        SPIO_UNUSED(which);
        Expects(is_open());

        const auto origin = [&]() {
            if (dir == seekdir::beg) {
                return SEEK_SET;
            }
            if (dir == seekdir::cur) {
                return SEEK_CUR;
            }
            return SEEK_END;
        }();
        if (std::fseek(m_handle, static_cast<long>(off), origin) != 0) {
            return nonstd::make_unexpected(SPIO_MAKE_ERRNO);
        }

        auto p = std::ftell(m_handle);
        if (p == -1) {
            return nonstd::make_unexpected(SPIO_MAKE_ERRNO);
        }
        return p;
    }

protected:
    std::FILE* m_handle{nullptr};
};

static_assert(is_sink<stdio_device>::value, "");
static_assert(is_source<stdio_device>::value, "");

class stdio_source : private stdio_device {
public:
    using stdio_device::stdio_device;

    using stdio_device::close;
    using stdio_device::get;
    using stdio_device::is_open;
    using stdio_device::putback;
    using stdio_device::seek;
};

static_assert(is_source<stdio_source>::value, "");

class stdio_sink : private stdio_device {
public:
    using stdio_device::stdio_device;

    using stdio_device::close;
    using stdio_device::is_open;
    using stdio_device::seek;
    using stdio_device::sync;
    using stdio_device::write;
};

static_assert(is_sink<stdio_sink>::value, "");

SPIO_END_NAMESPACE
}  // namespace spio

#endif  // SPIO_STDIO_DEVICE_H
