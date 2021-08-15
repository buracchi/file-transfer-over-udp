#include "rwfslock.h"

#include <string.h>

#include "map.h"

static struct rwfslock_vtbl* get_rwfslock_vtbl();

extern int rwfslock_init(struct rwfslock* this) {
	memset(this, 0, sizeof * this);
	return cmn_linked_list_map_init(&(this->map));
}

extern int rwfslock_destroy(struct rwfslock* this) {
	return 0;
}

extern int rwfslock_rdlock(struct rwfslock* this, const char* fname) {
	return 0;
}

extern int rwfslock_wrlock(struct rwfslock* this, const char* fname) {
	return 0;
}

extern int rwfslock_unlock(struct rwfslock* this, const char* fname) {
	return 0;
}
