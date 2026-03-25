#!/usr/bin/env bash

set -euo pipefail

source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/common.sh"

mkdir -p \
  "$ROOT_DIR/.local/doc_knowledge/logs" \
  "$QDRANT_HOME/data/storage" \
  "$QDRANT_HOME/data/snapshots"

if [[ -f "$QDRANT_PIDFILE" ]]; then
  existing_pid="$(cat "$QDRANT_PIDFILE")"
  if kill -0 "$existing_pid" 2>/dev/null; then
    echo "Qdrant already running: pid=$existing_pid url=$QDRANT_URL"
    exit 0
  fi
  rm -f "$QDRANT_PIDFILE"
fi

cd "$ROOT_DIR"
nohup "$QDRANT_BIN" --config-path "$QDRANT_CONFIG" --disable-telemetry >"$QDRANT_LOG" 2>&1 &
echo $! >"$QDRANT_PIDFILE"

for _ in $(seq 1 20); do
  if curl -fsS "$QDRANT_URL/healthz" >/dev/null 2>&1; then
    echo "Qdrant started: pid=$(cat "$QDRANT_PIDFILE") url=$QDRANT_URL"
    exit 0
  fi
  sleep 1
done

echo "Qdrant failed to become healthy. Recent log output:" >&2
tail -n 40 "$QDRANT_LOG" >&2 || true
exit 1
