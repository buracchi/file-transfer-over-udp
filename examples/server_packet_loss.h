# ifndef PACKET_LOSS_H
# define PACKET_LOSS_H

#include <logger.h>

void packet_loss_init(struct logger logger[static 1], double probability, int n, bool disable_fixed_seed);

#endif // PACKET_LOSS_H
