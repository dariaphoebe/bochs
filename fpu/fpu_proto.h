/*
 * Copyright (C) 2001  MandrakeSoft S.A.
 *
 *   MandrakeSoft S.A.
 *   43, rue d'Aboukir
 *   75002 Paris - France
 *   http: //www.linux-mandrake.com/
 *   http: //www.mandrakesoft.com/
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

/*
 * bochs.h is the master header file for all C++ code.  It includes all 
 * the system header files needed by bochs, and also includes all the bochs
 * C++ header files.  Because bochs.h and the files that it includes has 
 * structure and class definitions, it cannot be called from C code.
 */

#ifndef _FPU_PROTO_H
#define _FPU_PROTO_H

#include "linkage.h"

/* errors.c */
asmlinkage void FPU_exception(int n);
asmlinkage void FPU_internal(int n);
extern int real_1op_NaN(FPU_REG *a);
extern int real_2op_NaN(FPU_REG const *b, u_char tagb, int deststnr,
			FPU_REG const *defaultNaN);
extern int arith_invalid(int deststnr);
extern int FPU_divide_by_zero(int deststnr, u_char sign);
extern int set_precision_flag(int flags);
extern void set_precision_flag_up(void);
extern void set_precision_flag_down(void);
extern int denormal_operand(void);
extern int arith_overflow(FPU_REG *dest);
extern int arith_round_overflow(FPU_REG *dest, Bit8u sign);
extern int arith_underflow(FPU_REG *dest);
extern void FPU_stack_overflow(void);
extern void FPU_stack_underflow(void);
extern void FPU_stack_underflow_i(int i);
extern void FPU_stack_underflow_pop(int i);
/* fpu_tags.c */
extern int FPU_tagof(FPU_REG *ptr) BX_CPP_AttrRegparmN(1);
extern int FPU_gettag0(void);
extern int FPU_gettagi(int stnr) BX_CPP_AttrRegparmN(1);
extern int FPU_gettag(int regnr) BX_CPP_AttrRegparmN(1);
extern void FPU_settag0(int tag) BX_CPP_AttrRegparmN(1);
extern void FPU_settagi(int stnr, int tag) BX_CPP_AttrRegparmN(2);
extern void FPU_settag(int regnr, int tag) BX_CPP_AttrRegparmN(2);
extern int FPU_Special(FPU_REG const *ptr) BX_CPP_AttrRegparmN(1);
extern int isNaN(FPU_REG const *ptr) BX_CPP_AttrRegparmN(1);
extern void FPU_pop(void);
extern int FPU_empty_i(int stnr) BX_CPP_AttrRegparmN(1);
extern int FPU_stackoverflow(FPU_REG **st_new_ptr);
extern void FPU_copy_to_regi(FPU_REG const *r, u_char tag, int stnr) BX_CPP_AttrRegparmN(3);
extern void FPU_copy_to_reg1(FPU_REG const *r, u_char tag) BX_CPP_AttrRegparmN(2);
extern void FPU_copy_to_reg0(FPU_REG const *r, u_char tag) BX_CPP_AttrRegparmN(2);
/* poly_2xm1.c */
extern int poly_2xm1(u_char sign, FPU_REG *arg, FPU_REG *result);
/* poly_atan.c */
extern void poly_atan(FPU_REG *st0_ptr, u_char st0_tag, FPU_REG *st1_ptr,
		      u_char st1_tag);
/* poly_l2.c */
extern void poly_l2(FPU_REG *st0_ptr, FPU_REG *st1_ptr, u_char st1_sign);
extern int poly_l2p1(u_char s0, u_char s1, FPU_REG *r0, FPU_REG *r1,
		     FPU_REG *d);
/* poly_sin.c */
extern void poly_sine(FPU_REG *st0_ptr);
extern void poly_cos(FPU_REG *st0_ptr);
/* poly_tan.c */
extern void poly_tan(FPU_REG *st0_ptr, int flag);
/* reg_add_sub.c */
extern int FPU_add(FPU_REG const *b, u_char tagb, int destrnr, u16 control_w);
extern int FPU_sub(int flags, FPU_REG *rm, u16 control_w);
/* reg_mul.c */
extern int FPU_mul(FPU_REG const *b, u_char tagb, int deststnr, int control_w);
/* reg_convert.c */
extern int FPU_to_exp16(FPU_REG const *a, FPU_REG *x) BX_CPP_AttrRegparmN(2);

#endif /* _FPU_PROTO_H */
