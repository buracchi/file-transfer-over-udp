#pragma once

#include <stdint.h>
#include <string.h>

/**
 * @brief Protocol parameters
 */
#define WINDOW_SIZE 5
#define TIMEOUT 5000
#define PAYLOAD_LENGTH 1024

/**
 * @brief Packet types
 * 
 */
#define SYN         0        /* Opens a connection                          */
#define SYN_ACK     1        /* Acknowledgement of the SYN packet           */
#define DATA        2        /* Data packets                                */
#define DATA_ACK    3        /* Acknowledgement of the DATA packet          */
#define FIN         4        /* Ends a connection                           */
#define FIN_ACK     5        /* Acknowledgement of the FIN packet           */
#define RST         6        /* Reset packet used to reject new connections */

struct gbn_packet {
    uint8_t type;
    uint8_t seqnum;
    uint8_t data[PAYLOAD_LENGTH];
};

static inline void make_pkt(struct gbn_packet* pkt, int seqnum, const uint8_t* data, uint16_t data_length, uint8_t type) {
    memset(pkt, 0, sizeof *pkt);
    pkt->type = type;
    pkt->seqnum = seqnum;
    memcpy(pkt->data, data, data_length);
}
