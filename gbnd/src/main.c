#include <unistd.h>

#include "gbnd.h"
#include "try.h"

extern int main(int argc, char* arv[]) {
	daemon(0, 0);
	return gbnd_start();
}
