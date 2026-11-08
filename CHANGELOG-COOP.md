# Cambios del cooperativo

Las versiones van sobre las del port original: `v0.18.901` es el port `v0.18.9` más la versión 1 del cooperativo.
Los cambios del port están en su [CHANGELOG.md](CHANGELOG.md).

## v0.18.902
Sobre el port v0.18.9.

- **Cooperativo para hasta 4 jugadores.** Cada juego manda su posición a los demás y recibe la suya; no hay
  anfitrión, así que si uno se va los demás siguen. Cada compañero sale con un color distinto (rojo, verde, morado).
- **El cooperativo se configura en el launcher:** Ajustes → Mods → Cooperativo. Ahí se activa, se escriben las IP de
  los otros jugadores (sin puerto vale: se usa 27500) y se ve tu propia IP para pasarla, con la de Tailscale marcada.
- **Multiplicadores de EXP y dinero**, de x1 a x5, en la pestaña Mods. La EXP incluye la de los Digimon.
- **Activados por defecto:** viaje rápido, Digilog y EXP compartida completa (si ya tenías un `native.ini`, tus valores
  se respetan).
- **Quitados:** "Curar al subir de nivel" y "Guardar en cualquier sitio". Las partidas guardadas con este último siguen
  cargando.
- `coop.log` dice de qué jugador habla cada línea.
- Compatible con la v0.18.901 entre 2 jugadores, pero lo mejor es que todos tengáis la misma versión.

## v0.18.901
Sobre el port v0.18.9. Primera versión publicada.

- Cooperativo para 2 jugadores: ves a tu compañero en el mismo mapa, cada uno con su partida. Se configura en
  `native.ini` (`coop`, `coop_peer`).
- `coop.log` junto al juego: conexión, ping, paquetes perdidos y avisos de configuración.
- El launcher se actualiza desde este fork.
