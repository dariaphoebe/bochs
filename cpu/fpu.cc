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


#define UPDATE_LAST_OPCODE       1
#define CHECK_PENDING_EXCEPTIONS 1


#if BX_SUPPORT_FPU
void BX_CPU_C::prepareFPU(bxInstruction_c *i, 
	bx_bool check_pending_exceptions, bx_bool update_last_instruction)
{
  if (BX_CPU_THIS_PTR cr0.em || BX_CPU_THIS_PTR cr0.ts)
    exception(BX_NM_EXCEPTION, 0, 0);

  if (check_pending_exceptions)
    BX_CPU_THIS_PTR FPU_check_pending_exceptions();

  if (update_last_instruction)
  {
    BX_CPU_THIS_PTR the_i387.foo = ((Bit32u)(i->b1()) << 8) | (Bit32u)(i->modrm());    BX_CPU_THIS_PTR the_i387.fcs_= BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value;
    BX_CPU_THIS_PTR the_i387.fip_= BX_CPU_THIS_PTR prev_eip;

    if (! i->modC0()) {
         BX_CPU_THIS_PTR the_i387.fds_= BX_CPU_THIS_PTR sregs[i->seg()].selector.value;
         BX_CPU_THIS_PTR the_i387.fos_= RMAddr(i);
    } else {
         BX_CPU_THIS_PTR the_i387.fds_= BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].selector.value;
         BX_CPU_THIS_PTR the_i387.fos_= 0;
    }
  }
}

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
#endif

/* 9B */
void BX_CPU_C::FWAIT(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  if (BX_CPU_THIS_PTR cr0.ts && BX_CPU_THIS_PTR cr0.mp)
    exception(BX_NM_EXCEPTION, 0, 0);

  BX_CPU_THIS_PTR FPU_check_pending_exceptions();
#else
  BX_INFO(("FWAIT: requred FPU, use --enable-fpu"));
#endif
}

/* D9 /5 */
void BX_CPU_C::FLDCW(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i, CHECK_PENDING_EXCEPTIONS, !UPDATE_LAST_OPCODE);
  Bit16u cwd;
  read_virtual_word(i->seg(), RMAddr(i), &cwd);
  FPU_CONTROL_WORD = cwd;
#else
  BX_INFO(("FLDCW: required FPU, configure --enable-fpu"));
#endif
}

/* D9 /7 */
void BX_CPU_C::FNSTCW(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i, !CHECK_PENDING_EXCEPTIONS, !UPDATE_LAST_OPCODE);
  Bit16u cwd = BX_CPU_THIS_PTR the_i387.get_control_word();
  write_virtual_word(i->seg(), RMAddr(i), &cwd);
#else
  BX_INFO(("FNSTCW: required FPU, configure --enable-fpu"));
#endif
}

/* DD /7 */
void BX_CPU_C::FNSTSW(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i, !CHECK_PENDING_EXCEPTIONS, !UPDATE_LAST_OPCODE);
  Bit16u swd = BX_CPU_THIS_PTR the_i387.get_status_word();
  write_virtual_word(i->seg(), RMAddr(i), &swd);
#else
  BX_INFO(("FNSTSW: required FPU, configure --enable-fpu"));
#endif
}

/* DF E0 */
void BX_CPU_C::FNSTSW_AX(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i, !CHECK_PENDING_EXCEPTIONS, !UPDATE_LAST_OPCODE);
  AX = BX_CPU_THIS_PTR the_i387.get_status_word();
#else
  BX_INFO(("FNSTSW_AX: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FNSAVE(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i, !CHECK_PENDING_EXCEPTIONS, !UPDATE_LAST_OPCODE);

  fpu_execute(i);
//#else
  BX_INFO(("FNSAVE: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FRSTOR(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i, CHECK_PENDING_EXCEPTIONS, !UPDATE_LAST_OPCODE);

  fpu_execute(i);
//#else
  BX_INFO(("FRSTOR: required FPU, configure --enable-fpu"));
#endif
}

/* 9B E2 */
void BX_CPU_C::FNCLEX(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i, !CHECK_PENDING_EXCEPTIONS, !UPDATE_LAST_OPCODE);

  FPU_PARTIAL_STATUS &= ~(FPU_SW_Backward|FPU_SW_Summary|FPU_SW_Stack_Fault|FPU_SW_Precision|
		   FPU_SW_Underflow|FPU_SW_Overflow|FPU_SW_Zero_Div|FPU_SW_Denormal_Op|
		   FPU_SW_Invalid);

  // do not update last fpu instruction pointer
#else
  BX_INFO(("FNCLEX: required FPU, configure --enable-fpu"));
#endif
}

/* DB E3 */
void BX_CPU_C::FNINIT(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i, !CHECK_PENDING_EXCEPTIONS, !UPDATE_LAST_OPCODE);
  BX_CPU_THIS_PTR the_i387.init();
#else
  BX_INFO(("FNINIT: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FLDENV(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i, CHECK_PENDING_EXCEPTIONS, !UPDATE_LAST_OPCODE);

/*
On  the Intel486 processor, when a segment not present exception (#NP)
occurs in the middle of an FLDENV instruction, it can happen that part
of  the  environment  is  loaded  and part not. In such cases, the FPU
control  word is left with a value of 007FH. The P6 family and Pentium
processors  ensure  the  internal  state  is  correct  at all times by
attempting  to read the first and last bytes of the environment before
updating the internal state.
*/

  fpu_execute(i);
//#else
  BX_INFO(("FLDENV: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FNSTENV(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i, !CHECK_PENDING_EXCEPTIONS, !UPDATE_LAST_OPCODE);

  fpu_execute(i);
//#else
  BX_INFO(("FNSTENV: required FPU, configure --enable-fpu"));
#endif
}

/* D9 D0 */
void BX_CPU_C::FNOP(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i, CHECK_PENDING_EXCEPTIONS, !UPDATE_LAST_OPCODE);

  // Perform no FPU operation. This instruction takes up space in the
  // instruction stream but does not affect the FPU or machine
  // context, except the EIP register.
#else
  BX_INFO(("FNOP: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FPLEGACY(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i, !CHECK_PENDING_EXCEPTIONS, !UPDATE_LAST_OPCODE);

  // FPU performs no specific operation and no internak x87 states
  // are affected
#else
  BX_INFO(("legacy FPU opcodes: required FPU, configure --enable-fpu"));
#endif
}
