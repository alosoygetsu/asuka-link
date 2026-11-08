/* Co-op's network (milestone 2), apart from coop.c because winsock's
   windows.h and platform.h both define RECT. */
#include <winsock2.h>
#include <ws2tcpip.h>
#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "../include/native.h"
#include "coop_net.h"

/* --- The network: coop_peer = <host>:<port> in native.ini ---

   Each copy sends its player's state over UDP every field frame to coop_peer
   and listens on coop_port. Only packets from the peer's address count. The
   peer is drawn as it was COOP_BUFFER_MS ago on its clock, between the two
   states received around that time, so late or lost packets don't make it
   jump. No packet for COOP_TIMEOUT_MS: the peer is gone (off the field, or
   the game closed) and hidden. Without coop_peer, the ghost of milestone 1. */

#define COOP_MAGIC 0x43335744u /* "DW3C" */
#define COOP_VERSION 2
#define COOP_BUFFER_MS 80 /* the least; more with jitter */
#define COOP_TIMEOUT_MS 2000
#define COOP_RX 32 /* states kept, ~0.6 s at 50 Hz */
#define COOP_REPORT_MS 60000 /* the ping and lost packets in coop.log, once a minute */

char g_coop_peer[128]; /* coop_peer (main.c) */
int g_coop_port = 27500; /* coop_port (main.c) */

typedef struct CoopPacket {
    uint32_t magic;
    uint16_t version, size;
    uint32_t seq;
    uint32_t time;     /* the sender's ms */
    uint32_t ack_time; /* the newest packet's time the sender got from us, 0 none: the ping */
    uint32_t ack_age;  /* ms between that packet's arrival and this one leaving */
    int32_t mode, stageFile, x, y, z, dir, animSet, depth;
} CoopPacket;

/* How far behind the peer is drawn: enough that a late packet is in by then, for packets that come 20 ms
   apart (50 a second) and later than the fastest by s_jitter on average */
static double s_jitter; /* how much later than the fastest the packets come, smoothed */
static int coop_buffer_ms(void) {
    return SDL_clamp(COOP_BUFFER_MS + (int)(s_jitter * 3), COOP_BUFFER_MS, 300);
}

/* --- coop.log next to the exe: what a player can send when it doesn't connect --- */

static FILE *s_log;

static void coop_log(const char *fmt, ...) {
    va_list ap;
    char line[300];

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

typedef struct CoopRx {
    uint32_t time;
    NativeCoopState state;
} CoopRx;

static SOCKET s_sock = INVALID_SOCKET;
static struct sockaddr_storage s_peer_addr;
static int s_peer_len;
static int s_net_tried;
int g_coop_net_on;
static uint32_t s_seq, s_last_seq;
static int s_have_seq;
static CoopRx s_rx[COOP_RX]; /* by arrival, s_rx_head the next */
static int s_rx_head, s_rx_count;
static Uint64 s_last_rx; /* local ms of the last packet */
static double s_clock_off; /* local ms - peer ms, the smallest seen (drifting) */
static int s_here; /* the peer is sending: for coop.log's here / gone */
static uint32_t s_ack_time; /* the newest peer packet's time, sent back for the ping */
static Uint64 s_ack_at;     /* when it came, local ms */
static double s_rtt = -1;   /* ms, smoothed */
static uint32_t s_got, s_first_seq; /* packets taken since the peer came, and its first seq */
static Uint64 s_report_at;
static int s_said_stranger, s_said_version;

static int same_addr(const struct sockaddr_storage *a, int alen) {
    if (a->ss_family != s_peer_addr.ss_family) {
        return 0;
    }
    if (a->ss_family == AF_INET) {
        const struct sockaddr_in *p = (const void *)a, *q = (const void *)&s_peer_addr;
        return p->sin_addr.s_addr == q->sin_addr.s_addr && p->sin_port == q->sin_port;
    }
    if (a->ss_family == AF_INET6) {
        const struct sockaddr_in6 *p = (const void *)a, *q = (const void *)&s_peer_addr;
        return memcmp(&p->sin6_addr, &q->sin6_addr, sizeof p->sin6_addr) == 0 && p->sin6_port == q->sin6_port;
    }
    (void)alen;
    return 0;
}

/* Opens the socket the first time it's needed: 1 when the network is on */
int coop_net_start(void) {
    char host[128], port[16];
    const char *colon;
    struct addrinfo hints, *res = NULL;
    struct sockaddr_in6 bind_addr;
    WSADATA wsa;
    u_long nonblock = 1;
    DWORD off = 0;

    if (s_net_tried) {
        return g_coop_net_on;
    }
    s_net_tried = 1;
    if (g_coop_peer[0] == 0) {
        coop_log("no coop_peer in native.ini, the ghost of yourself (milestone 1)");
        return 0;
    }
    /* host:port, or [v6]:port */
    colon = SDL_strrchr(g_coop_peer, ':');
    if (colon == NULL || colon == g_coop_peer) {
        coop_log("coop_peer = %s: it needs a port, host:port", g_coop_peer);
        return 0;
    }
    if (g_coop_peer[0] == '[' && colon[-1] == ']') {
        SDL_strlcpy(host, g_coop_peer + 1, SDL_min(sizeof host, (size_t)(colon - g_coop_peer - 1)));
    } else {
        SDL_strlcpy(host, g_coop_peer, SDL_min(sizeof host, (size_t)(colon - g_coop_peer + 1)));
    }
    SDL_strlcpy(port, colon + 1, sizeof port);
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        coop_log("WSAStartup failed");
        return 0;
    }
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(host, port, &hints, &res) != 0 || res == NULL) {
        coop_log("can't find %s (coop_peer)", g_coop_peer);
        return 0;
    }
    /* a dual-stack socket: a v4 peer as ::ffff:a.b.c.d */
    if (res->ai_family == AF_INET) {
        struct sockaddr_in6 *m = (void *)&s_peer_addr;
        const struct sockaddr_in *v4 = (const void *)res->ai_addr;

        memset(m, 0, sizeof *m);
        m->sin6_family = AF_INET6;
        m->sin6_port = v4->sin_port;
        m->sin6_addr.s6_addr[10] = 0xFF;
        m->sin6_addr.s6_addr[11] = 0xFF;
        memcpy(&m->sin6_addr.s6_addr[12], &v4->sin_addr, 4);
    } else {
        memcpy(&s_peer_addr, res->ai_addr, res->ai_addrlen);
    }
    s_peer_len = sizeof(struct sockaddr_in6);
    freeaddrinfo(res);
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
    coop_log("listening on port %d, sending to %s", g_coop_port, g_coop_peer);
    g_coop_net_on = 1;
    return 1;
}

void coop_net_send(const NativeCoopState *now) {
    CoopPacket p;

    p.magic = COOP_MAGIC;
    p.version = COOP_VERSION;
    p.size = sizeof p;
    p.seq = ++s_seq;
    p.time = (uint32_t)SDL_GetTicks();
    p.ack_time = s_ack_time;
    p.ack_age = s_ack_time != 0 ? (uint32_t)(SDL_GetTicks() - s_ack_at) : 0;
    p.mode = now->mode;
    p.stageFile = now->stageFile;
    p.x = now->x;
    p.y = now->y;
    p.z = now->z;
    p.dir = now->dir;
    p.animSet = now->animSet;
    p.depth = now->depth;
    sendto(s_sock, (const char *)&p, sizeof p, 0, (const struct sockaddr *)&s_peer_addr, s_peer_len);
}

void coop_net_receive(void) {
    CoopPacket p;
    struct sockaddr_storage from;
    int len, flen;

    for (;;) {
        flen = sizeof from;
        len = recvfrom(s_sock, (char *)&p, sizeof p, 0, (struct sockaddr *)&from, &flen);
        if (len == SOCKET_ERROR) {
            /* WSAEWOULDBLOCK: nothing more; WSAECONNRESET: the peer's port closed (an ICMP), go on */
            if (WSAGetLastError() == WSAECONNRESET) {
                continue;
            }
            break;
        }
        if (len < 8 || p.magic != COOP_MAGIC) {
            continue;
        }
        if (!same_addr(&from, flen)) {
            if (!s_said_stranger) {
                char who[64] = "?";
                const struct sockaddr_in6 *a = (const void *)&from;

                s_said_stranger = 1;
                if (from.ss_family == AF_INET6) {
                    inet_ntop(AF_INET6, &a->sin6_addr, who, sizeof who);
                }
                coop_log("a packet from %s port %d, not coop_peer (%s): ignored. Is coop_peer the other player's IP, "
                         "and its port their coop_port?", who, ntohs(a->sin6_port), g_coop_peer);
            }
            continue;
        }
        if (len != sizeof p || p.version != COOP_VERSION || p.size != sizeof p) {
            if (!s_said_version) {
                s_said_version = 1;
                coop_log("the other player's game is another version of the co-op (%d, this is %d): update both",
                         len >= 8 ? p.version : 0, COOP_VERSION);
            }
            continue;
        }
        Uint64 now = SDL_GetTicks();

        /* a restarted peer starts its seq again: take it after a silence */
        if (s_have_seq && (int32_t)(p.seq - s_last_seq) <= 0 && now - s_last_rx < COOP_TIMEOUT_MS) {
            continue; /* late or doubled */
        }
        double off = (double)now - (double)p.time;

        if (!s_have_seq || now - s_last_rx >= COOP_TIMEOUT_MS) {
            s_rx_count = 0;
            s_clock_off = off;
            s_jitter = 0;
            s_got = 0;
            s_first_seq = p.seq;
            s_rtt = -1;
            s_report_at = now + 5000; /* the first ping once it settled */
            s_here = 1;
            coop_log("the other player is here");
        } else if (off < s_clock_off) {
            s_clock_off = off; /* this one came faster */
        } else {
            s_clock_off += (off - s_clock_off) * 0.002; /* the clocks drift apart slowly */
            s_jitter += ((off - s_clock_off) - s_jitter) * 0.05;
        }
        s_got++;
        s_ack_time = p.time;
        s_ack_at = now;
        if (p.ack_time != 0) {
            double rtt = (double)(int32_t)((uint32_t)now - p.ack_time - p.ack_age);

            if (rtt >= 0 && rtt < 10000) {
                s_rtt = s_rtt < 0 ? rtt : s_rtt + (rtt - s_rtt) * 0.1;
            }
        }
        s_have_seq = 1;
        s_last_seq = p.seq;
        s_last_rx = now;
        CoopRx *r = &s_rx[s_rx_head];

        r->time = p.time;
        r->state.mode = p.mode;
        r->state.stageFile = p.stageFile;
        r->state.x = p.x;
        r->state.y = p.y;
        r->state.z = p.z;
        r->state.dir = p.dir;
        r->state.animSet = p.animSet;
        r->state.depth = p.depth;
        s_rx_head = (s_rx_head + 1) % COOP_RX;
        if (s_rx_count < COOP_RX) {
            s_rx_count++;
        }
    }
    Uint64 now = SDL_GetTicks();

    if (s_here && now - s_last_rx >= COOP_TIMEOUT_MS) {
        s_here = 0;
        s_ack_time = 0;
        coop_log("the other player is gone (nothing for %d s: off the field, in a battle, or the game closed)",
                 COOP_TIMEOUT_MS / 1000);
    }
    if (s_here && now >= s_report_at) {
        uint32_t sent = s_last_seq - s_first_seq + 1;

        s_report_at = now + COOP_REPORT_MS;
        coop_log("ping %d ms, jitter %d ms, drawn %d ms behind, %d%% of their packets lost", (int)s_rtt, (int)s_jitter,
                 coop_buffer_ms(), sent > 0 && s_got <= sent ? (int)(100 - 100.0 * s_got / sent) : 0);
    }
}

static int lerp(int a, int b, double t) {
    return a + (int)((b - a) * t);
}

/* The peer at COOP_BUFFER_MS ago on its clock, between the states around it */
int coop_net_peer(NativeCoopState *out) {
    Uint64 now = SDL_GetTicks();
    int i;

    if (s_rx_count == 0 || now - s_last_rx >= COOP_TIMEOUT_MS) {
        return 0;
    }
    double want = (double)now - s_clock_off - coop_buffer_ms(); /* on the peer's clock */

    /* newest to oldest: the first at or before want, and the one after it */
    const CoopRx *after = NULL;

    for (i = 1; i <= s_rx_count; i++) {
        const CoopRx *r = &s_rx[(s_rx_head + COOP_RX - i) % COOP_RX];

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
    *out = s_rx[(s_rx_head + COOP_RX - s_rx_count) % COOP_RX].state; /* all newer than want: the oldest */
    return 1;
}

