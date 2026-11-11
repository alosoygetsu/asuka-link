# Credits

## The co-op fork

This is **Asuka Link**, **alosoygetsu**'s co-op fork ([alosoygetsu/asuka-link](https://github.com/alosoygetsu/asuka-link),
branch `coop`): playing together online, card trades, battles between players and boss raids, the free team and the
character creator, on top of the native port. None of it would exist without:

- **Feerdus**, whose [native port of Digimon World 2003](https://github.com/Feerdus95/dw3-native-port) (MIT License,
  © 2026 Feerdus) is the game this fork runs: the PC platform layer, the launcher, the options and every mod. Asuka Link
  is based on its v0.18.9: it keeps the port's patches as they are, adds its own in `native/coop/`, and has been
  maintained on its own since v1.0.0.
- **ReGame-Labs and juandav**, for [dw3_decomp](https://github.com/ReGame-Labs/dw3_decomp), the decompiled game both
  the port and the co-op are built from.
- Everyone credited below, whose work the port stands on.

The co-op has its own versions from v1.0.0, apart from the port's; each release names the port release it is built on.

## The native port

The native port of Digimon World 2003 builds on the work of others:

- **[dw3_decomp](https://github.com/ReGame-Labs/dw3_decomp)** (MIT License, Copyright (c) 2026 juandav, ReGame-Labs). It is the
  decompiled C of the game that this port compiles for the PC, with our patches in `native/patches/`. Without it there
  would be no port.
- **[Dmw2003FastText](https://github.com/markisha64/Dmw2003FastText)** (MIT License, by markisha64). The idea of
  Fast text: the text boxes reading X held as well as pressed, outside battles.
- **[ddw3](https://github.com/markisha64/ddw3)** (markisha64, Zenithsiz). A decompilation of the game, whose
  `mklang-file` encodes the extra starter packs' texts in the game's own text format (`tools/packs_text.py`).
- **[Flawe's Mod](https://www.romhacking.net/hacks/9376/)** (by Flawe, on Romhacking). The idea of Fast travel: no part
  of it is copied, the port's goes through the game's own doors.
- **[dw2_decomp](https://github.com/Wyrelade/Digimon-World-2-Decomp)** (CC0, by Wyrelade and its contributors). The
  decompiled Digimon World 2 that Flamedramon's model was read through.
- **[dcb_decomp](https://github.com/ReGame-Labs/dcb_decomp)** (MIT License, ReGame-Labs). The decompiled Digimon Digital
  Card Battle that Flamedramon's attacks were read through.
- **Pixel Digivolve** (by Neale Davidson, Pixel Sagas). The launcher's font, baked into `dw3launcher.exe`; its license
  is in `licenses/`.
- **[SDL 3](https://libsdl.org)** (zlib License, Copyright (C) 1997-2026 Sam Lantinga). The window, input and audio.
  `SDL3.dll` ships as it comes.
- **[stb](https://github.com/nothings/stb)** (public domain / MIT, by Sean Barrett and its contributors). `stb_image`
  and `stb_image_write`, in `platform/stb/`: the JPEG of the Digilog's pictures of the fields.
- **[Xelu's free controller prompts](https://thoseawesomeguys.com/prompts/)** (CC0, by Nicolae "Xelu" Berbece, in
  the SVG edition by [haaldor](https://github.com/haaldor/Xelu_prompts_SVG)). The button and key icons of the options
  menu and of the game's texts, scaled down by `tools/prompts.py`, in `prompts/` by the exe with their license.
- **[psx-spx](https://psx-spx.consoledev.net)** (the PlayStation specifications, after Martin Korth's nocash docs).
  The hardware behaviour the platform layer reproduces, including two data tables of the chips: the SPU's Gaussian
  interpolation weights and its reverb resampling filter.

PlayStation is a trademark of Sony Interactive Entertainment, Xbox of Microsoft and Nintendo Switch of Nintendo; the
icons only show which button to press, and the project has no relation to them.

Digimon World 2003 is © Bandai / Bandai Namco. This project carries no game data: you supply your own disc
(see `README.md`). The game's own tables the decompiled C carries are taken out of the exe when it is built and
read at run time from your own copy, as are the reverb presets.
