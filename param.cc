/////////////////////////////////////////////////////////////////////////
// $Id: param.cc,v 1.1.2.1 2003/03/30 07:53:53 bdenney Exp $
/////////////////////////////////////////////////////////////////////////

#ifndef PARAM_STANDALONE
#include "bochs.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "param.h"
#endif

logfunctions *param_log = NULL;
#define LOG_THIS param_log->
bx_list_c *root_param = NULL;

void bx_init_param()
{
#ifndef PARAM_STANDALONE
  param_log = new logfunctions ();
  param_log->put ("PARM");
  param_log->settype(PARAMLOG);
#endif
  if (root_param == NULL) {
    root_param = new bx_list_c (NULL, 
	"root",
	"list of top level bochs parameters", 
	30);
  }
}

bx_param_c *
bx_param_get_root ()
{
  return root_param;
}

/////////////////////////////////////////////////////////////////////////
// define methods of bx_param_* and family
/////////////////////////////////////////////////////////////////////////

bx_object_c::bx_object_c ()
{
}

void
bx_object_c::set_type (bx_objtype type)
{
  this->type = type;
}

const char* bx_param_c::default_text_format = NULL;

bx_param_c::bx_param_c (bx_param_c *parent, char *name, char *description)
  : bx_object_c ()
{
  set_type (BXT_PARAM);
  this->name = name;
  this->description = description;
  this->text_format = default_text_format;
  this->ask_format = NULL;
  this->runtime_param = 0;
  this->enabled = 1;
  this->parent = NULL;
  this->is_shadow = 0; // can be re-set to 1 in shadow constructors --BJS
  if (parent) {
    BX_ASSERT (parent->is_type (BXT_LIST));
    this->parent = (bx_list_c *)parent;
    this->parent->add (this);
  }
}

void bx_param_c::get_param_path (char *path_out, int maxlen)
{
  if (get_parent() == NULL || get_parent() == root_param) {
    // Start with an empty string.
    // Never print the name of the root param.
    path_out[0] = 0;
  } else {
    // build path of the parent, add a period, add path of this node
    get_parent()->get_param_path (path_out, maxlen);
    strncat (path_out, ".", maxlen);
  }
  strncat (path_out, name, maxlen);
}


bx_param_c *bx_param_c::get_by_name (const char *name)
{
  if (is_type (BXT_LIST)) {
    return ((bx_list_c *)this)->get_by_name (name);
  }
  char ppath[BX_PATHNAME_LEN];
  get_param_path (ppath, BX_PATHNAME_LEN);
  BX_PANIC (("get_by_name called on non-list param %s", ppath));
  return NULL;
}

bx_bool bx_param_c::child_of (bx_param_c *test_ancestor)
{
  bx_param_c *param;
  for (param = this; param != NULL; param=param->get_parent ()) {
    if (param == test_ancestor) return true;
  }
  return false;
}

void bx_param_c::set_parent (bx_param_c *newparent) {
  if (parent) {
    // if this object already had a parent, the correct thing
    // to do would be to remove this object from the parent's
    // list of children.  Deleting children is currently
    // not supported.
    BX_PANIC (("bx_list_c::set_parent: changing from one parent to another is not supported (object is '%s')", get_name()));
  }
  if (newparent) {
    BX_ASSERT(newparent->is_type (BXT_LIST));
    this->parent = (bx_list_c *)newparent;
    this->parent->add (this);
  }
}

const char* bx_param_c::set_default_format (const char *f) {
  const char *old = default_text_format;
  default_text_format = f; 
  return old;
}

bx_param_num_c::bx_param_num_c (bx_param_c *parent,
    char *name,
    char *description,
    Bit64s min, Bit64s max, Bit64s initial_val)
  : bx_param_c (parent, name, description)
{
  set_type (BXT_PARAM_NUM);
  this->min = min;
  this->max = max;
  this->initial_val = initial_val;
  this->val.number = initial_val;
  this->handler = NULL;
  this->base = default_base;
  // dependent_list must be initialized before the set(),
  // because set calls update_dependents().
  dependent_list = NULL;
  set (initial_val);
}

Bit32u bx_param_num_c::default_base = 10;

Bit32u bx_param_num_c::set_default_base (Bit32u val) {
  Bit32u old = default_base;
  default_base = val; 
  return old;
}

void 
bx_param_num_c::reset ()
{
  this->val.number = initial_val;
}

void 
bx_param_num_c::set_handler (param_event_handler handler)
{ 
  this->handler = handler; 
  // now that there's a handler, call set once to run the handler immediately
  //set (get ());
}

void bx_param_num_c::set_dependent_list (bx_list_c *l) {
  dependent_list = l; 
  update_dependents (dependent_list);
}

Bit64s 
bx_param_num_c::get64 ()
{
  if (handler) {
    // the handler can decide what value to return and/or do some side effect
    return (*handler)(this, 0, val.number);
  } else {
    // just return the value
    return val.number;
  }
}

void
bx_param_num_c::set (Bit64s newval, bx_bool ignore_handler)
{
  if (!ignore_handler && handler) {
    // the handler can override the new value and/or perform some side effect
    // BBD: we should really send the oldval and newval to the handler,
    // so that it can change it back if necessary.  Maybe it should be
    // changed to   val.number = (*handler)(this, 1, val.number, newval);
    val.number = newval;
    (*handler)(this, 1, newval);
  } else {
    // just set the value
    val.number = newval;
  }
  if ((val.number < min || val.number > max) && max != BX_MAX_BIT64U) {
    char ppath[BX_PATHNAME_LEN];
    get_param_path (ppath, BX_PATHNAME_LEN);
    BX_PANIC (("numerical parameter '%s' was set to %lld, which is out of range %lld to %lld", ppath, val.number, min, max));
  }
  if (dependent_list != NULL) update_dependents (dependent_list);
}

void bx_param_num_c::set_range (Bit64u min, Bit64u max)
{
  this->min = min;
  this->max = max;
}

void bx_param_num_c::set_initial_val (Bit64s initial_val) { 
  this->val.number = this->initial_val = initial_val;
}

void bx_param_num_c::update_dependents (bx_list_c *list)
{
  if (list) {
    int en = val.number && enabled;
    for (int i=0; i<list->get_size (); i++) {
      bx_param_c *param = list->get (i);
      if (param != this)
	param->set_enabled (en);
      // descend into lists
      if (param->is_type (BXT_LIST)) {
	BX_INFO (("update_dependents descending into list '%s'", param->get_name()));
	update_dependents ((bx_list_c *)param);
	BX_INFO (("update_dependents returned from descent into list '%s'", param->get_name()));
      }
    }
  }
}

void
bx_param_num_c::set_enabled (int en)
{
  bx_param_c::set_enabled (en);
  update_dependents (dependent_list);
}

// Signed 64 bit
bx_shadow_num_c::bx_shadow_num_c (bx_param_c *parent,
    char *name,
    char *description,
    Bit64s *ptr_to_real_val,
    Bit8u highbit,
    Bit8u lowbit)
: bx_param_num_c (parent, name, description, BX_MIN_BIT64S, BX_MAX_BIT64S, *ptr_to_real_val)
{
  this->varsize = 16;
  this->lowbit = lowbit;
  this->mask = (1 << (highbit - lowbit)) - 1;
  val.p64bit = ptr_to_real_val;
  this->is_shadow = 1;
}

// Unsigned 64 bit
bx_shadow_num_c::bx_shadow_num_c (bx_param_c *parent,
    char *name,
    char *description,
    Bit64u *ptr_to_real_val,
    Bit8u highbit,
    Bit8u lowbit)
: bx_param_num_c (parent, name, description, BX_MIN_BIT64U, BX_MAX_BIT64U, *ptr_to_real_val)
{
  this->varsize = 16;
  this->lowbit = lowbit;
  this->mask = (1 << (highbit - lowbit)) - 1;
  val.p64bit = (Bit64s*) ptr_to_real_val;
  this->is_shadow = 1;
}

// Signed 32 bit
bx_shadow_num_c::bx_shadow_num_c (bx_param_c *parent,
    char *name,
    char *description,
    Bit32s *ptr_to_real_val,
    Bit8u highbit,
    Bit8u lowbit)
: bx_param_num_c (parent, name, description, (Bit32s)BX_MIN_BIT32S, (Bit32s)BX_MAX_BIT32S, *ptr_to_real_val)
{
  this->varsize = 16;
  this->lowbit = lowbit;
  this->mask = (1 << (highbit - lowbit)) - 1;
  val.p32bit = ptr_to_real_val;
  this->is_shadow = 1;	
}

// Unsigned 32 bit
bx_shadow_num_c::bx_shadow_num_c (bx_param_c *parent,
    char *name,
    char *description,
    Bit32u *ptr_to_real_val,
    Bit8u highbit,
    Bit8u lowbit)
: bx_param_num_c (parent, name, description, BX_MIN_BIT32U, BX_MAX_BIT32U, *ptr_to_real_val)
{
  this->varsize = 32;
  this->lowbit = lowbit;
  this->mask = (1 << (highbit - lowbit)) - 1;
  val.p32bit = (Bit32s*) ptr_to_real_val;
  this->is_shadow = 1;
}

// Signed 16 bit
bx_shadow_num_c::bx_shadow_num_c (bx_param_c *parent,
    char *name,
    char *description,
    Bit16s *ptr_to_real_val,
    Bit8u highbit,
    Bit8u lowbit)
: bx_param_num_c (parent, name, description, BX_MIN_BIT16S, BX_MAX_BIT16S, *ptr_to_real_val)
{
  this->varsize = 16;
  this->lowbit = lowbit;
  this->mask = (1 << (highbit - lowbit)) - 1;
  val.p16bit = ptr_to_real_val;
  this->is_shadow = 1;
}

// Unsigned 16 bit
bx_shadow_num_c::bx_shadow_num_c (bx_param_c *parent,
    char *name,
    char *description,
    Bit16u *ptr_to_real_val,
    Bit8u highbit,
    Bit8u lowbit)
: bx_param_num_c (parent, name, description, BX_MIN_BIT16U, BX_MAX_BIT16U, *ptr_to_real_val)
{
  this->varsize = 16;
  this->lowbit = lowbit;
  this->mask = (1 << (highbit - lowbit)) - 1;
  val.p16bit = (Bit16s*) ptr_to_real_val;
  this->is_shadow = 1;
}

// Signed 8 bit
bx_shadow_num_c::bx_shadow_num_c (bx_param_c *parent,
    char *name,
    char *description,
    Bit8s *ptr_to_real_val,
    Bit8u highbit,
    Bit8u lowbit)
: bx_param_num_c (parent, name, description, BX_MIN_BIT8S, BX_MAX_BIT8S, *ptr_to_real_val)
{
  this->varsize = 16;
  this->lowbit = lowbit;
  this->mask = (1 << (highbit - lowbit)) - 1;
  val.p8bit = ptr_to_real_val;
  this->is_shadow = 1;
}

// Unsigned 8 bit
bx_shadow_num_c::bx_shadow_num_c (bx_param_c *parent,
    char *name,
    char *description,
    Bit8u *ptr_to_real_val,
    Bit8u highbit,
    Bit8u lowbit)
: bx_param_num_c (parent, name, description, BX_MIN_BIT8U, BX_MAX_BIT8U, *ptr_to_real_val)
{
  this->varsize = 8;
  this->lowbit = lowbit;
  this->mask = (1 << (highbit - lowbit)) - 1;
  val.p8bit = (Bit8s*) ptr_to_real_val;
  this->is_shadow = 1;
}

Bit64s
bx_shadow_num_c::get64 () {
  Bit64u current = 0;
  switch (varsize) {
    case 8: current = *(val.p8bit);  break;
    case 16: current = *(val.p16bit);  break;
    case 32: current = *(val.p32bit);  break;
    case 64: current = *(val.p64bit);  break;
    default: BX_PANIC(("unsupported varsize %d", varsize));
  }
  current = (current >> lowbit) & mask;
  if (handler) {
    // the handler can decide what value to return and/or do some side effect
    return (*handler)(this, 0, current) & mask;
  } else {
    // just return the value
    return current;
  }
}

void
bx_shadow_num_c::set (Bit64s newval, bx_bool ignore_handler)
{
  Bit64u tmp = 0;
  if ((newval < min || newval > max) && max != BX_MAX_BIT64U) {
    char ppath[BX_PATHNAME_LEN];
    get_param_path (ppath, BX_PATHNAME_LEN);
    BX_PANIC (("numerical parameter %s was set to %lld, which is out of range %lld to %lld", ppath, newval, min, max));
  }
  switch (varsize) {
    case 8: 
      tmp = (*(val.p8bit) >> lowbit) & mask;
      tmp |= (newval & mask) << lowbit;
      *(val.p8bit) = tmp;
      break;
    case 16:
      tmp = (*(val.p16bit) >> lowbit) & mask;
      tmp |= (newval & mask) << lowbit;
      *(val.p16bit) = tmp;
      break;
    case 32:
      tmp = (*(val.p32bit) >> lowbit) & mask;
      tmp |= (newval & mask) << lowbit;
      *(val.p32bit) = tmp;
      break;
    case 64:
      tmp = (*(val.p64bit) >> lowbit) & mask;
      tmp |= (newval & mask) << lowbit;
      *(val.p64bit) = tmp;
      break;
    default: 
      BX_PANIC(("unsupported varsize %d", varsize));
  }
  if (!ignore_handler && handler) {
    // the handler can override the new value and/or perform some side effect
    (*handler)(this, 1, tmp);
  }
}

void 
bx_shadow_num_c::reset ()
{
  BX_PANIC (("reset not supported on bx_shadow_num_c yet"));
}

bx_param_bool_c::bx_param_bool_c (bx_param_c *parent,
    char *name,
    char *description,
    Bit64s initial_val)
  : bx_param_num_c (parent, name, description, 0, 1, initial_val)
{
  set_type (BXT_PARAM_BOOL);
  set (initial_val);
}

bx_shadow_bool_c::bx_shadow_bool_c (bx_param_c *parent,
      char *name,
      char *description,
      bx_bool *ptr_to_real_val,
      Bit8u bitnum)
  : bx_param_bool_c (parent, name, description, (Bit64s) *ptr_to_real_val)
{
  val.pbool = ptr_to_real_val;
  this->bitnum = bitnum;
  this->is_shadow = 1;
}

Bit64s
bx_shadow_bool_c::get64 () {
  if (handler) {
    // the handler can decide what value to return and/or do some side effect
    Bit64s ret = (*handler)(this, 0, (Bit64s) *(val.pbool));
    return (ret>>bitnum) & 1;
  } else {
    // just return the value
    return (*(val.pbool)) & 1;
  }
}

void
bx_shadow_bool_c::set (Bit64s newval, bx_bool ignore_handler)
{
  // only change the bitnum bit
  Bit64s tmp = (newval&1) << bitnum;
  *(val.pbool) &= ~tmp;
  *(val.pbool) |= tmp;
  if (!ignore_handler && handler) {
    // the handler can override the new value and/or perform some side effect
    (*handler)(this, 1, newval&1);
  }
}

bx_param_enum_c::bx_param_enum_c (bx_param_c *parent,
      char *name,
      char *description,
      char **choices,
      Bit64s initial_val,
      Bit64s value_base)
  : bx_param_num_c (parent, name, description, value_base, BX_MAX_BIT64S, initial_val)
{
  set_type (BXT_PARAM_ENUM);
  this->choices = choices;
  // count number of choices, set max
  char **p = choices;
  while (*p != NULL) p++;
  this->min = value_base;
  // now that the max is known, replace the BX_MAX_BIT64S sent to the parent
  // class constructor with the real max.
  this->max = value_base + (p - choices - 1);
  set (initial_val);
}

int 
bx_param_enum_c::find_by_name (const char *string)
{
  char **p;
  for (p=&choices[0]; *p; p++) {
    if (!strcmp (string, *p))
      return p-choices;
  }
  return -1;
}

bool 
bx_param_enum_c::set_by_name (const char *string, bx_bool ignore_handler)
{
  int n = find_by_name (string);
  if (n<0) return false;
  set (n, ignore_handler);
  return true;
}

bx_param_string_c::bx_param_string_c (bx_param_c *parent,
    char *name,
    char *description,
    char *initial_val,
    int maxsize)
  : bx_param_c (parent, name, description)
{
  set_type (BXT_PARAM_STRING);
  if (maxsize < 0) 
    maxsize = strlen(initial_val) + 1;
  this->val = new char[maxsize];
  this->initial_val = new char[maxsize];
  this->handler = NULL;
  this->maxsize = maxsize;
  strncpy (this->val, initial_val, maxsize);
  strncpy (this->initial_val, initial_val, maxsize);
  this->options = new bx_param_num_c (NULL,
      "stringoptions", NULL, 0, BX_MAX_BIT64S, 0);
  set (initial_val);
}

bx_param_filename_c::bx_param_filename_c (bx_param_c *parent,
    char *name,
    char *description,
    char *initial_val,
    int maxsize)
  : bx_param_string_c (parent, name, description, initial_val, maxsize)
{
  get_options()->set (IS_FILENAME);
}

bx_param_string_c::~bx_param_string_c ()
{
    if ( this->val != NULL )
    {
        delete [] this->val;
        this->val = NULL;
    }
    if ( this->initial_val != NULL )
    {
        delete [] this->initial_val;
        this->initial_val = NULL;
    }

    if ( this->options != NULL )
    {
        delete [] this->options;
        this->options = NULL;
    }
}

void 
bx_param_string_c::reset () {
  strncpy (this->val, this->initial_val, maxsize);
}

void 
bx_param_string_c::set_handler (param_string_event_handler handler)
{
  this->handler = handler; 
  // now that there's a handler, call set once to run the handler immediately
  //set (getptr ());
}

Bit32s
bx_param_string_c::get (char *buf, int len)
{
  if (options->get () & RAW_BYTES)
    memcpy (buf, val, len);
  else
    strncpy (buf, val, len);
  if (handler) {
    // the handler can choose to replace the value in val/len.  Also its
    // return value is passed back as the return value of get.
    (*handler)(this, 0, buf, len);
  }
  return 0;
}

void 
bx_param_string_c::set (char *buf, bx_bool ignore_handler)
{
  if (options->get () & RAW_BYTES)
    memcpy (val, buf, maxsize);
  else
    strncpy (val, buf, maxsize);
  if (!ignore_handler && handler) {
    // the handler can return a different char* to be copied into the value
    buf = (*handler)(this, 1, buf, -1);
  }
}

bx_bool
bx_param_string_c::equals (const char *buf)
{
  if (options->get () & RAW_BYTES)
    return (memcmp (val, buf, maxsize) == 0);
  else
    return (strncmp (val, buf, maxsize) == 0);
}

bx_list_c::bx_list_c (bx_param_c *parent, int maxsize)
  : bx_param_c (parent, "list", "")
{
  set_type (BXT_LIST);
  this->size = 0;
  this->maxsize = maxsize;
  this->list = new bx_param_c*  [maxsize];
  init ();
}

bx_list_c::bx_list_c (bx_param_c *parent, char *name, char *description, 
    int maxsize)
  : bx_param_c (parent, name, description)
{
  set_type (BXT_LIST);
  this->size = 0;
  this->maxsize = maxsize;
  this->list = new bx_param_c*  [maxsize];
  init ();
}

bx_list_c::bx_list_c (bx_param_c *parent, char *name, char *description, bx_param_c **init_list)
  : bx_param_c (parent, name, description)
{
  set_type (BXT_LIST);
  this->size = 0;
  while (init_list[this->size] != NULL)
    this->size++;
  this->maxsize = this->size;
  this->list = new bx_param_c*  [maxsize];
  for (int i=0; i<this->size; i++)
    this->list[i] = init_list[i];
  init ();
}

bx_list_c::~bx_list_c()
{
    if (this->list)
    {
        delete [] this->list;
        this->list = NULL;
    }
    if ( this->title != NULL)
    {
        delete this->title;
        this->title = NULL;
    }
    if (this->options != NULL)
    {
        delete this->options;
        this->options = NULL;
    }
    if ( this->choice != NULL )
    {
        delete this->choice;
        this->choice = NULL;
    }
}

void
bx_list_c::init ()
{
  // the title defaults to the name
  this->title = new bx_param_string_c (NULL,
      "title of list",
      "",
      get_description (), 80);
  this->options = new bx_param_num_c (NULL,
      "list_option", "", 0, BX_MAX_BIT64S,
      0);
  this->choice = new bx_param_num_c (NULL,
      "list_choice", "", 0, BX_MAX_BIT64S,
      1);
}

bx_list_c *
bx_list_c::clone ()
{
  bx_list_c *newlist = new bx_list_c (NULL, name, description, maxsize);
  for (int i=0; i<get_size (); i++)
    newlist->add (get(i));
  newlist->set_options (get_options ());
  return newlist;
}

void
bx_list_c::add (bx_param_c *param)
{
  if (this->size >= this->maxsize) {
    char ppath[BX_PATHNAME_LEN];
    get_param_path (ppath, BX_PATHNAME_LEN);
    BX_PANIC (("add param '%s' to bx_list_c '%s': list capacity exceeded", param->get_name (), ppath));
  }
  list[size] = param;
  size++;
}

bx_param_c *
bx_list_c::get (int index)
{
  BX_ASSERT (index >= 0 && index < size);
  return list[index];
}

bx_param_c *
bx_list_c::get_by_name (const char *name)
{
  int i, imax = get_size ();
  for (i=0; i<imax; i++) {
    bx_param_c *p = get(i);
    if (0 == strcmp (name, p->get_name ())) {
      return p;
    }
  }
  return NULL;
}

void param_print_tree (bx_param_c *node, int level)
{
  int i;
  for (i=0; i<level; i++)
    printf ("  ");
  if (node == NULL) {
      printf ("NULL pointer\n");
      return;
  }
  switch (node->get_type()) {
    case BXT_PARAM_NUM:
      {
	bx_param_num_c *num = (bx_param_num_c *) node;
	int base = num->get_base ();
	BX_ASSERT (base==10 || base==16);
	printf ("%s = ", node->get_name ());
	if (base==10)
	  printf ("%d  (number)\n", num->get ());
	else
	  printf ("0x%x  (number)\n", num->get ());
      }
      break;
    case BXT_PARAM_BOOL:
      printf ("%s = %s  (boolean)\n", node->get_name(), ((bx_param_bool_c*)node)->get()?"true":"false");
      break;
    case BXT_PARAM_STRING:
      printf ("%s = '%s'  (string)\n", node->get_name(), ((bx_param_string_c*)node)->getptr());
      break;
    case BXT_LIST:
      {
	printf ("%s = \n", node->get_name ());
	bx_list_c *list = (bx_list_c*)node;
        for (i=0; i < list->get_size (); i++) {
          // distinguish between real children and 'links' where the 
          // child's parent ptr does not point to this list.  For links,
          // do not descend into the object.
          bx_bool is_link = (list != list->get(i)->get_parent ());
          if (is_link) {
            char ppath[BX_PATHNAME_LEN];
            list->get(i)->get_param_path (ppath, sizeof(ppath));
            for (int indent=0; indent<level+1; indent++) printf ("  ");
            printf ("%s --> link to %s\n", list->get(i)->get_name (), ppath);
	    } else {
              param_print_tree (list->get(i), level+1);
	    }
          }
	break;
      }
    case BXT_PARAM_ENUM:
      {
	bx_param_enum_c *e = (bx_param_enum_c*) node;
	int val = e->get ();
	char *choice = e->get_choice(val);
	printf ("%s = '%s'  (enum)\n", node->get_name (), choice);
      }
      break;
    case BXT_PARAM:
    default:
      printf ("%s = <PRINTING TYPE %d NOT SUPPORTED>\n", node->get_name (), node->get_type ());
  }
}



/*---------------------------------------------------------------------------*/
// FIXME: put these includes in a better place
#include <sys/stat.h> // for mkdir
#include <string.h>   // strcpy, strcat, etc.
#include <stdio.h>    // fopen, fgets, etc.
#include <stdlib.h>

// the following are for fstat()
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
                                                                         
                                                                              
/*---------------------------------------------------------------------------*/
void bx_checkpoint_c::save_param_tree(bx_param_c *node, int level)
{
  // indent 1 character per level of tree depth
  for (int i=0; i<level; i++)
    fprintf (m_ascii_fp, " ");
  if (node == NULL) {
    fprintf (m_ascii_fp, "NULL pointer\n");
    return;
  }

  // dump value based upon the parameter type
  switch (node->get_type()) {
  case BXT_PARAM_NUM:
    {
      // number parameters get dumped as either hex or decimal based upon the 
      // 'base' field of the param
      if (node->is_shadow_param())
        {
          bx_param_num_c *num = (bx_param_num_c *) node;
          int base = num->get_base ();
          BX_ASSERT (base==10 || base==16);
          fprintf (m_ascii_fp, "%s=", node->get_name ());
          if (base==10)
            fprintf (m_ascii_fp, "%d\n", num->get ());
          else
            fprintf (m_ascii_fp, "0x%x\n", num->get ());
          break;
        }
      else
        {
          bx_shadow_num_c *num = (bx_shadow_num_c *) node;
          int base = num->get_base ();
          BX_ASSERT (base==10 || base==16);
          fprintf (m_ascii_fp, "%s=", node->get_name ());
          if (base==10)
            fprintf (m_ascii_fp, "%d\n", num->get ());
          else
            fprintf (m_ascii_fp, "0x%x\n", num->get ());
          break;
        }
    }
  case BXT_PARAM_BOOL:
    {
      // boolean get dumped as either 'true' or 'false'
      fprintf (m_ascii_fp, "%s=%s\n", node->get_name(), 
               ((bx_param_bool_c*)node)->get()?"true":"false");
      break;
    }
  case BXT_PARAM_STRING:
    {
      fprintf (m_ascii_fp, "%s=\"%s\"\n", node->get_name(), 
               ((bx_param_string_c*)node)->getptr());
      break;
    }
  case BXT_PARAM_DATA:
    {
      if (node->is_shadow_param() == 0)
        {
          BX_PANIC(("bx_param_data_c is not implemented!"));
        }
      else 
        {
          Bit8u *data_p = (Bit8u*) ((bx_shadow_data_c*)node)->get();
          int data_size = ((bx_shadow_data_c*)node)->get_data_size();

          fprintf (m_ascii_fp, "%s=<data>%ld %d\n", node->get_name(), 
                   ftell(m_data_fp), data_size);
          
          if (fwrite(data_p, sizeof(Bit8u), data_size, m_data_fp) != data_size)
            {
              BX_PANIC(("data not written to data file in bx_checkpoint_c::save_param_tree()"));          
            }
          
        }
      break;
    }
  case BXT_LIST:
    {
      // lists begin with '{' and end with '}'.  each element is seperated by a 
      // line break.
      fprintf (m_ascii_fp, "%s={\n", node->get_name ());
      bx_list_c *list = (bx_list_c*)node;
      for (int i=0; i < list->get_size (); i++) {
        // distinguish between real children and 'links' where the 
        // child's parent ptr does not point to this list.  For links,
        // do not descend into the object.
        bx_bool is_link = (list != list->get(i)->get_parent ());
        if (is_link) {
          char ppath[BX_PATHNAME_LEN];
          list->get(i)->get_param_path (ppath, sizeof(ppath));
          for (int indent=0; indent<level+1; indent++) fprintf (m_ascii_fp, " ");
          fprintf (m_ascii_fp, "%s=<link>\"%s\"\n", list->get(i)->get_name (), ppath);
        } else {
          save_param_tree (list->get(i), level+1);
        }
      }
      
      // print ending '}' with indenting
      for (int i=0; i<level; i++)
        fprintf (m_ascii_fp, " ");
      fprintf(m_ascii_fp, "}\n");
      break;
    }
  case BXT_PARAM_ENUM:
    {
      bx_param_enum_c *e = (bx_param_enum_c*) node;
      int val = e->get ();
      fprintf (m_ascii_fp, "%s=<enum>0x%x\n", e->get_name (), val); 
      break;
    }
  case BXT_PARAM:
    {
    default:
      fprintf (m_ascii_fp, "%s=<PRINTING TYPE %d NOT SUPPORTED>\n", 
               node->get_name (), node->get_type ());
    }
  }
}


/*---------------------------------------------------------------------------*/
#define CHKPT_PARAM_NOT_FOUND()                                               \
  BX_PANIC(("error when loading checkpoint. "                                 \
          "param \"%s\" not found in list param \"%s\".\n",                   \
          param_str, qualified_path_str));

#define CHKPT_ASSERT_INPUT_TYPE(_param_type, _chk_type)                       \
  if (_param_type != _chk_type) {                                             \
    BX_PANIC(("error when loading checkpoint. "                               \
          "bad bx_objtype found in param \"%s\" not in list param \"%s\".\n", \
          param_str, qualified_path_str));                                    \
  }
  

#define CHKPT_DEBUG 0

#if CHKPT_DEBUG
static int level=0;
#endif

/*---------------------------------------------------------------------------*/
// The following recursive function is a somewhat crude and contstained parser.
// Each level of the param_tree is processed in one instance of this function
// call, and we descend the tree in a breadth first search.    --BJS
void bx_checkpoint_c::load_param_tree(bx_param_c *param_tree_p, 
                                      char *qualified_path_str)
{
  char *param_str = NULL, *value_str = NULL;
  
  BX_ASSERT(qualified_path_str != NULL);
  
  // read each parameter on this level
  while ((param_str = read_next_param()) != NULL)
    {
      // check for end of current level first
      if (strcmp(param_str, "}") == 0)
        {
          return;
        }

      // read the value for this param
      value_str = read_next_value();
      if (value_str == NULL)
        {
          BX_PANIC(("fatal error when loading checkpoint. " \
                   "value not present with param %s.\n", param_str));
        }

      // check for bx_list_c param
      if (strcmp(value_str, "{") == 0)
        {
          load_param_list(param_tree_p, param_str, value_str, 
                          qualified_path_str);
        }
      // check for bx_param_string_c
      else if ((value_str[0] == '\"') && (value_str[strlen(value_str)-1] == '\"')) 
        {
          load_param_string(param_tree_p, param_str, value_str, 
                            qualified_path_str);
        }
      // check for boolean
      else if ((strcmp(value_str, "false") == 0) || strcmp(value_str, "true") == 0)
        {
          load_param_bool(param_tree_p, param_str, value_str, 
                          qualified_path_str);
        }
      // check for hex string
      else if ((value_str[0] == '0') && (value_str[1] == 'x'))
        {
          load_param_hex_num(param_tree_p, param_str, value_str, 
                             qualified_path_str);
        }
      // check for enum
      else if (strncmp(value_str, "<enum>", 6) == 0)
        {
          load_param_enum(param_tree_p, param_str, &(value_str[6]), 
                          qualified_path_str);
        }
      // check for link
      else if (strncmp(value_str, "<link>", 6) == 0)
        {
          // we dont need to read through this link since it is somewhere
          // else in the tree
        }
      // check for data
      else if (strncmp(value_str, "<data>", 6) == 0)
        {
          load_param_data(param_tree_p, param_str, &(value_str[6]), 
                          qualified_path_str);
        }
      // assume that the value is an decimal string
      else
        {
          load_param_dec_num(param_tree_p, param_str, value_str, 
                             qualified_path_str);
        }
    }

  return;
}

/*---------------------------------------------------------------------------*/
int bx_checkpoint_c::write(const char *checkpoint_name, 
                            bx_param_c *param_tree_p)
{
  char *ascii_filename = NULL;
  char *data_filename = NULL;

  // before we open new files using, make sure any old handles are closed
  if (m_ascii_fp) 
    {
      fclose(m_ascii_fp);
      m_ascii_fp = NULL;
    }
  if (m_data_fp) 
    {
      fclose(m_data_fp);
      m_data_fp = NULL;
    }

  // see if checkpoint directory exists
  struct stat mystat;
  if( stat(checkpoint_name, &mystat) >= 0)
    {
      // FIXME:
      // checkpoint file exists ... is it a directory?
      //if (!S_ISDIR(mystat.st_rdev))
      //  {
      //    BX_PANIC(("could no create \"%s\" directory.  file exists.\n",
      //             checkpoint_name));
      //    return 1;
      //  }
      //else 
      //  {
      //    // already exists ... warn user.
      //    BX_INFO(("warning: checkpoint directory \"%s\"exists.\n",
      //            checkpoint_name));
      //  }
    }
  // create the checkpoint directory
  else if (mkdir(checkpoint_name, 0755) <0)
    {
      BX_PANIC(("could not create checkpoint directory \"%s\"\n",
               checkpoint_name));
      return 1;
    }

  // create ascii file name
  ascii_filename = 
    (char*) malloc(strlen(checkpoint_name)+strlen("/param_tree.txt")+1);
  if (ascii_filename)
    {
      strcpy(ascii_filename, checkpoint_name);
      strcat(ascii_filename, "/param_tree.txt");
    }

  // create data file name
  data_filename = 
    (char*) malloc(strlen(checkpoint_name)+strlen("/param_data")+1);
  if (data_filename)
    {
      strcpy(data_filename, checkpoint_name);
      strcat(data_filename, "/param_data");
    }

  if (data_filename == NULL || ascii_filename == NULL)
    {
      BX_PANIC(("system could not service malloc() call\n"));
      return 1;
    }
  // open ascii file for writing
  else if ((m_ascii_fp = fopen(ascii_filename, "w+b")) == NULL)
    {
      BX_PANIC(("could not create/open file \"%s\"\n",
               checkpoint_name));
      return 1;
    }
  // open binary data file for writing
  else if ((m_data_fp = fopen(data_filename, "w+b")) == NULL)
    {
      BX_PANIC(("could not create/open file \"%s\" for writing\n",
               checkpoint_name));
      return 1;
    }
  // if no errors and the files all opened succesfully ...
  else 
    {
      // dump the parameter tree to the ascii and data files
      save_param_tree(param_tree_p, 0);
    }

  // cleanup and return
  if (m_ascii_fp) 
    {
      fclose(m_ascii_fp);
      m_ascii_fp = NULL;
    }
  if (m_data_fp) 
    {
      fclose(m_data_fp);
      m_data_fp = NULL;
    }
  if (ascii_filename)
    free(ascii_filename);
  if (data_filename)
    free(data_filename);

  return 0;
}


/*---------------------------------------------------------------------------*/
int bx_checkpoint_c::read(const char *checkpoint_name, 
                           bx_param_c *param_tree_p)
{
  char *ascii_filename = NULL;
  char *data_filename = NULL;

  // before we open new files using, make sure any old handles are closed
  if (m_ascii_fp) 
    {
      fclose(m_ascii_fp);
      m_ascii_fp = NULL;
    }
  if (m_data_fp) 
    {
      fclose(m_data_fp);
      m_data_fp = NULL;
    }

  // create ascii file name
  ascii_filename = 
    (char*) malloc(strlen(checkpoint_name)+strlen("/param_tree.txt")+1);
  if (ascii_filename)
    {
      strcpy(ascii_filename, checkpoint_name);
      strcat(ascii_filename, "/param_tree.txt");
    }

  // create data file name
  data_filename = 
    (char*) malloc(strlen(checkpoint_name)+strlen("/param_data")+1);
  if (data_filename)
    {
      strcpy(data_filename, checkpoint_name);
      strcat(data_filename, "/param_data");
    }

  // check for malloc() failures
  if (data_filename == NULL || ascii_filename == NULL)
    {
      BX_PANIC(("system could not service malloc() call\n"));
      return 1;
    }
  // open ascii file for reading
  else if ((m_ascii_fp = fopen(ascii_filename, "rb")) == NULL)
    {
      
      BX_PANIC(("could not open file \"%s\" for reading",
               checkpoint_name));
      return 1;
    }
  // open binary data file for reading
  else if ((m_data_fp = fopen(data_filename, "rb")) == NULL)
    {
      BX_PANIC(("could not open file \"%s\" for reading",
               checkpoint_name));
      return 1;
    }
  // if no errors and the files all opened succesfully ...
  else 
    {
      // dump the parameter tree to the ascii and data files
      read_next_param();
      read_next_value();
      // TODO: assert that curly_str is '{'
#if CHKPT_DEBUG
      level = 0; 
#endif
      load_param_tree(param_tree_p, "root");
    }

  // cleanup and return
  if (m_ascii_fp) 
    {
      fclose(m_ascii_fp);
      m_ascii_fp = NULL;
    }
  if (m_data_fp) 
    {
      fclose(m_data_fp);
      m_data_fp = NULL;
    }
  if (ascii_filename)
    free(ascii_filename);
  if (data_filename)
    free(data_filename);

  return 0;
}


/*---------------------------------------------------------------------------*/
bx_checkpoint_c::bx_checkpoint_c()
{
  m_ascii_fp = NULL;
  m_data_fp = NULL;

  m_line_buf_cursor = NULL;

  return;
}


/*---------------------------------------------------------------------------*/
bx_checkpoint_c::~bx_checkpoint_c()
{
  if (m_ascii_fp)
    {
      fclose(m_ascii_fp);
    }
  if (m_data_fp)
    {
      fclose(m_data_fp);
    }

  return;
}


/*---------------------------------------------------------------------------*/
void bx_checkpoint_c::dump_param_tree(bx_param_c *param_tree_p)
{ 
  if (m_ascii_fp) 
    {
      fclose(m_ascii_fp);
      m_ascii_fp = NULL;
    }
  if (m_data_fp) 
    {
      fclose(m_data_fp);
      m_data_fp = NULL;
    }

  m_ascii_fp = stdout;
  save_param_tree(param_tree_p); 
  m_ascii_fp = NULL;

  return;
};


/*---------------------------------------------------------------------------*/
// read the next parameter in the line buffer
char* bx_checkpoint_c::read_next_param()
{
  if (m_ascii_fp == NULL)
    {
      m_line_buf_cursor = NULL;
      return NULL;
    }

  char* line_str = fgets(m_line_buf, MAX_CHECKPOINT_LINE_SIZE, m_ascii_fp);

  if (line_str == NULL)
    {
      m_line_buf_cursor = NULL;
      return NULL;
    }

  int line_size = strlen(m_line_buf);

  // this variable is persistent across both for loops
  int i = 0;

  // advance pointer to first non-white space
  for (i=0; i<line_size; i++)
    {
      if ( !(isspace(m_line_buf[i])) )
        {
          line_str = &(m_line_buf[i]);
          break;
        }
    }

  // look for end of first token (white space or '=' marks end-of-token)
  for (; i<line_size; i++)
    {
      if ( (m_line_buf[i] == '=') || isspace(m_line_buf[i]) )
        {
          // end the string here!
          m_line_buf[i] = '\0';
          break;
        }
    }

  // set the cursor for right after the param string
  m_line_buf_cursor = &(m_line_buf[i+1]);

  return line_str;
}


/*---------------------------------------------------------------------------*/
// read the next parameter value in the line buffer
char* bx_checkpoint_c::read_next_value()
{
  // advance pointer to first non-white space
  char *line_str = m_line_buf_cursor;

  if (line_str == NULL)
    {
      m_line_buf_cursor = NULL;
      return NULL;
    }

  int line_size = strlen(m_line_buf_cursor);

  // this variable is persistent across both for loops
  int i = 0;

  for (i=0; i<line_size; i++)
    {
      if ( !(isspace(m_line_buf_cursor[i])) )
        {
          line_str = &(m_line_buf_cursor[i]);
          break;
        }
    }
  
  int toggle_quote = 0;

  // look for end of first token (white space or '=' marks end-of-token)
  for (; i<line_size; i++)
    {
      if (m_line_buf_cursor[i] == '\"')
        {
          toggle_quote = toggle_quote ? 0 : 1;
        }
      else if ( !toggle_quote && isspace(m_line_buf_cursor[i]) )
        {
          m_line_buf_cursor[i] = '\0';
          break;
        }
    }

  // set the cursor for right after the value string
  m_line_buf_cursor = &(m_line_buf_cursor[i+1]);

  return line_str;
}


/*---------------------------------------------------------------------------*/
bx_bool 
bx_checkpoint_c::load_param_hex_num(bx_param_c *parent_p, 
                                    char *param_str, 
                                    char *value_str,
                                    char *qualified_path_str)
{
#if CHKPT_DEBUG
  for (int i=0; i<level; i++) printf(" ");
  printf("hex param = %s, value = %s\n", param_str, value_str);
#endif // #if CHKPT_DEBUG

  bx_param_c *param_p = parent_p->get_by_name(param_str);
  if (param_p == NULL)
    {
      CHKPT_PARAM_NOT_FOUND();
    }
  else
    {
      CHKPT_ASSERT_INPUT_TYPE(param_p->get_type(), BXT_PARAM_NUM);
                
      // convert the hex string to a long int
      char *str_ptr;
      long int value = strtol(value_str, &str_ptr, 0);
      if (*str_ptr != '\0')
        {
          BX_PANIC(("fatal error when loading checkpoint. " \
                   "value not valid with param %s.\n", param_str));
        }

#if CHKPT_DEBUG
      for (int i=0; i<level; i++) printf(" ");
      printf("            %s, put   = %0xlx\n", param_str, value);
#endif // #if CHKPT_DEBUG

      // actual loading of new value
      if (param_p->is_shadow_param())
        {
          ((bx_shadow_num_c*)param_p)->set(value);
        }
      else
        {
          ((bx_param_num_c*)param_p)->set(value, 1);
        }
    }
          


  return 1;
}

/*---------------------------------------------------------------------------*/
bx_bool 
bx_checkpoint_c::load_param_dec_num(bx_param_c *parent_p, 
                                    char *param_str, 
                                    char *value_str,
                                    char *qualified_path_str)
{
#if CHKPT_DEBUG
  for (int i=0; i<level; i++) printf(" ");
  printf("dec param = %s, value = %s\n", param_str, value_str);
#endif // #if CHKPT_DEBUG

  bx_param_c *param_p = parent_p->get_by_name(param_str);
  if (param_p == NULL)
    {
      CHKPT_PARAM_NOT_FOUND();
    }
  else
    {
      CHKPT_ASSERT_INPUT_TYPE(param_p->get_type(), BXT_PARAM_NUM);
          
      // convert the hex string to a long int
      char *str_ptr;
      long int value = strtol(value_str, &str_ptr, 0);
      if (*str_ptr != '\0')
        {
          BX_PANIC(("error when loading checkpoint. " \
                   "value not valid with param %s.\n", param_str));
        }

#if CHKPT_DEBUG
      for (int i=0; i<level; i++) printf(" ");
      printf("            %s, put   = %ld\n", param_str, value);
#endif // #if CHKPT_DEBUG
      
      // actual loading of new value into location pointed to by shadow
      if (param_p->is_shadow_param())
        {
          ((bx_shadow_num_c*)param_p)->set(value);
        }
      else 
        {
          ((bx_param_num_c*)param_p)->set(value, 1);
        }
      
    }
  return 1;
}

/*---------------------------------------------------------------------------*/
bx_bool 
bx_checkpoint_c::load_param_string(bx_param_c *parent_p, 
                                   char *param_str, 
                                   char *value_str,
                                   char *qualified_path_str)
{
#if CHKPT_DEBUG
  for (int i=0; i<level; i++) printf(" ");
  printf("str param = %s, value = %s\n", param_str, value_str);
#endif // #if CHKPT_DEBUG
          
  bx_param_c *param_p = parent_p->get_by_name(param_str);
  if (param_p == NULL)
    {
      BX_PANIC(("error when loading checkpoint. " \
                "param \"%s\" not found in list param \"%s\".\n", 
                param_str, qualified_path_str));
    }
  else
    {
      CHKPT_ASSERT_INPUT_TYPE(param_p->get_type(), BXT_PARAM_STRING);

      BX_ASSERT(param_p->is_shadow_param() == 0);

      // since the string contains '\"' at the beginning and the end, we
      // need to remove them.
      value_str = &(value_str[1]);
      value_str[strlen(value_str)-1] = '\0';

#if CHKPT_DEBUG
      for (int i=0; i<level; i++) printf(" ");
      printf("            %s, put   = \"%s\"\n", param_str, value_str);
#endif // #if CHKPT_DEBUG

      // actual loading of new value
      ((bx_param_string_c*)param_p)->set(value_str);
    }
          

  return 1;
}

/*---------------------------------------------------------------------------*/
bx_bool 
bx_checkpoint_c::load_param_bool(bx_param_c *parent_p, 
                                 char *param_str, 
                                 char *value_str,
                                 char *qualified_path_str)
{

#if CHKPT_DEBUG
  for (int i=0; i<level; i++) printf(" ");
  printf("boolean param = %s, value = %s\n", param_str, value_str);
#endif // #if CHKPT_DEBUG

  bx_param_c *param_p = parent_p->get_by_name(param_str);
  if (param_p == NULL)
    {
      CHKPT_PARAM_NOT_FOUND();
    }
  else
    {
      CHKPT_ASSERT_INPUT_TYPE(param_p->get_type(), BXT_PARAM_BOOL);
      
      bx_bool value;

      if (strcmp(value_str, "true") == 0)
        {
          value = 1;
        }
      else 
        {
          value = 0;
        }

      // actually set the value
      if (param_p->is_shadow_param())
        {
          ((bx_shadow_bool_c*)param_p)->set(value);
        }
      else
        {
          ((bx_param_bool_c*)param_p)->set(value, 1);
        }
    }

  return 1;
}

/*---------------------------------------------------------------------------*/
bx_bool 
bx_checkpoint_c::load_param_list(bx_param_c *parent_p, 
                                 char *param_str, 
                                 char *value_str,
                                 char *qualified_path_str)
{
  // this param is a bx_list_c, so descend param_tree and recursively
  // call load_param_tree() to process next level

#if CHKPT_DEBUG
  for (int i=0; i<level; i++) printf(" ");
  printf("list param = %s\n", param_str);
#endif // #if CHKPT_DEBUG

  // find the param in the current level of the tree
  bx_param_c *param_p = parent_p->get_by_name(param_str);
  if (param_p == NULL)
    {
      CHKPT_PARAM_NOT_FOUND();
    }
  else
    {
      CHKPT_ASSERT_INPUT_TYPE(param_p->get_type(), BXT_LIST);

      // we have found the param we are looking for.  create a new
      // qualified path string and recurse
      char *new_qual_str =  
        (char*) malloc(strlen(qualified_path_str)+
                       strlen(param_str)+1+1);
      if (new_qual_str == 0)
        {
          BX_PANIC(("fatal error when loading checkpoint. " \
                   "malloc() returned no memory in %s.%s.\n", 
                   qualified_path_str, param_str));
        }
      strcpy(new_qual_str, qualified_path_str);
      strcat(new_qual_str, ".");
      strcat(new_qual_str, param_str);

      // recurse down the tree
#if CHKPT_DEBUG
      level++;
      load_param_tree(param_p, new_qual_str);
      level--;
#else // #if CHKPT_DEBUG
      load_param_tree(param_p, new_qual_str);
#endif // #if CHKPT_DEBUG

      // free string memory
      free(new_qual_str);
    }

  return 1;
}

/*---------------------------------------------------------------------------*/
bx_bool 
bx_checkpoint_c::load_param_enum(bx_param_c *parent_p, 
                                    char *param_str, 
                                    char *value_str,
                                    char *qualified_path_str)
{
#if CHKPT_DEBUG
  for (int i=0; i<level; i++) printf(" ");
  printf("enum param = %s, value = %s\n", param_str, value_str);
#endif // #if CHKPT_DEBUG

  bx_param_c *param_p = ((bx_list_c*)parent_p)->get_by_name(param_str);
  if (param_p == NULL)
    {
      CHKPT_PARAM_NOT_FOUND();
    }
  else
    {
      CHKPT_ASSERT_INPUT_TYPE(param_p->get_type(), BXT_PARAM_ENUM);
          
      // convert the hex string to a long int
      char *str_ptr;
      long int value = strtol(value_str, &str_ptr, 0);

#if CHKPT_DEBUG
      for (int i=0; i<level; i++) printf(" ");
      printf("             %s, put   = %lx\n", param_str, value);
#endif // #if CHKPT_DEBUG

      if (*str_ptr != '\0')
        {
          BX_PANIC(("error when loading checkpoint. " \
                   "value not valid with param %s.\n", param_str));
        }
      
      // actual loading of new value into location pointed to by shadow
      if (param_p->is_shadow_param())
        {
          ((bx_shadow_num_c*)param_p)->set(value);
        }
      else 
        {
          ((bx_param_enum_c*)param_p)->set(value, 1);
        }
      
    }
  return 1;
}


/*---------------------------------------------------------------------------*/
bx_bool 
bx_checkpoint_c::load_param_data(bx_param_c *parent_p, 
                                    char *param_str, 
                                    char *value_str,
                                    char *qualified_path_str)
{
#if CHKPT_DEBUG
  for (int i=0; i<level; i++) printf(" ");
  printf("hex param = %s, value = %s\n", param_str, value_str);
#endif // #if CHKPT_DEBUG

  bx_param_c *param_p = parent_p->get_by_name(param_str);
  if (param_p == NULL)
    {
      CHKPT_PARAM_NOT_FOUND();
    }
  else
    {
      CHKPT_ASSERT_INPUT_TYPE(param_p->get_type(), BXT_PARAM_DATA);
                
      // convert the hex string to a long int
      char *str_ptr;
      long int value = strtol(value_str, &str_ptr, 0);
      if (*str_ptr != '\0')
        {
          BX_PANIC(("fatal error when loading checkpoint. " \
                   "value not valid with param %s.\n", param_str));
        }

#if CHKPT_DEBUG
      for (int i=0; i<level; i++) printf(" ");
      printf("            %s, put   = %0xlx\n", param_str, value);
#endif // #if CHKPT_DEBUG

      // actual loading of new value
      if (param_p->is_shadow_param())
        {
          char *size_str = read_next_value();
          char *str_ptr;
          long int size = strtol(size_str, &str_ptr, 0);
          if (*str_ptr != '\0')
            {
              BX_PANIC(("fatal error when loading checkpoint. " \
                        "value not valid with param %s.\n", param_str));
            }

          Bit8u* new_data_p;
          if (((bx_shadow_data_c*)param_p)->get_data_size() == size)
            {
              new_data_p = (Bit8u*) ((bx_shadow_data_c*)param_p)->get();
            }
          else
            {
              free(((bx_shadow_data_c*)param_p)->get());
              new_data_p = (Bit8u*) malloc(size);
              BX_ASSERT((new_data_p != NULL));
              ((bx_shadow_data_c*)param_p)->set(new_data_p);
              ((bx_shadow_data_c*)param_p)->set_data_size(size);
            }
          
          if (fseek(m_data_fp, size, SEEK_SET) < 0)
            {
              BX_PANIC(("fatal error when loading checkpoint. " \
                        "value not valid with param %s.\n", param_str));
            }

          if (fread(new_data_p, sizeof(Bit8u), size, m_data_fp) != size)
            {
              BX_PANIC(("fatal error when loading checkpoint. " \
                        "value not valid with param %s.\n", param_str));
            }
          
        }
      else
        {
          BX_PANIC(("bx_param_data_c is not implemented!"));          
        }
    }
          
  return 1;
}


/*---------------------------------------------------------------------------*/
bx_shadow_data_c::bx_shadow_data_c (bx_param_c *parent,
                                    char *name,
                                    char *description,
                                    void **ptr_to_real_ptr, 
                                    int data_size)
  : bx_param_c(parent, name, description)
{  
  this->data = ptr_to_real_ptr;
  this->data_size = data_size;
  this->set_type(BXT_PARAM_DATA);
  this->is_shadow = 1;
}

/*---------------------------------------------------------------------------*/
void
bx_shadow_data_c::set (void* new_data_ptr, bx_bool ignore_handler=0)
{
  // BJS TODO: implement handlers
  (*data) = new_data_ptr;
}

void*
bx_shadow_data_c::get()
{
  return (*data);
}

////////////////////////////////////////////////////////////////////////
// finding parameters by name
////////////////////////////////////////////////////////////////////////

// recursive function to find parameters from the path
static
bx_param_c *param_get2 (const char *full_ppath, const char *rest_of_ppath, bx_param_c *base)
{
  const char *from = rest_of_ppath;
  char component[BX_PATHNAME_LEN];
  char *to = component;
  // copy the first piece of ppath into component, stopping at first separator
  // or at the end of the string
  while (*from != 0 && *from != '.') {
    *to = *from;
    to++;
    from++;
  }
  *to = 0;
  if (!component[0]) {
    BX_PANIC (("param_get2: found empty component in parameter name %s", full_ppath));
    // or does that mean that we're done?
  }
  if (base->get_type() != BXT_LIST) {
    BX_PANIC (("param_get2: base was not a list!"));
  }
  BX_INFO (("searching for component '%s' in list '%s'", component, base->get_name()));

  // find the component in the list.
  bx_list_c *list = (bx_list_c *)base;
  bx_param_c *child = list->get_by_name (component);
  // if child not found, there is nothing else that can be done. return NULL.
  if (child == NULL) return NULL;
  if (from[0] == 0) {
    // that was the end of the path, we're done
    return child;
  }
  // continue parsing the path
  BX_ASSERT(from[0] == '.');
  from++;  // skip over the separator
  return param_get2 (full_ppath, from, child);
}


bx_param_c *
param_get(const char *ppath, bx_param_c *base) 
{
  if (base == NULL)
    base = bx_param_get_root ();
  // to access top level object, look for parameter "."
  if (ppath[0] == '.' && ppath[1] == 0)
    return base;
  return param_get2 (ppath, ppath, base);
}
