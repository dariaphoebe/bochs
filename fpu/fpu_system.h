/*---------------------------------------------------------------------------+
 |  fpu_system.h                                                             |
 |  $Id: fpu_system.h,v 1.21.8.1 2004/04/09 12:29:49 sshwarts Exp $
 |                                                                           |
 | Copyright (C) 1992,1994,1997                                              |
 |                       W. Metzenthen, 22 Parker St, Ormond, Vic 3163,      |
 |                       Australia.  E-mail   billm@suburbia.net             |
 |                                                                           |
 +---------------------------------------------------------------------------*/

#ifndef _FPU_SYSTEM_H
#define _FPU_SYSTEM_H

#include <stdio.h>

/* Get data sizes from config.h generated from simulator's
 * configure script
 */
#include "config.h"
typedef Bit8u  u8; /* for FPU only */
typedef Bit8s  s8;
typedef Bit16u u16;
typedef Bit16s s16;
typedef Bit32u u32;
typedef Bit32s s32;
typedef Bit64u u64;
typedef Bit64s s64;

#ifndef __APPLE__
typedef Bit8u u_char;
#endif

/* -----------------------------------------------------------
 * Slimmed down version used to compile against a CPU simulator
 * rather than a kernel (ported by Kevin Lawton)
 * ------------------------------------------------------------ */

#include <cpu/i387.h>

#ifndef WORDS_BIGENDIAN
#error "WORDS_BIGENDIAN not defined in config.h"
#elif WORDS_BIGENDIAN == 1
#define EMU_BIG_ENDIAN 1
#else
/* Nothing needed.  Lack of defining EMU_BIG_ENDIAN means
 * little endian.
 */
#endif

extern struct i387_t *current_i387;

#define i387     (*current_i387)

#define FPU_partial_status      (i387.swd)
#define FPU_control_word        (i387.cwd)
#define FPU_tag_word            (i387.twd)
#define FPU_registers           (i387.st_space)
#define FPU_tos                 (i387.tos)

#define FPU_register_base       ((u_char *) FPU_registers)

/*
 * Change a pointer to an int, with type conversions that make it legal.
 * First make it a void pointer, then convert to an integer of the same
 * size as the pointer.  Otherwise, on machines with 64-bit pointers, 
 * compilers complain when you typecast a 64-bit pointer into a 32-bit integer.
 */
#define PTR2INT(x)   ((bx_ptr_equiv_t)(void *)(x))

#endif
