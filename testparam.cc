/////////////////////////////////////////////////////////////////////////
// $Id: testparam.cc,v 1.1.2.1 2003/03/30 07:53:53 bdenney Exp $
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
foo_device
  enable
  status
  irq
bar_device
  enable
  status
  irq
baz_device
  enable
  status
  irq
*/

void register_params ()
{
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

int main ()
{
  bx_init_param ();
  root = bx_param_get_root ();
  register_params ();
  menu ();
  return 0;
}
