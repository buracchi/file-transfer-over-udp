#pragma once

#include <stdint.h>
#include <string.h>

/**
 * @brief Protocol parameters
 */
#define WINDOW_SIZE 5
#define TIMEOUT 5000
#define PAYLOAD_LENGTH 1024
#define GBN_HEADER_LENGTH ((size_t)(&(((struct gbn_packet*)0)->data)))

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
/* Using UDP we can't handle demultiplexing, so there is no way to send RST packet to reject new connections */

/**
 * @brief GBN over UDP packet
 * 
 * +------------------+------------------+
 * |   SOURCE PORT    | DESTINATION PORT |
 * +------------------+------------------+
 * |      LENGTH      |     CHECKSUM     |
 * +------------------+------------------+
 * |+-----------------------------------+|
 * ||          SEQUENCE NUMBER          ||
 * |+-----------------+-------+---------+|
 * || RECEIVE WINDOW  | TYPE  | UNUSED  ||
 * |+-----------------+-------+---------+|
 * ||               DATA                ||
 * |+-----------------------------------+|
 * +-------------------------------------+
 */
struct gbn_packet {
    uint32_t seqnum;
    uint16_t rcv_wndw;
    uint8_t type;
    uint8_t unused;
    uint8_t data[PAYLOAD_LENGTH];
};

static inline void make_pkt(struct gbn_packet *pkt, uint8_t type, uint32_t seqnum, const uint8_t *data,
                            uint16_t data_length) {
    memset(pkt, 0, sizeof *pkt);
    pkt->seqnum = seqnum;
    pkt->rcv_wndw = WINDOW_SIZE;
    pkt->type = type;
    memcpy(pkt->data, data, data_length);
}
