/*---------------------------------------------------------------------------+
 |  fpu_tags.c                                                               |
 |  $Id: fpu_tags.c,v 1.7.8.4 2004/06/09 19:38:31 sshwarts Exp $
 |                                                                           |
 |  Set FPU register tags.                                                   |
 |                                                                           |
 | Copyright (C) 1997                                                        |
 |                  W. Metzenthen, 22 Parker St, Ormond, Vic 3163, Australia |
 |                  E-mail   billm@jacobi.maths.monash.edu.au                |
 |                                                                           |
 |                                                                           |
 +---------------------------------------------------------------------------*/

/* Get data sizes from config.h generated from simulator's
 * configure script
 */
#include "config.h"

/* -----------------------------------------------------------
 * Slimmed down version used to compile against a CPU simulator
 * rather than a kernel (ported by Kevin Lawton)
 * ------------------------------------------------------------ */

#include <cpu/i387.h>

#include "tag_w.h"

#define exponent16(x)  ((x)->exp)

int BX_CPP_AttrRegparmN(1) FPU_tagof(FPU_REG *reg)
{
  int exp = exponent16(reg) & 0x7fff;
  if (exp == 0)
    {
      if (!(reg->sigh | reg->sigl))
	{
	  return FPU_Tag_Zero;
	}
      /* The number is a de-normal or pseudodenormal. */
      return FPU_Tag_Special;
    }

  if (exp == 0x7fff)
    {
      /* Is an Infinity, a NaN, or an unsupported data type. */
      return FPU_Tag_Special;
    }

  if (!(reg->sigh & 0x80000000))
    {
      /* Unsupported data type. */
      /* Valid numbers have the ms bit set to 1. */
      /* Unnormal. */
      return FPU_Tag_Special;
    }

  return FPU_Tag_Valid;
}
