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

#ifndef SPIO_SINK_H
#define SPIO_SINK_H

#include "config.h"

#include "device.h"
#include "error.h"
#include "result.h"

#ifndef SPIO_WRITE_ALL_MAX_ATTEMPTS
#define SPIO_WRITE_ALL_MAX_ATTEMPTS 8
#endif

SPIO_BEGIN_NAMESPACE

template <typename Device>
result write_all(Device& d, gsl::span<const gsl::byte> s)
{
    auto total_written = 0;
    for (auto i = 0; i < SPIO_WRITE_ALL_MAX_ATTEMPTS; ++i) {
        auto ret = d.write(s);
        total_written += ret.value();
        if (SPIO_UNLIKELY(ret.has_error())) {
            return make_result(total_written, ret.error());
        }
        if (SPIO_UNLIKELY(ret.value() != s.size())) {
            s = s.subspan(ret.value());
        }
        break;
    }
    return total_written;
}

template <typename Device>
result vwrite_all(Device& d, gsl::span<const gsl::byte> s)
{
}

SPIO_END_NAMESPACE

#endif  // SPIO_SINK_H
