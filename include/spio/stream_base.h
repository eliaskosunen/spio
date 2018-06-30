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

#ifndef SPIO_STREAM_BASE_H
#define SPIO_STREAM_BASE_H

#include "config.h"

#include <vector>
#include "device.h"

SPIO_BEGIN_NAMESPACE

class stream_base {
public:
    stream_base(const stream_base&) = delete;
    stream_base& operator=(const stream_base&) = delete;
    stream_base(stream_base&&) = default;
    stream_base& operator=(stream_base&&) = default;

    virtual ~stream_base() = default;

    bool bad() const noexcept
    {
        return m_flags[0];
    }
    virtual operator bool() const noexcept
    {
        return bad();
    }
    bool operator!() const noexcept
    {
        return !(operator bool());
    }

protected:
    stream_base() = default;

    void _set_bad()
    {
        m_flags[0] = true;
    }
    void _clear_bad()
    {
        m_flags[0] = false;
    }

private:
    static std::vector<bool> _flags_init()
    {
        return {false};
    }

    std::vector<bool> m_flags{_flags_init()};  // Intentional vector<bool>
};

SPIO_END_NAMESPACE

#endif  // SPIO_STREAM_BASE_H
