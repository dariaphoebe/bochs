/////////////////////////////////////////////////////////////////////////
// $Id: config.cc,v 1.4 2004/07/02 23:18:21 vruppert Exp $
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2002  MandrakeSoft S.A.
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

#include "bochs.h"
#include "iodev/iodev.h"
#include <assert.h>

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#if defined(macintosh)
// Work around a bug in SDL 1.2.4 on MacOS X, which redefines getenv to
// SDL_getenv, but then neglects to provide SDL_getenv.  It happens
// because we are defining -Dmacintosh.
#undef getenv
#endif

// BX_SHARE_PATH should be defined by the makefile.  If not, give it
// a value of NULL to avoid compile problems.
#ifndef BX_SHARE_PATH
#define BX_SHARE_PATH NULL
#endif


int bochsrc_include_count = 0;

#define LOG_THIS genlog->

extern bx_debug_t bx_dbg;

bx_options_t bx_options; // initialized in bx_init_options()

static Bit32s parse_line_unformatted(char *context, char *line);
static Bit32s parse_line_formatted(char *context, int num_params, char *params[]);
static int parse_bochsrc(char *rcfile);

static Bit64s
bx_param_handler (bx_param_c *param, int set, Bit64s val)
{
  bx_id id = param->get_id ();
  switch (id) {
    case BXP_VGA_UPDATE_INTERVAL:
      // if after init, notify the vga device to change its timer.
      if (set && SIM->get_init_done ())
        DEV_vga_set_update_interval (val);
      break;
    case BXP_MOUSE_ENABLED:
      // if after init, notify the GUI
      if (set && SIM->get_init_done ()) {
        bx_gui->mouse_enabled_changed (val!=0);
        DEV_mouse_enabled_changed (val!=0);
      }
      break;
    case BXP_NE2K_PRESENT:
      if (set) {
        int enable = (val != 0);
        SIM->get_param (BXP_NE2K_IOADDR)->set_enabled (enable);
        SIM->get_param (BXP_NE2K_IRQ)->set_enabled (enable);
        SIM->get_param (BXP_NE2K_MACADDR)->set_enabled (enable);
        SIM->get_param (BXP_NE2K_ETHMOD)->set_enabled (enable);
        SIM->get_param (BXP_NE2K_ETHDEV)->set_enabled (enable);
        SIM->get_param (BXP_NE2K_SCRIPT)->set_enabled (enable);
      }
      break;
    case BXP_LOAD32BITOS_WHICH:
      if (set) {
        int enable = (val != Load32bitOSNone);
        SIM->get_param (BXP_LOAD32BITOS_PATH)->set_enabled (enable);
        SIM->get_param (BXP_LOAD32BITOS_IOLOG)->set_enabled (enable);
        SIM->get_param (BXP_LOAD32BITOS_INITRD)->set_enabled (enable);
      }
      break;
    case BXP_ATA0_MASTER_STATUS:
    case BXP_ATA0_SLAVE_STATUS:
    case BXP_ATA1_MASTER_STATUS:
    case BXP_ATA1_SLAVE_STATUS:
    case BXP_ATA2_MASTER_STATUS:
    case BXP_ATA2_SLAVE_STATUS:
    case BXP_ATA3_MASTER_STATUS:
    case BXP_ATA3_SLAVE_STATUS:
      if ((set) && (SIM->get_init_done ())) {
        Bit8u device = id - BXP_ATA0_MASTER_STATUS;
        Bit32u handle = DEV_hd_get_device_handle (device/2, device%2);
        DEV_hd_set_cd_media_status(handle, val == BX_INSERTED);
        bx_gui->update_drive_status_buttons ();
      }
      break;
    case BXP_FLOPPYA_TYPE:
      if ((set) && (!SIM->get_init_done ())) {
        bx_options.floppya.Odevtype->set (val);
      }
      break;
    case BXP_FLOPPYA_STATUS:
      if ((set) && (SIM->get_init_done ())) {
        DEV_floppy_set_media_status(0, val == BX_INSERTED);
        bx_gui->update_drive_status_buttons ();
      }
      break;
    case BXP_FLOPPYB_TYPE:
      if ((set) && (!SIM->get_init_done ())) {
        bx_options.floppyb.Odevtype->set (val);
      }
      break;
    case BXP_FLOPPYB_STATUS:
      if ((set) && (SIM->get_init_done ())) {
        DEV_floppy_set_media_status(1, val == BX_INSERTED);
        bx_gui->update_drive_status_buttons ();
      }
      break;
    case BXP_KBD_PASTE_DELAY:
      if ((set) && (SIM->get_init_done ())) {
        DEV_kbd_paste_delay_changed ();
        }
      break;

    case BXP_ATA0_MASTER_MODE:
    case BXP_ATA0_SLAVE_MODE:
    case BXP_ATA1_MASTER_MODE:
    case BXP_ATA1_SLAVE_MODE:
    case BXP_ATA2_MASTER_MODE:
    case BXP_ATA2_SLAVE_MODE:
    case BXP_ATA3_MASTER_MODE:
    case BXP_ATA3_SLAVE_MODE:
      if (set) {
        int device = id - BXP_ATA0_MASTER_MODE;
        switch (val) {
          case BX_ATA_MODE_UNDOABLE:
          case BX_ATA_MODE_VOLATILE:
          //case BX_ATA_MODE_Z_UNDOABLE:
          //case BX_ATA_MODE_Z_VOLATILE:
            SIM->get_param ((bx_id)(BXP_ATA0_MASTER_JOURNAL + device))->set_enabled (1);
            break;
          default:
            SIM->get_param ((bx_id)(BXP_ATA0_MASTER_JOURNAL + device))->set_enabled (0);
          }
        }
      break;

    case BXP_ATA0_MASTER_TYPE:
    case BXP_ATA0_SLAVE_TYPE:
    case BXP_ATA1_MASTER_TYPE:
    case BXP_ATA1_SLAVE_TYPE:
    case BXP_ATA2_MASTER_TYPE:
    case BXP_ATA2_SLAVE_TYPE:
    case BXP_ATA3_MASTER_TYPE:
    case BXP_ATA3_SLAVE_TYPE:
      if (set) {
        int device = id - BXP_ATA0_MASTER_TYPE;
        switch (val) {
          case BX_ATA_DEVICE_DISK:
            SIM->get_param_num ((bx_id)(BXP_ATA0_MASTER_PRESENT + device))->set (1);
            SIM->get_param ((bx_id)(BXP_ATA0_MASTER_MODE + device))->set_enabled (1);
            SIM->get_param ((bx_id)(BXP_ATA0_MASTER_PATH + device))->set_enabled (1);
            //SIM->get_param ((bx_id)(BXP_ATA0_MASTER_JOURNAL + device))->set_enabled (1);
            SIM->get_param ((bx_id)(BXP_ATA0_MASTER_CYLINDERS + device))->set_enabled (1);
            SIM->get_param ((bx_id)(BXP_ATA0_MASTER_HEADS + device))->set_enabled (1);
            SIM->get_param ((bx_id)(BXP_ATA0_MASTER_SPT + device))->set_enabled (1);
            SIM->get_param ((bx_id)(BXP_ATA0_MASTER_STATUS + device))->set_enabled (0);
            SIM->get_param ((bx_id)(BXP_ATA0_MASTER_MODEL + device))->set_enabled (1);
            SIM->get_param ((bx_id)(BXP_ATA0_MASTER_BIOSDETECT + device))->set_enabled (1);
            SIM->get_param ((bx_id)(BXP_ATA0_MASTER_TRANSLATION + device))->set_enabled (1);
            SIM->get_param ((bx_id)(BXP_ATA0_MASTER_PATH + device))->set_runtime_param (0);
            SIM->get_param ((bx_id)(BXP_ATA0_MASTER_STATUS + device))->set_runtime_param (0);
            break;
          case BX_ATA_DEVICE_CDROM:
            SIM->get_param_num ((bx_id)(BXP_ATA0_MASTER_PRESENT + device))->set (1);
            SIM->get_param ((bx_id)(BXP_ATA0_MASTER_MODE + device))->set_enabled (0);
            SIM->get_param ((bx_id)(BXP_ATA0_MASTER_PATH + device))->set_enabled (1);
            SIM->get_param ((bx_id)(BXP_ATA0_MASTER_JOURNAL + device))->set_enabled (0);
            SIM->get_param ((bx_id)(BXP_ATA0_MASTER_CYLINDERS + device))->set_enabled (0);
            SIM->get_param ((bx_id)(BXP_ATA0_MASTER_HEADS + device))->set_enabled (0);
            SIM->get_param ((bx_id)(BXP_ATA0_MASTER_SPT + device))->set_enabled (0);
            SIM->get_param ((bx_id)(BXP_ATA0_MASTER_STATUS + device))->set_enabled (1);
            SIM->get_param ((bx_id)(BXP_ATA0_MASTER_MODEL + device))->set_enabled (1);
            SIM->get_param ((bx_id)(BXP_ATA0_MASTER_BIOSDETECT + device))->set_enabled (1);
            SIM->get_param ((bx_id)(BXP_ATA0_MASTER_TRANSLATION + device))->set_enabled (0);
            SIM->get_param ((bx_id)(BXP_ATA0_MASTER_PATH + device))->set_runtime_param (1);
            SIM->get_param ((bx_id)(BXP_ATA0_MASTER_STATUS + device))->set_runtime_param (1);
            break;
          }
        }
      break;
    default:
      BX_PANIC (("bx_param_handler called with unknown id %d", id));
      return -1;
  }
  return val;
}

char *bx_param_string_handler (bx_param_string_c *param, int set, char *val, int maxlen)
{
  bx_id id = param->get_id ();

  int empty = 0;
  if ((strlen(val) < 1) || !strcmp ("none", val)) {
    empty = 1;
    val = "none";
  }
  switch (id) {
    case BXP_FLOPPYA_PATH:
      if (set==1) {
        if (SIM->get_init_done ()) {
          if (empty) {
            DEV_floppy_set_media_status(0, 0);
            bx_gui->update_drive_status_buttons ();
          } else {
            if (!SIM->get_param_num(BXP_FLOPPYA_TYPE)->get_enabled()) {
              BX_ERROR(("Cannot add a floppy drive at runtime"));
              bx_options.floppya.Opath->set ("none");
            }
          }
          if ((DEV_floppy_present()) &&
              (SIM->get_param_num(BXP_FLOPPYA_STATUS)->get () == BX_INSERTED)) {
            // tell the device model that we removed, then inserted the disk
            DEV_floppy_set_media_status(0, 0);
            DEV_floppy_set_media_status(0, 1);
          }
        } else {
          SIM->get_param_num(BXP_FLOPPYA_DEVTYPE)->set_enabled (!empty);
          SIM->get_param_num(BXP_FLOPPYA_TYPE)->set_enabled (!empty);
          SIM->get_param_num(BXP_FLOPPYA_STATUS)->set_enabled (!empty);
        }
      }
      break;
    case BXP_FLOPPYB_PATH:
      if (set==1) {
        if (SIM->get_init_done ()) {
          if (empty) {
            DEV_floppy_set_media_status(1, 0);
            bx_gui->update_drive_status_buttons ();
          } else {
            if (!SIM->get_param_num(BXP_FLOPPYB_TYPE)->get_enabled ()) {
              BX_ERROR(("Cannot add a floppy drive at runtime"));
              bx_options.floppyb.Opath->set ("none");
            }
          }
          if ((DEV_floppy_present()) &&
              (SIM->get_param_num(BXP_FLOPPYB_STATUS)->get () == BX_INSERTED)) {
            // tell the device model that we removed, then inserted the disk
            DEV_floppy_set_media_status(1, 0);
            DEV_floppy_set_media_status(1, 1);
          }
        } else {
          SIM->get_param_num(BXP_FLOPPYB_DEVTYPE)->set_enabled (!empty);
          SIM->get_param_num(BXP_FLOPPYB_TYPE)->set_enabled (!empty);
          SIM->get_param_num(BXP_FLOPPYB_STATUS)->set_enabled (!empty);
        }
      }
      break;

    case BXP_ATA0_MASTER_PATH:
    case BXP_ATA0_SLAVE_PATH:
    case BXP_ATA1_MASTER_PATH:
    case BXP_ATA1_SLAVE_PATH:
    case BXP_ATA2_MASTER_PATH:
    case BXP_ATA2_SLAVE_PATH:
    case BXP_ATA3_MASTER_PATH:
    case BXP_ATA3_SLAVE_PATH:
      if (set==1) {
        if (SIM->get_init_done ()) {

          Bit8u device = id - BXP_ATA0_MASTER_PATH;
          Bit32u handle = DEV_hd_get_device_handle(device/2, device%2);

          if (empty) {
            DEV_hd_set_cd_media_status(handle, 0);
            bx_gui->update_drive_status_buttons ();
          } else {
            if (!SIM->get_param_num((bx_id)(BXP_ATA0_MASTER_PRESENT + device))->get ()) {
              BX_ERROR(("Cannot add a cdrom drive at runtime"));
              bx_options.atadevice[device/2][device%2].Opresent->set (0);
            }
            if (SIM->get_param_num((bx_id)(BXP_ATA0_MASTER_TYPE + device))->get () != BX_ATA_DEVICE_CDROM) {
              BX_ERROR(("Device is not a cdrom drive"));
              bx_options.atadevice[device/2][device%2].Opresent->set (0);
            }
          }
          if (DEV_hd_present() &&
              (SIM->get_param_num((bx_id)(BXP_ATA0_MASTER_STATUS + device))->get () == BX_INSERTED) &&
              (SIM->get_param_num((bx_id)(BXP_ATA0_MASTER_TYPE + device))->get () == BX_ATA_DEVICE_CDROM)) {
            // tell the device model that we removed, then inserted the cd
            DEV_hd_set_cd_media_status(handle, 0);
            DEV_hd_set_cd_media_status(handle, 1);
          }
        }
      }
      break;
    
    case BXP_SCREENMODE:
      if (set==1) {
        BX_INFO (("Screen mode changed to %s", val));
      }
      break;
    default:
        BX_PANIC (("bx_string_handler called with unexpected parameter %d", param->get_id()));
  }
  return val;
}

static int
bx_param_enable_handler (bx_param_c *param, int val)
{
  bx_id id = param->get_id ();
  switch (id) {
    case BXP_ATA0_MASTER_STATUS:
    case BXP_ATA0_SLAVE_STATUS:
    case BXP_ATA1_MASTER_STATUS:
    case BXP_ATA1_SLAVE_STATUS:
    case BXP_ATA2_MASTER_STATUS:
    case BXP_ATA2_SLAVE_STATUS:
    case BXP_ATA3_MASTER_STATUS:
    case BXP_ATA3_SLAVE_STATUS:
      if (val != 0) {
        Bit8u device = id - BXP_ATA0_MASTER_STATUS;
  
        switch (SIM->get_param_enum ((bx_id)(BXP_ATA0_MASTER_TYPE + device))->get()) {
          case BX_ATA_DEVICE_CDROM:
            return (1);
            break;
          }
        }
      return (0);
      break;

    case BXP_ATA0_MASTER_JOURNAL:
    case BXP_ATA0_SLAVE_JOURNAL:
    case BXP_ATA1_MASTER_JOURNAL:
    case BXP_ATA1_SLAVE_JOURNAL:
    case BXP_ATA2_MASTER_JOURNAL:
    case BXP_ATA2_SLAVE_JOURNAL:
    case BXP_ATA3_MASTER_JOURNAL:
    case BXP_ATA3_SLAVE_JOURNAL:
      if (val != 0) {
        Bit8u device = id - BXP_ATA0_MASTER_JOURNAL;
  
        switch (SIM->get_param_enum ((bx_id)(BXP_ATA0_MASTER_TYPE + device))->get()) {
          case BX_ATA_DEVICE_DISK:
            switch (SIM->get_param_enum ((bx_id)(BXP_ATA0_MASTER_MODE + device))->get()) {
              case BX_ATA_MODE_UNDOABLE:
              case BX_ATA_MODE_VOLATILE:
              //case BX_ATA_MODE_Z_UNDOABLE:
              //case BX_ATA_MODE_Z_VOLATILE:
              return (1);
              break;
            }
          }
        }
      return (0);
      break;

    default:
      BX_PANIC (("bx_param_handler called with unknown id %d", id));
  }
  return val;
}



void bx_init_options ()
{
  int i;
  bx_list_c *menu;
  bx_list_c *deplist;
  char name[1024], descr[1024], label[1024];

  memset (&bx_options, 0, sizeof(bx_options));

  // quick start option, set by command line arg
  new bx_param_enum_c (BXP_BOCHS_START,
      "Bochs start types",
      "Bochs start types",
      bochs_start_names,
      BX_RUN_START,
      BX_QUICK_START);

  // floppya
  bx_options.floppya.Opath = new bx_param_filename_c (BXP_FLOPPYA_PATH,
      "floppya:path",
      "Pathname of first floppy image file or device.  If you're booting from floppy, this should be a bootable floppy.",
      "", BX_PATHNAME_LEN);
  bx_options.floppya.Opath->set_ask_format ("Enter new filename, or 'none' for no disk: [%s] ");
  bx_options.floppya.Opath->set_label ("First floppy image/device");
  bx_options.floppya.Odevtype = new bx_param_enum_c (BXP_FLOPPYA_DEVTYPE,
      "floppya:devtype",
      "Type of floppy drive",
      floppy_type_names,
      BX_FLOPPY_NONE,
      BX_FLOPPY_NONE);
  bx_options.floppya.Otype = new bx_param_enum_c (BXP_FLOPPYA_TYPE,
      "floppya:type",
      "Type of floppy disk",
      floppy_type_names,
      BX_FLOPPY_NONE,
      BX_FLOPPY_NONE);
  bx_options.floppya.Otype->set_ask_format ("What type of floppy disk? [%s] ");
  bx_options.floppya.Ostatus = new bx_param_enum_c (BXP_FLOPPYA_STATUS,
      "Is floppya inserted",
      "Inserted or ejected",
      floppy_status_names,
      BX_INSERTED,
      BX_EJECTED);
  bx_options.floppya.Ostatus->set_ask_format ("Is the floppy inserted or ejected? [%s] ");
  bx_options.floppya.Opath->set_format ("%s");
  bx_options.floppya.Otype->set_format ("size=%s");
  bx_options.floppya.Ostatus->set_format ("%s");
  bx_param_c *floppya_init_list[] = {
    // if the order "path,type,status" changes, corresponding changes must
    // be made in gui/wxmain.cc, MyFrame::editFloppyConfig.
    bx_options.floppya.Opath,
    bx_options.floppya.Otype,
    bx_options.floppya.Ostatus,
    NULL
  };
  menu = new bx_list_c (BXP_FLOPPYA, "Floppy Disk 0", "All options for first floppy disk", floppya_init_list);
  menu->get_options ()->set (menu->SERIES_ASK);
  bx_options.floppya.Opath->set_handler (bx_param_string_handler);
  bx_options.floppya.Opath->set ("none");
  bx_options.floppya.Otype->set_handler (bx_param_handler);
  bx_options.floppya.Ostatus->set_handler (bx_param_handler);

  bx_options.floppyb.Opath = new bx_param_filename_c (BXP_FLOPPYB_PATH,
      "floppyb:path",
      "Pathname of second floppy image file or device.",
      "", BX_PATHNAME_LEN);
  bx_options.floppyb.Opath->set_ask_format ("Enter new filename, or 'none' for no disk: [%s] ");
  bx_options.floppyb.Opath->set_label ("Second floppy image/device");
  bx_options.floppyb.Odevtype = new bx_param_enum_c (BXP_FLOPPYB_DEVTYPE,
      "floppyb:devtype",
      "Type of floppy drive",
      floppy_type_names,
      BX_FLOPPY_NONE,
      BX_FLOPPY_NONE);
  bx_options.floppyb.Otype = new bx_param_enum_c (BXP_FLOPPYB_TYPE,
      "floppyb:type",
      "Type of floppy disk",
      floppy_type_names,
      BX_FLOPPY_NONE,
      BX_FLOPPY_NONE);
  bx_options.floppyb.Otype->set_ask_format ("What type of floppy disk? [%s] ");
  bx_options.floppyb.Ostatus = new bx_param_enum_c (BXP_FLOPPYB_STATUS,
      "Is floppyb inserted",
      "Inserted or ejected",
      floppy_status_names,
      BX_INSERTED,
      BX_EJECTED);
  bx_options.floppyb.Ostatus->set_ask_format ("Is the floppy inserted or ejected? [%s] ");
  bx_options.floppyb.Opath->set_format ("%s");
  bx_options.floppyb.Otype->set_format ("size=%s");
  bx_options.floppyb.Ostatus->set_format ("%s");
  bx_param_c *floppyb_init_list[] = {
    bx_options.floppyb.Opath,
    bx_options.floppyb.Otype,
    bx_options.floppyb.Ostatus,
    NULL
  };
  menu = new bx_list_c (BXP_FLOPPYB, "Floppy Disk 1", "All options for second floppy disk", floppyb_init_list);
  menu->get_options ()->set (menu->SERIES_ASK);
  bx_options.floppyb.Opath->set_handler (bx_param_string_handler);
  bx_options.floppyb.Opath->set ("none");
  bx_options.floppyb.Otype->set_handler (bx_param_handler);
  bx_options.floppyb.Ostatus->set_handler (bx_param_handler);

  // disk options

  // FIXME use descr and name
  char *s_atachannel[] = {
    "ATA channel 0",
    "ATA channel 1",
    "ATA channel 2",
    "ATA channel 3",
    };
  char *s_atadevice[4][2] = {
    { "First HD/CD on channel 0",
      "Second HD/CD on channel 0" },
    { "First HD/CD on channel 1",
    "Second HD/CD on channel 1" },
    { "First HD/CD on channel 2",
    "Second HD/CD on channel 2" },
    { "First HD/CD on channel 3",
    "Second HD/CD on channel 3" }
    };
  Bit16u ata_default_ioaddr1[BX_MAX_ATA_CHANNEL] = {
    0x1f0, 0x170, 0x1e8, 0x168 
  };
  Bit16u ata_default_ioaddr2[BX_MAX_ATA_CHANNEL] = {
    0x3f0, 0x370, 0x3e0, 0x360 
  };
  Bit8u ata_default_irq[BX_MAX_ATA_CHANNEL] = { 
    14, 15, 11, 9 
  };

  bx_list_c *ata[BX_MAX_ATA_CHANNEL];
  bx_list_c *ata_menu[BX_MAX_ATA_CHANNEL];

  Bit8u channel;
  for (channel=0; channel<BX_MAX_ATA_CHANNEL; channel ++) {

    ata[channel] = new bx_list_c ((bx_id)(BXP_ATAx(channel)), s_atachannel[channel], s_atachannel[channel], 8);
    ata[channel]->get_options ()->set (bx_list_c::SERIES_ASK);

    ata[channel]->add (bx_options.ata[channel].Opresent = new bx_param_bool_c ((bx_id)(BXP_ATAx_PRESENT(channel)),
      "ata:present",                                
      "Controls whether ata channel is installed or not",
      0));

    ata[channel]->add (bx_options.ata[channel].Oioaddr1 = new bx_param_num_c ((bx_id)(BXP_ATAx_IOADDR1(channel)),
      "ata:ioaddr1",
      "IO adress of ata command block",
      0, 0xffff,
      ata_default_ioaddr1[channel]));

    ata[channel]->add (bx_options.ata[channel].Oioaddr2 = new bx_param_num_c ((bx_id)(BXP_ATAx_IOADDR2(channel)),
      "ata:ioaddr2",
      "IO adress of ata control block",
      0, 0xffff,
      ata_default_ioaddr2[channel]));

    ata[channel]->add (bx_options.ata[channel].Oirq = new bx_param_num_c ((bx_id)(BXP_ATAx_IRQ(channel)),
      "ata:irq",
      "IRQ used by this ata channel",
      0, 15,
      ata_default_irq[channel]));

    // all items in the ata[channel] menu depend on the present flag.
    // The menu list is complete, but a few dependent_list items will
    // be added later.  Use clone() to make a copy of the dependent_list
    // so that it can be changed without affecting the menu.
    bx_options.ata[channel].Opresent->set_dependent_list (
        ata[channel]->clone());

    for (Bit8u slave=0; slave<2; slave++) {

      menu = bx_options.atadevice[channel][slave].Omenu = new bx_list_c ((bx_id)(BXP_ATAx_DEVICE(channel,slave)),
          s_atadevice[channel][slave], 
          s_atadevice[channel][slave],
          BXP_PARAMS_PER_ATA_DEVICE + 1 );
      menu->get_options ()->set (menu->SERIES_ASK);

      menu->add (bx_options.atadevice[channel][slave].Opresent = new bx_param_bool_c ((bx_id)(BXP_ATAx_DEVICE_PRESENT(channel,slave)),
        "ata-device:present",                                
        "Controls whether ata device is installed or not",  
        0));

      menu->add (bx_options.atadevice[channel][slave].Otype = new bx_param_enum_c ((bx_id)(BXP_ATAx_DEVICE_TYPE(channel,slave)),
          "ata-device:type",
          "Type of ATA device (disk or cdrom)",
          atadevice_type_names,
          BX_ATA_DEVICE_DISK,
          BX_ATA_DEVICE_DISK));

      menu->add (bx_options.atadevice[channel][slave].Opath = new bx_param_filename_c ((bx_id)(BXP_ATAx_DEVICE_PATH(channel,slave)),
          "ata-device:path",
          "Pathname of the image or physical device (cdrom only)",
          "", BX_PATHNAME_LEN));

      menu->add (bx_options.atadevice[channel][slave].Omode = new bx_param_enum_c ((bx_id)(BXP_ATAx_DEVICE_MODE(channel,slave)),
          "ata-device:mode",
          "Mode of the ATA harddisk",
          atadevice_mode_names,
          BX_ATA_MODE_FLAT,
          BX_ATA_MODE_FLAT));

      menu->add (bx_options.atadevice[channel][slave].Ostatus = new bx_param_enum_c ((bx_id)(BXP_ATAx_DEVICE_STATUS(channel,slave)),
       "ata-device:status",
       "CD-ROM media status (inserted / ejected)",
       atadevice_status_names,
       BX_INSERTED,
       BX_EJECTED));

      menu->add (bx_options.atadevice[channel][slave].Ojournal = new bx_param_filename_c ((bx_id)(BXP_ATAx_DEVICE_JOURNAL(channel,slave)),
          "ata-device:journal",
          "Pathname of the journal file",
          "", BX_PATHNAME_LEN));

      menu->add (bx_options.atadevice[channel][slave].Ocylinders = new bx_param_num_c ((bx_id)(BXP_ATAx_DEVICE_CYLINDERS(channel,slave)),
          "ata-device:cylinders",
          "Number of cylinders",
          0, 65535,
          0));
      menu->add (bx_options.atadevice[channel][slave].Oheads = new bx_param_num_c ((bx_id)(BXP_ATAx_DEVICE_HEADS(channel,slave)),
          "ata-device:heads",
          "Number of heads",
          0, 65535,
          0));
      menu->add (bx_options.atadevice[channel][slave].Ospt = new bx_param_num_c ((bx_id)(BXP_ATAx_DEVICE_SPT(channel,slave)),
          "ata-device:spt",
          "Number of sectors per track",
          0, 65535,
          0));
      
      menu->add (bx_options.atadevice[channel][slave].Omodel = new bx_param_string_c ((bx_id)(BXP_ATAx_DEVICE_MODEL(channel,slave)),
       "ata-device:model",
       "String returned by the 'identify device' command",
       "Generic 1234", 40));

      menu->add (bx_options.atadevice[channel][slave].Obiosdetect = new bx_param_enum_c ((bx_id)(BXP_ATAx_DEVICE_BIOSDETECT(channel,slave)),
       "ata-device:biosdetect",
       "Type of bios detection",
       atadevice_biosdetect_names,
       BX_ATA_BIOSDETECT_AUTO,
       BX_ATA_BIOSDETECT_NONE));

      menu->add (bx_options.atadevice[channel][slave].Otranslation = new bx_param_enum_c ((bx_id)(BXP_ATAx_DEVICE_TRANSLATION(channel,slave)),
       "How the ata-disk translation is done by the bios",
       "Type of translation",
       atadevice_translation_names,
       BX_ATA_TRANSLATION_AUTO,
       BX_ATA_TRANSLATION_NONE));

      bx_options.atadevice[channel][slave].Opresent->set_dependent_list (
          menu->clone ());
      // the menu and all items on it depend on the Opresent flag
      bx_options.atadevice[channel][slave].Opresent->get_dependent_list()->add(menu);
      // the present flag depends on the ATA channel's present flag
      bx_options.ata[channel].Opresent->get_dependent_list()->add (
          bx_options.atadevice[channel][slave].Opresent);
      }

      // set up top level menu for ATA[i] controller configuration.  This list
      // controls what will appear on the ATA configure dialog.  It now
      // requests the USE_TAB_WINDOW display, which is implemented in wx.
      char buffer[32];
      sprintf (buffer, "Configure ATA%d", channel);
      ata_menu[channel] = new bx_list_c ((bx_id)(BXP_ATAx_MENU(channel)), strdup(buffer), "", 4);
      ata_menu[channel]->add (ata[channel]);
      ata_menu[channel]->add (bx_options.atadevice[channel][0].Omenu);
      ata_menu[channel]->add (bx_options.atadevice[channel][1].Omenu);
      ata_menu[channel]->get_options()->set (bx_list_c::USE_TAB_WINDOW);
    }

  // Enable first ata interface by default, disable the others.
  bx_options.ata[0].Opresent->set_initial_val(1);

  // now that the dependence relationships are established, call set() on
  // the ata device present params to set all enables correctly.
  for (i=0; i<BX_MAX_ATA_CHANNEL; i++)
    bx_options.ata[i].Opresent->set (i==0);

  for (channel=0; channel<BX_MAX_ATA_CHANNEL; channel ++) {

    bx_options.ata[channel].Opresent->set_ask_format ("Channel is enabled: [%s] ");
    bx_options.ata[channel].Oioaddr1->set_ask_format ("Enter new ioaddr1: [0x%x] ");
    bx_options.ata[channel].Oioaddr2->set_ask_format ("Enter new ioaddr2: [0x%x] ");
    bx_options.ata[channel].Oirq->set_ask_format ("Enter new IRQ: [%d] ");
#if BX_WITH_WX
    bx_options.ata[channel].Opresent->set_label ("Enable this channel?");
    bx_options.ata[channel].Oioaddr1->set_label ("I/O Address 1:");
    bx_options.ata[channel].Oioaddr2->set_label ("I/O Address 2:");
    bx_options.ata[channel].Oirq->set_label ("IRQ:");
    bx_options.ata[channel].Oirq->set_options (bx_param_num_c::USE_SPIN_CONTROL);
#else
    bx_options.ata[channel].Opresent->set_format ("enabled: %s");
    bx_options.ata[channel].Oioaddr1->set_format ("ioaddr1: 0x%x");
    bx_options.ata[channel].Oioaddr2->set_format ("ioaddr2: 0x%x");
    bx_options.ata[channel].Oirq->set_format ("irq: %d");
#endif
    bx_options.ata[channel].Oioaddr1->set_base (16);
    bx_options.ata[channel].Oioaddr2->set_base (16);

    for (Bit8u slave=0; slave<2; slave++) {

      bx_options.atadevice[channel][slave].Opresent->set_ask_format (
          "Device is enabled: [%s] ");
      bx_options.atadevice[channel][slave].Otype->set_ask_format (
          "Enter type of ATA device, disk or cdrom: [%s] ");
      bx_options.atadevice[channel][slave].Omode->set_ask_format (
          "Enter mode of ATA device, (flat, concat, etc.): [%s] ");
      bx_options.atadevice[channel][slave].Opath->set_ask_format (
          "Enter new filename: [%s] ");
      bx_options.atadevice[channel][slave].Ocylinders->set_ask_format (
          "Enter number of cylinders: [%d] ");
      bx_options.atadevice[channel][slave].Oheads->set_ask_format (
          "Enter number of heads: [%d] ");
      bx_options.atadevice[channel][slave].Ospt->set_ask_format (
          "Enter number of sectors per track: [%d] ");
      bx_options.atadevice[channel][slave].Ostatus->set_ask_format (
          "Is the device inserted or ejected? [%s] ");
      bx_options.atadevice[channel][slave].Omodel->set_ask_format (
          "Enter new model name: [%s]");
      bx_options.atadevice[channel][slave].Otranslation->set_ask_format (
          "Enter translation type: [%s]");
      bx_options.atadevice[channel][slave].Obiosdetect->set_ask_format (
          "Enter bios detection type: [%s]");
      bx_options.atadevice[channel][slave].Ojournal->set_ask_format (
          "Enter path of journal file: [%s]");

#if BX_WITH_WX
      bx_options.atadevice[channel][slave].Opresent->set_label (
          "Enable this device?");
      bx_options.atadevice[channel][slave].Otype->set_label (
          "Type of ATA device:");
      bx_options.atadevice[channel][slave].Omode->set_label (
          "Type of disk image:");
      bx_options.atadevice[channel][slave].Opath->set_label (
          "Path or physical device name:");
      bx_options.atadevice[channel][slave].Ocylinders->set_label (
          "Cylinders:");
      bx_options.atadevice[channel][slave].Oheads->set_label (
          "Heads:");
      bx_options.atadevice[channel][slave].Ospt->set_label (
          "Sectors per track:");
      bx_options.atadevice[channel][slave].Ostatus->set_label (
          "Inserted?");
      bx_options.atadevice[channel][slave].Omodel->set_label (
          "Model name:");
      bx_options.atadevice[channel][slave].Otranslation->set_label (
          "Translation type:");
      bx_options.atadevice[channel][slave].Obiosdetect->set_label (
          "BIOS Detection:");
      bx_options.atadevice[channel][slave].Ojournal->set_label (
          "Path of journal file:");
#else
      bx_options.atadevice[channel][slave].Opresent->set_format ("enabled: %s");
      bx_options.atadevice[channel][slave].Otype->set_format ("type %s");
      bx_options.atadevice[channel][slave].Omode->set_format ("mode %s");
      bx_options.atadevice[channel][slave].Opath->set_format ("path '%s'");
      bx_options.atadevice[channel][slave].Ocylinders->set_format ("%d cylinders");
      bx_options.atadevice[channel][slave].Oheads->set_format ("%d heads");
      bx_options.atadevice[channel][slave].Ospt->set_format ("%d sectors/track");
      bx_options.atadevice[channel][slave].Ostatus->set_format ("%s");
      bx_options.atadevice[channel][slave].Omodel->set_format ("model '%s'");
      bx_options.atadevice[channel][slave].Otranslation->set_format ("translation '%s'");
      bx_options.atadevice[channel][slave].Obiosdetect->set_format ("biosdetect '%s'");
      bx_options.atadevice[channel][slave].Ojournal->set_format ("journal is '%s'");
#endif

      bx_options.atadevice[channel][slave].Otype->set_handler (bx_param_handler);
      bx_options.atadevice[channel][slave].Omode->set_handler (bx_param_handler);
      bx_options.atadevice[channel][slave].Ostatus->set_handler (bx_param_handler);
      bx_options.atadevice[channel][slave].Opath->set_handler (bx_param_string_handler);

      // Set the enable_hanlders
      bx_options.atadevice[channel][slave].Ojournal->set_enable_handler (bx_param_enable_handler);
      bx_options.atadevice[channel][slave].Ostatus->set_enable_handler (bx_param_enable_handler);

      }
    }

  bx_options.OnewHardDriveSupport = new bx_param_bool_c (BXP_NEWHARDDRIVESUPPORT,
      "New hard drive support",
      "Enables new features found on newer hard drives.",
      1);

  bx_options.Obootdrive = new bx_param_enum_c (BXP_BOOTDRIVE,
      "bootdrive",
      "Boot A, C or CD",
      floppy_bootdisk_names,
      BX_BOOT_FLOPPYA,
      BX_BOOT_FLOPPYA);
  bx_options.Obootdrive->set_format ("Boot from: %s drive");
  bx_options.Obootdrive->set_ask_format ("Boot from floppy drive, hard drive or cdrom ? [%s] ");

  bx_options.OfloppySigCheck = new bx_param_bool_c (BXP_FLOPPYSIGCHECK,
      "Skip Floppy Boot Signature Check",
      "Skips check for the 0xaa55 signature on floppy boot device.",
      0);

  // disk menu
  bx_param_c *disk_menu_init_list[] = {
    SIM->get_param (BXP_FLOPPYA),
    SIM->get_param (BXP_FLOPPYB),
    SIM->get_param (BXP_ATA0),
    SIM->get_param (BXP_ATA0_MASTER),
    SIM->get_param (BXP_ATA0_SLAVE),
#if BX_MAX_ATA_CHANNEL>1
    SIM->get_param (BXP_ATA1),
    SIM->get_param (BXP_ATA1_MASTER),
    SIM->get_param (BXP_ATA1_SLAVE),
#endif
#if BX_MAX_ATA_CHANNEL>2
    SIM->get_param (BXP_ATA2),
    SIM->get_param (BXP_ATA2_MASTER),
    SIM->get_param (BXP_ATA2_SLAVE),
#endif
#if BX_MAX_ATA_CHANNEL>3
    SIM->get_param (BXP_ATA3),
    SIM->get_param (BXP_ATA3_MASTER),
    SIM->get_param (BXP_ATA3_SLAVE),
#endif
    SIM->get_param (BXP_NEWHARDDRIVESUPPORT),
    SIM->get_param (BXP_BOOTDRIVE),
    SIM->get_param (BXP_FLOPPYSIGCHECK),
    NULL
  };
  menu = new bx_list_c (BXP_MENU_DISK, "Bochs Disk Options", "diskmenu", disk_menu_init_list);
  menu->get_options ()->set (menu->SHOW_PARENT);

  // memory options menu
  bx_options.memory.Osize = new bx_param_num_c (BXP_MEM_SIZE,
      "megs",
      "Amount of RAM in megabytes",
      1, BX_MAX_BIT32U,
      BX_DEFAULT_MEM_MEGS);
  bx_options.memory.Osize->set_ask_format ("Enter memory size (MB): [%d] ");
#if BX_WITH_WX
  bx_options.memory.Osize->set_label ("Memory size (megabytes)");
  bx_options.memory.Osize->set_options (bx_param_num_c::USE_SPIN_CONTROL);
#else
  bx_options.memory.Osize->set_format ("Memory size in megabytes: %d");
#endif

  // initialize serial and parallel port options
#define PAR_SER_INIT_LIST_MAX \
  ((BXP_PARAMS_PER_PARALLEL_PORT * BX_N_PARALLEL_PORTS) \
  + (BXP_PARAMS_PER_SERIAL_PORT * BX_N_SERIAL_PORTS) \
  + (BXP_PARAMS_PER_USB_HUB * BX_N_USB_HUBS))
  bx_param_c *par_ser_init_list[1+PAR_SER_INIT_LIST_MAX];
  bx_param_c **par_ser_ptr = &par_ser_init_list[0];

  // parallel ports
  for (i=0; i<BX_N_PARALLEL_PORTS; i++) {
        sprintf (name, "Enable parallel port #%d", i+1);
        sprintf (descr, "Controls whether parallel port #%d is installed or not", i+1);
        bx_options.par[i].Oenabled = new bx_param_bool_c (
                BXP_PARPORTx_ENABLED(i+1), 
                strdup(name), 
                strdup(descr), 
                (i==0)? 1 : 0);  // only enable #1 by default
        sprintf (name, "Parallel port #%d output file", i+1);
        sprintf (descr, "Data written to parport#%d by the guest OS is written to this file", i+1);
        bx_options.par[i].Ooutfile = new bx_param_filename_c (
                BXP_PARPORTx_OUTFILE(i+1), 
                strdup(name), 
                strdup(descr),
                "", BX_PATHNAME_LEN);
        deplist = new bx_list_c (BXP_NULL, 1);
        deplist->add (bx_options.par[i].Ooutfile);
        bx_options.par[i].Oenabled->set_dependent_list (deplist);
        // add to menu
        *par_ser_ptr++ = bx_options.par[i].Oenabled;
        *par_ser_ptr++ = bx_options.par[i].Ooutfile;
  }

  // serial ports
  for (i=0; i<BX_N_SERIAL_PORTS; i++) {
        // options for COM port
        sprintf (name, "Enable serial port #%d (COM%d)", i+1, i+1);
        sprintf (descr, "Controls whether COM%d is installed or not", i+1);
        bx_options.com[i].Oenabled = new bx_param_bool_c (
                BXP_COMx_ENABLED(i+1),
                strdup(name), 
                strdup(descr), 
                (i==0)?1 : 0);  // only enable the first by default
        sprintf (name, "Pathname of the serial device for COM%d", i+1);
        sprintf (descr, "The path can be a real serial device or a pty (X/Unix only)");
        bx_options.com[i].Odev = new bx_param_filename_c (
                BXP_COMx_PATH(i+1),
                strdup(name), 
                strdup(descr), 
                "", BX_PATHNAME_LEN);
        deplist = new bx_list_c (BXP_NULL, 1);
        deplist->add (bx_options.com[i].Odev);
        bx_options.com[i].Oenabled->set_dependent_list (deplist);
        // add to menu
        *par_ser_ptr++ = bx_options.com[i].Oenabled;
        *par_ser_ptr++ = bx_options.com[i].Odev;
  }

  // usb hubs
  for (i=0; i<BX_N_USB_HUBS; i++) {
        // options for USB hub
        sprintf (name, "usb%d:enabled", i+1);
        sprintf (descr, "Controls whether USB%d is installed or not", i+1);
        sprintf (label, "Enable usb hub #%d (USB%d)", i+1, i+1);
        bx_options.usb[i].Oenabled = new bx_param_bool_c (
                BXP_USBx_ENABLED(i+1),
                strdup(name), 
                strdup(descr), 
                (i==0)?1 : 0);  // only enable the first by default
        bx_options.usb[i].Oioaddr = new bx_param_num_c (
                BXP_USBx_IOADDR(i+1),
                "usb:ioaddr",
                "I/O base adress of USB hub",
                0, 0xffe0,
                (i==0)?0xff80 : 0);
        bx_options.usb[i].Oirq = new bx_param_num_c (
                BXP_USBx_IRQ(i+1),
                "usb:irq",
                "IRQ used by USB hub",
                0, 15,
                (i==0)?10 : 0);
        deplist = new bx_list_c (BXP_NULL, 2);
        deplist->add (bx_options.usb[i].Oioaddr);
        deplist->add (bx_options.usb[i].Oirq);
        bx_options.usb[i].Oenabled->set_dependent_list (deplist);
        // add to menu
        *par_ser_ptr++ = bx_options.usb[i].Oenabled;
        *par_ser_ptr++ = bx_options.usb[i].Oioaddr;
        *par_ser_ptr++ = bx_options.usb[i].Oirq;

        bx_options.usb[i].Oioaddr->set_ask_format ("Enter new ioaddr: [0x%x] ");
        bx_options.usb[i].Oioaddr->set_label ("I/O Address");
        bx_options.usb[i].Oioaddr->set_base (16);
        bx_options.usb[i].Oenabled->set_label (strdup(label));
        bx_options.usb[i].Oirq->set_label ("USB IRQ");
        bx_options.usb[i].Oirq->set_options (bx_param_num_c::USE_SPIN_CONTROL);
  }
  // add final NULL at the end, and build the menu
  *par_ser_ptr = NULL;
  menu = new bx_list_c (BXP_MENU_SERIAL_PARALLEL,
          "Serial and Parallel Port Options",
          "serial_parallel_menu",
          par_ser_init_list);
  menu->get_options ()->set (menu->SHOW_PARENT);

  // pci slots
  for (i=0; i<BX_N_PCI_SLOTS; i++) {
        sprintf (name, "Use PCI slot #%d", i+1);
        sprintf (descr, "Controls whether PCI alot #%d is used or not", i+1);
        bx_options.pcislot[i].Oused = new bx_param_bool_c (
                BXP_PCISLOTx_USED(i+1), 
                strdup(name), 
                strdup(descr), 
                0);
        sprintf (name, "PCI slot #%d device", i+1);
        sprintf (descr, "Name of the device connected to PCI slot #%d", i+1);
        bx_options.pcislot[i].Odevname = new bx_param_string_c (
                BXP_PCISLOTx_DEVNAME(i+1), 
                strdup(name), 
                strdup(descr),
                "", BX_PATHNAME_LEN);
        deplist = new bx_list_c (BXP_NULL, 1);
        deplist->add (bx_options.pcislot[i].Odevname);
        bx_options.pcislot[i].Oused->set_dependent_list (deplist);
  }

  bx_options.rom.Opath = new bx_param_filename_c (BXP_ROM_PATH,
      "romimage",
      "Pathname of ROM image to load",
      "", BX_PATHNAME_LEN);
  bx_options.rom.Opath->set_format ("Name of ROM BIOS image: %s");
  bx_options.rom.Oaddress = new bx_param_num_c (BXP_ROM_ADDRESS,
      "romaddr",
      "The address at which the ROM image should be loaded",
      0, BX_MAX_BIT32U, 
      0xf0000);
  bx_options.rom.Oaddress->set_base (16);
#if BX_WITH_WX
  bx_options.rom.Opath->set_label ("ROM BIOS image");
  bx_options.rom.Oaddress->set_label ("ROM BIOS address");
#else
  bx_options.rom.Oaddress->set_format ("ROM BIOS address: 0x%05x");
#endif

  bx_options.optrom[0].Opath = new bx_param_filename_c (BXP_OPTROM1_PATH,
      "optional romimage #1",
      "Pathname of optional ROM image #1 to load",
      "", BX_PATHNAME_LEN);
  bx_options.optrom[0].Opath->set_format ("Name of optional ROM image #1 : %s");
  bx_options.optrom[0].Oaddress = new bx_param_num_c (BXP_OPTROM1_ADDRESS,
      "optional romaddr #1",
      "The address at which the optional ROM image #1 should be loaded",
      0, BX_MAX_BIT32U, 
      0);
  bx_options.optrom[0].Oaddress->set_base (16);
#if BX_WITH_WX
  bx_options.optrom[0].Opath->set_label ("Optional ROM image #1");
  bx_options.optrom[0].Oaddress->set_label ("Address");
#else
  bx_options.optrom[0].Oaddress->set_format ("optional ROM #1 address: 0x%05x");
#endif

  bx_options.optrom[1].Opath = new bx_param_filename_c (BXP_OPTROM2_PATH,
      "optional romimage #2",
      "Pathname of optional ROM image #2 to load",
      "", BX_PATHNAME_LEN);
  bx_options.optrom[1].Opath->set_format ("Name of optional ROM image #2 : %s");
  bx_options.optrom[1].Oaddress = new bx_param_num_c (BXP_OPTROM2_ADDRESS,
      "optional romaddr #2",
      "The address at which the optional ROM image #2 should be loaded",
      0, BX_MAX_BIT32U, 
      0);
  bx_options.optrom[1].Oaddress->set_base (16);
#if BX_WITH_WX
  bx_options.optrom[1].Opath->set_label ("Optional ROM image #2");
  bx_options.optrom[1].Oaddress->set_label ("Address");
#else
  bx_options.optrom[1].Oaddress->set_format ("optional ROM #2 address: 0x%05x");
#endif

  bx_options.optrom[2].Opath = new bx_param_filename_c (BXP_OPTROM3_PATH,
      "optional romimage #3",
      "Pathname of optional ROM image #3 to load",
      "", BX_PATHNAME_LEN);
  bx_options.optrom[2].Opath->set_format ("Name of optional ROM image #3 : %s");
  bx_options.optrom[2].Oaddress = new bx_param_num_c (BXP_OPTROM3_ADDRESS,
      "optional romaddr #3",
      "The address at which the optional ROM image #3 should be loaded",
      0, BX_MAX_BIT32U, 
      0);
  bx_options.optrom[2].Oaddress->set_base (16);
#if BX_WITH_WX
  bx_options.optrom[2].Opath->set_label ("Optional ROM image #3");
  bx_options.optrom[2].Oaddress->set_label ("Address");
#else
  bx_options.optrom[2].Oaddress->set_format ("optional ROM #3 address: 0x%05x");
#endif

  bx_options.optrom[3].Opath = new bx_param_filename_c (BXP_OPTROM4_PATH,
      "optional romimage #4",
      "Pathname of optional ROM image #4 to load",
      "", BX_PATHNAME_LEN);
  bx_options.optrom[3].Opath->set_format ("Name of optional ROM image #4 : %s");
  bx_options.optrom[3].Oaddress = new bx_param_num_c (BXP_OPTROM4_ADDRESS,
      "optional romaddr #4",
      "The address at which the optional ROM image #4 should be loaded",
      0, BX_MAX_BIT32U, 
      0);
  bx_options.optrom[3].Oaddress->set_base (16);
#if BX_WITH_WX
  bx_options.optrom[3].Opath->set_label ("Optional ROM image #4");
  bx_options.optrom[3].Oaddress->set_label ("Address");
#else
  bx_options.optrom[3].Oaddress->set_format ("optional ROM #4 address: 0x%05x");
#endif

  bx_options.vgarom.Opath = new bx_param_filename_c (BXP_VGA_ROM_PATH,
      "vgaromimage",
      "Pathname of VGA ROM image to load",
      "", BX_PATHNAME_LEN);
  bx_options.vgarom.Opath->set_format ("Name of VGA BIOS image: %s");
#if BX_WITH_WX
  bx_options.vgarom.Opath->set_label ("VGA BIOS image");
#endif
  bx_param_c *memory_init_list[] = {
    bx_options.memory.Osize,
    bx_options.vgarom.Opath,
    bx_options.rom.Opath,
    bx_options.rom.Oaddress,
    bx_options.optrom[0].Opath,
    bx_options.optrom[0].Oaddress,
    bx_options.optrom[1].Opath,
    bx_options.optrom[1].Oaddress,
    bx_options.optrom[2].Opath,
    bx_options.optrom[2].Oaddress,
    bx_options.optrom[3].Opath,
    bx_options.optrom[3].Oaddress,
    NULL
  };
  menu = new bx_list_c (BXP_MENU_MEMORY, "Bochs Memory Options", "memmenu", memory_init_list);
  menu->get_options ()->set (menu->SHOW_PARENT);

  // interface
  bx_options.Ovga_update_interval = new bx_param_num_c (BXP_VGA_UPDATE_INTERVAL,
      "VGA Update Interval",
      "Number of microseconds between VGA updates",
      1, BX_MAX_BIT32U,
      30000);
  bx_options.Ovga_update_interval->set_handler (bx_param_handler);
  bx_options.Ovga_update_interval->set_runtime_param (1);
  bx_options.Ovga_update_interval->set_ask_format ("Type a new value for VGA update interval: [%d] ");
  bx_options.Omouse_enabled = new bx_param_bool_c (BXP_MOUSE_ENABLED,
      "Enable the mouse",
      "Controls whether the mouse sends events to the guest. The hardware emulation is always enabled.",
      0);
  bx_options.Omouse_enabled->set_handler (bx_param_handler);
  bx_options.Omouse_enabled->set_runtime_param (1);
  bx_options.Oips = new bx_param_num_c (BXP_IPS, 
      "Emulated instructions per second (IPS)",
      "Emulated instructions per second, used to calibrate bochs emulated time with wall clock time.",
      1, BX_MAX_BIT32U,
      500000);
  bx_options.Otext_snapshot_check = new bx_param_bool_c (BXP_TEXT_SNAPSHOT_CHECK,
      "Enable panic for use in bochs testing",
      "Enable panic when text on screen matches snapchk.txt.\nUseful for regression testing.\nIn win32, turns off CR/LF in snapshots and cuts.",
      0);
  bx_options.Oprivate_colormap = new bx_param_bool_c (BXP_PRIVATE_COLORMAP,
      "Use a private colormap",
      "Request that the GUI create and use it's own non-shared colormap. This colormap will be used when in the bochs window. If not enabled, a shared colormap scheme may be used. Not implemented on all GUI's.",
      0);
#if BX_WITH_AMIGAOS
  bx_options.Ofullscreen = new bx_param_bool_c (BXP_FULLSCREEN,
      "Use full screen mode",
      "When enabled, bochs occupies the whole screen instead of just a window.",
      0);
  bx_options.Oscreenmode = new bx_param_string_c (BXP_SCREENMODE,
      "Screen mode name",
      "Screen mode name",
      "", BX_PATHNAME_LEN);
  bx_options.Oscreenmode->set_handler (bx_param_string_handler);
#endif
  static char *config_interface_list[] = {
#if BX_USE_TEXTCONFIG
    "textconfig",
#endif
#if BX_WITH_WX
    "wx",
#endif
    NULL
  };
  bx_options.Osel_config = new bx_param_enum_c (
    BXP_SEL_CONFIG_INTERFACE,
    "Configuration interface",
    "Select configuration interface",
    config_interface_list,
    0,
    0);
  bx_options.Osel_config->set_by_name (BX_DEFAULT_CONFIG_INTERFACE);
  bx_options.Osel_config->set_ask_format ("Choose which configuration interface to use: [%s] ");
  // this is a list of gui libraries that are known to be available at
  // compile time.  The one that is listed first will be the default,
  // which is used unless the user overrides it on the command line or
  // in a configuration file.
  static char *display_library_list[] = {
#if BX_WITH_X11
    "x",
#endif
#if BX_WITH_WIN32
    "win32",
#endif
#if BX_WITH_CARBON
    "carbon",
#endif
#if BX_WITH_BEOS
    "beos",
#endif
#if BX_WITH_MACOS
    "macos",
#endif
#if BX_WITH_AMIGAOS
    "amigaos",
#endif
#if BX_WITH_SDL
    "sdl",
#endif
#if BX_WITH_SVGA
    "svga",
#endif
#if BX_WITH_TERM
    "term",
#endif
#if BX_WITH_RFB
    "rfb",
#endif
#if BX_WITH_WX
    "wx",
#endif
#if BX_WITH_NOGUI
    "nogui",
#endif
    NULL
  };
  bx_options.Osel_displaylib = new bx_param_enum_c (BXP_SEL_DISPLAY_LIBRARY,
    "VGA Display Library",
    "Select VGA Display Library",
    display_library_list,
    0,
    0);
  bx_options.Osel_displaylib->set_by_name (BX_DEFAULT_DISPLAY_LIBRARY);
  bx_options.Osel_displaylib->set_ask_format ("Choose which library to use for the Bochs display: [%s] ");
  bx_options.Odisplaylib_options = new bx_param_string_c (BXP_DISPLAYLIB_OPTIONS,
    "Display Library options",
    "Options passed to Display Library",
    "",
    BX_PATHNAME_LEN);
  bx_param_c *interface_init_list[] = {
    bx_options.Osel_config,
    bx_options.Osel_displaylib,
    bx_options.Odisplaylib_options,
    bx_options.Ovga_update_interval,
    bx_options.Omouse_enabled,
    bx_options.Oips,
    bx_options.Oprivate_colormap,
#if BX_WITH_AMIGAOS
    bx_options.Ofullscreen,
    bx_options.Oscreenmode,
#endif
    NULL
  };
  menu = new bx_list_c (BXP_MENU_INTERFACE, "Bochs Interface Menu", "intfmenu", interface_init_list);
  menu->get_options ()->set (menu->SHOW_PARENT);

  // pcidev options
  bx_options.pcidev.Ovendor = new bx_param_num_c(BXP_PCIDEV_VENDOR,
      "PCI Vendor ID",
      "The vendor ID of the host PCI device to map",
      0, 0xffff,
      0xffff); // vendor id 0xffff = no pci device present
  bx_options.pcidev.Odevice = new bx_param_num_c(BXP_PCIDEV_DEVICE,
      "PCI Device ID",
      "The device ID of the host PCI device to map",
      0, 0xffff,
      0x0);
  /*
  bx_param_c *pcidev_init_list[] = {
    bx_options.pcidev.Ovendor,
    bx_options.pcidev.Odevice,
    NULL
  };
  menu = new bx_list_c (BXP_PCIDEV, "Host PCI Device Mapping Configuration", "", pcidev_init_list);
  menu->get_options ()->set (menu->SHOW_PARENT);
  */

  // NE2K options
  bx_options.ne2k.Opresent = new bx_param_bool_c (BXP_NE2K_PRESENT,
      "Enable NE2K NIC emulation",
      "Enables the NE2K NIC emulation",
      0);
  bx_options.ne2k.Oioaddr = new bx_param_num_c (BXP_NE2K_IOADDR,
      "NE2K I/O Address",
      "I/O base address of the emulated NE2K device",
      0, 0xffff,
      0x240);
  bx_options.ne2k.Oioaddr->set_base (16);
  bx_options.ne2k.Oirq = new bx_param_num_c (BXP_NE2K_IRQ,
      "NE2K Interrupt",
      "IRQ used by the NE2K device",
      0, 15,
      9);
  bx_options.ne2k.Oirq->set_options (bx_param_num_c::USE_SPIN_CONTROL);
  bx_options.ne2k.Omacaddr = new bx_param_string_c (BXP_NE2K_MACADDR,
      "MAC Address",
      "MAC address of the NE2K device. Don't use an address of a machine on your net.",
      "\xfe\xfd\xde\xad\xbe\xef", 6);
  bx_options.ne2k.Omacaddr->get_options ()->set (bx_options.ne2k.Omacaddr->RAW_BYTES);
  bx_options.ne2k.Omacaddr->set_separator (':');
  static char *eth_module_list[] = {
    "null",
#if defined(ETH_LINUX)
    "linux",
#endif
#if HAVE_ETHERTAP
    "tap",
#endif
#if HAVE_TUNTAP
    "tuntap",
#endif
#if defined(ETH_WIN32)
    "win32",
#endif
#if defined(ETH_FBSD)
    "fbsd",
#endif
#ifdef ETH_ARPBACK
    "arpback",
#endif
    "vnet",
    NULL
  };
  bx_options.ne2k.Oethmod = new bx_param_enum_c (BXP_NE2K_ETHMOD,
      "Ethernet module",
      "Module used for the connection to the real net.",
       eth_module_list,
       0,
       0);
  bx_options.ne2k.Oethmod->set_by_name ("null");
  bx_options.ne2k.Oethdev = new bx_param_string_c (BXP_NE2K_ETHDEV,
      "Ethernet device",
      "Device used for the connection to the real net. This is only valid if an ethernet module other than 'null' is used.",
      "xl0", BX_PATHNAME_LEN);
  bx_options.ne2k.Oscript = new bx_param_string_c (BXP_NE2K_SCRIPT,
      "Device configuration script",
      "Name of the script that is executed after Bochs initializes the network interface (optional).",
      "none", BX_PATHNAME_LEN);
#if !BX_WITH_WX
  bx_options.ne2k.Oscript->set_ask_format ("Enter new script name, or 'none': [%s] ");
#endif
  bx_options.pnic.Oenabled = new bx_param_bool_c (BXP_PNIC_ENABLED,
      "Enable Pseudo NIC emulation",
      "Enables the Pseudo NIC emulation",
      0);
  bx_options.pnic.Oioaddr = new bx_param_num_c (BXP_PNIC_IOADDR,
      "Pseudo NIC I/O Address",
      "I/O base address of the emulated Pseudo NIC device",
      0, 0xffff,
      0xdc00);
  bx_options.pnic.Oioaddr->set_base (16);
  bx_options.pnic.Oirq = new bx_param_num_c (BXP_PNIC_IRQ,
      "Pseudo NIC Interrupt",
      "IRQ used by the Pseudo NIC device",
      0, 15,
      11);
  bx_options.pnic.Oirq->set_options (bx_param_num_c::USE_SPIN_CONTROL);
  bx_options.pnic.Omacaddr = new bx_param_string_c (BXP_PNIC_MACADDR,
      "MAC Address",
      "MAC address of the Pseudo NIC device. Don't use an address of a machine on your net.",
      "\xfe\xfd\xde\xad\xbe\xef", 6);
  bx_options.pnic.Omacaddr->get_options ()->set (bx_options.pnic.Omacaddr->RAW_BYTES);
  bx_options.pnic.Omacaddr->set_separator (':');
  bx_options.pnic.Oethmod = new bx_param_enum_c (BXP_PNIC_ETHMOD,
      "Ethernet module",
      "Module used for the connection to the real net.",
       eth_module_list,
       0,
       0);
  bx_options.pnic.Oethmod->set_by_name ("null");
  bx_options.pnic.Oethdev = new bx_param_string_c (BXP_PNIC_ETHDEV,
      "Ethernet device",
      "Device used for the connection to the real net. This is only valid if an ethernet module other than 'null' is used.",
      "xl0", BX_PATHNAME_LEN);
  bx_options.pnic.Oscript = new bx_param_string_c (BXP_PNIC_SCRIPT,
      "Device configuration script",
      "Name of the script that is executed after Bochs initializes the network interface (optional).",
      "none", BX_PATHNAME_LEN);
#if !BX_WITH_WX
  bx_options.pnic.Oscript->set_ask_format ("Enter new script name, or 'none': [%s] ");
#endif
  bx_param_c *ne2k_init_list[] = {
    bx_options.ne2k.Opresent,
    bx_options.ne2k.Oioaddr,
    bx_options.ne2k.Oirq,
    bx_options.ne2k.Omacaddr,
    bx_options.ne2k.Oethmod,
    bx_options.ne2k.Oethdev,
    bx_options.ne2k.Oscript,
    bx_options.pnic.Oenabled,
    bx_options.pnic.Oioaddr,
    bx_options.pnic.Oirq,
    bx_options.pnic.Omacaddr,
    bx_options.pnic.Oethmod,
    bx_options.pnic.Oethdev,
    bx_options.pnic.Oscript,
    NULL
  };
  menu = new bx_list_c (BXP_NE2K, "NE2K Configuration", "", ne2k_init_list);
  menu->get_options ()->set (menu->SHOW_PARENT);
  bx_param_c **ne2k_dependent_list = &ne2k_init_list[1];
  bx_options.ne2k.Opresent->set_dependent_list (
      new bx_list_c (BXP_NULL, "", "", ne2k_dependent_list));
  bx_options.ne2k.Opresent->set_handler (bx_param_handler);
  bx_options.ne2k.Opresent->set (0);

  // SB16 options
  bx_options.sb16.Opresent = new bx_param_bool_c (BXP_SB16_PRESENT,
      "Enable SB16 emulation",
      "Enables the SB16 emulation",
      0);
  bx_options.sb16.Omidifile = new bx_param_filename_c (BXP_SB16_MIDIFILE,
      "MIDI file",
      "The filename is where the MIDI data is sent. This can be device or just a file.",
      "", BX_PATHNAME_LEN);
  bx_options.sb16.Owavefile = new bx_param_filename_c (BXP_SB16_WAVEFILE,
      "Wave file",
      "This is the device/file where the wave output is stored",
      "", BX_PATHNAME_LEN);
  bx_options.sb16.Ologfile = new bx_param_filename_c (BXP_SB16_LOGFILE,
      "Log file",
      "The file to write the SB16 emulator messages to.",
      "", BX_PATHNAME_LEN);
  bx_options.sb16.Omidimode = new bx_param_num_c (BXP_SB16_MIDIMODE,
      "Midi mode",
      "Controls the MIDI output format.",
      0, 3,
      0);
  bx_options.sb16.Owavemode = new bx_param_num_c (BXP_SB16_WAVEMODE,
      "Wave mode",
      "Controls the wave output format.",
      0, 3,
      0);
  bx_options.sb16.Ologlevel = new bx_param_num_c (BXP_SB16_LOGLEVEL,
      "Log level",
      "Controls how verbose the SB16 emulation is (0 = no log, 5 = all errors and infos).",
      0, 5,
      0);
  bx_options.sb16.Odmatimer = new bx_param_num_c (BXP_SB16_DMATIMER,
      "DMA timer",
      "Microseconds per second for a DMA cycle.",
      0, BX_MAX_BIT32U,
      0);

#if BX_WITH_WX
  bx_options.sb16.Omidimode->set_options (bx_param_num_c::USE_SPIN_CONTROL);
  bx_options.sb16.Owavemode->set_options (bx_param_num_c::USE_SPIN_CONTROL);
  bx_options.sb16.Ologlevel->set_options (bx_param_num_c::USE_SPIN_CONTROL);
#endif
  bx_options.sb16.Odmatimer->set_runtime_param (1);
  bx_options.sb16.Odmatimer->set_group ("SB16");
  bx_options.sb16.Ologlevel->set_runtime_param (1);
  bx_options.sb16.Ologlevel->set_group ("SB16");
  bx_param_c *sb16_init_list[] = {
    bx_options.sb16.Opresent,
    bx_options.sb16.Omidimode,
    bx_options.sb16.Omidifile,
    bx_options.sb16.Owavemode,
    bx_options.sb16.Owavefile,
    bx_options.sb16.Ologlevel,
    bx_options.sb16.Ologfile,
    bx_options.sb16.Odmatimer,
    NULL
  };
  menu = new bx_list_c (BXP_SB16, "SB16 Configuration", "", sb16_init_list);
  menu->get_options ()->set (menu->SHOW_PARENT);
  // sb16_dependent_list is a null-terminated list including all the
  // sb16 fields except for the "present" field.  These will all be enabled/
  // disabled according to the value of the present field.
  bx_param_c **sb16_dependent_list = &sb16_init_list[1];
  bx_options.sb16.Opresent->set_dependent_list (
      new bx_list_c (BXP_NULL, "", "", sb16_dependent_list));

  bx_options.log.Ofilename = new bx_param_filename_c (BXP_LOG_FILENAME,
      "Log filename",
      "Pathname of bochs log file",
      "-", BX_PATHNAME_LEN);
  bx_options.log.Ofilename->set_ask_format ("Enter log filename: [%s] ");

  bx_options.log.Oprefix = new bx_param_string_c (BXP_LOG_PREFIX,
      "Log output prefix",
      "Prefix prepended to log output",
      "%t%e%d", BX_PATHNAME_LEN);
  bx_options.log.Oprefix->set_ask_format ("Enter log prefix: [%s] ");

  bx_options.log.Odebugger_filename = new bx_param_filename_c (BXP_DEBUGGER_LOG_FILENAME,
      "Debugger Log filename",
      "Pathname of debugger log file",
      "-", BX_PATHNAME_LEN);
  bx_options.log.Odebugger_filename->set_ask_format ("Enter debugger log filename: [%s] ");

  // loader
  bx_options.load32bitOSImage.OwhichOS = new bx_param_enum_c (BXP_LOAD32BITOS_WHICH,
      "Which operating system?",
      "Which OS to boot",
      loader_os_names,
      Load32bitOSNone,
      Load32bitOSNone);
  bx_options.load32bitOSImage.Opath = new bx_param_filename_c (BXP_LOAD32BITOS_PATH,
      "Pathname of OS to load",
      "Pathname of the 32-bit OS to load",
      "", BX_PATHNAME_LEN);
  bx_options.load32bitOSImage.Oiolog = new bx_param_filename_c (BXP_LOAD32BITOS_IOLOG,
      "Pathname of I/O log file",
      "I/O logfile used for initializing the hardware",
      "", BX_PATHNAME_LEN);
  bx_options.load32bitOSImage.Oinitrd = new bx_param_filename_c (BXP_LOAD32BITOS_INITRD,
      "Pathname of initrd",
      "Pathname of the initial ramdisk",
      "", BX_PATHNAME_LEN);
  bx_param_c *loader_init_list[] = {
    bx_options.load32bitOSImage.OwhichOS,
    bx_options.load32bitOSImage.Opath,
    bx_options.load32bitOSImage.Oiolog,
    bx_options.load32bitOSImage.Oinitrd,
    NULL
  };
  bx_options.load32bitOSImage.OwhichOS->set_format ("os=%s");
  bx_options.load32bitOSImage.Opath->set_format ("path=%s");
  bx_options.load32bitOSImage.Oiolog->set_format ("iolog=%s");
  bx_options.load32bitOSImage.Oinitrd->set_format ("initrd=%s");
  bx_options.load32bitOSImage.OwhichOS->set_ask_format ("Enter OS to load: [%s] ");
  bx_options.load32bitOSImage.Opath->set_ask_format ("Enter pathname of OS: [%s]");
  bx_options.load32bitOSImage.Oiolog->set_ask_format ("Enter pathname of I/O log: [%s] ");
  bx_options.load32bitOSImage.Oinitrd->set_ask_format ("Enter pathname of initrd: [%s] ");
  menu = new bx_list_c (BXP_LOAD32BITOS, "32-bit OS Loader", "", loader_init_list);
  menu->get_options ()->set (menu->SERIES_ASK);
  bx_options.load32bitOSImage.OwhichOS->set_handler (bx_param_handler);
  bx_options.load32bitOSImage.OwhichOS->set (Load32bitOSNone);

  // clock
  bx_options.clock.Otime0 = new bx_param_num_c (BXP_CLOCK_TIME0,
      "clock:time0",
      "Initial time for Bochs CMOS clock, used if you really want two runs to be identical",
      0, BX_MAX_BIT32U,
      BX_CLOCK_TIME0_LOCAL);
  bx_options.clock.Osync = new bx_param_enum_c (BXP_CLOCK_SYNC,
      "clock:sync",
      "Host to guest time synchronization method",
      clock_sync_names,
      BX_CLOCK_SYNC_NONE,
      BX_CLOCK_SYNC_NONE);
  bx_param_c *clock_init_list[] = {
    bx_options.clock.Osync,
    bx_options.clock.Otime0,
    NULL
  };
#if !BX_WITH_WX
  bx_options.clock.Osync->set_format ("sync=%s");
  bx_options.clock.Otime0->set_format ("initial time=%d");
#endif
  bx_options.clock.Otime0->set_ask_format ("Enter Initial CMOS time (1:localtime, 2:utc, other:time in seconds): [%d] ");
  bx_options.clock.Osync->set_ask_format ("Enter Synchronisation method: [%s] ");
  bx_options.clock.Otime0->set_label ("Initial CMOS time for Bochs\n(1:localtime, 2:utc, other:time in seconds)");
  bx_options.clock.Osync->set_label ("Synchronisation method");
  menu = new bx_list_c (BXP_CLOCK, "Clock parameters", "", clock_init_list);
  menu->get_options ()->set (menu->SERIES_ASK);

  // other
  bx_options.Okeyboard_serial_delay = new bx_param_num_c (BXP_KBD_SERIAL_DELAY,
      "Keyboard serial delay",
      "Approximate time in microseconds that it takes one character to be transfered from the keyboard to controller over the serial path.",
      1, BX_MAX_BIT32U,
      20000);
  bx_options.Okeyboard_paste_delay = new bx_param_num_c (BXP_KBD_PASTE_DELAY,
      "Keyboard paste delay",
      "Approximate time in microseconds between attemps to paste characters to the keyboard controller.",
      1000, BX_MAX_BIT32U,
      100000);
  bx_options.Okeyboard_paste_delay->set_handler (bx_param_handler);
  bx_options.Okeyboard_paste_delay->set_runtime_param (1);
  bx_options.Ofloppy_command_delay = new bx_param_num_c (BXP_FLOPPY_CMD_DELAY,
      "Floppy command delay",
      "Time in microseconds to wait before completing some floppy commands such as read/write/seek/etc, which normally have a delay associated.  This used to be hardwired to 50,000 before.",
      1, BX_MAX_BIT32U,
      50000);
  bx_options.Oi440FXSupport = new bx_param_bool_c (BXP_I440FX_SUPPORT,
      "PCI i440FX Support",
      "Controls whether to emulate the i440FX PCI chipset",
      0);
  bx_options.cmos.OcmosImage = new bx_param_bool_c (BXP_CMOS_IMAGE,
      "Use a CMOS image",
      "Controls the usage of a CMOS image",
      0);
  bx_options.cmos.Opath = new bx_param_filename_c (BXP_CMOS_PATH,
      "Pathname of CMOS image",
      "Pathname of CMOS image",
      "", BX_PATHNAME_LEN);
  deplist = new bx_list_c (BXP_NULL, 1);
  deplist->add (bx_options.cmos.Opath);
  bx_options.cmos.OcmosImage->set_dependent_list (deplist);

  // Keyboard mapping
  bx_options.keyboard.OuseMapping = new bx_param_bool_c(BXP_KEYBOARD_USEMAPPING,
      "Use keyboard mapping",
      "Controls whether to use the keyboard mapping feature",
      0);
  bx_options.keyboard.Okeymap = new bx_param_filename_c (BXP_KEYBOARD_MAP,
      "Keymap filename",
      "Pathname of the keymap file used",
      "", BX_PATHNAME_LEN);
  deplist = new bx_list_c (BXP_NULL, 1);
  deplist->add (bx_options.keyboard.Okeymap);
  bx_options.keyboard.OuseMapping->set_dependent_list (deplist);

 // Keyboard type
  bx_options.Okeyboard_type = new bx_param_enum_c (BXP_KBD_TYPE,
      "Keyboard type",
      "Keyboard type reported by the 'identify keyboard' command",
      keyboard_type_names,
      BX_KBD_MF_TYPE,
      BX_KBD_XT_TYPE);
  bx_options.Okeyboard_type->set_ask_format ("Enter keyboard type: [%s] ");

  // Userbutton shortcut
  bx_options.Ouser_shortcut = new bx_param_string_c (BXP_USER_SHORTCUT,
      "Userbutton shortcut",
      "Defines the keyboard shortcut to be sent when you press the 'user' button in the headerbar.",
      "none", 16);
  bx_options.Ouser_shortcut->set_runtime_param (1);

  // GDB stub
  bx_options.gdbstub.port = 1234;
  bx_options.gdbstub.text_base = 0;
  bx_options.gdbstub.data_base = 0;
  bx_options.gdbstub.bss_base = 0;

  bx_param_c *keyboard_init_list[] = {
      bx_options.Okeyboard_serial_delay,
      bx_options.Okeyboard_paste_delay,
      bx_options.keyboard.OuseMapping,
      bx_options.keyboard.Okeymap,
      bx_options.Okeyboard_type,
      bx_options.Ouser_shortcut,
      NULL
  };
  menu = new bx_list_c (BXP_MENU_KEYBOARD, "Configure Keyboard", "", keyboard_init_list);
  menu->get_options ()->set (menu->SHOW_PARENT);

  bx_param_c *other_init_list[] = {
      bx_options.Ofloppy_command_delay,
      bx_options.Oi440FXSupport,
      bx_options.cmos.OcmosImage,
      bx_options.cmos.Opath,
      SIM->get_param (BXP_CLOCK),
      SIM->get_param (BXP_LOAD32BITOS),
      NULL
  };
  menu = new bx_list_c (BXP_MENU_MISC, "Configure Everything Else", "", other_init_list);
  menu->get_options ()->set (menu->SHOW_PARENT);

#if BX_WITH_WX
  bx_param_c *other_init_list2[] = {
//    bx_options.Osel_config,
//    bx_options.Osel_displaylib,
      bx_options.Ovga_update_interval,
      bx_options.log.Oprefix,
      bx_options.Omouse_enabled,
      bx_options.OfloppySigCheck,
      bx_options.Ofloppy_command_delay,
      bx_options.OnewHardDriveSupport,
      bx_options.Oprivate_colormap,
#if BX_WITH_AMIGAOS
      bx_options.Ofullscreen,
      bx_options.Oscreenmode,
#endif
      bx_options.Oi440FXSupport,
      bx_options.cmos.OcmosImage,
      bx_options.cmos.Opath,
      NULL
  };
  menu = new bx_list_c (BXP_MENU_MISC_2, "Other options", "", other_init_list2);
#endif
  bx_param_c *runtime_init_list[] = {
      bx_options.Ovga_update_interval,
      bx_options.Omouse_enabled,
      bx_options.Okeyboard_paste_delay,
      bx_options.Ouser_shortcut,
      bx_options.sb16.Odmatimer,
      bx_options.sb16.Ologlevel,
      NULL
  };
  menu = new bx_list_c (BXP_MENU_RUNTIME, "Misc runtime options", "", runtime_init_list);
  menu->get_options ()->set (menu->SHOW_PARENT | menu->SHOW_GROUP_NAME);
}

void bx_reset_options ()
{
  Bit8u i;

  // drives
  bx_options.floppya.Opath->reset();
  bx_options.floppya.Odevtype->reset();
  bx_options.floppya.Otype->reset();
  bx_options.floppya.Ostatus->reset();
  bx_options.floppyb.Opath->reset();
  bx_options.floppyb.Odevtype->reset();
  bx_options.floppyb.Otype->reset();
  bx_options.floppyb.Ostatus->reset();

  for (Bit8u channel=0; channel<BX_MAX_ATA_CHANNEL; channel++) {
    bx_options.ata[channel].Opresent->reset();
    bx_options.ata[channel].Oioaddr1->reset();
    bx_options.ata[channel].Oioaddr2->reset();
    bx_options.ata[channel].Oirq->reset();

    for (Bit8u slave=0; slave<2; slave++) {
      bx_options.atadevice[channel][slave].Opresent->reset();
      bx_options.atadevice[channel][slave].Otype->reset();
      bx_options.atadevice[channel][slave].Omode->reset();
      bx_options.atadevice[channel][slave].Opath->reset();
      bx_options.atadevice[channel][slave].Ocylinders->reset();
      bx_options.atadevice[channel][slave].Oheads->reset();
      bx_options.atadevice[channel][slave].Ospt->reset();
      bx_options.atadevice[channel][slave].Ostatus->reset();
      bx_options.atadevice[channel][slave].Omodel->reset();
      bx_options.atadevice[channel][slave].Obiosdetect->reset();
      bx_options.atadevice[channel][slave].Otranslation->reset();
      }
    }
  bx_options.OnewHardDriveSupport->reset();

  // boot & memory
  bx_options.Obootdrive->reset();
  bx_options.OfloppySigCheck->reset();
  bx_options.memory.Osize->reset();

  // standard ports
  for (i=0; i<BX_N_SERIAL_PORTS; i++) {
    bx_options.com[i].Oenabled->reset();
    bx_options.com[i].Odev->reset();
    }
  for (i=0; i<BX_N_PARALLEL_PORTS; i++) {
    bx_options.par[i].Oenabled->reset();
    bx_options.par[i].Ooutfile->reset();
    }

  // rom images
  bx_options.rom.Opath->reset();
  bx_options.rom.Oaddress->reset();
  bx_options.optrom[0].Opath->reset();
  bx_options.optrom[0].Oaddress->reset();
  bx_options.optrom[1].Opath->reset();
  bx_options.optrom[1].Oaddress->reset();
  bx_options.optrom[2].Opath->reset();
  bx_options.optrom[2].Oaddress->reset();
  bx_options.optrom[3].Opath->reset();
  bx_options.optrom[3].Oaddress->reset();
  bx_options.vgarom.Opath->reset();

  // interface
  bx_options.Odisplaylib_options->reset();
  bx_options.Ovga_update_interval->reset();
  bx_options.Omouse_enabled->reset();
  bx_options.Oips->reset();
  bx_options.Oprivate_colormap->reset();
#if BX_WITH_AMIGAOS
  bx_options.Ofullscreen->reset();
  bx_options.Oscreenmode->reset();
#endif

  // ne2k
  bx_options.ne2k.Opresent->reset();
  bx_options.ne2k.Oioaddr->reset();
  bx_options.ne2k.Oirq->reset();
  bx_options.ne2k.Omacaddr->reset();
  bx_options.ne2k.Oethmod->reset();
  bx_options.ne2k.Oethdev->reset();
  bx_options.ne2k.Oscript->reset();

  // pcidev
  bx_options.pcidev.Ovendor->reset();
  bx_options.pcidev.Odevice->reset();
  
  // SB16
  bx_options.sb16.Opresent->reset();
  bx_options.sb16.Omidifile->reset();
  bx_options.sb16.Owavefile->reset();
  bx_options.sb16.Ologfile->reset();
  bx_options.sb16.Omidimode->reset();
  bx_options.sb16.Owavemode->reset();
  bx_options.sb16.Ologlevel->reset();
  bx_options.sb16.Odmatimer->reset();

  // logfile
  bx_options.log.Ofilename->reset();
  bx_options.log.Oprefix->reset();
  bx_options.log.Odebugger_filename->reset();

  // loader
  bx_options.load32bitOSImage.OwhichOS->reset();
  bx_options.load32bitOSImage.Opath->reset();
  bx_options.load32bitOSImage.Oiolog->reset();
  bx_options.load32bitOSImage.Oinitrd->reset();

  // keyboard
  bx_options.Okeyboard_serial_delay->reset();
  bx_options.Okeyboard_paste_delay->reset();
  bx_options.keyboard.OuseMapping->reset();
  bx_options.keyboard.Okeymap->reset();
  bx_options.Okeyboard_type->reset();
  bx_options.Ouser_shortcut->reset();

  // Clock
  bx_options.clock.Otime0->reset();
  bx_options.clock.Osync->reset();

  // other
  bx_options.Ofloppy_command_delay->reset();
  bx_options.Oi440FXSupport->reset();
  bx_options.cmos.OcmosImage->reset();
  bx_options.cmos.Opath->reset();
  bx_options.Otext_snapshot_check->reset();
}

int
bx_read_configuration (char *rcfile)
{
  // parse rcfile first, then parse arguments in order.
  BX_INFO (("reading configuration from %s", rcfile));
  if (parse_bochsrc(rcfile) < 0) {
    BX_PANIC (("reading from %s failed", rcfile));
    return -1;
  }
  // update log actions
  for (int level=0; level<N_LOGLEV; level++) {
    int action = SIM->get_default_log_action (level);
    io->set_log_action (level, action);
  }
  return 0;
}

int bx_parse_cmdline (int arg, int argc, char *argv[])
{
  //if (arg < argc) BX_INFO (("parsing command line arguments"));

  while (arg < argc) {
    BX_INFO (("parsing arg %d, %s", arg, argv[arg]));
    parse_line_unformatted("cmdline args", argv[arg]);
    arg++;
  }
  // update log actions
  for (int level=0; level<N_LOGLEV; level++) {
    int action = SIM->get_default_log_action (level);
    io->set_log_action (level, action);
  }
  return 0;
}

#if BX_PROVIDE_MAIN
char *
bx_find_bochsrc ()
{
  FILE *fd = NULL;
  char rcfile[512];
  Bit32u retry = 0, found = 0;
  // try several possibilities for the bochsrc before giving up
  while (!found) {
    rcfile[0] = 0;
    switch (retry++) {
    case 0: strcpy (rcfile, ".bochsrc"); break;
    case 1: strcpy (rcfile, "bochsrc"); break;
    case 2: strcpy (rcfile, "bochsrc.txt"); break;
#ifdef WIN32
    case 3: strcpy (rcfile, "bochsrc.bxrc"); break;
#elif !BX_WITH_MACOS
      // only try this on unix
    case 3:
      {
      char *ptr = getenv("HOME");
      if (ptr) snprintf (rcfile, sizeof(rcfile), "%s/.bochsrc", ptr);
      }
      break;
     case 4: strcpy (rcfile, "/etc/bochsrc"); break;
#endif
    default:
      return NULL;
    }
    if (rcfile[0]) {
      BX_DEBUG (("looking for configuration in %s", rcfile));
      fd = fopen(rcfile, "r");
      if (fd) found = 1;
    }
  }
  assert (fd != NULL && rcfile[0] != 0);
  fclose (fd);
  return strdup (rcfile);
}

  static int
parse_bochsrc(char *rcfile)
{
  FILE *fd = NULL;
  char *ret;
  char line[512];

  // try several possibilities for the bochsrc before giving up

  bochsrc_include_count++;

  fd = fopen (rcfile, "r");
  if (fd == NULL) return -1;

  int retval = 0;
  do {
    ret = fgets(line, sizeof(line)-1, fd);
    line[sizeof(line) - 1] = '\0';
    int len = strlen(line);
    if (len>0)
      line[len-1] = '\0';
    if ((ret != NULL) && strlen(line)) {
      if (parse_line_unformatted(rcfile, line) < 0) {
        retval = -1;
        break;  // quit parsing after first error
        }
      }
    } while (!feof(fd));
  fclose(fd);
  bochsrc_include_count--;
  return retval;
}

  static char *
get_builtin_variable(char *varname)
{
#ifdef WIN32
  int code;
  DWORD size;
  DWORD type = 0;
  HKEY hkey;
  char keyname[80];
  static char data[MAX_PATH];
#endif

  if (strlen(varname)<1) return NULL;
  else {
    if (!strcmp(varname, "BXSHARE")) {
#ifdef WIN32
      wsprintf(keyname, "Software\\Bochs %s", VER_STRING);
      code = RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyname, 0, KEY_READ, &hkey);
      if (code == ERROR_SUCCESS) {
        data[0] = 0;
        size = MAX_PATH;
        if (RegQueryValueEx(hkey, "", NULL, (LPDWORD)&type, (LPBYTE)data,
                            (LPDWORD)&size ) == ERROR_SUCCESS ) {
          RegCloseKey(hkey);
          return data;
        } else {
          RegCloseKey(hkey);
          return NULL;
        }
      } else {
        return NULL;
      }
#else
      return BX_SHARE_PATH;
#endif
    }
    return NULL;
  }
}

  static Bit32s
parse_line_unformatted(char *context, char *line)
{
#define MAX_PARAMS_LEN 40
  char *ptr;
  unsigned i, string_i = 0;
  char string[512];
  char *params[MAX_PARAMS_LEN];
  int num_params;
  bx_bool inquotes = 0;
  bx_bool comment = 0;

  memset(params, 0, sizeof(params));
  if (line == NULL) return 0;

  // if passed nothing but whitespace, just return
  for (i=0; i<strlen(line); i++) {
    if (!isspace(line[i])) break;
    }
  if (i>=strlen(line))
    return 0;

  num_params = 0;

  if (!strncmp(line, "#include", 8))
    ptr = strtok(line, " ");
  else
    ptr = strtok(line, ":");
  while ((ptr) && (!comment)) {
    if (!inquotes) {
      string_i = 0;
    } else {
      string[string_i++] = ',';
    }
    for (i=0; i<strlen(ptr); i++) {
      if (ptr[i] == '"')
        inquotes = !inquotes;
      else if ((ptr[i] == '#') && (strncmp(line+i, "#include", 8)) && !inquotes) {
        comment = 1;
        break;
      } else {
#if BX_HAVE_GETENV
        // substitute environment variables.
        if (ptr[i] == '$') {
          char varname[512];
          char *pv = varname;
          char *value;
          *pv = 0;
          i++;
          while (isalpha(ptr[i]) || ptr[i]=='_') {
            *pv = ptr[i]; pv++; i++;
          }
          *pv = 0;
          if (strlen(varname)<1 || !(value = getenv(varname))) {
            if ((value = get_builtin_variable(varname))) {
              // append value to the string
              for (pv=value; *pv; pv++)
                  string[string_i++] = *pv;
            } else {
              BX_PANIC (("could not look up environment variable '%s'", varname));
            }
          } else {
            // append value to the string
            for (pv=value; *pv; pv++)
                string[string_i++] = *pv;
          }
        }
#endif
        if (!isspace(ptr[i]) || inquotes) {
          string[string_i++] = ptr[i];
        }
      }
    }
    string[string_i] = '\0';
    if (string_i == 0) break;
    if (!inquotes) {
      if (params[num_params] != NULL) {
        free(params[num_params]);
        params[num_params] = NULL;
      }
      if (num_params < MAX_PARAMS_LEN) {
        params[num_params++] = strdup (string);
      } else {
        BX_PANIC (("too many parameters, max is %d\n", MAX_PARAMS_LEN));
      }
    }
    ptr = strtok(NULL, ",");
  }
  Bit32s retval = parse_line_formatted(context, num_params, &params[0]);
  for (i=0; i < MAX_PARAMS_LEN; i++)
  {
    if ( params[i] != NULL )
    {
        free(params[i]);
        params[i] = NULL;
    }
  }
  return retval;
}

// These macros are called for all parse errors, so that we can easily
// change the behavior of all occurrences.
#define PARSE_ERR(x)  \
  do { BX_PANIC(x); return -1; } while (0)
#define PARSE_WARN(x)  \
  BX_ERROR(x)

  static Bit32s
parse_line_formatted(char *context, int num_params, char *params[])
{
  int i, slot;

  if (num_params < 1) return 0;
  if (num_params < 2) {
    PARSE_ERR(("%s: a bochsrc option needs at least one parameter", context));
  }

  if (!strcmp(params[0], "#include")) {
    if (num_params != 2) {
      PARSE_ERR(("%s: ignoring malformed #include directive.", context));
      }
    if (!strcmp(params[1], context)) {
      PARSE_ERR(("%s: cannot include this file again.", context));
      }
    if (bochsrc_include_count == 2) {
      PARSE_ERR(("%s: include directive in an included file not supported yet.", context));
      }
    bx_read_configuration(params[1]);
    }
  else if (!strcmp(params[0], "floppya")) {
    for (i=1; i<num_params; i++) {
      if (!strncmp(params[i], "2_88=", 5)) {
        bx_options.floppya.Opath->set (&params[i][5]);
        bx_options.floppya.Otype->set (BX_FLOPPY_2_88);
        }
      else if (!strncmp(params[i], "1_44=", 5)) {
        bx_options.floppya.Opath->set (&params[i][5]);
        bx_options.floppya.Otype->set (BX_FLOPPY_1_44);
        }
      else if (!strncmp(params[i], "1_2=", 4)) {
        bx_options.floppya.Opath->set (&params[i][4]);
        bx_options.floppya.Otype->set (BX_FLOPPY_1_2);
        }
      else if (!strncmp(params[i], "720k=", 5)) {
        bx_options.floppya.Opath->set (&params[i][5]);
        bx_options.floppya.Otype->set (BX_FLOPPY_720K);
        }
      else if (!strncmp(params[i], "360k=", 5)) {
        bx_options.floppya.Opath->set (&params[i][5]);
        bx_options.floppya.Otype->set (BX_FLOPPY_360K);
        }
      // use CMOS reserved types?
      else if (!strncmp(params[i], "160k=", 5)) {
        bx_options.floppya.Opath->set (&params[i][5]);
        bx_options.floppya.Otype->set (BX_FLOPPY_160K);
        }
      else if (!strncmp(params[i], "180k=", 5)) {
        bx_options.floppya.Opath->set (&params[i][5]);
        bx_options.floppya.Otype->set (BX_FLOPPY_180K);
        }
      else if (!strncmp(params[i], "320k=", 5)) {
        bx_options.floppya.Opath->set (&params[i][5]);
        bx_options.floppya.Otype->set (BX_FLOPPY_320K);
        }
      else if (!strncmp(params[i], "status=ejected", 14)) {
        bx_options.floppya.Ostatus->set (BX_EJECTED);
        }
      else if (!strncmp(params[i], "status=inserted", 15)) {
        bx_options.floppya.Ostatus->set (BX_INSERTED);
        }
      else {
        PARSE_ERR(("%s: floppya attribute '%s' not understood.", context,
          params[i]));
        }
      }
    }
   else if (!strcmp(params[0], "gdbstub_port"))
     {
       if (num_params != 2)
         {
            fprintf(stderr, ".bochsrc: gdbstub_port directive: wrong # args.\n");
            exit(1);
         }
       bx_options.gdbstub.port = atoi(params[1]);
     }
   else if (!strcmp(params[0], "gdbstub_text_base"))
     {
       if (num_params != 2)
         {
            fprintf(stderr, ".bochsrc: gdbstub_text_base directive: wrong # args.\n");
            exit(1);
         }
       bx_options.gdbstub.text_base = atoi(params[1]);
     }
   else if (!strcmp(params[0], "gdbstub_data_base"))
     {
       if (num_params != 2)
         {
            fprintf(stderr, ".bochsrc: gdbstub_data_base directive: wrong # args.\n");
            exit(1);
         }
       bx_options.gdbstub.data_base = atoi(params[1]);
     }
   else if (!strcmp(params[0], "gdbstub_bss_base"))
     {
       if (num_params != 2)
         {
            fprintf(stderr, ".bochsrc: gdbstub_bss_base directive: wrong # args.\n");
            exit(1);
         }
       bx_options.gdbstub.bss_base = atoi(params[1]);
     }

  else if (!strcmp(params[0], "floppyb")) {
    for (i=1; i<num_params; i++) {
      if (!strncmp(params[i], "2_88=", 5)) {
        bx_options.floppyb.Opath->set (&params[i][5]);
        bx_options.floppyb.Otype->set (BX_FLOPPY_2_88);
        }
      else if (!strncmp(params[i], "1_44=", 5)) {
        bx_options.floppyb.Opath->set (&params[i][5]);
        bx_options.floppyb.Otype->set (BX_FLOPPY_1_44);
        }
      else if (!strncmp(params[i], "1_2=", 4)) {
        bx_options.floppyb.Opath->set (&params[i][4]);
        bx_options.floppyb.Otype->set (BX_FLOPPY_1_2);
        }
      else if (!strncmp(params[i], "720k=", 5)) {
        bx_options.floppyb.Opath->set (&params[i][5]);
        bx_options.floppyb.Otype->set (BX_FLOPPY_720K);
        }
      else if (!strncmp(params[i], "360k=", 5)) {
        bx_options.floppyb.Opath->set (&params[i][5]);
        bx_options.floppyb.Otype->set (BX_FLOPPY_360K);
        }
      // use CMOS reserved types?
      else if (!strncmp(params[i], "160k=", 5)) {
        bx_options.floppyb.Opath->set (&params[i][5]);
        bx_options.floppyb.Otype->set (BX_FLOPPY_160K);
        }
      else if (!strncmp(params[i], "180k=", 5)) {
        bx_options.floppyb.Opath->set (&params[i][5]);
        bx_options.floppyb.Otype->set (BX_FLOPPY_180K);
        }
      else if (!strncmp(params[i], "320k=", 5)) {
        bx_options.floppyb.Opath->set (&params[i][5]);
        bx_options.floppyb.Otype->set (BX_FLOPPY_320K);
        }
      else if (!strncmp(params[i], "status=ejected", 14)) {
        bx_options.floppyb.Ostatus->set (BX_EJECTED);
        }
      else if (!strncmp(params[i], "status=inserted", 15)) {
        bx_options.floppyb.Ostatus->set (BX_INSERTED);
        }
      else {
        PARSE_ERR(("%s: floppyb attribute '%s' not understood.", context,
          params[i]));
        }
      }
    }

  else if ((!strncmp(params[0], "ata", 3)) && (strlen(params[0]) == 4)) {
    Bit8u channel = params[0][3];

    if ((channel < '0') || (channel > '9')) {
      PARSE_ERR(("%s: ataX directive malformed.", context));
      }
    channel-='0';
    if (channel >= BX_MAX_ATA_CHANNEL) {
      PARSE_ERR(("%s: ataX directive malformed.", context));
      }

    if ((num_params < 2) || (num_params > 5)) {
      PARSE_ERR(("%s: ataX directive malformed.", context));
      }

    if (strncmp(params[1], "enabled=", 8)) {
      PARSE_ERR(("%s: ataX directive malformed.", context));
      }
    else {
      bx_options.ata[channel].Opresent->set (atol(&params[1][8]));
      }

    if (num_params > 2) {
      if (strncmp(params[2], "ioaddr1=", 8)) {
        PARSE_ERR(("%s: ataX directive malformed.", context));
        }
      else {
        if ( (params[2][8] == '0') && (params[2][9] == 'x') )
          bx_options.ata[channel].Oioaddr1->set (strtoul (&params[2][8], NULL, 16));
        else
          bx_options.ata[channel].Oioaddr1->set (strtoul (&params[2][8], NULL, 10));
        }
      }

    if (num_params > 3) {
      if (strncmp(params[3], "ioaddr2=", 8)) {
        PARSE_ERR(("%s: ataX directive malformed.", context));
        }
      else {
        if ( (params[3][8] == '0') && (params[3][9] == 'x') )
          bx_options.ata[channel].Oioaddr2->set (strtoul (&params[3][8], NULL, 16));
        else
          bx_options.ata[channel].Oioaddr2->set (strtoul (&params[3][8], NULL, 10));
        }
      }

    if (num_params > 4) {
      if (strncmp(params[4], "irq=", 4)) {
        PARSE_ERR(("%s: ataX directive malformed.", context));
        }
      else {
        bx_options.ata[channel].Oirq->set (atol(&params[4][4]));
        }
      }
    }

  // ataX-master, ataX-slave
  else if ((!strncmp(params[0], "ata", 3)) && (strlen(params[0]) > 4)) {
    Bit8u channel = params[0][3], slave = 0;

    if ((channel < '0') || (channel > '9')) {
      PARSE_ERR(("%s: ataX-master/slave directive malformed.", context));
      }
    channel-='0';
    if (channel >= BX_MAX_ATA_CHANNEL) {
      PARSE_ERR(("%s: ataX-master/slave directive malformed.", context));
      }

    if ((strcmp(&params[0][4], "-slave")) &&
        (strcmp(&params[0][4], "-master"))) {
      PARSE_ERR(("%s: ataX-master/slave directive malformed.", context));
      }

    if (!strcmp(&params[0][4], "-slave")) {
      slave = 1;
      }

    // This was originally meant to warn users about both diskc
    // and ata0-master defined, but it also prevent users to
    // override settings on the command line 
    // (see [ 661010 ] cannot override ata-settings from cmdline)
    // if (bx_options.atadevice[channel][slave].Opresent->get()) {
    //   BX_INFO(("%s: %s device of ata channel %d already defined.", context, slave?"slave":"master",channel));
    //   }

    for (i=1; i<num_params; i++) {
      if (!strcmp(params[i], "type=disk")) {
        bx_options.atadevice[channel][slave].Otype->set (BX_ATA_DEVICE_DISK);
        }
      else if (!strcmp(params[i], "type=cdrom")) {
        bx_options.atadevice[channel][slave].Otype->set (BX_ATA_DEVICE_CDROM);
        }
      else if (!strcmp(params[i], "mode=flat")) {
        bx_options.atadevice[channel][slave].Omode->set (BX_ATA_MODE_FLAT);
        }
      else if (!strcmp(params[i], "mode=concat")) {
        bx_options.atadevice[channel][slave].Omode->set (BX_ATA_MODE_CONCAT);
        }
      else if (!strcmp(params[i], "mode=external")) {
        bx_options.atadevice[channel][slave].Omode->set (BX_ATA_MODE_EXTDISKSIM);
        }
      else if (!strcmp(params[i], "mode=dll")) {
        bx_options.atadevice[channel][slave].Omode->set (BX_ATA_MODE_DLL_HD);
        }
      else if (!strcmp(params[i], "mode=sparse")) {
        bx_options.atadevice[channel][slave].Omode->set (BX_ATA_MODE_SPARSE);
        }
      else if (!strcmp(params[i], "mode=vmware3")) {
        bx_options.atadevice[channel][slave].Omode->set (BX_ATA_MODE_VMWARE3);
        }
//      else if (!strcmp(params[i], "mode=split")) {
//        bx_options.atadevice[channel][slave].Omode->set (BX_ATA_MODE_SPLIT);
//        }
      else if (!strcmp(params[i], "mode=undoable")) {
        bx_options.atadevice[channel][slave].Omode->set (BX_ATA_MODE_UNDOABLE);
        }
      else if (!strcmp(params[i], "mode=growing")) {
        bx_options.atadevice[channel][slave].Omode->set (BX_ATA_MODE_GROWING);
        }
      else if (!strcmp(params[i], "mode=volatile")) {
        bx_options.atadevice[channel][slave].Omode->set (BX_ATA_MODE_VOLATILE);
        }
//      else if (!strcmp(params[i], "mode=z-undoable")) {
//        bx_options.atadevice[channel][slave].Omode->set (BX_ATA_MODE_Z_UNDOABLE);
//        }
//      else if (!strcmp(params[i], "mode=z-volatile")) {
//        bx_options.atadevice[channel][slave].Omode->set (BX_ATA_MODE_Z_VOLATILE);
//        }
      else if (!strncmp(params[i], "path=", 5)) {
        bx_options.atadevice[channel][slave].Opath->set (&params[i][5]);
        }
      else if (!strncmp(params[i], "cylinders=", 10)) {
        bx_options.atadevice[channel][slave].Ocylinders->set (atol(&params[i][10]));
        }
      else if (!strncmp(params[i], "heads=", 6)) {
        bx_options.atadevice[channel][slave].Oheads->set (atol(&params[i][6]));
        }
      else if (!strncmp(params[i], "spt=", 4)) {
        bx_options.atadevice[channel][slave].Ospt->set (atol(&params[i][4]));
        }
      else if (!strncmp(params[i], "model=", 6)) {
        bx_options.atadevice[channel][slave].Omodel->set(&params[i][6]);
        }
      else if (!strcmp(params[i], "biosdetect=none")) {
        bx_options.atadevice[channel][slave].Obiosdetect->set(BX_ATA_BIOSDETECT_NONE);
        }
      else if (!strcmp(params[i], "biosdetect=cmos")) {
        bx_options.atadevice[channel][slave].Obiosdetect->set(BX_ATA_BIOSDETECT_CMOS);
        }
      else if (!strcmp(params[i], "biosdetect=auto")) {
        bx_options.atadevice[channel][slave].Obiosdetect->set(BX_ATA_BIOSDETECT_AUTO);
        }
      else if (!strcmp(params[i], "translation=none")) {
        bx_options.atadevice[channel][slave].Otranslation->set(BX_ATA_TRANSLATION_NONE);
        }
      else if (!strcmp(params[i], "translation=lba")) {
        bx_options.atadevice[channel][slave].Otranslation->set(BX_ATA_TRANSLATION_LBA);
        }
      else if (!strcmp(params[i], "translation=large")) { 
        bx_options.atadevice[channel][slave].Otranslation->set(BX_ATA_TRANSLATION_LARGE);
        }
      else if (!strcmp(params[i], "translation=echs")) { // synonym of large
        bx_options.atadevice[channel][slave].Otranslation->set(BX_ATA_TRANSLATION_LARGE);
        }
      else if (!strcmp(params[i], "translation=rechs")) {
        bx_options.atadevice[channel][slave].Otranslation->set(BX_ATA_TRANSLATION_RECHS);
        }
      else if (!strcmp(params[i], "translation=auto")) {
        bx_options.atadevice[channel][slave].Otranslation->set(BX_ATA_TRANSLATION_AUTO);
        }
      else if (!strcmp(params[i], "status=ejected")) {
        bx_options.atadevice[channel][slave].Ostatus->set(BX_EJECTED);
        }
      else if (!strcmp(params[i], "status=inserted")) {
        bx_options.atadevice[channel][slave].Ostatus->set(BX_INSERTED);
        }
      else if (!strncmp(params[i], "journal=", 8)) {
        bx_options.atadevice[channel][slave].Ojournal->set(&params[i][8]);
        }
      else {
        PARSE_ERR(("%s: ataX-master/slave directive malformed.", context));
        }
      }

    // Enables the ata device
    bx_options.atadevice[channel][slave].Opresent->set(1);

    // if enabled, check if device ok
    if (bx_options.atadevice[channel][slave].Opresent->get() == 1) {
      if (bx_options.atadevice[channel][slave].Otype->get() == BX_ATA_DEVICE_DISK) {
        if (strlen(bx_options.atadevice[channel][slave].Opath->getptr()) ==0)
          PARSE_WARN(("%s: ataX-master/slave has empty path", context));
        if ((bx_options.atadevice[channel][slave].Ocylinders->get() == 0) ||
            (bx_options.atadevice[channel][slave].Oheads->get() ==0 ) ||
            (bx_options.atadevice[channel][slave].Ospt->get() == 0)) {
          PARSE_WARN(("%s: ataX-master/slave cannot have zero cylinders, heads, or sectors/track", context));
          }
        }
      else if (bx_options.atadevice[channel][slave].Otype->get() == BX_ATA_DEVICE_CDROM) {
        if (strlen(bx_options.atadevice[channel][slave].Opath->getptr()) == 0) {
          PARSE_WARN(("%s: ataX-master/slave has empty path", context));
          }
        }
      else {
        PARSE_WARN(("%s: ataX-master/slave: type should be specified", context));
        }
      }
    }

  // Legacy disk options emulation
  else if (!strcmp(params[0], "diskc")) { // DEPRECATED
    BX_INFO(("WARNING: diskc directive is deprecated, use ata0-master: instead"));
    if (bx_options.atadevice[0][0].Opresent->get()) {
      PARSE_ERR(("%s: master device of ata channel 0 already defined.", context));
      }
    if (num_params != 5) {
      PARSE_ERR(("%s: diskc directive malformed.", context));
      }
    if (strncmp(params[1], "file=", 5) ||
        strncmp(params[2], "cyl=", 4) ||
        strncmp(params[3], "heads=", 6) ||
        strncmp(params[4], "spt=", 4)) {
      PARSE_ERR(("%s: diskc directive malformed.", context));
      }
    bx_options.ata[0].Opresent->set(1);
    bx_options.atadevice[0][0].Otype->set (BX_ATA_DEVICE_DISK);
    bx_options.atadevice[0][0].Opath->set (&params[1][5]);
    bx_options.atadevice[0][0].Ocylinders->set (atol(&params[2][4]));
    bx_options.atadevice[0][0].Oheads->set     (atol(&params[3][6]));
    bx_options.atadevice[0][0].Ospt->set       (atol(&params[4][4]));
    bx_options.atadevice[0][0].Opresent->set (1);
    }
  else if (!strcmp(params[0], "diskd")) { // DEPRECATED
    BX_INFO(("WARNING: diskd directive is deprecated, use ata0-slave: instead"));
    if (bx_options.atadevice[0][1].Opresent->get()) {
      PARSE_ERR(("%s: slave device of ata channel 0 already defined.", context));
      }
    if (num_params != 5) {
      PARSE_ERR(("%s: diskd directive malformed.", context));
      }
    if (strncmp(params[1], "file=", 5) ||
        strncmp(params[2], "cyl=", 4) ||
        strncmp(params[3], "heads=", 6) ||
        strncmp(params[4], "spt=", 4)) {
      PARSE_ERR(("%s: diskd directive malformed.", context));
      }
    bx_options.ata[0].Opresent->set(1);
    bx_options.atadevice[0][1].Otype->set (BX_ATA_DEVICE_DISK);
    bx_options.atadevice[0][1].Opath->set (&params[1][5]);
    bx_options.atadevice[0][1].Ocylinders->set (atol( &params[2][4]));
    bx_options.atadevice[0][1].Oheads->set     (atol( &params[3][6]));
    bx_options.atadevice[0][1].Ospt->set       (atol( &params[4][4]));
    bx_options.atadevice[0][1].Opresent->set (1);
    }
  else if (!strcmp(params[0], "cdromd")) { // DEPRECATED
    BX_INFO(("WARNING: cdromd directive is deprecated, use ata0-slave: instead"));
    if (bx_options.atadevice[0][1].Opresent->get()) {
      PARSE_ERR(("%s: slave device of ata channel 0 already defined.", context));
      }
    if (num_params != 3) {
      PARSE_ERR(("%s: cdromd directive malformed.", context));
      }
    if (strncmp(params[1], "dev=", 4) || strncmp(params[2], "status=", 7)) {
      PARSE_ERR(("%s: cdromd directive malformed.", context));
      }
    bx_options.ata[0].Opresent->set(1);
    bx_options.atadevice[0][1].Otype->set (BX_ATA_DEVICE_CDROM);
    bx_options.atadevice[0][1].Opath->set (&params[1][4]);
    if (!strcmp(params[2], "status=inserted"))
      bx_options.atadevice[0][1].Ostatus->set (BX_INSERTED);
    else if (!strcmp(params[2], "status=ejected"))
      bx_options.atadevice[0][1].Ostatus->set (BX_EJECTED);
    else {
      PARSE_ERR(("%s: cdromd directive malformed.", context));
      }
    bx_options.atadevice[0][1].Opresent->set (1);
    }

  else if (!strcmp(params[0], "boot")) {
    if (!strcmp(params[1], "a")) {
      bx_options.Obootdrive->set (BX_BOOT_FLOPPYA);
    } else if (!strcmp(params[1], "floppy")) {
      bx_options.Obootdrive->set (BX_BOOT_FLOPPYA);
    } else if (!strcmp(params[1], "c")) {
      bx_options.Obootdrive->set (BX_BOOT_DISKC);
    } else if (!strcmp(params[1], "disk")) {
      bx_options.Obootdrive->set (BX_BOOT_DISKC);
    } else if (!strcmp(params[1], "cdrom")) {
      bx_options.Obootdrive->set (BX_BOOT_CDROM);
    } else {
      PARSE_ERR(("%s: boot directive with unknown boot device '%s'.  use 'floppy', 'disk' or 'cdrom'.", context, params[1]));
      }
    }

  else if (!strcmp(params[0], "com1")) {
    for (i=1; i<num_params; i++) {
      if (!strncmp(params[i], "enabled=", 8)) {
        bx_options.com[0].Oenabled->set (atol(&params[i][8]));
        }
      else if (!strncmp(params[i], "dev=", 4)) {
        bx_options.com[0].Odev->set (&params[i][4]);
        bx_options.com[0].Oenabled->set (1);
        }
      else {
        PARSE_ERR(("%s: unknown parameter for com1 ignored.", context));
        }
      }
    }
  else if (!strcmp(params[0], "com2")) {
    for (i=1; i<num_params; i++) {
      if (!strncmp(params[i], "enabled=", 8)) {
        bx_options.com[1].Oenabled->set (atol(&params[i][8]));
        }
      else if (!strncmp(params[i], "dev=", 4)) {
        bx_options.com[1].Odev->set (&params[i][4]);
        bx_options.com[1].Oenabled->set (1);
        }
      else {
        PARSE_ERR(("%s: unknown parameter for com2 ignored.", context));
        }
      }
    }
  else if (!strcmp(params[0], "com3")) {
    for (i=1; i<num_params; i++) {
      if (!strncmp(params[i], "enabled=", 8)) {
        bx_options.com[2].Oenabled->set (atol(&params[i][8]));
        }
      else if (!strncmp(params[i], "dev=", 4)) {
        bx_options.com[2].Odev->set (&params[i][4]);
        bx_options.com[2].Oenabled->set (1);
        }
      else {
        PARSE_ERR(("%s: unknown parameter for com3 ignored.", context));
        }
      }
    }
  else if (!strcmp(params[0], "com4")) {
    for (i=1; i<num_params; i++) {
      if (!strncmp(params[i], "enabled=", 8)) {
        bx_options.com[3].Oenabled->set (atol(&params[i][8]));
        }
      else if (!strncmp(params[i], "dev=", 4)) {
        bx_options.com[3].Odev->set (&params[i][4]);
        bx_options.com[3].Oenabled->set (1);
        }
      else {
        PARSE_ERR(("%s: unknown parameter for com4 ignored.", context));
        }
      }
    }
  else if (!strcmp(params[0], "usb1")) {
    for (i=1; i<num_params; i++) {
      if (!strncmp(params[i], "enabled=", 8)) {
        bx_options.usb[0].Oenabled->set (atol(&params[i][8]));
        }
      else if (!strncmp(params[i], "ioaddr=", 7)) {
        if ( (params[i][7] == '0') && (params[i][8] == 'x') )
          bx_options.usb[0].Oioaddr->set (strtoul (&params[i][7], NULL, 16));
        else
          bx_options.usb[0].Oioaddr->set (strtoul (&params[i][7], NULL, 10));
        bx_options.usb[0].Oenabled->set (1);
        }
      else if (!strncmp(params[i], "irq=", 4)) {
        bx_options.usb[0].Oirq->set (atol(&params[i][4]));
        }
      else {
        PARSE_ERR(("%s: unknown parameter for usb1 ignored.", context));
        }
      }
    }
  else if (!strcmp(params[0], "pnic")) {
    int tmp[6];
    char tmpchar[6];
    int valid = 0;
    int n;
    for (i=1; i<num_params; i++) {
      if (!strncmp(params[i], "enabled=", 8)) {
        bx_options.pnic.Oenabled->set (atol(&params[i][8]));
	if ( bx_options.pnic.Oenabled->get() ) {
		// Force ne2k to be enabled
		bx_options.ne2k.Opresent->set (1);
          }
        }
      else if (!strncmp(params[i], "ioaddr=", 7)) {
        if ( (params[i][7] == '0') && (params[i][8] == 'x') )
          bx_options.pnic.Oioaddr->set (strtoul (&params[i][7], NULL, 16));
        else
          bx_options.pnic.Oioaddr->set (strtoul (&params[i][7], NULL, 10));
        bx_options.pnic.Oenabled->set (1);
        }
      else if (!strncmp(params[i], "irq=", 4)) {
        bx_options.pnic.Oirq->set (atol(&params[i][4]));
        }
      else if (!strncmp(params[i], "mac=", 4)) {
        n = sscanf(&params[i][4], "%x:%x:%x:%x:%x:%x",
                   &tmp[0],&tmp[1],&tmp[2],&tmp[3],&tmp[4],&tmp[5]);
        if (n != 6) {
          PARSE_ERR(("%s: pnic mac address malformed.", context));
        }
        for (n=0;n<6;n++)
          tmpchar[n] = (unsigned char)tmp[n];
        bx_options.pnic.Omacaddr->set (tmpchar);
        valid |= 0x04;
        }
      else if (!strncmp(params[i], "ethmod=", 7)) {
        if (!bx_options.pnic.Oethmod->set_by_name (strdup(&params[i][7])))
          PARSE_ERR(("%s: ethernet module '%s' not available", context, strdup(&params[i][7])));
        }
      else if (!strncmp(params[i], "ethdev=", 7)) {
        bx_options.pnic.Oethdev->set (strdup(&params[i][7]));
        }
      else if (!strncmp(params[i], "script=", 7)) {
        bx_options.pnic.Oscript->set (strdup(&params[i][7]));
        }
      else {
        PARSE_ERR(("%s: unknown parameter for pnic ignored.", context));
        }
      }
    }
  else if (!strcmp(params[0], "floppy_bootsig_check")) {
    if (num_params != 2) {
      PARSE_ERR(("%s: floppy_bootsig_check directive malformed.", context));
      }
    if (strncmp(params[1], "disabled=", 9)) {
      PARSE_ERR(("%s: floppy_bootsig_check directive malformed.", context));
      }
    if (params[1][9] == '0')
      bx_options.OfloppySigCheck->set (0);
    else if (params[1][9] == '1')
      bx_options.OfloppySigCheck->set (1);
    else {
      PARSE_ERR(("%s: floppy_bootsig_check directive malformed.", context));
      }
    }
  else if (!strcmp(params[0], "log")) {
    if (num_params != 2) {
      PARSE_ERR(("%s: log directive has wrong # args.", context));
      }
    bx_options.log.Ofilename->set (params[1]);
    }
  else if (!strcmp(params[0], "logprefix")) {
    if (num_params != 2) {
      PARSE_ERR(("%s: logprefix directive has wrong # args.", context));
      }
    bx_options.log.Oprefix->set (params[1]);
    }
  else if (!strcmp(params[0], "debugger_log")) {
    if (num_params != 2) {
      PARSE_ERR(("%s: debugger_log directive has wrong # args.", context));
      }
    bx_options.log.Odebugger_filename->set (params[1]);
    }
  else if (!strcmp(params[0], "panic")) {
    if (num_params != 2) {
      PARSE_ERR(("%s: panic directive malformed.", context));
      }
    if (strncmp(params[1], "action=", 7)) {
      PARSE_ERR(("%s: panic directive malformed.", context));
      }
    char *action = 7 + params[1];
    if (!strcmp(action, "fatal"))
      SIM->set_default_log_action (LOGLEV_PANIC, ACT_FATAL);
    else if (!strcmp (action, "report"))
      SIM->set_default_log_action (LOGLEV_PANIC, ACT_REPORT);
    else if (!strcmp (action, "ignore"))
      SIM->set_default_log_action (LOGLEV_PANIC, ACT_IGNORE);
    else if (!strcmp (action, "ask"))
      SIM->set_default_log_action (LOGLEV_PANIC, ACT_ASK);
    else {
      PARSE_ERR(("%s: panic directive malformed.", context));
      }
    }
  else if (!strcmp(params[0], "pass")) {
    if (num_params != 2) {
      PARSE_ERR(("%s: pass directive malformed.", context));
      }
    if (strncmp(params[1], "action=", 7)) {
      PARSE_ERR(("%s: pass directive malformed.", context));
      }
    char *action = 7 + params[1];
    if (!strcmp(action, "fatal"))
      SIM->set_default_log_action (LOGLEV_PASS, ACT_FATAL);
    else if (!strcmp (action, "report"))
      SIM->set_default_log_action (LOGLEV_PASS, ACT_REPORT);
    else if (!strcmp (action, "ignore"))
      SIM->set_default_log_action (LOGLEV_PASS, ACT_IGNORE);
    else if (!strcmp (action, "ask"))
      SIM->set_default_log_action (LOGLEV_PASS, ACT_ASK);
    else {
      PARSE_ERR(("%s: pass directive malformed.", context));
      }
    }
  else if (!strcmp(params[0], "error")) {
    if (num_params != 2) {
      PARSE_ERR(("%s: error directive malformed.", context));
      }
    if (strncmp(params[1], "action=", 7)) {
      PARSE_ERR(("%s: error directive malformed.", context));
      }
    char *action = 7 + params[1];
    if (!strcmp(action, "fatal"))
      SIM->set_default_log_action (LOGLEV_ERROR, ACT_FATAL);
    else if (!strcmp (action, "report"))
      SIM->set_default_log_action (LOGLEV_ERROR, ACT_REPORT);
    else if (!strcmp (action, "ignore"))
      SIM->set_default_log_action (LOGLEV_ERROR, ACT_IGNORE);
    else if (!strcmp (action, "ask"))
      SIM->set_default_log_action (LOGLEV_ERROR, ACT_ASK);
    else {
      PARSE_ERR(("%s: error directive malformed.", context));
      }
    }
  else if (!strcmp(params[0], "info")) {
    if (num_params != 2) {
      PARSE_ERR(("%s: info directive malformed.", context));
      }
    if (strncmp(params[1], "action=", 7)) {
      PARSE_ERR(("%s: info directive malformed.", context));
      }
    char *action = 7 + params[1];
    if (!strcmp(action, "fatal"))
      SIM->set_default_log_action (LOGLEV_INFO, ACT_FATAL);
    else if (!strcmp (action, "report"))
      SIM->set_default_log_action (LOGLEV_INFO, ACT_REPORT);
    else if (!strcmp (action, "ignore"))
      SIM->set_default_log_action (LOGLEV_INFO, ACT_IGNORE);
    else if (!strcmp (action, "ask"))
      SIM->set_default_log_action (LOGLEV_INFO, ACT_ASK);
    else {
      PARSE_ERR(("%s: info directive malformed.", context));
      }
    }
  else if (!strcmp(params[0], "debug")) {
    if (num_params != 2) {
      PARSE_ERR(("%s: debug directive malformed.", context));
      }
    if (strncmp(params[1], "action=", 7)) {
      PARSE_ERR(("%s: debug directive malformed.", context));
      }
    char *action = 7 + params[1];
    if (!strcmp(action, "fatal"))
      SIM->set_default_log_action (LOGLEV_DEBUG, ACT_FATAL);
    else if (!strcmp (action, "report"))
      SIM->set_default_log_action (LOGLEV_DEBUG, ACT_REPORT);
    else if (!strcmp (action, "ignore"))
      SIM->set_default_log_action (LOGLEV_DEBUG, ACT_IGNORE);
    else if (!strcmp (action, "ask"))
      SIM->set_default_log_action (LOGLEV_DEBUG, ACT_ASK);
    else {
      PARSE_ERR(("%s: debug directive malformed.", context));
      }
    }
  else if (!strcmp(params[0], "romimage")) {
    if (num_params != 3) {
      PARSE_ERR(("%s: romimage directive: wrong # args.", context));
      }
    if (strncmp(params[1], "file=", 5)) {
      PARSE_ERR(("%s: romimage directive malformed.", context));
      }
    if (strncmp(params[2], "address=", 8)) {
      PARSE_ERR(("%s: romimage directive malformed.", context));
      }
    bx_options.rom.Opath->set (&params[1][5]);
    if ( (params[2][8] == '0') && (params[2][9] == 'x') )
      bx_options.rom.Oaddress->set (strtoul (&params[2][8], NULL, 16));
    else
      bx_options.rom.Oaddress->set (strtoul (&params[2][8], NULL, 10));
    }
  else if (!strcmp(params[0], "optromimage1")) {
    if (num_params != 3) {
      PARSE_ERR(("%s: optromimage1 directive: wrong # args.", context));
      }
    if (strncmp(params[1], "file=", 5)) {
      PARSE_ERR(("%s: optromimage1 directive malformed.", context));
      }
    if (strncmp(params[2], "address=", 8)) {
      PARSE_ERR(("%s: optromimage2 directive malformed.", context));
      }
    bx_options.optrom[0].Opath->set (&params[1][5]);
    if ( (params[2][8] == '0') && (params[2][9] == 'x') )
      bx_options.optrom[0].Oaddress->set (strtoul (&params[2][8], NULL, 16));
    else
      bx_options.optrom[0].Oaddress->set (strtoul (&params[2][8], NULL, 10));
    }
  else if (!strcmp(params[0], "optromimage2")) {
    if (num_params != 3) {
      PARSE_ERR(("%s: optromimage2 directive: wrong # args.", context));
      }
    if (strncmp(params[1], "file=", 5)) {
      PARSE_ERR(("%s: optromimage2 directive malformed.", context));
      }
    if (strncmp(params[2], "address=", 8)) {
      PARSE_ERR(("%s: optromimage2 directive malformed.", context));
      }
    bx_options.optrom[1].Opath->set (&params[1][5]);
    if ( (params[2][8] == '0') && (params[2][9] == 'x') )
      bx_options.optrom[1].Oaddress->set (strtoul (&params[2][8], NULL, 16));
    else
      bx_options.optrom[1].Oaddress->set (strtoul (&params[2][8], NULL, 10));
    }
  else if (!strcmp(params[0], "optromimage3")) {
    if (num_params != 3) {
      PARSE_ERR(("%s: optromimage3 directive: wrong # args.", context));
      }
    if (strncmp(params[1], "file=", 5)) {
      PARSE_ERR(("%s: optromimage3 directive malformed.", context));
      }
    if (strncmp(params[2], "address=", 8)) {
      PARSE_ERR(("%s: optromimage2 directive malformed.", context));
      }
    bx_options.optrom[2].Opath->set (&params[1][5]);
    if ( (params[2][8] == '0') && (params[2][9] == 'x') )
      bx_options.optrom[2].Oaddress->set (strtoul (&params[2][8], NULL, 16));
    else
      bx_options.optrom[2].Oaddress->set (strtoul (&params[2][8], NULL, 10));
    }
  else if (!strcmp(params[0], "optromimage4")) {
    if (num_params != 3) {
      PARSE_ERR(("%s: optromimage4 directive: wrong # args.", context));
      }
    if (strncmp(params[1], "file=", 5)) {
      PARSE_ERR(("%s: optromimage4 directive malformed.", context));
      }
    if (strncmp(params[2], "address=", 8)) {
      PARSE_ERR(("%s: optromimage2 directive malformed.", context));
      }
    bx_options.optrom[3].Opath->set (&params[1][5]);
    if ( (params[2][8] == '0') && (params[2][9] == 'x') )
      bx_options.optrom[3].Oaddress->set (strtoul (&params[2][8], NULL, 16));
    else
      bx_options.optrom[3].Oaddress->set (strtoul (&params[2][8], NULL, 10));
    }
  else if (!strcmp(params[0], "vgaromimage")) {
    if (num_params != 2) {
      PARSE_ERR(("%s: vgaromimage directive: wrong # args.", context));
      }
    bx_options.vgarom.Opath->set (params[1]);
    }
  else if (!strcmp(params[0], "vga_update_interval")) {
    if (num_params != 2) {
      PARSE_ERR(("%s: vga_update_interval directive: wrong # args.", context));
      }
    bx_options.Ovga_update_interval->set (atol(params[1]));
    if (bx_options.Ovga_update_interval->get () < 50000) {
      BX_INFO(("%s: vga_update_interval seems awfully small!", context));
      }
    }
  else if (!strcmp(params[0], "keyboard_serial_delay")) {
    if (num_params != 2) {
      PARSE_ERR(("%s: keyboard_serial_delay directive: wrong # args.", context));
      }
    bx_options.Okeyboard_serial_delay->set (atol(params[1]));
    if (bx_options.Okeyboard_serial_delay->get () < 5) {
      PARSE_ERR (("%s: keyboard_serial_delay not big enough!", context));
      }
    }
  else if (!strcmp(params[0], "keyboard_paste_delay")) {
    if (num_params != 2) {
      PARSE_ERR(("%s: keyboard_paste_delay directive: wrong # args.", context));
      }
    bx_options.Okeyboard_paste_delay->set (atol(params[1]));
    if (bx_options.Okeyboard_paste_delay->get () < 1000) {
      PARSE_ERR (("%s: keyboard_paste_delay not big enough!", context));
      }
    }
  else if (!strcmp(params[0], "megs")) {
    if (num_params != 2) {
      PARSE_ERR(("%s: megs directive: wrong # args.", context));
      }
    bx_options.memory.Osize->set (atol(params[1]));
    }
  else if (!strcmp(params[0], "floppy_command_delay")) {
    if (num_params != 2) {
      PARSE_ERR(("%s: floppy_command_delay directive: wrong # args.", context));
      }
    bx_options.Ofloppy_command_delay->set (atol(params[1]));
    if (bx_options.Ofloppy_command_delay->get () < 100) {
      PARSE_ERR(("%s: floppy_command_delay not big enough!", context));
      }
    }
  else if (!strcmp(params[0], "ips")) {
    if (num_params != 2) {
      PARSE_ERR(("%s: ips directive: wrong # args.", context));
      }
    bx_options.Oips->set (atol(params[1]));
    if (bx_options.Oips->get () < BX_MIN_IPS) {
      BX_ERROR(("%s: WARNING: ips is AWFULLY low!", context));
      }
    }
  else if (!strcmp(params[0], "pit")) { // Deprecated
    if (num_params != 2) {
      PARSE_ERR(("%s: pit directive: wrong # args.", context));
      }
    BX_INFO(("WARNING: pit directive is deprecated, use clock: instead"));
    if (!strncmp(params[1], "realtime=", 9)) {
      switch (params[1][9]) {
        case '0': 
          BX_INFO(("WARNING: not disabling realtime pit"));
          break;
        case '1': bx_options.clock.Osync->set (BX_CLOCK_SYNC_REALTIME); break;
        default: PARSE_ERR(("%s: pit expected realtime=[0|1] arg", context));
        }
      }
    else PARSE_ERR(("%s: pit expected realtime=[0|1] arg", context));
    }
  else if (!strcmp(params[0], "max_ips")) {
    if (num_params != 2) {
      PARSE_ERR(("%s: max_ips directive: wrong # args.", context));
      }
    BX_INFO(("WARNING: max_ips not implemented"));
    }
  else if (!strcmp(params[0], "text_snapshot_check")) {
    if (num_params != 2) {
      PARSE_ERR(("%s: text_snapshot_check directive: wrong # args.", context));
      }
    if (!strncmp(params[1], "enable", 6)) {
      bx_options.Otext_snapshot_check->set (1);
      }
    else if (!strncmp(params[1], "disable", 7)) {
      bx_options.Otext_snapshot_check->set (0);
      }
    else bx_options.Otext_snapshot_check->set (!!(atol(params[1])));
    }
  else if (!strcmp(params[0], "mouse")) {
    if (num_params != 2) {
      PARSE_ERR(("%s: mouse directive malformed.", context));
      }
    if (strncmp(params[1], "enabled=", 8)) {
      PARSE_ERR(("%s: mouse directive malformed.", context));
      }
    if (params[1][8] == '0' || params[1][8] == '1')
      bx_options.Omouse_enabled->set (params[1][8] - '0');
    else
      PARSE_ERR(("%s: mouse directive malformed.", context));
    }
  else if (!strcmp(params[0], "private_colormap")) {
    if (num_params != 2) {
      PARSE_ERR(("%s: private_colormap directive malformed.", context));
      }
    if (strncmp(params[1], "enabled=", 8)) {
      PARSE_ERR(("%s: private_colormap directive malformed.", context));
      }
    if (params[1][8] == '0' || params[1][8] == '1')
      bx_options.Oprivate_colormap->set (params[1][8] - '0');
    else {
      PARSE_ERR(("%s: private_colormap directive malformed.", context));
      }
    }
  else if (!strcmp(params[0], "fullscreen")) {
#if BX_WITH_AMIGAOS
    if (num_params != 2) {
      PARSE_ERR(("%s: fullscreen directive malformed.", context));
      }
    if (strncmp(params[1], "enabled=", 8)) {
      PARSE_ERR(("%s: fullscreen directive malformed.", context));
      }
    if (params[1][8] == '0' || params[1][8] == '1') {
      bx_options.Ofullscreen->set (params[1][8] - '0');
    } else {
      PARSE_ERR(("%s: fullscreen directive malformed.", context));
      }
#endif
    }
  else if (!strcmp(params[0], "screenmode")) {
#if BX_WITH_AMIGAOS
    if (num_params != 2) {
      PARSE_ERR(("%s: screenmode directive malformed.", context));
      }
    if (strncmp(params[1], "name=", 5)) {
      PARSE_ERR(("%s: screenmode directive malformed.", context));
      }
    bx_options.Oscreenmode->set (strdup(&params[1][5]));
#endif
    }

  else if (!strcmp(params[0], "sb16")) {
    for (i=1; i<num_params; i++) {
      if (!strncmp(params[i], "midi=", 5)) {
        bx_options.sb16.Omidifile->set (strdup(&params[i][5]));
        }
      else if (!strncmp(params[i], "midimode=", 9)) {
        bx_options.sb16.Omidimode->set (atol(&params[i][9]));
        }
      else if (!strncmp(params[i], "wave=", 5)) {
        bx_options.sb16.Owavefile->set (strdup(&params[i][5]));
        }
      else if (!strncmp(params[i], "wavemode=", 9)) {
        bx_options.sb16.Owavemode->set (atol(&params[i][9]));
        }
      else if (!strncmp(params[i], "log=", 4)) {
        bx_options.sb16.Ologfile->set (strdup(&params[i][4]));
        }
      else if (!strncmp(params[i], "loglevel=", 9)) {
        bx_options.sb16.Ologlevel->set (atol(&params[i][9]));
        }
      else if (!strncmp(params[i], "dmatimer=", 9)) {
        bx_options.sb16.Odmatimer->set (atol(&params[i][9]));
        }
      }
    if (bx_options.sb16.Odmatimer->get () > 0)
      bx_options.sb16.Opresent->set (1);
    }

  else if (!strcmp(params[0], "parport1")) {
    for (i=1; i<num_params; i++) {
      if (!strncmp(params[i], "enabled=", 8)) {
        bx_options.par[0].Oenabled->set (atol(&params[i][8]));
        }
      else if (!strncmp(params[i], "file=", 5)) {
        bx_options.par[0].Ooutfile->set (strdup(&params[i][5]));
        bx_options.par[0].Oenabled->set (1);
        }
      else {
        BX_ERROR(("%s: unknown parameter for parport1 ignored.", context));
        }
    }
  }

  else if (!strcmp(params[0], "parport2")) {
    for (i=1; i<num_params; i++) {
      if (!strncmp(params[i], "enabled=", 8)) {
        bx_options.par[1].Oenabled->set (atol(&params[i][8]));
        }
      else if (!strncmp(params[i], "file=", 5)) {
        bx_options.par[1].Ooutfile->set (strdup(&params[i][5]));
        bx_options.par[1].Oenabled->set (1);
        }
      else {
        BX_ERROR(("%s: unknown parameter for parport2 ignored.", context));
        }
    }
  }

  else if (!strcmp(params[0], "i440fxsupport")) {
    if (num_params < 2) {
      PARSE_ERR(("%s: i440FXSupport directive malformed.", context));
      }
    if (strncmp(params[1], "enabled=", 8)) {
      PARSE_ERR(("%s: i440FXSupport directive malformed.", context));
      }
    if (params[1][8] == '0')
      bx_options.Oi440FXSupport->set (0);
    else if (params[1][8] == '1')
      bx_options.Oi440FXSupport->set (1);
    else {
      PARSE_ERR(("%s: i440FXSupport directive malformed.", context));
      }
    if (num_params > 2) {
      for (i=2; i<num_params; i++) {
        if ((!strncmp(params[i], "slot", 4)) && (params[i][5] == '=')) {
          slot = atol(&params[i][4]) - 1;
          if ((slot >= 0) && (slot < 5)) {
            bx_options.pcislot[slot].Odevname->set (strdup(&params[i][6]));
            bx_options.pcislot[slot].Oused->set (strlen(params[i]) > 6);
            }
          else {
            BX_ERROR(("%s: unknown pci slot number ignored.", context));
            }
          }
        else {
          BX_ERROR(("%s: unknown parameter for pci slot ignored.", context));
          }
        }
      }
    }
  else if (!strcmp(params[0], "pcidev")) {
    if (num_params != 3) {
      PARSE_ERR(("%s: pcidev directive malformed.", context));
    }
    for (i=1; i<num_params; i++) {
      if (!strncmp(params[i], "vendor=", 7)) {
        if ( (params[i][7] == '0') && (toupper(params[i][8]) == 'X') )
          bx_options.pcidev.Ovendor->set (strtoul (&params[i][7], NULL, 16));
        else
          bx_options.pcidev.Ovendor->set (strtoul (&params[i][7], NULL, 10));
      }
      else if (!strncmp(params[i], "device=", 7)) {
        if ( (params[i][7] == '0') && (toupper(params[i][8]) == 'X') )
          bx_options.pcidev.Odevice->set (strtoul (&params[i][7], NULL, 16));
        else
          bx_options.pcidev.Odevice->set (strtoul (&params[i][7], NULL, 10));
      }
      else {
        BX_ERROR(("%s: unknown parameter for pcidev ignored.", context));
      }
    }
  }
  else if (!strcmp(params[0], "newharddrivesupport")) {
    if (num_params != 2) {
      PARSE_ERR(("%s: newharddrivesupport directive malformed.", context));
      }
    if (strncmp(params[1], "enabled=", 8)) {
      PARSE_ERR(("%s: newharddrivesupport directive malformed.", context));
      }
    BX_INFO(("WARNING: newharddrivesupport directive is deprecated and should be removed."));
    if (params[1][8] == '0')
      bx_options.OnewHardDriveSupport->set (0);
    else if (params[1][8] == '1')
      bx_options.OnewHardDriveSupport->set (1);
    else {
      PARSE_ERR(("%s: newharddrivesupport directive malformed.", context));
      }
    }
  else if (!strcmp(params[0], "cmosimage")) {
    if (num_params != 2) {
      PARSE_ERR(("%s: cmosimage directive: wrong # args.", context));
      }
    bx_options.cmos.Opath->set (strdup(params[1]));
    bx_options.cmos.OcmosImage->set (1);                // CMOS Image is true
    }
  else if (!strcmp(params[0], "time0")) { // Deprectated
    BX_INFO(("WARNING: time0 directive is deprecated, use clock: instead"));
    if (num_params != 2) {
      PARSE_ERR(("%s: time0 directive: wrong # args.", context));
      }
    bx_options.clock.Otime0->set (atoi(params[1]));
    }
  else if (!strcmp(params[0], "clock")) {
    for (i=1; i<num_params; i++) {
      if (!strncmp(params[i], "sync=", 5)) {
        bx_options.clock.Osync->set_by_name (&params[i][5]);
        }
      else if (!strcmp(params[i], "time0=local")) {
        bx_options.clock.Otime0->set (BX_CLOCK_TIME0_LOCAL);
        }
      else if (!strcmp(params[i], "time0=utc")) {
        bx_options.clock.Otime0->set (BX_CLOCK_TIME0_UTC);
        }
      else if (!strncmp(params[i], "time0=", 6)) {
        bx_options.clock.Otime0->set (atoi(&params[i][6]));
        }
      else {
        BX_ERROR(("%s: unknown parameter for clock ignored.", context));
        }
      }
    }
#if BX_MAGIC_BREAKPOINT
  else if (!strcmp(params[0], "magic_break")) {
    if (num_params != 2) {
      PARSE_ERR(("%s: magic_break directive: wrong # args.", context));
      }
    if (strncmp(params[1], "enabled=", 8)) {
      PARSE_ERR(("%s: magic_break directive malformed.", context));
      }
    if (params[1][8] == '0') {
      BX_INFO(("Ignoring magic break points"));
      bx_dbg.magic_break_enabled = 0;
      }
    else if (params[1][8] == '1') {
      BX_INFO(("Stopping on magic break points"));
      bx_dbg.magic_break_enabled = 1;
      }
    else {
      PARSE_ERR(("%s: magic_break directive malformed.", context));
      }
    }
#endif
  else if (!strcmp(params[0], "ne2k")) {
    int tmp[6];
    char tmpchar[6];
    int valid = 0;
    int n;
    if (!bx_options.ne2k.Opresent->get ()) {
      bx_options.ne2k.Oethmod->set_by_name ("null");
      }
    for (i=1; i<num_params; i++) {
      if (!strncmp(params[i], "ioaddr=", 7)) {
        bx_options.ne2k.Oioaddr->set (strtoul(&params[i][7], NULL, 16));
        valid |= 0x01;
        }
      else if (!strncmp(params[i], "irq=", 4)) {
        bx_options.ne2k.Oirq->set (atol(&params[i][4]));
        valid |= 0x02;
        }
      else if (!strncmp(params[i], "mac=", 4)) {
        n = sscanf(&params[i][4], "%x:%x:%x:%x:%x:%x",
                   &tmp[0],&tmp[1],&tmp[2],&tmp[3],&tmp[4],&tmp[5]);
        if (n != 6) {
          PARSE_ERR(("%s: ne2k mac address malformed.", context));
        }
        for (n=0;n<6;n++)
          tmpchar[n] = (unsigned char)tmp[n];
        bx_options.ne2k.Omacaddr->set (tmpchar);
        valid |= 0x04;
        }
      else if (!strncmp(params[i], "ethmod=", 7)) {
        if (!bx_options.ne2k.Oethmod->set_by_name (strdup(&params[i][7])))
          PARSE_ERR(("%s: ethernet module '%s' not available", context, strdup(&params[i][7])));
        }
      else if (!strncmp(params[i], "ethdev=", 7)) {
        bx_options.ne2k.Oethdev->set (strdup(&params[i][7]));
        }
      else if (!strncmp(params[i], "script=", 7)) {
        bx_options.ne2k.Oscript->set (strdup(&params[i][7]));
        }
      else {
        PARSE_ERR(("%s: ne2k directive malformed.", context));
        }
    }
    if (!bx_options.ne2k.Opresent->get ()) {
      if (valid == 0x07) {
        bx_options.ne2k.Opresent->set (1);
        }
      else {
        PARSE_ERR(("%s: ne2k directive incomplete (ioaddr, irq and mac are required)", context));
        }
      }
    }

  else if (!strcmp(params[0], "load32bitOSImage")) {
    if ( (num_params!=4) && (num_params!=5) ) {
      PARSE_ERR(("%s: load32bitOSImage directive: wrong # args.", context));
      }
    if (strncmp(params[1], "os=", 3)) {
      PARSE_ERR(("%s: load32bitOSImage: directive malformed.", context));
      }
    if (!strcmp(&params[1][3], "nullkernel")) {
      bx_options.load32bitOSImage.OwhichOS->set (Load32bitOSNullKernel);
      }
    else if (!strcmp(&params[1][3], "linux")) {
      bx_options.load32bitOSImage.OwhichOS->set (Load32bitOSLinux);
      }
    else {
      PARSE_ERR(("%s: load32bitOSImage: unsupported OS.", context));
      }
    if (strncmp(params[2], "path=", 5)) {
      PARSE_ERR(("%s: load32bitOSImage: directive malformed.", context));
      }
    if (strncmp(params[3], "iolog=", 6)) {
      PARSE_ERR(("%s: load32bitOSImage: directive malformed.", context));
      }
    bx_options.load32bitOSImage.Opath->set (strdup(&params[2][5]));
    bx_options.load32bitOSImage.Oiolog->set (strdup(&params[3][6]));
    if (num_params == 5) {
      if (strncmp(params[4], "initrd=", 7)) {
        PARSE_ERR(("%s: load32bitOSImage: directive malformed.", context));
        }
      bx_options.load32bitOSImage.Oinitrd->set (strdup(&params[4][7]));
      }
    }
  else if (!strcmp(params[0], "keyboard_type")) {
    if (num_params != 2) {
      PARSE_ERR(("%s: keyboard_type directive: wrong # args.", context));
      }
    if(strcmp(params[1],"xt")==0){
      bx_options.Okeyboard_type->set (BX_KBD_XT_TYPE);
      }
    else if(strcmp(params[1],"at")==0){
      bx_options.Okeyboard_type->set (BX_KBD_AT_TYPE);
      }
    else if(strcmp(params[1],"mf")==0){
      bx_options.Okeyboard_type->set (BX_KBD_MF_TYPE);
      }
    else{
      PARSE_ERR(("%s: keyboard_type directive: wrong arg %s.", context,params[1]));
      }
    }

  else if (!strcmp(params[0], "keyboard_mapping")
         ||!strcmp(params[0], "keyboardmapping")) {
    for (i=1; i<num_params; i++) {
      if (!strncmp(params[i], "enabled=", 8)) {
        bx_options.keyboard.OuseMapping->set (atol(&params[i][8]));
        }
      else if (!strncmp(params[i], "map=", 4)) {
        bx_options.keyboard.Okeymap->set (strdup(&params[i][4]));
        }
      }
    }
  else if (!strcmp(params[0], "user_shortcut")) {
    if (num_params != 2) {
      PARSE_ERR(("%s: user_shortcut directive: wrong # args.", context));
      }
    if(!strncmp(params[1], "keys=", 4)) {
      bx_options.Ouser_shortcut->set (strdup(&params[1][5]));
      }
    }
  else if (!strcmp(params[0], "config_interface")) {
    if (num_params != 2) {
      PARSE_ERR(("%s: config_interface directive: wrong # args.", context));
      }
    if (!bx_options.Osel_config->set_by_name (params[1]))
      PARSE_ERR(("%s: config_interface '%s' not available", context, params[1]));
    }
  else if (!strcmp(params[0], "display_library")) {
    if ((num_params < 2) || (num_params > 3)) {
      PARSE_ERR(("%s: display_library directive: wrong # args.", context));
      }
    if (!bx_options.Osel_displaylib->set_by_name (params[1]))
      PARSE_ERR(("%s: display library '%s' not available", context, params[1]));
    if (num_params == 3) {
      if (!strncmp(params[2], "options=", 8)) {
        bx_options.Odisplaylib_options->set (strdup(&params[2][8]));
        }
      }
    }
  else {
    PARSE_ERR(( "%s: directive '%s' not understood", context, params[0]));
    }
  return 0;
}

static char *fdtypes[] = {
  "none", "1_2", "1_44", "2_88", "720k", "360k", "160k", "180k", "320k"
};


int 
bx_write_floppy_options (FILE *fp, int drive, bx_floppy_options *opt)
{
  BX_ASSERT (drive==0 || drive==1);
  if (opt->Otype->get () == BX_FLOPPY_NONE) {
    fprintf (fp, "# no floppy%c\n", (char)'a'+drive);
    return 0;
  }
  BX_ASSERT (opt->Otype->get () > BX_FLOPPY_NONE && opt->Otype->get () <= BX_FLOPPY_LAST);
  fprintf (fp, "floppy%c: %s=\"%s\", status=%s\n", 
    (char)'a'+drive, 
    fdtypes[opt->Otype->get () - BX_FLOPPY_NONE],
    opt->Opath->getptr (),
    opt->Ostatus->get ()==BX_EJECTED ? "ejected" : "inserted");
  return 0;
}

int 
bx_write_ata_options (FILE *fp, Bit8u channel, bx_ata_options *opt)
{
  fprintf (fp, "ata%d: enabled=%d", channel, opt->Opresent->get());

  if (opt->Opresent->get()) {
    fprintf (fp, ", ioaddr1=0x%x, ioaddr2=0x%x, irq=%d", opt->Oioaddr1->get(), 
      opt->Oioaddr2->get(), opt->Oirq->get());
    }

  fprintf (fp, "\n");
  return 0;
}

int 
bx_write_atadevice_options (FILE *fp, Bit8u channel, Bit8u drive, bx_atadevice_options *opt)
{
  if (opt->Opresent->get()) {
    fprintf (fp, "ata%d-%s: ", channel, drive==0?"master":"slave");

    if (opt->Otype->get() == BX_ATA_DEVICE_DISK) {
      fprintf (fp, "type=disk");

      switch(opt->Omode->get()) {
        case BX_ATA_MODE_FLAT:
          fprintf (fp, ", mode=flat");
          break;
        case BX_ATA_MODE_CONCAT:
          fprintf (fp, ", mode=concat");
          break;
        case BX_ATA_MODE_EXTDISKSIM:
          fprintf (fp, ", mode=external");
          break;
        case BX_ATA_MODE_DLL_HD:
          fprintf (fp, ", mode=dll");
          break;
        case BX_ATA_MODE_SPARSE:
          fprintf (fp, ", mode=sparse");
          break;
        case BX_ATA_MODE_VMWARE3:
          fprintf (fp, ", mode=vmware3");
          break;
//        case BX_ATA_MODE_SPLIT:
//          fprintf (fp, ", mode=split");
//          break;
        case BX_ATA_MODE_UNDOABLE:
          fprintf (fp, ", mode=undoable");
          break;
        case BX_ATA_MODE_GROWING:
          fprintf (fp, ", mode=growing");
          break;
        case BX_ATA_MODE_VOLATILE:
          fprintf (fp, ", mode=volatile");
          break;
//        case BX_ATA_MODE_Z_UNDOABLE:
//          fprintf (fp, ", mode=z-undoable");
//          break;
//        case BX_ATA_MODE_Z_VOLATILE:
//          fprintf (fp, ", mode=z-volatile");
//          break;
        }

      switch(opt->Otranslation->get()) {
        case BX_ATA_TRANSLATION_NONE:
          fprintf (fp, ", translation=none");
          break;
        case BX_ATA_TRANSLATION_LBA:
          fprintf (fp, ", translation=lba");
          break;
        case BX_ATA_TRANSLATION_LARGE:
          fprintf (fp, ", translation=large");
          break;
        case BX_ATA_TRANSLATION_RECHS:
          fprintf (fp, ", translation=rechs");
          break;
        case BX_ATA_TRANSLATION_AUTO:
          fprintf (fp, ", translation=auto");
          break;
        }

      fprintf (fp, ", path=\"%s\", cylinders=%d, heads=%d, spt=%d",
          opt->Opath->getptr(), 
          opt->Ocylinders->get(), opt->Oheads->get(), opt->Ospt->get());

      if (opt->Ojournal->getptr() != NULL)
        if ( strcmp(opt->Ojournal->getptr(), "") != 0)
          fprintf (fp, ", journal=\"%s\"", opt->Ojournal->getptr());

      }
    else if (opt->Otype->get() == BX_ATA_DEVICE_CDROM) {
      fprintf (fp, "type=cdrom, path=\"%s\", status=%s", 
        opt->Opath->getptr(), 
        opt->Ostatus->get ()==BX_EJECTED ? "ejected" : "inserted");
      }

    switch(opt->Obiosdetect->get()) {
      case BX_ATA_BIOSDETECT_NONE:
        fprintf (fp, ", biosdetect=none");
        break;
      case BX_ATA_BIOSDETECT_CMOS:
        fprintf (fp, ", biosdetect=cmos");
        break;
      case BX_ATA_BIOSDETECT_AUTO:
        fprintf (fp, ", biosdetect=auto");
        break;
      }
    if (strlen(opt->Omodel->getptr())>0) {
        fprintf (fp, ", model=\"%s\"", opt->Omodel->getptr());
      }

    fprintf (fp, "\n");
  }
  return 0;
}

int
bx_write_parport_options (FILE *fp, bx_parport_options *opt, int n)
{
  fprintf (fp, "parport%d: enabled=%d", n, opt->Oenabled->get ());
  if (opt->Oenabled->get ()) {
    fprintf (fp, ", file=\"%s\"", opt->Ooutfile->getptr ());
  }
  fprintf (fp, "\n");
  return 0;
}

int
bx_write_serial_options (FILE *fp, bx_serial_options *opt, int n)
{
  fprintf (fp, "com%d: enabled=%d", n, opt->Oenabled->get ());
  if (opt->Oenabled->get ()) {
    fprintf (fp, ", dev=\"%s\"", opt->Odev->getptr ());
  }
  fprintf (fp, "\n");
  return 0;
}

int
bx_write_usb_options (FILE *fp, bx_usb_options *opt, int n)
{
  fprintf (fp, "usb%d: enabled=%d", n, opt->Oenabled->get ());
  if (opt->Oenabled->get ()) {
    fprintf (fp, ", ioaddr=0x%04x, irq=%d", opt->Oioaddr->get (),
      opt->Oirq->get ());
  }
  fprintf (fp, "\n");
  return 0;
}

int
bx_write_pnic_options (FILE *fp, bx_pnic_options *opt)
{
  fprintf (fp, "pnic: enabled=%d", opt->Oenabled->get ());
  if (opt->Oenabled->get ()) {
    char *ptr = opt->Omacaddr->getptr ();
    fprintf (fp, ", ioaddr=0x%04x, irq=%d, mac=%02x:%02x:%02x:%02x:%02x:%02x, ethmod=%s, ethdev=%s, script=%s",
	     opt->Oioaddr->get (),
	     opt->Oirq->get (),
	     (unsigned int)(0xff & ptr[0]),
	     (unsigned int)(0xff & ptr[1]),
	     (unsigned int)(0xff & ptr[2]),
	     (unsigned int)(0xff & ptr[3]),
	     (unsigned int)(0xff & ptr[4]),
	     (unsigned int)(0xff & ptr[5]),
	     opt->Oethmod->get_choice(opt->Oethmod->get()),
	     opt->Oethdev->getptr (),
	     opt->Oscript->getptr () );
  }
  fprintf (fp, "\n");
  return 0;
}

int
bx_write_sb16_options (FILE *fp, bx_sb16_options *opt)
{
  if (!opt->Opresent->get ()) {
    fprintf (fp, "# no sb16\n");
    return 0;
  }
  fprintf (fp, "sb16: midimode=%d, midi=%s, wavemode=%d, wave=%s, loglevel=%d, log=%s, dmatimer=%d\n", opt->Omidimode->get (), opt->Omidifile->getptr (), opt->Owavemode->get (), opt->Owavefile->getptr (), opt->Ologlevel->get (), opt->Ologfile->getptr (), opt->Odmatimer->get ());
  return 0;
}

int
bx_write_ne2k_options (FILE *fp, bx_ne2k_options *opt)
{
  if (!opt->Opresent->get ()) {
    fprintf (fp, "# no ne2k\n");
    return 0;
  }
  char *ptr = opt->Omacaddr->getptr ();
  fprintf (fp, "ne2k: ioaddr=0x%x, irq=%d, mac=%02x:%02x:%02x:%02x:%02x:%02x, ethmod=%s, ethdev=%s, script=%s\n",
      opt->Oioaddr->get (), 
      opt->Oirq->get (),
      (unsigned int)(0xff & ptr[0]),
      (unsigned int)(0xff & ptr[1]),
      (unsigned int)(0xff & ptr[2]),
      (unsigned int)(0xff & ptr[3]),
      (unsigned int)(0xff & ptr[4]),
      (unsigned int)(0xff & ptr[5]),
      opt->Oethmod->get_choice(opt->Oethmod->get()),
      opt->Oethdev->getptr (),
      opt->Oscript->getptr ());
  return 0;
}

int
bx_write_loader_options (FILE *fp, bx_load32bitOSImage_t *opt)
{
  if (opt->OwhichOS->get () == 0) {
    fprintf (fp, "# no loader\n");
    return 0;
  }
  BX_ASSERT(opt->OwhichOS->get () == Load32bitOSLinux || opt->OwhichOS->get () == Load32bitOSNullKernel);
  fprintf (fp, "load32bitOSImage: os=%s, path=%s, iolog=%s, initrd=%s\n",
      (opt->OwhichOS->get () == Load32bitOSLinux) ? "linux" : "nullkernel",
      opt->Opath->getptr (),
      opt->Oiolog->getptr (),
      opt->Oinitrd->getptr ());
  return 0;
}

int
bx_write_clock_options (FILE *fp, bx_clock_options *opt)
{
  fprintf (fp, "clock: ");

  switch (opt->Osync->get()) {
    case BX_CLOCK_SYNC_NONE:
      fprintf (fp, "sync=none");
      break;
    case BX_CLOCK_SYNC_REALTIME:
      fprintf (fp, "sync=realtime");
      break;
    case BX_CLOCK_SYNC_SLOWDOWN:
      fprintf (fp, "sync=slowdown");
      break;
    case BX_CLOCK_SYNC_BOTH:
      fprintf (fp, "sync=both");
      break;
    default:
      BX_PANIC(("Unknown value for sync method"));
  }

  switch (opt->Otime0->get()) {
    case 0: break;
    case BX_CLOCK_TIME0_LOCAL: 
      fprintf (fp, ", time0=local");
      break;
    case BX_CLOCK_TIME0_UTC: 
      fprintf (fp, ", time0=utc");
      break;
    default: 
      fprintf (fp, ", time0=%u", opt->Otime0->get());
  }

  fprintf (fp, "\n");
  return 0;
}

int
bx_write_log_options (FILE *fp, bx_log_options *opt)
{
  fprintf (fp, "log: %s\n", opt->Ofilename->getptr ());
  fprintf (fp, "logprefix: %s\n", opt->Oprefix->getptr ());
  fprintf (fp, "debugger_log: %s\n", opt->Odebugger_filename->getptr ());
  fprintf (fp, "panic: action=%s\n",
      io->getaction(logfunctions::get_default_action (LOGLEV_PANIC)));
  fprintf (fp, "error: action=%s\n",
      io->getaction(logfunctions::get_default_action (LOGLEV_ERROR)));
  fprintf (fp, "info: action=%s\n",
      io->getaction(logfunctions::get_default_action (LOGLEV_INFO)));
  fprintf (fp, "debug: action=%s\n",
      io->getaction(logfunctions::get_default_action (LOGLEV_DEBUG)));
  fprintf (fp, "pass: action=%s\n",
      io->getaction(logfunctions::get_default_action (LOGLEV_PASS)));
  return 0;
}

int
bx_write_keyboard_options (FILE *fp, bx_keyboard_options *opt)
{
  fprintf (fp, "keyboard_mapping: enabled=%d, map=%s\n", opt->OuseMapping->get(), opt->Okeymap->getptr());
  return 0;
}

// return values:
//   0: written ok
//  -1: failed
//  -2: already exists, and overwrite was off
int
bx_write_configuration (char *rc, int overwrite)
{
  int i;

  BX_INFO (("write configuration to %s\n", rc));
  // check if it exists.  If so, only proceed if overwrite is set.
  FILE *fp = fopen (rc, "r");
  if (fp != NULL) {
    fclose (fp);
    if (!overwrite) return -2;
  }
  fp = fopen (rc, "w");
  if (fp == NULL) return -1;
  // finally it's open and we can start writing.
  fprintf (fp, "# configuration file generated by Bochs\n");
  fprintf (fp, "config_interface: %s\n", bx_options.Osel_config->get_choice(bx_options.Osel_config->get()));
  fprintf (fp, "display_library: %s", bx_options.Osel_displaylib->get_choice(bx_options.Osel_displaylib->get()));
  if (strlen (bx_options.Odisplaylib_options->getptr ()) > 0)
    fprintf (fp, ", options=\"%s\"\n", bx_options.Odisplaylib_options->getptr ());
  else
    fprintf (fp, "\n");
  fprintf (fp, "megs: %d\n", bx_options.memory.Osize->get ());
  if (strlen (bx_options.rom.Opath->getptr ()) > 0)
    fprintf (fp, "romimage: file=%s, address=0x%05x\n", bx_options.rom.Opath->getptr(), (unsigned int)bx_options.rom.Oaddress->get ());
  else
    fprintf (fp, "# no romimage\n");
  if (strlen (bx_options.vgarom.Opath->getptr ()) > 0)
    fprintf (fp, "vgaromimage: %s\n", bx_options.vgarom.Opath->getptr ());
  else
    fprintf (fp, "# no vgaromimage\n");
  int bootdrive = bx_options.Obootdrive->get ();
  fprintf (fp, "boot: %s\n", (bootdrive==BX_BOOT_FLOPPYA) ? "floppy" : (bootdrive==BX_BOOT_DISKC) ? "disk" : "cdrom");
  // it would be nice to put this type of function as methods on
  // the structs like bx_floppy_options::print or something.
  bx_write_floppy_options (fp, 0, &bx_options.floppya);
  bx_write_floppy_options (fp, 1, &bx_options.floppyb);
  for (Bit8u channel=0; channel<BX_MAX_ATA_CHANNEL; channel++) {
    bx_write_ata_options (fp, channel, &bx_options.ata[channel]);
    bx_write_atadevice_options (fp, channel, 0, &bx_options.atadevice[channel][0]);
    bx_write_atadevice_options (fp, channel, 1, &bx_options.atadevice[channel][1]);
    }
  if (strlen (bx_options.optrom[0].Opath->getptr ()) > 0)
    fprintf (fp, "optromimage1: file=%s, address=0x%05x\n", bx_options.optrom[0].Opath->getptr(), (unsigned int)bx_options.optrom[0].Oaddress->get ());
  if (strlen (bx_options.optrom[1].Opath->getptr ()) > 0)
    fprintf (fp, "optromimage2: file=%s, address=0x%05x\n", bx_options.optrom[1].Opath->getptr(), (unsigned int)bx_options.optrom[1].Oaddress->get ());
  if (strlen (bx_options.optrom[2].Opath->getptr ()) > 0)
    fprintf (fp, "optromimage3: file=%s, address=0x%05x\n", bx_options.optrom[2].Opath->getptr(), (unsigned int)bx_options.optrom[2].Oaddress->get ());
  if (strlen (bx_options.optrom[3].Opath->getptr ()) > 0)
    fprintf (fp, "optromimage4: file=%s, address=0x%05x\n", bx_options.optrom[3].Opath->getptr(), (unsigned int)bx_options.optrom[3].Oaddress->get ());
  // parallel ports
  for (i=0; i<BX_N_PARALLEL_PORTS; i++) {
    bx_write_parport_options (fp, &bx_options.par[i], i+1);
  }
  // serial ports
  for (i=0; i<BX_N_SERIAL_PORTS; i++) {
    bx_write_serial_options (fp, &bx_options.com[i], i+1);
  }
  // pci
  fprintf (fp, "i440fxsupport: enabled=%d", bx_options.Oi440FXSupport->get ());
  if (bx_options.Oi440FXSupport->get ()) {
    for (i=0; i<BX_N_PCI_SLOTS; i++) {
      if (bx_options.pcislot[i].Oused->get ()) {
        fprintf (fp, ", slot%d=%s", i+1, bx_options.pcislot[i].Odevname->getptr ());
      }
    }
  }
  fprintf (fp, "\n");
  bx_write_usb_options (fp, &bx_options.usb[0], 1);
  bx_write_sb16_options (fp, &bx_options.sb16);
  fprintf (fp, "floppy_bootsig_check: disabled=%d\n", bx_options.OfloppySigCheck->get ());
  fprintf (fp, "vga_update_interval: %u\n", bx_options.Ovga_update_interval->get ());
  fprintf (fp, "keyboard_serial_delay: %u\n", bx_options.Okeyboard_serial_delay->get ());
  fprintf (fp, "keyboard_paste_delay: %u\n", bx_options.Okeyboard_paste_delay->get ());
  fprintf (fp, "floppy_command_delay: %u\n", bx_options.Ofloppy_command_delay->get ());
  fprintf (fp, "ips: %u\n", bx_options.Oips->get ());
  fprintf (fp, "text_snapshot_check: %d\n", bx_options.Otext_snapshot_check->get ());
  fprintf (fp, "mouse: enabled=%d\n", bx_options.Omouse_enabled->get ());
  fprintf (fp, "private_colormap: enabled=%d\n", bx_options.Oprivate_colormap->get ());
#if BX_WITH_AMIGAOS
  fprintf (fp, "fullscreen: enabled=%d\n", bx_options.Ofullscreen->get ());
  fprintf (fp, "screenmode: name=\"%s\"\n", bx_options.Oscreenmode->getptr ());
#endif
  bx_write_clock_options (fp, &bx_options.clock);
  bx_write_ne2k_options (fp, &bx_options.ne2k);
  bx_write_pnic_options (fp, &bx_options.pnic);
  fprintf (fp, "newharddrivesupport: enabled=%d\n", bx_options.OnewHardDriveSupport->get ());
  bx_write_loader_options (fp, &bx_options.load32bitOSImage);
  bx_write_log_options (fp, &bx_options.log);
  bx_write_keyboard_options (fp, &bx_options.keyboard);
  fprintf (fp, "keyboard_type: %s\n", bx_options.Okeyboard_type->get ()==BX_KBD_XT_TYPE?"xt":
                                       bx_options.Okeyboard_type->get ()==BX_KBD_AT_TYPE?"at":"mf");
  fprintf (fp, "user_shortcut: keys=%s\n", bx_options.Ouser_shortcut->getptr ());
  if (strlen (bx_options.cmos.Opath->getptr ()) > 0)
    fprintf (fp, "cmosimage: %s\n", bx_options.cmos.Opath->getptr());
  else
    fprintf (fp, "# no cmosimage\n");
  fclose (fp);
  return 0;
}
#endif // #if BX_PROVIDE_MAIN
