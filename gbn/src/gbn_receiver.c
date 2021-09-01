#include "gbn_receiver.h"

#include "try.h"

extern void on_receiver_readable(evutil_socket_t fd, short events, void* arg) {
    struct receiver_state* state = arg;
    struct gbn_packet rcvpkt;
    struct gbn_packet sndpkt;
    try(recv(state->fd, &rcvpkt, sizeof rcvpkt, 0), -1, fail);
    if (rcvpkt.seqnum == state->expectedseqnum) {
        for (int i = 0; i < PAYLOAD_LENGTH; i++) {
            *(state->rslt_ptr) = rcvpkt.data[i];
            state->rslt_ptr++;
            if (state->rslt_ptr == state->rslt_end_ptr) {
                // exit somehow
                break;
            }
        }
        make_pkt(&sndpkt, state->expectedseqnum, &(uint8_t){0}, 0, DATA_ACK);
        try(send(state->fd, &sndpkt, sizeof sndpkt, 0), -1, fail);
        state->expectedseqnum++;
    }
    else {
        make_pkt(&sndpkt, state->expectedseqnum - 1, &(uint8_t){0}, 0, DATA_ACK);
        try(send(state->fd, &sndpkt, sizeof sndpkt, 0), -1, fail);
    }
    return;
fail:
    event_base_loopexit(state->event_base, &((struct timeval){.tv_sec=0, .tv_usec=0}));
}
