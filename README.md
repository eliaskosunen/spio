# spio

[![Build Status (Linux, macOS)](https://travis-ci.com/eliaskosunen/spio.svg)](https://travis-ci.com/eliaskosunen/spio)
[![Build Status (Windows)](https://ci.appveyor.com/api/projects/status/jejykd1ewk0mq4h5?svg=true)](https://ci.appveyor.com/project/eliaskosunen/spio)
[![Coverage Status](https://coveralls.io/repos/github/eliaskosunen/spio/badge.svg?branch=master)](https://coveralls.io/github/eliaskosunen/spio?branch=master)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/283acc16d6a942429df56eb69e2b51f2)](https://www.codacy.com/app/eliaskosunen/spio?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=eliaskosunen/spio&amp;utm_campaign=Badge_Grade)
![C++ Standard](https://img.shields.io/badge/C%2B%2B-11%2F14%2F17-blue.svg)
![License](https://img.shields.io/github/license/eliaskosunen/spio.svg)

C++ IO for the 21st century

## About

Standard IO facilities provided by C++ are very bad.
There's `<cstdio>` inherited from C, that doesn't provide type safety and user extensibility, and has an unintuitive mini-language built into it.
Then there's the more "modern" alternative: `<iostream>`, which is too slow for any real-world use and has an hard-to-use API.

This library attempts to be a new high-level IO library to explore different design choices with C++ from this decade and provide ideas
and tried-out designs for people smarter than me for standardization.

## Hello world example

```cpp
#include <spio/spio.h>

int main()
{
    // Line-buffered stream to stdout
    spio::stdio_handle_outstream out(stdout, spio::buffer_mode::line);
    // Write with fmtlib
    spio::print(out, "Hello {}!\n", "world");
    // Prints "Hello world!"
}
```

## License

spio is licensed under the Apache License 2.0. See LICENSE for details.
