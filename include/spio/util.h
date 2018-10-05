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

#ifndef SPIO_UTIL_H
#define SPIO_UTIL_H

#include "config.h"

#include <cstring>
#include <memory>
#include "third_party/gsl.h"

namespace spio {
SPIO_BEGIN_NAMESPACE

#define SPIO_STRINGIZE_DETAIL(x) #x
#define SPIO_STRINGIZE(x) SPIO_STRINGIZE_DETAIL(x)
#define SPIO_LINE SPIO_STRINGIZE(__LINE__)

#define SPIO_UNUSED(x) (static_cast<void>(x))

#if SPIO_HAS_MAKE_UNIQUE
using std::make_unique;
#else
template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
#endif

#if SPIO_HAS_VOID_T
using std::void_t;
#else
template <typename... Ts>
struct make_void {
    typedef void type;
};
template <typename... Ts>
using void_t = typename make_void<Ts...>::type;
#endif

#if SPIO_HAS_LOGICAL_TRAITS
using std::conjunction;
using std::disjunction;
using std::negation;
#else
// AND
template <class...>
struct conjunction : std::true_type {
};
template <class B1>
struct conjunction<B1> : B1 {
};
template <class B1, class... Bn>
struct conjunction<B1, Bn...>
    : std::conditional<bool(B1::value), conjunction<Bn...>, B1>::type {
};

// OR
template <class...>
struct disjunction : std::false_type {
};
template <class B1>
struct disjunction<B1> : B1 {
};
template <class B1, class... Bn>
struct disjunction<B1, Bn...>
    : std::conditional<bool(B1::value), B1, disjunction<Bn...>>::type {
};

// NOT
template <class B>
struct negation : std::integral_constant<bool, !bool(B::value)> {
};
#endif

struct nonesuch {
    nonesuch() = delete;
    nonesuch(const nonesuch&) = delete;
    nonesuch(nonesuch&&) noexcept = delete;
    void operator=(const nonesuch&) = delete;
    void operator=(nonesuch&&) noexcept = delete;
    ~nonesuch() = delete;
};

namespace detail {
    template <class Default,
              class AlwaysVoid,
              template <class...> class Op,
              class... Args>
    struct detector {
        using value_t = std::false_type;
        using type = Default;
    };

    template <class Default, template <class...> class Op, class... Args>
    struct detector<Default, void_t<Op<Args...>>, Op, Args...> {
        using value_t = std::true_type;
        using type = Op<Args...>;
    };
}  // namespace detail

template <template <class...> class Op, class... Args>
using is_detected =
    typename detail::detector<nonesuch, void, Op, Args...>::value_t;

template <template <class...> class Op, class... Args>
using detected_t = typename detail::detector<nonesuch, void, Op, Args...>::type;

template <class Default, template <class...> class Op, class... Args>
using detected_or = detail::detector<Default, void, Op, Args...>;

namespace detail {
    template <typename T>
    T round_up_power_of_two(T n)
    {
        T p = 1;
        while (p < n)
            p *= 2;
        return p;
    }
#if SPIO_HAS_BUILTIN(__builtin_clz)
    inline uint32_t round_up_power_of_two(uint32_t n)
    {
        Expects(n > 1);
        Expects(n <= std::numeric_limits<uint32_t>::max() / 2 + 1);
        return 1 << (32 - __builtin_clz(n - 1));
    }
#endif
}  // namespace detail

template <typename Container, typename Element, typename = int>
class memcpy_back_insert_iterator {
public:
    using value_type = void;
    using difference_type = void;
    using pointer = void;
    using reference = void;
    using iterator_category = std::output_iterator_tag;

    using container_type = Container;
    using element_type = Element;
    using container_value_type = typename Container::value_type;

    static_assert(sizeof(element_type) % sizeof(container_value_type) == 0,
                  "Element size not divisible by Container value_type");

    memcpy_back_insert_iterator(container_type& c) noexcept
        : m_container(std::addressof(c))
    {
    }

    memcpy_back_insert_iterator& operator=(const element_type& value) noexcept
    {
        Expects(m_container);
        const auto n = sizeof(element_type) / sizeof(container_value_type);
        for (size_t i = 0; i < n; ++i) {
            m_container->emplace_back();
        }
        std::memcpy(m_container->data() + m_container->size() - n,
                    std::addressof(value), sizeof(element_type));
        return *this;
    }

    memcpy_back_insert_iterator& operator*() noexcept
    {
        return *this;
    }

    memcpy_back_insert_iterator& operator++() noexcept
    {
        return *this;
    }
    memcpy_back_insert_iterator& operator++(int) noexcept
    {
        return *this;
    }

private:
    container_type* m_container;
};

template <typename Container, typename Element>
class memcpy_back_insert_iterator<
    Container,
    Element,
    typename std::enable_if<
        !std::is_same<typename Container::value_type, byte>::value &&
        sizeof(Element) == sizeof(typename Container::value_type)>::type> {
public:
    using value_type = void;
    using difference_type = void;
    using pointer = void;
    using reference = void;
    using iterator_category = std::output_iterator_tag;

    using container_type = Container;
    using element_type = Element;
    using container_value_type = typename Container::value_type;

    static_assert(sizeof(element_type) % sizeof(container_value_type) == 0,
                  "Element size not divisible by Container value_type");

    memcpy_back_insert_iterator(container_type& c) noexcept
        : m_container(std::addressof(c))
    {
    }

    memcpy_back_insert_iterator& operator=(const element_type& value) noexcept
    {
        m_container->emplace_back();
        std::memcpy(m_container->data() + m_container->size() - 1,
                    std::addressof(value), sizeof(element_type));
        return *this;
    }

    memcpy_back_insert_iterator& operator*()
    {
        return *this;
    }

    memcpy_back_insert_iterator& operator++()
    {
        return *this;
    }
    memcpy_back_insert_iterator& operator++(int)
    {
        return *this;
    }

private:
    container_type* m_container;
};

template <typename Container, typename Element>
class memcpy_back_insert_iterator<
    Container,
    Element,
    typename std::enable_if<
        std::is_same<typename Container::value_type, byte>::value &&
        sizeof(Element) == sizeof(typename Container::value_type)>::type> {
public:
    using value_type = void;
    using difference_type = void;
    using pointer = void;
    using reference = void;
    using iterator_category = std::output_iterator_tag;

    using container_type = Container;
    using element_type = Element;
    using container_value_type = typename Container::value_type;

    static_assert(sizeof(element_type) % sizeof(container_value_type) == 0,
                  "Element size not divisible by Container value_type");

    memcpy_back_insert_iterator(container_type& c) noexcept
        : m_container(std::addressof(c))
    {
    }

    memcpy_back_insert_iterator& operator=(const element_type& value) noexcept
    {
        m_container->push_back(to_byte(value));
        return *this;
    }

    memcpy_back_insert_iterator& operator*()
    {
        return *this;
    }

    memcpy_back_insert_iterator& operator++()
    {
        return *this;
    }
    memcpy_back_insert_iterator& operator++(int)
    {
        return *this;
    }

private:
    container_type* m_container;
};

template <typename Integral>
SPIO_CONSTEXPR14 int max_digits() noexcept
{
    auto i = std::numeric_limits<Integral>::max();

    int digits = 0;
    while (i) {
        i /= 10;
        digits++;
    }

    return digits + (std::is_signed<Integral>::value ? 1 : 0);
}

template <typename CharT>
SPIO_CONSTEXPR14 bool is_digit(CharT c, int base = 10)
{
    Expects(base >= 2 && base <= 36);
    if (base <= 10) {
        return c >= '0' && c <= '0' + (base - 1);
    }
    return is_digit(c, 10) || (c >= 'a' && c <= 'a' + (base - 1)) ||
           (c >= 'A' && c <= 'A' + (base - 1));
}

template <typename IntT, typename CharT>
SPIO_CONSTEXPR14 IntT char_to_int(CharT c, int base)
{
    Expects(base >= 2 && base <= 36);
    Expects(is_digit(c, base));
    if (base <= 10) {
        assert(c <= '0' + (base - 1));
        return static_cast<IntT>(c - '0');
    }
    if (c <= '9') {
        return static_cast<IntT>(c - '0');
    }
    if (c >= 'a' && c <= 'z') {
        return 10 + static_cast<IntT>(c - 'a');
    }
    auto ret = 10 + static_cast<IntT>(c - 'A');
    return ret;
}

namespace detail {
    template <typename T>
    struct powers_of_10_return;
    template <>
    struct powers_of_10_return<long double> {
        using type = std::array<long double, 11>;
    };
    template <>
    struct powers_of_10_return<double> {
        using type = std::array<double, 9>;
    };
    template <>
    struct powers_of_10_return<float> {
        using type = std::array<float, 6>;
    };

    template <typename FloatingT>
    constexpr typename powers_of_10_return<FloatingT>::type powers_of_10();

    template <>
    constexpr typename powers_of_10_return<long double>::type powers_of_10<long double>()
    {
#if SPIO_MSVC
        return std::array<long double, 11>{{10.l, 100.l, 1.0e4l, 1.0e8l,
                                            1.0e16l, 1.0e32l, 1.0e64l, 1.0e128l,
                                            1.0e256l}};
#else
        return std::array<long double, 11>{{10.l, 100.l, 1.0e4l, 1.0e8l,
                                            1.0e16l, 1.0e32l, 1.0e64l, 1.0e128l,
                                            1.0e256l, 1.0e512l, 1.0e1024l}};
#endif
    }
    template <>
    constexpr typename powers_of_10_return<float>::type powers_of_10<float>()
    {
        return std::array<float, 6>{
            {10.f, 100.f, 1.0e4f, 1.0e8f, 1.0e16f, 1.0e32f}};
    }
    template <>
    constexpr typename powers_of_10_return<double>::type powers_of_10<double>()
    {
        return std::array<double, 9>{{10., 100., 1.0e4, 1.0e8, 1.0e16, 1.0e32,
                                      1.0e64, 1.0e128, 1.0e256}};
    }

    template <typename FloatingT>
    constexpr int max_exponent()
    {
        return 2047;
    }
    template <>
    constexpr int max_exponent<float>()
    {
        return 63;
    }
    template <>
    constexpr int max_exponent<double>()
    {
        return 511;
    }
}  // namespace detail

/*
 * The original C implementation of this function:
 *
 * strtod.c --
 *
 *	Source code for the "strtod" library procedure.
 *
 * Copyright (c) 1988-1993 The Regents of the University of California.
 * Copyright (c) 1994 Sun Microsystems, Inc.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */
template <typename FloatingT, typename CharT>
FloatingT str_to_floating(const CharT* str, CharT** end)
{
    static int maxExponent = detail::max_exponent<FloatingT>();
    static auto powersOf10 = detail::powers_of_10<FloatingT>();

    bool sign;
    bool expSign = false;
    FloatingT fraction, dblExp, *d;
    const CharT* p;
    int c;
    int exp = 0;       /* Exponent read from "EX" field. */
    int fracExp = 0;   /* Exponent that derives from the fractional
                        * part.  Under normal circumstatnces, it is
                        * the negative of the number of digits in F.
                        * However, if I is very long, the last digits
                        * of I get dropped (otherwise a long I with a
                        * large negative exponent could cause an
                        * unnecessary overflow on I alone).  In this
                        * case, fracExp is incremented one for each
                        * dropped digit. */
    int mantSize;      /* Number of digits in mantissa. */
    int decPt;         /* Number of mantissa digits BEFORE decimal
                        * point. */
    const CharT* pExp; /* Temporarily holds location of exponent
                        * in string. */

    /*
     * Strip off leading blanks and check for a sign.
     */

    p = str;
    while (std::isspace(*p)) {
        p += 1;
    }
    if (*p == '-') {
        sign = true;
        p += 1;
    }
    else {
        if (*p == '+') {
            p += 1;
        }
        sign = false;
    }

    /*
     * Count the number of digits in the mantissa (including the decimal
     * point), and also locate the decimal point.
     */

    decPt = -1;
    for (mantSize = 0;; mantSize += 1) {
        c = *p;
        if (!std::isdigit(c)) {
            if ((c != '.') || (decPt >= 0)) {
                break;
            }
            decPt = mantSize;
        }
        p += 1;
    }

    /*
     * Now suck up the digits in the mantissa.  Use two integers to
     * collect 9 digits each (this is faster than using floating-point).
     * If the mantissa has more than 18 digits, ignore the extras, since
     * they can't affect the value anyway.
     */

    pExp = p;
    p -= mantSize;
    if (decPt < 0) {
        decPt = mantSize;
    }
    else {
        mantSize -= 1; /* One of the digits was the point. */
    }
    if (mantSize > 18) {
        fracExp = decPt - 18;
        mantSize = 18;
    }
    else {
        fracExp = decPt - mantSize;
    }
    if (mantSize == 0) {
        fraction = static_cast<FloatingT>(0.0);
        p = str;
        goto done;
    }
    else {
        int frac1, frac2;
        frac1 = 0;
        for (; mantSize > 9; mantSize -= 1) {
            c = *p;
            p += 1;
            if (c == '.') {
                c = *p;
                p += 1;
            }
            frac1 = 10 * frac1 + (c - '0');
        }
        frac2 = 0;
        for (; mantSize > 0; mantSize -= 1) {
            c = *p;
            p += 1;
            if (c == '.') {
                c = *p;
                p += 1;
            }
            frac2 = 10 * frac2 + (c - '0');
        }
        fraction = (static_cast<FloatingT>(1.0e9) * frac1) + frac2;
    }

    /*
     * Skim off the exponent.
     */

    p = pExp;
    if ((*p == 'E') || (*p == 'e')) {
        p += 1;
        if (*p == '-') {
            expSign = true;
            p += 1;
        }
        else {
            if (*p == '+') {
                p += 1;
            }
            expSign = false;
        }
        while (std::isdigit(*p)) {
            exp = exp * 10 + (*p - '0');
            p += 1;
        }
    }
    if (expSign) {
        exp = fracExp - exp;
    }
    else {
        exp = fracExp + exp;
    }

    /*
     * Generate a floating-point number that represents the exponent.
     * Do this by processing the exponent one bit at a time to combine
     * many powers of 2 of 10. Then combine the exponent with the
     * fraction.
     */

    if (exp < 0) {
        expSign = true;
        exp = -exp;
    }
    else {
        expSign = false;
    }
    if (exp > maxExponent) {
        exp = maxExponent;
        errno = ERANGE;
    }
    dblExp = static_cast<FloatingT>(1.0);
    for (d = &powersOf10[0]; exp != 0; exp >>= 1, d += 1) {
        if (exp & 01) {
            dblExp *= *d;
        }
    }
    if (expSign) {
        fraction /= dblExp;
    }
    else {
        fraction *= dblExp;
    }

done:
    if (end != nullptr) {
        *end = const_cast<CharT*>(p);
    }

    if (sign) {
        return -fraction;
    }
    return fraction;
}

SPIO_END_NAMESPACE
}  // namespace spio

#endif  // SPIO_UTIL_H
