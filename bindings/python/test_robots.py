#!/usr/bin/env python3
"""Tests for robotstxt Python bindings."""

import unittest
from robotstxt import RobotsMatcher, get_version, is_valid_user_agent


class TestRobotsMatcher(unittest.TestCase):
    def setUp(self):
        self.matcher = RobotsMatcher()

    def tearDown(self):
        del self.matcher

    def test_version(self):
        version = get_version()
        self.assertIsInstance(version, str)
        self.assertTrue(len(version) > 0)

    def test_valid_user_agent(self):
        self.assertTrue(is_valid_user_agent("Googlebot"))
        self.assertTrue(is_valid_user_agent("My-Bot"))
        self.assertTrue(is_valid_user_agent("my_bot"))
        self.assertFalse(is_valid_user_agent("Bot/1.0"))
        self.assertFalse(is_valid_user_agent(""))

    def test_basic_allow(self):
        robots_txt = "User-agent: *\nAllow: /\n"
        self.assertTrue(
            self.matcher.is_allowed(robots_txt, "Googlebot", "https://example.com/page")
        )

    def test_basic_disallow(self):
        robots_txt = "User-agent: *\nDisallow: /admin/\n"
        self.assertFalse(
            self.matcher.is_allowed(robots_txt, "Googlebot", "https://example.com/admin/secret")
        )
        self.assertTrue(
            self.matcher.is_allowed(robots_txt, "Googlebot", "https://example.com/public")
        )

    def test_specific_agent(self):
        robots_txt = """
User-agent: *
Disallow: /

User-agent: Googlebot
Allow: /
"""
        self.assertTrue(
            self.matcher.is_allowed(robots_txt, "Googlebot", "https://example.com/page")
        )
        self.assertFalse(
            self.matcher.is_allowed(robots_txt, "Bingbot", "https://example.com/page")
        )

    def test_crawl_delay(self):
        robots_txt = "User-agent: *\nCrawl-delay: 2.5\nDisallow:\n"
        self.matcher.is_allowed(robots_txt, "Googlebot", "https://example.com/")
        self.assertEqual(self.matcher.crawl_delay, 2.5)

    def test_no_crawl_delay(self):
        robots_txt = "User-agent: *\nDisallow:\n"
        self.matcher.is_allowed(robots_txt, "Googlebot", "https://example.com/")
        self.assertIsNone(self.matcher.crawl_delay)

    def test_request_rate(self):
        robots_txt = "User-agent: *\nRequest-rate: 1/10\nDisallow:\n"
        self.matcher.is_allowed(robots_txt, "Googlebot", "https://example.com/")
        rate = self.matcher.request_rate
        self.assertIsNotNone(rate)
        self.assertEqual(rate, (1, 10))

    def test_content_signal(self):
        robots_txt = "User-agent: *\nContent-Signal: ai-train=no, search=yes\nDisallow:\n"
        self.matcher.is_allowed(robots_txt, "Googlebot", "https://example.com/")
        signal = self.matcher.content_signal
        if signal is not None:  # Only if compiled with ROBOTS_SUPPORT_CONTENT_SIGNAL
            self.assertFalse(signal["ai_train"])
            self.assertTrue(signal["search"])
            self.assertIsNone(signal["ai_input"])

    def test_multi_user_agents(self):
        robots_txt = """
User-agent: *
Disallow: /

User-agent: Googlebot
User-agent: Bingbot
Allow: /allowed/
"""
        self.assertTrue(
            self.matcher.is_allowed_multi(
                robots_txt, ["Googlebot", "Bingbot"], "https://example.com/allowed/page"
            )
        )

    def test_context_manager(self):
        robots_txt = "User-agent: *\nAllow: /\n"
        with RobotsMatcher() as m:
            result = m.is_allowed(robots_txt, "Googlebot", "https://example.com/")
            self.assertTrue(result)


if __name__ == "__main__":
    unittest.main()
