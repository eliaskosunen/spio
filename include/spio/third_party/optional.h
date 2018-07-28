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

#ifndef SPIO_THIRD_PARTY_OPTIONAL_H
#define SPIO_THIRD_PARTY_OPTIONAL_H

#include "../config.h"

#if SPIO_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wundef"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Weffc++"
#endif

#include "nonstd/optional.hpp"

SPIO_BEGIN_NAMESPACE

namespace nonstd = ::nonstd;

SPIO_END_NAMESPACE

#if SPIO_GCC
#pragma GCC diagnostic pop
#endif

#endif  // SPIO_THIRD_PARTY_OPTIONAL_H
