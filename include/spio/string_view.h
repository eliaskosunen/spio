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

#ifndef SPIO_STRING_VIEW_H
#define SPIO_STRING_VIEW_H

#include "config.h"

#include <string>
#if SPIO_HAS_STD_STRING_VIEW
#include <string_view>
#elif SPIO_HAS_EXP_STRING_VIEW
#include <experimental/string_view>
#else
#include "third_party/gsl.h"
#endif

SPIO_BEGIN_NAMESPACE

#if SPIO_HAS_STD_STRING_VIEW
template <typename CharT, typename Traits = std::char_traits<CharT>>
using basic_string_view = std::basic_string_view<CharT, Traits>;
#elif SPIO_HAS_EXP_STRING_VIEW
template <typename CharT, typename Traits = std::char_traits<CharT>>
using basic_string_view = std::experimental::basic_string_view<CharT, Traits>;
#else
template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_string_view {
public:
    using traits_type = Traits;
    using value_type = CharT;
    using span_type = gsl::span<const value_type>;

    using pointer = value_type*;
    using const_pointer = const value_type*;
    using reference = value_type&;
    using const_reference = const value_type&;
    using iterator = typename span_type::const_iterator;
    using const_iterator = iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = reverse_iterator;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    static SPIO_CONSTEXPR_DECL const size_type npos = size_type(-1);

    SPIO_CONSTEXPR_STRICT basic_string_view() noexcept = default;
    SPIO_CONSTEXPR_STRICT basic_string_view(const_pointer s, size_type c)
        : m_data(s, c)
    {
    }
    SPIO_CONSTEXPR_STRICT basic_string_view(const_pointer s)
        : m_data(s, static_cast<std::ptrdiff_t>(Traits::length(s)))
    {
    }

    SPIO_CONSTEXPR_STRICT const_iterator begin() const noexcept
    {
        return cbegin();
    }
    SPIO_CONSTEXPR_STRICT const_iterator cbegin() const noexcept
    {
        return m_data.cbegin();
    }
    SPIO_CONSTEXPR_STRICT const_iterator end() const noexcept
    {
        return cend();
    }
    SPIO_CONSTEXPR_STRICT const_iterator cend() const noexcept
    {
        return m_data.cend();
    }

    SPIO_CONSTEXPR_STRICT const_iterator rbegin() const noexcept
    {
        return crbegin();
    }
    SPIO_CONSTEXPR_STRICT const_iterator crbegin() const noexcept
    {
        return m_data.crbegin();
    }
    SPIO_CONSTEXPR_STRICT const_iterator rend() const noexcept
    {
        return crend();
    }
    SPIO_CONSTEXPR_STRICT const_iterator crend() const noexcept
    {
        return m_data.crend();
    }

    SPIO_CONSTEXPR_STRICT const_reference operator[](size_type pos) const
    {
        return m_data[static_cast<typename span_type::index_type>(pos)];
    }
    SPIO_CONSTEXPR const_reference at(size_type pos) const
    {
        return m_data.at(static_cast<typename span_type::index_type>(pos));
    }

    SPIO_CONSTEXPR_STRICT const_reference front() const
    {
        return operator[](0);
    }
    SPIO_CONSTEXPR_STRICT const_reference back() const
    {
        return operator[](size() - 1);
    }
    SPIO_CONSTEXPR_STRICT const_pointer data() const noexcept
    {
        return m_data.data();
    }

    SPIO_CONSTEXPR_STRICT size_type size() const noexcept
    {
        return static_cast<size_type>(m_data.size());
    }
    SPIO_CONSTEXPR_STRICT size_type length() const noexcept
    {
        return size();
    }
    SPIO_CONSTEXPR_STRICT size_type max_size() const noexcept
    {
        return std::numeric_limits<size_type>::max() - 1;
    }
    SPIO_NODISCARD SPIO_CONSTEXPR_STRICT bool empty() const noexcept
    {
        return size() == 0;
    }

    SPIO_CONSTEXPR void remove_prefix(size_type n)
    {
        m_data = m_data.subspan(n);
    }
    SPIO_CONSTEXPR void remove_suffix(size_type n)
    {
        m_data = m_data.first(size() - n);
    }

    SPIO_CONSTEXPR void swap(basic_string_view& v) noexcept
    {
        using std::swap;
        swap(m_data, v.m_data);
    }

    size_type copy(pointer dest, size_type count, size_type pos = 0) const
    {
        auto n = std::min(count, size() - pos);
        Traits::copy(dest, data() + pos, n);
        return n;
    }
    SPIO_CONSTEXPR basic_string_view substr(size_type pos = 0,
                                            size_type count = npos) const
    {
        auto n = std::min(count, size() - pos);
        return m_data.subspan(pos, n);
    }

    int compare(basic_string_view v) const noexcept
    {
        auto n = std::min(size(), v.size());
        auto cmp = Traits::compare(data(), v.data(), n);
        if (cmp == 0) {
            if (size() == v.size())
                return 0;
            if (size() > v.size())
                return 1;
            return -1;
        }
        return cmp;
    }
    int compare(size_type pos1, size_type count1, basic_string_view v) const
    {
        return substr(pos1, count1).compare(v);
    }
    int compare(size_type pos1,
                size_type count1,
                basic_string_view v,
                size_type pos2,
                size_type count2) const
    {
        return substr(pos1, count1).compare(v.substr(pos2, count2));
    }
    int compare(const_pointer s) const
    {
        return compare(basic_string_view(s));
    }
    int compare(size_type pos1, size_type count1, const_pointer s) const
    {
        return substr(pos1, count1).compare(basic_string_view(s));
    }
    int compare(size_type pos1,
                size_type count1,
                const_pointer s,
                size_type count2) const
    {
        return substr(pos1, count1).compare(basic_string_view(s, count2));
    }

private:
    span_type m_data{};
};
#endif

using string_view = basic_string_view<char>;
using wstring_view = basic_string_view<wchar_t>;
using u16string_view = basic_string_view<char>;
using u32wstring_view = basic_string_view<wchar_t>;

SPIO_END_NAMESPACE

#endif  // SPIO_STRING_VIEW_H
