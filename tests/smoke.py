#!/usr/bin/env python3
"""Boot WintiOS headless, type commands via the QEMU monitor, check serial output."""
import subprocess, time, sys, os
IMG = "build/wintios.img"
CMDS = sys.argv[1:] or ["help", "mem", "memtest", "mem", "uptime"]
KEYMAP = {" ": "spc", "\n": "ret", ".": "dot", "-": "minus", "<": "shift-comma"}
out = "/tmp/wintios_serial.txt"
open(out, "w").close()
q = subprocess.Popen(["qemu-system-i386", "-m", "16M", "-drive", f"format=raw,file={IMG}",
                      "-display", "none", "-serial", f"file:{out}", "-monitor", "stdio"],
                     stdin=subprocess.PIPE, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, text=True)
def mon(c): q.stdin.write(c + "\n"); q.stdin.flush()
time.sleep(2.5)
for cmd in CMDS:
    for ch in cmd + "\n":
        mon("sendkey " + KEYMAP.get(ch, ch)); time.sleep(0.04)
    time.sleep(1.5)
mon("quit"); q.wait(timeout=5)
print(open(out, errors="replace").read())
