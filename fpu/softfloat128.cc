/*============================================================================
This C source file is part of the SoftFloat IEC/IEEE Floating-point Arithmetic
Package, Release 2b.

Written by John R. Hauser.  This work was made possible in part by the
International Computer Science Institute, located at Suite 600, 1947 Center
Street, Berkeley, California 94704.  Funding was partially provided by the
National Science Foundation under grant MIP-9311980.  The original version
of this code was written as part of a project to build a fixed-point vector
processor in collaboration with the University of California at Berkeley,
overseen by Profs. Nelson Morgan and John Wawrzynek.  More information
is available through the Web page `http://www.cs.berkeley.edu/~jhauser/
arithmetic/SoftFloat.html'.

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
 * Adapted for Bochs (x86 achitecture simulator) by
 *            Stanislav Shwartsman (gate at fidonet.org.il)
 * ==========================================================================*/ 

#define FLOAT128

#include "softfloat.h"
#include "softfloat-macros.h"
#include "softfloat-round-pack.h"
#include "softfloat-specialize.h"

#ifdef FLOAT128

/*----------------------------------------------------------------------------
| The pattern for a default generated quadruple-precision NaN. The `high' and
| `low' values hold the most- and least-significant bits, respectively.
*----------------------------------------------------------------------------*/
#define float128_default_nan_hi BX_CONST64(0xFFFF800000000000)
#define float128_default_nan_lo BX_CONST64(0x0000000000000000)

#ifdef BX_BIG_ENDIAN
static const float128 float128_default_nan = { float128_default_nan_hi, float128_default_nan_lo };
#else
static const float128 float128_default_nan = { float128_default_nan_lo, float128_default_nan_hi };
#endif

/*----------------------------------------------------------------------------
| Takes two quadruple-precision floating-point values `a' and `b', one of
| which is a NaN, and returns the appropriate NaN result.  If either `a' or
| `b' is a signaling NaN, the invalid exception is raised.
*----------------------------------------------------------------------------*/

static float128 propagateFloat128NaN(float128 a, float128 b, float_status_t &status)
{
    int aIsNaN, aIsSignalingNaN, bIsNaN, bIsSignalingNaN;
    aIsNaN = float128_is_nan(a);
    aIsSignalingNaN = float128_is_signaling_nan(a);
    bIsNaN = float128_is_nan(b);
    bIsSignalingNaN = float128_is_signaling_nan(b);
    a.hi |= BX_CONST64( 0x0000800000000000 );
    b.hi |= BX_CONST64( 0x0000800000000000 );
    if (aIsSignalingNaN | bIsSignalingNaN) float_raise(status, float_flag_invalid);
    if (aIsSignalingNaN) {
        if (bIsSignalingNaN) goto returnLargerSignificand;
        return bIsNaN ? b : a;
    }
    else if (aIsNaN) {
        if (bIsSignalingNaN | !bIsNaN) return a;
 returnLargerSignificand:
        if (lt128(a.hi<<1, a.lo, b.hi<<1, b.lo)) return b;
        if (lt128(b.hi<<1, b.lo, a.hi<<1, a.lo)) return a;
        return (a.hi < b.hi) ? a : b;
    }
    else {
        return b;
    }
}

/*----------------------------------------------------------------------------
| Returns the result of converting the quadruple-precision floating-point NaN
| `a' to the canonical NaN format.  If `a' is a signaling NaN, the invalid
| exception is raised.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE commonNaNT float128ToCommonNaN(float128 a, float_status_t &status)
{
    commonNaNT z;
    if (float128_is_signaling_nan(a)) float_raise(status, float_flag_invalid);
    z.sign = a.hi>>63;
    shortShift128Left(a.hi, a.lo, 16, &z.hi, &z.lo);
    return z;
}

/*----------------------------------------------------------------------------
| Returns the result of converting the canonical NaN `a' to the quadruple-
| precision floating-point format.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE float128 commonNaNToFloat128(commonNaNT a)
{
    float128 z;
    shift128Right(a.hi, a.lo, 16, &z.hi, &z.lo);
    z.hi |= (((Bit64u) a.sign)<<63) | BX_CONST64(0x7FFF800000000000);
    return z;
}

/*----------------------------------------------------------------------------
| Normalizes the subnormal quadruple-precision floating-point value
| represented by the denormalized significand formed by the concatenation of
| `aSig0' and `aSig1'.  The normalized exponent is stored at the location
| pointed to by `zExpPtr'.  The most significant 49 bits of the normalized
| significand are stored at the location pointed to by `zSig0Ptr', and the
| least significant 64 bits of the normalized significand are stored at the
| location pointed to by `zSig1Ptr'.
*----------------------------------------------------------------------------*/

static void
 normalizeFloat128Subnormal(
     Bit64u aSig0,
     Bit64u aSig1,
     Bit32s *zExpPtr,
     Bit64u *zSig0Ptr,
     Bit64u *zSig1Ptr
)
{
    int shiftCount;

    if (aSig0 == 0) {
        shiftCount = countLeadingZeros64(aSig1) - 15;
        if (shiftCount < 0) {
            *zSig0Ptr = aSig1 >>(-shiftCount);
            *zSig1Ptr = aSig1 << (shiftCount & 63);
        }
        else {
            *zSig0Ptr = aSig1 << shiftCount;
            *zSig1Ptr = 0;
        }
        *zExpPtr = - shiftCount - 63;
    }
    else {
        shiftCount = countLeadingZeros64(aSig0) - 15;
        shortShift128Left(aSig0, aSig1, shiftCount, zSig0Ptr, zSig1Ptr);
        *zExpPtr = 1 - shiftCount;
    }
}

/*----------------------------------------------------------------------------
| Takes an abstract floating-point value having sign `zSign', exponent `zExp',
| and extended significand formed by the concatenation of `zSig0', `zSig1',
| and `zSig2', and returns the proper quadruple-precision floating-point value
| corresponding to the abstract input.  Ordinarily, the abstract value is
| simply rounded and packed into the quadruple-precision format, with the
| inexact exception raised if the abstract input cannot be represented
| exactly.  However, if the abstract value is too large, the overflow and
| inexact exceptions are raised and an infinity or maximal finite value is
| returned.  If the abstract value is too small, the input value is rounded to
| a subnormal number, and the underflow and inexact exceptions are raised if
| the abstract input cannot be represented exactly as a subnormal quadruple-
| precision floating-point number.
|     The input significand must be normalized or smaller.  If the input
| significand is not normalized, `zExp' must be 0; in that case, the result
| returned is a subnormal number, and it must not require rounding.  In the
| usual case that the input significand is normalized, `zExp' must be 1 less
| than the ``true'' floating-point exponent.  The handling of underflow and
| overflow follows the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static float128
 roundAndPackFloat128(
     int zSign, Bit32s zExp, Bit64u zSig0, Bit64u zSig1, Bit64u zSig2, float_status_t &status)
{
    int increment = ((Bit64s) zSig2 < 0);
    if (0x7FFD <= (Bit32u) zExp) {
        if ((0x7FFD < zExp)
             || ((zExp == 0x7FFD)
                  && eq128(BX_CONST64(0x0001FFFFFFFFFFFF),
                         BX_CONST64(0xFFFFFFFFFFFFFFFF), zSig0, zSig1)
                  && increment)) 
        {
            float_raise(status, float_flag_overflow | float_flag_inexact);
            return packFloat128(zSign, 0x7FFF, 0, 0);
        }
        if (zExp < 0) {
            int isTiny = (zExp < -1)
                || ! increment
                || lt128(zSig0, zSig1,
                       BX_CONST64(0x0001FFFFFFFFFFFF),
                       BX_CONST64(0xFFFFFFFFFFFFFFFF)
                  );
            shift128ExtraRightJamming(
                zSig0, zSig1, zSig2, -zExp, &zSig0, &zSig1, &zSig2);
            zExp = 0;
            if (isTiny && zSig2) float_raise(status, float_flag_underflow);
            increment = ((Bit64s) zSig2 < 0);
        }
    }
    if (zSig2) float_raise(status, float_flag_inexact);
    if (increment) {
        add128(zSig0, zSig1, 0, 1, &zSig0, &zSig1);
        zSig1 &= ~((zSig2 + zSig2 == 0) & 1);
    }
    else {
        if ((zSig0 | zSig1) == 0) zExp = 0;
    }
    return packFloat128(zSign, zExp, zSig0, zSig1);
}

/*----------------------------------------------------------------------------
| Takes an abstract floating-point value having sign `zSign', exponent `zExp',
| and significand formed by the concatenation of `zSig0' and `zSig1', and
| returns the proper quadruple-precision floating-point value corresponding
| to the abstract input.  This routine is just like `roundAndPackFloat128'
| except that the input significand has fewer bits and does not have to be
| normalized.  In all cases, `zExp' must be 1 less than the ``true'' floating-
| point exponent.
*----------------------------------------------------------------------------*/

static float128
 normalizeRoundAndPackFloat128(
     int zSign, Bit32s zExp, Bit64u zSig0, Bit64u zSig1, float_status_t &status)
{
    Bit64u zSig2;

    if (zSig0 == 0) {
        zSig0 = zSig1;
        zSig1 = 0;
        zExp -= 64;
    }
    int shiftCount = countLeadingZeros64(zSig0) - 15;
    if (0 <= shiftCount) {
        zSig2 = 0;
        shortShift128Left(zSig0, zSig1, shiftCount, &zSig0, &zSig1);
    }
    else {
        shift128ExtraRightJamming(
            zSig0, zSig1, 0, -shiftCount, &zSig0, &zSig1, &zSig2);
    }
    zExp -= shiftCount;
    return roundAndPackFloat128(zSign, zExp, zSig0, zSig1, zSig2, status);
}

/*----------------------------------------------------------------------------
| Returns the result of converting the extended double-precision floating-
| point value `a' to the quadruple-precision floating-point format.  The
| conversion is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

float128 floatx80_to_float128(floatx80 a, float_status_t &status)
{
    Bit64u zSig0, zSig1;

    Bit64u aSig = extractFloatx80Frac(a);
    Bit32s aExp = extractFloatx80Exp(a);
    int   aSign = extractFloatx80Sign(a);

    if ((aExp == 0x7FFF) && (Bit64u) (aSig<<1))
        return commonNaNToFloat128(floatx80ToCommonNaN(a, status));

    shift128Right(aSig<<1, 0, 16, &zSig0, &zSig1);
    return packFloat128(aSign, aExp, zSig0, zSig1);
}

/*----------------------------------------------------------------------------
| Returns the result of converting the quadruple-precision floating-point
| value `a' to the extended double-precision floating-point format.  The
| conversion is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

floatx80 float128_to_floatx80(float128 a, float_status_t &status)
{
    Bit32s aExp;
    Bit64u aSig0, aSig1;

    aSig1 = extractFloat128Frac1(a);
    aSig0 = extractFloat128Frac0(a);
    aExp = extractFloat128Exp(a);
    int aSign = extractFloat128Sign(a);

    if (aExp == 0x7FFF) {
        if (aSig0 | aSig1) 
            return commonNaNToFloatx80(float128ToCommonNaN(a, status));

        return packFloatx80(aSign, 0x7FFF, BX_CONST64(0x8000000000000000));
    }

    if (aExp == 0) {
        if ((aSig0 | aSig1) == 0) return packFloatx80(aSign, 0, 0);
        normalizeFloat128Subnormal(aSig0, aSig1, &aExp, &aSig0, &aSig1);
    }
    else aSig0 |= BX_CONST64(0x0001000000000000);

    shortShift128Left(aSig0, aSig1, 15, &aSig0, &aSig1);
    return roundAndPackFloatx80(80, aSign, aExp, aSig0, aSig1, status);
}

/*----------------------------------------------------------------------------
| Returns the result of adding the absolute values of the quadruple-precision
| floating-point values `a' and `b'. If `zSign' is 1, the sum is negated
| before being returned. `zSign' is ignored if the result is a NaN.
| The addition is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static float128 addFloat128Sigs(float128 a, float128 b, int zSign, float_status_t &status)
{
    Bit32s aExp, bExp, zExp;
    Bit64u aSig0, aSig1, bSig0, bSig1, zSig0, zSig1, zSig2;
    Bit32s expDiff;

    aSig1 = extractFloat128Frac1(a);
    aSig0 = extractFloat128Frac0(a);
    aExp = extractFloat128Exp(a);
    bSig1 = extractFloat128Frac1(b);
    bSig0 = extractFloat128Frac0(b);
    bExp = extractFloat128Exp(b);
    expDiff = aExp - bExp;
    if (0 < expDiff) {
        if (aExp == 0x7FFF) {
            if (aSig0 | aSig1) return propagateFloat128NaN(a, b, status);
            return a;
        }
        if (bExp == 0) --expDiff;
        else {
            bSig0 |= BX_CONST64(0x0001000000000000);
        }
        shift128ExtraRightJamming(
            bSig0, bSig1, 0, expDiff, &bSig0, &bSig1, &zSig2);
        zExp = aExp;
    }
    else if (expDiff < 0) {
        if (bExp == 0x7FFF) {
            if (bSig0 | bSig1) return propagateFloat128NaN(a, b, status);
            return packFloat128(zSign, 0x7FFF, 0, 0);
        }
        if (aExp == 0) ++expDiff;
        else {
            aSig0 |= BX_CONST64(0x0001000000000000);
        }
        shift128ExtraRightJamming(
            aSig0, aSig1, 0, - expDiff, &aSig0, &aSig1, &zSig2);
        zExp = bExp;
    }
    else {
        if (aExp == 0x7FFF) {
            if (aSig0 | aSig1 | bSig0 | bSig1) 
                return propagateFloat128NaN(a, b, status);

            return a;
        }
        add128(aSig0, aSig1, bSig0, bSig1, &zSig0, &zSig1);
        if (aExp == 0) return packFloat128(zSign, 0, zSig0, zSig1);
        zSig2 = 0;
        zSig0 |= BX_CONST64(0x0002000000000000);
        zExp = aExp;
        goto shiftRight1;
    }
    aSig0 |= BX_CONST64(0x0001000000000000);
    add128(aSig0, aSig1, bSig0, bSig1, &zSig0, &zSig1);
    --zExp;
    if (zSig0 < BX_CONST64(0x0002000000000000)) goto roundAndPack;
    ++zExp;
 shiftRight1:
    shift128ExtraRightJamming(zSig0, zSig1, zSig2, 1, &zSig0, &zSig1, &zSig2);
 roundAndPack:
    return roundAndPackFloat128(zSign, zExp, zSig0, zSig1, zSig2, status);
}

/*----------------------------------------------------------------------------
| Returns the result of subtracting the absolute values of the quadruple-
| precision floating-point values `a' and `b'.  If `zSign' is 1, the
| difference is negated before being returned.  `zSign' is ignored if the
| result is a NaN.  The subtraction is performed according to the IEC/IEEE
| Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static float128 subFloat128Sigs(float128 a, float128 b, int zSign, float_status_t &status)
{
    Bit32s aExp, bExp, zExp;
    Bit64u aSig0, aSig1, bSig0, bSig1, zSig0, zSig1;
    Bit32s expDiff;

    aSig1 = extractFloat128Frac1(a);
    aSig0 = extractFloat128Frac0(a);
    aExp = extractFloat128Exp(a);
    bSig1 = extractFloat128Frac1(b);
    bSig0 = extractFloat128Frac0(b);
    bExp = extractFloat128Exp(b);

    expDiff = aExp - bExp;
    shortShift128Left(aSig0, aSig1, 14, &aSig0, &aSig1);
    shortShift128Left(bSig0, bSig1, 14, &bSig0, &bSig1);
    if (0 < expDiff) goto aExpBigger;
    if (expDiff < 0) goto bExpBigger;
    if (aExp == 0x7FFF) {
        if (aSig0 | aSig1 | bSig0 | bSig1)
            return propagateFloat128NaN(a, b, status);

        float_raise(status, float_flag_invalid);
        return float128_default_nan;
    }
    if (aExp == 0) {
        aExp = 1;
        bExp = 1;
    }
    if (bSig0 < aSig0) goto aBigger;
    if (aSig0 < bSig0) goto bBigger;
    if (bSig1 < aSig1) goto aBigger;
    if (aSig1 < bSig1) goto bBigger;
    return packFloat128(0, 0, 0, 0);

 bExpBigger:
    if (bExp == 0x7FFF) {
        if (bSig0 | bSig1) return propagateFloat128NaN(a, b, status);
        return packFloat128(zSign ^ 1, 0x7FFF, 0, 0);
    }
    if (aExp == 0) ++expDiff;
    else {
        aSig0 |= BX_CONST64(0x4000000000000000);
    }
    shift128RightJamming(aSig0, aSig1, - expDiff, &aSig0, &aSig1);
    bSig0 |= BX_CONST64(0x4000000000000000);
 bBigger:
    sub128(bSig0, bSig1, aSig0, aSig1, &zSig0, &zSig1);
    zExp = bExp;
    zSign ^= 1;
    goto normalizeRoundAndPack;
 aExpBigger:
    if (aExp == 0x7FFF) {
        if (aSig0 | aSig1) return propagateFloat128NaN(a, b, status);
        return a;
    }
    if (bExp == 0) --expDiff;
    else {
        bSig0 |= BX_CONST64(0x4000000000000000);
    }
    shift128RightJamming(bSig0, bSig1, expDiff, &bSig0, &bSig1);
    aSig0 |= BX_CONST64(0x4000000000000000);
 aBigger:
    sub128(aSig0, aSig1, bSig0, bSig1, &zSig0, &zSig1);
    zExp = aExp;
 normalizeRoundAndPack:
    --zExp;
    return normalizeRoundAndPackFloat128(zSign, zExp - 14, zSig0, zSig1, status);
}

/*----------------------------------------------------------------------------
| Returns the result of adding the quadruple-precision floating-point values
| `a' and `b'.  The operation is performed according to the IEC/IEEE Standard
| for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

float128 float128_add(float128 a, float128 b, float_status_t &status)
{
    int aSign = extractFloat128Sign(a);
    int bSign = extractFloat128Sign(b);

    if (aSign == bSign) {
        return addFloat128Sigs(a, b, aSign, status);
    }
    else {
        return subFloat128Sigs(a, b, aSign, status);
    }
}

/*----------------------------------------------------------------------------
| Returns the result of subtracting the quadruple-precision floating-point
| values `a' and `b'.  The operation is performed according to the IEC/IEEE
| Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

float128 float128_sub(float128 a, float128 b, float_status_t &status)
{
    int aSign = extractFloat128Sign(a);
    int bSign = extractFloat128Sign(b);

    if (aSign == bSign) {
        return subFloat128Sigs(a, b, aSign, status);
    }
    else {
        return addFloat128Sigs(a, b, aSign, status);
    }
}

/*----------------------------------------------------------------------------
| Returns the result of multiplying the quadruple-precision floating-point
| values `a' and `b'.  The operation is performed according to the IEC/IEEE
| Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

float128 float128_mul(float128 a, float128 b, float_status_t &status)
{
    int aSign, bSign, zSign;
    Bit32s aExp, bExp, zExp;
    Bit64u aSig0, aSig1, bSig0, bSig1, zSig0, zSig1, zSig2, zSig3;

    aSig1 = extractFloat128Frac1(a);
    aSig0 = extractFloat128Frac0(a);
    aExp = extractFloat128Exp(a);
    aSign = extractFloat128Sign(a);
    bSig1 = extractFloat128Frac1(b);
    bSig0 = extractFloat128Frac0(b);
    bExp = extractFloat128Exp(b);
    bSign = extractFloat128Sign(b);

    zSign = aSign ^ bSign;
    if (aExp == 0x7FFF) {
        if (   (aSig0 | aSig1)
             || ((bExp == 0x7FFF) && (bSig0 | bSig1))) {
            return propagateFloat128NaN(a, b, status);
        }
        if ((bExp | bSig0 | bSig1) == 0) goto invalid;
        return packFloat128(zSign, 0x7FFF, 0, 0);
    }
    if (bExp == 0x7FFF) {
        if (bSig0 | bSig1) return propagateFloat128NaN(a, b, status);
        if ((aExp | aSig0 | aSig1) == 0) {
 invalid:
            float_raise(status, float_flag_invalid);
            return float128_default_nan;
        }
        return packFloat128(zSign, 0x7FFF, 0, 0);
    }
    if (aExp == 0) {
        if ((aSig0 | aSig1) == 0) return packFloat128(zSign, 0, 0, 0);
        normalizeFloat128Subnormal(aSig0, aSig1, &aExp, &aSig0, &aSig1);
    }
    if (bExp == 0) {
        if ((bSig0 | bSig1) == 0) return packFloat128(zSign, 0, 0, 0);
        normalizeFloat128Subnormal(bSig0, bSig1, &bExp, &bSig0, &bSig1);
    }
    zExp = aExp + bExp - 0x4000;
    aSig0 |= BX_CONST64(0x0001000000000000);
    shortShift128Left(bSig0, bSig1, 16, &bSig0, &bSig1);
    mul128To256(aSig0, aSig1, bSig0, bSig1, &zSig0, &zSig1, &zSig2, &zSig3);
    add128(zSig0, zSig1, aSig0, aSig1, &zSig0, &zSig1);
    zSig2 |= (zSig3 != 0);
    if (BX_CONST64(0x0002000000000000) <= zSig0) {
        shift128ExtraRightJamming(
            zSig0, zSig1, zSig2, 1, &zSig0, &zSig1, &zSig2);
        ++zExp;
    }
    return roundAndPackFloat128(zSign, zExp, zSig0, zSig1, zSig2, status);
}

/*----------------------------------------------------------------------------
| Returns the result of dividing the quadruple-precision floating-point value
| `a' by the corresponding value `b'.  The operation is performed according to
| the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

float128 float128_div(float128 a, float128 b, float_status_t &status)
{
    int aSign, bSign, zSign;
    Bit32s aExp, bExp, zExp;
    Bit64u aSig0, aSig1, bSig0, bSig1, zSig0, zSig1, zSig2;
    Bit64u rem0, rem1, rem2, rem3, term0, term1, term2, term3;

    aSig1 = extractFloat128Frac1(a);
    aSig0 = extractFloat128Frac0(a);
    aExp = extractFloat128Exp(a);
    aSign = extractFloat128Sign(a);
    bSig1 = extractFloat128Frac1(b);
    bSig0 = extractFloat128Frac0(b);
    bExp = extractFloat128Exp(b);
    bSign = extractFloat128Sign(b);

    zSign = aSign ^ bSign;
    if (aExp == 0x7FFF) {
        if (aSig0 | aSig1) return propagateFloat128NaN(a, b, status);
        if (bExp == 0x7FFF) {
            if (bSig0 | bSig1) return propagateFloat128NaN(a, b, status);
            goto invalid;
        }
        return packFloat128(zSign, 0x7FFF, 0, 0);
    }
    if (bExp == 0x7FFF) {
        if (bSig0 | bSig1) return propagateFloat128NaN(a, b, status);
        return packFloat128(zSign, 0, 0, 0);
    }
    if (bExp == 0) {
        if ((bSig0 | bSig1) == 0) {
            if ((aExp | aSig0 | aSig1) == 0) {
 invalid:
                float_raise(status, float_flag_invalid);
                return float128_default_nan;
            }
            float_raise(status, float_flag_divbyzero);
            return packFloat128(zSign, 0x7FFF, 0, 0);
        }
        normalizeFloat128Subnormal(bSig0, bSig1, &bExp, &bSig0, &bSig1);
    }
    if (aExp == 0) {
        if ((aSig0 | aSig1) == 0) return packFloat128(zSign, 0, 0, 0);
        normalizeFloat128Subnormal(aSig0, aSig1, &aExp, &aSig0, &aSig1);
    }
    zExp = aExp - bExp + 0x3FFD;
    shortShift128Left(
        aSig0 | BX_CONST64(0x0001000000000000), aSig1, 15, &aSig0, &aSig1);
    shortShift128Left(
        bSig0 | BX_CONST64(0x0001000000000000), bSig1, 15, &bSig0, &bSig1);
    if (le128(bSig0, bSig1, aSig0, aSig1)) {
        shift128Right(aSig0, aSig1, 1, &aSig0, &aSig1);
        ++zExp;
    }
    zSig0 = estimateDiv128To64(aSig0, aSig1, bSig0);
    mul128By64To192(bSig0, bSig1, zSig0, &term0, &term1, &term2);
    sub192(aSig0, aSig1, 0, term0, term1, term2, &rem0, &rem1, &rem2);
    while ((Bit64s) rem0 < 0) {
        --zSig0;
        add192(rem0, rem1, rem2, 0, bSig0, bSig1, &rem0, &rem1, &rem2);
    }
    zSig1 = estimateDiv128To64(rem1, rem2, bSig0);
    if ((zSig1 & 0x3FFF) <= 4) {
        mul128By64To192(bSig0, bSig1, zSig1, &term1, &term2, &term3);
        sub192(rem1, rem2, 0, term1, term2, term3, &rem1, &rem2, &rem3);
        while ((Bit64s) rem1 < 0) {
            --zSig1;
            add192(rem1, rem2, rem3, 0, bSig0, bSig1, &rem1, &rem2, &rem3);
        }
        zSig1 |= ((rem1 | rem2 | rem3) != 0);
    }
    shift128ExtraRightJamming(zSig0, zSig1, 0, 15, &zSig0, &zSig1, &zSig2);
    return roundAndPackFloat128(zSign, zExp, zSig0, zSig1, zSig2, status);
}

#endif
