#include "rwfslock.h"

#include <string.h>

static struct rwfslock_vtbl* get_rwfslock_vtbl();
static int _destroy(struct rwfslock* this);
static int _rdlock(struct rwfslock* this, const char* fname);
static int _wrlock(struct rwfslock* this, const char* fname);
static int _unlock(struct rwfslock* this, const char* fname);

extern int rwfslock_init(struct rwfslock* this) {
	memset(this, 0, sizeof * this);
	this->__ops_vptr = get_rwfslock_vtbl();
	return 0;
}

static struct rwfslock_vtbl* get_rwfslock_vtbl() {
	struct rwfslock_vtbl vtbl_zero = { 0 };
	if (!memcmp(&vtbl_zero, &__rwfslock_ops_vtbl, sizeof * &__rwfslock_ops_vtbl)) {
		__rwfslock_ops_vtbl.destroy = _destroy;
		__rwfslock_ops_vtbl.rdlock = _rdlock;
		__rwfslock_ops_vtbl.wrlock = _wrlock;
		__rwfslock_ops_vtbl.unlock = _unlock;
	}
	return &__rwfslock_ops_vtbl;
}

static int _destroy(struct rwfslock* this) {
	return 0;
}

static int _rdlock(struct rwfslock* this, const char* fname) {
	return 0;
}

static int _wrlock(struct rwfslock* this, const char* fname) {
	return 0;
}

static int _unlock(struct rwfslock* this, const char* fname) {
	return 0;
}
