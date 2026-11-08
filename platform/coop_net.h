/* coop_net.c: the other player over UDP (coop_peer in native.ini) */
extern int g_coop_net_on;
int coop_net_start(void); /* the first call opens the socket: 1 when the network is on */
void coop_net_send(const NativeCoopState *now);
void coop_net_receive(void);
int coop_net_peer(NativeCoopState *out); /* 1 with the peer as it was a moment ago, 0 when gone */
