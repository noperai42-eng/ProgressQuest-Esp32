#!/usr/bin/env python3
"""Talk to a USB-plugged Tami disc (USB Serial JTAG console).

Examples:
  ./scripts/device-cli.py
  ./scripts/device-cli.py status
  ./scripts/device-cli.py rate 60
  ./scripts/device-cli.py hours 1
  ./scripts/device-cli.py watch 60
"""

from __future__ import annotations

import sys
import time
import serial
from serial.tools import list_ports

DEFAULT_PORT = "/dev/cu.usbmodem1101"


def find_port(explicit: str | None) -> str:
    if explicit:
        return explicit
    for p in list_ports.comports():
        if "usbmodem" in p.device or "USB" in (p.description or ""):
            return p.device
    return DEFAULT_PORT


def open_port(port: str) -> serial.Serial:
    ser = serial.Serial(port, 115200, timeout=0.2)
    time.sleep(0.25)
    ser.reset_input_buffer()
    return ser


def drain(ser: serial.Serial, wait: float = 0.4) -> str:
    deadline = time.time() + wait
    chunks: list[bytes] = []
    while time.time() < deadline:
        n = ser.in_waiting
        if n:
            chunks.append(ser.read(n))
            deadline = time.time() + 0.15
        else:
            time.sleep(0.05)
    return b"".join(chunks).decode("utf-8", errors="replace")


def send(ser: serial.Serial, line: str, wait: float = 0.5) -> str:
    ser.write((line.strip() + "\n").encode("utf-8"))
    ser.flush()
    return drain(ser, wait)


def main(argv: list[str]) -> int:
    args = argv[1:]
    port = None
    if args and args[0].startswith("/dev/"):
        port = args.pop(0)
    port = find_port(port)
    cmd = " ".join(args).strip()

    try:
        ser = open_port(port)
    except serial.SerialException as e:
        print(f"open {port}: {e}", file=sys.stderr)
        return 1

    print(f"# {port}", file=sys.stderr)
    banner = drain(ser, 0.3)
    if banner.strip():
        print(banner, end="" if banner.endswith("\n") else "\n")

    if cmd == "watch" or cmd.startswith("watch "):
        bits = cmd.split()
        if len(bits) > 1:
            print(send(ser, f"rate {bits[1]}"), end="")
        else:
            print(send(ser, "rate 60"), end="")
        print(send(ser, "status"), end="")
        try:
            while True:
                text = drain(ser, 0.6)
                if text:
                    print(text, end="" if text.endswith("\n") else "\n")
        except KeyboardInterrupt:
            print(send(ser, "auto"), end="")
            return 0

    if not cmd:
        print("type rate N | tick N | hours N | status | feed | pep | camp | bandage | quit")
        try:
            while True:
                line = input("> ").strip()
                if not line or line in ("quit", "exit", "q"):
                    break
                print(send(ser, line, 0.6), end="")
        except (EOFError, KeyboardInterrupt):
            print()
        return 0

    wait = 2.5 if cmd.startswith("hours") else 0.6
    print(send(ser, cmd, wait), end="")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
