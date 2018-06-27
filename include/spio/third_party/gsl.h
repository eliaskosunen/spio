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

#ifndef SPIO_THIRD_PARTY_GSL_H
#define SPIO_THIRD_PARTY_GSL_H

#include "../config.h"

#if SPIO_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#endif

#if SPIO_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-template"
#pragma clang diagnostic ignored "-Wsign-conversion"
#endif

#define gsl_CONFIG_SPAN_INDEX_TYPE std::ptrdiff_t
#define gsl_CONFIG_DEPRECATE_TO_LEVEL 5

#include "gsl.hpp"

SPIO_BEGIN_NAMESPACE

namespace gsl = ::gsl;

SPIO_END_NAMESPACE

#if SPIO_CLANG
#pragma clang diagnostic pop
#endif

#if SPIO_GCC
#pragma GCC diagnostic pop
#endif

#endif  // SPIO_THIRD_PARTY_GSL_H
