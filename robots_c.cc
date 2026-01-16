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
// File: robots_c.cc
// -----------------------------------------------------------------------------
//
// C API implementation for the robots.txt parser and matcher library.

#include "robots_c.h"
#include "robots.h"

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
