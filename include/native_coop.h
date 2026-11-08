/* The co-op: the other player on the field (platform/coop.c, coop_net.c; patches in coop/patches/) */
#ifndef NATIVE_COOP_H
#define NATIVE_COOP_H

extern int g_coop;
extern int g_coop_debug;
extern char g_coop_peer[128]; /* coop_peer = host:port, the other player (platform/coop.c) */
extern int g_coop_port;       /* coop_port: where this copy listens, 27500 */
/* native.ini's coop, coop_peer and coop_port (main.c's read_ini): 1 when key was one of them */
int native_coop_ini(const char *key, const char *value);
/* What the field shows of a player: the field's mode and stage file (the same
   place when both match), the actor's pos and z (1/256 units), direction,
   animation set and depth layer */
typedef struct NativeCoopState {
    int mode;
    int stageFile;
    int x, y, z;
    int dir;
    int animSet;
    int depth;
} NativeCoopState;
void native_coop_reset(void);
void native_coop_publish(const NativeCoopState *now);
/* the other player's state now: 1 when there is one */
int native_coop_peer(NativeCoopState *out);
/* VRAM for the field's actors: the slots kept, cleared when the field
   starts, a box kept for one, and a blank box free in a texture page */
void native_coop_vram_reset(void);
void native_coop_vram_reserve(int x, int y, int w, int h);
int native_coop_find_vram(int page_x, int page_y, int w, int h, int *out_x, int *out_y);
void native_coop_vram_dump(int page_x, int page_y);
/* or in any page right of the frame buffers, and a copy of VRAM (a CLUT) */
int native_coop_find_vram_any(int w, int h, int *out_x, int *out_y, int *page_x, int *page_y);
void native_coop_vram_copy(int from_x, int from_y, int to_x, int to_y, int w, int h);

#endif /* NATIVE_COOP_H */
