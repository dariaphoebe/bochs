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

// Pentium CPU uses only 68-bit precision M_PI approximation
// #define BETTER_THAN_PENTIUM

/*============================================================================
 * Written for Bochs (x86 achitecture simulator) by
 *            Stanislav Shwartsman (gate at fidonet.org.il)
 * ==========================================================================*/ 

#define FLOAT128

#include "softfloatx80.h"
#include "softfloat-round-pack.h"

/* 128-bit PI/2 fraction */
#ifdef BETTER_THAN_PENTIUM
static const Bit64u Hi = BX_CONST64(0xC90FDAA22168C234);
static const Bit64u Lo = BX_CONST64(0xC4C6628B80DC1CD1);
#else
static const Bit64u Hi = BX_CONST64(0xC90FDAA22168C234);
static const Bit64u Lo = BX_CONST64(0xC000000000000000);
#endif

/* reduce trigonometric function argument using 128-bit precision 
   M_PI approximation */
static Bit64u argument_reduction_kernel(Bit64u aSig0, int Exp, Bit64u *zSig0, Bit64u *zSig1)
{
    Bit64u term0, term1, term2;
    Bit64u aSig1 = 0;

    shortShift128Left(aSig1, aSig0, Exp, &aSig1, &aSig0);
    Bit64u q = estimateDiv128To64(aSig1, aSig0, Hi);
    mul128By64To192(Hi, Lo, q, &term0, &term1, &term2);
    sub128(aSig1, aSig0, term0, term1, zSig1, zSig0);
    while ((Bit64s)(*zSig1) < 0) {
        --q;
        add192(*zSig1, *zSig0, term2, 0, Hi, Lo, zSig1, zSig0, &term2);
    }
    *zSig1 = term2;
    return q;
}

static int reduce_trig_arg(int expDiff, int &zSign, Bit64u &aSig0, Bit64u &aSig1)
{
    Bit64u term0, term1, q = 0;

    if (expDiff < 0) {
        shift128Right(aSig0, 0, 1, &aSig0, &aSig1);
        expDiff = 0;
    }
    if (expDiff > 0) {
        q = argument_reduction_kernel(aSig0, expDiff, &aSig0, &aSig1);
    }
    else {
        if (Hi <= aSig0) {
            aSig0 -= Hi;
            q = 1;
        }
    }

    shift128Right(Hi, Lo, 1, &term0, &term1);
    if (! lt128(aSig0, aSig1, term0, term1))
    {
        int lt = lt128(term0, term1, aSig0, aSig1);
        int eq = eq128(aSig0, aSig1, term0, term1);
              
        if ((eq && (q & 1)) || lt) {
            zSign = !zSign;
            ++q;
        }
        if (lt) sub128(Hi, Lo, aSig0, aSig1, &aSig0, &aSig1);
    }

    return (int)(q & 3);
}

static const floatx80 floatx80_one = packFloatx80(0, 0x3fff, BX_CONST64(0x8000000000000000));

#define SIN_ARR_SIZE 9
#define COS_ARR_SIZE 9

static float128 sin_arr[SIN_ARR_SIZE] =
{
    packFloat128(BX_CONST64(0x3fff000000000000), BX_CONST64(0x0000000000000000)), /*  1 */
    packFloat128(BX_CONST64(0xbffc555555555555), BX_CONST64(0x5555555555555555)), /*  3 */
    packFloat128(BX_CONST64(0x3ff8111111111111), BX_CONST64(0x1111111111111111)), /*  5 */
    packFloat128(BX_CONST64(0xbff2a01a01a01a01), BX_CONST64(0xa01a01a01a01a01a)), /*  7 */
    packFloat128(BX_CONST64(0x3fec71de3a556c73), BX_CONST64(0x38faac1c88e50017)), /*  9 */
    packFloat128(BX_CONST64(0xbfe5ae64567f544e), BX_CONST64(0x38fe747e4b837dc7)), /* 11 */
    packFloat128(BX_CONST64(0x3fde6124613a86d0), BX_CONST64(0x97ca38331d23af68)), /* 13 */
    packFloat128(BX_CONST64(0xbfd6ae7f3e733b81), BX_CONST64(0xf11d8656b0ee8cb0)), /* 15 */
    packFloat128(BX_CONST64(0x3fce952c77030ad4), BX_CONST64(0xa6b2605197771b00))  /* 17 */
};

static float128 cos_arr[COS_ARR_SIZE] =
{
    packFloat128(BX_CONST64(0x3fff000000000000), BX_CONST64(0x0000000000000000)), /*  0 */
    packFloat128(BX_CONST64(0xbffe000000000000), BX_CONST64(0x0000000000000000)), /*  2 */
    packFloat128(BX_CONST64(0x3ffa555555555555), BX_CONST64(0x5555555555555555)), /*  4 */
    packFloat128(BX_CONST64(0xbff56c16c16c16c1), BX_CONST64(0x6c16c16c16c16c17)), /*  6 */
    packFloat128(BX_CONST64(0x3fefa01a01a01a01), BX_CONST64(0xa01a01a01a01a01a)), /*  8 */
    packFloat128(BX_CONST64(0xbfe927e4fb7789f5), BX_CONST64(0xc72ef016d3ea6679)), /* 10 */
    packFloat128(BX_CONST64(0x3fe21eed8eff8d89), BX_CONST64(0x7b544da987acfe85)), /* 12 */
    packFloat128(BX_CONST64(0xbfda93974a8c07c9), BX_CONST64(0xd20badf145dfa3e5)), /* 14 */
    packFloat128(BX_CONST64(0x3fd2ae7f3e733b81), BX_CONST64(0xf11d8656b0ee8cb0))  /* 16 */
};

static float128 poly_sincos(float128 x1, float128 *carr, float_status_t &status)
{
    float128 x2 = float128_mul(x1, x1, status);
    float128 x4 = float128_mul(x2, x2, status);
    float128 r1, r2;

    // negative = x2*(a_1 + x4*(a_3 + x4*(a_5+x4*a_7)));
    r1 = float128_mul(x4, carr[7], status);
    r1 = float128_add(r1, carr[5], status);
    r1 = float128_mul(r1, x4, status);
    r1 = float128_add(r1, carr[3], status);
    r1 = float128_mul(r1, x4, status);
    r1 = float128_add(r1, carr[1], status);
    r1 = float128_mul(r1, x2, status);

    // positive = x4*(a_2 + x4*(a_4 + x4*(a_6+x4*a_8)));
    r2 = float128_mul(x4, carr[8], status);
    r2 = float128_add(r2, carr[6], status);
    r2 = float128_mul(r2, x4, status);
    r2 = float128_add(r2, carr[4], status);
    r2 = float128_mul(r2, x4, status);
    r2 = float128_add(r2, carr[2], status);
    r2 = float128_mul(r2, x4, status);

    r1 = float128_add(r1, r2, status);
    return 
       float128_add(r1, carr[0], status);
}

/* 0 <= x <= pi/4 */
BX_CPP_INLINE float128 poly_sin(float128 x, float_status_t &status)
{
    float128 t = poly_sincos(x, sin_arr, status);
    t = float128_mul(t, x, status);
    return t;
}

/* 0 <= x <= pi/4 */
BX_CPP_INLINE float128 poly_cos(float128 x, float_status_t &status)
{
    return poly_sincos(x, cos_arr, status);
}

BX_CPP_INLINE void sincos_invalid(floatx80 *sin_a, floatx80 *cos_a, floatx80 a)
{
    if (sin_a) *sin_a = a;
    if (cos_a) *cos_a = a;
}

BX_CPP_INLINE void sincos_tiny_argument(floatx80 *sin_a, floatx80 *cos_a, floatx80 a)
{
    if (sin_a) *sin_a = a;
    if (cos_a) *cos_a = floatx80_one;
}

static floatx80 sincos_approximation(int neg, float128 r, Bit64u quotient, float_status_t &status)
{
    if (quotient & 0x1) {
        r = poly_cos(r, status);
        neg = 0;
    } else  {
        r = poly_sin(r, status);
    }

    floatx80 result = float128_to_floatx80(r, status);
    if (quotient & 0x2) 
        neg = ! neg;

    if (neg) 
        floatx80_chs(result);

    return result;
}

int fsincos(floatx80 a, floatx80 *sin_a, floatx80 *cos_a, float_status_t &status)
{
    Bit64u aSig0, aSig1 = 0;
    Bit32s aExp, zExp, expDiff;
    int aSign, zSign;
    int q = 0;

    // handle unsupported extended double-precision floating encodings
    if (floatx80_is_unsupported(a)) 
    {
        goto invalid;
    }

    aSig0 = extractFloatx80Frac(a);
    aExp = extractFloatx80Exp(a);
    aSign = extractFloatx80Sign(a);
     
    /* invalid argument */
    if (aExp == 0x7FFF) {
        if ((Bit64u) (aSig0<<1)) {
            sincos_invalid(sin_a, cos_a, propagateFloatx80NaN(a, status));
            return 0;
        }

    invalid:
        float_raise(status, float_flag_invalid);
        sincos_invalid(sin_a, cos_a, floatx80_default_nan);
        return 0;
    }

    if (aExp == 0) {
        if (aSig0 == 0) {
            sincos_tiny_argument(sin_a, cos_a, a);
            return 0;
        }

        float_raise(status, float_flag_denormal);

        /* handle pseudo denormals */
        if (! (aSig0 & BX_CONST64(0x8000000000000000)))
        {
            float_raise(status, float_flag_inexact);
            if (sin_a)
                float_raise(status, float_flag_underflow);
            sincos_tiny_argument(sin_a, cos_a, a);
            return 0;
        }

        normalizeFloatx80Subnormal(aSig0, &aExp, &aSig0);
    }
    
    zSign = aSign;
    zExp = 0x3FFF;
    expDiff = aExp - zExp;

    /* argument is out-of-range */
    if (expDiff >= 63) 
        return -1;

    float_raise(status, float_flag_inexact);

    if (expDiff < -1) {    // doesn't require reduction
        if (expDiff <= -68) {
            a = packFloatx80(aSign, aExp, aSig0);
            sincos_tiny_argument(sin_a, cos_a, a);
            return 0;
        }
        zExp = aExp;
    }
    else {
        q = reduce_trig_arg(expDiff, zSign, aSig0, aSig1);
    }

    /* **************************** */
    /* argument reduction completed */
    /* **************************** */

    /* using float128 for approximation */
    float128 r = normalizeRoundAndPackFloat128(0, zExp-0x10, aSig0, aSig1, status);

    if (aSign) q = -q;
    if (sin_a) *sin_a = sincos_approximation(zSign, r,   q, status);
    if (cos_a) *cos_a = sincos_approximation(zSign, r, q+1, status);

    return 0;
}

int fsin(floatx80 &a, float_status_t &status)
{
    return fsincos(a, &a, 0, status);
}

int fcos(floatx80 &a, float_status_t &status)
{
    return fsincos(a, 0, &a, status);
}

int ftan(floatx80 &a, float_status_t &status)
{
    Bit64u aSig0, aSig1 = 0;
    Bit32s aExp, zExp, expDiff;
    int aSign, zSign; 
    int q = 0;

    // handle unsupported extended double-precision floating encodings
    if (floatx80_is_unsupported(a)) 
    {
        goto invalid;
    }

    aSig0 = extractFloatx80Frac(a);
    aExp = extractFloatx80Exp(a);
    aSign = extractFloatx80Sign(a);
     
    /* invalid argument */
    if (aExp == 0x7FFF) {
        if ((Bit64u) (aSig0<<1))
        {
            a = propagateFloatx80NaN(a, status);
            return 0;
        }

    invalid:
        float_raise(status, float_flag_invalid);
        a = floatx80_default_nan;
        return 0;
    }

    if (aExp == 0) {
        if (aSig0 == 0) return 0;
        float_raise(status, float_flag_denormal);
        /* handle pseudo denormals */
        if (! (aSig0 & BX_CONST64(0x8000000000000000)))
        {
            float_raise(status, float_flag_inexact | float_flag_underflow);
            return 0;
        }
        normalizeFloatx80Subnormal(aSig0, &aExp, &aSig0);
    }
    
    zSign = aSign;
    zExp = 0x3FFF;
    expDiff = aExp - zExp;

    /* argument is out-of-range */
    if (expDiff >= 63) 
        return -1;

    float_raise(status, float_flag_inexact);

    if (expDiff < -1) {    // doesn't require reduction
        if (expDiff <= -68) {
            a = packFloatx80(aSign, aExp, aSig0);
            return 0;
        }
        zExp = aExp;
    }
    else {
        q = reduce_trig_arg(expDiff, zSign, aSig0, aSig1);
    }

    /* **************************** */
    /* argument reduction completed */
    /* **************************** */

    /* using float128 for approximation */
    float128 r = normalizeRoundAndPackFloat128(0, zExp-0x10, aSig0, aSig1, status);

    float128 sin_r = poly_sin(r, status);
    float128 cos_r = poly_cos(r, status);

    if (q & 0x1) {
        r = float128_div(cos_r, sin_r, status);
        zSign = ! zSign;
    } else {
        r = float128_div(sin_r, cos_r, status);
    }

    a = float128_to_floatx80(r, status);
    if (zSign)
        floatx80_chs(a);

    return 0;
}
