/////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2003 Stanislav Shwartsman
//          Written by Stanislav Shwartsman <gate@fidonet.org.il>
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



#ifndef BX_I387_RELATED_EXTENSIONS_H
#define BX_I387_RELATED_EXTENSIONS_H

#if BX_SUPPORT_FPU

// Endian  Host byte order         Guest (x86) byte order
// ======================================================
// Little  FFFFFFFFEEAAAAAA        FFFFFFFFEEAAAAAA
// Big     AAAAAAEEFFFFFFFF        FFFFFFFFEEAAAAAA
//
// Legend: F - fraction/mmx
//         E - exponent
//         A - alignment

#ifdef BX_BIG_ENDIAN
#if defined(__MWERKS__) && defined(macintosh)
#pragma options align=mac68k
#endif
struct bx_fpu_reg_t {
  Bit16u alignment1, alignment2, alignment3;
  Bit16s exp;   /* Signed quantity used in internal arithmetic. */
  Bit32u sigh;
  Bit32u sigl;
} GCC_ATTRIBUTE((aligned(16), packed));
#if defined(__MWERKS__) && defined(macintosh)
#pragma options align=reset
#endif
#else
struct bx_fpu_reg_t {
  Bit32u sigl;
  Bit32u sigh;
  Bit16s exp;   /* Signed quantity used in internal arithmetic. */
  Bit16u alignment1, alignment2, alignment3;
} GCC_ATTRIBUTE((aligned(16), packed));
#endif

typedef struct bx_fpu_reg_t FPU_REG;

#define FPU_Tag_Valid   0x00
#define FPU_Tag_Zero    0x01
#define FPU_Tag_Special 0x02
#define FPU_Tag_Empty   0x03

#define BX_FPU_REG(index) \
    (BX_CPU_THIS_PTR the_i387.st_space[index*2])

#if defined(NEED_CPU_REG_SHORTCUTS)
#define FPU_PARTIAL_STATUS     (BX_CPU_THIS_PTR the_i387.swd)
#define FPU_CONTROL_WORD       (BX_CPU_THIS_PTR the_i387.cwd)
#define FPU_TAG_WORD           (BX_CPU_THIS_PTR the_i387.twd)
#define FPU_TOS                (BX_CPU_THIS_PTR the_i387.tos)
#endif

/* Special exceptions: */
#define FPU_EX_Stack_Overflow	(0x0041|SW_C1)	/* stack overflow */
#define FPU_EX_Stack_Underflow	(0x0041)	/* stack underflow */

/* Exception flags: */
#define FPU_EX_Precision	(0x0020)  /* loss of precision */
#define FPU_EX_Underflow	(0x0010)  /* underflow */
#define FPU_EX_Overflow		(0x0008)  /* overflow */
#define FPU_EX_Zero_Div		(0x0004)  /* divide by zero */
#define FPU_EX_Denormal		(0x0002)  /* denormalized operand */
#define FPU_EX_Invalid		(0x0001)  /* invalid operation */

#define clear_C1() { FPU_PARTIAL_STATUS &= ~FPU_SW_C1; }

#define FPU_SW_Backward		(0x8000)  /* backward compatibility */
#define FPU_SW_C3	 	(0x4000)  /* condition bit 3 */
#define FPU_SW_Top		(0x3800)  /* top of stack */
#define FPU_SW_C2		(0x0400)  /* condition bit 2 */
#define FPU_SW_C1		(0x0200)  /* condition bit 1 */
#define FPU_SW_C0		(0x0100)  /* condition bit 0 */
#define FPU_SW_Summary   	(0x0080)  /* exception summary */
#define FPU_SW_Stack_Fault	(0x0040)  /* stack fault */
#define FPU_SW_Precision   	(0x0020)  /* loss of precision */
#define FPU_SW_Underflow   	(0x0010)  /* underflow */
#define FPU_SW_Overflow    	(0x0008)  /* overflow */
#define FPU_SW_Zero_Div    	(0x0004)  /* divide by zero */
#define FPU_SW_Denormal_Op   	(0x0002)  /* denormalized operand */
#define FPU_SW_Invalid    	(0x0001)  /* invalid operation */

#define FPU_SW_Exceptions_Mask  (0x027f)  /* status word exceptions bit mask */

#define FPU_CW_RC		(0x0C00)  /* rounding control */
#define FPU_CW_PC		(0x0300)  /* precision control */

#define FPU_CW_Precision	(0x0020)  /* loss of precision mask */
#define FPU_CW_Underflow	(0x0010)  /* underflow mask */
#define FPU_CW_Overflow		(0x0008)  /* overflow mask */
#define FPU_CW_Zero_Div		(0x0004)  /* divide by zero mask */
#define FPU_CW_Denormal		(0x0002)  /* denormalized operand mask */
#define FPU_CW_Invalid		(0x0001)  /* invalid operation mask */

#define FPU_CW_Exceptions_Mask 	(0x003f)  /* all masks */

//
// Minimal i387 structure
//
struct i387_t 
{
    Bit32u cwd; 	// control word
    Bit32u swd; 	// status word
    Bit32u twd;		// tag word
    Bit32u fip;
    Bit32u fcs;
    Bit32u foo; 	// last instruction opcode
    Bit32u fos;

    bx_address fip_;
    Bit16u fcs_;
    Bit16u fds_;
    bx_address fos_;

    unsigned char tos;
    unsigned char no_update;
    unsigned char rm;
    unsigned char align8;

    Bit64u st_space[16]; // 8*16 bytes per FP-reg (aligned) = 128 bytes
};

// for now solution, will be merged with i387_t when FPU 
// replacement will be done
#ifdef __cplusplus

extern "C" int FPU_tagof(FPU_REG *reg) BX_CPP_AttrRegparmN(1);

#include "softfloat.h"

struct i387_structure_t : public i387_t
{
    i387_structure_t() {}

    void	init();	// initalize fpu stuff

public:
    int    	get_tos() const { return tos; }

    Bit16u 	get_control_word() const { return cwd & 0xFFFF; }
    Bit16u 	get_tag_word() const { return twd & 0xFFFF; }
    Bit16u 	get_status_word() const { return (swd & ~FPU_SW_Top & 0xFFFF) | ((tos << 11) & FPU_SW_Top); }
    Bit16u 	get_partial_status() const { return swd & 0xFFFF; }

    void   	FPU_pop();

    void   	FPU_settag (int tag, int regnr);
    int    	FPU_gettag (int regnr);

    void   	FPU_settagi(int tag, int stnr) { FPU_settag(tag, tos+stnr); }
    int    	FPU_gettagi(int stnr) { return FPU_gettag(tos+stnr); }

    void	FPU_save_reg (floatx80 reg, int tag, int regnr);
    void	FPU_save_reg (floatx80 reg, int regnr);
    floatx80 	FPU_read_reg (int regnr);

    void  	FPU_save_regi(floatx80 reg, int stnr) { FPU_save_reg(reg, (tos+stnr) & 0x07); }
    floatx80 	FPU_read_regi(int stnr) { FPU_read_reg((tos+stnr) & 0x07); }
    void  	FPU_save_regi(floatx80 reg, int tag, int stnr) { FPU_save_reg(reg, tag, (tos+stnr) & 0x07); }
};

#define BX_FPU_REG_PTR(index) (&(st_space[index*2]))

#define IS_TAG_EMPTY(i) \
  ((BX_CPU_THIS_PTR the_i387.FPU_gettagi(i)) == FPU_Tag_Empty)

BX_CPP_INLINE int i387_structure_t::FPU_gettag(int regnr)
{
  return (get_tag_word() >> ((regnr & 7)*2)) & 3;
}

BX_CPP_INLINE void i387_structure_t::FPU_settag (int tag, int regnr)
{
  regnr &= 7;
  twd &= ~(3 << (regnr*2));
  twd |= (tag & 3) << (regnr*2);
}

BX_CPP_INLINE void i387_structure_t::FPU_pop(void)
{
  twd |= 3 << ((tos & 7)*2);
  tos++;
}

BX_CPP_INLINE floatx80 i387_structure_t::FPU_read_reg(int regnr)
{
  FPU_REG reg;
  memcpy(&reg, BX_FPU_REG_PTR(regnr), sizeof(FPU_REG));

  floatx80 result;
  result.exp = reg.exp;
  result.fraction = (((Bit64u)(reg.sigh)) << 32) | ((Bit64u)(reg.sigl));

  return result;
}

BX_CPP_INLINE void i387_structure_t::FPU_save_reg (floatx80 reg, int regnr)
{
  FPU_REG result;
  result.exp  = reg.exp;
  result.sigl = reg.fraction & 0xFFFFFFFF;
  result.sigh = reg.fraction >> 32;

  memcpy(BX_FPU_REG_PTR(regnr), &result, sizeof(FPU_REG));
  FPU_settag(FPU_tagof(&result), regnr);
}

BX_CPP_INLINE void i387_structure_t::FPU_save_reg (floatx80 reg, int tag, int regnr)
{
  FPU_REG result;
  result.exp  = reg.exp;
  result.sigl = reg.fraction & 0xFFFFFFFF;
  result.sigh = reg.fraction >> 32;

  memcpy(BX_FPU_REG_PTR(regnr), &result, sizeof(FPU_REG));
  FPU_settag(tag, regnr);
}

BX_CPP_INLINE void i387_structure_t::init()
{
  cwd = 0x037F;
  swd = 0;
  tos = 0;
  twd = 0xFFFF;

  fip_ = fcs_ = fds_ = fos_ = foo = 0;
  fip  = fcs  = fos  = 0;
}
#endif

#if BX_SUPPORT_MMX

typedef union {
  Bit8u u8;
  Bit8s s8;
} MMX_BYTE;

typedef union {
  Bit16u u16;
  Bit16s s16;
  struct {
#ifdef BX_BIG_ENDIAN
    MMX_BYTE hi;
    MMX_BYTE lo;
#else
    MMX_BYTE lo;
    MMX_BYTE hi;
#endif
  } bytes;
} MMX_WORD;

typedef union {
  Bit32u u32;
  Bit32s s32;
  struct {
#ifdef BX_BIG_ENDIAN
    MMX_WORD hi;
    MMX_WORD lo;
#else
    MMX_WORD lo;
    MMX_WORD hi;
#endif
  } words;
} MMX_DWORD;

typedef union {
  Bit64u u64;
  Bit64s s64;
  struct {
#ifdef BX_BIG_ENDIAN
    MMX_DWORD hi;
    MMX_DWORD lo;
#else
    MMX_DWORD lo;
    MMX_DWORD hi;
#endif
  } dwords;
} MMX_QWORD, BxPackedMmxRegister;

#define MMXSB0(reg) (reg.dwords.lo.words.lo.bytes.lo.s8)
#define MMXSB1(reg) (reg.dwords.lo.words.lo.bytes.hi.s8)
#define MMXSB2(reg) (reg.dwords.lo.words.hi.bytes.lo.s8)
#define MMXSB3(reg) (reg.dwords.lo.words.hi.bytes.hi.s8)
#define MMXSB4(reg) (reg.dwords.hi.words.lo.bytes.lo.s8)
#define MMXSB5(reg) (reg.dwords.hi.words.lo.bytes.hi.s8)
#define MMXSB6(reg) (reg.dwords.hi.words.hi.bytes.lo.s8)
#define MMXSB7(reg) (reg.dwords.hi.words.hi.bytes.hi.s8)

#define MMXUB0(reg) (reg.dwords.lo.words.lo.bytes.lo.u8)
#define MMXUB1(reg) (reg.dwords.lo.words.lo.bytes.hi.u8)
#define MMXUB2(reg) (reg.dwords.lo.words.hi.bytes.lo.u8)
#define MMXUB3(reg) (reg.dwords.lo.words.hi.bytes.hi.u8)
#define MMXUB4(reg) (reg.dwords.hi.words.lo.bytes.lo.u8)
#define MMXUB5(reg) (reg.dwords.hi.words.lo.bytes.hi.u8)
#define MMXUB6(reg) (reg.dwords.hi.words.hi.bytes.lo.u8)
#define MMXUB7(reg) (reg.dwords.hi.words.hi.bytes.hi.u8)

#define MMXSW0(reg) (reg.dwords.lo.words.lo.s16)
#define MMXSW1(reg) (reg.dwords.lo.words.hi.s16)
#define MMXSW2(reg) (reg.dwords.hi.words.lo.s16)
#define MMXSW3(reg) (reg.dwords.hi.words.hi.s16)

#define MMXUW0(reg) (reg.dwords.lo.words.lo.u16)
#define MMXUW1(reg) (reg.dwords.lo.words.hi.u16)
#define MMXUW2(reg) (reg.dwords.hi.words.lo.u16)
#define MMXUW3(reg) (reg.dwords.hi.words.hi.u16)
                                
#define MMXSD0(reg) (reg.dwords.lo.s32)
#define MMXSD1(reg) (reg.dwords.hi.s32)

#define MMXUD0(reg) (reg.dwords.lo.u32)
#define MMXUD1(reg) (reg.dwords.hi.u32)

#define MMXSQ(reg)  (reg.s64)
#define MMXUQ(reg)  (reg.u64)

// Endian  Host byte order         Guest (x86) byte order
// ======================================================
// Little  FFFFFFFFEEAAAAAA        FFFFFFFFEEAAAAAA
// Big     AAAAAAEEFFFFFFFF        FFFFFFFFEEAAAAAA
//
// Legend: F - fraction/mmx
//         E - exponent
//         A - alignment

#ifdef BX_BIG_ENDIAN
#if defined(__MWERKS__) && defined(macintosh)
#pragma options align=mac68k
#endif
struct bx_mmx_reg_t {
   Bit16u alignment1, alignment2, alignment3; 
   Bit16u exp; /* 2 byte FP-reg exponent */
   BxPackedMmxRegister packed_mmx_register;
} GCC_ATTRIBUTE((aligned(16), packed));
#if defined(__MWERKS__) && defined(macintosh)
#pragma options align=reset
#endif
#else
struct bx_mmx_reg_t {
   BxPackedMmxRegister packed_mmx_register;
   Bit16u exp; /* 2 byte FP reg exponent */
   Bit16u alignment1, alignment2, alignment3; 
} GCC_ATTRIBUTE((aligned(16), packed));
#endif

#define BX_MMX_REG(index) 						\
    (((bx_mmx_reg_t*)(BX_CPU_THIS_PTR the_i387.st_space))[index])

#define BX_READ_MMX_REG(index) 						\
    ((BX_MMX_REG(index)).packed_mmx_register)

#define BX_WRITE_MMX_REG(index, value)            			\
{                                 					\
   (BX_MMX_REG(index)).packed_mmx_register = value;			\
   (BX_MMX_REG(index)).exp = 0xffff;       				\
}                                                      

#endif 		/* BX_SUPPORT_MMX */

#endif		/* BX_SUPPORT_FPU */

#endif
