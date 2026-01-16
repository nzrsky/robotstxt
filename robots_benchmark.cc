// Copyright 2024 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <benchmark/benchmark.h>

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "robots.h"

namespace {

// Load all robots.txt files from binary format:
// uint32_t len; char data[len]; repeated
std::vector<std::string> LoadRobotsFiles(const char* filename) {
  std::vector<std::string> result;
  FILE* f = fopen(filename, "rb");
  if (!f) {
    fprintf(stderr, "Failed to open %s\n", filename);
    return result;
  }

  uint32_t len;
  while (fread(&len, sizeof(len), 1, f) == 1) {
    std::string buf(len, '\0');
    if (fread(buf.data(), 1, len, f) != len) break;
    result.push_back(std::move(buf));
  }
  fclose(f);
  return result;
}

// Global storage for robots.txt files (loaded once)
std::vector<std::string> g_robots_files;

void LoadFilesOnce() {
  if (g_robots_files.empty()) {
    g_robots_files = LoadRobotsFiles("robots_files/robots_all.bin");
    if (g_robots_files.empty()) {
      fprintf(stderr, "Warning: No robots.txt files loaded!\n");
    } else {
      fprintf(stderr, "Loaded %zu robots.txt files\n", g_robots_files.size());
    }
  }
}

// Benchmark: Parse all robots.txt files
static void BM_ParseAllRobotsTxt(benchmark::State& state) {
  LoadFilesOnce();

  for (auto _ : state) {
    for (const auto& robots_content : g_robots_files) {
      googlebot::RobotsMatcher matcher;
      std::vector<std::string> agents = {"Googlebot"};
      benchmark::DoNotOptimize(
          matcher.AllowedByRobots(robots_content, &agents, "/"));
    }
  }

  state.SetItemsProcessed(state.iterations() * g_robots_files.size());
  state.SetBytesProcessed(state.iterations() *
      [&]() {
        size_t total = 0;
        for (const auto& s : g_robots_files) total += s.size();
        return total;
      }());
}
BENCHMARK(BM_ParseAllRobotsTxt);

// Benchmark: Parse single robots.txt (average size)
static void BM_ParseSingleRobotsTxt(benchmark::State& state) {
  LoadFilesOnce();
  if (g_robots_files.empty()) return;

  // Pick a representative file (middle one)
  const std::string& robots_content = g_robots_files[g_robots_files.size() / 2];

  for (auto _ : state) {
    googlebot::RobotsMatcher matcher;
    std::vector<std::string> agents = {"Googlebot"};
    benchmark::DoNotOptimize(
        matcher.AllowedByRobots(robots_content, &agents, "/test/path"));
  }

  state.SetBytesProcessed(state.iterations() * robots_content.size());
}
BENCHMARK(BM_ParseSingleRobotsTxt);

// Benchmark: Match URL against parsed robots.txt (multiple user-agents)
static void BM_MatchMultipleUserAgents(benchmark::State& state) {
  LoadFilesOnce();
  if (g_robots_files.empty()) return;

  const std::string& robots_content = g_robots_files[g_robots_files.size() / 2];
  std::vector<std::string> agents = {"Googlebot", "Googlebot-Image", "Googlebot-News"};

  for (auto _ : state) {
    googlebot::RobotsMatcher matcher;
    benchmark::DoNotOptimize(
        matcher.AllowedByRobots(robots_content, &agents, "/some/path/to/check"));
  }
}
BENCHMARK(BM_MatchMultipleUserAgents);

// Benchmark: Just parsing without matching
class NoOpHandler : public googlebot::RobotsParseHandler {
 public:
  void HandleRobotsStart() override {}
  void HandleRobotsEnd() override {}
  void HandleUserAgent(int, std::string_view) override {}
  void HandleAllow(int, std::string_view) override {}
  void HandleDisallow(int, std::string_view) override {}
  void HandleSitemap(int, std::string_view) override {}
  void HandleUnknownAction(int, std::string_view, std::string_view) override {}
};

static void BM_ParseOnly(benchmark::State& state) {
  LoadFilesOnce();

  for (auto _ : state) {
    for (const auto& robots_content : g_robots_files) {
      NoOpHandler handler;
      googlebot::ParseRobotsTxt(robots_content, &handler);
    }
  }

  state.SetItemsProcessed(state.iterations() * g_robots_files.size());
}
BENCHMARK(BM_ParseOnly);

}  // namespace

BENCHMARK_MAIN();
