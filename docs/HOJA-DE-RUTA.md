# Hoja de ruta de Asuka Link

Inventario de todo lo que hay en el juego y en el port, en qué estado está aquí y qué se podría mejorar. Asuka Link
está basado en el port v0.18.9 de Feerdus y desde la v1.0.0 se mantiene por su cuenta: todo lo de esta lista es
responsabilidad nuestra.

**Coste:** **bajo** (unos días), **medio** (una o dos semanas), **alto** (varias semanas, con riesgo de atascarse).

## En marcha
- **v1.1.0:**
  - proyecto independiente (scripts en git, sin depender del port);
  - el launcher ya no descarga programas de fuera, y comprueba las actualizaciones;
  - entradas de red validadas;
  - el avance rápido se apaga en los combates en red;
  - quitadas del todo "Curar al subir de nivel", "Guardar en cualquier sitio" y los packs extra.
- **v1.2.0:** el Digilog completo: paneles de lista y detalle, fotos de los mapas, el objeto que pide cada misión y los
  iconos de las teclas.
- **v1.2.x:** pasos en todas las misiones del Digilog (y en 5 idiomas), misiones de cartas, arreglos de red.
- **v1.2.3, transición:** un solo zip por release (`asuka-link-<versión>.zip`, también para actualizar), el
  cooperativo activado por defecto. Es la última que sube además el `dw2003-native-<versión>.zip` de los launchers
  anteriores.
- **v1.3.0, la versión base** (hecha, falta publicarla): a partir de su lanzamiento es el punto de partida del proyecto.
  - **Opciones dentro del juego, en el menú de pausa del mapa:** una fila "Opciones" antes de "Salir". Select/Back y
    Esc ya no abren nada.
  - **Mods fijos:**
    - Desaparece la pestaña Mods (en el launcher y en el juego).
    - Activados: Digilog, viaje rápido, texto rápido, EXP compartida, entrenamiento sin azar, BladeGarurumon, efectos,
      sonidos y scripts propios y el cooperativo. El objetivo en el mapa va apagado.
    - Las líneas de esas opciones en un `native.ini` viejo se ignoran.
  - **Menos opciones:**
    - Se pueden cambiar: pantalla completa, tamaño de ventana, resolución interna, pantalla ancha en combates, idioma,
      iconos de botones, vibración y botones. EXP y dinero (x1–x5) solo se cambian en el launcher.
    - Fijo: 60 fps, VSync, vídeos suaves, sigue funcionando sin foco, sub-píxel, profundidad, perspectiva, texturas
      suaves, color real, sin tramado, sombras suaves, sombras y cielo en combate, zona muerta 10 %, menús en avance
      rápido, saltar PRESS START y cursor que da la vuelta.
  - **Launcher rehecho entero, también en lo gráfico:** el cooperativo en la pantalla principal, sin pasar por
    Ajustes.
  - **Estilo:** el atardecer de Asuka renovado: la escena a pantalla completa, un menú lateral (Jugar, Ajustes,
    Partidas, Herramientas, Salir) y la **sala** arriba a la derecha.
  - **La sala** (red: versión 8):
    - Tu grupo de juego, de hasta 4 contigo.
    - **En tu red:** los launchers de la misma red se anuncian por difusión y salen como **disponibles**, sin
      entrar en tu grupo. Con "Invitar", el otro ve la invitación y la acepta o la rechaza. Así puede haber dos
      grupos en la misma red si sois más de 4.
    - **Añadir por IP** (para Tailscale, ZeroTier o Internet): empareja directamente. Quien recibe se añade a tu
      grupo sin tener que escribir tu IP.
    - Un grupo lleno (4) no sale como invitable. "Salir de la sala" te deja solo.
    - El grupo se guarda como `coop_peers`, y el juego, si está abierto, sigue los cambios sin reiniciar.
  - Descartado para la 1.3: el código de sala (UPnP). Más adelante, quizá, un código de amigo fijo con un servidor
    pequeño nuestro.
  - **Una sola descarga:** solo `asuka-link-<versión>.zip` y su `.sha256` (release.ps1 deja de subir el nombre viejo
    a partir de la 1.3.0).
  - Al publicarla: decidir qué hacer con las releases y el historial anteriores.
- **Después:**
  - raids: "Turno de X", objetos y huir, ajuste de jefes;
  - avisos de "X ha entrado en…".

## Los sistemas del juego

| Sistema | Qué es | Con el cooperativo | Ideas | Coste |
|---|---|---|---|---|
| **Mapa e historia** (`fieldstg`, `stages`) | 21 zonas, con Asuka y Amaterasu como dos servidores; los eventos de la historia | Os veis en el mismo mapa, con aspecto y nombre | **Avisos** de entrar y salir (en marcha). **Ver a los amigos en la pantalla del mapa.** **Viajar al mapa de un amigo** con el viaje rápido | bajo / medio |
| **Combates** (`fightstg`, `wfightmn`, informe `stfgtrep`) | Combates por turnos con 3 compañeros; el informe del final con EXP y crecimiento | PvP por turnos alternos; raids de 2 a 4 contra 29 jefes | Raids: turno, objetos, huir, ajuste. **Combates juntos en el mapa:** unirse al combate aleatorio de un amigo | bajo / alto |
| **Compañeros y digievoluciones** (`stgdglab`, `ststatus`) | 8 compañeros, cada uno con sus formas; en el laboratorio, **equipo**, **ranuras** (tipo de digievolución y cargar técnica) y **recetas** de DDNA, comparadas con lo que tiene cada uno | Los equipos viajan en PvP y raids | **Recetas más útiles:** que la pantalla de recetas diga qué falta y cómo conseguirlo, en lugar de una pestaña nueva en el Digilog. Ver los equipos de los amigos | medio |
| **Entrenamiento** (`stgtrain`) | Los gimnasios: sesiones que suben stats | Entrenamiento sin azar (opción, activada) | — | — |
| **Tiendas** (`stitshop`) | Compra, venta, equipo y trueque | — | **Intercambio de objetos y dinero** entre jugadores (hoy solo cartas) | medio |
| **Juego de cartas** (`cardgame`, álbum `stcrdabm`, mazos `stcrddek`, tienda `stcrdshp`) | Duelos contra la máquina, álbum de 314 cartas, editor de mazos, sobres | Intercambio de cartas sueltas; **duelos entre jugadores** con premio para los dos | Torneo de 3-4 jugadores (alto). El menú principal ya tiene su apartado de cartas | alto |
| **Partida nueva** (`stplnmet`, `stdgname`, `cnty_sel`) | Nombre, idioma, equipo inicial | Equipo libre y creador de personaje (Junior, Chico, Chica, 8 tonos) | **Ampliar el creador:** más aspectos, y que los tonos del Chico se noten (hoy casi gris) | medio |
| **Memory card** (`stgmcard`) | Guardar y cargar | Cada uno, su partida | — | — |

## Los mods y opciones del port

| Opción | Estado aquí | Con el cooperativo | Ideas | Coste |
|---|---|---|---|---|
| Gráficos (resolución, sub-píxel, profundidad, perspectiva, color, sombras, cielo y niebla) | Como en el port | Nada que ver | **Pantalla ancha también en el mapa** (hoy solo en combates) | alto |
| Pantalla ancha en combates | Como en el port | — | — | — |
| Texto rápido | Como en el port | — | — | — |
| Avance rápido (x2, x3, x5) | Como en el port | En el mapa va bien; en PvP, raids y duelos **se apaga solo** (v1.1.0) | — | — |
| Viaje rápido | Activado por defecto | — | **Ir al mapa de un amigo** | medio |
| Objetivo en el mapa | Como en el port | — | **Los amigos en la pantalla del mapa** | bajo-medio |
| **Digilog** | Activado por defecto; completado en la v1.2.0 (paneles, fotos de los mapas, objeto que pide cada misión, iconos de teclas) | — | **Misiones que faltan** (hoy 29): hay que descifrar los scripts de eventos del juego (`wstag260.c`) para encontrarlas | medio-alto |
| BladeGarurumon y Digimon de mods (`data/mods/digimon`) | Como en el port | **Sin comprobar** en PvP, raids e intercambios si el otro no tiene el mod | Comprobarlo, y si falla, avisar o bloquear esa forma en red | bajo-medio |
| EXP compartida completa | Activada por defecto | — | — | — |
| Efectos, sonidos y scripts propios (`data/mods/`) | Como en el port | Solo cambian lo que ves | Comprobar que dos jugadores con mods distintos no se desincronizan | bajo |
| Multiplicadores de EXP y dinero | Del cooperativo | x3 en raids | — | — |
| Curar al subir de nivel, Guardar en cualquier sitio, packs extra | **Se quitan del todo** (v1.1.0) | — | — | — |
| Visor de modelos y editor de mapas | Herramientas de Feerdus | — | Usarlos **en local** para hacer mods; si los mantenemos, fork propio | — |

## Para más adelante
- Ampliación del creador de personaje.
- Intercambio de objetos y dinero.
- Combates juntos en el mapa.
- Torneo de cartas.
- Pantalla ancha en el mapa.

## Descartado
- **Chat:** bastan los avisos.
- **Seguir las versiones del port de Feerdus:** congelado en la v0.18.9.
- **Pestaña de cartas en el Digilog:** ya está en el menú principal.
- **Los amigos en el Digilog:** mejor en la pantalla del mapa.
