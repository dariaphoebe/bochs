/*---------------------------------------------------------------------------+
 |  reg_constant.c                                                           |
 |  $Id: reg_constant.c,v 1.7.2.4 2004/03/27 20:09:53 sshwarts Exp $
 |                                                                           |
 | All of the constant FPU_REGs                                              |
 |                                                                           |
 | Copyright (C) 1992,1993,1994,1997                                         |
 |                     W. Metzenthen, 22 Parker St, Ormond, Vic 3163,        |
 |                     Australia.  E-mail   billm@suburbia.net               |
 |                                                                           |
 |                                                                           |
 +---------------------------------------------------------------------------*/

#include "fpu_system.h"
#include "fpu_emu.h"
#include "status_w.h"
#include "reg_constant.h"
#include "control_w.h"

FPU_REG const CONST_1    = MAKE_REG(POS, 0, 0x00000000, 0x80000000);
FPU_REG const CONST_PI   = MAKE_REG(POS, 1, 0x2168c235, 0xc90fdaa2);
/* bbd: make CONST_PI2 non-const so that you can write "&CONST_PI2" when
   calling a function.  Otherwise you get const warnings.  Surely there's
   a better way. */
FPU_REG CONST_PI2  = MAKE_REG(POS, 0, 0x2168c235, 0xc90fdaa2);
FPU_REG const CONST_PI4  = MAKE_REG(POS, -1, 0x2168c235, 0xc90fdaa2);

/* Only the sign (and tag) is used in internal zeroes */
FPU_REG const CONST_Z    = MAKE_REG(POS, EXP_UNDER, 0x0, 0x0);

/* This is the real indefinite QNaN */
FPU_REG const CONST_QNaN = MAKE_REG(NEG, EXP_OVER, 0x00000000, 0xC0000000);

/* Only the sign (and tag) is used in internal infinities */
FPU_REG const CONST_INF  = MAKE_REG(POS, EXP_OVER, 0x00000000, 0x80000000);
