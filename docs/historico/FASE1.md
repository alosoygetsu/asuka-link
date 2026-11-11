# Fase 1: ver al otro jugador en el mapa

Objetivo: cada uno juega su propia partida en su PC. Cuando los dos estáis en el mismo mapa, ves a tu colega
caminando como un personaje más, y él a ti. Nada más: sin combates compartidos ni cambios en las partidas.

## Lo que dice el código del juego (dw3_decomp)

- **Los personajes del mapa son "actores"** (`include/fieldstg.h`, `struct Actor`). Se crean con
  `FIELDSTG_createActor(personaje, tipo, hueco_de_imagen, entrada)` (`src/fieldstg/actor.c`).
  - El jugador es el personaje `2` (`FIELD_CHARACTER_PLAYER`), tipo `0`, hueco de imagen `0`.
  - Los compañeros Digimon son los tipos `2`, `4` y `8`, con los huecos `1` a `3`.
  - Los NPC del mapa son el tipo `1`, con el hueco que indica su entrada.
- **Posición:** `actor->pos` (x, y en 1/256 de casilla), `actor->z` (altura), `actor->dir` (dirección 0 a 4),
  `actor->animSet` (andar, correr, quieto...).
- **Cada frame**, `FIELDSTG_updateActor` llama a `actor->control` (para el jugador lee el mando), mueve la casilla,
  anima y dibuja ordenando por `y`. Si un actor tiene `control = NULL`, no se mueve solo: nosotros le ponemos la
  posición cada frame. Así será el actor del colega.
- **Qué mapa es:** `GAME.funcs.getMode()` dice el modo actual (cada mapa es un modo) y `FIELDSTG_state.stageFile`
  el archivo del mapa. Con los dos se sabe si estáis en el mismo sitio.
- **El campo crea y destruye sus personajes** en `src/fieldstg/field.c` (`createParty`, `createCharacters`,
  `killCharacters`). Ahí enganchamos crear y destruir el actor del colega.

## El problema principal: el hueco de imagen

Cada actor tiene un hueco en la VRAM (la memoria de vídeo de la PS1, 1024x512) donde se copia su fotograma actual
(`ActorImage`, `FIELDSTG_animateActor`). Cada mapa reparte sus huecos a mano en su tabla `stageImages`, así que no
hay uno libre fijo para un personaje extra. Opciones, de más a menos preferida:

1. **Buscar al entrar en cada mapa una zona libre de VRAM** del tamaño del sprite del jugador y crear allí un
   `ActorImage` propio. Es lo más limpio si casi todos los mapas tienen hueco libre (hay que medirlo).
2. **Dibujarlo por fuera, desde el port** (`platform/`, con OpenGL), encima de la escena. Funciona siempre, pero
   no se ordena bien con los objetos del mapa (puede verse por encima de una pared).
3. **Ampliar la VRAM emulada del port** solo para este sprite. Es posible porque la VRAM es nuestra
   (`platform/gpu.c`), pero toca mucho código de dibujo.

El primer hito sirve para decidir esto con datos.

## La red

- **UDP directo entre los dos PCs** (hecho así: un paquete por frame, sin hilo aparte; ver Estado), plan inicial de 20 paquetes por segundo, unos 32 bytes cada uno:
  `versión, modo (mapa), stageFile, x, y, z, dir, animSet, personaje`.
- **Sin servidor ni abrir puertos:** usad [Tailscale](https://tailscale.com) (gratis). Os da a los dos una IP
  privada, como si estuvierais en la misma red local. La IP del colega va en `native.ini` (`coop_peer = 100.x.y.z`).
- **Hilo aparte** para la red (Winsock, `-lws2_32`): el juego solo lee "el último estado recibido", así que la red
  nunca lo frena. Entre paquetes, la posición se interpola para que se mueva suave.
- Si no llegan paquetes en 3 segundos, o el colega está en otro mapa, su personaje desaparece.

## Dónde va el código

- **`native/platform/coop.c`** (nuevo): la red, el estado del colega y la interpolación.
- **`native/patches/src/fieldstg/field.c.patch`** (ya existe, se amplía): al crear el grupo se crea también el actor
  del colega, y se destruye con los demás.
- **Un "control" propio para ese actor:** cada frame copia la posición, la dirección y la animación interpoladas.
- Nada de esto cambia las partidas guardadas: el colega es solo algo que se dibuja.

## Hitos

1. **Fantasma local, sin red:** un segundo protagonista que repite tus pasos con 2 segundos de retraso. Prueba que
   sabemos crear, mover, animar y dibujar al actor extra, y mide si hay VRAM libre en los mapas.
2. **Red en tu PC:** dos copias del juego en tu ordenador, conectadas por `127.0.0.1`.
3. **Con tu colega por Tailscale.**
4. **Extras:** nombre encima de la cabeza, chat sencillo (dibujado por el port, como su menú de opciones) y
   avisos tipo "tu colega ha entrado en Asuka City".

## Estado

- **Hito 1 hecho (2026-11-08):** `coop = 1` en `native.ini` muestra el fantasma. Lo aprendido:
  - Una tarea del juego solo se ejecuta si es hija de otra que corre: el fantasma ocupa un hueco libre de
    `FieldChildren.npcs`.
  - La página de VRAM del protagonista está llena (la hoja de sprites del mapa la ocupa entera). El fantasma usa una
    zona en blanco de otra página a la derecha de los framebuffers (x ≥ 640), con una copia de la paleta del
    protagonista. Encontrada en todos los mapas probados (0x203, 0x210, 0x214, 0x218, 0x230, 0x258, 0x2C0).
  - `DW3_COOP_OFFSET=N` (variable de entorno) lo desplaza N píxeles a la derecha, para verlo sin moverse.
  - Bug del port original, sin relación con el coop: `--party` cuelga el juego (la macro `SDL_clamp` evalúa
    `argv[++i]` tres veces), y `--field` solo acepta el modo en decimal.
- **Hito 2 hecho (2026-11-08):** red UDP en `native/platform/coop_net.c` (aparte de `coop.c` porque `windows.h` y
  `platform.h` definen los dos `RECT`). Configuración en `native.ini`:
  ```
  coop = 1
  coop_peer = 127.0.0.1:27501   ; IP:puerto del otro jugador (con Tailscale, su 100.x.y.z:27500)
  coop_port = 27500             ; dónde escucha esta copia (27500 si no se pone)
  ```
  Sin `coop_peer`, sigue el fantasma del hito 1.
  - Un paquete de 48 bytes por frame del mapa (50 por segundo, ~2,4 KB/s). Solo cuentan los paquetes que vienen de la
    dirección de `coop_peer`.
  - El colega se dibuja como estaba hace 80 ms en su reloj, interpolando entre los dos paquetes de alrededor: así los
    paquetes tardíos o perdidos no le hacen dar saltos. Sin paquetes en 2 s (fuera del mapa, en combate, juego
    cerrado), desaparece, y vuelve solo al llegar otro.
  - Probado con dos copias en el mismo PC: `test2\` tiene otra copia del exe, su `native.ini` (puertos al revés), su
    propia `saves\` y `data` como enlace (junction) a `native\build\data`. La copia de `saves` es necesaria: las dos
    escribirían en la misma memory card.
- **Hito 3 preparado (2026-11-08), falta probarlo con el colega:**
  - `coop.log` junto al exe: conexión, desconexión, ping, jitter y paquetes perdidos (cada minuto), y avisos de IP o
    puerto equivocados y de versiones distintas. El paquete (versión 2) devuelve el último `time` recibido, para medir
    el ping. El ping en local sale de ~20 ms porque la red se lee una vez por frame.
  - El retraso con el que se dibuja al otro se adapta al jitter: 80 ms + 3 × jitter, como mucho 300 ms.
  - Con un `coop_peer` que no funciona ya no sale el fantasma del hito 1: no sale nadie.
  - Paquete para el colega: `python native/tools/package_coop.py [IP]` crea `native/dist/dw2003-coop.zip`, con el exe
    sin datos del juego, el launcher, un `native.ini` con el coop y [LEEME-COOP.txt](LEEME-COOP.txt). Para que el exe
    salga sin los datos del juego, `native/data` es un enlace (junction) a `native/build/data`: si no existe, el exe
    los lleva dentro y `package.py` se niega a empaquetarlo. Probado: el zip descomprimido en limpio lee el disco
    (`--extract`) y se conecta.

## Riesgos

- **VRAM:** ver arriba.
- **Mapas especiales:** cuando vuelas (personaje `0x147`), en cinemáticas o con personajes grandes, al principio
  no mostraremos al colega en esos mapas.
- **Versiones distintas:** los dos tenéis que usar el mismo `dw2003.exe`; el paquete lleva un número de versión y
  se ignora si no coincide.

## Mantenerlo al día con el port (desde 2026-11-08)

- **Capas:** el coop no toca los `.patch` del port. Sus cambios a `field.c` y `actor_control.c` están en
  `native/coop/patches/`, y `build.py` los aplica encima del archivo ya parcheado por el port. Las declaraciones
  van en `include/native_coop.h`, y las claves del ini en `native_coop_ini()`. En `native.h` y `main.c` solo quedan
  un par de líneas nuestras. `sh mkpatch.sh src/fieldstg/field.c` regenera la capa.
- **Remotes en `native/`:** `upstream` es Feerdus95/dw3-native-port y `origin` es tu fork
  (https://github.com/alosoygetsu/asuka-link, rama por defecto `coop`).
- **`.\update.ps1`:** fusiona la última release del port en `coop`, pone `dw3_decomp` en su `DECOMP_COMMIT`, compila,
  pasa los tests y prueba dos copias conectadas. `-TestOnly` hace solo las comprobaciones.
- **`.\release.ps1`:** crea el tag `v<X>.<Y>.<Z*100+n>` (v0.18.9 → v0.18.901), compila el exe y el launcher y sube a
  la release del fork:
  - `dw2003-native-<tag>.zip`, el que descarga el botón Actualizar del launcher;
  - `dw2003-coop-<tag>.zip`, para la primera instalación.
  
  `-DryRun` solo empaqueta. El launcher (`launcher/release.c`, `REPO`) busca las actualizaciones en el fork.
- **Aviso semanal:** la tarea programada "dw2003-port-release-check" de la app, los lunes a las 10:12. Solo avisa.
- Primera release: **v0.18.901** (port v0.18.9).

## Hasta 4 jugadores y launcher (v0.18.902, 2026-11-08)
- **Red en malla:** `coop_peers = a, b, c` (sin puerto, 27500). Cada compañero tiene sus propios estados, reloj, ping y
  avisos en `coop.log` ("player N").
- **Hasta 3 fantasmas,** cada uno con su caja de VRAM y su copia de la paleta, en otro color (rojo, verde, morado).
  - `native_coop_vram_tint` cambia solo los azules, así que aplicarlo dos veces da lo mismo.
  - `animateActor` lo vuelve a aplicar tras cada carga de fotograma, porque cada TIM trae la paleta original.
  - Las claves de los fantasmas son 0x7C02, 0x7D02 y 0x7E02.
- **Launcher, pestaña Mods → Cooperativo:** activarlo, las IP de los jugadores 2 a 4 (filas de texto,
  `launcher/coop_rows.c`) y "Tu IP", con las IPv4 de este PC y la de Tailscale marcada.
- **Multiplicadores de EXP y dinero** (x1-x5), en las capas `coop/patches/src/stfgtrep/{report,partner}.c.patch`.
- **Por defecto:** viaje rápido, Digilog y EXP compartida completa activados. Curar al subir de nivel y guardar en
  cualquier sitio, quitados.
- **El fantasma de ti mismo** ya solo sale para pruebas, con `DW3_COOP_GHOST=1`.
- **Pruebas hechas:**
  - 4 copias en local: cada una ve a las otras 3 con sus colores.
  - Launcher: escribir IP y guardarlas.
  - Informe de combate (debug F1 → Battle Report, con un gancho temporal): dinero ×3 = 4260 de 1420. La EXP no se ve
    en ese informe; se comprobó en el código.
  - Actualización de la v0.18.901 a la v0.18.902 desde el launcher.
- **Siguiente:** ver [INVESTIGACION.md](INVESTIGACION.md) (intercambios, cartas, PvP, raids, creador de personaje).
