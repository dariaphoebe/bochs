/*
 * misc/bximage.c
 * $Id: bxuncompress.c,v 1.1.2.2 2004/05/31 19:29:29 cbothamy Exp $
 *
 * Uncompresses a compressed disk image to a flat disk image for bochs
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#ifdef WIN32
#  include <conio.h>
#ifndef __MINGW32__
#  define snprintf _snprintf
#endif
#endif
#include "config.h"

#if BX_HAVE_ZLIB != 1
#  error Zlib not available
#else
#  include <zlib.h>
#endif

#include <string.h>

#define uint8   Bit8u
#define uint16  Bit16u
#define uint32  Bit32u

#define INCLUDE_ONLY_HD_HEADERS 1
#include "../iodev/harddrv.h"

char *EOF_ERR = "ERROR: End of input";
char *rcsid = "$Id: bxuncompress.c,v 1.1.2.2 2004/05/31 19:29:29 cbothamy Exp $";
char *divider = "========================================================================";

void myexit (int code)
{
#ifdef WIN32
  printf ("\nPress any key to continue\n");
  getch();
#endif
  exit(code);
}

/* stolen from main.cc */
void bx_center_print (FILE *file, char *line, int maxwidth)
{
  int imax;
  int i;
  imax = (maxwidth - strlen(line)) >> 1;
  for (i=0; i<imax; i++) fputc (' ', file);
  fputs (line, file);
}

void
print_banner ()
{
  printf ("%s\n", divider);
  bx_center_print (stdout, "bxuncompress\n", 72);
  bx_center_print (stdout, "Uncompress Disk Image Tool for Bochs\n", 72);
  bx_center_print (stdout, rcsid, 72);
  printf ("\n%s\n", divider);
}

/* this is how we crash */
void fatal (char *c)
{
  printf ("%s\n", c);
  myexit (1);
}

/* remove leading spaces, newline junk at end.  returns pointer to 
 cleaned string, which is between s0 and the null */
char *
clean_string (char *s0)
{
  char *s = s0;
  char *ptr;
  /* find first nonblank */
  while (isspace (*s))
    s++;
  /* truncate string at first non-alphanumeric */
  ptr = s;
  while (isprint (*ptr))
    ptr++;
  *ptr = 0;
  return s;
}

/* returns 0 on success, -1 on failure.  The value goes into out. */
int 
ask_int (char *prompt, int min, int max, int the_default, int *out)
{
  int n = max + 1;
  char buffer[1024];
  char *clean;
  int illegal;
  while (1) {
    printf ("%s", prompt);
    printf ("[%d] ", the_default);
    if (!fgets (buffer, sizeof(buffer), stdin))
      return -1;
    clean = clean_string (buffer);
    if (strlen(clean) < 1) {
      // empty line, use the default
      *out = the_default;
      return 0;
    }
    illegal = (1 != sscanf (buffer, "%d", &n));
    if (illegal || n<min || n>max) {
      printf ("Your choice (%s) was not an integer between %d and %d.\n\n",
	  clean, min, max);
    } else {
      // choice is okay
      *out = n;
      return 0;
    }
  }
}

int 
ask_menu (char *prompt, int n_choices, char *choice[], int the_default, int *out)
{
  char buffer[1024];
  char *clean;
  int i;
  *out = -1;
  while (1) {
    printf ("%s", prompt);
    printf ("[%s] ", choice[the_default]);
    if (!fgets (buffer, sizeof(buffer), stdin))
      return -1;
    clean = clean_string (buffer);
    if (strlen(clean) < 1) {
      // empty line, use the default
      *out = the_default;
      return 0;
    }
    for (i=0; i<n_choices; i++) {
      if (!strcmp (choice[i], clean)) {
	// matched, return the choice number
	*out = i;
	return 0;
      }
    }
    printf ("Your choice (%s) did not match any of the choices:\n", clean);
    for (i=0; i<n_choices; i++) {
      if (i>0) printf (", ");
      printf ("%s", choice[i]);
    }
    printf ("\n");
  }
}

int 
ask_yn (char *prompt, int the_default, int *out)
{
  char buffer[16];
  char *clean;
  *out = -1;
  while (1) {
    printf ("%s", prompt);
    printf ("[%s] ", the_default?"yes":"no");
    if (!fgets (buffer, sizeof(buffer), stdin))
      return -1;
    clean = clean_string (buffer);
    if (strlen(clean) < 1) {
      // empty line, use the default
      *out = the_default;
      return 0;
    }
    switch (tolower(clean[0])) {
      case 'y': *out=1; return 0;
      case 'n': *out=0; return 0;
    }
    printf ("Please type either yes or no.\n");
  }
}

int 
ask_string (char *prompt, char *the_default, char *out)
{
  char buffer[1024];
  char *clean;
  out[0] = 0;
  printf ("%s", prompt);
  printf ("[%s] ", the_default);
  if (!fgets (buffer, sizeof(buffer), stdin))
    return -1;
  clean = clean_string (buffer);
  if (strlen(clean) < 1) {
    // empty line, use the default
    strcpy (out, the_default);
    return 0;
  }
  strcpy (out, clean);
  return 0;
}


#define DISK_IMAGE_UNKNOWN     (0)
#define DISK_IMAGE_FLAT        (1)
#define DISK_IMAGE_COMPRESSED  (2)

/* Check for image type */
int lookup_image_type (char *filename)
{
  int fd;
  compressed_header_t header;
 
  // check if file exists
  fd = open (filename, O_RDONLY
#ifdef O_BINARY
                         | O_BINARY
#endif
                  );
  if (fd<0) {
     fatal ("ERROR: file not found\n");
  }

  if (read(fd, &header, BX_HD_STANDARD_HEADER_SIZE) != BX_HD_STANDARD_HEADER_SIZE)
     return DISK_IMAGE_UNKNOWN;
  close(fd);

  if (strcmp(header.generic.magic, BX_HD_STANDARD_HEADER_MAGIC) != 0)
     return DISK_IMAGE_FLAT;

  if ((strcmp(header.generic.type, BX_HD_COMPRESSED_TYPE) == 0)
    &&(strcmp(header.generic.subtype, BX_HD_COMPRESSED_SUBTYPE_ZLIB) == 0))
    return DISK_IMAGE_COMPRESSED;

  return DISK_IMAGE_UNKNOWN;
}

/* Compress a flat file in a compress file */
int uncompress_image (char *flat_filename, char *compressed_filename)
{
        struct stat statbuf;
        compressed_header_t header;
        FILE *zfd,*ffd;
        char *fbuffer, *zbuffer;
        Bit32u extentCount, extentTotal;
        Bit32u extentSize, bufSize, dataSize, catalogSize;
        unsigned long zLen, fLen;
        Bit32u zBlocks, fBlocks;
        Bit64u catalog, diskSize, remainingSize;
        Bit64u extentPosition;

        memset(&header, 0, sizeof(header));

        // open compressed file
        zfd = fopen (compressed_filename, "rb");
        if (zfd==NULL) {
            fatal ("ERROR: can't open compressed file\n");
        }

        // check if file exists
        ffd = fopen (flat_filename, "rb");
        if (ffd!=NULL) {
            fatal ("ERROR: flat file file already exists\n");
        }

        // open flat file
        ffd = fopen (flat_filename, "wb");
        if (ffd==NULL) {
            fatal ("ERROR: can't open flat file for writing\n");
        }

        printf ("\nReading compressed header: [");
        // Write header
        if (fread(&header, sizeof(header), 1, zfd) != 1)
        {
                fclose (zfd);
                fclose (ffd);
                fatal ("ERROR: could not read header!\n");
        }

        printf("Type='%s', Subtype='%s', Version=%d.%d, ", 
           header.generic.type, header.generic.subtype,
           dtoh32(header.generic.version)/0x10000, 
           dtoh32(header.generic.version)%0x10000);
        printf("#entries=%d, exent size = %d] Done.",
           dtoh32(header.specific.catalog),
           dtoh32(header.specific.extent));
        
        diskSize = dtoh64(header.specific.disk);
        extentSize = dtoh32(header.specific.extent);
        catalogSize =  dtoh32(header.specific.catalog) * sizeof(Bit64u);
        printf ("\nChecking disk size: [%llu bytes]",diskSize);
        
        // Estimate number of extents, no need to be perfectly exact
        extentTotal = ((diskSize) / extentSize) + 1;
                
        extentCount = 0;
        remainingSize = diskSize;

        // allocate buffers and set size
        bufSize = extentSize;
        if (((fbuffer = (char*)malloc(bufSize)) == NULL)
         || ((zbuffer = (char*)malloc(bufSize)) == NULL)) {
                fclose (zfd);
                fclose (ffd);
                fatal ("ERROR: can't allocate buffers!\n");
        }
        
        printf ("\nWriting flat disk image: [  0%%]");
        while (remainingSize>0) {
                printf("\x8\x8\x8\x8\x8%3d%%]", (extentCount+1)*100/extentTotal);

                // Read catalog for current extent
                fseek(zfd, BX_HD_STANDARD_HEADER_SIZE + (extentCount * sizeof(Bit64u)), SEEK_SET);
                fread(&catalog, sizeof(Bit64u), 1, zfd);
                catalog = dtoh64(catalog);

                if(BX_HD_COMPRESSED_EXTENT_IS_ALLOCATED(catalog)) {
                        // Extent is allocated, we have to read data
                        extentPosition = BX_HD_COMPRESSED_EXTENT_POSITION(catalog);
                        zBlocks = BX_HD_COMPRESSED_EXTENT_SIZE(catalog);
                        fBlocks = extentSize / 512;

                        // read n 512-bytes data
                        fseek(zfd, BX_HD_STANDARD_HEADER_SIZE + catalogSize + (extentPosition * 512), SEEK_SET);
                        fread(zbuffer, 512, zBlocks, zfd);

                        if(zBlocks < fBlocks) {
                                int result;
                                // data is in compressed form
                                fLen = fBlocks * 512;
                                zLen = zBlocks * 512;
                                result = uncompress (fbuffer, &fLen, zbuffer, zLen); 
                                if (result != Z_OK) {
                                        printf("error %d.",result);
                                        fatal ("ERROR: while decompressing!\n");
                                }
                        }
                        else {
                                // data is stored uncompressed
                                // extentSize == zBlocks * 512
                                // Seek to position in the flat file
                                memcpy(fbuffer, zbuffer, bufSize);
                        }
                }
                else  {
                        // Extent is not allocated, we have to write a zeroed buffer
                        memset(fbuffer, 0, bufSize);
                }

                // If at end of disk
                dataSize = extentSize;
                if (remainingSize < (Bit64u)dataSize)
                        dataSize = (Bit32u)remainingSize;
                remainingSize -= (Bit64u)dataSize;

                // Seek to position in the flat file
                fseek(ffd, extentCount * extentSize, SEEK_SET);
                fwrite(fbuffer, dataSize, 1, ffd);

                extentCount++;
        }

        printf(" Done. Wrote %lld bytes\n", diskSize);

        fclose (zfd);
        fclose (ffd);
        free(fbuffer);
        free(zbuffer);

        return (0);
}

int main()
{
  char flat_filename[256];
  char compressed_filename[256];
  int result, remove;

  print_banner ();
  flat_filename[0] = 0;
  compressed_filename[0] = 0;

  if (ask_string ("\nWhat is the compressed image name?\n", "c.compressed.img", compressed_filename) < 0)
    fatal (EOF_ERR);

  result = lookup_image_type(compressed_filename);

  if (result == DISK_IMAGE_COMPRESSED) {

          if (ask_string ("\nWhat is the flat image name?\n", "c.img", flat_filename) < 0)
                fatal (EOF_ERR);

          if (ask_yn ("\nShall I remove the compressed image after compression ?\n", 0, &remove) < 0)
                fatal (EOF_ERR);

          if (uncompress_image(flat_filename, compressed_filename) < 0)
                fatal ("Error while compressing flat image !\n");

          if (remove) {
            if (unlink(compressed_filename) != 0)
               fatal ("ERROR: while removing the compressed disk image !\n");
          }
  }
  else 
       fatal ("ERROR: This is not a compressed diskk image !\n");


  // make picky compilers (c++, gcc) happy,
  // even though we leave via 'myexit' just above
  return 0;
}
