/* The room (la sala), top right of the home screen, compact: its name and Ready on top, you and your group one line
   each, the players you can invite (this network's and the recent ones), and adding one by address. The network is
   sala_net.c's. Its rows are R_PEER: SALA_INVITE (an invitation: Accept / Decline), SALA_READY, SALA_MEMBER + i
   (Test / Take out), SALA_NEAR + i (Invite), SALA_LEAVE and SALA_ADD; a row's two buttons go with g_app.btn. */
#include <windows.h>
#include <shellapi.h>
#include "launcher.h"

#define ADDR 64
enum { SALA_X = SCREEN_W - 208, SALA_Y = 8, SALA_W = 200, HEAD = 14, ROW = 12, LINES = 8, YOU = -1, RULE = -2 };

static bool s_editing;
static char s_edit[ADDR];
static int s_armed = -1;      /* a member's Take out asked once */
static Uint32 s_tested;       /* the member Test was pressed on, for the firewall's second press */
static Uint64 s_tested_at;
static bool s_all_ready_said; /* "all ready" said once each time it happens */
static int s_first;           /* the first line in sight */

void sala_load(void) { net_init(); }

/* ---- the rows, and the body's lines: the invitation, you, your group, a rule, the others, Leave and Add ---- */

static NetPlayer s_mem[ROOM_MAX], s_av[16];
static int s_nmem, s_nav;
static int s_line[32], s_lines;

static void refresh(void)
{
    s_nmem = net_members(s_mem, ROOM_MAX);
    s_nav = net_available(s_av, 16);
    s_lines = 0;
    if (net_invitation()) {
        s_line[s_lines++] = SALA_INVITE;
    }
    s_line[s_lines++] = YOU;
    for (int i = 1; i < s_nmem; i++) {
        s_line[s_lines++] = SALA_MEMBER + i;
    }
    if (s_nav) {
        s_line[s_lines++] = RULE;
    }
    for (int i = 0; i < s_nav; i++) {
        s_line[s_lines++] = SALA_NEAR + i;
    }
    if (s_nmem > 1) {
        s_line[s_lines++] = SALA_LEAVE;
    }
    if (s_nmem < ROOM_MAX) {
        s_line[s_lines++] = SALA_ADD;
    }
}

int sala_rows(Row *out, int max)
{
    int n = 0;
    refresh();
    if (n < max) {
        out[n++] = (Row){R_PEER, SALA_READY};
    }
    for (int i = 0; i < s_lines && n < max; i++) {
        if (s_line[i] >= 0) {
            out[n++] = (Row){R_PEER, (short)s_line[i]};
        }
    }
    return n;
}

int sala_btn_count(int id)
{
    return id == SALA_INVITE || (id >= SALA_MEMBER && id < SALA_MEMBER + ROOM_MAX) ? 2 : 1;
}

bool sala_editing(void) { return s_editing; }

void sala_moved(void)
{
    s_armed = -1;
    g_app.btn = 0;
}

/* Asuka Link's rule for Windows' firewall: the game and the launcher, UDP in, asked as administrator */
static void open_firewall(void)
{
    char args[2400], dir[1100];
    SDL_strlcpy(dir, SDL_GetBasePath(), sizeof dir);
    SDL_snprintf(args, sizeof args,
                 "/c netsh advfirewall firewall add rule name=\"Asuka Link (juego)\" dir=in action=allow protocol=UDP program=\"%sdw2003.exe\" & "
                 "netsh advfirewall firewall add rule name=\"Asuka Link (launcher)\" dir=in action=allow protocol=UDP program=\"%sdw3launcher.exe\"",
                 dir, dir);
    ShellExecuteA(NULL, "runas", "cmd.exe", args, NULL, SW_HIDE);
}

void sala_activate(int id)
{
    refresh();
    if (id == SALA_INVITE) {
        net_answer(g_app.btn == 0);
        g_app.btn = 0;
    } else if (id == SALA_READY) {
        net_set_ready(!net_ready());
        s_all_ready_said = false;
    } else if (id == SALA_LEAVE) {
        net_leave();
    } else if (id == SALA_ADD) {
        s_editing = true;
        s_edit[0] = 0;
        SDL_StartTextInput(g_app.window);
    } else if (id >= SALA_NEAR && id < SALA_NEAR + s_nav) {
        const NetPlayer *p = &s_av[id - SALA_NEAR];
        if (net_can_invite(p) || (p->recent && !p->online)) {
            net_invite(p->id);
            g_app.note = L->s[S_SALA_INVITED];
        }
    } else if (id >= SALA_MEMBER && id < SALA_MEMBER + s_nmem) {
        const NetPlayer *p = &s_mem[id - SALA_MEMBER];
        if (g_app.btn == 1) { /* Take out: a second press does it */
            if (s_armed != id) {
                s_armed = id;
                g_app.note = L->s[S_SALA_OUT_SURE];
                return;
            }
            net_kick(p->id);
            s_armed = -1;
            g_app.btn = 0;
        } else if (p->online) { /* Test */
            static char note[160];
            SDL_snprintf(note, sizeof note, L->s[S_SALA_TEST_OK], p->name, p->rtt >= 0 ? p->rtt : 0);
            g_app.note = note;
        } else if (s_tested == p->id && SDL_GetTicks() - s_tested_at < 15000) {
            open_firewall();
            g_app.note = L->s[S_SALA_FIREWALL];
        } else {
            s_tested = p->id;
            s_tested_at = SDL_GetTicks();
            g_app.note = L->s[S_SALA_TEST_NONE];
        }
    }
}

void sala_tick(void)
{
    static int pairing;
    net_poll();
    if (net_all_ready() && !s_all_ready_said) {
        s_all_ready_said = true;
        g_app.note = L->s[S_SALA_ALL_READY];
        SDL_FlashWindow(g_app.window, SDL_FLASH_UNTIL_FOCUSED);
    }
    if (net_pair_state() == 2 && pairing == 1) {
        g_app.note = L->s[S_SALA_NO_ANSWER];
    }
    pairing = net_pair_state();
}

/* while the address is being typed in: the keys are its own. true when the event was taken */
static void stop(bool keep)
{
    s_editing = false;
    SDL_StopTextInput(g_app.window);
    if (keep && s_edit[0]) {
        g_app.note = net_pair(s_edit) ? L->s[S_SALA_PAIRING] : L->s[S_SALA_NO_ANSWER];
    }
}

bool sala_event(const SDL_Event *e)
{
    if (!s_editing) {
        return false;
    }
    if (e->type == SDL_EVENT_TEXT_INPUT) {
        for (const char *c = e->text.text; *c; c++) {
            const bool ok = (*c >= '0' && *c <= '9') || (*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') || *c == '.' || *c == ':' || *c == '-';
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
        stop(true);
        return e->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN;
    }
    return false;
}

/* ---- where the rows are ---- */

static int line_of(int id)
{
    for (int i = 0; i < s_lines; i++) {
        if (s_line[i] == id) {
            return i;
        }
    }
    return -1;
}

static SDL_Rect ready_rect(void)
{
    const int w = text_width(L->s[S_SALA_READY], 1) + 16;
    return (SDL_Rect){SALA_X + SALA_W - w - 3, SALA_Y + 2, w, HEAD - 3};
}

SDL_Rect sala_row_rect(int id)
{
    refresh();
    if (id == SALA_READY) {
        return ready_rect();
    }
    const int line = line_of(id);
    if (line < s_first || line >= s_first + LINES) {
        return (SDL_Rect){0, 0, 0, 0};
    }
    return (SDL_Rect){SALA_X + 2, SALA_Y + HEAD + 2 + (line - s_first) * ROW, SALA_W - 4, ROW};
}

int sala_hit(int x, int y)
{
    Row r[24];
    const int n = sala_rows(r, 24);
    for (int i = 0; i < n; i++) {
        const SDL_Rect b = sala_row_rect(r[i].id);
        if (x >= b.x && x < b.x + b.w && y >= b.y && y < b.y + b.h) {
            if (sala_btn_count(r[i].id) == 2) { /* which button: the right one's half */
                g_app.btn = x >= SALA_X + SALA_W - 50;
            }
            return r[i].id;
        }
    }
    return -1;
}

/* ---- drawing ---- */

/* a player's figure, 5 x 9: the character creator's model (Junior's cap, the boy, the girl's hair), its clothes in the
   tone's colour; grey without a look */
static void portrait(int x, int y, int model, int tone)
{
    static const char *const FIG[3][9] = {
        {".hhh.", "hhhhh", ".sss.", ".sss.", "bbbbb", ".bbb.", ".bbb.", ".b.b.", ".b.b."},
        {".hhh.", "hhhhh", "hsssh", ".sss.", "bbbbb", ".bbb.", ".bbb.", ".b.b.", ".b.b."},
        {".hhh.", "hhhhh", "hsssh", "hsssh", "hbbbh", ".bbb.", "bbbbb", ".b.b.", ".b.b."},
    };
    static const Uint32 TONES[8] = {0x4F8FE0, 0x5CC8E0, 0x50C878, 0xC8D050, 0xE0A050, 0xE05555, 0xD060B0, 0x8A6BD0};
    const char *const *f = FIG[model >= 0 && model < 3 ? model : 1];
    for (int j = 0; j < 9; j++) {
        for (int i = 0; i < 5; i++) {
            const char c = f[j][i];
            if (c != '.') {
                const Uint32 col = model < 0 ? (c == 's' ? 0x8CA0A8 : 0x52707A)
                                 : c == 'h' ? (model == 0 ? 0xD04040 : 0x4A3020) : c == 's' ? 0xF0C8A0 : TONES[tone >= 0 ? tone & 7 : 0];
                fill(x + i, y + j, 1, 1, col);
            }
        }
    }
}

static void bars(int x, int y, const NetPlayer *p)
{
    const int level = p->you ? 3 : !p->online ? 0 : p->rtt < 0 ? 2 : p->rtt < 80 ? 3 : p->rtt < 160 ? 2 : 1;
    for (int i = 0; i < 3; i++) {
        fill(x + i * 3, y + 7 - 2 * i, 2, 2 + 2 * i, i < level ? C_LINK : C_RULE);
    }
}

/* a row's two buttons on its right: the one with the focus dark on the yellow */
static void buttons(int right, int y, int a, int b, bool sel)
{
    const char *s[2] = {L->s[a], L->s[b]};
    int x = right;
    for (int k = 1; k >= 0; k--) {
        const int w = text_width(s[k], 1) + 6;
        x -= w;
        if (sel && g_app.btn == k) {
            fill(x, y + 1, w, ROW - 2, C_ON_SELECT);
            text_flat(x + 3, y + 2, C_GOLD, s[k]);
        } else {
            text_sel(x + 3, y + 2, sel, C_LINK, s[k]);
        }
        x -= 3;
    }
}

static void player_line(int y, const NetPlayer *p, bool sel, int id)
{
    char name[40];
    portrait(SALA_X + 6, y + 1, p->model, p->tone);
    SDL_snprintf(name, sizeof name, "%s%s", p->name, p->recent && !p->member && !p->you ? " *" : "");
    text_sel(SALA_X + 15, y + 2, sel, p->you ? C_GOLD : C_TEXT, name);
    const int right = SALA_X + SALA_W - 6;
    if (id >= SALA_MEMBER && id < SALA_MEMBER + ROOM_MAX && sel) {
        buttons(right, y, S_SALA_TEST, S_SALA_KICK, true);
        return;
    }
    const bool listed = id >= SALA_NEAR && id < SALA_NEAR + 16;
    const char *state = p->other_version ? L->s[S_SALA_OTHER_VERSION]
                      : listed && p->online && p->room_size + net_room_size() > ROOM_MAX ? L->s[S_SALA_FULL]
                      : listed ? L->s[S_SALA_INVITE]
                      : !p->online ? L->s[S_SALA_OFFLINE]
                      : p->ready ? L->s[S_SALA_READY] : "";
    const Uint32 col = p->other_version ? C_RED : listed ? (p->online ? C_LINK : C_DIM) : !p->online ? C_FAINT : C_GOLD;
    int x = right;
    if (!listed) {
        x -= 9;
        bars(x, y + 1, p);
        x -= 4;
    }
    text_sel(x - text_width(state, 1), y + 2, sel, col, state);
}

void sala_draw(int sel)
{
    char title[48], count[8];
    refresh();
    const int at = line_of(sel);
    if (at >= 0 && at < s_first) {
        s_first = at;
    } else if (at >= s_first + LINES) {
        s_first = at - LINES + 1;
    }
    s_first = SDL_clamp(s_first, 0, SDL_max(s_lines - LINES, 0));
    const int shown = SDL_min(s_lines - s_first, LINES), h = HEAD + 4 + shown * ROW;
    fill_alpha(SALA_X, SALA_Y, SALA_W, h, C_BG, 220);
    outline(SALA_X, SALA_Y, SALA_W, h, C_GOLD);
    outline(SALA_X + 1, SALA_Y + 1, SALA_W - 2, h - 2, C_GOLD);
    fill(SALA_X, SALA_Y, SALA_W, HEAD, C_GOLD);
    net_room_name(title, sizeof title);
    SDL_snprintf(count, sizeof count, " %d/%d", net_room_size(), ROOM_MAX);
    SDL_strlcat(title, count, sizeof title);
    text_flat(SALA_X + 6, SALA_Y + 3, C_ON_SELECT, title);
    const SDL_Rect rd = ready_rect();
    const bool rsel = sel == SALA_READY;
    fill(rd.x, rd.y, rd.w, rd.h, rsel ? C_ON_SELECT : net_ready() ? C_LINK : C_GOLD_HI);
    if (net_ready()) { /* a tick */
        const Uint32 c = rsel ? C_GOLD : C_ON_SELECT;
        fill(rd.x + 3, rd.y + 5, 1, 2, c), fill(rd.x + 4, rd.y + 6, 1, 2, c), fill(rd.x + 5, rd.y + 4, 1, 3, c), fill(rd.x + 6, rd.y + 2, 1, 3, c);
    }
    text_flat(rd.x + 10, rd.y + 2, rsel ? C_GOLD : C_ON_SELECT, L->s[S_SALA_READY]);
    for (int line = s_first; line < s_first + shown; line++) {
        const int y = SALA_Y + HEAD + 2 + (line - s_first) * ROW, id = s_line[line];
        const bool on = id == sel && id >= 0;
        if (id == YOU) {
            player_line(y, &s_mem[0], false, YOU);
            continue;
        }
        if (id == RULE) {
            fill(SALA_X + 6, y + ROW / 2, SALA_W - 12, 1, C_RULE);
            continue;
        }
        if (on) {
            fill(SALA_X + 2, y, SALA_W - 4, ROW, C_SELECT);
        }
        if (id == SALA_INVITE) {
            const NetPlayer *p = net_invitation();
            char s[48];
            SDL_snprintf(s, sizeof s, L->s[S_SALA_INVITES], p ? p->name : "?");
            text_sel(SALA_X + 6, y + 2, on, C_GOLD, s);
            buttons(SALA_X + SALA_W - 6, y, S_SALA_ACCEPT, S_SALA_DECLINE, on);
        } else if (id == SALA_LEAVE) {
            text_sel(SALA_X + 6, y + 2, on, C_SOFT, L->s[S_SALA_LEAVE]);
        } else if (id == SALA_ADD) {
            if (s_editing) {
                char s[ADDR + 2];
                SDL_snprintf(s, sizeof s, "%s_", s_edit);
                text_sel(SALA_X + 6, y + 2, on, C_GOLD, s);
            } else {
                text_sel(SALA_X + 6, y + 2, on, net_pair_state() == 1 ? C_GOLD : C_SOFT, L->s[net_pair_state() == 1 ? S_SALA_PAIRING : S_SALA_ADD]);
            }
        } else if (id >= SALA_MEMBER && id < SALA_MEMBER + s_nmem) {
            player_line(y, &s_mem[id - SALA_MEMBER], on, id);
        } else if (id >= SALA_NEAR && id < SALA_NEAR + s_nav) {
            player_line(y, &s_av[id - SALA_NEAR], on, id);
        }
    }
    if (!net_up()) {
        text(SALA_X + 4, SALA_Y + h + 2, 1, C_RED, L->s[S_SALA_NO_PORT]);
    }
}

/* a player's team and ping, for the line under Play */
static const char *detail(const NetPlayer *p)
{
    static char d[200];
    SDL_snprintf(d, sizeof d, "%s:", p->name);
    for (int i = 0; i < p->count; i++) {
        SDL_snprintf(d + SDL_strlen(d), sizeof d - SDL_strlen(d), "%s %s %d", i ? " -" : "", team_name(p->partner[i]), p->level[i]);
    }
    if (p->online && p->rtt >= 0) {
        SDL_snprintf(d + SDL_strlen(d), sizeof d - SDL_strlen(d), " - %d ms", p->rtt);
    } else if (!p->online) {
        SDL_snprintf(d + SDL_strlen(d), sizeof d - SDL_strlen(d), " - %s", L->s[S_SALA_OFFLINE]);
    }
    return d;
}

const char *sala_desc(int id)
{
    static char d[400];
    refresh();
    if (id == SALA_ADD) {
        char mine[200];
        if (s_editing) {
            return L->s[S_COOP_TYPING];
        }
        net_my_addresses(mine, sizeof mine);
        SDL_snprintf(d, sizeof d, "%s %s %s", L->s[S_SALA_ADD_D], L->s[S_COOP_MINE_D], mine[0] ? mine : L->s[S_COOP_NO_IP]);
        return d;
    }
    if (id == SALA_READY) {
        return L->s[S_SALA_READY_D];
    }
    if (id == SALA_LEAVE) {
        return L->s[S_SALA_LEAVE_D];
    }
    if (id == SALA_INVITE) {
        return L->s[S_SALA_INVITE_D];
    }
    if (id >= SALA_MEMBER && id < SALA_MEMBER + s_nmem) {
        return detail(&s_mem[id - SALA_MEMBER]);
    }
    if (id >= SALA_NEAR && id < SALA_NEAR + s_nav) {
        return detail(&s_av[id - SALA_NEAR]);
    }
    return "";
}
