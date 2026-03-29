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
| TMU hardware timers | Done (TCNT0/1/2 real-time countdown) |
| SH-4 division routines | Done (DIV0U/DIV1 replaced with C division) |
| Memory allocator | Done (vtable dispatch, first-fit allocator) |
| Mid-function entry points | Done (26 entries extracted from dead code) |
| Game init completion | Done (0 unresolved calls, 6 heap allocations) |
| Game main loop | **Running at 60fps** |
| PVR2 rendering output | In progress (PVR not yet configured) |
| AICA sound | Not started |
| Card reader emulation | Stubbed |
| Deck window (card selection UI) | Stubbed (keyboard fallback working) |

## Current State

**The game runs its main loop at a stable 60fps** in an SDL2/OpenGL window. Each frame calls the game's frame processing function (func_8C1657AC) and render function (func_8C091AD4). The SDL window is responsive and handles input events.

The rendering pipeline is not yet producing visible output because the PVR2 hardware registers haven't been configured by the game's initialization code. Several mid-function entry points in the PVR init path are stubbed, preventing the Tile Accelerator and framebuffer from being set up.

### What works:
- Full Naomi initialization sequence (BIOS state, Maple/JVS, timers, allocator)
- 26,030-entry dispatch table with binary search (26,004 recompiled + 26 manual entries)
- Vtable-dispatched heap allocator (first-fit, 6 successful allocations totaling ~600KB)
- TMU hardware timer countdown for frame timing
- C replacements for SH-4 software division routines (DIV0U/DIV1)
- Maple DMA with MIE/JVS protocol (device enumeration, I/O commands)
- SPG scanline counter for VBlank timing
- Game main loop with SDL event integration at 60fps

### What's next:
- Configure PVR2 registers (framebuffer, video mode, TA parameters) so the game can submit geometry
- Implement remaining mid-function entries for PVR initialization
- Enable Store Queue → TA FIFO path for geometry submission
- First rendered frame

## Game Info

- **Title**: Mushiking: The King of Beetles 4 (MKG TKOB 4)
- **Platform**: Sega Naomi (SH-4 + PowerVR2)
- **Regions**: JPN / USA / EXP / KOR / AUS (multi-region ROM)
- **ROM Version**: 4E (VER2.000)
- **Encryption**: Naomi M4 board, security chip 317-0437-com
- **Entry Point**: 0x8C021000 (bootstrap) → func_8C0898A0 (game main)
- **Load Address**: 0x8C020000 (Naomi, vs DC's 0x8C010000)
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
│       └── mid_function_entries.c               (26 manually-extracted entries)
├── include/game/
│   └── game_functions.h   # 38,714 function declarations
└── data/
    └── extracted/         # ROM-extracted assets (88 directories)
```

## Key Technical Discoveries

### Mid-Function Entry Points
The recompiler creates dead code after `return` statements when a function has multiple entry points (common in SH-4 with JMP/BSR patterns). 26 addresses needed manual extraction into standalone callable wrappers, including the memory allocator, division routines, and game loop entry.

### Allocator Vtable
The game uses a vtable-dispatched memory allocator at `[0x8C2FB7A4]` → ROM vtable at 0x8C1733A4 with function pointers for alloc/free/resize. The vtable data is protected from corruption by the game's BIOS fill pattern.

### SH-4 Division
The recompiler didn't handle DIV0U (0x0019) and DIV1 (0x3204) instructions, emitting them as `/* UNKNOWN */` comments. The software divide routines were replaced with C implementations using native integer division.

### Game Loop Architecture
The game's main loop (at 0x8C089A5C, dead code inside func_8C089A34) calls frame processing and render functions, then dispatches callbacks via a task system. The loop continuation functions (at 0x8C089B92/B98) are tail calls back to the loop entry.

## Building

**Requirements**: CMake 3.16+, C11 compiler (MSVC/GCC/Clang)
**Optional**: SDL2, OpenGL 3.3 + GLEW (for graphical output)

```bash
git clone --recursive https://github.com/sp00nznet/kingofbeetle.git
cd kingofbeetle
mkdir build && cd build

# With graphics (requires vcpkg with SDL2 + GLEW):
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release -j 8
```

## ROM Files

ROM files are not included. You need:
- `mushik4e.zip` containing `fpr-24417.ic8`, `fpr-24418.ic9`, `317-0437-com.ic3`

## License

Private repository.
