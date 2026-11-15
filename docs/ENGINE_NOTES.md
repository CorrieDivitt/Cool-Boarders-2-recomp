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
| 0x800570E4-0x80057120 | Progress and session block (see "Progress and unlocks") |

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

## Progress and unlocks
INIT resets a block of 16-bit fields at boot; the memory card load then overwrites
the saved part. `SavePack` (0x80018CC0, one caller at 0x80018898) copies
0x800570F4-0x8005710D into the save buffer, followed by the button configuration at
0x80057FB8 and 0x80057FC2; `SaveUnpack` (0x80019154, one caller at 0x80018FE8)
copies it back after checking a length word of 0x998 and a checksum.

| Address | Meaning | Saved |
|---|---|---|
| 0x800570E0 | Selected game mode (word). 4 is set for the attract demo; 0 is probably Competition | no |
| 0x800570EA | Extra outfits switched on (set by the outfit button code) | no |
| 0x800570EC | Hard mode active for this run | no |
| 0x800570EE | Mirror mode active for this run | no |
| 0x800570F2 | Set to 9 at reset and to 10 when hard mode is chosen (probably the Competition course count) | no |
| 0x800570FC | Freestyle courses offered: 3, then 10 | yes |
| 0x800570FE | Mirror Freestyle courses offered: 0, 3, then 10 | yes |
| 0x80057100 | Rider mask. Bits 0-3 the starting riders (reset value 0x000F), bit 4 Boss, bit 5 Gray, bit 6 Snowman | yes |
| 0x80057102 | Special board mask. Bit 0 from trick records, bit 1 from total-score records, bit 2 from time records | yes |
| 0x80057104 | Mirror mode available | yes |
| 0x80057106 | Hard mode available | yes |
| 0x80057108 | Per-course mask: first place in time | yes |
| 0x8005710A | Per-course mask: first place in the second record table | yes |
| 0x8005710C | Per-course mask: first place in the third record table | yes |
| 0x80057112 / 0x80057114 | Board editor selection (-1 when none) | no |
| 0x80057120 | Competition finishing place (0 is first) | no |

Where each is earned:
- ENDING (0x8005CECC): finishing first in mirror Competition sets rider bit 4.
  Finishing first outside mirror mode once hard mode is available sets bit 6.
  Finishing in the top three sets 0x80057104, 3 mirror courses and all 10 courses
  the first time, and in mirror mode sets 0x80057106 and all 10 mirror courses.
- Half pipe (HP 0x80089538): a score of 376 or more sets rider bit 5. Big Air
  Master (TM 0x80089108) sets it when its result value is zero (by the published
  condition, every trick landed).
- Freestyle records (FREE 0x8008A094; the same code is in VS, BP and COMB): a
  special board bit is set when the matching per-course mask reaches five courses,
  and rider bit 6 when all three masks hold all ten. The check is skipped in
  mirror mode. Which of the second and third record tables is trick points and
  which is total score is inferred from the table order, not confirmed in play.
- Per course there are three record tables of three entries, at
  0x80057664 + course * 0xC0 + 0x0C (time, lower is better), + 0x30 and + 0x54
  (higher is better).

Only the menus read these fields to decide what to offer: MAIN_SEL builds the
rider list from 0x80057100 (0x80063AE8), tests board bits from 0x80057102 and
takes the course count from 0x800570FC or 0x800570FE; MODE_SEL tests 0x80057104
with R1 held (mirror) and 0x80057106 with L1 held on Competition (hard). In game
mode 0 MAIN_SEL zeroes 0x80057102 for as long as it runs and writes its own
copy back on the way out.

MODE_SEL's menu loop calls four button-code detectors each frame (0x8005CA78,
0x8005CE90, 0x8005CC60, 0x8005D078). 0x8005CC60 matches the outfit code and sets
0x800570EA; choosing the attract demo clears it. 0x8005CA78 opens all ten courses
(0x800570FC = 10) when a shoulder button is held after its sequence; 0x8005CE90
does something similar through a pointer that has not been traced.

## Controller
- `PadRead` (0x8001B320, player, analog out) returns the buttons as one 16-bit
  word, pressed = 1: Up 0x1000, Right 0x2000, Down 0x4000, Left 0x8000,
  Select 0x0100, Start 0x0800, L2 0x01, R2 0x02, L1 0x04, R1 0x08,
  Triangle 0x10, Circle 0x20, Cross 0x40, Square 0x80.
- Gameplay modules combine both pads into one word (player 2 in the high half;
  FREE 0x800A2B70) and test it through a table of 16 action masks (FREE
  0x8009DE1C: up, down, left, right, then the face and shoulder actions; jump is
  the entry at +0x0A, Cross by default).
- 0x80057FC2 is the saved button configuration, not live pad state.

## Rider object and trick wind-up
Each gameplay module keeps two rider pointers (the second is used in two-player
mode): FREE 0x800A39C8, VS 0x800A3260, HP 0x8009B878, BP 0x8009FA4C,
TM 0x8009D978, OMT 0x8009BD38, SBC1 0x80090A00, SBC2 0x8009B590, COMB 0x800A5B10.
The rider object is on the heap and has the same layout in all nine modules.

| Offset | Meaning |
|---|---|
| +0x01A | Jump state: 2 while crouching with jump held, 3 on release |
| +0x028 / +0x080 | Buttons this frame / previous frame |
| +0x744 | Flip wind-up: +1 per frame holding up, -1 holding down, while crouching |
| +0x748 | Spin wind-up: +1 per frame holding right, -1 holding left, while crouching |
| +0x754 | Frames the jump button has been held |

Releasing a direction resets its count. At take-off a count that is not zero is
raised to at least 30 (FREE 0x80068960). In the air the spin speed uses count / 10
up to 100 and the integer square root (0x8001F8AC) beyond; the flip uses the
square root throughout (FREE 0x80067C7C, 0x80067DC0). Both are cleared on landing.
The rider update function (rider in a0, called once per rider per game frame)
is at FREE 0x80066DF0, VS 0x8006624C, HP 0x80065104, BP 0x80064F38, TM 0x80064FCC,
OMT 0x80064FF8, SBC1 0x80065290, SBC2 0x80066FB0, COMB 0x80065F5C. The wind-up
counts are not cleared when a run ends, so stale values stay in the rider object
through the results screen.

## Cheat addresses (from a combined EU/US code list), checked against NTSC-U
| Address | Meaning | Result |
|---|---|---|
| 0x80057100 | Character unlocks | Confirmed: rider mask, see "Progress and unlocks" |
| 0x80057102 | Board unlocks | Confirmed: special board mask |
| 0x8009A868 | Half pipe score | Confirmed: referenced by HP and COMB |
| 0x8009103C | Race placement | Confirmed: referenced by SBC2 |
| 0x80130994 | Checkpoint timer | Not verifiable statically (heap memory) |

## Not yet known
- Confirming SBC1, SBC2 and COMB as the Competition stages.
- File formats (.FBD, .PMD, .DAT, .NPC, .DEM).
- Projection and culling sites for widescreen.
