// -*- C++ -*-
/////////////////////////////////////////////////////////////////////////
// $Id: param.h,v 1.1.2.5 2003/11/22 08:07:05 slechta Exp $
/////////////////////////////////////////////////////////////////////////
// support for compiling param.cc without Bochs
#ifdef PARAM_STANDALONE
#define BOCHSAPI /*empty*/
#define BOCHSAPI_CYGONLY /*empty*/
typedef unsigned char Bit8u;
typedef   signed char Bit8s;
typedef unsigned short Bit16u;
typedef   signed short Bit16s;
typedef unsigned long Bit32u;
typedef   signed long Bit32s;
typedef unsigned long long Bit64u;
typedef unsigned long Bit32u;
typedef   signed long long Bit64s;
typedef Bit32u bx_bool;
class logfunctions {};
#define BX_ASSERT(x) assert(x)
#define BX_PANIC(x) {printf("PANIC: ");printf x; printf("\n");exit(0);}
#define BX_ERROR(x) {printf("ERROR: ");printf x; printf("\n");}
#define BX_INFO(x)  {printf("INFO: " );printf x; printf("\n");}
#define BX_DEBUG(x) /* {printf("DEBUG: ");printf x; printf("\n");} */
#endif

#define BX_PARAM_PATH_MAX 512

// technically, in an 8 bit signed the real minimum is -128, not -127.
// But if you decide to negate -128 you tend to get -128 again, so it's
// better not to use the absolute maximum in the signed range.
#define BX_MAX_BIT64U ( (Bit64u) -1           )
#define BX_MIN_BIT64U ( 0                     )
#define BX_MAX_BIT64S ( ((Bit64u) -1) >> 1    )
#define BX_MIN_BIT64S ( ~(BX_MAX_BIT64S)      )
#define BX_MAX_BIT32U ( (Bit32u) -1           )
#define BX_MIN_BIT32U ( 0                     )
#define BX_MAX_BIT32S ( ((Bit32u) -1) >> 1    )
#define BX_MIN_BIT32S ( ~(BX_MAX_BIT32S)      )
#define BX_MAX_BIT16U ( (Bit16u) -1           )
#define BX_MIN_BIT16U ( 0                     )
#define BX_MAX_BIT16S ( ((Bit16u) -1) >> 1    )
#define BX_MIN_BIT16S ( ~(BX_MAX_BIT16S)      )
#define BX_MAX_BIT8U  ( (Bit8u) -1            )
#define BX_MIN_BIT8U  ( 0                     )
#define BX_MAX_BIT8S  ( ((Bit8u) -1) >> 1     )
#define BX_MIN_BIT8S  ( ~(BX_MAX_BIT8S)       )



// list of possible types for bx_param_c and descendant objects
typedef enum {
  BXT_OBJECT = 201,
  BXT_PARAM,
  BXT_PARAM_NUM,
  BXT_PARAM_BOOL,
  BXT_PARAM_ENUM,
  BXT_PARAM_STRING,
  BXT_PARAM_DATA,
  BXT_LIST
} bx_objtype;

////////////////////////////////////////////////////////////////////
// parameter classes: bx_param_c and family
////////////////////////////////////////////////////////////////////
//
// All variables that can be configured through the CI are declared as
// "parameters" or objects of type bx_param_*.  There is a bx_param_*
// class for each type of data that the user would need to see and
// edit, e.g. integer, boolean, enum, string, filename, or list of
// other parameters.  The purpose of the bx_param_* class, in addition
// to storing the parameter's value, is to hold the name, description,
// and constraints on the value.  The bx_param_* class should hold
// everything that the CI would need to display the value and allow
// the user to modify it.  For integer parameters, the minimum and
// maximum allowed value can be defined, and the base in which it
// should be displayed and interpreted.  For enums, the
// bx_param_enum_c structure includes the list of values which the
// parameter can have.
//
// Also, some parameter classes support get/set callback functions to
// allow arbitrary code to be executed when the parameter is get/set. 
// An example of where this is useful: if you disable the NE2K card,
// the set() handler for that parameter can tell the user interface
// that the NE2K's irq, I/O address, and mac address should be
// disabled (greyed out, hidden, or made inaccessible).  The get/set
// methods can also check if the set() value is acceptable using
// whatever means and override it.
//
// The parameter concept is similar to the use of parameters in JavaBeans.

class bx_object_c;
class bx_param_c;
class bx_param_num_c;
class bx_param_enum_c;
class bx_param_bool_c;
class bx_param_string_c;
class bx_param_filename_c;
class bx_list_c;

class BOCHSAPI bx_object_c {
private:
  bx_objtype type;
protected:
  void set_type (bx_objtype type);
public:
  bx_object_c ();
  Bit8u get_type () { return type; }
  bx_bool is_type (Bit8u test_type) {
    // for now, do simple type comparison.
    // really should test for inherited types as well.
    return (type == test_type);
  }
};

class BOCHSAPI bx_param_c : public bx_object_c {
  BOCHSAPI_CYGONLY static const char *default_text_format;
protected:
  bx_list_c *parent;
  char *name;
  char *description;
  const char *text_format;  // printf format string. %d for ints, %s for strings, etc.
  char *ask_format;  // format string for asking for a new value
  int runtime_param;
  int enabled;
  bx_bool is_shadow;
public:
  bx_param_c (bx_param_c *parent, char *name, char *description);
  bx_param_c *get_parent () { return (bx_param_c *) parent; }
  bx_bool child_of (bx_param_c *test_ancestor);
  void get_param_path (char *path_out, int maxlen);
  bx_param_c *get_by_name (const char *name);
  void set_parent (bx_param_c *newparent);
  void set_format (const char *format) {text_format = format;}
  const char *get_format () {return text_format;}
  void set_ask_format (char *format) {ask_format = format; }
  char *get_ask_format () {return ask_format;}
  void set_runtime_param (int val) { runtime_param = val; }
  char *get_name () { return name; }
  char *get_description () { return description; }
  int get_enabled () { return enabled; }
  virtual void set_enabled (int enabled) { this->enabled = enabled; }
  virtual void reset () {}
  int getint () {return -1;}
  static const char* set_default_format (const char *f);
  static const char *get_default_format () { return default_text_format; }
  virtual bx_list_c *get_dependent_list () { return NULL; }
#if BX_UI_TEXT
  virtual void text_print (FILE *fp) {}
  virtual int text_ask (FILE *fpin, FILE *fpout) {return -1;}
#endif
  bx_bool  is_shadow_param() {return is_shadow;};
};

typedef Bit64s (*param_event_handler)(class bx_param_c *, int set, Bit64s val);

class BOCHSAPI bx_param_num_c : public bx_param_c {
  BOCHSAPI_CYGONLY static Bit32u default_base;
  // The dependent_list is initialized to NULL.  If dependent_list is modified
  // to point to a bx_list_c of other parameters, the set() method of
  // bx_param_bool_c will enable those parameters when this bool is true, and
  // disable them when this bool is false.
  bx_list_c *dependent_list;
  void update_dependents (bx_list_c *list);
protected:
  Bit64s min, max, initial_val;
  union _uval_ {
    Bit64s number;   // used by bx_param_num_c and bx_param_bool
    Bit64s *p64bit;  // used by bx_shadow_num_c
    Bit32s *p32bit;  // used by bx_shadow_num_c
    Bit16s *p16bit;  // used by bx_shadow_num_c
    Bit8s  *p8bit;   // used by bx_shadow_num_c
    bx_bool *pbool;  // used by bx_shadow_bool_c
  } val;
  param_event_handler handler;
  int base;
public:
  bx_param_num_c (bx_param_c *parent,
      char *name,
      char *description,
      Bit64s min, Bit64s max, Bit64s initial_val);
  virtual void reset ();
  void set_handler (param_event_handler handler);
  virtual bx_list_c *get_dependent_list () { return dependent_list; }
  void set_dependent_list (bx_list_c *l);
  virtual void set_enabled (int enabled);
  virtual Bit32s get (bx_bool ignore_handler=0) 
    { return (Bit32s) get64(ignore_handler); }
  virtual Bit64s get64 (bx_bool ignore_handler=0);
  virtual void set (Bit64s val, bx_bool ignore_handler=0);
  void set_base (int base) { this->base = base; }
  void set_initial_val (Bit64s initial_val);
  int get_base () { return base; }
  void set_range (Bit64u min, Bit64u max);
  Bit64s get_min () { return min; }
  Bit64s get_max () { return max; }
  static Bit32u set_default_base (Bit32u val);
  static Bit32u get_default_base () { return default_base; }
#if BX_UI_TEXT
  virtual void text_print (FILE *fp);
  virtual int text_ask (FILE *fpin, FILE *fpout);
#endif
};

// a bx_shadow_num_c is like a bx_param_num_c except that it doesn't
// store the actual value with its data. Instead, it uses val.p32bit
// to keep a pointer to the actual data.  This is used to register
// existing variables as parameters, without having to access it via
// set/get methods.
class BOCHSAPI bx_shadow_num_c : public bx_param_num_c {
  Bit8u varsize;   // must be 64, 32, 16, or 8
  Bit8u lowbit;   // range of bits associated with this param
  Bit64u mask;     // mask is ANDed with value before it is returned from get
  void init (int highbit, int lowbit);
public:
  bx_shadow_num_c (bx_param_c *parent,
      char *name,
      char *description,
      Bit64s *ptr_to_real_val,
      Bit8u highbit = 63,
      Bit8u lowbit = 0);
  bx_shadow_num_c (bx_param_c *parent,
      char *name,
      char *description,
      Bit64u *ptr_to_real_val,
      Bit8u highbit = 63,
      Bit8u lowbit = 0);
  bx_shadow_num_c (bx_param_c *parent,
      char *name,
      char *description,
      Bit32s *ptr_to_real_val,
      Bit8u highbit = 31,
      Bit8u lowbit = 0);
  bx_shadow_num_c (bx_param_c *parent,
      char *name,
      char *description,
      Bit32u *ptr_to_real_val,
      Bit8u highbit = 31,
      Bit8u lowbit = 0);
  bx_shadow_num_c (bx_param_c *parent,
      char *name,
      char *description,
      Bit16s *ptr_to_real_val,
      Bit8u highbit = 15,
      Bit8u lowbit = 0);
  bx_shadow_num_c (bx_param_c *parent,
      char *name,
      char *description,
      Bit16u *ptr_to_real_val,
      Bit8u highbit = 15,
      Bit8u lowbit = 0);
  bx_shadow_num_c (bx_param_c *parent,
      char *name,
      char *description,
      Bit8s *ptr_to_real_val,
      Bit8u highbit = 7,
      Bit8u lowbit = 0);
  bx_shadow_num_c (bx_param_c *parent,
      char *name,
      char *description,
      Bit8u *ptr_to_real_val,
      Bit8u highbit = 7,
      Bit8u lowbit = 0);
  virtual Bit64s get64 (bx_bool ignore_handler=0);
  virtual void set (Bit64s val, bx_bool ignore_handler=0);
  virtual void reset ();
};


typedef void* (*shadow_data_handler)(class bx_param_c *, int set, void** val);

class BOCHSAPI bx_shadow_data_c : public bx_param_c
{
 private:
  int data_size;
  void **data;
  shadow_data_handler handler;

 public:
  bx_shadow_data_c (bx_param_c *parent,
                    char *name,
                    char *description,
                    void **ptr_to_real_ptr, 
                    int data_size);

  void set_handler (shadow_data_handler handler);

  void set_data_size(int size) {data_size = size;};
  int  get_data_size() {return data_size;};
  virtual void set (void* new_data_ptr, bx_bool ignore_handler=0);
  virtual void* get(bx_bool ignore_handler=0);
};

class BOCHSAPI bx_param_bool_c : public bx_param_num_c {
  // many boolean variables are used to enable/disable modules.  In the
  // user interface, the enable variable should enable/disable all the
  // other parameters associated with that module.
public:
  bx_param_bool_c (bx_param_c *parent,
      char *name,
      char *description,
      Bit64s initial_val);
#if BX_UI_TEXT
  virtual void text_print (FILE *fp);
  virtual int text_ask (FILE *fpin, FILE *fpout);
#endif
};

// a bx_shadow_bool_c is a shadow param based on bx_param_bool_c.
class BOCHSAPI bx_shadow_bool_c : public bx_param_bool_c {
  // each bit of a bitfield can be a separate value.  bitnum tells which
  // bit is used.  get/set will only modify that bit.
  Bit8u bitnum;
public:
  bx_shadow_bool_c (bx_param_c *parent,
      char *name,
      char *description,
      bx_bool *ptr_to_real_val,
      Bit8u bitnum = 0);
  virtual Bit64s get64 (bx_bool ignore_handler=0);
  virtual void set (Bit64s val, bx_bool ignore_handler=0);
};


class BOCHSAPI bx_param_enum_c : public bx_param_num_c {
  char **choices;
public:
  bx_param_enum_c (bx_param_c *parent,
      char *name,
      char *description,
      char **choices,
      Bit64s initial_val,
      Bit64s value_base = 0);
  char *get_choice (int n) { return choices[n]; }
  int find_by_name (const char *string);
  bool set_by_name (const char *string, bx_bool ignore_handler=0);
#if BX_UI_TEXT
  virtual void text_print (FILE *fp);
  virtual int text_ask (FILE *fpin, FILE *fpout);
#endif
};

typedef char* (*param_string_event_handler)(class bx_param_string_c *, int set, char *val, int maxlen);

class BOCHSAPI bx_param_string_c : public bx_param_c {
  int maxsize;
  char *val, *initial_val;
  param_string_event_handler handler;
  bx_param_num_c *options;
  char separator;
public:
  enum {
    RAW_BYTES = 1,         // use binary text editor, like MAC addr
    IS_FILENAME = 2,       // 1=yes it's a filename, 0=not a filename.
                           // Some guis have a file browser. This
                           // bit suggests that they use it.
    SAVE_FILE_DIALOG = 4   // Use save dialog opposed to open file dialog
  } bx_string_opt_bits;
  bx_param_string_c (bx_param_c *parent,
      char *name,
      char *description,
      char *initial_val,
      int maxsize=-1);
  virtual ~bx_param_string_c ();
  virtual void reset ();
  void set_handler (param_string_event_handler handler);
  Bit32s get (char *buf, int len, bx_bool ignore_handler=0);
  char *getptr () {return val; }
  void set (char *buf, bx_bool ignore_handler=0);
  bx_bool equals (const char *buf);
  bx_param_num_c *get_options () { return options; }
  void set_separator (char sep) {separator = sep; }
#if BX_UI_TEXT
  virtual void text_print (FILE *fp);
  virtual int text_ask (FILE *fpin, FILE *fpout);
#endif
};

// Declare a filename class.  It is identical to a string, except that
// it initializes the options differently.  This is just a shortcut
// for declaring a string param and setting the options with IS_FILENAME.
class BOCHSAPI bx_param_filename_c : public bx_param_string_c {
public:
  bx_param_filename_c (bx_param_c *parent,
      char *name,
      char *description,
      char *initial_val,
      int maxsize=-1);
};

#define BX_DEFAULT_LIST_SIZE 6

class BOCHSAPI bx_list_c : public bx_param_c {
protected:
  // just a list of bx_param_c objects.  size tells current number of
  // objects in the list, and maxsize tells how many list items are
  // allocated in the constructor.
  bx_param_c **list;
  int size, maxsize;
  // options is a bit field whose bits are defined by bx_listopt_bits ORed
  // together.  Options is a bx_param so that if necessary the bx_list could
  // install a handler to cause get/set of options to have side effects.
  bx_param_num_c *options;
  // for a menu, the value of choice before the call to "ask" is default.
  // After ask, choice holds the value that the user chose.  Choice defaults
  // to 1 in the constructor.
  bx_param_num_c *choice;
  // title of the menu or series
  bx_param_string_c *title;
  // if the menu shows a "return to previous menu" type of choice,
  // "parent" controls where that choice will go.
  void init ();
public:
  enum {
    // When a bx_list_c is displayed as a menu, SHOW_PARENT controls whether or
    // not the menu shows a "Return to parent menu" choice or not.
    SHOW_PARENT = (1<<0),
    // Some lists are best displayed shown as menus, others as a series of
    // related questions.  This bit suggests to the CI that the series of
    // questions format is preferred.
    SERIES_ASK = (1<<1),
    // When a bx_list_c is displayed in a dialog, BX_USE_TAB_WINDOW suggests
    // to the CI that each item in the list should be shown as a separate
    // tab.  This would be most appropriate when each item is another list
    // of parameters.
    USE_TAB_WINDOW = (1<<2),
    // Normally, bx_list_c::add will not allow a child to be added if its
    // name matches another child.  However, in some cases we don't care if
    // names are unique.  When this option bit is set, children with nonunique
    // names are allowed.
    ALLOW_DUPS = (1<<3)
  } bx_listopt_bits;
  bx_list_c (bx_param_c *parent, int maxsize = BX_DEFAULT_LIST_SIZE);
  bx_list_c (bx_param_c *parent, char *name, char *description, bx_param_c **init_list);
  bx_list_c (bx_param_c *parent, char *name, char *description, int maxsize = BX_DEFAULT_LIST_SIZE);
  virtual ~bx_list_c();
  void add (bx_param_c *param);
  bx_param_c *get (int index);
  bx_param_c *get_by_name (const char *name);
  int get_size () { return size; }
  bx_param_num_c *get_options () { return options; }
  void set_options (bx_param_num_c *newopt) { options = newopt; }
  bx_param_num_c *get_choice () { return choice; }
  bx_param_string_c *get_title () { return title; }
  bx_param_c *get_parent () { return parent; }
#if BX_UI_TEXT
  virtual void text_print (FILE *);
  virtual int text_ask (FILE *fpin, FILE *fpout);
#endif
};

////////////////////////////////////////////////////////////////

void param_print_tree (bx_param_c *node, int level = 0);

/*---------------------------------------------------------------------------*/
// NOTE:  The following macros are testing the possibility of using implicit
// operands rather than using the lare number of explicit parameters that the
// BX_REGISTER functions require.  The goal is to make registering state in a
// given class extremely systematic and automatic which is all that is
// necessary for a large number of classes. --BJS
// TODO: for large arrays, we want to use a different approach.  perhaps, 
// we can use this macro to decide what approach we take with large arrays,
// and that way, the registration of devices is ignorant of this decision.
/*---------------------------------------------------------------------------*/

extern int bx_need_checkpoint;

/*---------------------------------------------------------------------------*/
// This macro sets up the environment necessary for the rest of the BXRS
// macros.  This environment includes a current "_bxrs_this" pointer which is
// a pointer to the current structure we are registering variables, a current
// "_bxrs_this_t" type which defines the type of the current "this" object,
// a current "_bxrs_cur_list_p" which a pointer to the bx_param_c list object
// that represents the object pointed to by "this", and a "_def_size" which is 
// the default size of the enclosing list params.
#define BXRS_START(_type, _this, _parent_p, _def_size)                        \
{                           /* one may look at some of this code and wonder */\
  void *_bxrs_this = _this; /* what is going on.  the seemingling pointless */\
/*void *_old_this = NULL+1;    operations were added to stop compiler       */\
/*_old_this--;                 warnings with unused variables.    --BJS     */\
  Bit64u _bxrs_def_size = _def_size+1;                                        \
  _bxrs_def_size--;                                                           \
  typedef _type _bxrs_this_t;                                                 \
  bx_list_c *_bxrs_cur_list_p = (bx_list_c*)_parent_p;

#define BXRS_END                                                              \
}

/*---------------------------------------------------------------------------*/
// register a struct in the current bxrs scope and begins a new bxrs scope
#define BXRS_STRUCT_START(_type, _var)                                        \
  BXRS_STRUCT_START_D(_type, _var, "")

#define BXRS_STRUCT_START_D(_type, _var, _desc)                               \
{                                                                             \
  void *_bxrs_old_this = (_bxrs_this_t *)_bxrs_this;                          \
  bx_list_c *_bxrs_old_list_p = _bxrs_cur_list_p;                             \
  bx_list_c *_bxrs_cur_list_p = new bx_list_c (_bxrs_old_list_p,              \
                                               #_var,                         \
                                               _desc,                         \
                                    _bxrs_def_size);                          \
  void *_bxrs_this = (&((*((_bxrs_this_t *)_bxrs_old_this))._var));           \
  typedef _type _bxrs_this_t;

#define BXRS_STRUCT_END                                                       \
}

/*---------------------------------------------------------------------------*/
// register a struct pointer in the current bxrs scope and begins a new bxrs scope
#define BXRS_STRUCTP_START(_type, _var)                                       \
  BXRS_STRUCT_START_D(_type, _var, "")

#define BXRS_STRUCTP_START_D(_type, _var, _desc)                              \
{                                                                             \
  void *_bxrs_old_this = (_bxrs_this_t *)_bxrs_this;                          \
  bx_list_c *_bxrs_old_list_p = _bxrs_cur_list_p;                             \
  bx_list_c *_bxrs_cur_list_p = new bx_list_c (_bxrs_old_list_p,              \
                                               #_var,                         \
                                               _desc,                         \
                                    _bxrs_def_size);                          \
  void *_bxrs_this = (&((((_bxrs_this_t *)_bxrs_old_this))->_var));           \
  typedef _type _bxrs_this_t;

#define BXRS_STRUCTP_END                                                      \
}

/*---------------------------------------------------------------------------*/
// register an enum variable in the current bxrs scope
#define BXRS_ENUM(_type, _var)                                                \
  BXRS_ENUM_D(_type, _var, "")

#define BXRS_ENUM_D(_type, _var, _desc)                                       \
  if (sizeof(_type) == 1) {                                                   \
    new bx_shadow_num_c(_bxrs_cur_list_p,                                     \
                        #_var,                                                \
                        _desc,                                                \
                        (Bit8u*)(&((*((_bxrs_this_t*)_bxrs_this))._var)));    \
  }                                                                           \
  else if (sizeof(_type) == 2) {                                              \
    new bx_shadow_num_c(_bxrs_cur_list_p,                                     \
                        #_var,                                                \
                        _desc,                                                \
                        (Bit16u*)(&((*((_bxrs_this_t*)_bxrs_this))._var)));   \
  }                                                                           \
  else if (sizeof(_type) == 4) {                                              \
    new bx_shadow_num_c(_bxrs_cur_list_p,                                     \
                        #_var,                                                \
                        _desc,                                                \
                        (Bit32u*)(&((*((_bxrs_this_t*)_bxrs_this))._var)));   \
  }                                                                           \
  else if (sizeof(_type) == 8) {                                              \
    new bx_shadow_num_c(_bxrs_cur_list_p,                                     \
                        #_var,                                                \
                        _desc,                                                \
                        (Bit64u*)(&((*((_bxrs_this_t*)_bxrs_this))._var)));   \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    BX_PANIC(("encountered an enum that is not 1, 2, 4, or 8 bytes"));        \
  }

/*---------------------------------------------------------------------------*/
// register an number variable in the current bxrs scope
#define BXRS_NUM(_type, _var)                                                 \
  BXRS_NUM_DH(_type, _var, "", NULL)

#define BXRS_NUM_D(_type, _var, _desc)                                        \
  BXRS_NUM_DH(_type, _var, _desc, NULL);

#define BXRS_NUM_H(_type, _var, _handler)                                     \
  BXRS_NUM_DH(_type, _var, "", _handler);

#define BXRS_NUM_D(_type, _var, _desc)                                        \
  BXRS_NUM_DH(_type, _var, _desc, NULL);

#define BXRS_NUM_DH(_type, _var, _desc, _handler)                              \
  (new bx_shadow_num_c(_bxrs_cur_list_p,                                       \
                     #_var,                                                    \
                     _desc,                                                    \
                     (_type*)(&((*((_bxrs_this_t*)_bxrs_this))._var))))->      \
  set_handler(_handler);

/*---------------------------------------------------------------------------*/
// register an boolean variable in the current bxrs scope
#define BXRS_BOOL(_type, _var)                                                \
  BXRS_BOOL_D(_type, _var, "")

#define BXRS_BOOL_D(_type, _var, _desc)                                       \
  new bx_shadow_bool_c(_bxrs_cur_list_p,                                      \
                     #_var,                                                   \
                     _desc,                                                   \
                     (_type*)(&((*((_bxrs_this_t*)_bxrs_this))._var)));

/*---------------------------------------------------------------------------*/
// register certain bits of a number variable in the current bxrs scope
#define BXRS_BITS(_type, _var, _highbit, _lowbit)                             \
  BXRS_BITS_D(_type, _var, _highbit, _lowbit, "")

#define BXRS_BITS_D(_type, _var, _highbit, _lowbit, _desc)                    \
  new bx_shadow_num_c(_bxrs_cur_list_p,                                       \
                     #_var,                                                   \
                     _desc,                                                   \
                     (_type*)(&((*((_bxrs_this_t*)_bxrs_this))._var)),        \
                     _highbit,                                                \
                     _lowbit);

/*---------------------------------------------------------------------------*/
// register an array of structs variable in the current bxrs scope and begin
// a new bxrs scope.
#define BXRS_ARRAY_START(_type, _var, _size)                                  \
  BXRS_ARRAY_START_D(_type, _var, _size, "")

#define BXRS_ARRAY_START_D(_type, _var, _size, _desc)                         \
{                                                                             \
  static char foobar[_size][30];                                              \
  char *_index_name = foobar[0];                                              \
  bx_list_c *_bxrs_old_list_p = _bxrs_cur_list_p;                             \
  bx_list_c *_bxrs_cur_list_p =                                               \
    new bx_list_c(_bxrs_old_list_p, #_var, _desc,_size);                      \
  void *_bxrs_arr_this = &(((_bxrs_this_t *)_bxrs_this)->_var);               \
  for (int _itr = 0;                                                          \
       (_itr < _size) &&                                                      \
         (sprintf(foobar[_itr], "%d", _itr),                                  \
          _index_name = foobar[_itr],                                         \
          true);                                                              \
       _itr++)                                                                \
{                                                                             \
  typedef _type _bxrs_this_t;                                                 \
  bx_list_c *_bxrs_old_list_p = _bxrs_cur_list_p;                             \
  bx_list_c *_bxrs_cur_list_p = new bx_list_c (_bxrs_old_list_p,              \
                                    _index_name,                              \
                                    "",                                       \
                                    _bxrs_def_size);                          \
  void *_bxrs_this = &(((_bxrs_this_t *)_bxrs_arr_this)[_itr]);

#define BXRS_ARRAY_END                                                        \
}}

/*---------------------------------------------------------------------------*/
// register an array of number variables in the current bxrs scope
// TODO: for large arrays, we want to use a different approach.  perhaps, 
// we can use this macro to decide what approach we take with large arrays,
// and that way, the registration of devices is ignorant of this decisio.
#define BXRS_ARRAY_NUM(_type, _var, _size)                                    \
  BXRS_ARRAY_NUM_D(_type, _var, _size, "")                                    \

#define BXRS_ARRAY_NUM_D(_type, _var, _size, _desc)                           \
{                                                                             \
  static char foobar[_size][30];                                              \
  char *_index_name = foobar[0];                                              \
  bx_list_c *_bxrs_old_list_p = _bxrs_cur_list_p;                             \
  bx_list_c *_bxrs_cur_list_p =                                               \
    new bx_list_c(_bxrs_old_list_p, #_var, _desc, _size);                     \
  for (int _itr = 0;                                                          \
       (_itr < _size) &&                                                      \
         (sprintf(foobar[_itr], "%d", _itr),                                  \
          _index_name = foobar[_itr],                                         \
          true);                                                              \
       _itr++)                                                                \
  new bx_shadow_num_c(_bxrs_cur_list_p, _index_name, "",                      \
                      &((*((_bxrs_this_t*)_bxrs_this))._var[_itr])   );       \
}

/*---------------------------------------------------------------------------*/
// register an array of enum variables in the current bxrs scope
#define BXRS_ARRAY_ENUM(_type, _var, _size)                                   \
  BXRS_ARRAY_ENUM_D(_type, _var, _size, "")

#define BXRS_ARRAY_ENUM_D(_type, _var, _size, _desc)                          \
{                                                                             \
  static char foobar[_size][30];                                              \
  char *_index_name = foobar[0];                                              \
  bx_list_c *_bxrs_old_list_p = _bxrs_cur_list_p;                             \
  bx_list_c *_bxrs_cur_list_p =                                               \
    new bx_list_c(_bxrs_old_list_p, #_var, _desc,_size);                      \
  for (int _itr = 0;                                                          \
       (_itr < _size) &&                                                      \
         (sprintf(foobar[_itr], "%d", _itr),                                  \
          _index_name = foobar[_itr],                                         \
          true);                                                              \
       _itr++) {                                                              \
    if (sizeof(_type) == 1) {                                                 \
      new bx_shadow_num_c(_bxrs_cur_list_p, _index_name, "",                  \
                          (Bit8u*)&((*((_bxrs_this_t*)_bxrs_this))._var[_itr]));\
    }                                                                         \
    else if (sizeof(_type) == 2) {                                            \
      new bx_shadow_num_c(_bxrs_cur_list_p, _index_name, "",                  \
                          (Bit16u*)&((*((_bxrs_this_t*)_bxrs_this))._var[_itr]));\
    }                                                                         \
    else if (sizeof(_type) == 4) {                                            \
      new bx_shadow_num_c(_bxrs_cur_list_p, _index_name, "",                  \
                          (Bit32u*)&((*((_bxrs_this_t*)_bxrs_this))._var[_itr]));\
    }                                                                         \
    else if (sizeof(_type) == 8) {                                            \
      new bx_shadow_num_c(_bxrs_cur_list_p, _index_name, "",                  \
                          (Bit64u*)&((*((_bxrs_this_t*)_bxrs_this))._var[_itr]));\
    }                                                                         \
  }                                                                           \
}  

/*---------------------------------------------------------------------------*/
// register an array of bx_bool variables in the current bxrs scope
#define BXRS_ARRAY_BOOL(_type, _var, _size)                                   \
  BXRS_ARRAY_BOOL_D(_type, _var, _size, "")

#define BXRS_ARRAY_BOOL_D(_type, _var, _size, _desc)                           \
{                                                                             \
  static char foobar[_size][30];                                              \
  char *_index_name = foobar[0];                                              \
  bx_list_c *_bxrs_old_list_p = _bxrs_cur_list_p;                             \
  bx_list_c *_bxrs_cur_list_p =                                               \
    new bx_list_c(_bxrs_old_list_p, #_var, _desc, _size);                     \
  for (int _itr = 0;                                                          \
       (_itr < _size) &&                                                      \
         (sprintf(foobar[_itr], "%d", _itr),                                  \
          _index_name = foobar[_itr],                                         \
          true);                                                              \
       _itr++)                                                                \
  new bx_shadow_bool_c((bx_param_c*)_bxrs_cur_list_p, _index_name, "",        \
                      &(((((_bxrs_this_t*)_bxrs_this))->_var)[_itr])   );     \
}


/*---------------------------------------------------------------------------*/
// register an object variable in the current bxrs scope by calling that 
// object's register_state() function.
#define BXRS_OBJ(_type, _var)                                                 \
  BXRS_OBJ_D(_type, _var, "")

#define BXRS_OBJ_D(_type, _var, _desc)                                        \
{                                                                             \
  _bxrs_this_t* _bxrs_obj = (_bxrs_this_t*)_bxrs_this;                        \
  BXRS_STRUCT_START_D(_type, _var, _desc);                                    \
  ((_type*)&(( _bxrs_obj )->_var))->                                          \
    register_state(/*#_var, _desc,*/ _bxrs_cur_list_p);                       \
  UNUSED(_bxrs_this);                                                         \
  BXRS_STRUCT_END;                                                            \
}

/*---------------------------------------------------------------------------*/
// register an pointer object variable in the current bxrs scope by calling that 
// object's register_state() function.
#define BXRS_OBJP(_type, _var_p)                                              \
   BXRS_OBJP_D(_type, _var_p, "")                                             \

#define BXRS_OBJP_D(_type, _var_p, _desc)                                     \
{                                                                             \
  _bxrs_this_t* _bxrs_obj = (_bxrs_this_t*)_bxrs_this;                        \
  BXRS_STRUCTP_START_D(_type, _var_p, _desc);                                 \
  ((_type*)(( _bxrs_obj )->_var_p))->register_state(_bxrs_cur_list_p);        \
  UNUSED(_bxrs_this);                                                         \
  BXRS_STRUCTP_END;                                                           \
}


/*---------------------------------------------------------------------------*/
// register an array of object variables in the current bxrs scope by calling
// that object's register_state() function.
#define BXRS_ARRAY_OBJ(_type, _var, _size)                                    \
  BXRS_ARRAY_OBJ_D(_type, _var, _size, "")                                    \

#define BXRS_ARRAY_OBJ_D(_type, _var, _size, _desc)                           \
{                                                                             \
  _bxrs_this_t* _bxrs_obj = (_bxrs_this_t*)_bxrs_this;                        \
  BXRS_ARRAY_START_D(_type, _var, _size, _desc);                              \
  ((_type*)&(( _bxrs_obj )->_var[_itr]))->                                    \
    register_state(/*#_var, _desc,*/ _bxrs_cur_list_p);                       \
  UNUSED(_bxrs_this);                                                         \
  BXRS_ARRAY_END;                                                             \
}
/*---------------------------------------------------------------------------*/
//#define BXRS_UNION_START(_unionvar)            
//  BXRS_UNION_START(_unionvar, "")              
//                                               
//#define BXRS_UNION_START_D(_unionvar, _desc)   
//  BXRS_STRUCT_START_D(u_t , _unionvar, _desc); 
//                                               
//#define BXRS_UNION_END                         
//  BXRS_STRUCT_END;
  
#define BXRS_UNION_START {
#define BXRS_UNION_START_D BXRS_UNION_START
#define BXRS_UNION_END }

/*---------------------------------------------------------------------------*/
// for registering pointer variables that are relative to another pointer
#define BXRS_RELPTR(_type, _var, _sizetype, _basevar)                         \
  BXRS_RELTPR_D(_type, _var, _sizetype, _basevar, "")

#define BXRS_RELPTR_D(_type, _var, _sizetype, _basevar, _desc) assert(0);
// BJS TODO: implement BXRS_RELNUM_D

/*---------------------------------------------------------------------------*/
#define BXRS_DARRAY(_ptr, _len)                                               \
  BXRS_DARRAY_DH(_ptr, _len, "", NULL);

#define BXRS_DARRAY_D(_ptr, _len, _desc)                                      \
  BXRS_DARRAY_DH(_ptr, _len, _desc, NULL);

#define BXRS_DARRAY_H(_ptr, _len, _handler)                                   \
  BXRS_DARRAY_DH(_ptr, _len, "", _handler);

#define BXRS_DARRAY_DH(_ptr, _len, _desc, _handler)                           \
  (new bx_shadow_data_c(_bxrs_cur_list_p,                                     \
                     #_ptr,                                                   \
                     _desc,                                                   \
                     (void**) &(((*((_bxrs_this_t*)_bxrs_this))._ptr)),       \
                     _len)); 



/*---------------------------------------------------------------------------*/
#define BXRS_UNUSED                                                           \
  UNUSED(_bxrs_this);

#define BXRS_THIS ((_bxrs_this_t*)_bxrs_this)

#define BXRS_THIS_TYPE _bxrs_this_t

/*---------------------------------------------------------------------------*/
// maximum size of a line in checkpoint ascii file
#define MAX_CHECKPOINT_LINE_SIZE 256  

static const char* m_state_filename = "state";
static const char* m_data_filename = "data";

/*---------------------------------------------------------------------------*/
// This class was created to encapsulate the save/restore mechanisms related
// to checkpointing the bochs pc system.  --BJS
// FIXME: this class should have a different name since it can include other
// functions related to the state registration tree.  For instance, we can
// merge checkpoint functions with functions that are used to display the tree
// to the screen (ie, dump to STDOUT or display internal debugger windows)
class bx_checkpoint_c {
 public:
  bx_checkpoint_c();
  ~bx_checkpoint_c();

  // given a checkpoint name and the state registry tree, read() and write()
  // serialize the bochs system with the appropriate files on the host 
  // platform disk.  These files will be saved and loaded from the directory
  // named by variable checkpoint_name.
  int write(const char *checkpoint_name, bx_param_c *current_tree_p);

  // The local tree passed in must be an initialized tree of params that
  // correspond to the tree of registered state.  The checkpoint files will
  // be read from the host platform's disk, and the values of corresponding 
  // parameters will be loaded into the params and pointers in the
  // current_tree_p passed into the function.
  int read(const char *checkpoint_name, bx_param_c *param_tree_p);

  // the state registration tree will be dumped to the STDOUT
  void dump_param_tree(bx_param_c *param_tree_p);

 private:
  // file pointers used for serializing bochs with data files
  FILE *m_ascii_fp;
  FILE *m_data_fp;
  
  // buffer used to get data from file and a cursor marking the current pos.
  char  m_line_buf[MAX_CHECKPOINT_LINE_SIZE];  
  char *m_line_buf_cursor;

  // dump the parameter tree to the appropriate FILE stream.
  void save_param_tree(bx_param_c *param_tree_p, int level=0);
  void load_param_tree(bx_param_c *param_tree_p,
                       char *qualified_path_str="");

  // Once a the parameter name and value string have been extracted, these
  // functions load the values (or traverse the tree in the case of list)
  // into the appropriate parameter on the current tree level in bx_list_c*
  // parent_p.
  bx_bool load_param_hex_num(bx_param_c *parent_p, 
                             char *param_str, 
                             char *value_str,
                             char *qualified_path_str);
  bx_bool load_param_dec_num(bx_param_c *parent_p, 
                             char *param_str, 
                             char *value_str,
                             char *qualified_path_str);
  bx_bool load_param_string(bx_param_c *parent_p, 
                            char *param_str, 
                            char *value_str,
                            char *qualified_path_str);
  bx_bool load_param_bool(bx_param_c *parent_p, 
                          char *param_str, 
                          char *value_str,
                          char *qualified_path_str);
  bx_bool load_param_enum(bx_param_c *parent_p, 
                          char *param_str, 
                          char *value_str,
                          char *qualified_path_str);
  bx_bool load_param_list(bx_param_c *parent_p, 
                          char *param_str, 
                          char *value_str,
                          char *qualified_path_str);
  bx_bool load_param_data(bx_param_c *parent_p, 
                          char *param_str, 
                          char *value_str,
                          char *qualified_path_str);

  // read one token from the current ascii file (m_ascii_fp)
  // NOTE: read_next_param() consumes the '=' character but does not return it
  // and read_next_value () consumes '\n' but does not return it.  Also, both
  // functions do not return surrounding white space
  char *read_next_param();
  char *read_next_value();
};


void bx_init_param();
bx_param_c *bx_param_get_root();
bx_param_c *param_get(const char *ppath, bx_param_c *base) ;
