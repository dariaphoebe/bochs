/////////////////////////////////////////////////////////////////////////
// $Id: slowdown_timer.h,v 1.7.6.1 2003/04/04 03:46:09 slechta Exp $
/////////////////////////////////////////////////////////////////////////
//

#if BX_USE_SLOWDOWN_TIMER

class bx_slowdown_timer_c {

private:
  struct s_type {
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
  void register_state(bx_param_c *list_p);
  void reset(unsigned type);

  static void timer_handler(void * this_ptr);

  void handle_timer();

};

extern bx_slowdown_timer_c bx_slowdown_timer;

#endif

