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

#ifndef SPIO_CONFIG_H
#define SPIO_CONFIG_H

#include <cstddef>

#define SPIO_STD_11 201103L
#define SPIO_STD_14 201402L
#define SPIO_STD_17 201703L

#define SPIO_COMPILER(major, minor, patch) \
    ((major)*10000000 /* 10,000,000 */ + (minor)*10000 /* 10,000 */ + (patch))

#ifdef __INTEL_COMPILER
// Intel
#define SPIO_INTEL                                                      \
    SPIO_COMPILER(__INTEL_COMPILER / 100, (__INTEL_COMPILER / 10) % 10, \
                  __INTEL_COMPILER % 10)
#elif defined(_MSC_VER) && defined(_MSC_FULL_VER)
// MSVC
#if _MSC_VER == _MSC_FULL_VER / 10000
#define SPIO_MSVC \
    SPIO_COMPILER(_MSC_VER / 100, _MSC_VER % 100, _MSC_FULL_VER % 10000)
#else
#define SPIO_MSVC                                                \
    SPIO_COMPILER(_MSC_VER / 100, (_MSC_FULL_VER / 100000) % 10, \
                  _MSC_FULL_VER % 100000)
#endif  // _MSC_VER == _MSC_FULL_VER / 10000
#elif defined(__clang__) && defined(__clang_minor__) && \
    defined(__clang_patchlevel__)
// Clang
#define SPIO_CLANG \
    SPIO_COMPILER(__clang_major__, __clang_minor__, __clang_patchlevel__)
#elif defined(__GNUC__) && defined(__GNUC_MINOR__) && \
    defined(__GNUC_PATCHLEVEL__)
#define SPIO_GCC SPIO_COMPILER(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)
#endif

#ifndef SPIO_INTEL
#define SPIO_INTEL 0
#endif
#ifndef SPIO_MSVC
#define SPIO_MSVC 0
#endif
#ifndef SPIO_CLANG
#define SPIO_CLANG 0
#endif
#ifndef SPIO_GCC
#define SPIO_GCC 0
#endif

#if defined(__MINGW64__)
#define SPIO_MINGW 64
#elif defined(__MINGW32__)
#define SPIO_MINGW 32
#else
#define SPIO_MINGW 0
#endif

// Compiler is gcc or pretends to be (clang/icc)
#if defined(__GNUC__) && !defined(__GNUC_MINOR__)
#define SPIO_GCC_COMPAT SPIO_COMPILER(__GNUC__, 0, 0)
#elif defined(__GNUC__) && !defined(__GNUC_PATCHLEVEL_)
#define SPIO_GCC_COMPAT SPIO_COMPILER(__GNUC__, __GNUC_MINOR__, 0)
#elif defined(__GNUC__)
#define SPIO_GCC_COMPAT \
    SPIO_COMPILER(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)
#else
#define SPIO_GCC_COMPAT 0
#endif

#if defined(_WIN32) || defined(_WINDOWS)
#define SPIO_WINDOWS 1
#else
#define SPIO_WINDOWS 0
#endif

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#define SPIO_POSIX 1
#else
#define SPIO_POSIX 0
#endif

#ifdef _MSVC_LANG
#define SPIO_MSVC_LANG _MSVC_LANG
#else
#define SPIO_MSVC_LANG 0
#endif

#ifdef __has_include
#define SPIO_HAS_INCLUDE(x) __has_include(x)
#else
#define SPIO_HAS_INCLUDE(x) 0
#endif

#ifdef __has_cpp_attribute
#define SPIO_HAS_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
#else
#define SPIO_HAS_CPP_ATTRIBUTE(x) 0
#endif

#ifdef __has_feature
#define SPIO_HAS_FEATURE(x) __has_feature(x)
#else
#define SPIO_HAS_FEATURE(x) 0
#endif

#ifdef __has_builtin
#define SPIO_HAS_BUILTIN(x) __has_builtin(x)
#else
#define SPIO_HAS_BUILTIN(x) 0
#endif

#ifndef SPIO_BEGIN_NAMESPACE
#define SPIO_INLINE_NAMESPACE_NAME v0
#if SPIO_HAS_FEATURE(cxx_inline_namespaces) || \
    SPIO_MSVC >= SPIO_COMPILER(19, 0, 0) ||    \
    SPIO_INTEL >= SPIO_COMPILER(14, 0, 0)
#define SPIO_INLINE_NAMESPACE inline namespace
#define SPIO_END_NAMESPACE \
    }                      \
    }
#else
#define SPIO_INLINE_NAMESPACE namespace
#define SPIO_END_NAMESPACE                      \
    }                                           \
    using namespace SPIO_INLINE_NAMESPACE_NAME; \
    }
#endif  // cxx_inline_namespaces
#define SPIO_BEGIN_NAMESPACE                         \
    namespace spio {                                 \
    SPIO_INLINE_NAMESPACE SPIO_INLINE_NAMESPACE_NAME \
    {
#endif

// Detect string_view
#if defined(__cpp_lib_string_view) || (__has_include(<string_view>) && __cplusplus >= SPIO_STD_17) || \
    (SPIO_MSVC >= SPIO_COMPILER(19, 10, 0) && SPIO_MSVC_LANG >= SPIO_STD_17)
#define SPIO_HAS_STD_STRING_VIEW 1
#endif
#if defined(__cpp_lib_experimental_string_view) || (__has_include(<experimental/string_view>) && __cplusplus >= SPIO_STD_14)
#define SPIO_HAS_EXP_STRING_VIEW 1
#endif

#ifndef SPIO_HAS_STD_STRING_VIEW
#define SPIO_HAS_STD_STRING_VIEW 0
#endif
#ifndef SPIO_HAS_EXP_STRING_VIEW
#define SPIO_HAS_EXP_STRING_VIEW 0
#endif

// Detect constexpr
#if defined(__cpp_constexpr)
#if __cpp_constexpr >= 201603
#define SPIO_HAS_CONSTEXPR 3  // constexpr lambdas
#elif __cpp_constexpr >= 201304
#define SPIO_HAS_CONSTEXPR 2  // relaxed constexpr
#elif __cpp_constexpr >= 200704
#define SPIO_HAS_CONSTEXPR 1  // basic C++11 constexpr
#else
#define SPIO_HAS_CONSTEXPR 0
#endif
#endif

#ifndef SPIO_HAS_CONSTEXPR
#if __has_feature(cxx_relaxed_constexpr) ||     \
    SPIO_MSVC >= SPIO_COMPILER(19, 10, 0) ||    \
    ((SPIO_GCC >= SPIO_COMPILER(6, 0, 0) ||     \
      SPIO_INTEL >= SPIO_COMPILER(17, 0, 0)) && \
     __cplusplus >= SPIO_STD_14)
#define SPIO_HAS_CONSTEXPR 2  // relaxed
#elif __has_feature(cxx_constexpr) || SPIO_MSVC >= SPIO_COMPILER(19, 0, 0) || \
    SPIO_GCC >= SPIO_COMPILER(4, 6, 0) ||                                     \
    SPIO_INTEL >= SPIO_COMPILER(14, 0, 0)
#define SPIO_HAS_CONSTEXPR 1
#else
#define SPIO_HAS_CONSTEXPR 0
#endif
#endif

#if SPIO_HAS_CONSTEXPR >= 2
#define SPIO_CONSTEXPR constexpr
#define SPIO_CONSTEXPR_STRICT constexpr
#define SPIO_CONSTEXPR_DECL constexpr
#elif SPIO_HAS_CONSTEXPR
#define SPIO_CONSTEXPR inline
#define SPIO_CONSTEXPR_STRICT constexpr
#define SPIO_CONSTEXPR_DECL
#else
#define SPIO_CONSTEXPR inline
#define SPIO_CONSTEXPR_STRICT inline
#define SPIO_CONSTEXPR_DECL
#endif

// Detect noexcept
#if SPIO_HAS_FEATURE(cxx_noexcept) || SPIO_MSVC >= SPIO_COMPILER(19, 0, 0) || \
    SPIO_GCC >= SPIO_COMPILER(4, 6, 0) ||                                     \
    SPIO_INTEL >= SPIO_COMPILER(14, 0, 0)
#define SPIO_NOEXCEPT noexcept
#else
#define SPIO_NOEXCEPT throw()
#endif

// Detect [[nodiscard]]
#if (SPIO_HAS_CPP_ATTRIBUTE(nodiscard) && __cplusplus >= SPIO_STD_17) || \
    (SPIO_MSVC >= SPIO_COMPILER(19, 11, 0) &&                            \
     SPIO_MSVC_LANG >= SPIO_STD_17) ||                                   \
    ((SPIO_GCC >= SPIO_COMPILER(7, 0, 0) ||                              \
      SPIO_INTEL >= SPIO_COMPILER(18, 0, 0)) &&                          \
     __cplusplus >= SPIO_STD_17)
#define SPIO_NODISCARD [[nodiscard]]
#else
#define SPIO_NODISCARD /*nodiscard*/
#endif

// Detect [[noreturn]]
#if (SPIO_HAS_CPP_ATTRIBUTE(noreturn) && __cplusplus >= SPIO_STD_17) || \
    (SPIO_MSVC >= SPIO_COMPILER(19, 11, 0) &&                           \
     SPIO_MSVC_LANG >= SPIO_STD_17) ||                                  \
    ((SPIO_GCC >= SPIO_COMPILER(7, 0, 0) ||                             \
      SPIO_INTEL >= SPIO_COMPILER(18, 0, 0)) &&                         \
     __cplusplus >= SPIO_STD_17)
#define SPIO_NORETURN [[noreturn]]
#else
#define SPIO_NORETURN /*noreturn*/
#endif

// Detect make_unique
#if __cplusplus >= SPIO_STD_14 || SPIO_MSVC_LANG >= SPIO_STD_14
#define SPIO_HAS_MAKE_UNIQUE 1
#else
#define SPIO_HAS_MAKE_UNIQUE 0
#endif

// Detect void_t
#if (defined(__cpp_lib_void_t) && __cpp_lib_void_t >= 201411) || \
    __cplusplus >= SPIO_STD_17 || SPIO_MSVC_LANG >= SPIO_STD_17
#define SPIO_HAS_VOID_T 1
#else
#define SPIO_HAS_VOID_T 0
#endif

// Detect conjunction, disjunction, negation
#if (defined(__cpp_lib_logical_traits) &&   \
     __cpp_lib_logical_traits >= 201510) || \
    __cplusplus >= SPIO_STD_17 || SPIO_MSVC_LANG >= SPIO_STD_17
#define SPIO_HAS_LOGICAL_TRAITS 1
#else
#define SPIO_HAS_LOGICAL_TRAITS 0
#endif

// Detect __assume
#if SPIO_INTEL || SPIO_MSVC
#define SPIO_HAS_ASSUME 1
#else
#define SPIO_HAS_ASSUME 0
#endif

// Detect __builtin_assume
#if SPIO_HAS_BUILTIN(__builtin_assume)
#define SPIO_HAS_BUILTIN_ASSUME 1
#else
#define SPIO_HAS_BUILTIN_ASSUME 0
#endif

// Detect __builtin_unreachable
#if SPIO_HAS_BUILTIN(__builtin_unreachable) || \
    (SPIO_GCC >= SPIO_COMPILER(4, 5, 0))
#define SPIO_HAS_BUILTIN_UNREACHABLE 1
#else
#define SPIO_HAS_BUILTIN_UNREACHABLE 0
#endif

#if SPIO_HAS_ASSUME
#define SPIO_ASSUME(x) __assume(x)
#elif SPIO_HAS_BUILTIN_ASSUME
#define SPIO_ASSUME(x) __builtin_assume(x)
#elif SPIO_HAS_BUILTIN_UNREACHABLE
#define SPIO_ASSUME(x)               \
    do {                             \
        if (!(x)) {                  \
            __builtin_unreachable(); \
        }                            \
    } while (false)
#else
#define SPIO_ASSUME(x) static_cast<void>(sizeof(x))
#endif

#if SPIO_HAS_BUILTIN_UNREACHABLE
#define SPIO_UNREACHABLE __builtin_unreachable()
#else
#define SPIO_UNEACHABLE SPIO_ASSUME(false)
#endif

// Detect __builtin_expect
#if SPIO_HAS_BUILTIN(__builtin_expect) || SPIO_GCC_COMPAT
#define SPIO_HAS_BUILTIN_EXPECT 1
#else
#define SPIO_HAS_BUILTIN_EXPECT 0
#endif

#if SPIO_HAS_BUILTIN_EXPECT
#define SPIO_LIKELY(x) __builtin_expect(!!(x), 1)
#define SPIO_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define SPIO_LIKELY(x) (x)
#define SPIO_UNLIKELY(x) (x)
#endif

// Min version:
//
// = default:
// gcc 4.4
// clang 3.0
// msvc 18.0 (2013)
// intel 12.0
//
// variadic templates:
// gcc 4.4
// clang 2.9
// msvc 18.0 (2013)
// intel 12.1
//
// nullptr:
// gcc 4.6
// clang 2.9
// msvc 16.0 (2010)
// intel 12.1

#ifndef SPIO_USE_AFIO
#define SPIO_USE_AFIO \
    __cplusplus >= SPIO_STD_17 || SPIO_MSVC_LANG >= SPIO_STD_17
#endif

#endif  // SPIO_CONFIG_H
