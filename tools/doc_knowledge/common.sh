#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
DOCLING_VENV="$ROOT_DIR/.venv-docling"
DOCLING_BIN="$DOCLING_VENV/bin/docling"
QDRANT_HOME="$ROOT_DIR/.local/doc_knowledge/qdrant"
QDRANT_BIN="$QDRANT_HOME/bin/qdrant"
QDRANT_CONFIG="$ROOT_DIR/tools/doc_knowledge/qdrant.yaml"
QDRANT_LOG="$ROOT_DIR/.local/doc_knowledge/logs/qdrant.log"
QDRANT_PIDFILE="$QDRANT_HOME/qdrant.pid"
QDRANT_URL="http://127.0.0.1:6333"
