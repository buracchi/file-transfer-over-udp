#include "ftcp.h"

#include <stdlib.h>
#include <string.h>

extern ftcp_pp_t ftcp_pp_init(enum ftcp_type type, enum ftcp_operation operation, uint8_t arg[256], uint64_t dplen) {
    ftcp_pp_t packet;
    packet = malloc(sizeof *packet * ftcp_pp_size());
    if (packet) {
        packet[0] = type;
        packet[1] = operation;
        if (arg) {
            memcpy(packet + 2, arg, 256);
        }
        else {
            memset(packet + 2, 0, 256);
        }
        for (int i = 0; i < 8; i++) {
            packet[258 + i] = (dplen >> 8 * (7 - i)) & 0xFF;
        }
    }
    return packet;
}

extern inline enum ftcp_type ftcp_get_type(ftcp_pp_t ftcp_packet) {
    return ftcp_packet[0];
}

extern inline enum ftcp_operation ftcp_get_operation(ftcp_pp_t ftcp_packet) {
    return ftcp_packet[1];
}

extern enum ftcp_result ftcp_get_result(ftcp_pp_t ftcp_packet) {
    return ftcp_packet[1];
}

extern inline uint8_t *ftcp_get_arg(ftcp_pp_t ftcp_packet) {
    return ftcp_packet + 2;
}

extern uint64_t ftcp_get_dplen(ftcp_pp_t ftcp_packet) {
    uint64_t result = 0;
    for (int i = 0; i < 8; i++) {
        result += (uint64_t) ftcp_packet[258 + i] << 8 * (7 - i);
    }
    return result;
}
