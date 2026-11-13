# Cambios de Asuka Link (el cooperativo)

Desde la **v1.0.0**, el cooperativo tiene sus propias versiones: sube **1.1, 1.2…** con cosas nuevas y **1.1.1,
1.1.2…** con arreglos. Está basado en la v0.18.9 del port nativo de Feerdus (sus cambios, en
[CHANGELOG.md](CHANGELOG.md)), y desde la v1.0.0 se mantiene aquí por su cuenta.

## v1.4.0
Basado en el port v0.18.9, con cosas de sus versiones hasta la v0.19.7 traídas a mano. Red: versión 8, como la
1.3.x (se juntan sin problema).
- **Viaje rápido al momento.** En el mapa del menú, "Sí" te lleva enseguida: la pantalla se funde y apareces en esa
  zona, sin tener que cerrar el menú. Los demás jugadores te ven llegar como si hubieras cruzado una puerta.
- **El viaje rápido solo funciona donde podrías salir andando.** En una sala de la que solo se sale con un evento o en
  un sitio que la historia tiene cerrado, X en el mapa suena a cancelar y no pregunta. Antes se podía viajar desde ahí
  y saltarse partes de la historia.
- **Emblemas de Búsqueda y de Contraataque:** ahora también funcionan junto a un arma que envenena, paraliza, confunde,
  noquea, absorbe o da críticos; el juego los apagaba. El de Búsqueda no suma con un arma de críticos, porque dan lo
  mismo. El Emblema Triple sigue igual: un golpe triple no aplica el efecto del arma. Es igual para todos, también en
  los combates entre jugadores y las raids.
- **La tienda dice "Comprar" y "Vender"** al confirmar, en español; el disco trae "Buy" y "Sell" en inglés.
- **Digilog, el Digihuevo de la Sinceridad:** el paso ya no destripa al Jefe Qing Long antes de tiempo. Se llama
  "Huevo Sinceridad", empieza preguntando a Nami en la Bahía Fénix y, con el muro del Pantano Bios abierto, te manda a
  buscar el Digihuevo.

## v1.3.2
Basado en el port v0.18.9. Red: versión 8, como la 1.3.x (se juntan sin problema).
- **Las misiones de DDNA ya no se dan por hechas antes de tiempo.** Contaban como hechas al conseguir la DDNA del
  jefe, pero el compañero no se une hasta que se la llevas al investigador DRI. Ahora están hechas cuando el DRI te lo
  da, y tienen ese último paso.
- **Los DRI te piden su DDNA aunque ya tengas a ese compañero.** Antes solo se lo pedían a quien no lo tenía, y el
  jefe de la misión no aparecía nunca. Si ya lo tienes, la misión se hace al conseguir la DDNA.
- **Cada DRI dice qué nivel de equipo pide.** Solo te hacen el encargo con una suma de niveles del equipo concreta
  (Johann y Jimmy de 30 a 59; Kazuki y Michael de 45 a 74; Ken, Yuji y Yang de 60 a 89; Bob, menos de 30), y ese
  paso del Digilog ahora lo dice, con dónde está cada DRI.
- **Una misión sale en el Digilog en cuanto empiezas su primer paso** (por ejemplo, al hablar con un DRI), no solo al
  pisar el mapa de su jefe.
- **La búsqueda de Etemon** (historia, la Máscara Sepik) enseña todos sus pasos: el Sepikmon falso del Puente de
  Asuka, Etemon diciendo que se fue al Lago Divermon, Nick en el lago y la vuelta con Etemon.
- **Trofeo Asuka:** los Divermon de la Isla del Duelo iban al revés en el Digilog. Ahora el primero es el del pie de
  la escalera, y el paso avanza al ganarle.
- **Más pasos con su mapa:** la bomba de Sepikmon, las Bolas TNT, la Bolsa de Carpetas, la Tela de Araña, la
  Jabalina, la carta de Gabumon, la taberna, Seehomon y Depthmon, y las compras de las cadenas de armas (Torre
  Delirios, Xuan Wu, Ciudad Amaterasu).
- **El Cañón Láser** (cadena de Monmon) se compra en Bai Hu o Xuan Wu, y su paso lo dice.
- **Las ventanas del Guardromon son las del juego, en lila:** su menú, la lista de raids, los avisos de espera
  ("Esperando a otro jugador...", la sala de la raid, "Preparando el combate...") y el intercambio de cartas usan
  las ventanas, letra, cursor y sonidos de los menús de los personajes (como el guía de Ciudad Asuka), con el
  fondo lila de Asuka Link para distinguirlas. En las listas el cursor se para en los extremos y △ cierra o vuelve
  atrás; en la de raids, el jefe elegido sale abajo con su nivel. En el intercambio, tus cartas sueltas van a la
  derecha (con lo que ofreces de cada una), la oferta del otro a la izquierda y los botones abajo.
- **"Intercambiar cartas" y "Duelo" salen en el Guardromon cuando ya tienes la Bolsa de Carpetas**, igual que las
  cartas en el menú del juego. Antes salían desde el principio.
- **Equipo libre:** los nombres de los Digimon elegidos salían un poco bajos en su barra; ahora van centrados.
- **Ken, Yuji y Yang:** Ken está en la primera sala del Desierto Mobius y Yuji y Yang más adentro, y sus pasos lo
  dicen.

## v1.3.1
Basado en el port v0.18.9. Red: versión 8, como la 1.3.0 (se juntan sin problema).
- **Cuadros negros en el mapa cuando llega otro jugador.** En los mapas grandes (por ejemplo las casas de arena de
  0x23F) aparecían rectángulos negros de 64×64 sobre el fondo, a veces con un muñeco dentro. Era el otro jugador: sus
  fotogramas se guardaban en la parte de la memoria de vídeo donde se cargan las baldosas del fondo a medida que te
  mueves, y cuando llegaba una baldosa a ese hueco se mezclaban. Ahora van a una zona que el mapa no usa, y el otro
  jugador se sigue viendo.

## v1.3.0
Basado en el port v0.18.9. **Red: versión 8**: no se conecta con las 1.2.x. Actualizad todos.

La versión base de Asuka Link: a partir de aquí, el punto de partida del proyecto.
- **La sala**, en la pantalla principal del launcher, arriba a la derecha. Sustituye a las IP de Ajustes.
  - **Los de tu red salen solos.** Cada launcher se anuncia en la red local, así que en la misma casa no hace falta
    Tailscale ni escribir IP. Eliges a uno y **Invitar**; el otro ve "X te invita" y acepta o rechaza.
  - **Añadir por IP**, para jugar desde lejos (Tailscale, ZeroTier, Internet): escribes la IP del otro y entra
    directamente en tu sala, sin que tenga que escribir la tuya.
  - **Tus IP**, siempre a la vista en un recuadro del menú lateral (la de Tailscale, en azul), para pasárselas a quien
    te quiera añadir.
  - Hasta 4 por sala. Si en una red sois más, cada grupo invita a los suyos y no se mezclan.
  - Cada jugador sale con su **nombre de domador**, su **personaje** (Junior, Chico o Chica, en su tono) y sus **barras
    de señal**. Al seleccionarlo se ven su equipo con niveles y su ping.
  - **Probar** mide la conexión. Si no llega nada, una segunda pulsación abre el cortafuegos de Windows para el juego
    y el launcher (pide permiso de administrador).
  - **Listo:** cuando lo estáis todos, os avisa (también con el launcher minimizado).
  - **Recientes y reconexión:** los que ya jugaron contigo salen para invitarlos aunque estén lejos, y al abrir el
    launcher se rehace la sala de la última vez.
  - **El juego sigue la sala:** si alguien entra o sale mientras jugáis, aparece o se va sin reiniciar.
  - `sala.log`, junto al launcher, dice qué invitaciones y cambios de sala hubo.
- **Launcher nuevo, con aspecto propio** (ya no se parece al del port):
  - menú lateral (Jugar, Ajustes, Partidas, Herramientas, Salir) en todas las pantallas;
  - Asuka City en movimiento: estrellas, antenas, ondas y un paquete por cada jugador de tu sala viajando por el enlace;
  - colores propios (azul tinta, cian y amarillo), una letra nueva (Silkscreen, de licencia libre) y un icono nuevo:
    "AL" con ondas de señal, también en los `.exe`.
  - el primer arranque, también nuevo: la bienvenida, los tres pasos (leer el disco, formar la sala, jugar) y un
    panel para soltar el disco, que gira mientras se lee.
- **Opciones dentro del juego, en el menú de pausa:** OPCIONES, antes de SALIR (Start en el mapa). Select/Back y Esc ya
  no las abren. Back es ahora el Select del juego.
- **Menos opciones, los mods fijos:**
  - Se pueden cambiar pantalla completa, ventana, resolución, pantalla ancha en combates, iconos, vibración, botones
    e idioma. EXP y dinero, en el launcher (Ajustes → Partida).
  - Lo demás va fijo: viaje rápido, Digilog, texto rápido, EXP compartida completa, entrenamiento sin azar,
    BladeGarurumon y los efectos, sonidos y scripts propios, activados; el objetivo en el mapa, apagado; y los
    gráficos con sus mejores valores (60 fps…).
  - Las líneas de esas opciones en un `native.ini` viejo se ignoran.
- **Una sola descarga:** `asuka-link-<versión>.zip`.

## v1.2.3
Basado en el port v0.18.9. Red: versión 7, la misma que la v1.2.2.

Versión de paso hacia la 1.3.
- **Una sola descarga:** cada versión trae solo `asuka-link-<versión>.zip`, que sirve para instalar y para que el
  launcher actualice (comprobando su `.sha256`). Ya no lleva `native.ini`: al actualizar se queda el tuyo, y en una
  instalación nueva el juego usa los valores por defecto.
- **El cooperativo viene activado por defecto.** Sin direcciones de otros jugadores no abre ninguna conexión, así que
  jugar solo sigue igual.
- Esta es la última versión que sube también `dw2003-native-<versión>.zip`, para que los launchers de antes puedan
  actualizarse a esta. Desde la 1.3, solo el de Asuka Link.

## v1.2.2
Basado en el port v0.18.9. Red: versión 7, la misma que la v1.2.1.

- **Arreglado: "Intercambio cancelado" al empezar un combate PvP o una raid.** La comprobación del equipo que llega
  por la red (v1.1.0) tomaba los huecos de combate de cada compañero por posiciones de su lista, cuando el juego guarda
  ahí el número de la forma; cualquier compañero con una digievolución avanzada en esos huecos se rechazaba ("a slot
  out of range" en coop.log). No tenía que ver con la conexión.

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

**Primera versión con nombre y numeración propios: Asuka Link.** El cooperativo ya hace todo lo que se planteó al empezar, y tiene a partir de aquí
su propia numeración, aparte de la del port.

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
