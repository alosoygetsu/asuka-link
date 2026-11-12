/* The room (la sala), on the home screen's right: your group of up to 4, the players this PC can see and adding one by
   address. The group goes in native.ini as coop_peers = a, b, c (the game reads it: platform/coop.c), saved on every
   change. Its rows are R_PEER, ids SALA_MEMBER + i and SALA_ADD.
   For now the group is the addresses typed in; the players on the local network come with the room's protocol. */
#include <winsock2.h>
#include <iphlpapi.h>
#include "launcher.h"

#define PEERS 3
#define ADDR 64

enum { SALA_X = SCREEN_W - 150, SALA_Y = 8, SALA_W = 142, SALA_ROW = 12 };

static char s_peer[PEERS][ADDR];
static int s_editing = -1; /* the slot being typed in */
static int s_armed = -1;   /* a member's row asked once to be taken out */
static char s_edit[ADDR];
static char s_mine[160]; /* "100.1.2.3 (Tailscale), 192.168.1.5" */
static char s_name[64];  /* this PC's */

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

void sala_load(void)
{
    char v[3 * ADDR + 8];
    DWORD n = sizeof s_name;
    SDL_memset(s_peer, 0, sizeof s_peer);
    s_editing = s_armed = -1;
    if (!GetComputerNameA(s_name, &n)) {
        SDL_strlcpy(s_name, "?", sizeof s_name);
    }
    if (ini_get("native.ini", "coop_peers", v, sizeof v) || ini_get("native.ini", "coop_peer", v, sizeof v)) {
        int k = 0;
        for (char *p = v; *p && k < PEERS;) {
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
                SDL_strlcpy(s_peer[k++], p, SDL_min(len + 1, ADDR));
            }
            p += len;
            while (*p && *p != ',') {
                p++;
            }
        }
    }
    find_mine();
}

/* the group, packed (no empty slot before a full one), to native.ini */
static bool save(void)
{
    char v[3 * ADDR + 8] = "";
    int k = 0;
    for (int i = 0; i < PEERS; i++) {
        if (s_peer[i][0]) {
            if (k != i) {
                SDL_memcpy(s_peer[k], s_peer[i], ADDR);
            }
            k++;
        }
    }
    for (int i = k; i < PEERS; i++) {
        s_peer[i][0] = 0;
    }
    for (int i = 0; i < k; i++) {
        SDL_snprintf(v + SDL_strlen(v), sizeof v - SDL_strlen(v), "%s%s", v[0] ? ", " : "", s_peer[i]);
    }
    /* coop_peer was the one-player key: it goes, coop_peers says it all */
    IniEdit edits[2] = {{"coop_peers", v[0] ? v : NULL, false}, {"coop_peer", NULL, false}};
    return ini_write("native.ini", edits, 2);
}

static int members(void)
{
    int n = 0;
    while (n < PEERS && s_peer[n][0]) {
        n++;
    }
    return n;
}

int sala_rows(Row *out, int max)
{
    int n = 0;
    for (int i = 0; i < members() && n < max; i++) {
        out[n++] = (Row){R_PEER, (short)(SALA_MEMBER + i)};
    }
    if ((members() < PEERS || s_editing >= 0) && n < max) {
        out[n++] = (Row){R_PEER, SALA_ADD};
    }
    return n;
}

bool sala_editing(void) { return s_editing >= 0; }

static void stop(bool keep)
{
    if (keep && s_edit[0]) {
        SDL_strlcpy(s_peer[s_editing], s_edit, ADDR);
        if (!save()) {
            g_app.note = L->s[S_SAVE_FAILED];
        }
    }
    s_editing = -1;
    SDL_StopTextInput(g_app.window);
}

void sala_activate(int id)
{
    if (id == SALA_ADD) {
        if (members() < PEERS) {
            find_mine(); /* the description shows them: Tailscale may have started since */
            s_editing = members();
            s_edit[0] = 0;
            SDL_StartTextInput(g_app.window);
        }
        return;
    }
    if (id >= SALA_MEMBER && id < SALA_MEMBER + PEERS) {
        if (s_armed != id) { /* a second Enter takes them out */
            s_armed = id;
            return;
        }
        s_peer[id - SALA_MEMBER][0] = 0;
        s_armed = -1;
        if (!save()) {
            g_app.note = L->s[S_SAVE_FAILED];
        }
    }
}

void sala_moved(void) { s_armed = -1; }

/* while the address is being typed in: the keys are its own. true when the event was taken */
bool sala_event(const SDL_Event *e)
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

/* ---- the panel ---- */

static int row_y(int i) { return SALA_Y + 4 + (2 + i) * SALA_ROW; } /* under the heading and you */

SDL_Rect sala_row_rect(int id)
{
    Row r[PEERS + 1];
    const int n = sala_rows(r, PEERS + 1);
    for (int i = 0; i < n; i++) {
        if (r[i].id == id) {
            return (SDL_Rect){SALA_X + 1, row_y(i) - 1, SALA_W - 2, SALA_ROW};
        }
    }
    return (SDL_Rect){0, 0, 0, 0};
}

int sala_hit(int x, int y)
{
    Row r[PEERS + 1];
    const int n = sala_rows(r, PEERS + 1);
    for (int i = 0; i < n; i++) {
        const SDL_Rect b = sala_row_rect(r[i].id);
        if (x >= b.x && x < b.x + b.w && y >= b.y && y < b.y + b.h) {
            return r[i].id;
        }
    }
    return -1;
}

/* a player's signal: three bars, lit up to `level` (you: 3; the others' come with the room's protocol) */
static void signal(int x, int y, int level)
{
    for (int i = 0; i < 3; i++) {
        fill(x + i * 3, y + 7 - 2 * i, 2, 2 + 2 * i, i < level ? C_LINK : C_RULE);
    }
}

void sala_draw(int sel)
{
    Row r[PEERS + 1];
    const int n = sala_rows(r, PEERS + 1), h = (2 + n) * SALA_ROW + 8;
    char count[8], line[ADDR + 72];
    fill_alpha(SALA_X, SALA_Y, SALA_W, h, C_BG, 215);
    outline(SALA_X, SALA_Y, SALA_W, h, C_GOLD);
    text(SALA_X + 6, SALA_Y + 4, 1, C_GOLD, L->s[S_SALA]);
    SDL_snprintf(count, sizeof count, "%d/4", 1 + members());
    text_right(SALA_X + SALA_W - 6, SALA_Y + 4, C_SOFT, count);
    signal(SALA_X + 5, SALA_Y + 4 + SALA_ROW, 3);
    SDL_snprintf(line, sizeof line, "%s: %s", L->s[S_SALA_YOU], s_name);
    text(SALA_X + 16, SALA_Y + 5 + SALA_ROW, 1, C_TEXT, line);
    for (int i = 0; i < n; i++) {
        const int y = row_y(i), id = r[i].id;
        const bool on = id == sel;
        if (on) {
            fill(SALA_X + 1, y - 1, SALA_W - 2, SALA_ROW, C_SELECT);
        }
        if (id == SALA_ADD) {
            if (s_editing >= 0) {
                SDL_snprintf(line, sizeof line, "%s_", s_edit);
                text(SALA_X + 14, y, 1, C_GOLD, line);
            } else {
                text_sel(SALA_X + 6, y + 1, on, C_SOFT, L->s[S_SALA_ADD]);
            }
        } else {
            signal(SALA_X + 5, y, 1);
            text_sel(SALA_X + 16, y + 1, on, C_TEXT, s_peer[id - SALA_MEMBER]);
        }
    }
}

const char *sala_desc(int id)
{
    static char d[320];
    if (id == SALA_ADD) {
        if (s_editing >= 0) {
            return L->s[S_COOP_TYPING];
        }
        SDL_snprintf(d, sizeof d, "%s %s %s", L->s[S_SALA_ADD_D], L->s[S_COOP_MINE_D], s_mine[0] ? s_mine : L->s[S_COOP_NO_IP]);
        return d;
    }
    return L->s[s_armed == id ? S_SALA_OUT_SURE : S_SALA_MEMBER_D];
}
