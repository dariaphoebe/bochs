/*---------------------------------------------------------------------------+
 |  fpu_tags.c                                                               |
 |  $Id: fpu_tags.cc,v 1.1.2.1 2004/06/11 06:26:29 sshwarts Exp $
 |                                                                           |
 |  Set FPU register tags.                                                   |
 |                                                                           |
 | Copyright (C) 1997                                                        |
 |                  W. Metzenthen, 22 Parker St, Ormond, Vic 3163, Australia |
 |                  E-mail   billm@jacobi.maths.monash.edu.au                |
 |                                                                           |
 |                                                                           |
 +---------------------------------------------------------------------------*/

#include "softfloat.h"
#include "softfloat-specialize.h"

/* -----------------------------------------------------------
 * Slimmed down version used to compile against a CPU simulator
 * rather than a kernel (ported by Kevin Lawton)
 * ------------------------------------------------------------ */

#include <cpu/i387.h>

int FPU_tagof(const floatx80 &reg)
{
   Bit32s exp = floatx80_exp(reg);
   if (exp == 0)
   {
      if (! floatx80_fraction(reg))
	  return FPU_Tag_Zero;

      /* The number is a de-normal or pseudodenormal. */
      return FPU_Tag_Special;
   }

   if (exp == 0x7fff)
   {
      /* Is an Infinity, a NaN, or an unsupported data type. */
      return FPU_Tag_Special;
   }

   if (!(reg.fraction & BX_CONST64(0x8000000000000000)))
   {
      /* Unsupported data type. */
      /* Valid numbers have the ms bit set to 1. */
      return FPU_Tag_Special;
   }

   return FPU_Tag_Valid;
}
