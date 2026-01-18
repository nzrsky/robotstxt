# RobotsTxt Go Bindings

Go bindings for Google's robots.txt parser library using cgo.

## Requirements

- Go 1.18+
- The `librobots` shared library must be built and installed

## Building the Library

First, build the main robots library:

```bash
cd ../..
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

## Installation

```bash
# Set library path for building and linking
export CGO_LDFLAGS="-L/path/to/robotstxt/build -lrobots"
export CGO_CFLAGS="-I/path/to/robotstxt/bindings/c"
export LD_LIBRARY_PATH="/path/to/robotstxt/build:$LD_LIBRARY_PATH"
# or on macOS:
export DYLD_LIBRARY_PATH="/path/to/robotstxt/build:$DYLD_LIBRARY_PATH"

go get github.com/nzrsky/robotstxt/bindings/go
```

## Usage

```go
package main

import (
    "fmt"
    robotstxt "github.com/nzrsky/robotstxt/bindings/go"
)

func main() {
    // Create a matcher
    matcher := robotstxt.NewMatcher()
    defer matcher.Close()

    // Check if URL is allowed
    robotsTxt := `
User-agent: *
Disallow: /admin/
Crawl-delay: 2.5
`
    allowed := matcher.IsAllowed(robotsTxt, "Googlebot", "https://example.com/page")
    fmt.Printf("Access: %s\n", map[bool]string{true: "allowed", false: "disallowed"}[allowed])

    // Get crawl delay
    if delay, ok := matcher.CrawlDelay(); ok {
        fmt.Printf("Crawl delay: %.1f seconds\n", delay)
    }

    // Get request rate
    if rate, ok := matcher.RequestRate(); ok {
        fmt.Printf("Request rate: %d requests per %d seconds\n", rate.Requests, rate.Seconds)
        fmt.Printf("Delay between requests: %.1f seconds\n", rate.DelaySeconds())
    }

    // Check Content-Signal (if supported)
    if robotstxt.ContentSignalSupported() {
        fmt.Printf("AI training allowed: %v\n", matcher.AllowsAITrain())
        fmt.Printf("AI input allowed: %v\n", matcher.AllowsAIInput())
        fmt.Printf("Search allowed: %v\n", matcher.AllowsSearch())
    }

    // Get matching line number
    fmt.Printf("Matched at line: %d\n", matcher.MatchingLine())

    // Check library version
    fmt.Printf("Library version: %s\n", robotstxt.Version())

    // Validate user-agent strings
    fmt.Printf("Valid user-agent: %v\n", robotstxt.IsValidUserAgent("Googlebot"))
}
```

## API Reference

### Functions

- `NewMatcher() *Matcher` - Create a new matcher
- `Version() string` - Get library version
- `IsValidUserAgent(userAgent string) bool` - Check if user-agent is valid
- `ContentSignalSupported() bool` - Whether Content-Signal is compiled in

### `Matcher`

The main struct for parsing and matching robots.txt rules.

#### Methods

- `Close()` - Release resources (use with defer)
- `IsAllowed(robotsTxt, userAgent, url string) bool` - Check if URL is allowed
- `MatchingLine() int` - Line number of the last match (0 if none)
- `EverSeenSpecificAgent() bool` - True if a specific user-agent block was found
- `CrawlDelay() (float64, bool)` - Crawl delay in seconds
- `RequestRate() (RequestRate, bool)` - Request rate limit
- `ContentSignal() (ContentSignal, bool)` - Content signal values
- `AllowsAITrain() bool` - Whether AI training is allowed
- `AllowsAIInput() bool` - Whether AI input is allowed
- `AllowsSearch() bool` - Whether search indexing is allowed

### `RequestRate`

Request rate limit struct.

- `Requests int` - Number of requests allowed
- `Seconds int` - Time period in seconds
- `RequestsPerSecond() float64` - Computed requests per second
- `DelaySeconds() float64` - Computed delay between requests

### `ContentSignal`

Content signal values (tri-state: nil=unset, true=yes, false=no).

- `AITrain *bool` - AI training preference
- `AIInput *bool` - AI input preference
- `Search *bool` - Search indexing preference

## Running Tests

```bash
# Set library paths
export CGO_LDFLAGS="-L../../build -lrobots -lc++"
export CGO_CFLAGS="-I../c"
export DYLD_LIBRARY_PATH="../../build:$DYLD_LIBRARY_PATH"

go test -v
```

## License

Apache 2.0 - See the main repository LICENSE file.
