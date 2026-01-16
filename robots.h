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

#ifndef THIRD_PARTY_ROBOTSTXT_ROBOTS_H__
#define THIRD_PARTY_ROBOTSTXT_ROBOTS_H__

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
#endif  // THIRD_PARTY_ROBOTSTXT_ROBOTS_H__
