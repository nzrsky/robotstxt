#!/bin/bash
# Download benchmark data from https://github.com/nzrsky/robotstxt-benchmark-data

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
DATA_DIR="$REPO_ROOT/robots_files"
DATA_FILE="$DATA_DIR/robots_all.bin"
DATA_URL="https://github.com/nzrsky/robotstxt-benchmark-data/raw/main/robots_all.bin.gz"

mkdir -p "$DATA_DIR"

if [ -f "$DATA_FILE" ]; then
    echo "Data file already exists: $DATA_FILE"
    exit 0
fi

echo "Downloading benchmark data..."
curl -L "$DATA_URL" | gunzip > "$DATA_FILE"
echo "Done: $DATA_FILE ($(du -h "$DATA_FILE" | cut -f1))"
