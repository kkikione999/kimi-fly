#!/usr/bin/env python3
"""Build AI-readable hardware knowledge chunks from local PDFs.

Default behavior is intentionally conservative:
- extract all pages for smaller datasheets
- keep very large reference manuals opt-in
- preserve curated summaries with source page numbers
"""

from __future__ import annotations

import argparse
import json
import re
import shutil
from pathlib import Path

from pypdf import PdfReader


REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_MANIFEST = REPO_ROOT / "knowledge-hub" / "manifests" / "hardware-docs.json"
DEFAULT_OUTPUT_ROOT = REPO_ROOT / "knowledge-hub" / "extracted" / "chunks"
DEFAULT_OUTPUT_MANIFEST = REPO_ROOT / "knowledge-hub" / "manifests" / "generated" / "hardware_chunks.jsonl"
LARGE_DOCUMENT_PAGE_THRESHOLD = 250


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--manifest",
        type=Path,
        default=DEFAULT_MANIFEST,
        help="Input manifest JSON file.",
    )
    parser.add_argument(
        "--chip",
        action="append",
        dest="chips",
        help="Build only the given chip slug. Can be specified multiple times.",
    )
    parser.add_argument(
        "--include-large-manuals",
        action="store_true",
        help="Also extract full-page chunks for large manuals/TRMs/reference manuals.",
    )
    parser.add_argument(
        "--curated-only",
        action="store_true",
        help="Only build curated chunks, skip page-by-page extraction.",
    )
    parser.add_argument(
        "--clean",
        action="store_true",
        help="Delete existing generated chunk directories before rebuilding selected chips.",
    )
    return parser.parse_args()


def load_manifest(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def load_generated_records(path: Path) -> list[dict]:
    if not path.exists():
        return []
    records = []
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if line:
            records.append(json.loads(line))
    return records


def slugify(value: str) -> str:
    value = value.lower().strip()
    value = re.sub(r"[^a-z0-9]+", "-", value)
    return value.strip("-")


def normalize_text(text: str) -> str:
    lines = [line.strip() for line in text.splitlines()]
    lines = [line for line in lines if line]
    return "\n".join(lines)


def should_extract_all_pages(doc: dict, include_large_manuals: bool, curated_only: bool) -> bool:
    if curated_only:
        return False
    if doc.get("extract_mode") != "all_pages":
        return False
    if doc.get("is_large_manual") and not include_large_manuals:
        return False
    return True


def safe_remove(path: Path) -> None:
    if path.exists():
        shutil.rmtree(path)


def write_markdown(path: Path, lines: list[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")


def build_page_chunk(
    *,
    chip_name: str,
    chip_slug: str,
    doc: dict,
    page_number: int,
    page_text: str,
    output_root: Path,
) -> dict | None:
    normalized = normalize_text(page_text)
    if not normalized:
        return None

    doc_slug = slugify(doc["id"])
    chunk_dir = output_root / chip_slug / doc_slug
    chunk_path = chunk_dir / f"p{page_number:04d}.md"

    lines = [
        f"# {chip_name} {doc['doc_kind']} Page {page_number}",
        "",
        f"- Chip: `{chip_name}`",
        f"- Chip Slug: `{chip_slug}`",
        f"- Document: `{doc['id']}`",
        f"- Document Kind: `{doc['doc_kind']}`",
        f"- Source PDF: `{doc['source_pdf']}`",
        f"- Page: `{page_number}`",
        f"- Tags: `{', '.join(doc.get('tags', []))}`",
        "",
        "## Extracted Page Text",
        "",
        normalized,
    ]
    write_markdown(chunk_path, lines)
    return {
        "chip": chip_name,
        "chip_slug": chip_slug,
        "document_id": doc["id"],
        "document_kind": doc["doc_kind"],
        "source_pdf": doc["source_pdf"],
        "page": page_number,
        "section": f"Page {page_number}",
        "tags": doc.get("tags", []),
        "summary": "",
        "chunk_path": str(chunk_path.relative_to(REPO_ROOT)),
        "chunk_type": "page_extract",
    }


def build_curated_chunk(
    *,
    chip_name: str,
    chip_slug: str,
    doc: dict,
    record: dict,
    page_text: str,
    output_root: Path,
) -> dict:
    page_number = int(record["page"])
    section_slug = slugify(record["section"])
    doc_slug = slugify(doc["id"])
    chunk_dir = output_root / chip_slug / doc_slug
    chunk_path = chunk_dir / f"curated-p{page_number:04d}-{section_slug}.md"

    lines = [
        f"# {record['section']}",
        "",
        f"- Chip: `{chip_name}`",
        f"- Chip Slug: `{chip_slug}`",
        f"- Document: `{doc['id']}`",
        f"- Document Kind: `{doc['doc_kind']}`",
        f"- Source PDF: `{doc['source_pdf']}`",
        f"- Page: `{page_number}`",
        f"- Tags: `{', '.join(record.get('tags', []))}`",
        "",
        "## Curated Summary",
        "",
        record["summary"],
        "",
        "## Extracted Page Text",
        "",
        normalize_text(page_text),
    ]
    write_markdown(chunk_path, lines)
    return {
        "chip": chip_name,
        "chip_slug": chip_slug,
        "document_id": doc["id"],
        "document_kind": doc["doc_kind"],
        "source_pdf": doc["source_pdf"],
        "page": page_number,
        "section": record["section"],
        "tags": record.get("tags", []),
        "summary": record["summary"],
        "chunk_path": str(chunk_path.relative_to(REPO_ROOT)),
        "chunk_type": "curated",
    }


def build_document(
    *,
    doc: dict,
    output_root: Path,
    include_large_manuals: bool,
    curated_only: bool,
) -> list[dict]:
    pdf_path = REPO_ROOT / doc["source_pdf"]
    reader = PdfReader(str(pdf_path))
    page_count = len(reader.pages)
    chip_name = doc["chip_name"]
    chip_slug = doc["chip_slug"]
    records: list[dict] = []

    if should_extract_all_pages(doc, include_large_manuals, curated_only):
        for page_index, page in enumerate(reader.pages, start=1):
            record = build_page_chunk(
                chip_name=chip_name,
                chip_slug=chip_slug,
                doc=doc,
                page_number=page_index,
                page_text=page.extract_text() or "",
                output_root=output_root,
            )
            if record is not None:
                records.append(record)

    for curated in doc.get("curated_pages", []):
        page_number = int(curated["page"])
        if page_number < 1 or page_number > page_count:
            raise ValueError(
                f"Curated page {page_number} out of range for {doc['source_pdf']} ({page_count} pages)"
            )
        page_text = reader.pages[page_number - 1].extract_text() or ""
        records.append(
            build_curated_chunk(
                chip_name=chip_name,
                chip_slug=chip_slug,
                doc=doc,
                record=curated,
                page_text=page_text,
                output_root=output_root,
            )
        )

    return records


def clean_selected_chip_dirs(output_root: Path, chip_slugs: set[str]) -> None:
    for chip_slug in chip_slugs:
        safe_remove(output_root / chip_slug)


def main() -> None:
    args = parse_args()
    manifest = load_manifest(args.manifest)
    output_root = DEFAULT_OUTPUT_ROOT
    output_manifest = DEFAULT_OUTPUT_MANIFEST
    output_manifest.parent.mkdir(parents=True, exist_ok=True)

    documents = manifest["documents"]
    if args.chips:
        requested = set(args.chips)
        documents = [doc for doc in documents if doc["chip_slug"] in requested]
    else:
        requested = {doc["chip_slug"] for doc in documents}

    if args.clean:
        clean_selected_chip_dirs(output_root, requested)

    all_records: list[dict] = []
    for doc in documents:
        all_records.extend(
            build_document(
                doc=doc,
                output_root=output_root,
                include_large_manuals=args.include_large_manuals,
                curated_only=args.curated_only,
            )
        )

    existing_records = [
        record
        for record in load_generated_records(output_manifest)
        if record.get("chip_slug") not in requested
    ]

    all_records = existing_records + all_records
    all_records.sort(key=lambda item: (item["chip_slug"], item["document_id"], item["page"], item["section"]))
    with output_manifest.open("w", encoding="utf-8") as handle:
        for record in all_records:
            handle.write(json.dumps(record, ensure_ascii=False) + "\n")

    print(f"Built {len(all_records)} chunk records into {output_manifest.relative_to(REPO_ROOT)}")


if __name__ == "__main__":
    main()
