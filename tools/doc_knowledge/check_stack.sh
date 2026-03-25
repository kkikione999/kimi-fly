#!/usr/bin/env bash

set -euo pipefail

source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/common.sh"

if [[ ! -x "$DOCLING_BIN" ]]; then
  echo "Docling is not installed at $DOCLING_BIN" >&2
  exit 1
fi

if [[ ! -x "$QDRANT_BIN" ]]; then
  echo "Qdrant is not installed at $QDRANT_BIN" >&2
  exit 1
fi

echo "Docling: $("$DOCLING_BIN" --version)"
echo "Qdrant binary: $("$QDRANT_BIN" --version)"

if curl -fsS "$QDRANT_URL/healthz" >/dev/null 2>&1; then
  echo "Qdrant service: healthy at $QDRANT_URL"
else
  echo "Qdrant service: not running"
fi
