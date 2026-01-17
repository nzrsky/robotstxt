/*
 * Copyright 2024 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.robotstxt;

import org.junit.Test;
import static org.junit.Assert.*;

public class RobotsMatcherTest {

    @Test
    public void testVersion() {
        String version = RobotsMatcher.getVersion();
        assertNotNull(version);
        assertFalse(version.isEmpty());
    }

    @Test
    public void testValidUserAgent() {
        assertTrue(RobotsMatcher.isValidUserAgent("Googlebot"));
        assertTrue(RobotsMatcher.isValidUserAgent("My-Bot"));
        assertFalse(RobotsMatcher.isValidUserAgent("Bot/1.0"));
    }

    @Test
    public void testBasicAllow() {
        try (RobotsMatcher matcher = new RobotsMatcher()) {
            String robotsTxt = "User-agent: *\nAllow: /\n";
            assertTrue(matcher.isAllowed(robotsTxt, "Googlebot", "https://example.com/page"));
        }
    }

    @Test
    public void testBasicDisallow() {
        try (RobotsMatcher matcher = new RobotsMatcher()) {
            String robotsTxt = "User-agent: *\nDisallow: /admin/\n";
            assertFalse(matcher.isAllowed(robotsTxt, "Googlebot", "https://example.com/admin/secret"));
            assertTrue(matcher.isAllowed(robotsTxt, "Googlebot", "https://example.com/public"));
        }
    }

    @Test
    public void testSpecificUserAgent() {
        try (RobotsMatcher matcher = new RobotsMatcher()) {
            String robotsTxt = "User-agent: Googlebot\n" +
                              "Disallow: /google-only/\n\n" +
                              "User-agent: *\n" +
                              "Disallow: /admin/\n";
            assertFalse(matcher.isAllowed(robotsTxt, "Googlebot", "https://example.com/google-only/"));
            // Check everSeenSpecificAgent right after Googlebot match (state may change on next call)
            assertTrue(matcher.everSeenSpecificAgent());
            assertTrue(matcher.isAllowed(robotsTxt, "Bingbot", "https://example.com/google-only/"));
        }
    }

    @Test
    public void testCrawlDelay() {
        try (RobotsMatcher matcher = new RobotsMatcher()) {
            String robotsTxt = "User-agent: *\nCrawl-delay: 2.5\nDisallow:\n";
            matcher.isAllowed(robotsTxt, "Googlebot", "https://example.com/");
            Double delay = matcher.getCrawlDelay();
            assertNotNull(delay);
            assertEquals(2.5, delay, 0.001);
        }
    }

    @Test
    public void testNoCrawlDelay() {
        try (RobotsMatcher matcher = new RobotsMatcher()) {
            String robotsTxt = "User-agent: *\nDisallow:\n";
            matcher.isAllowed(robotsTxt, "Googlebot", "https://example.com/");
            assertNull(matcher.getCrawlDelay());
        }
    }

    @Test
    public void testRequestRate() {
        try (RobotsMatcher matcher = new RobotsMatcher()) {
            String robotsTxt = "User-agent: *\nRequest-rate: 1/10\nDisallow:\n";
            matcher.isAllowed(robotsTxt, "Googlebot", "https://example.com/");
            RequestRate rate = matcher.getRequestRate();
            assertNotNull(rate);
            assertEquals(1, rate.getRequests());
            assertEquals(10, rate.getSeconds());
            assertEquals(0.1, rate.getRequestsPerSecond(), 0.001);
            assertEquals(10.0, rate.getDelaySeconds(), 0.001);
        }
    }

    @Test
    public void testNoRequestRate() {
        try (RobotsMatcher matcher = new RobotsMatcher()) {
            String robotsTxt = "User-agent: *\nDisallow:\n";
            matcher.isAllowed(robotsTxt, "Googlebot", "https://example.com/");
            assertNull(matcher.getRequestRate());
        }
    }

    @Test
    public void testContentSignal() {
        if (!RobotsMatcher.isContentSignalSupported()) {
            System.out.println("Content-Signal not supported, skipping test");
            return;
        }

        try (RobotsMatcher matcher = new RobotsMatcher()) {
            String robotsTxt = "User-agent: *\nContent-Signal: ai-train=no, search=yes\nDisallow:\n";
            matcher.isAllowed(robotsTxt, "Googlebot", "https://example.com/");
            assertFalse(matcher.allowsAITrain());
            assertTrue(matcher.allowsSearch());
        }
    }

    @Test
    public void testMatchingLine() {
        try (RobotsMatcher matcher = new RobotsMatcher()) {
            String robotsTxt = "User-agent: *\nDisallow: /admin/\n";
            matcher.isAllowed(robotsTxt, "Googlebot", "https://example.com/admin/secret");
            assertEquals(2, matcher.getMatchingLine());
        }
    }

    @Test
    public void testEmptyRobotsTxt() {
        try (RobotsMatcher matcher = new RobotsMatcher()) {
            assertTrue(matcher.isAllowed("", "Googlebot", "https://example.com/anything"));
        }
    }

    @Test
    public void testAutoCloseable() {
        RobotsMatcher matcher = new RobotsMatcher();
        assertTrue(matcher.isAllowed("User-agent: *\nAllow: /\n", "Bot", "https://example.com/"));
        matcher.close();
        // After close, should return default values
        assertEquals(0, matcher.getMatchingLine());
    }
}
