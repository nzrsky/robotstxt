# RobotsTxt Rust Bindings

Rust bindings for Google's robots.txt parser library.

## Requirements

- Rust 2021 edition
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

Add to your `Cargo.toml`:

```toml
[dependencies]
robotstxt = { path = "path/to/robotstxt/bindings/rust" }
```

Set library path for linking:

```bash
export DYLD_LIBRARY_PATH="/path/to/robotstxt/build:$DYLD_LIBRARY_PATH"
# or on Linux:
export LD_LIBRARY_PATH="/path/to/robotstxt/build:$LD_LIBRARY_PATH"
```

## Usage

```rust
use robotstxt::{RobotsMatcher, version, is_valid_user_agent, content_signal_supported};

fn main() {
    // Create a matcher
    let matcher = RobotsMatcher::new();

    // Check if URL is allowed
    let robots_txt = "User-agent: *\nDisallow: /admin/\nCrawl-delay: 2.5\n";
    let allowed = matcher.is_allowed(robots_txt, "Googlebot", "https://example.com/page");
    println!("Access: {}", if allowed { "allowed" } else { "disallowed" });

    // Get crawl delay
    if let Some(delay) = matcher.crawl_delay() {
        println!("Crawl delay: {} seconds", delay);
    }

    // Get request rate
    if let Some(rate) = matcher.request_rate() {
        println!("Request rate: {} requests per {} seconds", rate.requests, rate.seconds);
    }

    // Check Content-Signal (if supported)
    if content_signal_supported() {
        println!("AI training allowed: {}", matcher.allows_ai_train());
        println!("AI input allowed: {}", matcher.allows_ai_input());
        println!("Search allowed: {}", matcher.allows_search());
    }

    // Get matching line number
    println!("Matched at line: {}", matcher.matching_line());

    // Check library version
    println!("Library version: {}", version());

    // Validate user-agent strings
    println!("Valid user-agent: {}", is_valid_user_agent("Googlebot"));
}
```

## API Reference

### Functions

- `version() -> String` - Get library version
- `is_valid_user_agent(user_agent: &str) -> bool` - Check if user-agent is valid
- `content_signal_supported() -> bool` - Whether Content-Signal is compiled in

### `RobotsMatcher`

The main struct for parsing and matching robots.txt rules. Implements `Send`, `Sync`, `Default`, and `Drop`.

#### Methods

- `new() -> Self` - Create a new matcher
- `is_allowed(&self, robots_txt: &str, user_agent: &str, url: &str) -> bool` - Check if URL is allowed
- `matching_line(&self) -> i32` - Line number of the last match (0 if none)
- `ever_seen_specific_agent(&self) -> bool` - True if a specific user-agent block was found
- `crawl_delay(&self) -> Option<f64>` - Crawl delay in seconds
- `request_rate(&self) -> Option<RequestRate>` - Request rate limit
- `content_signal(&self) -> Option<ContentSignal>` - Content signal values
- `allows_ai_train(&self) -> bool` - Whether AI training is allowed
- `allows_ai_input(&self) -> bool` - Whether AI input is allowed
- `allows_search(&self) -> bool` - Whether search indexing is allowed

### `RequestRate`

Request rate limit struct (`#[repr(C)]`).

- `requests: c_int` - Number of requests allowed
- `seconds: c_int` - Time period in seconds

### `ContentSignal`

Content signal values struct (`#[repr(C)]`). Values are tri-state: -1=unset, 0=no, 1=yes.

- `ai_train: i8` - AI training preference
- `ai_input: i8` - AI input preference
- `search: i8` - Search indexing preference

## Thread Safety

`RobotsMatcher` is `Send` and `Sync` - it can be safely shared between threads for read operations after parsing.

## Running Tests

```bash
# Set library path
export DYLD_LIBRARY_PATH="../../build:$DYLD_LIBRARY_PATH"

cargo test
```

## License

Apache 2.0 - See the main repository LICENSE file.
