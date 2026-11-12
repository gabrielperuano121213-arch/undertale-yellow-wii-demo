#include "utils.h"

static unsigned rng_s = 0x2545F491u;

int iabs(int v) { return v < 0 ? -v : v; }

int rnd(int n) 
{ 
    if (n <= 0) return 0;
    rng_s = rng_s * 1664525u + 1013904223u; 
    return (int)((rng_s >> 12) % (unsigned)n); 
}

void rng_seed(unsigned s) { rng_s = s; }
unsigned rng_get(void) { return rng_s; }
