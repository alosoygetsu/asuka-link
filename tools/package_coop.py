"""The co-op build for the other player: package.py's folder plus a native.ini
with the co-op on and LEEME-COOP.txt, zipped as dist/dw2003-coop.zip.
Usage: python native/tools/package_coop.py [the other player's IP]
(the IP the friend's game sends to: the Tailscale 100.x.y.z of whoever runs this)"""
import shutil
import subprocess
import sys
from pathlib import Path

NATIVE = Path(__file__).resolve().parents[1]
DIST = NATIVE / "dist"
OUT = DIST / "dw2003-native"
GUIDE = NATIVE.parent / "LEEME-COOP.txt"


def main():
    peer = sys.argv[1] if len(sys.argv) > 1 else "PON_AQUI_LA_IP_DE_TU_COLEGA"
    if subprocess.run([sys.executable, NATIVE / "tools/package.py"]).returncode != 0:
        sys.exit("package.py failed")
    (OUT / "native.ini").write_text(
        "; Co-op: coop_peer es la IP de Tailscale de tu colega (100.x.y.z) y su puerto\n"
        "coop = 1\n"
        "coop_port = 27500\n"
        f"coop_peer = {peer}:27500\n", encoding="utf-8")
    shutil.copyfile(GUIDE, OUT / "LEEME-COOP.txt")
    zip_path = shutil.make_archive(str(DIST / "dw2003-coop"), "zip", OUT)
    print(f"OK {zip_path} (coop_peer = {peer}:27500)")


if __name__ == "__main__":
    main()
