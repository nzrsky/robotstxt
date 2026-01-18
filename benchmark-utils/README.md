# Benchmark Utilities

Wrapper programs for benchmarking robots.txt parser implementations.

## Benchmark Data

Download the benchmark data (~6800 real-world robots.txt files):

```bash
./benchmark-utils/download-data.sh
```

Or manually:
```bash
curl -L https://github.com/nzrsky/robotstxt-benchmark-data/raw/main/robots_all.bin.gz | gunzip > robots_files/robots_all.bin
```

### File Format

The binary file contains length-prefixed records:
```
[uint32_le length][content bytes] repeated
```

## Building

### Go ([jimsmart/grobotstxt](https://github.com/jimsmart/grobotstxt))

```bash
cd go
go mod tidy
go build -o go-bench .
```

### Rust ([Folyd/robotstxt](https://github.com/Folyd/robotstxt))

```bash
cd rust
cargo build --release
# Binary at: target/release/rust-bench
```

### Python ([Cocon-Se/gpyrobotstxt](https://github.com/Cocon-Se/gpyrobotstxt))

```bash
cd python
uv venv
source .venv/bin/activate
uv pip install -r requirements.txt
```

## Running Benchmarks

```bash
# From repository root
hyperfine --warmup 3 --runs 10 \
  -n "C++ (this repo)" "./build/robots-bench robots_files/robots_all.bin" \
  -n "Go" "./benchmark-utils/go/go-bench robots_files/robots_all.bin" \
  -n "Rust" "./benchmark-utils/rust/target/release/rust-bench robots_files/robots_all.bin" \
  -n "Python" "./benchmark-utils/python/.venv/bin/python benchmark-utils/python/bench.py robots_files/robots_all.bin"
```

## Expected Output

Each benchmark prints:
```
Processed 6863 files, XXXX allowed
```

The "allowed" count may differ slightly between implementations due to minor parsing differences.
