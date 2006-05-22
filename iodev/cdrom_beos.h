/////////////////////////////////////////////////////////////////////////
// $Id: cdrom_beos.h,v 1.1.44.1 2006/05/22 17:09:50 vruppert Exp $
/////////////////////////////////////////////////////////////////////////

#ifndef CDROM_BEOS_H
#define CDROM_BEOS_H

#include <stddef.h> //for off_t

off_t GetNumDeviceBlocks(int fd, int block_size);
int GetLogicalBlockSize(int fd);
int GetDeviceBlockSize(int fd);

#endif
