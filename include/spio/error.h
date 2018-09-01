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

#ifndef SPIO_ERROR_H
#define SPIO_ERROR_H

#include "config.h"

#include <cerrno>
#include <cstdio>
#include <exception>
#include <stdexcept>
#include <string>
#include <system_error>
#include "third_party/fmt.h"
#include "third_party/gsl.h"

namespace spio {
SPIO_BEGIN_NAMESPACE

enum error {
    invalid_input,
    invalid_operation,
    end_of_file,
    unknown_io_error,
    bad_variant_access,
    out_of_range,
    out_of_memory,
    sentry_error,
    scanner_error,
    unimplemented,
    unreachable,
    undefined_error
};

SPIO_END_NAMESPACE
}  // namespace spio

namespace std {
template <>
struct is_error_code_enum<spio::error> : true_type {
};
}  // namespace std

namespace spio {
SPIO_BEGIN_NAMESPACE

struct error_category : public std::error_category {
    const char* name() const noexcept override
    {
        return "spio";
    }

    std::string message(int ev) const override
    {
#if SPIO_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-default"  // this switch is exhaustive
#endif
        switch (static_cast<error>(ev)) {
            case invalid_input:
                return "Invalid input";
            case invalid_operation:
                return "Invalid operation";
            case end_of_file:
                return "EOF";
            case unknown_io_error:
                return "Unknown IO error";
            case bad_variant_access:
                return "Bad variant access";
            case out_of_range:
                return "Out of range";
            case out_of_memory:
                return "Out of memory";
            case sentry_error:
                return "Sentry error";
            case scanner_error:
                return "Scanner error";
            case unimplemented:
                return "Unimplemented";
            case unreachable:
                return "Unreachable code";
            case undefined_error:
                return "[undefined error]";
        }
        assert(false);
        std::terminate();
#if SPIO_GCC
#pragma GCC diagnostic pop
#endif
    }
};

namespace detail {
#if SPIO_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif
    inline const error_category& get_error_category()
    {
        static error_category inst;
        return inst;
    }
#if SPIO_CLANG
#pragma clang diagnostic pop
#endif
}  // namespace detail

inline std::error_code make_error_code(error e)
{
    return {static_cast<int>(e), detail::get_error_category()};
}

#if SPIO_WINDOWS
#define SPIO_MAKE_ERRNO ::std::error_code(errno, ::std::generic_category())
#define SPIO_MAKE_WIN32_ERROR \
    ::std::error_code(::GetLastError(), ::std::system_category())
#else
#define SPIO_MAKE_ERRNO ::std::error_code(errno, ::std::system_category())
#endif

class failure : public std::system_error {
public:
    failure(std::error_code c) : system_error(c) {}
    failure(std::error_code c, const std::string& desc) : system_error(c, desc)
    {
    }

    failure(error c)
        : system_error(static_cast<int>(c), detail::get_error_category())
    {
    }
    failure(error c, const std::string& desc)
        : system_error(static_cast<int>(c), detail::get_error_category(), desc)
    {
    }
};

#ifdef _MSC_VER
#define SPIO_DEBUG_UNREACHABLE                                            \
    __pragma(warning(suppress : 4702)) throw failure(::spio::unreachable, \
                                                     "Unreachable");      \
    __pragma(warning(suppress : 4702)) std::terminate()
#else
#define SPIO_DEBUG_UNREACHABLE                         \
    throw failure(::spio::unreachable, "Unreachable"); \
    std::terminate()
#endif

#ifdef _MSC_VER
#define SPIO_UNIMPLEMENTED_DEBUG                                            \
    __pragma(warning(suppress : 4702)) throw failure(::spio::unimplemented, \
                                                     "Unimplemented");      \
    __pragma(warning(suppress : 4702)) std::terminate()
#else
#define SPIO_UNIMPLEMENTED_DEBUG                           \
    throw failure(::spio::unimplemented, "Unimplemented"); \
    std::terminate()
#endif

#ifdef NDEBUG
#define SPIO_UNIMPLEMENTED SPIO_UNREACHABLE
#else
#define SPIO_UNIMPLEMENTED SPIO_UNIMPLEMENTED_DEBUG
#endif  // NDEBUG

SPIO_END_NAMESPACE
}  // namespace spio

#endif  // SPIO_ERROR_H
