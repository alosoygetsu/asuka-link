/* Entry point: the window, the game's data, then the game's own main()
   (renamed psx_main), which never returns. */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdarg.h>
#include <stdlib.h>
#include "platform.h"
#include "../include/native.h"

int psx_main(void);
int video_init(int scale, int fullscreen);
void install_crash_handler(void);
void native_snapshot_overlays(void);
int native_fill(const char *data);
#include "extract.h"
extern uint8_t STAGE_VRAM[], OVERLAY_VRAM[];
extern uint32_t HEAP_BASE[];

/* Where the extracted game is (extract.c): DW3_DATA, native.ini's data, or data/ next to the exe */
static char s_data[1024];

const char *data_dir(void) {
    return s_data;
}

/* Logs `what` the first time only (keyed by the pointer: callers pass literals) */
void log_once(const char *what) {
    static const char *seen[1024];
    static int count;
    int i;

    for (i = 0; i < count; i++) {
        if (seen[i] == what) {
            return;
        }
    }
    if (count < 1024) {
        seen[count++] = what;
    }
    LOG("%s", what);
}

/* printf to stderr, for probes in game code (native.h) */
void native_log(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    fflush(stderr);
}

/* The BIOS loaded all of SLES_039.36's text, and the 320x480 boot image at
   its end lands in the sub-overlay area, where main() reads it */
static int load_boot_image(void) {
    char path[1100];
    FILE *f;
    long size, off = 0x800 + (0x800A5DE0 - 0x80010000); /* STAGE_VRAM - EXE_VRAM + header */

    snprintf(path, sizeof path, "%s/SLES_039.36", s_data);
    f = fopen(path, "rb");
    if (f == NULL) {
        LOG("no %s", path);
        return 0;
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, off, SEEK_SET);
    fread(STAGE_VRAM, 1, size - off, f);
    fclose(f);
    return 1;
}

/* native.ini next to the exe, all optional: data = <path>, fullscreen = 0/1,
   scale = <window size in 320x240 steps>, render_scale = 1-8, widescreen = 0/1, rumble = 0/1, icons = <name>,
   debug = 0/1, perf = 0/1 (the battle's performance meter; F10), log = <subsystems>, the buttons, and the co-op's
   (platform/coop.c); the options menu (menu.c) writes its own back. The rest is fixed (fixed_option) */
static int s_fullscreen, s_scale, s_fps60 = 1, s_res_set; /* s_scale 0: from the screen */
extern int NTSC_MODE;
int g_debug, g_smooth_movies = 1, g_log;

/* "cd,snd" or "all" to LOG_* bits */
static int log_bits(const char *names) {
    static const char *NAMES[] = {"gpu", "cd", "snd", "pad", "mc", "ovl"};
    int bits = 0, i;

    for (i = 0; i < 6; i++) {
        if (SDL_strstr(names, NAMES[i]) != NULL || SDL_strstr(names, "all") != NULL) {
            bits |= 1 << i;
        }
    }
    return bits;
}

/* The options Asuka Link fixes since v1.3.0, at their defaults (where each is defined): no menu shows them, and an
   old native.ini's lines are skipped */
static int fixed_option(const char *key) {
    static const char *const FIXED[] = {"fps", "vsync", "smooth_movies", "inactive_window", "dither", "true_color", "perspective", "smooth_3d",
                                        "soft_shadows", "battle_shadows", "battle_sky", "deadzone", "ff_menu_speed", "skip_press_start", "menu_wrap",
                                        "fast_text", "fast_travel", "map_objective", "journal", "bladegarurumon", "full_exp_share",
                                        "custom_effects", "custom_sounds", "custom_scripts"};

    for (size_t i = 0; i < SDL_arraysize(FIXED); i++) {
        if (SDL_strcasecmp(key, FIXED[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

/* data_only: just the data folder, before the game's tables are filled in */
static void read_ini(int data_only) {
    char path[1100], line[1200], key[32], value[1100];
    FILE *f;

    snprintf(path, sizeof path, "%snative.ini", SDL_GetBasePath());
    if ((f = fopen(path, "r")) == NULL) {
        return;
    }
    while (fgets(line, sizeof line, f) != NULL) {
        if (line[0] == '#' || line[0] == ';' || sscanf(line, " %31[^= ] = %1099[^\r\n]", key, value) != 2) {
            continue;
        }
        if (SDL_strcasecmp(key, "data") == 0) {
            if (!data_only) {
                continue; /* read already, and DW3_DATA may have overridden it */
            }
            /* relative to the exe's folder, as everything else here */
            int absolute = value[0] == '/' || value[0] == '\\' || (value[0] != 0 && value[1] == ':');

            snprintf(s_data, sizeof s_data, "%s%s", absolute ? "" : SDL_GetBasePath(), value);
        } else if (data_only) {
            continue;
        } else if (native_coop_ini(key, value)) { /* the co-op's keys, and the options it takes out */
        } else if (fixed_option(key)) { /* Asuka Link fixes it (v1.3.0): an old native.ini's line is skipped */
        } else if (SDL_strcasecmp(key, "fullscreen") == 0) {
            s_fullscreen = atoi(value) != 0;
        } else if (SDL_strcasecmp(key, "scale") == 0 && atoi(value) >= 1) {
            s_scale = atoi(value);
        } else if (SDL_strcasecmp(key, "widescreen") == 0) {
            g_widescreen = atoi(value) != 0;
        } else if (SDL_strcasecmp(key, "debug") == 0) {
            g_debug = atoi(value) != 0;
        } else if (SDL_strcasecmp(key, "perf") == 0) {
            g_perf = atoi(value) != 0;
        } else if (SDL_strcasecmp(key, "log") == 0) {
            g_log = log_bits(value);
        } else if (SDL_strcasecmp(key, "rumble") == 0) {
            g_rumble = atoi(value) != 0;
        } else if (SDL_strcasecmp(key, "render_scale") == 0) {
            g_res_scale = SDL_clamp(atoi(value), 1, 8);
            s_res_set = 1;
        } else if (SDL_strcasecmp(key, "icons") == 0) {
            for (int i = 0; i < ICONS_COUNT; i++) {
                if (SDL_strcasecmp(value, ICON_NAMES[i]) == 0) {
                    g_icons = i;
                }
            }
        } else {
            pad_bind_ini(key, value);
        }
    }
    fclose(f);
}

/* No game yet: the launcher, next to the exe, reads the disc. Not if it started this very run, which would loop. */
static int open_launcher(void) {
    char path[1100];
    const char *args[] = {path, NULL};
    SDL_Process *launcher;

    if (getenv("DW3_FROM_LAUNCHER") != NULL) {
        return 0;
    }
    snprintf(path, sizeof path, "%sdw3launcher.exe", SDL_GetBasePath());
    launcher = SDL_GetPathInfo(path, NULL) ? SDL_CreateProcess(args, false) : NULL;
    if (launcher == NULL) {
        return 0;
    }
    SDL_DestroyProcess(launcher); /* it goes on by itself */
    return 1;
}

void *native_must(void *p, const char *what) {
    if (p == NULL) {
        char text[200];

        snprintf(text, sizeof text, "Out of memory (%s). The game can't go on; close other programs and start it again.", what);
        LOG("%s", text);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Digimon World 2003", text, NULL);
        exit(1);
    }
    return p;
}

/* --check-mods: what the loader makes of each folder of data/mods/digimon, as the game and the launcher get it from native_mods_report.
   The exit code is how many were rejected */
static int check_mods_cli(void) {
    static const char *const STATUS[] = {"loaded", "disabled", "REJECTED", "skipped"};
    int n, dropped, bad = 0;
    const NativeModReport *r = native_mods_report(&n, &dropped);

    for (int i = 0; i < n; i++) {
        bad += r[i].status == NATIVE_MOD_REJECTED;
        printf("%-9s %-24s %s", STATUS[r[i].status], r[i].folder, r[i].name);
        if (r[i].status == NATIVE_MOD_LOADED || r[i].status == NATIVE_MOD_DISABLED) {
            printf(" (id %d, model %d)", r[i].id, r[i].fid);
        }
        printf("\n");
        if (r[i].reason[0]) {
            printf("          why: %s\n", r[i].reason);
        }
        if (r[i].warnings[0]) {
            printf("          warning: %s\n", r[i].warnings);
        }
    }
    if (dropped > 0) {
        printf("%d more folders are not listed here (the report's room is %d)\n", dropped, NATIVE_MOD_REPORTS);
    }
    printf("%d folders, %d rejected\n", n + dropped, bad);
    bad += native_scripts_check();
    fflush(stdout);
    return bad;
}

int main(int argc, char **argv) {
    const char *env = getenv("DW3_DATA");

    char exe[1100];

    install_crash_handler();
    read_ini(1);
    if (env != NULL) {
        snprintf(s_data, sizeof s_data, "%s", env);
    } else if (s_data[0] == 0) {
        /* data/ next to the exe, or native/data from native/build */
        snprintf(s_data, sizeof s_data, "%s../data/SLES_039.36", SDL_GetBasePath());
        snprintf(s_data, sizeof s_data, SDL_GetPathInfo(s_data, NULL) ? "%s../data" : "%sdata", SDL_GetBasePath());
    }
    if (argc == 3 && SDL_strcmp(argv[1], "--extract") == 0) {
        int e = native_extract(argv[2], s_data, NULL);
        LOG("%s", e ? native_extract_error(e) : "OK");
        return e;
    }
    /* the first run reads the player's disc; then the game's tables, before
       anything touches the game's variables (fill.c) */
    snprintf(exe, sizeof exe, "%s/SLES_039.36", s_data);
    if (!SDL_GetPathInfo(exe, NULL)) {
        LOG("no game in %s", s_data);
        if (argc > 1) {
            return 1;
        }
        if (open_launcher()) {
            return 0;
        }
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Digimon World 2003",
                                 "The game's data isn't there yet. Open dw3launcher.exe to read your disc, "
                                 "or run: dw2003 --extract IMAGE", NULL);
        return 1;
    }
    if (!native_fill(s_data)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Digimon World 2003",
                                 "The game's data doesn't match this version of the port. Delete the data folder "
                                 "and start the game again to read your disc anew.", NULL);
        return 1;
    }
    native_bladegarurumon_setup(); /* the game's tables as read, then its own */
    native_mods_load(); /* data/mods/digimon/<name>/digimon.json, before the tables take them */
    if (argc == 2 && SDL_strcmp(argv[1], "--check-mods") == 0) {
        return check_mods_cli();
    }
    native_mods_setup();
    native_snapshot_overlays();
    native_fast_travel_setup(); /* the stages' doors, their variables put back */
    read_ini(0);
    NTSC_MODE = s_fps60; /* the game's own 60 Hz mode, set before it boots */
    g_coop_debug = getenv("DW3_COOP_DEBUG") != NULL;
    if (getenv("DW3_LOG") != NULL) {
        g_log = log_bits(getenv("DW3_LOG"));
    }
    if ((uintptr_t)STAGE_VRAM + 512 * 1024 > 0x01000000 || (uintptr_t)OVERLAY_VRAM + 512 * 1024 > 0x01000000 ||
        (uintptr_t)HEAP_BASE + NATIVE_HEAP_SIZE > 0x01000000) {
        LOG("the game's memory is above 16 MB: the GPU's 24-bit tags can't reach it");
        return 1;
    }
    if ((argc == 3 || argc == 4) && SDL_strcmp(argv[1], "--render-sounds") == 0 && native_render_parse(argv[2], argc == 4 ? argv[3] : "") == 0) {
        LOG("--render-sounds DIR 28,0x2A: no sound index (0 to 0x59) in the list");
        return 1;
    }
    if (argc == 3 && SDL_strcmp(argv[1], "--dump-stages") == 0) {
        return stage_dump(argv[2]);
    }
    if (argc == 2 && SDL_strcmp(argv[1], "--postcards") == 0) {
        return postcards_make();
    }
    postcards_start();
    for (int i = 1; i + 1 < argc; i++) {
        if (SDL_strcmp(argv[i], "--field") == 0) {
            g_field_mode = (int)SDL_strtol(argv[++i], NULL, 0); /* 560 or 0x230 */
        } else if (SDL_strcmp(argv[i], "--arg") == 0) {
            g_field_arg = (int)SDL_strtol(argv[++i], NULL, 0);
        } else if (SDL_strcmp(argv[i], "--event") == 0 && i + 1 < argc) {
            g_field_event = (int)SDL_strtol(argv[++i], NULL, 0);
        } else if (SDL_strcmp(argv[i], "--at") == 0) {
            SDL_sscanf(argv[++i], "%d,%d", &g_field_at_x, &g_field_at_y);
        } else if (SDL_strcmp(argv[i], "--party") == 0 && i + 1 < argc) {
            int party = (int)SDL_strtol(argv[++i], NULL, 0); /* SDL_clamp reads its argument more than once */
            g_field_party = party >= NATIVE_TEAM_BASE ? party : SDL_clamp(party, 0, 2); /* or a free team (platform/team.c) */
        } else if (SDL_strcmp(argv[i], "--give") == 0) {
            g_give_form = (int)SDL_strtol(argv[++i], NULL, 0);
        } else if (SDL_strcmp(argv[i], "--progress") == 0) {
            g_field_progress = (int)SDL_strtol(argv[++i], NULL, 0);
        } else if (SDL_strcmp(argv[i], "--set") == 0 && g_field_set_count < 64) {
            int *s = g_field_sets[g_field_set_count];
            if (SDL_sscanf(argv[++i], "%x=%d", &s[0], &s[1]) == 2) {
                g_field_set_count++;
            }
        }
    }
    if (s_scale == 0) {
        /* the biggest whole multiple of 320x240 that fits the screen, title bar
           and taskbar aside: the PSX's pixels even; the 3D as fine, up to 4x */
        SDL_Rect usable = {0, 0, 960, 720};

        SDL_InitSubSystem(SDL_INIT_VIDEO);
        SDL_GetDisplayUsableBounds(SDL_GetPrimaryDisplay(), &usable);
        s_scale = SDL_max(1, SDL_min(usable.w / (g_widescreen ? 427 : 320), (usable.h - 40) / 240));
        if (!s_res_set) {
            g_res_scale = SDL_min(s_scale, 4);
        }
    }
    if (!load_boot_image() || !video_init(s_scale, s_fullscreen)) {
        return 1;
    }
    return psx_main();
}
