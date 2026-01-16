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
// File: robots.cc
// -----------------------------------------------------------------------------
//
// Implements expired internet draft
//   http://www.robotstxt.org/norobots-rfc.txt
// with Google-specific optimizations detailed at
//   https://developers.google.com/search/reference/robots_txt

#include "robots.h"

#include <stdlib.h>

#ifdef ROBOTS_USE_ADA
#include <ada.h>
#endif
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstring>
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

// Returns true if URI path matches the specified pattern. Pattern is anchored
// at the beginning of path. '$' is special only at the end of pattern.
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

  for (auto pat = pattern.begin(); pat != pattern.end(); ++pat) {
    if (*pat == '$' && pat + 1 == pattern.end()) {
      return (pos[numpos - 1] == pathlen);
    }
    if (*pat == '*') {
      numpos = pathlen - pos[0] + 1;
      for (int i = 1; i < numpos; i++) {
        pos[i] = pos[i-1] + 1;
      }
    } else {
      // Includes '$' when not at end of pattern.
      int newnumpos = 0;
      for (int i = 0; i < numpos; i++) {
        if (pos[i] < pathlen && path[pos[i]] == *pat) {
          pos[newnumpos++] = pos[i] + 1;
        }
      }
      numpos = newnumpos;
      if (numpos == 0) return false;
    }
  }

  return true;
}

constexpr std::string_view kHexDigits = "0123456789ABCDEF";

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
      return (hash_pos == std::string::npos) ? url : url.substr(0, hash_pos);
    }
    return "/";
  }

  std::string result(parsed->get_pathname());
  std::string_view search = parsed->get_search();
  if (!search.empty()) result += search;
  return result.empty() ? "/" : result;

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
      return "/" + std::string(s);
    }
    path_start = slash_pos;
  }

  // Extract from path_start to end, but strip fragment
  s = s.substr(path_start);
  size_t hash_pos = s.find('#');
  if (hash_pos != std::string_view::npos) {
    s = s.substr(0, hash_pos);
  }

  return s.empty() ? "/" : std::string(s);
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
  // Note that `key` and `value` are only set when `metadata->has_directive
  // == true`.
  static void GetKeyAndValueFrom(char** key, char** value, char* line,
                                 RobotsParseHandler::LineMetadata* metadata);
  static void StripWhitespaceSlowly(char ** s);

  void ParseAndEmitLine(int current_line, char* line,
                        bool* line_too_long_strict);
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

// Removes leading and trailing whitespace from null-terminated string s.
/* static */ void RobotsTxtParser::StripWhitespaceSlowly(char ** s) {
  std::string_view stripped = StripAsciiWhitespace(*s);
  *s = const_cast<char*>(stripped.data());
  (*s)[stripped.size()] = '\0';
}

void RobotsTxtParser::GetKeyAndValueFrom(
    char** key, char** value, char* line,
    RobotsParseHandler::LineMetadata* metadata) {
  // Remove comments from the current robots.txt line.
  char* const comment = strchr(line, '#');
  if (nullptr != comment) {
    metadata->has_comment = true;
    *comment = '\0';
  }
  StripWhitespaceSlowly(&line);
  // If the line became empty after removing the comment, return.
  if (strlen(line) == 0) {
    if (metadata->has_comment) {
      metadata->is_comment = true;
    } else {
      metadata->is_empty = true;
    }
    return;
  }

  // Rules must match the following pattern:
  //   <key>[ \t]*:[ \t]*<value>
  char* sep = strchr(line, ':');
  if (nullptr == sep) {
    // Google-specific optimization: some people forget the colon, so we need to
    // accept whitespace in its stead.
    constexpr const char* kWhite = " \t";
    sep = strpbrk(line, kWhite);
    if (nullptr != sep) {
      const char* const val = sep + strspn(sep, kWhite);
      assert(*val);  // since we dropped trailing whitespace above.
      if (nullptr != strpbrk(val, kWhite)) {
        // We only accept whitespace as a separator if there are exactly two
        // sequences of non-whitespace characters.  If we get here, there were
        // more than 2 such sequences since we stripped trailing whitespace
        // above.
        return;
      }
      metadata->is_missing_colon_separator = true;
    }
  }
  if (nullptr == sep) {
    return;                     // Couldn't find a separator.
  }

  *key = line;                        // Key starts at beginning of line.
  *sep = '\0';                        // And stops at the separator.
  StripWhitespaceSlowly(key);         // Get rid of any trailing whitespace.

  if (strlen(*key) > 0) {
    *value = 1 + sep;                 // Value starts after the separator.
    StripWhitespaceSlowly(value);     // Get rid of any leading whitespace.
    metadata->has_directive = true;
    return;
  }
}

void RobotsTxtParser::ParseAndEmitLine(int current_line, char* line,
                                       bool* line_too_long_strict) {
  char* string_key;
  char* value;
  RobotsParseHandler::LineMetadata line_metadata;
  // Note that `string_key` and `value` are only set when
  // `line_metadata->has_directive == true`.
  GetKeyAndValueFrom(&string_key, &value, line, &line_metadata);
  line_metadata.is_line_too_long = *line_too_long_strict;
  if (!line_metadata.has_directive) {
    handler_->ReportLineMetadata(current_line, line_metadata);
    return;
  }
  Key key;
  key.Parse(string_key, &line_metadata.is_acceptable_typo);
  if (NeedEscapeValueForKey(key)) {
    char* escaped_value = nullptr;
    const bool is_escaped = MaybeEscapePattern(value, &escaped_value);
    EmitKeyValueToHandler(current_line, key, escaped_value, handler_);
    if (is_escaped) delete[] escaped_value;
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
  constexpr int kBrowserMaxLineLen = 2083;
  constexpr int kMaxLineLen = kBrowserMaxLineLen * 8;
  // Allocate a buffer used to process the current line.
  char* const line_buffer = new char[kMaxLineLen];
  // last_line_pos is the last writeable pos within the line array
  // (only a final '\0' may go here).
  const char* const line_buffer_end = line_buffer + kMaxLineLen - 1;
  bool line_too_long_strict = false;
  char* line_pos = line_buffer;
  int line_num = 0;
  size_t bom_pos = 0;
  bool last_was_carriage_return = false;
  handler_->HandleRobotsStart();

      {
        for (const unsigned char ch : robots_body_) {
      ROBOTS_ASSERT(line_pos <= line_buffer_end);
      // Google-specific optimization: UTF-8 byte order marks should never
      // appear in a robots.txt file, but they do nevertheless. Skipping
      // possible BOM-prefix in the first bytes of the input.
      if (bom_pos < sizeof(utf_bom) && ch == utf_bom[bom_pos++]) {
        continue;
      }
      bom_pos = sizeof(utf_bom);
      if (ch != 0x0A && ch != 0x0D) {  // Non-line-ending char case.
        // Put in next spot on current line, as long as there's room.
        if (line_pos < line_buffer_end) {
          *(line_pos++) = ch;
        } else {
          line_too_long_strict = true;
        }
      } else {                         // Line-ending character char case.
        *line_pos = '\0';
        // Only emit an empty line if this was not due to the second character
        // of the DOS line-ending \r\n .
        const bool is_CRLF_continuation =
            (line_pos == line_buffer) && last_was_carriage_return && ch == 0x0A;
        if (!is_CRLF_continuation) {
          ParseAndEmitLine(++line_num, line_buffer, &line_too_long_strict);
          line_too_long_strict = false;
        }
        line_pos = line_buffer;
        last_was_carriage_return = (ch == 0x0D);
      }
    }
  }
  *line_pos = '\0';
  ParseAndEmitLine(++line_num, line_buffer, &line_too_long_strict);
  handler_->HandleRobotsEnd();
  delete [] line_buffer;
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

}  // namespace googlebot
