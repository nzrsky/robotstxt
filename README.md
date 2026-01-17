# Robots.txt Parser and Matcher Library

A high-performance fork of [Google's robots.txt parser](https://github.com/google/robotstxt) with bug fixes and ~30% faster parsing (C++20).

## About the library

The Robots Exclusion Protocol (REP) is a standard that enables website owners to
control which URLs may be accessed by automated clients (i.e. crawlers) through
a simple text file with a specific syntax. It's one of the basic building blocks
of the internet as we know it and what allows search engines to operate.

Because the REP was only a de-facto standard for the past 25 years, different
implementers implement parsing of robots.txt slightly differently, leading to
confusion. This project aims to fix that by releasing the parser that Google
uses.

The library is slightly modified (i.e. some internal headers and equivalent
symbols) production code used by Googlebot, Google's crawler, to determine which
URLs it may access based on rules provided by webmasters in robots.txt files.
The library is released open-source to help developers build tools that better
reflect Google's robots.txt parsing and matching.

For webmasters, we included a small binary in the project that allows testing a
single URL and user-agent against a robots.txt.

## Changes in this fork

This fork includes several improvements over the original Google implementation:

### Performance
- **Zero-copy parsing**: Eliminated 16KB heap allocation per parse by using `string_view` to reference lines directly from input buffer (~30% faster)
- **[ada](https://github.com/ada-url/ada) URL parser**: Replaced hand-rolled URL parsing with SIMD-optimized, WHATWG-compliant ada library
- **Modern C++20**: Uses `std::string_view` instead of abseil's `absl::string_view`, removing the abseil dependency

### RFC 9309 Compliance Fixes
- **Issue [#57](https://github.com/google/robotstxt/issues/57)**: Percent-encoded special characters (`%2A` for `*`, `%24` for `$`) in robots.txt rules now correctly match their literal equivalents in URLs
- **Issue [#64](https://github.com/google/robotstxt/issues/64)**: Percent-encoded characters in query strings (e.g., `http%3A%2F%2F`) now match their decoded equivalents (`http://`) per RFC 9309 section 2.2.2

### Other Fixes
- **Issue [#51](https://github.com/google/robotstxt/issues/51)**: Combination of Crawl-delay and badbot Disallow results in blocking of Googlebot

### New Features
- **Extended Directives**: Support for `Crawl-delay`, `Request-rate`, and `Content-Signal` (AI training/indexing preferences) (**Issue [#80](https://github.com/google/robotstxt/issues/80)**)
- **C API**: Full-featured C bindings for easy integration with any language via FFI
- **Language Bindings**: Official bindings for Python, Go, Rust, Ruby, Java, and Swift

## Benchmark

Performance comparison on 6,863 real robots.txt files (~9 MB):

| Implementation | Parse + Match | Relative |
|:---|---:|---:|
| **C++ (this repo)** | 20.1 ms | 1.00 |
| C++ (Google original) | 26.1 ms | 1.30× slower |
| [folyd's Rust port](https://github.com/Folyd/robotstxt) | 82.8 ms | 4.1× slower |
| [jimsmart's Go port](https://github.com/jimsmart/grobotstxt) | 93.2 ms | 4.6× slower |
| [cocon-se's Python port](https://github.com/Cocon-Se/gpyrobotstxt) | 1.35 s | 67× slower |

This fork is **~30% faster** than the original Google implementation thanks to zero-copy parsing optimization.

See [BENCHMARK_RESULTS](BENCHMARK_RESULTS) for details.

## Building the library

### Quickstart

We included with the library a small binary to test a local robots.txt against a
user-agent and URL. Running the included binary requires:

*   A compatible platform (e.g. Windows, macOS, Linux, etc.). Most platforms are
    fully supported.
*   A compatible C++ compiler supporting at least C++20. Most major compilers
    are supported.
*   [Git](https://git-scm.com/) for interacting with the source code repository.
    To install Git, consult the
    [Set Up Git](https://help.github.com/articles/set-up-git/) guide on
    [GitHub](https://github.com/).
*   Although you are free to use your own build system, most of the
    documentation within this guide will assume you are using
    [Bazel](https://bazel.build/). To download and install Bazel (and any of its
    dependencies), consult the
    [Bazel Installation Guide](https://docs.bazel.build/versions/master/install.html)

#### Building with Bazel

[Bazel](https://bazel.build/) is the official build system for the library,
which is supported on most major platforms (Linux, Windows, MacOS, for example)
and compilers.

To build and run the binary:

```bash
$ git clone https://github.com/google/robotstxt.git robotstxt
Cloning into 'robotstxt'...
...
$ cd robotstxt/
bazel-robots$ bazel test :robots_test
...
/:robots_test                                                      PASSED in 0.1s

Executed 1 out of 1 test: 1 test passes.
...
bazel-robots$ bazel build :robots_main
...
Target //:robots_main up-to-date:
  bazel-bin/robots_main
...
bazel-robots$ bazel run robots_main -- ~/local/path/to/robots.txt YourBot https://example.com/url
  user-agent 'YourBot' with URI 'https://example.com/url': ALLOWED
```

#### Building with CMake

[CMake](https://cmake.org) is the community-supported build system for the
library.

To build the library using CMake, just follow the steps below:

```bash
$ git clone https://github.com/google/robotstxt.git robotstxt
Cloning into 'robotstxt'...
...
$ cd robotstxt/
...
$ mkdir c-build && cd c-build
...
$ cmake .. -DROBOTS_BUILD_TESTS=ON
...
$ make
...
$ make test
Running tests...
Test project robotstxt/c-build
    Start 1: robots-test
1/1 Test #1: robots-test ......................   Passed    0.02 sec

100% tests passed, 0 tests failed out of 1

Total Test time (real) =   0.02 sec
...
$ robots ~/local/path/to/robots.txt YourBot https://example.com/url
  user-agent 'YourBot' with URI 'https://example.com/url': ALLOWED
```
> **Note**: If the robots file is empty, the parser also prints:
>
> ```
> notice: robots file is empty so all user-agents are allowed
> ```

> **Exit codes:** `0` = ALLOWED, `1` = DISALLOWED

## Language Bindings

This library provides official bindings for multiple programming languages. All bindings expose the same core functionality with idiomatic APIs.

### C

```c
#include "robots_c.h"

robots_matcher_t* matcher = robots_matcher_create();

const char* robots_txt = "User-agent: *\nDisallow: /admin/\n";
const char* user_agent = "Googlebot";
const char* url = "https://example.com/admin/secret";

bool allowed = robots_allowed(matcher, robots_txt, user_agent, url);

printf("Access: %s\n", allowed ? "allowed" : "disallowed");

// Check crawl-delay
if (robots_has_crawl_delay(matcher)) {
    printf("Crawl-delay: %.1f seconds\n", robots_get_crawl_delay(matcher));
}

robots_matcher_free(matcher);
```

### Python

```bash
pip install robotstxt  # or use from source
```

```python
from robotstxt import RobotsMatcher

matcher = RobotsMatcher()
robots_txt = """
User-agent: *
Disallow: /admin/
Crawl-delay: 1.5
"""

allowed = matcher.is_allowed(robots_txt, "Googlebot", "https://example.com/page")
print(f"Access: {'allowed' if allowed else 'disallowed'}")

# Extended directives
if matcher.crawl_delay is not None:
    print(f"Crawl-delay: {matcher.crawl_delay} seconds")

if matcher.allows_ai_train():
    print("AI training: allowed")
```

### Go

```go
package main

import (
    "fmt"
    "github.com/nzrsky/robotstxt/bindings/go"
)

func main() {
    matcher := robotstxt.NewMatcher()
    defer matcher.Free()

    robotsTxt := `
User-agent: *
Disallow: /admin/
Crawl-delay: 2.0
`
    allowed := matcher.IsAllowed(robotsTxt, "Googlebot", "https://example.com/page")
    fmt.Printf("Access: %v\n", allowed)

    // Extended directives
    if delay, ok := matcher.CrawlDelay(); ok {
        fmt.Printf("Crawl-delay: %.1f seconds\n", delay)
    }
}
```

### Rust

```toml
# Cargo.toml
[dependencies]
robotstxt = { path = "bindings/rust" }
```

```rust
use robotstxt::RobotsMatcher;

fn main() {
    let matcher = RobotsMatcher::new();
    let robots_txt = "User-agent: *\nDisallow: /admin/\n";

    let allowed = matcher.is_allowed(robots_txt, "Googlebot", "https://example.com/page");
    println!("Access: {}", if allowed { "allowed" } else { "disallowed" });

    // Extended directives
    if let Some(delay) = matcher.crawl_delay() {
        println!("Crawl-delay: {} seconds", delay);
    }

    if matcher.allows_ai_train() {
        println!("AI training: allowed");
    }
}
```

### Ruby

```ruby
require 'robotstxt'

matcher = RobotsTxt::Matcher.new
robots_txt = <<~ROBOTS
  User-agent: *
  Disallow: /admin/
  Crawl-delay: 1.0
ROBOTS

allowed = matcher.allowed?(robots_txt, "Googlebot", "https://example.com/page")
puts "Access: #{allowed ? 'allowed' : 'disallowed'}"

# Extended directives
if matcher.crawl_delay
  puts "Crawl-delay: #{matcher.crawl_delay} seconds"
end

puts "AI training: #{matcher.allows_ai_train? ? 'allowed' : 'disallowed'}"
```

### Java

```java
import com.google.robotstxt.RobotsMatcher;

public class Example {
    public static void main(String[] args) {
        try (RobotsMatcher matcher = new RobotsMatcher()) {
            String robotsTxt = "User-agent: *\nDisallow: /admin/\n";
            boolean allowed = matcher.isAllowed(robotsTxt, "Googlebot", "https://example.com/page");
            System.out.println("Access: " + (allowed ? "allowed" : "disallowed"));

            // Extended directives
            Double crawlDelay = matcher.getCrawlDelay();
            if (crawlDelay != null) {
                System.out.println("Crawl-delay: " + crawlDelay + " seconds");
            }

            System.out.println("AI training: " + (matcher.allowsAITrain() ? "allowed" : "disallowed"));
        }
    }
}
```

### Swift

```swift
import RobotsTxt

let matcher = RobotsMatcher()
let robotsTxt = """
User-agent: *
Disallow: /admin/
"""

let allowed = matcher.isAllowed(robotsTxt: robotsTxt, userAgent: "Googlebot", url: "https://example.com/page")
print("Access: \(allowed ? "allowed" : "disallowed")")

// Extended directives
if let delay = matcher.crawlDelay {
    print("Crawl-delay: \(delay) seconds")
}

if matcher.allowsAITrain {
    print("AI training: allowed")
}
```

### Building Bindings

All bindings require the C++ library to be built first:

```bash
cmake -B _build -DCMAKE_BUILD_TYPE=Release
cmake --build _build
```

Then set the library path:
- **Linux**: `export LD_LIBRARY_PATH=$PWD/_build`
- **macOS**: `export DYLD_LIBRARY_PATH=$PWD/_build`
- **Windows**: Add `_build` directory to `PATH`

## Notes

Parsing of robots.txt files themselves is done exactly as in the production
version of Googlebot, including how percent codes and unicode characters in
patterns are handled. The user must ensure however that the URI passed to the
AllowedByRobots and OneAgentAllowedByRobots functions, or to the URI parameter
of the robots tool, follows the format specified by RFC3986, since this library
will not perform full normalization of those URI parameters. Only if the URI is
in this format, the matching will be done according to the REP specification.

Also note that the library, and the included binary, do not handle
implementation logic that a crawler might apply outside of parsing and matching,
for example: `Googlebot-Image` respecting the rules specified for `User-agent:
Googlebot` if not explicitly defined in the robots.txt file being tested.

## License

The robots.txt parser and matcher C++ library is licensed under the terms of the
Apache license. See LICENSE for more information.

## Links

To learn more about this project:

*   check out the
    [Robots Exclusion Protocol standard](https://www.rfc-editor.org/rfc/rfc9309.html),
*   how
    [Google Handles robots.txt](https://developers.google.com/search/reference/robots_txt),
*   or for a high level overview, the
    [robots.txt page on Wikipedia](https://en.wikipedia.org/wiki/Robots_exclusion_standard).
