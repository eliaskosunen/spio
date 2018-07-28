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
