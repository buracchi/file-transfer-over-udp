#include "nproto_service.h"
#include "nproto/nproto_service_unix.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/un.h>

#include "socket2.h"
#include "try.h"
#include "utilities.h"

static int set_address(struct cmn_nproto_service* service, struct cmn_socket2* socket, const char* url);
static int parse_url(const char* url, char** address, uint16_t *port);

static struct cmn_nproto_service_vtbl __cmn_nproto_service_ops_vtbl = { .set_address = set_address };

static struct cmn_nproto_service_unix service = {
    .super = {
        .__ops_vptr = &__cmn_nproto_service_ops_vtbl,
        .domain = AF_UNIX
    }
};

#ifdef WIN32 // May the gcc guy who forgot to create a flag for this stupid warning be cursed
    extern struct cmn_nproto_service* cmn_nproto_service_unix = &(service.super);
#elif __unix__
    struct cmn_nproto_service* cmn_nproto_service_unix = &(service.super);
#endif

static int set_address(struct cmn_nproto_service* service, struct cmn_socket2* socket, const char* url) {
    struct sockaddr_un* saddr_un;
    try(saddr_un = malloc(sizeof * saddr_un), NULL, fail);
    memset(saddr_un, 'x', sizeof * saddr_un);
    saddr_un->sun_family = AF_UNIX;
    saddr_un->sun_path[0] = '\0';
    strcpy((char*)&(saddr_un->sun_path) + 1, url);
    socket->address = (struct sockaddr*)saddr_un;
    socket->addrlen = offsetof(struct sockaddr_un, sun_path) + 1 + strlen(url);
    return 0;
fail:
    return 1;
}