# ifndef PACKET_LOSS_H
# define PACKET_LOSS_H

#include <logger.h>

void packet_loss_init(struct logger logger[static 1], double probability);

#endif // PACKET_LOSS_H
