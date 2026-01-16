# frozen_string_literal: true

require 'minitest/autorun'
require_relative '../lib/robotstxt'

class TestRobotsTxt < Minitest::Test
  def test_version
    version = RobotsTxt.version
    refute_empty version
  end

  def test_valid_user_agent
    assert RobotsTxt.valid_user_agent?('Googlebot')
    assert RobotsTxt.valid_user_agent?('My-Bot')
    refute RobotsTxt.valid_user_agent?('Bot/1.0')
  end

  def test_basic_allow
    matcher = RobotsTxt::Matcher.new
    robots_txt = "User-agent: *\nAllow: /\n"
    assert matcher.allowed?(robots_txt, 'Googlebot', 'https://example.com/page')
  end

  def test_basic_disallow
    matcher = RobotsTxt::Matcher.new
    robots_txt = "User-agent: *\nDisallow: /admin/\n"
    refute matcher.allowed?(robots_txt, 'Googlebot', 'https://example.com/admin/secret')
    assert matcher.allowed?(robots_txt, 'Googlebot', 'https://example.com/public')
  end

  def test_specific_user_agent
    matcher = RobotsTxt::Matcher.new
    robots_txt = <<~ROBOTS
      User-agent: Googlebot
      Disallow: /google-only/

      User-agent: *
      Disallow: /admin/
    ROBOTS

    refute matcher.allowed?(robots_txt, 'Googlebot', 'https://example.com/google-only/')
    assert matcher.allowed?(robots_txt, 'Bingbot', 'https://example.com/google-only/')
    assert matcher.ever_seen_specific_agent?
  end

  def test_crawl_delay
    matcher = RobotsTxt::Matcher.new
    robots_txt = "User-agent: *\nCrawl-delay: 2.5\nDisallow:\n"
    matcher.allowed?(robots_txt, 'Googlebot', 'https://example.com/')
    assert_in_delta 2.5, matcher.crawl_delay, 0.001
  end

  def test_no_crawl_delay
    matcher = RobotsTxt::Matcher.new
    robots_txt = "User-agent: *\nDisallow:\n"
    matcher.allowed?(robots_txt, 'Googlebot', 'https://example.com/')
    assert_nil matcher.crawl_delay
  end

  def test_request_rate
    matcher = RobotsTxt::Matcher.new
    robots_txt = "User-agent: *\nRequest-rate: 1/10\nDisallow:\n"
    matcher.allowed?(robots_txt, 'Googlebot', 'https://example.com/')

    rate = matcher.request_rate
    refute_nil rate
    assert_equal 1, rate.requests
    assert_equal 10, rate.seconds
    assert_in_delta 0.1, rate.requests_per_second, 0.001
    assert_in_delta 10.0, rate.delay_seconds, 0.001
  end

  def test_no_request_rate
    matcher = RobotsTxt::Matcher.new
    robots_txt = "User-agent: *\nDisallow:\n"
    matcher.allowed?(robots_txt, 'Googlebot', 'https://example.com/')
    assert_nil matcher.request_rate
  end

  def test_content_signal
    skip 'Content-Signal not supported' unless RobotsTxt::Matcher.content_signal_supported?

    matcher = RobotsTxt::Matcher.new
    robots_txt = "User-agent: *\nContent-Signal: ai-train=no, search=yes\nDisallow:\n"
    matcher.allowed?(robots_txt, 'Googlebot', 'https://example.com/')

    refute matcher.allows_ai_train?
    assert matcher.allows_search?
  end

  def test_matching_line
    matcher = RobotsTxt::Matcher.new
    robots_txt = "User-agent: *\nDisallow: /admin/\n"
    matcher.allowed?(robots_txt, 'Googlebot', 'https://example.com/admin/secret')
    assert_equal 2, matcher.matching_line
  end

  def test_empty_robots_txt
    matcher = RobotsTxt::Matcher.new
    assert matcher.allowed?('', 'Googlebot', 'https://example.com/anything')
  end
end
