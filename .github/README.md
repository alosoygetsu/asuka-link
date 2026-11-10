# Digimon World 2003: port nativo para PC, con cooperativo online

*English below.*

Este repositorio es un fork de [Feerdus95/dw3-native-port](https://github.com/Feerdus95/dw3-native-port), el port nativo
de *Digimon World 2003* para Windows, con un **modo cooperativo online para hasta 4 jugadores**. Cada uno juega su
propia partida, y cuando estáis en el mismo mapa veis a los demás moverse por él.

Se actualiza con las versiones del port original: todo lo que trae el port está aquí, más lo del cooperativo.

> **Necesitas tu propio disco.** Aquí no hay nada del juego: ni gráficos, ni sonido, ni textos. El port lee tu imagen
> del disco europeo (*Digimon World 2003*, SLES-03936).

## Instalar
1. Descarga `dw2003-coop-<versión>.zip` de la [última versión](https://github.com/alosoygetsu/dw3-native-port/releases/latest) y descomprímelo en una carpeta
   tuya (no en "Archivos de programa").
2. Abre `dw3launcher.exe` y dale tu imagen del disco (`.cue`/`.bin`). La lee una vez.
3. Juega con **Jugar**.

**Actualizar:** cuando haya versión nueva, el launcher la anuncia en la pantalla de inicio y se instala con un clic.
Tus ajustes y tus partidas no se tocan.

## Jugar con amigos
Necesitáis estar en una misma red. Lo más fácil es [Tailscale](https://tailscale.com) (gratis, sin abrir puertos en
el router), pero también vale una red local, ZeroTier, Radmin, etc.

1. En el launcher: **Ajustes → Mods → Cooperativo**.
2. Activa **Cooperativo**.
3. En **Tu IP** ves tus direcciones; pásale la tuya a los demás.
4. Escribe las de los demás en **IP del jugador 2, 3 y 4** (Enter para escribir, Enter para guardar).
5. Entrad en el mismo mapa: a los 1-2 segundos os veréis, cada uno con su personaje y su nombre encima.

Si algo no va, junto a `dw2003.exe` hay un `coop.log` que dice quién está conectado, el ping y los errores
habituales (IP equivocada, cortafuegos, versiones distintas). La guía completa está en `LEEME-COOP.txt`, dentro del
zip.

## Qué añade sobre el port original
| | |
|---|---|
| Cooperativo | Hasta 4 jugadores en red, cada uno con su partida; os veis en el mismo mapa, cada uno con su personaje y su nombre encima |
| Intercambio de cartas | En cualquier Guardromon: dos jugadores que eligen "Intercambiar cartas" se cambian cartas sueltas (las que no usa ningún mazo) |
| Raids | En cualquier Guardromon: "Raid" junta a 2 jugadores contra un jefe en un mismo combate por turnos; de momento, una raid básica (Kuwagamon) |
| Combates entre jugadores | En cualquier Guardromon: "Combatir" enfrenta vuestros equipos por turnos alternos; es una exhibición, sin game over y sin cambios en la partida |
| Equipo libre | Al empezar partida no se elige un pack: formas tu equipo con 3 de los 8 Digimon, sin repetir, viendo la descripción de cada uno. Los demás se siguen desbloqueando en la historia como siempre |
| Creador de personaje | Al empezar partida eliges ser Junior, Chico o Chica y uno de 8 tonos de color; con todas las animaciones, también en las escenas |
| Entrenamiento sin azar | En los gimnasios todos los intentos salen bien, cada stat sube lo máximo y ninguno baja |
| Multiplicador de EXP | x1 a x5 sobre la EXP de combate (compañeros y Digimon) |
| Multiplicador de dinero | x1 a x5 sobre el dinero de combate |
| Por defecto | Viaje rápido, Digilog, EXP compartida completa y entrenamiento sin azar vienen activados |
| Quitado | "Curar al subir de nivel", "Guardar en cualquier sitio" y los packs extra (el equipo libre los sustituye) |

Los cambios de cada versión están en [CHANGELOG-COOP.md](https://github.com/alosoygetsu/dw3-native-port/blob/coop/CHANGELOG-COOP.md).

## Hoja de ruta
- [x] Verse en el mapa (hasta 4 jugadores, configuración en el launcher)
- [x] Elegir tu propio equipo inicial de 3 Digimon
- [x] Nombre encima de cada jugador y su aspecto visible para los demás
- [ ] Chat y avisos ("X ha entrado en Asuka City")
- [x] Intercambio de cartas (en los Guardromon)
- [ ] Duelos de cartas entre jugadores
- [x] Combates entre jugadores
- [ ] Combates cooperativos contra jefes (raids)
- [x] Personalizar el aspecto del personaje (modelo y color)

## Créditos
Todo el juego nativo es obra de [Feerdus95/dw3-native-port](https://github.com/Feerdus95/dw3-native-port), que se
apoya en la decompilación [ReGame-Labs/dw3_decomp](https://github.com/ReGame-Labs/dw3_decomp). Este fork solo añade el
cooperativo y los cambios de la tabla. Licencia MIT, como el port. *Digimon* es © Akiyoshi Hongo, Toei Animation; es
un proyecto de fans sin relación con Bandai Namco.

El [README del port original](https://github.com/alosoygetsu/dw3-native-port/blob/coop/README.md) explica todo lo demás: requisitos, opciones, mods y cómo compilarlo.

---

## English

A fork of [Feerdus95/dw3-native-port](https://github.com/Feerdus95/dw3-native-port) (the native Windows port of
*Digimon World 2003*) with **online co-op for up to 4 players**: each plays their own save, and players on the same map
see each other. You need your own European disc; nothing of the game is here. Get `dw2003-coop-<version>.zip` from the
[latest release](https://github.com/alosoygetsu/dw3-native-port/releases/latest), set the other players' addresses in the launcher (Settings → Mods → Co-op),
and meet on a map: you see each other with your own look and name, and can trade spare cards at any Guardromon. A new game lets you pick your look (Junior, a boy or a girl, in 8 colours) and starts with a team you build yourself (any 3 of the 8 partners) instead of a starter
pack, and there are EXP and money multipliers. Updates come through the launcher's Update button.
