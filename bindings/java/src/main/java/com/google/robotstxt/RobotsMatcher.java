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

import java.nio.charset.StandardCharsets;

/**
 * Robots.txt matcher - checks if URLs are allowed for given user-agents.
 *
 * <p>Example usage:
 * <pre>{@code
 * try (RobotsMatcher matcher = new RobotsMatcher()) {
 *     String robotsTxt = "User-agent: *\nDisallow: /admin/\n";
 *     boolean allowed = matcher.isAllowed(robotsTxt, "Googlebot", "https://example.com/page");
 *     System.out.println("Access: " + (allowed ? "allowed" : "disallowed"));
 * }
 * }</pre>
 */
public class RobotsMatcher implements AutoCloseable {
    static {
        try {
            System.loadLibrary("robots_jni");
        } catch (UnsatisfiedLinkError e) {
            // Try loading from environment variable or common paths
            java.util.List<String> paths = new java.util.ArrayList<>();

            // Check ROBOTS_LIB_PATH environment variable first
            String envPath = System.getenv("ROBOTS_LIB_PATH");
            if (envPath != null && !envPath.isEmpty()) {
                paths.add(envPath + "/librobots_jni.dylib");
                paths.add(envPath + "/librobots_jni.so");
                paths.add(envPath + "/robots_jni.dll");
            }

            // Common build paths
            String[] commonPaths = {
                "_build", "build", "cmake-build", "cmake-build-release",
                "../../_build", "../../build", "../../cmake-build"
            };
            for (String base : commonPaths) {
                paths.add(base + "/librobots_jni.dylib");
                paths.add(base + "/librobots_jni.so");
                paths.add(base + "/robots_jni.dll");
            }

            boolean loaded = false;
            for (String path : paths) {
                try {
                    java.io.File f = new java.io.File(path);
                    if (f.exists()) {
                        System.load(f.getAbsolutePath());
                        loaded = true;
                        break;
                    }
                } catch (UnsatisfiedLinkError ignored) {
                }
            }
            if (!loaded) {
                throw e;
            }
        }
    }

    private long nativeHandle;

    /**
     * Creates a new RobotsMatcher instance.
     */
    public RobotsMatcher() {
        nativeHandle = nativeCreate();
        if (nativeHandle == 0) {
            throw new RuntimeException("Failed to create RobotsMatcher");
        }
    }

    /**
     * Checks if a URL is allowed for a single user-agent.
     *
     * @param robotsTxt The robots.txt content
     * @param userAgent The user-agent string to check
     * @param url       The URL to check (should be %-encoded per RFC3986)
     * @return true if the URL is allowed, false otherwise
     */
    public boolean isAllowed(String robotsTxt, String userAgent, String url) {
        if (nativeHandle == 0) {
            throw new IllegalStateException("RobotsMatcher has been closed");
        }
        byte[] robotsBytes = robotsTxt.getBytes(StandardCharsets.UTF_8);
        byte[] uaBytes = userAgent.getBytes(StandardCharsets.UTF_8);
        byte[] urlBytes = url.getBytes(StandardCharsets.UTF_8);
        return nativeIsAllowed(nativeHandle, robotsBytes, uaBytes, urlBytes);
    }

    /**
     * Returns the line number that matched, or 0 if no match.
     */
    public int getMatchingLine() {
        if (nativeHandle == 0) {
            return 0;
        }
        return nativeGetMatchingLine(nativeHandle);
    }

    /**
     * Returns true if a specific user-agent block was found (not just '*').
     */
    public boolean everSeenSpecificAgent() {
        if (nativeHandle == 0) {
            return false;
        }
        return nativeEverSeenSpecificAgent(nativeHandle);
    }

    /**
     * Returns the crawl-delay in seconds, or null if not specified.
     */
    public Double getCrawlDelay() {
        if (nativeHandle == 0) {
            return null;
        }
        if (!nativeHasCrawlDelay(nativeHandle)) {
            return null;
        }
        return nativeGetCrawlDelay(nativeHandle);
    }

    /**
     * Returns the request-rate, or null if not specified.
     */
    public RequestRate getRequestRate() {
        if (nativeHandle == 0) {
            return null;
        }
        int[] rate = nativeGetRequestRate(nativeHandle);
        if (rate == null) {
            return null;
        }
        return new RequestRate(rate[0], rate[1]);
    }

    /**
     * Returns true if Content-Signal support is compiled in.
     */
    public static boolean isContentSignalSupported() {
        return nativeContentSignalSupported();
    }

    /**
     * Returns the content-signal values, or null if not specified.
     */
    public ContentSignal getContentSignal() {
        if (nativeHandle == 0) {
            return null;
        }
        if (!isContentSignalSupported()) {
            return null;
        }
        int[] signal = nativeGetContentSignal(nativeHandle);
        if (signal == null) {
            return null;
        }
        return new ContentSignal(
            triState(signal[0]),
            triState(signal[1]),
            triState(signal[2])
        );
    }

    /**
     * Returns true if AI training is allowed (defaults to true if not specified).
     */
    public boolean allowsAITrain() {
        if (nativeHandle == 0) {
            return true;
        }
        return nativeAllowsAITrain(nativeHandle);
    }

    /**
     * Returns true if AI input is allowed (defaults to true if not specified).
     */
    public boolean allowsAIInput() {
        if (nativeHandle == 0) {
            return true;
        }
        return nativeAllowsAIInput(nativeHandle);
    }

    /**
     * Returns true if search indexing is allowed (defaults to true if not specified).
     */
    public boolean allowsSearch() {
        if (nativeHandle == 0) {
            return true;
        }
        return nativeAllowsSearch(nativeHandle);
    }

    /**
     * Returns the library version string.
     */
    public static String getVersion() {
        return nativeGetVersion();
    }

    /**
     * Checks if a user-agent string contains only valid characters [a-zA-Z_-].
     */
    public static boolean isValidUserAgent(String userAgent) {
        byte[] uaBytes = userAgent.getBytes(StandardCharsets.UTF_8);
        return nativeIsValidUserAgent(uaBytes);
    }

    @Override
    public void close() {
        if (nativeHandle != 0) {
            nativeFree(nativeHandle);
            nativeHandle = 0;
        }
    }

    private static Boolean triState(int value) {
        switch (value) {
            case -1: return null;
            case 0: return Boolean.FALSE;
            case 1: return Boolean.TRUE;
            default: return null;
        }
    }

    // Native methods
    private static native long nativeCreate();
    private static native void nativeFree(long handle);
    private static native boolean nativeIsAllowed(long handle, byte[] robotsTxt, byte[] userAgent, byte[] url);
    private static native int nativeGetMatchingLine(long handle);
    private static native boolean nativeEverSeenSpecificAgent(long handle);
    private static native boolean nativeHasCrawlDelay(long handle);
    private static native double nativeGetCrawlDelay(long handle);
    private static native int[] nativeGetRequestRate(long handle);
    private static native boolean nativeContentSignalSupported();
    private static native int[] nativeGetContentSignal(long handle);
    private static native boolean nativeAllowsAITrain(long handle);
    private static native boolean nativeAllowsAIInput(long handle);
    private static native boolean nativeAllowsSearch(long handle);
    private static native String nativeGetVersion();
    private static native boolean nativeIsValidUserAgent(byte[] userAgent);
}
