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
| Python | CPython 3.13.0 |

### Implementations

| Implementation | Repository | Notes |
|----------------|------------|-------|
| C++ (this repo) | [nzrsky/robotstxt](https://github.com/nzrsky/robotstxt) | Uses [ada](https://github.com/ada-url/ada) for WHATWG-compliant URL parsing |
| C++ (Google original) | [google/robotstxt](https://github.com/google/robotstxt) | Uses abseil, custom URL parsing |
| Go | [jimsmart/grobotstxt](https://github.com/jimsmart/grobotstxt) v1.0.3 | Direct port of Google's C++ |
| Rust | [Folyd/robotstxt](https://github.com/Folyd/robotstxt) v0.3.0 | Direct port of Google's C++ |
| Python | [Cocon-Se/gpyrobotstxt](https://github.com/Cocon-Se/gpyrobotstxt) v1.0.0 | Direct port of Google's C++ |

## Results

| Implementation | Mean | Min | Max | vs This Repo |
|:---|---:|---:|---:|---:|
| **C++ (this repo + ada)** | 38.5 ± 1.8 ms | 36.2 ms | 41.3 ms | 1.00 |
| C++ (Google original) | 46.5 ± 2.4 ms | 42.2 ms | 49.6 ms | 1.21× slower |
| Rust | 85.8 ± 2.7 ms | 80.1 ms | 88.8 ms | 2.23× slower |
| Go | 94.7 ± 1.6 ms | 92.5 ms | 97.1 ms | 2.46× slower |
| Python | 1.35 ± 0.01 s | 1.33 s | 1.37 s | 35× slower |

## Summary

This fork is **~21% faster** than the original Google implementation, **~2.3× faster** than Rust/Go ports, and **~35× faster** than the Python port.


### Throughput

| Implementation | Files/sec | Relative |
|----------------|-----------|----------|
| C++ (this repo) | ~178k | 1.00× |
| C++ (Google) | ~148k | 0.83× |
| Rust | ~80k | 0.45× |
| Go | ~72k | 0.41× |
| Python | ~5k | 0.03× |

## Reproduce

See [benchmark-utils/](benchmark-utils/) for wrapper programs and build instructions.

```bash
# Build this repo
mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make

# Build benchmark wrappers
cd benchmark-utils/go && go mod tidy && go build -o go-bench .
cd benchmark-utils/rust && cargo build --release
cd benchmark-utils/python && uv venv && source .venv/bin/activate && uv pip install -r requirements.txt

# Run hyperfine
hyperfine --warmup 3 --runs 10 \
  -n "C++ (this repo)" "./build/robots-bench robots_files/robots_all.bin" \
  -n "Go" "./benchmark-utils/go/go-bench robots_files/robots_all.bin" \
  -n "Rust" "./benchmark-utils/rust/target/release/rust-bench robots_files/robots_all.bin" \
  -n "Python" "./benchmark-utils/python/.venv/bin/python benchmark-utils/python/bench.py robots_files/robots_all.bin"
```
