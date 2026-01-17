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

#include <jni.h>
#include <cstring>
#include "robots_c.h"

extern "C" {

JNIEXPORT jlong JNICALL
Java_com_google_robotstxt_RobotsMatcher_nativeCreate(JNIEnv* env, jclass clazz) {
    return reinterpret_cast<jlong>(robots_matcher_create());
}

JNIEXPORT void JNICALL
Java_com_google_robotstxt_RobotsMatcher_nativeFree(JNIEnv* env, jclass clazz, jlong handle) {
    if (handle != 0) {
        robots_matcher_free(reinterpret_cast<robots_matcher_t*>(handle));
    }
}

JNIEXPORT jboolean JNICALL
Java_com_google_robotstxt_RobotsMatcher_nativeIsAllowed(
    JNIEnv* env, jclass clazz, jlong handle,
    jbyteArray robotsTxt, jbyteArray userAgent, jbyteArray url) {

    if (handle == 0) return JNI_TRUE;

    jbyte* robots_bytes = env->GetByteArrayElements(robotsTxt, nullptr);
    jsize robots_len = env->GetArrayLength(robotsTxt);

    jbyte* ua_bytes = env->GetByteArrayElements(userAgent, nullptr);
    jsize ua_len = env->GetArrayLength(userAgent);

    jbyte* url_bytes = env->GetByteArrayElements(url, nullptr);
    jsize url_len = env->GetArrayLength(url);

    bool result = robots_allowed_by_robots(
        reinterpret_cast<robots_matcher_t*>(handle),
        reinterpret_cast<const char*>(robots_bytes), robots_len,
        reinterpret_cast<const char*>(ua_bytes), ua_len,
        reinterpret_cast<const char*>(url_bytes), url_len
    );

    env->ReleaseByteArrayElements(robotsTxt, robots_bytes, JNI_ABORT);
    env->ReleaseByteArrayElements(userAgent, ua_bytes, JNI_ABORT);
    env->ReleaseByteArrayElements(url, url_bytes, JNI_ABORT);

    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jint JNICALL
Java_com_google_robotstxt_RobotsMatcher_nativeGetMatchingLine(JNIEnv* env, jclass clazz, jlong handle) {
    if (handle == 0) return 0;
    return robots_matching_line(reinterpret_cast<const robots_matcher_t*>(handle));
}

JNIEXPORT jboolean JNICALL
Java_com_google_robotstxt_RobotsMatcher_nativeEverSeenSpecificAgent(JNIEnv* env, jclass clazz, jlong handle) {
    if (handle == 0) return JNI_FALSE;
    return robots_ever_seen_specific_agent(reinterpret_cast<const robots_matcher_t*>(handle)) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_google_robotstxt_RobotsMatcher_nativeHasCrawlDelay(JNIEnv* env, jclass clazz, jlong handle) {
    if (handle == 0) return JNI_FALSE;
    return robots_has_crawl_delay(reinterpret_cast<const robots_matcher_t*>(handle)) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jdouble JNICALL
Java_com_google_robotstxt_RobotsMatcher_nativeGetCrawlDelay(JNIEnv* env, jclass clazz, jlong handle) {
    if (handle == 0) return 0.0;
    return robots_get_crawl_delay(reinterpret_cast<const robots_matcher_t*>(handle));
}

JNIEXPORT jintArray JNICALL
Java_com_google_robotstxt_RobotsMatcher_nativeGetRequestRate(JNIEnv* env, jclass clazz, jlong handle) {
    if (handle == 0) return nullptr;

    robots_request_rate_t rate;
    if (!robots_get_request_rate(reinterpret_cast<const robots_matcher_t*>(handle), &rate)) {
        return nullptr;
    }

    jintArray result = env->NewIntArray(2);
    if (result == nullptr) return nullptr;

    jint values[2] = { rate.requests, rate.seconds };
    env->SetIntArrayRegion(result, 0, 2, values);
    return result;
}

JNIEXPORT jboolean JNICALL
Java_com_google_robotstxt_RobotsMatcher_nativeContentSignalSupported(JNIEnv* env, jclass clazz) {
    return robots_content_signal_supported() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jintArray JNICALL
Java_com_google_robotstxt_RobotsMatcher_nativeGetContentSignal(JNIEnv* env, jclass clazz, jlong handle) {
    if (handle == 0) return nullptr;

    robots_content_signal_t signal;
    if (!robots_get_content_signal(reinterpret_cast<const robots_matcher_t*>(handle), &signal)) {
        return nullptr;
    }

    jintArray result = env->NewIntArray(3);
    if (result == nullptr) return nullptr;

    jint values[3] = { signal.ai_train, signal.ai_input, signal.search };
    env->SetIntArrayRegion(result, 0, 3, values);
    return result;
}

JNIEXPORT jboolean JNICALL
Java_com_google_robotstxt_RobotsMatcher_nativeAllowsAITrain(JNIEnv* env, jclass clazz, jlong handle) {
    if (handle == 0) return JNI_TRUE;
    return robots_allows_ai_train(reinterpret_cast<const robots_matcher_t*>(handle)) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_google_robotstxt_RobotsMatcher_nativeAllowsAIInput(JNIEnv* env, jclass clazz, jlong handle) {
    if (handle == 0) return JNI_TRUE;
    return robots_allows_ai_input(reinterpret_cast<const robots_matcher_t*>(handle)) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_google_robotstxt_RobotsMatcher_nativeAllowsSearch(JNIEnv* env, jclass clazz, jlong handle) {
    if (handle == 0) return JNI_TRUE;
    return robots_allows_search(reinterpret_cast<const robots_matcher_t*>(handle)) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jstring JNICALL
Java_com_google_robotstxt_RobotsMatcher_nativeGetVersion(JNIEnv* env, jclass clazz) {
    const char* version = robots_version();
    return env->NewStringUTF(version ? version : "");
}

JNIEXPORT jboolean JNICALL
Java_com_google_robotstxt_RobotsMatcher_nativeIsValidUserAgent(JNIEnv* env, jclass clazz, jbyteArray userAgent) {
    jbyte* ua_bytes = env->GetByteArrayElements(userAgent, nullptr);
    jsize ua_len = env->GetArrayLength(userAgent);

    bool result = robots_is_valid_user_agent(
        reinterpret_cast<const char*>(ua_bytes), ua_len
    );

    env->ReleaseByteArrayElements(userAgent, ua_bytes, JNI_ABORT);
    return result ? JNI_TRUE : JNI_FALSE;
}

}  // extern "C"
