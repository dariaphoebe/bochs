/////////////////////////////////////////////////////////////////////////
// $Id: parallel.h,v 1.12.12.1 2004/11/05 00:56:46 slechta Exp $
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


#if BX_USE_PAR_SMF
#  define BX_PAR_SMF  static
#  define BX_PAR_THIS theParallelDevice->
#  define BX_PAR_THISP theParallelDevice
#else
#  define BX_PAR_SMF
#  define BX_PAR_THIS this->
#  define BX_PAR_THISP this
#endif

#define BX_PARPORT_MAXDEV   2

#define BX_PAR_DATA  0
#define BX_PAR_STAT  1
#define BX_PAR_CTRL  2

typedef struct {
  Bit8u data;
  struct STATUS_t {
    bx_bool error;
    bx_bool slct;
    bx_bool pe;
    bx_bool ack;
    bx_bool busy;
  } STATUS;
  struct CONTROL_t {
    bx_bool strobe;
    bx_bool autofeed;
    bx_bool init;
    bx_bool slct_in;
    bx_bool irq;
    bx_bool input;
  } CONTROL;
  Bit8u IRQ;
  FILE *output;
  bx_bool initmode;
} bx_par_t;



class bx_parallel_c : public bx_devmodel_c {
public:

  bx_parallel_c(void);
  ~bx_parallel_c(void);
  virtual void   init(void);
  virtual void   reset(unsigned type);

#if BX_SAVE_RESTORE
  virtual void register_state(sr_param_c *list_p);
  virtual void   before_save_state () {};
  virtual void   after_restore_state () {};
#endif

private:
  bx_par_t s[BX_PARPORT_MAXDEV];

  static void   virtual_printer(Bit8u port);

  static Bit32u read_handler(void *this_ptr, Bit32u address, unsigned io_len);
  static void   write_handler(void *this_ptr, Bit32u address, Bit32u value, unsigned io_len);
#if !BX_USE_PAR_SMF
  Bit32u read(Bit32u address, unsigned io_len);
  void   write(Bit32u address, Bit32u value, unsigned io_len);
#endif
  };
