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

#include "softfloatx80.h"
#include "softfloat-round-pack.h"
#include "softfloat-macros.h"

/* 128-bit PI/2 fraction */
static const Bit64u Hi = BX_CONST64(0xC90FDAA22168C234);
static const Bit64u Lo = BX_CONST64(0xC4C6628B80DC1CD1);

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
    return q;
}

floatx80 trig_arg_reduction(floatx80 a, Bit64u &q, float_status_t &status)
{
    Bit64u aSig0, aSig1, term0, term1;
    Bit32s aExp, expDiff, zExp;
    int aSign; 
    q = 0;

    // handle unsupported extended double-precision floating encodings
    if (floatx80_is_unsupported(a))
    {
        float_raise(status, float_flag_invalid);
        return floatx80_default_nan;
    }

    aSig0 = extractFloatx80Frac(a);
    aExp = extractFloatx80Exp(a);
    aSign = extractFloatx80Sign(a);

    if (aExp == 0x7FFF) {
        if ((Bit64u) (aSig0<<1)) 
            return propagateFloatx80NaN(a, status);

        float_raise(status, float_flag_invalid);
        return floatx80_default_nan;
    }
    if (aExp == 0) {
        if ((Bit64u) (aSig0<<1) == 0) return a;
        float_raise(status, float_flag_denormal);
        normalizeFloatx80Subnormal(aSig0, &aExp, &aSig0);
    }
    expDiff = aExp - 0x3FFF;
    aSig1 = 0;

    if (expDiff >= 63) 
    {
        q = (Bit64u) -1;
    }
    else {
        zExp = 0x3FFF;

        if (expDiff < 0) {
            if (expDiff < -1)
            {
               return (a.fraction & BX_CONST64(0x8000000000000000)) ? 
                    packFloatx80(aSign, aExp, aSig0) : a;
            }
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
               aSign = !aSign;
               ++q;
            }
            if (lt) sub128(Hi, Lo, aSig0, aSig1, &aSig0, &aSig1);
        }
    }

    return normalizeRoundAndPackFloatx80(80, aSign, zExp, aSig0, aSig1, status);
}
