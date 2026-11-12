"""Los pasos del Digilog: dentro de cada paso de la historia y de cada misión secundaria, con la condición que da cada
uno por hecho y su mapa (docs/MISIONES.md es el estudio de dónde salen). De aquí salen sus textos (tools/coop_text.py
los añade a los del cooperativo) y su tabla en C (include/journal_steps.h, que lee el Digilog,
coop/patches/src/ststatus/item_list.c).

Una condición es la de FLAGS_00.checkCondition (código, 1): una marca F(grupo, n), un objeto ITEM(n), un combate de
equipo PARTY(n); o AFTER(paso): la historia ya ha pasado ese paso. El paso actual es el siguiente al último que se
cumple (los objetos que se entregan dejan de cumplirse, y la misión no retrocede por eso).

Un mapa es un modo (M(0x230)) o el de un escenario (S(420): el primero de los suyos en data/stages/wstag420.json).
Cada texto, en español e inglés (francés, italiano y alemán usan el inglés hasta que se traduzcan), cabe en dos líneas
de 39 letras: se cortan solas.
Uso: python native/tools/journal_steps.py   (escribe include/journal_steps.h)
"""
import json
import sys
from pathlib import Path

NATIVE = Path(__file__).resolve().parents[1]
STAGES = NATIVE / "build" / "data" / "stages"


def F(group, n): return group << 8 | n
def ITEM(n): return 0x8000 | n
def PARTY(n): return 0x7200 | n
def AFTER(p): return 0xF000 | p  # el Digilog: GAME.progress > p
def M(mode): return ("m", mode)
def S(stage): return ("s", stage)


def mode_of(where):
    kind, n = where
    if kind == "m":
        return n
    d = json.load(open(STAGES / f"wstag{n}.json"))
    return (d.get("euModes") or d.get("modes"))[0][0]


# ---- la historia: por cada grupo de pasos (los que comparten la misma lista), sus pasos ----
STORY = [
    ([0x04], [
        (F(0x1C, 0x0F), M(0x230), "Habla con Tom el Repetidor en Ciudad Seiryu.", "Talk to Tom the Repeater in Seiryu City."),
        (F(0x1C, 0x10), M(0x226), "Reta al Líder Seiryu en las Ruinas Protocolo.", "Challenge Leader Seiryu at the Protocol Ruins."),
        (F(0x1C, 0x11), M(0x22B), "Vence a MasterTyranno en el Valle Tyranno.", "Beat MasterTyranno in Tyranno Valley."),
        (F(0x40, 0x12), S(420), "Vence al Líder Seiryu en Ciudad Seiryu.", "Beat Leader Seiryu in Seiryu City."),
    ]),
    ([0x05, 0x06], [
        (F(0x40, 0x11), S(395), "Teddy ha visto al Ratón Suerte en la Pradera del Viento.", "Teddy saw Lucky Mouse on the Wind Prairie."),
        (F(0x1C, 0x44), M(0x20C), "Busca la Tarjeta Azul: pregunta a los Guilmon.", "Look for the Blue Card: ask the Guilmon."),
        (F(0x1C, 0x45), M(0x223), "El primo de Guilmon dice que la tiene otro Guilmon.", "Guilmon's cousin says another Guilmon has it."),
        (F(0x1C, 0x46), M(0x22C), "La tarjeta del Guilmon Astuto no funciona en la estación.", "Sly Guilmon's card doesn't work at the station."),
        (F(0x1C, 0x47), M(0x22E), "Era una farsa: busca al Guilmon Astuto.", "It was a fake: find Sly Guilmon."),
        (ITEM(0x18E), M(0x224), "Consigue la Tarjeta Azul verdadera.", "Get the real Blue Card."),
        (F(0x40, 0x06), S(205), "Gana a Keith en el Puente de Asuka.", "Beat Keith on Asuka Bridge."),
        (F(0x40, 0x18), S(415), "Toma la góndola: un terremoto la para en la Grieta Profunda.", "Take the gondola: a quake stops it in the Deeper Crevice."),
        (AFTER(0x06), S(440), "Llega a la Estación Sur.", "Reach South Station."),
    ]),
    ([0x07, 0x08, 0x09], [
        (F(0x40, 0x00), S(480), "Zanbamon te corta el paso en la Tumba de la Selva.", "Zanbamon blocks the way at the Jungle Grave."),
        (F(0x1C, 0x43), M(0x238), "Gatomon dice que Sepikmon sabrá qué hacer.", "Gatomon says Sepikmon will know what to do."),
        (AFTER(0x07), S(475), "Sepikmon perdió su máscara: la tenía con Baronmon.", "Sepikmon lost his mask while with Baronmon."),
        (F(0x1A, 0x18), M(0x226), "Baronmon: Sepikmon anda gastando bromas en Asuka.", "Baronmon: Sepikmon is up to pranks in Asuka."),
        (F(0x1A, 0x1B), M(0x201), "Encuentra a Etemon en el Vestíbulo de Asuka.", "Find Etemon in Asuka's Main Lobby."),
        (ITEM(0x015), M(0x21B), "Recupera la Máscara Sepik en las Alcantarillas.", "Get the Sepik Mask back in the Sewers."),
        (AFTER(0x09), S(475), "Devuélvele la máscara a Sepikmon.", "Give the mask back to Sepikmon."),
    ]),
    ([0x0A], [
        (F(0x40, 0x02), S(520), "Vence al Líder Suzaku en el Salón Suzaku.", "Beat Leader Suzaku in Suzaku Hall."),
    ]),
    ([0x0B, 0x0C, 0x0D], [
        (F(0x40, 0x04), S(212), "Kail quiere buscar a Teddy en el Centro Admin.", "Kail wants to look for Teddy at the Admin Center."),
        (AFTER(0x0B), S(285), "El guarda no deja pasar: pregunta por la ciudad.", "The guard won't let you in: ask around town."),
        (ITEM(0x00F), M(0x20B), "Ponte el Traje Agumon: dejan pasar a los Digimon.", "Wear the Agumon Suit: Digimon may pass."),
        (F(0x40, 0x05), S(210), "Kail, disfrazada, busca por su cuenta.", "Kail, in disguise, searches on her own."),
        (F(0x1C, 0x0E), S(305), "En la Sala Maestra os pilla el Maestro de Juego.", "The Game Master catches you in the Master Room."),
    ]),
    ([0x0E], [
        (F(0x1A, 0x2B), None, "Te piden una bomba para abrir un muro.", "You need a bomb to open a wall."),
        (F(0x1C, 0x4F), M(0x226), "Baronmon haría un Chip TNT con una Bola TNT.", "Baronmon could make a TNT Chip from a TNT Ball."),
        (ITEM(0x05A), None, "Los Triceramon del Valle Tyranno sueltan Bolas TNT.", "Triceramon in Tyranno Valley drop TNT Balls."),
        (ITEM(0x00E), M(0x226), "Baronmon te hace el Chip TNT.", "Baronmon makes you the TNT Chip."),
        (F(0x40, 0x07), S(455), "Pon el Chip TNT en el muro del Pantano Bios.", "Set the TNT Chip on the wall in Bios Swamp."),
    ]),
    ([0x0F, 0x10, 0x11], [
        (F(0x40, 0x1E), S(675), "Vence al Líder Byakko en la Cúpula Byakko.", "Beat Leader Byakko in Byakko Dome."),
        (AFTER(0x0F), S(685), "Era una trampa: busca una salida del sótano.", "It was a trap: find a way out of the basement."),
        (F(0x40, 0xA2), S(575), "Numemon te saca: la gente de Byakko es falsa.", "Numemon gets you out: Byakko's people are fakes."),
        (F(0x40, 0x3C), S(580), "Encuentra a Numemon en la Fábrica Dum Dum.", "Find Numemon in the Dum Dum Factory."),
        (F(0x0A, 0x04), M(0x252), "Vence a HiAndromon: abre las escaleras secretas.", "Beat HiAndromon: he opens the secret stairs."),
    ]),
    ([0x12, 0x13, 0x14, 0x15], [
        (F(0x40, 0x39), S(615), "Sigue a Lisa por las alcantarillas.", "Follow Lisa through the sewers."),
        (F(0x40, 0x3B), S(620), "Vence al Atacante en el escondite.", "Beat the Attacker at the hideout."),
        (F(0x40, 0x3A), S(620), "Sin un Pase de Personal no se entra en Asuka.", "You can't enter Asuka without a Staff Pass."),
        (F(0x1C, 0x1B), M(0x21C), "Datamon abriría los cimientos con un pase.", "Datamon would open the foundation with a pass."),
        (F(0x40, 0x03), S(620), "Nick busca un pase en el ordenador.", "Nick looks for a pass on the computer."),
        (F(0x1C, 0x1D), M(0x21C), "Usa el Pase de Personal con Datamon.", "Use the Staff Pass with Datamon."),
    ]),
    ([0x16], [
        (F(0x40, 0x4A), S(300), "Lucha contra los soldados del Centro Admin.", "Fight past the soldiers in the Admin Center."),
        (F(0x1C, 0x27), S(305), "Toca los sensores de la Sala de Control: MONSTER.", "Touch the Control Room's sensors: MONSTER."),
        (AFTER(0x16), S(305), "Sube por la puerta secreta de la Sala Maestra.", "Go up the Master Room's secret door."),
    ]),
    ([0x17], [
        (F(0x40, 0x4E), S(310), "Vence a los guardias del Cuartel A.o.A.", "Beat the guards at the A.o.A HQ."),
        (F(0x40, 0x4F), S(310), "Vence al Maestro de Juego.", "Beat the Game Master."),
        (F(0x40, 0x50), S(210), "Interrogad al Maestro de Juego.", "Question the Game Master."),
    ]),
    ([0x1A, 0x1B, 0x1C], [
        (F(0x40, 0x51), S(545), "Vence a Bulbmon en el Pozo del Laberinto.", "Beat Bulbmon in the Bug Maze Pit."),
        (F(0x40, 0xA3), S(421), "Vence al Jefe Qing Long: Pase Azul.", "Beat Chief Qing Long: Blue Pass."),
        (F(0x40, 0x52), S(521), "Vence al Jefe Zhu Que: Pase ID Rojo.", "Beat Chief Zhu Que: Red ID Pass."),
    ]),
    ([0x1E], [
        (F(0x40, 0x01), S(755), "Vence al Líder Genbu en la Puerta de Batalla.", "Beat Leader Genbu at the Battle Gate."),
    ]),
    ([0x20, 0x21], [
        (F(0x40, 0x43), S(795), "En el Mundo Real: llega a la Base Submarina.", "In the Real World: reach the Undersea Base."),
        (F(0x40, 0x66), S(750), "Se acaba el tiempo: el Juggernaut despega.", "Time's up: the Juggernaut takes off."),
        (F(0x40, 0x59), S(305), "Habla con Lisa y los demás.", "Talk to Lisa and the others."),
        (F(0x40, 0x58), S(295), "Interroga al Maestro de Juego en la Torre Prisión.", "Question the Game Master in the Prison Tower."),
    ]),
    ([0x22, 0x23, 0x24], [
        (F(0x40, 0x5A), S(676), "Consigue el Pase Blanco en la Cúpula Bai Hu.", "Get the White Pass in Bai Hu Dome."),
        (F(0x40, 0x5B), S(756), "Vence al Jefe Xuen Wu: Pase Negro.", "Beat Chief Xuen Wu: Black Pass."),
        (F(0x40, 0xCA), S(206), "Cruza el Puente Amaterasu con la Resistencia.", "Cross Amaterasu Bridge with the Resistance."),
    ]),
    ([0x25, 0x26], [
        (F(0x40, 0x5E), S(311), "Vence a la Guardia Real en el Cuartel A.o.A.", "Beat the Royal Guard at the A.o.A HQ."),
        (F(0x40, 0x67), S(311), "Vence al Oficial Jefe.", "Beat the Chief Officer."),
        (F(0x40, 0x5F), S(311), "Arrestad al Presidente de MAGAMI.", "Arrest MAGAMI's President."),
        (F(0x40, 0x60), S(226), "Volved a la Sala de Registro.", "Go back to the Login Room."),
        (F(0x40, 0x69), S(305), "Oinkmon en la Sala Maestra de Asuka.", "Oinkmon in Asuka's Master Room."),
        (AFTER(0x26), S(310), "Sigue a Vemmon en el Cuartel A.o.A.", "Follow Vemmon in the A.o.A HQ."),
        (AFTER(0x27), None, "¡Última ocasión! Luego no habrá armas épicas ni subastas.", "Last chance! After this, no epic weapons or auctions."),
    ]),
    ([0x29, 0x2A], [
        (F(0x40, 0x74), S(820), "Llega a la Sala de Control del Satélite Espía.", "Reach the Spy Satellite's Control Room."),
        (AFTER(0x2A), S(820), "Vence a Lord Megadeath y a Snatchmon.", "Beat Lord Megadeath and Snatchmon."),
    ]),
]

# ---- las misiones secundarias: las nuevas (su objeto, su mapa, la condición de hecha) y los pasos de cada una, por su objeto ----
NEW_QUESTS = [
    (0x005, M(0x21F), ITEM(0x005), "Tai Kong Wang, en Shell Beach, quiere\nhacerte una caña de pescar.",
     "Tai Kong Wang at Shell Beach wants to\nmake you a fishing pole."),
    (0x008, M(0x213), ITEM(0x008), "Ponchomon, en El Dorado, da su ID a los\ndomadores con un buen equipo.",
     "Ponchomon at El Dorado gives his ID to\ntamers with a strong team."),
    (0x012, M(0x228), ITEM(0x012), "Gana el torneo de cartas de Duel Island.",
     "Win the card tournament on Duel Island."),
    (0x014, M(0x297), ITEM(0x014), "Gana el torneo de los Etemon y a\nKingEtemon.",
     "Win the Etemons' tournament and beat\nKingEtemon."),
]

QUEST_STEPS = {
    0x005: [
        (F(0x1A, 0x36), M(0x21F), "Habla con Tai Kong Wang en Shell Beach.", "Talk to Tai Kong Wang at Shell Beach."),
        (ITEM(0x192), None, "Consigue la Bolsa de Carpetas.", "Get the Folder Bag."),
        (F(0x1A, 0x21), M(0x21F), "Te hará una caña: necesita materiales.", "She'll make a pole: she needs materials."),
        (F(0x1A, 0x2C), M(0x227), "Ayuda a Divermon en su lago: un Pez Rojo.", "Help Divermon at his lake: a Red Snapper."),
        (ITEM(0x059), None, "Los Dokugumon del Bosque Protocolo sueltan Tela de Araña.", "Dokugumon in Protocol Forest drop Spider Web."),
        (ITEM(0x0A7), None, "Compra una Jabalina de Bambú en Ciudad Asuka.", "Buy a Bamboo Spear in Asuka City."),
        (ITEM(0x005), M(0x21F), "Llévale los tres materiales a Tai Kong Wang.", "Bring the three materials to Tai Kong Wang."),
    ],
    0x008: [
        (F(0x1A, 0x05), M(0x213), "Habla con Ponchomon en El Dorado.", "Talk to Ponchomon at El Dorado."),
        (PARTY(2), None, "Sube el carisma del equipo a 210.", "Raise your team's charisma to 210."),
        (ITEM(0x008), M(0x213), "Ponchomon te da el ID El Dorado.", "Ponchomon gives you the El Dorado ID."),
    ],
    0x012: [
        (F(0x1A, 0x3B), M(0x228), "Gana al primer Divermon a las cartas.", "Beat the first Divermon at cards."),
        (F(0x1A, 0x3A), M(0x228), "Gana al segundo Divermon.", "Beat the second Divermon."),
        (F(0x1A, 0x39), M(0x228), "Gana al tercer Divermon.", "Beat the third Divermon."),
        (F(0x1A, 0x38), M(0x228), "Gana al cuarto Divermon.", "Beat the fourth Divermon."),
        (F(0x1A, 0x37), M(0x228), "Gana al quinto Divermon.", "Beat the fifth Divermon."),
        (ITEM(0x012), M(0x228), "Gana a KingDivermon: Trofeo Asuka.", "Beat KingDivermon: Asuka Trophy."),
    ],
    0x014: [
        (F(0x1A, 0x3C), M(0x297), "Gana al primer Etemon a las cartas.", "Beat the first Etemon at cards."),
        (F(0x1A, 0x3D), M(0x297), "Gana al segundo Etemon.", "Beat the second Etemon."),
        (F(0x1A, 0x3E), M(0x297), "Gana al tercer Etemon.", "Beat the third Etemon."),
        (F(0x1C, 0x49), M(0x297), "El último Etemon no juega sin su micrófono.", "The last Etemon won't play without his mic."),
        (F(0x1C, 0x3F), M(0x2EA), "Un Black King Agumon: lo tiene su líder.", "A Black King Agumon: their leader has it."),
        (F(0x1C, 0x40), M(0x2EA), "Gana al líder de los Black Kings: el micro.", "Beat the Black Kings' leader: the mic."),
        (F(0x1A, 0x3F), M(0x297), "Devuélveselo a Etemon y gánale.", "Give it back to Etemon and beat him."),
        (F(0x1A, 0x40), M(0x297), "Gana a MetalEtemon.", "Beat MetalEtemon."),
        (ITEM(0x014), M(0x297), "Gana a KingEtemon: Trofeo Sol.", "Beat KingEtemon: Sun Trophy."),
    ],
}

# las cadenas de armas (docs/MISIONES.md, la guía de PhalX): el arma vieja de un jefe, tres forjas con un objeto
# comprado o encontrado cada una; la última, MetalEtemon en Kulon Weapons
NEW_QUESTS += [
    (0x069, M(0x243), ITEM(0x069), "Kotemon: Katana Oxidada, Kotetsu,\nMasamune y Muramasa.", "Kotemon: Rusty Katana, Kotetsu,\nMasamune and Muramasa."),
    (0x077, M(0x21C), ITEM(0x077), "Kumamon: Guante Oxidado, Zodiaco,\nProminencia y Super Nova.", "Kumamon: Rusty Glove, Zodiac,\nProminence and Super Nova."),
    (0x083, M(0x252), ITEM(0x083), "Monmon: Rifle Oxidado, Justicia,\nJuicio y Castigo.", "Monmon: Rusty Rifle, Justice,\nJudgment and Punishment."),
    (0x090, M(0x226), ITEM(0x090), "Patamon o Renamon: de la Varita Vieja al Eternamente.", "Patamon or Renamon: from the Old Wand to the Eternally."),
    (0x09C, M(0x22B), ITEM(0x09C), "Agumon, Guilmon o Veemon: de la Garra Antigua al Invencible.", "Agumon, Guilmon or Veemon: from the Old Claw to the Invincible."),
]
SP = lambda i: 0x7000 | i  # una condición especial: un compañero a nivel 45
QUEST_STEPS.update({
    0x069: [
        (ITEM(0x066), M(0x243), "Vence a Zanbamon en la Catacumba: Katana Oxidada.", "Beat Zanbamon in the Catacomb: Rusty Katana."),
        (ITEM(0x063), None, "Compra el Onimaru en la Torre Delirios, Amaterasu.", "Buy the Onimaru in Delirium Tower, Amaterasu."),
        (ITEM(0x067), M(0x2A9), "Kabukimon forja el Kotetsu, Jungla de Éter, Amaterasu.", "Kabukimon forges the Kotetsu, Ether Jungle, Amaterasu."),
        (SP(0x4A), None, "Sube a Kotemon a nivel 45.", "Raise Kotemon to level 45."),
        (ITEM(0x064), M(0x2DA), "Consigue el Tenjinmaru, Magasta B1F.", "Get the Tenjinmaru, Magasta B1F."),
        (ITEM(0x068), M(0x2BF), "Yashamon forja el Masamune, Sala Secreta, Amaterasu.", "Yashamon forges the Masamune, Secret Room, Amaterasu."),
        (ITEM(0x065), None, "Compra el Acero Fujin en Ciudad Amaterasu.", "Buy the Fuujin Blade in Amaterasu City."),
        (ITEM(0x069), M(0x269), "MetalEtemon forja la Muramasa: la última de las cinco.", "MetalEtemon forges the Muramasa: the last of the five."),
    ],
    0x077: [
        (ITEM(0x074), M(0x21C), "Vence a Datamon en Ciudad Asuka: Guante Oxidado.", "Beat Datamon in Asuka City: Rusty Glove."),
        (ITEM(0x071), None, "Compra el Puño Kaiser en la Torre Delirios, Amaterasu.", "Buy the Kaiser Fist in Delirium Tower, Amaterasu."),
        (ITEM(0x075), M(0x2B0), "Nohemon forja el Zodiaco, Catacumbas, Amaterasu.", "Nohemon forges the Zodiac, Catacomb, Amaterasu."),
        (SP(0x4C), None, "Sube a Kumamon a nivel 45.", "Raise Kumamon to level 45."),
        (ITEM(0x072), None, "Compra el Brazo Tempest en Bai Hu o Xuan Wu.", "Buy the Tempest Arm in Bai Hu or Xuan Wu."),
        (ITEM(0x076), None, "Seehomon forja la Prominencia, bucea en Central Park.", "Seehomon forges the Prominence, dive in Central Park."),
        (ITEM(0x073), None, "Compra el Puño Raijin en Ciudad Amaterasu.", "Buy the Raijin Fist in Amaterasu City."),
        (ITEM(0x077), M(0x269), "MetalEtemon forja la Super Nova, pide Invencible y Eternamente.", "MetalEtemon forges the Super Nova, needs Invincible, Eternally."),
    ],
    0x083: [
        (ITEM(0x080), M(0x252), "Vence a HiAndromon, Fábrica Dum Dum: Rifle Oxidado.", "Beat HiAndromon, Dum Dum Factory: Rusty Rifle."),
        (ITEM(0x07D), None, "Compra el Cañón Racimo en la Torre Delirios, Amaterasu.", "Buy the Cluster Cannon in Delirium Tower, Amaterasu."),
        (ITEM(0x081), M(0x2AE), "Shurimon forja la Justicia, Lago SB Zhu Que.", "Shurimon forges the Justice, Zhu Que UG Lake."),
        (SP(0x4B), None, "Sube a Monmon a nivel 45.", "Raise Monmon to level 45."),
        (ITEM(0x07E), None, "Consigue el Cañón Láser.", "Get the Laser Cannon."),
        (ITEM(0x082), M(0x2C1), "Sagitarimon forja el Juicio, Desierto Mobius, Amaterasu.", "Sagittarimon forges the Judgment, Mobius Desert, Amaterasu."),
        (ITEM(0x07F), None, "Compra el Cañón Suijin en Ciudad Amaterasu.", "Buy the Suijin Cannon in Amaterasu City."),
        (ITEM(0x083), M(0x269), "MetalEtemon forja el Castigo, pide Invencible, Eternamente, Super Nova.", "MetalEtemon forges the Punishment, needs the three before."),
    ],
    0x090: [
        (ITEM(0x08D), M(0x226), "Vence a Pharaohmon en las Ruinas Protocolo: Varita Vieja.", "Beat Pharaohmon at the Protocol Ruins: Old Wand."),
        (ITEM(0x08A), None, "Compra la Khakkhara en la Torre Delirios, Amaterasu.", "Buy the Khakkhara in Delirium Tower, Amaterasu."),
        (ITEM(0x08E), M(0x295), "Ponchomon forja el Brillo, con Patamon o Renamon.", "Ponchomon forges the Twinkle, with Patamon or Renamon."),
        (ITEM(0x08B), None, "Compra la Vara Cristal en Bai Hu o Xuan Wu.", "Buy the Crystal Rod in Bai Hu or Xuan Wu."),
        (ITEM(0x08F), M(0x2CD), "Kongoumon forja el Inmaculado, compañero a nivel 45.", "Kongoumon forges the Immaculate, partner at level 45."),
        (ITEM(0x08C), None, "Compra el Milenio en Ciudad Amaterasu.", "Buy the Millenium in Amaterasu City."),
        (ITEM(0x090), M(0x269), "MetalEtemon forja el Eternamente, pide Invencible.", "MetalEtemon forges the Eternally, needs Invincible."),
    ],
    0x09C: [
        (ITEM(0x099), M(0x22B), "Vence a MasterTyranno en el Valle Tyranno: Garra Antigua.", "Beat MasterTyranno in Tyranno Valley: Old Claw."),
        (ITEM(0x096), None, "Compra el Colmillo Sable en la Torre Delirios, Amaterasu.", "Buy the Saber Fang in Delirium Tower, Amaterasu."),
        (ITEM(0x09A), M(0x2BE), "Shadramon forja el Vencer, con Agumon, Guilmon o Veemon.", "Shadramon forges the Vanquish, Agumon, Guilmon or Veemon."),
        (ITEM(0x097), None, "Compra el Colmillo Veneno en Bai Hu o Xuan Wu.", "Buy the Venom Fang in Bai Hu or Xuan Wu."),
        (ITEM(0x09B), None, "Depthmon forja la Destrucción, buceando, compañero a nivel 45.", "Depthmon forges the Destruction, diving, partner at level 45."),
        (ITEM(0x098), None, "Compra el Colmillo Oculto en Ciudad Amaterasu.", "Buy the Hidden Fang in Amaterasu City."),
        (ITEM(0x09C), M(0x269), "MetalEtemon forja el Invencible: la primera de las cinco.", "MetalEtemon forges the Invincible: the first of the five."),
    ],
    # las Botas Árbol (ya en el Digilog del port; sus pasos)
    0x004: [
        (F(0x1A, 0x22), M(0x220), "Un chico en la Capa de Conexión quiere su carta de Gabumon.", "A boy at Plug Cape wants his Gabumon card."),
        (F(0x1A, 0x23), None, "La carta está en la Posada de Asuka, abajo.", "The card is downstairs in the Asuka Inn."),
        (F(0x1A, 0x24), M(0x220), "Dásela: te manda a hablar con Veemon.", "Give it to him: he sends you to Veemon."),
        (F(0x1A, 0x25), None, "En la taberna de Asuka vieron a Veemon en la Pradera del Viento.", "At Asuka's tavern they saw Veemon on the Wind Prairie."),
        (F(0x1A, 0x26), M(0x229), "Encuentra a Veemon pateando árboles en el Bosque de las Patadas.", "Find Veemon by kicking trees in Kicking Forest."),
        (ITEM(0x004), M(0x229), "Veemon te da las botas para patear árboles.", "Veemon gives you the Tree Boots."),
    ],
})

# las 8 DDNA: el mismo patrón (docs/MISIONES.md)
DDNA = [  # DDNA, DRI y su mapa, su marca, la de su encargo, el Digimon y su mapa, la de su combate
    (0x006, "Johann", M(0x22B), 0x2D, 0, "MetalGreymon", M(0x233), 0x12),
    (0x022, "Jimmy", M(0x22E), 0x2E, 1, "WarGrowlmon", M(0x23C), 0x13),
    (0x027, "Kazuki", M(0x21D), 0x1F, 2, "MagnaAngemon", M(0x247), 0x14),
    (0x026, "Michael", M(0x21D), 0x20, 3, "Taomon", M(0x24A), 0x15),
    (0x013, "Ken", S(635), 0x2F, 4, "Kyukimon", M(0x241), 0x16),
    (0x18B, "Yuji", S(635), 0x30, 5, "GrapLeomon", M(0x23C), 0x18),
    (0x168, "Yang", S(635), 0x31, 6, "Armormon", M(0x228), 0x17),
    (0x023, "Bob", M(0x226), 0x29, 7, "Paildramon", M(0x234), 0x19),
]
for item, dri, dri_map, talk, ask, mon, mon_map, fight in DDNA:
    QUEST_STEPS[item] = [
        (F(0x1A, talk), dri_map, f"Habla con el DRI {dri}.", f"Talk to DRI {dri}."),
        (F(0x06, ask), dri_map, f"El DRI {dri} te pide que busques a {mon}.", f"DRI {dri} asks you to find {mon}."),
        (F(0x1C, fight), mon_map, f"Vence a {mon}.", f"Beat {mon}."),
        (ITEM(item), mon_map, f"{mon} te da su DDNA.", f"{mon} gives you its DDNA."),
    ]


def wrap(s, width=39):
    if "\n" in s:
        lines = s.split("\n")
    else:
        lines, cur = [], ""
        for w in s.split(" "):
            if cur and len(cur) + 1 + len(w) > width:
                lines.append(cur)
                cur = w
            else:
                cur = f"{cur} {w}" if cur else w
        lines.append(cur)
    if len(lines) > 2 or any(len(x) > width for x in lines):
        sys.exit(f"no cabe en dos líneas de {width}: {s!r}")
    return "\n".join(lines)


def texts():
    """Los textos de los pasos, en el orden de la tabla: (español, inglés) de cada paso de la historia, de cada misión
    nueva (su descripción) y de cada paso de misión"""
    out = []
    for _, steps in STORY:
        out += [(wrap(es), wrap(en)) for _, _, es, en in steps]
    out += [(wrap(es), wrap(en)) for _, _, _, es, en in NEW_QUESTS]
    for item in sorted(QUEST_STEPS):
        out += [(wrap(es), wrap(en)) for _, _, es, en in QUEST_STEPS[item]]
    return out


def main(first_text):
    """include/journal_steps.h; first_text: el índice de su primer texto en los del cooperativo"""
    rows, groups, quests, new = [], [], [], []
    t = first_text
    for steps_of, steps in STORY:
        start = len(rows)
        for code, where, _, _ in steps:
            rows.append((code, mode_of(where) if where else 0, t))
            t += 1
        for p in steps_of:
            groups.append((p, start, len(steps)))
    new_text = t
    for item, where, done, _, _ in NEW_QUESTS:
        new.append((item, mode_of(where), done, t))
        t += 1
    for item in sorted(QUEST_STEPS):
        start = len(rows)
        for code, where, _, _ in QUEST_STEPS[item]:
            rows.append((code, mode_of(where) if where else 0, t))
            t += 1
        quests.append((item, start, len(QUEST_STEPS[item])))
    h = ["/* Generated by tools/journal_steps.py: do not edit. The Digilog's steps, coop/patches ststatus/item_list.c. */",
         "",
         "typedef struct JournalStep { unsigned short code, mode, text; } JournalStep; /* code: checkCondition's, or 0xF000 | p: past step p */",
         "typedef struct JournalSteps { unsigned short key, first, count; } JournalSteps; /* a story step's or a side quest's, its item */",
         "typedef struct JournalQuest { unsigned short item, mode, done, text; } JournalQuest; /* a side quest the port's list hasn't */",
         "",
         "static const JournalStep JOURNAL_STEPS[] = {"]
    h += [f"    {{0x{c:04X}, 0x{m:03X}, {x}}}," for c, m, x in rows]
    h += ["};", "static const JournalSteps JOURNAL_STORY[] = {"]
    h += [f"    {{0x{p:02X}, {a}, {n}}}," for p, a, n in groups]
    h += ["};", "static const JournalSteps JOURNAL_QUEST_STEPS[] = {"]
    h += [f"    {{0x{i:03X}, {a}, {n}}}," for i, a, n in quests]
    h += ["};", "static const JournalQuest JOURNAL_NEW_QUESTS[] = {"]
    h += [f"    {{0x{i:03X}, 0x{m:03X}, 0x{d:04X}, {x}}}," for i, m, d, x in new]
    h += ["};", "/* the new side quests as rows of the port's SIDE_QUESTS, item, field, done, last step, item needed */",
          "#define JOURNAL_NEW_SIDE_QUESTS \\"]
    h += [f"    {{ 0x{i:03X}, 0x{m:03X}, 0x{d:04X}, 0, 0 }}, \\" for i, m, d, x in new]
    h += ["", ""]
    (NATIVE / "include" / "journal_steps.h").write_text("\n".join(h), encoding="utf-8", newline="\n")
    print(f"include/journal_steps.h: {len(rows)} pasos, {len(groups)} pasos de historia con lista, {len(quests)} misiones con pasos")


if __name__ == "__main__":
    texts()
    print("los textos los añade tools/coop_text.py; la tabla, también")
