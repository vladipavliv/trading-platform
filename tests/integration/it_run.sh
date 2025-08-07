#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/../.."

export PYTHONPATH="$(pwd):$PYTHONPATH"

source venv/bin/activate
pytest tests/integration -s "$@"
