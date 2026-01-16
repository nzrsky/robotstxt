import XCTest
@testable import RobotsTxt

final class RobotsTxtTests: XCTestCase {
    func testVersion() {
        let version = robotsVersion()
        XCTAssertFalse(version.isEmpty)
    }

    func testValidUserAgent() {
        XCTAssertTrue(isValidUserAgent("Googlebot"))
        XCTAssertTrue(isValidUserAgent("My-Bot"))
        XCTAssertFalse(isValidUserAgent("Bot/1.0"))
    }

    func testBasicAllow() {
        let matcher = RobotsMatcher()
        let robotsTxt = """
        User-agent: *
        Allow: /
        """
        XCTAssertTrue(matcher.isAllowed(robotsTxt: robotsTxt, userAgent: "Googlebot", url: "https://example.com/page"))
    }

    func testBasicDisallow() {
        let matcher = RobotsMatcher()
        let robotsTxt = """
        User-agent: *
        Disallow: /admin/
        """
        XCTAssertFalse(matcher.isAllowed(robotsTxt: robotsTxt, userAgent: "Googlebot", url: "https://example.com/admin/secret"))
        XCTAssertTrue(matcher.isAllowed(robotsTxt: robotsTxt, userAgent: "Googlebot", url: "https://example.com/public"))
    }

    func testSpecificUserAgent() {
        let matcher = RobotsMatcher()
        let robotsTxt = """
        User-agent: Googlebot
        Disallow: /google-only/

        User-agent: *
        Disallow: /admin/
        """
        XCTAssertFalse(matcher.isAllowed(robotsTxt: robotsTxt, userAgent: "Googlebot", url: "https://example.com/google-only/"))
        XCTAssertTrue(matcher.isAllowed(robotsTxt: robotsTxt, userAgent: "Bingbot", url: "https://example.com/google-only/"))
        XCTAssertTrue(matcher.everSeenSpecificAgent)
    }

    func testCrawlDelay() {
        let matcher = RobotsMatcher()
        let robotsTxt = """
        User-agent: *
        Crawl-delay: 2.5
        Disallow:
        """
        _ = matcher.isAllowed(robotsTxt: robotsTxt, userAgent: "Googlebot", url: "https://example.com/")
        XCTAssertEqual(matcher.crawlDelay, 2.5)
    }

    func testRequestRate() {
        let matcher = RobotsMatcher()
        let robotsTxt = """
        User-agent: *
        Request-rate: 1/10
        Disallow:
        """
        _ = matcher.isAllowed(robotsTxt: robotsTxt, userAgent: "Googlebot", url: "https://example.com/")
        if let rate = matcher.requestRate {
            XCTAssertEqual(rate.requests, 1)
            XCTAssertEqual(rate.seconds, 10)
            XCTAssertEqual(rate.requestsPerSecond, 0.1, accuracy: 0.001)
            XCTAssertEqual(rate.delaySeconds, 10.0, accuracy: 0.001)
        } else {
            XCTFail("Expected request rate to be set")
        }
    }

    func testContentSignal() {
        guard RobotsMatcher.contentSignalSupported else {
            print("Content-Signal not supported, skipping test")
            return
        }

        let matcher = RobotsMatcher()
        let robotsTxt = """
        User-agent: *
        Content-Signal: ai-train=no, search=yes
        Disallow:
        """
        _ = matcher.isAllowed(robotsTxt: robotsTxt, userAgent: "Googlebot", url: "https://example.com/")
        XCTAssertFalse(matcher.allowsAITrain)
        XCTAssertTrue(matcher.allowsSearch)
    }

    func testMatchingLine() {
        let matcher = RobotsMatcher()
        let robotsTxt = """
        User-agent: *
        Disallow: /admin/
        """
        _ = matcher.isAllowed(robotsTxt: robotsTxt, userAgent: "Googlebot", url: "https://example.com/admin/secret")
        XCTAssertEqual(matcher.matchingLine, 2)
    }

    func testEmptyRobotsTxt() {
        let matcher = RobotsMatcher()
        XCTAssertTrue(matcher.isAllowed(robotsTxt: "", userAgent: "Googlebot", url: "https://example.com/anything"))
    }

    static var allTests = [
        ("testVersion", testVersion),
        ("testValidUserAgent", testValidUserAgent),
        ("testBasicAllow", testBasicAllow),
        ("testBasicDisallow", testBasicDisallow),
        ("testSpecificUserAgent", testSpecificUserAgent),
        ("testCrawlDelay", testCrawlDelay),
        ("testRequestRate", testRequestRate),
        ("testContentSignal", testContentSignal),
        ("testMatchingLine", testMatchingLine),
        ("testEmptyRobotsTxt", testEmptyRobotsTxt),
    ]
}
