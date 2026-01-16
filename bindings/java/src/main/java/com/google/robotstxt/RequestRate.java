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

/**
 * Request-rate value (requests per time period).
 */
public class RequestRate {
    private final int requests;
    private final int seconds;

    public RequestRate(int requests, int seconds) {
        this.requests = requests;
        this.seconds = seconds;
    }

    public int getRequests() {
        return requests;
    }

    public int getSeconds() {
        return seconds;
    }

    /**
     * Returns requests per second.
     */
    public double getRequestsPerSecond() {
        if (seconds == 0) {
            return 0.0;
        }
        return (double) requests / seconds;
    }

    /**
     * Returns delay between requests in seconds.
     */
    public double getDelaySeconds() {
        if (requests == 0) {
            return 0.0;
        }
        return (double) seconds / requests;
    }

    @Override
    public String toString() {
        return "RequestRate{requests=" + requests + ", seconds=" + seconds + "}";
    }
}
