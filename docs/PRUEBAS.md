# Pruebas (solo para nosotros)

Lo más barato primero, y texto antes que capturas. Las copias de prueba (`test2..test5`) se abren con
`DW3_TEST_WINDOW=1`: en la segunda pantalla (o detrás), sin coger nunca el foco, sin consola y sin teclado, ratón ni
mandos reales (solo reciben las pulsaciones del script; el juego lee el mando aunque no tenga el foco, por eso no los
abren). El log va a `testN/prueba.log` (`DW3_TEST_LOG`). **Nunca cerrar el juego ni el launcher del usuario
(`native/build`).** No lanzar una copia con `Start-Process`: `dw2003.exe` es de consola y abre una que roba el foco
(se lanza con `CreateNoWindow`, como `Start-Copy` en `probar.ps1`). Para medir el foco, `GetForegroundWindow` de
cualquier proceso (la consola es de conhost).

`probar.ps1` usa la copia `test5`, la cierra al acabar, saca solo las líneas útiles y sale con 1 si falla.

## Qué prueba usar
| Cambio | Prueba |
|---|---|
| Launcher, sus textos | `build/dw3launcher.exe --check`; ver una pantalla: `--lang N --page P --state nogame\|error --keys "right,down" --shot f.png` |
| Lógica sin pantalla (formas, mods, GTE, sonido…) | `python native/tools/run_tests.py` |
| Structs del juego | `python native/tools/layout_check.py` |
| Digilog (pasos, misiones) | `probar.ps1 digilog -Save <card1.mcd> [-Slot N]`: lo vuelca en texto |
| Mapa, historia, gráficos | `probar.ps1 campo -Field 0x22B [-Progress N] [-Set "1C0F=1"] [-Shot]` (captura a la mitad en `test-shots/`) |
| Menús con botones | `probar.ps1 campo -Press "200:triangle,260:down,300:cross" -Until "<línea del log>" -Shot` |
| Que arranca sin caerse | `python native/tools/smoke.py` o `probar.ps1 campo` |
| Menú del Guardromon (una copia) | `probar.ps1 campo -CoopAuto menu [-Set "8192=1"] -Press "330:down" -Shot` |
| Partida nueva (aspecto, nombre, equipo) | `probar.ps1 campo -Field 0x500 -Press "…"` (X para pasar los textos) |
| Red, PvP, raids, duelos, intercambio | `powershell -File native\scripts\coop_test.ps1 -Mode see\|battle\|raid\|duel\|trade [-Players 2..4]` |
| Que un combate en red sale igual en todos | ver «Combates en red» |

## Opciones de `probar.ps1`
- Sin `-Save`: partida nueva con el equipo `-Party`. `-Save … -Slot N` carga una partida.
- `-Place "1,4"`: una sala concreta de un laberinto (GAME.place, placeArg). Un NPC puede quedar fuera de pantalla
  (Yang, sala 7 del Desierto Mobius): comprobarlo por el volcado, no por la captura.
- `-Steps "free,talk:45,text,digilog"`: pasos que esperan a poder hacerse (`talk:N` = personaje N de `dw3.py charlas`;
  `text` pasa los diálogos; `digilog` lo vuelca y acaba; `free` comprueba que se ha vuelto al campo). Con botones de
  menú, `wait:150` tras abrir el mapa del menú (con 60 la X llega antes de tiempo).
- `-Set "XXYY=1"`: marca F(XX):YY. `8192=1` da un objeto (0x8000 | id; 0x192 = Bolsa de Carpetas, la que abre las
  cartas); `20NN=1` marca el mapa 0x2NN como visitado (una misión aparece sin ir: El Dorado `2013=1`).
- `-Press "500:f2"` vuelca GAME a `test5/game_<vsync>.bin` (leerlo con `dw3.py estado`). `-Log` enseña el log
  entero; `-Keep` deja la copia abierta.
- `| Select-Object -First 1` corta el script antes de la captura (`campo.png` queda vieja): `-First 2` o más.
- Una captura puede pillar el cursor del juego en un fotograma casi vacío de su balanceo: no es un fallo.
- Recortar y ampliar la captura con PIL sirve para ver iconos y bordes sin pedir otra.

## Atajos para llegar rápido
- `-Field` usa el **modo** del mapa, no su `wstagNNN`: `dw3.py` lo da entre corchetes (`wstag225 [0x207]`) y
  `build/data/stages/wstagNNN.json` (`euModes`).
- `-Field 0x500` partida nueva; `0xF00` Armería de Asuka (las partidas de prueba no llegan a la confirmación);
  `0xD00` laboratorio (con `-Save native\saves\card1.mcd`); Tabla de digievolución:
  `-Press "300:down,330:down,360:cross,420:cross"`.
- `-Save test2/saves/card1.pre-dw3x.mcd -Slot 1` tiene aspecto guardado (Chica, tono 6): escenas con aspecto sin
  pasar por la partida nueva.
- Digilog sin partida: `probar.ps1 digilog -Progress 10 -Set "8192=1,1A22=1,…"` (6 s; el volcado sale en español y
  nombra las misiones con los nombres del disco: para un título propio, `campo -Set … -Steps "free,digilog" -Shot`).
  Para ver otra entrada: `-Keep -Press "600:down"` y capturar `test5`. El modo `digilog` se cierra solo: no da capturas.
- Pantallas largas de alcanzar: que el usuario llegue en `test5` (`-Keep`) y capturar su ventana con `PrintWindow`;
  más rápido que buscar los tiempos de `-Press` a ciegas.
- Guardromon: personaje 0x15 en todos los mapas; en Asuka (0x230) la primera charla depende de F1A:09 (`-Set
  "1A09=1"`). Probar el camino real del jugador: `-CoopAuto talk -Steps "free,talk:21"` (sin sala), no solo
  `DW3_COOP_AUTO`. `-CoopAuto` pone también `DW3_COOP_RAID=-1` (todas las raids en la lista).

## Combates en red (`coop_test.ps1`)
- `$env:DW3_COOP_DEBUG=1` enciende los registros (`coop.log`: un volcado por evento con tirada, luchadores, estados y
  cola). Luego `python native/tools/coop_sync.py --pvp` (raid: `test2 test3 test4 --from=2`, porque la primera línea
  siempre difiere) da la primera línea distinta. `native_coop_note` corta a 400 caracteres: partir las largas.
- `DW3_COOP_HP=6` alarga los combates (si no, mueren en dos golpes). `DW3_COOP_TECH=115,0,...` técnicas,
  `DW3_COOP_EQUIP=2:0x97,4:0x13E` equipo; raid: `DW3_COOP_ITEM=0x2B` (todos usan ese objeto en su 2.º turno),
  `DW3_COOP_FLEE=<asiento>` (huye en su 3.º).
- `-ShotAt "24,32"` captura todas las copias a esos segundos (juntarlas en una hoja con PIL). Raid de 2: primer
  turno a ~22 s, cada turno ~6 s.
- `-Mode trade [-Press …]` abre el intercambio con cartas de sobra; sin `-Press` se para a los 20 s y captura las dos.
- `DW3_COOP_STOMP=<fotogramas>` borra la copia lila con la ventana abierta (prueba que se rehace; el log dice
  `lilac CLUTs ... at (x, y)`).
- Las copias heredan el entorno: `$env:DW3_STEPS` antes de lanzar hace que todas sigan los mismos pasos.

## Launcher
- `dw3launcher.exe --shot f.png` abre la sala (escribe `sala.log`) y sale; con `DW3_LOBBY_PORT=275xx` no toca el 27499
  del usuario.
- Un launcher de prueba que juega: `DW3_TEST_WINDOW=1` lo deja oculto (no se minimiza ni se levanta) y lanza el juego
  sin consola; `--keys enter` le da a Jugar y `DW3_GAME_ARGS="--field 0x230 --progress 10"` pasa argumentos al juego
  (solo con `DW3_TEST_WINDOW`). Los avisos de la sala en el juego (`sala_aviso.txt` / `sala_respuesta.txt` junto al
  exe) salen en el log como `room notice:`. Para hacer de otro jugador basta un script de Python que hable el
  `Packet` de `sala_net.c` (HELLO, INVITE, ROOM, LEAVE); SELECT mantenido: `DW3_PRESS` con `N:select` cada 6 vsyncs. Un programa de 50 líneas con la misma SDL3 (en el scratchpad) sirvió de "launcher viejo" para probar
  la actualización sin descargar nada.

## Tests unitarios
- Si `run_tests.py` falla al enlazar ("símbolo sin enlazar"), el archivo probado llama a algo nuevo: un stub vacío
  en `tools/test_<nombre>.c` (así `postcard_draw` en test_softvram).
