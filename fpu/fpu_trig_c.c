/*---------------------------------------------------------------------------+
 |  fpu_trig.c                                                               |
 |  $Id: fpu_trig_c.c,v 1.1.2.3 2004/05/25 18:38:34 sshwarts Exp $
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
static FPU_REG const CONST_1    = MAKE_REG(POS,  0, 0x00000000, 0x80000000);
static FPU_REG const CONST_PI   = MAKE_REG(POS,  1, 0x2168c235, 0xc90fdaa2);
static FPU_REG const CONST_PI2  = MAKE_REG(POS,  0, 0x2168c235, 0xc90fdaa2);
static FPU_REG const CONST_PI4  = MAKE_REG(POS, -1, 0x2168c235, 0xc90fdaa2);
static FPU_REG const CONST_3PI4 = MAKE_REG(POS,  1, 0x990e91a8, 0x96cbe3f9);

static void single_arg_error(FPU_REG *st0_ptr, u_char st0_tag)
{
  if (st0_tag == TAG_Empty)
    FPU_stack_underflow();  /* Puts a QNaN in st(0) */
  else if (st0_tag == TW_NaN)
    real_1op_NaN(st0_ptr);       /* return with a NaN in st(0) */
#ifdef PARANOID
  else
    INTERNAL(0x0112);
#endif /* PARANOID */
}

void f2xm1(FPU_REG *st0_ptr, u_char tag)
{
  FPU_REG a;

  clear_C1();

  if (tag == TAG_Valid)
    {
      /* For an 80486 FPU, the result is undefined if the arg is >= 1.0 */
      if (exponent(st0_ptr) < 0)
	{
	denormal_arg:

	  FPU_to_exp16(st0_ptr, &a);

	  /* poly_2xm1(x) requires 0 < st(0) < 1. */
	  poly_2xm1(getsign(st0_ptr), &a, st0_ptr);
	}
      set_precision_flag_up();   /* 80486 appears to always do this */
      return;
    }

  if (tag == TAG_Zero)
    return;

  if (tag == TAG_Special)
    tag = FPU_Special(st0_ptr);

  switch (tag)
    {
    case TW_Denormal:
      if (denormal_operand() < 0)
	return;
      goto denormal_arg;
    case TW_Infinity:
      if (signnegative(st0_ptr))
	{
	  /* -infinity gives -1 (p16-10) */
	  FPU_copy_to_reg0(&CONST_1, TAG_Valid);
	  setnegative(st0_ptr);
	}
      return;
    default:
      single_arg_error(st0_ptr, tag);
    }
}

/*---------------------------------------------------------------------------*/
/* The following all require two arguments: st(0) and st(1) */

/* ST(1) <- ST(1) * log ST;  pop ST */
void fyl2x(FPU_REG *st0_ptr, u_char st0_tag)
{
  FPU_REG *st1_ptr = &st(1), exponent;
  u_char st1_tag = FPU_gettagi(1);
  u_char sign;
  int e, tag;

  clear_C1();

  if ((st0_tag == TAG_Valid) && (st1_tag == TAG_Valid))
    {
    both_valid:
      /* Both regs are Valid or Denormal */
      if (signpositive(st0_ptr))
	{
	  if (st0_tag == TW_Denormal)
	    FPU_to_exp16(st0_ptr, st0_ptr);
	  else
	    /* Convert st(0) for internal use. */
	    setexponent16(st0_ptr, exponent(st0_ptr));

	  if ((st0_ptr->sigh == 0x80000000) && (st0_ptr->sigl == 0))
	    {
	      /* Special case. The result can be precise. */
	      u_char esign;
	      e = exponent16(st0_ptr);
	      if (e >= 0)
		{
		  exponent.sigh = e;
		  esign = SIGN_POS;
		}
	      else
		{
		  exponent.sigh = -e;
		  esign = SIGN_NEG;
		}
	      exponent.sigl = 0;
	      setexponent16(&exponent, 31);
              tag = FPU_normalize_nuo(&exponent, 0);
	      stdexp(&exponent);
	      setsign(&exponent, esign);
	      tag = FPU_mul(&exponent, tag, 1, FULL_PRECISION);
	      if (tag >= 0)
		FPU_settagi(1, tag);
	    }
	  else
	    {
	      /* The usual case */
	      sign = getsign(st1_ptr);
	      if (st1_tag == TW_Denormal)
		FPU_to_exp16(st1_ptr, st1_ptr);
	      else
		/* Convert st(1) for internal use. */
		setexponent16(st1_ptr, exponent(st1_ptr));
	      poly_l2(st0_ptr, st1_ptr, sign);
	    }
	}
      else
	{
	  /* negative */
	  if (arith_invalid(1) < 0)
	    return;
	}

      FPU_pop();

      return;
    }

  if (st0_tag == TAG_Special)
    st0_tag = FPU_Special(st0_ptr);
  if (st1_tag == TAG_Special)
    st1_tag = FPU_Special(st1_ptr);

  if ((st0_tag == TAG_Empty) || (st1_tag == TAG_Empty))
    {
      FPU_stack_underflow_pop(1);
      return;
    }
  else if ((st0_tag <= TW_Denormal) && (st1_tag <= TW_Denormal))
    {
      if (st0_tag == TAG_Zero)
	{
	  if (st1_tag == TAG_Zero)
	    {
	      /* Both args zero is invalid */
	      if (arith_invalid(1) < 0)
		return;
	    }
	  else
	    {
	      u_char sign;
	      sign = getsign(st1_ptr)^SIGN_NEG;
	      if (FPU_divide_by_zero(1, sign) < 0)
		return;

	      setsign(st1_ptr, sign);
	    }
	}
      else if (st1_tag == TAG_Zero)
	{
	  /* st(1) contains zero, st(0) valid <> 0 */
	  /* Zero is the valid answer */
	  sign = getsign(st1_ptr);
	  
	  if (signnegative(st0_ptr))
	    {
	      /* log(negative) */
	      if (arith_invalid(1) < 0)
		return;
	    }
	  else if ((st0_tag == TW_Denormal) && (denormal_operand() < 0))
	    return;
	  else
	    {
	      if (exponent(st0_ptr) < 0)
		sign ^= SIGN_NEG;

	      FPU_copy_to_reg1(&CONST_Z, TAG_Zero);
	      setsign(st1_ptr, sign);
	    }
	}
      else
	{
	  /* One or both operands are denormals. */
	  if (denormal_operand() < 0)
	    return;
	  goto both_valid;
	}
    }
  else if ((st0_tag == TW_NaN) || (st1_tag == TW_NaN))
    {
      if (real_2op_NaN(st0_ptr, st0_tag, 1, st0_ptr) < 0)
	return;
    }
  /* One or both arg must be an infinity */
  else if (st0_tag == TW_Infinity)
    {
      if ((signnegative(st0_ptr)) || (st1_tag == TAG_Zero))
	{
	  /* log(-infinity) or 0*log(infinity) */
	  if (arith_invalid(1) < 0)
	    return;
	}
      else
	{
	  u_char sign = getsign(st1_ptr);

	  if ((st1_tag == TW_Denormal) && (denormal_operand() < 0))
	    return;

	  FPU_copy_to_reg1(&CONST_INF, TAG_Special);
	  setsign(st1_ptr, sign);
	}
    }
  /* st(1) must be infinity here */
  else if (((st0_tag == TAG_Valid) || (st0_tag == TW_Denormal))
	    && (signpositive(st0_ptr)))
    {
      if (exponent(st0_ptr) >= 0)
	{
	  if ((exponent(st0_ptr) == 0) &&
	      (st0_ptr->sigh == 0x80000000) &&
	      (st0_ptr->sigl == 0))
	    {
	      /* st(0) holds 1.0 */
	      /* infinity*log(1) */
	      if (arith_invalid(1) < 0)
		return;
	    }
	  /* else st(0) is positive and > 1.0 */
	}
      else
	{
	  /* st(0) is positive and < 1.0 */

	  if ((st0_tag == TW_Denormal) && (denormal_operand() < 0))
	    return;

	  changesign(st1_ptr);
	}
    }
  else
    {
      /* st(0) must be zero or negative */
      if (st0_tag == TAG_Zero)
	{
	  /* This should be invalid, but a real 80486 is happy with it. */

#ifndef PECULIAR_486
	  sign = getsign(st1_ptr);
	  if (FPU_divide_by_zero(1, sign) < 0)
	    return;
#endif /* PECULIAR_486 */

	  changesign(st1_ptr);
	}
      else if (arith_invalid(1) < 0)	  /* log(negative) */
	return;
    }

  FPU_pop();
}


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

void fyl2xp1(FPU_REG *st0_ptr, u_char st0_tag)
{
  u_char sign, sign1;
  FPU_REG *st1_ptr = &st(1), a, b;
  u_char st1_tag = FPU_gettagi(1);

  clear_C1();
  if (!((st0_tag ^ TAG_Valid) | (st1_tag ^ TAG_Valid)))
    {
    valid_yl2xp1:

      sign = getsign(st0_ptr);
      sign1 = getsign(st1_ptr);

      FPU_to_exp16(st0_ptr, &a);
      FPU_to_exp16(st1_ptr, &b);

      if (poly_l2p1(sign, sign1, &a, &b, st1_ptr))
	return;

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

      goto valid_yl2xp1;
    }
  else if ((st0_tag == TAG_Empty) | (st1_tag == TAG_Empty))
    {
      FPU_stack_underflow_pop(1);
      return;
    }
  else if (st0_tag == TAG_Zero)
    {
      switch (st1_tag)
	{
	case TW_Denormal:
	  if (denormal_operand() < 0)
	    return;

	case TAG_Zero:
	case TAG_Valid:
	  setsign(st0_ptr, getsign(st0_ptr) ^ getsign(st1_ptr));
	  FPU_copy_to_reg1(st0_ptr, st0_tag);
	  break;

	case TW_Infinity:
	  /* Infinity*log(1) */
	  if (arith_invalid(1) < 0)
	    return;
	  break;

	case TW_NaN:
	  if (real_2op_NaN(st0_ptr, st0_tag, 1, st0_ptr) < 0)
	    return;
	  break;

	default:
#ifdef PARANOID
	  INTERNAL(0x116);
	  return;
#endif /* PARANOID */
	}
    }
  else if ((st0_tag == TAG_Valid) || (st0_tag == TW_Denormal))
    {
      switch (st1_tag)
	{
	case TAG_Zero:
	  if (signnegative(st0_ptr))
	    {
	      if (exponent(st0_ptr) >= 0)
		{
		  /* st(0) holds <= -1.0 */
#ifdef PECULIAR_486   /* Stupid 80486 doesn't worry about log(negative). */
		  changesign(st1_ptr);
#else
		  if (arith_invalid(1) < 0)
		    return;
#endif /* PECULIAR_486 */
		}
	      else if ((st0_tag == TW_Denormal) && (denormal_operand() < 0))
		return;
	      else
		changesign(st1_ptr);
	    }
	  else if ((st0_tag == TW_Denormal) && (denormal_operand() < 0))
	    return;
	  break;

	case TW_Infinity:
	  if (signnegative(st0_ptr))
	    {
	      if ((exponent(st0_ptr) >= 0) &&
		  !((st0_ptr->sigh == 0x80000000) &&
		    (st0_ptr->sigl == 0)))
		{
		  /* st(0) holds < -1.0 */
#ifdef PECULIAR_486   /* Stupid 80486 doesn't worry about log(negative). */
		  changesign(st1_ptr);
#else
		  if (arith_invalid(1) < 0) return;
#endif /* PECULIAR_486 */
		}
	      else if ((st0_tag == TW_Denormal) && (denormal_operand() < 0))
		return;
	      else
		changesign(st1_ptr);
	    }
	  else if ((st0_tag == TW_Denormal) && (denormal_operand() < 0))
	    return;
	  break;

	case TW_NaN:
	  if (real_2op_NaN(st0_ptr, st0_tag, 1, st0_ptr) < 0)
	    return;
	}

    }
  else if (st0_tag == TW_NaN)
    {
      if (real_2op_NaN(st0_ptr, st0_tag, 1, st0_ptr) < 0)
	return;
    }
  else if (st0_tag == TW_Infinity)
    {
      if (st1_tag == TW_NaN)
	{
	  if (real_2op_NaN(st0_ptr, st0_tag, 1, st0_ptr) < 0)
	    return;
	}
      else if (signnegative(st0_ptr))
	{
#ifndef PECULIAR_486
	  /* This should have higher priority than denormals, but... */
	  if (arith_invalid(1) < 0)  /* log(-infinity) */
	    return;
#endif /* PECULIAR_486 */
	  if ((st1_tag == TW_Denormal) && (denormal_operand() < 0))
	    return;
#ifdef PECULIAR_486
	  /* Denormal operands actually get higher priority */
	  if (arith_invalid(1) < 0)  /* log(-infinity) */
	    return;
#endif /* PECULIAR_486 */
	}
      else if (st1_tag == TAG_Zero)
	{
	  /* log(infinity) */
	  if (arith_invalid(1) < 0)
	    return;
	}
	
      /* st(1) must be valid here. */

      else if ((st1_tag == TW_Denormal) && (denormal_operand() < 0))
	return;

      /* The Manual says that log(Infinity) is invalid, but a real
	 80486 sensibly says that it is o.k. */
      else
	{
	  u_char sign = getsign(st1_ptr);
	  FPU_copy_to_reg1(&CONST_INF, TAG_Special);
	  setsign(st1_ptr, sign);
	}
    }

  FPU_pop();
}
