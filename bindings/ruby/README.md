# RobotsTxt Ruby Bindings

Ruby bindings for Google's robots.txt parser library.

## Requirements

- Ruby 2.7+
- The `ffi` gem
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

### From Source

```bash
gem build robotstxt.gemspec
gem install robotstxt-1.0.0.gem
```

### In Bundler

Add to your `Gemfile`:

```ruby
gem 'robotstxt', path: 'path/to/robotstxt/bindings/ruby'
```

### Library Path

Set the library path if not in standard locations:

```bash
export ROBOTS_LIB_PATH=/path/to/robotstxt/build
```

## Usage

```ruby
require 'robotstxt'

# Create a matcher
matcher = RobotsTxt::Matcher.new

# Check if a URL is allowed
robots_txt = <<~ROBOTS
  User-agent: *
  Disallow: /admin/
ROBOTS

allowed = matcher.allowed?(robots_txt, 'Googlebot', 'https://example.com/page')
puts "Access: #{allowed ? 'allowed' : 'disallowed'}"

# Get crawl delay
if (delay = matcher.crawl_delay)
  puts "Crawl delay: #{delay} seconds"
end

# Get request rate
if (rate = matcher.request_rate)
  puts "Request rate: #{rate.requests} requests per #{rate.seconds} seconds"
  puts "Delay between requests: #{rate.delay_seconds} seconds"
end

# Check Content-Signal (if supported)
if RobotsTxt::Matcher.content_signal_supported?
  puts "AI training allowed: #{matcher.allows_ai_train?}"
  puts "AI input allowed: #{matcher.allows_ai_input?}"
  puts "Search allowed: #{matcher.allows_search?}"
end

# Get matching line number
puts "Matched at line: #{matcher.matching_line}"

# Check library version
puts "Library version: #{RobotsTxt.version}"

# Validate user-agent strings
puts "Valid user-agent: #{RobotsTxt.valid_user_agent?('Googlebot')}"
```

## API Reference

### `RobotsTxt::Matcher`

The main class for parsing and matching robots.txt rules.

#### Methods

- `allowed?(robots_txt, user_agent, url)` - Check if URL is allowed
- `is_allowed(robots_txt, user_agent, url)` - Alias for `allowed?`
- `matching_line` - Line number of the last match (0 if none)
- `ever_seen_specific_agent?` - True if a specific user-agent block was found
- `crawl_delay` - Crawl delay in seconds (nil if not specified)
- `request_rate` - Request rate limit (nil if not specified)
- `content_signal` - Content signal values (nil if not specified)
- `allows_ai_train?` - Whether AI training is allowed
- `allows_ai_input?` - Whether AI input is allowed
- `allows_search?` - Whether search indexing is allowed

#### Class Methods

- `content_signal_supported?` - Whether Content-Signal is compiled in

### `RobotsTxt::RequestRate`

Request rate limit structure.

- `requests` - Number of requests allowed
- `seconds` - Time period in seconds
- `requests_per_second` - Computed requests per second
- `delay_seconds` - Computed delay between requests

### `RobotsTxt::ContentSignal`

Content signal values (tri-state: nil=unset, true=yes, false=no).

- `ai_train` - AI training preference
- `ai_input` - AI input preference
- `search` - Search indexing preference

### Module Methods

- `RobotsTxt.version` - Get library version
- `RobotsTxt.valid_user_agent?(user_agent)` - Check if user-agent is valid

## Running Tests

```bash
ruby test/test_robotstxt.rb
```

## License

Apache 2.0 - See the main repository LICENSE file.
