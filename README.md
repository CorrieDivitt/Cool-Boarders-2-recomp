# Cool Boarders 2 Recompiled

An unofficial native Windows version of **Cool Boarders 2** (PlayStation, 1997),
made by statically recompiling the game's code to C with
[PSXRecomp](https://github.com/RetroPortingToolKit/psxrecomp), then improving
graphics, controls and quality of life.

> **Bring your own disc.** This repository and its releases contain nothing
> from the game. You need your own legally obtained copy of the NTSC-U disc
> (SCUS-94358), dumped as BIN/CUE with all 21 tracks. The game is translated
> and compiled on your machine from that disc. See [LEGAL.md](LEGAL.md).

## Status: beta 0.2.0

The game is playable from boot to the finish line in every mode tested, with
music, video and memory card saves. This is a beta: expect rough edges, and
please report problems (see [Reporting problems](#reporting-problems)).

### What works

- **The full game runs natively.** Boot, menus, Freestyle, Board Park, Half
  Pipe, Big Air Master and Contest, Competition and two-player versus have all
  been played. 12 of the game's 14 code modules have been exercised in play.
- **CD audio and full-motion video** play as on the original.
- **Memory card saves** load and save.
- **Higher internal resolution.** Starts at 4x and goes up to 32x, limited by
  your graphics card.
- **Quick save and load** (F5 / Shift+F5) and **fast-forward** (Tab).
- **Launcher** (F10 or Ctrl+L in game) for video, controls and mods.

### Mods included

All are on the launcher's Mods page and can be switched off to get the
original game back.

| Mod | What it does |
|---|---|
| **Skip Intro** | Boots straight to the title menu, skipping the warning and logo screens. Pair it with Settings > Skip FMVs to skip the opening movie as well. |
| **Unlocks** | A tick box each for Boss, Gray, Snowman, the three special boards, all ten Freestyle courses, mirror mode, hard mode and the extra outfits. Your memory card is never changed: it keeps only what you really earned, so unticking an item takes it away again. Earning things normally still works. |
| **Trick Charge Meter** | A gauge that shows how far you have wound up a trick. Hold a direction while crouching: the arm towards it fills, left and right for the spin, up and down for the flip. Ten cells per arm, full after about 3.3 seconds; white beyond that shows the slower extra gain. Position and size options. Needs the OpenGL renderer (the default). |

The framework's own options are there too: CD drive speed, CRT and image
filters, and PGXP geometry correction.

### Known issues

- **Black screen of about 10 seconds before the title menu.** Not a hang: the
  game is reading the memory card and then loading the title menu at the
  original 2x drive speed.
- **Thin seams between snow polygons.** They are in the original geometry and
  higher resolutions make them easier to see.
- **30 frames per second.** The game's logic runs at 30 Hz. 60 fps needs frame
  interpolation and is planned.
- **Not yet reached in testing:** the half-pipe stage of Competition and the
  ending.
- **Quick saves and the Unlocks mod.** Quick saves made with Unlocks switched
  off will not load while it is on, and the other way round. Memory card saves
  are not affected.

### Planned

Widescreen, 60 fps, analog steering and modern controller support, full
rebinding, and online versus.

## Installing (players)

1. Install [Python 3](https://www.python.org/downloads/).
2. Download the self-compile kit zip from the
   [Releases](https://github.com/CorrieDivitt/Cool-Boarders-2-recomp/releases)
   page and extract it to a folder of its own.
3. Run `CoolBoarders2_Recompiled.exe` and choose your disc's `.cue` file when
   asked.
4. Follow the **Generate & rebuild** wizard. It translates the game from your
   disc and compiles it on your machine. The first time, it downloads the
   build tools it needs. This takes a few minutes and happens once.

The kit contains no game files, no game code and no retail BIOS. It uses the
bundled OpenBIOS.

## Building (developers)

Requirements: Git for Windows, Python 3, and the PSXRecomp Windows toolchain
(clang, CMake, Ninja). See `psxrecomp/docs/GETTING_STARTED.md`.

```powershell
git clone --recurse-submodules https://github.com/CorrieDivitt/Cool-Boarders-2-recomp.git
cd Cool-Boarders-2-recomp
powershell -File tools\install_hooks.ps1        # required: installs the IP guard hooks

# Translate the game from YOUR disc (output goes to generated/, which is never committed)
python psxrecomp\psxrecomp_cli.py generate --config game.toml --project-root . --disc "D:\path\to\Cool Boarders 2 (USA).cue"

# Build
python psxrecomp\psxrecomp_cli.py rebuild --config game.toml --project-root . --build-dir build-release --no-pgo
```

`game.toml` holds the path to the developer's disc; point it at yours.

To build the release kit:
`powershell -ExecutionPolicy Bypass -File tools\build_selfcompile_kit.ps1`.
It builds the setup host (the runtime with no game code), packages it and
audits the zip with the IP guard.

### Repository layout

| Path | Contents |
|---|---|
| `src/mods/` | The mods' source code (original code; addresses and fingerprints only) |
| `mods/preloaded/packages/` | Mod manifests: names, options, descriptions |
| `docs/ENGINE_NOTES.md` | What is known about the game's engine: memory layout, modules, unlocks, controls, trick physics |
| `symbols.toml` | Names for known functions |
| `tools/ip_guard.py` | Keeps anything from the disc out of Git and releases |
| `psxrecomp/`, `recomp-ui/` | Framework and launcher (Git submodules) |

## Reporting problems

Open an issue on GitHub with what you were doing, what happened and, if the
game froze, the `psx_freeze_dump_*.json` and `psx_last_run_report.json` files
from the game folder. **Never attach disc images, game files, memory card
files, save states or screenshots of copyrighted game content.**

## Contributing

Never commit anything from the disc. The hooks and the IP guard CI check block
it, but read [LEGAL.md](LEGAL.md) first. Notes, symbol names, addresses and
original code are welcome.

## Licence

This project's own code is licensed under the
[PolyForm Noncommercial License 1.0.0](LICENSE), matching the PSXRecomp
framework: free to use, share and modify, not for commercial use.

## Credits

- [PSXRecomp](https://github.com/RetroPortingToolKit/psxrecomp) and
  [recomp-ui](https://github.com/RetroPortingToolKit/recomp-ui) (their own licences apply)
- OpenBIOS (MIT), bundled with PSXRecomp

Cool Boarders 2 was developed by UEP Systems. This project is not affiliated
with or endorsed by Sony Interactive Entertainment or UEP Systems.
