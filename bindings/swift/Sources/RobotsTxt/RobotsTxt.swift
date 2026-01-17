/// Swift bindings for Google's robots.txt parser library.
///
/// Example usage:
/// ```swift
/// let matcher = RobotsMatcher()
/// let robotsTxt = """
/// User-agent: *
/// Disallow: /admin/
/// """
/// let allowed = matcher.isAllowed(robotsTxt: robotsTxt, userAgent: "Googlebot", url: "https://example.com/page")
/// print("Access: \(allowed ? "allowed" : "disallowed")")
/// ```

import Foundation
import CRobots

// MARK: - Types

/// Request-rate value (requests per time period).
public struct RequestRate {
    public let requests: Int
    public let seconds: Int

    /// Returns requests per second.
    public var requestsPerSecond: Double {
        guard seconds > 0 else { return 0 }
        return Double(requests) / Double(seconds)
    }

    /// Returns delay between requests in seconds.
    public var delaySeconds: Double {
        guard requests > 0 else { return 0 }
        return Double(seconds) / Double(requests)
    }
}

/// Content-Signal values for AI content preferences.
public struct ContentSignal {
    /// AI training allowed (nil = not set, true = yes, false = no)
    public let aiTrain: Bool?
    /// AI input allowed (nil = not set, true = yes, false = no)
    public let aiInput: Bool?
    /// Search indexing allowed (nil = not set, true = yes, false = no)
    public let search: Bool?
}

/// Robots.txt matcher - checks if URLs are allowed for given user-agents.
public final class RobotsMatcher {
    private var handle: OpaquePointer?

    /// Creates a new RobotsMatcher instance.
    public init() {
        handle = robots_matcher_create()
    }

    deinit {
        if let handle = handle {
            robots_matcher_free(handle)
        }
    }

    /// Checks if a URL is allowed for a single user-agent.
    /// - Parameters:
    ///   - robotsTxt: The robots.txt content.
    ///   - userAgent: The user-agent string to check.
    ///   - url: The URL to check (should be %-encoded per RFC3986).
    /// - Returns: True if the URL is allowed, false otherwise.
    public func isAllowed(robotsTxt: String, userAgent: String, url: String) -> Bool {
        guard let handle = handle else { return true }

        return robotsTxt.withCString { robotsPtr in
            userAgent.withCString { uaPtr in
                url.withCString { urlPtr in
                    robots_allowed_by_robots(
                        handle,
                        robotsPtr, robotsTxt.utf8.count,
                        uaPtr, userAgent.utf8.count,
                        urlPtr, url.utf8.count
                    )
                }
            }
        }
    }

    /// Returns the line number that matched, or 0 if no match.
    public var matchingLine: Int {
        guard let handle = handle else { return 0 }
        return Int(robots_matching_line(handle))
    }

    /// Returns true if a specific user-agent block was found (not just '*').
    public var everSeenSpecificAgent: Bool {
        guard let handle = handle else { return false }
        return robots_ever_seen_specific_agent(handle)
    }

    /// Returns the crawl-delay in seconds, or nil if not specified.
    public var crawlDelay: Double? {
        guard let handle = handle else { return nil }
        guard robots_has_crawl_delay(handle) else { return nil }
        return robots_get_crawl_delay(handle)
    }

    /// Returns the request-rate, or nil if not specified.
    public var requestRate: RequestRate? {
        guard let handle = handle else { return nil }
        var rate = robots_request_rate_t()
        guard robots_get_request_rate(handle, &rate) else { return nil }
        return RequestRate(requests: Int(rate.requests), seconds: Int(rate.seconds))
    }

    /// Returns true if Content-Signal support is compiled in.
    public static var contentSignalSupported: Bool {
        return robots_content_signal_supported()
    }

    /// Returns the content-signal values, or nil if not specified.
    public var contentSignal: ContentSignal? {
        guard let handle = handle else { return nil }
        guard Self.contentSignalSupported else { return nil }

        var signal = robots_content_signal_t()
        guard robots_get_content_signal(handle, &signal) else { return nil }

        func triState(_ v: Int8) -> Bool? {
            switch v {
            case -1: return nil
            case 0: return false
            case 1: return true
            default: return nil
            }
        }

        return ContentSignal(
            aiTrain: triState(signal.ai_train),
            aiInput: triState(signal.ai_input),
            search: triState(signal.search)
        )
    }

    /// Returns true if AI training is allowed (defaults to true if not specified).
    public var allowsAITrain: Bool {
        guard let handle = handle else { return true }
        return robots_allows_ai_train(handle)
    }

    /// Returns true if AI input is allowed (defaults to true if not specified).
    public var allowsAIInput: Bool {
        guard let handle = handle else { return true }
        return robots_allows_ai_input(handle)
    }

    /// Returns true if search indexing is allowed (defaults to true if not specified).
    public var allowsSearch: Bool {
        guard let handle = handle else { return true }
        return robots_allows_search(handle)
    }
}

// MARK: - Utility Functions

/// Returns the library version string.
public func robotsVersion() -> String {
    guard let ptr = robots_version() else { return "" }
    return String(cString: ptr)
}

/// Checks if a user-agent string contains only valid characters [a-zA-Z_-].
public func isValidUserAgent(_ userAgent: String) -> Bool {
    return userAgent.withCString { ptr in
        robots_is_valid_user_agent(ptr, userAgent.utf8.count)
    }
}
