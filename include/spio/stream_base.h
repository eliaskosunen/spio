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

namespace spio {
SPIO_BEGIN_NAMESPACE

class stream_base {
public:
    stream_base(const stream_base&) = delete;
    stream_base& operator=(const stream_base&) = delete;
    stream_base(stream_base&&) noexcept = default;
    stream_base& operator=(stream_base&&) noexcept = default;

    virtual ~stream_base() = default;

    bool bad() const noexcept
    {
        return m_flags[0];
    }
    bool eof() const noexcept
    {
        return m_flags[1];
    }
    virtual operator bool() const noexcept
    {
        return !bad();
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

struct any_tag {
};

struct sink_tag : virtual any_tag {
};
struct writable_tag : sink_tag {
};
struct random_access_writable_tag : sink_tag {
};
struct byte_writable_tag : sink_tag {
};

struct flushable_tag : virtual any_tag {
};
struct syncable_tag : virtual any_tag {
};

struct source_tag : virtual any_tag {
};
struct readable_tag : source_tag {
};
struct random_access_readable_tag : source_tag {
};
struct byte_readable_tag : source_tag {
};

struct putbackable_span_tag : virtual any_tag {
};
struct putbackable_byte_tag : virtual any_tag {
};

struct absolute_seekable_tag : virtual any_tag {
};
struct relative_seekable_tag : virtual any_tag {
};
struct seekable_tag : absolute_seekable_tag, relative_seekable_tag {
};
struct tellable_tag : virtual any_tag {
};

template <typename... Bases>
struct make_tag : virtual Bases... {
};

template <typename Stream>
using is_writable_stream = is_writable<typename Stream::device_type>;
template <typename Stream>
using is_random_access_writable_stream =
    is_random_access_writable<typename Stream::device_type>;
template <typename Stream>
using is_byte_writable_stream = is_byte_writable<typename Stream::device_type>;

template <typename Stream>
using is_flushable_stream = is_writable<typename Stream::device_type>;
template <typename Stream>
using is_syncable_stream = is_syncable<typename Stream::device_type>;

template <typename Stream>
using is_readable_stream = is_readable<typename Stream::device_type>;
template <typename Stream>
using is_random_access_readable_stream =
    is_random_access_readable<typename Stream::device_type>;
template <typename Stream>
using is_byte_readable_stream = is_byte_readable<typename Stream::device_type>;

template <typename Stream>
using is_putbackable_span_stream = is_readable_stream<Stream>;
template <typename Stream>
using is_putbackable_byte_stream =
    conjunction<is_byte_readable_stream<Stream>,
                negation<is_putbackable_span_stream<Stream>>>;

template <typename Stream>
using is_absolute_seekable_stream = is_detected<absolute_seekable_op, Stream>;
template <typename Stream>
using is_relative_seekable_stream = is_detected<relative_seekable_op, Stream>;

template <typename Stream>
using is_tellable_stream = is_seekable<typename Stream::device_type>;

SPIO_END_NAMESPACE
}  // namespace spio

#endif  // SPIO_STREAM_BASE_H
