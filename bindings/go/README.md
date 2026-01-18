# RobotsTxt Go Bindings

Go bindings for Google's robots.txt parser library using cgo.

## Requirements

- Go 1.18+
- C++ compiler (GCC, Clang, or MSVC)

## Installation

```bash
go get github.com/nzrsky/robotstxt/bindings/go
```

The library uses single-header C++ implementation, so no external library installation is required.

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
    defer matcher.Free()

    // Check if URL is allowed
    robotsTxt := `
User-agent: *
Disallow: /admin/
Crawl-delay: 2.5
`
    allowed := matcher.IsAllowed(robotsTxt, "Googlebot", "https://example.com/page")
    fmt.Printf("Access: %s\n", map[bool]string{true: "allowed", false: "disallowed"}[allowed])

    // Get crawl delay
    if delay := matcher.CrawlDelay(); delay != nil {
        fmt.Printf("Crawl delay: %.1f seconds\n", *delay)
    }

    // Check Content-Signal (if supported)
    if robotstxt.ContentSignalSupported() {
        fmt.Printf("AI training allowed: %v\n", matcher.AllowsAITrain())
    }

    // Check library version
    fmt.Printf("Library version: %s\n", robotstxt.Version())
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

- `Free()` - Release resources (use with defer)
- `IsAllowed(robotsTxt, userAgent, url string) bool` - Check if URL is allowed
- `IsAllowedMulti(robotsTxt string, userAgents []string, url string) bool` - Check for multiple user-agents
- `MatchingLine() int` - Line number of the last match (0 if none)
- `EverSeenSpecificAgent() bool` - True if a specific user-agent block was found
- `CrawlDelay() *float64` - Crawl delay in seconds (nil if not specified)
- `RequestRate() *RequestRate` - Request rate limit (nil if not specified)
- `ContentSignal() *ContentSignal` - Content signal values (nil if not specified)
- `AllowsAITrain() bool` - Whether AI training is allowed
- `AllowsAIInput() bool` - Whether AI input is allowed
- `AllowsSearch() bool` - Whether search indexing is allowed

### `RequestRate`

Request rate limit struct.

- `Requests int` - Number of requests allowed
- `Seconds int` - Time period in seconds

### `ContentSignal`

Content signal values (tri-state: nil=unset, true=yes, false=no).

- `AITrain *bool` - AI training preference
- `AIInput *bool` - AI input preference
- `Search *bool` - Search indexing preference

## Running Tests

```bash
go test -v
```

## License

Apache 2.0 - See the main repository LICENSE file.
