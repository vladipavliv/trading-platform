#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
cd "$PROJECT_ROOT"

export PYTHONPATH="$PROJECT_ROOT:$PYTHONPATH"

if [ -f "/opt/venv/bin/activate" ]; then
    echo "Using Docker venv..."
    source /opt/venv/bin/activate
elif [ -f "$PROJECT_ROOT/venv/bin/activate" ]; then
    echo "Using local venv..."
    source "$PROJECT_ROOT/venv/bin/activate"
else
    echo "Warning: No virtual environment found. Using system python."
fi

echo "PYTHONPATH: $PYTHONPATH"
echo "Current Directory: $(pwd)"
echo "Listing root contents:"
ls -F

python3 -m pytest tests/integration -s "$@"