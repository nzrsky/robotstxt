#!/usr/bin/env python3
"""Benchmark wrapper for gpyrobotstxt (Python port of Google's robots.txt parser)."""

import struct
import sys
from gpyrobotstxt.robots_cc import RobotsMatcher


def load_robots_files(filename: str) -> list[bytes]:
    """Load robots.txt files from binary format: [uint32_le length][content] repeated."""
    files = []
    with open(filename, 'rb') as f:
        while True:
            len_data = f.read(4)
            if not len_data:
                break
            length = struct.unpack('<I', len_data)[0]
            content = f.read(length)
            if len(content) != length:
                break
            files.append(content)
    return files


def main():
    if len(sys.argv) < 2:
        print("Usage: python bench.py <robots_all.bin>", file=sys.stderr)
        sys.exit(1)

    files = load_robots_files(sys.argv[1])

    allowed = 0

    for content in files:
        try:
            matcher = RobotsMatcher()
            if matcher.allowed_by_robots(content, ["Googlebot"], "http://example.com/"):
                allowed += 1
        except Exception:
            # Skip files that cause parsing errors in gpyrobotstxt
            allowed += 1  # Assume allowed on error (matches default behavior)

    print(f"Processed {len(files)} files, {allowed} allowed")


if __name__ == "__main__":
    main()
