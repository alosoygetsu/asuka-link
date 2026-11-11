# Investigación: lo que viene después de verse en el mapa

Estudio sobre la decompilación (`dw3_decomp`) y el port, a 8 de noviembre de 2026. Aquí no hay nada programado: es
para decidir el orden. "Coste" es una estimación: **poco** (días), **medio** (una o dos semanas), **mucho** (varias
semanas, con riesgo de atascarse).

## Base común: lo que hace falta para todo lo demás
Lo de hoy (las posiciones) puede perder paquetes sin problema. Lo que viene no: un intercambio o un turno de combate
**no se puede perder ni duplicar**.

- **Mensajes fiables** encima del UDP actual: número de mensaje, confirmación y reenvío, por compañero. Va en
  `platform/coop_net.c`, junto a lo que ya hay. **Coste: poco.**
- **Identidad de cada jugador:** un nombre (el que el jugador le puso al protagonista, que guarda la partida; queda
  por ver el campo exacto de `GAME`) y un id fijo. Así se pueden mostrar nombres encima (el hito 4) y saber con quién se intercambia.
  **Coste: poco.**
- **Proponer y aceptar:** una forma de que un jugador proponga algo (intercambio, duelo, combate) y el otro acepte o
  rechace, con una ventana del port sobre el juego. Ya existe un menú propio del port (`platform/menu.c`, dibujado
  encima del juego). **Coste: medio.**

## 1. Intercambios (Digimon, objetos, cartas)
**Qué guarda el juego:**
- **Objetos:** `GAME.items[0x193]`, cantidades hasta 99.
- **Cartas:** `GAME.cards[0x13D]`, cantidades hasta el máximo de copias.
- **Dinero:** `GAME.money`.
- **Digimon:** `GAME.partners[8]`, un `Partner` por cada uno de los 8 Digimon compañeros.
  - Cada uno tiene `unlocked` y un `PartnerStats` (nombre, EXP, 19 estadísticas, estados, equipo y las entradas de
    sus digievoluciones).
  - Ocupa unos 0x3D0 bytes, todo en la partida, así que se puede copiar entero.

**Cómo sería:**
- **Objetos y cartas:** fácil. Uno resta y otro suma, con confirmación de los dos: el que da no lo pierde hasta que el
  otro confirma que lo recibió. **Coste: poco**, después de la base común.
- **Digimon:** se intercambia el `Partner` entero.
  - Hay que decidir qué pasa con el equipo puesto (¿viaja con él?) y si el Digimon recibido sustituye a uno tuyo del
    mismo tipo (los 8 son fijos: Agumon, Kotemon... cada partida tiene sus 8).
  - Lo más simple es intercambiar **el mismo compañero**: tu Agumon por su Agumon. Uno distinto rompería la lógica de
    "tus 8 compañeros".
  - **BladeGarurumon y los Digimon de mods** guardan datos fuera del formato normal, en el espacio libre de la memory
    card. Solo se podrían intercambiar si los dos tienen el mismo mod.
  - **Coste: medio.**
- **Dónde hacerlo:** desde el menú del port, estando en el mismo mapa que el otro. El cambio se guarda cuando cada uno
  guarde su partida (como cualquier otra cosa del juego).

## 2. Duelos de cartas entre jugadores
**Qué hay:** el juego de cartas (`include/cardgame/battle.h`, `src/cardgame/`) enfrenta al jugador (lado 0) contra la
máquina (lado 1).
- La máquina decide con funciones concretas: `CARDGAME_pickComputerCard`, `CARDGAME_pickComputerTarget` y
  `CARDGAME_checkComputerCondition`, más su plan de mazo (`opponentPlans`, `opponentDraws`).
- El rival sale del archivo de rivales (`CardOpponent`: un mazo de 40 cartas y un nivel).

**Cómo sería:**
- Cada jugador ve la partida desde su lado. Lo que haría la máquina, en mi juego lo decide el otro jugador.
- En vez de llamar a `pickComputerCard`, se espera a la jugada que llega por red, y mi jugada se manda al otro.
- El mazo del rival es **el mazo real del otro jugador** (`GAME.decks`, que viaja al empezar), no uno del archivo.
- **Lo delicado es el azar:** barajar (`shufflePile`) y robar tienen que dar lo mismo en los dos juegos. Se resuelve
  compartiendo una semilla al empezar y usando nuestro propio generador para el duelo, en vez del del juego.
- **Premio:** ninguno, o una carta acordada.
- **Coste: medio-mucho.** El juego de cartas está entero decompilado, que es lo que lo hace viable. Es probablemente
  lo más asequible de los combates.

## 3. Combates entre jugadores (PvP)
**Qué hay:**
- El combate (`FIGHTSTG`/`WFIGHTMN`) es de 2 lados con 3 luchadores cada uno (`FIGHTSTG_battle.fighters[2][3]`), con
  un activo por lado.
- Los enemigos salen de `BATTLE_SETUP.enemies[3]` (luchador, nivel, HP, MP, fuerza).
- La IA enemiga está en `fightstg/enemy_turn.c`.
- Hay tiradas de azar en daño, críticos, estados y parálisis (`rolls.c`).

**Cómo sería:**
- El lado enemigo se rellena con **los Digimon del otro jugador**: sus estadísticas y técnicas reales, no un enemigo
  del juego. El juego espera enemigos de su tabla, así que hay que montar un "enemigo" con los datos de un compañero.
  Esto es lo más incierto, a comprobar.
- **Los turnos:** en lugar de `enemy_turn.c`, la acción llega por red. Los dos juegos simulan el mismo combate a la vez
  (lockstep), con una semilla compartida para que las tiradas coincidan.
- **Riesgo:** cualquier diferencia (un número aleatorio de más, un orden distinto) hace que cada uno vea un combate
  diferente. Hay que detectarlo, por ejemplo comparando la vida de los dos lados tras cada turno, y cortar el combate
  con un aviso.
- **Coste: mucho.**

## 4. Combates cooperativos y raids contra jefes
El combate del juego admite **3 Digimon por lado y uno activo a la vez**, así que meter a 4 jugadores con sus equipos en
la misma pantalla no encaja con cómo está hecho.

**Propuesta realista: el jefe compartido.**
- Cada jugador lucha **en su propio juego** contra el mismo jefe, que tiene mucha más vida (×2, ×3 o ×4 según cuántos
  seáis).
- El daño que hace cada uno se manda a los demás y se resta en todos los juegos, así que la barra de vida del jefe
  baja con lo que hacéis todos.
- Cuando el jefe llega a 0 en cualquier juego, termina en todos y cada uno recibe su recompensa (aquí entran los
  multiplicadores).
- Si alguien pierde o se desconecta, los demás siguen.
- **Lo que hace falta:**
  - un gancho donde el juego resta vida al enemigo, para mandarla y para aplicar la que llega;
  - una forma de empezar el combate a la vez: un jugador lo propone, los demás aceptan y todos entran con el mismo
    `BATTLE_SETUP`;
  - jefes elegidos (los de las misiones, o una lista propia).
- **No hace falta lockstep:** cada juego tiene su propio combate, y solo se comparte la vida del jefe. Es mucho más
  robusto que el PvP.
- **Coste: medio-mucho.** Recomendado antes que el PvP.

**Extra visual:** dibujar en el combate a los compañeros de los otros (pequeños, o en la barra del jefe con sus
nombres). **Coste: medio.**

## 5. Creador de personaje
Todos los jugadores son Junpei: el protagonista es siempre el personaje 2 (`FIELD_CHARACTER_PLAYER`), y sus
animaciones van por archivos según el conjunto (`actor_control.c`, `animateActor`). El juego tiene otros "personajes
del jugador" para tramos de la historia (1, 0x6A, 0x146 y 0x147, en `field_map.h`), además de los NPC.

- **Ligero: la paleta.** Elegir los colores del pelo, la ropa, etc. de Junpei (sus 16 colores) en el launcher, y
  mandarlos a los demás al conectar. Usa lo mismo que los colores por jugador que acabamos de hacer
  (`native_coop_vram_tint`, que se vuelve a aplicar en cada fotograma). **Coste: poco.** Es lo recomendado para
  empezar.
- **Medio: ser otro personaje del juego.** Usar como avatar los sprites de otro personaje: uno de los alternativos o
  un NPC.
  - Hay que comprobar si tienen andar en las 8 direcciones y las mismas animaciones (los NPC suelen tener menos).
  - Al otro se le dibuja con ese personaje; tu propio protagonista no cambia.
  - **Coste: medio.**
- **Grande: sprites propios.** Hojas de sprites dibujadas por los jugadores (PNG), cargadas como los efectos de los mods
  (`data/mods/`). Funciona igual en lo técnico, pero alguien tiene que dibujar todas las animaciones.
  **Coste: medio en código, mucho en dibujo.**

## Orden recomendado
1. **Base común + hito 4:** mensajes fiables, nombres encima de cada jugador, chat y avisos. Todo lo demás se apoya en
   esto.
2. **Creador ligero (paleta):** rápido y hace que cada uno sea "suyo".
3. **Intercambio de objetos y cartas**, y después **de Digimon**.
4. **Duelos de cartas:** el combate más asequible, y prueba la sincronización con azar compartido.
5. **Raids con jefe compartido.**
6. **PvP de combate:** lo más arriesgado; mejor al final, con lo aprendido en las cartas.
7. **Avatares de otros personajes y sprites propios**, cuando apetezca.

## Para comprobar antes de cada fase
- **Intercambio de Digimon:** si `Partner.unlocked` y `battleDigivolve` necesitan algo más al recibir uno (por ejemplo,
  digievoluciones que dependen del progreso de la historia).
- **Cartas:** que el juego de cartas no lea el temporizador ni el pad para el azar (`rand` está en sus archivos).
- **Raids:** la función exacta donde baja el HP del enemigo y si los jefes de historia permiten volver a lucharlos.
- **PvP:** si se puede montar un enemigo con un `fighter` de compañero (los compañeros y los enemigos comparten
  modelos, pero quizá no tablas).
- **Avatares:** qué animaciones tiene cada personaje alternativo.
