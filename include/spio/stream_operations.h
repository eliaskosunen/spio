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

#ifndef SPIO_STREAM_OPERATIONS_H
#define SPIO_STREAM_OPERATIONS_H

#include "config.h"

#include "stream_ref.h"

namespace spio {
SPIO_BEGIN_NAMESPACE

template <typename Stream>
auto putchar(Stream& s, typename Stream::char_type ch) ->
    typename std::enable_if<is_writable_stream<Stream>::value, result>
{
    auto r = write(s, gsl::as_bytes(gsl::make_span(std::addressof(ch), 1)));
    r.value() = Stream::character_type::from_device(r.value());
    return r;
}
template <typename Stream>
auto putchar(Stream& s, typename Stream::char_type ch) ->
    typename std::enable_if<is_byte_writable_stream<Stream>::value &&
                                !is_writable_stream<Stream>::value,
                            result>
{
    auto res = result(0);
    for (auto b : gsl::as_bytes(gsl::make_span(ch, 1))) {
        auto r = put(s, b);
        if (r.error() || r.value() != 1) {
            res.value() += r.value();
            res.value() = Stream::character_type::from_device(res.value());
            return make_result(res.value(), r.error());
        }
        ++res.value();
    }
    res.value() = Stream::character_type::from_device(res.value());
    res.value() /= sizeof(decltype(ch));
    return res;
}
template <typename Stream>
result putchar_at(Stream& s, typename Stream::char_type ch, streampos pos)
{
    auto r =
        write_at(s, gsl::as_bytes(gsl::make_span(std::addressof(ch), 1)), pos);
    r.value() = Stream::character_type::from_device(r.value());
    return r;
}

template <typename Stream>
auto getchar(Stream& s, typename Stream::char_type& ch) ->
    typename std::enable_if<is_readable_stream<Stream>::value, result>::type
{
    auto ret =
        read(s, gsl::as_writeable_bytes(gsl::make_span(std::addressof(ch), 1)));
    ret.value() = Stream::character_type::from_device(ret.value());
    return ret;
}
template <typename Stream>
auto getchar(Stream& s, typename Stream::char_type& ch) ->
    typename std::enable_if<is_byte_readable_stream<Stream>::value &&
                                !is_readable_stream<Stream>::value,
                            result>::type
{
    auto res = result(0);
    typename Stream::char_type tmp;
    for (auto& b : gsl::as_writeable_bytes(gsl::make_span(tmp, 1))) {
        auto r = get(s, b);
        if (r.error() || r.value() != 1) {
            res.value() += r.value();
            if (res.value() > 0) {
                if (putback(s, tmp.first(res.value()))) {
                    res.value() = 0;
                }
            }
            res.value() = Stream::character_type::from_device(res.value());
            return make_result(res.value(), r.error());
        }
        ++res.value();
    }
    ch = tmp;
    res.value() = Stream::character_type::from_device(res.value());
    return res;
}
template <typename Stream>
result getchar_at(Stream& s, typename Stream::char_type& ch, streampos pos)
{
    auto ret = read_at(
        s, gsl::as_writeable_bytes(gsl::make_span(std::addressof(ch), 1)), pos);
    ret.value() = Stream::character_type::from_device(ret.value());
    return ret;
}

SPIO_END_NAMESPACE
}  // namespace spio

#endif  // SPIO_STREAM_OPERATIONS_H
