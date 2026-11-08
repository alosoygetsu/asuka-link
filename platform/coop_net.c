/* Co-op's network, apart from coop.c because winsock's windows.h and platform.h both define RECT.

   Up to COOP_PEERS other players: coop_peers = a, b, c in native.ini (coop_peer, one, also works), each host or
   host:port (27500 without). A mesh: each copy sends its player's state over UDP every field frame to every peer and
   listens on coop_port; there is no host, so one leaving changes nothing for the rest. A packet counts for the peer
   whose address it comes from, others are ignored.

   Each peer is drawn as it was a moment ago on its own clock (80 ms, more with jitter), between the two states
   received around that time, so late or lost packets don't make it jump. No packet for COOP_TIMEOUT_MS: that peer is
   gone (off the field, in a battle, or the game closed) and hidden. */
#include <winsock2.h>
#include <ws2tcpip.h>
#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "../include/native.h"
#include "coop_net.h"

#define COOP_MAGIC 0x43335744u /* "DW3C" */
#define COOP_VERSION 2
#define COOP_BUFFER_MS 80 /* the least; more with jitter */
#define COOP_TIMEOUT_MS 2000
#define COOP_RX 32 /* states kept, ~0.6 s at 50 Hz */
#define COOP_REPORT_MS 60000 /* the ping and lost packets in coop.log, once a minute */
#define COOP_DEFAULT_PORT "27500"

char g_coop_peer[256]; /* coop_peers / coop_peer (coop.c's native_coop_ini) */
int g_coop_port = 27500; /* coop_port */
int g_coop_net_on;

typedef struct CoopPacket {
    uint32_t magic;
    uint16_t version, size;
    uint32_t seq;
    uint32_t time;     /* the sender's ms */
    uint32_t ack_time; /* the newest packet's time the sender got from the one it goes to, 0 none: the ping */
    uint32_t ack_age;  /* ms between that packet's arrival and this one leaving */
    int32_t mode, stageFile, x, y, z, dir, animSet, depth;
} CoopPacket;

typedef struct CoopRx {
    uint32_t time;
    NativeCoopState state;
} CoopRx;

typedef struct CoopPeer {
    char name[80];                 /* as coop_peers says it, for coop.log */
    struct sockaddr_in6 addr;      /* a v4 peer as ::ffff:a.b.c.d (the socket is dual-stack) */
    uint32_t last_seq;
    int have_seq;
    CoopRx rx[COOP_RX];            /* by arrival, rx_head the next */
    int rx_head, rx_count;
    Uint64 last_rx;                /* local ms of the last packet */
    double clock_off;              /* local ms - peer ms, the smallest seen (drifting) */
    double jitter;                 /* how much later than the fastest the packets come, smoothed */
    int here;                      /* sending: for coop.log's here / gone */
    uint32_t ack_time;             /* its newest packet's time, sent back for the ping */
    Uint64 ack_at;                 /* when it came, local ms */
    double rtt;                    /* ms, smoothed; < 0 none yet */
    uint32_t got, first_seq;       /* packets taken since it came, and its first seq */
    Uint64 report_at;
    int said_version;
} CoopPeer;

static SOCKET s_sock = INVALID_SOCKET;
static int s_net_tried;
static CoopPeer s_peers[COOP_PEERS];
static int s_peer_count;
static uint32_t s_seq;
static int s_said_stranger;

/* --- coop.log next to the exe: what a player can send when it doesn't connect --- */

static FILE *s_log;

static void coop_log(const char *fmt, ...) {
    va_list ap;
    char line[400];

    va_start(ap, fmt);
    SDL_vsnprintf(line, sizeof line, fmt, ap);
    va_end(ap);
    native_log("coop: %s", line);
    if (s_log == NULL) {
        char path[1100];

        SDL_snprintf(path, sizeof path, "%scoop.log", SDL_GetBasePath());
        s_log = fopen(path, "w");
    }
    if (s_log != NULL) {
        SDL_Time t;
        SDL_DateTime dt;

        if (SDL_GetCurrentTime(&t) && SDL_TimeToDateTime(t, &dt, true)) {
            fprintf(s_log, "%02d:%02d:%02d ", dt.hour, dt.minute, dt.second);
        }
        fprintf(s_log, "%s\n", line);
        fflush(s_log);
    }
}

/* How far behind a peer is drawn: enough that a late packet is in by then, for packets that come 20 ms apart
   (50 a second) and later than the fastest by its jitter on average */
static int buffer_ms(const CoopPeer *p) {
    return SDL_clamp(COOP_BUFFER_MS + (int)(p->jitter * 3), COOP_BUFFER_MS, 300);
}

/* "host", "host:port" or "[v6]:port" into p->addr: 1 when it was found */
static int resolve(CoopPeer *p, const char *entry) {
    char host[128], port[16] = COOP_DEFAULT_PORT;
    const char *colon = SDL_strrchr(entry, ':');
    struct addrinfo hints, *res = NULL;

    SDL_strlcpy(p->name, entry, sizeof p->name);
    if (entry[0] == '[') { /* [v6] or [v6]:port */
        const char *close = SDL_strchr(entry, ']');

        if (close == NULL) {
            return 0;
        }
        SDL_strlcpy(host, entry + 1, SDL_min(sizeof host, (size_t)(close - entry)));
        if (close[1] == ':') {
            SDL_strlcpy(port, close + 2, sizeof port);
        }
    } else if (colon != NULL && SDL_strchr(entry, ':') == colon) { /* one colon: host:port */
        SDL_strlcpy(host, entry, SDL_min(sizeof host, (size_t)(colon - entry + 1)));
        SDL_strlcpy(port, colon + 1, sizeof port);
    } else { /* no colon, or a bare v6 address */
        SDL_strlcpy(host, entry, sizeof host);
    }
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(host, port, &hints, &res) != 0 || res == NULL) {
        return 0;
    }
    memset(&p->addr, 0, sizeof p->addr);
    if (res->ai_family == AF_INET) {
        const struct sockaddr_in *v4 = (const void *)res->ai_addr;

        p->addr.sin6_family = AF_INET6;
        p->addr.sin6_port = v4->sin_port;
        p->addr.sin6_addr.s6_addr[10] = 0xFF;
        p->addr.sin6_addr.s6_addr[11] = 0xFF;
        memcpy(&p->addr.sin6_addr.s6_addr[12], &v4->sin_addr, 4);
    } else {
        memcpy(&p->addr, res->ai_addr, SDL_min(sizeof p->addr, (size_t)res->ai_addrlen));
    }
    freeaddrinfo(res);
    return 1;
}

static CoopPeer *peer_of(const struct sockaddr_storage *from) {
    const struct sockaddr_in6 *a = (const void *)from;
    int i;

    if (from->ss_family != AF_INET6) {
        return NULL;
    }
    for (i = 0; i < s_peer_count; i++) {
        if (memcmp(&a->sin6_addr, &s_peers[i].addr.sin6_addr, sizeof a->sin6_addr) == 0 && a->sin6_port == s_peers[i].addr.sin6_port) {
            return &s_peers[i];
        }
    }
    return NULL;
}

/* Opens the socket the first time it's needed: 1 when the network is on */
int coop_net_start(void) {
    struct sockaddr_in6 bind_addr;
    WSADATA wsa;
    u_long nonblock = 1;
    DWORD off = 0;
    char list[sizeof g_coop_peer], *entry, *next;

    if (s_net_tried) {
        return g_coop_net_on;
    }
    s_net_tried = 1;
    if (g_coop_peer[0] == 0) {
        coop_log("no other players' addresses yet (launcher: Settings, Mods, Co-op; or coop_peers in native.ini)");
        return 0;
    }
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        coop_log("WSAStartup failed");
        return 0;
    }
    SDL_strlcpy(list, g_coop_peer, sizeof list);
    for (entry = list; entry != NULL && s_peer_count < COOP_PEERS; entry = next) {
        next = SDL_strchr(entry, ',');
        if (next != NULL) {
            *next++ = 0;
        }
        while (*entry == ' ' || *entry == '\t') {
            entry++;
        }
        for (char *e = entry + SDL_strlen(entry); e > entry && (e[-1] == ' ' || e[-1] == '\t'); ) {
            *--e = 0;
        }
        if (*entry == 0) {
            continue;
        }
        CoopPeer *p = &s_peers[s_peer_count];

        memset(p, 0, sizeof *p);
        p->rtt = -1;
        if (resolve(p, entry)) {
            s_peer_count++;
        } else {
            coop_log("can't find %s (coop_peers): left out", entry);
        }
    }
    if (next != NULL) {
        coop_log("coop_peers has more than %d players: the rest are left out", COOP_PEERS);
    }
    if (s_peer_count == 0) {
        return 0;
    }
    s_sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (s_sock == INVALID_SOCKET) {
        coop_log("no socket (%d)", WSAGetLastError());
        return 0;
    }
    setsockopt(s_sock, IPPROTO_IPV6, IPV6_V6ONLY, (const char *)&off, sizeof off);
    memset(&bind_addr, 0, sizeof bind_addr);
    bind_addr.sin6_family = AF_INET6;
    bind_addr.sin6_port = htons((u_short)g_coop_port);
    /* sin6_addr all zero: any address (in6addr_any is a data import the stub check trips on) */
    if (bind(s_sock, (struct sockaddr *)&bind_addr, sizeof bind_addr) != 0) {
        coop_log("port %d is taken (%d); another copy of the game? coop_port in native.ini", g_coop_port, WSAGetLastError());
        closesocket(s_sock);
        s_sock = INVALID_SOCKET;
        return 0;
    }
    ioctlsocket(s_sock, FIONBIO, &nonblock);
    for (int i = 0; i < s_peer_count; i++) {
        coop_log("player %d: %s", i + 2, s_peers[i].name);
    }
    coop_log("listening on port %d, %d other player%s", g_coop_port, s_peer_count, s_peer_count > 1 ? "s" : "");
    g_coop_net_on = 1;
    return 1;
}

int coop_net_peer_count(void) {
    return s_peer_count;
}

void coop_net_send(const NativeCoopState *now) {
    CoopPacket p;
    Uint64 t = SDL_GetTicks();

    p.magic = COOP_MAGIC;
    p.version = COOP_VERSION;
    p.size = sizeof p;
    p.seq = ++s_seq;
    p.time = (uint32_t)t;
    p.mode = now->mode;
    p.stageFile = now->stageFile;
    p.x = now->x;
    p.y = now->y;
    p.z = now->z;
    p.dir = now->dir;
    p.animSet = now->animSet;
    p.depth = now->depth;
    for (int i = 0; i < s_peer_count; i++) {
        CoopPeer *peer = &s_peers[i];

        p.ack_time = peer->ack_time;
        p.ack_age = peer->ack_time != 0 ? (uint32_t)(t - peer->ack_at) : 0;
        sendto(s_sock, (const char *)&p, sizeof p, 0, (const struct sockaddr *)&peer->addr, sizeof peer->addr);
    }
}

static void take(CoopPeer *peer, const CoopPacket *p) {
    Uint64 now = SDL_GetTicks();

    /* a restarted peer starts its seq again: take it after a silence */
    if (peer->have_seq && (int32_t)(p->seq - peer->last_seq) <= 0 && now - peer->last_rx < COOP_TIMEOUT_MS) {
        return; /* late or doubled */
    }
    double off = (double)now - (double)p->time;

    if (!peer->have_seq || now - peer->last_rx >= COOP_TIMEOUT_MS) {
        peer->rx_count = 0;
        peer->clock_off = off;
        peer->jitter = 0;
        peer->got = 0;
        peer->first_seq = p->seq;
        peer->rtt = -1;
        peer->report_at = now + 5000; /* the first ping once it settled */
        peer->here = 1;
        coop_log("player %d (%s) is here", (int)(peer - s_peers) + 2, peer->name);
    } else if (off < peer->clock_off) {
        peer->clock_off = off; /* this one came faster */
    } else {
        peer->clock_off += (off - peer->clock_off) * 0.002; /* the clocks drift apart slowly */
        peer->jitter += ((off - peer->clock_off) - peer->jitter) * 0.05;
    }
    peer->got++;
    peer->ack_time = p->time;
    peer->ack_at = now;
    if (p->ack_time != 0) {
        double rtt = (double)(int32_t)((uint32_t)now - p->ack_time - p->ack_age);

        if (rtt >= 0 && rtt < 10000) {
            peer->rtt = peer->rtt < 0 ? rtt : peer->rtt + (rtt - peer->rtt) * 0.1;
        }
    }
    peer->have_seq = 1;
    peer->last_seq = p->seq;
    peer->last_rx = now;
    CoopRx *r = &peer->rx[peer->rx_head];

    r->time = p->time;
    r->state.mode = p->mode;
    r->state.stageFile = p->stageFile;
    r->state.x = p->x;
    r->state.y = p->y;
    r->state.z = p->z;
    r->state.dir = p->dir;
    r->state.animSet = p->animSet;
    r->state.depth = p->depth;
    peer->rx_head = (peer->rx_head + 1) % COOP_RX;
    if (peer->rx_count < COOP_RX) {
        peer->rx_count++;
    }
}

void coop_net_receive(void) {
    CoopPacket p;
    struct sockaddr_storage from;
    int len, flen;

    for (;;) {
        flen = sizeof from;
        len = recvfrom(s_sock, (char *)&p, sizeof p, 0, (struct sockaddr *)&from, &flen);
        if (len == SOCKET_ERROR) {
            /* WSAEWOULDBLOCK: nothing more; WSAECONNRESET: a peer's port closed (an ICMP), go on */
            if (WSAGetLastError() == WSAECONNRESET) {
                continue;
            }
            break;
        }
        if (len < 8 || p.magic != COOP_MAGIC) {
            continue;
        }
        CoopPeer *peer = peer_of(&from);

        if (peer == NULL) {
            if (!s_said_stranger) {
                char who[64] = "?";
                const struct sockaddr_in6 *a = (const void *)&from;

                s_said_stranger = 1;
                if (from.ss_family == AF_INET6) {
                    inet_ntop(AF_INET6, &a->sin6_addr, who, sizeof who);
                }
                coop_log("a packet from %s port %d, which isn't in coop_peers (%s): ignored. Is it the other player's "
                         "IP, and the port their coop_port?", who, ntohs(a->sin6_port), g_coop_peer);
            }
            continue;
        }
        if (len != sizeof p || p.version != COOP_VERSION || p.size != sizeof p) {
            if (!peer->said_version) {
                peer->said_version = 1;
                coop_log("player %d (%s) has another version of the co-op (%d, this is %d): update both",
                         (int)(peer - s_peers) + 2, peer->name, len >= 8 ? p.version : 0, COOP_VERSION);
            }
            continue;
        }
        take(peer, &p);
    }
    Uint64 now = SDL_GetTicks();

    for (int i = 0; i < s_peer_count; i++) {
        CoopPeer *peer = &s_peers[i];

        if (peer->here && now - peer->last_rx >= COOP_TIMEOUT_MS) {
            peer->here = 0;
            peer->ack_time = 0;
            coop_log("player %d (%s) is gone (nothing for %d s: off the field, in a battle, or the game closed)", i + 2,
                     peer->name, COOP_TIMEOUT_MS / 1000);
        }
        if (peer->here && now >= peer->report_at) {
            uint32_t sent = peer->last_seq - peer->first_seq + 1;

            peer->report_at = now + COOP_REPORT_MS;
            coop_log("player %d: ping %d ms, jitter %d ms, drawn %d ms behind, %d%% of their packets lost", i + 2,
                     (int)peer->rtt, (int)peer->jitter, buffer_ms(peer),
                     sent > 0 && peer->got <= sent ? (int)(100 - 100.0 * peer->got / sent) : 0);
        }
    }
}

static int lerp(int a, int b, double t) {
    return a + (int)((b - a) * t);
}

/* Peer i as it was a moment ago on its clock, between the states around it: 1, or 0 when it's gone */
int coop_net_peer(int i, NativeCoopState *out) {
    Uint64 now = SDL_GetTicks();
    const CoopPeer *peer;

    if (i < 0 || i >= s_peer_count) {
        return 0;
    }
    peer = &s_peers[i];
    if (peer->rx_count == 0 || now - peer->last_rx >= COOP_TIMEOUT_MS) {
        return 0;
    }
    double want = (double)now - peer->clock_off - buffer_ms(peer); /* on the peer's clock */
    const CoopRx *after = NULL;

    /* newest to oldest: the first at or before want, and the one after it */
    for (int k = 1; k <= peer->rx_count; k++) {
        const CoopRx *r = &peer->rx[(peer->rx_head + COOP_RX - k) % COOP_RX];

        if ((double)r->time <= want) {
            *out = r->state;
            if (after != NULL && after->state.mode == r->state.mode && after->state.stageFile == r->state.stageFile) {
                double span = (double)after->time - (double)r->time;
                double t = span > 0 ? (want - (double)r->time) / span : 0;
                int dx = after->state.x - r->state.x, dy = after->state.y - r->state.y;

                /* not across a jump (a door in the same file) */
                if (t > 0 && t <= 1 && dx < (64 << 8) && dx > -(64 << 8) && dy < (64 << 8) && dy > -(64 << 8)) {
                    out->x = lerp(r->state.x, after->state.x, t);
                    out->y = lerp(r->state.y, after->state.y, t);
                    out->z = lerp(r->state.z, after->state.z, t);
                }
            }
            return 1;
        }
        after = r;
    }
    *out = peer->rx[(peer->rx_head + COOP_RX - peer->rx_count) % COOP_RX].state; /* all newer than want: the oldest */
    return 1;
}
