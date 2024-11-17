#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdint.h>

#include <libdragon.h>

uint64_t rng_u64_next(uint64_t rng) {
    return rng * 2862933555777941757 + 3037000493;
}

/* Compute 32-bit signed multiply on 64-bit operands */
void mult_test_one(uint64_t *hi, uint64_t *lo, uint64_t a, uint64_t b) {
    __asm__ volatile (
        "mult    %2, %3\n"
        "mfhi    %0\n"
        "mflo    %1\n"
        : "=r"(*hi), "=r"(*lo) : "r"(a), "r"(b)
    );
}

/* Compute 32-bit unsigned multiply on 64-bit operands */
void multu_test_one(uint64_t *hi, uint64_t *lo, uint64_t a, uint64_t b) {
    __asm__ volatile (
        "multu   %2, %3\n"
        "mfhi    %0\n"
        "mflo    %1\n"
        : "=r"(*hi), "=r"(*lo) : "r"(a), "r"(b)
    );
}

/* Compute 32-bit signed division on 64-bit operands */
void div_test_one(uint64_t *hi, uint64_t *lo, uint64_t a, uint64_t b) {
    __asm__ volatile (
        "div     $zero, %2, %3\n"
        "mfhi    %0\n"
        "mflo    %1\n"
        : "=r"(*hi), "=r"(*lo) : "r"(a), "r"(b)
    );
}

/* Compute 32-bit unsigned division on 64-bit operands */
void divu_test_one(uint64_t *hi, uint64_t *lo, uint64_t a, uint64_t b) {
    __asm__ volatile (
        "divu    $zero, %2, %3\n"
        "mfhi    %0\n"
        "mflo    %1\n"
        : "=r"(*hi), "=r"(*lo) : "r"(a), "r"(b)
    );
}

/* "Emulate" the 32-bit signed multiply */
int emu_mult_test_one(uint64_t *hi, uint64_t *lo, uint64_t a, uint64_t b)
{
    // Signed 32-bit multiplies act like 64-bit x 35-bit
    b = (int64_t)(b << 29) >> 29;
    uint64_t lotemp = (int64_t)a * (int64_t)b;
    *hi = (uint64_t)(int32_t)(lotemp >> 32);
    *lo = (uint64_t)(int32_t)(lotemp >>  0);
    return 0;
}

/* "Emulate" the 32-bit signed division */
int emu_div_test_one(uint64_t *hi, uint64_t *lo, uint64_t a, uint64_t b)
{
    // Usually acts like 32-bit x 35-bit division,
    // except when bits 63 and 31 of the divisor are not equal
    if (((b >> 32) & 0x80000000) != ((b >> 0) & 0x80000000)) {
        // TODO this behavior hasn't been figured out yet.
        return -1;
    }
    a = (int32_t)a;
    // b = (int64_t)(b << 29) >> 29;
    uint64_t hitemp = (int64_t)a % (int64_t)b;
    uint64_t lotemp = (int64_t)a / (int64_t)b;
    *hi = (uint64_t)(int32_t)hitemp;
    *lo = (uint64_t)(int32_t)lotemp;
    return 0;
}

int main(void)
{
    /* Initialize peripherals */
    console_init();
    console_set_debug(true);
    debug_init_usblog();
    debug_init_isviewer();
    console_set_render_mode(RENDER_MANUAL);

    int n_success, n_fails, n_failed_emulations;
    uint64_t r1;
    uint64_t r2;

    r1 = 65521;
    r2 = 2;
    n_success = 0;
    n_fails = 0;
    n_failed_emulations = 0;

#define NUM_RUNS 1000000

    for (unsigned i = 0; i < NUM_RUNS; i++) {
        uint64_t hi1, lo1;
        mult_test_one(&hi1, &lo1, r1, r2);

        uint64_t hi2, lo2;
        if (emu_mult_test_one(&hi2, &lo2, r1, r2) == 0) {
            // emulation success, compare

            if (hi1 != hi2 || lo1 != lo2) {
                n_fails++;
                // mismatch
                // printf("mult mismatch: inputs=[%016llX %016llX] outputs=[%016llX %016llX] expected=[%016llX %016llX]\n", r1, r2, hi1, lo1, hi2, lo2);
                // console_render();
            } else {
                n_success++;
            }
        } else {
            // emulation failed
            n_failed_emulations++;
        }

        r1 = rng_u64_next(r1);
        r2 = rng_u64_next(r2);
    }

    printf("Done\n"
           "    %d successes\n"
           "    %d failures\n"
           "    %d failed emulations\n",
           n_success, n_fails, n_failed_emulations);
    console_render();

    r1 = 65521;
    r2 = 2;
    n_success = 0;
    n_fails = 0;
    n_failed_emulations = 0;

    for (unsigned i = 0; i < NUM_RUNS; i++) {
        uint64_t hi1, lo1;
        div_test_one(&hi1, &lo1, r1, r2);

        uint64_t hi2, lo2;
        if (emu_div_test_one(&hi2, &lo2, r1, r2) == 0) {
            // emulation success, compare

            if (hi1 != hi2 || lo1 != lo2) {
                n_fails++;
                // mismatch
                // printf("div mismatch: inputs=[%016llX %016llX] outputs=[%016llX %016llX] expected=[%016llX %016llX]\n", r1, r2, hi1, lo1, hi2, lo2);
                // console_render();
            } else {
                n_success++;
            }
        } else {
            // emulation failed
            n_failed_emulations++;
        }

        r1 = rng_u64_next(r1);
        r2 = rng_u64_next(r2);
    }

    printf("Done\n"
           "    %d successes\n"
           "    %d failures\n"
           "    %d failed emulations\n",
           n_success, n_fails, n_failed_emulations);
    console_render();
}
