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

#define FPATAN_ARR_SIZE 9

static const float128 float128_one = 
	packFloat128(BX_CONST64(0x3fff000000000000), BX_CONST64(0x0000000000000000));
static const float128 float128_sqrt3 =
	packFloat128(BX_CONST64(0x3fffbb67ae8584ca), BX_CONST64(0xa73b25742d7078b8));
static const floatx80 floatx80_pi2 = 
	packFloatx80(0, 0x3fff, BX_CONST64(0xc90fdaa22168c235));
static const floatx80 floatx80_pi6 =
	packFloatx80(0, 0x3ffe, BX_CONST64(0x860a91c16b9b2c23));

static float128 atan_arr[FPATAN_ARR_SIZE] =
{
    packFloat128(BX_CONST64(0x3fff000000000000), BX_CONST64(0x0000000000000000)), /*  1 */
    packFloat128(BX_CONST64(0xbffd555555555555), BX_CONST64(0x5555555555555555)), /*  3 */
    packFloat128(BX_CONST64(0x3ffc999999999999), BX_CONST64(0x999999999999999a)), /*  5 */
    packFloat128(BX_CONST64(0xbffc249249249249), BX_CONST64(0x2492492492492492)), /*  7 */
    packFloat128(BX_CONST64(0x3ffbc71c71c71c71), BX_CONST64(0xc71c71c71c71c71c)), /*  9 */
    packFloat128(BX_CONST64(0xbffb745d1745d174), BX_CONST64(0x5d1745d1745d1746)), /* 11 */
    packFloat128(BX_CONST64(0x3ffb3b13b13b13b1), BX_CONST64(0x3b13b13b13b13b14)), /* 13 */
    packFloat128(BX_CONST64(0xbffb111111111111), BX_CONST64(0x1111111111111111)), /* 15 */
    packFloat128(BX_CONST64(0x3ffae1e1e1e1e1e1), BX_CONST64(0xe1e1e1e1e1e1e1e2))  /* 17 */
};

static float128 poly_atan(float128 x1, float_status_t &status)
{
    float128 x2 = float128_mul(x1, x1, status);
    float128 x4 = float128_mul(x2, x2, status);
    float128 r1, r2;

    // r1 = x2*(a_1 + x4*(a_3 + x4*(a_5+x4*a_7)));
    r1 = float128_mul(x4, atan_arr[7], status);
    r1 = float128_add(r1, atan_arr[5], status);
    r1 = float128_mul(r1, x4, status);
    r1 = float128_add(r1, atan_arr[3], status);
    r1 = float128_mul(r1, x4, status);
    r1 = float128_add(r1, atan_arr[1], status);
    r1 = float128_mul(r1, x2, status);

    // r2 = x4*(a_2 + x4*(a_4 + x4*(a_6+x4*a_8)));
    r2 = float128_mul(x4, atan_arr[8], status);
    r2 = float128_add(r2, atan_arr[6], status);
    r2 = float128_mul(r2, x4, status);
    r2 = float128_add(r2, atan_arr[4], status);
    r2 = float128_mul(r2, x4, status);
    r2 = float128_add(r2, atan_arr[2], status);
    r2 = float128_mul(r2, x4, status);

    r1 = float128_add(r1, r2, status);
    r1 = float128_add(r1, atan_arr[0], status);

    return 
        float128_mul(r1, x1, status);
}

floatx80 fpatan(floatx80 a, floatx80 b, float_status_t &status)
{
    // handle unsupported extended double-precision floating encodings
    if (floatx80_is_unsupported(a)) {
        float_raise(status, float_flag_invalid);
        return floatx80_default_nan;
    }

    Bit64u aSig = extractFloatx80Frac(a);
    Bit32s aExp = extractFloatx80Exp(a);
    int aSign = extractFloatx80Sign(a);
    Bit64u bSig = extractFloatx80Frac(b);
    Bit32s bExp = extractFloatx80Exp(b);
    int bSign = extractFloatx80Sign(b);

    int zSign = aSign ^ bSign;
     
    if (bExp == 0x7FFF)
    {
        if ((Bit64u) (bSig<<1))
            return propagateFloatx80NaN(a, b, status);

        if (aExp == 0x7FFF) {
            if ((Bit64u) (aSig<<1))
                return propagateFloatx80NaN(a, b, status);

            if (aSign) {   /* return 3PI/4 */
                return packFloatx80(bSign, 0x3FFF, BX_CONST64(0x990e91a896cbe3f9));
            }
            else {         /* return  PI/4 */
                return packFloatx80(bSign, 0x3FFE, BX_CONST64(0xc90fdaa22168c235));
            }
        }

        if (aSig && (aExp == 0)) 
            float_raise(status, float_flag_denormal);

        /* return PI/2 */
        return packFloatx80(bSign, 0x3FFF, BX_CONST64(0xc90fdaa22168c235));
    }
    if (aExp == 0x7FFF)
    {
        if ((Bit64u) (aSig<<1)) 
            return propagateFloatx80NaN(a, b, status);

return_PI_or_ZERO:
        if (aSig && (aExp == 0)) 
            float_raise(status, float_flag_denormal);

        if (aSign) {   /* return PI */
            return packFloatx80(bSign, 0x4000, BX_CONST64(0xc90fdaa22168c235));
        } else {       /* return  0 */
            return packFloatx80(bSign, 0, 0);
        }
    }
    if (bExp == 0)
    {
        if (bSig == 0) 
             goto return_PI_or_ZERO;

        float_raise(status, float_flag_denormal);
        normalizeFloatx80Subnormal(bSig, &bExp, &bSig);
    }
    if (aExp == 0)
    {
        if (aSig == 0)   /* return PI/2 */
            return packFloatx80(bSign, 0x3FFF, BX_CONST64(0xc90fdaa22168c235));

        float_raise(status, float_flag_denormal);
        normalizeFloatx80Subnormal(aSig, &aExp, &aSig);
    }

    float_raise(status, float_flag_inexact);

    /* |a| = |b| */
    if (aSig == bSig && aExp == bExp)
        return packFloatx80(zSign, 0x3FFE, BX_CONST64(0xc90fdaa22168c235));
         
    /* ******************************** */
    /* using float128 for approximation */
    /* ******************************** */

    float128 a128 = normalizeRoundAndPackFloat128(0, aExp-0x10, aSig, 0, status);
    float128 b128 = normalizeRoundAndPackFloat128(0, bExp-0x10, bSig, 0, status);
    float128 x;
    int swap = 0, add_pi6 = 0;

    if (aExp > bExp || (aExp == bExp && aSig > bSig))
    {
        x = float128_div(b128, a128, status);
    }
    else {
	x = float128_div(a128, b128, status);
	swap = 1;
    }

    Bit32s xExp = extractFloat128Exp(x);
    floatx80 result = packFloatx80(0, 0, 0);

    if (xExp <= 0x3FBB) goto approximation_completed;

    /* argument correction */
    if (xExp >= 0x3FFD) {
        float128 t1 = float128_mul(x, float128_sqrt3, status);
        float128 t2 = float128_add(x, float128_sqrt3, status);
        x = float128_sub(t1, float128_one, status);
        x = float128_div(t1, t2, status);
    	add_pi6 = 1;
    }

    x = poly_atan(x, status);
    result = float128_to_floatx80(x, status);
    if (add_pi6) result = floatx80_add(result, floatx80_pi6, status);

approximation_completed:
    if (swap) result = floatx80_sub(floatx80_pi2, result, status);
    if (zSign) floatx80_chs(result);
    return result;
}
