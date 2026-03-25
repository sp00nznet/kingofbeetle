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
| Headless execution | Done (300 frames, clean shutdown) |
| SDL2 + OpenGL window | Done (640x480, OpenGL 3.3 on RTX 5070) |
| Entry point traced | Done (bootstrap→func_8C165F74) |
| Flycast HW backend | In progress (subsystems extracted, adapter API defined) |
| PVR2 rendering via Flycast | Next up |
| AICA sound via Flycast | Not started |
| JVS I/O / input mapping | Stubbed (Flycast JVS protocol available) |
| Card reader emulation | Stubbed (Flycast has barcodeInit() for Mushiking) |
| Deck window (card selection UI) | Stubbed (keyboard fallback working) |

## Game Info

- **Title**: Mushiking: The King of Beetles 4 (MKG TKOB 4)
- **Platform**: Sega Naomi (SH-4 + PowerVR2)
- **Regions**: JPN / USA / EXP / KOR / AUS (multi-region ROM)
- **ROM Version**: 4E (VER2.000)
- **Encryption**: Naomi M4 board, security chip 317-0437-com
- **Entry Point**: 0x8C021000 (bootstrap) → func_8C165F74 (game init)
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
│       └── game_stubs.c                         (12,710 stubs)
├── include/game/
│   └── game_functions.h   # 38,714 function declarations
├── data/
│   ├── extracted/         # ROM-extracted assets (88 directories)
│   │   ├── CAUCASUS/      # Beetle 3D models (.NJ/.NJM)
│   │   ├── HERCULES/
│   │   ├── GIRAFFA/
│   │   ├── CARD/          # Card data
│   │   ├── SOUND_DATA/    # Audio
│   │   ├── PVR_*/         # Textures
│   │   └── ...
│   └── cards/             # Card data files (for deck window)
└── tools/
    └── extract_cards.py   # Card data extractor (TODO)
```

## Recompilation Pipeline

```bash
# 1. Decrypt ROM (M4 encryption, subkeys from 317-0437-com.ic3)
python dcrecomp/tools/naomi_m4_decrypt.py --zip mushik4e.zip data/extracted/decrypted_ic8.bin

# 2. Extract filesystem from decrypted ROM
python dcrecomp/tools/extract_naomi_rom.py data/extracted/decrypted_ic8.bin data/extracted

# 3. Extract game executable (first 8MB = decrypted code)
# (handled by step 1 output)

# 4. Disassemble
python dcrecomp/tools/sh4_disasm.py data/extracted/1ST_READ.BIN --base 0x8C020000

# 5. Recompile to C
python dcrecomp/tools/static_recompile.py data/extracted/1ST_READ.BIN src/game include/game --base 0x8C020000

# 6. Generate stubs
python dcrecomp/tools/generate_stubs.py

# 7. Build
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release -j 8
```

## Card Reader

The original arcade machine uses a physical card reader where players scan beetle cards. The recomp simulates this with a **deck window** - a separate SDL2 window showing available beetle cards. Click a card to "insert" it.

Known beetles from ROM data:
Caucasus, Hercules, Hercules Blue, Giraffe Stag, Grandis Stag, Five-Horned, Atlas, Centaurus, Didier's Stag, Fabricius' Stag, Feisthamel's Stag, Haster's Stag, Imperial Stag, Burmese Stag, Japanese Rhino, Hime, Hirata's Stag, Hosoaka, Firefly, Blue Swallowtail, Anubis, Heraduno, and more.

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
