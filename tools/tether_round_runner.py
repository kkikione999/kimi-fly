#!/usr/bin/env python3
"""Autonomous single-round tether workflow: build, upload, capture, summarize."""

from __future__ import annotations

import argparse
import datetime as dt
import glob
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path

import serial


def find_pio() -> str:
    env_pio = os.environ.get("PIO_BIN")
    if env_pio:
        return env_pio

    resolved = shutil.which("pio")
    if resolved:
        return resolved

    fallback = os.path.expanduser("~/Library/Python/3.9/bin/pio")
    if os.path.exists(fallback):
        return fallback

    raise FileNotFoundError("Cannot find pio binary. Set PIO_BIN or install platformio.")


def find_serial_port(preferred: str | None) -> str:
    if preferred:
        return preferred

    candidates = sorted(
        glob.glob("/dev/cu.usbmodem*") + glob.glob("/dev/tty.usbmodem*")
    )
    if not candidates:
        raise FileNotFoundError("No usbmodem serial port found.")

    return candidates[-1]


def run_cmd(cmd: list[str], cwd: Path) -> None:
    print("[RUN]", " ".join(cmd))
    subprocess.run(cmd, cwd=str(cwd), check=True)


def capture_serial(port: str, baud: int, duration_s: float, output: Path) -> int:
    output.parent.mkdir(parents=True, exist_ok=True)
    total = 0
    deadline = time.time() + duration_s

    with serial.Serial(port, baudrate=baud, timeout=0.2, rtscts=False, dsrdtr=False) as ser:
        with output.open("wb") as f:
            while time.time() < deadline:
                data = ser.read(4096)
                if data:
                    f.write(data)
                    total += len(data)

    return total


def summarize_log(script_root: Path, log_path: Path) -> None:
    summary_py = script_root / "tether_log_summary.py"
    cmd = [sys.executable, str(summary_py), str(log_path), "--top", "1"]
    print("[RUN]", " ".join(cmd))
    subprocess.run(cmd, check=True)


def default_log_name(prefix: str, log_dir: Path) -> Path:
    stamp = dt.datetime.now().strftime("%Y%m%d_%H%M%S")
    return log_dir / f"{prefix}_{stamp}.log"


def main() -> int:
    parser = argparse.ArgumentParser(description="Run one autonomous tether balance round.")
    parser.add_argument("--env", default="flight_tether_balance", help="PlatformIO env")
    parser.add_argument("--port", default=None, help="Serial port, auto-detect if omitted")
    parser.add_argument("--baud", type=int, default=460800, help="Serial baudrate")
    parser.add_argument("--duration", type=float, default=35.0, help="Capture duration in seconds")
    parser.add_argument(
        "--log-prefix",
        default="tether_balance_auto",
        help="Output log file prefix",
    )
    parser.add_argument(
        "--log-dir",
        default="artifacts/flight_logs",
        help="Directory for captured logs",
    )
    parser.add_argument("--skip-build", action="store_true", help="Skip pio build")
    parser.add_argument("--skip-upload", action="store_true", help="Skip pio upload")
    parser.add_argument("--skip-summary", action="store_true", help="Skip summary output")
    parser.add_argument("--output", default=None, help="Explicit output log path")
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parents[1]
    stm32_dir = repo_root / "firmware" / "stm32"
    log_dir = repo_root / args.log_dir
    output = Path(args.output) if args.output else default_log_name(args.log_prefix, log_dir)

    pio = find_pio()

    if not args.skip_build:
        run_cmd([pio, "run", "-e", args.env], cwd=stm32_dir)

    if not args.skip_upload:
        run_cmd([pio, "run", "-e", args.env, "-t", "upload"], cwd=stm32_dir)

    port = find_serial_port(args.port)
    print(f"[INFO] Capturing {args.duration:.1f}s from {port} @ {args.baud} -> {output}")
    byte_count = capture_serial(port, args.baud, args.duration, output)
    print(f"[INFO] Captured {byte_count} bytes")

    if not args.skip_summary:
        summarize_log(repo_root / "tools", output)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
