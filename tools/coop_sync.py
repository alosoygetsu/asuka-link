"""Compare the "Sync N:" lines (DW3_COOP_DEBUG=1, fightstg/events.c) of two or more test copies' coop.log after a
battle in lockstep (scripts/coop_test.ps1): the first event where the games went apart.
  python tools/coop_sync.py --pvp            test2 against test3, the second's sides swapped (P<->E)
  python tools/coop_sync.py test2 test3 test4 --from=2   a raid (its first line shows each player's own party)
  --max=N  how many differences to show (3)."""
import re
import sys
from pathlib import Path

ROOT = str(Path(__file__).resolve().parents[2])  # the project's folder: test2..test5
LINE = re.compile(r"Sync (\d+): event (\d+) \((-?\d+) (-?\d+)\) at (\d+), roll ([0-9A-F]+), seat (-?\d+) \|(.*)\|(.*)$")
FIGHTER = re.compile(r" ([PE])(\d)(\*?) (.*?);")


def parse(path, swap):
    out = []
    joined = []
    for raw in open(path, encoding="utf-8", errors="replace"):
        raw = raw.rstrip("\r\n")
        if " Sync+ " in raw and joined:
            joined[-1] += raw.split(" Sync+ ", 1)[1]
        elif " Sync " in raw:
            joined.append(raw)
    for raw in joined:
        m = LINE.search(raw)
        if not m:
            continue
        n, typ, a0, a1, idx, roll, seat, fighters, events = m.groups()
        a0 = int(a0)
        if swap and typ not in ("2", "3"):
            a0 = 16 - a0 if a0 in (0, 16) else a0
        if swap and typ in ("2", "3"):
            typ = {"2": "3", "3": "2"}[typ]
        fs = []
        for side, i, act, rest in FIGHTER.findall(fighters):
            if swap:
                side = {"P": "E", "E": "P"}[side]
            fs.append(f"{side}{i}{act} {rest}")
        evs = []
        for e in [x for x in events.split() if "@" in x and x.count("/") == 2]:
            t, s, f = e.split("@")[0].split("/")
            tm = e.split("@")[1]
            s = int(s)
            if swap:
                s = 16 - s if s in (0, 16) else s
                if t in ("2", "3"):
                    t = {"2": "3", "3": "2"}[t]
            evs.append(f"{t}/{s}/{f if t not in ('2', '3') else '-'}@{tm}")
        out.append(dict(n=int(n), typ=typ, a0=a0, a1=a1, roll=roll, seat=seat, fs=sorted(fs), evs=sorted(evs),
                        raw=raw.strip()))
    return out


def main():
    args = sys.argv[1:]
    pvp = "--pvp" in args
    copies = [a for a in args if not a.startswith("--")] or ["test2", "test3"]
    logs = [parse(f"{ROOT}\\{c}\\coop.log", pvp and k == 1) for k, c in enumerate(copies)]
    for c, l in zip(copies, logs):
        print(f"{c}: {len(l)} lines")
    base = logs[0]
    shown = 0
    for k in range(1, len(logs)):
        other = logs[k]
        for i in range(int(next((x[7:] for x in args if x.startswith("--from=")), "1")) - 1, min(len(base), len(other))):
            a, b = base[i], other[i]
            diffs = []
            for key in ("typ", "a0") + (() if a["typ"] in ("2", "3") else ("a1",)) + ("roll", "fs", "evs") + (() if pvp else ("seat",)):
                if a[key] != b[key]:
                    diffs.append(key)
            if diffs:
                print(f"-- {copies[0]} vs {copies[k]}: line {i + 1} differs in {', '.join(diffs)}")
                print(f"   {copies[0]}: {a['raw'][:600]}")
                print(f"   {copies[k]}: {b['raw'][:600]}")
                if i > 0:
                    print(f"   before: {base[i - 1]['raw'][:300]}")
                shown += 1
                if shown >= int(next((x[6:] for x in args if x.startswith("--max=")), "3")):
                    return
                break
        else:
            print(f"-- {copies[0]} vs {copies[k]}: the same for {min(len(base), len(other))} lines")


main()
