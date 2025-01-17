#ifndef STATS_H
#define STATS_H

#include <buracchi/tftp/client.h>
#include <logger.h>

void stats_init(struct tftp_client_stats stats[static 1]);

#endif //STATS_H
