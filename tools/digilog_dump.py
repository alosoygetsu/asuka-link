"""El Digilog en texto, de las líneas DIGILOG que el juego escribe con DW3_TEST=digilog (scripts/probar.ps1 digilog):
cada entrada de las dos pestañas con su título, su paso ("3/5") y el texto de ese paso, sin mirar una captura.

    python native/tools/digilog_dump.py <log>      (o la salida del juego por la entrada estándar)

Línea del juego (coop/patches/src/ststatus/item_list.c, journalTestDump):
    DIGILOG active quest 12 item 1A4 step 2/4 text 130 mode 22B held 0 need 0
"""
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import dw3  # noqa: E402
import journal_steps  # noqa: E402

LINE = re.compile(r"DIGILOG (active|done) (story|quest) (\d+) item ([0-9A-F]+) step (\d+)/(\d+) text (\d+) "
                  r"mode ([0-9A-F]+) held (\d) need ([0-9A-F]+)")


def main():
    src = open(sys.argv[1], encoding="utf-8", errors="replace") if len(sys.argv) > 1 else sys.stdin
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    steps = journal_steps.texts()  # (español, inglés) desde el texto FIRST_STEP_TEXT
    items = dw3.names_of("ITMNAM", "S")
    tab = None
    found = False
    for raw in src:
        if "DIGILOG begin" in raw:
            found = True
            print(f"Digilog (paso de la historia {raw.split()[-1]})")
            continue
        total = re.search(r"DIGILOG total (\d+)", raw)
        if total:  # under a step that asks for a party stat: the team's total now
            print(f"      ahora: {total.group(1)}")
            continue
        m = LINE.search(raw)
        if not m:
            continue
        t, kind, n, item, k, total, text, mode, held, need = m.groups()
        if t != tab:
            tab = t
            print("  ACTIVAS" if t == "active" else "  HECHAS")
        title = f"historia {n}" if kind == "story" else (
            items[int(item, 16)] if int(item, 16) < 0x200 else f"misión propia {n}")
        line = f"    {title} [mapa {mode}]"
        if int(total):
            i = int(text) - journal_steps.FIRST_STEP_TEXT
            words = steps[i][0].replace("\n", " ") if 0 <= i < len(steps) else f"texto {text}"
            line += f" paso {k}/{total}{' (hecho)' if held == '1' else ''}: {words}"
        if int(need, 16):
            line += f"  · necesita {items[int(need, 16)]}"
        print(line)
    if not found:
        sys.exit("no hay líneas DIGILOG: ¿se abrió el Digilog? (DW3_TEST=digilog)")


if __name__ == "__main__":
    main()
