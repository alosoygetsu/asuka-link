# Cambios del cooperativo

Las versiones van sobre las del port original: `v0.18.901` es el port `v0.18.9` más la versión 1 del cooperativo.
Los cambios del port están en su [CHANGELOG.md](CHANGELOG.md).

## v0.18.913
Sobre el port v0.18.9.

- **Raids: se ve a quién se espera.** Cuando le toca a otro jugador, su Digimon no entra hasta que llega su orden, y
  mientras tanto sale "Esperando a <nombre>...". Si no responde en 8 segundos se le da por desconectado: sale
  "<nombre> se ha desconectado.", su equipo queda fuera y la raid sigue con los demás.

## v0.18.912
Sobre el port v0.18.9.

- **Raids de hasta 4 jugadores.** Los que eligen el mismo jefe en cualquier Guardromon se juntan en una sala: en cuanto
  hay dos, empieza una cuenta atrás de 10 segundos (sale en pantalla: "2 de 4  9") para que entre quien quiera, y con
  4 se empieza ya. En el combate, cada jugador hace su turno por orden y después el jefe ataca una vez a cada uno; su
  vida se multiplica por el número de jugadores.
- **Raids: 29 jefes para elegir.** Al escoger "Raid" en el Guardromon sale la lista de jefes. Están los jefes de la historia y sus revanchas del final, de Pharaohmon (Nv 12) a Knightmon
  (Nv 99). Cada uno ve los que le quedan hasta 5 niveles por encima de su mejor compañero; Kuwagamon, siempre.
- **Recompensas de raid:** al ganar, la EXP y el dinero del combate se multiplican por 3 (además del multiplicador de
  Mods), y cada jugador se lleva un objeto seguro según el nivel del jefe: potenciadores 1 y Cargas en los jefes de
  nivel bajo; potenciadores 2, chips de stats y equipo en los medios; potenciadores S, chips de elemento, chips de
  entrenamiento, Danza del Ala, Brazal. Poder o Cable Carmesí en los altos.
- **Raids, arreglos al cambiar de equipo:** arriba sale siempre el nombre y la vida del Digimon que actúa (antes podía
  quedarse el del otro jugador), los estados y mejoras de cada equipo se quedan en su equipo, y la EXP de cada uno
  solo cuenta a sus propios Digimon.

## v0.18.911
Sobre el port v0.18.9.

- **Raids (primera versión, para 2 jugadores):** en cualquier Guardromon hay una cuarta opción, "Raid". Si dos
  jugadores la eligen, entran juntos en un mismo combate contra un jefe, con sus dos equipos en el mismo lado.
  - **Turnos por orden:** jugador 1, jugador 2 y después el jefe ataca una vez a cada uno. En cada turno sale en
    pantalla el Digimon de quien actúa; los turnos del otro los decide el otro jugador.
  - El jefe tiene su vida multiplicada por el número de jugadores.
  - Si un jugador se queda sin Digimon, el otro sigue solo. Si caéis los dos, volvéis al mapa sin game over y sin
    cambios en la partida. Si ganáis, cada uno recibe la EXP y el dinero del combate.
  - De momento hay una raid básica, abierta para todos: Kuwagamon. No se pueden usar objetos ni huir.
- **El menú del Guardromon tiene "Salir"**, que lo cierra sin hablar con él.

## v0.18.910
Sobre el port v0.18.9.

- **Entrenamiento sin azar** (nuevo en Mods, activado por defecto): en los gimnasios todos los intentos salen bien,
  también el intento extra, cada stat sube lo máximo que da esa intensidad y ningún otro stat baja. Por ejemplo,
  Puñetazo a TP10 da siempre +68 de Fuerza (3 x 16 y el extra, 20). Se puede desactivar en el launcher o en el
  menú del juego (Esc → Mods → "Training without luck").

## v0.18.909
Sobre el port v0.18.9.

- **Os volvéis a ver al cambiar de mapa.** Cada mapa gastaba un hueco de la memoria de vídeo para dibujar a los demás
  y no lo devolvía; tras unos cuantos cambios de mapa ya no quedaba sitio ("has no room in VRAM" en `coop.log`) y
  dejabais de veros. Ahora el hueco del mapa anterior se reutiliza.
- **Combates entre jugadores:** al cambiar de Digimon, el que entra lo hace en la forma elegida también en la pantalla
  del otro (antes conservaba la de antes, el daño salía distinto en cada juego y el combate se podía quedar atascado).
- **Combates entre jugadores:** las técnicas del rival se ven en su sitio. Sus animaciones están hechas para el lado del
  jugador, así que salía colocado encima de tu Digimon y con la cámara mirando a otra parte; ahora se le dan la vuelta
  (posiciones, efectos y cámaras).
- `coop.log` apunta cada orden del combate y cualquier diferencia de vida entre los dos juegos.

## v0.18.908
Sobre el port v0.18.9.

- **Combates entre jugadores en los Guardromon.** Al hablar con un Guardromon sale "Hablar / Intercambiar cartas /
  Combatir". Si dos jugadores eligen Combatir (cualquier Guardromon, cualquier mapa), entran a la vez en un combate en
  la arena, cada uno con sus 3 Digimon contra los del otro.
  - **Turnos alternos:** empieza el Digimon más rápido y después un turno cada uno. Los turnos del rival los decide el
    otro jugador.
  - Se puede **atacar, usar técnicas, digievolucionar, cambiar de Digimon y rendirse** (Huir). Los objetos no se
    pueden usar.
  - Cada uno ve la animación de digievolución de su propio Digimon; el del rival entra con su nueva forma, como al
    cambiar. Arriba sale el nombre de cada compañero, sea cual sea su forma.
  - El primer Digimon empieza con su digievolución de combate (la del laboratorio), igual en los dos lados.
  - **Exhibición:** se empieza con la vida y los MP llenos y, ganes, pierdas o te rindas, vuelves al mapa con la partida
    exactamente como estaba. Sin EXP, sin dinero y **sin game over**.
  - Si el otro se rinde o se desconecta, ganas y vuelves al mapa.
- El paquete de red cambia (versión 6): **todos tenéis que actualizar**.

## v0.18.907
Sobre el port v0.18.9.

- **Intercambio de cartas:** al terminar (o cancelar) ya no salta el diálogo de guardar del Guardromon; el intercambio
  termina ahí.

## v0.18.906
Sobre el port v0.18.9.

- **Intercambio de cartas en los Guardromon.** Al hablar con un Guardromon (el de guardar) sale primero
  "Hablar / Intercambiar cartas"; Hablar sigue con lo de siempre.
  - Quien elige intercambiar espera; en cuanto otro jugador hace lo mismo en cualquier Guardromon, a los dos se les
    abre la ventana de intercambio. No hace falta estar en el mismo mapa.
  - Solo se ofrecen **cartas sueltas**: las copias que no usa ningún mazo (si un mazo usa una carta 2 veces, esas 2 se
    quedan). Hasta 5 cartas distintas por oferta, y nada que al otro no le quepa (máximo 9 copias).
  - Arriba/abajo eligen carta, izquierda/derecha cuántas copias. Las dos ofertas se ven en directo; primero los dos
    marcan **Listo** y después **Confirmar**. Si alguien cambia algo, el otro vuelve a mirar.
  - Se aplica a la vez en las dos partidas y una sola vez; si alguien se desconecta antes, se cancela. Se guarda al
    guardar la partida, como siempre.
  - Con la letra del juego, en el idioma de cada uno.
- El paquete de red cambia (versión 4): **todos tenéis que actualizar**.

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
