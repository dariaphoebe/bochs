/*============================================================================
This source file is an extension to the SoftFloat IEC/IEEE Floating-point
Arithmetic Package, Release 2b, written for Bochs (x86 achitecture simulator)
floating point emulation.

THIS SOFTWARE IS DISTRIBUTED AS IS, FOR FREE.  Although reasonable effort has
been made to avoid it, THIS SOFTWARE MAY CONTAIN FAULTS THAT WILL AT TIMES
RESULT IN INCORRECT BEHAVIOR.  USE OF THIS SOFTWARE IS RESTRICTED TO PERSONS
AND ORGANIZATIONS WHO CAN AND WILL TAKE FULL RESPONSIBILITY FOR ALL LOSSES,
COSTS, OR OTHER PROBLEMS THEY INCUR DUE TO THE SOFTWARE, AND WHO FURTHERMORE
EFFECTIVELY INDEMNIFY JOHN HAUSER AND THE INTERNATIONAL COMPUTER SCIENCE
INSTITUTE (possibly via similar legal warning) AGAINST ALL LOSSES, COSTS, OR
OTHER PROBLEMS INCURRED BY THEIR CUSTOMERS AND CLIENTS DUE TO THE SOFTWARE.

Derivative works are acceptable, even for commercial purposes, so long as
(1) the source code for the derivative work includes prominent notice that
the work is derivative, and (2) the source code includes prominent notice with
these four paragraphs for those parts of this code that are retained.
=============================================================================*/

/*============================================================================
 * Written for Bochs (x86 achitecture simulator) by
 *            Stanislav Shwartsman (gate at fidonet.org.il)
 * ==========================================================================*/ 

#define FLOAT128

#include "softfloatx80.h"
#include "softfloat-round-pack.h"

static const floatx80 floatx80_one =
    packFloatx80(0, 0x3fff, BX_CONST64(0x8000000000000000));

static const float128 float128_one =
    packFloat128(BX_CONST64(0x3fff000000000000), BX_CONST64(0x0000000000000000));
static const float128 float128_two =
    packFloat128(BX_CONST64(0x4000000000000000), BX_CONST64(0x0000000000000000));
static const float128 float128_ln2 = 
    packFloat128(BX_CONST64(0x3ffe62e42fefa39e), BX_CONST64(0xf35793c7673007e6));

static const float128 float128_ln2inv  = 
    packFloat128(BX_CONST64(0x3FFF71547652b82f), BX_CONST64(0xe1777d0ffda0d23a));
static const float128 float128_ln2inv2 = 
    packFloat128(BX_CONST64(0x400071547652b82f), BX_CONST64(0xe1777d0ffda0d23a));

#define SQRT2_HALF_SIG 	BX_CONST64(0xb504f333f9de6484)

#define L2_ARR_SIZE 9

static float128 ln_arr[L2_ARR_SIZE] =
{
    packFloat128(BX_CONST64(0x3fff000000000000), BX_CONST64(0x0000000000000000)), /*  1 */
    packFloat128(BX_CONST64(0x3ffd555555555555), BX_CONST64(0x5555555555555555)), /*  3 */
    packFloat128(BX_CONST64(0x3ffc999999999999), BX_CONST64(0x999999999999999a)), /*  5 */
    packFloat128(BX_CONST64(0x3ffc249249249249), BX_CONST64(0x2492492492492492)), /*  7 */
    packFloat128(BX_CONST64(0x3ffbc71c71c71c71), BX_CONST64(0xc71c71c71c71c71c)), /*  9 */
    packFloat128(BX_CONST64(0x3ffb745d1745d174), BX_CONST64(0x5d1745d1745d1746)), /* 11 */
    packFloat128(BX_CONST64(0x3ffb3b13b13b13b1), BX_CONST64(0x3b13b13b13b13b14)), /* 13 */
    packFloat128(BX_CONST64(0x3ffb111111111111), BX_CONST64(0x1111111111111111)), /* 15 */
    packFloat128(BX_CONST64(0x3ffae1e1e1e1e1e1), BX_CONST64(0xe1e1e1e1e1e1e1e2))  /* 17 */
};

/* required sqrt(2)/2 < x < sqrt(2) */
static float128 poly_ln(float128 x1, float_status_t &status)
{
    float128 x2 = float128_mul(x1, x1, status);
    float128 x4 = float128_mul(x2, x2, status);
    float128 r1, r2;

    // r1 = x2*(a_1 + x4*(a_3 + x4*(a_5+x4*a_7)));
    r1 = float128_mul(x4, ln_arr[7], status);
    r1 = float128_add(r1, ln_arr[5], status);
    r1 = float128_mul(r1, x4, status);
    r1 = float128_add(r1, ln_arr[3], status);
    r1 = float128_mul(r1, x4, status);
    r1 = float128_add(r1, ln_arr[1], status);
    r1 = float128_mul(r1, x2, status);

    // r2 = x4*(a_2 + x4*(a_4 + x4*(a_6+x4*a_8)));
    r2 = float128_mul(x4, ln_arr[8], status);
    r2 = float128_add(r2, ln_arr[6], status);
    r2 = float128_mul(r2, x4, status);
    r2 = float128_add(r2, ln_arr[4], status);
    r2 = float128_mul(r2, x4, status);
    r2 = float128_add(r2, ln_arr[2], status);
    r2 = float128_mul(r2, x4, status);

    r1 = float128_add(r1, r2, status);
    r1 = float128_add(r1, ln_arr[0], status);

    return 
        float128_mul(r1, x1, status);
}

static float128 poly_l2(float128 x, float_status_t &status)
{
    /* using float128 for approximation */
    float128 x_p1 = float128_add(x, float128_one, status);
    float128 x_m1 = float128_sub(x, float128_one, status);
    x = float128_div(x_m1, x_p1, status);
    x = poly_ln(x, status);
    x = float128_mul(x, float128_ln2inv2, status);
    return x;
}

static float128 poly_l2p1(float128 x, float_status_t &status)
{
    /* using float128 for approximation */
    float128 x_p2 = float128_add(x, float128_two, status);
    x = float128_div(x, x_p2, status);
    x = poly_ln(x, status);
    x = float128_mul(x, float128_ln2inv2, status);
    return x;
}

floatx80 fyl2x(floatx80 a, floatx80 b, float_status_t &status)
{
    // handle unsupported extended double-precision floating encodings
    if (floatx80_is_unsupported(a)) {
invalid:
        float_raise(status, float_flag_invalid);
        return floatx80_default_nan;
    }

    Bit64u aSig = extractFloatx80Frac(a);
    Bit32s aExp = extractFloatx80Exp(a);
    int aSign = extractFloatx80Sign(a);
    Bit64u bSig = extractFloatx80Frac(b);
    Bit32s bExp = extractFloatx80Exp(b);
    int bSign = extractFloatx80Sign(b);

    int zSign = bSign ^ 1;
     
    if (aExp == 0x7FFF) {
        if ((Bit64u) (aSig<<1)
             || ((bExp == 0x7FFF) && (Bit64u) (bSig<<1))) 
        {
            return propagateFloatx80NaN(a, b, status);
        }
        if (aSign) goto invalid;
        else {
            if (bExp == 0) {
                if (bSig == 0) goto invalid;
                float_raise(status, float_flag_denormal);
            }
            return packFloatx80(bSign, 0x7FFF, BX_CONST64(0x8000000000000000));
        }
    }
    if (bExp == 0x7FFF)
    {
        if ((Bit64u) (bSig<<1)) return propagateFloatx80NaN(a, b, status);
        if (aSign && (Bit64u)(aExp | aSig)) goto invalid;
        if (aSig && (aExp == 0)) 
            float_raise(status, float_flag_denormal);
        if (aExp < 0x3FFF) {
            return packFloatx80(zSign, 0x7FFF, BX_CONST64(0x8000000000000000));
        }
        if (aExp == 0x3FFF && ((Bit64u) (aSig<<1) == 0)) goto invalid;
        return packFloatx80(bSign, 0x7FFF, BX_CONST64(0x8000000000000000));
    }
    if (aExp == 0) {
        if (aSig == 0) {
            if ((bExp | bSig) == 0) goto invalid;
            float_raise(status, float_flag_divbyzero);
            return packFloatx80(zSign, 0x7FFF, BX_CONST64(0x8000000000000000));
        }
        if (aSign) goto invalid;
        float_raise(status, float_flag_denormal);
        normalizeFloatx80Subnormal(aSig, &aExp, &aSig);
    }
    if (aSign) goto invalid;
    if (bExp == 0) {
        if (bSig == 0) {
            if (aExp < 0x3FFF) return packFloatx80(zSign, 0, 0);
            return packFloatx80(bSign, 0, 0);
        }
        float_raise(status, float_flag_denormal);
        normalizeFloatx80Subnormal(bSig, &bExp, &bSig);
    }
    if (aExp == 0x3FFF && ((Bit64u) (aSig<<1) == 0)) 
        return packFloatx80(bSign, 0, 0);

    float_raise(status, float_flag_inexact);

    int ExpDiff = aExp - 0x3FFF;
    aExp = 0;
    if (aSig >= SQRT2_HALF_SIG) {
        ExpDiff++;
        aExp--;
    }

    /* ******************************** */
    /* using float128 for approximation */
    /* ******************************** */

    float128 x = normalizeRoundAndPackFloat128(0, aExp+0x3FEF, aSig, 0, status);
    x = poly_l2(x, status);
    floatx80 r = float128_to_floatx80(x, status);
    r = floatx80_add(r, int32_to_floatx80(ExpDiff), status);
    return floatx80_mul(r, b, status);
}

floatx80 fyl2xp1(floatx80 a, floatx80 b, float_status_t &status)
{
    Bit64u aSig, bSig;
    Bit32s aExp, bExp;
    int aSign, bSign;

    // handle unsupported extended double-precision floating encodings
    if (floatx80_is_unsupported(a)) {
invalid:
        float_raise(status, float_flag_invalid);
        return floatx80_default_nan;
    }

    aSig = extractFloatx80Frac(a);
    aExp = extractFloatx80Exp(a);
    aSign = extractFloatx80Sign(a);
    bSig = extractFloatx80Frac(b);
    bExp = extractFloatx80Exp(b);
    bSign = extractFloatx80Sign(b);
    int zSign = aSign ^ bSign;
     
    if (aExp == 0x7FFF) {
        if ((Bit64u) (aSig<<1)
             || ((bExp == 0x7FFF) && (Bit64u) (bSig<<1))) 
        {
            return propagateFloatx80NaN(a, b, status);
        }
        if (aSign) goto invalid;
        else {
            if (bExp == 0) {
                if (bSig == 0) goto invalid;
                float_raise(status, float_flag_denormal);
            }
            return packFloatx80(bSign, 0x7FFF, BX_CONST64(0x8000000000000000));
        }
    }
    if (bExp == 0x7FFF)
    {
        if ((Bit64u) (bSig<<1)) 
            return propagateFloatx80NaN(a, b, status);

        if (aExp == 0) {
            if (aSig == 0) goto invalid;
            float_raise(status, float_flag_denormal);
        }

        return packFloatx80(zSign, 0x7FFF, BX_CONST64(0x8000000000000000));
    }
    if (aExp == 0) {
        if (aSig == 0) {
            if (bSig && (bExp == 0)) float_raise(status, float_flag_denormal);
            return packFloatx80(zSign, 0, 0);
        }
        float_raise(status, float_flag_denormal);
        normalizeFloatx80Subnormal(aSig, &aExp, &aSig);
    }
    if (bExp == 0) {
        if (bSig == 0) return packFloatx80(zSign, 0, 0);
        float_raise(status, float_flag_denormal);
        normalizeFloatx80Subnormal(bSig, &bExp, &bSig);
    }

    float_raise(status, float_flag_inexact);

    if (aSign && aExp >= 0x3FFF) 
        return a;

    if (aExp >= 0x3FFC) // big argument
    {
        return fyl2x(floatx80_add(a, floatx80_one, status), b, status);
    }

    /* ******************************** */
    /* using float128 for approximation */
    /* ******************************** */

    float128 x = normalizeRoundAndPackFloat128(aSign, aExp-0x10, aSig, 0, status);
    if (aExp < 0x3FB9)  // handle tiny argument
        x = float128_mul(x, float128_ln2inv, status);
    else {               // regular polynomial approximation
        if (aExp >= 0x3FFC)
        {
            x = float128_add(x, float128_one, status);
            x = poly_l2(x, status);
        }
        else
        {
            x = poly_l2p1(x, status);
        }
    }
    floatx80 r = float128_to_floatx80(x, status);
    return floatx80_mul(r, b, status);
}
