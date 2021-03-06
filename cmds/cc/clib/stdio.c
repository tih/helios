/* stdio.c: ANSI draft (X3J11 Oct 86) library code, section 4.9 */
/* Copyright (C) A.C. Norman and A. Mycroft */
/* version 0.17a */
/* $Id: stdio.c,v 1.12 1996/10/22 14:21:28 david Exp $ */

/* N.B. I am trying to factor out machine dependence via calls to           */
/* routines like _sys_read_ which can be implemented as _osgbpb or          */
/* NIOP as required.  This file SHOULD therefore be machine independent     */

/* BLV - for the sake of efficiency I have decided to get rid of _sys_read  */
/* etc. and to use the system library directly.				    */

/* the #include <stdio.h> imports macros getc/putc etc.  Note that we
   must keep the two files in step (more details in ctype.c).
*/

#ifdef POSIX
#include <syslib.h>
#include <posix.h>
#include <fcntl.h>
#endif

#include "norcrosys.h"    /* _sys_alloc() etc */
#include "sysdep.h"
#include <stdio.h>     /* macros for putc, getc, putchar, getchar */
#include <string.h>    /* for memcpy etc */
#include <stddef.h>    /* for size_t etc */
#include <stdlib.h>    /* for free() */
#include <time.h>      /* for time() for tmpfil() */



#ifndef _SYS_OPEN
/* Temp feature to cope with upgrade path of compiler */
#define _SYS_OPEN SYS_OPEN
#endif

#ifdef POSIX
extern
#endif
FILE _iob[_SYS_OPEN];

/* Note that the real name for this variable is _errno, and the macro    */
/* errno is defined in math.h and stddef.h                               */
#ifndef POSIX
volatile int errno;                              /* error code           */
#endif
/* Explanation of the _IOxxx bits:                                       */
/* IOFP1 says that the file is positioned at the end of the current      */
/* buffer (as when performing sequential reads), while IOFP2 indicates   */
/* that its position is at the start of the current buffer (as in the    */
/* usual case when writing a file). When the relevant bit is not set     */
/* and a transfer is needed _sys_seek() is used to reposition the file.  */
/* Now extra bit _IOSEEK indicating that repositioning of the file       */
/* is required before performing I/O                                     */
/* I now use IOFP1 and IOFP2 to check the restriction                    */
/* that read and write operations must be separated by fseek/fflush.     */

/* N.B. bits up to (and including 0xfff) in <stdio.h>                    */
#define _IOFP1    0x1000        /* last op was a read                    */
#define _IOFP2    0x2000        /* last op was a write                   */
#define _IOPEOF   0x4000        /* 'pending' EOF                         */
#define _IOAPPEND 0x8000        /* must seek to eof before any write     */
#ifdef never
BLV - I need some more flags
#define _IODEL    0xdead0000   /* for safety check 16 bits              */
#define _IODELMSK 0xffff0000
#endif
#define _IODEL	  0xdea00000	/* so check is cut to 12 bits */
#define _IODELMSK 0xfff00000

				/* BLV addition */
#define _IOFUNNY  0x00010000	/* for MSdos-style files, where I have to */
				/* convert carriage returns etc.	  */
#define _IOCRREAD 0x00020000

/* first functions for macros in <stdio.h>  */
int (fgetc)(FILE *stream) { return getc(stream); }
int (fputc)(int ch, FILE *stream) { return putc(ch, stream); }
int (getc)(FILE *stream) { return getc(stream); }
int (getchar)() { return getchar(); }
int (putc)(int ch, FILE *stream) { return putc(ch, stream); }
int (putchar)(int ch) { return putchar(ch); }
int (feof)(FILE *stream) { return feof(stream); }
int (ferror)(FILE *stream) { return ferror(stream); }


/* put this here too */
void clearerr(FILE *stream)
{   /* we should do more in 'clearerr' resetting _pos _ptr and _cnt      */
    stream->_flag &= ~(_IOEOF+_IOERR+_IOPEOF);
}

#define seterr(stream) ((stream)->_flag |= _IOERR)

int setvbuf(FILE *stream, char *buf, int type, size_t size)
{   int flags = stream -> _flag;
    unsigned char *ubuf = (unsigned char *) buf;   /* costs nothing */
    if (!(flags & _IOREAD+_IOWRITE) || (flags & _IONBF+_IOFBF+_IOLBF+_IOSBF))
        return 1;             /* failure - not open, or already buffered */
    switch (type)
    {   default: return 1;    /* failure */
        case _IONBF:
            ubuf = stream->_lilbuf;
            size = 1;
            break;
        case _IOLBF:
        case _IOFBF:
            if (size-1 >= 0xffffff) return 1;  /* unsigned! */
            if ( ubuf == NULL ) ubuf = (unsigned char *)_sys_alloc(size),type |= _IOSBF;
            break;
    }
    stream -> _ptr = stream -> _base = ubuf;
    stream -> _bufsiz = size;
    stream -> _flag |= type;
    return 0;                 /* success */
}

/**
*** BLV - the test for buffering used to be == instead of !=
**/
void setbuf(FILE *stream, char *buf)
{   (void) setvbuf(stream, buf, (buf != NULL ? _IOFBF : _IONBF), BUFSIZ);
}

int ungetc(int c,FILE *stream)
{   /* made into a fn to evaluate each arg once. */
    /* we should allocate a buffer if no reads so far when called */
    if (c==EOF || stream->_ptr == stream->_base) return EOF;
    (stream->_icnt)++;
/* The next line avoids writing to a scanf pseudo-stream (maybe write-      */
/* protected string).  Scanf guarantees to only ungetc() last getc()'d char */
    if (stream->_flag & _IOSTRG) return (unsigned char)(--stream->_ptr,c);
    return *--(stream->_ptr) = c;
}

static int _writebuf(unsigned char *buf, int len, FILE *stream)
{   int w;
    FILEHANDLE fh = stream->_file;
    int flag = stream->_flag;

    if (flag & _IOSEEK)
    {   
#ifndef POSIX
	(void) Seek(fh, S_Beginning, stream->_pos);
#else
	(void) lseek(fh, stream->_pos, SEEK_SET);

#endif
        stream->_flag = (flag &= ~_IOSEEK);
    }

				/* BLV - I need to allow for MSdos files */
    if ((stream->_flag & _IOFUNNY) eq 0)
     { 
/**
*** For normal files I can just write the data, the buffer must have been
*** allocated already or there would be no data.
**/

#ifndef POSIX
       w = ((Write(fh, (BYTE *) buf, len, -1) eq len) ? 0 : -1);
#else
       w = write(fh, (char *)buf, len)==len ? 0 : -1; /* HELIOSARM fix */
#endif
       stream->_pos += len - (w & 0x7fffffff);
       if (w!=0)    /* AM: was (w<0) but trap unwritten chars as error too */
       {   seterr(stream);
           return(EOF);
       }
       stream->_sysbase = (sysbase *) stream->_pos;
     }
    else
/**
*** For MSdos files things are rather more complicated. First of all, MSdos
*** files need a separate buffer so that I can perform the carriage return
*** conversions. This buffer is part of the sysbase structure, and I may have
*** to obtain space for it.
**/
     { int i, newlen; unsigned char *ptr1, *ptr2, *buffer;

					/* get sys buffer if needed */
       if (stream->_sysbase eq Null(sysbase))
	{ stream->_sysbase = (sysbase *)
               malloc(sizeof(sysbase) + stream->_bufsiz * 2);
	  if (stream->_sysbase eq Null(sysbase))
	    { seterr(stream);
	      return(EOF);
	    }
	}
       buffer = (unsigned char *) &(stream->_sysbase->data[0]);

/**
*** Copy the data to the system buffer, expanding newline characters
**/
					/* copy to sys buffer */
       for (i=0, newlen=0, ptr1=buf, ptr2=buffer; i<len; i++)
	{ if (*ptr1 eq '\n') *ptr2++ = '\r', newlen++;
	  *ptr2++ = *ptr1++; newlen++;
	}

					/* and the usual Write */
#ifndef POSIX
       w = ((Write(fh, (BYTE *) buffer, newlen, -1) eq newlen) ? 0 : -1);
#else
	w = (write(fh, (char *)buffer, newlen) == newlen) ? 0 : -1; /* HELIOSARM fix */
#endif
/**
*** Update the file pointers. After a write both pos and file_pos point to
*** the current Helios file position, and all of the system buffer has been
*** used. Hence if the next operation is a read the file is at the right
*** position and I do not try to use some nonsensical bit of the buffer.
**/
       stream->_pos += newlen - (w & 0x7fffffff);
       stream->_sysbase->file_pos = stream->_pos;
       stream->_sysbase->used     = 0;
       stream->_sysbase->read     = 0;

       if (w!=0)    /* AM: was (w<0) but trap unwritten chars as error too */
       {   seterr(stream);
           return(EOF);
       }
     }

    return 0;
}

int _flushbuf(int ch, FILE *stream)
{   int flag = stream -> _flag;

    if ((flag & _IOERR+_IOSTRG+_IOWRITE+_IOFP1) != _IOWRITE)
    {   stream->_ocnt = 0;                        /* 2^31 is big but finite */
        seterr(stream);
        return EOF;
    }

/* the next conditional code is ACN's view of that APPEND means seek to     */
/* EOF after EVERY fflush, not just initially.  Hmm, ANSI really should     */
/* clarify - the problem is perhaps that we wish to seek to EOF after       */
/* fflush after read, but not after fflush after write?                     */
    if ((flag & (_IOFP2+_IOSEEK+_IOAPPEND)) == _IOAPPEND)
    {   /* first write to APPEND file after FFLUSH, but not FSEEK nor       */
        /* fopen (does its own FSEEK)                                       */
        fseek(stream, 0L, SEEK_END);
        flag = stream->_flag;
    }

    stream->_flag = (flag |= _IOFP2);             /* we are writing         */
    if ((flag & _IOFBF+_IOSBF+_IONBF+_IOLBF) == 0)
    { 						/* terminal - unbuffered  */
#ifndef POSIX
	if ((stream->_file->Flags & Flags_Interactive) ne 0)
#else
	if( isatty(stream->_file) )
#endif
	    stream->_ptr = stream->_base = (unsigned char *) _sys_alloc(256),
	    stream->_bufsiz = 256,
	    stream->_flag = (flag |= _IOLBF);
        else
            /* allocate default system buffer */
            stream->_ptr = stream->_base = (unsigned char *)_sys_alloc(BUFSIZ),
            stream->_bufsiz = BUFSIZ,
            stream->_flag |= (flag |= _IOSBF);
    }
      
    if (flag & _IOFBF+_IOSBF)    /* system or user buffer */
    {   unsigned char *buff = stream->_base;
        int count = stream->_ptr - buff;

        if (count != 0)
        {   if (_writebuf(buff, count, stream)) return EOF;
        }
        stream->_ptr = buff+1;

        stream->_ocnt = stream->_bufsiz - 1;
        return (*buff = ch);
    }
    else     /* no buffer (i.e. 1 char private one) or line buffer */
    {   unsigned char *buff = stream->_base;
        int count;

        *stream->_ptr = ch;   /* always room */  /* BLV - another *ptr++ */
        (stream->_ptr)++; 
        count = stream->_ptr - buff;
        if ((flag & _IONBF) ||
               (unsigned char)ch == '\n' || count >= stream->_bufsiz)
        {   stream->_ptr = buff;
            stream->_ocnt = 0;                 /* 2^31 is big but finite */
            return _writebuf(buff, count, stream) ? EOF : (unsigned char)ch;
        }
        return (unsigned char)ch;
    }
}

int fflush(FILE *stream)
{
    if ((stream->_flag & _IOERR+_IOWRITE) != _IOWRITE) return EOF;
    /* N.B. really more to do here for ANSI input stream */
    if (stream->_flag & _IOFP2)
    {   /* only write if dirty buffer - this avoids problems with
           writing to a file opened in append (or read+write) mode
           when only input has been done since the last fflush/fseek.
        */
        unsigned char *buff = stream->_base;
        if (stream->_ptr != buff)       /* nothing to do */
        {   if (_writebuf(buff, stream->_ptr - buff, stream)) return EOF;
        }
        stream->_ptr = buff;
/* the next line forces a call to _fillbuf/_flushbuf on next putc/getc -    */
/* this is necessary since change of direction may happen.                  */
        stream->_ocnt = 0;
        stream->_flag &= ~_IOFP2;
    }
    return 0;
}

int
_fillbuf( FILE * stream )
{
  signed int 		w;
  unsigned char *	buff;
  FILEHANDLE 		fh;
  int 			flag = stream->_flag;


  /* note that sscanf (q.v.) requires this next line to yield EOF */

  if ((flag & (_IOEOF+_IOERR+_IOSTRG+_IOREAD+_IOPEOF+_IOFP2)) != _IOREAD)
    {
      stream->_icnt = 0;                      /* 2^31 is big but finite */

      if (flag & _IOEOF+_IOPEOF)
	{
	  /* writing ok after EOF read according to ansi */
	  
	  stream->_flag = flag & ~(_IOFP1+_IOPEOF) | _IOEOF;
	}
      else
	{
	  seterr(stream);
	}
      
      return(EOF);
    }

  stream->_flag = (flag |= _IOFP1);           /* we are reading         */

  if ((flag & _IOFBF+_IOSBF+_IONBF+_IOLBF) == 0)
    {
      /* allocate default system buffer */

      stream->_ptr    = stream->_base = (unsigned char *)_sys_alloc(BUFSIZ),
      stream->_bufsiz = BUFSIZ,
      stream->_flag   = (flag |= _IOSBF);
    }

  buff = stream->_base;
  fh = stream->_file;

  if (flag & _IOSEEK)
    { 
#ifndef POSIX
      (void) Seek(fh, S_Beginning, stream->_pos);
#else
      (void) lseek( fh, stream->_pos, SEEK_SET );
#endif
      stream->_flag = (flag &= ~_IOSEEK);
    }

  /* BLV - I need to allow for MSdos files */
  
  if ((stream->_flag & _IOFUNNY) eq 0)
    {
/**
*** For normal files there is no problem, the data can be read into the
*** C buffer straight away.
**/
#ifndef POSIX
      w = Read(fh, (BYTE *)buff, stream->_bufsiz, -1);
      
      if (w == -1)			/* EOF condition */
#else
      w = read(fh,(char *)buff,stream->_bufsiz);

      if (w < 0)
	{
	  w = 0;

	  seterr( stream );
	}
      
      if (w == 0)			/* EOF condition */
#endif
        {
	  stream->_flag |= _IOEOF;
	  stream->_flag &= ~_IOFP1;	/* writing OK after EOF read */
	  stream->_icnt  = 0;
#if 0
	  stream->_ptr   = buff; 	/* just for fun - NB affects ftell() - check */
#endif
	  
	  return(EOF);
        }

      stream->_icnt    = w - 1;		/* do not allow for zero length here */
      stream->_ptr     = buff + 1;
      stream->_sysbase = (sysbase *)stream->_pos;
      stream->_pos    += w;	  	/* BLV - this was done earlier before */
    }
   else
/**
*** Now it is time for some real nasties. First of all, I cannot read the data
*** directly from the file into the buffer, because I need to do some
*** carriage return conversions. Hence I need to read it into another buffer
*** and copy it into the C buffer, converting as necessary. This buffer may
*** need to be allocated.
**/
     {
       int 		flag = stream->_flag;
       int		used;
       int		i;
       unsigned char *	dest;
       unsigned char *	source;
       
       
	/* get system buffer if necessary */
       
       if (stream->_sysbase eq Null(sysbase))
	 {
	   stream->_sysbase = (sysbase *)
	     malloc(sizeof(sysbase) + stream->_bufsiz * 2);
	   
	   if (stream->_sysbase eq Null(sysbase))
	     {
	       seterr(stream);

	       return(EOF);
	     }

	   stream->_sysbase->file_pos = stream->_pos;
	   stream->_sysbase->used     = 0;
	   stream->_sysbase->read     = 0;
	 }

/**
*** Next I worry about reading in the data. I shall want to read in "i"
*** bytes of data at location dest. Now, I always want more data in the
*** sysbase buffer then the C library wants in the C buffer, because some
*** of the carriage return characters are going to be taken out. Hence
*** initially I want to read in 2*bufsiz bytes. However, having read in this
*** much I am not going to use all of it, so some will be left ready for the
*** next call to fillbuf().  This data is copied to the earlier bit of the
*** buffer, and then the remainder of the buffer is filled.
***
*** There is going to be some confusion about file pointers. stream->_pos
*** corresponds to the current Helios file position, i.e. where the next bit
*** of data is going to be read from. stream->_sysbase->file_pos is the
*** file position corresponding to the start of the current buffer.
***
*** Some sample numbers : bufsize is 1024, and the last read provided 2048
*** bytes. Of these, the library used 1048 (stripped out 24 \r's), so there
*** are 1000 bytes unused in the buffer. The used field contains 1048.
*** Hence I want to copy 1000 bytes from buf+1048 to buf, and read 1048 bytes
*** into buf+1000. used will be 1048, and i will be 1000.
***
*** EOF condition - suppose the last read only supplied 1000 characters, all
*** of which were used.
**/

       dest = (unsigned char *) &(stream->_sysbase->data[0]);
       used = stream->_sysbase->used;
       i    = stream->_sysbase->read - used;

       if (i > 0)
	 {
	   memmove(dest, dest + used, i);
	   dest += i;
	   stream->_sysbase->file_pos += used;
	 }
       else
	 {
	   used = 2 * stream->_bufsiz;     /* fill whole buffer */
	 }
       
#ifndef POSIX
       w = Read( fh, (BYTE *)dest, used, -1 );
       
       if (w == -1)			/* EOF condition */
	 w= 0;
#else
       w = read( fh, (char *)dest, used );       
       /*------------------------------------*
        *   Dave Williams (22/10/96)         *
        *   BUG FIX:  fgets() hung when      *
        *   the file became invalid during   *
        *   the read operation.              *
        *------------------------------------*/
        if (w < 0)
        {
           w = 0;
           seterr( stream );
        }
#endif

       stream->_sysbase->read = w + i;

       /* EOF condition - nothing left in buffer or file */
       
       if (stream->_sysbase->read eq 0)
	 {
	   stream->_flag |= _IOEOF;
	   stream->_flag &= ~_IOFP1;             /* writing OK after EOF read */
	   stream->_icnt  = 0;
/*	   stream->_ptr   = buff; 	 / * just for fun - NB affects ftell() - check */

	   return(EOF);
	 }

       stream->_pos += w;    /* more data read, so update Helios pointer */

/**
*** This is where I do the carriage return conversion. There are some
*** unfortunate names here. i is the offset in the sysbase buffer, and used
*** is the offset in the C buffer.
**/
       source = (unsigned char *) &(stream->_sysbase->data[0]);
       dest   = buff;
       
       for (i = 0, used = 0; (i < stream->_sysbase->read) && 
                         (used < stream->_bufsiz); i++)
	 {
	   if (*source eq '\r')
	     {
	       if ((flag & _IOCRREAD) eq 0)
		 {
		   flag |= _IOCRREAD; source++; continue;
		 }
	       else
		 {
		   *dest++ = '\r'; source++; used++; continue;
		 }
	     }
	   else if (*source eq '\n')
	     {
	       *dest++ = '\n'; source++; used++; flag &= ~_IOCRREAD; continue;
	     }
	   else if (*source eq 0x1A)
	     {
	       if ((flag & _IOCRREAD) ne 0)
		 *dest++ = '\r', used++;
	      
	       stream->_flag |= _IOEOF;
	       stream->_flag &= ~_IOFP1;          /* writing OK after EOF read */

	       if (dest == buff)
		 {
		   stream->_icnt = 0; stream->_ptr = buff; return(EOF);
		 }
	       break;
	     }
	   else
	     {
	       if ((flag & _IOCRREAD) ne 0)
		 *dest++ = '\r', used++;

	       *dest++ = *source++; used++;
	       flag &= ~_IOCRREAD;
	     }
	 }

       stream->_sysbase->used = i;       /* how many characters of the sysbase */
                                        /* have been used */

       stream->_icnt = dest - buff - 1;	/* this does not work if near eof */
       stream->_ptr  = buff+1;		/* and I have carriage returns there */
     }

  return (buff[ 0 ]);
}

static int
_fillb2( FILE * stream )
{
  if (_fillbuf(stream) == EOF)
    return EOF;

  stream->_icnt++;
  stream->_ptr--;

  return 0;
}

int
fclose( FILE * stream )
{
  /* MUST be callable on a closed file - if stream clr then no-op. */

  FILEHANDLE fh = stream->_file;
  unsigned char *buff = stream->_base;
  int flag = stream->_flag;

  if (!(flag & _IOREAD+_IOWRITE)) return 1;   /* already closed        */

  if (!(flag & _IOSTRG))                      /* from _fopen_string    */
    {
      fflush(stream);

#ifndef POSIX
      (void) Close(fh);			/* close real file       */
#else
      close(fh);
#endif

      if ((flag & _IOFUNNY) && stream->_sysbase ne Null(sysbase))
	free(stream->_sysbase);

      if (flag & _IOSBF) free(buff);          /* free buffer if system */

      /* BLV - temporary files again...	*/

      if ((flag & _IODELMSK) == _IODEL)
        {
	  char name[L_tmpnam];

	  sprintf(name, "/helios/tmp/T%.7x", stream->_signature);

	  remove(name);                 /* delete the file if possible */
        }
    }

  memclr(stream, sizeof(FILE));

  return 0;                               /* success */
}

#ifdef POSIX
static word modtab[6] = {/* r  */ O_RDONLY,
			 /* r+ */ O_RDWR,
			 /* w  */ O_WRONLY + O_CREAT + O_TRUNC,
			 /* w+ */ O_RDWR   + O_CREAT + O_TRUNC,
			 /* a  */ O_RDWR   + O_CREAT,
			 /* a+ */ O_RDWR   + O_CREAT };
#else
static word modtab[6] = { /* r  */ O_ReadOnly,
			 /* r+ */ O_ReadWrite,
			 /* w  */ O_WriteOnly + O_Create + O_Truncate,
			 /* w+ */ O_ReadWrite + O_Create + O_Truncate,
			 /* a  */ O_ReadWrite + O_Create,
			 /* a+ */ O_ReadWrite + O_Create };
#endif


FILE *freopen(const char *name, const char *mode, FILE *iob)
{
/* The use of modes "r+", "w+" and "a+" is not fully thought out   */
/* yet, in that calls to _flushbuf may write back stuff that was   */
/* loaded by _fillbuf and thereby corrupt the file.                */
/* This is now just about fixed given the ANSI restriction that    */
/* calls to getc/putc must be fflush/fseek separated.              */

#ifdef HELIOS
    extern Object *CurrentDir;
#endif
    FILEHANDLE fh;
    word Heliosmode;
    int flag, openmode;		/* nasty magic numbers for openmode */

    fclose(iob);
    
    switch (*mode++)
    {   default:  return(NULL);               /* mode is incorrect */
        case 'r': flag = _IOREAD;  openmode = 0; break;
        case 'w': flag = _IOWRITE; openmode = 4; break;
        case 'a': flag = _IOWRITE | _IOAPPEND;
                                   openmode = 8; break;
    }

    for (;;)
    {   switch (*mode++)
        {
    case '+':   flag |= _IOREAD+_IOWRITE, openmode |= 2;
                continue;
    case 'b':   flag |= _IOBIN, openmode |= 1;
                continue;
        }
        break;
    }

    Heliosmode = modtab[ openmode >> 1 ];		/* ignore binary bit */

#ifndef POSIX
    if ((fh = Open(CurrentDir, (char *)name, Heliosmode)) eq Null(Stream))
		return((FILE *) NULL);
					/* check for silly files */
    if (((fh->Flags & Flags_MSdos) ne 0) && ((openmode & 0x1) eq 0))
	flag |= _IOFUNNY;
#else
	if( (fh = open((char *)name,(int)Heliosmode)) == -1 )
	  {
	    return NULL; /* HELIOSARM fix */
	  }
    
	if ( ( ( fdstream(fh)->Flags & Flags_MSdos ) != 0 ) && ((openmode & 0x1) eq 0))
	  {
	    flag |= _IOFUNNY;
	  }    
#endif
       
    iob->_flag = flag;
    iob->_file = fh;
				    /* BLV - so that I can check for buffer */
    iob->_sysbase = Null(sysbase);

    if (openmode & 8) fseek(iob, 0L, SEEK_END);  /* a or a+             */

    return iob;
}

                   
FILE *fopen(const char *name, const char *mode)
{
  int i;

  for (i = 3; i<_SYS_OPEN; i++)
    {
      FILE *stream = &_iob[i];
      if (!(stream->_flag & _IOREAD+_IOWRITE))  /* if not open then try it */
	{
	  return(freopen(name, mode, stream));
	}      
    }
  return NULL;   /* no more i/o channels allowed for */
}

            /* BLV - compiler objects with error rather than warning */
/* FILE */ void *_fopen_string_file(const char *data, int length)
{
/* open a file that will read data from the given string argument        */
/* The declaration of this function in <nonansi.norcrosys.h> suggests    */
/* that this function is of type (void *), so this definition will lead  */
/* to a warning message.                                                 */
    int i;
    for (i=3; i<_SYS_OPEN; i++)
    {   FILE *stream = &_iob[i];
        if (!(stream->_flag & _IOREAD+_IOWRITE))  /* if not open then try it */
        {
            fclose(stream);
            stream->_flag = _IOSTRG+_IOREAD;
            stream->_ptr = stream->_base = (unsigned char *)data;
            stream->_icnt = length;
            return (void *) stream;
        }
    }
    return 0;   /* no more i/o channels allowed for */
}

int _fisatty(FILE *stream)   /* not in ANSI, but related needed for ML */
{ 	
#ifndef POSIX
  if ((stream->_flag & _IOREAD) && 
      ((stream->_file->Flags & Flags_Interactive) ne 0))
       return 1;
    return 0;
#else
	return isatty(stream->_file);
#endif
}

#ifndef POSIX
/* initialisation/termination code... */

void _initio(char *f1,char *f2,char *f3)
{
    char v[128];
    memclr(_iob, sizeof(_iob));
    /* In the next lines DO NOT use standard I/O for error msgs (not open yet)
       Moreover, open in this order so we do not create/overwrite output if
       input does not exist. */

    if (freopen(f3, "w", stderr) == (FILE *) NULL) /* BLV - used to be == 0 */
        sprintf(v,"Couldn't write %s", f3), _sys_msg(v), exit(1);

    if (freopen(f1, "r", stdin) == (FILE *) NULL)
        sprintf(v,"Couldn't read %s", f1), _sys_msg(v), exit(1);

    if (freopen(f2, "w", stdout) == (FILE *) NULL)
        sprintf(v,"Couldn't write %s", f2), _sys_msg(v), exit(1);

    return;
}
#endif

void _terminateio()
{   int i;
    for (i=3; i<_SYS_OPEN; i++) fclose(&_iob[i]);
    /* for cowardice do stdin, stdout, stderr last (in that order) */
    for (i=0; i<3; i++) fclose(&_iob[i]);
}


/* now the less machine dependent functions ... */

char *fgets(char *s, int n, FILE *stream)
{   char *a = s;
    if (n <= 1) return NULL;                  /* best of a bad deal */
    do { int ch = getc(stream);
         if (ch == EOF)                       /* error or EOF       */
         {   if (s == a) return NULL;         /* no chars -> leave  */
             if (ferror(stdin)) a = NULL;
             break; /* add NULL even if ferror(), spec says 'indeterminate' */
         }
         if ((*s++ = ch) == '\n') break;
       }
       while (--n > 1);
    *s = 0;
    return a;
}        

char *gets(char *s)
{   char *a = s;
    for (;;)
    {    int ch = getc(stdin);
         if (ch == EOF)                       /* error or EOF       */
         {   if (s == a) return NULL;         /* no chars -> leave  */
             if (ferror(stdin)) a = NULL;
             break; /* add NULL even if ferror(), spec says 'indeterminate' */
         }
         if (ch == '\n') break;
         *s++ = ch;
    }
    *s = 0;
    return a;
}        

int fputs(const char *s, FILE *stream)
{
    char c;
    while((c = *s++) != 0)
       /* if(putc(c, stream)==EOF) return(EOF);*/
        if((putc)((int) c, stream)==EOF) return(EOF); /* BLV - added cast */
    return(0);
}

int puts(const char *s)
{
    char c;
    while ((c = *s++) != 0)
       if (putchar(c) == EOF) return EOF;
    return putchar('\n');
}

/* _read improved to use _fillbuf and block moves.  Optimisation
   to memcpy too if word move.  Still possible improvments avoiding copy
   but I don't want to do these yet because of interactions
   (e.g. _pos of a file).   N.B.  _read is not far from unix 'read' */
int
_read(
      char *	ptr,
      int 	nbytes,
      FILE *	stream )
{
  int 		i = nbytes;


  do
    {
      if (i <= stream->_icnt)
        {
	  memcpy( ptr, stream->_ptr, i );
	  
	  stream->_icnt -= i;
	  stream->_ptr  += i;
	  
	  return nbytes;
        }
      else
        {
	  if (stream->_icnt > 0)
	    {	      
	      memcpy( ptr, stream->_ptr, stream->_icnt );

	      ptr += stream->_icnt;
	      i   -= stream->_icnt;

	      stream->_ptr += stream->_icnt;
	    }
	  
	  stream->_icnt = -1; /* for _pos */
        }
    }
  while (_fillb2( stream ) != EOF);

  return nbytes - i;

  /*
   * for (i=0; i<nbytes; i++)
   * {
   * if ((ch = getc(stream)) == EOF) return i;
   * *ptr++ = ch;
   * }
   * return nbytes;
   */
}

size_t fread(void *ptr, size_t itemsize, size_t count, FILE *stream)
{    /* ANSI spec says EOF and ERR treated the same as far as fread
      * is concerned and that the number of WHOLE items read is returned.
      */
    return itemsize == 0 ? 0   /* slight ansi irrationality */
                         : _read((char *)ptr, itemsize*count, stream) / itemsize;
}

int _write(const char *ptr, int nbytes, FILE *stream)
{   int i;
    for(i=0; i<nbytes; i++)
      { if (putc(*ptr, stream) == EOF) return 0;
        else ptr++;     /* BLV - used to be putc(*ptr++,) but did not work */
        /* H&S say 0 on error */
      }
    return nbytes;
}

size_t fwrite(const void *ptr, size_t itemsize, size_t count, FILE *stream)
{
/* The comments made about fread apply here too */
    return itemsize == 0 ? count
                         : _write((char *)ptr, itemsize*count, stream) / itemsize;
}

/* back to machine dependent functions */

		/* ftell is nasty if I have a funny i.e. MSdos file	*/
		/* I need to use the data in _sysbuf to work out the	*/
		/* absolute file position.				*/
long int ftell(FILE *stream)
{
/**
*** For normal files, the file position is the current Helios file position
*** minus the buffer size, which gives the file position of the start of the
*** buffer, plus however far along I am in the C buffer.
**/
  if ((stream->_flag & _IOFUNNY) eq 0)
  {
    return((long int )((WORD) stream->_sysbase + 
                       (stream->_ptr - stream->_base)));
  }
  else
/**
*** Here things are more complicated. i gives the number of characters used up
*** in the C buffer. I need to find out how many characters in the sysbase
*** buffer this corresponds to, by looking for carriage return/newline pairs,
*** and add this to the file offset corresponding to the start of the buffer.
**/
   { int i = (stream->_ptr - stream->_base);
     unsigned char *ptr;		/* ftell at beginning ? */
     int stripped = 0;

     if (stream->_sysbase == Null(sysbase))
     {
     	if( stream->_base == NULL ) return stream->_pos;
     	ptr = stream->_base;
     	for(; i > 0; i-- ) if( *ptr++ == '\n' ) stripped++;
        return(stream->_pos + stripped + stream->_ptr - stream->_base );
     }

     ptr = (unsigned char *) &(stream->_sysbase->data[0]);

     for ( ; i > 0; i--)
       if (*ptr++ eq '\r')	/* allow for multiple carriage returns */
         if (*ptr eq '\n')
           { ptr++; stripped++; }

    return (stream->_sysbase->file_pos + stripped +
            stream->_ptr - stream->_base) ;
   }
}

/* The treatment of files that can be written to seems complicated in fseek */

int fseek(FILE *stream, long int offset, int whence)
{
    FILEHANDLE fh = stream->_file;
    unsigned char *buff = stream->_base;

    if (!(stream->_flag & _IOREAD+_IOWRITE)
#ifndef POSIX
         || ((fh->Flags & Flags_Interactive) ne 0)
#else
	 || isatty(fh)
#endif
        )
        return(2);                              /* fseek impossible  */

    switch(whence)
    {
case SEEK_SET:
        break;                                  /* relative to file start */
case SEEK_CUR:
        offset += ftell(stream);               /* relative seek */
        break;
case SEEK_END:
        {   int filelen, filepos;
            filelen = (int)GetFileSize(fdstream(fh));           /* length of this file      */
            if (filelen<0)                      /* failed to read length    */
            {   seterr(stream);
                return 1;
            }
            filepos = (int)ftell(stream);
            if (filepos>filelen)                /* only possible on write   */
                filelen = filepos;              /* allow for stuff buffered */
            offset += filelen;                  /* relative to end of file  */
        }
        break;
default:
        return(2);                              /* illegal operation code   */
    }


/**
*** This line, I assume, is meant to clear out the buffers.
**/
    fflush(stream);                             /* may be over-keen         */

    stream->_flag = stream->_flag & ~(_IOEOF | _IOFP1 | _IOFP2 | _IOPEOF)
                                  | _IOSEEK;    /* clear EOF condition      */
    stream->_icnt = stream->_ocnt = 0;
    stream->_pos = (int)offset;
    stream->_ptr = buff;

    if ((stream->_flag & _IOFUNNY) ne 0)
      { if (stream->_sysbase ne Null(sysbase))
         { stream->_sysbase->read     = 0;
           stream->_sysbase->used     = 0;
           stream->_sysbase->file_pos = stream->_pos;
         }
      }
      else stream->_sysbase = (sysbase *)offset;

    return 0;
}

void rewind(FILE *stream)
{
    fseek(stream, 0L, SEEK_SET);
    clearerr(stream);
}

/* the following routines need to become the main entry I suppose          */
int fgetpos(FILE *stream, fpos_t *pos)
{  pos->lo = ftell(stream);
   return 0;
}

int fsetpos(FILE *stream, const fpos_t *pos)
{  fseek(stream, pos->lo, SEEK_SET);
   return 0;
}

/*============================================================================
	BLV - I need to think about temporary names. Probably some combination
	of environment variable TMPDIR, task string, and sequence number.
============================================================================*/

static char _tmp_file_name[L_tmpnam];
static int _tmp_file_ser = 0;

char *tmpnam(char *a)
{
/* Obtaining an unique name is tolerably nasty - what I do here is       */
/* put the name in a special directory (/helios/tmp) and give it a name keyed  */
/* to an integer that is constructed out of a serial number combined     */
/* with the current clock setting. An effect of this is that the file    */
/* name can be reconstructed from a 32-bit integer for when I want to    */
/* delete the file.                                                      */
    int signature = ((int)time(NULL) << 8) | (_tmp_file_ser++ & 0xff);
    if (a== (char *) NULL) a = _tmp_file_name;
    signature &= 0x0FFFFFFF;
    sprintf(a, "/helios/tmp/T%.7x", signature);
    return a;
}

FILE *
tmpfile( void )
{
  char *	name = _tmp_file_name;
  int 		signature = ((int)time(NULL) << 8) | (_tmp_file_ser++ & 0xff);
  FILE *	f;


  signature &= 0x0FFFFFFF;

  sprintf( name, "/helios/tmp/T%.7x", signature );

  f = fopen( name, "w+b" );

  if (f)
    {
      f->_flag |= _IODEL;
      f->_signature = signature;
    }

  return f;

} /* tmpfile */

#ifndef POSIX
/*============================================================================
	BLV addition : I need to access a Helios stream given a C stream, to
	test attributes etc. It is useful so it will probably stay here.
============================================================================*/

Stream *Heliosno(FILE *stream)
{ return((Stream *)stream->_file);
}
#endif

/* end of stdio.c */
