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

//
// *** AMALGAMATED SINGLE-HEADER VERSION ***
// Generated: 2026-01-16 23:01:33 +0200
// Commit: 0e413cb
//
// This file is auto-generated. Do not edit directly.
// Run: python3 singleheader/amalgamate.py
//

#ifndef ROBOTS_C_H
#define ROBOTS_C_H

#ifdef __cplusplus
// === Begin embedded robots.h (C++ only) ===
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
// File: robots.h
// -----------------------------------------------------------------------------
//
// This file implements the standard defined by the Robots Exclusion Protocol
// (REP) internet draft (I-D).
//   https://www.rfc-editor.org/rfc/rfc9309.html
//
// Google doesn't follow the standard strictly, because there are a lot of
// non-conforming robots.txt files out there, and we err on the side of
// disallowing when this seems intended.
//
// An more user-friendly description of how Google handles robots.txt can be
// found at:
//   https://developers.google.com/search/docs/crawling-indexing/robots/robots_txt
//
// This library provides a low-level parser for robots.txt (ParseRobotsTxt()),
// and a matcher for URLs against a robots.txt (class RobotsMatcher).


#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Content-Signal directive support (proposed for AI content preferences).
// Define ROBOTS_SUPPORT_CONTENT_SIGNAL=0 to disable for smaller binary/faster parsing.
// Default: enabled (1)
#ifndef ROBOTS_SUPPORT_CONTENT_SIGNAL
#define ROBOTS_SUPPORT_CONTENT_SIGNAL 1
#endif

namespace googlebot {

// Request-rate directive value: requests per time period.
// Format in robots.txt: "Request-rate: requests/seconds" (e.g., "1/5" = 1 request per 5 seconds)
struct RequestRate {
  int requests = 1;    // Number of requests allowed
  int seconds = 1;     // Time period in seconds

  // Returns requests per second as a double.
  double RequestsPerSecond() const {
    return seconds > 0 ? static_cast<double>(requests) / seconds : 0.0;
  }

  // Returns minimum delay between requests in seconds.
  double DelaySeconds() const {
    return requests > 0 ? static_cast<double>(seconds) / requests : 0.0;
  }
};

#if ROBOTS_SUPPORT_CONTENT_SIGNAL
// Content-Signal directive value: AI content preferences.
// Format in robots.txt: "Content-Signal: ai-train=no, search=yes, ai-input=yes"
// See: https://github.com/nicksellen/cc-signals
struct ContentSignal {
  std::optional<bool> ai_train;   // ai-train: Training or fine-tuning AI models
  std::optional<bool> ai_input;   // ai-input: Using content in AI models for real-time generation
  std::optional<bool> search;     // search: Building search indexes and providing results

  // Returns true if any signal is set.
  bool HasAnySignal() const {
    return ai_train.has_value() || ai_input.has_value() || search.has_value();
  }

  // Returns true if AI training is allowed (defaults to true if not specified).
  bool AllowsAiTrain() const {
    return ai_train.value_or(true);
  }

  // Returns true if AI input is allowed (defaults to true if not specified).
  bool AllowsAiInput() const {
    return ai_input.value_or(true);
  }

  // Returns true if search indexing is allowed (defaults to true if not specified).
  bool AllowsSearch() const {
    return search.value_or(true);
  }
};
#endif  // ROBOTS_SUPPORT_CONTENT_SIGNAL
// Handler for directives found in robots.txt. These callbacks are called by
// ParseRobotsTxt() in the sequence they have been found in the file.
class RobotsParseHandler {
 public:
  RobotsParseHandler() = default;
  virtual ~RobotsParseHandler() = default;

  // Disallow copying and assignment.
  RobotsParseHandler(const RobotsParseHandler&) = delete;
  RobotsParseHandler& operator=(const RobotsParseHandler&) = delete;

  virtual void HandleRobotsStart() = 0;
  virtual void HandleRobotsEnd() = 0;

  virtual void HandleUserAgent(int line_num, std::string_view value) = 0;
  virtual void HandleAllow(int line_num, std::string_view value) = 0;
  virtual void HandleDisallow(int line_num, std::string_view value) = 0;

  virtual void HandleSitemap(int line_num, std::string_view value) = 0;

  // Crawl-delay directive (non-standard but widely used).
  // Value is in seconds. Note: Google ignores this directive.
  virtual void HandleCrawlDelay(int line_num, double value) = 0;

  // Request-rate directive (non-standard, used by Bing and others).
  // Format: "requests/seconds" (e.g., "1/5" = 1 request per 5 seconds).
  virtual void HandleRequestRate(int line_num, const RequestRate& rate) = 0;

#if ROBOTS_SUPPORT_CONTENT_SIGNAL
  // Content-Signal directive (proposed for AI content preferences).
  // Format: "ai-train=no, search=yes, ai-input=yes"
  // See: https://github.com/nicksellen/cc-signals
  virtual void HandleContentSignal(int line_num, const ContentSignal& signal) = 0;
#endif  // ROBOTS_SUPPORT_CONTENT_SIGNAL

  // Any other unrecognized name/value pairs.
  virtual void HandleUnknownAction(int line_num, std::string_view action,
                                   std::string_view value) = 0;

  struct LineMetadata {
    // Indicates if the line is totally empty.
    bool is_empty = false;
    // Indicates if the line has a comment (may have content before it).
    bool has_comment = false;
    // Indicates if the whole line is a comment.
    bool is_comment = false;
    // Indicates that the line has a valid robots.txt directive and one of the
    // `Handle*` methods will be called.
    bool has_directive = false;
    // Indicates that the found directive is one of the acceptable typo variants
    // of the directive. See the key functions in ParsedRobotsKey for accepted
    // typos.
    bool is_acceptable_typo = false;
    // Indicates that the line is too long, specifically over 2083 * 8 bytes.
    bool is_line_too_long = false;
    // Indicates that the key-value pair is missing the colon separator.
    bool is_missing_colon_separator = false;
  };

  virtual void ReportLineMetadata(int line_num, const LineMetadata& metadata) {}
};

// Parses body of a robots.txt and emits parse callbacks. This will accept
// typical typos found in robots.txt, such as 'disalow'.
//
// Note, this function will accept all kind of input but will skip
// everything that does not look like a robots directive.
void ParseRobotsTxt(std::string_view robots_body,
                    RobotsParseHandler* parse_callback);

// RobotsMatcher - matches robots.txt against URLs.
//
// The Matcher uses a default match strategy for Allow/Disallow patterns which
// is the official way of Google crawler to match robots.txt. It is also
// possible to provide a custom match strategy.
//
// The entry point for the user is to call one of the *AllowedByRobots()
// methods that return directly if a URL is being allowed according to the
// robots.txt and the crawl agent.
// The RobotsMatcher can be re-used for URLs/robots.txt but is not thread-safe.
class RobotsMatchStrategy;
class RobotsMatcher : protected RobotsParseHandler {
 public:
  // Create a RobotsMatcher with the default matching strategy. The default
  // matching strategy is longest-match as opposed to the former internet draft
  // that provisioned first-match strategy. Analysis shows that longest-match,
  // while more restrictive for crawlers, is what webmasters assume when writing
  // directives. For example, in case of conflicting matches (both Allow and
  // Disallow), the longest match is the one the user wants. For example, in
  // case of a robots.txt file that has the following rules
  //   Allow: /
  //   Disallow: /cgi-bin
  // it's pretty obvious what the webmaster wants: they want to allow crawl of
  // every URI except /cgi-bin. However, according to the expired internet
  // standard, crawlers should be allowed to crawl everything with such a rule.
  RobotsMatcher();

  ~RobotsMatcher() override;

  // Disallow copying and assignment.
  RobotsMatcher(const RobotsMatcher&) = delete;
  RobotsMatcher& operator=(const RobotsMatcher&) = delete;

  // Verifies that the given user agent is valid to be matched against
  // robots.txt. Valid user agent strings only contain the characters
  // [a-zA-Z_-].
  static bool IsValidUserAgentToObey(std::string_view user_agent);

  // Returns true iff 'url' is allowed to be fetched by any member of the
  // "user_agents" vector after collapsing all rules applying to any member of 
  // the "user_agents" vector into a single ruleset. 'url' must be %-encoded 
  // according to RFC3986.
  bool AllowedByRobots(std::string_view robots_body,
                       const std::vector<std::string>* user_agents,
                       const std::string& url);

  // Do robots check for 'url' when there is only one user agent. 'url' must
  // be %-encoded according to RFC3986.
  bool OneAgentAllowedByRobots(std::string_view robots_txt,
                               const std::string& user_agent,
                               const std::string& url);

  // Returns true if we are disallowed from crawling a matching URI.
  bool disallow() const;

  // Returns true if we are disallowed from crawling a matching URI. Ignores any
  // rules specified for the default user agent, and bases its results only on
  // the specified user agents.
  bool disallow_ignore_global() const;

  // Returns true iff, when AllowedByRobots() was called, the robots file
  // referred explicitly to one of the specified user agents.
  bool ever_seen_specific_agent() const;

  // Returns the line that matched or 0 if none matched.
  int matching_line() const;

  // Returns the crawl-delay value in seconds for the matched user-agent.
  // Returns std::nullopt if no crawl-delay was specified.
  // Note: This is a non-standard directive that Google ignores, but other
  // crawlers may use it.
  std::optional<double> GetCrawlDelay() const;

  // Returns the request-rate for the matched user-agent.
  // Returns std::nullopt if no request-rate was specified.
  // Note: This is a non-standard directive that Google ignores.
  std::optional<RequestRate> GetRequestRate() const;

#if ROBOTS_SUPPORT_CONTENT_SIGNAL
  // Returns the content-signal for the matched user-agent.
  // Returns std::nullopt if no content-signal was specified.
  // Note: This is a proposed directive for AI content preferences.
  std::optional<ContentSignal> GetContentSignal() const;
#endif  // ROBOTS_SUPPORT_CONTENT_SIGNAL

 protected:
  // Parse callbacks.
  // Protected because used in unittest. Never override RobotsMatcher, implement
  // googlebot::RobotsParseHandler instead.
  void HandleRobotsStart() override;
  void HandleRobotsEnd() override {}

  void HandleUserAgent(int line_num, std::string_view user_agent) override;
  void HandleAllow(int line_num, std::string_view value) override;
  void HandleDisallow(int line_num, std::string_view value) override;

  void HandleSitemap(int line_num, std::string_view value) override;
  void HandleCrawlDelay(int line_num, double value) override;
  void HandleRequestRate(int line_num, const RequestRate& rate) override;
#if ROBOTS_SUPPORT_CONTENT_SIGNAL
  void HandleContentSignal(int line_num, const ContentSignal& signal) override;
#endif  // ROBOTS_SUPPORT_CONTENT_SIGNAL
  void HandleUnknownAction(int line_num, std::string_view action,
                           std::string_view value) override;

 protected:
  // Extract the matchable part of a user agent string, essentially stopping at
  // the first invalid character.
  // Example: 'Googlebot/2.1' becomes 'Googlebot'
  static std::string_view ExtractUserAgent(std::string_view user_agent);

  // Initialize next path and user-agents to check. Path must contain only the
  // path, params, and query (if any) of the url and must start with a '/'.
  void InitUserAgentsAndPath(const std::vector<std::string>* user_agents,
                             const char* path);

  // Returns true if any user-agent was seen.
  bool seen_any_agent() const {
    return seen_global_agent_ || seen_specific_agent_;
  }

  // Instead of just maintaining a Boolean indicating whether a given line has
  // matched, we maintain a count of the maximum number of characters matched by
  // that pattern.
  //
  // This structure stores the information associated with a match (e.g. when a
  // Disallow is matched) as priority of the match and line matching.
  //
  // The priority is initialized with a negative value to make sure that a match
  // of priority 0 is higher priority than no match at all.
  class Match {
   private:
    static const int kNoMatchPriority = -1;

   public:
    Match(int priority, int line) : priority_(priority), line_(line) {}
    Match() : priority_(kNoMatchPriority), line_(0) {}

    void Set(int priority, int line) {
      priority_ = priority;
      line_ = line;
    }

    void Clear() { Set(kNoMatchPriority, 0); }

    int line() const { return line_; }
    int priority() const { return priority_; }

    static const Match& HigherPriorityMatch(const Match& a, const Match& b) {
      if (a.priority() > b.priority()) {
        return a;
      } else {
        return b;
      }
    }

   private:
    int priority_;
    int line_;
  };

  // For each of the directives within user-agents, we keep global and specific
  // match scores.
  struct MatchHierarchy {
    Match global;    // Match for '*'
    Match specific;  // Match for queried agent.
    void Clear() {
      global.Clear();
      specific.Clear();
    }
  };
  MatchHierarchy allow_;     // Characters of 'url' matching Allow.
  MatchHierarchy disallow_;  // Characters of 'url' matching Disallow.

  bool seen_global_agent_;         // True if processing global agent rules.
  bool seen_specific_agent_;       // True if processing our specific agent.
  bool ever_seen_specific_agent_;  // True if we ever saw a block for our agent.
  bool seen_separator_;            // True if saw any key: value pair.

  // Length of the most specific user-agent we've matched so far.
  // Used to implement "most specific wins" rule per Google's documentation.
  size_t best_specific_agent_length_;

  // The path we want to pattern match. Not owned and only a valid pointer
  // during the lifetime of *AllowedByRobots calls.
  const char* path_;
  // The User-Agents we are interested in. Not owned and only a valid
  // pointer during the lifetime of *AllowedByRobots calls.
  const std::vector<std::string>* user_agents_;

  RobotsMatchStrategy* match_strategy_;

  // Crawl-delay values for global (*) and specific user-agent groups.
  // Uses std::optional to distinguish between "not set" and "set to 0".
  std::optional<double> crawl_delay_global_;
  std::optional<double> crawl_delay_specific_;

  // Request-rate values for global (*) and specific user-agent groups.
  std::optional<RequestRate> request_rate_global_;
  std::optional<RequestRate> request_rate_specific_;

#if ROBOTS_SUPPORT_CONTENT_SIGNAL
  // Content-signal values for global (*) and specific user-agent groups.
  std::optional<ContentSignal> content_signal_global_;
  std::optional<ContentSignal> content_signal_specific_;
#endif  // ROBOTS_SUPPORT_CONTENT_SIGNAL
};

}  // namespace googlebot
// === End embedded robots.h ===
#endif  // __cplusplus


#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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
robots_matcher_t* robots_matcher_create(void);

// Frees a RobotsMatcher instance.
// Safe to call with NULL.
void robots_matcher_free(robots_matcher_t* matcher);

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
bool robots_allowed_by_robots(
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
bool robots_allowed_by_robots_multi(
    robots_matcher_t* matcher,
    const char* robots_txt, size_t robots_txt_len,
    const char* const* user_agents, const size_t* user_agent_lens,
    size_t num_user_agents,
    const char* url, size_t url_len);

// =============================================================================
// Matcher state accessors (call after robots_allowed_by_robots)
// =============================================================================

// Returns the line number that matched, or 0 if no match.
int robots_matching_line(const robots_matcher_t* matcher);

// Returns true if a specific user-agent block was found (not just '*').
bool robots_ever_seen_specific_agent(const robots_matcher_t* matcher);

// =============================================================================
// Crawl-delay support (non-standard directive)
// =============================================================================

// Returns true if a crawl-delay was specified for the matched user-agent.
bool robots_has_crawl_delay(const robots_matcher_t* matcher);

// Returns the crawl-delay in seconds, or 0.0 if not specified.
// Call robots_has_crawl_delay() first to distinguish "not set" from "0".
double robots_get_crawl_delay(const robots_matcher_t* matcher);

// =============================================================================
// Request-rate support (non-standard directive)
// =============================================================================

// Returns true if a request-rate was specified for the matched user-agent.
bool robots_has_request_rate(const robots_matcher_t* matcher);

// Gets the request-rate value. Returns false if not specified.
// On success, fills in the rate struct and returns true.
bool robots_get_request_rate(const robots_matcher_t* matcher,
                              robots_request_rate_t* rate);

// =============================================================================
// Content-Signal support (proposed AI directive)
// =============================================================================

// Returns true if Content-Signal directive support is compiled in.
bool robots_content_signal_supported(void);

// Returns true if a content-signal was specified for the matched user-agent.
bool robots_has_content_signal(const robots_matcher_t* matcher);

// Gets the content-signal values. Returns false if not specified.
// On success, fills in the signal struct and returns true.
// Each field is: -1 = not set, 0 = no, 1 = yes.
bool robots_get_content_signal(const robots_matcher_t* matcher,
                                robots_content_signal_t* signal);

// Convenience functions for content-signal (return default true if not set).
bool robots_allows_ai_train(const robots_matcher_t* matcher);
bool robots_allows_ai_input(const robots_matcher_t* matcher);
bool robots_allows_search(const robots_matcher_t* matcher);

// =============================================================================
// Utility functions
// =============================================================================

// Validates that a user-agent string contains only valid characters [a-zA-Z_-].
bool robots_is_valid_user_agent(const char* user_agent, size_t len);

// Returns the library version string.
const char* robots_version(void);

#ifdef __cplusplus
}
#endif


// ============================================================================
// IMPLEMENTATION (C++ required for implementation)
// ============================================================================
// Generated: 2026-01-16 23:01:33 +0200
// Commit: 0e413cb
//
// Define ROBOTS_IMPLEMENTATION in exactly one C++ source file before including
// this header to include the implementation:
//
//   #define ROBOTS_IMPLEMENTATION
//   #include "robots_c.h"
//
// Note: The implementation requires C++, but the API can be called from C.
// ============================================================================

#if defined(ROBOTS_IMPLEMENTATION) && defined(__cplusplus)

// === Begin robots.cc implementation ===
#include <stdlib.h>

#ifdef ROBOTS_USE_ADA
#include <ada.h>
#endif
#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Replacement for ROBOTS_ASSERT
#define ROBOTS_ASSERT(x) assert(x)

namespace {

// String utility functions - constexpr for compile-time evaluation (C++20)

constexpr bool AsciiIsAlpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

constexpr bool AsciiIsXDigit(char c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

constexpr bool AsciiIsLower(char c) {
  return c >= 'a' && c <= 'z';
}

constexpr char AsciiToUpper(char c) {
  return (c >= 'a' && c <= 'z') ? static_cast<char>(c - 'a' + 'A') : c;
}

constexpr char AsciiToLower(char c) {
  return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
}

constexpr bool AsciiIsSpace(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

constexpr std::string_view StripAsciiWhitespace(std::string_view s) {
  size_t start = 0;
  while (start < s.size() && AsciiIsSpace(s[start])) ++start;
  size_t end = s.size();
  while (end > start && AsciiIsSpace(s[end - 1])) --end;
  return s.substr(start, end - start);
}

// StartsWith helper for C++17 compatibility (C++20 has std::string_view::starts_with())
constexpr bool StartsWith(std::string_view s, std::string_view prefix) {
  return s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix;
}

constexpr bool StartsWithIgnoreCase(std::string_view s, std::string_view prefix) {
  if (s.size() < prefix.size()) return false;
  for (size_t i = 0; i < prefix.size(); ++i) {
    if (AsciiToLower(s[i]) != AsciiToLower(prefix[i])) return false;
  }
  return true;
}

constexpr bool EqualsIgnoreCase(std::string_view a, std::string_view b) {
  if (a.size() != b.size()) return false;
  for (size_t i = 0; i < a.size(); ++i) {
    if (AsciiToLower(a[i]) != AsciiToLower(b[i])) return false;
  }
  return true;
}

}  // namespace

// Allow for typos such as DISALOW in robots.txt.
constexpr bool kAllowFrequentTypos = true;

namespace googlebot {

// A RobotsMatchStrategy defines a strategy for matching individual lines in a
// robots.txt file. Each Match* method should return a match priority, which is
// interpreted as:
//
// match priority < 0:
//    No match.
//
// match priority == 0:
//    Match, but treat it as if matched an empty pattern.
//
// match priority > 0:
//    Match.
class RobotsMatchStrategy {
 public:
  virtual ~RobotsMatchStrategy() = default;

  virtual int MatchAllow(std::string_view path,
                         std::string_view pattern) = 0;
  virtual int MatchDisallow(std::string_view path,
                            std::string_view pattern) = 0;

 protected:
  // Implements robots.txt pattern matching.
  static bool Matches(std::string_view path, std::string_view pattern);
};

// Helper: decode a hex digit to its value (0-15), or -1 if invalid.
static int HexDigitValue(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

// Helper: check if position in string is a valid %XX sequence.
// If so, returns the decoded character and sets *advance to 3.
// Otherwise returns the character at pos and sets *advance to 1.
static char DecodePercentOrChar(std::string_view s, size_t pos, int* advance) {
  if (pos + 2 < s.size() && s[pos] == '%') {
    int hi = HexDigitValue(s[pos + 1]);
    int lo = HexDigitValue(s[pos + 2]);
    if (hi >= 0 && lo >= 0) {
      *advance = 3;
      return static_cast<char>((hi << 4) | lo);
    }
  }
  *advance = 1;
  return s[pos];
}

// Returns true if URI path matches the specified pattern. Pattern is anchored
// at the beginning of path. '$' is special only at the end of pattern.
//
// Per RFC 9309 section 2.2.2, percent-encoded characters should match their
// decoded equivalents (e.g., %2F matches /, %26 matches &).
//
// Since 'path' and 'pattern' are both externally determined (by the webmaster),
// we make sure to have acceptable worst-case performance.
/* static */ bool RobotsMatchStrategy::Matches(
    std::string_view path, std::string_view pattern) {
  const size_t pathlen = path.length();
  std::vector<size_t> pos(pathlen + 1);
  int numpos;

  // The pos[] array holds a sorted list of indexes of 'path', with length
  // 'numpos'.  At the start and end of each iteration of the main loop below,
  // the pos[] array will hold a list of the prefixes of the 'path' which can
  // match the current prefix of 'pattern'. If this list is ever empty,
  // return false. If we reach the end of 'pattern' with at least one element
  // in pos[], return true.

  pos[0] = 0;
  numpos = 1;

  for (size_t pat_idx = 0; pat_idx < pattern.size(); ) {
    char pat_char = pattern[pat_idx];

    if (pat_char == '$' && pat_idx + 1 == pattern.size()) {
      return (pos[numpos - 1] == pathlen);
    }
    if (pat_char == '*') {
      numpos = pathlen - pos[0] + 1;
      for (int i = 1; i < numpos; i++) {
        pos[i] = pos[i-1] + 1;
      }
      ++pat_idx;
    } else {
      // Decode pattern character (handle %XX sequences)
      int pat_advance;
      char decoded_pat = DecodePercentOrChar(pattern, pat_idx, &pat_advance);

      // Includes '$' when not at end of pattern.
      int newnumpos = 0;
      for (int i = 0; i < numpos; i++) {
        if (pos[i] < pathlen) {
          // Decode path character (handle %XX sequences)
          int path_advance;
          char decoded_path = DecodePercentOrChar(path, pos[i], &path_advance);

          if (decoded_path == decoded_pat) {
            pos[newnumpos++] = pos[i] + path_advance;
          }
        }
      }
      numpos = newnumpos;
      if (numpos == 0) return false;
      pat_idx += pat_advance;
    }
  }

  return true;
}

constexpr std::string_view kHexDigits = "0123456789ABCDEF";

// Percent-encode special robots.txt characters (* and $) in a path.
// Per RFC 9309 section 2.2.3, these characters have special meaning in
// robots.txt patterns, so they must be encoded in URLs to match correctly
// against patterns containing %2A or %24.
// See: https://github.com/google/robotstxt/issues/57
static std::string EncodePathForMatching(std::string_view path) {
  // Quick check: if no special chars, return as-is
  bool has_special = false;
  for (char c : path) {
    if (c == '*' || c == '$') {
      has_special = true;
      break;
    }
  }
  if (!has_special) {
    return std::string(path);
  }

  // Encode * as %2A and $ as %24
  std::string result;
  result.reserve(path.size() + 6);
  for (char c : path) {
    if (c == '*') {
      result += "%2A";
    } else if (c == '$') {
      result += "%24";
    } else {
      result += c;
    }
  }
  return result;
}

// GetPathParamsQuery is not in anonymous namespace to allow testing.
//
// Extracts path (with params) and query part from URL. Removes scheme,
// authority, and fragment. Result always starts with "/".
// Returns "/" if the url doesn't have a path or is not valid.
std::string GetPathParamsQuery(const std::string& url) {
  if (url.empty()) return "/";

#ifdef ROBOTS_USE_ADA
  // Use ada-url for WHATWG-compliant URL parsing (faster, handles edge cases)
  // url_aggregator uses string_view slices, not copies
  auto parsed = ada::parse<ada::url_aggregator>(url);

  // If that fails, try adding http:// prefix for schemeless URLs
  if (!parsed) {
    // Handle protocol-relative URLs (//example.com/path)
    if (url.size() >= 2 && url[0] == '/' && url[1] == '/') {
      parsed = ada::parse<ada::url_aggregator>("http:" + url);
    } else if (url[0] != '/') {
      // Try adding scheme for URLs like "example.com/path"
      parsed = ada::parse<ada::url_aggregator>("http://" + url);
    }
  }

  if (!parsed) {
    // Last resort: if URL starts with '/', treat it as a path
    if (url[0] == '/') {
      size_t hash_pos = url.find('#');
      std::string path = (hash_pos == std::string::npos) ? url : url.substr(0, hash_pos);
      return EncodePathForMatching(path);
    }
    return "/";
  }

  std::string result(parsed->get_pathname());
  std::string_view search = parsed->get_search();
  if (!search.empty()) result += search;
  return result.empty() ? "/" : EncodePathForMatching(result);

#else
  // Fallback: simple URL parsing without ada-url dependency
  std::string_view s(url);

  // Skip scheme if present (e.g., "http://", "https://")
  size_t scheme_end = s.find("://");
  if (scheme_end != std::string_view::npos) {
    s.remove_prefix(scheme_end + 3);  // Skip "://"
  } else if (s.size() >= 2 && s[0] == '/' && s[1] == '/') {
    // Protocol-relative URL (//example.com/path)
    s.remove_prefix(2);
  }

  // Find the start of the path (or query string)
  size_t path_start = 0;
  if (!s.empty() && s[0] != '/' && s[0] != '?') {
    // Skip authority (host:port) - path starts at '/' or '?'
    size_t slash_pos = s.find('/');
    size_t query_pos = s.find('?');
    if (slash_pos == std::string_view::npos && query_pos == std::string_view::npos) {
      return "/";  // No path or query, just authority
    }
    // Use whichever comes first (/ or ?)
    if (slash_pos == std::string_view::npos) {
      // No path, but has query string - return /?query
      s = s.substr(query_pos);
      size_t hash_pos = s.find('#');
      if (hash_pos != std::string_view::npos) {
        s = s.substr(0, hash_pos);
      }
      return EncodePathForMatching("/" + std::string(s));
    }
    path_start = slash_pos;
  }

  // Extract from path_start to end, but strip fragment
  s = s.substr(path_start);
  size_t hash_pos = s.find('#');
  if (hash_pos != std::string_view::npos) {
    s = s.substr(0, hash_pos);
  }

  return s.empty() ? "/" : EncodePathForMatching(s);
#endif
}

// MaybeEscapePattern is not in anonymous namespace to allow testing.
//
// Canonicalize the allowed/disallowed paths. For example:
//     /SanJoséSellers ==> /Sanjos%C3%A9Sellers
//     %aa ==> %AA
// When the function returns, (*dst) either points to src, or is newly
// allocated.
// Returns true if dst was newly allocated.
bool MaybeEscapePattern(const char* src, char** dst) {
  int num_to_escape = 0;
  bool need_capitalize = false;

  // First, scan the buffer to see if changes are needed. Most don't.
  for (int i = 0; src[i] != 0; i++) {
    // (a) % escape sequence.
    if (src[i] == '%' &&
        AsciiIsXDigit(src[i+1]) && AsciiIsXDigit(src[i+2])) {
      if (AsciiIsLower(src[i+1]) || AsciiIsLower(src[i+2])) {
        need_capitalize = true;
      }
      i += 2;
    // (b) needs escaping.
    } else if (src[i] & 0x80) {
      num_to_escape++;
    }
    // (c) Already escaped and escape-characters normalized (eg. %2f -> %2F).
  }
  // Return if no changes needed.
  if (!num_to_escape && !need_capitalize) {
    (*dst) = const_cast<char*>(src);
    return false;
  }
  (*dst) = new char[num_to_escape * 2 + strlen(src) + 1];
  int j = 0;
  for (int i = 0; src[i] != 0; i++) {
    // (a) Normalize %-escaped sequence (eg. %2f -> %2F).
    if (src[i] == '%' &&
        AsciiIsXDigit(src[i+1]) && AsciiIsXDigit(src[i+2])) {
      (*dst)[j++] = src[i++];
      (*dst)[j++] = AsciiToUpper(src[i++]);
      (*dst)[j++] = AsciiToUpper(src[i]);
      // (b) %-escape octets whose highest bit is set. These are outside the
      // ASCII range.
    } else if (src[i] & 0x80) {
      (*dst)[j++] = '%';
      (*dst)[j++] = kHexDigits[(src[i] >> 4) & 0xf];
      (*dst)[j++] = kHexDigits[src[i] & 0xf];
    // (c) Normal character, no modification needed.
    } else {
      (*dst)[j++] = src[i];
    }
  }
  (*dst)[j] = 0;
  return true;
}

// Zero-copy version: returns escaped string only if escaping was needed.
// If no escaping needed, returns std::nullopt (caller should use original).
std::optional<std::string> MaybeEscapePattern(std::string_view src) {
  int num_to_escape = 0;
  bool need_capitalize = false;

  // First, scan the buffer to see if changes are needed. Most don't.
  for (size_t i = 0; i < src.size(); i++) {
    // (a) % escape sequence.
    if (src[i] == '%' && i + 2 < src.size() &&
        AsciiIsXDigit(src[i+1]) && AsciiIsXDigit(src[i+2])) {
      if (AsciiIsLower(src[i+1]) || AsciiIsLower(src[i+2])) {
        need_capitalize = true;
      }
      i += 2;
    // (b) needs escaping.
    } else if (src[i] & 0x80) {
      num_to_escape++;
    }
  }
  // Return if no changes needed.
  if (!num_to_escape && !need_capitalize) {
    return std::nullopt;
  }

  std::string dst;
  dst.reserve(num_to_escape * 2 + src.size());
  for (size_t i = 0; i < src.size(); i++) {
    // (a) Normalize %-escaped sequence (eg. %2f -> %2F).
    if (src[i] == '%' && i + 2 < src.size() &&
        AsciiIsXDigit(src[i+1]) && AsciiIsXDigit(src[i+2])) {
      dst += src[i++];
      dst += AsciiToUpper(src[i++]);
      dst += AsciiToUpper(src[i]);
    // (b) %-escape octets whose highest bit is set.
    } else if (src[i] & 0x80) {
      dst += '%';
      dst += kHexDigits[(static_cast<unsigned char>(src[i]) >> 4) & 0xf];
      dst += kHexDigits[static_cast<unsigned char>(src[i]) & 0xf];
    // (c) Normal character, no modification needed.
    } else {
      dst += src[i];
    }
  }
  return dst;
}

// Internal helper classes and functions.
namespace {

// A robots.txt has lines of key/value pairs. A ParsedRobotsKey represents
// a key. This class can parse a text-representation (including common typos)
// and represent them as an enumeration which allows for faster processing
// afterwards.
// For unparsable keys, the original string representation is kept.
class ParsedRobotsKey {
 public:
  enum KeyType {
    // Generic highlevel fields.
    USER_AGENT,
    SITEMAP,

    // Fields within a user-agent.
    ALLOW,
    DISALLOW,

    // Non-standard but widely used directives.
    CRAWL_DELAY,
    REQUEST_RATE,

#if ROBOTS_SUPPORT_CONTENT_SIGNAL
    // AI content signal directive (proposed).
    CONTENT_SIGNAL,
#endif  // ROBOTS_SUPPORT_CONTENT_SIGNAL

    // Unrecognized field; kept as-is. High number so that additions to the
    // enumeration above does not change the serialization.
    UNKNOWN = 128
  };

  ParsedRobotsKey() : type_(UNKNOWN) {}

  // Disallow copying and assignment.
  ParsedRobotsKey(const ParsedRobotsKey&) = delete;
  ParsedRobotsKey& operator=(const ParsedRobotsKey&) = delete;

  // Parse given key text and report in the is_acceptable_typo output parameter
  // whether the key is one of the accepted typo-variants of a supported key.
  // Does not copy the text, so the text_key must stay valid for the object's
  // life-time or the next Parse() call.
  void Parse(std::string_view key, bool* is_acceptable_typo);

  // Returns the type of key.
  KeyType type() const { return type_; }

  // If this is an unknown key, get the text.
  std::string_view GetUnknownText() const;

 private:
  // The KeyIs* methods return whether a passed in key is one of the a supported
  // keys, and report in the is_acceptable_typo output parameter whether the key
  // is one of the accepted typo-variants of a supported key.
  static bool KeyIsUserAgent(std::string_view key, bool* is_acceptable_typo);
  static bool KeyIsAllow(std::string_view key, bool* is_acceptable_typo);
  static bool KeyIsDisallow(std::string_view key, bool* is_acceptable_typo);
  static bool KeyIsSitemap(std::string_view key, bool* is_acceptable_typo);
  static bool KeyIsCrawlDelay(std::string_view key, bool* is_acceptable_typo);
  static bool KeyIsRequestRate(std::string_view key, bool* is_acceptable_typo);
#if ROBOTS_SUPPORT_CONTENT_SIGNAL
  static bool KeyIsContentSignal(std::string_view key, bool* is_acceptable_typo);
#endif  // ROBOTS_SUPPORT_CONTENT_SIGNAL

  KeyType type_;
  std::string_view key_text_;
};

void EmitKeyValueToHandler(int line, const ParsedRobotsKey& key,
                           std::string_view value,
                           RobotsParseHandler* handler) {
  typedef ParsedRobotsKey Key;
  switch (key.type()) {
    case Key::USER_AGENT:     handler->HandleUserAgent(line, value); break;
    case Key::ALLOW:          handler->HandleAllow(line, value); break;
    case Key::DISALLOW:       handler->HandleDisallow(line, value); break;
    case Key::SITEMAP:        handler->HandleSitemap(line, value); break;
    case Key::CRAWL_DELAY: {
      // Parse value as double (seconds). Invalid values are treated as 0.
      double delay = 0.0;
      if (!value.empty()) {
        // Use strtod for parsing. Create a null-terminated copy.
        std::string value_str(value);
        char* endptr = nullptr;
        delay = strtod(value_str.c_str(), &endptr);
        // If parsing failed or value is negative, use 0.
        if (endptr == value_str.c_str() || delay < 0) {
          delay = 0.0;
        }
      }
      handler->HandleCrawlDelay(line, delay);
      break;
    }
    case Key::REQUEST_RATE: {
      // Parse "requests/seconds" format (e.g., "1/5", "1/5s", "30/60", "1").
      RequestRate rate;
      if (!value.empty()) {
        std::string value_str(value);
        char* endptr = nullptr;
        long requests = strtol(value_str.c_str(), &endptr, 10);
        if (endptr != value_str.c_str() && requests > 0) {
          rate.requests = static_cast<int>(requests);
          // Check for "/" separator
          if (*endptr == '/') {
            ++endptr;
            char* seconds_end = nullptr;
            long seconds = strtol(endptr, &seconds_end, 10);
            if (seconds_end != endptr && seconds > 0) {
              rate.seconds = static_cast<int>(seconds);
            }
            // Ignore optional 's' suffix (e.g., "1/5s")
          }
          // If no "/" found, treat as "requests/1" (requests per second)
        }
      }
      handler->HandleRequestRate(line, rate);
      break;
    }
#if ROBOTS_SUPPORT_CONTENT_SIGNAL
    case Key::CONTENT_SIGNAL: {
      // Parse "key=value, key=value" format
      // (e.g., "ai-train=no, search=yes, ai-input=yes").
      ContentSignal signal;
      if (!value.empty()) {
        // Parse comma-separated key=value pairs
        size_t pos = 0;
        while (pos < value.size()) {
          // Skip whitespace and commas
          while (pos < value.size() && (value[pos] == ' ' || value[pos] == '\t' || value[pos] == ',')) {
            ++pos;
          }
          if (pos >= value.size()) break;

          // Find the '=' separator
          size_t eq_pos = value.find('=', pos);
          if (eq_pos == std::string_view::npos) break;

          std::string_view key_part = StripAsciiWhitespace(value.substr(pos, eq_pos - pos));

          // Find end of value (next comma or end of string)
          size_t val_start = eq_pos + 1;
          size_t val_end = value.find(',', val_start);
          if (val_end == std::string_view::npos) {
            val_end = value.size();
          }

          std::string_view val_part = StripAsciiWhitespace(value.substr(val_start, val_end - val_start));

          // Parse boolean value (yes/no, true/false, 1/0)
          std::optional<bool> bool_val;
          if (EqualsIgnoreCase(val_part, "yes") || EqualsIgnoreCase(val_part, "true") || val_part == "1") {
            bool_val = true;
          } else if (EqualsIgnoreCase(val_part, "no") || EqualsIgnoreCase(val_part, "false") || val_part == "0") {
            bool_val = false;
          }

          // Set the appropriate signal field
          if (bool_val.has_value()) {
            if (EqualsIgnoreCase(key_part, "ai-train")) {
              signal.ai_train = *bool_val;
            } else if (EqualsIgnoreCase(key_part, "ai-input")) {
              signal.ai_input = *bool_val;
            } else if (EqualsIgnoreCase(key_part, "search")) {
              signal.search = *bool_val;
            }
          }

          pos = val_end;
        }
      }
      handler->HandleContentSignal(line, signal);
      break;
    }
#endif  // ROBOTS_SUPPORT_CONTENT_SIGNAL
    case Key::UNKNOWN:
      handler->HandleUnknownAction(line, key.GetUnknownText(), value);
      break;
      // No default case Key:: to have the compiler warn about new values.
  }
}

class RobotsTxtParser {
 public:
  typedef ParsedRobotsKey Key;

  RobotsTxtParser(std::string_view robots_body,
                  RobotsParseHandler* handler)
      : robots_body_(robots_body), handler_(handler) {
  }

  void Parse();

 private:
  // Parses a line into key and value string_views without copying.
  // Note that `key` and `value` are only set when `metadata->has_directive
  // == true`.
  static void GetKeyAndValueFrom(std::string_view line,
                                 std::string_view* key, std::string_view* value,
                                 RobotsParseHandler::LineMetadata* metadata);

  void ParseAndEmitLine(int current_line, std::string_view line,
                        bool line_too_long);
  static bool NeedEscapeValueForKey(const Key& key);

  std::string_view robots_body_;
  RobotsParseHandler* const handler_;
};

bool RobotsTxtParser::NeedEscapeValueForKey(const Key& key) {
  switch (key.type()) {
    case RobotsTxtParser::Key::USER_AGENT:
    case RobotsTxtParser::Key::SITEMAP:
      return false;
    default:
      return true;
  }
}

// Finds the first occurrence of any character from `chars` in `s`.
// Returns npos if not found.
constexpr size_t FindFirstOf(std::string_view s, std::string_view chars) {
  for (size_t i = 0; i < s.size(); ++i) {
    for (char c : chars) {
      if (s[i] == c) return i;
    }
  }
  return std::string_view::npos;
}

// Zero-copy version: parses line into key/value string_views.
/* static */ void RobotsTxtParser::GetKeyAndValueFrom(
    std::string_view line,
    std::string_view* key, std::string_view* value,
    RobotsParseHandler::LineMetadata* metadata) {
  // Remove comments from the current robots.txt line.
  size_t comment_pos = line.find('#');
  if (comment_pos != std::string_view::npos) {
    metadata->has_comment = true;
    line = line.substr(0, comment_pos);
  }

  line = StripAsciiWhitespace(line);

  // If the line became empty after removing the comment, return.
  if (line.empty()) {
    if (metadata->has_comment) {
      metadata->is_comment = true;
    } else {
      metadata->is_empty = true;
    }
    return;
  }

  // Rules must match the following pattern:
  //   <key>[ \t]*:[ \t]*<value>
  size_t sep_pos = line.find(':');
  if (sep_pos == std::string_view::npos) {
    // Google-specific optimization: some people forget the colon, so we need to
    // accept whitespace in its stead.
    constexpr std::string_view kWhite = " \t";
    sep_pos = FindFirstOf(line, kWhite);
    if (sep_pos != std::string_view::npos) {
      std::string_view val = line.substr(sep_pos);
      val = StripAsciiWhitespace(val);
      // We only accept whitespace as a separator if there are exactly two
      // sequences of non-whitespace characters.
      if (FindFirstOf(val, kWhite) != std::string_view::npos) {
        // More than 2 sequences of non-whitespace.
        return;
      }
      metadata->is_missing_colon_separator = true;
    }
  }
  if (sep_pos == std::string_view::npos) {
    return;  // Couldn't find a separator.
  }

  *key = StripAsciiWhitespace(line.substr(0, sep_pos));

  if (!key->empty()) {
    *value = StripAsciiWhitespace(line.substr(sep_pos + 1));
    metadata->has_directive = true;
  }
}

void RobotsTxtParser::ParseAndEmitLine(int current_line, std::string_view line,
                                       bool line_too_long) {
  std::string_view string_key;
  std::string_view value;
  RobotsParseHandler::LineMetadata line_metadata;
  // Note that `string_key` and `value` are only set when
  // `line_metadata->has_directive == true`.
  GetKeyAndValueFrom(line, &string_key, &value, &line_metadata);
  line_metadata.is_line_too_long = line_too_long;
  if (!line_metadata.has_directive) {
    handler_->ReportLineMetadata(current_line, line_metadata);
    return;
  }
  Key key;
  key.Parse(string_key, &line_metadata.is_acceptable_typo);
  if (NeedEscapeValueForKey(key)) {
    auto escaped = MaybeEscapePattern(value);
    if (escaped) {
      EmitKeyValueToHandler(current_line, key, *escaped, handler_);
    } else {
      EmitKeyValueToHandler(current_line, key, value, handler_);
    }
  } else {
    EmitKeyValueToHandler(current_line, key, value, handler_);
  }
  // Finish adding metadata to the line.
  handler_->ReportLineMetadata(current_line, line_metadata);
}

void RobotsTxtParser::Parse() {
  // UTF-8 byte order marks.
  constexpr unsigned char utf_bom[3] = {0xEF, 0xBB, 0xBF};

  // Certain browsers limit the URL length to 2083 bytes. In a robots.txt, it's
  // fairly safe to assume any valid line isn't going to be more than many times
  // that max url length of 2KB. We want some padding for
  // UTF-8 encoding/nulls/etc. but a much smaller bound would be okay as well.
  // If so, we can ignore the chars on a line past that.
  constexpr size_t kBrowserMaxLineLen = 2083;
  // Note: original code used a buffer of kMaxLineLen bytes with the last byte
  // reserved for null terminator, so max content length is kMaxLineLen - 1.
  constexpr size_t kMaxLineLen = kBrowserMaxLineLen * 8 - 1;

  // Zero-copy parsing: track line boundaries via indices into robots_body_.
  int line_num = 0;
  size_t bom_skip = 0;
  bool last_was_carriage_return = false;
  handler_->HandleRobotsStart();

  // Skip UTF-8 BOM prefix if present (even partial BOM is skipped).
  while (bom_skip < sizeof(utf_bom) && bom_skip < robots_body_.size() &&
         static_cast<unsigned char>(robots_body_[bom_skip]) == utf_bom[bom_skip]) {
    ++bom_skip;
  }

  size_t line_start = bom_skip;
  for (size_t i = bom_skip; i < robots_body_.size(); ++i) {
    const unsigned char ch = static_cast<unsigned char>(robots_body_[i]);
    if (ch == 0x0A || ch == 0x0D) {  // Line-ending character.
      // Only emit an empty line if this was not due to the second character
      // of the DOS line-ending \r\n.
      const bool is_CRLF_continuation =
          (i == line_start) && last_was_carriage_return && ch == 0x0A;
      if (!is_CRLF_continuation) {
        size_t line_len = i - line_start;
        bool line_too_long = line_len > kMaxLineLen;
        if (line_too_long) {
          line_len = kMaxLineLen;
        }
        std::string_view line = robots_body_.substr(line_start, line_len);
        ParseAndEmitLine(++line_num, line, line_too_long);
      }
      line_start = i + 1;
      last_was_carriage_return = (ch == 0x0D);
    }
  }

  // Handle final line (if no trailing newline) or emit empty line if file
  // ends with newline.
  if (line_start <= robots_body_.size()) {
    size_t line_len = robots_body_.size() - line_start;
    bool line_too_long = line_len > kMaxLineLen;
    if (line_too_long) {
      line_len = kMaxLineLen;
    }
    std::string_view line = robots_body_.substr(line_start, line_len);
    ParseAndEmitLine(++line_num, line, line_too_long);
  }
  handler_->HandleRobotsEnd();
}

// Implements the default robots.txt matching strategy. The maximum number of
// characters matched by a pattern is returned as its match priority.
class LongestMatchRobotsMatchStrategy : public RobotsMatchStrategy {
 public:
  LongestMatchRobotsMatchStrategy() = default;

  // Disallow copying and assignment.
  LongestMatchRobotsMatchStrategy(const LongestMatchRobotsMatchStrategy&) =
      delete;
  LongestMatchRobotsMatchStrategy& operator=(
      const LongestMatchRobotsMatchStrategy&) = delete;

  int MatchAllow(std::string_view path, std::string_view pattern) override;
  int MatchDisallow(std::string_view path, std::string_view pattern) override;
};
}  // end anonymous namespace

void ParseRobotsTxt(std::string_view robots_body,
                    RobotsParseHandler* parse_callback) {
  RobotsTxtParser parser(robots_body, parse_callback);
  parser.Parse();
}

RobotsMatcher::RobotsMatcher()
    : seen_global_agent_(false),
      seen_specific_agent_(false),
      ever_seen_specific_agent_(false),
      seen_separator_(false),
      best_specific_agent_length_(0),
      path_(nullptr),
      user_agents_(nullptr) {
  match_strategy_ = new LongestMatchRobotsMatchStrategy();
}

RobotsMatcher::~RobotsMatcher() {
  delete match_strategy_;
}

bool RobotsMatcher::ever_seen_specific_agent() const {
  return ever_seen_specific_agent_;
}

void RobotsMatcher::InitUserAgentsAndPath(
    const std::vector<std::string>* user_agents, const char* path) {
  // The RobotsParser object doesn't own path_ or user_agents_, so overwriting
  // these pointers doesn't cause a memory leak.
  path_ = path;
  ROBOTS_ASSERT('/' == *path_);
  user_agents_ = user_agents;
  best_specific_agent_length_ = 0;
}

bool RobotsMatcher::AllowedByRobots(std::string_view robots_body,
                                    const std::vector<std::string>* user_agents,
                                    const std::string& url) {
  // The url is not normalized (escaped, percent encoded) here because the user
  // is asked to provide it in escaped form already.
  std::string path = GetPathParamsQuery(url);
  InitUserAgentsAndPath(user_agents, path.c_str());
  ParseRobotsTxt(robots_body, this);
  return !disallow();
}

bool RobotsMatcher::OneAgentAllowedByRobots(std::string_view robots_txt,
                                            const std::string& user_agent,
                                            const std::string& url) {
  std::vector<std::string> v;
  v.push_back(user_agent);
  return AllowedByRobots(robots_txt, &v, url);
}

bool RobotsMatcher::disallow() const {
  if (allow_.specific.priority() > 0 || disallow_.specific.priority() > 0) {
    return (disallow_.specific.priority() > allow_.specific.priority());
  }

  if (ever_seen_specific_agent_) {
    // Matching group for user-agent but either without disallow or empty one,
    // i.e. priority == 0.
    return false;
  }

  if (disallow_.global.priority() > 0 || allow_.global.priority() > 0) {
    return disallow_.global.priority() > allow_.global.priority();
  }
  return false;
}

bool RobotsMatcher::disallow_ignore_global() const {
  if (allow_.specific.priority() > 0 || disallow_.specific.priority() > 0) {
    return disallow_.specific.priority() > allow_.specific.priority();
  }
  return false;
}

int RobotsMatcher::matching_line() const {
  if (ever_seen_specific_agent_) {
    return Match::HigherPriorityMatch(disallow_.specific, allow_.specific)
        .line();
  }
  return Match::HigherPriorityMatch(disallow_.global, allow_.global).line();
}

void RobotsMatcher::HandleRobotsStart() {
  // This is a new robots.txt file, so we need to reset all the instance member
  // variables. We do it in the same order the instance member variables are
  // declared, so it's easier to keep track of which ones we have (or maybe
  // haven't!) done.
  allow_.Clear();
  disallow_.Clear();

  seen_global_agent_ = false;
  seen_specific_agent_ = false;
  ever_seen_specific_agent_ = false;
  seen_separator_ = false;

  crawl_delay_global_.reset();
  crawl_delay_specific_.reset();
  request_rate_global_.reset();
  request_rate_specific_.reset();
#if ROBOTS_SUPPORT_CONTENT_SIGNAL
  content_signal_global_.reset();
  content_signal_specific_.reset();
#endif  // ROBOTS_SUPPORT_CONTENT_SIGNAL
}

/*static*/ std::string_view RobotsMatcher::ExtractUserAgent(
    std::string_view user_agent) {
  // Allowed characters in user-agent are [a-zA-Z_-].
  const char* end = user_agent.data();
  while (AsciiIsAlpha(*end) || *end == '-' || *end == '_') {
    ++end;
  }
  return user_agent.substr(0, end - user_agent.data());
}

/*static*/ bool RobotsMatcher::IsValidUserAgentToObey(
    std::string_view user_agent) {
  return user_agent.length() > 0 && ExtractUserAgent(user_agent) == user_agent;
}

void RobotsMatcher::HandleUserAgent(int line_num,
                                    std::string_view user_agent) {
  if (seen_separator_) {
    seen_specific_agent_ = seen_global_agent_ = seen_separator_ = false;
  }

  // Google-specific optimization: a '*' followed by space and more characters
  // in a user-agent record is still regarded a global rule.
  if (user_agent.length() >= 1 && user_agent[0] == '*' &&
      (user_agent.length() == 1 || isspace(user_agent[1]))) {
    seen_global_agent_ = true;
  } else {
    user_agent = ExtractUserAgent(user_agent);
    for (const auto& agent : *user_agents_) {
      if (EqualsIgnoreCase(user_agent, agent)) {        
        // Implement "most specific user-agent wins" rule per Google's docs:
        // https://developers.google.com/search/reference/robots_txt#order-of-precedence-for-user-agents
        
        // A longer matching user-agent string is more specific.
        
        if (user_agent.length() > best_specific_agent_length_) {
          // Found a more specific match - reset previous specific rules.
          best_specific_agent_length_ = user_agent.length();
          allow_.specific.Clear();
          disallow_.specific.Clear();
          ever_seen_specific_agent_ = seen_specific_agent_ = true;
        
        } else if (user_agent.length() == best_specific_agent_length_) {
          // Same specificity - allow this group to contribute rules.
          ever_seen_specific_agent_ = seen_specific_agent_ = true;
        }
        
        // If user_agent.length() < best_specific_agent_length_, we ignore
        // this less specific group by not setting seen_specific_agent_.
        break;
      }
    }
  }
}

void RobotsMatcher::HandleAllow(int line_num, std::string_view value) {
  if (!seen_any_agent()) return;
  seen_separator_ = true;
  const int priority = match_strategy_->MatchAllow(path_, value);
  if (priority >= 0) {
    if (seen_specific_agent_) {
      if (allow_.specific.priority() < priority) {
        allow_.specific.Set(priority, line_num);
      }
    } else {
      assert(seen_global_agent_);
      if (allow_.global.priority() < priority) {
        allow_.global.Set(priority, line_num);
      }
    }
  } else {
    // Google-specific optimization: 'index.htm' and 'index.html' are normalized
    // to '/'.
    const size_t slash_pos = value.find_last_of('/');

    if (slash_pos != std::string_view::npos &&
        StartsWith(value.substr(slash_pos), "/index.htm")) {
      const int len = slash_pos + 1;
      std::vector<char> newpattern(len + 1);
      strncpy(newpattern.data(), value.data(), len);
      newpattern[len] = '$';
      HandleAllow(line_num,
                  std::string_view(newpattern.data(), newpattern.size()));
    }
  }
}

void RobotsMatcher::HandleDisallow(int line_num, std::string_view value) {
  if (!seen_any_agent()) return;
  seen_separator_ = true;
  const int priority = match_strategy_->MatchDisallow(path_, value);
  if (priority >= 0) {
    if (seen_specific_agent_) {
      if (disallow_.specific.priority() < priority) {
        disallow_.specific.Set(priority, line_num);
      }
    } else {
      assert(seen_global_agent_);
      if (disallow_.global.priority() < priority) {
        disallow_.global.Set(priority, line_num);
      }
    }
  }
}

int LongestMatchRobotsMatchStrategy::MatchAllow(std::string_view path,
                                                std::string_view pattern) {
  return Matches(path, pattern) ? pattern.length() : -1;
}

int LongestMatchRobotsMatchStrategy::MatchDisallow(std::string_view path,
                                                   std::string_view pattern) {
  return Matches(path, pattern) ? pattern.length() : -1;
}

void RobotsMatcher::HandleSitemap(int line_num, std::string_view value) {}

void RobotsMatcher::HandleCrawlDelay(int line_num, double value) {
  if (!seen_any_agent()) return;
  // Store crawl-delay for the current user-agent group.
  // Does NOT set seen_separator_ - crawl-delay doesn't close the group.
  if (seen_specific_agent_) {
    // Only store if not already set (first value wins within a group).
    if (!crawl_delay_specific_.has_value()) {
      crawl_delay_specific_ = value;
    }
  } else if (seen_global_agent_) {
    if (!crawl_delay_global_.has_value()) {
      crawl_delay_global_ = value;
    }
  }
}

std::optional<double> RobotsMatcher::GetCrawlDelay() const {
  // Return specific user-agent's crawl-delay if available, otherwise global.
  if (ever_seen_specific_agent_ && crawl_delay_specific_.has_value()) {
    return crawl_delay_specific_;
  }
  return crawl_delay_global_;
}

void RobotsMatcher::HandleRequestRate(int line_num, const RequestRate& rate) {
  if (!seen_any_agent()) return;
  // Store request-rate for the current user-agent group.
  // Does NOT set seen_separator_ - request-rate doesn't close the group.
  if (seen_specific_agent_) {
    if (!request_rate_specific_.has_value()) {
      request_rate_specific_ = rate;
    }
  } else if (seen_global_agent_) {
    if (!request_rate_global_.has_value()) {
      request_rate_global_ = rate;
    }
  }
}

std::optional<RequestRate> RobotsMatcher::GetRequestRate() const {
  // Return specific user-agent's request-rate if available, otherwise global.
  if (ever_seen_specific_agent_ && request_rate_specific_.has_value()) {
    return request_rate_specific_;
  }
  return request_rate_global_;
}

#if ROBOTS_SUPPORT_CONTENT_SIGNAL
void RobotsMatcher::HandleContentSignal(int line_num, const ContentSignal& signal) {
  if (!seen_any_agent()) return;
  // Store content-signal for the current user-agent group.
  // Does NOT set seen_separator_ - content-signal doesn't close the group.
  if (seen_specific_agent_) {
    if (!content_signal_specific_.has_value()) {
      content_signal_specific_ = signal;
    }
  } else if (seen_global_agent_) {
    if (!content_signal_global_.has_value()) {
      content_signal_global_ = signal;
    }
  }
}

std::optional<ContentSignal> RobotsMatcher::GetContentSignal() const {
  // Return specific user-agent's content-signal if available, otherwise global.
  if (ever_seen_specific_agent_ && content_signal_specific_.has_value()) {
    return content_signal_specific_;
  }
  return content_signal_global_;
}
#endif  // ROBOTS_SUPPORT_CONTENT_SIGNAL

void RobotsMatcher::HandleUnknownAction(int line_num, std::string_view action,
                                        std::string_view value) {}

void ParsedRobotsKey::Parse(std::string_view key, bool* is_acceptable_typo) {
  key_text_ = std::string_view();
  if (KeyIsUserAgent(key, is_acceptable_typo)) {
    type_ = USER_AGENT;
  } else if (KeyIsAllow(key, is_acceptable_typo)) {
    type_ = ALLOW;
  } else if (KeyIsDisallow(key, is_acceptable_typo)) {
    type_ = DISALLOW;
  } else if (KeyIsSitemap(key, is_acceptable_typo)) {
    type_ = SITEMAP;
  } else if (KeyIsCrawlDelay(key, is_acceptable_typo)) {
    type_ = CRAWL_DELAY;
  } else if (KeyIsRequestRate(key, is_acceptable_typo)) {
    type_ = REQUEST_RATE;
#if ROBOTS_SUPPORT_CONTENT_SIGNAL
  } else if (KeyIsContentSignal(key, is_acceptable_typo)) {
    type_ = CONTENT_SIGNAL;
#endif  // ROBOTS_SUPPORT_CONTENT_SIGNAL
  } else {
    type_ = UNKNOWN;
    key_text_ = key;
  }
}

std::string_view ParsedRobotsKey::GetUnknownText() const {
  ROBOTS_ASSERT(type_ == UNKNOWN && !key_text_.empty());
  return key_text_;
}

bool ParsedRobotsKey::KeyIsUserAgent(std::string_view key,
                                     bool* is_acceptable_typo) {
  *is_acceptable_typo =
      (kAllowFrequentTypos && (StartsWithIgnoreCase(key, "useragent") ||
                               StartsWithIgnoreCase(key, "user agent")));
  return (StartsWithIgnoreCase(key, "user-agent") || *is_acceptable_typo);
}

bool ParsedRobotsKey::KeyIsAllow(std::string_view key,
                                 bool* is_acceptable_typo) {
  // We don't support typos for the "allow" key.
  *is_acceptable_typo = false;
  return StartsWithIgnoreCase(key, "allow");
}

bool ParsedRobotsKey::KeyIsDisallow(std::string_view key,
                                    bool* is_acceptable_typo) {
  *is_acceptable_typo =
      (kAllowFrequentTypos && ((StartsWithIgnoreCase(key, "dissallow")) ||
                               (StartsWithIgnoreCase(key, "dissalow")) ||
                               (StartsWithIgnoreCase(key, "disalow")) ||
                               (StartsWithIgnoreCase(key, "diasllow")) ||
                               (StartsWithIgnoreCase(key, "disallaw"))));
  return (StartsWithIgnoreCase(key, "disallow") || *is_acceptable_typo);
}

bool ParsedRobotsKey::KeyIsSitemap(std::string_view key,
                                   bool* is_acceptable_typo) {
  *is_acceptable_typo =
      (kAllowFrequentTypos && (StartsWithIgnoreCase(key, "site-map")));
  return StartsWithIgnoreCase(key, "sitemap") || *is_acceptable_typo;
}

bool ParsedRobotsKey::KeyIsCrawlDelay(std::string_view key,
                                      bool* is_acceptable_typo) {
  // Accept common variants: "crawl-delay", "crawldelay", "crawl delay"
  *is_acceptable_typo =
      (kAllowFrequentTypos && (StartsWithIgnoreCase(key, "crawldelay") ||
                               StartsWithIgnoreCase(key, "crawl delay")));
  return StartsWithIgnoreCase(key, "crawl-delay") || *is_acceptable_typo;
}

bool ParsedRobotsKey::KeyIsRequestRate(std::string_view key,
                                       bool* is_acceptable_typo) {
  // Only accept "request-rate" (no typo variants, unlike crawl-delay).
  *is_acceptable_typo = false;
  return StartsWithIgnoreCase(key, "request-rate");
}

#if ROBOTS_SUPPORT_CONTENT_SIGNAL
bool ParsedRobotsKey::KeyIsContentSignal(std::string_view key,
                                         bool* is_acceptable_typo) {
  // Accept "content-signal" and common variants.
  *is_acceptable_typo =
      (kAllowFrequentTypos && (StartsWithIgnoreCase(key, "contentsignal") ||
                               StartsWithIgnoreCase(key, "content signal")));
  return StartsWithIgnoreCase(key, "content-signal") || *is_acceptable_typo;
}
#endif  // ROBOTS_SUPPORT_CONTENT_SIGNAL

}  // namespace googlebot

// === End robots.cc implementation ===

// === Begin robots_c.cc implementation ===
#include <string>
#include <string_view>
#include <vector>

#define ROBOTS_VERSION "1.0.0"

// =============================================================================
// Internal wrapper struct
// =============================================================================

struct robots_matcher_s {
  googlebot::RobotsMatcher matcher;
};

// =============================================================================
// Matcher lifecycle
// =============================================================================

extern "C" robots_matcher_t* robots_matcher_create(void) {
  try {
    return new robots_matcher_t();
  } catch (...) {
    return nullptr;
  }
}

extern "C" void robots_matcher_free(robots_matcher_t* matcher) {
  delete matcher;
}

// =============================================================================
// URL checking
// =============================================================================

extern "C" bool robots_allowed_by_robots(
    robots_matcher_t* matcher,
    const char* robots_txt, size_t robots_txt_len,
    const char* user_agent, size_t user_agent_len,
    const char* url, size_t url_len) {
  if (!matcher || !robots_txt || !user_agent || !url) {
    return true;  // Allow on invalid input
  }

  std::string_view robots_body(robots_txt, robots_txt_len);
  std::string agent(user_agent, user_agent_len);
  std::string target_url(url, url_len);

  return matcher->matcher.OneAgentAllowedByRobots(robots_body, agent, target_url);
}

extern "C" bool robots_allowed_by_robots_multi(
    robots_matcher_t* matcher,
    const char* robots_txt, size_t robots_txt_len,
    const char* const* user_agents, const size_t* user_agent_lens,
    size_t num_user_agents,
    const char* url, size_t url_len) {
  if (!matcher || !robots_txt || !user_agents || !user_agent_lens || !url) {
    return true;  // Allow on invalid input
  }

  std::string_view robots_body(robots_txt, robots_txt_len);
  std::vector<std::string> agents;
  agents.reserve(num_user_agents);
  for (size_t i = 0; i < num_user_agents; ++i) {
    agents.emplace_back(user_agents[i], user_agent_lens[i]);
  }
  std::string target_url(url, url_len);

  return matcher->matcher.AllowedByRobots(robots_body, &agents, target_url);
}

// =============================================================================
// Matcher state accessors
// =============================================================================

extern "C" int robots_matching_line(const robots_matcher_t* matcher) {
  if (!matcher) return 0;
  return matcher->matcher.matching_line();
}

extern "C" bool robots_ever_seen_specific_agent(const robots_matcher_t* matcher) {
  if (!matcher) return false;
  return matcher->matcher.ever_seen_specific_agent();
}

// =============================================================================
// Crawl-delay support
// =============================================================================

extern "C" bool robots_has_crawl_delay(const robots_matcher_t* matcher) {
  if (!matcher) return false;
  return matcher->matcher.GetCrawlDelay().has_value();
}

extern "C" double robots_get_crawl_delay(const robots_matcher_t* matcher) {
  if (!matcher) return 0.0;
  auto delay = matcher->matcher.GetCrawlDelay();
  return delay.value_or(0.0);
}

// =============================================================================
// Request-rate support
// =============================================================================

extern "C" bool robots_has_request_rate(const robots_matcher_t* matcher) {
  if (!matcher) return false;
  return matcher->matcher.GetRequestRate().has_value();
}

extern "C" bool robots_get_request_rate(const robots_matcher_t* matcher,
                                         robots_request_rate_t* rate) {
  if (!matcher || !rate) return false;
  auto opt_rate = matcher->matcher.GetRequestRate();
  if (!opt_rate.has_value()) return false;
  rate->requests = opt_rate->requests;
  rate->seconds = opt_rate->seconds;
  return true;
}

// =============================================================================
// Content-Signal support
// =============================================================================

extern "C" bool robots_content_signal_supported(void) {
#if ROBOTS_SUPPORT_CONTENT_SIGNAL
  return true;
#else
  return false;
#endif
}

extern "C" bool robots_has_content_signal(const robots_matcher_t* matcher) {
#if ROBOTS_SUPPORT_CONTENT_SIGNAL
  if (!matcher) return false;
  return matcher->matcher.GetContentSignal().has_value();
#else
  (void)matcher;
  return false;
#endif
}

extern "C" bool robots_get_content_signal(const robots_matcher_t* matcher,
                                           robots_content_signal_t* signal) {
#if ROBOTS_SUPPORT_CONTENT_SIGNAL
  if (!matcher || !signal) return false;
  auto opt_signal = matcher->matcher.GetContentSignal();
  if (!opt_signal.has_value()) return false;

  signal->ai_train = opt_signal->ai_train.has_value()
                         ? (opt_signal->ai_train.value() ? 1 : 0)
                         : -1;
  signal->ai_input = opt_signal->ai_input.has_value()
                         ? (opt_signal->ai_input.value() ? 1 : 0)
                         : -1;
  signal->search = opt_signal->search.has_value()
                       ? (opt_signal->search.value() ? 1 : 0)
                       : -1;
  return true;
#else
  (void)matcher;
  (void)signal;
  return false;
#endif
}

extern "C" bool robots_allows_ai_train(const robots_matcher_t* matcher) {
#if ROBOTS_SUPPORT_CONTENT_SIGNAL
  if (!matcher) return true;
  auto opt_signal = matcher->matcher.GetContentSignal();
  if (!opt_signal.has_value()) return true;
  return opt_signal->AllowsAiTrain();
#else
  (void)matcher;
  return true;
#endif
}

extern "C" bool robots_allows_ai_input(const robots_matcher_t* matcher) {
#if ROBOTS_SUPPORT_CONTENT_SIGNAL
  if (!matcher) return true;
  auto opt_signal = matcher->matcher.GetContentSignal();
  if (!opt_signal.has_value()) return true;
  return opt_signal->AllowsAiInput();
#else
  (void)matcher;
  return true;
#endif
}

extern "C" bool robots_allows_search(const robots_matcher_t* matcher) {
#if ROBOTS_SUPPORT_CONTENT_SIGNAL
  if (!matcher) return true;
  auto opt_signal = matcher->matcher.GetContentSignal();
  if (!opt_signal.has_value()) return true;
  return opt_signal->AllowsSearch();
#else
  (void)matcher;
  return true;
#endif
}

// =============================================================================
// Utility functions
// =============================================================================

extern "C" bool robots_is_valid_user_agent(const char* user_agent, size_t len) {
  if (!user_agent || len == 0) return false;
  return googlebot::RobotsMatcher::IsValidUserAgentToObey(
      std::string_view(user_agent, len));
}

extern "C" const char* robots_version(void) {
  return ROBOTS_VERSION;
}

// === End robots_c.cc implementation ===

#endif  // ROBOTS_IMPLEMENTATION && __cplusplus


#endif  // ROBOTS_C_H
