/*---------------------------------------------------------------------------+
 |  status_w.h                                                               |
 |  $Id: status_w.h,v 1.6.8.3 2004/03/20 19:53:32 sshwarts Exp $
 |                                                                           |
 | Copyright (C) 1992,1993                                                   |
 |                       W. Metzenthen, 22 Parker St, Ormond, Vic 3163,      |
 |                       Australia.  E-mail   billm@vaxc.cc.monash.edu.au    |
 |                                                                           |
 +---------------------------------------------------------------------------*/

#ifndef _STATUS_H_
#define _STATUS_H_

#define SW_Backward    	(0x8000)	/* backward compatibility */
#define SW_C3		(0x4000)	/* condition bit 3 */
#define SW_Top		(0x3800)	/* top of stack */
#define SW_C2		(0x0400)	/* condition bit 2 */
#define SW_C1		(0x0200)	/* condition bit 1 */
#define SW_C0		(0x0100)	/* condition bit 0 */
#define SW_Summary     	(0x0080)	/* exception summary */
#define SW_Stack_Fault	(0x0040)	/* stack fault */
#define SW_Precision   	(0x0020)	/* loss of precision */
#define SW_Underflow   	(0x0010)	/* underflow */
#define SW_Overflow    	(0x0008)	/* overflow */
#define SW_Zero_Div    	(0x0004)	/* divide by zero */
#define SW_Denorm_Op   	(0x0002)	/* denormalized operand */
#define SW_Invalid     	(0x0001)	/* invalid operation */

#define SW_Exc_Mask     (0x27f)  /* Status word exception bit mask */

/*
 * bbd: use do {...} while (0) structure instead of using curly brackets
 * inside parens, which most compilers do not like.
 */
#define setcc(cc) do { \
  FPU_partial_status &= ~(SW_C0|SW_C1|SW_C2|SW_C3); \
  FPU_partial_status |= (cc) & (SW_C0|SW_C1|SW_C2|SW_C3); } while(0)

#define clear_C1()  { FPU_partial_status &= ~SW_C1; }

#endif /* _STATUS_H_ */
