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

#ifndef SPIO_SCANNER_H
#define SPIO_SCANNER_H

#include "config.h"

#include "stream_ref.h"

namespace spio {
SPIO_BEGIN_NAMESPACE

// Scan char-by-char and putback the final char
// For byte_readable, but can be used by readable
struct scan_char_strategy {
};
// Scan as much as possible at once and putback extra bytes
// For readable
struct scan_bulk_strategy {
};
// Scan as much as possible at once but don't putback extra bytes
// For random_access_readable
struct scan_bulk_nopb_strategy {
};

template <typename CharT, typename T>
result custom_scan(T&,
                   scan_char_strategy,
                   const CharT*&,
                   std::function<result(CharT&)>&,
                   std::function<bool(CharT)>&);
template <typename CharT, typename T>
result custom_scan(T&,
                   scan_bulk_strategy,
                   const CharT*&,
                   std::function<result(gsl::span<gsl::byte>)>&,
                   std::function<bool(gsl::span<gsl::byte>)>&);
template <typename CharT, typename T>
result custom_scan(T&,
                   scan_bulk_nopb_strategy,
                   const CharT*&,
                   std::function<result(gsl::span<gsl::byte>)>&);

template <typename CharT, typename Strategy>
struct basic_arg;

template <typename CharT>
struct basic_arg<CharT, scan_char_strategy> {
    using fn_type =
        nonstd::expected<void, failure> (*)(const CharT*&,
                                            std::function<result(CharT&)>&,
                                            std::function<bool(CharT)>&,
                                            void*);

    void* value;
    fn_type scan;
};
template <typename CharT>
struct basic_arg<CharT, scan_bulk_strategy> {
    using fn_type = nonstd::expected<void, failure> (*)(
        const CharT*&,
        std::function<result(gsl::span<gsl::byte>)>&,
        std::function<bool(gsl::span<gsl::byte>)>&,
        void*);

    void* value;
    fn_type scan;
};
template <typename CharT>
struct basic_arg<CharT, scan_bulk_nopb_strategy> {
    using fn_type = nonstd::expected<void, failure> (*)(
        const CharT*&,
        std::function<result(gsl::span<gsl::byte>)>&,
        void*);

    void* value;
    fn_type scan;
};

template <typename Arg, typename Allocator = std::allocator<Arg>>
using basic_arg_list = std::vector<Arg*, Allocator>;

template <typename CharT>
nonstd::expected<void, failure> skip_format(const CharT*& f)
{
    if (*f == 0) {
        return {};
    }
    if (*f != CharT('}')) {
        return nonstd::make_unexpected(
            failure{std::make_error_code(std::errc::invalid_argument),
                    "Invalid format string: expected '}'"});
    }
    ++f;
    return {};
}

template <typename CharT, typename Strategy>
struct basic_scanner;

template <typename CharT>
struct basic_scanner<CharT, scan_char_strategy> {
    nonstd::expected<void, failure> operator()(
        basic_string_view<CharT> f,
        std::function<result(gsl::byte&)>& r,
        std::function<bool(gsl::byte)>& pb,
        basic_arg_list<basic_arg<CharT, scan_char_strategy>*> a);
};
template <typename CharT>
struct basic_scanner<CharT, scan_bulk_strategy> {
    nonstd::expected<void, failure> operator()(
        basic_string_view<CharT> f,
        std::function<result(gsl::span<gsl::byte>)>& r,
        std::function<bool(gsl::span<gsl::byte>)>& pb,
        basic_arg_list<basic_arg<CharT, scan_bulk_strategy>*> a);
};
template <typename CharT>
struct basic_scanner<CharT, scan_bulk_nopb_strategy> {
    nonstd::expected<void, failure> operator()(
        basic_string_view<CharT> f,
        std::function<result(gsl::span<gsl::byte>)>& r,
        basic_arg_list<basic_arg<CharT, scan_bulk_nopb_strategy>*> a);
};

SPIO_END_NAMESPACE
}

#endif  // SPIO_SCANNER_H
