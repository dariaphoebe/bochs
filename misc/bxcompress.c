/*
 * misc/bximage.c
 * $Id: bxcompress.c,v 1.1.2.2 2004/05/18 20:35:15 cbothamy Exp $
 *
 * Compresses a flat disk image to a compressed disk image for bochs
 * Also reclaims space unused in a compressed disk image for bochs
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
char *rcsid = "$Id: bxcompress.c,v 1.1.2.2 2004/05/18 20:35:15 cbothamy Exp $";
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
  bx_center_print (stdout, "bxcompress\n", 72);
  bx_center_print (stdout, "Compressed Disk Image Maintenance Tool for Bochs\n", 72);
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


/* Create a suited redolog header */
void make_compressed_header(compressed_header_t *header, const char* type, Bit64u size)
{
        Bit32u entries, extent_size;
        Bit64u maxsize;

        // Set generic header values
        strcpy((char*)header->generic.magic, STANDARD_HEADER_MAGIC);
        strcpy((char*)header->generic.type, COMPRESSED_TYPE);
        strcpy((char*)header->generic.subtype, type);
        header->generic.version = htod32(STANDARD_HEADER_VERSION);
        header->generic.header = htod32(STANDARD_HEADER_SIZE);

        entries = 4 * 1024;
        extent_size = 16 * 512; // bytes

        // Compute #entries and extent size values
        do {
                header->specific.catalog = htod32(entries);
                header->specific.extent = htod32(extent_size);
                
                maxsize = (Bit64u)entries * (Bit64u)extent_size;

                // limit to 64k entries
                if (entries < (64 * 1024)) {
                        entries *= 2;
                }
                else { // then increase extent size
                        extent_size *= 2;

                        if (extent_size > 0x10000)
                                fatal("Disk size to big!\n");
                }
        } while (maxsize < size);

        header->specific.disk = htod64(size);
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

  if (read(fd, &header, STANDARD_HEADER_SIZE) != STANDARD_HEADER_SIZE)
     return DISK_IMAGE_UNKNOWN;
  close(fd);

  if (strcmp(header.generic.magic, STANDARD_HEADER_MAGIC) != 0)
     return DISK_IMAGE_FLAT;

  if ((strcmp(header.generic.type, COMPRESSED_TYPE) == 0)
    &&(strcmp(header.generic.subtype, COMPRESSED_SUBTYPE_ZLIB) == 0))
    return DISK_IMAGE_COMPRESSED;

  return DISK_IMAGE_UNKNOWN;
}

/* Compress a flat file in a compress file */
int compress_flat_image (char *flat_filename, char *compressed_filename)
{
        struct stat statbuf;
        compressed_header_t header;
        FILE *zfd,*ffd;
        char *fbuffer, *zbuffer;
        Bit32u i, count, zBufSize, fBufSize;
        unsigned long zLen, fLen;
        Bit32u zBlocks, fBlocks, oBlocks;
        Bit32u extentCount, extentTotal;
        fpos_t lastPos;
        Bit64u notAllocated = htod64(COMPRESSED_EXTENT_NOT_ALLOCATED);
        Bit64u catalog, extentPosition, inCount, outCount;

        if (stat(flat_filename, &statbuf)<0) {
                fatal ("ERROR: flat file not found\n");
        }

        // make header
        memset(&header, 0, sizeof(header));
        make_compressed_header(&header, COMPRESSED_SUBTYPE_ZLIB, statbuf.st_size);

        // Estimate number of extents, no need to be perfectly exact
        extentTotal = ((statbuf.st_size) / dtoh32(header.specific.extent)) + 1;
        
        // open flat file
        ffd = fopen (flat_filename, "rb");
        if (ffd==NULL) {
            fatal ("ERROR: flat file not found\n");
        }

        // check if file exists
        zfd = fopen (compressed_filename, "rb");
        if (zfd!=NULL) {
            fclose(zfd);
            fatal ("ERROR: compressed file already exists\n");
        }

        // open compressed file for writing
        zfd = fopen (compressed_filename, "wb");
        if (zfd==NULL) {
            fatal ("ERROR: can't open compressed file for writing\n");
        }

        printf ("\nWriting compressed header: [");

        // Write header
        if (fwrite(&header, sizeof(header), 1, zfd) != 1)
        {
                fclose (zfd);
                fclose (ffd);
                fatal ("ERROR: The disk image is not complete - could not write header!\n");
        }

        printf("Type='%s', Subtype='%s', Version=%d.%d, ", 
           header.generic.type, header.generic.subtype,
           dtoh32(header.generic.version)/0x10000, 
           dtoh32(header.generic.version)%0x10000);
        printf("#entries=%d, exent size = %d] Done.",
           dtoh32(header.specific.catalog),
           dtoh32(header.specific.extent));
        
        // Write catalog
        printf ("\nWriting catalog: [");
        for (i=0; i<dtoh32(header.specific.catalog); i++)
        {
                if (fwrite(&notAllocated, sizeof(Bit64u), 1, zfd) != 1)
                {
                        fclose (zfd);
                        fclose (ffd);
                        fatal ("ERROR: The disk image is not complete - could not write catalog!\n");
                }
        }
        printf("%d entries] Done.", dtoh32(header.specific.catalog));

        extentCount = 0;
        extentPosition = 0;

        // to compute compression ratio
        inCount = 0;
        outCount = 0;

        // Remember where to start writing blocks
        fgetpos(zfd, &lastPos);

        printf ("\nCompressing flat file: [  0%%]");

        // Compute sizes and allocate flat and copressed buffer
        fBufSize = dtoh32(header.specific.extent);
        zBufSize = (int)(fBufSize * 1.001) + 13; //  Overestimate by one byte zlib spec
        if ((fbuffer = (char*)malloc(fBufSize)) == NULL) {
                fatal ("ERROR: can't malloc fbuffer!\n");
        }
        if ((zbuffer = (char*)malloc(zBufSize)) == NULL) {
                fatal ("ERROR: can't malloc zbuffer!\n");
        }

        while (!feof(ffd)) {
                printf("\x8\x8\x8\x8\x8%3d%%]", (extentCount+1)*100/extentTotal);
                fflush(stdout);

                fLen = fBufSize;
                zLen = zBufSize;

                memset(fbuffer, 0, fBufSize);
                memset(zbuffer, 0, zBufSize);

                // Read extent sized flat disk buffer
                count = fread(fbuffer, 1, fLen, ffd);
                if (count == 0) continue;

                inCount += count;

                // Check if buffer is filled with 0s
                if (memcmp(fbuffer, zbuffer, count)==0) {
                        // If so, no need to compress it,
                        // just continue with next extent.
                        extentCount++;
                        continue;
                }

                // Here we must compress fLen bytes, so the last extent 
                // will be decompressed to a "fLen" length buffer.
                if (compress2 (zbuffer, &zLen, fbuffer, fLen, 6) != Z_OK)
                        fatal ("ERROR: while compressing!\n");

                // Compute flat and compressed sizes in 512-bytes blocks
                zBlocks = (((zLen==0?1:zLen)-1) / 512)+1;
                fBlocks = fLen / 512;

                // Did we gain something ?
                if (zBlocks < fBlocks) {
                        // Compress gained space, writing compressed form
                        fsetpos(zfd, &lastPos);
                        oBlocks = zBlocks;
                        fwrite(zbuffer, 512, oBlocks, zfd);
                        fgetpos(zfd, &lastPos);
                }
                else {
                        // Compress did not gain space, writing as is
                        fsetpos(zfd, &lastPos);
                        oBlocks = fBlocks;
                        fwrite(fbuffer, 512, oBlocks, zfd);
                        fgetpos(zfd, &lastPos);
                }

                fseek(zfd, STANDARD_HEADER_SIZE + (extentCount * sizeof(Bit64u)), SEEK_SET);
                catalog = htod64(COMPRESSED_EXTENT_CATALOG(extentPosition, oBlocks));
                fwrite(&catalog, sizeof(Bit64u), 1, zfd);

                extentPosition += (Bit64u)oBlocks;
                outCount += (oBlocks * 512);
                extentCount ++;
        }
        printf(" Done. Compression ratio is %.2f%% of original size\n",100.0*(float)(outCount)/(float)(inCount));

        fclose (zfd);
        fclose (ffd);
        free(fbuffer);
        free(zbuffer);

        return (0);
}

/* Reclaim lost space */
int recompress_image (char *filename)
{
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

  if (ask_string ("\nWhat is the image name?\n", "c.img", flat_filename) < 0)
    fatal (EOF_ERR);

  result = lookup_image_type(flat_filename);

  if (result == DISK_IMAGE_COMPRESSED) {
          strcpy(compressed_filename, flat_filename);
          printf("%s is a compressed disk image.\n", compressed_filename);

          if (recompress_image(compressed_filename) < 0)
                fatal ("Error while compressing flat image !\n");

  }
  else if (result == DISK_IMAGE_FLAT) {
          printf("%s seems to be a flat disk image.\n", flat_filename);
          if (ask_string ("\nWhat is the compressed image name?\n", "c.compressed.img", compressed_filename) < 0)
                fatal (EOF_ERR);

          if (ask_yn ("\nShall I remove the flat image after compression ?\n", 0, &remove) < 0)
                fatal (EOF_ERR);

          if (compress_flat_image(flat_filename, compressed_filename) < 0)
                fatal ("Error while compressing flat image !\n");

          if (remove) {
            if (unlink(flat_filename) != 0)
               fatal ("ERROR: while removing the flat disk image !\n");
          }
  }
  else 
       fatal ("ERROR: could not determine disk image type !\n");


  // make picky compilers (c++, gcc) happy,
  // even though we leave via 'myexit' just above
  return 0;
}
