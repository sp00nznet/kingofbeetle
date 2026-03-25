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

    /* Load at Naomi game address */
    uint32_t load_offset = GAME_ENTRY_ADDR & cpu->ram_mask;

    if (load_offset + size > cpu->ram_size) {
        fprintf(stderr, "ERROR: Binary too large (%ld bytes) for RAM\n", size);
        fclose(f);
        return -1;
    }

    size_t read = fread(cpu->ram + load_offset, 1, size, f);
    fclose(f);

    printf("[BOOT] Loaded %s: %zu bytes at 0x%08X\n", path, read, GAME_LOAD_ADDR);
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

    /* Set up initial CPU state */
    cpu->r[15] = 0x8C00FC00;  /* Stack pointer */
    cpu->vbr = 0x8C00F400;    /* Vector base */
    cpu->sr = 0x700000F0;     /* Supervisor mode, interrupts masked */

    printf("[BOOT] Naomi BIOS state initialized\n");
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

    /* Set PC to game entry point (from Naomi cart header at offset 0x420) */
    g_cpu.pc = 0x8C021000;

    printf("[BOOT] Entry point: 0x%08X\n", g_cpu.pc);
    printf("[BOOT] Starting game loop...\n\n");

    /* Call the game initialization function */
    func_8C021000(&g_cpu);

    /* Main game loop */
    while (g_cpu.running && platform_poll_events(g_hw)) {
        /* The game's main loop runs inside the recompiled code.
         * If we reach here, the init function returned. */

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

/* ========== Indirect Dispatch ========== */

/* These are called by recompiled code when a JSR/JMP target
 * is computed at runtime (indirect call/jump). They look up
 * the target address in the dispatch table and call the
 * corresponding recompiled function. */

void sh4_call_indirect(SH4CPU *cpu) {
    /* TODO: Implement dispatch table lookup after recompilation
     * For now, log and return */
    static int log_count = 0;
    if (log_count < 20) {
        log_count++;
        printf("[DISPATCH] indirect call to 0x%08X (pr=0x%08X)\n",
               cpu->pc, cpu->pr);
    }
}

void sh4_jump_indirect(SH4CPU *cpu) {
    static int log_count = 0;
    if (log_count < 20) {
        log_count++;
        printf("[DISPATCH] indirect jump to 0x%08X\n", cpu->pc);
    }
    cpu->running = false;  /* Stop for now until dispatch table is ready */
}
