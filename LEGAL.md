# Legal

This project is an unofficial, non-commercial static recompilation toolkit for
**Cool Boarders 2** (PlayStation, 1997). It is not affiliated with, endorsed by
or sponsored by Sony Interactive Entertainment, UEP Systems or any rights holder.
Game titles and trademarks are used only to identify the game the tools work with.

## What this repository contains

Only original work: build configuration, scripts, notes, and code written for
this project, plus the PSXRecomp framework and recomp-ui as Git submodules
(their own licences apply).

It contains **nothing from the game disc**: no disc images, no extracted files,
no game code (original or translated), no compiled game executable, no
textures, models, music, sound, video, text, screenshots or box art.

Some files record *facts* about the disc so the tools can recognise a correct
copy: checksums, track counts, memory addresses and function start addresses.
These identify the disc; they do not reproduce any of it.

## Bring your own disc

You must supply your own legally obtained copy of the game, dumped yourself
from a disc you own. The tools translate and compile the game **on your
machine**, from your disc. The output stays on your machine; do not share it.

## How this is enforced

- `.gitignore` excludes disc images, extracted files, generated (translated)
  C, overlay captures, caches, saves, BIOS dumps, executables and box art.
- `tools/ip_guard.py` scans by path and by content (PlayStation executable
  headers, CD sector patterns, ISO 9660 signatures, translated-code markers,
  game build strings, the boot executable's checksum, oversized files).
- A pre-commit hook and a pre-push hook run it on every commit and push
  (install once with `sh tools/install_hooks.sh` or
  `powershell -File tools\install_hooks.ps1`).
- The **IP guard** GitHub Actions workflow runs it on every push and pull
  request, over the current tree and the whole history.
- Release packages are audited with `python tools/ip_guard.py --release <zip>`
  and must not contain a compiled game executable.

## BIOS

Builds use the bundled OpenBIOS (MIT). Retail PlayStation BIOS dumps are never
included or distributed.
