#include "gbn_sender.h"

#include "try.h"

extern void on_sender_writeable(evutil_socket_t fd, short events, void* arg) {
    struct sender_state* state = arg;
    if (state->nextseqnum < state->base + WINDOW_SIZE) {
        uint16_t data_length = (state->data_end_ptr - state->data_ptr) % PAYLOAD_LENGTH;
        make_pkt(&(state->sndpkt[state->nextseqnum]), state->nextseqnum, state->data_ptr, data_length, DATA);
        try(send(state->fd, &(state->sndpkt[state->nextseqnum]), sizeof *(state->sndpkt), 0), -1, fail);
        if (state->base == state->nextseqnum) {
            try(evtimer_add(state->timeout_event, &((struct timeval){.tv_sec=0, .tv_usec=TIMEOUT})), -1, fail);
        }
        state->nextseqnum++;
        state->data_ptr += data_length;
        if (state->data_ptr == state->data_end_ptr) {
            // exit somehow
        }
    }
    return;
fail:
    event_base_loopexit(state->event_base, &((struct timeval){.tv_sec=0, .tv_usec=0}));
}

extern void on_sender_readable(evutil_socket_t fd, short events, void* arg) {
    struct sender_state* state = arg;
    struct gbn_packet rcvpkt;
    try(recv(state->fd, &rcvpkt, sizeof rcvpkt, 0), -1, fail);
    if (rcvpkt.type == DATA_ACK && rcvpkt.seqnum + 1 == state->nextseqnum) {
        try(evtimer_del(state->timeout_event), -1, fail);
    }
    else {
        try(evtimer_add(state->timeout_event, &((struct timeval){.tv_sec=0, .tv_usec=TIMEOUT})), -1, fail);
    }
    return;
fail:
    event_base_loopexit(state->event_base, &((struct timeval){.tv_sec=0, .tv_usec=0}));
}

extern void on_sender_timeout(evutil_socket_t fd, short events, void* arg) {
    struct sender_state* state = arg;
    try(evtimer_add(state->timeout_event, &((struct timeval){.tv_sec=0, .tv_usec=TIMEOUT})), -1, fail);
    for (int i = state->base; i < state->nextseqnum; i++) {
        try(send(state->fd, &(state->sndpkt[i]), sizeof *(state->sndpkt), 0), -1, fail);
    }
    return;
fail:
    event_base_loopexit(state->event_base, &((struct timeval){.tv_sec=0, .tv_usec=0}));
}
