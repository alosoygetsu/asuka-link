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
1. Ganar a los 5 Divermon de la isla en duelo de cartas: `F1a:37`, `F1a:38`, `F1a:39`, `F1a:3a`, `F1a:3b` (un paso que
   cuenta cuántos llevas).
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
2. Te pide encontrar al Digimon (`F06:0n`). Solo te lo pide si **no tienes** a ese compañero y tu equipo tiene cierto
   nivel (suma de niveles entre 30 y 60).
3. Aparece en su mapa (el de la misión): vencerlo (marca `F1c:1x` y combate).
4. Coger su DDNA (`+objeto`).
5. Llevárselo al DRI: el compañero se une (`desbloquea`). Hecha.
- **Agumon:** DRI Johann en las Ruinas (0x22B) (`F1a:2d`, `F06:00`); MetalGreymon en 0x233 (`F1c:12`).
- **Veemon:** DRI Bob en las Ruinas Protocolo (0x226) (`F1a:29`, `F06:07`); Paildramon en 0x234 (`F1c:19`).

- **Guilmon:** DRI Jimmy (`F1a:2e`, `F06:01`); WarGrowlmon en 0x23C (`F1c:13`).
- **Patamon:** DRI Kazuki (`F1a:1f`, `F06:02`, equipo 45-75); MagnaAngemon en 0x247 (`F1c:14`).
- **Renamon:** DRI Michael (`F1a:20`, `F06:03`, equipo 45-75); Taomon en 0x24A (`F1c:15`).
- **Kotemon:** DRI Ken (`F1a:2f`, `F06:04`, equipo 60-90); Kyukimon en 0x241 (`F1c:16`).
- **Kumamon:** DRI Yuji (`F1a:30`, `F06:05`, equipo 60-90); GrapLeomon en 0x23C (`F1c:18`).
- **Monmon:** DRI Yang (`F1a:31`, `F06:06`, equipo 60-90); Armormon en 0x228 (`F1c:17`).
- Los DRI están en los mapas 0x21D, 0x22E, 0x258 (por asignar a cada uno).

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
