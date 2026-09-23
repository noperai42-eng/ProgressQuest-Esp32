#include "tami/sim.h"

void tami_rng_seed(TamiRng *r, uint32_t seed) {
    r->s = seed ? seed : 0xA5A5u;
}

uint32_t tami_rng_u32(TamiRng *r) {
    uint32_t x = r->s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    r->s = x ? x : 0xA5A5u;
    return r->s;
}

uint32_t tami_rng_below(TamiRng *r, uint32_t n) {
    if (n == 0) {
        return 0;
    }
    return tami_rng_u32(r) % n;
}
