#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/.."

REQ_HASH_FILE=".req_hash"
CURRENT_HASH=$(md5sum requirements.txt | awk '{ print $1 }')

if [ ! -d "venv" ]; then
    python3 -m venv venv
    source venv/bin/activate
    pip install -r requirements.txt
    echo "$CURRENT_HASH" > "$REQ_HASH_FILE"
else
    source venv/bin/activate
    if [ ! -f "$REQ_HASH_FILE" ] || [ "$(cat "$REQ_HASH_FILE")" != "$CURRENT_HASH" ]; then
        echo "Requirements changed, reinstalling..."
        pip install -r requirements.txt
        echo "$CURRENT_HASH" > "$REQ_HASH_FILE"
    fi
fi
