/*---------------------------------------------------------------------------+
 |  fpu_emu.h                                                                |
 |  $Id: fpu_emu.h,v 1.23.6.6 2004/03/28 20:26:53 sshwarts Exp $
 |                                                                           |
 | Copyright (C) 1992,1993,1994,1997                                         |
 |                       W. Metzenthen, 22 Parker St, Ormond, Vic 3163,      |
 |                       Australia.  E-mail   billm@suburbia.net             |
 |                                                                           |
 +---------------------------------------------------------------------------*/


#ifndef _FPU_EMU_H_
#define _FPU_EMU_H_

#include "linkage.h"

/*
 * Define PECULIAR_486 to get a closer approximation to 80486 behaviour,
 * rather than behaviour which appears to be cleaner.
 * This is a matter of opinion: for all I know, the 80486 may simply
 * be complying with the IEEE spec. Maybe one day I'll get to see the
 * spec...
 */
#define PECULIAR_486

#define EXP_BIAS	(0)
#define EXP_OVER	(0x4000)    /* smallest invalid large exponent */
#define	EXP_UNDER	(-0x3fff)   /* largest invalid small exponent */
#define EXP_WAY_UNDER   (-0x6000)   /* Below the smallest denormal, but still a 16 bit nr */

#define EXTENDED_Ebias (0x3fff)

#define SIGN_POS	(0)
#define SIGN_NEG	(0x80)

#define SIGN_Positive	(0)
#define SIGN_Negative	(0x8000)

/* Keep the order TAG_Valid, TAG_Zero, TW_Denormal */
/* The following fold to 2 (Special) in the Tag Word */
#define TW_Denormal      4      /* De-normal */
#define TW_Infinity	 5 	/* + or - infinity */
#define	TW_NaN		 6      /* Not a Number */
#define	TW_Unsupported	 7 	/* Not supported by an 80486 */

#define TAG_Valid	 0 	/* valid */
#define TAG_Zero	 1 	/* zero */
#define TAG_Special	 2 	/* De-normal, + or - infinity, or NaN */
#define TAG_Empty	 3 	/* empty */

/* Special st() number to identify loaded data (not on stack). */
#define LOADED_DATA	(10101)	

/* A few flags (must be >= 0x10). */
#define REV             0x10
#define LOADED          0x40

#define FPU_Exception   (0x80000000)   /* Added to tag returns. */

#include "fpu_system.h"

#define	st(x)      ( *((FPU_REG *)(FPU_register_base + sizeof(FPU_REG) * ((FPU_tos+x) & 7) )))

/* FPU_push() does not affect the tags */
#define FPU_push()	{ FPU_tos--; }

/* register accessors */
#ifdef EMU_BIG_ENDIAN

#define MAKE_REG(s,e,l,h) { 0,0,0, \
		((EXTENDED_Ebias+(e)) | ((SIGN_##s != 0)*0x8000)) , h, l}

#define signbyte(x) (((u_char *)(x))[6])
#define significand(x) (((u64 *)&((x)->sigh))[0])

#else

#define MAKE_REG(s,e,l,h) { l, h, \
		((EXTENDED_Ebias+(e)) | ((SIGN_##s != 0)*0x8000)), 0,0,0 }

#define signbyte(x) (((u_char *)(x))[9])
#define significand(x) (((u64 *)&((x)->sigl))[0])

#endif

#define getsign(a) (signbyte(a) & 0x80)
#define setsign(a,b) { if (b) signbyte(a) |= 0x80; else signbyte(a) &= 0x7f; }
#define copysign(a,b) { if (getsign(a)) signbyte(b) |= 0x80; \
                        else signbyte(b) &= 0x7f; }
#define changesign(a) { signbyte(a) ^= 0x80; }
#define setpositive(a) { signbyte(a) &= 0x7f; }
#define setnegative(a) { signbyte(a) |= 0x80; }
#define signpositive(a) ( (signbyte(a) & 0x80) == 0 )
#define signnegative(a) (signbyte(a) & 0x80)

BX_C_INLINE void reg_copy(FPU_REG const *x, FPU_REG *y)
{
  y->exp = x->exp;
  significand(y) = significand(x);
}

#define exponent(x)  (((x)->exp & 0x7fff) - EXTENDED_Ebias)
#define setexponentpos(x,y) { (x)->exp = ((y) + EXTENDED_Ebias) & 0x7fff; }
#define exponent16(x)         (x)->exp
#define setexponent16(x,y)  { (x)->exp = (y); }
#define addexponent(x,y)    { (x)->exp += (y); }
#define stdexp(x)           { (x)->exp += EXTENDED_Ebias; }

#define isdenormal(ptr)   (exponent(ptr) == EXP_BIAS+EXP_UNDER)

/*----- Prototypes for functions written in assembler -----*/
asmlinkage int FPU_normalize_nuo(FPU_REG *x, int bias);
asmlinkage int FPU_u_sub(FPU_REG const *arg1, FPU_REG const *arg2,
			 FPU_REG *answ, u16 control_w, u_char sign,
			 s32 expa, s32 expb);
asmlinkage int FPU_u_div(FPU_REG const *arg1, FPU_REG const *arg2,
			 FPU_REG *answ, u16 control_w, u_char sign);
asmlinkage int FPU_u_mul(FPU_REG const *arg1, FPU_REG const *arg2,
			 FPU_REG *answ, u16 control_w, u_char sign,
			 s32 expon);
asmlinkage int FPU_u_add(FPU_REG const *arg1, FPU_REG const *arg2,
			 FPU_REG *answ, u16 control_w, u_char sign,
			 s32 expa, s32 expb);
asmlinkage u32 FPU_shrx(void *l, u32 x);
asmlinkage u32 FPU_shrxs(void *v, u32 x);
asmlinkage int FPU_round(FPU_REG *arg, u32 extent,
			 u16 control_w, u_char sign);

#ifndef MAKING_PROTO
#include "fpu_proto.h"
#endif

#endif /* !defined _FPU_EMU_H_ */
