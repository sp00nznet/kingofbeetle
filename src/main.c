/**
 * Mushiking: King of Beetles - Static Recompilation
 *
 * Bootstrap and main loop for the statically recompiled Naomi game.
 * Uses dcrecomp framework for SH-4 CPU, PVR2 GPU, and JVS I/O.
 */

#include "recompiler/sh4_cpu.h"
#include "hal/dc_hardware.h"
#include "hal/naomi_io.h"
#include "hal/pvr2.h"
#include "platform/platform.h"
#include "card_reader.h"
#include "deck_window.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Game binary info */
#define GAME_TITLE      "Mushiking: King of Beetles"
#define NAOMI_RAM_SIZE  DC_RAM_SIZE_32MB  /* Naomi has 32MB RAM */
#define GAME_ENTRY_ADDR 0x8C020000        /* Naomi load address */

/* Forward declarations for recompiled game entry points */
#include "game/game_functions.h"

/* The ROM entry point at 0x8C021000 is BIOS-level init code that:
 * 1. Writes to hardware regs (PVR soft reset, cache control)
 * 2. Sets SR=0x700000F0 (supervisor mode, interrupts masked)
 * 3. Sets VBR=0x8C00F400 (vector base register)
 * 4. Sets R15=0xAC00FC00 (stack pointer, uncached)
 * 5. Sets FPSCR=0x00040000 (DN mode)
 * 6. Calls func_8C165F74 (the actual game init function)
 * We replicate this setup in naomi_bios_init() and call the game directly. */

/* Indirect dispatch (used by recompiled code) */
void sh4_call_indirect(SH4CPU *cpu);
void sh4_jump_indirect(SH4CPU *cpu);

/* Global state */
static SH4CPU g_cpu;
static DCHardware *g_hw = NULL;
static NaomiIO *g_naomi_io = NULL;

/**
 * Load game binary into CPU RAM.
 * For Naomi, this loads the extracted 1ST_READ.BIN at the standard address.
 */
static int load_game_binary(SH4CPU *cpu, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "ERROR: Could not open game binary: %s\n", path);
        return -1;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    /* Load at Naomi game address (0x8C020000) */
    uint32_t load_offset = GAME_ENTRY_ADDR & cpu->ram_mask;
    printf("[BOOT] Loading at RAM offset 0x%08X (addr 0x%08X)\n",
           load_offset, GAME_ENTRY_ADDR);

    if (load_offset + size > cpu->ram_size) {
        fprintf(stderr, "ERROR: Binary too large (%ld bytes) for RAM\n", size);
        fclose(f);
        return -1;
    }

    size_t read = fread(cpu->ram + load_offset, 1, size, f);
    fclose(f);

    printf("[BOOT] Loaded %s: %zu bytes at 0x%08X\n", path, read, GAME_ENTRY_ADDR);
    return 0;
}

/**
 * Initialize Naomi hardware state.
 * Sets up BIOS-like state that the game expects.
 */
static void naomi_bios_init(SH4CPU *cpu) {
    /* Fill BIOS area with "SEGA" pattern (mimics DC BIOS) */
    uint32_t bios_start = 0x8C00C000 & cpu->ram_mask;
    uint32_t bios_end = 0x8C00F400 & cpu->ram_mask;
    for (uint32_t addr = bios_start; addr < bios_end; addr += 4) {
        *(uint32_t *)(cpu->ram + addr) = 0x41474553; /* "SEGA" */
    }

    /* Replicate the bootstrap at 0x8C021000:
     * - Sets SR to supervisor mode with interrupts masked
     * - Sets VBR to exception vector base
     * - Sets stack pointer (uncached mirror)
     * - Sets FPSCR for denormalized number handling
     * - Initializes cache control registers */
    cpu->sr = 0x700000F0;       /* MD=1, RB=1, BL=1, IMASK=0xF */
    cpu->vbr = 0x8C00F400;      /* Vector Base Register */
    cpu->r[15] = 0x8C00FC00;    /* Stack pointer (cached mirror) */
    cpu->fpscr = 0x00040001;    /* DN=1, RM=nearest */
    cpu->gbr = 0;
    cpu->pr = 0;                /* No return address (top-level) */

    printf("[BOOT] Naomi BIOS state initialized\n");
    printf("[BOOT]   SR=0x%08X VBR=0x%08X SP=0x%08X FPSCR=0x%08X\n",
           cpu->sr, cpu->vbr, cpu->r[15], cpu->fpscr);
}

int main(int argc, char *argv[]) {
    printf("=== %s ===\n", GAME_TITLE);
    printf("Static Recompilation (Sega Naomi / SH-4)\n\n");

    /* Determine game binary path */
    const char *bin_path = "data/extracted/1ST_READ.BIN";
    if (argc > 1) {
        bin_path = argv[1];
    }

    /* Initialize SH-4 CPU with Naomi's 32MB RAM */
    sh4_init_ex(&g_cpu, NAOMI_RAM_SIZE);

    /* Initialize hardware */
    g_hw = dc_hw_init();
    if (!g_hw) {
        fprintf(stderr, "FATAL: Hardware init failed\n");
        return 1;
    }

    /* Initialize Naomi JVS I/O */
    g_naomi_io = naomi_io_init();

    /* Connect CPU to hardware */
    sh4_set_hardware(g_hw);
    sh4_set_cpu_ref(&g_cpu);

    /* Initialize platform (window + OpenGL) */
    platform_init(640, 480, GAME_TITLE);

    /* Initialize PVR2 renderer */
    pvr2_ta_init();
    pvr2_render_init(640, 480);

    /* Initialize card reader subsystem */
    card_reader_init(g_naomi_io);

    /* Initialize deck window */
    deck_window_init();

    /* Load game binary */
    if (load_game_binary(&g_cpu, bin_path) != 0) {
        printf("\nNo game binary found at '%s'.\n", bin_path);
        printf("Run the extraction pipeline first:\n");
        printf("  1. python dcrecomp/tools/extract_naomi_rom.py mushik2k.zip data/extracted\n");
        printf("  2. python dcrecomp/tools/sh4_disasm.py data/extracted/1ST_READ.BIN\n");
        printf("  3. python dcrecomp/tools/static_recompile.py\n");
        printf("  4. python dcrecomp/tools/generate_stubs.py\n");
        printf("  5. Rebuild with CMake\n");
        goto cleanup;
    }

    /* Set up BIOS state */
    naomi_bios_init(&g_cpu);

    /* The bootstrap at 0x8C021000 does:
     * 1. Cache control init (CCR at 0xFF00001C)
     * 2. Set SR, SP=0xAC00F400, VBR=0x8C00F400, FPSCR=0x00040000
     * 3. Fill 0x8C00C000-0x8C1C6124 with "SEGA" pattern
     * 4. Clear BSS regions (RAM is already zero from calloc)
     * 5. Call func_8C0898A0 = the real game main
     * We replicate steps 1-4 in naomi_bios_init() and call the game directly. */
    g_cpu.pc = 0x8C0898A0;

    printf("[BOOT] Game main function: func_8C0898A0\n");
    /* VBlank handler at 0x8C0367EC.
     *
     * The Naomi variant of the same Katana idiom Crazy Taxi and ChuChu Rocket
     * use: masks interrupts, walks a per-frame callback table, increments a
     * frame counter and acks SB_ISTNRM. Unlike theirs it has its own prologue
     * (r14/r13/r12/r11 then pr), so it can be called directly rather than
     * having the pushes reproduced here.
     *
     * On hardware it is reached through VBR+0x600, via a trampoline copied into
     * low RAM at boot that the recompiler never sees. Without this registered
     * the game runs entirely on polled VBlank and its state machines never
     * advance.
     */
    sh4_set_irq_handler(func_8C0367EC);

    printf("[BOOT] Starting game...\n\n");

    func_8C0898A0(&g_cpu);

    /* Debug: check allocator vtable state */
    {
        uint8_t *ram = sh4_get_ram_ptr();
        uint32_t vtable_ptr = *(uint32_t *)(ram + (0x8C2FB7A4 - 0x8C000000));
        printf("[DBG] Allocator vtable ptr @0x8C2FB7A4 = 0x%08X\n", vtable_ptr);
        if (vtable_ptr != 0 && vtable_ptr != 0xFFFFFFFF) {
            uint32_t fn_ptr = *(uint32_t *)(ram + ((vtable_ptr & 0x1FFFFFFF) - 0x0C000000));
            printf("[DBG] vtable[0] = 0x%08X\n", fn_ptr);
        }
    }
    printf("[BOOT] Game init returned, entering main loop\n");

    /* Main game loop - the game init may have set up its own frame loop,
     * or it may return and expect us to drive the loop. */
    while (g_cpu.running && platform_poll_events(g_hw)) {
        /* Update deck window (card selection UI) */
        deck_window_update(g_naomi_io);

        /* VBlank */
        dc_pvr_wait_vblank(g_hw);
        platform_swap_buffers();
    }

cleanup:
    printf("\n[BOOT] Shutting down...\n");

    deck_window_destroy();
    card_reader_destroy();
    pvr2_render_destroy();
    pvr2_ta_destroy();
    platform_shutdown();
    naomi_io_destroy(g_naomi_io);
    dc_hw_destroy(g_hw);
    sh4_destroy(&g_cpu);

    return 0;
}

/* sh4_call_indirect / sh4_jump_indirect are implemented in dispatch_table.c */
