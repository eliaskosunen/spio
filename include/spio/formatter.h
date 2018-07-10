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

#ifndef SPIO_FORMATTER_H
#define SPIO_FORMATTER_H

#include "config.h"

#include "device.h"
#include "result.h"
#include "string_view.h"
#include "third_party/fmt.h"
#include "util.h"

SPIO_BEGIN_NAMESPACE

template <typename CharT>
struct basic_formatter {
    template <typename OutputIt, typename... Args>
    OutputIt operator()(OutputIt it,
                        basic_string_view<CharT> f,
                        const Args&... a)
    {
        return fmt::format_to(it,
                              fmt::basic_string_view<CharT>(
                                  f.data(), static_cast<size_t>(f.size())),
                              a...);
    }
};

template <typename Stream, typename... Args>
auto print(Stream& s,
           basic_string_view<typename Stream::char_type> f,
           const Args&... a) ->
    typename std::enable_if<is_writable<typename Stream::device_type>::value,
                            result>::type
{
    std::vector<gsl::byte> buf;
    s.formatter()(memcpy_back_insert_iterator<std::vector<gsl::byte>,
                                              typename Stream::char_type>(buf),
                  f, a...);
    return write(s, buf);
}
template <typename Stream, typename... Args>
auto print(Stream& s,
           basic_string_view<typename Stream::char_type> f,
           const Args&... a) ->
    typename std::enable_if<
        is_byte_writable<typename Stream::device_type>::value,
        result>::type
{
    std::vector<gsl::byte> buf;
    s.formatter()(memcpy_back_insert_iterator<std::vector<gsl::byte>,
                                              typename Stream::char_type>(buf),
                  f, a...);
    auto r = result{0};
    for (auto& ch : buf) {
        auto tmp = put(s, ch);
        if (tmp.value() != 1 || tmp.has_error()) {
            return make_result(r.value(), tmp.error());
        }
        ++r.value();
    }
    return write(s, buf);
}
template <typename Stream, typename... Args>
auto print_at(Stream& s,
              streampos pos,
              basic_string_view<typename Stream::char_type> f,
              const Args&... a) ->
    typename std::enable_if<
        is_random_access_writable<typename Stream::device_type>::value,
        result>::type
{
    std::vector<gsl::byte> buf;
    s.formatter()(memcpy_back_insert_iterator<std::vector<gsl::byte>,
                                              typename Stream::char_type>(buf),
                  f, a...);
    return write_at(s, buf, pos);
}

SPIO_END_NAMESPACE

#endif  // SPIO_FORMATTER_H
