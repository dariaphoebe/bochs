/////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2002 Stanislav Shwartsman
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

#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#define LOG_THIS BX_CPU_THIS_PTR

#include "softfloat.h"


#if BX_SUPPORT_SSE

void BX_CPU_C::check_exceptionsSSE(int exceptions_flags)
{
  int unmasked = ~(MXCSR.get_exceptions_masks()) & exceptions_flags;
  MXCSR.set_exceptions(exceptions_flags);

  if (unmasked) 
  {
     if(BX_CPU_THIS_PTR cr4.get_OSXMMEXCPT())
        exception(BX_XM_EXCEPTION, 0, 0);
     else
        exception(BX_UD_EXCEPTION, 0, 0);
  }
}

static softfloat_status_word_t MXCSR_to_softfloat_status_word(bx_mxcsr_t mxcsr)
{
  softfloat_status_word_t status;

  status.float_detect_tininess = float_tininess_before_rounding;
  status.float_exception_flags = 0; // clear exceptions before execution ???
  status.float_rounding_mode = mxcsr.get_rounding_mode();
  status.denormals_are_zeroes = mxcsr.get_DAZ();
  /* if underflow is masked and FUZ is 1, set it to 1, else to 0 */
  status.underflow_flush_to_zero = 0;  			  // FixMe

  return status;
}

#endif


void BX_CPU_C::CVTSI2SD_VsdEd(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("CVTSI2SD_VsdEd: SSE2 instruction still not implemented"));
#else
  BX_INFO(("CVTSI2SD_VsdEd: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::CVTPI2PD_VpdQd(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("CVTPI2PD_VpdQd: SSE2 instruction still not implemented"));
#else
  BX_INFO(("CVTPI2PD_VpdQd: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::CVTTSD2SI_GdWsd(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("CVTTSD2SI_GdWsd: SSE2 instruction still not implemented"));
#else
  BX_INFO(("CVTTSD2SI_GdWsd: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::CVTSD2SI_GdWsd(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("CVTSD2SI_GdWsd: SSE2 instruction still not implemented"));
#else
  BX_INFO(("CVTSD2SI_GdWsd: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::CVTTPD2PI_PqWpd(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("CVTTPD2PI_PqWpd: SSE2 instruction still not implemented"));
#else
  BX_INFO(("CVTTPD2PI_PqWpd: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::CVTPS2PD_VpsWps(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("CVTPS2PD_VpsWps: SSE2 instruction still not implemented"));
#else
  BX_INFO(("CVTPS2PD_VpsWps: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::CVTPD2PS_VpdWpd(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("CVTPD2PS_VpdWpd: SSE2 instruction still not implemented"));
#else
  BX_INFO(("CVTPD2PS_VpdWpd: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::CVTPD2PI_PqWpd(bxInstruction_c *i)
{                      
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("CVTPD2PI_PqWpd: SSE2 instruction still not implemented"));
#else
  BX_INFO(("CVTPD2PI_PqWpd: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::CVTSD2SS_VsdWsd(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("CVTSD2SS_VsdWsd: SSE2 instruction still not implemented"));
#else
  BX_INFO(("CVTSD2SS_VsdWsd: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::CVTSS2SD_VssWss(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("CVTSS2SD_VssWss: SSE2 instruction still not implemented"));
#else
  BX_INFO(("CVTSS2SD_VssWss: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::CVTDQ2PS_VpsWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("CVTDQ2PS_VpsWdq: SSE2 instruction still not implemented"));
#else
  BX_INFO(("CVTDQ2PS_VpsWdq: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::CVTPS2DQ_VdqWps(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("CVTPS2DQ_VdqWps: SSE2 instruction still not implemented"));
#else
  BX_INFO(("CVTPS2DQ_VdqWps: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::CVTTPS2DQ_VdqWps(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("CVTTPS2DQ_VdqWps: SSE2 instruction still not implemented"));
#else
  BX_INFO(("CVTTPS2DQ_VdqWps: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::CVTPI2PS_VpsQq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("CVTPI2PS_VpsQq: SSE instruction still not implemented"));
#else
  BX_INFO(("CVTPI2PS_VpsQq: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::CVTSI2SS_VssEd(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("CVTSI2SS_VssEd: SSE instruction still not implemented"));
#else
  BX_INFO(("CVTSI2SS_VssEd: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::CVTTPS2PI_PqWps(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("CVTTPS2PI_PqWps: SSE instruction still not implemented"));
#else
  BX_INFO(("CVTTPS2PI_PqWps: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::CVTTSS2SI_GdWss(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("CVTTSS2SI_GdWss: SSE instruction still not implemented"));
#else
  BX_INFO(("CVTTSS2SI_GdWss: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::CVTPS2PI_PqWps(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("CVTPS2PI_PqWps: SSE instruction still not implemented"));
#else
  BX_INFO(("CVTPS2PI_PqWps: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::CVTSS2SI_GdWss(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("CVTSS2SI_GdWss: SSE instruction still not implemented"));
#else
  BX_INFO(("CVTSS2SI_GdWss: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::CVTTPD2DQ_VqWpd(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("CVTTPD2DQ_VqWpd: SSE2 instruction still not implemented"));
#else
  BX_INFO(("CVTTPD2DQ_VqWpd: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::CVTPD2DQ_VqWpd(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("CVTPD2DQ_VqWpd: SSE2 instruction still not implemented"));
#else
  BX_INFO(("CVTPD2DQ_VqWpd: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::CVTDQ2PD_VpdWq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("CVTDQ2PD_VpdWq: SSE2 instruction still not implemented"));
#else
  BX_INFO(("CVTDQ2PD_VpdWq: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::COMISS_VpsWps(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("COMISS_VpsWps: SSE instruction still not implemented"));
#else
  BX_INFO(("COMISS_VpsWps: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::COMISD_VpdWpd(bxInstruction_c *i)   
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("COMISD_VpdWpd: SSE2 instruction still not implemented"));
#else
  BX_INFO(("COMISD_VpdWpd: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::UCOMISS_VssWss(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("UCOMISS_VssWss: SSE instruction still not implemented"));
#else
  BX_INFO(("UCOMISS_VssWss: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::UCOMISD_VsdWsd(bxInstruction_c *i)            	
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("UCOMISD_VsdWsd: SSE2 instruction still not implemented"));
#else
  BX_INFO(("UCOMISD_VsdWsd: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* 
 * Opcode: 0F 51
 * Square Root packed single precision.
 * Possible exceptions: #I, #D, #P
 */
void BX_CPU_C::SQRTPS_VpsWps(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op, result;

  /* op is a register or memory reference */
  if (i->modC0()) {
    op = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op);
  }

  softfloat_status_word_t status_word;
  status_word = MXCSR_to_softfloat_status_word(MXCSR);

  result.xmm32u(0) = 
     float32_sqrt(op.xmm32u(0), status_word);
  result.xmm32u(1) = 
     float32_sqrt(op.xmm32u(1), status_word);
  result.xmm32u(2) = 
     float32_sqrt(op.xmm32u(2), status_word);
  result.xmm32u(3) = 
     float32_sqrt(op.xmm32u(3), status_word);

  BX_CPU_THIS_PTR check_exceptionsSSE(status_word.float_exception_flags);
  BX_WRITE_XMM_REG(i->nnn(), result);

#else
  BX_INFO(("SQRTPS_VpsWps: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* 
 * Opcode: 66 0F 51
 * Square Root packed double precision.
 * Possible exceptions: #I, #D, #P
 */
void BX_CPU_C::SQRTPD_VpdWpd(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op, result;

  /* op is a register or memory reference */
  if (i->modC0()) {
    op = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op);
  }

  softfloat_status_word_t status_word;
  status_word = MXCSR_to_softfloat_status_word(MXCSR);

  result.xmm64u(0) = 
     float64_sqrt(op.xmm64u(0), status_word);
  result.xmm64u(1) = 
     float32_sqrt(op.xmm64u(1), status_word);

  BX_CPU_THIS_PTR check_exceptionsSSE(status_word.float_exception_flags);
  BX_WRITE_XMM_REG(i->nnn(), result);

#else
  BX_INFO(("SQRTPD_VpdWpd: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* 
 * Opcode: F2 0F 51
 * Square Root scalar double precision.
 * Possible exceptions: #I, #D, #P
 */
void BX_CPU_C::SQRTSD_VsdWsd(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  Float64 op, result;

  /* op is a register or memory reference */
  if (i->modC0()) {
    op = BX_READ_XMM_REG_LO_QWORD(i->rm());
  }
  else {
    /* pointer, segment address pair */
    read_virtual_qword(i->seg(), RMAddr(i), &op);
  }

  softfloat_status_word_t status_word = 
             MXCSR_to_softfloat_status_word(MXCSR);

  result = float64_sqrt(op, status_word);
  BX_CPU_THIS_PTR check_exceptionsSSE(status_word.float_exception_flags);
  BX_WRITE_XMM_REG_LO_QWORD(i->nnn(), result);

#else
  BX_INFO(("SQRTSD_VsdWsd: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* 
 * Opcode: F3 0F 51
 * Square Root scalar single precision.
 * Possible exceptions: #I, #D, #P
 */
void BX_CPU_C::SQRTSS_VssWss(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  Float32 op, result;

  /* op is a register or memory reference */
  if (i->modC0()) {
    op = BX_READ_XMM_REG_LO_DWORD(i->rm());
  }
  else {
    /* pointer, segment address pair */
    read_virtual_dword(i->seg(), RMAddr(i), &op);
  }

  softfloat_status_word_t status_word = 
             MXCSR_to_softfloat_status_word(MXCSR);

  result = float32_sqrt(op, status_word);
  BX_CPU_THIS_PTR check_exceptionsSSE(status_word.float_exception_flags);
  BX_WRITE_XMM_REG_LO_DWORD(i->nnn(), result);

#else
  BX_INFO(("SQRTSS_VssWss: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::RSQRTPS_VpsWps(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("RSQRTPS_VpsWps: SSE instruction still not implemented"));
#else
  BX_INFO(("RSQRTPS_VpsWps: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::RSQRTSS_VssWss(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("RSQRTSS_VssWss: SSE instruction still not implemented"));
#else
  BX_INFO(("RSQRTSS_VssWss: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::RCPPS_VpsWps(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("RCPPS_VpsWps: SSE instruction still not implemented"));
#else
  BX_INFO(("RCPPS_VpsWps: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::RCPSS_VssWss(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("RCPSS_VssWss: SSE instruction still not implemented"));
#else
  BX_INFO(("RCPSS_VssWss: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* 
 * Opcode: 0F 58
 * Add packed single precision FP numbers from XMM2/MEM to XMM1.
 * Possible exceptions: #I, #D, #O, #U, #P
 */
void BX_CPU_C::ADDPS_VpsWps(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  softfloat_status_word_t status_word;
  status_word = MXCSR_to_softfloat_status_word(MXCSR);

  result.xmm32u(0) = 
     float32_add(op1.xmm32u(0), op2.xmm32u(0), status_word);
  result.xmm32u(1) = 
     float32_add(op1.xmm32u(1), op2.xmm32u(1), status_word);
  result.xmm32u(2) = 
     float32_add(op1.xmm32u(2), op2.xmm32u(2), status_word);
  result.xmm32u(3) = 
     float32_add(op1.xmm32u(3), op2.xmm32u(3), status_word);

  BX_CPU_THIS_PTR check_exceptionsSSE(status_word.float_exception_flags);
  BX_WRITE_XMM_REG(i->nnn(), result);

#else
  BX_INFO(("ADDPS_VpsWps: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* 
 * Opcode: 66 0F 58
 * Add packed double precision FP numbers from XMM2/MEM to XMM1.
 * Possible exceptions: #I, #D, #O, #U, #P
 */
void BX_CPU_C::ADDPD_VpdWpd(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  softfloat_status_word_t status_word;
  status_word = MXCSR_to_softfloat_status_word(MXCSR);

  result.xmm64u(0) = 
     float64_add(op1.xmm64u(0), op2.xmm64u(0), status_word);
  result.xmm64u(1) = 
     float32_add(op1.xmm64u(1), op2.xmm64u(1), status_word);

  BX_CPU_THIS_PTR check_exceptionsSSE(status_word.float_exception_flags);
  BX_WRITE_XMM_REG(i->nnn(), result);

#else
  BX_INFO(("ADDPD_VpdWpd: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* 
 * Opcode: F2 0F 58
 * Add the lower double precision FP number from XMM2/MEM to XMM1.
 * Possible exceptions: #I, #D, #O, #U, #P
 */
void BX_CPU_C::ADDSD_VsdWsd(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  Float64 op1 = BX_READ_XMM_REG_LO_QWORD(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG_LO_QWORD(i->rm());
  }
  else {
    /* pointer, segment address pair */
    read_virtual_qword(i->seg(), RMAddr(i), &op2);
  }

  softfloat_status_word_t status_word = 
             MXCSR_to_softfloat_status_word(MXCSR);

  result = float64_add(op1, op2, status_word);
  BX_CPU_THIS_PTR check_exceptionsSSE(status_word.float_exception_flags);
  BX_WRITE_XMM_REG_LO_QWORD(i->nnn(), result);

#else
  BX_INFO(("ADDSD_VsdWsd: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* 
 * Opcode: F3 0F 58
 * Add the lower single precision FP number from XMM2/MEM to XMM1.
 * Possible exceptions: #I, #D, #O, #U, #P
 */
void BX_CPU_C::ADDSS_VssWss(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  Float32 op1 = BX_READ_XMM_REG_LO_DWORD(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG_LO_DWORD(i->rm());
  }
  else {
    /* pointer, segment address pair */
    read_virtual_dword(i->seg(), RMAddr(i), &op2);
  }

  softfloat_status_word_t status_word = 
             MXCSR_to_softfloat_status_word(MXCSR);

  result = float32_add(op1, op2, status_word);
  BX_CPU_THIS_PTR check_exceptionsSSE(status_word.float_exception_flags);
  BX_WRITE_XMM_REG_LO_DWORD(i->nnn(), result);

#else
  BX_INFO(("ADDSS_VssWss: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* 
 * Opcode: 0F 59
 * Multiply packed single precision FP numbers from XMM2/MEM to XMM1.
 * Possible exceptions: #I, #D, #O, #U, #P
 */
void BX_CPU_C::MULPS_VpsWps(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  softfloat_status_word_t status_word;
  status_word = MXCSR_to_softfloat_status_word(MXCSR);

  result.xmm32u(0) = 
     float32_mul(op1.xmm32u(0), op2.xmm32u(0), status_word);
  result.xmm32u(1) = 
     float32_mul(op1.xmm32u(1), op2.xmm32u(1), status_word);
  result.xmm32u(2) = 
     float32_mul(op1.xmm32u(2), op2.xmm32u(2), status_word);
  result.xmm32u(3) = 
     float32_mul(op1.xmm32u(3), op2.xmm32u(3), status_word);

  BX_CPU_THIS_PTR check_exceptionsSSE(status_word.float_exception_flags);
  BX_WRITE_XMM_REG(i->nnn(), result);

#else
  BX_INFO(("MULPS_VpsWps: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* 
 * Opcode: 66 0F 59
 * Multiply packed double precision FP numbers from XMM2/MEM to XMM1.
 * Possible exceptions: #I, #D, #O, #U, #P
 */
void BX_CPU_C::MULPD_VpdWpd(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  softfloat_status_word_t status_word;
  status_word = MXCSR_to_softfloat_status_word(MXCSR);

  result.xmm64u(0) = 
     float64_mul(op1.xmm64u(0), op2.xmm64u(0), status_word);
  result.xmm64u(1) = 
     float32_mul(op1.xmm64u(1), op2.xmm64u(1), status_word);

  BX_CPU_THIS_PTR check_exceptionsSSE(status_word.float_exception_flags);
  BX_WRITE_XMM_REG(i->nnn(), result);

#else
  BX_INFO(("MULPD_VpdWpd: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* 
 * Opcode: F2 0F 59
 * Multiply the lower double precision FP number from XMM2/MEM to XMM1.
 * Possible exceptions: #I, #D, #O, #U, #P
 */
void BX_CPU_C::MULSD_VsdWsd(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  Float64 op1 = BX_READ_XMM_REG_LO_QWORD(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG_LO_QWORD(i->rm());
  }
  else {
    /* pointer, segment address pair */
    read_virtual_qword(i->seg(), RMAddr(i), &op2);
  }

  softfloat_status_word_t status_word = 
             MXCSR_to_softfloat_status_word(MXCSR);

  result = float64_mul(op1, op2, status_word);
  BX_CPU_THIS_PTR check_exceptionsSSE(status_word.float_exception_flags);
  BX_WRITE_XMM_REG_LO_QWORD(i->nnn(), result);

#else
  BX_INFO(("MULSD_VsdWsd: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* 
 * Opcode: F3 0F 59
 * Multiply the lower single precision FP number from XMM2/MEM to XMM1.
 * Possible exceptions: #I, #D, #O, #U, #P
 */
void BX_CPU_C::MULSS_VssWss(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  Float32 op1 = BX_READ_XMM_REG_LO_DWORD(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG_LO_DWORD(i->rm());
  }
  else {
    /* pointer, segment address pair */
    read_virtual_dword(i->seg(), RMAddr(i), &op2);
  }

  softfloat_status_word_t status_word = 
             MXCSR_to_softfloat_status_word(MXCSR);

  result = float32_mul(op1, op2, status_word);
  BX_CPU_THIS_PTR check_exceptionsSSE(status_word.float_exception_flags);
  BX_WRITE_XMM_REG_LO_DWORD(i->nnn(), result);

#else
  BX_INFO(("MULSS_VssWss: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* 
 * Opcode: 0F 5C
 * Subtract packed single precision FP numbers from XMM2/MEM to XMM1.
 * Possible exceptions: #I, #D, #O, #U, #P
 */
void BX_CPU_C::SUBPS_VpsWps(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  softfloat_status_word_t status_word;
  status_word = MXCSR_to_softfloat_status_word(MXCSR);

  result.xmm32u(0) = 
     float32_sub(op1.xmm32u(0), op2.xmm32u(0), status_word);
  result.xmm32u(1) = 
     float32_sub(op1.xmm32u(1), op2.xmm32u(1), status_word);
  result.xmm32u(2) = 
     float32_sub(op1.xmm32u(2), op2.xmm32u(2), status_word);
  result.xmm32u(3) = 
     float32_sub(op1.xmm32u(3), op2.xmm32u(3), status_word);

  BX_CPU_THIS_PTR check_exceptionsSSE(status_word.float_exception_flags);
  BX_WRITE_XMM_REG(i->nnn(), result);

#else
  BX_INFO(("SUBPS_VpsWps: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* 
 * Opcode: 66 0F 5C
 * Subtract packed double precision FP numbers from XMM2/MEM to XMM1.
 * Possible exceptions: #I, #D, #O, #U, #P
 */
void BX_CPU_C::SUBPD_VpdWpd(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  softfloat_status_word_t status_word;
  status_word = MXCSR_to_softfloat_status_word(MXCSR);

  result.xmm64u(0) = 
     float64_sub(op1.xmm64u(0), op2.xmm64u(0), status_word);
  result.xmm64u(1) = 
     float32_sub(op1.xmm64u(1), op2.xmm64u(1), status_word);

  BX_CPU_THIS_PTR check_exceptionsSSE(status_word.float_exception_flags);
  BX_WRITE_XMM_REG(i->nnn(), result);

#else
  BX_INFO(("SUBPD_VpdWpd: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* 
 * Opcode: F2 0F 5C
 * Subtract the lower double precision FP number from XMM2/MEM to XMM1.
 * Possible exceptions: #I, #D, #O, #U, #P
 */
void BX_CPU_C::SUBSD_VsdWsd(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  Float64 op1 = BX_READ_XMM_REG_LO_QWORD(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG_LO_QWORD(i->rm());
  }
  else {
    /* pointer, segment address pair */
    read_virtual_qword(i->seg(), RMAddr(i), &op2);
  }

  softfloat_status_word_t status_word = 
             MXCSR_to_softfloat_status_word(MXCSR);

  result = float64_sub(op1, op2, status_word);
  BX_CPU_THIS_PTR check_exceptionsSSE(status_word.float_exception_flags);
  BX_WRITE_XMM_REG_LO_QWORD(i->nnn(), result);

#else
  BX_INFO(("SUBSD_VsdWsd: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* 
 * Opcode: F3 0F 5C
 * Subtract the lower single precision FP number from XMM2/MEM to XMM1.
 * Possible exceptions: #I, #D, #O, #U, #P
 */
void BX_CPU_C::SUBSS_VssWss(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  Float32 op1 = BX_READ_XMM_REG_LO_DWORD(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG_LO_DWORD(i->rm());
  }
  else {
    /* pointer, segment address pair */
    read_virtual_dword(i->seg(), RMAddr(i), &op2);
  }

  softfloat_status_word_t status_word = 
             MXCSR_to_softfloat_status_word(MXCSR);

  result = float32_sub(op1, op2, status_word);
  BX_CPU_THIS_PTR check_exceptionsSSE(status_word.float_exception_flags);
  BX_WRITE_XMM_REG_LO_DWORD(i->nnn(), result);

#else
  BX_INFO(("SUBSS_VssWss: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::MINPS_VpsWps(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("MINPS_VpsWps: SSE instruction still not implemented"));
#else
  BX_INFO(("MINPS_VpsWps: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::MINPD_VpdWpd(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("MINPD_VpdWpd: SSE2 instruction still not implemented"));
#else
  BX_INFO(("MINPD_VpdWpd: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::MINSD_VsdWsd(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("MINSD_VsdWsd: SSE2 instruction still not implemented"));
#else
  BX_INFO(("MINSD_VsdWsd: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::MINSS_VssWss(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("MINSS_VssWss: SSE instruction still not implemented"));
#else
  BX_INFO(("MINSS_VssWss: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* 
 * Opcode: 0F 5E
 * Divide packed single precision FP numbers from XMM2/MEM to XMM1.
 * Possible exceptions: #I, #D, #Z, #O, #U, #P
 */
void BX_CPU_C::DIVPS_VpsWps(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  softfloat_status_word_t status_word;
  status_word = MXCSR_to_softfloat_status_word(MXCSR);

  result.xmm32u(0) = 
     float32_div(op1.xmm32u(0), op2.xmm32u(0), status_word);
  result.xmm32u(1) = 
     float32_div(op1.xmm32u(1), op2.xmm32u(1), status_word);
  result.xmm32u(2) = 
     float32_div(op1.xmm32u(2), op2.xmm32u(2), status_word);
  result.xmm32u(3) = 
     float32_div(op1.xmm32u(3), op2.xmm32u(3), status_word);

  BX_CPU_THIS_PTR check_exceptionsSSE(status_word.float_exception_flags);
  BX_WRITE_XMM_REG(i->nnn(), result);

#else
  BX_INFO(("DIVPS_VpsWps: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* 
 * Opcode: 66 0F 5E
 * Divide packed double precision FP numbers from XMM2/MEM to XMM1.
 * Possible exceptions: #I, #D, #Z, #O, #U, #P
 */
void BX_CPU_C::DIVPD_VpdWpd(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  softfloat_status_word_t status_word;
  status_word = MXCSR_to_softfloat_status_word(MXCSR);

  result.xmm64u(0) = 
     float64_div(op1.xmm64u(0), op2.xmm64u(0), status_word);
  result.xmm64u(1) = 
     float32_div(op1.xmm64u(1), op2.xmm64u(1), status_word);

  BX_CPU_THIS_PTR check_exceptionsSSE(status_word.float_exception_flags);
  BX_WRITE_XMM_REG(i->nnn(), result);

#else
  BX_INFO(("DIVPD_VpdWpd: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* 
 * Opcode: F2 0F 5E
 * Divide the lower double precision FP number from XMM2/MEM to XMM1.
 * Possible exceptions: #I, #D, #Z, #O, #U, #P
 */
void BX_CPU_C::DIVSD_VsdWsd(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  Float64 op1 = BX_READ_XMM_REG_LO_QWORD(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG_LO_QWORD(i->rm());
  }
  else {
    /* pointer, segment address pair */
    read_virtual_qword(i->seg(), RMAddr(i), &op2);
  }

  softfloat_status_word_t status_word = 
             MXCSR_to_softfloat_status_word(MXCSR);

  result = float64_div(op1, op2, status_word);
  BX_CPU_THIS_PTR check_exceptionsSSE(status_word.float_exception_flags);
  BX_WRITE_XMM_REG_LO_QWORD(i->nnn(), result);

#else
  BX_INFO(("DIVSD_VsdWsd: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* 
 * Opcode: F3 0F 5E
 * Divide the lower single precision FP number from XMM2/MEM to XMM1.
 * Possible exceptions: #I, #D, #Z, #O, #U, #P
 */
void BX_CPU_C::DIVSS_VssWss(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  Float32 op1 = BX_READ_XMM_REG_LO_DWORD(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG_LO_DWORD(i->rm());
  }
  else {
    /* pointer, segment address pair */
    read_virtual_dword(i->seg(), RMAddr(i), &op2);
  }

  softfloat_status_word_t status_word = 
             MXCSR_to_softfloat_status_word(MXCSR);

  result = float32_div(op1, op2, status_word);
  BX_CPU_THIS_PTR check_exceptionsSSE(status_word.float_exception_flags);
  BX_WRITE_XMM_REG_LO_DWORD(i->nnn(), result);

#else
  BX_INFO(("DIVSS_VssWss: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::MAXPS_VpsWps(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("MAXPS_VpsWps: SSE instruction still not implemented"));
#else
  BX_INFO(("MAXPS_VpsWps: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::MAXPD_VpdWpd(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("MAXPD_VpdWpd: SSE2 instruction still not implemented"));
#else
  BX_INFO(("MAXPD_VpdWpd: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::MAXSD_VsdWsd(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("MAXSD_VsdWsd: SSE2 instruction still not implemented"));
#else
  BX_INFO(("MAXSD_VsdWsd: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::MAXSS_VssWss(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("MAXSS_VssWss: SSE instruction still not implemented"));
#else
  BX_INFO(("MAXSS_VssWss: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::CMPPS_VpsWpsIb(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("CMPPS_VpsWpsIb: SSE instruction still not implemented"));
#else
  BX_INFO(("CMPPS_VpsWpsIb: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::CMPPD_VpdWpdIb(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("CMPPD_VpdWpdIb: SSE2 instruction still not implemented"));
#else
  BX_INFO(("CMPPD_VpdWpdIb: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::CMPSD_VsdWsdIb(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("CMPSD_VsdWsdIb: SSE2 instruction still not implemented"));
#else
  BX_INFO(("CMPSD_VsdWsdIb: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::CMPSS_VssWssIb(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("CMPSS_VssWssIb: SSE instruction still not implemented"));
#else
  BX_INFO(("CMPSS_VssWssIb: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}
