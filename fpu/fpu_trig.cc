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


#if BX_SUPPORT_FPU
extern float128 poly_eval(float128 x, float128 *carr, int n, float_status_t &status);

static const floatx80 floatx80_one = packFloatx80(0, 0x3fff, BX_CONST64(0x8000000000000000));
static const floatx80 floatx80_pi2 = packFloatx80(0, 0x3fff, BX_CONST64(0xc90fdaa22168c235));

static float128 poly_sin(float128 x, float_status_t &status)
{
    if (float128_exp(x) < 0x3FBB) /* handle tiny arguments */
    {
        return x;
    }

    return poly_eval(x, /*sin_arr*/0, /*7*/0, status);
}

static float128 poly_cos(float128 x, float_status_t &status)
{
    if (float128_exp(x) < 0x3FBB) /* handle tiny arguments */
    {                               
        return floatx80_to_float128(floatx80_one, status); // for now
    }

    return poly_eval(x, /*cos_arr*/0, /*7*/0, status);
}

static void sincos_approximation(floatx80 &x, int quotient, float_status_t &status)
{
  float128 r = floatx80_to_float128(x, status);
  quotient &= 3;
  if (quotient & 0x1)
      r = poly_cos(r, status);
  else  
      r = poly_sin(r, status);

  x = float128_to_floatx80(r, status);
  if (quotient & 0x2)
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

  Bit64u quotient;

  /* reduce trigonometric function argument */
  floatx80 y = floatx80_ieee754_remainder(BX_READ_FPU_REG(0), 
	floatx80_pi2, quotient, status);
  if (quotient == (Bit64u) -1) 
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

  Bit64u quotient;

  /* reduce trigonometric function argument */
  floatx80 y = floatx80_ieee754_remainder(BX_READ_FPU_REG(0), 
	floatx80_pi2, quotient, status);
  if (quotient == (Bit64u) -1) 
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

  sincos_approximation(y, quotient+1, status);

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

  Bit64u quotient;

  /* reduce trigonometric function argument */
  floatx80 y = floatx80_ieee754_remainder(BX_READ_FPU_REG(0), 
	floatx80_pi2, quotient, status);
  if (quotient == (Bit64u) -1) 
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

  floatx80 siny, cosy;

  sincos_approximation(siny, quotient, status);
  quotient++;
  sincos_approximation(cosy, quotient, status);

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

  Bit64u quotient;

  /* reduce trigonometric function argument */
  floatx80 y = floatx80_ieee754_remainder(BX_READ_FPU_REG(0), 
	floatx80_pi2, quotient, status);
  if (quotient == (Bit64u) -1) 
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

  float128 r = floatx80_to_float128(y, status);
  float128 sin_r = poly_sin(r, status);
  float128 cos_r = poly_cos(r, status);

  if (quotient & 0x1) {
      r = float128_div(cos_r, sin_r, status);
  } else {
      r = float128_div(sin_r, cos_r, status);
  }

  y = float128_to_floatx80(r, status);

  if (BX_CPU_THIS_PTR FPU_exception(status.float_exception_flags))
      return;

  if (quotient & 0x2)
  {
      floatx80_chs(y);
  }

  BX_WRITE_FPU_REG(y, 0);
  BX_CPU_THIS_PTR the_i387.FPU_push();
  BX_WRITE_FPU_REG(floatx80_one, 0);
#else
  BX_INFO(("FPTAN: required FPU, configure --enable-fpu"));
#endif
}
