/* The room's network (la sala): the launchers talk over UDP on their own port (27499, lobby_port in launcher.ini), apart
   from the game's (27500). Each one says who it is once a second (HELLO): to the local networks' broadcast, so the
   others there see it as available, and to the players it knows (its room, the recent ones, the one being paired
   with), which also measures the ping. Nobody leads a room: each member keeps its list, and a change (someone joins,
   is taken out) goes to all of them as ROOM with a revision number; the higher one wins.
   - INVITE: the other one sees it and answers ACCEPT or DECLINE; on ACCEPT the inviter adds them and sends ROOM.
   - PAIR (an address typed in): the same, answered ACCEPT without asking.
   - LEAVE: to the members, when you leave or join another room. Taken out: ROOM without you, to you too.
   - REJOIN: at start, to the last room's members for a minute; taken (as PAIR) only by one that is alone.
   The room's other members go in native.ini as coop_peers (ip:game port), which the game follows.
   Every packet is one fixed struct, checked for its magic and size; the network's version (NET_VERSION, the game's
   COOP_VERSION) is in it, and a player with another one shows as such and can't be invited. */
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdarg.h>
#include <stdio.h>
#include "launcher.h"

#define NET_VERSION 8 /* the game's COOP_VERSION (platform/coop_net.c): both change together */
#define LOBBY_PORT 27499
#define KNOWN 24
#define RECENTS 6

enum { T_HELLO = 1, T_INVITE, T_PAIR, T_ACCEPT, T_DECLINE, T_ROOM, T_LEAVE, T_REJOIN };

#pragma pack(push, 1)
typedef struct {
    Uint32 id, ip;
    Uint16 port, game_port;
} Member;

typedef struct {
    char magic[4]; /* "ALSA" */
    Uint8 proto, net, type, ready;
    Uint32 from, t, echo_t, echo_hold; /* t: the sender's clock (ms); echo_t: the receiver's last t, back, held echo_hold ms: the ping */
    char name[16];
    Sint8 model, tone;
    Uint8 count, room_size;
    Uint8 partner[3], level[3];
    Uint16 game_port;
    Uint32 room, owner, rev;
    Uint8 members;
    Member member[ROOM_MAX];
} Packet;
#pragma pack(pop)

typedef struct {
    NetPlayer p;
    Uint32 ip;
    Uint16 port;
    Uint64 heard;   /* SDL_GetTicks of its last HELLO, 0 never */
    Uint32 their_t; /* its last t, echoed back */
    Uint64 their_t_at; /* when it came */
    Uint64 other_room_since; /* a member saying it's in another room since then */
} Known;

static SOCKET s_sock = INVALID_SOCKET;
static Uint16 s_port = LOBBY_PORT, s_game_port = 27500;
static Uint32 s_id;
static Known s_known[KNOWN];
static Uint64 s_last_hello;
/* the room: you are always in member[0] */
static Uint32 s_room, s_owner, s_rev;
static Member s_member[ROOM_MAX];
static int s_members = 1;
static bool s_ready;
/* an invitation to answer */
static Uint32 s_invite_from;
static Uint64 s_invite_at;
/* pairing with an address typed in, until it answers or 10 s go by */
static Uint32 s_pair_ip;
static Uint16 s_pair_port;
static Uint64 s_pair_since;
static int s_pair_state; /* 0 none, 1 waiting, 2 no answer */
/* the ones you played with (launcher.ini recent_1..), and the last room (last_room), asked back at start */
static struct { char name[16]; Uint32 ip; Uint16 port; } s_recent[RECENTS];
static int s_recents;
static Uint32 s_rejoin_ip[ROOM_MAX];
static Uint16 s_rejoin_port[ROOM_MAX];
static int s_rejoins;
static Uint64 s_rejoin_until;
/* extra addresses to say hello to (tests: two launchers on one PC), DW3_LOBBY_PEERS = 127.0.0.1:27509, ... */
static Uint32 s_extra_ip[4];
static Uint16 s_extra_port[4];
static int s_extras;

/* sala.log next to the launcher: what came and went, but the HELLOs (once a second each) */
static void note(const char *fmt, ...)
{
    static FILE *f;
    static bool tried;
    if (!tried) {
        char path[1200];
        tried = true;
        path_in_root(path, sizeof path, "sala.log");
        f = fopen(path, "w");
    }
    if (f) {
        va_list ap;
        va_start(ap, fmt);
        fprintf(f, "%7.1f ", SDL_GetTicks() / 1000.0);
        vfprintf(f, fmt, ap);
        fputc('\n', f);
        fflush(f);
        va_end(ap);
    }
}

static const char *const TYPE_NAME[] = {"?", "HELLO", "INVITE", "PAIR", "ACCEPT", "DECLINE", "ROOM", "LEAVE", "REJOIN"};

/* ---- addresses ---- */

static bool parse_addr(const char *s, Uint32 *ip, Uint16 *port, Uint16 def)
{
    char host[96];
    const char *colon = SDL_strrchr(s, ':');
    SDL_strlcpy(host, s, colon ? SDL_min((size_t)(colon - s + 1), sizeof host) : sizeof host);
    *port = colon && SDL_atoi(colon + 1) > 0 && SDL_atoi(colon + 1) < 65536 ? (Uint16)SDL_atoi(colon + 1) : def;
    struct addrinfo hints = {0}, *res = NULL;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(host, NULL, &hints, &res) != 0 || res == NULL) {
        return false;
    }
    *ip = ((struct sockaddr_in *)res->ai_addr)->sin_addr.s_addr;
    freeaddrinfo(res);
    return true;
}

static void addr_text(Uint32 ip, Uint16 port, char *out, int size, bool with_port)
{
    const Uint8 *b = (const Uint8 *)&ip;
    if (with_port) {
        SDL_snprintf(out, size, "%d.%d.%d.%d:%d", b[0], b[1], b[2], b[3], port);
    } else {
        SDL_snprintf(out, size, "%d.%d.%d.%d", b[0], b[1], b[2], b[3]);
    }
}

/* ---- you ---- */

static void me(Packet *p)
{
    const Team *t = team_get();
    DWORD n = sizeof p->name;
    char pc[MAX_COMPUTERNAME_LENGTH + 1];
    SDL_zerop(p);
    SDL_memcpy(p->magic, "ALSA", 4);
    p->proto = 1;
    p->net = NET_VERSION;
    p->ready = s_ready;
    p->from = s_id;
    p->t = (Uint32)SDL_GetTicks();
    if (t && t->name[0]) {
        SDL_strlcpy(p->name, t->name, sizeof p->name);
    } else {
        static char computer[MAX_COMPUTERNAME_LENGTH + 1];
        if (!computer[0]) {
            n = sizeof pc;
            SDL_strlcpy(computer, GetComputerNameA(pc, &n) ? pc : "?", sizeof computer);
        }
        SDL_strlcpy(p->name, computer, sizeof p->name);
    }
    p->model = (Sint8)(t ? t->model : -1);
    p->tone = (Sint8)(t ? t->tone : -1);
    if (t) {
        p->count = (Uint8)t->count;
        for (int i = 0; i < t->count && i < 3; i++) {
            p->partner[i] = (Uint8)t->partner[i];
            p->level[i] = (Uint8)t->level[i];
        }
    }
    p->game_port = s_game_port;
    p->room = s_room;
    p->owner = s_owner;
    p->rev = s_rev;
    p->room_size = (Uint8)s_members;
    p->members = (Uint8)s_members;
    SDL_memcpy(p->member, s_member, sizeof s_member);
}

static void send_to(Packet *p, Uint32 ip, Uint16 port)
{
    struct sockaddr_in to = {0};
    to.sin_family = AF_INET;
    to.sin_addr.s_addr = ip;
    to.sin_port = htons(port);
    sendto(s_sock, (const char *)p, sizeof *p, 0, (struct sockaddr *)&to, sizeof to);
}

static Known *known(Uint32 id)
{
    for (int i = 0; i < KNOWN; i++) {
        if (s_known[i].heard && s_known[i].p.id == id) {
            return &s_known[i];
        }
    }
    return NULL;
}

static void send_type(int type, Uint32 ip, Uint16 port)
{
    Packet p;
    me(&p);
    p.type = (Uint8)type;
    if (type != T_HELLO) {
        const Uint8 *b = (const Uint8 *)&ip;
        note("out %s to %d.%d.%d.%d:%d", TYPE_NAME[type], b[0], b[1], b[2], b[3], port);
    }
    for (int i = 0; i < KNOWN; i++) {
        if (s_known[i].heard && s_known[i].ip == ip && s_known[i].port == port) {
            p.echo_t = s_known[i].their_t;
            p.echo_hold = (Uint32)(SDL_GetTicks() - s_known[i].their_t_at);
        }
    }
    send_to(&p, ip, port);
}

/* ---- the room ---- */

static int member_index(Uint32 id)
{
    for (int i = 0; i < s_members; i++) {
        if (s_member[i].id == id) {
            return i;
        }
    }
    return -1;
}

/* the others' addresses, ip:game port, to native.ini's coop_peers (the game follows them) */
static void write_peers(void)
{
    char v[ROOM_MAX * 32] = "";
    for (int i = 1; i < s_members; i++) {
        char a[32];
        addr_text(s_member[i].ip, s_member[i].game_port, a, sizeof a, true);
        SDL_snprintf(v + SDL_strlen(v), sizeof v - SDL_strlen(v), "%s%s", v[0] ? ", " : "", a);
    }
    IniEdit edits[2] = {{"coop_peers", v[0] ? v : NULL, false}, {"coop_peer", NULL, false}};
    ini_write("native.ini", edits, 2);
    /* and the room, to ask back at the next start */
    char last[ROOM_MAX * 32] = "";
    for (int i = 1; i < s_members; i++) {
        char a[32];
        addr_text(s_member[i].ip, s_member[i].port, a, sizeof a, true);
        SDL_snprintf(last + SDL_strlen(last), sizeof last - SDL_strlen(last), "%s%s", last[0] ? ", " : "", a);
    }
    IniEdit room = {"last_room", last[0] ? last : NULL, false};
    ini_write("launcher.ini", &room, 1);
}

static void remember(const Known *k)
{
    int at = 0;
    while (at < s_recents && !(s_recent[at].ip == k->ip && s_recent[at].port == k->port)) {
        at++;
    }
    if (at == s_recents) {
        at = SDL_min(s_recents, RECENTS - 1);
        s_recents = SDL_min(s_recents + 1, RECENTS);
    }
    for (int i = at; i > 0; i--) { /* the newest first */
        s_recent[i] = s_recent[i - 1];
    }
    SDL_strlcpy(s_recent[0].name, k->p.name, sizeof s_recent[0].name);
    s_recent[0].ip = k->ip;
    s_recent[0].port = k->port;
    char key[16], v[RECENTS][64];
    IniEdit edits[RECENTS];
    for (int i = 0; i < RECENTS; i++) {
        char a[32];
        SDL_snprintf(key, sizeof key, "recent_%d", i + 1);
        addr_text(s_recent[i].ip, s_recent[i].port, a, sizeof a, true);
        SDL_snprintf(v[i], sizeof v[i], "%s|%s", s_recent[i].name, a);
        edits[i] = (IniEdit){SDL_strdup(key), i < s_recents ? v[i] : NULL, false};
    }
    ini_write("launcher.ini", edits, RECENTS);
    for (int i = 0; i < RECENTS; i++) {
        SDL_free((void *)edits[i].key);
    }
}

static void room_changed(void)
{
    if (s_members > 1) {
        s_rejoins = 0; /* the start's asking back is over once there's a room */
    }
    write_peers();
    for (int i = 1; i < s_members; i++) {
        const Known *k = known(s_member[i].id);
        if (k) {
            remember(k);
        }
    }
}

static void send_room(Uint32 also_ip, Uint16 also_port)
{
    Packet p;
    me(&p);
    p.type = T_ROOM;
    for (int i = 1; i < s_members; i++) {
        send_to(&p, s_member[i].ip, s_member[i].port);
    }
    if (also_ip) {
        send_to(&p, also_ip, also_port);
    }
}

static void alone(void)
{
    s_room = s_owner = s_id;
    s_rev = 0;
    s_members = 1;
    s_member[0] = (Member){s_id, 0, s_port, s_game_port};
    s_ready = false;
}

void net_leave(void)
{
    s_rejoins = 0; /* leaving means it: no asking back */
    for (int i = 1; i < s_members; i++) {
        send_type(T_LEAVE, s_member[i].ip, s_member[i].port);
    }
    alone();
    room_changed();
}

void net_kick(Uint32 id)
{
    s_rejoins = 0;
    const int i = member_index(id);
    if (i <= 0) {
        return;
    }
    const Member out = s_member[i];
    s_member[i] = s_member[--s_members];
    s_rev++;
    send_room(out.ip, out.port); /* to them too: they see they're out */
    room_changed();
}

/* ---- what comes in ---- */

static void heard(const Packet *p, Uint32 ip, Uint16 port)
{
    Known *k = known(p->from);
    if (!k) {
        Uint64 oldest = ~(Uint64)0;
        for (int i = 0; i < KNOWN; i++) {
            if (s_known[i].heard < oldest) {
                oldest = s_known[i].heard;
                k = &s_known[i];
            }
        }
        SDL_zerop(k);
    }
    k->p.id = p->from;
    SDL_strlcpy(k->p.name, p->name, sizeof k->p.name);
    k->p.model = p->model;
    k->p.tone = p->tone;
    k->p.count = SDL_min(p->count, 3);
    for (int i = 0; i < k->p.count; i++) {
        k->p.partner[i] = p->partner[i];
        k->p.level[i] = p->level[i];
    }
    k->p.ready = p->ready != 0;
    k->p.other_version = p->net != NET_VERSION;
    k->p.room_size = p->room_size;
    k->p.room = p->room;
    k->p.game_port = p->game_port;
    k->ip = ip;
    k->port = port;
    k->their_t = p->t;
    k->their_t_at = SDL_GetTicks();
    k->heard = SDL_GetTicks() | 1;
    if (p->echo_t) {
        const int rtt = (int)((Uint32)SDL_GetTicks() - p->echo_t - p->echo_hold);
        k->p.rtt = rtt >= 0 && rtt < 10000 ? rtt : k->p.rtt;
    }
}

/* a ROOM we're in: taken when it comes from one of its members and is newer than ours, or is the one we just
   joined (we answered its invitation or pairing) */
static void take_room(const Packet *p, Uint32 ip, Uint16 port)
{
    int mine = -1, theirs = -1;
    for (int i = 0; i < p->members && i < ROOM_MAX; i++) {
        mine = p->member[i].id == s_id ? i : mine;
        theirs = p->member[i].id == p->from ? i : theirs;
    }
    if (theirs < 0) {
        return;
    }
    if (mine < 0) { /* taken out, if it was our room */
        if (p->room == s_room && member_index(p->from) > 0) {
            alone();
            room_changed();
        }
        return;
    }
    const bool joined = p->room != s_room;
    if (!joined && p->rev <= s_rev) {
        return;
    }
    /* two rooms that took each other in at once (both asked back at start): the lower room's id wins, so both end
       in the same one instead of each in the other's */
    if (joined && member_index(p->from) > 0 && p->room > s_room) {
        return;
    }
    if (joined) {
        for (int i = 1; i < s_members; i++) { /* the old room's members, not in the new one, hear we left */
            bool stays = false;
            for (int j = 0; j < p->members; j++) {
                stays |= p->member[j].id == s_member[i].id;
            }
            if (!stays) {
                send_type(T_LEAVE, s_member[i].ip, s_member[i].port);
            }
        }
    }
    s_room = p->room;
    s_owner = p->owner;
    s_rev = p->rev;
    s_members = 1;
    s_member[0] = (Member){s_id, 0, s_port, s_game_port};
    for (int i = 0; i < p->members && i < ROOM_MAX; i++) {
        if (i != mine) {
            Member m = p->member[i];
            if (m.id == p->from) { /* the sender's as we see it, which works through NAT */
                m.ip = ip;
                m.port = port;
            }
            s_member[s_members++] = m;
        }
    }
    s_pair_state = 0;
    room_changed();
}

static void receive(const Packet *p, Uint32 ip, Uint16 port)
{
    if (SDL_memcmp(p->magic, "ALSA", 4) != 0 || p->proto != 1 || p->from == s_id) {
        return;
    }
    heard(p, ip, port);
    const Known *k = known(p->from);
    if (p->type != T_HELLO && p->type < SDL_arraysize(TYPE_NAME)) {
        const Uint8 *b = (const Uint8 *)&ip;
        note("in %s from %s (%08x) at %d.%d.%d.%d:%d, room %08x rev %u, %d members", TYPE_NAME[p->type], p->name, p->from, b[0], b[1], b[2], b[3],
             port, p->room, p->rev, p->members);
    }
    if (p->net != NET_VERSION) {
        return; /* seen, marked, nothing else */
    }
    switch (p->type) {
    case T_HELLO: {
        const int i = member_index(p->from);
        if (i > 0) {
            s_member[i].ip = ip; /* it may have moved */
            s_member[i].port = port;
        }
        break;
    }
    case T_INVITE:
        if (member_index(p->from) < 0) {
            s_invite_from = p->from;
            s_invite_at = SDL_GetTicks();
            SDL_FlashWindow(g_app.window, SDL_FLASH_UNTIL_FOCUSED);
        }
        break;
    case T_PAIR:
        if (member_index(p->from) < 0) {
            send_type(T_ACCEPT, ip, port);
        }
        break;
    case T_REJOIN: /* only alone: it doesn't take anyone out of a room */
        if (s_members == 1 && p->room_size == 1) {
            send_type(T_ACCEPT, ip, port);
        }
        break;
    case T_ACCEPT:
        if (member_index(p->from) < 0 && s_members < ROOM_MAX) {
            s_member[s_members++] = (Member){p->from, ip, port, p->game_port};
            s_rev++;
            send_room(0, 0);
            room_changed();
            s_pair_state = 0;
            SDL_FlashWindow(g_app.window, SDL_FLASH_BRIEFLY);
        }
        break;
    case T_DECLINE:
        if (k) {
            g_app.note = L->s[S_SALA_DECLINED];
        }
        break;
    case T_ROOM: take_room(p, ip, port); break;
    case T_LEAVE: {
        const int i = member_index(p->from);
        if (i > 0) {
            s_member[i] = s_member[--s_members];
            if (s_members == 1) {
                alone();
            }
            room_changed();
        }
        break;
    }
    default: break;
    }
}

/* ---- the clock ---- */

/* each up network's broadcast address, and the all-ones one */
static void broadcast(Packet *p)
{
    ULONG size = 16384;
    IP_ADAPTER_ADDRESSES *list = SDL_malloc(size);
    send_to(p, INADDR_BROADCAST, s_port);
    if (list && GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER, NULL, list, &size) == NO_ERROR) {
        for (IP_ADAPTER_ADDRESSES *a = list; a; a = a->Next) {
            if (a->OperStatus != IfOperStatusUp || a->IfType == IF_TYPE_SOFTWARE_LOOPBACK) {
                continue;
            }
            for (IP_ADAPTER_UNICAST_ADDRESS *u = a->FirstUnicastAddress; u; u = u->Next) {
                const Uint32 ip = ((struct sockaddr_in *)u->Address.lpSockaddr)->sin_addr.s_addr, bits = u->OnLinkPrefixLength;
                if (bits > 0 && bits < 31) {
                    const Uint32 mask = htonl(0xFFFFFFFFu << (32 - bits));
                    send_to(p, ip | ~mask, s_port);
                }
            }
        }
    }
    SDL_free(list);
}

static void hello_all(void)
{
    Packet p;
    me(&p);
    p.type = T_HELLO;
    broadcast(&p);
    /* the ones known by address: the room, the recent ones, the extra (tests), with the ping's echo */
    for (int i = 1; i < s_members; i++) {
        send_type(T_HELLO, s_member[i].ip, s_member[i].port);
    }
    for (int i = 0; i < s_recents; i++) {
        send_type(T_HELLO, s_recent[i].ip, s_recent[i].port);
    }
    for (int i = 0; i < s_extras; i++) {
        send_type(T_HELLO, s_extra_ip[i], s_extra_port[i]);
    }
    if (s_pair_state == 1) {
        if (SDL_GetTicks() - s_pair_since > 10000) {
            s_pair_state = 2;
        } else {
            send_type(T_PAIR, s_pair_ip, s_pair_port);
        }
    }
    if (s_rejoins && SDL_GetTicks() < s_rejoin_until && s_members == 1) {
        for (int i = 0; i < s_rejoins; i++) {
            send_type(T_REJOIN, s_rejoin_ip[i], s_rejoin_port[i]);
        }
    }
    /* a member that says it's in another room for 5 s has left it (its LEAVE was lost) */
    for (int i = s_members - 1; i >= 1; i--) {
        Known *k = known(s_member[i].id);
        if (k && SDL_GetTicks() - k->heard < 3000 && k->p.room != s_room) {
            if (!k->other_room_since) {
                k->other_room_since = SDL_GetTicks();
            } else if (SDL_GetTicks() - k->other_room_since > 5000) {
                s_member[i] = s_member[--s_members];
                if (s_members == 1) {
                    alone();
                }
                room_changed();
            }
        } else if (k) {
            k->other_room_since = 0;
        }
    }
}

void net_poll(void)
{
    if (s_sock == INVALID_SOCKET) {
        return;
    }
    Packet p;
    struct sockaddr_in from;
    int len = sizeof from, n;
    while ((n = recvfrom(s_sock, (char *)&p, sizeof p, 0, (struct sockaddr *)&from, &len)) >= 0 || WSAGetLastError() == WSAECONNRESET) {
        if (n == (int)sizeof p) {
            receive(&p, from.sin_addr.s_addr, ntohs(from.sin_port));
        }
        len = sizeof from;
    }
    if (SDL_GetTicks() - s_last_hello >= 1000) {
        s_last_hello = SDL_GetTicks();
        hello_all();
    }
    if (s_invite_from && SDL_GetTicks() - s_invite_at > 30000) {
        s_invite_from = 0; /* not answered: gone */
    }
}

/* ---- start ---- */

static void load_lists(void)
{
    char v[256], key[16];
    s_recents = 0;
    for (int i = 0; i < RECENTS; i++) {
        SDL_snprintf(key, sizeof key, "recent_%d", i + 1);
        char *bar;
        if (ini_get("launcher.ini", key, v, sizeof v) && (bar = SDL_strchr(v, '|')) != NULL) {
            *bar = 0;
            if (parse_addr(bar + 1, &s_recent[s_recents].ip, &s_recent[s_recents].port, LOBBY_PORT)) {
                SDL_strlcpy(s_recent[s_recents].name, v, sizeof s_recent[0].name);
                s_recents++;
            }
        }
    }
    /* the last room, asked back for a minute; else the addresses an older version had in coop_peers */
    s_rejoins = 0;
    const bool last = ini_get("launcher.ini", "last_room", v, sizeof v);
    if (last || ini_get("native.ini", "coop_peers", v, sizeof v)) {
        char *save = NULL;
        for (char *t = SDL_strtok_r(v, ", ", &save); t && s_rejoins < ROOM_MAX; t = SDL_strtok_r(NULL, ", ", &save)) {
            Uint16 port;
            if (parse_addr(t, &s_rejoin_ip[s_rejoins], &port, LOBBY_PORT)) {
                s_rejoin_port[s_rejoins++] = last ? port : LOBBY_PORT; /* coop_peers had the game's port */
            }
        }
    }
    s_rejoin_until = SDL_GetTicks() + 60000;
    const char *extra = SDL_getenv("DW3_LOBBY_PEERS");
    if (extra) {
        SDL_strlcpy(v, extra, sizeof v);
        char *save = NULL;
        for (char *t = SDL_strtok_r(v, ", ", &save); t && s_extras < 4; t = SDL_strtok_r(NULL, ", ", &save)) {
            if (parse_addr(t, &s_extra_ip[s_extras], &s_extra_port[s_extras], LOBBY_PORT)) {
                s_extras++;
            }
        }
    }
}

void net_init(void)
{
    WSADATA wsa;
    char v[32];
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        return;
    }
    if (ini_get("launcher.ini", "lobby_port", v, sizeof v) && SDL_atoi(v) > 0 && SDL_atoi(v) < 65536) {
        s_port = (Uint16)SDL_atoi(v);
    }
    if (SDL_getenv("DW3_LOBBY_PORT")) {
        s_port = (Uint16)SDL_atoi(SDL_getenv("DW3_LOBBY_PORT"));
    }
    if (ini_get("native.ini", "coop_port", v, sizeof v) && SDL_atoi(v) > 0 && SDL_atoi(v) < 65536) {
        s_game_port = (Uint16)SDL_atoi(v);
    }
    /* this launcher's id: random once, kept */
    /* the top bit clear: net_available marks a recent one not heard with it */
    if (ini_get("launcher.ini", "sala_id", v, sizeof v) && (SDL_strtoul(v, NULL, 16) & 0x7FFFFFFFu) != 0) {
        s_id = (Uint32)SDL_strtoul(v, NULL, 16) & 0x7FFFFFFFu;
    } else {
        SDL_srand(0);
        s_id = ((Uint32)SDL_rand_bits() & 0x7FFFFFFFu) | 1;
        SDL_snprintf(v, sizeof v, "%08x", s_id);
        IniEdit e = {"sala_id", v, false};
        ini_write("launcher.ini", &e, 1);
    }
    alone();
    load_lists();
    s_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s_sock == INVALID_SOCKET) {
        return;
    }
    BOOL yes = TRUE;
    u_long nonblock = 1;
    setsockopt(s_sock, SOL_SOCKET, SO_BROADCAST, (const char *)&yes, sizeof yes);
    ioctlsocket(s_sock, FIONBIO, &nonblock);
    struct sockaddr_in at = {0};
    at.sin_family = AF_INET;
    at.sin_addr.s_addr = INADDR_ANY;
    at.sin_port = htons(s_port);
    note("id %08x, port %d, game port %d, %d recent, %d to ask back", s_id, s_port, s_game_port, s_recents, s_rejoins);
    if (bind(s_sock, (struct sockaddr *)&at, sizeof at) != 0) {
        note("port %d taken", s_port);
        closesocket(s_sock); /* another launcher has the port: this one only shows the room it can't join */
        s_sock = INVALID_SOCKET;
    }
}

/* ---- for the screen ---- */

bool net_up(void) { return s_sock != INVALID_SOCKET; }

int net_members(NetPlayer *out, int max)
{
    int n = 0;
    Packet p;
    me(&p);
    if (n < max) {
        NetPlayer you = {0};
        you.id = s_id;
        SDL_strlcpy(you.name, p.name, sizeof you.name);
        you.model = p.model;
        you.tone = p.tone;
        you.count = p.count;
        for (int i = 0; i < p.count; i++) {
            you.partner[i] = p.partner[i];
            you.level[i] = p.level[i];
        }
        you.ready = s_ready;
        you.online = true;
        you.rtt = -1;
        you.you = true;
        out[n++] = you;
    }
    for (int i = 1; i < s_members && n < max; i++) {
        const Known *k = known(s_member[i].id);
        NetPlayer m = {0};
        if (k) {
            m = k->p;
            m.online = SDL_GetTicks() - k->heard < 4000;
        } else {
            m.id = s_member[i].id;
            addr_text(s_member[i].ip, 0, m.name, sizeof m.name, false);
            m.model = m.tone = -1;
            m.rtt = -1;
        }
        m.member = true;
        out[n++] = m;
    }
    return n;
}

/* the ones heard here (not in the room), then the recent ones not heard; each with whether it can be invited */
int net_available(NetPlayer *out, int max)
{
    int n = 0;
    for (int i = 0; i < KNOWN && n < max; i++) {
        const Known *k = &s_known[i];
        if (k->heard && SDL_GetTicks() - k->heard < 6000 && member_index(k->p.id) < 0) {
            out[n] = k->p;
            out[n].online = true;
            for (int r = 0; r < s_recents; r++) {
                out[n].recent |= s_recent[r].ip == k->ip && s_recent[r].port == k->port;
            }
            n++;
        }
    }
    for (int r = 0; r < s_recents && n < max; r++) {
        bool shown = false;
        for (int i = 0; i < KNOWN; i++) {
            const Known *k = &s_known[i];
            shown |= k->heard && k->ip == s_recent[r].ip && k->port == s_recent[r].port &&
                     (SDL_GetTicks() - k->heard < 6000 || member_index(k->p.id) >= 0);
        }
        if (!shown) {
            NetPlayer o = {0};
            o.id = 0x80000000u | (Uint32)r; /* not an id: the recent one's place */
            SDL_strlcpy(o.name, s_recent[r].name, sizeof o.name);
            o.model = o.tone = -1;
            o.rtt = -1;
            o.recent = true;
            out[n++] = o;
        }
    }
    return n;
}

bool net_can_invite(const NetPlayer *p)
{
    return p->online && !p->other_version && p->room_size + s_members <= ROOM_MAX;
}

void net_invite(Uint32 id)
{
    if (id & 0x80000000u) { /* a recent one not heard: asked at its address */
        const int r = (int)(id & 0xFF);
        if (r < s_recents) {
            send_type(T_INVITE, s_recent[r].ip, s_recent[r].port);
        }
        return;
    }
    const Known *k = known(id);
    if (k && s_members < ROOM_MAX) {
        send_type(T_INVITE, k->ip, k->port);
    }
}

bool net_pair(const char *addr)
{
    if (!parse_addr(addr, &s_pair_ip, &s_pair_port, LOBBY_PORT)) {
        s_pair_state = 2;
        return false;
    }
    s_pair_state = 1;
    s_pair_since = SDL_GetTicks();
    send_type(T_PAIR, s_pair_ip, s_pair_port);
    return true;
}

int net_pair_state(void) { return s_pair_state; }

const NetPlayer *net_invitation(void)
{
    const Known *k = s_invite_from ? known(s_invite_from) : NULL;
    return k ? &k->p : NULL;
}

void net_answer(bool yes)
{
    const Known *k = s_invite_from ? known(s_invite_from) : NULL;
    if (k) {
        send_type(yes ? T_ACCEPT : T_DECLINE, k->ip, k->port);
    }
    s_invite_from = 0;
}

void net_set_ready(bool ready)
{
    s_ready = ready;
    s_last_hello = 0; /* say it now */
}

bool net_ready(void) { return s_ready; }

bool net_all_ready(void)
{
    if (s_members < 2 || !s_ready) {
        return false;
    }
    for (int i = 1; i < s_members; i++) {
        const Known *k = known(s_member[i].id);
        if (!k || !k->p.ready || SDL_GetTicks() - k->heard > 4000) {
            return false;
        }
    }
    return true;
}

/* the room's name: its owner's ("Sala de Getsu") */
void net_room_name(char *out, int size)
{
    const char *owner = NULL;
    Packet p;
    me(&p);
    if (s_owner == s_id) {
        owner = p.name;
    } else {
        const Known *k = known(s_owner);
        owner = k ? k->p.name : NULL;
    }
    SDL_snprintf(out, size, owner ? L->s[S_SALA_OF] : "%s", owner ? owner : L->s[S_SALA]);
}

int net_room_size(void) { return s_members; }

/* this PC's IPv4 addresses, one each, Tailscale's first (tailscale[i] says which); looked up every 5 s at most */
int net_my_ips(char out[][20], bool *tailscale, int max)
{
    static char ip[6][20];
    static bool ts[6];
    static int n;
    static Uint64 at;
    if (!at || SDL_GetTicks() - at > 5000) {
        ULONG len = 16384;
        IP_ADAPTER_ADDRESSES *list = SDL_malloc(len);
        at = SDL_GetTicks() | 1;
        n = 0;
        if (list && GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER, NULL, list, &len) == NO_ERROR) {
            for (int pass = 0; pass < 2; pass++) { /* Tailscale's, then the rest */
                for (IP_ADAPTER_ADDRESSES *a = list; a; a = a->Next) {
                    if (a->OperStatus != IfOperStatusUp || a->IfType == IF_TYPE_SOFTWARE_LOOPBACK) {
                        continue;
                    }
                    for (IP_ADAPTER_UNICAST_ADDRESS *u = a->FirstUnicastAddress; u && n < 6; u = u->Next) {
                        const Uint32 v = ((struct sockaddr_in *)u->Address.lpSockaddr)->sin_addr.s_addr;
                        const Uint8 *b = (const Uint8 *)&v;
                        const bool t = b[0] == 100 && (b[1] & 0xC0) == 64;
                        if ((b[0] == 169 && b[1] == 254) || t != (pass == 0)) {
                            continue;
                        }
                        addr_text(v, 0, ip[n], sizeof ip[n], false);
                        ts[n++] = t;
                    }
                }
            }
        }
        SDL_free(list);
    }
    const int k = SDL_min(n, max);
    for (int i = 0; i < k; i++) {
        SDL_strlcpy(out[i], ip[i], 20);
        tailscale[i] = ts[i];
    }
    return k;
}

/* the addresses this PC has, for the description of Add by IP (Tailscale's first) */
void net_my_addresses(char *out, int size)
{
    ULONG len = 16384;
    IP_ADAPTER_ADDRESSES *list = SDL_malloc(len);
    char other[160] = "";
    out[0] = 0;
    if (list && GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER, NULL, list, &len) == NO_ERROR) {
        for (IP_ADAPTER_ADDRESSES *a = list; a; a = a->Next) {
            if (a->OperStatus != IfOperStatusUp || a->IfType == IF_TYPE_SOFTWARE_LOOPBACK) {
                continue;
            }
            for (IP_ADAPTER_UNICAST_ADDRESS *u = a->FirstUnicastAddress; u; u = u->Next) {
                const Uint32 ip = ((struct sockaddr_in *)u->Address.lpSockaddr)->sin_addr.s_addr;
                const Uint8 *b = (const Uint8 *)&ip;
                char a_[24];
                if (b[0] == 169 && b[1] == 254) {
                    continue;
                }
                addr_text(ip, 0, a_, sizeof a_, false);
                if (b[0] == 100 && (b[1] & 0xC0) == 64) {
                    SDL_snprintf(out + SDL_strlen(out), size - SDL_strlen(out), "%s%s (Tailscale)", out[0] ? ", " : "", a_);
                } else {
                    SDL_snprintf(other + SDL_strlen(other), sizeof other - SDL_strlen(other), "%s%s", other[0] ? ", " : "", a_);
                }
            }
        }
    }
    SDL_free(list);
    if (other[0]) {
        SDL_snprintf(out + SDL_strlen(out), size - SDL_strlen(out), "%s%s", out[0] ? ", " : "", other);
    }
    if (s_port != LOBBY_PORT) {
        SDL_snprintf(out + SDL_strlen(out), size - SDL_strlen(out), " (:%d)", s_port);
    }
}
