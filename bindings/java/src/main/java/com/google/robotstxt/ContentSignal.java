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
 * Content-Signal values for AI content preferences.
 *
 * <p>Values are tri-state: null means not set, true means yes, false means no.
 */
public class ContentSignal {
    private final Boolean aiTrain;
    private final Boolean aiInput;
    private final Boolean search;

    public ContentSignal(Boolean aiTrain, Boolean aiInput, Boolean search) {
        this.aiTrain = aiTrain;
        this.aiInput = aiInput;
        this.search = search;
    }

    /**
     * AI training allowed (null = not set, true = yes, false = no).
     */
    public Boolean getAiTrain() {
        return aiTrain;
    }

    /**
     * AI input allowed (null = not set, true = yes, false = no).
     */
    public Boolean getAiInput() {
        return aiInput;
    }

    /**
     * Search indexing allowed (null = not set, true = yes, false = no).
     */
    public Boolean getSearch() {
        return search;
    }

    @Override
    public String toString() {
        return "ContentSignal{aiTrain=" + aiTrain + ", aiInput=" + aiInput + ", search=" + search + "}";
    }
}
