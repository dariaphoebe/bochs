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

static const floatx80 floatx80_negone  = packFloatx80(1, 0x3fff, BX_CONST64(0x8000000000000000));
static const floatx80 floatx80_neghalf = packFloatx80(1, 0x3ffe, BX_CONST64(0x8000000000000000));
static const floatx80 floatx80_ln2     = packFloatx80(0, 0x3ffe, BX_CONST64(0xb17217f7d1cf79ac));
static const float128 float128_ln2     = 
    packFloat128(BX_CONST64(0x3ffe62e42fefa39e), BX_CONST64(0xf35793c7673007e6));

#define EXP_ARR_SIZE 15

static float128 exp_arr[EXP_ARR_SIZE] =
{
    packFloat128(BX_CONST64(0x3fff000000000000), BX_CONST64(0x0000000000000000)), /*  1 */
    packFloat128(BX_CONST64(0x3ffe000000000000), BX_CONST64(0x0000000000000000)), /*  2 */
    packFloat128(BX_CONST64(0x3ffc555555555555), BX_CONST64(0x5555555555555555)), /*  3 */
    packFloat128(BX_CONST64(0x3ffa555555555555), BX_CONST64(0x5555555555555555)), /*  4 */
    packFloat128(BX_CONST64(0x3ff8111111111111), BX_CONST64(0x1111111111111111)), /*  5 */
    packFloat128(BX_CONST64(0x3ff56c16c16c16c1), BX_CONST64(0x6c16c16c16c16c17)), /*  6 */
    packFloat128(BX_CONST64(0x3ff2a01a01a01a01), BX_CONST64(0xa01a01a01a01a01a)), /*  7 */
    packFloat128(BX_CONST64(0x3fefa01a01a01a01), BX_CONST64(0xa01a01a01a01a01a)), /*  8 */
    packFloat128(BX_CONST64(0x3fec71de3a556c73), BX_CONST64(0x38faac1c88e50017)), /*  9 */
    packFloat128(BX_CONST64(0x3fe927e4fb7789f5), BX_CONST64(0xc72ef016d3ea6679)), /* 10 */
    packFloat128(BX_CONST64(0x3fe5ae64567f544e), BX_CONST64(0x38fe747e4b837dc7)), /* 11 */
    packFloat128(BX_CONST64(0x3fe21eed8eff8d89), BX_CONST64(0x7b544da987acfe85)), /* 12 */
    packFloat128(BX_CONST64(0x3fde6124613a86d0), BX_CONST64(0x97ca38331d23af68)), /* 13 */
    packFloat128(BX_CONST64(0x3fda93974a8c07c9), BX_CONST64(0xd20badf145dfa3e5)), /* 14 */
    packFloat128(BX_CONST64(0x3fd6ae7f3e733b81), BX_CONST64(0xf11d8656b0ee8cb0))  /* 15 */
};

/* required -1 < x < 1 */
static float128 poly_exp(float128 x1, float_status_t &status)
{
    float128 x2 = float128_mul(x1, x1, status);
    float128 r1, r2;

    // r1 = x1*(a_1 + x2*(a_3 + x2*(a_5 + x2*(a_7 + x2*(a_9 + x2*(a_11+x2*a_13)))))
    r1 = float128_mul(x2, exp_arr[13], status);
    r1 = float128_add(r1, exp_arr[11], status);
    r1 = float128_mul(r1, x2, status);
    r1 = float128_add(r1, exp_arr[9], status);
    r1 = float128_mul(r1, x2, status);
    r1 = float128_add(r1, exp_arr[7], status);
    r1 = float128_mul(r1, x2, status);
    r1 = float128_add(r1, exp_arr[5], status);
    r1 = float128_mul(r1, x2, status);
    r1 = float128_add(r1, exp_arr[3], status);
    r1 = float128_mul(r1, x2, status);
    r1 = float128_add(r1, exp_arr[1], status);
    r1 = float128_mul(r1, x1, status);

    // r2 = x2*(a_2 + x2*(a_4 + x2*(a_6 + x2*(a_8 + x2*(a_10 + x2*(a_12+x2*a_14))))))
    r2 = float128_mul(x2, exp_arr[14], status);
    r2 = float128_add(r2, exp_arr[12], status);
    r2 = float128_mul(r2, x2, status);
    r2 = float128_add(r2, exp_arr[10], status);
    r2 = float128_mul(r2, x2, status);
    r2 = float128_add(r2, exp_arr[8], status);
    r2 = float128_mul(r2, x2, status);
    r2 = float128_add(r2, exp_arr[6], status);
    r2 = float128_mul(r2, x2, status);
    r2 = float128_add(r2, exp_arr[4], status);
    r2 = float128_mul(r2, x2, status);
    r2 = float128_add(r2, exp_arr[2], status);
    r2 = float128_mul(r2, x2, status);

    r1 = float128_add(r1, r2, status);
    r1 = float128_add(r1, exp_arr[0], status);
    r1 = float128_mul(r1, x1, status);

    return r1;
}

floatx80 f2xm1(floatx80 a, float_status_t &status)
{
    // handle unsupported extended double-precision floating encodings
    if (floatx80_is_unsupported(a)) 
    {
        float_raise(status, float_flag_invalid);
        return floatx80_default_nan;
    }

    Bit64u aSig = extractFloatx80Frac(a);
    Bit32s aExp = extractFloatx80Exp(a);
    int aSign = extractFloatx80Sign(a);
     
    if (aExp == 0x7FFF) {
        if ((Bit64u) (aSig<<1))
            return propagateFloatx80NaN(a, status);

        return (aSign) ? floatx80_negone : a;
    }

    if (aExp == 0) {
        if ((Bit64u) (aSig<<1) == 0) 
            return a;

        float_raise(status, float_flag_denormal | float_flag_inexact);
        return floatx80_mul(a, floatx80_ln2, status);
    }

    float_raise(status, float_flag_inexact);

    if (aExp < 0x3FFF)
    {
        if (aExp < 0x3FBB)
            return floatx80_mul(a, floatx80_ln2, status);

        /* using float128 for approximation */
        float128 x = floatx80_to_float128(a, status);
        x = float128_mul(x, float128_ln2, status);
        x = poly_exp(x, status);
        return float128_to_floatx80(x, status);
    }
    else 
    {
        if ((a.exp == 0xBFFF) && (! (aSig<<1)))
           return floatx80_neghalf;

        return a;
    }
}
