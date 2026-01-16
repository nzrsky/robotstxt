package robotstxt

import (
	"testing"
)

func TestVersion(t *testing.T) {
	v := Version()
	if v == "" {
		t.Error("Version should not be empty")
	}
}

func TestIsValidUserAgent(t *testing.T) {
	tests := []struct {
		ua    string
		valid bool
	}{
		{"Googlebot", true},
		{"My-Bot", true},
		{"my_bot", true},
		{"Bot/1.0", false},
		{"", false},
	}

	for _, tt := range tests {
		if got := IsValidUserAgent(tt.ua); got != tt.valid {
			t.Errorf("IsValidUserAgent(%q) = %v, want %v", tt.ua, got, tt.valid)
		}
	}
}

func TestBasicAllow(t *testing.T) {
	m := NewMatcher()
	defer m.Free()

	robotsTxt := "User-agent: *\nAllow: /\n"
	if !m.IsAllowed(robotsTxt, "Googlebot", "https://example.com/page") {
		t.Error("Expected URL to be allowed")
	}
}

func TestBasicDisallow(t *testing.T) {
	m := NewMatcher()
	defer m.Free()

	robotsTxt := "User-agent: *\nDisallow: /admin/\n"

	if m.IsAllowed(robotsTxt, "Googlebot", "https://example.com/admin/secret") {
		t.Error("Expected /admin/secret to be disallowed")
	}

	if !m.IsAllowed(robotsTxt, "Googlebot", "https://example.com/public") {
		t.Error("Expected /public to be allowed")
	}
}

func TestSpecificAgent(t *testing.T) {
	m := NewMatcher()
	defer m.Free()

	robotsTxt := `
User-agent: *
Disallow: /

User-agent: Googlebot
Allow: /
`
	if !m.IsAllowed(robotsTxt, "Googlebot", "https://example.com/page") {
		t.Error("Expected Googlebot to be allowed")
	}

	if m.IsAllowed(robotsTxt, "Bingbot", "https://example.com/page") {
		t.Error("Expected Bingbot to be disallowed")
	}
}

func TestCrawlDelay(t *testing.T) {
	m := NewMatcher()
	defer m.Free()

	robotsTxt := "User-agent: *\nCrawl-delay: 2.5\nDisallow:\n"
	m.IsAllowed(robotsTxt, "Googlebot", "https://example.com/")

	delay := m.CrawlDelay()
	if delay == nil {
		t.Fatal("Expected crawl-delay to be set")
	}
	if *delay != 2.5 {
		t.Errorf("Expected crawl-delay 2.5, got %v", *delay)
	}
}

func TestRequestRate(t *testing.T) {
	m := NewMatcher()
	defer m.Free()

	robotsTxt := "User-agent: *\nRequest-rate: 1/10\nDisallow:\n"
	m.IsAllowed(robotsTxt, "Googlebot", "https://example.com/")

	rate := m.RequestRate()
	if rate == nil {
		t.Fatal("Expected request-rate to be set")
	}
	if rate.Requests != 1 || rate.Seconds != 10 {
		t.Errorf("Expected 1/10, got %d/%d", rate.Requests, rate.Seconds)
	}
}

func TestContentSignal(t *testing.T) {
	if !ContentSignalSupported() {
		t.Skip("Content-Signal not supported")
	}

	m := NewMatcher()
	defer m.Free()

	robotsTxt := "User-agent: *\nContent-Signal: ai-train=no, search=yes\nDisallow:\n"
	m.IsAllowed(robotsTxt, "Googlebot", "https://example.com/")

	signal := m.ContentSignal()
	if signal == nil {
		t.Fatal("Expected content-signal to be set")
	}

	if signal.AITrain == nil || *signal.AITrain != false {
		t.Error("Expected ai-train=no")
	}
	if signal.Search == nil || *signal.Search != true {
		t.Error("Expected search=yes")
	}
	if signal.AIInput != nil {
		t.Error("Expected ai-input to be unset")
	}
}
