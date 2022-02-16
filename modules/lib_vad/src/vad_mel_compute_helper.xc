#include <stdint.h>

void mul_mel(uint32_t &h, uint32_t &l,
                 uint32_t scale) {
    uint32_t hi, li;
    asm("lmul %0, %1, %2, %3, %4, %5" : "=r" (hi), "=r" (li) : "r" (scale), "r" (l), "r" (0), "r" (0));
    asm("lmul %0, %1, %2, %3, %4, %5" : "=r" (h), "=r" (l) : "r" (scale), "r" (h), "r" (hi), "r" (0));
}

void add_unsigned_hl(uint32_t &sumH, uint32_t &sumL,
                                uint32_t h, uint32_t l) {
    uint32_t cout, cou2;
    asm("ladd %0, %1, %2, %3, %4" : "=r" (cout), "=r" (sumL) : "r" (sumL), "r" (l), "r" (0));
    asm("ladd %0, %1, %2, %3, %4" : "=r" (cou2), "=r" (sumH) : "r" (sumH), "r" (h), "r" (cout));
}