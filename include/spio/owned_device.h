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

#ifndef SPIO_OWNED_DEVICE_H
#define SPIO_OWNED_DEVICE_H

#include "config.h"

#include "device.h"
#include "error.h"
#include "third_party/expected.h"

SPIO_BEGIN_NAMESPACE

template <typename Device>
class basic_owned_device {
public:
    using device_type = Device;

    basic_owned_device() = default;
    template <typename... Args>
    basic_owned_device(Args&&... a) : m_dev(std::forward<Args>(a)...)
    {
    }

    basic_owned_device(const basic_owned_device&) = delete;
    basic_owned_device& operator=(const basic_owned_device&) = delete;
    basic_owned_device(basic_owned_device&&) noexcept = default;
    basic_owned_device& operator=(basic_owned_device&&) noexcept = default;
    ~basic_owned_device()
    {
        if (is_open()) {
            close();
        }
    }

    template <typename... Args>
    void open(Args&&... a)
    {
        m_dev = device_type(std::forward<Args>(a)...);
    }
    bool is_open() const
    {
        return m_dev.is_open();
    }
    void close()
    {
        Expects(is_open());
        m_dev.close();
    }

    device_type& get() &
    {
        return m_dev;
    }
    const device_type& get() const&
    {
        return m_dev;
    }
    device_type&& get() &&
    {
        return std::move(m_dev);
    }

    device_type& operator*() &
    {
        return get();
    }
    const device_type& operator*() const&
    {
        return get();
    }
    device_type&& operator*() &&
    {
        return get();
    }

    device_type* operator->()
    {
        return std::addressof(get());
    }
    const device_type* operator->() const
    {
        return std::addressof(get());
    }

private:
    device_type m_dev{};
};

SPIO_END_NAMESPACE

#endif  // SPIO_OWNED_DEVICE_H
