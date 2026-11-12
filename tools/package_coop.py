"""Asuka Link's zip, the only one a release has (release.ps1 names it asuka-link-<tag>.zip): package.py's folder plus
LEEME-COOP.txt, zipped as dist/dw2003-coop.zip. It is both the first install and the launcher's update, so it has no
native.ini: an update keeps the player's, and a first install takes the defaults (the co-op on, platform/coop.c).
Usage: python native/tools/package_coop.py [addresses for coop_peers]
(with them, a native.ini with those addresses: a build for one friend, not for a release)"""
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
    ini = OUT / "native.ini"
    if peer:
        ini.write_text(f"coop_peers = {peer}\n", encoding="utf-8")
    elif ini.exists():
        ini.unlink()  # one left by an earlier build for a friend
    shutil.copyfile(GUIDE, OUT / "LEEME-COOP.txt")
    zip_path = shutil.make_archive(str(DIST / "dw2003-coop"), "zip", OUT)
    print(f"OK {zip_path}" + (f" (coop_peers = {peer})" if peer else ""))


if __name__ == "__main__":
    main()
