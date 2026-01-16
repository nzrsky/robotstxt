// C API demo using single-header version.
// Note: Implementation requires C++ compilation, but API is callable from C.
//
// Build:
//   c++ -std=c++20 -O2 demo_c.cpp -o demo_c
//

#define ROBOTS_IMPLEMENTATION
#include "robots_c.h"

#include <cstdio>
#include <cstring>

int main() {
  const char* robots_txt =
      "User-agent: *\n"
      "Disallow: /admin/\n"
      "Crawl-delay: 2.5\n"
      "Content-Signal: ai-train=no, search=yes\n";

  printf("robots.txt:\n%s\n", robots_txt);
  printf("Library version: %s\n\n", robots_version());

  robots_matcher_t* matcher = robots_matcher_create();
  if (!matcher) {
    fprintf(stderr, "Failed to create matcher\n");
    return 1;
  }

  // Test
  const char* user_agent = "Bingbot";
  const char* url = "https://example.com/admin/secret";

  bool allowed = robots_allowed_by_robots(
      matcher,
      robots_txt, strlen(robots_txt),
      user_agent, strlen(user_agent),
      url, strlen(url));

  printf("%s -> %s: %s\n", user_agent, url, allowed ? "ALLOWED" : "DISALLOWED");

  if (robots_has_crawl_delay(matcher)) {
    printf("Crawl-delay: %.1f seconds\n", robots_get_crawl_delay(matcher));
  }

  if (robots_content_signal_supported() && robots_has_content_signal(matcher)) {
    printf("AI training allowed: %s\n",
           robots_allows_ai_train(matcher) ? "yes" : "no");
  }

  robots_matcher_free(matcher);
  return 0;
}
