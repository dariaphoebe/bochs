/////////////////////////////////////////////////////////////////////////
// $Id: slowdown_timer.h,v 1.8.14.1 2004/11/05 00:56:48 slechta Exp $
/////////////////////////////////////////////////////////////////////////
//

class bx_slowdown_timer_c : public logfunctions {

private:
  struct s_t {
    Bit64u start_time;
    Bit64u start_emulated_time;
    Bit64u lasttime;

    int timer_handle;

    float MAXmultiplier;
    Bit64u Q; // (Q (in seconds))
  } s;

public:
  bx_slowdown_timer_c();

  void init(void);
  void reset(unsigned type);

#if BX_SAVE_RESTORE
  virtual void register_state(sr_param_c *list_p);
  virtual void before_save_state () {};
  virtual void after_restore_state () {};
#endif // #if BX_SAVE_RESTORE

  static void timer_handler(void * this_ptr);

  void handle_timer();

};

extern bx_slowdown_timer_c bx_slowdown_timer;

