# Cambios de Asuka Link (el cooperativo)

Desde la **v1.0.0**, el cooperativo tiene sus propias versiones: sube **1.1, 1.2…** con cosas nuevas y **1.1.1,
1.1.2…** con arreglos. Está basado en la v0.18.9 del port nativo de Feerdus (sus cambios, en
[CHANGELOG.md](CHANGELOG.md)), y desde la v1.0.0 se mantiene aquí por su cuenta. Hasta la v0.18.918, el número iba pegado al del port: `v0.18.901` era el
port `v0.18.9` más la versión 1 del cooperativo.

## v1.2.2
Basado en el port v0.18.9. Red: versión 7, la misma que la v1.2.1.

- **Todas las misiones del Digilog tienen pasos.** Las 19 que quedaban (las 4 armas viejas, la Bruja de Hielo, los
  Caballeros de Fuego y Oscuro, los 3 Maestros, Kenny, la Bolsa de Carpetas, los 6 Black Kings y los 2 de la Base
  Submarina) dicen ahora dónde está el rival y **hasta cuándo** se puede hacer, sacado del código del juego:
  - Zanbamon solo está en la Catacumba al buscar la máscara y tras Vemmon; Pharaohmon, antes del Líder Seiryu o tras
    el final.
  - La Bruja y los Caballeros, hasta asaltar el Cuartel A.o.A; los Maestros, solo al final (tras seguir a Vemmon y
    antes del Satélite Espía).
  - Kenny, solo antes de vencer al Jefe Qing Long.
  - Cinco Black Kings, hasta cruzar el Puente Amaterasu; WarGreymon BK, después.
- **Dos misiones nuevas de cartas:**
  - **Cartas épicas:** con el Trofeo Asuka, cinco domadores dan cada uno una carta épica tras un combate y un duelo
    (Nakano, Bob, Mitch, Jessica y Mitaka).
  - **Cartas de los BK:** con el Trofeo Sol, los BK Agumon y Numemon de la Placa de Circuitos dan diez cartas raras.

  Se consiguen en cualquier orden: el paso que sale es la primera carta que te falta, y la misión está hecha al
  tenerlas todas.
- **El Digilog, en francés, italiano y alemán:** todos sus pasos y misiones, con los nombres de lugares, objetos y
  cartas de cada versión del juego (antes salían en inglés).
- **Las misiones que ya no se pueden hacer salen de "Activas":** Kenny, los Black Kings, la Bruja de Hielo, los
  Caballeros, los Maestros, Zanbamon y MasterTyranno solo están en un tramo de la historia. Pasado ese tramo, el
  Digilog deja de mostrarlas (como ya hacía el port con las de la Base Submarina), en vez de dejarlas pendientes para
  siempre.
- **El pie del Digilog ya no se monta:** en español, francés, italiano y alemán la tecla del MAPA pisaba "ACTIVAS /
  HECHAS" (venía así del port). Cuando las cuatro teclas no caben, se quita "ELEGIR" y las pestañas ocupan su sitio.
- **Por dentro, cada cosa en su sitio** (no cambia nada al jugar):
  - Los textos del Digilog tienen su propio archivo, aparte de los del cooperativo.
  - Los del cooperativo van en bloques por función (equipo, aspecto, intercambio, Guardromon, raid) y cada frase
    tiene su nombre: ya no hay números de texto escritos a mano, y añadir una frase no mueve las demás.
  - `platform/coop.c` se parte en cinco: lo básico, el aspecto, las etiquetas, los combates en lockstep y las ayudas
    de las pruebas.
  - El código del cooperativo en el mapa sale del parche de `field.c` (de 1.662 a 49 líneas) a seis archivos propios
    en `coop/src/fieldstg/`: los fantasmas, el intercambio, la ventana del Guardromon, PvP y duelo, la sala de la
    raid y el menú.

## v1.2.1
Basado en el port v0.18.9. Red: versión 7, la misma que la v1.2.0.

- **Las 5 cadenas de armas, en el Digilog** (Muramasa, Super Nova, Castigo, Eternamente e Invencible): del jefe que
  da el arma vieja a la forja final de MetalEtemon. Cada paso dice qué comprar y dónde (la Torre Delirios de
  Amaterasu, Bai Hu o Xuan Wu, Ciudad Amaterasu), a qué herrero ir y cuándo subir al compañero a nivel 45.
- **Pasos de las Botas Árbol:** la carta de Gabumon del chico de la Capa de Conexión, la taberna de Asuka y Veemon en el
  Bosque de las Patadas.
- **Correcciones con la guía de PhalX:** dónde están la Tela de Araña (los Dokugumon del Bosque Protocolo), la
  Jabalina de Bambú (se compra en Ciudad Asuka) y la Bola TNT (los Triceramon); el ID El Dorado pide 210 de carisma.
- **Aviso de última ocasión** en la historia, antes del punto sin retorno: después ya no hay armas épicas ni
  subastas.

## v1.2.0
Basado en el port v0.18.9. Red: versión 7, la misma que la v1.1.0.

- **Pasos en el Digilog (en pruebas):** la historia y las misiones ya no son una sola frase.
  - **Dónde se ven:** donde ponía "En curso." ahora pone **"Paso 2 de 5"** (en verde si ese paso ya está hecho). La
    caja de abajo dice qué toca hacer, y la foto, el lugar y el MAPA (X) son los de ese paso.
  - **□ (Cuadrado)** va pasando por todos los pasos de la misión, para ver lo que ya hiciste y lo que queda.
  - **La historia:** pasos dentro de casi todos sus tramos (el Líder Seiryu, la Tarjeta Azul, la máscara de Sepikmon,
    el Chip TNT, Ciudad Byakko, los sensores MONSTER, el Mundo Real, Amaterasu…).
  - **Las 8 misiones de DDNA:** hablar con el investigador DRI, el encargo, el combate y el DDNA.
- **Misiones nuevas en el Digilog:** la **Caña de Pescar** (Tai Kong Wang, en Shell Beach, con sus materiales como
  pasos), el **ID El Dorado** y los torneos de cartas: el **Trofeo Asuka** (Duel Island) y el **Trofeo Sol** (con el
  micrófono de Etemon).
- Los pasos salen en español; en los demás idiomas, de momento en inglés.
- Lo que falta (las cadenas de armas, el resto de misiones) está en `docs/MISIONES.md`.
- **El Digilog, completo:**
  - lista de misiones a la izquierda y detalle de la elegida a la derecha;
  - **una foto del mapa** donde ocurre cada misión;
  - el objeto que pide cada misión, marcado;
  - las teclas con sus iconos al pie;
  - textos que ya no se salen de sus paneles.

  Viene de la versión posterior del port de Feerdus, traído a mano a nuestra base.
- **Fotos de los mapas:** las hace el propio juego a partir de tu disco la primera vez que arranca, en unos segundos y
  en segundo plano, y las guarda en `data/postcards`. No se reparten, porque son imágenes del juego. Mientras no
  están, el Digilog muestra el marco vacío.

## v1.1.0
Basado en el port v0.18.9. Red: versión 7, la misma que la v1.0.0.

- **Asuka Link va por su cuenta:** ya no sigue las versiones del port de Feerdus. Se queda en la v0.18.9 y todo lo
  demás se mantiene aquí. La [hoja de ruta](docs/HOJA-DE-RUTA.md) repasa cada sistema del juego y cada mod del port,
  con su estado y las ideas para mejorarlos.
- **Actualizaciones comprobadas:** el launcher comprueba la huella (SHA-256) de cada actualización antes de
  instalarla, y no instala una descarga rota o que no es la de la versión.
- **Herramientas:**
  - El launcher ya no descarga el visor de modelos ni el editor de mapas.
  - Solo aparecen si están compilados en tu ordenador, para hacer mods.
  - El launcher solo descarga Asuka Link.
- **El avance rápido se apaga solo** al entrar en un combate entre jugadores, una raid o un duelo de cartas, y no se
  puede encender hasta que acaba: así todos van al mismo ritmo. En el mapa sigue funcionando.
- **Eliminado del todo** (antes solo estaba oculto): "Curar al subir de nivel", "Guardar en cualquier sitio" y los
  packs iniciales extra. Las partidas hechas con un pack extra siguen cargando, y las líneas que queden en un
  `native.ini` viejo se ignoran.
- **Más seguro en red:** antes de usarlos, se comprueba lo que llega de los demás.
  - Los equipos de PvP y raids: compañeros, ranuras, formas, equipo y nivel. Un equipo con un Digimon de un mod que
    tú no tienes se rechaza con un aviso, en vez de meterlo en el combate.
  - Los mensajes del duelo.
  - Las cartas de un intercambio.

## v1.0.0
Sobre el port v0.18.9. Red: versión 7.

**Primera versión con nombre y numeración propios: Asuka Link.** El cooperativo ya hace todo lo que se planteó al empezar, y deja de numerarse
con el port, que ya va por la v0.19 y se cruzaba con los números de aquí (port v0.18.10 frente a cooperativo
v0.18.910).

- **Lo que trae la 1.0:**
  - **Online:** hasta 4 jugadores en red, cada uno con su partida, viéndose en el mapa con su personaje y su nombre.
  - **En cualquier Guardromon:**
    - intercambio de cartas;
    - combates entre jugadores;
    - raids de 2 a 4 contra 29 jefes;
    - duelos de cartas con premio para los dos.
  - **En solitario:** equipo libre, creador de personaje, entrenamiento sin azar y multiplicadores de EXP y dinero.
- **Launcher nuevo:** Asuka City al atardecer, con el sol poniéndose entre los edificios y un enlace entre dos
  antenas; colores de noche y naranja; la insignia "AL" y un icono nuevo. Al pie: "Asuka Link by alosoygetsu".
- **El repositorio pasará a llamarse `asuka-link`.** Este launcher ya busca las actualizaciones con los dos nombres,
  así que seguirá encontrándolas después del cambio.
- **Qué es de quién:** el README tiene una tabla nueva que separa el juego decompilado (ReGame-Labs), el port nativo
  (Feerdus) y el cooperativo (alosoygetsu), cada uno con sus versiones.
- **Red, versión 7:** las versiones anteriores ya no se conectan con esta. Actualizad todos a la vez.
- Las notas de cada release dicen su versión, la del port en la que está hecha y la versión de red.


## v0.18.918
Sobre el port v0.18.9.

- **Los duelos de cartas dan premio a los dos:** al acabar, quien gana se lleva un sobre al azar de la serie *b*
  (Booster 1b-15b). Si su mejor Digimon del equipo es de nivel 30 o más, una de cada cuatro veces es uno *R*
  (R MBooster 01-05). Quien pierde se lleva uno de la serie *a* (Booster 01a-15a). Cada uno ve su sobre en la
  pantalla final del duelo y lo abre en la tienda de cartas, como los de los rivales del juego. Fuera del sobre, la
  partida sigue como estaba antes del duelo.

## v0.18.917
Sobre el port v0.18.9.

- **Duelos de cartas entre jugadores (en pruebas):** en cualquier Guardromon, "Duelo". Cada uno elige uno de sus mazos
  con la pantalla de siempre y jugáis una partida del juego de cartas el uno contra el otro, sin máquina de por medio:
  las cartas que sacáis a la mesa, las que jugáis (o el pasar) y lo que eligen sus efectos van por la red. Empieza
  quien saque la moneda menor, y un empate de puntos de vida da la ronda a quien empezó la partida. Es una
  exhibición: no hay premio y la partida guardada no cambia.
- Si los dos juegos llegan a ver distinta la mano o el mazo del rival, `coop.log` lo apunta ("Duel: ...") y se sigue
  con lo que diga el dueño de esas cartas.
- Raids, combates e intercambios: emparejar a dos jugadores ya no falla la mitad de las veces cuando los dos eligen
  la opción casi a la vez.

## v0.18.916
Sobre el port v0.18.9.

- **Arreglado: en las raids, un jugador que tardaba más de 8 segundos en su primera orden se daba por desconectado**,
  y a partir de ahí cada juego seguía su propio combate (en uno se ganaba en el segundo turno y en el otro no). Ahora
  solo se da por ido a quien deja de mandar datos o vuelve al mapa; pensar lo que haga falta no cuenta.
- **Raids: arranque más seguro.** La lista de jugadores de la sala ya no se recalcula en el último momento, y quien
  entra primero en el combate sigue diciendo "listo" a los demás, para que nadie empiece sin saber con quién juega.

## v0.18.915
Sobre el port v0.18.9.

- **Arreglado: el juego se cerraba al entrar a un combate entre jugadores (PvP)** en la v0.18.914. Una parte del juego
  se había quedado compilada con una versión vieja de los datos del cooperativo y leía mal el equipo del rival. La
  compilación ahora tiene en cuenta esos cambios, para que no vuelva a pasar.
- **Os veis más a menudo:** después de un tramo de la historia en el que llevas a otro personaje, el hueco de la
  memoria de vídeo donde se dibuja a los demás se quedaba ocupado ("no room in VRAM" en `coop.log`) y dejabais de
  veros. Ahora se recupera en cuanto se vuelve a un mapa normal.
- Mientras llevas a otro personaje de la historia (no al protagonista), los demás siguen sin verse en esos mapas.

## v0.18.914
Sobre el port v0.18.9.

- **El launcher, con la cara del cooperativo:** colores nuevos (noche violeta con el turquesa de un Digivice en lugar
  del azul y dorado), la insignia DW3 a juego, "port nativo + coop" bajo el título y, al pie, "Co-op fork by
  alosoygetsu".
- **Créditos:** en Herramientas → Créditos y en `CREDITS.md`, el agradecimiento a Feerdus, cuyo port nativo es la base
  de todo, y a ReGame-Labs por la decompilación, además de los créditos de siempre del port.

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
