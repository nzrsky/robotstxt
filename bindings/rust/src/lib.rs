//! Rust bindings for Google's robots.txt parser library.
//!
//! # Example
//!
//! ```no_run
//! use robotstxt::RobotsMatcher;
//!
//! let matcher = RobotsMatcher::new();
//! let robots_txt = "User-agent: *\nDisallow: /admin/\n";
//! let allowed = matcher.is_allowed(robots_txt, "Googlebot", "https://example.com/page");
//! println!("Access: {}", if allowed { "allowed" } else { "disallowed" });
//! ```

use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_double, c_int, c_void};

// FFI declarations
#[repr(C)]
struct RobotsMatcherOpaque {
    _private: [u8; 0],
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct RequestRate {
    pub requests: c_int,
    pub seconds: c_int,
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct ContentSignal {
    pub ai_train: i8,
    pub ai_input: i8,
    pub search: i8,
}

extern "C" {
    fn robots_matcher_create() -> *mut RobotsMatcherOpaque;
    fn robots_matcher_free(matcher: *mut RobotsMatcherOpaque);

    fn robots_allowed_by_robots(
        matcher: *mut RobotsMatcherOpaque,
        robots_txt: *const c_char,
        robots_txt_len: usize,
        user_agent: *const c_char,
        user_agent_len: usize,
        url: *const c_char,
        url_len: usize,
    ) -> bool;

    fn robots_matching_line(matcher: *const RobotsMatcherOpaque) -> c_int;
    fn robots_ever_seen_specific_agent(matcher: *const RobotsMatcherOpaque) -> bool;

    fn robots_has_crawl_delay(matcher: *const RobotsMatcherOpaque) -> bool;
    fn robots_get_crawl_delay(matcher: *const RobotsMatcherOpaque) -> c_double;

    fn robots_has_request_rate(matcher: *const RobotsMatcherOpaque) -> bool;
    fn robots_get_request_rate(
        matcher: *const RobotsMatcherOpaque,
        rate: *mut RequestRate,
    ) -> bool;

    fn robots_content_signal_supported() -> bool;
    fn robots_has_content_signal(matcher: *const RobotsMatcherOpaque) -> bool;
    fn robots_get_content_signal(
        matcher: *const RobotsMatcherOpaque,
        signal: *mut ContentSignal,
    ) -> bool;
    fn robots_allows_ai_train(matcher: *const RobotsMatcherOpaque) -> bool;
    fn robots_allows_ai_input(matcher: *const RobotsMatcherOpaque) -> bool;
    fn robots_allows_search(matcher: *const RobotsMatcherOpaque) -> bool;

    fn robots_is_valid_user_agent(user_agent: *const c_char, len: usize) -> bool;
    fn robots_version() -> *const c_char;
}

/// Returns the library version string.
pub fn version() -> String {
    unsafe {
        let ptr = robots_version();
        CStr::from_ptr(ptr).to_string_lossy().into_owned()
    }
}

/// Checks if a user-agent string contains only valid characters [a-zA-Z_-].
pub fn is_valid_user_agent(user_agent: &str) -> bool {
    let c_ua = CString::new(user_agent).unwrap_or_default();
    unsafe { robots_is_valid_user_agent(c_ua.as_ptr(), user_agent.len()) }
}

/// Returns true if Content-Signal support is compiled in.
pub fn content_signal_supported() -> bool {
    unsafe { robots_content_signal_supported() }
}

/// Robots.txt matcher - checks if URLs are allowed for given user-agents.
pub struct RobotsMatcher {
    ptr: *mut RobotsMatcherOpaque,
}

impl RobotsMatcher {
    /// Creates a new RobotsMatcher instance.
    pub fn new() -> Self {
        let ptr = unsafe { robots_matcher_create() };
        assert!(!ptr.is_null(), "Failed to create RobotsMatcher");
        Self { ptr }
    }

    /// Checks if a URL is allowed for a single user-agent.
    pub fn is_allowed(&self, robots_txt: &str, user_agent: &str, url: &str) -> bool {
        let c_robots = CString::new(robots_txt).unwrap_or_default();
        let c_ua = CString::new(user_agent).unwrap_or_default();
        let c_url = CString::new(url).unwrap_or_default();

        unsafe {
            robots_allowed_by_robots(
                self.ptr,
                c_robots.as_ptr(),
                robots_txt.len(),
                c_ua.as_ptr(),
                user_agent.len(),
                c_url.as_ptr(),
                url.len(),
            )
        }
    }

    /// Returns the line number that matched, or 0 if no match.
    pub fn matching_line(&self) -> i32 {
        unsafe { robots_matching_line(self.ptr) }
    }

    /// Returns true if a specific user-agent block was found (not just '*').
    pub fn ever_seen_specific_agent(&self) -> bool {
        unsafe { robots_ever_seen_specific_agent(self.ptr) }
    }

    /// Returns the crawl-delay in seconds, or None if not specified.
    pub fn crawl_delay(&self) -> Option<f64> {
        unsafe {
            if robots_has_crawl_delay(self.ptr) {
                Some(robots_get_crawl_delay(self.ptr))
            } else {
                None
            }
        }
    }

    /// Returns the request-rate, or None if not specified.
    pub fn request_rate(&self) -> Option<RequestRate> {
        unsafe {
            let mut rate = RequestRate {
                requests: 0,
                seconds: 0,
            };
            if robots_get_request_rate(self.ptr, &mut rate) {
                Some(rate)
            } else {
                None
            }
        }
    }

    /// Returns the content-signal values, or None if not specified.
    pub fn content_signal(&self) -> Option<ContentSignal> {
        unsafe {
            if !robots_content_signal_supported() {
                return None;
            }
            let mut signal = ContentSignal {
                ai_train: -1,
                ai_input: -1,
                search: -1,
            };
            if robots_get_content_signal(self.ptr, &mut signal) {
                Some(signal)
            } else {
                None
            }
        }
    }

    /// Returns true if AI training is allowed (defaults to true if not specified).
    pub fn allows_ai_train(&self) -> bool {
        unsafe { robots_allows_ai_train(self.ptr) }
    }

    /// Returns true if AI input is allowed (defaults to true if not specified).
    pub fn allows_ai_input(&self) -> bool {
        unsafe { robots_allows_ai_input(self.ptr) }
    }

    /// Returns true if search indexing is allowed (defaults to true if not specified).
    pub fn allows_search(&self) -> bool {
        unsafe { robots_allows_search(self.ptr) }
    }
}

impl Default for RobotsMatcher {
    fn default() -> Self {
        Self::new()
    }
}

impl Drop for RobotsMatcher {
    fn drop(&mut self) {
        if !self.ptr.is_null() {
            unsafe {
                robots_matcher_free(self.ptr);
            }
        }
    }
}

// RobotsMatcher is Send + Sync safe because the underlying C++ implementation
// is thread-safe for read operations after parsing.
unsafe impl Send for RobotsMatcher {}
unsafe impl Sync for RobotsMatcher {}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_version() {
        let v = version();
        assert!(!v.is_empty());
    }

    #[test]
    fn test_is_valid_user_agent() {
        assert!(is_valid_user_agent("Googlebot"));
        assert!(is_valid_user_agent("My-Bot"));
        assert!(!is_valid_user_agent("Bot/1.0"));
    }

    #[test]
    fn test_basic_allow() {
        let m = RobotsMatcher::new();
        let robots = "User-agent: *\nAllow: /\n";
        assert!(m.is_allowed(robots, "Googlebot", "https://example.com/page"));
    }

    #[test]
    fn test_basic_disallow() {
        let m = RobotsMatcher::new();
        let robots = "User-agent: *\nDisallow: /admin/\n";
        assert!(!m.is_allowed(robots, "Googlebot", "https://example.com/admin/secret"));
        assert!(m.is_allowed(robots, "Googlebot", "https://example.com/public"));
    }

    #[test]
    fn test_crawl_delay() {
        let m = RobotsMatcher::new();
        let robots = "User-agent: *\nCrawl-delay: 2.5\nDisallow:\n";
        m.is_allowed(robots, "Googlebot", "https://example.com/");
        assert_eq!(m.crawl_delay(), Some(2.5));
    }
}
