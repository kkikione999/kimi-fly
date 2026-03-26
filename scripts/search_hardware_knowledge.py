#!/usr/bin/env python3
"""Search generated hardware knowledge chunks with a simple local scorer."""

from __future__ import annotations

import argparse
import json
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
MANIFEST_PATH = REPO_ROOT / "knowledge-hub" / "manifests" / "generated" / "hardware_chunks.jsonl"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("query", nargs="+", help="Query terms to search.")
    parser.add_argument("--chip", help="Restrict to one chip slug.")
    parser.add_argument("--limit", type=int, default=8, help="Maximum number of hits.")
    parser.add_argument(
        "--chunk-type",
        choices=["curated", "page_extract"],
        help="Restrict to one generated chunk type.",
    )
    return parser.parse_args()


def load_records() -> list[dict]:
    if not MANIFEST_PATH.exists():
        raise SystemExit(
            "Generated manifest not found. Run `python3 scripts/build_hardware_knowledge.py` first."
        )
    records = []
    for line in MANIFEST_PATH.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if line:
            records.append(json.loads(line))
    return records


def score_record(record: dict, terms: list[str]) -> int:
    haystack = " ".join(
        [
            record.get("chip", ""),
            record.get("chip_slug", ""),
            record.get("document_id", ""),
            record.get("document_kind", ""),
            record.get("source_pdf", ""),
            record.get("section", ""),
            " ".join(record.get("tags", [])),
            record.get("summary", ""),
        ]
    ).lower()

    score = 0
    for term in terms:
        if term in haystack:
            score += 5

    chunk_path = REPO_ROOT / record["chunk_path"]
    if chunk_path.exists():
        content = chunk_path.read_text(encoding="utf-8").lower()
        for term in terms:
            if term in content:
                score += 1

    return score


def main() -> None:
    args = parse_args()
    query = " ".join(args.query).strip().lower()
    terms = [term for term in query.split() if term]
    records = load_records()

    filtered = []
    for record in records:
        if args.chip and record["chip_slug"] != args.chip:
            continue
        if args.chunk_type and record["chunk_type"] != args.chunk_type:
            continue
        score = score_record(record, terms)
        if score > 0:
            filtered.append((score, record))

    filtered.sort(key=lambda item: (-item[0], item[1]["chip_slug"], item[1]["page"]))
    for score, record in filtered[: args.limit]:
        print(f"[score={score}] {record['chip_slug']} {record['section']} p{record['page']}")
        print(f"  pdf: {record['source_pdf']}")
        print(f"  path: {record['chunk_path']}")
        if record.get("summary"):
            print(f"  summary: {record['summary']}")
        print()


if __name__ == "__main__":
    main()
