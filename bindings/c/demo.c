// C API demo for robotstxt library.
//
// Build (after cmake build):
//   cc -o robots_c_demo robots_c_demo.c -L./cmake-build -lrobots -lstdc++
//
// Or link with the static library.

#include "robots_c.h"
#include <stdio.h>
#include <string.h>

int main(void) {
  const char* robots_txt =
      "User-agent: *\n"
      "Disallow: /admin/\n"
      "Allow: /admin/public/\n"
      "Crawl-delay: 2.5\n"
      "Request-rate: 1/10\n"
      "Content-Signal: ai-train=no, search=yes\n"
      "\n"
      "User-agent: Googlebot\n"
      "Allow: /\n";

  printf("robots.txt:\n%s\n", robots_txt);
  printf("Library version: %s\n\n", robots_version());

  // Create matcher
  robots_matcher_t* matcher = robots_matcher_create();
  if (!matcher) {
    fprintf(stderr, "Failed to create matcher\n");
    return 1;
  }

  // Test cases
  struct {
    const char* user_agent;
    const char* url;
  } tests[] = {
      {"Googlebot", "https://example.com/admin/secret"},
      {"Googlebot", "https://example.com/page"},
      {"Bingbot", "https://example.com/admin/secret"},
      {"Bingbot", "https://example.com/admin/public/file"},
      {"Bingbot", "https://example.com/page"},
  };

  size_t num_tests = sizeof(tests) / sizeof(tests[0]);

  printf("Results:\n");
  printf("----------------------------------------\n");

  for (size_t i = 0; i < num_tests; i++) {
    bool allowed = robots_allowed_by_robots(
        matcher,
        robots_txt, strlen(robots_txt),
        tests[i].user_agent, strlen(tests[i].user_agent),
        tests[i].url, strlen(tests[i].url));

    printf("%s -> %s\n", tests[i].user_agent, tests[i].url);
    printf("  %s (line %d)\n",
           allowed ? "ALLOWED" : "DISALLOWED",
           robots_matching_line(matcher));
  }

  // Check Bingbot to get * rules
  robots_allowed_by_robots(
      matcher,
      robots_txt, strlen(robots_txt),
      "Bingbot", 7,
      "https://example.com/", 20);

  printf("\n");

  // Crawl-delay
  if (robots_has_crawl_delay(matcher)) {
    printf("Crawl-delay: %.1f seconds\n", robots_get_crawl_delay(matcher));
  }

  // Request-rate
  robots_request_rate_t rate;
  if (robots_get_request_rate(matcher, &rate)) {
    printf("Request-rate: %d requests per %d seconds\n",
           rate.requests, rate.seconds);
  }

  // Content-Signal
  if (robots_content_signal_supported()) {
    robots_content_signal_t signal;
    if (robots_get_content_signal(matcher, &signal)) {
      printf("Content-Signal:\n");
      printf("  ai-train: %s\n",
             signal.ai_train == -1 ? "not set" :
             signal.ai_train == 1 ? "yes" : "no");
      printf("  ai-input: %s\n",
             signal.ai_input == -1 ? "not set" :
             signal.ai_input == 1 ? "yes" : "no");
      printf("  search: %s\n",
             signal.search == -1 ? "not set" :
             signal.search == 1 ? "yes" : "no");
    }
  }

  // Cleanup
  robots_matcher_free(matcher);

  return 0;
}
