/* coop_net.c: the other players over UDP (coop_peers in native.ini) */
#define COOP_PEERS 3 /* other players: 4 in all */
extern int g_coop_net_on;
int coop_net_start(void); /* the first call opens the socket: 1 when the network is on */
void coop_net_follow(void); /* native.ini's coop_peers read again (the launcher's room), every 2 s */
int coop_net_peer_count(void); /* the players coop_peers names (that were found) */
void coop_net_send(const NativeCoopState *now); /* to each of them */
void coop_net_receive(void);
int coop_net_peer(int i, NativeCoopState *out); /* 1 with player i as it was a moment ago, 0 when gone */
