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

#ifndef SPIO_STREAM_BASE_H
#define SPIO_STREAM_BASE_H

#include "config.h"

#include <vector>
#include "device.h"

SPIO_BEGIN_NAMESPACE

class stream_base {
public:
    stream_base(const stream_base&) = delete;
    stream_base& operator=(const stream_base&) = delete;
    stream_base(stream_base&&) = default;
    stream_base& operator=(stream_base&&) = default;

    virtual ~stream_base() = default;

    bool bad() const noexcept
    {
        return m_flags[0];
    }
    bool eof() const noexcept
    {
        return m_flags[1];
    }
    virtual explicit operator bool() const noexcept
    {
        return bad();
    }
    bool operator!() const noexcept
    {
        return !(operator bool());
    }

    void set_bad()
    {
        m_flags[0] = true;
    }
    void clear_bad()
    {
        m_flags[0] = false;
    }

    void set_eof()
    {
        m_flags[1] = true;
    }
    void clear_eof()
    {
        m_flags[1] = false;
    }

protected:
    stream_base() = default;

private:
    static std::vector<bool> _flags_init()
    {
        return {false, false};
    }

    std::vector<bool> m_flags{_flags_init()};  // Intentional vector<bool>
};

SPIO_END_NAMESPACE

#endif  // SPIO_STREAM_BASE_H
