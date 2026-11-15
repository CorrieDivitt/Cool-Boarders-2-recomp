# Cool Boarders 2 engine notes (NTSC-U, SCUS-94358)

Facts found by static analysis of a legally owned disc. Addresses and
measurements only; nothing here is copied from the game. Labels marked
*guessed* in `symbols.toml` are working names, not confirmed behaviour.

## Disc
- 21 tracks: 1 data (MODE2/2352) + 20 CD audio. 345 files on the data track.
- Boot file `SCUS_943.58`, loaded at 0x80010000, size 0x40800, entry 0x800120FC,
  stack 0x801FFFF0. SYSTEM.CNF sets TBC=4, EVENT=16, STACK=801FFF00.
- NTSC-U carries the `97/9/17 9:44:27` build string (EXE offset 0x820).
- PsyQ library version tags dated December 1996.

## Memory layout
| Range | Contents |
|---|---|
| 0x80010000-0x80050800 | Boot EXE code and data |
| 0x80050800-0x8005AB68 | EXE BSS (cleared at boot up to 0x8005AB68) |
| 0x8005AB68-~0x800A6000 | Overlay slot: one mode module at a time |
| 0x80057FC2-0x80057FDA | Controller state block (inside BSS) |
| 0x80057100 / 0x80057102 | Character / board unlock flags (cheat addresses, confirmed referenced) |

## Overlays (code modules)
Fourteen top-level `.BIN` files. All load at **0x8005AB68**; the first word is a
small module number (4 to 8) and the entry point is at **0x8005AB6C**.
`OverlayBootLoop` (0x800121B8) loads a module and calls its entry. The main EXE
names only `INIT.BIN`; the table of the other 13 file names lives in INIT.BIN.

| Module | Size | Ends at | Notes |
|---|---|---|---|
| INIT | 19,496 | 0x8005F790 | Boot: system and save setup, intro screens; holds the module name table |
| MAIN_SEL | 139,850 | 0x8007CDB2 | Menus (title, options), 512x240 display |
| MODE_SEL | 164,120 | 0x80082C80 | Mode select menus, 512x240 display |
| BDEDIT | 34,464 | 0x80063208 | Board editor (confirmed in play) |
| ENDING | 108,172 | 0x800751F4 | Ending, 512x240 display; not yet reached in play |
| FREE | 299,608 | 0x800A3DC0 | Gameplay: Freestyle (confirmed in play) |
| VS | 297,720 | 0x800A3660 | Gameplay: two-player versus (confirmed in play) |
| HP | 266,504 | 0x8009BC70 | Gameplay: half pipe (confirmed in play; score at 0x8009A868) |
| BP | 283,488 | 0x8009FEC8 | Gameplay: Board Park (confirmed in play) |
| TM | 291,432 | 0x800A1DD0 | Gameplay: Big Air Master (confirmed: a Big Air Master-only session loaded only TM) |
| OMT | 267,712 | 0x8009C128 | Gameplay: Big Air Contest (by elimination) |
| SBC1 | 221,832 | 0x80090DF0 | Gameplay: Competition (likely) |
| SBC2 | 265,768 | 0x8009B990 | Gameplay: Competition (likely; uses race placement at 0x8009103C) |
| COMB | 308,128 | 0x800A5F08 | Gameplay: probably the half-pipe stage of competition mode (uses the half-pipe score); not yet reached in play |

Coverage (overlay capture, 15 Nov 2026): 12 of 14 modules reached in play; COMB and
ENDING not yet.

Overlay addresses repeat across modules, so overlay functions are recorded here
per module rather than in `symbols.toml`.

## Frame timing
- `VSync` is at 0x8003BD60 (PsyQ libetc).
- Every gameplay module has a frame loop that ends in `VSync(interval)`, where
  `interval` is a module variable set once to **2** by a small setter. The setter
  ignores negative values and **refuses 1**. Gameplay therefore runs at
  **30 fps** on NTSC, and game logic is stepped once per 30 Hz frame.
- Frame-loop functions: FREE 0x8005D4BC, VS 0x8005D8BC, HP 0x8005D170,
  BP 0x8005D1AC, TM 0x8005D28C, OMT 0x8005D1CC, SBC1 0x8005D1C4,
  SBC2 0x8005D744 and 0x8006CEA8, COMB 0x8005D774.
- Menus call `VSync(0)` directly. Transitions wait 3 vblanks (`ModeTransition`, 0x800142F0).
- Implication for the 60 fps work: game logic must keep stepping at 30 Hz.
  Rendering extra frames needs interpolation, not a halved interval.

## Display
- Gameplay: 320x240 (`DisplayInit320x240`, 0x80012770, called by every module).
- Menus and ending: 512x240 (`DisplayInit512x240`, 0x8002EE40).

## Input and memory card
- `InputInit` 0x8001B228 calls the PadInit wrapper (0x8003CC78, BIOS InitPAD)
  and sets up the state block at 0x80057FC2.
- `InputRead` 0x8001B64C reads the pads into that block. This is where
  proportional analog steering would be fed in.
- `MemCardInit` 0x8001787C wraps BIOS InitCARD/StartCARD.

## Cheat addresses (from a combined EU/US code list), checked against NTSC-U
| Address | Meaning | Result |
|---|---|---|
| 0x80057100 | Character unlocks | Confirmed: referenced by most modules |
| 0x80057102 | Board unlocks | Confirmed: referenced by most modules |
| 0x8009A868 | Half pipe score | Confirmed: referenced by HP and COMB |
| 0x8009103C | Race placement | Confirmed: referenced by SBC2 |
| 0x80130994 | Checkpoint timer | Not verifiable statically (heap memory) |

## Not yet known
- Confirming SBC1, SBC2 and COMB as the Competition stages.
- File formats (.FBD, .PMD, .DAT, .NPC, .DEM).
- Projection and culling sites for widescreen.
