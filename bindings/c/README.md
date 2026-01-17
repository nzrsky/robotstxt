# C Bindings for robotstxt

C API for Google's robots.txt parser library.

## Files

- `robots_c.h` — C API header
- `robots_c.cc` — Implementation (C++)
- `demo.c` — Usage example

## Building

The C API is built automatically with the main library:

```bash
cmake -B build
cmake --build build
```

## Usage

```c
#include "robots_c.h"
#include <stdio.h>
#include <string.h>

int main() {
    const char* robots_txt = "User-agent: *\nDisallow: /admin/\n";
    const char* user_agent = "Googlebot";
    const char* url = "https://example.com/admin/secret";

    // Create matcher
    robots_matcher_t* matcher = robots_matcher_create();

    // Check if URL is allowed
    bool allowed = robots_allowed_by_robots(
        matcher,
        robots_txt, strlen(robots_txt),
        user_agent, strlen(user_agent),
        url, strlen(url));

    printf("Access: %s\n", allowed ? "allowed" : "disallowed");

    // Get crawl-delay
    if (robots_has_crawl_delay(matcher)) {
        printf("Crawl-delay: %.1f seconds\n", robots_get_crawl_delay(matcher));
    }

    // Cleanup
    robots_matcher_free(matcher);
    return 0;
}
```

## Compiling

```bash
# With shared library
cc -o myapp myapp.c -I/path/to/include -L/path/to/lib -lrobots

# With static library
cc -o myapp myapp.c -I/path/to/include /path/to/lib/librobots.a -lstdc++
```

## API Reference

### Lifecycle

- `robots_matcher_create()` — Create a matcher instance
- `robots_matcher_free(matcher)` — Free a matcher instance

### URL Checking

- `robots_allowed_by_robots(matcher, robots_txt, len, user_agent, len, url, len)` — Check single user-agent
- `robots_allowed_by_robots_multi(...)` — Check multiple user-agents

### Accessors (after URL check)

- `robots_matching_line(matcher)` — Get matching line number
- `robots_ever_seen_specific_agent(matcher)` — Check if specific agent was found

### Crawl-delay

- `robots_has_crawl_delay(matcher)` — Check if crawl-delay is specified
- `robots_get_crawl_delay(matcher)` — Get crawl-delay in seconds

### Request-rate

- `robots_has_request_rate(matcher)` — Check if request-rate is specified
- `robots_get_request_rate(matcher, &rate)` — Get request-rate struct

### Content-Signal

- `robots_content_signal_supported()` — Check if compiled with Content-Signal support
- `robots_has_content_signal(matcher)` — Check if content-signal is specified
- `robots_get_content_signal(matcher, &signal)` — Get content-signal struct
- `robots_allows_ai_train(matcher)` — Check AI training permission
- `robots_allows_ai_input(matcher)` — Check AI input permission
- `robots_allows_search(matcher)` — Check search indexing permission

### Utilities

- `robots_is_valid_user_agent(user_agent, len)` — Validate user-agent string
- `robots_version()` — Get library version

## License

Apache License 2.0
