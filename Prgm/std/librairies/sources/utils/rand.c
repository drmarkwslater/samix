#include <rand.h>

static long randx=1;

/* ========================================================================= */
void srand(x)  unsigned x; { randx = x; return; }
/* ========================================================================= */
int rand(void) { return(((randx = randx * 1103515245L + 12345)>>16) & 0x7fff); }
