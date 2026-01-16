# Performance Comparison: robots.txt Parsers

Benchmark comparing this C++ implementation against the original Google library and community ports to Go and Rust.

## Test Setup

- **Dataset**: 6,863 real robots.txt files from `robots_files/robots_all.bin`
- **Task**: Parse each file and check if `Googlebot` is allowed to access `/`
- **Tool**: [hyperfine](https://github.com/sharkdp/hyperfine) with 3 warmup runs and 10 measured runs

### System

- **CPU**: Apple M3 Max
- **OS**: macOS (Darwin 24.6.0)

### Compilers/Runtimes

| Language | Version |
|----------|---------|
| C++ | Apple clang 16.0.0 |
| Go | go1.25.1 darwin/arm64 |
| Rust | rustc 1.89.0 |

### Implementations

| Implementation | Repository | Notes |
|----------------|------------|-------|
| C++ (this repo) | [nzrsky/robotstxt](https://github.com/nzrsky/robotstxt) | Uses [ada](https://github.com/ada-url/ada) for WHATWG-compliant URL parsing |
| C++ (Google original) | [google/robotstxt](https://github.com/google/robotstxt) | Uses abseil, custom URL parsing |
| Go | [jimsmart/grobotstxt](https://github.com/jimsmart/grobotstxt) v1.0.3 | Direct port of Google's C++ |
| Rust | [Folyd/robotstxt](https://github.com/Folyd/robotstxt) v0.3.0 | Direct port of Google's C++ |

## Results

| Implementation | Mean | Min | Max | vs This Repo |
|:---|---:|---:|---:|---:|
| **C++ (this repo + ada)** | 36.8 ± 1.9 ms | 34.2 ms | 39.4 ms | 1.00 |
| C++ (Google original) | 43.4 ± 3.9 ms | 34.3 ms | 48.3 ms | 1.18× slower |
| Rust | 82.4 ± 2.5 ms | 79.8 ms | 87.4 ms | 2.24× slower |
| Go | 96.4 ± 1.5 ms | 93.6 ms | 98.7 ms | 2.62× slower |

## Summary

This fork is **18% faster** than the original Google implementation and approximately **2.5× faster** than the Rust and Go ports.


### Throughput

| Implementation | Files/sec | Relative |
|----------------|-----------|----------|
| C++ (this repo) | ~186k | 1.00× |
| C++ (Google) | ~158k | 0.85× |
| Rust | ~83k | 0.45× |
| Go | ~71k | 0.38× |

## Reproduce

```bash
# Clone comparison repos
git clone https://github.com/google/robotstxt.git google-robotstxt
git clone https://github.com/jimsmart/grobotstxt.git
git clone https://github.com/Folyd/robotstxt.git rust-robotstxt

# Build this repo
mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make

# Build Google original (requires bazel)
cd google-robotstxt && bazel build //:robots_main --compilation_mode=opt

# Build Go
cd grobotstxt && go build ./...

# Build Rust
cd rust-robotstxt && cargo build --release

# Run hyperfine (create benchmark binaries first - see bench/ directory)
hyperfine --warmup 3 --runs 10 \
  "./build/bench robots_files/robots_all.bin" \
  "./google-robotstxt/bazel-bin/bench robots_files/robots_all.bin" \
  "./rust-robotstxt/target/release/rust-bench robots_files/robots_all.bin" \
  "./grobotstxt/go-bench robots_files/robots_all.bin"
```
