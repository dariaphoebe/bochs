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

#if BX_SUPPORT_FPU
void BX_CPU_C::prepareFPU(void)
{
  if (BX_CPU_THIS_PTR cr0.em || BX_CPU_THIS_PTR cr0.ts)
    exception(BX_NM_EXCEPTION, 0, 0);
}
#endif

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

void BX_CPU_C::FNSTENV(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FNSTENV: required FPU, configure --enable-fpu"));
#endif
}

/* D9 /7 */
void BX_CPU_C::FNSTCW(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();
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
  BX_CPU_THIS_PTR prepareFPU();
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
  BX_CPU_THIS_PTR prepareFPU();
  AX = BX_CPU_THIS_PTR the_i387.get_status_word();
#else
  BX_INFO(("FNSTSW_AX: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FNSAVE(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FNSAVE: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FNCLEX(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FNCLEX: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FNINIT(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FNINIT: required FPU, configure --enable-fpu"));
#endif
}

/* DD C0 */
void BX_CPU_C::FFREE_STi(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();
  BX_CPU_THIS_PTR FPU_check_pending_exceptions();
  BX_CPU_THIS_PTR the_i387.FPU_settagi(FPU_Tag_Empty, i->rm());
#else
  BX_INFO(("FFREE_STi: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FLDENV(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FLDENV: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FLDCW(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FLDCW: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FRSTOR(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FRSTOR: required FPU, configure --enable-fpu"));
#endif
}
