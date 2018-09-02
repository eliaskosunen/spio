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

#ifndef SPIO_SPIO_H
#define SPIO_SPIO_H

#include "config.h"

#include "ring.h"
#include "string_view.h"
#include "util.h"

#include "container_device.h"
#include "memory_device.h"
#include "stdio_device.h"

#if SPIO_USE_LLFIO
#include "llfio_device.h"
#endif

#include "sink.h"
#include "source.h"

#include "device_stream.h"
#include "filter.h"
#include "formatter.h"
#include "scanner.h"
#include "stream.h"
#include "stream_base.h"
#include "stream_operations.h"
#include "stream_ref.h"

#endif  // SPIO_SPIO_H
