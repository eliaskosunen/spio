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

SPIO_END_NAMESPACE
}  // namespace spio

#endif  // SPIO_UTIL_H
