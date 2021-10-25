# KTL
Kernel Template Library is open-source library providing CRT environment, STL-style containers and RAII tools for Windows Kernel programming.

* [Features](#features)
  * [C Runtime environment](#build-requirements)
  * [C++ Standard Library implementation](#C++-standard-library-implementation)
  * [CMake](#cmake)
* [Installation & Usage](#installation-&-Usage)
  * [CMakeLists.txt to build with a pre-built KTL](#cmakelists.txt-to-build-with-a-pre-built-ktl)
  * [Build requirements](#build-requirements)
* [Examples](#samples)
* [Roadmap for the near future](#roadmap-for-the-near-future)
* [License](#license)


# Features

* C Runtime environment
    * **Exception handling mechanism** (now for x64 only)
    * Buffer security checks
    * C++ Standard compatible termination if execution is run out of control 
    * Construction and destruction of the non-trivial static objects
    * Memory allocation using new and delete
    * Filesystem Mini-Filter support routines


* C++ Standard Library implementation
    * `<atomic>` (now for x86 and x64 only)
    * Optimized, C++ Standard compatible `<algorithm>` library
    * `<allocator>` with standard allocators for different pool types
    * Boost-based implementation of the `compressed_pair`
    * Exceptions objects hierarchy (`std::exception` analog optimized for use in the kernel)
    * Iterators
    * MSVC-intrinsic-based coroutines
    * Mutexes, events and condition variables based on kernel synchronization primitives with RAII wrappers
    * Smart pointers (`unique_ptr`, `shared_ptr` and `weak_ptr`, `intrusive_ptr`)
    * `<type_traits>`
    * `<thread>` for managing driver-dedicated threads
    * `<tuple>`
    * `<optional>` with constexpr support
    * `unordered_node_map`, `unordered_node_set`, `unordered_flat_map` and `unordered_flat_set` using [robin-hood-hashing](https://github.com/martinus/robin-hood-hashing)
    * `<vector>`
    * Lock-free queue, `node_allocator` and some auxiliary algorithms 
    * [fmt](https://github.com/fmtlib/fmt/) as a string formatting library 
    * Designed in C++17, feel free to build with C++20


* CMake
    * Building of kernel drivers
    * Generating a test-signing certificate
    * Driver signing 

## Header Overview

|             **Header**              |       **Library**        |     **Status**     |                                       **Implementation Progress (Spreadsheet)**                                        |
| :---------------------------------: | :----------------------: | :----------------: | :--------------------------------------------------------------------------------------------------------------------: |
|       [algorithm](#algorithm)       |        Algorithms        | :heavy_check_mark: |  [algorithm](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1451123716)  |
|                 any                 |         Utility          |        :x:         |                                                                                                                        |
|           [array](#array)           |        Containers        | :heavy_check_mark: |    [array](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1320059600)    |
|               atomic                |          Atomic          |        :x:         |                                                                                                                        |
|               barrier               |          Thread          |        :x:         |                                                                                                                        |
|             [bit](#bit)             |         Numeric          | :heavy_check_mark: |     [bit](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1927645890)     |
|          [bitset](#bitset)          |         Utility          | :heavy_check_mark: |    [bitset](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=692946382)    |
|         [cassert](#cassert)         | Utility / Error Handling | :heavy_check_mark: |   [cassert](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=460740183)    |
|          [cctype](#cctype)          |         Strings          | :heavy_check_mark: |    [cctype](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=522168028)    |
|               cerrno                | Utility / Error Handling |        :x:         |                                                                                                                        |
|                cfenv                |         Numeric          |        :x:         |                                                          TODO                                                          |
|          [cfloat](#cfloat)          | Utility / Numeric Limits | :heavy_check_mark: |   [cfloat](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1012838019)    |
|        [charconv](#charconv)        |         Strings          | :heavy_check_mark: |   [charconv](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=345887816)   |
|          [chrono](#chrono)          |         Utility          | :heavy_check_mark: |   [chrono](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1279150724)    |
|              cinttypes              | Utility / Numeric Limits |        :x:         |                                                          TODO                                                          |
|         [climits](#climits)         | Utility / Numeric Limits | :heavy_check_mark: |   [climits](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1904156895)   |
|               clocale               |       Localization       |        :x:         |                                                                                                                        |
|           [cmath](#cmath)           |         Numeric          | :heavy_check_mark: |    [cmath](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=868070087)     |
|               compare               |         Utility          |        :x:         |                                                          TODO                                                          |
|         [complex](#complex)         |         Numeric          | :heavy_check_mark: |   [complex](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1768885550)   |
|        [concepts](#concepts)        |         Concepts         | :heavy_check_mark: |   [concepts](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=73781271)    |
|         condition_variable          |          Thread          |        :x:         |                                                                                                                        |
|              coroutine              |        Coroutines        |        :x:         |                                                                                                                        |
|               csetjmp               |         Utility          |        :x:         |                                                                                                                        |
|               csignal               |         Utility          |        :x:         |                                                                                                                        |
|         [cstdarg](#cstdarg)         |         Utility          | :heavy_check_mark: |   [cstdarg](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1280782172)   |
|         [cstddef](#cstddef)         |         Utility          | :heavy_check_mark: |   [cstddef](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1660546405)   |
|         [cstdint](#cstdint)         | Utility / Numeric Limits | :heavy_check_mark: |   [cstdint](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=2005735528)   |
|          [cstdio](#cstdio)          |       Input/Output       | :heavy_check_mark: |   [cstdio](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1576270107)    |
|         [cstdlib](#cstdlib)         |         Utility          | :heavy_check_mark: |   [cstdlib](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1705155517)   |
|         [cstring](#cstring)         |         Strings          | :heavy_check_mark: |   [cstring](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1824871501)   |
|           [ctime](#ctime)           |         Utility          | :heavy_check_mark: |    [ctime](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1082109762)    |
|               cuchar                |         Strings          |        :x:         |                                                                                                                        |
|          [cwchar](#cwchar)          |         Strings          |        :x:         |                                                                                                                        |
|               cwctype               |         Strings          | :heavy_check_mark: |   [cwchar](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1105944467)    |
|                deque                |        Containers        |        :x:         |                                                          TODO                                                          |
|              exception              | Utility / Error Handling |        :x:         |                                                                                                                        |
|              execution              |        Algorithms        |        :x:         |                                                                                                                        |
|        [expected](#expected)        | Utility / Error Handling | :heavy_check_mark: |  [expected](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1624993362)   |
|             filesystem              |        Filesystem        |        :x:         |                                                                                                                        |
|          [format](#format)          |         Strings          | :heavy_check_mark: |    [format](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=159875067)    |
|            forward_list             |        Containers        |        :x:         |                                                                                                                        |
|      [functional](#functional)      |         Utility          | :heavy_check_mark: |  [functional](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=291953395)  |
|               future                |          Thread          |        :x:         |                                                                                                                        |
|               fstream               |       Input/Output       |        :x:         |                                                                                                                        |
|              ifstream               |       Input/Output       |        :x:         |                                                                                                                        |
|          initializer_list           |         Utility          |        :x:         |                                                                                                                        |
|               iomanip               |       Input/Output       |        :x:         |                                                                                                                        |
|             [ios](#ios)             |       Input/Output       | :heavy_check_mark: |     [ios](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=2084657878)     |
|               iosfwd                |       Input/Output       |        :x:         |                                                                                                                        |
|              iostream               |       Input/Output       |        :x:         |                                                                                                                        |
|        [iterator](#iterator)        |         Iterator         | :heavy_check_mark: |  [iterator](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=2084657878)   |
|               istream               |       Input/Output       |        :x:         |                                                                                                                        |
|                latch                |          Thread          |        :x:         |                                                                                                                        |
|          [limits](#limits)          | Utility / Numeric Limits | :heavy_check_mark: |   [limits](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=2084657878)    |
|                list                 |        Containers        |        :x:         |                                                                                                                        |
|               locale                |       Localization       |        :x:         |                                                                                                                        |
|             [map](#map)             |        Containers        | :heavy_check_mark: |     [map](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=2084657878)     |
|          [memory](#memory)          | Utility / Dynamic Memory | :heavy_check_mark: |   [memory](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=2084657878)    |
|           memory_resource           | Utility / Dynamic Memory |        :x:         |                                                                                                                        |
|           [mutex](#mutex)           |          Thread          | :heavy_check_mark: |    [mutex](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=2084657878)    |
|             [new](#new)             | Utility / Dynamic Memory | :heavy_check_mark: |     [new](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=2084657878)     |
|         [numbers](#numbers)         |         Numeric          | :heavy_check_mark: |   [numbers](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=641824361)    |
|         [numeric](#numeric)         |         Numeric          | :heavy_check_mark: |   [numeric](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1599843301)   |
|        [optional](#optional)        |         Utility          | :heavy_check_mark: |  [optional](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1965816070)   |
|               ostream               |       Input/Output       |        :x:         |                                                                                                                        |
|                queue                |        Containers        |        :x:         |                                                          TODO                                                          |
|               random                |         Numeric          |        :x:         |                                                                                                                        |
|               ranges                |          Ranges          |        :x:         |                                                          TODO                                                          |
|                regex                |   Regular Expressions    |        :x:         |                                                                                                                        |
|           [ratio](#ratio)           |         Numeric          | :heavy_check_mark: |    [ratio](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1383686309)    |
|          scoped_allocator           | Utility / Dynamic Memory |        :x:         |                                                                                                                        |
|           [scope](#scope)           |         Utility          | :heavy_check_mark: |                                                                                                                        |
|              semaphore              |          Thread          |        :x:         |                                                                                                                        |
| [source_location](#source_location) |         Utility          | :heavy_check_mark: |                                                                                                                        |
|             [set](#set)             |        Containers        | :heavy_check_mark: |     [set](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=930086747)      |
|            shared_mutex             |          Thread          |        :x:         |                                                                                                                        |
|            [span](#span)            |        Containers        | :heavy_check_mark: |    [span](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1750377555)     |
|           [stack](#stack)           |        Containers        | :heavy_check_mark: |    [stack](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=385809287)     |
|             stack_trace             |         Utility          |        :x:         |                                                                                                                        |
|              stdexcept              | Utility / Error Handling |        :x:         |                                                                                                                        |
|              streambuf              |       Input/Output       |        :x:         |                                                                                                                        |
|          [string](#string)          |         Strings          | :heavy_check_mark: |    [string](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=43463000)     |
|     [string_view](#string_view)     |         Strings          | :heavy_check_mark: | [string_view](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1803550736) |
|             stop_token              |          Thread          |        :x:         |                                                                                                                        |
|               sstream               |       Input/Output       |        :x:         |                                                                                                                        |
|    [system_error](#system_error)    | Utility / Error Handling | :heavy_check_mark: | [system_error](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=635426347) |
|             sync_stream             |       Input/Output       |        :x:         |                                                                                                                        |
|               thread                |          Thread          |        :x:         |                                                                                                                        |
|           [tuple](#tuple)           |         Utility          | :heavy_check_mark: |    [tuple](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=857929646)     |
|             type_index              |         Utility          |        :x:         |                                                                                                                        |
|              type_info              |         Utility          |        :x:         |                                                                                                                        |
|     [type_traits](#type_traits)     |         Utility          | :heavy_check_mark: | [type_traits](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1691010448) |
|            unordered_map            |        Containers        |        :x:         |                                                          TODO                                                          |
|            unordered_set            |        Containers        |        :x:         |                                                          TODO                                                          |
|         [utility](#utility)         |         Utility          | :heavy_check_mark: |   [utility](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1484976254)   |
|              valarray               |         Numeric          |        :x:         |                                                                                                                        |
|         [variant](#variant)         |         Utility          | :heavy_check_mark: |   [variant](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=503059518)    |
|          [vector](#vector)          |        Containers        | :heavy_check_mark: |   [vector](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1613833122)    |
|         [version](#version)         |         Utility          | :heavy_check_mark: |                                                                                                                        |
|         [warning](#warning)         |         Utility          | :heavy_check_mark: |                                                     Not standard.                                                      |

## Header Detail

### algorithm

- **Library:** Algorithms
- **Include:** [`etl/algorithm.hpp`](./etl/algorithm.hpp)
- **Tests:** [test_algorithm.cpp](./tests/test_algorithm.cpp)
- **Example:** [algorithm.cpp](./examples/algorithm.cpp)
- **Implementation Progress:** [algorithm](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1451123716)
- **Changes:**
  - Implementations are optimize for code size. See [etl::search vs. libstdc++ (godbolt.org)](https://godbolt.org/z/dY9zPf8cs) as an example.
  - All overloads using an execution policy are not implemented.

### array

- **Library:** Containers
- **Include:** [`etl/array.hpp`](./etl/array.hpp)
- **Tests:** [test_array.cpp](./tests/test_array.cpp)
- **Example:** [array.cpp](./examples/array.cpp)
- **Implementation Progress:** [array](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1320059600)
- **Changes:**
  - None

### bit

- **Library:** Numeric
- **Include:** [`etl/bit.hpp`](./etl/bit.hpp)
- **Tests:** [test_bit.cpp](./tests/test_bit.cpp)
- **Example:** [bit.cpp](./examples/bit.cpp)
- **Implementation Progress:** [bit](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1927645890)
- **Changes:**
  - None

### bitset

- **Library:** Utility
- **Include:** [`etl/bitset.hpp`](./etl/bitset.hpp)
- **Tests:** [test_bitset.cpp](./tests/test_bitset.cpp)
- **Example:** [bitset.cpp](./examples/bitset.cpp)
- **Implementation Progress:** [bitset](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=692946382)
- **Changes:**
  - TODO

### cassert

- **Library:** Utility / Error Handling
- **Include:** [`etl/cassert.hpp`](./etl/cassert.hpp)
- **Tests:** [test_cassert.cpp](./tests/test_cassert.cpp)
- **Example:** [cassert.cpp](./examples/cassert.cpp)
- **Implementation Progress:** [cassert](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=460740183)
- **Changes:**
  - Added custom assertion macro `TETL_ASSERT`. The behavoir can be customized. The macro get's called every time an exceptional case has occurred inside the library. See the example file for more details.

### cctype

- **Library:** Strings
- **Include:** [`etl/cctype.hpp`](./etl/cctype.hpp)
- **Tests:** [test_cctype.cpp](./tests/test_cctype.cpp)
- **Example:** TODO
- **Implementation Progress:** [cctype](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=522168028)
- **Changes:**
  - Locale independent

### cfloat

- **Library:** Utility / Numeric Limits
- **Include:** [`etl/cfloat.hpp`](./etl/cfloat.hpp)
- **Tests:** [test_cfloat.cpp](./tests/test_cfloat.cpp)
- **Example:** TODO
- **Implementation Progress:** [cfloat](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1012838019)
- **Changes:**
  - None

### charconv

- **Library:** Strings
- **Include:** [`etl/charconv.hpp`](./etl/charconv.hpp)
- **Tests:** [test_charconv.cpp](./tests/test_charconv.cpp)
- **Example:** TODO
- **Implementation Progress:** [charconv](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=345887816)
- **Changes:**
  - None

### chrono

- **Library:** Utility
- **Include:** [`etl/chrono.hpp`](./etl/chrono.hpp)
- **Tests:** [test_chrono.cpp](./tests/test_chrono.cpp)
- **Example:** [chrono.cpp](./examples/chrono.cpp)
- **Implementation Progress:** [chrono](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1279150724)
- **Changes:**
  - No clocks are implemented. You have to provide your own, which must at least meet the requirements of [Clock](https://en.cppreference.com/w/cpp/named_req/Clock).

### climits

- **Library:** Utility / Numeric Limits
- **Include:** [`etl/climits.hpp`](./etl/climits.hpp)
- **Tests:** [test_climits.cpp](./tests/test_climits.cpp)
- **Example:** TODO
- **Implementation Progress:** [climits](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1904156895)
- **Changes:**
  - None

### cmath

- **Library:** Numeric
- **Include:** [`etl/cmath.hpp`](./etl/cmath.hpp)
- **Tests:** [test_cmath.cpp](./tests/test_cmath.cpp)
- **Example:** TODO
- **Implementation Progress:** [cmath](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=868070087)
- **Changes:**
  - None

### complex

- **Library:** Numeric
- **Include:** [`etl/complex.hpp`](./etl/complex.hpp)
- **Tests:** [test_complex.cpp](./tests/test_complex.cpp)
- **Example:** TODO
- **Implementation Progress:** [complex](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1768885550)
- **Changes:**
  - None

### concepts

- **Library:** Concepts
- **Include:** [`etl/concepts.hpp`](./etl/concepts.hpp)
- **Tests:** [test_concepts.cpp](./tests/test_concepts.cpp)
- **Example:** TODO
- **Implementation Progress:** [concepts](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=73781271)
- **Changes:**
  - None

### cstdarg

- **Library:** Utility
- **Include:** [`etl/cstdarg.hpp`](./etl/cstdarg.hpp)
- **Tests:** [test_cstdarg.cpp](./tests/test_cstdarg.cpp)
- **Example:** TODO
- **Implementation Progress:** [cstdarg](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1280782172)
- **Changes:**
  - None

### cstddef

- **Library:** Utility
- **Include:** [`etl/cstddef.hpp`](./etl/cstddef.hpp)
- **Tests:** [test_cstddef.cpp](./tests/test_cstddef.cpp)
- **Example:** TODO
- **Implementation Progress:** [cstddef](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1660546405)
- **Changes:**
  - None

### cstdint

- **Library:** Utility / Numeric Limits
- **Include:** [`etl/cstdint.hpp`](./etl/cstdint.hpp)
- **Tests:** [test_cstdint.cpp](./tests/test_cstdint.cpp)
- **Example:** TODO
- **Implementation Progress:** [cstdint](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=2005735528)
- **Changes:**
  - None

### cstdio

- **Library:** Input/Output
- **Include:** [`etl/cstdio.hpp`](./etl/cstdio.hpp)
- **Tests:** [test_cstdio.cpp](./tests/test_cstdio.cpp)
- **Example:** TODO
- **Implementation Progress:** [cstdio](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1576270107)
- **Changes:**
  - TODO

### cstdlib

- **Library:** Utility
- **Include:** [`etl/cstdlib.hpp`](./etl/cstdlib.hpp)
- **Tests:** [test_cstdlib.cpp](./tests/test_cstdlib.cpp)
- **Example:** TODO
- **Implementation Progress:** [cstdlib](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1705155517)
- **Changes:**
  - None

### cstring

- **Library:** Strings
- **Include:** [`etl/cstring.hpp`](./etl/cstring.hpp)
- **Tests:** [test_cstring.cpp](./tests/test_cstring.cpp)
- **Example:** TODO
- **Implementation Progress:** [cstring](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1824871501)
- **Changes:**
  - TODO

### ctime

- **Library:** Utility
- **Include:** [`etl/ctime.hpp`](./etl/ctime.hpp)
- **Tests:** [test_ctime.cpp](./tests/test_ctime.cpp)
- **Example:** TODO
- **Implementation Progress:** [ctime](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1082109762)
- **Changes:**
  - TODO

### cwchar

- **Library:** Strings
- **Include:** [`etl/cwchar.hpp`](./etl/cwchar.hpp)
- **Tests:** [test_cwchar.cpp](./tests/test_cwchar.cpp)
- **Example:** TODO
- **Implementation Progress:** [cwchar](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1105944467)
- **Changes:**
  - TODO

### expected

- **Library:** Utility / Error Handling
- **Include:** [`etl/expected.hpp`](./etl/expected.hpp)
- **Tests:** [test_expected.cpp](./tests/test_expected.cpp)
- **Example:** TODO
- **Implementation Progress:** [expected](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1624993362)
- **Changes:**
  - TODO

### format

- **Library:** Strings
- **Include:** [`etl/format.hpp`](./etl/format.hpp)
- **Tests:** [test_format.cpp](./tests/test_format.cpp)
- **Example:** TODO
- **Implementation Progress:** [format](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=159875067)
- **Changes:**
  - WIP. Don't use.

### functional

- **Library:** Utility
- **Include:** [`etl/functional.hpp`](./etl/functional.hpp)
- **Tests:** [test_functional.cpp](./tests/test_functional.cpp)
- **Example:** TODO
- **Implementation Progress:** [functional](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=291953395)
- **Changes:**
  - TODO

### ios

- **Library:** Input/Output
- **Include:** [`etl/ios.hpp`](./etl/ios.hpp)
- **Tests:** [test_ios.cpp](./tests/test_ios.cpp)
- **Example:** TODO
- **Implementation Progress:** [ios](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=2084657878)
- **Changes:**
  - TODO

### iterator

- **Library:** Iterator
- **Include:** [`etl/iterator.hpp`](./etl/iterator.hpp)
- **Tests:** [test_iterator.cpp](./tests/test_iterator.cpp)
- **Example:** TODO
- **Implementation Progress:** [iterator](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1722716093)
- **Changes:**
  - TODO

### limits

- **Library:** Utility / Numeric Limits
- **Include:** [`etl/limits.hpp`](./etl/limits.hpp)
- **Tests:** [test_limits.cpp](./tests/test_limits.cpp)
- **Example:** TODO
- **Implementation Progress:** [limits](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1966217100)
- **Changes:**
  - None

### map

- **Library:** Containers
- **Include:** [`etl/map.hpp`](./etl/map.hpp)
- **Tests:** [test_map.cpp](./tests/test_map.cpp)
- **Example:** [map.cpp](./examples/map.cpp)
- **Implementation Progress:** [map](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1845210258)
- **Changes:**
  - Renamed `map` to `static_map`. Fixed compile-time capacity.

### memory

- **Library:** Utility / Dynamic Memory
- **Include:** [`etl/memory.hpp`](./etl/memory.hpp)
- **Tests:** [test_memory.cpp](./tests/test_memory.cpp)
- **Example:** [memory.cpp](./examples/memory.cpp)
- **Implementation Progress:** [memory](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1321444012)
- **Changes:**
  - Non-standard class templates `small_ptr` (compressed pointer) & `pointer_int_pair` (pointer + integer) are provided.

### mutex

- **Library:** Thread
- **Include:** [`etl/mutex.hpp`](./etl/mutex.hpp)
- **Tests:** [test_mutex.cpp](./tests/test_mutex.cpp)
- **Example:** [mutex.cpp](./examples/mutex.cpp)
- **Implementation Progress:** [mutex](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=965791558)
- **Changes:**
  - Only RAII lock types are implemented. You have to provide a mutex type that at least meets the [BasicLockable](https://en.cppreference.com/w/cpp/named_req/BasicLockable) requirements.

### new

- **Library:** Utility / Dynamic Memory
- **Include:** [`etl/new.hpp`](./etl/new.hpp)
- **Tests:** [test_new.cpp](./tests/test_new.cpp)
- **Example:** TODO
- **Implementation Progress:** [new](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1146466573)
- **Changes:**
  - None
  - If the standard `<new>` is availble it is used to define the global placement new functions to avoid ODR violations when mixing `std` & `etl` headers.

### numbers

- **Library:** Numeric
- **Include:** [`etl/numbers.hpp`](./etl/numbers.hpp)
- **Tests:** [test_numbers.cpp](./tests/test_numbers.cpp)
- **Example:** TODO
- **Implementation Progress:** [numbers](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=641824361)
- **Changes:**
  - None

### numeric

- **Library:** Numeric
- **Include:** [`etl/numeric.hpp`](./etl/numeric.hpp)
- **Tests:** [test_numeric.cpp](./tests/test_numeric.cpp)
- **Example:** [numeric.cpp](./examples/numeric.cpp)
- **Implementation Progress:** [numeric](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1599843301)
- **Changes:**
  - Implementations are optimize for code size. See [etl::search vs. libstdc++ (godbolt.org)](https://godbolt.org/z/dY9zPf8cs) as an example.
  - All overloads using an execution policy are not implemented.

### optional

- **Library:** Utility
- **Include:** [`etl/optional.hpp`](./etl/optional.hpp)
- **Tests:** [test_optional.cpp](./tests/test_optional.cpp)
- **Example:** [optional.cpp](./examples/optional.cpp)
- **Implementation Progress:** [optional](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1965816070)
- **Changes:**
  - TODO

### ratio

- **Library:** Numeric
- **Include:** [`etl/ratio.hpp`](./etl/ratio.hpp)
- **Tests:** [test_ratio.cpp](./tests/test_ratio.cpp)
- **Example:** [ratio.cpp](./examples/ratio.cpp)
- **Implementation Progress:** [ratio](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1383686309)
- **Changes:**
  - None

### scope

- **Library:** Utility
- **Include:** [`etl/scope.hpp`](./etl/scope.hpp)
- **Tests:** [test_scope.cpp](./tests/test_scope.cpp)
- **Example:** TODO
- **Implementation Progress:** TODO
- **Reference:** [en.cppreference.com/w/cpp/experimental/scope_exit](https://en.cppreference.com/w/cpp/experimental/scope_exit)
- **Changes:**
  - Based on [p0052r8](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0052r8.pdf)
  - Only provides `scope_exit`

### source_location

- **Library:** Utility
- **Include:** [`etl/source_location.hpp`](./etl/source_location.hpp)
- **Tests:** TODO
- **Example:** [source_location.cpp](./examples/source_location.cpp)
- **Implementation Progress:** TODO
- **Changes:**
  - None

### set

- **Library:** Containers
- **Include:** [`etl/set.hpp`](./etl/set.hpp)
- **Tests:** [test_set.cpp](./tests/test_set.cpp)
- **Example:** [set.cpp](./examples/set.cpp)
- **Implementation Progress:** [set](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=930086747)
- **Changes:**
  - Renamed `set` to `static_set`. Fixed compile-time capacity.
  - If `is_trivial_v<T>`, then `is_trivially_copyable_v<static_set<T, Capacity>>`
  - If `is_trivial_v<T>`, then `is_trivially_destructible_v<static_set<T, Capacity>>`

### span

- **Library:** Containers
- **Include:** [`etl/span.hpp`](./etl/span.hpp)
- **Tests:** [test_span.cpp](./tests/test_span.cpp)
- **Example:** TODO
- **Implementation Progress:** [span](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1750377555)
- **Changes:**
  - None. Available in C++17.

### stack

- **Library:** Containers
- **Include:** [`etl/stack.hpp`](./etl/stack.hpp)
- **Tests:** [test_stack.cpp](./tests/test_stack.cpp)
- **Example:** TODO
- **Implementation Progress:** [stack](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=385809287)
- **Changes:**
  - None. Works with `static_vector`.

### string

- **Library:** Strings
- **Include:** [`etl/string.hpp`](./etl/string.hpp)
- **Tests:** [test_string.cpp](./tests/test_string.cpp)
- **Example:** [string.cpp](./examples/string.cpp)
- **Implementation Progress:** [string](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=43463000)
- **Changes:**
  - Only implemeted for type `char` at the moment.
  - Renamed `basic_string` to `basic_static_string`. Fixed compile-time capacity.

### string_view

- **Library:** Strings
- **Include:** [`etl/string_view.hpp`](./etl/string_view.hpp)
- **Tests:** [test_string_view.cpp](./tests/test_string_view.cpp)
- **Example:** [string_view.cpp](./examples/string_view.cpp)
- **Implementation Progress:** [string_view](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1803550736)
- **Changes:**
  - None
  - Only implemeted for type `char` at the moment.

### system_error

- **Library:** Utility / Error Handling
- **Include:** [`etl/system_error.hpp`](./etl/system_error.hpp)
- **Tests:** [test_system_error.cpp](./tests/test_system_error.cpp)
- **Example:** TODO
- **Implementation Progress:** [system_error](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=635426347)
- **Changes:**
  - Only provides `errc` enum and helper traits.

### tuple

- **Library:** Utility
- **Include:** [`etl/tuple.hpp`](./etl/tuple.hpp)
- **Tests:** [test_tuple.cpp](./tests/test_tuple.cpp)
- **Example:** [tuple.cpp](./examples/tuple.cpp)
- **Implementation Progress:** [tuple](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=857929646)
- **Changes:**
  - Broken at the moment.

### type_traits

- **Library:** Utility
- **Include:** [`etl/type_traits.hpp`](./etl/type_traits.hpp)
- **Tests:** [test_type_traits.cpp](./tests/test_type_traits.cpp)
- **Example:** [type_traits.cpp](./examples/type_traits.cpp)
- **Implementation Progress:** [type_traits](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1691010448)
- **Changes:**
  - None

### utility

- **Library:** Utility
- **Include:** [`etl/utility.hpp`](./etl/utility.hpp)
- **Tests:** [test_utility.cpp](./tests/test_utility.cpp)
- **Example:** [utility.cpp](./examples/utility.cpp)
- **Implementation Progress:** [utility](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1484976254)
- **Changes:**
  - None

### variant

- **Library:** Utility
- **Include:** [`etl/variant.hpp`](./etl/variant.hpp)
- **Tests:** [test_variant.cpp](./tests/test_variant.cpp)
- **Example:** TODO
- **Implementation Progress:** [variant](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=503059518)
- **Changes:**
  - Broken at the moment.

### vector

- **Library:** Containers
- **Include:** [`etl/vector.hpp`](./etl/vector.hpp)
- **Tests:** [test_vector.cpp](./tests/test_vector.cpp)
- **Example:** [vector.cpp](./examples/vector.cpp)
- **Implementation Progress:** [vector](https://docs.google.com/spreadsheets/d/1-qwa7tFnjFdgY9XKBy2fAsDozAfG8lXsJXHwA_ITQqM/edit#gid=1613833122)
- **Changes:**
  - Renamed `vector` to `static_vector`. Fixed compile-time capacity.
  - Based on `P0843r3` and the reference implementation from [github.com/gnzlbg/static_vector](https://github.com/gnzlbg/static_vector).
  - If `is_trivial_v<T>`, then `is_trivially_copyable_v<static_vector<T, Capacity>>`
  - If `is_trivial_v<T>`, then `is_trivially_destructible_v<static_vector<T, Capacity>>`

### version

- **Library:** Utility
- **Include:** [`etl/version.hpp`](./etl/version.hpp)
- **Tests:** [test_version.cpp](./tests/test_version.cpp)


# Installation & Usage
You can use KTL directly as driver CMake project subdirectory or link with pre-built KTL binaries applying `find_package()`.

It includes **3 static libraries**: 
* `basic_runtime.lib` (CRT)
* `cpp_runtime.lib` (C++ tools)
* `minifilter_runtime.lib` (optional Filesystem Mini-Filter support)

### CMakeLists.txt to build with a pre-built KTL:
```cmake
    project(MyDriver CXX)

    find_package(WDK REQUIRED)  # Windows Driver Kit
    find_package(KTL REQUIRED)

    wdk_add_driver(
      ${DRIVER_NAME} minifilter.cpp
        CUSTOM_ENTRY_POINT KtlDriverEntry   # Required for C and C++ Runtime initialization and destruction
		    EXTENDED_CPP_FEATURES               # Exceptions handling enabled
    )
    target_include_directories(
	    ${DRIVER_NAME} PUBLIC
		    ${KTL_INCLUDE_DIR}
		    ${KTL_MODULES_DIR}                  # Lock-Free tools
		    ${KTL_RUNTIME_INCLUDE_DIR}
		    ${CMAKE_INCLUDE_CURRENT_DIR}
    )
    target_link_libraries(
      ${DRIVER_NAME} PRIVATE
        ktl::basic_runtime
        ktl::minifilter_runtime
        ktl::cpp_runtime
    )
    wdk_sign_driver(        # Driver signing (in this example - using given certificate)
	    ${DRIVER_NAME}
		    ${YOUR_CERTIFICATE_NAME}
		    CERTIFICATE_PATH ${YOUR_CERTIFICATE_PATH}
		    TIMESTAMP_SERVER ${YOUR_TIMESTAMP_SERVER} # Default is timestamp.verisign.com
    )
```

### Build requirements:
* WDK10
* Visual Studio 2019 (not tested on older versions)
* CMake 3.0 and higher
* [FindWDK](https://github.com/DymOK93/FindWDK/tree/develop) (used as a Git submodule)

# Examples
* [CoroDriverSample](https://github.com/DymOK93/CoroDriverSample) - a simple driver demonstrating the use of C++20 coroutines in kernel mode 

# Roadmap for the near future 
* Linked lists and Red-Black-Tree containers
* Intrusive containers
* Coroutine-compatible async primitives
* **Exception handling on the x86 platforms**

# License
KTL distributed under MIT [license](https://github.com/DymOK93/KTL/blob/master/LICENSE.md).
