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

#include <benchmark/benchmark.h>
#include <spio/spio.h>
#include <functional>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <string>

static std::vector<std::string> generate_data(size_t len)
{
    const std::vector<char> chars = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',  'A',  'B',
        'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',  'M',  'N',
        'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',  'Y',  'Z',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',  'k',  'l',
        'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',  'w',  'x',
        'y', 'z', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '\n', '\n', '\t'};
    std::default_random_engine rng(std::random_device{}());
    std::uniform_int_distribution<> dist(0, static_cast<int>(chars.size() - 1));

    std::vector<std::string> data;
    data.emplace_back();
    for (std::size_t i = 0; i < len; ++i) {
        auto c = chars[static_cast<size_t>(dist(rng))];
        if (std::isspace(c)) {
            data.emplace_back();
        }
        else {
            data.back().push_back(c);
        }
    }
    return data;
}

static void print_spio_write_device(benchmark::State& state)
{
    size_t bytes = 0;
    for (auto _ : state) {
        state.PauseTiming();
        auto data = generate_data(static_cast<size_t>(state.range(0)));
        std::vector<gsl::byte> str;
        spio::vector_sink sink{str};
        state.ResumeTiming();

        for (auto& n : data) {
            sink.write(gsl::as_bytes(
                gsl::make_span(&n[0], static_cast<int64_t>(n.size()))));
            bytes += n.length();
            benchmark::DoNotOptimize(str);
            benchmark::DoNotOptimize(sink);
        }
    }
    state.SetBytesProcessed(static_cast<int64_t>(bytes));
}
static void print_spio_write_stream(benchmark::State& state)
{
    size_t bytes = 0;
    for (auto _ : state) {
        state.PauseTiming();
        auto data = generate_data(static_cast<size_t>(state.range(0)));
        std::vector<gsl::byte> str;

        spio::vector_sink sink{str};
        using stream_type =
            spio::stream<spio::vector_sink, spio::encoding<char>,
                         spio::sink_filter_chain>;
        stream_type s(sink, stream_type::input_base{},
                      stream_type::output_base{}, stream_type::chain_type{});
        s.sink_storage() = nonstd::make_optional(
            stream_type::sink_type(sink, spio::buffer_mode::none));
        spio::basic_stream_ref<spio::encoding<char>, spio::writable_tag> ref(s);
        state.ResumeTiming();

        for (auto& n : data) {
            spio::write(s, gsl::as_bytes(gsl::make_span(
                               &n[0], static_cast<int64_t>(n.size()))));
            bytes += n.length();
            benchmark::DoNotOptimize(str);
            benchmark::DoNotOptimize(s);
            benchmark::DoNotOptimize(sink);
        }
    }
    state.SetBytesProcessed(static_cast<int64_t>(bytes));
}
static void print_spio_write_stream_ref(benchmark::State& state)
{
    size_t bytes = 0;
    for (auto _ : state) {
        state.PauseTiming();
        auto data = generate_data(static_cast<size_t>(state.range(0)));
        std::vector<gsl::byte> str;

        spio::vector_sink sink{str};
        using stream_type =
            spio::stream<spio::vector_sink, spio::encoding<char>,
                         spio::sink_filter_chain>;
        stream_type s(sink, stream_type::input_base{},
                      stream_type::output_base{}, stream_type::chain_type{});
        s.sink_storage() = nonstd::make_optional(
            stream_type::sink_type(sink, spio::buffer_mode::none));
        spio::basic_stream_ref<spio::encoding<char>, spio::writable_tag> ref(s);
        state.ResumeTiming();

        for (auto& n : data) {
            spio::write(ref, gsl::as_bytes(gsl::make_span(
                                 &n[0], static_cast<int64_t>(n.size()))));
            bytes += n.length();
            benchmark::DoNotOptimize(str);
            benchmark::DoNotOptimize(s);
            benchmark::DoNotOptimize(sink);
        }
    }
    state.SetBytesProcessed(static_cast<int64_t>(bytes));
}
static void print_spio_stream(benchmark::State& state)
{
    size_t bytes = 0;
    for (auto _ : state) {
        state.PauseTiming();
        auto data = generate_data(static_cast<size_t>(state.range(0)));
        std::vector<gsl::byte> str;

        spio::vector_sink sink{str};
        using stream_type =
            spio::stream<spio::vector_sink, spio::encoding<char>,
                         spio::sink_filter_chain>;
        stream_type s(sink, stream_type::input_base{},
                      stream_type::output_base{}, stream_type::chain_type{});
        s.sink_storage() = nonstd::make_optional(
            stream_type::sink_type(sink, spio::buffer_mode::none));
        state.ResumeTiming();

        for (auto& n : data) {
            spio::print(s, "{}", n);
            bytes += n.length();
            benchmark::DoNotOptimize(str);
            benchmark::DoNotOptimize(s);
            benchmark::DoNotOptimize(sink);
        }
    }
    state.SetBytesProcessed(static_cast<int64_t>(bytes));
}
static void print_spio_stream_ref(benchmark::State& state)
{
    size_t bytes = 0;
    for (auto _ : state) {
        state.PauseTiming();
        auto data = generate_data(static_cast<size_t>(state.range(0)));
        std::vector<gsl::byte> str;

        spio::vector_sink sink{str};
        using stream_type =
            spio::stream<spio::vector_sink, spio::encoding<char>,
                         spio::sink_filter_chain>;
        stream_type s(sink, stream_type::input_base{},
                      stream_type::output_base{}, stream_type::chain_type{});
        s.sink_storage() = nonstd::make_optional(
            stream_type::sink_type(sink, spio::buffer_mode::none));
        spio::basic_stream_ref<spio::encoding<char>, spio::writable_tag> ref(s);
        state.ResumeTiming();

        for (auto& n : data) {
            spio::print(ref, "{}", n);
            bytes += n.length();
            benchmark::DoNotOptimize(str);
            benchmark::DoNotOptimize(s);
            benchmark::DoNotOptimize(sink);
        }
    }
    state.SetBytesProcessed(static_cast<int64_t>(bytes));
}
static void print_fmt(benchmark::State& state)
{
    size_t bytes = 0;
    for (auto _ : state) {
        state.PauseTiming();
        auto data = generate_data(static_cast<size_t>(state.range(0)));
        std::vector<char> str;
        state.ResumeTiming();

        for (auto& n : data) {
            fmt::format_to(std::back_inserter(str), "{}", n);
            bytes += n.length();
            benchmark::DoNotOptimize(str);
        }
    }
    state.SetBytesProcessed(static_cast<int64_t>(bytes));
}
static void print_insert(benchmark::State& state)
{
    size_t bytes = 0;
    for (auto _ : state) {
        state.PauseTiming();
        auto data = generate_data(static_cast<size_t>(state.range(0)));
        std::vector<char> str;
        state.ResumeTiming();

        for (auto& n : data) {
            str.insert(str.end(), n.begin(), n.end());
            bytes += n.length();
            benchmark::DoNotOptimize(str);
        }
    }
    state.SetBytesProcessed(static_cast<int64_t>(bytes));
}
static void print_stringstream(benchmark::State& state)
{
    size_t bytes = 0;
    for (auto _ : state) {
        state.PauseTiming();
        auto data = generate_data(static_cast<size_t>(state.range(0)));
        std::ostringstream ss;
        state.ResumeTiming();

        for (auto& n : data) {
            ss << n;
            bytes += n.length();
            benchmark::DoNotOptimize(ss);
        }
        benchmark::DoNotOptimize(ss.str());
    }
    state.SetBytesProcessed(static_cast<int64_t>(bytes));
}

BENCHMARK(print_spio_write_device)->Range(8, 8 << 8);
BENCHMARK(print_spio_write_stream)->Range(8, 8 << 8);
BENCHMARK(print_spio_write_stream_ref)->Range(8, 8 << 8);
BENCHMARK(print_spio_stream)->Range(8, 8 << 8);
BENCHMARK(print_spio_stream_ref)->Range(8, 8 << 8);
BENCHMARK(print_fmt)->Range(8, 8 << 8);
BENCHMARK(print_insert)->Range(8, 8 << 8);
BENCHMARK(print_stringstream)->Range(8, 8 << 8);
