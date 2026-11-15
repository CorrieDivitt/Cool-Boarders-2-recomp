# Cool Boarders 2 Recompiled

An unofficial native Windows version of **Cool Boarders 2** (PlayStation, 1997),
made by statically recompiling the game's code to C with
[PSXRecomp](https://github.com/RetroPortingToolKit/psxrecomp), then improving
graphics and controls.

> **Bring your own disc.** This repository contains nothing from the game.
> You need your own legally obtained copy of the NTSC-U disc (SCUS-94358),
> dumped as BIN/CUE with all 21 tracks. The game is translated and built on
> your machine from that disc. See [LEGAL.md](LEGAL.md).

## Status

Early development. The unmodified game boots, plays races with music and video,
and runs without crashing in testing. Enhancements (widescreen, higher frame
rate, analog steering, online versus) are planned.

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

## Contributing

Never commit anything from the disc. The hooks and the IP guard CI check block
it, but read [LEGAL.md](LEGAL.md) first. Notes, symbol names, addresses and
original code are welcome.

## Credits

- [PSXRecomp](https://github.com/RetroPortingToolKit/psxrecomp) and
  [recomp-ui](https://github.com/RetroPortingToolKit/recomp-ui) (their own licences apply)
- OpenBIOS (MIT), bundled with PSXRecomp

Cool Boarders 2 was developed by UEP Systems. This project is not affiliated
with or endorsed by Sony Interactive Entertainment or UEP Systems.
