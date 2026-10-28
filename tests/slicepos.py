#!/usr/bin/python3
"""Where a slice of the wheel is on screen, for the e2e test.
usage: slicepos.py WINX WINY WHEEL OPID FILE...   (WHEEL convert|tools; OPID "repeat" = the single repeat slice)
Prints "x y" (the middle of the slice) using the same geometry as qml/Wheel.qml and the
engine's own order (lingmo-fileaction-engine list --json)."""
import json
import math
import os
import shlex
import subprocess
import sys

INNER, MARGIN = 58, 28
wx, wy, wheel, opid, files = int(sys.argv[1]), int(sys.argv[2]), sys.argv[3], sys.argv[4], sys.argv[5:]
if opid == "repeat":
    ids, i, n = ["repeat"], 0, 1
else:
    engine = shlex.split(os.environ.get("LINGMO_FILEACTION_ENGINE", "lingmo-fileaction-engine"))
    doc = json.loads(subprocess.check_output(engine + ["list", "--json"] + files, env=dict(os.environ, LC_ALL="C.UTF-8")))
    ids = [o["id"] for o in doc["wheels"][wheel]][:12]
    i, n = ids.index(opid), len(ids)
OUTER = 200 if n > 8 else 168
cx, cy = wx + MARGIN + OUTER, wy + MARGIN + OUTER
r_mid = (INNER + 6) + ((OUTER - 7) - (INNER + 6)) * 0.55
if n == 1:
    print(int(cx), int(cy - r_mid))
    sys.exit(0)
step = 360 / n
a = math.radians(-90 + i * step)
print(int(cx + r_mid * math.cos(a)), int(cy + r_mid * math.sin(a)))
