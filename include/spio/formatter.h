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

#ifndef SPIO_FORMATTER_H
#define SPIO_FORMATTER_H

#include "config.h"

#include "device.h"
#include "result.h"
#include "stream_base.h"
#include "string_view.h"
#include "third_party/fmt.h"
#include "util.h"

SPIO_BEGIN_NAMESPACE

template <typename CharT>
struct basic_formatter {
    template <typename OutputIt>
    OutputIt operator()(
        OutputIt it,
        basic_string_view<CharT> f,
        fmt::basic_format_args<
            typename fmt::format_context_t<OutputIt, CharT>::type> a)
    {
        return fmt::vformat_to(it,
                               fmt::basic_string_view<CharT>(
                                   f.data(), static_cast<size_t>(f.size())),
                               a);
    }
};

template <typename Stream, typename... Args>
auto print(Stream& s,
           basic_string_view<typename Stream::char_type> f,
           const Args&... a)
    -> decltype(write(std::declval<Stream&>(),
                      std::declval<std::vector<gsl::byte>>()),
                result())
{
    std::vector<gsl::byte> buf;
    using iterator = memcpy_back_insert_iterator<std::vector<gsl::byte>,
                                                 typename Stream::char_type>;
    get_formatter(s)(iterator(buf), f,
                     fmt::make_format_args<typename fmt::format_context_t<
                         iterator, typename Stream::char_type>::type>(a...));
    return write(s, std::move(buf));
}
template <typename Stream, typename... Args>
auto print(Stream& s,
           basic_string_view<typename Stream::char_type> f,
           const Args&... a)
    -> decltype(write(std::declval<Stream&>(), std::declval<gsl::byte>()),
                result())
{
    std::vector<gsl::byte> buf;
    using iterator = memcpy_back_insert_iterator<std::vector<gsl::byte>,
                                                 typename Stream::char_type>;
    get_formatter(s)(iterator(buf), f,
                     fmt::make_format_args<typename fmt::format_context_t<
                         iterator, typename Stream::char_type>::type>(a...));
    auto r = result{0};
    for (auto& ch : buf) {
        auto tmp = put(s, ch);
        if (tmp.value() != 1 || tmp.has_error()) {
            return make_result(r.value(), tmp.error());
        }
        ++r.value();
    }
    return r;
}
template <typename Stream, typename... Args>
auto print_at(Stream& s,
              streampos pos,
              basic_string_view<typename Stream::char_type> f,
              const Args&... a)
    -> decltype(write_at(std::declval<Stream&>(),
                         std::declval<std::vector<gsl::byte>>(),
                         std::declval<streampos>()),
                result())
{
    std::vector<gsl::byte> buf;
    using iterator = memcpy_back_insert_iterator<std::vector<gsl::byte>,
                                                 typename Stream::char_type>;
    get_formatter(s)(iterator(buf), f,
                     fmt::make_format_args<typename fmt::format_context_t<
                         iterator, typename Stream::char_type>::type>(a...));
    return write_at(s, std::move(buf), pos);
}

SPIO_END_NAMESPACE

#endif  // SPIO_FORMATTER_H
