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

#ifndef SPIO_STDIO_DEVICE_H
#define SPIO_STDIO_DEVICE_H

#include "config.h"

#include <cstdio>
#include "device.h"
#include "error.h"
#include "third_party/expected.h"
#include "third_party/gsl.h"
#include "util.h"

SPIO_BEGIN_NAMESPACE

class stdio_handle_device {
public:
    SPIO_CONSTEXPR_STRICT stdio_handle_device() = default;
    SPIO_CONSTEXPR_STRICT stdio_handle_device(std::FILE* h) : m_handle(h) {}

    nonstd::expected<std::FILE*, failure> open(std::FILE* h)
    {
        Expects(h);
        m_handle = h;
        return h;
    }
    SPIO_CONSTEXPR_STRICT bool is_open() const SPIO_NOEXCEPT
    {
        return m_handle != nullptr;
    }

    SPIO_CONSTEXPR std::FILE* handle() SPIO_NOEXCEPT
    {
        return m_handle;
    }
    SPIO_CONSTEXPR_STRICT const std::FILE* handle() const SPIO_NOEXCEPT
    {
        return m_handle;
    }

    nonstd::expected<streamsize, failure> read(gsl::span<gsl::byte> s)
    {
        Expects(is_open());

        if (std::feof(m_handle) != 0) {
            return nonstd::make_unexpected(make_error_code(end_of_file));
        }

        auto b = std::fread(s.data(), 1, s.size_bytes(), m_handle);
        if (b < s.size_bytes()) {
            if (std::ferror(m_handle) != 0) {
                return nonstd::make_unexpected(SPIO_MAKE_ERRNO);
            }
            if (std::feof(m_handle) != 0) {
                return -1;
            }
        }
        return b;
    }

    bool putback(gsl::byte b)
    {
        Expects(is_open());

        return std::ungetc(gsl::to_uchar(b), m_handle) != EOF;
    }

    nonstd::expected<streamsize, failure> write(gsl::span<const gsl::byte> s)
    {
        Expects(is_open());

        auto b = std::fwrite(s.data(), 1, s.size_bytes(), m_handle);
        if (b < s.size_bytes()) {
            if (std::ferror(m_handle) != 0) {
                return nonstd::make_unexpected(SPIO_MAKE_ERRNO);
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
        return seek(pos, seekdir::beg);
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
        if (std::fseek(m_handle, off, origin) != 0) {
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

static_assert(is_sink<stdio_handle_device>::value, "");
static_assert(is_source<stdio_handle_device>::value, "");

class stdio_device : public stdio_handle_device {
public:
    stdio_device() = default;

    stdio_device(const stdio_device&) = delete;
    stdio_device& operator=(const stdio_device&) = delete;
    stdio_device(stdio_device&&) SPIO_NOEXCEPT = default;
    stdio_device& operator=(stdio_device&&) SPIO_NOEXCEPT = default;
    ~stdio_device() SPIO_NOEXCEPT
    {
        if (is_open()) {
            close();
        }
    }

    nonstd::expected<std::FILE*, failure> open(const char* path,
                                               const char* flags)
    {
        Expects(!is_open());

        auto h = std::fopen(path, flags);
        if (!h) {
            return nonstd::make_unexpected(SPIO_MAKE_ERRNO);
        }
        if (!std::strchr(flags, 'r')) {
            if (std::setvbuf(h, nullptr, _IONBF, 0) != 0) {
                return nonstd::make_unexpected(SPIO_MAKE_ERRNO);
            }
        }
        m_handle = h;
        return m_handle;
    }

    void close()
    {
        Expects(is_open());
        std::fclose(m_handle);
        m_handle = nullptr;
    }
};

class stdio_source : private stdio_device {
public:
    using stdio_device::stdio_device;

    using stdio_device::close;
    using stdio_device::is_open;
    using stdio_device::open;
    using stdio_device::putback;
    using stdio_device::read;
    using stdio_device::seek;
};

static_assert(is_source<stdio_source>::value, "");

class stdio_sink : private stdio_device {
public:
    using stdio_device::stdio_device;

    using stdio_device::close;
    using stdio_device::is_open;
    using stdio_device::open;
    using stdio_device::seek;
    using stdio_device::sync;
    using stdio_device::write;
};

static_assert(is_sink<stdio_sink>::value, "");

SPIO_END_NAMESPACE

#endif  // SPIO_STDIO_DEVICE_H
