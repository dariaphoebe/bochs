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


#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#define LOG_THIS BX_CPU_THIS_PTR

///////////////////////////////////////////////////////
/// TODO: Handle FPU last instruction and data pointers 
///////////////////////////////////////////////////////


#if BX_SUPPORT_FPU

#include "softfloat.h"
#include "softfloat-specialize.h"

static const floatx80 CONST_QNaN = { floatx80_default_nan_exp, floatx80_default_nan_fraction };

void BX_CPU_C::FPU_check_pending_exceptions(void)
{
  if(BX_CPU_THIS_PTR the_i387.get_partial_status() & FPU_SW_Summary)
  {
    if (BX_CPU_THIS_PTR cr0.ne == 0)
    {
      // MSDOS compatibility external interrupt (IRQ13)
      BX_INFO (("math_abort: MSDOS compatibility FPU exception"));
      DEV_pic_raise_irq(13);
    }
    else
      exception(BX_MF_EXCEPTION, 0, 0);
  }
}

void BX_CPU_C::FPU_exception(int exception)
{
  /* Extract only the bits which we use to set the status word */
  exception &= (FPU_SW_Exceptions_Mask);
  /* Set the corresponding exception bit */
  FPU_PARTIAL_STATUS |= exception;
  /* Set summary bits iff exception isn't masked */
  if (FPU_PARTIAL_STATUS & ~FPU_CONTROL_WORD & FPU_CW_Exceptions_Mask)
      FPU_PARTIAL_STATUS |= (FPU_SW_Summary | FPU_SW_Backward);

  if (exception & (FPU_SW_Stack_Fault | FPU_EX_Precision))
  {
      if (! (exception & FPU_SW_C1))
        /* This bit distinguishes over- from underflow for a stack fault,
             and roundup from round-down for precision loss. */
        FPU_PARTIAL_STATUS &= ~FPU_SW_C1;
  }
}

void BX_CPU_C::FPU_stack_underflow(int stnr, int pop_stack)
{
  /* The masked response */
  if (BX_CPU_THIS_PTR the_i387.get_control_word() & FPU_CW_Invalid)
  {
      BX_CPU_THIS_PTR the_i387.FPU_save_regi(CONST_QNaN, FPU_Tag_Special, stnr);
      if (pop_stack) 
          BX_CPU_THIS_PTR the_i387.FPU_pop();
  }
  BX_CPU_THIS_PTR FPU_exception(FPU_EX_Stack_Underflow);
}

softfloat_status_word_t BX_CPU_C::FPU_pre_exception_handling()
{
  softfloat_status_word_t status;

  int precision = BX_CPU_THIS_PTR the_i387.get_control_word() & FPU_CW_PC;

/* p 15-5: Precision control bits affect only the following:
   ADD, SUB(R), MUL, DIV(R), and SQRT */
#define FPU_PR_32_BITS        (0x000)
#define FPU_PR_RESERVED_BITS  (0x100)
#define FPU_PR_64_BITS        (0x200)
#define FPU_PR_80_BITS        (0x300)
  
  switch(precision)
  {
     case FPU_PR_32_BITS:
       status.float_rounding_precision = 32;
       break;
     case FPU_PR_64_BITS:
       status.float_rounding_precision = 64;
       break;
     default:
       status.float_rounding_precision = 80;
  }

  status.float_detect_tininess = float_tininess_after_rounding;
  status.float_exception_flags = 0; // clear exceptions before execution
  status.float_nan_handling_mode = float_first_operand_nan;
  status.float_rounding_mode = (BX_CPU_THIS_PTR the_i387.get_control_word() & FPU_CW_RC) >> 10;
  status.flush_underflow_to_zero = 0;

  return status;
}

void BX_CPU_C::FPU_post_exception_handling(const softfloat_status_word_t &status)
{
  BX_CPU_THIS_PTR FPU_exception(status.float_exception_flags);
}

#endif

void BX_CPU_C::FADD_ST0_STj(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  clear_C1();

  if (IS_TAG_EMPTY(0) || IS_TAG_EMPTY(i->rm()))
    FPU_stack_underflow(0);

  softfloat_status_word_t status = 
	BX_CPU_THIS_PTR FPU_pre_exception_handling();

  floatx80 result = floatx80_add(BX_CPU_THIS_PTR the_i387.FPU_read_regi(0), 
	BX_CPU_THIS_PTR the_i387.FPU_read_regi(i->rm()), status);

  BX_CPU_THIS_PTR FPU_post_exception_handling(status);
  BX_CPU_THIS_PTR the_i387.FPU_save_regi(result, 0);
#else
  BX_INFO(("FADD_ST0_STj: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FADD_STi_ST0(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  int pop_stack = (i->b1() & 0x10) >> 1;

  clear_C1();

  if (IS_TAG_EMPTY(0) || IS_TAG_EMPTY(i->rm()))
  {
    FPU_stack_underflow(i->rm(), pop_stack);
  }

  softfloat_status_word_t status = 
	BX_CPU_THIS_PTR FPU_pre_exception_handling();

  floatx80 result = floatx80_add(BX_CPU_THIS_PTR the_i387.FPU_read_regi(i->rm()), 
	BX_CPU_THIS_PTR the_i387.FPU_read_regi(0), status);

  BX_CPU_THIS_PTR FPU_post_exception_handling(status);
  BX_CPU_THIS_PTR the_i387.FPU_save_regi(result, i->rm());
  if (pop_stack) 
     BX_CPU_THIS_PTR the_i387.FPU_pop();
#else
  BX_INFO(("FADD(P)_STi_ST0: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FADD_SINGLE_REAL(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FADD_SINGLE_REAL: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FADD_DOUBLE_REAL(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FADD_DOUBLE_REAL: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FIADD_WORD_INTEGER(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FIADD_WORD_INTEGER: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FIADD_DWORD_INTEGER(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FIADD_DWORD_INTEGER: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FMUL_ST0_STj(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  clear_C1();

  if (IS_TAG_EMPTY(0) || IS_TAG_EMPTY(i->rm()))
    FPU_stack_underflow(0);

  softfloat_status_word_t status = 
	BX_CPU_THIS_PTR FPU_pre_exception_handling();

  floatx80 result = floatx80_mul(BX_CPU_THIS_PTR the_i387.FPU_read_regi(0), 
	BX_CPU_THIS_PTR the_i387.FPU_read_regi(i->rm()), status);

  BX_CPU_THIS_PTR FPU_post_exception_handling(status);
  BX_CPU_THIS_PTR the_i387.FPU_save_regi(result, 0);
#else
  BX_INFO(("FMUL_ST0_STj: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FMUL_STi_ST0(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  int pop_stack = (i->b1() & 0x10) >> 1;

  clear_C1();

  if (IS_TAG_EMPTY(0) || IS_TAG_EMPTY(i->rm()))
  {
    FPU_stack_underflow(i->rm(), pop_stack);
  }

  softfloat_status_word_t status = 
	BX_CPU_THIS_PTR FPU_pre_exception_handling();

  floatx80 result = floatx80_mul(BX_CPU_THIS_PTR the_i387.FPU_read_regi(i->rm()), 
	BX_CPU_THIS_PTR the_i387.FPU_read_regi(0), status);

  BX_CPU_THIS_PTR FPU_post_exception_handling(status);
  BX_CPU_THIS_PTR the_i387.FPU_save_regi(result, i->rm());
  if (pop_stack) 
     BX_CPU_THIS_PTR the_i387.FPU_pop();
#else
  BX_INFO(("FMUL(P)_STi_ST0: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FMUL_SINGLE_REAL(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FMUL_SINGLE_REAL: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FMUL_DOUBLE_REAL(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FMUL_DOUBLE_REAL: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FIMUL_WORD_INTEGER(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FIMUL_WORD_INTEGER: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FIMUL_DWORD_INTEGER(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FIMUL_DWORD_INTEGER: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FSUB_ST0_STj(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  clear_C1();

  if (IS_TAG_EMPTY(0) || IS_TAG_EMPTY(i->rm()))
    FPU_stack_underflow(0);

  softfloat_status_word_t status = 
	BX_CPU_THIS_PTR FPU_pre_exception_handling();

  floatx80 result = floatx80_sub(BX_CPU_THIS_PTR the_i387.FPU_read_regi(0), 
	BX_CPU_THIS_PTR the_i387.FPU_read_regi(i->rm()), status);

  BX_CPU_THIS_PTR FPU_post_exception_handling(status);
  BX_CPU_THIS_PTR the_i387.FPU_save_regi(result, 0);
#else
  BX_INFO(("FSUB_ST0_STj: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FSUBR_ST0_STj(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  clear_C1();

  if (IS_TAG_EMPTY(0) || IS_TAG_EMPTY(i->rm()))
    FPU_stack_underflow(0);

  softfloat_status_word_t status = 
	BX_CPU_THIS_PTR FPU_pre_exception_handling();

  floatx80 result = floatx80_sub(BX_CPU_THIS_PTR the_i387.FPU_read_regi(i->rm()), 
	BX_CPU_THIS_PTR the_i387.FPU_read_regi(0), status);

  BX_CPU_THIS_PTR FPU_post_exception_handling(status);
  BX_CPU_THIS_PTR the_i387.FPU_save_regi(result, 0);
#else
  BX_INFO(("FSUBR_ST0_STj: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FSUB_STi_ST0(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  int pop_stack = (i->b1() & 0x10) >> 1;

  clear_C1();

  if (IS_TAG_EMPTY(0) || IS_TAG_EMPTY(i->rm()))
  {
    FPU_stack_underflow(i->rm(), pop_stack);
  }

  softfloat_status_word_t status = 
	BX_CPU_THIS_PTR FPU_pre_exception_handling();

  floatx80 result = floatx80_sub(BX_CPU_THIS_PTR the_i387.FPU_read_regi(i->rm()), 
	BX_CPU_THIS_PTR the_i387.FPU_read_regi(0), status);

  BX_CPU_THIS_PTR FPU_post_exception_handling(status);
  BX_CPU_THIS_PTR the_i387.FPU_save_regi(result, i->rm());
  if (pop_stack) 
     BX_CPU_THIS_PTR the_i387.FPU_pop();
#else
  BX_INFO(("FSUB(P)_STi_ST0: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FSUBR_STi_ST0(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  int pop_stack = (i->b1() & 0x10) >> 1;

  clear_C1();

  if (IS_TAG_EMPTY(0) || IS_TAG_EMPTY(i->rm()))
  {
    FPU_stack_underflow(i->rm(), pop_stack);
  }

  softfloat_status_word_t status = 
	BX_CPU_THIS_PTR FPU_pre_exception_handling();

  floatx80 result = floatx80_sub(BX_CPU_THIS_PTR the_i387.FPU_read_regi(0), 
	BX_CPU_THIS_PTR the_i387.FPU_read_regi(i->rm()), status);

  BX_CPU_THIS_PTR FPU_post_exception_handling(status);
  BX_CPU_THIS_PTR the_i387.FPU_save_regi(result, i->rm());
  if (pop_stack) 
     BX_CPU_THIS_PTR the_i387.FPU_pop();
#else
  BX_INFO(("FSUBR(P)_STi_ST0: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FSUB_SINGLE_REAL(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FSUB_SINGLE_REAL: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FSUBR_SINGLE_REAL(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FSUBR_SINGLE_REAL: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FSUB_DOUBLE_REAL(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FSUB_DOUBLE_REAL: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FSUBR_DOUBLE_REAL(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FSUBR_DOUBLE_REAL: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FISUB_WORD_INTEGER(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FISUB_WORD_INTEGER: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FISUBR_WORD_INTEGER(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FISUBR_WORD_INTEGER: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FISUB_DWORD_INTEGER(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FISUB_DWORD_INTEGER: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FISUBR_DWORD_INTEGER(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FISUBR_DWORD_INTEGER: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FDIV_ST0_STj(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  clear_C1();

  if (IS_TAG_EMPTY(0) || IS_TAG_EMPTY(i->rm()))
    FPU_stack_underflow(0);

  softfloat_status_word_t status = 
	BX_CPU_THIS_PTR FPU_pre_exception_handling();

  floatx80 result = floatx80_div(BX_CPU_THIS_PTR the_i387.FPU_read_regi(0), 
	BX_CPU_THIS_PTR the_i387.FPU_read_regi(i->rm()), status);

  BX_CPU_THIS_PTR FPU_post_exception_handling(status);
  BX_CPU_THIS_PTR the_i387.FPU_save_regi(result, 0);
#else
  BX_INFO(("FDIV_ST0_STj: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FDIVR_ST0_STj(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  clear_C1();

  if (IS_TAG_EMPTY(0) || IS_TAG_EMPTY(i->rm()))
    FPU_stack_underflow(0);

  softfloat_status_word_t status = 
	BX_CPU_THIS_PTR FPU_pre_exception_handling();

  floatx80 result = floatx80_div(BX_CPU_THIS_PTR the_i387.FPU_read_regi(i->rm()), 
	BX_CPU_THIS_PTR the_i387.FPU_read_regi(0), status);

  BX_CPU_THIS_PTR FPU_post_exception_handling(status);
  BX_CPU_THIS_PTR the_i387.FPU_save_regi(result, 0);
#else
  BX_INFO(("FDIVR_ST0_STj: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FDIV_STi_ST0(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  int pop_stack = (i->b1() & 0x10) >> 1;

  clear_C1();

  if (IS_TAG_EMPTY(0) || IS_TAG_EMPTY(i->rm()))
  {
    FPU_stack_underflow(i->rm(), pop_stack);
  }

  softfloat_status_word_t status = 
	BX_CPU_THIS_PTR FPU_pre_exception_handling();

  floatx80 result = floatx80_div(BX_CPU_THIS_PTR the_i387.FPU_read_regi(i->rm()), 
	BX_CPU_THIS_PTR the_i387.FPU_read_regi(0), status);

  BX_CPU_THIS_PTR FPU_post_exception_handling(status);
  BX_CPU_THIS_PTR the_i387.FPU_save_regi(result, i->rm());
  if (pop_stack) 
     BX_CPU_THIS_PTR the_i387.FPU_pop();
#else
  BX_INFO(("FDIV(P)_STi_ST0: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FDIVR_STi_ST0(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  int pop_stack = (i->b1() & 0x10) >> 1;

  clear_C1();

  if (IS_TAG_EMPTY(0) || IS_TAG_EMPTY(i->rm()))
  {
    FPU_stack_underflow(i->rm(), pop_stack);
  }

  softfloat_status_word_t status = 
	BX_CPU_THIS_PTR FPU_pre_exception_handling();

  floatx80 result = floatx80_div(BX_CPU_THIS_PTR the_i387.FPU_read_regi(0), 
	BX_CPU_THIS_PTR the_i387.FPU_read_regi(i->rm()), status);

  BX_CPU_THIS_PTR FPU_post_exception_handling(status);
  BX_CPU_THIS_PTR the_i387.FPU_save_regi(result, i->rm());
  if (pop_stack) 
     BX_CPU_THIS_PTR the_i387.FPU_pop();
#else
  BX_INFO(("FDIVR(P)_STi_ST0: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FDIV_SINGLE_REAL(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FDIV_SINGLE_REAL: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FDIVR_SINGLE_REAL(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FDIVR_SINGLE_REAL: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FDIV_DOUBLE_REAL(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FDIV_DOUBLE_REAL: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FDIVR_DOUBLE_REAL(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FDIVR_DOUBLE_REAL: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FIDIV_WORD_INTEGER(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FIDIV_WORD_INTEGER: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FIDIVR_WORD_INTEGER(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FIDIVR_WORD_INTEGER: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FIDIV_DWORD_INTEGER(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FIDIV_DWORD_INTEGER: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FIDIVR_DWORD_INTEGER(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FIDIVR_DWORD_INTEGER: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FXTRACT(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FXTRACT: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FPREM1(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FPREM1: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FPREM(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FPREM: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FSQRT(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FSQRT: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FRNDINT(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FRNDINT: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FSCALE(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FSCALE: required FPU, configure --enable-fpu"));
#endif
}
