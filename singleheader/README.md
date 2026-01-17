# Single-Header Robotstxt

This directory contains amalgamated single-header versions of the robotstxt library.

## Files

- **`robots.h`** — Core C++ library (parser + matcher)
- **`robots_c.h`** — C API (callable from C, implementation in C++)
- **`reporting_robots.h`** — Core + reporting utilities

## Usage

### C++ API (robots.h)

```cpp
// In exactly ONE .cpp file:
#define ROBOTS_IMPLEMENTATION
#include "robots.h"

// In other files, just include without the define:
#include "robots.h"
```

### C API (robots_c.h)

```c
// In exactly ONE .cpp file (implementation requires C++):
#define ROBOTS_IMPLEMENTATION
#include "robots_c.h"

// In C files, just include the header:
#include "robots_c.h"

// Usage:
robots_matcher_t* m = robots_matcher_create();
bool allowed = robots_allowed_by_robots(m, robots_txt, len, agent, alen, url, ulen);
robots_matcher_free(m);
```

### With Reporting (reporting_robots.h)

```cpp
// In exactly ONE .cpp file:
#define ROBOTS_IMPLEMENTATION
#include "reporting_robots.h"

// In other files:
#include "reporting_robots.h"
```

## Build Examples

```bash
# C++ demo
c++ -std=c++20 -O2 demo.cpp -o demo

# C API demo (requires C++ compiler for implementation)
c++ -std=c++20 -O2 demo_c.cpp -o demo_c

# With CMake
cmake -B build && cmake --build build
```

## Regenerating

Run from the repository root:

```bash
python3 singleheader/amalgamate.py
```

## Configuration

Define before including to configure:

```cpp
// Disable Content-Signal directive support (smaller binary)
#define ROBOTS_SUPPORT_CONTENT_SIGNAL 0
#define ROBOTS_IMPLEMENTATION
#include "robots.h"
```
