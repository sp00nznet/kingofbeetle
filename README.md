# Mushiking: King of Beetles - Static Recompilation

Static recompilation of **Mushiking: The King of Beetles** (Sega Naomi arcade, 2006) from SH-4 binary to native x86-64 C code. This is not an emulator - the original game code is translated ahead-of-time into C functions that compile and run natively.

## Status

| Milestone | Status |
|-----------|--------|
| ROM decryption (M4 / 317-0437-com) | Done |
| ROM extraction (ISO9660 filesystem) | Done (88 dirs, 96MB assets) |
| SH-4 disassembly | Done (21,792 functions detected) |
| Static recompilation to C | Done (26,004 functions, 53 files, 185MB source) |
| Stub generation | Done (12,710 stubs) |
| First build (MSVC x64) | Done |
| SDL2 + OpenGL window | Done (640x480, OpenGL 3.3) |
| Entry point traced | Done (bootstrap 0x8C021000 → func_8C0898A0) |
| Maple DMA / JVS protocol | Done (MIE device info, reset, I/O commands) |
| SPG scanline timing | Done (VBlank simulation for game timing loops) |
| Memory allocator | Done (vtable dispatch, first-fit allocator) |
| Mid-function entry points | Done (8 entries extracted from dead code) |
| Game init completion | Done (init returns without crashes) |
| PVR2 rendering output | In progress (TA receives no geometry yet) |
| Flycast HW backend | Partially extracted (adapter API defined) |
| AICA sound | Not started |
| Card reader emulation | Stubbed (Flycast has barcodeInit() for Mushiking) |
| Deck window (card selection UI) | Stubbed (keyboard fallback working) |

## Current State

The game boots through the full Naomi initialization sequence:
1. SH-4 CPU state setup (SR, VBR, SP, FPSCR)
2. Maple bus scan (MIE/JVS device enumeration on 4 buses)
3. SPG timing synchronization (VBlank polling)
4. Heap allocator initialization (vtable-dispatched first-fit allocator)
5. Game init function returns to main loop

The allocator and dispatch table handle 26,012 function entries including 8 manually-extracted mid-function entry points (addresses inside recompiled functions that are called indirectly via vtables and constructor tables).

**Next milestone**: Getting geometry data into the PVR2 Tile Accelerator for first rendered frame.

## Game Info

- **Title**: Mushiking: The King of Beetles 4 (MKG TKOB 4)
- **Platform**: Sega Naomi (SH-4 + PowerVR2)
- **Regions**: JPN / USA / EXP / KOR / AUS (multi-region ROM)
- **ROM Version**: 4E (VER2.000)
- **Encryption**: Naomi M4 board, security chip 317-0437-com
- **Entry Point**: 0x8C021000 (bootstrap) → func_8C0898A0 (game main)
- **Load Address**: 0x8C020000 (Naomi, vs DC's 0x8C010000)
- **Code Size**: ~1.9MB across two regions (main + test mode)
- **Decryption Keys**: subkey1=0xC093, subkey2=0xA378

## Architecture

```
kingofbeetle/
├── dcrecomp/              # Framework submodule (SH-4 CPU, PVR2, JVS I/O)
├── src/
│   ├── main.c             # Naomi bootstrap (32MB RAM, entry point setup)
│   ├── card_reader.c      # Beetle card database + JVS card reader bridge
│   ├── deck_window.c      # Visual card selection UI (second window)
│   └── game/              # Auto-generated recompiled code
│       ├── game_code_000.c .. game_code_052.c  (26,004 functions)
│       ├── dispatch_table.c                     (indirect call lookup)
│       ├── game_stubs.c                         (12,710 stubs)
│       └── mid_function_entries.c               (8 manually-extracted entries)
├── include/game/
│   └── game_functions.h   # 38,714 function declarations
├── data/
│   ├── extracted/         # ROM-extracted assets (88 directories)
│   │   ├── CAUCASUS/      # Beetle 3D models (.NJ/.NJM)
│   │   ├── HERCULES/
│   │   ├── GIRAFFA/
│   │   └── ...
│   └── cards/             # Card data files (for deck window)
└── tools/
    └── extract_cards.py   # Card data extractor (TODO)
```

## Key Technical Discoveries

### Bootstrap Chain
The Naomi ROM entry point at 0x8C021000 is data-interleaved bootstrap code that the recompiler couldn't handle. Manually traced:
`0x8C021000 → 0x8C0210E8 → JMP 0x8C021150 → fill BIOS "SEGA" → clear BSS → JSR func_8C0898A0 → halt`

### Allocator Vtable
The game uses a vtable-dispatched memory allocator at `[0x8C2FB7A4]` pointing to ROM address 0x8C1733A4:
- vtable[0] = 0x8C0220EE (alloc - first-fit from free list)
- vtable[1] = 0x8C0221BC (free list insert)
- vtable[2] = 0x8C02241C (resize)
- vtable[3] = 0x8C022274 (query)

Both vtable entries are mid-function addresses (dead code after `return` in recompiled C). The alloc function was manually extracted (~60 lines) from the dead code section.

### Maple/JVS Protocol
Naomi uses Maple bus for JVS I/O board communication. Implemented MIE commands:
- 0x01 Device Info, 0x03 Reset, 0x82 JVS Get ID ("315-6149 SEGA")
- 0x80 Firmware Upload, 0x84 Self Test, 0x86 JVS I/O Commands

## Building

**Requirements**: CMake 3.16+, C11 compiler (MSVC/GCC/Clang)
**Optional**: SDL2, OpenGL 3.3 + GLEW (for graphical output)

```bash
git clone --recursive https://github.com/sp00nznet/kingofbeetle.git
cd kingofbeetle
mkdir build && cd build

# Headless (no SDL2 required):
cmake ..
cmake --build . --config Release -j 8

# With graphics (requires vcpkg with SDL2 + GLEW):
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release -j 8
```

## ROM Files

ROM files are not included. You need:
- `mushik4e.zip` containing `fpr-24417.ic8`, `fpr-24418.ic9`, `317-0437-com.ic3`

## License

Private repository.
