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

void BX_CPU_C::FCOM_STi(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i);

  fpu_execute(i);
//#else
  BX_INFO(("FCOM_STi: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FCOMP_STi(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i);

  fpu_execute(i);
//#else
  BX_INFO(("FCOMP_STi: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FCOMI_ST0_STj(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i);
  BX_PANIC(("FCOMI_ST0_STj: instruction still not implemented"));
#else
  BX_INFO(("FCOMI_ST0_STj: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FCOMIP_ST0_STj(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i);
  BX_PANIC(("FCOMIP_ST0_STj: instruction still not implemented"));
#else
  BX_INFO(("FCOMIP_ST0_STj: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FUCOMI_ST0_STj(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i);
  BX_PANIC(("FUCOMI_ST0_STj: instruction still not implemented"));
#else
  BX_INFO(("FUCOMI_ST0_STj: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FUCOMIP_ST0_STj(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i);
  BX_PANIC(("FUCOMIP_ST0_STj: instruction still not implemented"));
#else
  BX_INFO(("FUCOMIP_ST0_STj: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FUCOM_STi(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i);

  fpu_execute(i);
//#else
  BX_INFO(("FUCOM_STi: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FUCOMP_STi(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i);

  fpu_execute(i);
//#else
  BX_INFO(("FUCOMP_STi: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FCOM_SINGLE_REAL(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i);

  fpu_execute(i);
//#else
  BX_INFO(("FCOM_SINGLE_REAL: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FCOMP_SINGLE_REAL(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i);

  fpu_execute(i);
//#else
  BX_INFO(("FCOMP_SINGLE_REAL: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FCOM_DOUBLE_REAL(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i);

  fpu_execute(i);
//#else
  BX_INFO(("FCOM_DOUBLE_REAL: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FCOMP_DOUBLE_REAL(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i);

  fpu_execute(i);
//#else
  BX_INFO(("FCOMP_DOUBLE_REAL: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FICOM_WORD_INTEGER(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i);

  fpu_execute(i);
//#else
  BX_INFO(("FICOM_WORD_INTEGER: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FICOMP_WORD_INTEGER(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i);

  fpu_execute(i);
//#else
  BX_INFO(("FICOMP_WORD_INTEGER: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FICOM_DWORD_INTEGER(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i);

  fpu_execute(i);
//#else
  BX_INFO(("FICOM_DWORD_INTEGER: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FICOMP_DWORD_INTEGER(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i);

  fpu_execute(i);
//#else
  BX_INFO(("FICOMP_DWORD_INTEGER: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FCOMPP(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i);

  fpu_execute(i);
//#else
  BX_INFO(("FCOMPP: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FUCOMPP(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i);

  fpu_execute(i);
//#else
  BX_INFO(("FUCOMPP: required FPU, configure --enable-fpu"));
#endif
}

/* DA C0 */
void BX_CPU_C::FCMOVB_ST0_STj(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU && (BX_CPU_LEVEL == 6)
  BX_CPU_THIS_PTR prepareFPU(i);

  int st0_tag = BX_CPU_THIS_PTR the_i387.FPU_gettagi(0);
  int sti_tag = BX_CPU_THIS_PTR the_i387.FPU_gettagi(i->rm());

  if (st0_tag == FPU_Tag_Empty || sti_tag == FPU_Tag_Empty)
  {
     FPU_stack_underflow(0);
     return;
  }

  floatx80 sti_reg = BX_READ_FPU_REG(i->rm());

  if (get_CF()) {
     BX_WRITE_FPU_REGISTER_AND_TAG(sti_reg, sti_tag, 0);
  }
#else
  BX_INFO(("FCMOVB_ST0_STj: required P6 FPU, configure --enable-fpu, cpu-level=6"));
  UndefinedOpcode(i);
#endif
}

/* DA C8 */
void BX_CPU_C::FCMOVE_ST0_STj(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU && (BX_CPU_LEVEL == 6)
  BX_CPU_THIS_PTR prepareFPU(i);

  int st0_tag = BX_CPU_THIS_PTR the_i387.FPU_gettagi(0);
  int sti_tag = BX_CPU_THIS_PTR the_i387.FPU_gettagi(i->rm());

  if (st0_tag == FPU_Tag_Empty || sti_tag == FPU_Tag_Empty)
  {
     FPU_stack_underflow(0);
     return;
  }

  floatx80 sti_reg = BX_READ_FPU_REG(i->rm());

  if (get_ZF()) {
     BX_WRITE_FPU_REGISTER_AND_TAG(sti_reg, sti_tag, 0);
  }
#else
  BX_INFO(("FCMOVE_ST0_STj: required P6 FPU, configure --enable-fpu, cpu-level=6"));
  UndefinedOpcode(i);
#endif
}

/* DA D0 */
void BX_CPU_C::FCMOVBE_ST0_STj(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU && (BX_CPU_LEVEL == 6)
  BX_CPU_THIS_PTR prepareFPU(i);

  int st0_tag = BX_CPU_THIS_PTR the_i387.FPU_gettagi(0);
  int sti_tag = BX_CPU_THIS_PTR the_i387.FPU_gettagi(i->rm());

  if (st0_tag == FPU_Tag_Empty || sti_tag == FPU_Tag_Empty)
  {
     FPU_stack_underflow(0);
     return;
  }

  floatx80 sti_reg = BX_READ_FPU_REG(i->rm());

  if (get_CF() || get_ZF()) {
     BX_WRITE_FPU_REGISTER_AND_TAG(sti_reg, sti_tag, 0);
  }
#else
  BX_INFO(("FCMOVBE_ST0_STj: required P6 FPU, configure --enable-fpu, cpu-level=6"));
  UndefinedOpcode(i);
#endif
}

/* DA D8 */
void BX_CPU_C::FCMOVU_ST0_STj(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU && (BX_CPU_LEVEL == 6)
  BX_CPU_THIS_PTR prepareFPU(i);

  int st0_tag = BX_CPU_THIS_PTR the_i387.FPU_gettagi(0);
  int sti_tag = BX_CPU_THIS_PTR the_i387.FPU_gettagi(i->rm());

  if (st0_tag == FPU_Tag_Empty || sti_tag == FPU_Tag_Empty)
  {
     FPU_stack_underflow(0);
     return;
  }

  floatx80 sti_reg = BX_READ_FPU_REG(i->rm());

  if (get_PF()) {
     BX_WRITE_FPU_REGISTER_AND_TAG(sti_reg, sti_tag, 0);
  }
#else
  BX_INFO(("FCMOVU_ST0_STj: required P6 FPU, configure --enable-fpu, cpu-level=6"));
  UndefinedOpcode(i);
#endif
}

/* DB C0 */
void BX_CPU_C::FCMOVNB_ST0_STj(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU && (BX_CPU_LEVEL == 6)
  BX_CPU_THIS_PTR prepareFPU(i);

  int st0_tag = BX_CPU_THIS_PTR the_i387.FPU_gettagi(0);
  int sti_tag = BX_CPU_THIS_PTR the_i387.FPU_gettagi(i->rm());

  if (st0_tag == FPU_Tag_Empty || sti_tag == FPU_Tag_Empty)
  {
     FPU_stack_underflow(0);
     return;
  }

  floatx80 sti_reg = BX_READ_FPU_REG(i->rm());

  if (! get_CF()) {
     BX_WRITE_FPU_REGISTER_AND_TAG(sti_reg, sti_tag, 0);
  }
#else
  BX_INFO(("FCMOVNB_ST0_STj: required P6 FPU, configure --enable-fpu, cpu-level=6"));
  UndefinedOpcode(i);
#endif
}

/* DB C8 */
void BX_CPU_C::FCMOVNE_ST0_STj(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU && (BX_CPU_LEVEL == 6)
  BX_CPU_THIS_PTR prepareFPU(i);

  int st0_tag = BX_CPU_THIS_PTR the_i387.FPU_gettagi(0);
  int sti_tag = BX_CPU_THIS_PTR the_i387.FPU_gettagi(i->rm());

  if (st0_tag == FPU_Tag_Empty || sti_tag == FPU_Tag_Empty)
  {
     FPU_stack_underflow(0);
     return;
  }

  floatx80 sti_reg = BX_READ_FPU_REG(i->rm());

  if (! get_ZF()) {
     BX_WRITE_FPU_REGISTER_AND_TAG(sti_reg, sti_tag, 0);
  }
#else
  BX_INFO(("FCMOVNE_ST0_STj: required P6 FPU, configure --enable-fpu, cpu-level=6"));
  UndefinedOpcode(i);
#endif
}

/* DB D0 */
void BX_CPU_C::FCMOVNBE_ST0_STj(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU && (BX_CPU_LEVEL == 6)
  BX_CPU_THIS_PTR prepareFPU(i);

  int st0_tag = BX_CPU_THIS_PTR the_i387.FPU_gettagi(0);
  int sti_tag = BX_CPU_THIS_PTR the_i387.FPU_gettagi(i->rm());

  if (st0_tag == FPU_Tag_Empty || sti_tag == FPU_Tag_Empty)
  {
     FPU_stack_underflow(0);
     return;
  }

  floatx80 sti_reg = BX_READ_FPU_REG(i->rm());

  if ((!get_CF()) && (!get_ZF())) {
     BX_WRITE_FPU_REGISTER_AND_TAG(sti_reg, sti_tag, 0);
  }
#else
  BX_INFO(("FCMOVNBE_ST0_STj: required P6 FPU, configure --enable-fpu, cpu-level=6"));
  UndefinedOpcode(i);
#endif
}

/* DB D8 */
void BX_CPU_C::FCMOVNU_ST0_STj(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU && (BX_CPU_LEVEL == 6)
  BX_CPU_THIS_PTR prepareFPU(i);

  int st0_tag = BX_CPU_THIS_PTR the_i387.FPU_gettagi(0);
  int sti_tag = BX_CPU_THIS_PTR the_i387.FPU_gettagi(i->rm());

  if (st0_tag == FPU_Tag_Empty || sti_tag == FPU_Tag_Empty)
  {
     FPU_stack_underflow(0);
     return;
  }

  floatx80 sti_reg = BX_READ_FPU_REG(i->rm());

  if (! get_PF()) {
     BX_WRITE_FPU_REGISTER_AND_TAG(sti_reg, sti_tag, 0);
  }
#else
  BX_INFO(("FCMOVNU_ST0_STj: required P6 FPU, configure --enable-fpu, cpu-level=6"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::FTST(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i);

  fpu_execute(i);
//#else
  BX_INFO(("FTST: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FXAM(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU(i);

  fpu_execute(i);
//#else
  BX_INFO(("FXAM: required FPU, configure --enable-fpu"));
#endif
}
