// Single-header demo with reporting for robotstxt library.
//
// Build:
//   c++ -std=c++20 -O2 demo_reporting.cpp -o demo_reporting
//

#define ROBOTS_IMPLEMENTATION
#include "reporting_robots.h"

#include <iostream>
#include <string>

int main() {
  const std::string robots_txt = R"(
User-agent: *
Disalow: /typo/
Disallow: /secret/
Allow: /public/

Sitemap: https://example.com/sitemap.xml
Crawl-delay: 2
)";

  std::cout << "robots.txt:\n" << robots_txt << "\n";
  std::cout << "Parsing report:\n";
  std::cout << "----------------------------------------\n";

  googlebot::RobotsParsingReporter reporter;
  googlebot::ParseRobotsTxt(robots_txt, &reporter);

  std::cout << "Total lines: " << reporter.last_line_seen() << "\n";
  std::cout << "Valid directives: " << reporter.valid_directives() << "\n";
  std::cout << "Unused directives: " << reporter.unused_directives() << "\n\n";

  std::cout << "Line details:\n";
  for (const auto& line : reporter.parse_results()) {
    if (line.metadata.is_empty || line.metadata.is_comment) continue;

    std::cout << "  Line " << line.line_num << ": ";
    switch (line.tag_name) {
      case googlebot::RobotsParsedLine::kUserAgent:
        std::cout << "User-agent";
        break;
      case googlebot::RobotsParsedLine::kAllow:
        std::cout << "Allow";
        break;
      case googlebot::RobotsParsedLine::kDisallow:
        std::cout << "Disallow";
        break;
      case googlebot::RobotsParsedLine::kSitemap:
        std::cout << "Sitemap";
        break;
      case googlebot::RobotsParsedLine::kCrawlDelay:
        std::cout << "Crawl-delay";
        break;
      case googlebot::RobotsParsedLine::kRequestRate:
        std::cout << "Request-rate";
        break;
#if ROBOTS_SUPPORT_CONTENT_SIGNAL
      case googlebot::RobotsParsedLine::kContentSignal:
        std::cout << "Content-signal";
        break;
#endif
      case googlebot::RobotsParsedLine::kUnused:
        std::cout << "Unused";
        break;
      default:
        std::cout << "Unknown";
    }
    if (line.is_typo) {
      std::cout << " (typo)";
    }
    std::cout << "\n";
  }

  return 0;
}
