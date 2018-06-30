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

#ifndef SPIO_RESULT_H
#define SPIO_RESULT_H

#include "config.h"

#include "device.h"
#include "error.h"
#include "third_party/optional.h"

SPIO_BEGIN_NAMESPACE

template <typename T, typename E>
class basic_result {
public:
    using success_type = T;
    using error_type = E;
    using wrapped_error_type = nonstd::optional<E>;

    basic_result() = default;
    basic_result(success_type ok) : m_ok(std::move(ok)) {}
#if !SPIO_HAS_DEDUCTION_GUIDES
    // ambiguous if std::optional has deduction guides
    basic_result(success_type ok, error_type err)
        : m_ok(std::move(ok)), m_err(std::move(err))
    {
    }
#endif
    basic_result(success_type ok, wrapped_error_type err)
        : m_ok(std::move(ok)), m_err(std::move(err))
    {
    }

    success_type& value()
    {
        return m_ok;
    }
    const success_type& value() const
    {
        return m_ok;
    }

    bool has_error() const
    {
        return m_err.has_value();
    }
    wrapped_error_type& inspect_error() noexcept
    {
        return m_err;
    }
    const wrapped_error_type& inspect_error() const noexcept
    {
        return m_err;
    }

    error_type& error()
    {
        Expects(has_error());
        return *m_err;
    }
    const error_type& error() const
    {
        Expects(has_error());
        return *m_err;
    }

    template <typename T_,
              typename E_,
              typename Other = basic_result<T, E>,
              typename std::enable_if<
                  std::is_convertible<success_type,
                                      typename Other::success_type>::value &&
                  std::is_convertible<wrapped_error_type,
                                      typename Other::wrapped_error_type>::
                      value>::type = nullptr>
    operator basic_result<T_, E_>()
    {
        return Other(m_ok, m_err);
    }

private:
    success_type m_ok{};
    wrapped_error_type m_err{nonstd::nullopt};
};

using result = basic_result<streamsize, failure>;

inline result make_result(streamsize s, failure e)
{
    return result(s, std::move(e));
}

SPIO_END_NAMESPACE

#endif  // SPIO_RESULT_H
