"""The co-op build for the other player: package.py's folder plus a native.ini
with the co-op on and LEEME-COOP.txt, zipped as dist/dw2003-coop.zip.
Usage: python native/tools/package_coop.py [addresses for coop_peers]
(without, the players type them in the launcher)"""
import shutil
import subprocess
import sys
from pathlib import Path

NATIVE = Path(__file__).resolve().parents[1]
DIST = NATIVE / "dist"
OUT = DIST / "dw2003-native"
GUIDE = NATIVE / "LEEME-COOP.txt"


def main():
    peer = sys.argv[1] if len(sys.argv) > 1 else ""
    if subprocess.run([sys.executable, NATIVE / "tools/package.py"]).returncode != 0:
        sys.exit("package.py failed")
    # the co-op on; the other players' addresses go in from the launcher (Settings, Mods, Co-op)
    (OUT / "native.ini").write_text("coop = 1\n" + (f"coop_peers = {peer}\n" if peer else ""), encoding="utf-8")
    shutil.copyfile(GUIDE, OUT / "LEEME-COOP.txt")
    zip_path = shutil.make_archive(str(DIST / "dw2003-coop"), "zip", OUT)
    print(f"OK {zip_path}" + (f" (coop_peers = {peer})" if peer else ""))


if __name__ == "__main__":
    main()
