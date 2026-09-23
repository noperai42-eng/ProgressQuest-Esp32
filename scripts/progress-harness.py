#!/usr/bin/env python3
"""Drive Tami progression: desk binary and/or the USB disc.

Desk (fast, all 16 hatchlings + late game):
  make paths
  ./scripts/progress-harness.py --desk

Device (same work-day + field tour on the hooked-up AMOLED):
  ./scripts/progress-harness.py --device
  ./scripts/progress-harness.py --device --quick
"""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CLI = ROOT / "scripts" / "device-cli.py"
PATHS = ROOT / "build" / "tami-paths"
PEOPLE = ("human", "orc", "elf", "undead")
CALLS = ("warrior", "ranger", "mage", "rogue")
FIELDS = ("Greenroad", "Ironpit", "Moonwood", "Barrow", "Ashfen")
QUICK = (("orc", "warrior"), ("elf", "mage"), ("human", "ranger"), ("undead", "rogue"))


def run_desk() -> int:
    if not PATHS.exists():
        subprocess.check_call(["make", "paths"], cwd=ROOT)
    return subprocess.call([str(PATHS)], cwd=ROOT)


def _load_cli():
    import importlib.util

    spec = importlib.util.spec_from_file_location("device_cli", CLI)
    if spec is None or spec.loader is None:
        raise RuntimeError("device-cli.py missing")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


class Disc:
    def __init__(self, port: str | None) -> None:
        self.dc = _load_cli()
        self.port = self.dc.find_port(port)
        self.ser = self.dc.open_port(self.port)
        self.verbose = bool(os.environ.get("TAMI_HARNESS_VERBOSE"))
        banner = self.dc.drain(self.ser, 0.4)
        self._echo(banner)

    def _echo(self, text: str) -> None:
        if not text or not text.strip():
            return
        if self.verbose:
            print(text, end="" if text.endswith("\n") else "\n")
            return
        keep: list[str] = []
        for line in text.splitlines():
            if "esp_lvgl" in line or "task_wdt" in line or "Backtrace:" in line:
                continue
            if line.startswith("E (") or line.startswith("W ("):
                continue
            keep.append(line)
        if keep:
            print("\n".join(keep))

    def ask(self, line: str, ready, timeout: float = 8.0) -> str:
        leftover = self.dc.drain(self.ser, 0.15)
        self._echo(leftover)
        self.ser.write((line.strip() + "\n").encode("utf-8"))
        self.ser.flush()
        buf: list[str] = []
        deadline = time.time() + timeout
        while time.time() < deadline:
            chunk = self.dc.drain(self.ser, 0.25)
            if chunk:
                self._echo(chunk)
                buf.append(chunk)
                if ready("".join(buf)):
                    return "".join(buf)
        return "".join(buf)

    def hatch(self, people: str, calling: str) -> str:
        needle = f"hatched "
        return self.ask(
            f"hatch {people} {calling}",
            lambda t: needle in t or "lv 1" in t,
            4.0,
        )

    def hours(self, n: int) -> str:
        # Status lines look like "Name  lv 10 9%  Greenroad  |  …" — not STORY's lv=.
        want = 5 if n >= 8 else 1
        return self.ask(
            f"hours {n}",
            lambda t: "  |  " in t and _max_lv(t) >= want,
            8.0,
        )

    def story(self) -> str:
        return self.ask(
            "story",
            lambda t: "RECAP " in t.replace("\t", " ") or ("lv=" in t and "kills=" in t),
            6.0,
        )


def _max_lv(text: str) -> int:
    found = [int(m.group(1)) for m in re.finditer(r"\blv[= ](\d+)", text)]
    return max(found) if found else 0


def parse_story(text: str) -> dict[str, str]:
    recap = ""
    row: dict[str, str] = {}
    blob = text.replace("\t", " ")
    for line in blob.splitlines():
        if "RECAP " in line:
            recap = line.split("RECAP ", 1)[-1].strip()
        if "lv=" in line and "kills=" in line:
            bits = line.replace("STORY", " ").split()
            prev = ""
            for bit in bits:
                if "=" in bit:
                    k, _, v = bit.partition("=")
                    # Keep the first token of spaced values (plot=Act, quest=Seek, …).
                    if k not in row:
                        row[k] = v
                    prev = k
                elif prev == "xp" and "field" not in row:
                    row["field"] = bit
                    prev = "field"
    row["recap"] = recap
    return row


def run_device(port: str | None, quick: bool) -> int:
    matrix = list(QUICK) if quick else [(p, c) for p in PEOPLE for c in CALLS]
    disc = Disc(port)
    fails = 0
    # Live 20× floods the UART and races STORY. Catch-up (`hours`) does not need it.
    disc.ask("rate 1", lambda t: "rate" in t.lower(), 2.0)
    print(f"=== device work day ({len(matrix)} hatchlings) ===")
    recaps: list[str] = []
    for people, calling in matrix:
        disc.hatch(people, calling)
        disc.hours(8)
        row = parse_story(disc.story())
        recap = row.get("recap") or ""
        recaps.append(recap)
        lv = int(row.get("lv") or "0")
        kills = int(row.get("kills") or "0")
        faded = row.get("faded") == "1"
        print(f"  {people:7} {calling:8}  lv{lv:<3} kills {kills:<4}  {recap}")
        if lv < 5 or kills < 20 or not recap or faded:
            print("    FAIL work-day story")
            fails += 1

    print("=== device field tour ===")
    disc.hatch("orc", "warrior")
    disc.hours(8)
    for f in FIELDS:
        disc.ask(f"field {f}", lambda t, name=f: f"field {name}" in t, 3.0)
        disc.hours(1)
        row = parse_story(disc.story())
        here = row.get("field") or "?"
        print(f"  {f:10}  now {here}  {row.get('recap', '')}")
        if here != f and here != "?":
            print("    FAIL field")
            fails += 1
        if here == "?":
            print("    FAIL field parse")
            fails += 1

    print("=== device late gulp ===")
    disc.hours(12)
    disc.ask("field Ashfen", lambda t: "field Ashfen" in t, 3.0)
    disc.hours(1)
    row = parse_story(disc.story())
    print(f"  {row.get('recap', '')}")
    if int(row.get("lv") or "0") < 8:
        print("    FAIL late level")
        fails += 1

    disc.ask("auto", lambda t: "rate" in t.lower(), 2.0)
    print(f"{len(recaps)} recaps, {fails} fail(s)")
    return 1 if fails else 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--desk", action="store_true", help="run build/tami-paths")
    ap.add_argument("--device", action="store_true", help="drive the USB disc")
    ap.add_argument("--quick", action="store_true", help="4 hatchlings instead of 16")
    ap.add_argument("--port", default=None)
    args = ap.parse_args()
    if not args.desk and not args.device:
        args.desk = True
    rc = 0
    if args.desk:
        rc |= run_desk()
    if args.device:
        rc |= run_device(args.port, args.quick)
    return rc


if __name__ == "__main__":
    sys.exit(main())
