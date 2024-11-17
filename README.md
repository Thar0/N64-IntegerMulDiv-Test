# N64 32-bit integer multiply and divide test ROM

This repository contains the source code for testing 32-bit signed multiplication and division instructions (`mult` and `div`) on the N64's VR4300 processor. This test ROM compares the result of executing these instructions against equivalent functions that emulate their behavior without using the target instruction. It does 1000000 tests of each instructions with inputs sourced from a 64-bit random number generator with a fixed seed to explore the input space.

These operations have some interesting edge-cases when the input operands not sign-extended 32-bit integers. The VR4300 processor's general purpose registers (GPRs) are 64-bit, but the expectation for these instructions is for them to treat the GPRs as 32-bit. In that case, the result should be that each input register is sign-extended from 32-bit to 64-bit before the calculation is performed. However, this does not happen.

32-bit signed multiplication instead acts as a 64-bit by 35-bit signed multiplication. Equivalent C code:
```c
int emu_mult_test_one(uint64_t *hi, uint64_t *lo, uint64_t a, uint64_t b)
{
    // Signed 32-bit multiplies act like 64-bit x 35-bit
    b = (int64_t)(b << 29) >> 29;
    uint64_t lotemp = (int64_t)a * (int64_t)b;
    *hi = (uint64_t)(int32_t)(lotemp >> 32);
    *lo = (uint64_t)(int32_t)(lotemp >>  0);
    return 0;
}
```

32-bit signed division instead acts as a 32-bit by 64-bit signed division, except when bits 31 and 63 of the divisor disagree. Equivalent C code, lacking the special case:
```c
int emu_div_test_one(uint64_t *hi, uint64_t *lo, uint64_t a, uint64_t b)
{
    // Usually acts like 32-bit x 35-bit division,
    // except when bits 63 and 31 of the divisor are not equal
    if (((b >> 32) & 0x80000000) != ((b >> 0) & 0x80000000)) {
        // TODO this behavior hasn't been figured out yet.
        return -1;
    }
    uint64_t hitemp = (int32_t)a % (int64_t)b;
    uint64_t lotemp = (int32_t)a / (int64_t)b;
    *hi = (uint64_t)(int32_t)hitemp;
    *lo = (uint64_t)(int32_t)lotemp;
    return 0;
}
```

When bits 31 and 63 of the divisor do not agree, the quotient output to the `LO` register is meaningless. Often it consists of repeating bit patterns producing outputs such as `0xAAAAAAAA` or `0x55555555`, but this is not always true. It is at least always true that `HI` and `LO` are related by the expected relation `HI = (int32_t)(a - b * LO)` for dividend `a` and divisor `b`.

A couple of examples:
```
0x00000000_000000012 / 0x00000000_FFFFFFFE produces
LO = 0xFFFFFFFF_AAAAAAAB
HI = 0x00000000_55555568

0x00000000_00000012 / 0x80000000_33333333 produces
LO = 0x00000000_55555555
HI = 0x00000000_11111123
```

Based on the 32-bit sign of the dividend `a` and the 64-bit sign of the divisor `b`, these outputs are heavily preferred but the full behavior is not captured by just these four outputs:
```
b[63] = 0, a[31] = 0, LO = 0xFFFFFFFF_AAAAAAAB
b[63] = 0, a[31] = 1, LO = 0x00000000_55555555
b[63] = 1, a[31] = 0, LO = 0x00000000_55555555
b[63] = 1, a[31] = 1, LO = 0xFFFFFFFF_AAAAAAAB
```

Finally, it may be that `LO(a, b) + LO(a ^ 0x80000000, b) == 0 mod 2^32` holds.

Since this division behavior is not well understood, this test ROM outputs 3 metrics for both the signed multiplication and signed division tests:
- Successes: When the behavior is well understood and the emulation function agrees with the result of the instruction.
- Failures: When the behavior is well understood and the emulation function does not agree with the result of the instruction.
- Failed Emulations: When the behavior is not properly understood so cannot be properly checked.

The expected result from running this test ROM on hardware is:
```
Done
    1000000 successes
    0 failures
    0 failed emulations
Done
    499479 successes
    0 failures
    500521 failed emulations
```
