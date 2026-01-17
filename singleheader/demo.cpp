// Single-header demo for robotstxt library.
//
// Build:
//   c++ -std=c++20 -O2 demo.cpp -o demo
//
// Or with CMake:
//   cmake -B build && cmake --build build
//

#define ROBOTS_IMPLEMENTATION
#include "robots.h"

#include <iostream>
#include <string>

int main() {
  const std::string robots_txt = R"(
User-agent: *
Disallow: /admin/
Allow: /admin/public/
Crawl-delay: 1.5

User-agent: Googlebot
Allow: /
)";

  googlebot::RobotsMatcher matcher;

  // Test URLs
  struct TestCase {
    std::string user_agent;
    std::string url;
  };

  TestCase tests[] = {
      {"Googlebot", "https://example.com/admin/secret"},
      {"Googlebot", "https://example.com/page"},
      {"Bingbot", "https://example.com/admin/secret"},
      {"Bingbot", "https://example.com/admin/public/file"},
      {"Bingbot", "https://example.com/page"},
  };

  std::cout << "robots.txt:\n" << robots_txt << "\n";
  std::cout << "Results:\n";
  std::cout << "----------------------------------------\n";

  for (const auto& test : tests) {
    bool allowed = matcher.OneAgentAllowedByRobots(robots_txt, test.user_agent,
                                                    test.url);
    std::cout << test.user_agent << " -> " << test.url << "\n";
    std::cout << "  " << (allowed ? "ALLOWED" : "DISALLOWED") << "\n";
  }

  // Show crawl-delay for Bingbot (uses * rules)
  matcher.OneAgentAllowedByRobots(robots_txt, "Bingbot", "https://example.com/");
  auto delay = matcher.GetCrawlDelay();
  if (delay) {
    std::cout << "\nCrawl-delay for Bingbot: " << *delay << "s\n";
  }

  return 0;
}
