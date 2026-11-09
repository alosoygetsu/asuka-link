# Cambios del cooperativo

Las versiones van sobre las del port original: `v0.18.901` es el port `v0.18.9` más la versión 1 del cooperativo.
Los cambios del port están en su [CHANGELOG.md](CHANGELOG.md).

## v0.18.905
Sobre el port v0.18.9.

- **Os veis como sois:** cada jugador ve a los demás con el personaje y el tono que eligieron en el creador, con
  todas sus animaciones. Quien todavía no tiene aspecto (partida de antes del creador, o antes de entrar al Mundo
  Digital) sigue saliendo como Junior con su color de jugador.
- **Nombre encima de cada jugador**, en una placa con la franja de su color.
- El paquete de red cambia (versión 3): **todos tenéis que actualizar**. Con una versión anterior, `coop.log` lo
  avisa ("has another version of the co-op").

## v0.18.904
Sobre el port v0.18.9.

- **Creador de personaje** al empezar partida, antes del nombre: eliges entre **Junior** (Junpei), **Chico** y
  **Chica**, y uno de **8 tonos** de color (pelo y ropa; la piel no cambia). La fila de tonos muestra a tu personaje
  en cada color.
  - Se aplica **después de la cinemática de entrada al Mundo Digital**; antes sigues siendo Junior.
  - Sale también en las escenas de historia.
  - Chico y Chica tienen todas las animaciones de Junior: las que no tenían (correr, trepar, caer, gestos de las
    escenas…) se generan con sus propios dibujos y el ritmo de las de Junior.
  - Se guarda con tu partida, en bytes que el juego no usa: la memory card sigue siendo normal.
  - Los retratos del registro (nombre, equipo y confirmación) muestran a tu personaje en vez de Junior.
  - En el cooperativo, los demás todavía te ven como Junior con tu color de jugador.
- **`coop.log` más claro:** dice "you see player N on map X" la primera vez que ves a cada jugador en un mapa, y
  avisa si en un mapa no se le puede mostrar (sin sitio en VRAM, 15 personajes, o eres otro personaje en ese mapa).

## v0.18.903
Sobre el port v0.18.9.

- **Equipo libre al empezar partida:** en vez de elegir un pack, eliges a tus 3 compañeros entre los 8 Digimon, sin
  repetir. La pantalla muestra tu nombre, los 3 elegidos sobre sus plataformas con su nombre, el botón Aceptar, los 8
  en fila (los elegidos, oscurecidos) y la descripción del que tengas seleccionado.
  - Izquierda/derecha recorren la fila; arriba sube a los elegidos y a Aceptar. El botón de confirmar elige o quita;
    Aceptar (o START) sigue con 3.
  - La escena de bienvenida del vestíbulo aparece con tus 3 compañeros, sean los que sean.
  - Si eliges justo los de uno de los packs del juego, se usa la escena original de ese pack.
- **Quitada la opción de packs extra:** ya no hace falta.

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
