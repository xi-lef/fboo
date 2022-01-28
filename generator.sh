#!/usr/bin/env sh
set -euo pipefail

if [ $# -ne 2 ]; then
    echo "Usage: $0 -t target.json"
    exit 1
fi

./fboo $2
