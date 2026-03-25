#!/usr/bin/env python3
"""Summarize AUTO_TETHER_BALANCE_TEST logs and rank rounds by objective metrics."""

from __future__ import annotations

import argparse
import glob
import os
import re
from dataclasses import dataclass, field
from typing import Dict, Iterable, List, Optional

STAGE_RE = re.compile(
    r"\[AUTO_TEST\] stage=(?P<idx>\d+) name=(?P<name>[A-Za-z0-9_]+) "
    r"throttle=-?\d+(?:\.\d+)? roll=-?\d+(?:\.\d+)? pitch=-?\d+(?:\.\d+)? "
    r"blend=\d+ armed=\d+ mode=\d+ t=(?P<t>\d+)"
)
ABORT_RE = re.compile(
    r"\[AUTO_TEST\] ABORT_TILT roll=(?P<roll>-?\d+(?:\.\d+)?) "
    r"pitch=(?P<pitch>-?\d+(?:\.\d+)?).* t=(?P<t>\d+)"
)
COMPLETE_RE = re.compile(r"\[AUTO_TEST\] COMPLETE total=(?P<t>\d+) ms")
RESULT_RE = re.compile(
    r"\[AUTO_TEST\] result stage=(?P<idx>\d+) name=(?P<name>[A-Za-z0-9_]+) "
    r"dt=(?P<dt>\d+) cmd_t=(?P<cmd_t>-?\d+(?:\.\d+)?) "
    r"cmd_r=(?P<cmd_r>-?\d+(?:\.\d+)?) cmd_p=(?P<cmd_p>-?\d+(?:\.\d+)?) "
    r"roll_min=(?P<roll_min>-?\d+(?:\.\d+)?) roll_max=(?P<roll_max>-?\d+(?:\.\d+)?) "
    r"pitch_min=(?P<pitch_min>-?\d+(?:\.\d+)?)"
)


@dataclass
class StageResult:
    idx: int
    name: str
    dt_ms: int
    cmd_t: float
    cmd_r: float
    cmd_p: float
    roll_min: float
    roll_max: float
    pitch_min: float


@dataclass
class LogSummary:
    path: str
    last_stage_idx: int = -1
    last_stage_name: str = "-"
    abort_time_ms: Optional[int] = None
    abort_roll: Optional[float] = None
    abort_pitch: Optional[float] = None
    abort_stage_idx: Optional[int] = None
    abort_stage_name: Optional[str] = None
    complete_time_ms: Optional[int] = None
    results: Dict[int, StageResult] = field(default_factory=dict)

    @property
    def axis(self) -> str:
        if self.abort_roll is None or self.abort_pitch is None:
            return "-"
        return "roll" if abs(self.abort_roll) >= abs(self.abort_pitch) else "pitch"

    @property
    def score(self) -> float:
        if self.complete_time_ms is not None:
            return 1_000_000.0 + float(self.complete_time_ms)

        stage = self.abort_stage_idx if self.abort_stage_idx is not None else self.last_stage_idx
        abort_t = float(self.abort_time_ms or 0)
        base = stage * 10_000.0 + abort_t

        if self.abort_roll is not None and self.abort_pitch is not None:
            base -= abs(self.abort_roll) * 200.0
            base -= abs(self.abort_pitch) * 120.0

        return base


def sanitize_log_bytes(raw: bytes) -> str:
    # Keep ASCII text + line breaks; replace binary garbage with spaces.
    chars = []
    for b in raw:
        if b in (9, 10, 13) or 32 <= b <= 126:
            chars.append(chr(b))
        else:
            chars.append(" ")
    return "".join(chars)


def parse_log(path: str) -> LogSummary:
    summary = LogSummary(path=path)
    with open(path, "rb") as f:
        text = sanitize_log_bytes(f.read())
    events = []
    for m in STAGE_RE.finditer(text):
        events.append((m.start(), "stage", m))
    for m in ABORT_RE.finditer(text):
        events.append((m.start(), "abort", m))
    for m in COMPLETE_RE.finditer(text):
        events.append((m.start(), "complete", m))
    for m in RESULT_RE.finditer(text):
        events.append((m.start(), "result", m))

    for _, kind, m in sorted(events, key=lambda item: item[0]):
        if kind == "stage":
            summary.last_stage_idx = int(m.group("idx"))
            summary.last_stage_name = m.group("name")
            continue

        if kind == "abort":
            summary.abort_time_ms = int(m.group("t"))
            summary.abort_roll = float(m.group("roll"))
            summary.abort_pitch = float(m.group("pitch"))
            summary.abort_stage_idx = summary.last_stage_idx
            summary.abort_stage_name = summary.last_stage_name
            continue

        if kind == "complete":
            summary.complete_time_ms = int(m.group("t"))
            continue

        if kind == "result":
            idx = int(m.group("idx"))
            summary.results[idx] = StageResult(
                idx=idx,
                name=m.group("name"),
                dt_ms=int(m.group("dt")),
                cmd_t=float(m.group("cmd_t")),
                cmd_r=float(m.group("cmd_r")),
                cmd_p=float(m.group("cmd_p")),
                roll_min=float(m.group("roll_min")),
                roll_max=float(m.group("roll_max")),
                pitch_min=float(m.group("pitch_min")),
            )

    return summary


def expand_paths(inputs: Iterable[str]) -> List[str]:
    paths: List[str] = []
    for item in inputs:
        matches = sorted(glob.glob(item))
        if matches:
            paths.extend(matches)
        elif os.path.exists(item):
            paths.append(item)
    return sorted(set(paths))


def fmt_stage(summary: LogSummary, idx: int) -> str:
    r = summary.results.get(idx)
    if not r:
        return "-"
    return f"{r.name}:dt={r.dt_ms},rmin={r.roll_min:.2f},pmin={r.pitch_min:.2f}"


def print_report(summaries: List[LogSummary], top_n: int) -> None:
    ranked = sorted(summaries, key=lambda s: s.score, reverse=True)
    print("rank\tfile\tscore\tstage\tabort_t\tabort(r,p)\taxis\tS10\tS11")
    for i, s in enumerate(ranked[:top_n], start=1):
        abort_rp = (
            f"{s.abort_roll:.2f},{s.abort_pitch:.2f}"
            if s.abort_roll is not None and s.abort_pitch is not None
            else "-"
        )
        stage = (
            f"{s.abort_stage_idx}:{s.abort_stage_name}"
            if s.abort_stage_idx is not None
            else f"{s.last_stage_idx}:{s.last_stage_name}"
        )
        print(
            f"{i}\t{os.path.basename(s.path)}\t{s.score:.1f}\t{stage}\t"
            f"{s.abort_time_ms or '-'}\t{abort_rp}\t{s.axis}\t"
            f"{fmt_stage(s, 10)}\t{fmt_stage(s, 11)}"
        )

    if ranked:
        best = ranked[0]
        print("\nBest-run diagnosis:")
        if best.complete_time_ms is not None:
            print("- Already reached COMPLETE; optimize smoothness only.")
        elif (
            best.abort_stage_idx is not None
            and best.abort_stage_idx >= 11
            and best.abort_roll is not None
            and best.abort_pitch is not None
            and abs(best.abort_roll) >= 5.0
            and abs(best.abort_pitch) <= 1.5
        ):
            print(
                "- Dominant failure is roll-only drift at STAB_26 entry; keep pitch path frozen and tune roll authority in 0.22-0.26 only."
            )
        else:
            print(
                "- Keep one-variable-per-round tuning and prioritize the first stage where |roll| or |pitch| crosses 4.5 deg."
            )


def main() -> int:
    parser = argparse.ArgumentParser(description="Summarize tether-balance logs.")
    parser.add_argument("logs", nargs="+", help="Log files or glob patterns")
    parser.add_argument("--top", type=int, default=20, help="How many ranked rows to print")
    args = parser.parse_args()

    paths = expand_paths(args.logs)
    if not paths:
        print("No log files matched.")
        return 1

    summaries = [parse_log(path) for path in paths]
    print_report(summaries, max(1, args.top))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
