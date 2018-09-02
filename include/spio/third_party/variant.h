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

#ifndef SPIO_THIRD_PARTY_VARIANT_H
#define SPIO_THIRD_PARTY_VARIANT_H

#include "../config.h"

#if SPIO_CLANG
#pragma clang diagnostic push
#endif

#include "nonstd/variant.hpp"

namespace spio {
SPIO_BEGIN_NAMESPACE
namespace nonstd = ::nonstd;

using ::nonstd::variant;
SPIO_END_NAMESPACE
}  // namespace spio

#if SPIO_CLANG
#pragma clang diagnostic pop
#endif

#endif  // SPIO_THIRD_PARTY_VARIANT_H
