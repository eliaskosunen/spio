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

#ifndef SPIO_THIRD_PARTY_FMT_H
#define SPIO_THIRD_PARTY_FMT_H

#include "../config.h"

#if SPIO_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wswitch-enum"
#pragma GCC diagnostic ignored "-Wswitch-default"
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wstrict-overflow"
#endif

#if SPIO_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wswitch-enum"
#pragma clang diagnostic ignored "-Wmissing-noreturn"
#pragma clang diagnostic ignored "-Wweak-vtables"
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#pragma clang diagnostic ignored "-Wunused-member-function"
#pragma clang diagnostic ignored "-Wglobal-constructors"
#endif

#if SPIO_MSVC
#pragma warning(push)
#pragma warning(disable : 4127)
#endif

#include "fmt/format.h"

namespace spio {
SPIO_BEGIN_NAMESPACE
namespace fmt {
    using ::fmt::basic_format_args;
    using ::fmt::basic_format_context;
    using ::fmt::basic_string_view;
    using ::fmt::format;
    using ::fmt::format_context_t;
    using ::fmt::format_to;
    using ::fmt::make_format_args;
    using ::fmt::vformat;
    using ::fmt::vformat_to;
}  // namespace fmt
SPIO_END_NAMESPACE
}  // namespace spio

#if SPIO_MSVC
#pragma warning(pop)
#endif

#if SPIO_CLANG
#pragma clang diagnostic pop
#endif

#if SPIO_GCC
#pragma GCC diagnostic pop
#endif

#endif  // SPIO_THIRD_PARTY_FMT_H
