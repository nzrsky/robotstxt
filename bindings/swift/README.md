# RobotsTxt Swift Bindings

Swift bindings for Google's robots.txt parser library.

## Requirements

- Swift 5.5+
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

### Swift Package Manager

Add to your `Package.swift`:

```swift
dependencies: [
    .package(path: "path/to/robotstxt/bindings/swift")
]
```

Or add as a local dependency with library path:

```swift
.target(
    name: "YourTarget",
    dependencies: ["RobotsTxt"],
    linkerSettings: [
        .unsafeFlags(["-L/path/to/robotstxt/build", "-Wl,-rpath,/path/to/robotstxt/build"])
    ]
)
```

## Usage

```swift
import RobotsTxt

// Create a matcher
let matcher = RobotsMatcher()

// Check if a URL is allowed
let robotsTxt = """
User-agent: *
Disallow: /admin/
"""

let allowed = matcher.isAllowed(
    robotsTxt: robotsTxt,
    userAgent: "Googlebot",
    url: "https://example.com/page"
)
print("Access: \(allowed ? "allowed" : "disallowed")")

// Get crawl delay
if let delay = matcher.crawlDelay {
    print("Crawl delay: \(delay) seconds")
}

// Get request rate
if let rate = matcher.requestRate {
    print("Request rate: \(rate.requests) requests per \(rate.seconds) seconds")
    print("Delay between requests: \(rate.delaySeconds) seconds")
}

// Check Content-Signal (if supported)
if RobotsMatcher.contentSignalSupported {
    print("AI training allowed: \(matcher.allowsAITrain)")
    print("AI input allowed: \(matcher.allowsAIInput)")
    print("Search allowed: \(matcher.allowsSearch)")
}

// Get matching line number
print("Matched at line: \(matcher.matchingLine)")

// Check library version
print("Library version: \(robotsVersion())")

// Validate user-agent strings
print("Valid user-agent: \(isValidUserAgent("Googlebot"))")
```

## API Reference

### `RobotsMatcher`

The main class for parsing and matching robots.txt rules.

#### Properties

- `matchingLine: Int` - Line number of the last match (0 if none)
- `everSeenSpecificAgent: Bool` - True if a specific user-agent block was found
- `crawlDelay: Double?` - Crawl delay in seconds (nil if not specified)
- `requestRate: RequestRate?` - Request rate limit (nil if not specified)
- `contentSignal: ContentSignal?` - Content signal values (nil if not specified)
- `allowsAITrain: Bool` - Whether AI training is allowed (defaults to true)
- `allowsAIInput: Bool` - Whether AI input is allowed (defaults to true)
- `allowsSearch: Bool` - Whether search indexing is allowed (defaults to true)

#### Methods

- `isAllowed(robotsTxt:userAgent:url:) -> Bool` - Check if URL is allowed

#### Static Properties

- `contentSignalSupported: Bool` - Whether Content-Signal is compiled in

### `RequestRate`

Request rate limit structure.

- `requests: Int` - Number of requests allowed
- `seconds: Int` - Time period in seconds
- `requestsPerSecond: Double` - Computed requests per second
- `delaySeconds: Double` - Computed delay between requests

### `ContentSignal`

Content signal values (tri-state: nil=unset, true=yes, false=no).

- `aiTrain: Bool?` - AI training preference
- `aiInput: Bool?` - AI input preference
- `search: Bool?` - Search indexing preference

### Utility Functions

- `robotsVersion() -> String` - Get library version
- `isValidUserAgent(_:) -> Bool` - Check if user-agent string is valid

## License

Apache 2.0 - See the main repository LICENSE file.
