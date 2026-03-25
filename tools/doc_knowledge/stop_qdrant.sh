#!/usr/bin/env bash

set -euo pipefail

source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/common.sh"

if [[ ! -f "$QDRANT_PIDFILE" ]]; then
  echo "Qdrant is not running."
  exit 0
fi

pid="$(cat "$QDRANT_PIDFILE")"
if kill -0 "$pid" 2>/dev/null; then
  kill "$pid"
  for _ in $(seq 1 10); do
    if ! kill -0 "$pid" 2>/dev/null; then
      rm -f "$QDRANT_PIDFILE"
      echo "Qdrant stopped."
      exit 0
    fi
    sleep 1
  done
  kill -9 "$pid" 2>/dev/null || true
fi

rm -f "$QDRANT_PIDFILE"
echo "Qdrant stopped."
