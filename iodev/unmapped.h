/////////////////////////////////////////////////////////////////////////
// $Id: unmapped.h,v 1.10.6.3 2003/04/06 17:29:49 bdenney Exp $
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




#if BX_USE_UM_SMF
#  define BX_UM_SMF  static
#  define BX_UM_THIS_PTR theUnmappedDevice->
#  define BX_UM_THIS theUnmappedDevice
#else
#  define BX_UM_SMF
#  define BX_UM_THIS_PTR this->
#  define BX_UM_THIS this
#endif



class bx_unmapped_c : public bx_devmodel_c {
public:
  bx_unmapped_c(void);
  ~bx_unmapped_c(void);

  virtual void init(void);
  virtual void reset (unsigned type);
  virtual void register_state(bx_param_c *list_p);
  virtual void before_save_state ();
  virtual void after_restore_state ();

private:

  static Bit32u read_handler(void *this_ptr, Bit32u address, unsigned io_len);
  static void   write_handler(void *this_ptr, Bit32u address, Bit32u value, unsigned io_len);
#if !BX_USE_UM_SMF
  Bit32u read(Bit32u address, unsigned io_len);
  void   write(Bit32u address, Bit32u value, unsigned io_len);
#endif


  struct s_t {
    Bit8u port80;
    Bit8u port8e;
    Bit8u shutdown;
    } s;  // state information

  };
