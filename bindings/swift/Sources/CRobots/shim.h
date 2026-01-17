// C API shim header for Swift bindings
// This file re-exports the robots C API for Swift consumption

#ifndef CROBOTS_SHIM_H
#define CROBOTS_SHIM_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque matcher type
typedef struct robots_matcher_s robots_matcher_t;

// Request-rate structure
typedef struct {
    int requests;
    int seconds;
} robots_request_rate_t;

// Content-Signal structure (tri-state: -1=unset, 0=no, 1=yes)
typedef struct {
    int8_t ai_train;
    int8_t ai_input;
    int8_t search;
} robots_content_signal_t;

// Matcher lifecycle
robots_matcher_t* robots_matcher_create(void);
void robots_matcher_free(robots_matcher_t* matcher);

// Core matching function
bool robots_allowed_by_robots(
    robots_matcher_t* matcher,
    const char* robots_txt, size_t robots_txt_len,
    const char* user_agent, size_t user_agent_len,
    const char* url, size_t url_len
);

// Match information
int robots_matching_line(const robots_matcher_t* matcher);
bool robots_ever_seen_specific_agent(const robots_matcher_t* matcher);

// Crawl-delay support
bool robots_has_crawl_delay(const robots_matcher_t* matcher);
double robots_get_crawl_delay(const robots_matcher_t* matcher);

// Request-rate support
bool robots_has_request_rate(const robots_matcher_t* matcher);
bool robots_get_request_rate(const robots_matcher_t* matcher, robots_request_rate_t* rate);

// Content-Signal support
bool robots_content_signal_supported(void);
bool robots_has_content_signal(const robots_matcher_t* matcher);
bool robots_get_content_signal(const robots_matcher_t* matcher, robots_content_signal_t* signal);
bool robots_allows_ai_train(const robots_matcher_t* matcher);
bool robots_allows_ai_input(const robots_matcher_t* matcher);
bool robots_allows_search(const robots_matcher_t* matcher);

// Utility functions
bool robots_is_valid_user_agent(const char* user_agent, size_t len);
const char* robots_version(void);

#ifdef __cplusplus
}
#endif

#endif // CROBOTS_SHIM_H
