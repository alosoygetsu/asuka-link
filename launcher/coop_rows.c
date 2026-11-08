/* The co-op's rows of the Mods tab (R_TEXT): the other players' addresses, typed in, and this PC's own, to give them.
   They go in native.ini as coop_peers = a, b, c (the game reads it: platform/coop.c). Apart from options.c, which holds
   numbers, so the port's files only get the hooks. */
#include <winsock2.h>
#include <iphlpapi.h>
#include "launcher.h"

#define PEERS 3
#define ADDR 64

static char s_peer[PEERS][ADDR];
static bool s_dirty;
static int s_editing = -1;
static char s_edit[ADDR];
static char s_mine[160]; /* "100.1.2.3 (Tailscale), 192.168.1.5" */

/* this PC's IPv4 addresses, Tailscale's (100.64.0.0/10) first */
static void find_mine(void)
{
    ULONG size = 16384;
    IP_ADAPTER_ADDRESSES *list = SDL_malloc(size);
    char other[160] = "";
    s_mine[0] = 0;
    if (list == NULL) {
        return;
    }
    if (GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER, NULL, list, &size) == ERROR_BUFFER_OVERFLOW) {
        SDL_free(list);
        list = SDL_malloc(size);
        if (list == NULL || GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER, NULL, list, &size) != NO_ERROR) {
            SDL_free(list);
            return;
        }
    }
    for (IP_ADAPTER_ADDRESSES *a = list; a != NULL; a = a->Next) {
        if (a->OperStatus != IfOperStatusUp || a->IfType == IF_TYPE_SOFTWARE_LOOPBACK) {
            continue;
        }
        for (IP_ADAPTER_UNICAST_ADDRESS *u = a->FirstUnicastAddress; u != NULL; u = u->Next) {
            const Uint8 *b = (const Uint8 *)&((struct sockaddr_in *)u->Address.lpSockaddr)->sin_addr;
            char ip[24];
            if (b[0] == 169 && b[1] == 254) {
                continue; /* no DHCP answer: nobody can reach it */
            }
            SDL_snprintf(ip, sizeof ip, "%d.%d.%d.%d", b[0], b[1], b[2], b[3]);
            if (b[0] == 100 && (b[1] & 0xC0) == 64) {
                SDL_snprintf(s_mine + SDL_strlen(s_mine), sizeof s_mine - SDL_strlen(s_mine), "%s%s (Tailscale)", s_mine[0] ? ", " : "", ip);
            } else {
                SDL_snprintf(other + SDL_strlen(other), sizeof other - SDL_strlen(other), "%s%s", other[0] ? ", " : "", ip);
            }
        }
    }
    SDL_free(list);
    if (other[0]) {
        SDL_snprintf(s_mine + SDL_strlen(s_mine), sizeof s_mine - SDL_strlen(s_mine), "%s%s", s_mine[0] ? ", " : "", other);
    }
}

void coop_rows_load(void)
{
    char v[3 * ADDR + 8];
    SDL_memset(s_peer, 0, sizeof s_peer);
    s_dirty = false;
    s_editing = -1;
    if (ini_get("native.ini", "coop_peers", v, sizeof v) || ini_get("native.ini", "coop_peer", v, sizeof v)) {
        int n = 0;
        for (char *p = v; *p && n < PEERS;) {
            while (*p == ' ' || *p == ',') {
                p++;
            }
            int len = 0;
            while (p[len] && p[len] != ',') {
                len++;
            }
            while (len > 0 && p[len - 1] == ' ') {
                len--;
            }
            if (len > 0) {
                SDL_strlcpy(s_peer[n++], p, SDL_min(len + 1, ADDR));
            }
            p += len;
            while (*p && *p != ',') {
                p++;
            }
        }
    }
    find_mine();
}

bool coop_rows_save(void)
{
    char v[3 * ADDR + 8] = "";
    if (!s_dirty) {
        return true;
    }
    for (int i = 0; i < PEERS; i++) {
        if (s_peer[i][0]) {
            SDL_snprintf(v + SDL_strlen(v), sizeof v - SDL_strlen(v), "%s%s", v[0] ? ", " : "", s_peer[i]);
        }
    }
    /* coop_peer was the one-player key: it goes, coop_peers says it all */
    IniEdit edits[2] = {{"coop_peers", v[0] ? v : NULL, false}, {"coop_peer", NULL, false}};
    if (!ini_write("native.ini", edits, 2)) {
        return false;
    }
    s_dirty = false;
    return true;
}

bool coop_row_editing(void) { return s_editing >= 0; }

static void stop(bool keep)
{
    if (keep && SDL_strcmp(s_peer[s_editing], s_edit) != 0) {
        SDL_strlcpy(s_peer[s_editing], s_edit, ADDR);
        s_dirty = true;
    }
    s_editing = -1;
    SDL_StopTextInput(g_app.window);
}

void coop_row_activate(int id)
{
    if (id >= PEERS) {
        find_mine(); /* Enter on this PC's addresses looks again (Tailscale started since) */
        return;
    }
    s_editing = id;
    SDL_strlcpy(s_edit, s_peer[id], ADDR);
    SDL_StartTextInput(g_app.window);
}

/* while a row is being typed in: the keys are its own. true when the event was taken */
bool coop_row_event(const SDL_Event *e)
{
    if (s_editing < 0) {
        return false;
    }
    if (e->type == SDL_EVENT_TEXT_INPUT) {
        for (const char *c = e->text.text; *c; c++) {
            const bool ok = (*c >= '0' && *c <= '9') || (*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') || *c == '.' || *c == ':' || *c == '-' ||
                            *c == '[' || *c == ']';
            const size_t n = SDL_strlen(s_edit);
            if (ok && n < ADDR - 1) {
                s_edit[n] = *c;
                s_edit[n + 1] = 0;
            }
        }
        return true;
    }
    if (e->type == SDL_EVENT_KEY_DOWN) {
        switch (e->key.key) {
        case SDLK_RETURN:
        case SDLK_KP_ENTER: stop(true); break;
        case SDLK_ESCAPE: stop(false); break;
        case SDLK_BACKSPACE:
            if (s_edit[0]) {
                s_edit[SDL_strlen(s_edit) - 1] = 0;
            }
            break;
        case SDLK_DELETE: s_edit[0] = 0; break;
        default: break;
        }
        return true;
    }
    if (e->type == SDL_EVENT_MOUSE_BUTTON_DOWN || e->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
        stop(true); /* a click or a button elsewhere ends the typing, keeping it */
        return e->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN;
    }
    return false;
}

void coop_row_draw(int id, int x, int right, int y, bool sel)
{
    const char *label = id < PEERS ? L->s[S_COOP_PEER] : L->s[S_COOP_MINE];
    char name[48], value[ADDR + 2];
    if (id < PEERS) {
        SDL_snprintf(name, sizeof name, label, id + 2); /* "Player %d's IP" */
        label = name;
    }
    text(x + 13, y, 1, sel ? C_GOLD : C_TEXT, label);
    if (id == s_editing) {
        SDL_snprintf(value, sizeof value, "%s_", s_edit);
        text_right(right, y, C_GOLD, value);
    } else if (id < PEERS) {
        text_right(right, y, s_peer[id][0] ? C_TEXT : C_SOFT, s_peer[id][0] ? s_peer[id] : L->s[S_COOP_EMPTY]);
    } else {
        /* the first address in sight: the description has them all */
        char first[48];
        const char *end = SDL_strchr(s_mine, ',');
        SDL_strlcpy(first, s_mine[0] ? s_mine : L->s[S_COOP_NO_IP], end ? SDL_min((size_t)(end - s_mine + 1), sizeof first) : sizeof first);
        text_right(right, y, s_mine[0] ? C_TEXT : C_SOFT, first);
    }
}

const char *coop_row_desc(int id)
{
    static char d[320];
    if (id < PEERS) {
        return L->s[s_editing == id ? S_COOP_TYPING : S_COOP_PEER_D];
    }
    SDL_snprintf(d, sizeof d, "%s %s", L->s[S_COOP_MINE_D], s_mine[0] ? s_mine : L->s[S_COOP_NO_IP]);
    return d;
}
