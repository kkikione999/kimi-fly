#!/usr/bin/env bash

set -euo pipefail

source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/common.sh"

if [[ $# -lt 1 ]]; then
  echo "Usage: $0 <input.pdf> [output_dir] [extra docling args...]" >&2
  exit 1
fi

input_path="$1"
output_dir="${2:-$ROOT_DIR/.local/doc_knowledge/exports}"

mkdir -p "$output_dir"

cmd=(
  "$DOCLING_BIN"
  "$input_path"
  --from pdf
  --to md
  --to json
  --output "$output_dir"
  --ocr
  --num-threads 4
)

if [[ $# -gt 2 ]]; then
  cmd+=("${@:3}")
fi

"${cmd[@]}"
