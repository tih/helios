#-----------------------------------------------------------------------------
# Helios generic make system - HOST SPECIFIC MAKEFILE -
#-----------------------------------------------------------------------------
# MINIX Host system specific *DEFAULT* make rules.
# 
# File: /HSRC/makeinc/MINIX.mak
#
# This file contains definitions of variables and rules which are
# common to all Helios makefiles, or which need customizing to 
# a particular host. You may tailor to a particular processor via:
# ifeq ($(HPROC),MINIX) directives. This will allow you for instance, to select
# a particular compiler on this host to create MINIX processor binaries.
#
# SccsId: %W% %G%
# RcsId: $Id: hhost.template,v 1.2 91/02/28 17:34:50 martyn Exp $
#		(C) Copyright 1990 Perihelion Software
#
# WARNING: assumes you are using GNUmake.
#-----------------------------------------------------------------------------
# Host system directories:

ifndef HHOSTBIN
  HHOSTBIN	= /usr/local/bin # Where to place host utilities e.g. IO Server
endif

TEMP	= /tmp			# dir for temporary files (/ram, /fifo, etc)
NULL	= /dev/null		# Bit bucket

#-----------------------------------------------------------------------------
# Host system commands:

# For optional commands that do not exist, simply replace with dummy:
# e.g. DISASS = -@echo#

# Native host compiler (used to generate host utilities)
HOSTCC = cc

# Cross C Compiler
ifeq ($(HPROC),TRAN)
  NC		= cc#			# Transputer C compil. on native Helios
else
  ifeq ($(HPROC),ARM)
    NC		= ncc#			# ARM C compiler on Native Helios
  else
    ifeq ($(HPROC),I860)
      NC	= ncci860#		# i860 C compiler on Native Helios
    endif
  endif
endif

# Name of Cross linker
ifeq ($(HPROC),TRAN)
  LINK	= asm#
else
  ifeq ($(HPROC),ARM)
    LINK = armlink#
  else
    ifeq ($(HPROC),I860)
      LINK = i860link#
    endif
  endif
endif

# Name of Cross assembler
ifeq ($(HPROC),TRAN)
  ASM = asm#
else
  ifeq ($(HPROC),ARM)
    ASM = hobjasm#
  else
    ifeq ($(HPROC),I860)
      ASM = i860asm#
    endif
  endif
endif

# Name of Cross diassembler
ifeq ($(HPROC),TRAN)
  DISASS = -@echo#
else
  ifeq ($(HPROC),ARM)
    DISASS = armdis#
  else
    ifeq ($(HPROC),I860)
      DISASS = -@echo#
    endif
  endif
endif

# Name of object dump program
ifeq ($(HPROC),TRAN)
  OBJDUMP = -@echo#
else
  ifeq ($(HPROC),ARM)
    OBJDUMP = objdump#
  else
    ifeq ($(HPROC),I860)
      OBJDUMP = objdump#
    endif
  endif
endif

TCP	= cp#			# text file copy
RMTEMP	= rm#			# remove temporary files
CP	= cp#			# binary file copy
OBJED	= objed#		# object editor
AMPP	= ampp#			# macro preprocessor
RM	= rm#			# file remover
SYSBUILD = sysbuild#		# nucleus image builder
TOUCH	= touch#			# update file modification time

#BACKUP	= backup -t#		# backup utility
#UPLOAD	= upload#		# upload utility


#-----------------------------------------------------------------------------
# Generic variables and rules for making Helios
#
# No changes should be necessary beyond this point
#
#-----------------------------------------------------------------------------
# Directories where things will be found...

INCLUDE	= $(HSRC)/include#	# standard include directory
NUCLEUS	= $(HSRC)/nucleus#	# nucleus source directory
KERNEL	= $(HSRC)/kernel#	# kernel source directory
UTIL	= $(HSRC)/util#		# util source directory
POSIX	= $(HSRC)/posix#	# posix source directory
CLIB	= $(HSRC)/cmds/cc/clib#	# C library source directory
FPLIB	= $(HSRC)/fplib#	# floating point library source directory
FAULT	= $(HSRC)/fault#	# fault library source directory
TCPIP	= $(HSRC)/tcpip#	# tcp/ip source directory
MACRO	= $(KERNEL)/include#	# AMPP macro include files
CMDS	= $(HSRC)/cmds#		# Commands source directory

BIN	= $(HPROD)/bin#		# production binaries
LIB	= $(HPROD)/lib#		# production libraries
ETC	= $(HPROD)/etc#		# production etc directory
TMP	= $(HPROD)/tmp#		# production temp directory

CSTART  = $(LIB)/cstart.o#	# Standard C runtime init. object file

#-----------------------------------------------------------------------------
# Following two variables are NOT USED at present
# OEMDIR should be set from command line or environment, if not make a
# suitable default here
ifndef OEMDIR
OEMDIR		= /a/helios#		# OEM source distribution directory
endif

# same for BACKUPDIR
ifndef BACKUPDIR
BACKUPDIR	= /c/helios#		# BACKUP system directory
endif

#-----------------------------------------------------------------------------
# Rule to make native objects.
# This will be overlayed if processor specific rules are included later

.SUFFIXES: .o .c

%.o: %.c
	$(HOSTCC) $(HOSTCFLAGS) -D$(HHOST) -c $<

#-----------------------------------------------------------------------------

