/////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2004  MandrakeSoft S.A.
//
//    MandrakeSoft S.A.
//    43, rue d'Aboukir
//    75002 Paris - France
//    http://www.linux-mandrake.com/
//    http://www.mandrakesoft.com/
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
/////////////////////////////////////////////////////////////////////////

#define FLOAT128

#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_FPU
#include "softfloatx80.h"
#endif

extern Bit64u trig_arg_reduction(floatx80 &a, int function, float_status_t &status);

#define F_SIN 0
#define F_COS 1

#if BX_SUPPORT_FPU
static const floatx80 floatx80_one = packFloatx80(0, 0x3fff, BX_CONST64(0x8000000000000000));
static const float128 float128_one = packFloat128(0, 0x3fff, 0, 0);

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
       float128_add(float128_one, r1, status);
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

#define EXP_BIAS 0x3FFF

static int handle_small_argument(floatx80 &a, Bit64u quotient)
{
    if (floatx80_exp(a) <= EXP_BIAS - 68)
    {
        if (quotient & 0x1) a = floatx80_one;
        if (quotient & 0x2) 
           floatx80_chs(a);

        return 1;
    }

    return 0;
}

static void sincos_approximation(floatx80 &x, Bit64u quotient, float_status_t &status)
{
  int neg = floatx80_sign(x);
  float128 r = floatx80_to_float128(floatx80_abs(x), status);
  if (quotient & 0x1) {
      r = poly_cos(r, status);
      neg = 0;
  } else  {
      r = poly_sin(r, status);
  }

  x = float128_to_floatx80(r, status);
  if (quotient & 0x2) 
      neg = ! neg;

  if (neg) 
      floatx80_chs(x);
}
#endif

/* D9 FE */
void BX_CPU_C::FSIN(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i);

  clear_C1();

  if (IS_TAG_EMPTY(0))
  {
     BX_CPU_THIS_PTR FPU_stack_underflow(0);
     return;
  }

  softfloat_status_word_t status = 
	FPU_pre_exception_handling(BX_CPU_THIS_PTR the_i387.get_control_word());

  floatx80 y = BX_READ_FPU_REG(0);

  /* reduce trigonometric function argument */
  Bit64u quotient = trig_arg_reduction(y, F_SIN, status);
  if (quotient == -1)
  {
      FPU_PARTIAL_STATUS |= FPU_SW_C2;
      return;
  }

  clear_C2();

  if (floatx80_is_nan(y))
  {   
      if (! BX_CPU_THIS_PTR FPU_exception(status.float_exception_flags))
          BX_WRITE_FPU_REGISTER_AND_TAG(y, FPU_Tag_Special, 0);

      return;
  }

  if (handle_small_argument(y, quotient))
  {   
      if (! BX_CPU_THIS_PTR FPU_exception(status.float_exception_flags))
          BX_WRITE_FPU_REG(y, 0);

      return;
  }

  sincos_approximation(y, quotient, status);

  if (BX_CPU_THIS_PTR FPU_exception(status.float_exception_flags))
      return;

  BX_WRITE_FPU_REG(y, 0);
#else
  BX_INFO(("FSIN: required FPU, configure --enable-fpu"));
#endif
}

/* D9 FF */
void BX_CPU_C::FCOS(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i);

  clear_C1();

  if (IS_TAG_EMPTY(0))
  {
     BX_CPU_THIS_PTR FPU_stack_underflow(0);
     return;
  }

  softfloat_status_word_t status = 
	FPU_pre_exception_handling(BX_CPU_THIS_PTR the_i387.get_control_word());

  floatx80 y = BX_READ_FPU_REG(0);

  /* reduce trigonometric function argument */
  Bit64u quotient = trig_arg_reduction(y, F_COS, status);
  if (quotient == -1)
  {
      FPU_PARTIAL_STATUS |= FPU_SW_C2;
      return;
  }

  ++ quotient;
  clear_C2();

  if (floatx80_is_nan(y))
  {
      if (! BX_CPU_THIS_PTR FPU_exception(status.float_exception_flags))
          BX_WRITE_FPU_REGISTER_AND_TAG(y, FPU_Tag_Special, 0);

      return;
  }

  if (handle_small_argument(y, quotient))
  {   
      if (! BX_CPU_THIS_PTR FPU_exception(status.float_exception_flags))
          BX_WRITE_FPU_REG(y, 0);

      return;
  }

  sincos_approximation(y, quotient, status);

  if (BX_CPU_THIS_PTR FPU_exception(status.float_exception_flags))
      return;

  BX_WRITE_FPU_REG(y, 0);
#else
  BX_INFO(("FCOS: required FPU, configure --enable-fpu"));
#endif
}

/* D9 FB */
void BX_CPU_C::FSINCOS(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i);

  clear_C1();

  if (! IS_TAG_EMPTY(-1) || IS_TAG_EMPTY(0))
  {
      BX_CPU_THIS_PTR FPU_exception(FPU_EX_Stack_Overflow);

      /* The masked response */
      if (BX_CPU_THIS_PTR the_i387.is_IA_masked())
      {
          BX_WRITE_FPU_REGISTER_AND_TAG(floatx80_default_nan, FPU_Tag_Special, 0);
          BX_CPU_THIS_PTR the_i387.FPU_push();
          BX_WRITE_FPU_REGISTER_AND_TAG(floatx80_default_nan, FPU_Tag_Special, 0);
      }

      return; 
  }

  softfloat_status_word_t status = 
	FPU_pre_exception_handling(BX_CPU_THIS_PTR the_i387.get_control_word());

  floatx80 y = BX_READ_FPU_REG(0);

  /* reduce trigonometric function argument */
  Bit64u quotient = trig_arg_reduction(y, F_SIN, status);
  if (quotient == -1)
  {
      FPU_PARTIAL_STATUS |= FPU_SW_C2;
      return;
  }

  clear_C2();

  if (floatx80_is_nan(y))
  {
      if (! BX_CPU_THIS_PTR FPU_exception(status.float_exception_flags))
      {   /* masked responce */
          BX_WRITE_FPU_REGISTER_AND_TAG(y, FPU_Tag_Special, 0);
          BX_CPU_THIS_PTR the_i387.FPU_push();
          BX_WRITE_FPU_REGISTER_AND_TAG(y, FPU_Tag_Special, 0);
      }

      return;
  }

  floatx80 siny = y, cosy = y;

  if (handle_small_argument(siny, quotient))
  {
      quotient++;
      handle_small_argument(cosy, quotient);
  }
  else {
      sincos_approximation(siny, quotient, status);
      quotient++;
      sincos_approximation(cosy, quotient, status);
  }

  if (BX_CPU_THIS_PTR FPU_exception(status.float_exception_flags))
      return;

  BX_WRITE_FPU_REG(siny, 0);
  BX_CPU_THIS_PTR the_i387.FPU_push();
  BX_WRITE_FPU_REG(cosy, 0);
#else
  BX_INFO(("FSINCOS: required FPU, configure --enable-fpu"));
#endif
}

/* D9 F2 */
void BX_CPU_C::FPTAN(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i);

  clear_C1();

  if (! IS_TAG_EMPTY(-1) || IS_TAG_EMPTY(0))
  {
      BX_CPU_THIS_PTR FPU_exception(FPU_EX_Stack_Overflow);

      /* The masked response */
      if (BX_CPU_THIS_PTR the_i387.is_IA_masked())
      {
          BX_WRITE_FPU_REGISTER_AND_TAG(floatx80_default_nan, FPU_Tag_Special, 0);
          BX_CPU_THIS_PTR the_i387.FPU_push();
          BX_WRITE_FPU_REGISTER_AND_TAG(floatx80_default_nan, FPU_Tag_Special, 0);
      }

      return; 
  }

  softfloat_status_word_t status = 
	FPU_pre_exception_handling(BX_CPU_THIS_PTR the_i387.get_control_word());

  floatx80 y = BX_READ_FPU_REG(0);

  /* reduce trigonometric function argument */
  Bit64u quotient = trig_arg_reduction(y, F_SIN, status);
  if (quotient == -1)
  {
      FPU_PARTIAL_STATUS |= FPU_SW_C2;
      return;
  }

  clear_C2();

  if (floatx80_is_nan(y))
  {
      if (! BX_CPU_THIS_PTR FPU_exception(status.float_exception_flags))
      {
          BX_WRITE_FPU_REGISTER_AND_TAG(y, FPU_Tag_Special, 0);
          BX_CPU_THIS_PTR the_i387.FPU_push();
          BX_WRITE_FPU_REGISTER_AND_TAG(y, FPU_Tag_Special, 0);
      }

      return;
  }

  if (handle_small_argument(y, quotient))
  {   
      if (! BX_CPU_THIS_PTR FPU_exception(status.float_exception_flags))
          BX_WRITE_FPU_REG(y, 0);

      return;
  }

  int neg = floatx80_sign(y);
  float128 r = floatx80_to_float128(floatx80_abs(y), status);
  float128 sin_r = poly_sin(r, status);
  float128 cos_r = poly_cos(r, status);

  if (quotient & 0x1) {
      r = float128_div(cos_r, sin_r, status);
      neg = ! neg;
  } else {
      r = float128_div(sin_r, cos_r, status);
  }

  y = float128_to_floatx80(r, status);

  if (BX_CPU_THIS_PTR FPU_exception(status.float_exception_flags))
      return;

  if (neg)
      floatx80_chs(y);

  BX_WRITE_FPU_REG(y, 0);
  BX_CPU_THIS_PTR the_i387.FPU_push();
  BX_WRITE_FPU_REGISTER_AND_TAG(floatx80_one, FPU_Tag_Valid, 0);
#else
  BX_INFO(("FPTAN: required FPU, configure --enable-fpu"));
#endif
}
