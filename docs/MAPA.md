# Mapa del código

Para orientarse rápido. Las consultas concretas (direcciones, símbolos, textos, marcas, Digimon), con
`python native/tools/dw3.py` (sin argumentos lista los subcomandos).

## El juego: `dw3_decomp/src` (el original, no se edita)
El ejecutable (`main`) siempre está cargado; los overlays (`*.PRO`) cargan todos en 0x80082CB0, uno cada vez, y los
mapas (`WSTAGnnn.PRO`) en 0x800A5DE0, encima de FIELDSTG.

- `main/` el ejecutable: `game/` estado y reglas (`events.c`: marcas, `checkCondition`/`applyAction`;
  `digimon.c`: `DIGIMON_DATA`; `items.c`: objetos y `TECHS`; `party.c`, `partner.c`, `stats.c`), `menu/` el menú del campo y la posada, `text/`
  ventanas de texto, `memcard/`, `pad/`, `sound/`, `gfx/`, `file/` carga de archivos, `system/` bucle y overlays,
  `task/` las tareas (el "planificador" del juego).
- `fieldstg/` el campo: actores, cámara, conversaciones (`speech.c`), eventos de historia (`story.c`,
  `event.c`, `script.c` el intérprete de guiones), combates del mapa (`battle.c`), el estado del campo (`field.c`,
  `state.c`), carga del mapa (`map*.c`, `field_stage.c`).
- `stages/<zona>/wstagNNN.c` cada mapa: su `setupStage`, sus actores y conversaciones (`actorNTalkM*`), sus guiones de
  evento (`scriptN`) y lo que hace cada evento al acabar (`endEventN`). `stages/README.md`: qué mapa es cada uno.
- `fightstg/` el combate (daño, turnos, técnicas, IA enemiga); `wfightmn/` su menú y eventos de turno;
  `wfightts/` el combate de pruebas.
- `cardgame/` el juego de cartas; `stcrdabm` álbum, `stcrddek` editor de mazos, `stcrdshp` tienda de cartas.
- `stfgtrep/` el informe tras el combate: crecimiento y digievoluciones (`STFGTREP_evolutions`).
- `ststatus/` el menú de estado (objetos, equipo, digievolución, mapa; el Digilog va en `item_list.c`).
- `stgdglab/` el DigiLab; `stgtrain/` el gimnasio; `stitshop/` la tienda; `stgmcard/` la tarjeta de memoria.
- `stplnmet/` partida nueva (nombre, aspecto, compañeros); `stdgname/` nombrar Digimon; `stdwtitl/` el título;
  `cnty_sel/` elegir idioma; `stagslct/`, `soundtst/`, `shocktst/` menús de depuración.
- `menu_common/` piezas compartidas entre overlays (teclado de nombres, barras de desplazamiento).

## Las capas del port
`build.py` copia el original, aplica `native/patches/` (los de Feerdus, no se tocan) y luego `native/coop/patches/`
(los nuestros), y compila desde `native/build/patched/src`.

- `native/platform/` el PC: `main.c` arranque y opciones fijas, `video.c`/`gpu*.c`/`gl.c` gráficos, `snd.c`/`spu.c`
  sonido, `pad.c`/`controls.c` mandos, `menu.c` las opciones del juego, `memcard.c`, `mods.c`/`stagemods.c` mods,
  `team.c` el equipo libre, `journal_text.c` textos del Digilog, `state.c` teclas de depuración.
- Cooperativo: `platform/coop.c` los otros jugadores en el campo, `coop_net.c` la red (UDP 27500, `COOP_VERSION`),
  `coop_battle.c` combates en paralelo (PvP, raids, duelos de cartas), `coop_look.c` su aspecto, `coop_tags.c` los
  nombres, `coop_test.c` ayudas de nuestras pruebas, `coop_text.h` textos propios (`tools/coop_text.py`).
- `native/coop/src/fieldstg/coop_*.inc.c` lo que el cooperativo hace dentro de FIELDSTG (los incluye el parche de
  `fieldstg/field.c`): `ghosts` los otros jugadores, `trade` intercambio, `window` la ventana del Guardromon,
  `pvp`, `raid`, `guard` su menú y el fotograma del cooperativo.
- `native/coop/patches/src/...` nuestros cambios a archivos del juego (combate, informe, partida nueva, Digilog…).

## El launcher: `native/launcher/`
`main.c` ventana y bucle; `home.c` pantalla principal; `sala.c`/`sala_net.c` la sala (UDP 27499, `NET_VERSION`);
`game.c` lanza el juego; `options.c`/`binds.c` opciones y teclas; `team.c`/`saves.c` lee la partida;
`release.c`/`self_update.c` actualizaciones; `first_run.c`, `picker.c`, `check.c` (`--check`); `lang_*.c` textos.

## Qué tocar para…
| Cambio | Dónde |
|---|---|
| Una regla del juego (combate, crecimiento, objetos) | `native/build/patched/src/<ruta>` + `sh mkpatch.sh src/<ruta>` |
| Algo del cooperativo en el campo | `native/coop/src/fieldstg/coop_*.inc.c` |
| La red, sincronía de combates | `platform/coop_net.c`, `coop_battle.c` (subir `COOP_VERSION` y `NET_VERSION` juntos) |
| Una opción del menú del juego | `platform/menu.c` (y su texto en `tools/coop_text.py`) |
| Una opción fija o de arranque | `platform/main.c` |
| Pasos del Digilog | `tools/journal_steps.py` (genera `include/journal_steps.h` y `platform/journal_text.h`) |
| Textos propios dentro del juego | `tools/coop_text.py` → `platform/coop_text.h` |
| La sala o el launcher | `launcher/sala*.c`, `home.c`; textos en `launcher/lang_*.c` (comprobar con `--check`) |
| Mods de mapas o de Digimon | `platform/stagemods.c`, `digimon_mods.c` |
| Estudiar una misión | `dw3.py marca`, `texto`, `charlas`, `pasos`, `eventos`, `dialogo`; `docs/MISIONES.md` |
