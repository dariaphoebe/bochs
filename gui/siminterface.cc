/////////////////////////////////////////////////////////////////////////
// $Id: siminterface.cc,v 1.94.4.15 2003/04/06 15:57:05 bdenney Exp $
/////////////////////////////////////////////////////////////////////////
//
// See siminterface.h for description of the siminterface concept.
// Basically, the siminterface is visible from both the simulator and
// the configuration user interface, and allows them to talk to each other.

#include "bochs.h"

bx_simulator_interface_c *SIM = NULL;
logfunctions *siminterface_log = NULL;
#define LOG_THIS siminterface_log->

// bx_simulator_interface just defines the interface that the Bochs simulator
// and the gui will use to talk to each other.  None of the methods of
// bx_simulator_interface are implemented; they are all virtual.  The
// bx_real_sim_c class is a child of bx_simulator_interface_c, and it
// implements all the methods.  The idea is that a gui needs to know only
// definition of bx_simulator_interface to talk to Bochs.  The gui should
// not need to include bochs.h.  
//
// I made this separation to ensure that all guis use the siminterface to do
// access bochs internals, instead of accessing things like
// bx_keyboard.s.internal_buffer[4] (or whatever) directly. -Bryce
// 

class bx_real_sim_c : public bx_simulator_interface_c {
  bxevent_handler bxevent_callback;
  void *bxevent_callback_data;
  const char *registered_ci_name;
  config_interface_callback_t ci_callback;
  void *ci_callback_data;
  int init_done;
  int enabled;
  // save context to jump to if we must quit unexpectedly
  jmp_buf *quit_context;
  int exit_code;
public:
  bx_real_sim_c ();
  virtual ~bx_real_sim_c ();
  virtual void set_quit_context (jmp_buf *context) { quit_context = context; }
  virtual int get_init_done () { return init_done; }
  virtual int set_init_done (int n) { init_done = n; return 0;}
  virtual void reset_all_param ();

  virtual bx_param_c *get_param (const char *ppath, bx_param_c *base=NULL);
  virtual bx_param_num_c *get_param_num (const char *ppath);
  virtual bx_param_string_c *get_param_string (const char *ppath);
  virtual bx_param_bool_c *get_param_bool (const char *ppath);
  virtual bx_param_enum_c *get_param_enum (const char *ppath);
  virtual int get_n_log_modules ();
  virtual char *get_prefix (int mod);
  virtual int get_log_action (int mod, int level);
  virtual void set_log_action (int mod, int level, int action);
  virtual char *get_action_name (int action);
  virtual int get_default_log_action (int level) {
	return logfunctions::get_default_action (level);
  }
  virtual void set_default_log_action (int level, int action) {
	logfunctions::set_default_action (level, action);
  }
  virtual const char *get_log_level_name (int level);
  virtual int get_max_log_level ();
  virtual void quit_sim (int code);
  virtual int get_exit_code () { return exit_code; }
  virtual int get_default_rc (char *path, int len);
  virtual int read_rc (char *path);
  virtual int write_rc (char *path, int overwrite);
  virtual int get_log_file (char *path, int len);
  virtual int set_log_file (char *path);
  virtual int get_log_prefix (char *prefix, int len);
  virtual int set_log_prefix (char *prefix);
  virtual int get_debugger_log_file (char *path, int len);
  virtual int set_debugger_log_file (char *path);
  virtual int get_floppy_options (int drive, bx_floppy_options *out);
  virtual int get_cdrom_options (int drive, bx_atadevice_options *out, int *device = NULL);
  virtual char *get_floppy_type_name (int type);
  virtual void set_notify_callback (bxevent_handler func, void *arg);
  virtual void get_notify_callback (bxevent_handler *func, void **arg);
  virtual BxEvent* sim_to_ci_event (BxEvent *event);
  virtual int log_msg (const char *prefix, int level, const char *msg);
  virtual int ask_param (const char *ppath);
  virtual int ask_param (bx_param_c *param);
  // ask the user for a pathname
  virtual int ask_filename (char *filename, int maxlen, char *prompt, char *the_default, int flags);
  // called at a regular interval, currently by the keyboard handler.
  virtual void periodic ();
  virtual int create_disk_image (const char *filename, int sectors, bx_bool overwrite);
  virtual void refresh_ci ();
  virtual void refresh_vga () {
    // maybe need to check if something has been initialized yet?
    DEV_vga_refresh();
  }
  virtual void handle_events () {
    // maybe need to check if something has been initialized yet?
    bx_gui->handle_events ();
  }
  // find first hard drive or cdrom
  bx_param_c *get_first_atadevice (Bit32u search_type);
  bx_param_c *get_first_cdrom () {
    return get_first_atadevice (BX_ATA_DEVICE_CDROM);
  }
  bx_param_c *get_first_hd () {
    return get_first_atadevice (BX_ATA_DEVICE_DISK);
  }
#if BX_DEBUGGER
  virtual void debug_break ();
  virtual void debug_interpret_cmd (char *cmd);
  virtual char *debug_get_next_command ();
  virtual void debug_puts (const char *cmd);
#endif
  virtual void register_configuration_interface (
    const char* name, 
    config_interface_callback_t callback,
    void *userdata);
  virtual int configuration_interface(const char* name, ci_command_t command);
  virtual int begin_simulation (int argc, char *argv[]);
  virtual void set_sim_thread_func (is_sim_thread_func_t func) {}
  virtual bool is_sim_thread ();
  bool wxsel;
  virtual bool is_wx_selected () { return wxsel; }
  // provide interface to bx_gui->set_display_mode() method for config
  // interfaces to use.
  virtual void set_display_mode (disp_mode_t newmode) {
    if (bx_gui != NULL)
      bx_gui->set_display_mode (newmode);
  }
  virtual bool test_for_text_console ();
  // save/restore interface
  virtual bool save_state (const char *checkpoint_name);
  virtual bool restore_state (const char *checkpoint_name);
};

bx_param_c *
bx_real_sim_c::get_param (const char *ppath, bx_param_c *base) 
{
  return param_get (ppath, base);
}

bx_param_num_c *
bx_real_sim_c::get_param_num (const char *ppath) {
  bx_param_c *generic = get_param(ppath);
  if (generic==NULL) {
    BX_PANIC (("get_param_num(%s) could not find a parameter", ppath));
    return NULL;
  }
  int type = generic->get_type ();
  if (type == BXT_PARAM_NUM || type == BXT_PARAM_BOOL || type == BXT_PARAM_ENUM)
    return (bx_param_num_c *)generic;
  BX_PANIC (("get_param_num(%s) could not find an integer parameter with that name", ppath));
  return NULL;
}

bx_param_string_c *
bx_real_sim_c::get_param_string (const char *ppath) {
  bx_param_c *generic = get_param(ppath);
  if (generic==NULL) {
    BX_PANIC (("get_param_string(%s) could not find a parameter", ppath));
    return NULL;
  }
  if (generic->get_type () == BXT_PARAM_STRING)
    return (bx_param_string_c *)generic;
  BX_PANIC (("get_param_string(%s) could not find an integer parameter with that name", ppath));
  return NULL;
}

bx_param_bool_c *
bx_real_sim_c::get_param_bool (const char *ppath) {
  bx_param_c *generic = get_param(ppath);
  if (generic==NULL) {
    BX_PANIC (("get_param_bool(%s) could not find a parameter", ppath));
    return NULL;
  }
  if (generic->get_type () == BXT_PARAM_BOOL)
    return (bx_param_bool_c *)generic;
  BX_PANIC (("get_param_bool(%s) could not find a bool parameter with that name", ppath));
  return NULL;
}

bx_param_enum_c *
bx_real_sim_c::get_param_enum (const char *ppath) {
  bx_param_c *generic = get_param(ppath);
  if (generic==NULL) {
    BX_PANIC (("get_param_enum(%s) could not find a parameter", ppath));
    return NULL;
  }
  if (generic->get_type () == BXT_PARAM_ENUM)
    return (bx_param_enum_c *)generic;
  BX_PANIC (("get_param_enum(%s) could not find a enum parameter with that name", ppath));
  return NULL;
}

void bx_init_siminterface ()
{
  siminterface_log = new logfunctions ();
  siminterface_log->put ("CTRL");
  siminterface_log->settype(CTRLLOG);
  if (SIM == NULL) 
    SIM = new bx_real_sim_c();
}

bx_simulator_interface_c::bx_simulator_interface_c ()
{
}

bx_real_sim_c::bx_real_sim_c ()
{
  bxevent_callback = NULL;
  bxevent_callback_data = NULL;
  ci_callback = NULL;
  ci_callback_data = NULL;
  is_sim_thread_func = NULL;
  wxsel = false;
  
  enabled = 1;
  init_done = 0;
  quit_context = NULL;
  exit_code = 0;
}

// called by constructor of bx_param_c, so that every parameter that is
// initialized gets registered.  This builds a list of all parameters
// which can be used to look them up by number (get_param).
bx_real_sim_c::~bx_real_sim_c ()
{
}

void 
bx_real_sim_c::reset_all_param ()
{
#warning FIXME: reset_all_param can no longer be used to "restore to factory defaults"
  BX_PANIC (("reset_all_param used to be used to reset configuration parameters.  But now every state of every devices is a parameter.  Resetting everything back to its original values would be basically resetting the machine. This needs work."));
  bx_reset_options ();
}

int 
bx_real_sim_c::get_n_log_modules ()
{
  return io->get_n_logfns ();
}

char *
bx_real_sim_c::get_prefix (int mod)
{
  logfunc_t *logfn = io->get_logfn (mod);
  return logfn->getprefix ();
}

int 
bx_real_sim_c::get_log_action (int mod, int level)
{
  logfunc_t *logfn = io->get_logfn (mod);
  return logfn->getonoff (level);
}

void 
bx_real_sim_c::set_log_action (int mod, int level, int action)
{
  // normal
  if (mod >= 0) {
	logfunc_t *logfn = io->get_logfn (mod);
	logfn->setonoff (level, action);
	return;
  }
  // if called with mod<0 loop over all
  int nmod = get_n_log_modules ();
  for (mod=0; mod<nmod; mod++)
	set_log_action (mod, level, action);
}

char *
bx_real_sim_c::get_action_name (int action)
{
  return io->getaction (action);
}

const char *
bx_real_sim_c::get_log_level_name (int level)
{
  return io->getlevel (level);
}

int 
bx_real_sim_c::get_max_log_level ()
{
  return N_LOGLEV;
}

void 
bx_real_sim_c::quit_sim (int code) {
  BX_INFO (("quit_sim called with exit code %d", code));
  exit_code = code;
  // use longjmp to quit cleanly, no matter where in the stack we are.
  //fprintf (stderr, "using longjmp() to jump directly to the quit context!\n");
  if (quit_context != NULL) {
    longjmp (*quit_context, 1);
    BX_PANIC (("in bx_real_sim_c::quit_sim, longjmp should never return"));
  }
  if (SIM->is_wx_selected ()) {
    // in wxWindows, the whole simulator is running in a separate thread.
    // our only job is to end the thread as soon as possible, NOT to shut
    // down the whole application with an exit.
    BX_CPU(0)->async_event = 1;
    BX_CPU(0)->kill_bochs_request = 1;
    // the cpu loop will exit very soon after this condition is set.
  } else {
    // just a single thread.  Use exit() to stop the application.
    if (!code)
      BX_PANIC (("Quit simulation command"));
    ::exit (exit_code);
  }
}

int
bx_real_sim_c::get_default_rc (char *path, int len)
{
  char *rc = bx_find_bochsrc ();
  if (rc == NULL) return -1;
  strncpy (path, rc, len);
  path[len-1] = 0;
  return 0;
}

int 
bx_real_sim_c::read_rc (char *rc)
{
  return bx_read_configuration (rc);
}

// return values:
//   0: written ok
//  -1: failed
//  -2: already exists, and overwrite was off
int 
bx_real_sim_c::write_rc (char *rc, int overwrite)
{
  return bx_write_configuration (rc, overwrite);
}

int 
bx_real_sim_c::get_log_file (char *path, int len)
{
  strncpy (path, bx_options.log.Ofilename->getptr (), len);
  return 0;
}

int 
bx_real_sim_c::set_log_file (char *path)
{
  bx_options.log.Ofilename->set (path);
  return 0;
}

int 
bx_real_sim_c::get_log_prefix (char *prefix, int len)
{
  strncpy (prefix, bx_options.log.Oprefix->getptr (), len);
  return 0;
}

int 
bx_real_sim_c::set_log_prefix (char *prefix)
{
  bx_options.log.Oprefix->set (prefix);
  return 0;
}

int 
bx_real_sim_c::get_debugger_log_file (char *path, int len)
{
  strncpy (path, bx_options.log.Odebugger_filename->getptr (), len);
  return 0;
}

int 
bx_real_sim_c::set_debugger_log_file (char *path)
{
  bx_options.log.Odebugger_filename->set (path);
  return 0;
}

int 
bx_real_sim_c::get_floppy_options (int drive, bx_floppy_options *out)
{
  *out = (drive==0)? bx_options.floppya : bx_options.floppyb;
  return 0;
}

int 
bx_real_sim_c::get_cdrom_options (int level, bx_atadevice_options *out, int *where)
{
  for (Bit8u channel=0; channel<BX_MAX_ATA_CHANNEL; channel++) {
    for (Bit8u device=0; device<2; device++) {
      if (bx_options.atadevice[channel][device].Otype->get() == BX_ATA_DEVICE_CDROM) {
        if (level==0) {
          *out = bx_options.atadevice[channel][device];
	  if (where != NULL) *where=(channel*2)+device;
          return 1;
          }
        else level--;
	}
      }
    }
  return 0;
}

char *bochs_start_names[] = { "quick", "load", "edit", "run" };
int n_bochs_start_names = 3;

char *floppy_type_names[] = { "none", "1.2M", "1.44M", "2.88M", "720K", "360K", "160K", "180K", "320K", NULL };
int floppy_type_n_sectors[] = { -1, 80*2*15, 80*2*18, 80*2*36, 80*2*9, 40*2*9, 40*1*8, 40*1*9, 40*2*8 };
int n_floppy_type_names = 9;

char *floppy_status_names[] = { "ejected", "inserted", NULL };
int n_floppy_status_names = 2;
char *floppy_bootdisk_names[] = { "floppy", "hard","cdrom", NULL };
int n_floppy_bootdisk_names = 3;
char *loader_os_names[] = { "none", "linux", "nullkernel", NULL };
int n_loader_os_names = 3;
char *keyboard_type_names[] = { "xt", "at", "mf", NULL };
int n_keyboard_type_names = 3;
char *atadevice_type_names[] = { "hard disk", "cdrom", NULL };
int n_atadevice_type_names = 2;
char *atadevice_status_names[] = { "ejected", "inserted", NULL };
int n_atadevice_status_names = 2;
char *atadevice_biosdetect_names[] = { "none", "auto", "cmos", NULL };
int n_atadevice_biosdetect_names = 3;
char *atadevice_translation_names[] = { "none", "lba", "large", "rechs", "auto", NULL };
int n_atadevice_translation_names = 3;

char *
bx_real_sim_c::get_floppy_type_name (int type)
{
  BX_ASSERT (type >= BX_FLOPPY_NONE && type <= BX_FLOPPY_LAST);
  type -= BX_FLOPPY_NONE;
  return floppy_type_names[type];
}

void 
bx_real_sim_c::set_notify_callback (bxevent_handler func, void *arg)
{
  bxevent_callback = func;
  bxevent_callback_data = arg;
}

void bx_real_sim_c::get_notify_callback (
    bxevent_handler *func,
    void **arg)
{
  *func = bxevent_callback;
  *arg = bxevent_callback_data;
}

BxEvent *
bx_real_sim_c::sim_to_ci_event (BxEvent *event)
{
  if (bxevent_callback == NULL) {
    BX_ERROR (("notify called, but no bxevent_callback function is registered"));
    return NULL;
  } else {
    return (*bxevent_callback)(bxevent_callback_data, event);
  }
}

// returns 0 for continue, 1 for alwayscontinue, 2 for die.
int 
bx_real_sim_c::log_msg (const char *prefix, int level, const char *msg)
{
  BxEvent be;
  be.type = BX_SYNC_EVT_LOG_ASK;
  be.u.logmsg.prefix = prefix;
  be.u.logmsg.level = level;
  be.u.logmsg.msg = msg;
  // default return value in case something goes wrong.
  be.retcode = BX_LOG_ASK_CHOICE_DIE;
  //fprintf (stderr, "calling notify.\n");
  sim_to_ci_event (&be);
  return be.retcode;
}

// Called by simulator whenever it needs the user to choose a new value
// for a registered parameter.  Create a synchronous ASK_PARAM event, 
// send it to the CI, and wait for the response.  The CI will call the
// set() method on the parameter if the user changes the value.
int 
bx_real_sim_c::ask_param (bx_param_c *param)
{
  BX_ASSERT (param != NULL);
  // create appropriate event
  BxEvent event;
  event.type = BX_SYNC_EVT_ASK_PARAM;
  event.u.param.param = param;
  sim_to_ci_event (&event);
  return event.retcode;
}

int
bx_real_sim_c::ask_param (const char *ppath)
{
  bx_param_c *param = SIM->get_param(ppath);
  return ask_param (param);
}

int
bx_real_sim_c::ask_filename (char *filename, int maxlen, char *prompt, char *the_default, int flags)
{
  // implement using ASK_PARAM on a newly created param.  I can't use
  // ask_param because I don't intend to register this param.
  BxEvent event;
  bx_param_string_c param (NULL, "filename", prompt, the_default, maxlen);
  flags |= param.IS_FILENAME;
  param.get_options()->set (flags);
  event.type = BX_SYNC_EVT_ASK_PARAM;
  event.u.param.param = &param;
  sim_to_ci_event (&event);
  if (event.retcode >= 0)
    memcpy (filename, param.getptr(), maxlen);
  return event.retcode;
}

void
bx_real_sim_c::periodic ()
{
  // give the GUI a chance to do periodic things on the bochs thread. in 
  // particular, notice if the thread has been asked to die.
  BxEvent tick;
  tick.type = BX_SYNC_EVT_TICK;
  sim_to_ci_event (&tick);
  if (tick.retcode < 0) {
    BX_INFO (("Bochs thread has been asked to quit."));
    bx_atexit ();
    quit_sim (0);
  }
  static int refresh_counter = 0;
  if (++refresh_counter == 50) {
    // only ask the CI to refresh every 50 times periodic() is called.
    // This should obviously be configurable because system speeds and
    // user preferences vary.
    refresh_ci ();
    refresh_counter = 0;
  }
#if 0
  // watch for memory leaks.  Allocate a small block of memory, print the
  // pointer that is returned, then free.
  BxEvent *memcheck = new BxEvent ();
  BX_INFO(("memory allocation at %p", memcheck));
  delete memcheck;
#endif
}

// create a disk image file called filename, size=512 bytes * sectors.
// If overwrite is true and the file exists, returns -1 without changing it.
// Otherwise, opens up the image and starts writing.  Returns -2 if
// the image could not be opened, or -3 if there are failures during
// write, e.g. disk full.
// 
// wxWindows: This may be called from the gui thread.
int 
bx_real_sim_c::create_disk_image (
    const char *filename,
    int sectors,
    bx_bool overwrite) 
{
  FILE *fp;
  if (!overwrite) {
    // check for existence first
    fp = fopen (filename, "r");
    if (fp) {
      // yes it exists
      fclose (fp);
      return -1;
    }
  }
  fp = fopen (filename, "w");
  if (fp == NULL) {
#ifdef HAVE_PERROR
    char buffer[1024];
    sprintf (buffer, "while opening '%s' for writing", filename);
    perror (buffer);
    // not sure how to get this back into the CI
#endif
    return -2;
  }
  int sec = sectors;
  /*
   * seek to sec*512-1 and write a single character.
   * can't just do: fseek(fp, 512*sec-1, SEEK_SET)
   * because 512*sec may be too large for signed int.
   */
  while (sec > 0)
  {
    /* temp <-- min(sec, 4194303)
     * 4194303 is (int)(0x7FFFFFFF/512)
     */
    int temp = ((sec < 4194303) ? sec : 4194303);
    fseek(fp, 512*temp, SEEK_CUR);
    sec -= temp;
  }

  fseek(fp, -1, SEEK_CUR);
  if (fputc('\0', fp) == EOF)
  {
    fclose (fp);
    return -3;
  }
  fclose (fp);
  return 0;
}

void bx_real_sim_c::refresh_ci () {
  if (SIM->is_wx_selected ()) {
    // presently, only wxWindows interface uses these events
    // It's an async event, so allocate a pointer and send it.
    // The event will be freed by the recipient.
    BxEvent *event = new BxEvent ();
    event->type = BX_ASYNC_EVT_REFRESH;
    sim_to_ci_event (event);
  }
}

bx_param_c *
bx_real_sim_c::get_first_atadevice (Bit32u search_type) {
  for (int channel=0; channel<BX_MAX_ATA_CHANNEL; channel++) {
    if (!bx_options.ata[channel].Opresent->get ())
      continue;
    for (int slave=0; slave<2; slave++) {
      Bit32u present = bx_options.atadevice[channel][slave].Opresent->get ();
      Bit32u type = bx_options.atadevice[channel][slave].Otype->get ();
      if (present && (type == search_type)) {
	return bx_options.atadevice[channel][slave].Omenu;
      }
    }
  }
  return NULL;
}

#if BX_DEBUGGER

// this can be safely called from either thread.
void bx_real_sim_c::debug_break () {
  bx_debug_break ();
}

// this should only be called from the sim_thread.
void bx_real_sim_c::debug_interpret_cmd (char *cmd) {
  if (!is_sim_thread ()) {
    fprintf (stderr, "ERROR: debug_interpret_cmd called but not from sim_thread\n");
    return;
  }
  bx_dbg_interpret_line (cmd);
}

char *bx_real_sim_c::debug_get_next_command ()
{
  fprintf (stderr, "begin debug_get_next_command\n");
  BxEvent event;
  event.type = BX_SYNC_EVT_GET_DBG_COMMAND;
  BX_INFO (("asking for next debug command"));
  sim_to_ci_event (&event);
  BX_INFO (("received next debug command: '%s'", event.u.debugcmd.command));
  if (event.retcode >= 0)
    return event.u.debugcmd.command;
  return NULL;
}

void bx_real_sim_c::debug_puts (const char *text)
{
  if (SIM->is_wx_selected ()) {
    // send message to the wxWindows debugger
    BxEvent *event = new BxEvent ();
    event->type = BX_ASYNC_EVT_DBG_MSG;
    event->u.logmsg.msg = text;
    sim_to_ci_event (event);
    // the event will be freed by the recipient
  } else {
    // text mode debugger: just write to console
    fputs (text, stderr);
    delete [] (char *)text;
  }
}
#endif

void 
bx_real_sim_c::register_configuration_interface (
  const char* name, 
  config_interface_callback_t callback,
  void *userdata)
{
  ci_callback = callback;
  ci_callback_data = userdata;
  registered_ci_name = name;
}

int 
bx_real_sim_c::configuration_interface(const char *ignore, ci_command_t command)
{
  bx_param_enum_c *ci_param = SIM->get_param_enum ("display.config_interface");
  char *name = ci_param->get_choice (ci_param->get ());
  if (!ci_callback) {
    BX_PANIC (("no configuration interface was loaded"));
    return -1;
  }
  if (strcmp (name, registered_ci_name) != 0) {
    BX_PANIC (("siminterface does not support loading one configuration interface and then calling another"));
    return -1;
  }
  if (!strcmp (name, "wx")) 
    wxsel = true;
  else
    wxsel = false;
  // enter configuration mode, just while running the configuration interface
  set_display_mode (DISP_MODE_CONFIG);
  int retval = (*ci_callback)(ci_callback_data, command);
  set_display_mode (DISP_MODE_SIM);
  return retval;
}

int 
bx_real_sim_c::begin_simulation (int argc, char *argv[])
{
  return bx_begin_simulation (argc, argv);
}

bool bx_real_sim_c::is_sim_thread ()
{
  if (is_sim_thread_func == NULL) return true;
  return (*is_sim_thread_func)();
}

// check if the text console exists.  On some platforms, if Bochs is
// started from the "Start Menu" or by double clicking on it on a Mac,
// there may be nothing attached to stdin/stdout/stderr.  This function
// tests if stdin/stdout/stderr are usable and returns false if not.
bool 
bx_real_sim_c::test_for_text_console ()
{
#if BX_WITH_CARBON
  // In a Carbon application, you have a text console if you run the app from
  // the command line, but if you start it from the finder you don't.
  if(!isatty(STDIN_FILENO)) return false;
#endif
  // default: yes
  return true;
}

bool 
bx_real_sim_c::save_state (const char *checkpoint_name)
{
  BX_INFO (("save_state(%s)", checkpoint_name));
  bx_param_c *root = get_param (".");
  bx_checkpoint_c chkpt;
  chkpt.write(checkpoint_name, root);
  //param_print_tree (root, 0);
  return true;
}

bool 
bx_real_sim_c::restore_state (const char *checkpoint_name)
{
  BX_INFO (("restore_state(%s)", checkpoint_name));
  bx_param_c *root = get_param (".");
  // some params can only be changed at configure time, not runtime.
  // Set init_done to 0 so that these values can be changed.
  set_init_done (0);
  bx_checkpoint_c chkpt;
  chkpt.read(checkpoint_name, root);
  //chkpt.write("checkpt2", root);
  set_init_done (1);
  return true;
}
