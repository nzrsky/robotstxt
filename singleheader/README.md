# Single-Header Robotstxt

This directory contains amalgamated single-header versions of the robotstxt library.

## Files

- **`robots.h`** — Core library (parser + matcher)
- **`reporting_robots.h`** — Core + reporting utilities

## Usage

### Basic (robots.h)

```cpp
// In exactly ONE .cpp file:
#define ROBOTS_IMPLEMENTATION
#include "robots.h"

// In other files, just include without the define:
#include "robots.h"
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
# Simple compilation
c++ -std=c++20 -O2 demo.cpp -o demo

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
