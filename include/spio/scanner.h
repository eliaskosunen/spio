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

#ifndef SPIO_SCANNER_H
#define SPIO_SCANNER_H

#include "config.h"

#include "stream.h"

SPIO_BEGIN_NAMESPACE

struct readable_scan_tag {
};
struct char_readable_scan_tag {
};
struct random_access_readable_scan_tag {
};

template <typename CharT, typename T, typename Enable = void>
result custom_scan(readable_scan_tag,
                   std::function<result(gsl::span<CharT>)>&,
                   const CharT*&,
                   T&);
template <typename CharT, typename T, typename Enable = void>
result custom_scan(char_readable_scan_tag,
                   std::function<result(CharT&)>&,
                   const CharT*&,
                   T&);
template <typename CharT, typename T, typename Enable = void>
result custom_scan(random_access_readable_scan_tag,
                   std::function<result(gsl::span<CharT>, streampos)>&,
                   streampos,
                   const CharT*&,
                   T&);

namespace detail {
    template <typename CharT>
    struct basic_arg {
        using scanner_fn_type = nonstd::expected<void, failure> (*)(
            std::function<result(gsl::span<CharT>)>&,
            const CharT*&,
            void*);

        void* value;
        scanner_fn_type scan;
    };
    template <typename CharT>
    struct basic_char_arg {
        using scanner_fn_type =
            nonstd::expected<void, failure> (*)(std::function<result(CharT&)>&,
                                                const CharT*&,
                                                void*);

        void* value;
        scanner_fn_type scan;
    };
    template <typename CharT>
    struct basic_random_access_arg {
        using scanner_fn_type = nonstd::expected<void, failure> (*)(
            std::function<result(gsl::span<CharT>, streampos)>&,
            streampos,
            const CharT*&,
            void*);

        void* value;
        scanner_fn_type scan;
    };

    template <typename Stream,
              typename Arg,
              typename Allocator = std::allocator<Arg>>
    class basic_arg_list {
    public:
        using stream_type = Stream;
        using arg_type = Arg;
        using allocator_type = Allocator;

        using storage_type = std::vector<arg_type, allocator_type>;

        basic_arg_list(storage_type v) : m_vec(std::move(v)) {}
        basic_arg_list(std::initializer_list<arg_type> i,
                       const Allocator& a = Allocator())
            : m_vec(i, a)
        {
        }

        arg_type& operator[](std::size_t i) &
        {
            return m_vec[i];
        }
        const arg_type& operator[](std::size_t i) const&
        {
            return m_vec[i];
        }
        arg_type&& operator[](std::size_t i) &&
        {
            return std::move(m_vec[i]);
        }

        storage_type& get() &
        {
            return m_vec;
        }
        const storage_type& get() const&
        {
            return m_vec;
        }
        storage_type&& get() &&
        {
            return m_vec;
        }

    private:
        storage_type m_vec;
    };

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

    template <typename CharT, typename T, typename = void>
    struct do_scan {
        static nonstd::expected<void, failure> scan(
            std::function<result(gsl::span<CharT>)>& s,
            CharT*& format,
            void* data)
        {
            T& val = *reinterpret_cast<T*>(data);
            auto ret = custom_scan(readable_scan_tag{}, s, format, val);
            if (!ret) {
                return nonstd::make_unexpected(ret.error());
            }
            return {};
        }
        static nonstd::expected<void, failure>
        scan(std::function<result(CharT&)>& s, CharT*& format, void* data)
        {
            T& val = *reinterpret_cast<T*>(data);
            auto ret = custom_scan(char_readable_scan_tag{}, s, format, val);
            if (!ret) {
                return nonstd::make_unexpected(ret.error());
            }
            return {};
        }
        static nonstd::expected<void, failure> scan(
            std::function<result(gsl::span<CharT>, streampos)>& s,
            streampos pos,
            CharT*& format,
            void* data)
        {
            T& val = *reinterpret_cast<T*>(data);
            auto ret = custom_scan(random_access_readable_scan_tag{}, s, pos,
                                   format, val);
            if (!ret) {
                return nonstd::make_unexpected(ret.error());
            }
            return {};
        }
    };

    template <typename CharT, typename T>
    struct do_scan<
        CharT,
        T,
        typename std::enable_if<
            std::is_same<typename std::decay<T>::type, CharT>::value>::type> {
        static nonstd::expected<void, failure> scan(
            std::function<result(gsl::span<CharT>)>& s,
            const CharT*& format,
            void* data)
        {
            T& ch = *reinterpret_cast<CharT*>(data);
            auto ret = s(gsl::make_span(std::addressof(ch), 1));
            skip_format(format);
            if (ret.value() == 1 && !ret.has_error()) {
                return {};
            }
            return nonstd::make_unexpected(ret.error());
        }
        static nonstd::expected<void, failure>
        scan(std::function<result(CharT&)>& s, const CharT*& format, void* data)
        {
            T& ch = *reinterpret_cast<CharT*>(data);
            auto ret = s(ch);
            skip_format(format);
            if (ret.value() == 1 && !ret.has_error()) {
                return {};
            }
            return nonstd::make_unexpected(ret.error());
        }
        static nonstd::expected<void, failure> scan(
            std::function<result(gsl::span<CharT>, streampos)>& s,
            streampos pos,
            const CharT*& format,
            void* data)
        {
            T& ch = *reinterpret_cast<CharT*>(data);
            auto ret = s(gsl::make_span(std::addressof(ch), 1), pos);
            skip_format(format);
            if (ret.value() == 1 && !ret.has_error()) {
                return {};
            }
            return nonstd::make_unexpected(ret.error());
        }
    };

    template <typename CharT, typename T>
    struct do_scan<
        CharT,
        gsl::span<T>,
        typename std::enable_if<
            std::is_same<typename std::decay<T>::type, CharT>::value>::type> {
        static nonstd::expected<void, failure> scan(
            std::function<result(gsl::span<CharT>)>& s,
            const CharT*& format,
            void* data)
        {
            auto val = *reinterpret_cast<gsl::span<T>*>(data);
            if (val.size() == 0) {
                return s;
            }

            const bool read_till_ws = *format == CharT('w');
            if (read_till_ws) {
                ++format;
            }


        }
    };
}  // namespace detail

SPIO_END_NAMESPACE

#endif  // SPIO_SCANNER_H
