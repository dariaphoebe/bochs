/*---------------------------------------------------------------------------+
 |  fpu_trig.c                                                               |
 |  $Id: fpu_trig.c,v 1.10.10.9 2004/06/05 14:50:55 sshwarts Exp $
 |                                                                           |
 | Implementation of the FPU "transcendental" functions.                     |
 |                                                                           |
 | Copyright (C) 1992,1993,1994,1997,1999                                    |
 |                       W. Metzenthen, 22 Parker St, Ormond, Vic 3163,      |
 |                       Australia.  E-mail   billm@melbpc.org.au            |
 |                                                                           |
 |                                                                           |
 +---------------------------------------------------------------------------*/

#include "fpu_system.h"
#include "fpu_emu.h"
#include "status_w.h"
#include "control_w.h"

/* bbd: make CONST_PI2 non-const so that you can write "&CONST_PI2" when
   calling a function.  Otherwise you get const warnings.  Surely there's
   a better way. */
static FPU_REG const CONST_PI   = MAKE_REG(POS,  1, 0x2168c235, 0xc90fdaa2);
static FPU_REG const CONST_PI2  = MAKE_REG(POS,  0, 0x2168c235, 0xc90fdaa2);
static FPU_REG const CONST_PI4  = MAKE_REG(POS, -1, 0x2168c235, 0xc90fdaa2);
static FPU_REG const CONST_3PI4 = MAKE_REG(POS,  1, 0x990e91a8, 0x96cbe3f9);

/*---------------------------------------------------------------------------*/
/* The following all require two arguments: st(0) and st(1) */

void fpatan(FPU_REG *st0_ptr, u_char st0_tag)
{
  FPU_REG *st1_ptr = &st(1);
  u_char st1_tag = FPU_gettagi(1);
  int tag;

  clear_C1();
  if (!((st0_tag ^ TAG_Valid) | (st1_tag ^ TAG_Valid)))
    {
    valid_atan:

      poly_atan(st0_ptr, st0_tag, st1_ptr, st1_tag);
      FPU_pop();
      return;
    }

  if (st0_tag == TAG_Special)
    st0_tag = FPU_Special(st0_ptr);
  if (st1_tag == TAG_Special)
    st1_tag = FPU_Special(st1_ptr);

  if (((st0_tag == TAG_Valid) && (st1_tag == TW_Denormal))
	    || ((st0_tag == TW_Denormal) && (st1_tag == TAG_Valid))
	    || ((st0_tag == TW_Denormal) && (st1_tag == TW_Denormal)))
    {
      if (denormal_operand() < 0)
	return;

      goto valid_atan;
    }
  else if ((st0_tag == TAG_Empty) || (st1_tag == TAG_Empty))
    {
      FPU_stack_underflow_pop(1);
      return;
    }
  else if ((st0_tag == TW_NaN) || (st1_tag == TW_NaN))
    {
      if (real_2op_NaN(st0_ptr, st0_tag, 1, st0_ptr) >= 0)
	  FPU_pop();
      return;
    }
  else if ((st0_tag == TW_Infinity) || (st1_tag == TW_Infinity))
    {
      u_char sign = getsign(st1_ptr);
      if (st0_tag == TW_Infinity)
	{
	  if (st1_tag == TW_Infinity)
	    {
	      if (signpositive(st0_ptr))
		{
		  FPU_copy_to_reg1(&CONST_PI4,  TAG_Valid);
		}
	      else
		{
		  FPU_copy_to_reg1(&CONST_3PI4, TAG_Valid);
		}
	    }
	  else
	    {
	      if ((st1_tag == TW_Denormal) && (denormal_operand() < 0))
		return;

	      if (signpositive(st0_ptr))
		{
		  FPU_copy_to_reg1(&CONST_Z, TAG_Zero);
		  setsign(st1_ptr, sign);   /* An 80486 preserves the sign */
		  FPU_pop();
		  return;
		}
	      else
		{
		  FPU_copy_to_reg1(&CONST_PI, TAG_Valid);
		}
	    }
	}
      else
	{
	  /* st(1) is infinity, st(0) not infinity */
	  if ((st0_tag == TW_Denormal) && (denormal_operand() < 0))
	    return;

	  FPU_copy_to_reg1(&CONST_PI2, TAG_Valid);
	}
      setsign(st1_ptr, sign);
    }
  else if (st1_tag == TAG_Zero)
    {
      /* st(0) must be valid or zero */
      u_char sign = getsign(st1_ptr);

      if ((st0_tag == TW_Denormal) && (denormal_operand() < 0))
	return;

      if (signpositive(st0_ptr))
	{
	  /* An 80486 preserves the sign */
	  FPU_pop();
	  return;
	}

      FPU_copy_to_reg1(&CONST_PI, TAG_Valid);
      setsign(st1_ptr, sign);
    }
  else if (st0_tag == TAG_Zero)
    {
      /* st(1) must be TAG_Valid here */
      u_char sign = getsign(st1_ptr);

      if ((st1_tag == TW_Denormal) && (denormal_operand() < 0))
	return;

      FPU_copy_to_reg1(&CONST_PI2, TAG_Valid);
      setsign(st1_ptr, sign);
    }

  FPU_pop();
  set_precision_flag_up();  /* We do not really know if up or down */
}
