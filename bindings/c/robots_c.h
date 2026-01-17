// Copyright 1999 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// -----------------------------------------------------------------------------
// File: robots_c.h
// -----------------------------------------------------------------------------
//
// C API for the robots.txt parser and matcher library.
//
// Example usage:
//
//   robots_matcher_t* matcher = robots_matcher_create();
//
//   const char* robots_txt = "User-agent: *\nDisallow: /admin/\n";
//   const char* user_agent = "Googlebot";
//   const char* url = "https://example.com/admin/secret";
//
//   bool allowed = robots_allowed_by_robots(
//       matcher, robots_txt, strlen(robots_txt),
//       user_agent, strlen(user_agent),
//       url, strlen(url));
//
//   printf("Access: %s\n", allowed ? "allowed" : "disallowed");
//
//   robots_matcher_free(matcher);
//

#ifndef ROBOTS_C_H
#define ROBOTS_C_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// DLL export/import macros for Windows
#if defined(_WIN32) || defined(_WIN64)
  #ifdef DLL_EXPORT
    #define ROBOTS_API __declspec(dllexport)
  #else
    #define ROBOTS_API __declspec(dllimport)
  #endif
#else
  #define ROBOTS_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Types
// =============================================================================

// Opaque pointer to RobotsMatcher instance.
typedef struct robots_matcher_s robots_matcher_t;

// Request-rate value (requests per time period).
typedef struct {
  int requests;  // Number of requests allowed
  int seconds;   // Time period in seconds
} robots_request_rate_t;

// Content-Signal values for AI content preferences.
// Each field uses a tri-state: -1 = not set, 0 = no, 1 = yes.
typedef struct {
  int8_t ai_train;  // ai-train: Training or fine-tuning AI models
  int8_t ai_input;  // ai-input: Using content in AI models for real-time generation
  int8_t search;    // search: Building search indexes and providing results
} robots_content_signal_t;

// =============================================================================
// Matcher lifecycle
// =============================================================================

// Creates a new RobotsMatcher instance.
// Returns NULL on allocation failure.
// Caller must free with robots_matcher_free().
ROBOTS_API robots_matcher_t* robots_matcher_create(void);

// Frees a RobotsMatcher instance.
// Safe to call with NULL.
ROBOTS_API void robots_matcher_free(robots_matcher_t* matcher);

// =============================================================================
// URL checking
// =============================================================================

// Checks if a URL is allowed for a single user-agent.
//
// Parameters:
//   matcher:          RobotsMatcher instance (will be modified)
//   robots_txt:       robots.txt content
//   robots_txt_len:   length of robots_txt
//   user_agent:       user-agent string to check
//   user_agent_len:   length of user_agent
//   url:              URL to check (must be %-encoded per RFC3986)
//   url_len:          length of url
//
// Returns true if the URL is allowed, false if disallowed.
ROBOTS_API bool robots_allowed_by_robots(
    robots_matcher_t* matcher,
    const char* robots_txt, size_t robots_txt_len,
    const char* user_agent, size_t user_agent_len,
    const char* url, size_t url_len);

// Checks if a URL is allowed for multiple user-agents.
// Rules from all matching user-agents are combined.
//
// Parameters:
//   matcher:          RobotsMatcher instance (will be modified)
//   robots_txt:       robots.txt content
//   robots_txt_len:   length of robots_txt
//   user_agents:      array of user-agent strings
//   user_agent_lens:  array of user-agent string lengths
//   num_user_agents:  number of user-agents
//   url:              URL to check (must be %-encoded per RFC3986)
//   url_len:          length of url
//
// Returns true if the URL is allowed, false if disallowed.
ROBOTS_API bool robots_allowed_by_robots_multi(
    robots_matcher_t* matcher,
    const char* robots_txt, size_t robots_txt_len,
    const char* const* user_agents, const size_t* user_agent_lens,
    size_t num_user_agents,
    const char* url, size_t url_len);

// =============================================================================
// Matcher state accessors (call after robots_allowed_by_robots)
// =============================================================================

// Returns the line number that matched, or 0 if no match.
ROBOTS_API int robots_matching_line(const robots_matcher_t* matcher);

// Returns true if a specific user-agent block was found (not just '*').
ROBOTS_API bool robots_ever_seen_specific_agent(const robots_matcher_t* matcher);

// =============================================================================
// Crawl-delay support (non-standard directive)
// =============================================================================

// Returns true if a crawl-delay was specified for the matched user-agent.
ROBOTS_API bool robots_has_crawl_delay(const robots_matcher_t* matcher);

// Returns the crawl-delay in seconds, or 0.0 if not specified.
// Call robots_has_crawl_delay() first to distinguish "not set" from "0".
ROBOTS_API double robots_get_crawl_delay(const robots_matcher_t* matcher);

// =============================================================================
// Request-rate support (non-standard directive)
// =============================================================================

// Returns true if a request-rate was specified for the matched user-agent.
ROBOTS_API bool robots_has_request_rate(const robots_matcher_t* matcher);

// Gets the request-rate value. Returns false if not specified.
// On success, fills in the rate struct and returns true.
ROBOTS_API bool robots_get_request_rate(const robots_matcher_t* matcher,
                                         robots_request_rate_t* rate);

// =============================================================================
// Content-Signal support (proposed AI directive)
// =============================================================================

// Returns true if Content-Signal directive support is compiled in.
ROBOTS_API bool robots_content_signal_supported(void);

// Returns true if a content-signal was specified for the matched user-agent.
ROBOTS_API bool robots_has_content_signal(const robots_matcher_t* matcher);

// Gets the content-signal values. Returns false if not specified.
// On success, fills in the signal struct and returns true.
// Each field is: -1 = not set, 0 = no, 1 = yes.
ROBOTS_API bool robots_get_content_signal(const robots_matcher_t* matcher,
                                           robots_content_signal_t* signal);

// Convenience functions for content-signal (return default true if not set).
ROBOTS_API bool robots_allows_ai_train(const robots_matcher_t* matcher);
ROBOTS_API bool robots_allows_ai_input(const robots_matcher_t* matcher);
ROBOTS_API bool robots_allows_search(const robots_matcher_t* matcher);

// =============================================================================
// Utility functions
// =============================================================================

// Validates that a user-agent string contains only valid characters [a-zA-Z_-].
ROBOTS_API bool robots_is_valid_user_agent(const char* user_agent, size_t len);

// Returns the library version string.
ROBOTS_API const char* robots_version(void);

#ifdef __cplusplus
}
#endif

#endif  // ROBOTS_C_H
