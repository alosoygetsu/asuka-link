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
- **Después:**
  - completar el Digilog;
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
| **Digilog** | Activado por defecto | — | **Completarlo:** paneles de lista y detalle, fotos de los mapas, marcar el objeto que pide cada misión, iconos de teclas, misiones que faltan (hoy 29), y en qué paso va cada amigo | medio |
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
