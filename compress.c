/* Compression/decompression routines */

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#ifdef HAVE_BZLIB_H
# include <bzlib.h>
#else
# ifdef SHIPPED_LIBBZ2
#  include SHIPPED_LIBBZ2_H
# endif
#endif
#ifdef HAVE_ZLIB_H
# include <zlib.h>
#else
# ifdef SHIPPED_LIBZ
#  include SHIPPED_LIBZ_H
# endif
#endif
#include "stream.h"
#include "utils.h"
#include "gettext.h"

#define ERRORMSG(x) write(fd,(x),strlen(x))

#define BUFFER_SIZE 32768

#if (defined HAVE_LIBBZ2) || (defined SHIPPED_LIBBZ2)
static void read_bz2(FILE *f, int fd)
{
    BZFILE* b;
    int     nBuf;
    char    buf[BUFFER_SIZE];
    int     bzerror;

    b = BZ2_bzReadOpen ( &bzerror, f, 0, 0, NULL, 0 );
    if (bzerror != BZ_OK) {
       BZ2_bzReadClose ( &bzerror, b );
       /* error */
       ERRORMSG(_("Invalid/corrupt .bz2 file.\n"));
       return;
    }

    bzerror = BZ_OK;
    while (bzerror == BZ_OK) {
       nBuf = BZ2_bzRead ( &bzerror, b, buf, BUFFER_SIZE);
       if (write(fd, buf, nBuf)!=nBuf)
           return;
    }
    if (bzerror != BZ_STREAM_END) {
       BZ2_bzReadClose ( &bzerror, b );
       /* error */
       ERRORMSG("\e[0m");
       ERRORMSG(_("bzip2: Error during decompression.\n"));
    } else {
       BZ2_bzReadClose ( &bzerror, b );
    }
}

static void write_bz2(FILE *f, int fd)
{
    BZFILE* b;
    int     nBuf;
    char    buf[BUFFER_SIZE];
    int     bzerror;

    b = BZ2_bzWriteOpen ( &bzerror, f, 9, 0, 0 );
    if (bzerror != BZ_OK) {
       BZ2_bzWriteClose ( &bzerror, b, 0,0,0 );
       /* error */
       /* the writer will get smitten with sigpipe */
       return;
    }

    bzerror = BZ_OK;
    while ((nBuf=read(fd, buf, BUFFER_SIZE))>0) {
       BZ2_bzWrite ( &bzerror, b, buf, nBuf);
       if (bzerror!=BZ_OK)
       {
           BZ2_bzWriteClose( &bzerror, b, 0,0,0 );
           return;
       }
    }
    BZ2_bzWriteClose( &bzerror, b, 0,0,0 );
}
#endif

#if (defined HAVE_LIBZ) || (SHIPPED_LIBZ)
static void read_gz(FILE *f, int fd)
{
    gzFile  g;
    int     nBuf;
    char    buf[BUFFER_SIZE];
    
    g=gzdopen(fileno(f), "rb");
    if (!g)
    {
        ERRORMSG(_("Invalid/corrupt .gz file.\n"));
        return;
    }
    while((nBuf=gzread(g, buf, BUFFER_SIZE))>0)
    {
        if (write(fd, buf, nBuf)!=nBuf)
        {
            gzclose(g);
            return;
        }
    }
    if (nBuf)
    {
        ERRORMSG("\e[0m");
        ERRORMSG(_("gzip: Error during decompression.\n"));
    }
    gzclose(g);        
}

static void write_gz(FILE *f, int fd)
{
    gzFile  g;
    int     nBuf;
    char    buf[BUFFER_SIZE];

    g=gzdopen(fileno(f), "wb9");
    if (!g)
        return;
    while((nBuf=read(fd, buf, BUFFER_SIZE))>0)
    {
        if (gzwrite(g, buf, nBuf)!=nBuf)
        {
            gzclose(g);
            return;
        }
    }
    gzclose(g);
}
#endif

compress_info compressors[]={
#if (defined HAVE_LIBZ) || (SHIPPED_LIBZ)
{"gzip",	".gz",	write_gz},
#endif
#if (defined HAVE_LIBBZ2) || (defined SHIPPED_LIBBZ2)
{"bzip2",	".bz2",	write_bz2},
#endif
{0, 0, 0},
};

compress_info decompressors[]={
#if (defined HAVE_LIBZ) || (SHIPPED_LIBZ)
{"gzip",	".gz",	read_gz},
#endif
#if (defined HAVE_LIBBZ2) || (defined SHIPPED_LIBBZ2)
{"bzip2",	".bz2",	read_bz2},
#endif
{0, 0, 0},
};

compress_info *comp_from_ext(char *name, compress_info *ci)
{
    for(;ci->name;ci++)
        if (match_suffix(name, ci->ext, 0))
            return ci;
    return 0;
}
