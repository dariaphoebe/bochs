/*---------------------------------------------------------------------------+
 |  reg_constant.c                                                           |
 |  $Id: reg_constant.c,v 1.7.4.3 2004/05/23 18:46:19 sshwarts Exp $
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

/* Only the sign (and tag) is used in internal zeroes */
FPU_REG const CONST_Z    = MAKE_REG(POS, EXP_UNDER, 0x0, 0x0);

/* Only the sign (and tag) is used in internal infinities */
FPU_REG const CONST_INF  = MAKE_REG(POS, EXP_OVER, 0x00000000, 0x80000000);
