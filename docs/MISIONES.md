# Misiones del Digilog: estudio

Lo que dicen los datos del juego (las conversaciones de cada mapa, `tools/quest_talks.py`) sobre cada misión
secundaria y sus pasos. Cada paso es una condición que el juego ya sabe comprobar (una marca, un objeto, una condición
especial), así que el Digilog solo tiene que preguntarla, como hacen las conversaciones. Nomenclatura: `F1a:36` es la
marca 0x36 del grupo 0x1A; `+X` tener el objeto X; `esp58` la condición especial 58.

Candidatos completos (los 77 objetos que da un personaje): [misiones-candidatas.md](misiones-candidatas.md).

## Descartes
- **Cofres** (unos 50): la marca del grupo 0x02 y nada más. No son misiones, salvo los que guardan un objeto que pide
  una misión (abajo, en sus pasos).
- **Partes de la historia principal:** la Máscara Sepik (historia en el paso 9), el Chip TNT (paso 14) y la Tarjeta
  Azul (paso 6) solo se pueden hacer en ese paso de la historia, que el Digilog ya muestra.
- **El mapa de pruebas** (`wstag586`) y los duplicados del capítulo extra (`wstag9xx`).

## Encargos nuevos

### Caña de Pescar (Shell Beach, 0x21F; Tai Kong Wang; historia 0x04-0x26)
1. Hablar con Tai Kong Wang: `F1a:36`.
2. Tener la Bolsa de Carpetas: `+0x192`.
3. Que te pida los materiales: `F1a:21`. Esto abre lo del Pez Rojo.
4. Conseguir el **Pez Rojo**: en el Lago de Divermon (0x227), Divermon tiene algo clavado en la garganta; se lo
   quitas y es un Pez Rojo (`F1a:2c`, `+0x007`).
5. Conseguir la **Tela de Araña** (`+0x059`) y la **Jabalina de Bambú** (`+0x0A7`): por localizar (cofres).
6. Llevárselo todo (`esp58`: los tres objetos): te da la Caña de Pescar (`+0x005`). Hecha.

### ID El Dorado (El Dorado, 0x213; Ponchomon)
1. Hablar con Ponchomon: `F1a:05`.
2. Un equipo fuerte: `PARTY_STAT(2)` (por precisar el umbral).
3. Te da el ID El Dorado (`+0x008`). Hecha. Abre el club de Mike el Rapero.

### Trofeo Asuka (Duel Island, 0x228; torneo de cartas)
1. Ganar a los 5 Divermon de la isla en duelo de cartas, que cortan las escaleras de su torre: de abajo arriba,
   `F1a:37`, `F1a:38`, `F1a:39`, `F1a:3a` y `F1a:3b` (por sus sitios en `wstag385`; el orden es obligado). Los 4 Etemon
   del Trofeo Sol (`wstag386`), igual: `F1a:3c` a `F1a:3f` y MetalEtemon `F1a:40`.
2. Ganar a KingDivermon: Trofeo Asuka (`+0x012`, `F1c:53`). Hecha. Al ganar, las 5 marcas vuelven a 0.

### Trofeo Sol (0x297; torneo de cartas)
1. Ganar a los 4 Etemon (`F1a:3c`-`F1a:3f`) y a MetalEtemon (`F1a:40`). Uno de los Etemon (`F1a:3f`) no juega sin su
   micrófono: te lo pide (`F1c:49`), y el **Micro de Etemon** (`+0x?`) lo tiene el líder BKZ en el Tablero de Circuitos.
2. Ganar a KingEtemon: Trofeo Sol (`+0x014`, `F1c:54`). Hecha.

## Cadenas de armas (los herreros)
Cada arma antigua u oxidada sigue con tres mejoras. Cada herrero pide **el compañero de esa arma en el equipo, a nivel
45**, el arma anterior y un objeto más (que se consume). Las últimas las forja MetalEtemon en Kulon Weapons (0x269),
en este orden, porque cada una pide las anteriores: Invencible, Eternamente, Super Nova, Castigo y Muramasa.

| Cadena (compañero) | 1.ª mejora | 2.ª mejora | 3.ª (MetalEtemon) |
|---|---|---|---|
| Katana (Kotemon) | Katana Oxidada + Onimaru → **Kotetsu** (Kabukimon, 0x2A9) | + Tenjinmaru → **Masamune** (Yashamon, 0x2BF) | + Acero Fuujin → **Muramasa** (pide las otras 4 finales) |
| Guante (Kumamon) | Guante Oxidado + Puño Kaiser → **Zodiaco** (Nohemon, 0x2B0) | + Brazo Tempest → **Prominencia** (Seehomon, 0x2E4) | + Puño Raijin → **Super Nova** (pide Invencible y Eternamente) |
| Rifle (Monmon) | Rifle Oxidado + Cañón Racimo → **Justicia** (Shurimon, 0x2AE) | + Cañón Láser → **Juicio** (Sagitarimon, 0x2C1) | + Cañón Suijin → **Castigo** (pide Invencible, Eternamente y Super Nova) |
| Varita (Patamon) | Varita Vieja + Khakkhara → **Brillo** (Ponchomon, 0x295) | + Vara Cristal → **Inmaculado** (Kongoumon, 0x2CD) | + Milenio → **Eternamente** (pide Invencible) |
| Garra (Agumon o Guilmon) | Garra Antigua + Colmillo Sable → **Vencer** (Shadramon, 0x2BE; o Shadromon, 0x28E) | + Colmillo Veneno → **Destrucción** (Depthmon, 0x2E4) | + Colmillo Oculto → **Invencible** |

- **Con las 5 finales** (`esp150`): los líderes Zhu Que y Bai Hu aceptan un combate (`F10:0e`, `F10:0f`). Sería la
  misión final de la cadena: "Las armas legendarias".
- **Objetos que piden y dónde están** (por completar; los cofres de la lista de candidatos): Tenjinmaru y Colmillo
  Veneno en Magasta B1F (base submarina)…
- **Rifle Oxidado:** no lo da ningún personaje (salvo el mapa de pruebas). Las otras cuatro armas viejas las dan jefes
  (Zanbamon, Datamon, MasterTyranno, Pharaohmon); el rifle será de otro combate. Por localizar.

### Otros encargos vistos
- **Micro de Etemon** (para el Trofeo Sol): un Black King Agumon en el Tablero de Circuitos (0x2EA) te dice que lo
  tiene su líder (duelo, `F1c:3f`); el líder BKZ te lo da si le ganas a las cartas (`F1c:40`).
- **Cartas de los Black Kings:** con el Trofeo Sol, varios BK Agumon y Numemon del Tablero de Circuitos dan cartas
  raras si les ganas (Puerta de Fortuna, Acceso Deseado, Borrar Matriz, Rosemon II, Demon…). Podrían ser una misión de
  cartas.
- **Chips de las salas de conductos** (0x24E-0x251): HP, MP, Potencia y Mente salen al vencer a cada maestro
  (`F1c:20`, `F1c:23`, `F1c:24`, `F1c:22`). MP y Mente ya están (Maestro de Hielo y de Fuego); HP y Potencia serían
  otros dos.

## Pasos de las misiones que ya están
### DDNA (8): todas siguen el mismo patrón
1. Hablar con el investigador DRI (marca propia).
2. Te pide encontrar al Digimon (`F06:0n`) si la suma de niveles del equipo está en su tramo. En el juego original,
   además, solo si **no tienes** a ese compañero; desde la v1.3.2 lo pide siempre (`coop/patches/src/stages/`: se
   quita «le falta X» de sus charlas), porque vencer al jefe abre su raid.
3. Aparece en su mapa (el de la misión): vencerlo (marca `F1c:1x` y combate).
4. Coger su DDNA (`+objeto`).
5. Llevárselo al DRI: el compañero se une (`desbloquea`). Hecha: en el Digilog, tener la DDNA **y** al compañero
   (`JOURNAL_PARTNER_DDNA`; la DDNA no se gasta). Antes contaba como hecha al coger la DDNA (`F40:24`…).
- **Cuándo te hace el encargo cada DRI** (sus charlas; `PROGRESS(n), 0` es «historia ≠ n»):
  Johann (Valle Tyranno) y Jimmy (Ciudad Seiryu), historia 0x05-0x26, equipo 30-59; Kazuki y Michael (Central Park),
  0x0E-0x26, 45-74; Ken, Yuji y Yang (entrada del Desierto Mobius, según la puerta), desde 0x0F salvo 0x27-0x28,
  60-89; Bob (Ruinas Protocolo), 0x11-0x26, **menos de 30**.
- **Tras el final**, en la Placa de Circuitos (0x2EA/0x2EB), cada jefe da su DDNA y el compañero si tienes a todos a
  nivel 45 y te falta ese compañero.
- **Agumon:** DRI Johann en las Ruinas (0x22B) (`F1a:2d`, `F06:00`); MetalGreymon en 0x233 (`F1c:12`).
- **Veemon:** DRI Bob en las Ruinas Protocolo (0x226) (`F1a:29`, `F06:07`); Paildramon en 0x234 (`F1c:19`).

- **Guilmon:** DRI Jimmy (`F1a:2e`, `F06:01`); WarGrowlmon en 0x23C (`F1c:13`).
- **Patamon:** DRI Kazuki (`F1a:1f`, `F06:02`, equipo 45-75); MagnaAngemon en 0x247 (`F1c:14`).
- **Renamon:** DRI Michael (`F1a:20`, `F06:03`, equipo 45-75); Taomon en 0x24A (`F1c:15`).
- **Kotemon:** DRI Ken (`F1a:2f`, `F06:04`, equipo 60-90); Kyukimon en 0x241 (`F1c:16`).
- **Kumamon:** DRI Yuji (`F1a:30`, `F06:05`, equipo 60-90); GrapLeomon en 0x23C (`F1c:18`).
- **Monmon:** DRI Yang (`F1a:31`, `F06:06`, equipo 60-90); Armormon en 0x228 (`F1c:17`).
- Los DRI están en los mapas 0x21D, 0x22E, 0x258. El 0x258 (entrada del Desierto Mobius, `wstag635`) y el 0x259
  (`wstag640`) son un laberinto de 8 salas cada uno (`placePoints`, la sala en `GAME.placeArg`): Ken en la sala 1 (la
  de S Noise Desert, 0x257), Yuji en la 4 y Yang en la 7 (`WARP_ARG(0x1E/0x21/0x24)`). Las salidas se cruzan con X.
- **Tiendas** (`stitshop/data/stitshop.c`): el Cañón Láser, con el Tenjinmaru, el Brazo Tempest, la Vara Cristal y
  el Colmillo Veneno, en las tiendas 26 y 28 (Bai Hu y Xuan Wu); los de la Torre Delirios, en la 25; los finales, en
  la 15.

## Herreros: uno por compañero
Cada mejora la hacen herreros distintos según el compañero que lleves (por ejemplo Vencer: Shadramon con Agumon en
0x2BE, Shadromon con Guilmon en 0x28E, otro con Veemon en 0x294; Brillo: Ponchomon con Patamon en 0x295, con Renamon en
0x2C3). Casi todos los ingredientes (Onimaru, Colmillo Sable, Puño Kaiser, Cañón Racimo, Khakkhara, Brazo Tempest,
Vara Cristal, Cañón Láser…) están en **cofres del capítulo extra** (mapas 0x2E4-0x2EE): las cadenas son sobre todo
contenido de después de la historia. En la historia solo está el Tenjinmaru (Magasta B1F, 0x2DA). El mapa de pruebas
(`wstag586`, `--field 0x2b8`) tiene un personaje que da todos los ingredientes: sirve para probar.

## Diseño en el Digilog
- **Cada misión tiene pasos:** un paso es una condición que el juego ya sabe comprobar (`FLAGS_00.checkCondition`:
  marca, objeto, condición especial) más lo necesario para escribirlo. El paso actual es el primero que no se
  cumple.
- **El texto de cada paso se compone:** una plantilla nuestra en los 5 idiomas ("Consigue …", "Vence a …", "Habla con
  …", "Lleva a … a nivel 45") más el **nombre oficial del juego** (objeto, Digimon, compañero), sacado de sus textos.
  Se monta en memoria como un archivo de texto de una sola cadena, el formato que ya aceptan las ventanas del juego.
- **Cada paso tiene su mapa:** la foto, el nombre del lugar y el MAPA (X) del detalle muestran dónde toca ir ahora.
- **En el estado:** donde ponía "En curso." sale el paso actual.
- **Lista de pasos:** □ cambia la lista de la izquierda por los pasos de la misión elegida, con los hechos marcados.
  □ otra vez vuelve a las misiones.

## La historia: los pasos dentro de cada paso
El Digilog tiene 46 pasos de historia (`GAME.progress` 0x00-0x2D), cada uno con una frase. Dentro de muchos hay
varias cosas que hacer. Salen de las conversaciones que solo ocurren en ese paso (`tools/story_steps.py`), de los
eventos que lanza cada mapa en ese paso, en orden, cada uno con su marca de "visto" `F40:xx`
(`tools/story_events.py`, `tools/story_code.py`), y de lo que se dice en ellos (`tools/story_dialogue.py`). Un subpaso
está hecho cuando su condición se cumple. Abajo, los subpasos de cada paso, con su condición y su mapa.

### 0x04 Líder Seiryu
1. Habla con Tom el Repetidor, en Ciudad Seiryu: el líder entrena en las Ruinas Protocolo. (`F1c:0f`, 0x230)
2. Reta al Líder Seiryu en las Ruinas Protocolo: no lucha con débiles. (`F1c:10`, 0x226)
3. Vence a MasterTyranno en el Valle Tyranno. Te da la Garra Antigua. (`F1c:11`, 0x22B)
4. Vence al Líder Seiryu en Ciudad Seiryu: Pin Seiryu y Pase de Plata. (`F40:12`, 0x22E)

### 0x05-0x06 La Tarjeta Azul y el tren al sur
1. Teddy ha visto al Ratón Suerte en la Pradera del Viento. (`F40:11`, 0x229)
2. Busca la Tarjeta Azul: un Guilmon te manda a su primo. (`F1c:44`, 0x20C)
3. El primo dice que la tiene otro Guilmon. (`F1c:45`, 0x223)
4. Un Guilmon Astuto te da una tarjeta… que no funciona en la estación. (`F1c:46`, 0x22C)
5. En Ciudad Seiryu te cuentan que era una farsa del Guilmon Astuto. (`F1c:47`, 0x22E)
6. Encuentra al Guilmon Astuto y consigue la Tarjeta Azul verdadera. (`+Tarjeta Azul`, 0x224)
7. Registra la Tarjeta Azul en la estación. (0x22C)
8. Gana a Keith en el Puente de Asuka. (`F40:06`, 0x205)
9. Un terremoto para la góndola en la Grieta Profunda. (`F40:18`, 0x415)
10. Llega a la Estación Sur. (paso 0x07)

### 0x07-0x09 Zanbamon y la máscara
1. Zanbamon te corta el paso en la Tumba de la Selva. (`F40:00`, 0x480)
2. Gatomon dice que Sepikmon, en la Casa del Chamán, sabrá qué hacer. (`F1c:43`, 0x238)
3. Sepikmon perdió su máscara cuando estaba con Baronmon. (evento 180, `F1a:17`, Casa del Chamán, 0x475)
4. Baronmon, en las Ruinas Protocolo: Sepikmon ha estado haciendo bromas en Ciudad Asuka. (`F1a:18`, 0x226)
5. En el Puente de Asuka, un Sepikmon (falso) dice que encontró la máscara. (`F1a:19`, 0x202)
6. Etemon, en Ciudad Asuka: Sepikmon huyó al Lago Divermon. (`F1a:1a`, 0x201)
7. En el lago solo está Nick (evento 205: la historia pasa a 0x09). (0x227)
8. Etemon, otra vez en Ciudad Asuka, muy contento. (`F1a:1b`, 0x201)
9. Recupera la Máscara Sepik de Etemon en las Alcantarillas de Asuka. (`+Máscara Sepik`, 0x21B)
10. Devuélvesela a Sepikmon. (evento 230, paso 0x0A)

### 0x0A Líder Suzaku
1. Vence al Líder Suzaku en el Salón Suzaku: Pin Suzaku. Para el Sector Oeste hay que llamar a Submarimon. (`F40:02`, 0x240)

### 0x0B-0x0D Buscar a Teddy
1. Kail quiere buscar a Teddy en el Centro Admin. de Asuka. (`F40:04`, Vestíbulo)
2. El guarda no deja pasar: separaos y pregunta en la ciudad. (paso 0x0C)
3. Ponte el Traje Agumon: el guarda deja pasar a los Digimon. (`+Traje Agumon`, 0x20B)
4. Kail, disfrazada, sigue buscando por su cuenta. (`F40:05`, Vestíbulo)
5. En la Sala Maestra, el Maestro de Juego os pilla: Teddy lo tiene preso el Ratón Suerte, en el Sector Oeste. (`F1c:0e`, 0x305)

### 0x0E Jefe Qing Long y el Chip TNT
1. Te piden una bomba para abrir un muro. (`F1a:2b`)
2. Baronmon, en las Ruinas Protocolo, haría un Chip TNT con una Bola TNT. (`F1c:4f`, 0x226)
3. Consigue la Bola TNT. (`+Bola TNT`)
4. Baronmon te hace el Chip TNT. (`+Chip TNT`, 0x226)
5. Pon el Chip TNT en el muro del Pantano Bios. (`F40:07`, 0x455)

### 0x0F-0x11 Ciudad Byakko
1. Vence al Líder Byakko: el Pin Byakko está en una sala. (`F40:1e`, Cúpula Byakko)
2. Era una trampa: caes al sótano. Busca una salida. (paso 0x10)
3. Numemon te saca: la gente de Byakko es falsa. (`F40:a2`, Valle de las Balas)
4. Encuentra a Numemon en la Fábrica Dum Dum. (`F40:3c`)
5. Vence a HiAndromon: te abre las escaleras secretas y te da el Rifle Oxidado. (`F0a:04`, 0x252)

### 0x12-0x15 Lisa, el Ratón Suerte y el Pase de Personal
1. Sigue a Lisa por las alcantarillas; el Ratón Suerte es Numemon. (`F40:39`, Alcantarillas)
2. En su escondite, vence al Atacante de la A.o.A. (`F40:3b`, Sala Secreta)
3. Sin un Pase de Personal no se entra en Ciudad Asuka. (`F40:3a`)
4. Datamon abriría los cimientos con un pase. (`F1c:1b`, 0x21C)
5. Nick busca un pase en el ordenador. (`F40:03`, Sala Secreta)
6. Usa el Pase de Personal con Datamon. (`F1c:1d`, 0x21C)

### 0x16 Maestro de Juego
1. Ábrete paso entre los soldados del Centro Admin. (combates `F0c:05`-`F0c:18`)
2. Un Digimon raro en el Centro Admin. 2F. (`F40:4a`, `F40:4b`)
3. Toca los sensores de la Sala de Control en orden: M-O-N-S-T-E-R. (`F1c:27`-`F1c:2e`)
4. Se abre la puerta secreta de la Sala Maestra. (evento 661)

### 0x17 Vemmon
1. Vence a los guardias y al Maestro de Juego en el Cuartel A.o.A. (`F40:4e`, `F40:4f`)
2. Lisa y Nick son de la S.A.P.: interrogad al Maestro de Juego. (`F40:50`)

### 0x1A-0x1C Bulbmon y el Servidor Amaterasu
1. Vence a Bulbmon en el Pozo del Laberinto y sigue su agujero. (`F40:51`, 0x545)
2. Vence al Jefe Qing Long: Pase Azul. (`F40:a3`, Ciudad Qing Long)
3. Vence al Jefe Zhu Que: Pase ID Rojo. (`F40:52`, Salón Zhu Que)

### 0x1E Jefe Xuen Wu
1. Vence al Líder Genbu en la Puerta de Batalla. (`F40:01`, 0x755)

### 0x20-0x21 El Mundo Real
1. 180 segundos en el Mundo Real: llega a la Base Submarina. (`F40:43`, 0x795)
2. Se acaba el tiempo: el Juggernaut despega. (`F40:66`, Sala Cámara)
3. Habla con Lisa y los demás. (`F40:59`, Sala Maestra)
4. Interroga al Maestro de Juego en la Torre Prisión: Destromon. (`F40:57`, `F40:58`)

### 0x22-0x24 Los cuatro códigos
1. Para entrar en Amaterasu hacen falta cuatro códigos contra los Knightmon. (evento 893)
2. Pase Blanco: Cúpula Bai Hu. (`F40:5a`)
3. Pase Negro: vence al Jefe Xuen Wu en la Puerta de Batalla. (`F40:5b`)
4. Cruza el Puente Amaterasu con Lisa y la Resistencia. (`F40:ca`)

### 0x25-0x28 Ciudad Amaterasu
1. Vence a la Guardia Real en el Cuartel A.o.A. (`F40:5e`)
2. Vence al Oficial Jefe. (`F40:67`)
3. Arrestad al Presidente de MAGAMI. (`F40:5f`)
4. Destromon detenido: volved a la Sala de Registro. (`F40:60`)
5. La Cámara no funciona: Oinkmon en la Sala Maestra. (`F40:69`)
6. Sigue a Vemmon en el Cuartel. (paso 0x27)

### 0x29-0x2A El satélite
1. Llega a la Sala de Control del Satélite Espía. (`F40:74`)
2. Vence a Lord Megadeath y a Snatchmon. (evento 1070)

Los pasos que no aparecen aquí (0x00-0x03, 0x18-0x19, 0x1D, 0x1F, 0x2B-0x2D) ya se cuentan bien con su frase.

## Los jefes de las misiones del port: sus marcas y sus tramos
Cada jefe pone una marca al encontrarlo (y empieza el combate) y otra al vencerlo (la de hecha). Casi todos solo están
en un tramo de la historia: sus actores llevan una condición especial de rango (`SPECIAL(id)`, cuyo rango está en
`PROGRESS_RANGES` de `main/game/events.c`; `SPECIAL(0x19)` es 0x1B-0x25, `SPECIAL(0x1A)` es 0x27-0x28). Ojo:
`PROGRESS(n), 1` es igualdad, no "desde".
Desde la v1.2.2, cada una de estas lleva su último paso en `SIDE_QUESTS` (el campo `last`): pasado ese paso, el
Digilog deja de mostrarla en "Activas". Kenny 0x1B; cinco Black Kings 0x24; WarGreymon BK 0x28; la Bruja y los
Caballeros 0x25; los Maestros 0x28; Zanbamon 0x18; MasterTyranno 0x04. Pharaohmon y las cartas épicas no, porque
vuelven en el capítulo extra.
- **Armas viejas:** Zanbamon `F40:55`/`56` (Catacumba, solo en los pasos 0x08 y 0x18); Datamon `F40:08`/`09` (Sala
  de Control de Asuka, siempre); MasterTyranno `F40:0f`/`10` (Valle Tyranno, solo en 0x04); Pharaohmon `F40:53`/`54`
  (Ruinas Protocolo, en 0x04 y en el capítulo extra).
- **Mazmorras del Sector Norte** (al oeste del Agujero Kulon): la Bruja de Hielo `F40:0e`/`19`, el Caballero de Fuego
  `F40:3f`/`40` y el Oscuro `F40:41`/`42`, de 0x1B a 0x25. Los Maestros de Hielo `F40:82`/`83`, Fuego `F40:84`/`85` y
  Oscuro `F40:86`/`87`, solo en 0x27-0x28: tras el punto sin retorno (la guía los pone antes; el código manda).
- **Kenny:** el evento 720 de la Posada del Bosque BF (`F40:1d`) solo con la historia en 0x1B justo.
- **Bolsa de Carpetas:** hablar con Divermon en el Crucero Amarillo de Asuka (desde 0x04): `F1c:48` y el evento 1415
  (el 92 de la tabla de eventos de campo), que da la bolsa y `F40:a1`.
- **Black Kings** (servidor Amaterasu): MagnaGargomon `F40:78`/`79` (Ruinas Protocolo), Imperialdramon `F40:7a`/`7b`
  (Valle Tyranno), Seraphimon `F40:7c`/`7d` (Catacumba), WarGrowlmon `F40:7e`/`7f` (Fábrica Dum Dum), KingNumemon
  `F40:80`/`81` (Alcantarillas), de 0x1B a 0x24; WarGreymon `F40:76`/`77` (Sala de Control de Amaterasu, buceando por
  sus alcantarillas), de 0x25 a 0x28.
- **Base Submarina** (los 180 segundos del paso 0x20): el Soldado de Metal en Magasta B2F `F40:44`/`45`; la Guardia
  Real en Magasta 1F, `F40:46` (se pone al empezar el combate).

## Lo que da cada jefe (sirve para las misiones)
- **Armas viejas:** la Katana Oxidada, Zanbamon (Catacumba); los Guantes Oxidados, Datamon (Sala de Control de
  Asuka); el Rifle Oxidado, HiAndromon (Sala de Operaciones, historia 0x11); la Garra Antigua, MasterTyranno (historia
  0x04); la Varita Antigua, Pharaohmon (Ruinas Protocolo).
- **Black Kings:** WarGreymon (ID Platino), MagnaGargomon (Casco Salvaje), Imperialdramon (Casco de Peligro),
  Seraphimon (Traje de Tirador), WarGrowlmon (Armadura Peligro), KingNumemon (Adaptador BIT).
- **Mazmorras:** la Bruja de Hielo, los Caballeros de Fuego y Oscuro, y los Maestros de Hielo (Chip PM), Fuego (Chip
  Mente) y Oscuro (Adaptador EXP).

## Lo que añade la guía de PhalX
Datos sacados de la *Guía para Digimon World 2003* de **PhalX** (versión 3.5, 2012), que permite usarla citándolo. Aquí
van resumidos con nuestras palabras; la guía no se copia al repositorio.

### Encargos
- **Caña de Pescar:** la Jabalina de Bambú se **compra en Ciudad Asuka**; la Tela de Araña la sueltan los
  **Dokugumon del Bosque Protocolo**; el Pez Rojo, Divermon en su lago. Con la caña se pesca en la orilla de lagos y
  mares (pocos usos por mapa).
- **ID El Dorado:** pide **210 de carisma** (la condición del juego es el umbral 2 de `PARTY_STAT_THRESHOLDS`, 210).
- **Bola TNT** (Chip TNT de la historia): la sueltan los **Triceramon** (Capa de Conexión y Valle Tyranno de Asuka).
- **Trofeo Asuka:** se abre al conseguir el **Digihuevo de la Seriedad**. Rivales: 4 Divermon (Maestros Blanco,
  Negro, Verde y Rojo), un quinto (Maestro Azul) y KingDivermon.
- **Trofeo Sol:** mejor tras el **Digihuevo de la Sabiduría**. 3 Etemon; el cuarto pide su micrófono:
  - **BK Agumon** está en la Placa de Circuitos: por el agujero frente a donde se conoce a Zanbamon en la Tumba de la
    Selva de Asuka, tres pantallas al norte.
  - **El líder BKZ**, en el Central Park de Amaterasu, por el hoyo junto al gimnasio de Panjyamon.
  - Luego un Etemon más, MetalEtemon y KingEtemon.
- **Rivales de cartas sueltos** (podría ser una misión): 5 Numemon y 5 BK Agumon por el mapa, que dan cartas especiales
  (MarineAngemon, SaberLeomon, Rosemon II, Gryphonmon, Creepymon, Puerta de la Fortuna, Anti Poder, Puerta de la
  Miseria, Borrar Matriz, Acceso al Deseo).

### Cadenas de armas: los ingredientes se compran
Corrige lo que pensábamos: no son solo del capítulo extra.
- **1.ª mejora**, en el servidor Amaterasu: el arma vieja (de su jefe) + un objeto de la **Torre Delirios** de
  Amaterasu (Colmillo Sable, Khakkhara, Puño Kaiser, Cañón Racimo, Onimaru).
- **2.ª mejora**, con el compañero a **nivel 45**: un objeto de las **ciudades Bai Hu o Xuan Wu** (Colmillo Veneno,
  Vara de Cristal, Brazo Tempestad; el Juicio y el Masamune no piden compra).
- **3.ª mejora**, con **MetalEtemon** en cualquier servidor: un objeto de **Ciudad Amaterasu** (Colmillo Oculto, Milenio,
  Puño Raijin, Cañón Suijin, Acero Fujin).

Los herreros, por compañero:
- **Invencible (Agumon, Guilmon o Veemon):**
  - Shadramon: con Agumon en las alcantarillas de la Fábrica Dum Dum; con Guilmon en Shell Beach; con Veemon en el
    Bosque Protocolo, detrás del pilar.
  - Depthmon, buceando: con Guilmon en el fondo de Shell Beach (Asuka); con Agumon en el de la Bahía Fénix
    (Amaterasu); con Veemon en el del Lago de Hielo.
- **Eternamente (Renamon o Patamon):**
  - Ponchomon: con Renamon fuera de la Torre Delirios (Amaterasu); con Patamon subiendo las escaleras de las Ruinas
    Protocolo (Amaterasu).
  - Kongoumon: con Renamon en el Gimnasio Legendario (Amaterasu); con Patamon en la Mina Kulon (entrando con Digmon
    por la Tumba de la Selva de Amaterasu).
- **Super Nova (Kumamon):** Nohemon en las Catacumbas (Amaterasu); Seehomon buceando en el Central Park de Asuka.
- **Castigo (Monmon):** Shurimon en el Lago SB Zhu Que; Sagitarimon en el Desierto Mobius (Amaterasu).
- **Muramasa (Kotemon):** Kabukimon en una zona escondida de la Jungla de Éter (Amaterasu), buceando desde la Bahía
  Fénix; Yashamon en la Sala Secreta (Amaterasu).

### Más misiones que salen de la guía
- **Botas Árbol** (ya en el Digilog; sus pasos):
  1. Un chico pateando un árbol en la Capa de Conexión (Plug Cape) quiere su **carta de Gabumon** (`F1a:22`).
  2. Está en la Posada de Asuka, abajo, junto a la niña (`F1a:23`).
  3. Dásela: te manda a hablar con Veemon (`F1a:24`, `F1c:08`).
  4. La señora de traje de la taberna Chuleta Cordero (al oeste de Asuka, junto a la armería) lo vio cerca de la
     Llanura del Viento (`F1a:25`).
  5. Veemon te reta a encontrarlo en el Bosque de las Patadas: patea los árboles de la entrada (`F1a:26`, `F1a:27`).
  6. Te da las Botas Árbol (`+0x004`). Con ellas se patean árboles para encontrar Cardmon.
- **ID Compinche** (Kenny, ya en el Digilog): Kenny el Valiente, en la Posada del Bosque de Amaterasu, al llegar al
  servidor Amaterasu: hace que los soldados de la A.o.A te tomen por uno de ellos.
- **Cartas épicas** (hecha en la v1.2.2, clave 0x200): cinco domadores, con el **Trofeo Asuka**, de 0x04 a 0x26.
  Cada uno pide antes un combate de Digimon (`F0e:xx`) y luego un duelo con carisma alto (`equipo13`, 2277; Mitaka,
  2472):
  - Nakano en el Salón Delirios (Javalina Sagrada, carta 1);
  - Bob en el Bosque de las Patadas (Onda Tidal, 7);
  - Mitch en la Entrada del Bosque Alambre (Aura Wield, 13);
  - Jessica en el Pantano Tranquilo (Cañón Volcánico, 19);
  - Mitaka en el Agujero Kulon (Gale Oscuro, 25).

  Tras el final, MetalEtemon las da en la Placa de Circuitos (`wstag972`).
- **Cartas de los BK** (hecha en la v1.2.2, clave 0x201): con el **Trofeo Sol**, en la Placa de Circuitos (0x2EA y
  0x2EC; cada rival sale según el agujero por el que se entra, condiciones `0x7exx`): BK Agumon con Borrar Matriz (31),
  Puerta de Fortuna (32), Puerta de Miseria (33), Acceso a Deseo (34) y Antipoder (35); Numemon con RosemonII (69),
  MarineAngemon (112), SaberLeomon (155), Gryphonmon (198) y Creepymon (241).
- **Cómo van en el Digilog:** una carta es la condición `0x9200 | id` (`checkCard`). Su clave no es un objeto
  (`JOURNAL_TITLED`, 0x200 en adelante), así que llevan título propio y no tienen caja de recompensa; están hechas
  cuando se tienen todas (`JOURNAL_ALL`), y su paso actual es la primera carta que falta, porque se consiguen en
  cualquier orden.

### Para la historia
- **0x0E:** tras el muro del Chip TNT, en el Punto Fiable, se coge el **Digihuevo de la Seriedad**, que abre la Isla
  del Duelo (el Trofeo Asuka).
- **Punto sin retorno:** al entrar en la Sala Maestra de Amaterasu, tras los Oinkmon (historia 0x26-0x27), ya no se
  pueden conseguir los artículos épicos ni las subastas. Un aviso en el Digilog antes de ese paso.
- **Subastas de El Dorado:** hay una nueva en muchos puntos de la historia (la guía las marca); la última, con la
  Armadura Valor de Agumon (32000 BIT), justo antes del punto sin retorno.
- **Para entrar al servidor Amaterasu por otro lado:** con Digmon, desde el Desierto Norte 0 (North Badland W), por la
  Placa de Circuitos.
