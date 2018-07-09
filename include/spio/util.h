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

#ifndef SPIO_UTIL_H
#define SPIO_UTIL_H

#include "config.h"

#include <cstring>
#include <memory>
#include "third_party/gsl.h"

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
    ~nonesuch() = delete;
    nonesuch(const nonesuch&) = delete;
    void operator=(const nonesuch&) = delete;
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

template <typename Container, typename Element>
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

    memcpy_back_insert_iterator(container_type& c)
        : m_container(std::addressof(c))
    {
    }

    memcpy_back_insert_iterator& operator=(const element_type& value)
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

#endif  // SPIO_UTIL_H
