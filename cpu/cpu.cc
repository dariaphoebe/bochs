/////////////////////////////////////////////////////////////////////////
// $Id: cpu.cc,v 1.76.2.4 2003/04/02 08:54:20 slechta Exp $
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2001  MandrakeSoft S.A.
//
//    MandrakeSoft S.A.
//    43, rue d'Aboukir
//    75002 Paris - France
//    http://www.linux-mandrake.com/
//    http://www.mandrakesoft.com/
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



#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_USE_CPU_SMF
#define BX_CPU_THIS (BX_CPU(0))
#else
#define BX_CPU_THIS this
#endif


#if BX_SIM_ID == 0   // only need to define once
// This array defines a look-up table for the even parity-ness
// of an 8bit quantity, for optimal assignment of the parity bit
// in the EFLAGS register
const bx_bool bx_parity_lookup[256] = {
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1
  };
#endif


#if BX_SMP_PROCESSORS==1
// single processor simulation, so there's one of everything
BOCHSAPI BX_CPU_C    bx_cpu;
BOCHSAPI BX_MEM_C    bx_mem;
#else
// multiprocessor simulation, we need an array of cpus and memories
BOCHSAPI BX_CPU_C    *bx_cpu_array[BX_SMP_PROCESSORS];
BOCHSAPI BX_MEM_C    *bx_mem_array[BX_ADDRESS_SPACES];
#endif



// notes:
//
// check limit of CS?

#ifdef REGISTER_IADDR
extern void REGISTER_IADDR(bx_addr addr);
#endif

// The CHECK_MAX_INSTRUCTIONS macro allows cpu_loop to execute a few
// instructions and then return so that the other processors have a chance to
// run.  This is used only when simulating multiple processors.
// 
// If maximum instructions have been executed, return.  A count less
// than zero means run forever.
#define CHECK_MAX_INSTRUCTIONS(count) \
  if (count >= 0) {                   \
    count--; if (count == 0) return;  \
  }

#if BX_SMP_PROCESSORS==1
#  define BX_TICK1_IF_SINGLE_PROCESSOR() BX_TICK1()
#else
#  define BX_TICK1_IF_SINGLE_PROCESSOR()
#endif

// Make code more tidy with a few macros.
#if BX_SUPPORT_X86_64==0
#define RIP EIP
#define RSP ESP
#endif


  void
BX_CPU_C::cpu_loop(Bit32s max_instr_count)
{
  unsigned ret;
  bxInstruction_c iStorage BX_CPP_AlignN(32);
  bxInstruction_c *i = &iStorage;

  BxExecutePtr_t execute;

#if BX_DEBUGGER
  BX_CPU_THIS_PTR break_point = 0;
#ifdef MAGIC_BREAKPOINT
  BX_CPU_THIS_PTR magic_break = 0;
#endif
  BX_CPU_THIS_PTR stop_reason = STOP_NO_REASON;
#endif

#if BX_INSTRUMENTATION
  if (setjmp( BX_CPU_THIS_PTR jmp_buf_env )) 
  { 
    // only from exception function can we get here ...
    BX_INSTR_NEW_INSTRUCTION(BX_CPU_ID);
  }
#else
  (void) setjmp( BX_CPU_THIS_PTR jmp_buf_env );
#endif

#if BX_DEBUGGER
  // If the exception() routine has encountered a nasty fault scenario,
  // the debugger may request that control is returned to it so that
  // the situation may be examined.
  if (bx_guard.special_unwind_stack) {
    return;
    }
#endif

  // We get here either by a normal function call, or by a longjmp
  // back from an exception() call.  In either case, commit the
  // new EIP/ESP, and set up other environmental fields.  This code
  // mirrors similar code below, after the interrupt() call.
  BX_CPU_THIS_PTR prev_eip = RIP; // commit new EIP
  BX_CPU_THIS_PTR prev_esp = RSP; // commit new ESP
  BX_CPU_THIS_PTR EXT = 0;
  BX_CPU_THIS_PTR errorno = 0;

  while (1) {

  // First check on events which occurred for previous instructions
  // (traps) and ones which are asynchronous to the CPU
  // (hardware interrupts).
  if (BX_CPU_THIS_PTR async_event) {
    if (handleAsyncEvent()) {
      // If request to return to caller ASAP.
      return;
      }
    }

#if BX_DEBUGGER
  {
  Bit32u debug_eip = BX_CPU_THIS_PTR prev_eip;
  if ( dbg_is_begin_instr_bpoint(
         BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value,
         debug_eip,
         BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.base + debug_eip,
         BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.d_b) ) {
    return;
    }
  }
#endif  // #if BX_DEBUGGER

#if BX_EXTERNAL_DEBUGGER
  if (regs.debug_state != debug_run) {
    bx_external_debugger(BX_CPU_THIS);
  }
#endif

  {
  bx_address eipBiased;
  Bit8u *fetchPtr;

  eipBiased = RIP + BX_CPU_THIS_PTR eipPageBias;

  if ( eipBiased >= BX_CPU_THIS_PTR eipPageWindowSize ) {
    prefetch();
    eipBiased = RIP + BX_CPU_THIS_PTR eipPageBias;
    }

#if BX_SupportICache
  unsigned iCacheHash;
  Bit32u pAddr, pageWriteStamp, fetchModeMask;

  pAddr = BX_CPU_THIS_PTR pAddrA20Page + eipBiased;
  iCacheHash = BX_CPU_THIS_PTR iCache.hash( pAddr );
  i = & BX_CPU_THIS_PTR iCache.entry[iCacheHash].i;

  pageWriteStamp = BX_CPU_THIS_PTR iCache.pageWriteStampTable[pAddr>>12];
  fetchModeMask  = BX_CPU_THIS_PTR iCache.fetchModeMask;

  if ( (BX_CPU_THIS_PTR iCache.entry[iCacheHash].pAddr == pAddr) &&
       (BX_CPU_THIS_PTR iCache.entry[iCacheHash].writeStamp == pageWriteStamp) &&
       ((pageWriteStamp & fetchModeMask) == fetchModeMask) ) {

    // iCache hit.  Instruction is already decoded and stored in
    // the instruction cache.
    BxExecutePtr_tR resolveModRM = i->ResolveModrm; // Get as soon as possible for speculation.

    execute = i->execute; // fetch as soon as possible for speculation.
    if (resolveModRM) {
      BX_CPU_CALL_METHODR(resolveModRM, (i));
    }
#if BX_INSTRUMENTATION
    // An instruction was found in the iCache.
    BX_INSTR_OPCODE(BX_CPU_ID, BX_CPU_THIS_PTR eipFetchPtr + eipBiased,
          i->ilen(), BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.d_b);
#endif
    }
  else
#endif
    {
    // iCache miss.  No validated instruction with matching fetch parameters
    // is in the iCache.  Or we're not compiling iCache support in, in which
    // case we always have an iCache miss.  :^)
    bx_address remainingInPage;
    unsigned maxFetch;

    remainingInPage = (BX_CPU_THIS_PTR eipPageWindowSize - eipBiased);
    maxFetch = 15;
    if (remainingInPage < 15)
      maxFetch = remainingInPage;
    fetchPtr = BX_CPU_THIS_PTR eipFetchPtr + eipBiased;

#if BX_SupportICache
    // In the case where the page is marked ICacheWriteStampInvalid, all
    // counter bits will be high, being eqivalent to ICacheWriteStampMax.
    // In the case where the page is marked as possibly having associated
    // iCache entries, we need to leave the counter as-is, unless we're
    // willing to dump all iCache entries which can hash to this page.
    // Therefore, in either case, we can keep the counter as-is and
    // replace the fetch mode bits.
    pageWriteStamp &= 0x1fffffff;    // Clear out old fetch mode bits.
    pageWriteStamp |= fetchModeMask; // Add in new ones.
    BX_CPU_THIS_PTR iCache.pageWriteStampTable[pAddr>>12] = pageWriteStamp;
    BX_CPU_THIS_PTR iCache.entry[iCacheHash].pAddr = pAddr;
    BX_CPU_THIS_PTR iCache.entry[iCacheHash].writeStamp = pageWriteStamp;
#endif

#if BX_SUPPORT_X86_64
    if (BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_64) {
      ret = fetchDecode64(fetchPtr, i, maxFetch);
      }
    else
#endif
      {
      ret = fetchDecode(fetchPtr, i, maxFetch);
      }

    BxExecutePtr_tR resolveModRM = i->ResolveModrm; // Get function pointers early.
    if (ret==0) {
#if BX_SupportICache
      // Invalidate entry, since fetch-decode failed with partial updates
      // to the i-> structure.
      BX_CPU_THIS_PTR iCache.entry[iCacheHash].writeStamp =
        ICacheWriteStampInvalid;
      i = &iStorage;
#endif
      boundaryFetch(i);
      resolveModRM = i->ResolveModrm; // Get function pointers as early
    }
#if BX_INSTRUMENTATION
    else
    {
      // An instruction was either fetched, or found in the iCache.
      BX_INSTR_OPCODE(BX_CPU_ID, fetchPtr, i->ilen(),
                  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.d_b);
    }
#endif
    execute = i->execute; // fetch as soon as possible for speculation.
    if (resolveModRM) {
      BX_CPU_CALL_METHODR(resolveModRM, (i));
      }
    }
  }

    // An instruction will have been fetched using either the normal case,
    // or the boundary fetch (across pages), by this point.
    BX_INSTR_FETCH_DECODE_COMPLETED(BX_CPU_ID, i);

#if BX_DEBUGGER
    if (BX_CPU_THIS_PTR trace) {
      // print the instruction that is about to be executed.
      bx_dbg_disassemble_current (BX_CPU_ID, 1);  // only one cpu, print time stamp
    }
#endif

    if ( !(i->repUsedL() && i->repeatableL()) ) {
      // non repeating instruction
      RIP += i->ilen();
      BX_CPU_CALL_METHOD(execute, (i));

      BX_CPU_THIS_PTR prev_eip = RIP; // commit new EIP
      BX_CPU_THIS_PTR prev_esp = RSP; // commit new ESP
#ifdef REGISTER_IADDR
      REGISTER_IADDR(RIP + BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.base);
#endif

      BX_TICK1_IF_SINGLE_PROCESSOR();
      }

    else {
repeat_loop:
      if (i->repeatableZFL()) {
#if BX_SUPPORT_X86_64
        if (i->as64L()) {
          if (RCX != 0) {
            BX_CPU_CALL_METHOD(execute, (i));
            RCX -= 1;
            }
          if ((i->repUsedValue()==3) && (get_ZF()==0)) goto repeat_done;
          if ((i->repUsedValue()==2) && (get_ZF()!=0)) goto repeat_done;
          if (RCX == 0) goto repeat_done;
          goto repeat_not_done;
          }
        else
#endif
        if (i->as32L()) {
          if (ECX != 0) {
            BX_CPU_CALL_METHOD(execute, (i));
            ECX -= 1;
            }
          if ((i->repUsedValue()==3) && (get_ZF()==0)) goto repeat_done;
          if ((i->repUsedValue()==2) && (get_ZF()!=0)) goto repeat_done;
          if (ECX == 0) goto repeat_done;
          goto repeat_not_done;
          }
        else {
          if (CX != 0) {
            BX_CPU_CALL_METHOD(execute, (i));
            CX -= 1;
            }
          if ((i->repUsedValue()==3) && (get_ZF()==0)) goto repeat_done;
          if ((i->repUsedValue()==2) && (get_ZF()!=0)) goto repeat_done;
          if (CX == 0) goto repeat_done;
          goto repeat_not_done;
          }
        }
      else { // normal repeat, no concern for ZF
#if BX_SUPPORT_X86_64
        if (i->as64L()) {
          if (RCX != 0) {
            BX_CPU_CALL_METHOD(execute, (i));
            RCX -= 1;
            }
          if (RCX == 0) goto repeat_done;
          goto repeat_not_done;
          }
        else
#endif
        if (i->as32L()) {
          if (ECX != 0) {
            BX_CPU_CALL_METHOD(execute, (i));
            ECX -= 1;
            }
          if (ECX == 0) goto repeat_done;
          goto repeat_not_done;
          }
        else { // 16bit addrsize
          if (CX != 0) {
            BX_CPU_CALL_METHOD(execute, (i));
            CX -= 1;
            }
          if (CX == 0) goto repeat_done;
          goto repeat_not_done;
          }
        }
      // shouldn't get here from above
repeat_not_done:
#ifdef REGISTER_IADDR
      REGISTER_IADDR(RIP + BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.base);
#endif

      BX_INSTR_REPEAT_ITERATION(BX_CPU_ID);
      BX_TICK1_IF_SINGLE_PROCESSOR();

#if BX_DEBUGGER == 0
      if (BX_CPU_THIS_PTR async_event) {
        invalidate_prefetch_q();
        goto debugger_check;
      }
      goto repeat_loop;
#else  /* if BX_DEBUGGER == 1 */
      invalidate_prefetch_q();
      goto debugger_check;
#endif


repeat_done:
      RIP += i->ilen();

      BX_CPU_THIS_PTR prev_eip = RIP; // commit new EIP
      BX_CPU_THIS_PTR prev_esp = RSP; // commit new ESP
#ifdef REGISTER_IADDR
      REGISTER_IADDR(RIP + BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.base);
#endif

      BX_INSTR_REPEAT_ITERATION(BX_CPU_ID);
      BX_TICK1_IF_SINGLE_PROCESSOR();
      }

debugger_check:

    // inform instrumentation about new instruction
    BX_INSTR_NEW_INSTRUCTION(BX_CPU_ID);

#if (BX_SMP_PROCESSORS>1 && BX_DEBUGGER==0)
    // The CHECK_MAX_INSTRUCTIONS macro allows cpu_loop to execute a few
    // instructions and then return so that the other processors have a chance
    // to run.  This is used only when simulating multiple processors.  If only
    // one processor, don't waste any cycles on it!  Also, it is not needed
    // with the debugger because its guard mechanism provides the same
    // functionality.
    CHECK_MAX_INSTRUCTIONS(max_instr_count);
#endif

#if BX_DEBUGGER

    // BW vm mode switch support is in dbg_is_begin_instr_bpoint
    // note instr generating exceptions never reach this point.

    // (mch) Read/write, time break point support
    if (BX_CPU_THIS_PTR break_point) {
      switch (BX_CPU_THIS_PTR break_point) {
        case BREAK_POINT_TIME:
          BX_INFO(("[%lld] Caught time breakpoint", bx_pc_system.time_ticks()));
          BX_CPU_THIS_PTR stop_reason = STOP_TIME_BREAK_POINT;
          return;
        case BREAK_POINT_READ:
          BX_INFO(("[%lld] Caught read watch point", bx_pc_system.time_ticks()));
          BX_CPU_THIS_PTR stop_reason = STOP_READ_WATCH_POINT;
          return;
        case BREAK_POINT_WRITE:
          BX_INFO(("[%lld] Caught write watch point", bx_pc_system.time_ticks()));
          BX_CPU_THIS_PTR stop_reason = STOP_WRITE_WATCH_POINT;
          return;
        default:
          BX_PANIC(("Weird break point condition"));
        }
      }
#ifdef MAGIC_BREAKPOINT
    // (mch) Magic break point support
    if (BX_CPU_THIS_PTR magic_break) {
      if (bx_dbg.magic_break_enabled) {
        BX_DEBUG(("Stopped on MAGIC BREAKPOINT"));
        BX_CPU_THIS_PTR stop_reason = STOP_MAGIC_BREAK_POINT;
        return;
        }
      else {
        BX_CPU_THIS_PTR magic_break = 0;
        BX_CPU_THIS_PTR stop_reason = STOP_NO_REASON;
        BX_DEBUG(("Ignoring MAGIC BREAKPOINT"));
        }
      }
#endif

    {
      // check for icount or control-C.  If found, set guard reg and return.
    Bit32u debug_eip = BX_CPU_THIS_PTR prev_eip;
    if ( dbg_is_end_instr_bpoint(
           BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value,
           debug_eip,
           BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.base + debug_eip,
           BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.d_b) ) {
      return;
      }
    }

#endif  // #if BX_DEBUGGER
#if BX_GDBSTUB
    {
    unsigned int reason;
    if ((reason = bx_gdbstub_check(EIP)) != GDBSTUB_STOP_NO_REASON) {
      return;
      }
    }
#endif

  }  // while (1)
}

  unsigned
BX_CPU_C::handleAsyncEvent(void)
{
  //
  // This area is where we process special conditions and events.
  //

  if (BX_CPU_THIS_PTR debug_trap & 0x80000000) {
    // I made up the bitmask above to mean HALT state.
#if BX_SMP_PROCESSORS==1
    BX_CPU_THIS_PTR debug_trap = 0; // clear traps for after resume
    BX_CPU_THIS_PTR inhibit_mask = 0; // clear inhibits for after resume
    // for one processor, pass the time as quickly as possible until
    // an interrupt wakes up the CPU.
#if BX_DEBUGGER
    while (bx_guard.interrupt_requested != 1)
#else
    while (1)
#endif
      {
      if (BX_CPU_INTR && BX_CPU_THIS_PTR get_IF ()) {
        break;
        }
      if (BX_CPU_THIS_PTR async_event == 2) {
        BX_INFO(("decode: reset detected in halt state"));
        break;
        }
      BX_TICK1();
      }
#else      /* BX_SMP_PROCESSORS != 1 */
    // for multiprocessor simulation, even if this CPU is halted we still
    // must give the others a chance to simulate.  If an interrupt has 
    // arrived, then clear the HALT condition; otherwise just return from
    // the CPU loop with stop_reason STOP_CPU_HALTED.
    if (BX_CPU_INTR && BX_CPU_THIS_PTR get_IF ()) {
      // interrupt ends the HALT condition
      BX_CPU_THIS_PTR debug_trap = 0; // clear traps for after resume
      BX_CPU_THIS_PTR inhibit_mask = 0; // clear inhibits for after resume
      //bx_printf ("halt condition has been cleared in %s", name);
    } else {
      // HALT condition remains, return so other CPUs have a chance
#if BX_DEBUGGER
      BX_CPU_THIS_PTR stop_reason = STOP_CPU_HALTED;
#endif
      return 1; // Return to caller of cpu_loop.
    }
#endif
  } else if (BX_CPU_THIS_PTR kill_bochs_request) {
    // setting kill_bochs_request causes the cpu loop to return ASAP.
    return 1; // Return to caller of cpu_loop.
  }


  // Priority 1: Hardware Reset and Machine Checks
  //   RESET
  //   Machine Check
  // (bochs doesn't support these)

  // Priority 2: Trap on Task Switch
  //   T flag in TSS is set
  if (BX_CPU_THIS_PTR debug_trap & 0x00008000) {
    BX_CPU_THIS_PTR dr6 |= BX_CPU_THIS_PTR debug_trap;
    exception(BX_DB_EXCEPTION, 0, 0); // no error, not interrupt
    }

  // Priority 3: External Hardware Interventions
  //   FLUSH
  //   STOPCLK
  //   SMI
  //   INIT
  // (bochs doesn't support these)

  // Priority 4: Traps on Previous Instruction
  //   Breakpoints
  //   Debug Trap Exceptions (TF flag set or data/IO breakpoint)
  if ( BX_CPU_THIS_PTR debug_trap &&
       !(BX_CPU_THIS_PTR inhibit_mask & BX_INHIBIT_DEBUG) ) {
    // A trap may be inhibited on this boundary due to an instruction
    // which loaded SS.  If so we clear the inhibit_mask below
    // and don't execute this code until the next boundary.
    // Commit debug events to DR6
    BX_CPU_THIS_PTR dr6 |= BX_CPU_THIS_PTR debug_trap;
    exception(BX_DB_EXCEPTION, 0, 0); // no error, not interrupt
    }

  // Priority 5: External Interrupts
  //   NMI Interrupts
  //   Maskable Hardware Interrupts
  if (BX_CPU_THIS_PTR inhibit_mask & BX_INHIBIT_INTERRUPTS) {
    // Processing external interrupts is inhibited on this
    // boundary because of certain instructions like STI.
    // inhibit_mask is cleared below, in which case we will have
    // an opportunity to check interrupts on the next instruction
    // boundary.
    }
  else if (BX_CPU_INTR && BX_CPU_THIS_PTR get_IF () &&
           BX_DBG_ASYNC_INTR) {
    Bit8u vector;

    // NOTE: similar code in ::take_irq()
#if BX_SUPPORT_APIC
    if (BX_CPU_THIS_PTR local_apic.INTR)
      vector = BX_CPU_THIS_PTR local_apic.acknowledge_int ();
    else
      vector = DEV_pic_iac(); // may set INTR with next interrupt
#else
    // if no local APIC, always acknowledge the PIC.
    vector = DEV_pic_iac(); // may set INTR with next interrupt
#endif
    //BX_DEBUG(("decode: interrupt %u",
    //                                   (unsigned) vector));
    BX_CPU_THIS_PTR errorno = 0;
    BX_CPU_THIS_PTR EXT   = 1; /* external event */
    interrupt(vector, 0, 0, 0);
    BX_INSTR_HWINTERRUPT(BX_CPU_ID, vector,
        BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value, EIP);
    // Set up environment, as would be when this main cpu loop gets
    // invoked.  At the end of normal instructions, we always commmit
    // the new EIP/ESP values.  But here, we call interrupt() much like
    // it was a sofware interrupt instruction, and need to effect the
    // commit here.  This code mirrors similar code above.
    BX_CPU_THIS_PTR prev_eip = RIP; // commit new RIP
    BX_CPU_THIS_PTR prev_esp = RSP; // commit new RSP
    BX_CPU_THIS_PTR EXT = 0;
    BX_CPU_THIS_PTR errorno = 0;
    }
  else if (BX_HRQ && BX_DBG_ASYNC_DMA) {
    // NOTE: similar code in ::take_dma()
    // assert Hold Acknowledge (HLDA) and go into a bus hold state
    DEV_dma_raise_hlda();
    }

  // Priority 6: Faults from fetching next instruction
  //   Code breakpoint fault
  //   Code segment limit violation (priority 7 on 486/Pentium)
  //   Code page fault (priority 7 on 486/Pentium)
  // (handled in main decode loop)

  // Priority 7: Faults from decoding next instruction
  //   Instruction length > 15 bytes
  //   Illegal opcode
  //   Coprocessor not available
  // (handled in main decode loop etc)

  // Priority 8: Faults on executing an instruction
  //   Floating point execution
  //   Overflow
  //   Bound error
  //   Invalid TSS
  //   Segment not present
  //   Stack fault
  //   General protection
  //   Data page fault
  //   Alignment check
  // (handled by rest of the code)


  if (BX_CPU_THIS_PTR get_TF ()) {
    // TF is set before execution of next instruction.  Schedule
    // a debug trap (#DB) after execution.  After completion of
    // next instruction, the code above will invoke the trap.
    BX_CPU_THIS_PTR debug_trap |= 0x00004000; // BS flag in DR6
    }

  // Now we can handle things which are synchronous to instruction
  // execution.
  if (BX_CPU_THIS_PTR get_RF ()) {
    BX_CPU_THIS_PTR clear_RF ();
    }
#if BX_X86_DEBUGGER
  else {
    // only bother comparing if any breakpoints enabled
    if ( BX_CPU_THIS_PTR dr7 & 0x000000ff ) {
      Bit32u iaddr =
        BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.base +
        BX_CPU_THIS_PTR prev_eip;
      Bit32u dr6_bits;
      if ( (dr6_bits = hwdebug_compare(iaddr, 1, BX_HWDebugInstruction,
                                       BX_HWDebugInstruction)) ) {
        // Add to the list of debug events thus far.
        BX_CPU_THIS_PTR debug_trap |= dr6_bits;
        BX_CPU_THIS_PTR async_event = 1;
        // If debug events are not inhibited on this boundary,
        // fire off a debug fault.  Otherwise handle it on the next
        // boundary. (becomes a trap)
        if ( !(BX_CPU_THIS_PTR inhibit_mask & BX_INHIBIT_DEBUG) ) {
          // Commit debug events to DR6
          BX_CPU_THIS_PTR dr6 = BX_CPU_THIS_PTR debug_trap;
          exception(BX_DB_EXCEPTION, 0, 0); // no error, not interrupt
          }
        }
      }
    }
#endif

  // We have ignored processing of external interrupts and
  // debug events on this boundary.  Reset the mask so they
  // will be processed on the next boundary.
  BX_CPU_THIS_PTR inhibit_mask = 0;

  if ( !(BX_CPU_INTR ||
         BX_CPU_THIS_PTR debug_trap ||
         BX_HRQ ||
         BX_CPU_THIS_PTR get_TF ()) )
    BX_CPU_THIS_PTR async_event = 0;

  return 0; // Continue executing cpu_loop.
}




// boundaries of consideration:
//
//  * physical memory boundary: 1024k (1Megabyte) (increments of...)
//  * A20 boundary:             1024k (1Megabyte)
//  * page boundary:            4k
//  * ROM boundary:             2k (dont care since we are only reading)
//  * segment boundary:         any



  void
BX_CPU_C::prefetch(void)
{
  // cs:eIP
  // prefetch QSIZE byte quantity aligned on corresponding boundary
  bx_address laddr;
  Bit32u pAddr;
  bx_address temp_rip;
  Bit32u temp_limit;
  bx_address laddrPageOffset0, eipPageOffset0;

  temp_rip   = RIP;
  temp_limit = BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled;

  laddr = BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.base +
                    temp_rip;

  if (((Bit32u)temp_rip) > temp_limit) {
    BX_PANIC(("prefetch: RIP > CS.limit"));
    }

#if BX_SUPPORT_PAGING
  if (BX_CPU_THIS_PTR cr0.pg) {
    // aligned block guaranteed to be all in one page, same A20 address
    pAddr = itranslate_linear(laddr, CPL==3);
    pAddr = A20ADDR(pAddr);
    }
  else
#endif // BX_SUPPORT_PAGING
    {
    pAddr = A20ADDR(laddr);
    }

  // check if segment boundary comes into play
  //if ((temp_limit - (Bit32u)temp_rip) < 4096) {
  //  }

  // Linear address at the beginning of the page.
#if BX_SUPPORT_X86_64
  laddrPageOffset0 = laddr & BX_CONST64(0xfffffffffffff000);
#else
  laddrPageOffset0 = laddr & 0xfffff000;
#endif
  // Calculate RIP at the beginning of the page.
  eipPageOffset0 = RIP - (laddr - laddrPageOffset0);
  BX_CPU_THIS_PTR eipPageBias = - eipPageOffset0;
  BX_CPU_THIS_PTR eipPageWindowSize = 4096; // FIXME:
  BX_CPU_THIS_PTR pAddrA20Page = pAddr & 0xfffff000;
  BX_CPU_THIS_PTR eipFetchPtr =
      BX_CPU_THIS_PTR mem->getHostMemAddr(BX_CPU_THIS, BX_CPU_THIS_PTR pAddrA20Page,
                                          BX_READ);

  // Sanity checks
  if ( !BX_CPU_THIS_PTR eipFetchPtr ) {
    if ( pAddr >= BX_CPU_THIS_PTR mem->len ) {
      BX_PANIC(("prefetch: running in bogus memory"));
      }
    else {
      BX_PANIC(("prefetch: getHostMemAddr vetoed direct read, pAddr=0x%x.",
                pAddr));
      }
    }
}


  void
BX_CPU_C::boundaryFetch(bxInstruction_c *i)
{
    unsigned j;
    Bit8u fetchBuffer[16]; // Really only need 15
    bx_address eipBiased, remainingInPage;
    Bit8u *fetchPtr;
    unsigned ret;

    eipBiased = RIP + BX_CPU_THIS_PTR eipPageBias;
    remainingInPage = (BX_CPU_THIS_PTR eipPageWindowSize - eipBiased);
    if (remainingInPage > 15) {
      BX_PANIC(("fetch_decode: remaining > max ilen"));
      }
    fetchPtr = BX_CPU_THIS_PTR eipFetchPtr + eipBiased;

    // Read all leftover bytes in current page up to boundary.
    for (j=0; j<remainingInPage; j++) {
      fetchBuffer[j] = *fetchPtr++;
      }

    // The 2nd chunk of the instruction is on the next page.
    // Set RIP to the 0th byte of the 2nd page, and force a
    // prefetch so direct access of that physical page is possible, and
    // all the associated info is updated.
    RIP += remainingInPage;
    prefetch();
    if (BX_CPU_THIS_PTR eipPageWindowSize < 15) {
      BX_PANIC(("fetch_decode: small window size after prefetch"));
      }

    // We can fetch straight from the 0th byte, which is eipFetchPtr;
    fetchPtr = BX_CPU_THIS_PTR eipFetchPtr;

    // read leftover bytes in next page
    for (; j<15; j++) {
      fetchBuffer[j] = *fetchPtr++;
      }
#if BX_SUPPORT_X86_64
    if (BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_64) {
      ret = fetchDecode64(fetchBuffer, i, 15);
      }
    else
#endif
      {
      ret = fetchDecode(fetchBuffer, i, 15);
      }
    // Restore EIP since we fudged it to start at the 2nd page boundary.
    RIP = BX_CPU_THIS_PTR prev_eip;
    if (ret==0)
      BX_PANIC(("fetchDecode: cross boundary: ret==0"));

// Since we cross an instruction boundary, note that we need a prefetch()
// again on the next instruction.  Perhaps we can optimize this to
// eliminate the extra prefetch() since we do it above, but have to
// think about repeated instructions, etc.
BX_CPU_THIS_PTR eipPageWindowSize = 0; // Fixme

  BX_INSTR_OPCODE(BX_CPU_ID, fetchBuffer, i->ilen(),
                  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.d_b);
}


#if 0
// Now a no-op.

  // If control has transfered locally, it is possible the prefetch Q is
  // still valid.  This would happen for repeat instructions, and small
  // branches.
  void
BX_CPU_C::revalidate_prefetch_q(void)
{
#ifdef __GNUC__
#warning "::revalidate_prefetch_q() is ifdef'd out."
#endif
  bx_address eipBiased;

  eipBiased = RIP + BX_CPU_THIS_PTR eipPageBias;
  if ( eipBiased < BX_CPU_THIS_PTR eipPageWindowSize ) {
    // Good, EIP still within prefetch window.
    }
  else {
    // EIP has branched outside the prefetch window.  Mark the
    // prefetch info as invalid, and requiring update.
    BX_CPU_THIS_PTR eipPageWindowSize = 0;
    }
}
#endif


#if BX_EXTERNAL_DEBUGGER

  void
BX_CPU_C::ask (int level, const char *prefix, const char *fmt, va_list ap)
{
  char buf1[1024];
  vsprintf (buf1, fmt, ap);
  printf ("%s %s\n", prefix, buf1);
  trap_debugger(1);
  //BX_CPU_THIS->logfunctions::ask(level,prefix,fmt,ap);
}

  void
BX_CPU_C::trap_debugger (bx_bool callnow)
{
  regs.debug_state = debug_step;
  if (callnow) {
    bx_external_debugger(BX_CPU_THIS);
  }
}

#endif  // #if BX_EXTERNAL_DEBUGGER


#if BX_DEBUGGER
extern unsigned int dbg_show_mask;

  bx_bool
BX_CPU_C::dbg_is_begin_instr_bpoint(Bit32u cs, Bit32u eip, Bit32u laddr,
                                    Bit32u is_32)
{
  //fprintf (stderr, "begin_instr_bp: checking cs:eip %04x:%08x\n", cs, eip);
  BX_CPU_THIS_PTR guard_found.cs  = cs;
  BX_CPU_THIS_PTR guard_found.eip = eip;
  BX_CPU_THIS_PTR guard_found.laddr = laddr;
  BX_CPU_THIS_PTR guard_found.is_32bit_code = is_32;

  // BW mode switch breakpoint
  // instruction which generate exceptions never reach the end of the
  // loop due to a long jump. Thats why we check at start of instr.
  // Downside is that we show the instruction about to be executed
  // (not the one generating the mode switch).
  if (BX_CPU_THIS_PTR mode_break && 
      (BX_CPU_THIS_PTR debug_vm != BX_CPU_THIS_PTR getB_VM ())) {
    BX_INFO(("Caught vm mode switch breakpoint"));
    BX_CPU_THIS_PTR debug_vm = BX_CPU_THIS_PTR getB_VM ();
    BX_CPU_THIS_PTR stop_reason = STOP_MODE_BREAK_POINT;
    return 1;
  }

  if( (BX_CPU_THIS_PTR show_flag) & (dbg_show_mask)) {
    int rv;
    if((rv = bx_dbg_symbolic_output()))
      return rv;
  }

  // see if debugger is looking for iaddr breakpoint of any type
  if (bx_guard.guard_for & BX_DBG_GUARD_IADDR_ALL) {
#if BX_DBG_SUPPORT_VIR_BPOINT
    if (bx_guard.guard_for & BX_DBG_GUARD_IADDR_VIR) {
      if (BX_CPU_THIS_PTR guard_found.icount!=0) {
        for (unsigned i=0; i<bx_guard.iaddr.num_virtual; i++) {
          if ( (bx_guard.iaddr.vir[i].cs  == cs) &&
               (bx_guard.iaddr.vir[i].eip == eip) ) {
            BX_CPU_THIS_PTR guard_found.guard_found = BX_DBG_GUARD_IADDR_VIR;
            BX_CPU_THIS_PTR guard_found.iaddr_index = i;
            return(1); // on a breakpoint
            }
          }
        }
      }
#endif
#if BX_DBG_SUPPORT_LIN_BPOINT
    if (bx_guard.guard_for & BX_DBG_GUARD_IADDR_LIN) {
      if (BX_CPU_THIS_PTR guard_found.icount!=0) {
        for (unsigned i=0; i<bx_guard.iaddr.num_linear; i++) {
          if ( bx_guard.iaddr.lin[i].addr == BX_CPU_THIS_PTR guard_found.laddr ) {
            BX_CPU_THIS_PTR guard_found.guard_found = BX_DBG_GUARD_IADDR_LIN;
            BX_CPU_THIS_PTR guard_found.iaddr_index = i;
            return(1); // on a breakpoint
            }
          }
        }
      }
#endif
#if BX_DBG_SUPPORT_PHY_BPOINT
    if (bx_guard.guard_for & BX_DBG_GUARD_IADDR_PHY) {
      Bit32u phy;
      bx_bool valid;
      dbg_xlate_linear2phy(BX_CPU_THIS_PTR guard_found.laddr,
                              &phy, &valid);
      // The "guard_found.icount!=0" condition allows you to step or
      // continue beyond a breakpoint.  Bryce tried removing it once,
      // and once you get to a breakpoint you are stuck there forever.
      // Not pretty.
      if (valid && (BX_CPU_THIS_PTR guard_found.icount!=0)) {
        for (unsigned i=0; i<bx_guard.iaddr.num_physical; i++) {
          if ( bx_guard.iaddr.phy[i].addr == phy ) {
            BX_CPU_THIS_PTR guard_found.guard_found = BX_DBG_GUARD_IADDR_PHY;
            BX_CPU_THIS_PTR guard_found.iaddr_index = i;
            return(1); // on a breakpoint
            }
          }
        }
      }
#endif
    }
  return(0); // not on a breakpoint
}


  bx_bool
BX_CPU_C::dbg_is_end_instr_bpoint(Bit32u cs, Bit32u eip, Bit32u laddr,
                                  Bit32u is_32)
{
  //fprintf (stderr, "end_instr_bp: checking for icount or ^C\n");
  BX_CPU_THIS_PTR guard_found.icount++;

  // convenient point to see if user typed Ctrl-C
  if (bx_guard.interrupt_requested &&
      (bx_guard.guard_for & BX_DBG_GUARD_CTRL_C)) {
    BX_CPU_THIS_PTR guard_found.guard_found = BX_DBG_GUARD_CTRL_C;
    return(1);
    }

  // see if debugger requesting icount guard
  if (bx_guard.guard_for & BX_DBG_GUARD_ICOUNT) {
    if (BX_CPU_THIS_PTR guard_found.icount >= bx_guard.icount) {
      BX_CPU_THIS_PTR guard_found.cs  = cs;
      BX_CPU_THIS_PTR guard_found.eip = eip;
      BX_CPU_THIS_PTR guard_found.laddr = laddr;
      BX_CPU_THIS_PTR guard_found.is_32bit_code = is_32;
      BX_CPU_THIS_PTR guard_found.guard_found = BX_DBG_GUARD_ICOUNT;
      return(1);
      }
    }

#if (BX_NUM_SIMULATORS >= 2)
  // if async event pending, acknowlege them
  if (bx_guard.async_changes_pending.which) {
    if (bx_guard.async_changes_pending.which & BX_DBG_ASYNC_PENDING_A20)
      bx_dbg_async_pin_ack(BX_DBG_ASYNC_PENDING_A20,
                           bx_guard.async_changes_pending.a20);
    if (bx_guard.async_changes_pending.which) {
      BX_PANIC(("decode: async pending unrecognized."));
      }
    }
#endif
  return(0); // no breakpoint
}


  void
BX_CPU_C::dbg_take_irq(void)
{
  unsigned vector;

  // NOTE: similar code in ::cpu_loop()

  if ( BX_CPU_INTR && BX_CPU_THIS_PTR get_IF () ) {
    if ( setjmp(BX_CPU_THIS_PTR jmp_buf_env) == 0 ) {
      // normal return from setjmp setup
      vector = DEV_pic_iac(); // may set INTR with next interrupt
      BX_CPU_THIS_PTR errorno = 0;
      BX_CPU_THIS_PTR EXT   = 1; // external event
      BX_CPU_THIS_PTR async_event = 1; // set in case INTR is triggered
      interrupt(vector, 0, 0, 0);
      }
    }
}

  void
BX_CPU_C::dbg_force_interrupt(unsigned vector)
{
  // Used to force slave simulator to take an interrupt, without
  // regard to IF

  if ( setjmp(BX_CPU_THIS_PTR jmp_buf_env) == 0 ) {
    // normal return from setjmp setup
    BX_CPU_THIS_PTR errorno = 0;
    BX_CPU_THIS_PTR EXT   = 1; // external event
    BX_CPU_THIS_PTR async_event = 1; // probably don't need this
    interrupt(vector, 0, 0, 0);
    }
}

  void
BX_CPU_C::dbg_take_dma(void)
{
  // NOTE: similar code in ::cpu_loop()
  if ( BX_HRQ ) {
    BX_CPU_THIS_PTR async_event = 1; // set in case INTR is triggered
    DEV_dma_raise_hlda();
    }
}
#endif  // #if BX_DEBUGGER


void
BX_CPU_C::register_state(bx_param_c *list_p)
{
  BXRS_START(BX_CPU_C, BX_CPU_THIS, "", list_p, 100);
  {
    BXRS_ARRAY_ENUM(char, name, 64);
    
    BXRS_NUM(unsigned, bx_cpuid);

#   if BX_SUPPORT_X86_64
    {
      BXRS_ARRAY_OBJ(bx_gen_reg_t, gen_reg, 16);

      BXRS_UNION_START;
      {
#       ifdef BX_BIG_ENDIAN
        BXRS_STRUCT_START(ip_dword_t, dword);
        {
          BXRS_NUM(Bit32u, rip_upper);
          BXRS_NUM(Bit32u, eip);
        }
        BXRS_STRUCT_END;
#       else // #ifdef BX_BIG_ENDIAN
        BXRS_STRUCT_START(ip_dword_t, dword);
        {
          BXRS_NUM(Bit32u, eip);
          BXRS_NUM(Bit32u, rip_upper);
        }
        BXRS_STRUCT_END;
#       endif // #ifdef BX_BIG_ENDIAN #else
        Bit64u rip;
      }
      BXRS_UNION_END;
    }
#   else
    {
      BXRS_ARRAY_OBJ(bx_gen_reg_t, gen_reg, 8);

      BXRS_STRUCT_START(ip_dword_t, dword);
      {
        BXRS_NUM_D(Bit32u, eip, "instruction pointer");
      }
      BXRS_STRUCT_END;
    }
#   endif
    
#   if BX_CPU_LEVEL > 0
    BXRS_NUM(bx_address, prev_eip);
#   endif
    
    // status and control flags register set
    // BJS TODO: Bit32u   lf_flags_status;
    // BJS TODO: bx_flags_reg_t eflags;
    // BJS TODO: 
    // BJS TODO: bx_lf_flags_entry oszapc;
    // BJS TODO: bx_lf_flags_entry oszap;
    
    BXRS_NUM(bx_address, prev_esp);

    BXRS_NUM_D(unsigned, inhibit_mask, "What events to inhibit at any given time");

    BXRS_ARRAY_OBJ_D(bx_segment_reg_t, sregs, 6, "user segment register set");

    /* system segment registers */
#   if BX_CPU_LEVEL >= 2
    BXRS_STRUCT_START_D(bx_global_segment_reg_t, gdtr, 
                        "global descriptor table register");
    {
      BXRS_NUM_D(bx_address, base , 
                 "base address: 24bits=286,32bits=386,64bits=x86-64");
      BXRS_NUM_D(Bit16u    , limit, 
                 "limit, 16bits");
    }
    BXRS_STRUCT_END;

    BXRS_STRUCT_START_D(bx_global_segment_reg_t, idtr, 
                        "global descriptor table register");
    {
      BXRS_NUM_D(bx_address, base , 
                 "base address: 24bits=286,32bits=386,64bits=x86-64");
      BXRS_NUM_D(Bit16u    , limit, 
                 "limit, 16bits");
    }
    BXRS_STRUCT_END;
#   endif

    BXRS_OBJ_D(bx_segment_reg_t, ldtr, "interrupt descriptor table register");
    BXRS_OBJ_D(bx_segment_reg_t, tr,   "task register");
    

#   if BX_CPU_LEVEL >= 3
    BXRS_NUM(Bit32u, dr0);
    BXRS_NUM(Bit32u, dr1);
    BXRS_NUM(Bit32u, dr2);
    BXRS_NUM(Bit32u, dr3);
    BXRS_NUM(Bit32u, dr6);
    BXRS_NUM(Bit32u, dr7);
#   endif
    
#   if BX_CPU_LEVEL >= 2
    {
      BXRS_OBJ(bx_cr0_t, cr0);
      
      BXRS_NUM_D(unsigned, protectedMode, "CR0.PE=1, EFLAGS.VM=0");
      BXRS_NUM_D(unsigned, v8086Mode    , "CR0.PE=1, EFLAGS.VM=1");
      BXRS_NUM_D(unsigned, realMode     , "CR0.PE=1");
      
      BXRS_NUM(Bit32u    , cr1);
      BXRS_NUM(bx_address, cr2);
      BXRS_NUM(bx_address, cr3);
    }
#   endif
    
#   if BX_CPU_LEVEL >= 4
    BXRS_STRUCT_START(bx_cr4_t, cr4);
    {
      BXRS_NUM_D(Bit32u, registerValue, "32bit value of register");
    }
    BXRS_STRUCT_END;
#   endif  // #if BX_CPU_LEVEL >= 4
    
#   if BX_CPU_LEVEL >= 5
    BXRS_OBJ(bx_regs_msr_t, msr);
#   endif
    
#warning FPU registration not implemented
    // BJS TODO: BXRS_OBJ(i387_t, the_i387);

#if BX_SUPPORT_SSE
#warning SSE state registration not tested
    BXRS_ARRAY_OBJ(bx_xmm_reg_t, xmm, BX_XMM_REGISTERS);
    BXRS_OBJ(bx_mxcsr_t, mxcsr);
#endif

    BXRS_OBJP(BX_MEM_C, mem);

    BXRS_BOOL (bx_bool, EXT);
    BXRS_NUM_D(unsigned, errorno, "signal exception during instruction emulation");

    BXRS_NUM_D(Bit32u , debug_trap, "holds DR6 value to be set as well");
    BXRS_BOOL (bx_bool, async_event);
    BXRS_BOOL (bx_bool, INTR);
    BXRS_BOOL (bx_bool, kill_bochs_request);

    BXRS_BOOL_D(bx_bool, bsp, "wether this CPU is the BSP always set for UP");

    // for accessing registers by index number
    // BJS TODO: dont think we need to save these
    // Bit16u *_16bit_base_reg[8];
    // Bit16u *_16bit_index_reg[8];
    // Bit32u empty_register;

    // for decoding instructions; accessing seg reg's by index
    BXRS_ARRAY_NUM(unsigned, sreg_mod00_rm16, 8);
    BXRS_ARRAY_NUM(unsigned, sreg_mod01_rm16, 8);
    BXRS_ARRAY_NUM(unsigned, sreg_mod10_rm16, 8);
#   if BX_SUPPORT_X86_64   
    BXRS_ARRAY_NUM(unsigned, sreg_mod01_rm32, 16);
    BXRS_ARRAY_NUM(unsigned, sreg_mod10_rm32, 16);
    BXRS_ARRAY_NUM(unsigned, sreg_mod0_base32, 16);
    BXRS_ARRAY_NUM(unsigned, sreg_mod1or2_base32, 16);
#   else                   
    BXRS_ARRAY_NUM(unsigned, sreg_mod01_rm32, 8);
    BXRS_ARRAY_NUM(unsigned, sreg_mod10_rm32, 8);
    BXRS_ARRAY_NUM(unsigned, sreg_mod0_base32, 8);
    BXRS_ARRAY_NUM(unsigned, sreg_mod1or2_base32, 8);
#   endif

    // for exceptions
    // BJS TODO: dont think we should save this...should we?
    // jmp_buf jmp_buf_env;
    BXRS_ARRAY_NUM(Bit8u, curr_exception, 2);

    // BJS TODO: dont think we we need to save this
    // static const bx_bool is_exception_OK[3][3];

    BXRS_OBJ(bx_segment_reg_t, save_cs);
    BXRS_OBJ(bx_segment_reg_t, save_ss);
    BXRS_NUM(Bit32u, save_eip);
    BXRS_NUM(Bit32u, save_esp);

    // Boundaries of current page, based on EIP
    BXRS_NUM(bx_address, eipPageBias);
    BXRS_NUM(bx_address, eipPageWindowSize);

    // BJS TODO: invalidate prefetch_q
    //Bit8u     *eipFetchPtr;
    
    BXRS_NUM_D(Bit32u, pAddrA20Page, "Guest physical address of current instruction");

#if BX_SUPPORT_X86_64
    // for x86-64  (MODE_IA32,MODE_LONG,MODE_64)
    BXRS_NUM(unsigned, cpu_mode);
#else
    // x86-32 is always in IA32 mode.
    // BJS TODO: dont think we we need to save this
    //enum { cpu_mode = BX_MODE_IA32 };
#endif

#if BX_DEBUGGER
    BXRS_NUM(Bit32u, watchpoint);
    BXRS_NUM(Bit8u, break_point);
#ifdef MAGIC_BREAKPOINT
    BXRS_NUM(Bit8u, magic_break);
#endif
    BXRS_NUM   (Bit8u, stop_reason);
    BXRS_NUM   (Bit8u, trace);
    BXRS_NUM   (Bit8u, trace_reg);
    BXRS_NUM   (Bit8u, mode_break, "BW");
    BXRS_BOOL_D(bx_bool, debug_vm, "BW contains current mode");
    BXRS_NUM_D (Bit8u, show_eip,   "BW record eip at special instr f.ex eip");
    BXRS_NUM_D (Bit8u, show_flag,  "BW shows instr class executed");
    BXRS_OBJ(bx_guard_found_t, guard_found);
#endif

#if BX_SUPPORT_X86_64
#define TLB_GENERATION_MAX (BX_TLB_SIZE-1)
#endif

    // SYSENTER/SYSEXIT instruction msr's
#if BX_SUPPORT_SEP
    BXRS_NUM(Bit32u, sysenter_cs_msr);
    BXRS_NUM(Bit32u, sysenter_esp_msr);
    BXRS_NUM(Bit32u, sysenter_eip_msr);
#endif

    // for paging
#if BX_USE_TLB
    BXRS_STRUCT_START(TLB_t, TLB);
    {
      BXRS_ARRAY_START(bx_TLB_entry, entry, BX_TLB_SIZE);
      {
        BXRS_NUM_D(bx_address, lpf, "linear page frame");
        BXRS_NUM_D(Bit32u, ppf, "physical page frame");
        BXRS_NUM_D(Bit32u, accessBits, "Page Table Address for updating A & D bits");
        BXRS_NUM  (bx_hostpageaddr_t, hostPageAddr);
      }
      BXRS_ARRAY_END;
#if BX_USE_QUICK_TLB_INVALIDATE
      BXRS_NUM(Bit32u, tlb_invalidate);
#endif
    }
    BXRS_END;
#endif  // #if BX_USE_TLB


    // An instruction cache.  Each entry should be exactly 32 bytes, and
    // this structure should be aligned on a 32-byte boundary to be friendly
    // with the host cache lines.
#if BX_SupportICache
#warning bxICache_c state registration is not implemented
    // BJS TODO: implement registration of bxICache_c iCache  BX_CPP_AlignN(32);
#endif


    BXRS_STRUCT_START(struct address_xlation_t, address_xlation);
    {
      BXRS_NUM_D(bx_address,  rm_addr, "The address offset after resolution.");
      BXRS_NUM_D(Bit32u,  paddress1,   "phys addr after translation of 1st len1 bytes of data");
      BXRS_NUM_D(Bit32u,  paddress2,   "phys addr after translation of 2nd len2 bytes of data");
      BXRS_NUM_D(Bit32u,  len1,        "Number of bytes in page 1");
      BXRS_NUM_D(Bit32u,  len2,        "Number of bytes in page 2");
      BXRS_NUM_D(bx_ptr_equiv_t, pages,"Number of pages access spans (1 or 2).");
    }
    BXRS_STRUCT_END;

#if BX_SUPPORT_X86_64
    // BJS TODO: dont think we need to register this
    // data upper 32 bits - not used any longer
    //Bit32s daddr_upper;    // upper bits must be canonical  (-virtmax --> + virtmax)
    // instruction upper 32 bits - not used any longer
    //Bit32s iaddr_upper;    // upper bits must be canonical  (-virtmax --> + virtmax)
#endif


#warning TESTME bbd: test compile in SMP mode to test apic.cc changes
#if BX_SUPPORT_APIC
#warning local_apic state registration not implemented
    // BJS TODO: implement state registration of bx_local_apic_c local_apic
#endif

  }
  BXRS_END;
}

#if BX_CPU_LEVEL >= 2
void
bx_cr0_t::register_state(bx_param_c *list_p)
{
  BXRS_START(bx_cr0_t, this, "", list_p, 15);
  {
    BXRS_NUM_D(Bit32u, val32, "32bit value of register");
    
#   if BX_CPU_LEVEL >= 3
    BXRS_BOOL_D(bx_bool, pg, "paging");
#   endif
      
#   if BX_CPU_LEVEL >= 4
    BXRS_BOOL_D(bx_bool, cd, "cache disable");
    BXRS_BOOL_D(bx_bool, nw, "no write-through");
    BXRS_BOOL_D(bx_bool, am, "alignment mask");
    BXRS_BOOL_D(bx_bool, wp, "write-protect");
    BXRS_BOOL_D(bx_bool, ne, "numerics exception");
#   endif
    
    BXRS_BOOL_D(bx_bool, ts, "task switched");
    BXRS_BOOL_D(bx_bool, em, "emulate math coprocessor");
    BXRS_BOOL_D(bx_bool, mp, "monitor coprocessor");
    BXRS_BOOL_D(bx_bool, pe, "protected mode enable");
  }
  BXRS_END;
}
#endif // #if BX_CPU_LEVEL >= 2


void
bx_gen_reg_t::register_state(bx_param_c *list_p)
{
# if BX_SUPPORT_X86_64
  {
#   ifdef BX_BIG_ENDIAN
    {
      BXRS_START(bx_gen_reg_t, this, "General register set", list_p, 5);
      {
        BXRS_UNION_START;
        {
          BXRS_STRUCT_START(struct bx_gen_reg_t::word_t, word);
          {
            BXRS_NUM(Bit32u, dword_filler);
            BXRS_NUM(Bit16u, word_filler);
            BXRS_UNION_START; 
            {
              BXRS_NUM(Bit16u, rx);
              BXRS_STRUCT_START(struct bx_gen_reg_t::word_t::byte_t, byte);
              {
                BXRS_NUM(Bit8u, rh);
                BXRS_NUM(Bit8u, rl);
              }
              BXRS_STRUCT_END;
            };
            BXRS_UNION_END;
          }
          BXRS_STRUCT_END;
          
          BXRS_NUM(Bit64u, rrx);
          BXRS_STRUCT_START(struct bx_gen_reg_t::dword_t, dword);
          {
            BXRS_NUM_D(Bit32u, hrx, "hi 32 bits");  
            BXRS_NUM_D(Bit32u, erx, "low 32 bits"); 
          }
          BXRS_STRUCT_END;
        };
        BXRS_UNION_END;
      }
      BXRS_END;
    }
#   else // #ifdef BX_BIG_ENDIAN
    {
      BXRS_START(bx_gen_reg_t, BX_CPU_THIS, "General register set", list_p, 5);
      {
        BXRS_UNION_START;
        {
          BXRS_STRUCT_START(struct bx_gen_reg_t::word_t, word); 
          {
            BXRS_UNION_START; 
            {
              BXRS_NUM(Bit16u, rx);
              BXRS_STRUCT_START(struct bx_gen_reg_t::word_t::byte_t, byte);
              {
                BXRS_NUM(Bit8u, rl);
                BXRS_NUM(Bit8u, rh);
              }
              BXRS_STRUCT_END;
            }
            BXRS_UNION_END;
            BXRS_NUM(Bit16u, word_filler);
            BXRS_NUM(Bit32u, dword_filler);
          }
          BXRS_STRUCT_END;
          
          BXRS_NUM(Bit64u, rrx);
          BXRS_STRUCT_START(struct bx_gen_reg_t::dword_t, dword);
          {
            BXRS_NUM_D(Bit32u, erx, "low 32 bits");
            BXRS_NUM_D(Bit32u, hrx, "hi 32 bits"); 
          }
          BXRS_STRUCT_END;
        }
        BXRS_UNION_END;
      }
      BXRS_END;
    }
#   endif // #ifdef BX_BIG_ENDIAN #else
  }
# else // #if BX_SUPPORT_X86_64
  {
#   ifdef BX_BIG_ENDIAN
    {
      BXRS_START(bx_gen_reg_t, this, "General register set", list_p, 5);
      {
        BXRS_UNION_START;
        {
          BXRS_STRUCT_START(bx_gen_reg_t::dword_t, dword);
          {
            BXRS_NUM(Bit32u, erx);
          }
          BXRS_STRUCT_END;
          
          BXRS_STRUCT_START(bx_gen_reg_t::word_t, word);
          {
            BXRS_NUM(Bit16u, word_filler);
            BXRS_UNION_START;
            {
              BXRS_NUM(Bit16u, rx);
              BXRS_STRUCT_START(bx_gen_reg_t::word_t::byte_t, byte);
              {
                BXRS_NUM(Bit8u, rh);
                BXRS_NUM(Bit8u, rl);
              }
              BXRS_STRUCT_END;
            }
            BXRS_UNION_END;
          }
          BXRS_STRUCT_END;
        }
        BXRS_UNION_END;
      }
      BXRS_END;
    }
#   else  // #ifdef BX_BIG_ENDIAN
    {
      BXRS_START(bx_gen_reg_t, this, "General register set", list_p, 5);
      {
        BXRS_UNION_START;
        {
          BXRS_STRUCT_START(bx_gen_reg_t::gr_dword_t, dword);
          {
            BXRS_NUM(Bit32u, erx);
          }
          BXRS_STRUCT_END;
          
          BXRS_STRUCT_START(bx_gen_reg_t::gr_word_t, word);
          {
            BXRS_UNION_START;
            {
              BXRS_NUM(Bit16u, rx);
              BXRS_STRUCT_START(bx_gen_reg_t::gr_word_t::gr_byte_t, byte);
              {
                BXRS_NUM(Bit8u, rl);
                BXRS_NUM(Bit8u, rh);
              }
              BXRS_STRUCT_END;
            }
            BXRS_UNION_END;
            
            BXRS_NUM(Bit16u, word_filler);
          }
          BXRS_STRUCT_END;
        }
        BXRS_UNION_END;
      }
      BXRS_END;
    }
#   endif // #ifdef BX_BIG_ENDIAN #else
  }
# endif // #if BX_SUPPORT_X86_64 #else
  
}


void
bx_segment_reg_t::register_state(bx_param_c *list_p)
{
  BXRS_START(bx_segment_reg_t, BX_CPU_THIS, "", list_p, 25);
  {
    BXRS_STRUCT_START(bx_selector_t, selector);
    {
      BXRS_NUM_D(Bit16u, value, "the 16bit value of the selector");
#     if BX_CPU_LEVEL >= 2     
      BXRS_NUM_D(Bit16u, index, "13bit index extracted from value in protected mode");
      BXRS_NUM_D(Bit8u , ti   , "table indicator bit extracted from value");
      BXRS_NUM_D(Bit8u , rpl  , "RPL extracted from value");
#     endif
    }
    BXRS_STRUCT_END;

    BXRS_STRUCT_START(bx_descriptor_t, cache);
    {
      BXRS_BOOL  (bx_bool, valid);
      BXRS_BOOL_D(bx_bool, p      , "/* present");
      BXRS_NUM_D (Bit8u  , dpl    , "/* descriptor privilege level 0..3");
      BXRS_BOOL_D(bx_bool, segment, "/* 0 = system/gate, 1 = data/code segment");
      BXRS_NUM_D (Bit8u  , type   , "/* For system & gate descriptors, only");
      BXRS_STRUCT_START(bx_descriptor_t::desc_u_t, u);
      {
        BXRS_STRUCT_START(bx_descriptor_t::segment_t, segment);
        {
          BXRS_BOOL_D(bx_bool, executable,  "1=code, 0=data or stack segment");
          BXRS_BOOL_D(bx_bool, c_ed,        "code: 1=conforming, data/stack: 1=expand down");
          BXRS_BOOL_D(bx_bool, r_w,         "code: readable?, data/stack: writeable?");
          BXRS_BOOL_D(bx_bool, a,           "accessed?");
          BXRS_NUM_D (bx_address,  base,    "base address: 286=24bits, 386=32bits, long=64");
          BXRS_NUM_D (Bit32u,  limit,       "limit: 286=16bits, 386=20bits");
          BXRS_NUM_D (Bit32u,  limit_scaled,"for efficiency");
#         if BX_CPU_LEVEL >= 3                       
          BXRS_BOOL_D(bx_bool, g,           "granularity: 0=byte, 1=4K (page)");
          BXRS_BOOL_D(bx_bool, d_b,         "default size: 0=16bit, 1=32bit");
#         if BX_SUPPORT_X86_64                       
          BXRS_BOOL_D(bx_bool, l,           "long mode: 0=compat, 1=64 bit");
#         endif                                      
          BXRS_BOOL_D(bx_bool, avl,         "available for use by system");
#         endif
        }
        BXRS_STRUCT_END;

        BXRS_STRUCT_START(bx_descriptor_t::gate286_t, gate286);
        {
          BXRS_NUM_D(Bit8u ,  word_count, "5bits (0..31) (call gates only)");
          BXRS_NUM  (Bit16u,  dest_selector);
          BXRS_NUM  (Bit16u,  dest_offset);
        }
        BXRS_STRUCT_END;

        BXRS_STRUCT_START_D(bx_descriptor_t::taskgate_t, taskgate, "type 5: Task Gate Descriptor");
        {
          BXRS_NUM_D(Bit16u, tss_selector, "TSS segment selector");
        }
        BXRS_STRUCT_END;

#       if BX_CPU_LEVEL >= 3
        BXRS_STRUCT_START(bx_descriptor_t::gate386_t, gate386);
        {
          BXRS_NUM_D(Bit8u , dword_count, "5bits (0..31) (call gates only)");
          BXRS_NUM  (Bit16u, dest_selector);
          BXRS_NUM  (Bit32u, dest_offset);
        }
        BXRS_STRUCT_END;
#       endif

        BXRS_STRUCT_START(bx_descriptor_t::tss286_t, tss286);
        {
          BXRS_NUM_D(Bit32u, base, "24 bit 286 TSS base");
          BXRS_NUM_D(Bit16u, limit, "16 bit 286 TSS limit");
        }
        BXRS_STRUCT_END;

#       if BX_CPU_LEVEL >= 3
        BXRS_STRUCT_START(bx_descriptor_t::tss386_t, tss386);
        {
          BXRS_NUM_D (bx_address, base,     "32/64 bit 386 TSS base");
          BXRS_NUM_D (Bit32u, limit,        "20 bit 386 TSS limit");
          BXRS_NUM_D (Bit32u, limit_scaled, "Same notes as for \'segment\' field");
          BXRS_BOOL_D(bx_bool, g,           "granularity: 0=byte, 1=4K (page)");
          BXRS_BOOL_D(bx_bool, avl,         "available for use by system");
        }
        BXRS_STRUCT_END;
#       endif

        BXRS_STRUCT_START(bx_descriptor_t::ldt_t, ldt); 
        {
          BXRS_NUM_D (bx_address, base, "286=24 386+ =32/64 bit LDT base");
          BXRS_NUM_D (Bit16u, limit, "286+ =16 bit LDT limit");
        }
        BXRS_STRUCT_END;

      }
      BXRS_STRUCT_END;
    } 
    BXRS_STRUCT_END;
  }
  BXRS_END;
}


#if BX_CPU_LEVEL >= 5
void 
bx_regs_msr_t::register_state(bx_param_c *list_p)
{
  BXRS_START(bx_regs_msr_t, this, "", list_p, 10);
  {
    BXRS_NUM(Bit64u, apicbase);
#   if BX_SUPPORT_X86_64
    // x86-64 EFER bits
    BXRS_BOOL(bx_bool, sce);
    BXRS_BOOL(bx_bool, lme);
    BXRS_BOOL(bx_bool, lma);
    
    BXRS_NUM(Bit64u, star);
    BXRS_NUM(Bit64u, lstar);
    BXRS_NUM(Bit64u, cstar);
    BXRS_NUM(Bit64u, fmask);
    BXRS_NUM(Bit64u, kernelgsbase);
#   endif
  }
  BXRS_END;
}
#endif
