// Package robotstxt provides Go bindings for Google's robots.txt parser library.
//
// Example usage:
//
//	matcher := robotstxt.NewMatcher()
//	defer matcher.Free()
//
//	allowed := matcher.IsAllowed(robotsTxt, "Googlebot", "https://example.com/page")
//	fmt.Printf("Access: %v\n", allowed)
package robotstxt

/*
#cgo CFLAGS: -I${SRCDIR}/../c
#cgo LDFLAGS: -L${SRCDIR}/../../build -L${SRCDIR}/../../cmake-build -lrobots -lstdc++
#cgo darwin LDFLAGS: -Wl,-rpath,${SRCDIR}/../../build -Wl,-rpath,${SRCDIR}/../../cmake-build

#include "robots_c.h"
#include <stdlib.h>
*/
import "C"
import (
	"runtime"
	"unsafe"
)

// Version returns the library version string.
func Version() string {
	return C.GoString(C.robots_version())
}

// IsValidUserAgent checks if a user-agent string contains only valid characters [a-zA-Z_-].
func IsValidUserAgent(userAgent string) bool {
	ua := C.CString(userAgent)
	defer C.free(unsafe.Pointer(ua))
	return bool(C.robots_is_valid_user_agent(ua, C.size_t(len(userAgent))))
}

// RequestRate represents a request-rate value (requests per time period).
type RequestRate struct {
	Requests int
	Seconds  int
}

// ContentSignal represents AI content preferences.
// Values are: nil = not set, true = yes, false = no.
type ContentSignal struct {
	AITrain *bool
	AIInput *bool
	Search  *bool
}

// Matcher is a robots.txt matcher that checks if URLs are allowed for given user-agents.
type Matcher struct {
	ptr *C.struct_robots_matcher_s
}

// NewMatcher creates a new RobotsMatcher instance.
// The caller must call Free() when done.
func NewMatcher() *Matcher {
	m := &Matcher{
		ptr: C.robots_matcher_create(),
	}
	runtime.SetFinalizer(m, (*Matcher).Free)
	return m
}

// Free releases the matcher resources.
func (m *Matcher) Free() {
	if m.ptr != nil {
		C.robots_matcher_free(m.ptr)
		m.ptr = nil
	}
}

// IsAllowed checks if a URL is allowed for a single user-agent.
func (m *Matcher) IsAllowed(robotsTxt, userAgent, url string) bool {
	cRobots := C.CString(robotsTxt)
	defer C.free(unsafe.Pointer(cRobots))
	cUA := C.CString(userAgent)
	defer C.free(unsafe.Pointer(cUA))
	cURL := C.CString(url)
	defer C.free(unsafe.Pointer(cURL))

	return bool(C.robots_allowed_by_robots(
		m.ptr,
		cRobots, C.size_t(len(robotsTxt)),
		cUA, C.size_t(len(userAgent)),
		cURL, C.size_t(len(url)),
	))
}

// IsAllowedMulti checks if a URL is allowed for multiple user-agents.
func (m *Matcher) IsAllowedMulti(robotsTxt string, userAgents []string, url string) bool {
	cRobots := C.CString(robotsTxt)
	defer C.free(unsafe.Pointer(cRobots))
	cURL := C.CString(url)
	defer C.free(unsafe.Pointer(cURL))

	// Prepare user-agent arrays
	cUAs := make([]*C.char, len(userAgents))
	cLens := make([]C.size_t, len(userAgents))
	for i, ua := range userAgents {
		cUAs[i] = C.CString(ua)
		defer C.free(unsafe.Pointer(cUAs[i]))
		cLens[i] = C.size_t(len(ua))
	}

	return bool(C.robots_allowed_by_robots_multi(
		m.ptr,
		cRobots, C.size_t(len(robotsTxt)),
		&cUAs[0], &cLens[0], C.size_t(len(userAgents)),
		cURL, C.size_t(len(url)),
	))
}

// MatchingLine returns the line number that matched, or 0 if no match.
func (m *Matcher) MatchingLine() int {
	return int(C.robots_matching_line(m.ptr))
}

// EverSeenSpecificAgent returns true if a specific user-agent block was found.
func (m *Matcher) EverSeenSpecificAgent() bool {
	return bool(C.robots_ever_seen_specific_agent(m.ptr))
}

// CrawlDelay returns the crawl-delay in seconds, or nil if not specified.
func (m *Matcher) CrawlDelay() *float64 {
	if !C.robots_has_crawl_delay(m.ptr) {
		return nil
	}
	delay := float64(C.robots_get_crawl_delay(m.ptr))
	return &delay
}

// RequestRate returns the request-rate, or nil if not specified.
func (m *Matcher) RequestRate() *RequestRate {
	var rate C.robots_request_rate_t
	if !C.robots_get_request_rate(m.ptr, &rate) {
		return nil
	}
	return &RequestRate{
		Requests: int(rate.requests),
		Seconds:  int(rate.seconds),
	}
}

// ContentSignalSupported returns true if Content-Signal support is compiled in.
func ContentSignalSupported() bool {
	return bool(C.robots_content_signal_supported())
}

// ContentSignal returns the content-signal values, or nil if not specified.
func (m *Matcher) ContentSignal() *ContentSignal {
	if !C.robots_content_signal_supported() {
		return nil
	}
	var signal C.robots_content_signal_t
	if !C.robots_get_content_signal(m.ptr, &signal) {
		return nil
	}

	triState := func(v C.int8_t) *bool {
		if v == -1 {
			return nil
		}
		b := v == 1
		return &b
	}

	return &ContentSignal{
		AITrain: triState(signal.ai_train),
		AIInput: triState(signal.ai_input),
		Search:  triState(signal.search),
	}
}

// AllowsAITrain returns true if AI training is allowed (defaults to true if not specified).
func (m *Matcher) AllowsAITrain() bool {
	return bool(C.robots_allows_ai_train(m.ptr))
}

// AllowsAIInput returns true if AI input is allowed (defaults to true if not specified).
func (m *Matcher) AllowsAIInput() bool {
	return bool(C.robots_allows_ai_input(m.ptr))
}

// AllowsSearch returns true if search indexing is allowed (defaults to true if not specified).
func (m *Matcher) AllowsSearch() bool {
	return bool(C.robots_allows_search(m.ptr))
}
