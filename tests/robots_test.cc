// Copyright 2019 Google LLC
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
// This file tests the robots.txt parsing and matching code found in robots.cc
// against the current Robots Exclusion Protocol (REP) RFC.
// https://www.rfc-editor.org/rfc/rfc9309.html
#include "robots.h"

#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "gtest/gtest.h"

namespace {

using ::googlebot::RobotsMatcher;

std::vector<std::string> SplitString(const std::string& s, char delim) {
  std::vector<std::string> result;
  std::istringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    result.push_back(item);
  }
  return result;
}

// Helper to concatenate strings (replacement for StrCat)
template<typename... Args>
std::string StrCat(Args&&... args) {
  std::ostringstream oss;
  (oss << ... << std::forward<Args>(args));
  return oss.str();
}

// Helper to append to string (replacement for StrAppend)
template<typename... Args>
void StrAppend(std::string* dest, Args&&... args) {
  std::ostringstream oss;
  (oss << ... << std::forward<Args>(args));
  dest->append(oss.str());
}

bool IsUserAgentAllowed(std::string_view robotstxt,
                        const std::string& useragent, const std::string& url) {
  RobotsMatcher matcher;
  return matcher.OneAgentAllowedByRobots(robotstxt, useragent, url);
}

bool AllowedByRobots(std::string_view robotstxt,
                     const std::string& useragents_csv,
                     const std::string& url) {
  std::vector<std::string> useragents = SplitString(useragents_csv, ',');
  RobotsMatcher matcher;
  return matcher.AllowedByRobots(robotstxt, &useragents, url);
}

// Google-specific: system test.
TEST(RobotsUnittest, GoogleOnly_SystemTest) {
  const std::string_view robotstxt =
      "user-agent: FooBot\n"
      "disallow: /\n";
  // Empty robots.txt: everything allowed.
  EXPECT_TRUE(IsUserAgentAllowed("", "FooBot", ""));

  // Empty user-agent to be matched: everything allowed.
  EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "", ""));

  // Empty url: implicitly disallowed, see method comment for GetPathParamsQuery
  // in robots.cc.
  EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "FooBot", ""));

  // All params empty: same as robots.txt empty, everything allowed.
  EXPECT_TRUE(IsUserAgentAllowed("", "", ""));
}
// Rules are colon separated name-value pairs. The following names are
// provisioned:
//     user-agent: <value>
//     allow: <value>
//     disallow: <value>
// See REP RFC section "Protocol Definition".
// https://www.rfc-editor.org/rfc/rfc9309.html#section-2.1
//
// Google specific: webmasters sometimes miss the colon separator, but it's
// obvious what they mean by "disallow /", so we assume the colon if it's
// missing.
TEST(RobotsUnittest, ID_LineSyntax_Line) {
  const std::string_view robotstxt_correct =
      "user-agent: FooBot\n"
      "disallow: /\n";
  const std::string_view robotstxt_incorrect =
      "foo: FooBot\n"
      "bar: /\n";
  const std::string_view robotstxt_incorrect_accepted =
      "user-agent FooBot\n"
      "disallow /\n";
  const std::string url = "http://foo.bar/x/y";

  EXPECT_FALSE(IsUserAgentAllowed(robotstxt_correct, "FooBot", url));
  EXPECT_TRUE(IsUserAgentAllowed(robotstxt_incorrect, "FooBot", url));
  EXPECT_FALSE(IsUserAgentAllowed(robotstxt_incorrect_accepted, "FooBot", url));
}

// A group is one or more user-agent line followed by rules, and terminated
// by a another user-agent line. Rules for same user-agents are combined
// opaquely into one group. Rules outside groups are ignored.
// See REP RFC section "Protocol Definition".
// https://www.rfc-editor.org/rfc/rfc9309.html#section-2.1
TEST(RobotsUnittest, ID_LineSyntax_Groups) {
  const std::string_view robotstxt =
      "allow: /foo/bar/\n"
      "\n"
      "user-agent: FooBot\n"
      "disallow: /\n"
      "allow: /x/\n"
      "user-agent: BarBot\n"
      "disallow: /\n"
      "allow: /y/\n"
      "\n"
      "\n"
      "allow: /w/\n"
      "user-agent: BazBot\n"
      "\n"
      "user-agent: FooBot\n"
      "allow: /z/\n"
      "disallow: /\n";

  const std::string url_w = "http://foo.bar/w/a";
  const std::string url_x = "http://foo.bar/x/b";
  const std::string url_y = "http://foo.bar/y/c";
  const std::string url_z = "http://foo.bar/z/d";
  const std::string url_foo = "http://foo.bar/foo/bar/";

  EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "FooBot", url_x));
  EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "FooBot", url_z));
  EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "FooBot", url_y));
  EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "BarBot", url_y));
  EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "BarBot", url_w));
  EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "BarBot", url_z));
  EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "BazBot", url_z));

  // Lines with rules outside groups are ignored.
  EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "FooBot", url_foo));
  EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "BarBot", url_foo));
  EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "BazBot", url_foo));
}

// Group must not be closed by rules not explicitly defined in the REP RFC.
// See REP RFC section "Protocol Definition".
// https://www.rfc-editor.org/rfc/rfc9309.html#section-2.1
TEST(RobotsUnittest, ID_LineSyntax_Groups_OtherRules) {
  {
    const std::string_view robotstxt =
        "User-agent: BarBot\n"
        "Sitemap: https://foo.bar/sitemap\n"
        "User-agent: *\n"
        "Disallow: /\n";
    std::string url = "http://foo.bar/";
    EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "FooBot", url));
    EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "BarBot", url));
  }
  {
    const std::string_view robotstxt =
        "User-agent: FooBot\n"
        "Invalid-Unknown-Line: unknown\n"
        "User-agent: *\n"
        "Disallow: /\n";
    std::string url = "http://foo.bar/";
    EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "FooBot", url));
    EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "BarBot", url));
  }
  // Test case from https://github.com/google/robotstxt/issues/51
  // Crawl-delay directive should not close the user-agent group.
  // This test verifies that Crawl-delay (like Sitemap and unknown directives)
  // does not act as a group separator.
  {
    const std::string_view robotstxt =
        "User-agent: FooBot\n"
        "Crawl-delay: 10\n"
        "User-agent: *\n"
        "Disallow: /\n";
    std::string url = "http://example.com/";
    // FooBot and * are in the same group because Crawl-delay doesn't separate.
    // Both should be blocked by Disallow: /
    EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "FooBot", url));
    EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "BarBot", url));
  }
}

// Test Crawl-delay parsing and retrieval via GetCrawlDelay().
TEST(RobotsUnittest, ID_CrawlDelay) {
  // Test basic crawl-delay parsing.
  {
    const std::string_view robotstxt =
        "User-agent: *\n"
        "Crawl-delay: 10\n"
        "Disallow: /private/\n";
    googlebot::RobotsMatcher matcher;
    std::vector<std::string> agents = {"Googlebot"};
    EXPECT_TRUE(matcher.AllowedByRobots(robotstxt, &agents, "http://example.com/"));
    auto delay = matcher.GetCrawlDelay();
    ASSERT_TRUE(delay.has_value());
    EXPECT_DOUBLE_EQ(10.0, delay.value());
  }
  // Test crawl-delay with decimal value.
  {
    const std::string_view robotstxt =
        "User-agent: *\n"
        "Crawl-delay: 0.5\n";
    googlebot::RobotsMatcher matcher;
    std::vector<std::string> agents = {"Googlebot"};
    matcher.AllowedByRobots(robotstxt, &agents, "http://example.com/");
    auto delay = matcher.GetCrawlDelay();
    ASSERT_TRUE(delay.has_value());
    EXPECT_DOUBLE_EQ(0.5, delay.value());
  }
  // Test specific user-agent crawl-delay takes precedence.
  {
    const std::string_view robotstxt =
        "User-agent: *\n"
        "Crawl-delay: 10\n"
        "\n"
        "User-agent: FooBot\n"
        "Crawl-delay: 5\n";
    googlebot::RobotsMatcher matcher;
    std::vector<std::string> agents = {"FooBot"};
    matcher.AllowedByRobots(robotstxt, &agents, "http://example.com/");
    auto delay = matcher.GetCrawlDelay();
    ASSERT_TRUE(delay.has_value());
    EXPECT_DOUBLE_EQ(5.0, delay.value());
  }
  // Test no crawl-delay returns nullopt.
  {
    const std::string_view robotstxt =
        "User-agent: *\n"
        "Disallow: /private/\n";
    googlebot::RobotsMatcher matcher;
    std::vector<std::string> agents = {"Googlebot"};
    matcher.AllowedByRobots(robotstxt, &agents, "http://example.com/");
    auto delay = matcher.GetCrawlDelay();
    EXPECT_FALSE(delay.has_value());
  }
  // Test crawl-delay with typo variant "crawldelay".
  {
    const std::string_view robotstxt =
        "User-agent: *\n"
        "crawldelay: 3\n";
    googlebot::RobotsMatcher matcher;
    std::vector<std::string> agents = {"Googlebot"};
    matcher.AllowedByRobots(robotstxt, &agents, "http://example.com/");
    auto delay = matcher.GetCrawlDelay();
    ASSERT_TRUE(delay.has_value());
    EXPECT_DOUBLE_EQ(3.0, delay.value());
  }
  // Test crawl-delay with invalid value (should be 0).
  {
    const std::string_view robotstxt =
        "User-agent: *\n"
        "Crawl-delay: invalid\n";
    googlebot::RobotsMatcher matcher;
    std::vector<std::string> agents = {"Googlebot"};
    matcher.AllowedByRobots(robotstxt, &agents, "http://example.com/");
    auto delay = matcher.GetCrawlDelay();
    ASSERT_TRUE(delay.has_value());
    EXPECT_DOUBLE_EQ(0.0, delay.value());
  }
  // Test crawl-delay with negative value (should be 0).
  {
    const std::string_view robotstxt =
        "User-agent: *\n"
        "Crawl-delay: -5\n";
    googlebot::RobotsMatcher matcher;
    std::vector<std::string> agents = {"Googlebot"};
    matcher.AllowedByRobots(robotstxt, &agents, "http://example.com/");
    auto delay = matcher.GetCrawlDelay();
    ASSERT_TRUE(delay.has_value());
    EXPECT_DOUBLE_EQ(0.0, delay.value());
  }
}

TEST(RobotsUnittest, ID_RequestRate) {
  // Test basic request-rate parsing (requests/seconds format).
  {
    const std::string_view robotstxt =
        "User-agent: *\n"
        "Request-rate: 1/5\n"
        "Disallow: /private/\n";
    googlebot::RobotsMatcher matcher;
    std::vector<std::string> agents = {"Googlebot"};
    EXPECT_TRUE(
        matcher.AllowedByRobots(robotstxt, &agents, "http://example.com/"));
    auto rate = matcher.GetRequestRate();
    ASSERT_TRUE(rate.has_value());
    EXPECT_EQ(1, rate.value().requests);
    EXPECT_EQ(5, rate.value().seconds);
    EXPECT_DOUBLE_EQ(0.2, rate.value().RequestsPerSecond());
    EXPECT_DOUBLE_EQ(5.0, rate.value().DelaySeconds());
  }
  // Test request-rate with larger values.
  {
    const std::string_view robotstxt =
        "User-agent: *\n"
        "Request-rate: 30/60\n";
    googlebot::RobotsMatcher matcher;
    std::vector<std::string> agents = {"Googlebot"};
    matcher.AllowedByRobots(robotstxt, &agents, "http://example.com/");
    auto rate = matcher.GetRequestRate();
    ASSERT_TRUE(rate.has_value());
    EXPECT_EQ(30, rate.value().requests);
    EXPECT_EQ(60, rate.value().seconds);
    EXPECT_DOUBLE_EQ(0.5, rate.value().RequestsPerSecond());
    EXPECT_DOUBLE_EQ(2.0, rate.value().DelaySeconds());
  }
  // Test request-rate with 's' suffix (1/5s format).
  {
    const std::string_view robotstxt =
        "User-agent: *\n"
        "Request-rate: 1/10s\n";
    googlebot::RobotsMatcher matcher;
    std::vector<std::string> agents = {"Googlebot"};
    matcher.AllowedByRobots(robotstxt, &agents, "http://example.com/");
    auto rate = matcher.GetRequestRate();
    ASSERT_TRUE(rate.has_value());
    EXPECT_EQ(1, rate.value().requests);
    EXPECT_EQ(10, rate.value().seconds);
  }
  // Test specific user-agent request-rate takes precedence.
  {
    const std::string_view robotstxt =
        "User-agent: *\n"
        "Request-rate: 1/10\n"
        "\n"
        "User-agent: FooBot\n"
        "Request-rate: 1/5\n";
    googlebot::RobotsMatcher matcher;
    std::vector<std::string> agents = {"FooBot"};
    matcher.AllowedByRobots(robotstxt, &agents, "http://example.com/");
    auto rate = matcher.GetRequestRate();
    ASSERT_TRUE(rate.has_value());
    EXPECT_EQ(1, rate.value().requests);
    EXPECT_EQ(5, rate.value().seconds);
  }
  // Test no request-rate returns nullopt.
  {
    const std::string_view robotstxt =
        "User-agent: *\n"
        "Disallow: /private/\n";
    googlebot::RobotsMatcher matcher;
    std::vector<std::string> agents = {"Googlebot"};
    matcher.AllowedByRobots(robotstxt, &agents, "http://example.com/");
    auto rate = matcher.GetRequestRate();
    EXPECT_FALSE(rate.has_value());
  }
  // Test request-rate with single number (no slash = requests per second).
  {
    const std::string_view robotstxt =
        "User-agent: *\n"
        "Request-rate: 2\n";
    googlebot::RobotsMatcher matcher;
    std::vector<std::string> agents = {"Googlebot"};
    matcher.AllowedByRobots(robotstxt, &agents, "http://example.com/");
    auto rate = matcher.GetRequestRate();
    ASSERT_TRUE(rate.has_value());
    EXPECT_EQ(2, rate.value().requests);
    EXPECT_EQ(1, rate.value().seconds);
  }
}

#if ROBOTS_SUPPORT_CONTENT_SIGNAL
// Test Content-Signal parsing and retrieval via GetContentSignal().
// See: https://github.com/google/robotstxt/issues/80
TEST(RobotsUnittest, ID_ContentSignal) {
  // Test basic content-signal parsing with all signals.
  {
    const std::string_view robotstxt =
        "User-agent: *\n"
        "Content-Signal: ai-train=no, search=yes, ai-input=yes\n"
        "Disallow: /private/\n";
    googlebot::RobotsMatcher matcher;
    std::vector<std::string> agents = {"Googlebot"};
    EXPECT_TRUE(matcher.AllowedByRobots(robotstxt, &agents, "http://example.com/"));
    auto signal = matcher.GetContentSignal();
    ASSERT_TRUE(signal.has_value());
    ASSERT_TRUE(signal.value().ai_train.has_value());
    EXPECT_FALSE(signal.value().ai_train.value());
    ASSERT_TRUE(signal.value().search.has_value());
    EXPECT_TRUE(signal.value().search.value());
    ASSERT_TRUE(signal.value().ai_input.has_value());
    EXPECT_TRUE(signal.value().ai_input.value());
    // Test convenience methods.
    EXPECT_FALSE(signal.value().AllowsAiTrain());
    EXPECT_TRUE(signal.value().AllowsSearch());
    EXPECT_TRUE(signal.value().AllowsAiInput());
  }
  // Test content-signal with only ai-train signal.
  {
    const std::string_view robotstxt =
        "User-agent: *\n"
        "Content-Signal: ai-train=no\n";
    googlebot::RobotsMatcher matcher;
    std::vector<std::string> agents = {"Googlebot"};
    matcher.AllowedByRobots(robotstxt, &agents, "http://example.com/");
    auto signal = matcher.GetContentSignal();
    ASSERT_TRUE(signal.has_value());
    ASSERT_TRUE(signal.value().ai_train.has_value());
    EXPECT_FALSE(signal.value().ai_train.value());
    EXPECT_FALSE(signal.value().search.has_value());
    EXPECT_FALSE(signal.value().ai_input.has_value());
    // Unset signals default to true via convenience methods.
    EXPECT_FALSE(signal.value().AllowsAiTrain());
    EXPECT_TRUE(signal.value().AllowsSearch());  // Defaults to true.
    EXPECT_TRUE(signal.value().AllowsAiInput());  // Defaults to true.
  }
  // Test content-signal with "true/false" syntax.
  {
    const std::string_view robotstxt =
        "User-agent: *\n"
        "Content-Signal: ai-train=false, search=true\n";
    googlebot::RobotsMatcher matcher;
    std::vector<std::string> agents = {"Googlebot"};
    matcher.AllowedByRobots(robotstxt, &agents, "http://example.com/");
    auto signal = matcher.GetContentSignal();
    ASSERT_TRUE(signal.has_value());
    EXPECT_FALSE(signal.value().AllowsAiTrain());
    EXPECT_TRUE(signal.value().AllowsSearch());
  }
  // Test content-signal with "1/0" syntax.
  {
    const std::string_view robotstxt =
        "User-agent: *\n"
        "Content-Signal: ai-train=0, search=1, ai-input=1\n";
    googlebot::RobotsMatcher matcher;
    std::vector<std::string> agents = {"Googlebot"};
    matcher.AllowedByRobots(robotstxt, &agents, "http://example.com/");
    auto signal = matcher.GetContentSignal();
    ASSERT_TRUE(signal.has_value());
    EXPECT_FALSE(signal.value().AllowsAiTrain());
    EXPECT_TRUE(signal.value().AllowsSearch());
    EXPECT_TRUE(signal.value().AllowsAiInput());
  }
  // Test specific user-agent content-signal takes precedence.
  {
    const std::string_view robotstxt =
        "User-agent: *\n"
        "Content-Signal: ai-train=yes\n"
        "\n"
        "User-agent: FooBot\n"
        "Content-Signal: ai-train=no\n";
    googlebot::RobotsMatcher matcher;
    std::vector<std::string> agents = {"FooBot"};
    matcher.AllowedByRobots(robotstxt, &agents, "http://example.com/");
    auto signal = matcher.GetContentSignal();
    ASSERT_TRUE(signal.has_value());
    EXPECT_FALSE(signal.value().AllowsAiTrain());
  }
  // Test no content-signal returns nullopt.
  {
    const std::string_view robotstxt =
        "User-agent: *\n"
        "Disallow: /private/\n";
    googlebot::RobotsMatcher matcher;
    std::vector<std::string> agents = {"Googlebot"};
    matcher.AllowedByRobots(robotstxt, &agents, "http://example.com/");
    auto signal = matcher.GetContentSignal();
    EXPECT_FALSE(signal.has_value());
  }
  // Test content-signal with typo variant "contentsignal".
  {
    const std::string_view robotstxt =
        "User-agent: *\n"
        "contentsignal: ai-train=no\n";
    googlebot::RobotsMatcher matcher;
    std::vector<std::string> agents = {"Googlebot"};
    matcher.AllowedByRobots(robotstxt, &agents, "http://example.com/");
    auto signal = matcher.GetContentSignal();
    ASSERT_TRUE(signal.has_value());
    EXPECT_FALSE(signal.value().AllowsAiTrain());
  }
  // Test content-signal with extra whitespace.
  {
    const std::string_view robotstxt =
        "User-agent: *\n"
        "Content-Signal:   ai-train = no  ,  search = yes  \n";
    googlebot::RobotsMatcher matcher;
    std::vector<std::string> agents = {"Googlebot"};
    matcher.AllowedByRobots(robotstxt, &agents, "http://example.com/");
    auto signal = matcher.GetContentSignal();
    ASSERT_TRUE(signal.has_value());
    EXPECT_FALSE(signal.value().AllowsAiTrain());
    EXPECT_TRUE(signal.value().AllowsSearch());
  }
  // Test content-signal case insensitivity for keys and values.
  {
    const std::string_view robotstxt =
        "User-agent: *\n"
        "Content-Signal: AI-TRAIN=NO, SEARCH=YES\n";
    googlebot::RobotsMatcher matcher;
    std::vector<std::string> agents = {"Googlebot"};
    matcher.AllowedByRobots(robotstxt, &agents, "http://example.com/");
    auto signal = matcher.GetContentSignal();
    ASSERT_TRUE(signal.has_value());
    EXPECT_FALSE(signal.value().AllowsAiTrain());
    EXPECT_TRUE(signal.value().AllowsSearch());
  }
  // Test HasAnySignal() method.
  {
    const std::string_view robotstxt =
        "User-agent: *\n"
        "Content-Signal: search=yes\n";
    googlebot::RobotsMatcher matcher;
    std::vector<std::string> agents = {"Googlebot"};
    matcher.AllowedByRobots(robotstxt, &agents, "http://example.com/");
    auto signal = matcher.GetContentSignal();
    ASSERT_TRUE(signal.has_value());
    EXPECT_TRUE(signal.value().HasAnySignal());
  }
  // Test content-signal with invalid key (should be ignored).
  {
    const std::string_view robotstxt =
        "User-agent: *\n"
        "Content-Signal: ai-train=no, unknown-key=value, search=yes\n";
    googlebot::RobotsMatcher matcher;
    std::vector<std::string> agents = {"Googlebot"};
    matcher.AllowedByRobots(robotstxt, &agents, "http://example.com/");
    auto signal = matcher.GetContentSignal();
    ASSERT_TRUE(signal.has_value());
    EXPECT_FALSE(signal.value().AllowsAiTrain());
    EXPECT_TRUE(signal.value().AllowsSearch());
  }
  // Test global content-signal is used when no specific user-agent matches.
  {
    const std::string_view robotstxt =
        "User-agent: *\n"
        "Content-Signal: ai-train=no\n"
        "\n"
        "User-agent: FooBot\n"
        "Disallow: /foo/\n";
    googlebot::RobotsMatcher matcher;
    std::vector<std::string> agents = {"BarBot"};
    matcher.AllowedByRobots(robotstxt, &agents, "http://example.com/");
    auto signal = matcher.GetContentSignal();
    ASSERT_TRUE(signal.has_value());
    EXPECT_FALSE(signal.value().AllowsAiTrain());
  }
}
#endif  // ROBOTS_SUPPORT_CONTENT_SIGNAL

// Test based on the documentation at
// https://developers.google.com/search/reference/robots_txt#order-of-precedence-for-user-agents
// "Only one group is valid for a particular crawler"
// "The group followed is group 1. Only the most specific group is followed, all others are ignored"
TEST(RobotsUnittest, ID_Multiple_Useragents) {
  const std::string_view robotstxt =
      "user-agent: googlebot-news\n"
      "Disallow: /bar/\n"
      "\n"
      "user-agent: *\n"
      "Disallow: /baz/\n"
      "\n\n"
      "user-agent: googlebot\n"
      "Disallow: /foo/\n";

  const std::string url_foo = "http://foo.bar/foo/";
  const std::string url_bar = "http://foo.bar/bar/";
  const std::string url_baz = "http://foo.bar/baz/";
  const std::string url_qux = "http://foo.bar/qux/";

  EXPECT_TRUE(AllowedByRobots(robotstxt, "googlebot,googlebot-news", url_foo));
  EXPECT_FALSE(AllowedByRobots(robotstxt, "googlebot,googlebot-news", url_bar));
  EXPECT_TRUE(AllowedByRobots(robotstxt, "googlebot,googlebot-news", url_baz));
  EXPECT_TRUE(AllowedByRobots(robotstxt, "googlebot,googlebot-news", url_qux));
}

// REP lines are case insensitive. See REP RFC section "Protocol Definition".
// https://www.rfc-editor.org/rfc/rfc9309.html#section-2.1
TEST(RobotsUnittest, ID_REPLineNamesCaseInsensitive) {
  const std::string_view robotstxt_upper =
      "USER-AGENT: FooBot\n"
      "ALLOW: /x/\n"
      "DISALLOW: /\n";
  const std::string_view robotstxt_lower =
      "user-agent: FooBot\n"
      "allow: /x/\n"
      "disallow: /\n";
  const std::string_view robotstxt_camel =
      "uSeR-aGeNt: FooBot\n"
      "AlLoW: /x/\n"
      "dIsAlLoW: /\n";
  const std::string url_allowed = "http://foo.bar/x/y";
  const std::string url_disallowed = "http://foo.bar/a/b";

  EXPECT_TRUE(IsUserAgentAllowed(robotstxt_upper, "FooBot", url_allowed));
  EXPECT_TRUE(IsUserAgentAllowed(robotstxt_lower, "FooBot", url_allowed));
  EXPECT_TRUE(IsUserAgentAllowed(robotstxt_camel, "FooBot", url_allowed));
  EXPECT_FALSE(IsUserAgentAllowed(robotstxt_upper, "FooBot", url_disallowed));
  EXPECT_FALSE(IsUserAgentAllowed(robotstxt_lower, "FooBot", url_disallowed));
  EXPECT_FALSE(IsUserAgentAllowed(robotstxt_camel, "FooBot", url_disallowed));
}

// A user-agent line is expected to contain only [a-zA-Z_-] characters and must
// not be empty. See REP RFC section "The user-agent line".
// https://www.rfc-editor.org/rfc/rfc9309.html#section-2.2.1
TEST(RobotsUnittest, ID_VerifyValidUserAgentsToObey) {
  EXPECT_TRUE(RobotsMatcher::IsValidUserAgentToObey("Foobot"));
  EXPECT_TRUE(RobotsMatcher::IsValidUserAgentToObey("Foobot-Bar"));
  EXPECT_TRUE(RobotsMatcher::IsValidUserAgentToObey("Foo_Bar"));

  EXPECT_FALSE(RobotsMatcher::IsValidUserAgentToObey(std::string_view()));
  EXPECT_FALSE(RobotsMatcher::IsValidUserAgentToObey(""));
  EXPECT_FALSE(RobotsMatcher::IsValidUserAgentToObey("ツ"));

  EXPECT_FALSE(RobotsMatcher::IsValidUserAgentToObey("Foobot*"));
  EXPECT_FALSE(RobotsMatcher::IsValidUserAgentToObey(" Foobot "));
  EXPECT_FALSE(RobotsMatcher::IsValidUserAgentToObey("Foobot/2.1"));

  EXPECT_FALSE(RobotsMatcher::IsValidUserAgentToObey("Foobot Bar"));
}

// User-agent line values are case insensitive. See REP RFC section "The
// user-agent line".
// https://www.rfc-editor.org/rfc/rfc9309.html#section-2.2.1
TEST(RobotsUnittest, ID_UserAgentValueCaseInsensitive) {
  const std::string_view robotstxt_upper =
      "User-Agent: FOO BAR\n"
      "Allow: /x/\n"
      "Disallow: /\n";
  const std::string_view robotstxt_lower =
      "User-Agent: foo bar\n"
      "Allow: /x/\n"
      "Disallow: /\n";
  const std::string_view robotstxt_camel =
      "User-Agent: FoO bAr\n"
      "Allow: /x/\n"
      "Disallow: /\n";
  const std::string url_allowed = "http://foo.bar/x/y";
  const std::string url_disallowed = "http://foo.bar/a/b";

  EXPECT_TRUE(IsUserAgentAllowed(robotstxt_upper, "Foo", url_allowed));
  EXPECT_TRUE(IsUserAgentAllowed(robotstxt_lower, "Foo", url_allowed));
  EXPECT_TRUE(IsUserAgentAllowed(robotstxt_camel, "Foo", url_allowed));
  EXPECT_FALSE(IsUserAgentAllowed(robotstxt_upper, "Foo", url_disallowed));
  EXPECT_FALSE(IsUserAgentAllowed(robotstxt_lower, "Foo", url_disallowed));
  EXPECT_FALSE(IsUserAgentAllowed(robotstxt_camel, "Foo", url_disallowed));
  EXPECT_TRUE(IsUserAgentAllowed(robotstxt_upper, "foo", url_allowed));
  EXPECT_TRUE(IsUserAgentAllowed(robotstxt_lower, "foo", url_allowed));
  EXPECT_TRUE(IsUserAgentAllowed(robotstxt_camel, "foo", url_allowed));
  EXPECT_FALSE(IsUserAgentAllowed(robotstxt_upper, "foo", url_disallowed));
  EXPECT_FALSE(IsUserAgentAllowed(robotstxt_lower, "foo", url_disallowed));
  EXPECT_FALSE(IsUserAgentAllowed(robotstxt_camel, "foo", url_disallowed));
}

// Google specific: accept user-agent value up to the first space. Space is not
// allowed in user-agent values, but that doesn't stop webmasters from using
// them. This is more restrictive than the RFC, since in case of the bad value
// "Googlebot Images" we'd still obey the rules with "Googlebot".
// Extends REP RFC section "The user-agent line"
// https://www.rfc-editor.org/rfc/rfc9309.html#section-2.2.1
TEST(RobotsUnittest, GoogleOnly_AcceptUserAgentUpToFirstSpace) {
  EXPECT_FALSE(RobotsMatcher::IsValidUserAgentToObey("Foobot Bar"));
  const std::string_view robotstxt =
      "User-Agent: *\n"
      "Disallow: /\n"
      "User-Agent: Foo Bar\n"
      "Allow: /x/\n"
      "Disallow: /\n";
  const std::string url = "http://foo.bar/x/y";

  EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "Foo", url));
  EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "Foo Bar", url));
}

// If no group matches the user-agent, crawlers must obey the first group with a
// user-agent line with a "*" value, if present. If no group satisfies either
// condition, or no groups are present at all, no rules apply.
// See REP RFC section "The user-agent line".
// https://www.rfc-editor.org/rfc/rfc9309.html#section-2.2.1
TEST(RobotsUnittest, ID_GlobalGroups_Secondary) {
  const std::string_view robotstxt_empty = "";
  const std::string_view robotstxt_global =
      "user-agent: *\n"
      "allow: /\n"
      "user-agent: FooBot\n"
      "disallow: /\n";
  const std::string_view robotstxt_only_specific =
      "user-agent: FooBot\n"
      "allow: /\n"
      "user-agent: BarBot\n"
      "disallow: /\n"
      "user-agent: BazBot\n"
      "disallow: /\n";
  const std::string url = "http://foo.bar/x/y";

  EXPECT_TRUE(IsUserAgentAllowed(robotstxt_empty, "FooBot", url));
  EXPECT_FALSE(IsUserAgentAllowed(robotstxt_global, "FooBot", url));
  EXPECT_TRUE(IsUserAgentAllowed(robotstxt_global, "BarBot", url));
  EXPECT_TRUE(IsUserAgentAllowed(robotstxt_only_specific, "QuxBot", url));
}

// Matching rules against URIs is case sensitive.
// See REP RFC section "The Allow and Disallow lines".
// https://www.rfc-editor.org/rfc/rfc9309.html#section-2.2.2
TEST(RobotsUnittest, ID_AllowDisallow_Value_CaseSensitive) {
  const std::string_view robotstxt_lowercase_url =
      "user-agent: FooBot\n"
      "disallow: /x/\n";
  const std::string_view robotstxt_uppercase_url =
      "user-agent: FooBot\n"
      "disallow: /X/\n";
  const std::string url = "http://foo.bar/x/y";

  EXPECT_FALSE(IsUserAgentAllowed(robotstxt_lowercase_url, "FooBot", url));
  EXPECT_TRUE(IsUserAgentAllowed(robotstxt_uppercase_url, "FooBot", url));
}

// The most specific match found MUST be used. The most specific match is the
// match that has the most octets. In case of multiple rules with the same
// length, the least strict rule must be used.
// See REP RFC section "The Allow and Disallow lines".
// https://www.rfc-editor.org/rfc/rfc9309.html#section-2.2.2
TEST(RobotsUnittest, ID_LongestMatch) {
  const std::string url = "http://foo.bar/x/page.html";
  {
    const std::string_view robotstxt =
        "user-agent: FooBot\n"
        "disallow: /x/page.html\n"
        "allow: /x/\n";

    EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "FooBot", url));
  }
  {
    const std::string_view robotstxt =
        "user-agent: FooBot\n"
        "allow: /x/page.html\n"
        "disallow: /x/\n";

    EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "FooBot", url));
    EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/x/"));
  }
  {
    const std::string_view robotstxt =
        "user-agent: FooBot\n"
        "disallow: \n"
        "allow: \n";
    // In case of equivalent disallow and allow patterns for the same
    // user-agent, allow is used.
    EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "FooBot", url));
  }
  {
    const std::string_view robotstxt =
        "user-agent: FooBot\n"
        "disallow: /\n"
        "allow: /\n";
    // In case of equivalent disallow and allow patterns for the same
    // user-agent, allow is used.
    EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "FooBot", url));
  }
  {
    std::string url_a = "http://foo.bar/x";
    std::string url_b = "http://foo.bar/x/";
    const std::string_view robotstxt =
        "user-agent: FooBot\n"
        "disallow: /x\n"
        "allow: /x/\n";
    EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "FooBot", url_a));
    EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "FooBot", url_b));
  }

  {
    const std::string_view robotstxt =
        "user-agent: FooBot\n"
        "disallow: /x/page.html\n"
        "allow: /x/page.html\n";
    // In case of equivalent disallow and allow patterns for the same
    // user-agent, allow is used.
    EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "FooBot", url));
  }
  {
    const std::string_view robotstxt =
        "user-agent: FooBot\n"
        "allow: /page\n"
        "disallow: /*.html\n";
    // Longest match wins.
    EXPECT_FALSE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/page.html"));
    EXPECT_TRUE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/page"));
  }
  {
    const std::string_view robotstxt =
        "user-agent: FooBot\n"
        "allow: /x/page.\n"
        "disallow: /*.html\n";
    // Longest match wins.
    EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "FooBot", url));
    EXPECT_FALSE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/x/y.html"));
  }
  {
    const std::string_view robotstxt =
        "User-agent: *\n"
        "Disallow: /x/\n"
        "User-agent: FooBot\n"
        "Disallow: /y/\n";
    // Most specific group for FooBot allows implicitly /x/page.
    EXPECT_TRUE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/x/page"));
    EXPECT_FALSE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/y/page"));
  }
}

// Octets in the URI and robots.txt paths outside the range of the US-ASCII
// coded character set, and those in the reserved range defined by RFC3986,
// MUST be percent-encoded as defined by RFC3986 prior to comparison.
// See REP RFC section "The Allow and Disallow lines".
// https://www.rfc-editor.org/rfc/rfc9309.html#section-2.2.2
//
// NOTE: It's up to the caller to percent encode a URL before passing it to the
// parser. Percent encoding URIs in the rules is unnecessary.
TEST(RobotsUnittest, ID_Encoding) {
  // Per RFC 9309 section 2.2.2, reserved characters in query string values
  // should be percent-encoded. Both encoded and unencoded URLs should match.
  // See: https://github.com/google/robotstxt/issues/64
  {
    const std::string_view robotstxt =
        "User-agent: FooBot\n"
        "Disallow: /\n"
        "Allow: /foo/bar?qux=taz&baz=http://foo.bar?tar&par\n";
    // Unencoded URL matches unencoded rule (both get normalized)
    EXPECT_TRUE(IsUserAgentAllowed(
        robotstxt, "FooBot",
        "http://foo.bar/foo/bar?qux=taz&baz=http://foo.bar?tar&par"));
    // RFC-compliant encoded URL should also match
    EXPECT_TRUE(IsUserAgentAllowed(
        robotstxt, "FooBot",
        "http://foo.bar/foo/bar?qux=taz&baz=http%3A%2F%2Ffoo.bar%3Ftar%26par"));
  }
  // Test with already-encoded rule
  {
    const std::string_view robotstxt =
        "User-agent: FooBot\n"
        "Disallow: /\n"
        "Allow: /foo/bar?qux=taz&baz=http%3A%2F%2Ffoo.bar%3Ftar%26par\n";
    // Encoded URL matches encoded rule
    EXPECT_TRUE(IsUserAgentAllowed(
        robotstxt, "FooBot",
        "http://foo.bar/foo/bar?qux=taz&baz=http%3A%2F%2Ffoo.bar%3Ftar%26par"));
    // Unencoded URL should also match (gets normalized to encoded form)
    EXPECT_TRUE(IsUserAgentAllowed(
        robotstxt, "FooBot",
        "http://foo.bar/foo/bar?qux=taz&baz=http://foo.bar?tar&par"));
  }

  // 3 byte character: /foo/bar/ツ -> /foo/bar/%E3%83%84
  // Per RFC 9309 section 2.2.2, percent-encoded octets are decoded for comparison,
  // so both encoded and raw UTF-8 URLs should match.
  {
    const std::string_view robotstxt =
        "User-agent: FooBot\n"
        "Disallow: /\n"
        "Allow: /foo/bar/ツ\n";
    // Encoded URL matches (rule's ツ is encoded to %E3%83%84, then decoded for comparison)
    EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "FooBot",
                                   "http://foo.bar/foo/bar/%E3%83%84"));
    // Raw UTF-8 URL also matches (both are decoded/compared as raw bytes)
    EXPECT_TRUE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/foo/bar/ツ"));
  }
  // Percent encoded 3 byte character: /foo/bar/%E3%83%84 -> /foo/bar/%E3%83%84
  {
    const std::string_view robotstxt =
        "User-agent: FooBot\n"
        "Disallow: /\n"
        "Allow: /foo/bar/%E3%83%84\n";
    // Encoded URL matches encoded rule
    EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "FooBot",
                                   "http://foo.bar/foo/bar/%E3%83%84"));
    // Raw UTF-8 URL also matches (decoded for comparison per RFC 9309)
    EXPECT_TRUE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/foo/bar/ツ"));
  }
  // Percent encoded unreserved US-ASCII: /foo/bar/%62%61%7A matches /foo/bar/baz
  // Per RFC 9309 section 2.2.2: "If a percent-encoded ASCII octet is encountered
  // in the URI, it MUST be unencoded prior to comparison, unless it is a reserved
  // character in the URI as defined by RFC3986..."
  // Note: While encoding unreserved chars is discouraged by RFC 3986, RFC 9309
  // requires decoding them for comparison.
  {
    const std::string_view robotstxt =
        "User-agent: FooBot\n"
        "Disallow: /\n"
        "Allow: /foo/bar/%62%61%7A\n";
    // Per RFC 9309, %62%61%7A decodes to "baz" and should match
    EXPECT_TRUE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/foo/bar/baz"));
    EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "FooBot",
                                   "http://foo.bar/foo/bar/%62%61%7A"));
  }
}

// Per RFC 9309 section 2.2.3, percent-encoded special characters (%2A for *,
// %24 for $) in robots.txt rules should match their literal counterparts in
// URIs. Since * and $ are reserved characters in URIs (RFC 3986), they can
// appear unencoded in paths.
// See: https://github.com/google/robotstxt/issues/57
TEST(RobotsUnittest, ID_EscapedSpecialCharacters) {
  // %2A in robots.txt should match literal * in URI path
  {
    const std::string_view robotstxt =
        "User-agent: FooBot\n"
        "Disallow: /path/file-with-%2A.html\n";
    // Should match: %2A matches literal *
    EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "FooBot",
                                    "http://foo.bar/path/file-with-*.html"));
    // Should also match: %2A matches %2A
    EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "FooBot",
                                    "http://foo.bar/path/file-with-%2A.html"));
    // Should NOT match: %2A is NOT a wildcard
    EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "FooBot",
                                   "http://foo.bar/path/file-with-x.html"));
  }
  // %24 in robots.txt should match literal $ in URI path
  {
    const std::string_view robotstxt =
        "User-agent: FooBot\n"
        "Disallow: /path/price%24.html\n";
    // Should match: %24 matches literal $
    EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "FooBot",
                                    "http://foo.bar/path/price$.html"));
    // Should also match: %24 matches %24
    EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "FooBot",
                                    "http://foo.bar/path/price%24.html"));
    // Should NOT match: %24 is NOT end-of-pattern anchor
    EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "FooBot",
                                   "http://foo.bar/path/price"));
  }
  // Combined test: both * and $ as literals
  {
    const std::string_view robotstxt =
        "User-agent: FooBot\n"
        "Disallow: /buy/%2A%24\n";
    EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "FooBot",
                                    "http://foo.bar/buy/*$"));
    EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "FooBot",
                                    "http://foo.bar/buy/%2A%24"));
    // Not a wildcard - should not match arbitrary patterns
    EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "FooBot",
                                   "http://foo.bar/buy/anything"));
  }
}

// The REP RFC defines the following characters that have special meaning in
// robots.txt:
// # - inline comment.
// $ - end of pattern.
// * - any number of characters.
// See REP RFC section "Special Characters".
// https://www.rfc-editor.org/rfc/rfc9309.html#section-2.2.3
TEST(RobotsUnittest, ID_SpecialCharacters) {
  {
    const std::string_view robotstxt =
        "User-agent: FooBot\n"
        "Disallow: /foo/bar/quz\n"
        "Allow: /foo/*/qux\n";
    EXPECT_FALSE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/foo/bar/quz"));
    EXPECT_TRUE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/foo/quz"));
    EXPECT_TRUE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/foo//quz"));
    EXPECT_TRUE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/foo/bax/quz"));
  }
  {
    const std::string_view robotstxt =
        "User-agent: FooBot\n"
        "Disallow: /foo/bar$\n"
        "Allow: /foo/bar/qux\n";
    EXPECT_FALSE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/foo/bar"));
    EXPECT_TRUE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/foo/bar/qux"));
    EXPECT_TRUE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/foo/bar/"));
    EXPECT_TRUE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/foo/bar/baz"));
  }
  {
    const std::string_view robotstxt =
        "User-agent: FooBot\n"
        "# Disallow: /\n"
        "Disallow: /foo/quz#qux\n"
        "Allow: /\n";
    EXPECT_TRUE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/foo/bar"));
    EXPECT_FALSE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/foo/quz"));
  }
}

// Google-specific: "index.html" (and only that) at the end of a pattern is
// equivalent to "/".
TEST(RobotsUnittest, GoogleOnly_IndexHTMLisDirectory) {
  const std::string_view robotstxt =
      "User-Agent: *\n"
      "Allow: /allowed-slash/index.html\n"
      "Disallow: /\n";
  // If index.html is allowed, we interpret this as / being allowed too.
  EXPECT_TRUE(
      IsUserAgentAllowed(robotstxt, "foobot", "http://foo.com/allowed-slash/"));
  // Does not exatly match.
  EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "foobot",
                                  "http://foo.com/allowed-slash/index.htm"));
  // Exact match.
  EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "foobot",
                                 "http://foo.com/allowed-slash/index.html"));
  EXPECT_FALSE(
      IsUserAgentAllowed(robotstxt, "foobot", "http://foo.com/anyother-url"));
}

// Google-specific: long lines are ignored after 8 * 2083 bytes. See comment in
// RobotsTxtParser::Parse().
TEST(RobotsUnittest, GoogleOnly_LineTooLong) {
  size_t kEOLLen = std::string("\n").length();
  int kMaxLineLen = 2083 * 8;
  std::string allow = "allow: ";
  std::string disallow = "disallow: ";

  // Disallow rule pattern matches the URL after being cut off at kMaxLineLen.
  {
    std::string robotstxt = "user-agent: FooBot\n";
    std::string longline = "/x/";
    size_t max_length =
        kMaxLineLen - longline.length() - disallow.length() + kEOLLen;
    while (longline.size() < max_length) {
      StrAppend(&longline, "a");
    }
    StrAppend(&robotstxt, disallow, longline, "/qux\n");

    // Matches nothing, so URL is allowed.
    EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/fux"));
    // Matches cut off disallow rule.
    EXPECT_FALSE(IsUserAgentAllowed(
        robotstxt, "FooBot", StrCat("http://foo.bar", longline, "/fux")));
  }

  {
    std::string robotstxt =
        "user-agent: FooBot\n"
        "disallow: /\n";
    std::string longline_a = "/x/";
    std::string longline_b = "/x/";
    size_t max_length =
        kMaxLineLen - longline_a.length() - allow.length() + kEOLLen;
    while (longline_a.size() < max_length) {
      StrAppend(&longline_a, "a");
      StrAppend(&longline_b, "b");
    }
    StrAppend(&robotstxt, allow, longline_a, "/qux\n");
    StrAppend(&robotstxt, allow, longline_b, "/qux\n");

    // URL matches the disallow rule.
    EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/"));
    // Matches the allow rule exactly.
    EXPECT_TRUE(
        IsUserAgentAllowed(robotstxt, "FooBot",
                           StrCat("http://foo.bar", longline_a, "/qux")));
    // Matches cut off allow rule.
    EXPECT_TRUE(
        IsUserAgentAllowed(robotstxt, "FooBot",
                           StrCat("http://foo.bar", longline_b, "/fux")));
  }
}

TEST(RobotsUnittest, GoogleOnly_DocumentationChecks) {
  // Test documentation from
  // https://developers.google.com/search/reference/robots_txt
  // Section "URL matching based on path values".
  {
    std::string robotstxt =
        "user-agent: FooBot\n"
        "disallow: /\n"
        "allow: /fish\n";
    EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/bar"));

    EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/fish"));
    EXPECT_TRUE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/fish.html"));
    EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "FooBot",
                                   "http://foo.bar/fish/salmon.html"));
    EXPECT_TRUE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/fishheads"));
    EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "FooBot",
                                   "http://foo.bar/fishheads/yummy.html"));
    EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "FooBot",
                                   "http://foo.bar/fish.html?id=anything"));

    EXPECT_FALSE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/Fish.asp"));
    EXPECT_FALSE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/catfish"));
    EXPECT_FALSE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/?id=fish"));
  }
  // "/fish*" equals "/fish"
  {
    std::string robotstxt =
        "user-agent: FooBot\n"
        "disallow: /\n"
        "allow: /fish*\n";
    EXPECT_FALSE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/bar"));

    EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/fish"));
    EXPECT_TRUE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/fish.html"));
    EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "FooBot",
                                   "http://foo.bar/fish/salmon.html"));
    EXPECT_TRUE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/fishheads"));
    EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "FooBot",
                                   "http://foo.bar/fishheads/yummy.html"));
    EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "FooBot",
                                   "http://foo.bar/fish.html?id=anything"));

    EXPECT_FALSE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/Fish.bar"));
    EXPECT_FALSE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/catfish"));
    EXPECT_FALSE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/?id=fish"));
  }
  // "/fish/" does not equal "/fish"
  {
    std::string robotstxt =
        "user-agent: FooBot\n"
        "disallow: /\n"
        "allow: /fish/\n";
    EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/bar"));

    EXPECT_TRUE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/fish/"));
    EXPECT_TRUE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/fish/salmon"));
    EXPECT_TRUE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/fish/?salmon"));
    EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "FooBot",
                                   "http://foo.bar/fish/salmon.html"));
    EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "FooBot",
                                   "http://foo.bar/fish/?id=anything"));

    EXPECT_FALSE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/fish"));
    EXPECT_FALSE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/fish.html"));
    EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "FooBot",
                                    "http://foo.bar/Fish/Salmon.html"));
  }
  // "/*.php"
  {
    std::string robotstxt =
        "user-agent: FooBot\n"
        "disallow: /\n"
        "allow: /*.php\n";
    EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/bar"));

    EXPECT_TRUE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/filename.php"));
    EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "FooBot",
                                   "http://foo.bar/folder/filename.php"));
    EXPECT_TRUE(IsUserAgentAllowed(
        robotstxt, "FooBot", "http://foo.bar/folder/filename.php?parameters"));
    EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "FooBot",
                                   "http://foo.bar//folder/any.php.file.html"));
    EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "FooBot",
                                   "http://foo.bar/filename.php/"));
    EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "FooBot",
                                   "http://foo.bar/index?f=filename.php/"));
    EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "FooBot",
                                    "http://foo.bar/php/"));
    EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "FooBot",
                                    "http://foo.bar/index?php"));

    EXPECT_FALSE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/windows.PHP"));
  }
  // "/*.php$"
  {
    std::string robotstxt =
        "user-agent: FooBot\n"
        "disallow: /\n"
        "allow: /*.php$\n";
    EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/bar"));

    EXPECT_TRUE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/filename.php"));
    EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "FooBot",
                                   "http://foo.bar/folder/filename.php"));

    EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "FooBot",
                                    "http://foo.bar/filename.php?parameters"));
    EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "FooBot",
                                    "http://foo.bar/filename.php/"));
    EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "FooBot",
                                    "http://foo.bar/filename.php5"));
    EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "FooBot",
                                    "http://foo.bar/php/"));
    EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "FooBot",
                                    "http://foo.bar/filename?php"));
    EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "FooBot",
                                    "http://foo.bar/aaaphpaaa"));
    EXPECT_FALSE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar//windows.PHP"));
  }
  // "/fish*.php"
  {
    std::string robotstxt =
        "user-agent: FooBot\n"
        "disallow: /\n"
        "allow: /fish*.php\n";
    EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/bar"));

    EXPECT_TRUE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/fish.php"));
    EXPECT_TRUE(
        IsUserAgentAllowed(robotstxt, "FooBot",
                           "http://foo.bar/fishheads/catfish.php?parameters"));

    EXPECT_FALSE(
        IsUserAgentAllowed(robotstxt, "FooBot", "http://foo.bar/Fish.PHP"));
  }
  // Section "Order of precedence for group-member records".
  {
    std::string robotstxt =
        "user-agent: FooBot\n"
        "allow: /p\n"
        "disallow: /\n";
    std::string url = "http://example.com/page";
    EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "FooBot", url));
  }
  {
    std::string robotstxt =
        "user-agent: FooBot\n"
        "allow: /folder\n"
        "disallow: /folder\n";
    std::string url = "http://example.com/folder/page";
    EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "FooBot", url));
  }
  {
    std::string robotstxt =
        "user-agent: FooBot\n"
        "allow: /page\n"
        "disallow: /*.htm\n";
    std::string url = "http://example.com/page.htm";
    EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "FooBot", url));
  }
  {
    std::string robotstxt =
        "user-agent: FooBot\n"
        "allow: /$\n"
        "disallow: /\n";
    std::string url = "http://example.com/";
    std::string url_page = "http://example.com/page.html";
    EXPECT_TRUE(IsUserAgentAllowed(robotstxt, "FooBot", url));
    EXPECT_FALSE(IsUserAgentAllowed(robotstxt, "FooBot", url_page));
  }
}

class RobotsStatsReporter : public googlebot::RobotsParseHandler {
 public:
  void HandleRobotsStart() override {
    last_line_seen_ = 0;
    valid_directives_ = 0;
    unknown_directives_ = 0;
    sitemap_.clear();
  }
  void HandleRobotsEnd() override {}

  void HandleUserAgent(int line_num, std::string_view value) override {
    Digest(line_num);
  }
  void HandleAllow(int line_num, std::string_view value) override {
    Digest(line_num);
  }
  void HandleDisallow(int line_num, std::string_view value) override {
    Digest(line_num);
  }

  void HandleSitemap(int line_num, std::string_view value) override {
    Digest(line_num);
    sitemap_.append(value.data(), value.length());
  }

  void HandleCrawlDelay(int line_num, double value) override {
    Digest(line_num);
    crawl_delay_ = value;
  }

  void HandleRequestRate(int line_num,
                         const googlebot::RequestRate& rate) override {
    Digest(line_num);
    request_rate_ = rate;
  }

#if ROBOTS_SUPPORT_CONTENT_SIGNAL
  void HandleContentSignal(int line_num,
                           const googlebot::ContentSignal& signal) override {
    Digest(line_num);
    content_signal_ = signal;
  }
#endif  // ROBOTS_SUPPORT_CONTENT_SIGNAL

  // Any other unrecognized name/v pairs.
  void HandleUnknownAction(int line_num, std::string_view action,
                           std::string_view value) override {
    last_line_seen_ = line_num;
    unknown_directives_++;
  }

  int last_line_seen() const { return last_line_seen_; }

  // All directives found, including unknown.
  int valid_directives() const { return valid_directives_; }

  // Number of unknown directives.
  int unknown_directives() const { return unknown_directives_; }

  // Parsed sitemap line.
  std::string sitemap() const { return sitemap_; }

  // Parsed crawl-delay value.
  std::optional<double> crawl_delay() const { return crawl_delay_; }

  // Parsed request-rate value.
  std::optional<googlebot::RequestRate> request_rate() const {
    return request_rate_;
  }

#if ROBOTS_SUPPORT_CONTENT_SIGNAL
  // Parsed content-signal value.
  std::optional<googlebot::ContentSignal> content_signal() const {
    return content_signal_;
  }
#endif  // ROBOTS_SUPPORT_CONTENT_SIGNAL

 private:
  void Digest(int line_num) {
    ASSERT_GE(line_num, last_line_seen_);
    last_line_seen_ = line_num;
    valid_directives_++;
  }

  int last_line_seen_ = 0;
  int valid_directives_ = 0;
  int unknown_directives_ = 0;
  std::string sitemap_;
  std::optional<double> crawl_delay_;
  std::optional<googlebot::RequestRate> request_rate_;
#if ROBOTS_SUPPORT_CONTENT_SIGNAL
  std::optional<googlebot::ContentSignal> content_signal_;
#endif  // ROBOTS_SUPPORT_CONTENT_SIGNAL
};

// Different kinds of line endings are all supported: %x0D / %x0A / %x0D.0A
TEST(RobotsUnittest, ID_LinesNumbersAreCountedCorrectly) {
  RobotsStatsReporter report;
  static const char kUnixFile[] =
      "User-Agent: foo\n"
      "Allow: /some/path\n"
      "User-Agent: bar\n"
      "\n"
      "\n"
      "Disallow: /\n";
  googlebot::ParseRobotsTxt(kUnixFile, &report);
  EXPECT_EQ(4, report.valid_directives());
  EXPECT_EQ(6, report.last_line_seen());

  static const char kDosFile[] =
      "User-Agent: foo\r\n"
      "Allow: /some/path\r\n"
      "User-Agent: bar\r\n"
      "\r\n"
      "\r\n"
      "Disallow: /\r\n";
  googlebot::ParseRobotsTxt(kDosFile, &report);
  EXPECT_EQ(4, report.valid_directives());
  EXPECT_EQ(6, report.last_line_seen());

  static const char kMacFile[] =
      "User-Agent: foo\r"
      "Allow: /some/path\r"
      "User-Agent: bar\r"
      "\r"
      "\r"
      "Disallow: /\r";
  googlebot::ParseRobotsTxt(kMacFile, &report);
  EXPECT_EQ(4, report.valid_directives());
  EXPECT_EQ(6, report.last_line_seen());

  static const char kNoFinalNewline[] =
      "User-Agent: foo\n"
      "Allow: /some/path\n"
      "User-Agent: bar\n"
      "\n"
      "\n"
      "Disallow: /";
  googlebot::ParseRobotsTxt(kNoFinalNewline, &report);
  EXPECT_EQ(4, report.valid_directives());
  EXPECT_EQ(6, report.last_line_seen());

  static const char kMixedFile[] =
      "User-Agent: foo\n"
      "Allow: /some/path\r\n"
      "User-Agent: bar\n"
      "\r\n"
      "\n"
      "Disallow: /";
  googlebot::ParseRobotsTxt(kMixedFile, &report);
  EXPECT_EQ(4, report.valid_directives());
  EXPECT_EQ(6, report.last_line_seen());
}

// BOM characters are unparseable and thus skipped. The rules following the line
// are used.
TEST(RobotsUnittest, ID_UTF8ByteOrderMarkIsSkipped) {
  RobotsStatsReporter report;
  static const char kUtf8FileFullBOM[] =
      "\xEF\xBB\xBF"
      "User-Agent: foo\n"
      "Allow: /AnyValue\n";
  googlebot::ParseRobotsTxt(kUtf8FileFullBOM, &report);
  EXPECT_EQ(2, report.valid_directives());
  EXPECT_EQ(0, report.unknown_directives());

  // We allow as well partial ByteOrderMarks.
  static const char kUtf8FilePartial2BOM[] =
      "\xEF\xBB"
      "User-Agent: foo\n"
      "Allow: /AnyValue\n";
  googlebot::ParseRobotsTxt(kUtf8FilePartial2BOM, &report);
  EXPECT_EQ(2, report.valid_directives());
  EXPECT_EQ(0, report.unknown_directives());

  static const char kUtf8FilePartial1BOM[] =
      "\xEF"
      "User-Agent: foo\n"
      "Allow: /AnyValue\n";
  googlebot::ParseRobotsTxt(kUtf8FilePartial1BOM, &report);
  EXPECT_EQ(2, report.valid_directives());
  EXPECT_EQ(0, report.unknown_directives());

  // If the BOM is not the right sequence, the first line looks like garbage
  // that is skipped (we essentially see "\x11\xBFUser-Agent").
  static const char kUtf8FileBrokenBOM[] =
      "\xEF\x11\xBF"
      "User-Agent: foo\n"
      "Allow: /AnyValue\n";
  googlebot::ParseRobotsTxt(kUtf8FileBrokenBOM, &report);
  EXPECT_EQ(1, report.valid_directives());
  EXPECT_EQ(1, report.unknown_directives());  // We get one broken line.

  // Some other messed up file: BOMs only valid in the beginning of the file.
  static const char kUtf8BOMSomewhereInMiddleOfFile[] =
      "User-Agent: foo\n"
      "\xEF\xBB\xBF"
      "Allow: /AnyValue\n";
  googlebot::ParseRobotsTxt(kUtf8BOMSomewhereInMiddleOfFile, &report);
  EXPECT_EQ(1, report.valid_directives());
  EXPECT_EQ(1, report.unknown_directives());
}

// Google specific: the RFC allows any line that crawlers might need, such as
// sitemaps, which Google supports.
// See REP RFC section "Other records".
// https://www.rfc-editor.org/rfc/rfc9309.html#section-2.2.4
TEST(RobotsUnittest, ID_NonStandardLineExample_Sitemap) {
  RobotsStatsReporter report;
  {
    std::string sitemap_loc = "http://foo.bar/sitemap.xml";
    std::string robotstxt =
        "User-Agent: foo\n"
        "Allow: /some/path\n"
        "User-Agent: bar\n"
        "\n"
        "\n";
    StrAppend(&robotstxt, "Sitemap: ", sitemap_loc, "\n");

    googlebot::ParseRobotsTxt(robotstxt, &report);
    EXPECT_EQ(sitemap_loc, report.sitemap());
  }
  // A sitemap line may appear anywhere in the file.
  {
    std::string robotstxt;
    std::string sitemap_loc = "http://foo.bar/sitemap.xml";
    std::string robotstxt_temp =
        "User-Agent: foo\n"
        "Allow: /some/path\n"
        "User-Agent: bar\n"
        "\n"
        "\n";
    StrAppend(&robotstxt, "Sitemap: ", sitemap_loc, "\n", robotstxt_temp);

    googlebot::ParseRobotsTxt(robotstxt, &report);
    EXPECT_EQ(sitemap_loc, report.sitemap());
  }
}

}  // namespace

// Integrity tests. These functions are available to the linker, but not in the
// header, because they should only be used for testing.
namespace googlebot {
std::string GetPathParamsQuery(const std::string& url);
bool MaybeEscapePattern(const char* src, char** dst);
}  // namespace googlebot

void TestPath(const std::string& url, const std::string& expected_path) {
  EXPECT_EQ(expected_path, googlebot::GetPathParamsQuery(url));
}

void TestEscape(const std::string& url, const std::string& expected) {
  char* escaped_value = nullptr;
  const bool is_escaped =
      googlebot::MaybeEscapePattern(url.c_str(), &escaped_value);
  const std::string escaped = escaped_value;
  if (is_escaped) delete[] escaped_value;

  EXPECT_EQ(expected, escaped);
}

TEST(RobotsUnittest, TestGetPathParamsQuery) {
  // Only testing URLs that are already correctly escaped here.
  TestPath("", "/");
  TestPath("http://www.example.com", "/");
  TestPath("http://www.example.com/", "/");
  TestPath("http://www.example.com/a", "/a");
  TestPath("http://www.example.com/a/", "/a/");
  TestPath("http://www.example.com/a/b?c=http://d.e/", "/a/b?c=http://d.e/");
  TestPath("http://www.example.com/a/b?c=d&e=f#fragment", "/a/b?c=d&e=f");
  TestPath("example.com", "/");
  TestPath("example.com/", "/");
  TestPath("example.com/a", "/a");
  TestPath("example.com/a/", "/a/");
  TestPath("example.com/a/b?c=d&e=f#fragment", "/a/b?c=d&e=f");
  TestPath("a", "/");
  TestPath("a/", "/");
  TestPath("/a", "/a");
  TestPath("a/b", "/b");
  TestPath("example.com?a", "/?a");
  TestPath("example.com/a;b#c", "/a;b");
  TestPath("//a/b/c", "/b/c");
}

TEST(RobotsUnittest, TestMaybeEscapePattern) {
  TestEscape("http://www.example.com", "http://www.example.com");
  TestEscape("/a/b/c", "/a/b/c");
  TestEscape("á", "%C3%A1");
  TestEscape("%aa", "%AA");
}
