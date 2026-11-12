# Cambios de Asuka Link (el cooperativo)

Desde la **v1.0.0**, el cooperativo tiene sus propias versiones: sube **1.1, 1.2…** con cosas nuevas y **1.1.1,
1.1.2…** con arreglos. Está basado en la v0.18.9 del port nativo de Feerdus (sus cambios, en
[CHANGELOG.md](CHANGELOG.md)), y desde la v1.0.0 se mantiene aquí por su cuenta.

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
