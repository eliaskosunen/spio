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

namespace spio {
SPIO_BEGIN_NAMESPACE

template <typename T, typename E>
class basic_result {
public:
    using success_type = T;
    using error_type = E;
    using wrapped_error_type = optional<E>;

    SPIO_CONSTEXPR basic_result() = default;
    SPIO_CONSTEXPR basic_result(success_type ok) noexcept : m_ok(std::move(ok))
    {
    }
#if !SPIO_HAS_DEDUCTION_GUIDES
    // ambiguous if optional has deduction guides
    basic_result(success_type ok, error_type err)
        : m_ok(std::move(ok)), m_err(std::move(err))
    {
    }
#endif
    basic_result(success_type ok, wrapped_error_type err)
        : m_ok(std::move(ok)), m_err(std::move(err))
    {
    }

    SPIO_CONSTEXPR14 success_type& value() & noexcept
    {
        return m_ok;
    }
    SPIO_CONSTEXPR const success_type& value() const& noexcept
    {
        return m_ok;
    }
    SPIO_CONSTEXPR14 success_type&& value() && noexcept
    {
        return m_ok;
    }

    SPIO_CONSTEXPR bool has_error() const noexcept
    {
        return m_err.has_value();
    }

    SPIO_CONSTEXPR14 wrapped_error_type& inspect_error() & noexcept
    {
        return m_err;
    }
    SPIO_CONSTEXPR const wrapped_error_type& inspect_error() const& noexcept
    {
        return m_err;
    }

    SPIO_CONSTEXPR14 error_type& error() & noexcept
    {
        Expects(has_error());
        return *m_err;
    }
    SPIO_CONSTEXPR14 const error_type& error() const& noexcept
    {
        Expects(has_error());
        return *m_err;
    }
    SPIO_CONSTEXPR14 error_type&& error() && noexcept
    {
        Expects(has_error());
        return *std::move(m_err);
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
    wrapped_error_type m_err{nullopt};
};

using result = basic_result<streamsize, failure>;

inline result make_result(streamsize s, failure e) noexcept
{
    return result(s, std::move(e));
}

SPIO_END_NAMESPACE
}  // namespace spio

#endif  // SPIO_RESULT_H
