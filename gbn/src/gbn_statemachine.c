#include "gbn_statemachine.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <buracchi/common/networking/socket2.h>
#include <buracchi/common/utilities/try.h>
#include <event2/event.h>
#include <statemachine.h>

#include "gbn_packet.h"

#define TRANSACTIONS_NUMBER (sizeof gbn_transitions / sizeof * gbn_transitions)
#define udp_send(statemachine, pkt) sendto(\
                                        cmn_socket2_get_fd(statemachine->socket), \
                                        pkt, \
                                        sizeof * pkt, \
                                        0, \
                                        statemachine->peer_addr, \
                                        statemachine->peer_addrlen)

static void on_socket_writeable(evutil_socket_t fd, short events, void *arg);

static void on_socket_readable(evutil_socket_t fd, short events, void *arg);

static void on_timeout(evutil_socket_t fd, short events, void *arg);

bool received_syn(cmn_statemachine_context_t context);

bool received_syn_ack(cmn_statemachine_context_t context);

bool received_data_ack(cmn_statemachine_context_t context);

bool received_fin(cmn_statemachine_context_t context);

bool received_fin_ack(cmn_statemachine_context_t context);

void send_syn(cmn_statemachine_context_t context);

void send_syn_ack(cmn_statemachine_context_t context);

void send_data_ack(cmn_statemachine_context_t context);

void send_fin_ack_and_fin(cmn_statemachine_context_t context);

void send_fin(cmn_statemachine_context_t context);

void send_fin_ack(cmn_statemachine_context_t context);

struct gbn_statemachine {
    struct cmn_statemachine super;
    cmn_socket2_t socket;
    struct sockaddr *peer_addr;
    socklen_t peer_addrlen;
    struct gbn_packet sndpkt[WINDOW_SIZE];
    uint32_t sndbase;
    uint32_t sndseqnum;
    uint32_t rcvseqnum;
    struct event_base *event_base;
    struct event *writeable_event;
    struct event *readable_event;
    struct event *timeout_event;
};

struct cmn_statemachine_state state_closed = {NULL, NULL};
struct cmn_statemachine_state state_listen = {NULL, NULL};
struct cmn_statemachine_state state_syn_rcvd = {NULL, NULL};
struct cmn_statemachine_state state_syn_sent = {NULL, NULL};
struct cmn_statemachine_state state_established = {NULL, NULL};
struct cmn_statemachine_state state_fin_wait_1 = {NULL, NULL};
struct cmn_statemachine_state state_fin_wait_2 = {NULL, NULL};
struct cmn_statemachine_state state_time_wait = {NULL, NULL};
struct cmn_statemachine_state state_last_ack = {NULL, NULL};
// add state for too much attempt

struct cmn_statemachine_transition gbn_transitions[] = {
        {&state_closed,      GBN_EV_CONNECT, NULL,                      &send_syn,             &state_syn_sent},
        {&state_syn_sent,    GBN_EV_TIMEOUT, NULL,                      &send_syn,             &state_syn_sent},
        {&state_syn_sent,    GBN_EV_SOCKET_READABLE, &received_syn_ack, &send_data_ack,        &state_established},
        {&state_closed,      GBN_EV_ACCEPT,  NULL,                       NULL,                 &state_listen},
        {&state_listen,      GBN_EV_SOCKET_READABLE, &received_syn,     &send_syn_ack,         &state_closed},
        {&state_syn_rcvd,    GBN_EV_TIMEOUT, NULL,                      &send_syn_ack,         &state_syn_rcvd},
        {&state_syn_rcvd,    GBN_EV_SOCKET_READABLE, &received_data_ack, NULL,                 &state_established},
        {&state_established, GBN_EV_RECV,    NULL,                       NULL,                 &state_established},
        {&state_established, GBN_EV_SEND,    NULL,                       NULL,                 &state_established},
        {&state_established, GBN_EV_TIMEOUT, NULL,                       NULL,                 &state_established},
        {&state_established, GBN_EV_SOCKET_READABLE, &received_fin,     &send_fin_ack_and_fin, &state_last_ack},
        {&state_last_ack,    GBN_EV_TIMEOUT, NULL,                      &send_fin_ack_and_fin, &state_last_ack},
        {&state_last_ack,    GBN_EV_SOCKET_READABLE, &received_fin_ack,  NULL,                 &state_closed},
        {&state_established, GBN_EV_CLOSE,   NULL,                      &send_fin,             &state_fin_wait_1},
        {&state_fin_wait_1,  GBN_EV_TIMEOUT, NULL,                      &send_fin,             &state_fin_wait_1},
        {&state_fin_wait_1,  GBN_EV_SOCKET_READABLE, &received_fin_ack,  NULL,                 &state_fin_wait_2},
        {&state_fin_wait_2,  GBN_EV_SOCKET_READABLE, &received_fin,     &send_fin_ack,         &state_time_wait},
        {&state_time_wait,   GBN_EV_TIMEOUT, NULL,                      &send_fin_ack,         &state_time_wait},
        {&state_time_wait,   GBN_EV_SOCKET_READABLE, &received_fin_ack,  NULL,                 &state_closed},
};

extern gbn_statemachine_t gbn_statemachine_init(cmn_socket2_t socket) {
    gbn_statemachine_t statemachine;
    try(statemachine = (gbn_statemachine_t) cmn_statemachine_init(&state_closed, gbn_transitions, TRANSACTIONS_NUMBER),
        NULL, FAIL);
    try(statemachine = realloc(statemachine, sizeof *statemachine), NULL, FAIL2);
    statemachine->socket = socket;
    try(statemachine->event_base = event_base_new(), NULL, FAIL2);
    try(statemachine->writeable_event = event_new(statemachine->event_base, cmn_socket2_get_fd(socket),
                                                  EV_WRITE | EV_PERSIST, on_socket_writeable, statemachine), NULL,
        FAIL3);
    try(statemachine->readable_event = event_new(statemachine->event_base, cmn_socket2_get_fd(socket),
                                                 EV_READ | EV_PERSIST, on_socket_readable, statemachine), NULL, FAIL3);
    try(statemachine->timeout_event = evtimer_new(statemachine->event_base, on_timeout, statemachine), NULL, FAIL3);
    try(event_add(statemachine->writeable_event, NULL), -1, FAIL3);
    try(event_add(statemachine->readable_event, NULL), -1, FAIL3);
    return statemachine;
FAIL3:
    event_free(statemachine->writeable_event);
    event_free(statemachine->readable_event);
    event_free(statemachine->timeout_event);
    event_base_free(statemachine->event_base);
FAIL2:
    cmn_statemachine_destroy((cmn_statemachine_t) statemachine);
FAIL:
    return NULL;
}

extern void gbn_statemachine_set_peer_addr(gbn_statemachine_t statemachine, struct sockaddr *addr, socklen_t addr_len) {
    statemachine->peer_addr = addr;
    statemachine->peer_addrlen = addr_len;
}

static void on_socket_writeable(evutil_socket_t fd, short events, void *arg) {
    cmn_statemachine_t statemachine = (cmn_statemachine_t) arg;
    cmn_statemachine_send_event(statemachine, GBN_EV_SOCKET_WRITABLE);
}

static void on_socket_readable(evutil_socket_t fd, short events, void *arg) {
    cmn_statemachine_t statemachine = (cmn_statemachine_t) arg;
    cmn_statemachine_send_event(statemachine, GBN_EV_SOCKET_READABLE);
}

static void on_timeout(evutil_socket_t fd, short events, void *arg) {
    cmn_statemachine_t statemachine = (cmn_statemachine_t) arg;
    cmn_statemachine_send_event(statemachine, GBN_EV_TIMEOUT);
}

bool inline received_packet_type(cmn_statemachine_context_t context, uint8_t type) {
    gbn_statemachine_t statemachine = (gbn_statemachine_t) context->statemachine;
    struct gbn_packet rcvpkt = {0};
    int fd = cmn_socket2_get_fd(statemachine->socket);
    recvfrom(fd, &rcvpkt, GBN_HEADER_LENGTH, MSG_PEEK, NULL, NULL);
    return rcvpkt.type == type;
}

bool received_syn(cmn_statemachine_context_t context) { return received_packet_type(context, SYN); }

bool received_syn_ack(cmn_statemachine_context_t context) { return received_packet_type(context, SYN_ACK); }

bool received_data_ack(cmn_statemachine_context_t context) { return received_packet_type(context, DATA_ACK); }

bool received_fin(cmn_statemachine_context_t context) { return received_packet_type(context, FIN); }

bool received_fin_ack(cmn_statemachine_context_t context) { return received_packet_type(context, FIN_ACK); }

void send_syn(cmn_statemachine_context_t context) {
    gbn_statemachine_t statemachine = (gbn_statemachine_t) context->statemachine;
    struct gbn_packet *sndpkt;
    statemachine->sndbase = (uint32_t) random();
    statemachine->sndseqnum = statemachine->sndbase;
    sndpkt = &(statemachine->sndpkt[statemachine->sndseqnum % WINDOW_SIZE]);
    make_pkt(sndpkt, SYN, statemachine->sndseqnum, &(uint8_t) {0}, 0);
    udp_send(statemachine, sndpkt);
}

void send_syn_ack(cmn_statemachine_context_t context) {
    struct gbn_packet rcvpkt;
    struct sockaddr *src_addr;  // TODO: malloc the correct data structure and set the length using the network protocol
    socklen_t addrlen;
    recvfrom(statemachine->fd, &rcvpkt, sizeof rcvpkt, 0, src_addr, &addrlen);
    if (rcvpkt.type == SYN) {
        struct gbn_packet sndpkt;
        uint32_t server_isn;
        uint32_t client_isn;
        // TODO: save the accepted_socket and the accepted_statemachine somewere
        gbn_statemachine_t accepted_statemachine;
        cmn_socket2_t accepted_socket;
        accepted_statemachine = gbn_statemachine_init(accepted_socket);
        accepted_statemachine->peer_addr = src_addr;
        accepted_statemachine->peer_addrlen = addrlen;
        // bind accepted_statemachine ?
        // choose a free port?
        // add event to fd of accepted_socket?
        server_isn = (uint32_t) random();
        client_isn = rcvpkt.seqnum;
        make_pkt(&sndpkt, SYN_ACK, (client_isn + 1) ^ server_isn, &(uint8_t) {0}, 0);
        sendto(accepted_statemachine->fd, &sndpkt, sizeof sndpkt, 0, accepted_statemachine->peer_addr,
               accepted_statemachine->peer_addrlen);
        // calculate expectedseqnum
        accepted_statemachine->state = SYN_RCVD;
    }
}

void send_data_ack(cmn_statemachine_context_t context) {

}

extern void closed_on_accept(gbn_statemachine_t statemachine) {
    // bind to socket_addr?
    statemachine->state = LISTEN;
}

extern void syn_rcvd_on_recv(gbn_statemachine_t statemachine) {
    struct gbn_packet rcvpkt;
    uint32_t server_isn;
    recvfrom(statemachine->fd, &rcvpkt, sizeof rcvpkt, 0, NULL, NULL);
    if (rcvpkt.type == SYN_ACK && rcvpkt.seqnum == statemachine->receiver_info.expectedseqnum) {
        struct gbn_packet sndpkt;
        make_pkt(&sndpkt, DATA_ACK, server_isn + 1, &(uint8_t) {0}, 0);
        sendto(statemachine->fd, &sndpkt, sizeof sndpkt, 0, statemachine->peer_addr, statemachine->peer_addrlen);
        statemachine->state = ESTABILISHED;
    }
}

extern void syn_rcvd_on_timeout(gbn_statemachine_t statemachine) {
    //resend SYNACK
}

extern void syn_sent_on_recv(gbn_statemachine_t statemachine) {
    //rcvpkt = udp_recv(&sndr_addr);
    //if (getType(rcvpkt) == SYNACK && rcvpkt.seqnum XOR (client_isn + 1) XOR (client_isn + 1) == (client_isn + 1) {
    // allocate resources
    //server_isn = getSeqnum(rcvpkt) XOR (client_isn + 1);
    //pkt = make_pkt(ACK, server_isn + 1, NULL);
    //udp_send(pkt, sndr_addr);
    statemachine->state = ESTABILISHED;
}

extern void estabilished_on_close_op(gbn_statemachine_t statemachine) {
    // send fin
    statemachine->state = FIN_WAIT_1;
}

extern void estabilished_on_socket_readable(gbn_statemachine_t statemachine) {
    struct gbn_packet rcvpkt;
    recv(statemachine->fd, &rcvpkt, sizeof rcvpkt, 0);
    if (statemachine->state == FIN) {
        // send finack
        statemachine->state = LAST_ACK;
    }
}

extern void estabilished_on_recv_op(gbn_statemachine_t statemachine) {
    struct gbn_packet rcvpkt;
    try(recv(statemachine->fd, &rcvpkt, sizeof rcvpkt, 0), -1, fail);
    switch (rcvpkt.type) {
        case DATA: {
            struct gbn_packet sndpkt;
            if (rcvpkt.type == DATA && rcvpkt.seqnum == statemachine->receiver_info.expectedseqnum) {
                for (int i = 0; i < PAYLOAD_LENGTH; i++) {
                    *(statemachine->receiver_info.rslt_ptr) = rcvpkt.data[i];
                    statemachine->receiver_info.rslt_ptr++;
                    if (statemachine->receiver_info.rslt_ptr == statemachine->receiver_info.rslt_end_ptr) {
                        // exit somehow
                        break;
                    }
                }
                make_pkt(&sndpkt, DATA_ACK, statemachine->receiver_info.expectedseqnum, &(uint8_t) {0}, 0);
                try(send(statemachine->fd, &sndpkt, sizeof sndpkt, 0), -1, fail);
                statemachine->receiver_info.expectedseqnum++;
            }
            else {
                make_pkt(&sndpkt, DATA_ACK, statemachine->receiver_info.expectedseqnum - 1, &(uint8_t) {0}, 0);
                try(send(statemachine->fd, &sndpkt, sizeof sndpkt, 0), -1, fail);
            }
            if (statemachine->receiver_info.rslt_ptr == statemachine->receiver_info.rslt_end_ptr) {
                goto fail;
            }
        }
            break;
        case DATA_ACK: {
            if (rcvpkt.seqnum + 1 == statemachine->sender_info.nextseqnum) {
                try(evtimer_del(statemachine->timeout_event), -1, fail);
                statemachine->sender_info.base++;
            }
            else {
                try(evtimer_add(statemachine->timeout_event, &((struct timeval) {.tv_sec=0, .tv_usec=TIMEOUT})), -1,
                    fail);
            }
        }
            break;
        default:
            break;
    }
fail:
    return;
}

extern void estabilished_on_send_op(gbn_statemachine_t statemachine) {
    if (statemachine->sender_info.nextseqnum < statemachine->sender_info.base + WINDOW_SIZE) {
        uint16_t data_length =
                (statemachine->sender_info.data_end_ptr - statemachine->sender_info.data_ptr) % PAYLOAD_LENGTH;
        make_pkt(&(statemachine->sender_info.sndpkt[statemachine->sender_info.nextseqnum]), DATA,
                 statemachine->sender_info.nextseqnum, statemachine->sender_info.data_ptr, data_length);
        try(send(statemachine->fd, &(statemachine->sender_info.sndpkt[statemachine->sender_info.nextseqnum]),
                 sizeof *(statemachine->sender_info.sndpkt), 0), -1, fail);
        if (statemachine->sender_info.base == statemachine->sender_info.nextseqnum) {
            try(evtimer_add(statemachine->timeout_event, &((struct timeval) {.tv_sec=0, .tv_usec=TIMEOUT})), -1, fail);
        }
        statemachine->sender_info.nextseqnum++;
        statemachine->sender_info.data_ptr += data_length;
        if (statemachine->sender_info.data_ptr == statemachine->sender_info.data_end_ptr) {
            // exit somehow
        }
    }
    if (statemachine->sender_info.data_ptr == statemachine->sender_info.data_end_ptr) {
        goto fail;
    }
    return;
fail:
    event_base_loopexit(statemachine->event_base, &((struct timeval) {.tv_sec=0, .tv_usec=0}));
}

extern void estabilished_on_timeout(gbn_statemachine_t statemachine) {
    try(evtimer_add(statemachine->timeout_event, &((struct timeval) {.tv_sec=0, .tv_usec=TIMEOUT})), -1, fail);
    for (int i = statemachine->sender_info.base; i < statemachine->sender_info.nextseqnum; i++) {
        try(send(statemachine->fd, &(statemachine->sender_info.sndpkt[i]), sizeof *(statemachine->sender_info.sndpkt),
                 0), -1, fail);
    }
    return;
fail:
    event_base_loopexit(statemachine->event_base, &((struct timeval) {.tv_sec=0, .tv_usec=0}));
}

extern void fin_wait_1_on_recv(gbn_statemachine_t statemachine) {
    // receive finack
}

extern void fin_wait_2_on_recv(gbn_statemachine_t statemachine) {
    // receive fin
    // send finack
}

extern void time_wait_on_recv(gbn_statemachine_t statemachine) {
    // close
}

extern void time_wait_on_timeout(gbn_statemachine_t statemachine) {
    // close
}

extern void last_ack_on_recv(gbn_statemachine_t statemachine) {
    // receive finack
    // close
}
