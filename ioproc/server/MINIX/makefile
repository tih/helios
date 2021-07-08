#------------------------------------------------------------------------------
# Helios generic make system - NATIVE HOST MAKEFILE -
#-----------------------------------------------------------------------------
# MINIX Host specific make rules to make NATIVE HOST version of server component.
#
# File: /HSRC/ioproc/server/MINIX/makefile
#
# This file contains ALL the definitions and rules that are
# required to make a NATIVE HOST version of this component.
# IT IS NOT COMPLIMENTARY TO THE GENERIC VERSION.
#
# This makefile is always executed within this host specific subdir.
# All host specific source files should be kept here.
# All intermediate, object and binary files should be made in here.
#
# SccsId: %W% %G%
# RcsId: $Id: makefile,v 1.1 1993/07/22 15:45:36 bart Exp $
#		(C) Copyright 1990 Perihelion Software
#
# WARNING: assumes you are using GNUmake.
#-----------------------------------------------------------------------------

InProcMake = TRUE

include ../makefile

sys 	  = /usr/include
SUN = $(HSRC)/$(COMPONENT)/sun
MINIX = $(HSRC)/$(COMPONENT)/minix

HOSTCC = cc
HOSTCFLAGS := -O -D$(HHOST) -D__$(HPROC) -D__HELIOS$(HPROC) $(HOSTCFLAGS)

#-----------------------------------------------------------------------------
# Note all final targets here

XTARGETS = server # hydra hydramon

Xdefault : $(XTARGETS)

Xinstall : $(XTARGETS)
	   $(CP) server $(HPROD)/server.minix
	   $(TOUCH) Xinstall
#	   $(CP) hydra $(HPROD)/hydra.minix
#	   $(CP) hydramon $(HPROD)/hydramon.minix

Xclean	:


# Note all sources
XSOURCES	= $(XSOURCES.co) \
                  $(XSOURCES.link)
                  $(XSOURCES.odd)

XSOURCES.co     = $(MINIX)/colib.c
XSOURCES.link   = $(MINIX)/linkif.c

XSOURCES.odd	= makefile

# Object files

XOBJECTS 	= $(XOBJECTS.co) \
		  $(XOBJECTS.link) \
		  $(SERVOBJ) \
		  $(LINKOBJS)
#		  $(HYDRAOBJ) \
#		  $(HYDRAMON)


XOBJECTS.co    = colib.o
XOBJECTS.link  = linkif.o


$(XOBJECTS) : helios.h

#-----------------------------------------------------------------------------
# Native Host targets and their dependencies:

# uncomment if you have compiler or makedepend generated dependencies
#include depends

HOSTCFLAGS := $(HOSTCFLAGS) -I.

helios.h: $(GENHEADERS.c)
	cat $(sys)/stdio.h $(sys)/sys/types.h $(sys)/ctype.h $(sys)/setjmp.h \
	$(sys)/sys/time.h $(sys)/errno.h $(sys)/sys/socket.h \
	$(sys)/sys/times.h $(sys)/signal.h $(sys)/fcntl.h \
	$(sys)/unistd.h $(sys)/sys/stat.h $(sys)/sys/param.h \
	$(sys)/sys/file.h $(sys)/memory.h $(sys)/stdlib.h \
	$(sys)/netinet/in.h $(sys)/netdb.h $(sys)/sys/un.h \
	$(sys)/dirent.h  $(sys)/termios.h \
	$(SUN)/sunlocal.h $(MINIX)/minix.h \
	$(GENHEADERS.c) 
	> helios.h

colib_link = colib.o -ltermcap

colib.o : $(MINIX)/colib.c
	$(HOSTCC) -c $(HOSTCFLAGS) -D$(HHOST) $(MINIX)/colib.c

linkif.o : $(MINIX)/linkif.c
	$(HOSTCC) -c $(HOSTCFLAGS) -D$(HHOST) $(MINIX)/linkif.c

server : helios.h $(SERVOBJ) $(XOBJECTS.co) $(LINKOBJS)
	$(HOSTCC) -o $@ $(SERVOBJ) $(LINKOBJS) $(colib_link)

hydra : $(HYDRAOBJ) $(LINKOBJS)
	$(HOSTCC) -o $@ $(HYDRAOBJ) $(LINKOBJS) 

hydramon : $(HYDRAMON)
	$(HOSTCC) -o $@ $(HYDRAMON)

#-----------------------------------------------------------------------------
