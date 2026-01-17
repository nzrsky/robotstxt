# robotstxt

Python bindings for Google's robots.txt parser library - high-performance, RFC 9309 compliant.

## Installation

```bash
pip install fast-robotstxt
```

Pre-built wheels are available for:
- **Linux**: x86_64, aarch64
- **macOS**: x86_64, arm64 (Apple Silicon)
- **Windows**: AMD64

Python 3.8 - 3.13 supported.

### Build from source

If you need to build from source:

```bash
git clone https://github.com/nzrsky/robotstxt.git
cd robotstxt/bindings/python
pip install .
```

## Usage

```python
from robotstxt import RobotsMatcher

# Create a matcher
matcher = RobotsMatcher()

robots_txt = """
User-agent: *
Disallow: /admin/
Allow: /admin/public/
Crawl-delay: 2.5

User-agent: Googlebot
Allow: /
"""

# Check if URL is allowed
allowed = matcher.is_allowed(robots_txt, "Bingbot", "https://example.com/admin/secret")
print(f"Access: {'allowed' if allowed else 'disallowed'}")

# Get crawl delay
if matcher.crawl_delay is not None:
    print(f"Crawl delay: {matcher.crawl_delay}s")

# Get request rate
if matcher.request_rate is not None:
    requests, seconds = matcher.request_rate
    print(f"Request rate: {requests} per {seconds}s")

# Check Content-Signal (AI preferences)
signal = matcher.content_signal
if signal:
    print(f"AI training allowed: {signal['ai_train']}")
```

## API Reference

### `RobotsMatcher`

Main class for matching URLs against robots.txt.

#### Methods

- `is_allowed(robots_txt, user_agent, url) -> bool`
  Check if URL is allowed for a single user-agent.

- `is_allowed_multi(robots_txt, user_agents, url) -> bool`
  Check if URL is allowed for multiple user-agents.

#### Properties

- `matching_line: int` - Line number that matched (0 if no match)
- `ever_seen_specific_agent: bool` - True if specific user-agent was found
- `crawl_delay: Optional[float]` - Crawl delay in seconds
- `request_rate: Optional[Tuple[int, int]]` - (requests, seconds) tuple
- `content_signal: Optional[dict]` - AI content preferences
- `allows_ai_train: bool` - True if AI training is allowed
- `allows_ai_input: bool` - True if AI input is allowed
- `allows_search: bool` - True if search indexing is allowed

### Functions

- `get_version() -> str` - Get library version
- `is_valid_user_agent(user_agent) -> bool` - Validate user-agent string

## Context Manager

```python
with RobotsMatcher() as matcher:
    allowed = matcher.is_allowed(robots_txt, "Googlebot", url)
```

## License

Apache License 2.0
