//  Copyright (C) 2001  MandrakeSoft S.A.
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


//
// This is the glue logic needed to connect the wm-FPU-emu
// FPU emulator written by Bill Metzenthen to bochs.
//


#include "bochs.h"
#include <math.h>

#if !BX_WITH_MACOS
extern "C" {
#endif

#include "fpu_emu.h"
#include "linux/signal.h"

#if !BX_WITH_MACOS
}
#endif

#define LOG_THIS genlog->
#if BX_USE_CPU_SMF
#define this (BX_CPU(0))
#endif

i387_t *current_i387;

void FPU_initalize_i387(struct i387_t *the_i387)
{
  current_i387 = the_i387;
}

void BX_CPU_C::print_state_FPU()
{
  static double sigh_scale_factor = pow(2.0, -31.0);
  static double sigl_scale_factor = pow(2.0, -63.0);

  Bit32u reg;
  reg = i387.cwd;
  fprintf(stderr, "cwd            0x%04x\n", reg);
  reg = i387.swd;
  fprintf(stderr, "swd            0x%04x\n", reg);
  reg = i387.twd;
  fprintf(stderr, "twd            0x%04x\n", reg);
  reg = i387.foo;
  fprintf(stderr, "foo            0x%04x\n", reg);
  reg = i387.fip & 0xffffffff;
  fprintf(stderr, "fip            0x%08x\n", reg);
  reg = i387.fcs;
  fprintf(stderr, "fcs            0x%04x\n", reg);
  reg = i387.fdp & 0xffffffff;
  fprintf(stderr, "fip            0x%08x\n", reg);
  reg = i387.fds;
  fprintf(stderr, "fcs            0x%04x\n", reg);

  // print stack too
  for (int i=0; i<8; i++) {
    FPU_REG *fpr = &st(i);
    double f1 = pow(2.0, ((0x7fff&fpr->exp) - EXTENDED_Ebias));
    if (fpr->exp & SIGN_Negative) f1 = -f1;
    double f2 = ((double)fpr->sigh * sigh_scale_factor);
    double f3 = ((double)fpr->sigl * sigl_scale_factor);
    double f = f1*(f2+f3);
    fprintf(stderr, "st%d            %.10f (raw 0x%04x%08x%08x)\n", i, f, 0xffff&fpr->exp, fpr->sigh, fpr->sigl);
  }
}

extern "C" int printk(const char * fmt, ...)
{
  BX_INFO(("math abort: %s", fmt));
  return 0;
}
