/////////////////////////////////////////////////////////////////////////
// $Id: testparam.cc,v 1.1.2.4 2003/05/03 15:58:45 bdenney Exp $
// bx_param's can now be compiled separately from Bochs
// This demonstrates parameter registration and save/restore on a very small
// scale.
/////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "param.h"

#define CHECKPOINT_NAME "chkpt"

bx_param_c *root = NULL;

/*
Here is the parameter tree:
foo
  enable
  status
  irq
bar
  enable
  status
  irq
baz
  enable
  status
  irq
*/

void register_param1 ()
{
  // these parameter constructors show one way of creating the desired
  // parameter structure.  This uses normal params, not shadow params.
  bx_list_c *foo = new bx_list_c (root, "foo", "device called foo");
  bx_list_c *bar = new bx_list_c (root, "bar", "device called bar");
  bx_list_c *baz = new bx_list_c (root, "baz", "device called baz");
  new bx_param_num_c (foo, "enable", "foo enable", 0, 1, 0);
  new bx_param_num_c (foo, "status", "foo status", 0, 1, 0);
  new bx_param_num_c (foo, "irq", "foo irq", 0, 10, 0);
  new bx_param_num_c (bar, "enable", "bar enable", 0, 1, 0);
  new bx_param_num_c (bar, "status", "bar status", 0, 1, 0);
  new bx_param_num_c (bar, "irq", "bar irq", 0, 10, 0);
  new bx_param_num_c (baz, "enable", "baz enable", 0, 1, 0);
  new bx_param_num_c (baz, "status", "baz status", 0, 1, 0);
  new bx_param_num_c (baz, "irq", "baz irq", 0, 10, 0);
}

struct device {
  Bit32u enable;
  Bit32u status;
  Bit32u irq;
};
device *foo = new device();
device *bar = new device();
device *baz = new device();

void register_param2 ()
{
  // this function registers shadow parameters that point to the 
  // three device structures, using Brian's BXRS macros.
  bx_list_c *foo_list_p = new bx_list_c (root, "foo", "foo device");
  bx_list_c *bar_list_p = new bx_list_c (root, "bar", "bar device");
  bx_list_c *baz_list_p = new bx_list_c (root, "baz", "baz device");
  BXRS_START(device, foo, "foo device", foo_list_p, 5);
  {
    BXRS_NUM (Bit32u, enable);
    BXRS_NUM (Bit32u, status);
    BXRS_NUM (Bit32u, irq);
  }
  BXRS_END;
  BXRS_START(device, bar, "bar device", bar_list_p, 5);
  {
    BXRS_NUM (Bit32u, enable);
    BXRS_NUM (Bit32u, status);
    BXRS_NUM (Bit32u, irq);
  }
  BXRS_END;
  BXRS_START(device, baz, "baz device", baz_list_p, 5);
  {
    BXRS_NUM (Bit32u, enable);
    BXRS_NUM (Bit32u, status);
    BXRS_NUM (Bit32u, irq);
  }
  BXRS_END;
}

void edit_params ()
{
  while (1) {
    printf ("These are the current values:\n");
    param_print_tree (root);
    printf ("Enter parameter name to edit (example: foo.status): ");
    char ppath[512], buf[512];
    gets(ppath);
    if (ppath[0] == 0) return;
    bx_param_num_c *param = (bx_param_num_c *) param_get (ppath, root);
    if (param == NULL) {
      BX_ERROR (("Not found '%s'", ppath));
      continue;
    }
    printf ("%s values must be between %d and %d\n", 
	param->get_name(),
	(int)param->get_min(),
	(int)param->get_max());
    printf ("Old value: %d\n", (int)param->get ());
    printf ("New value: ");
    gets (buf);
    int n;
    if (sscanf (buf, "%d", &n) != 1) {
      BX_ERROR (("That was not an integer."));
      continue;
    }
    param->set (n);
    return;
  }
}

int load_params ()
{
  bx_checkpoint_c checkpt;
  return checkpt.read (CHECKPOINT_NAME, root);
}

int save_params ()
{
  bx_checkpoint_c checkpt;
  return checkpt.write (CHECKPOINT_NAME, root);
}

void menu ()
{
  while (1) {
    printf ("--------------\n");
    param_print_tree (root);
    printf ("--------------\n");
    printf ("1. Load from checkpoint\n");
    printf ("2. Save checkpoint\n");
    printf ("3. Edit values\n");
    printf ("4. Quit\n");
    printf ("--> ");
    int choice = -1;
    char buf[512];
    gets (buf);
    sscanf (buf, "%d", &choice);
    switch (choice) {
      case 1:
	load_params();
	break;
      case 2:
	save_params();
	break;
      case 3:
	edit_params();
	break;
      case 4:
	return;
      default:
	printf ("Huh?\n");
    }
  }
}

void test_shadow_range ()
{
#define TYPE bx_bool
#define TYPE_NAME "Bit8u"
  TYPE byte;
  int n;
  bx_shadow_num_c *shadow;
  int highbit, lowbit;
  printf ("Enter highbit: ");
  scanf ("%d", &highbit);
  printf ("Enter lowbit: ");
  scanf ("%d", &lowbit);
  printf ("Enter initial value: ");
  scanf ("%d", &n);
  byte = (TYPE) n;
  shadow = new bx_shadow_num_c (NULL, 
      "8bit", "shadow byte",
      &byte,
      highbit,
      lowbit);
  printf ("Testing a %s\n", TYPE_NAME);
  while (1) {
    int input;
    printf ("Raw byte is %d. Param is %d. Enter a number: ", (int)byte, (int)shadow->get ());
    scanf ("%d", &input);
    printf ("Setting param to %d\n", input);
    shadow->set ((TYPE) input);
    int output = shadow->get ();
    printf ("Now param is %d\n", output);
    if (input != output) {
      printf ("Mismatch!!\n");
    }
  }
}

void test_bool()
{
  bx_bool actual = 0;
  bx_param_bool_c *param;
  param = new bx_shadow_bool_c (NULL, "name", "desc", &actual, 0);
  int n;
  while (1) {
    printf ("param is %d\n", (int)param->get ());
    printf ("enter new: ");
    scanf ("%d", &n);
    param->set (n);
  }
}

int main ()
{
  bx_init_param ();
  //test_bool();
  //return 0;
  //test_shadow_range ();
  root = bx_param_get_root ();
  register_param1 ();
  menu ();
  return 0;
}
