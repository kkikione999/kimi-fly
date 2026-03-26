#!/usr/bin/env python3
"""Compatibility wrapper for the generic hardware knowledge builder."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent


def main() -> int:
    cmd = [
        sys.executable,
        str(REPO_ROOT / "scripts" / "build_hardware_knowledge.py"),
        "--chip",
        "icm-42688-p",
        "--clean",
    ]
    return subprocess.call(cmd)


if __name__ == "__main__":
    raise SystemExit(main())
