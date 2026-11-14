# Asuka Link

**Cooperativo online para *Digimon World 2003***, sobre el port nativo para PC.

*English below.*

Este repositorio es un fork de [Feerdus95/dw3-native-port](https://github.com/Feerdus95/dw3-native-port), el port nativo
de *Digimon World 2003* para Windows, con un **modo cooperativo online para hasta 4 jugadores**. Cada uno juega su
propia partida, y cuando estáis en el mismo mapa veis a los demás moverse por él.

Está hecho sobre la versión v0.18.9 del port de Feerdus, que trae todo lo de esa versión. Desde la v1.0.0 el
proyecto se mantiene aquí, por su cuenta, con sus propias versiones (v1.x); más abajo, en [Qué es de quién](#qué-es-de-quién), está qué es de
cada uno.

> **Necesitas tu propio disco.** Aquí no hay nada del juego: ni gráficos, ni sonido, ni textos. El port lee tu imagen
> del disco europeo (*Digimon World 2003*, SLES-03936).

## Instalar
1. Descarga `asuka-link-<versión>.zip` de la [última versión](https://github.com/alosoygetsu/asuka-link/releases/latest) y descomprímelo en una carpeta
   tuya (no en "Archivos de programa").
2. Abre `dw3launcher.exe` y dale tu imagen del disco (`.cue`/`.bin`). La lee una vez.
3. Juega con **Jugar**.

**Actualizar:** cuando haya versión nueva, el launcher la anuncia en la pantalla de inicio y se instala con un clic.
Tus ajustes y tus partidas no se tocan.

## Jugar con amigos
Todo se hace en la **sala**, arriba a la derecha de la pantalla principal del launcher.
- **En la misma red** (la de casa, un cibercafé…): los demás salen solos en la sala. Elige uno y **Invitar**; el otro
  acepta y ya estáis en el mismo grupo (hasta 4). Si sois más, cada grupo va por su lado.
- **Desde lejos** (Tailscale, ZeroTier, Internet con el puerto abierto): **+ Añadir por IP** y escribe la IP del otro;
  entra directamente en tu sala, sin que tenga que escribir la tuya.
- Cada jugador sale con su nombre, su personaje, sus barras de señal y, al seleccionarlo, su equipo y su ping.
  **Probar** comprueba la conexión y, si no llega nada, ofrece abrir el cortafuegos de Windows.
- **Listo** avisa a todos cuando estáis preparados. La sala se recuerda: la próxima vez que abráis el launcher os
  volvéis a juntar solos, y los que ya jugaron contigo salen como recientes.

Luego, **Jugar**, y entrad en el mismo mapa: a los 1-2 segundos os veréis, cada uno con su personaje y su nombre
encima. El juego sigue los cambios de la sala sin reiniciarlo.

Si algo no va, junto a `dw2003.exe` hay un `coop.log` (el juego) y un `sala.log` (la sala) que dicen quién está
conectado, el ping y los errores habituales. La guía completa está en `LEEME-COOP.txt`, dentro del zip.

## Qué añade sobre el port original
| | |
|---|---|
| Cooperativo | Hasta 4 jugadores en red, cada uno con su partida; os veis en el mismo mapa, cada uno con su personaje y su nombre encima |
| La sala | En la pantalla principal del launcher: los de tu red aparecen solos, se invitan con un clic, o se añaden por IP; con listo, prueba de conexión, recientes y reconexión al abrir |
| Launcher propio | Rehecho entero en la v1.3: menú lateral, Asuka City en movimiento, colores, letra (Silkscreen) e icono propios |
| Intercambio de cartas | En cualquier Guardromon: dos jugadores que eligen "Intercambiar cartas" (con la Bolsa de Carpetas) se cambian cartas sueltas (las que no usa ningún mazo) |
| Raids | En cualquier Guardromon: "Raid" junta de 2 a 4 jugadores contra un jefe (29 para elegir) en un mismo combate por turnos que avanza solo: juega cada uno ("Turno de…") y el jefe golpea a todos a la vez; con objetos, y huir saca solo al que huye. Al ganar, EXP y dinero x3 y un objeto seguro |
| Combates entre jugadores | En cualquier Guardromon: "Combatir" enfrenta vuestros equipos por turnos alternos, con las mismas reglas para los dos (armas, emblemas, estados); es una exhibición, sin game over y sin cambios en la partida |
| Duelos de cartas | En cualquier Guardromon: "Duelo" (con la Bolsa de Carpetas) os enfrenta en el juego de cartas, cada uno con uno de sus mazos; los dos ganan un sobre de cartas (mejor el del ganador) |
| Equipo libre | Al empezar partida no se elige un pack: formas tu equipo con 3 de los 8 Digimon, sin repetir, viendo la descripción de cada uno. Los demás se siguen desbloqueando en la historia como siempre |
| Creador de personaje | Al empezar partida eliges ser Junior, Chico o Chica y uno de 8 tonos de color; con todas las animaciones, también en las escenas |
| Entrenamiento sin azar | En los gimnasios todos los intentos salen bien, cada stat sube lo máximo y ninguno baja |
| Laboratorio de formas | En la Tabla de digievolución, □ enseña todas las formas del compañero, cuáles puede conseguir ya, lo que pide cada una (cumplido o no) y su línea: de qué formas viene y a cuáles lleva |
| Viaje rápido seguro | En el mapa del menú, "Sí" te lleva al momento a una zona visitada, pero solo desde donde podrías salir andando (no desde salas de evento) |
| Emblemas | Los de Búsqueda y Contraataque funcionan también junto a armas con efecto (igual para todos en PvP y raids); en PvP, las armas y los emblemas del rival funcionan igual que los tuyos |
| Multiplicador de EXP | x1 a x5 sobre la EXP de combate (compañeros y Digimon) |
| Multiplicador de dinero | x1 a x5 sobre el dinero de combate |
| Opciones | Dentro del juego, en el menú de pausa (OPCIONES, con Start en el mapa): pantalla completa, ventana, resolución, pantalla ancha en combates, iconos, vibración y botones. Los mods del port van fijos (viaje rápido, Digilog, texto rápido, EXP compartida completa, entrenamiento sin azar…) y EXP y dinero se cambian en el launcher |
| Eliminado | "Curar al subir de nivel", "Guardar en cualquier sitio" y los packs extra, del todo (el equipo libre sustituye a los packs; las partidas hechas con ellos siguen cargando) |

Los cambios de cada versión están en [CHANGELOG-COOP.md](https://github.com/alosoygetsu/asuka-link/blob/coop/CHANGELOG-COOP.md).

## Hoja de ruta
- [x] Verse en el mapa (hasta 4 jugadores)
- [x] La sala en el launcher: encontrar a los de tu red, invitar, añadir por IP (v1.3)
- [x] Launcher propio y opciones en el menú de pausa (v1.3)
- [x] Elegir tu propio equipo inicial de 3 Digimon
- [x] Nombre encima de cada jugador y su aspecto visible para los demás
- [x] Intercambio de cartas (en los Guardromon)
- [x] Duelos de cartas entre jugadores
- [x] Combates entre jugadores
- [x] Combates cooperativos contra jefes (raids)
- [x] Personalizar el aspecto del personaje (modelo y color)
- [ ] Avisos ("X ha entrado en Asuka City")
- [x] Completar el Digilog (paneles, fotos de los mapas, objeto que pide cada misión)
- [x] Pasos en la historia y en todas las misiones del Digilog
- [x] Encontrar las misiones que el Digilog aún no tiene (y sus pasos, en los 5 idiomas)
- [x] Misiones de DDNA: hechas cuando el DRI te da al compañero, y con su jefe aunque ya lo tengas
- [x] Raids: aviso de turno, objetos y huir (v1.6)
- [ ] Raids: ajustar al jefe según los jugadores
- [ ] Ver a los amigos en la pantalla del mapa e ir a su mapa

Todo lo del juego y del port, con su estado y las ideas, está en la [hoja de ruta completa](https://github.com/alosoygetsu/asuka-link/blob/coop/docs/HOJA-DE-RUTA.md).

## Qué es de quién
| | De quién | Versión |
|---|---|---|
| **Juego decompilado** | [ReGame-Labs/dw3_decomp](https://github.com/ReGame-Labs/dw3_decomp): el código del juego, sacado del disco | la que usa el port |
| **Port nativo** | [Feerdus](https://github.com/Feerdus95/dw3-native-port): que el juego funcione en PC, los gráficos mejorados, el launcher, las opciones y sus mods | la suya (v0.18.9, v0.19.0…) |
| **Cooperativo: Asuka Link** | alosoygetsu: todo lo de las tablas de arriba, en una capa aparte (`coop/patches/`, `coop/src/` y los ficheros `coop*`) | la propia: v1.0.0, v1.1.0… |

Cada release del cooperativo dice en qué versión del port está hecha, por ejemplo *"Asuka Link v1.0.0 (port
v0.18.9)"*, y cuál es su versión de red: todos los que jugáis juntos tenéis que tener la misma.
- **v1.x:** el número sube **1.1, 1.2…** cuando hay cosas nuevas y **1.1.1, 1.1.2…** cuando solo son arreglos.
- **Lo que cambia sobre el port:**
  - el launcher, rehecho entero (v1.3), y que se actualiza desde este repositorio;
  - las opciones: en el menú de pausa del juego y solo las de pantalla y controles; los mods, fijos;
  - "Curar al subir de nivel", "Guardar en cualquier sitio" y los packs extra, eliminados.

  El juego en sí, los gráficos y el Digilog vienen del port.

## Créditos
Todo el juego nativo es obra de [Feerdus95/dw3-native-port](https://github.com/Feerdus95/dw3-native-port), que se
apoya en la decompilación [ReGame-Labs/dw3_decomp](https://github.com/ReGame-Labs/dw3_decomp). Este fork solo añade el
cooperativo y los cambios de la tabla. Licencia MIT, como el port. *Digimon* es © Akiyoshi Hongo, Toei Animation; es
un proyecto de fans sin relación con Bandai Namco.

El [README del port original](https://github.com/alosoygetsu/asuka-link/blob/coop/README.md) explica todo lo demás: requisitos, opciones, mods y cómo compilarlo.

---

## English

A fork of [Feerdus95/dw3-native-port](https://github.com/Feerdus95/dw3-native-port) (the native Windows port of
*Digimon World 2003*) with **online co-op for up to 4 players**: each plays their own save, and players on the same map
see each other. You need your own European disc; nothing of the game is here. Get `asuka-link-<version>.zip` from the
[latest release](https://github.com/alosoygetsu/asuka-link/releases/latest). The launcher's home screen has a **room**:
players on your network show up by themselves and you invite them, or you add someone far away by IP (Tailscale,
ZeroTier…); then meet on a map: you see each other with your own look and name, and can trade spare cards at any Guardromon. A new game lets you pick your look (Junior, a boy or a girl, in 8 colours) and starts with a team you build yourself (any 3 of the 8 partners) instead of a starter
pack, and there are EXP and money multipliers. In the Digimon Lab's evolution chart, square lists every form of a partner, what each needs (met or not) and its line. From v1.3 the launcher is Asuka Link's own (side menu, a moving
Asuka City, its own colours, font and icon), and the in-game options live in the field's pause menu. Updates come
through the launcher's Update button.

**Whose is what, and versions.** The decompiled game is ReGame-Labs' (dw3_decomp); the native port (the PC platform, the
launcher, the options and its mods) is Feerdus's, with its own versions (v0.18.9, v0.19.0…); the co-op is alosoygetsu's,
kept in a layer of its own (`coop/patches/`, the `coop*` files), with its own versions from **v1.0.0** (1.1, 1.2… for new
things, 1.1.1… for fixes). Each release names the port release it stands on and its network version, which all players
must share.
