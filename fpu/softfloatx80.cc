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
 * Adapted for Bochs (x86 achitecture simulator) by
 *            Stanislav Shwartsman (gate@fidonet.org.il)
 * ==========================================================================*/ 

/*============================================================================
 * Unlike original softfloat library code, the functions included in this
 * source package  fully handle unsupported encodings for extended double
 * precision floating point values.
 * ==========================================================================*/ 

#include "softfloatx80.h"
#include "softfloat-round-pack.h"

/*----------------------------------------------------------------------------
| Primitive arithmetic functions, including multi-word arithmetic, and
| division and square root approximations. (Can be specialized to target
| if desired).
*----------------------------------------------------------------------------*/

#include "softfloat-macros.h"

/*----------------------------------------------------------------------------
| Returns the result of converting the extended double-precision floating-
| point value `a' to the 16-bit two's complement integer format.  The
| conversion is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic - which means in particular that the conversion
| is rounded according to the current rounding mode. If `a' is a NaN or the 
| conversion overflows, the integer indefinite value is returned.
*----------------------------------------------------------------------------*/

Bit16s floatx80_to_int16(floatx80 a, float_status_t &status)
{
   if (floatx80_is_unsupported(a))
   {
        float_raise(status, float_flag_invalid);
        return int16_indefinite;
   }

   Bit32s v32 = floatx80_to_int32(a, status);

   if ((v32 > (Bit32s) BX_MAX_BIT16S) || (v32 < (Bit32s) BX_MIN_BIT16S))
   {
        float_raise(status, float_flag_invalid);
        return int16_indefinite;
   }

   return (Bit16s) v32;
}

/*----------------------------------------------------------------------------
| Returns the result of converting the extended double-precision floating-
| point value `a' to the 16-bit two's complement integer format.  The
| conversion is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic, except that the conversion is always rounded
| toward zero.  If `a' is a NaN or the conversion overflows, the integer 
| indefinite value is returned.
*----------------------------------------------------------------------------*/

Bit16s floatx80_to_int16_round_to_zero(floatx80 a, float_status_t &status)
{
   if (floatx80_is_unsupported(a))
   {
        float_raise(status, float_flag_invalid);
        return int16_indefinite;
   }

   Bit32s v32 = floatx80_to_int32_round_to_zero(a, status);

   if ((v32 > (Bit32s) BX_MAX_BIT16S) || (v32 < (Bit32s) BX_MIN_BIT16S))
   {
        float_raise(status, float_flag_invalid);
        return int16_indefinite;
   }

   return (Bit16s) v32;
}

/*----------------------------------------------------------------------------
| Separate the source extended double-precision floating point value `a'
| into its exponent and significand, store the significant back to the
| 'a' and return the exponent. The  operation performed is a superset of 
| the IEC/IEEE recommended logb(x) function.
*----------------------------------------------------------------------------*/

floatx80 floatx80_extract(floatx80 &a, float_status_t &status)
{
    Bit64u aSig = extractFloatx80Frac(a);
    Bit32s aExp = extractFloatx80Exp(a);
    int   aSign = extractFloatx80Sign(a);

    if (floatx80_is_unsupported(a))
    {
        float_raise(status, float_flag_invalid);
        a = Const_QNaN;
        return a;
    }

    if (aExp == 0x7FFF) {
        if ((Bit64u) (aSig<<1)) 
        {
            float_raise(status, float_flag_invalid);
            a = propagateFloatx80NaN(a, status);
            return a;
        }
        return Const_INF;
    }
    if (aExp == 0)
    {
        if (aSig == 0) {
            float_raise(status, float_flag_divbyzero);
            a = packFloatx80(aSign, 0, 0);
            return packFloatx80(1, 0x7FFF, BX_CONST64(0x8000000000000000));
        }
        float_raise(status, float_flag_denormal);
        normalizeFloatx80Subnormal(aSig, &aExp, &aSig);
    }

    a.exp = (aSign << 15) + 0x3FFF;
    a.fraction = aSig;
    return int32_to_floatx80(aExp - 0x3FFF);
}

/*----------------------------------------------------------------------------
| Determine extended-precision floating-point number class.
*----------------------------------------------------------------------------*/

float_class_t floatx80_class(floatx80 a)
{
   Bit32s aExp = extractFloatx80Exp(a);
   Bit64u aSig = extractFloatx80Frac(a);

   if(aExp == 0) {
       if (aSig == 0)
           return float_zero;

       /* denormal or pseudo-denormal */
       return float_denormal;
   }

   /* valid numbers have the MS bit set */
   if (!(aSig & BX_CONST64(0x8000000000000000)))
       return float_NaN; /* report unsupported as NaNs */

   if(aExp == 0x7fff) {
       int aSign = extractFloatx80Sign(a);

       if (((Bit64u) (aSig<< 1)) == 0)
           return (aSign) ? float_negative_inf : float_positive_inf;

       return float_NaN;
   }
    
   return float_normalized;
}

/*----------------------------------------------------------------------------
| Compare  between  two extended precision  floating  point  numbers. Returns
| 'float_relation_equal'  if the operands are equal, 'float_relation_less' if
| the    value    'a'   is   less   than   the   corresponding   value   `b',
| 'float_relation_greater' if the value 'a' is greater than the corresponding
| value `b', or 'float_relation_unordered' otherwise. 
*----------------------------------------------------------------------------*/

int floatx80_compare(floatx80 a, floatx80 b, float_status_t &status)
{
    float_class_t aClass = floatx80_class(a);
    float_class_t bClass = floatx80_class(b);

    if (aClass == float_NaN || bClass == float_NaN)
    {
        float_raise(status, float_flag_invalid);
        return float_relation_unordered;
    }

    if (aClass == float_denormal || bClass == float_denormal) 
    {
        float_raise(status, float_flag_denormal);
    }

    if ((a.fraction == b.fraction) && (a.exp == b.exp))
    {
        return float_relation_equal;
    }

    if (aClass == float_zero && bClass == float_zero)
    {
        return float_relation_equal;
    }

    int aSign = extractFloatx80Sign(a);
    int bSign = extractFloatx80Sign(b);
    if (aSign != bSign)
        return (aSign) ? float_relation_less : float_relation_greater;

    int less_than = 
	aSign ? lt128(b.exp, b.fraction, a.exp, a.fraction)
	      : lt128(a.exp, a.fraction, b.exp, b.fraction);

    if (less_than) return float_relation_less;
    return float_relation_greater;
}

/*----------------------------------------------------------------------------
| Compare  between  two extended precision  floating  point  numbers. Returns
| 'float_relation_equal'  if the operands are equal, 'float_relation_less' if
| the    value    'a'   is   less   than   the   corresponding   value   `b',
| 'float_relation_greater' if the value 'a' is greater than the corresponding
| value `b', or 'float_relation_unordered' otherwise. Quiet NaNs do not cause 
| an exception.
*----------------------------------------------------------------------------*/

int floatx80_compare_quiet(floatx80 a, floatx80 b, float_status_t &status)
{
    float_class_t aClass = floatx80_class(a);
    float_class_t bClass = floatx80_class(b);

    if (aClass == float_NaN || bClass == float_NaN)
    {
        if (floatx80_is_unsupported(a) || floatx80_is_unsupported(b))
            float_raise(status, float_flag_invalid);

        if (floatx80_is_signaling_nan(a) || floatx80_is_signaling_nan(b))
            float_raise(status, float_flag_invalid);

        return float_relation_unordered;
    }

    if (aClass == float_denormal || bClass == float_denormal) 
    {
        float_raise(status, float_flag_denormal);
    }

    if ((a.fraction == b.fraction) && (a.exp == b.exp))
    {
        return float_relation_equal;
    }

    if (aClass == float_zero && bClass == float_zero)
    {
        return float_relation_equal;
    }

    int aSign = extractFloatx80Sign(a);
    int bSign = extractFloatx80Sign(b);
    if (aSign != bSign)
        return (aSign) ? float_relation_less : float_relation_greater;

    int less_than = 
	aSign ? lt128(b.exp, b.fraction, a.exp, a.fraction)
	      : lt128(a.exp, a.fraction, b.exp, b.fraction);

    if (less_than) return float_relation_less;
    return float_relation_greater;
}
