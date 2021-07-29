#include "tproto/tproto_gbn.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "try.h"
#include "utilities.h"

static struct tproto_gbn_vtbl* get_tproto_gbn_vtbl();
static void tproto_vtbl_init(struct tproto_vtbl** pvtbl);
static int destroy(struct tproto_gbn* this);
static ssize_t _peek(struct tproto* tproto, const struct socket2* socket2, uint8_t* buff, uint64_t n);
static ssize_t _recv(struct tproto* tproto, const struct socket2* socket2, uint8_t* buff, uint64_t n);
static ssize_t _send(struct tproto* tproto, const struct socket2* socket2, const uint8_t* buff, uint64_t n);

extern int tproto_gbn_init(struct tproto_gbn* this) {
    memset(this, 0, sizeof * this);
    tproto_vtbl_init(&this->super.tproto.__ops_vptr);
    this->__ops_vptr = get_tproto_gbn_vtbl();
    this->super.tproto.type = SOCK_DGRAM;
    return 0;
}

static struct tproto_gbn_vtbl* get_tproto_gbn_vtbl() {
    struct tproto_gbn_vtbl vtbl_zero = { 0 };
    if (!memcmp(&vtbl_zero, &__tproto_gbn_ops_vtbl, sizeof * &__tproto_gbn_ops_vtbl)) {
        __tproto_gbn_ops_vtbl.destroy = destroy;
    }
    return &__tproto_gbn_ops_vtbl;
}

static void tproto_vtbl_init(struct tproto_vtbl** pvtbl) {
    struct tproto_vtbl vtbl_zero = { 0 };
    if (!memcmp(&vtbl_zero, &__tproto_ops_vtbl, sizeof * &__tproto_ops_vtbl)) {
        __tproto_ops_vtbl.peek = _peek;
        __tproto_ops_vtbl.recv = _recv;
        __tproto_ops_vtbl.send = _send;
    }
    *pvtbl = &__tproto_ops_vtbl;
}

static int destroy(struct tproto_gbn* this) {
    return 0;
}

struct gbn_packet {
    uint8_t* paiload;
};

size_t pkt_size = 256;

struct socket_state {
    uint32_t base;
    uint32_t next_seqnum;
    struct gbn_packet** sndpkt;
};

struct timeout_arg {
    int socketfd;
    struct socket_state* state;
};

static ssize_t _peek(struct tproto* tproto, const struct socket2* socket2, uint8_t* buff, uint64_t n) {
    return recv(socket2->fd, buff, n, MSG_PEEK);
}

static ssize_t _recv(struct tproto* tproto, const struct socket2* socket2, uint8_t* buff, uint64_t n) {
    ssize_t bsent = 0;
    struct gbn_packet* rcvpkt = malloc(pkt_size);
    recv(socket2->fd, rcvpkt, pkt_size, 0);
}

static ssize_t _send(struct tproto* tproto, const struct socket2* socket2, const uint8_t* buff, uint64_t n) {
    struct tproto_gbn* this = container_of(tproto, struct tproto_gbn, super.tproto);
    ssize_t bsent = 0;
    uint8_t* data = buff;
    struct socket_state* state = get_scoket_state(this->socket_state_map, socket2->fd);
    if (state->next_seqnum < state->base - this->dw_size) {
        state->sndpkt[state->next_seqnum] = make_pkt(state->next_seqnum, data);
        bsent = send(socket2->fd, state->sndpkt[state->next_seqnum], pkt_size, 0);
        if (state->base == state->next_seqnum) {
            start_timer();
        }
        state->next_seqnum = ++state->next_seqnum % (this->dw_size + 1);
        //
        struct gbn_packet recv_pkt;
        recv(socket2->fd, &recv_pkt, pkt_size, 0);
        state->base = get_ack_number(recv_pkt) + 1;
        if (state->base == state->next_seqnum) {
            stop_timer();
        }
        else {
            start_timer();
        }
    }
    else {
        refuse_data(data);
    }
    return bsent;
}

static struct gbn_packet* make_pkt(int seq_nuber, uint8_t* data) {
    return NULL;
}

static void start_timer() {

}

static void stop_timer() {

}

static void timeout(struct timeout_arg* arg) {
    start_timer();
    for (int i = arg->state->base; i < arg->state->next_seqnum; i++) {
        send(arg->socketfd, arg->state->sndpkt[i], pkt_size, 0);
    }
}

static struct socket_state* get_scoket_state(void* socket_state_map, int fd) {
    struct socket_state* state = map_get(socket_state_map, fd);
    if (!state) {
        state = malloc(sizeof * state);
        state->base = 1;
        state->next_seqnum = 1;
        state->sndpkt = malloc(sizeof * state->sndpkt);
        // add observer to the fd object
        map_add(socket_state_map, fd, state);
    }
    return state;
}
