#-----------------------------------------------------------------------------
# Warning: "@@@" in comment = Work here to be done!
#-----------------------------------------------------------------------------
#
# TO DO:
#
# Still to have generic makefiles written:
#
#	Bart: ioproc/iserver
#	Bart: cmds/games
#	bcpl - compiler/interpreter
#	fortran
#	X11
#	gnu utilities	- *LOTS of WORK*
#	other pd stuff	- *LOTS of WORK*
#	Fortran - wait for norcroft version?
#	PC Graphics - msgfx microsoft graphics library + inc in master make
#	Paul: generic makefile for ncc/ARM
#	Martyn: masterdisks - tar	get to create a set of master disks for
#		      this level in /HeliosRoot/MasterDisks/disk1, etc.
#	Add a public domain c pre-processor (cpp) to hostutils directory.
#		This will then be used by the release system.
#
# Check:
#	make clean under helios - env size problem (to be fixed in 1.3?)
#
# Change all makefiles to use DFLT.mak and RSRC capability:
#	All makefiles to include DFLT.mak and HVPATH
#
# Write doc defining the steps to be taken at each release:
#	clean/make/test/debug/fix/srctar/check make/freeze/masterdisks+srctar
#
# Document the Helios release system in this makefile
#
# Other than that...
#

#-----------------------------------------------------------------------------
# Helios Generic Make System - Top Level Makefile - Make Entire Helios World
#-----------------------------------------------------------------------------
#
# File: /HSRC/makefile
#
# These are host/processor independent rules to make the entire Helios
# system. It does this by executing other makefiles in individual component
# directories.
#
# You should only alter this makefile by using "ifeq ($(HPROC),YYY)"
# or "ifeq ($(HHOST),YYY)" clauses for a specific processor/host.
#
# Note that unlike all other makefiles in the Helios Generic make system,
# This makefile is executed in the Helios source root directory and not
# a processor specific directory.
#
# For more info consult "/HSRC/oemdocs/makesystem".
#
#
# SccsId: %W% %G%
# RcsId: $Id: makefile,v 1.141 1994/05/16 10:33:35 nickc Exp $
#		(C) Copyright 1990 Perihelion Software
#
# WARNING: assumes you are using GNUmake.
#-----------------------------------------------------------------------------
#
#
# BASIC TARGETS:
#
# default:		(if no target specified) makes the entire Helios system
# hostutil:		makes the utilities to make Helios on the host system
# install:		makes host utilities and then uses them to make Helios
#
#
# INDIVIDUAL TARGETS:
#
# Nucleus: / Nuc:	build just the Helios nucleus
# MkDefs:		make all .def library definition files
# Linklibs:		make link libraries c.lib and helios.lib
# LibHdrs:		both of the above
# ScanLibs:		make all of the scan libraries
#
#
# All the Library and Command items can be made individually as well:
#
# Libraries:		makes all the libraries:
#				Nucleus, Kernel, Util, Fault, Fplib, Posix,
#				all Scanlibs, etc
#
# Commands:		makes following:
#				Com, Ammp, Link, Asm, Make, GMake, Emacs,
#				Shell, Ncc, Tar, etc 
#
#
# RELEASE SYSTEM: @@@ Document!
#
# TestRelease:
# SaRelease:
# TarRelease:
# CopyRelease:
# List:
#
#
# CLEANUP FACILITIES:
#
# clean:		removes all object and intermediate Helios system files
# hostclean:		removes all object and intermediate host utility files
# binclean:		removes all production binary programs and libraries
# realclean:		combines the three previous targets
# ultraclean:		all the other cleans + rcsclean
# quickclean:		just kernel:nucleus:util:fault:posix:clib:shell:cmd/com
# nucclean:		just kernel:nucleus:util
#
#
# BACKUP/SRC DELIVERY FACILITIES:
#
# srctar:		Intelligent tar backup of makefile defined sources
# gettar:		Get contents of tar backup
# tsttar:		Check consistency of tar backup
#
# tarbakall:		tar backup Helios source, rcs and binaries
#
#
#-----------------------------------------------------------------------------
# Reminders about make:
#
#	<tab> - "cmd"       = ignore exit status
#	<tab> @ "cmd"       = dont print command
#	<tab> cd dir; "cmd" = cd and other command must be on same line if
#                             command is to be executed in that dir.
#
#	Consult GNU Make manual for more info.
#-----------------------------------------------------------------------------

# The variables HPROC, HHOST, HSRC, and HPROD should already be set by 
# your make script or environment, if not then get local defaults from
# $(HSRC)/makeinc/DFLT.mak. Note that the following pathname cannot use $(HSRC).
ifndef HPROC
include makeinc/DFLT.mak
endif

ifndef HLICENSEE
 Stop the make!
 You have not defined who you are. I do not know what parts of Helios
 to build. Please check your make script (look in /hsrc/makeinc) or DFLT.mak
 file.
endif

include $(HSRC)/makeinc/$(HHOST).mak	# Default host system variables


# note what targets are phoney and not actual file targets
.PHONY: default install Mess1 EndMess1 MakeProdDir \
	TestRelease SaRelease TarRelease CopyRelease List \
	clean hostclean binclean realclean ultraclean quickclean nucclean \
	HostStartM HostEndM HostC HostIOS HostAmpp HostLink HostAsm \
	HostMisc hostutil HostUtil \
	LibHdrs MkDefs Linklibs \
	nuc2 nuc Servers Nucleus Network romsys \
	Fault Kernel Util Posix Clib Fplib \
	PatchLib Abclib \
	Scanlibs Bsd Termcap Curses MsWin \
	Asm Shell Com Link Ampp Emacs Make GMake Tar Cdl \
	Debugger Tcpip Hfs Help Fortran Bcpl Public Gnu \
	srctar gettar tsttar tarbakall makeltree \
	rcsunfreeze rcsfreeze rcslevel rcsclean rcscheck rcsreport

# tih: temporarily removed Demos from the build
# this is from the above block, other changes follow below
#	Debugger Tcpip Demos Hfs Help Fortran Bcpl Public Gnu \

#-----------------------------------------------------------------------------
# Define what part of Helios to build for different licensee's
#

# Lists of source directories for use with clean, rcs and backup targets:

# Std source components:

NUCLEUS := 	text fault kernel nucleus nucleus/syslib util posix

NETWORK := 	network
NETPKGS :=
ifndef HSINGLEPROC
 NETPKGS := $(NETPKGS)  network/packages/farmlib network/packages/tftests \
	network/packages/hwtests1
endif

LIBRARIES := 	cmds/cc/clib fplib
#ifeq ($(HLICENSEE), ABC)
# LIBRARIES := 	$(LIBRARIES) abclib/patchlib abclib 
#endif

SCANLIBS :=	scanlibs scanlibs/bsd scanlibs/termcap scanlibs/curses 

ifeq ($(HPROC),TRAN)
 SCANLIBS := $(SCANLIBS) scanlibs/vectlib
endif
ifeq ($(HPROC),C40)
 SCANLIBS := $(SCANLIBS) scanlibs/vectlib scanlibs/IEEE64
endif

MSWIN :=		ioproc/mswin/lib

SERVERS := 	servers servers/common servers/servtask \
		servers/ttyserv servers/ttyserv/devs servers/ttyserv/ttyserv \
		servers/lock servers/include servers/logger servers/familiar

ifeq ($(HLICENSEE), ABC)
 SERVERS := 	$(SERVERS) servers/serial servers/codec servers/fdc \
		servers/helios servers/ramdisk servers/romdisk \
		servers/keyboard
endif

COMMANDS := 	cmds/support cmds/shell cmds/com cmds/textutil cmds/emacs \
		cmds/emacs.311 cmds/ampp cmds/private cmds/cdl cmds/make \
		cmds/cmdtests cmds/help

ifeq ($(HPROC),TRAN)
 COMMANDS := 	$(COMMANDS) cmds/asm
else
 COMMANDS := 	$(COMMANDS) cmds/linker cmds/assembler
endif

# DEMOS :=	demos demos/hello demos/tut
# 
# ifndef HSINGLEPROC
# DEMOS :=	$(DEMOS) demos/factor demos/lb \
# 		demos/pi demos/pi/pi_farm demos/pi/pi_fast demos/pi/pi_fort \
# 		demos/pi/pi_ring demos/servers demos/servers/keyboard \
# 		demos/rmlib demos/rmlib/mappipe demos/rmlib/buildrm \
# 		demos/rmlib/owners
# 
#  ifeq ($(HPROC), TRAN)
#  DEMOS :=	$(DEMOS) demos/pi/pi_mix demos/pi/pi_mod2 \
# 		demos/pi/pi_pasc
#  endif
# endif

PUBLIC :=	cmds/public cmds/public/spreadsh cmds/public/clock \
		cmds/public/cookie cmds/public/yacc-1.4
		# @@@ TODO generic makefiles:	ed stevie rcs mille sed

GNU :=		cmds/gnu cmds/gnu/gmake cmds/gnu/tar
		# @@@ TODO generic makefiles:	cmds/gnu/bison cmds/gnu/diff \
		# cmds/gnu/flex cmds/gnu/gawk cmds/gnu/gcc
GNUMISC :=	cmds/gnu/gmake/make-doc

HOSTUTIL :=	cmds/hostutil

# std source license:

STDHELIOS :=	$(NUCLEUS) $(LIBRARIES) $(NETWORK) $(NETPKGS) $(SCANLIBS) \
		$(SERVERS) $(MSWIN) $(COMMANDS) $(PUBLIC) $(GNU) $(HOSTUTIL)
#		$(SERVERS) $(MSWIN) $(COMMANDS) $(DEMOS) $(PUBLIC) $(GNU) \
#		$(HOSTUTIL)


# cut down nucleus only license (but includes fault+posix)

NUCHELIOS :=	$(NUCLEUS)


# Optional source licenses:

DEBUGGER := 	cmds/debugger cmds/debugger/lib

# TCP/IP
TCPCMDS :=	tcpip/cmds/ftp tcpip/cmds/ftpd tcpip/cmds/inetd \
		tcpip/cmds/ping tcpip/cmds/rexecd tcpip/cmds/rlogin \
		tcpip/cmds/rsh tcpip/cmds/rshd tcpip/cmds/telnet \
		tcpip/cmds/telnetd tcpip/cmds/rcp tcpip/cmds/rlogind \
		tcpip/cmds/binmail tcpip/cmds/sendmail tcpip/cmds/lpr

TCPDEVS :=	tcpip/devs/pc-ether
ifeq ($(HPROC),TRAN)
 TCPDEVS := $(TCPDEVS) tcpip/devs/in-ether tcpip/devs/sq-ether \
		tcpip/devs/ims-b431
endif

TCPIP := 	tcpip tcpip/cmds tcpip/helios tcpip/net \
		tcpip/netinet tcpip/sys tcpip/rpc/rpc \
		tcpip/nfs $(TCPCMDS) $(TCPDEVS)

# @@@ No generic makefiles for these items (yet):
TCPEXTRAS :=	tcpip/etc tcpip/h tcpip/include tcpip/lib tcpip/machine

# @@@ to be added:
#		tcpip/netstat tcpip/rpc tcpip/test tcpip/cmds/test 
#		tcpip/cmds/tftp tcpip/cmds/route tcpip/devs/hpt02
#		tcpip/laserp
# also, sort out mount.x and nfs_prot.x in NFS
# and the makefile for rpc/rpc

X11 := 		#@@@ to add: X

ifeq ($(HPROC),C40)
  XBIN = $(HPROD)/../C40_X_Clients
endif


MSDOSFS :=	servers/msdosfs

HFSDEVS := filesys/devs/raw
ifeq ($(HPROC),TRAN)
 HFSDEVS := $(HFSDEVS) filesys/devs/m212 filesys/devs/he1000 \
		filesys/devs/msc filesys/devs/b422
endif

HFS :=		filesys filesys/fs filesys/cmds $(HFSDEVS)

ifeq ($(HPROC),TRAN)
  CCOMPILER := 	cmds/cc/compiler sa	# + standalone support
else
  ifeq ($(HPROC),C40)
    CCOMPILER :=	cmds/cc/ncc/cc350
  else
    CCOMPILER :=	cmds/cc/ncc/cc450
  endif
endif


# Host Tools:

# Tools that are run on both the host and on Helios
# i.e. must have a generic makefile

HOSTTOOLS := cmds/hostutil cmds/ampp Embedded/esysbuild

ifeq ($(HPROC),C40)
  HOSTTOOLS :=	$(HOSTTOOLS) cmds/public/yacc-1.4
else
  HOSTTOOLS :=  $(HOSTTOOLS) cmds/ampp cmds/help
endif


ifeq ($(HPROC),TRAN)
 HOSTTOOLS :=	$(HOSTTOOLS) cmds/asm cmds/com
else
 HOSTTOOLS := $(HOSTTOOLS) cmds/linker cmds/assembler
endif

# HOSTTOOLS will include C Compiler if NCC_LIC is set (see below)


# IO Server sources:
# So we can give out individual parts i.e. PC to some, unix to others, etc
# @@@ The IO Server source delivery system has to be sorted out - will it
# have a generic style listing of the sources in each makefile - see Alan?

IOSSRC :=	ioproc/server
IOSMISC :=	ioproc/server/testprog
IOSMINI :=	ioproc/miniserv
IOSISERV := 	ioproc/iserver
IOSMSGFX :=	ioproc/msgfx
IOSUNIX :=	ioproc/server/sun
IOSPC :=	ioproc/server/ibm ioproc/pcmisc

# Extra places to check RCS files (subdirectories to these packages)
IOSRCSXTRA :=	ioproc/msgfx/xp ioproc/msgfx/msg

# The other bits (these don't have a generic makefile to note their filenames):

ALLINC :=	include include/sys include/net include/netinet include/arpa \
		include/dev include/X11

OEMDOC :=	oemdoc/$(HPROC) oemdoc/slots oemdoc/filetypes oemdoc/makesystem

# extras for the TCPIP package
TCPMISC :=	servers/ttyserv/debug servers/ttyserv/window $(TCPEXTRAS)

# otherbits for the C compiler package
ifeq ($(HPROC),TRAN)
  NCCMISC :=	cmds/cc/compiler/SUN4/include
endif
# Extra places to check RCS files (subdirectories to these packages)
NCCRCSXTRA :=	cmds/cc/compiler/SUN4/include/sys

# extras for the helios filesystem
HFSMISC :=	filesys/msdos

# if you have a source std licence you also get:
STDMISC := $(GNUMISC)

# if you have any source licence you get:
FREEMISC :=	makeinc $(OEMDOC)
# Extra places to check RCS files (subdirectories to these packages)
FREERCSXTRA :=	makeinc/template

# Odd bits in root directory
ODDFILES :=	makefile 


# Who has licenses for what:
# @@@ Should also define the processor versions they have licenses for

ifeq ($(HLICENSEE), PERIHELION)		# The lot!

# These define what parts of Helios will be built/cleaned/tar'ed/rcs'ed
 SRC_LIC = TRUE
 NCC_LIC = TRUE
 HFS_LIC = TRUE
 MSDOS_LIC = TRUE
 DEBUG_LIC = TRUE
 TCPIP_LIC = TRUE
 IOS_LIC = TRUE
 X_LIC = TRUE

# What hosts we support the Helios build on:
 HOSTSUPPORT := SUN4 R140 HELIOSTRAN

# What hosts we support the IOServer on other than the PC:
 HOSTIOSUPPORT := SUN4 R140 HELIOSTRAN

# ALL licensee specific code that we support:
 XTRASRC := 	$(NETWORK)/telmat $(NETWORK)/parsytec $(NETWORK)/meiko
 EXTRAIOS :=	$(IOSSRC)/telmat $(IOSSRC)/bleistein $(IOSSRC)/gemini \
		$(IOSSRC)/st $(IOSSRC)/synergy $(IOSSRC)/windows \
		$(IOSSRC)/helios $(IOSSRC)/parsy $(IOSMSGFX) $(IOSISERV)
endif

ifeq ($(HLICENSEE), PERIHELION_C40)		# C40 system components

# These define what parts of Helios will be built/cleaned/tar'ed/rcs'ed
 SRC_LIC = TRUE
 NCC_LIC = TRUE
# HFS_LIC = TRUE
# MSDOS_LIC = TRUE
 DEBUG_LIC = TRUE
 TCPIP_LIC = TRUE
 IOS_LIC = TRUE
# X_LIC = TRUE

# What hosts we support the Helios build on:
 HOSTSUPPORT := SUN4 R140 HELIOSC40 HP

# What hosts we support the IOServer on other than the PC:
 HOSTIOSUPPORT := SUN4 R140 SUN3

# ALL licensee specific code that we support:
XTRASRC := 	
EXTRAIOS :=  $(IOSSRC)/windows	# $(IOSSRC)/helios $(IOSMSGFX)
endif

ifeq ($(HLICENSEE), PERIHELION_ARM)		# Helios-ARM system components

# These define what parts of Helios will be built/cleaned/tar'ed/rcs'ed
 SRC_LIC = TRUE
 NCC_LIC = TRUE
# HFS_LIC = TRUE
 MSDOS_LIC = TRUE
# DEBUG_LIC = TRUE
# TCPIP_LIC = TRUE
 IOS_LIC = TRUE
# X_LIC = TRUE

# What hosts we support the Helios build on:
 HOSTSUPPORT := SUN4 R140 HP

# What hosts we support the IOServer on other than the PC:
 HOSTIOSUPPORT := SUN4 R140 SUN3

# ALL licensee specific code that we support:
# XTRASRC := 	
EXTRAIOS :=  $(IOSSRC)/windows	# $(IOSSRC)/helios $(IOSMSGFX)
endif

ifeq ($(HLICENSEE), TELMAT)
 SRC_LIC = TRUE
 HFS_LIC = TRUE
 MSDOS_LIC = TRUE
 DEBUG_LIC = TRUE
 TCPIP_LIC = TRUE
 IOS_LIC = TRUE
 X_LIC = TRUE

 XTRASRC := 	$(NETWORK)/telmat
 HOSTSUPPORT := SUN4 HELIOSTRAN
 HOSTIOSUPPORT := SUN4
 EXTRAIOS :=	$(IOSSRC)/telmat $(IOSSRC)/helios $(IOSSRC)/windows/winsrvr_exe
endif

ifeq ($(HLICENSEE), PARSYTEC)
 SRC_LIC = TRUE
 HFS_LIC = TRUE
 DEBUG_LIC = TRUE
 TCPIP_LIC = TRUE
 IOS_LIC = TRUE
 X_LIC = TRUE

 XTRASRC :=	$(NETWORK)/parsytec
 HOSTSUPPORT := SUN4 HELIOSTRAN
 HOSTIOSUPPORT := SUN4
 EXTRAIOS := $(IOSSRC)/parsy
endif

ifeq ($(HLICENSEE), ETRI)
 SRC_LIC = TRUE
 HFS_LIC = TRUE
 DEBUG_LIC = TRUE
 TCPIP_LIC = TRUE
 IOS_LIC = TRUE
 X_LIC = TRUE

 XTRASRC :=
 HOSTSUPPORT := SUN4 HELIOSTRAN
 HOSTIOSUPPORT := SUN4
 EXTRAIOS :=
endif

ifeq ($(HLICENSEE), CDAC)
 SRC_LIC = TRUE
 IOS_LIC = TRUE

 HOSTSUPPORT := SUN4 HELIOSTRAN
 HOSTIOSUPPORT := SUN4
 XTRASRC :=
 XTRAIOS :=
endif

ifeq ($(HLICENSEE), INMOS)
 NCC_LIC = TRUE

 HOSTSUPPORT := SUN4 HELIOSTRAN
 HOSTIOSUPPORT := SUN4
 XTRASRC :=
 XTRAIOS :=
endif

ifeq ($(HLICENSEE),ABC)
 SRC_LIC = TRUE
 MSDOS_LIC = TRUE
 NCC_LIC = TRUE
 IOS_LIC = TRUE

 HOSTSUPPORT := R140
 HOSTIOSUPPORT := R140
 XTRASRC :=
 XTRAIOS :=
endif

ifeq ($(HLICENSEE), IGM)
 NUC_LIC = TRUE
 HFS_LIC = TRUE
 MSDOS_LIC = TRUE
 IOS_LIC = TRUE

 HOSTSUPPORT := SUN4 HELIOSTRAN
 HOSTIOSUPPORT := SUN4
 XTRASRC :=
 XTRAIOS :=
endif

ifeq ($(HLICENSEE), HPLABS)
 NUC_LIC = TRUE
 IOS_LIC = TRUE

 HOSTSUPPORT := SUN4 HELIOSTRAN
 HOSTIOSUPPORT := SUN4
 XTRASRC :=
 XTRAIOS :=
endif

ifeq ($(HLICENSEE), CSIR)
 HFS_LIC = TRUE
 IOS_LIC = TRUE

 HOSTSUPPORT := SUN4 HELIOSTRAN
 HOSTIOSUPPORT := SUN4
 XTRASRC :=
 XTRAIOS :=
endif

ifeq ($(HLICENSEE), ALENIA)
 SRC_LIC = TRUE
 IOS_LIC = TRUE

 HOSTSUPPORT := HELIOSC40
 HOSTIOSUPPORT := SUN4
 XTRAIOS :=	$(IOSSRC)/unix/hepc $(IOSSRC)/unix/nidio $(IOSSRC)/rs6000
endif

# Define what parts of Helios will be targeted in rcs, clean and tar rules

ALLSRC :=	$(XTRASRC)
ALLMISC :=	$(FREEMISC)

ifdef SRC_LIC
 ALLSRC :=	$(ALLSRC) $(STDHELIOS)
 ALLMISC :=	$(ALLMISC) $(STDMISC) $(foreach MC, $(HOSTSUPPORT), $(HOSTUTIL)/$(MC)/makefile)
endif

ifdef DEBUG_LIC
 ALLSRC :=	$(ALLSRC) $(DEBUGGER)
endif

ifdef NUC_LIC
 SERVERS :=	servers/servtask
 ALLSRC :=	$(ALLSRC) $(NUCLEUS) fplib $(HOSTUTIL) cmds/support \
		$(SERVERS)
# ALLMISC :=	$(ALLMISC) $(foreach MC, $(HOSTSUPPORT), $(HOSTUTIL)/$(MC))
endif

ifdef MSDOS_LIC
 ALLSRC :=	$(ALLSRC) $(MSDOSFS)
endif

ifdef HFS_LIC
 ALLSRC :=  	$(ALLSRC) $(HFS)
 ALLMISC :=	$(ALLMISC) $(HFSMISC)
endif

ifdef TCPIP_LIC
 ALLSRC :=	$(ALLSRC) $(TCPIP)
 ALLMISC :=	$(ALLMISC) $(TCPMISC)
endif

ifdef X_LIC
 ALLSRC := 	$(ALLSRC) $(X)
endif

ifdef NCC_LIC
 ALLSRC := 	$(ALLSRC) $(CCOMPILER)
 ALLMISC :=	$(ALLMISC) $(NCCMISC)

 ifeq ($(HPROC),TRAN)
   HOSTTOOLS :=	$(HOSTTOOLS) cmds/cc/compiler
 else
  ifeq ($(HPROC),C40)
   HOSTTOOLS := $(HOSTTOOLS) cmds/cc/ncc/cc350
  else
    ifeq ($(HPROC),ARM)
     HOSTTOOLS := $(HOSTTOOLS) cmds/cc/ncc/cc450
    endif
  endif
 endif
endif

ifdef IOS_LIC
 STDIOS :=	$(IOSSRC)
 ALLIOS :=	$(XTRAIOS) $(IOSMISC) $(IOSMINI) $(IOSUNIX) $(IOSPC)

 # Add extra ioserver versions for the supported hosts
 ALLIOS :=	$(ALLIOS) $(foreach HST, $(HOSTIOSUPPORT), $(STDIOS)/$(HST) )
endif

# Define the host tool support directories
ALLHOST :=	$(foreach MC, $(HOSTSUPPORT), $(foreach HDIR, $(HOSTTOOLS), $(HDIR)/$(MC)))


#-----------------------------------------------------------------------------
# default rule to MAKE ENTIRE HELIOS SYSTEM
#

ifdef NUC_LIC
 MAINTARGETS := LibHdrs Nucleus 
endif

ifdef SRC_LIC
 MAINTARGETS := LibHdrs Nucleus Libraries Network MsWin Commands Gnu Public
# MAINTARGETS := LibHdrs Nucleus Libraries Network MsWin Commands Demos Gnu Public
 ifeq ($(HLICENSEE), ABC)
  MAINTARGETS := $(MAINTARGETS) romsys
 endif
endif

ifdef DEBUG_LIC
 MAINTARGETS := $(MAINTARGETS) Debugger
endif
ifdef TCPIP_LIC
 MAINTARGETS := $(MAINTARGETS) Tcpip
endif

default: Mess1 $(MAINTARGETS) EndMess1


# Make entire Helios system from scratch
install: MakeProdDir HostUtil default


#-----------------------------------------------------------------------------
# Start and End Messages

Mess1:
	@echo " [[[[[ Making entire **Helios/$(HPROC)** world ]]]]]"

EndMess1:
	@echo "	[[[[[ Entire **Helios/$(HPROC)** world made successfully ]]]]]"


#-----------------------------------------------------------------------------
# Shorthand

nuc: LibHdrs Kernel Util Nuc
hostutil: HostUtil


#-----------------------------------------------------------------------------
# Make production directories
# @@@ Should also copy cshrc, loginrc, initrc, motd, etc?

MakeProdDir:
	@ test -d $(HPROD)     || mkdir $(HPROD)
	@ test -d $(HPROD)/tmp || mkdir $(HPROD)/tmp
	-touch $(HPROD)/tmp/dummy
	@ test -d $(HPROD)/etc || mkdir $(HPROD)/etc
ifeq ($(HHOST),SUN4)
	-rm $(HPROD)/etc0 $(HPROD)/etc1 $(HPROD)/tmp0 $(HPROD)/tmp1
	-ln -s $(HPROD)/etc $(HPROD)/etc0
	-ln -s $(HPROD)/etc $(HPROD)/etc1
	-ln -s $(HPROD)/tmp $(HPROD)/tmp0
	-ln -s $(HPROD)/tmp $(HPROD)/tmp1
endif
	@ test -d  $(HPROD)/lib ||	mkdir $(HPROD)/lib
	@ test -d  $(HPROD)/bin ||	mkdir $(HPROD)/bin
	@ test -d  $(HPROD)/bin/private ||	mkdir $(HPROD)/bin/private
	@ test -d  $(HPROD)/system ||	mkdir $(HPROD)/system
	@ test -d  $(HPROD)/users ||	mkdir $(HPROD)/users
	@ test -d  $(HPROD)/users/root ||	mkdir $(HPROD)/users/root
	@ test -d  $(HPROD)/users/shutdown ||	mkdir $(HPROD)/users/shutdown
	@ test -d  $(HPROD)/users/guest ||	mkdir $(HPROD)/users/guest
	@ test -d  $(HPROD)/users/guest/examples ||	mkdir $(HPROD)/users/guest/examples
	@ test -d  $(HPROD)/users/guest/examples/servers ||	mkdir $(HPROD)/users/guest/examples/servers
	@ test -d  $(HPROD)/local || mkdir $(HPROD)/local
	@ test -d  $(HPROD)/local/lib ||	mkdir $(HPROD)/local/lib
	@ test -d  $(HPROD)/local/lib/tex ||	mkdir $(HPROD)/local/lib/tex
	@ test -d  $(HPROD)/local/bin ||	mkdir $(HPROD)/local/bin
	@ test -d  $(HPROD)/local/bin/X11 ||	mkdir $(HPROD)/local/bin/X11
	@ test -d  $(HPROD)/local/games ||	mkdir $(HPROD)/local/games
	@ test -d  $(HPROD)/local/src ||	mkdir $(HPROD)/local/src
	@ test -d  $(HPROD)/local/src/hfs ||	mkdir $(HPROD)/local/src/hfs
	@ test -d  $(HPROD)/local/src/hfs/b422 ||	mkdir $(HPROD)/local/src/hfs/b422
	@ test -d  $(HPROD)/local/src/hfs/he1000 ||	mkdir $(HPROD)/local/src/hfs/he1000
	@ test -d  $(HPROD)/local/src/hfs/m212 ||	mkdir $(HPROD)/local/src/hfs/m212
	@ test -d  $(HPROD)/local/src/hfs/msc ||	mkdir $(HPROD)/local/src/hfs/msc
	@ test -d  $(HPROD)/local/src/hfs/raw ||	mkdir $(HPROD)/local/src/hfs/raw
	@ test -d  $(HPROD)/local/tcpip ||	mkdir $(HPROD)/local/tcpip
	@ test -d  $(HPROD)/local/tcpip/example ||	mkdir $(HPROD)/local/tcpip/example
	@ test -d  $(HPROD)/local/tcpip/pc-ether ||	mkdir $(HPROD)/local/tcpip/pc-ether
ifeq ($(HHOST),HELIOSTRAN)
	@ test -d  $(HPROD)/include ||	mkdir $(HPROD)/include
	-cp -r $(HSRC)/include/* $(HPROD)/include
else
ifeq ($(HHOST),HELIOSC40)
	test -d  $(HPROD)/include ||	mkdir $(HPROD)/include
	-cp -r $(HSRC)/include/* $(HPROD)/include
else
	@ test -L $(HPROD)/include || ln -s $(HSRC)/include $(HPROD)
	@ test -L $(HPROD)/bin/X11 || ln -s $(XBIN)/bin/X11 $(HPROD)/bin
	@ test -L $(HPROD)/lib/X11 || ln -s $(XBIN)/lib/X11 $(HPROD)/lib
endif
endif
	-cp $(HSRC)/ioproc/server/ibm/server_exe/server.exe $(HPROD)
	-cp $(HSRC)/ioproc/server/windows/winsrvr_exe/winsrvr.exe $(HPROD)
	$(MAKE) -C text install


#-----------------------------------------------------------------------------
# Software Release System

ifeq ($(HHOST),HELIOSTRAN)
 CPP = /helios/lib/cpp#
endif

TestRelease :
	$(CPP)  -D$(OPTS) -DHELIOS=$(HPROD) -DHPROC=$(HPROC) -DSYSTEM <Files >test.rel
ifeq (HELIOS,$(findstring HELIOS,$(HHOST)))
	chmod 0777 test.rel
	./test.rel
else
	chmod 0777 test.rel
	./test.rel
endif

TarRelease :
	$(CPP)  -D$(OPTS) -DTAR -DHELIOS=$(HPROD) -DTARFILE=helios.tar \
		-DHPROC=$(HPROC) -DSYSTEM <Files >tar.rel
ifeq (HELIOS,$(findstring HELIOS,$(HHOST)))
	chmod 0777 tar.rel
	./tar.rel
else
	chmod 0777 tar.rel
	./tar.rel
endif

SaRelease :
	$(CPP) -DTAR -DHELIOS=$(HPROD) -DTARFILE=sa.tar \
		-DHPROC=$(HPROC) -DSA <Files >tar.rel
ifeq (HELIOS,$(findstring HELIOS,$(HHOST)))
	chmod 0777 tar.rel
	./tar.rel
else
	chmod 0777 tar.rel
	./tar.rel
endif

List :
	$(CPP)  -D$(OPTS) -DLIST -DHELIOS=$(HPROD) \
		-DHPROC=$(HPROC) -DSYSTEM <Files >list.rel
ifeq (HELIOS,$(findstring HELIOS,$(HHOST)))
	chmod 0777 list.rel
	./list.rel
else
	chmod 0777 list.rel
	./list.rel
endif

CopyRelease :
	$(CPP)  -D$(OPTS) -DCOPY -DHELIOS=$(HPROD) -DRELDIR=$(RELDIR) \
		-DHPROC=$(HPROC) -DSYSTEM <Files >cp.rel
ifeq (HELIOS,$(findstring HELIOS,$(HHOST)))
	chmod 0777 cp.rel
	./cp.rel
else
	chmod 0777 cp.rel
	./cp.rel
endif


#-----------------------------------------------------------------------------
# Host Utility programs:

ifdef NUC_LIC
 HOSTTARGETS := HostMisc HostSysbuild
endif
ifdef SRC_LIC
 ifeq ($(HPROC),TRAN)
  HOSTTARGETS := HostMisc HostAmpp HostAsm HostIOS HostHelp HostSysbuild
 else
  HOSTTARGETS := HostMisc HostAmpp HostYacc HostAsm HostIOS HostHelp HostLink HostSysbuild
 endif
endif

ifdef NCC_LIC
 HOSTTARGETS := $(HOSTTARGETS) HostC
endif

HostStartM:
	@echo "	[[[[[ Making $(HHOST) HOST utilities for Helios/$(HPROC) ]]]]]"

HostEndM:
	@echo "	[[[[[ Successfully made $(HHOST) HOST utilities for Helios/$(HPROC) ]]]]]"

HostMisc:
	@echo "				[[[ Making HOST misc utils ]]]"
	$(MAKE) -C cmds/hostutil/$(HHOST) install	

HostSysbuild:
	@echo "				[[[ Making HOST Sysbuild ]]]"
	$(MAKE) -C Embedded/esysbuild/$(HHOST) install

HostAmpp:
	@echo "				[[[ Making HOST ampp ]]]"
	$(MAKE) -C cmds/ampp/$(HHOST) install

HostYacc:
    ifeq ($(HHOST),HELIOSTRAN)
	@echo "				[[[ Skipping HOST yacc ]]]"
    else
	@echo "				[[[ Making HOST yacc ]]]"
	$(MAKE) -C cmds/public/yacc-1.4/$(HHOST) install
    endif

HostLink:
	ifeq ($(HHOST),HELIOSC40)
		@echo "			[[[ Skipping HOST linker ]]]"
	else
		@echo "			[[[ Making HOST linker ]]]"
		$(MAKE) -C cmds/linker/$(HHOST) install
	endif

HostIOS:
ifeq ($(HHOST),HP)
	@echo "				[[[ No HOST IO Server for HPUX ]]]"
else
    ifeq ($(HHOST),HELIOSTRAN)
	@echo "				[[[ Skipping HOST IO Server ]]]"
    else
    ifeq ($(HHOST),HELIOSC40)
	@echo "				[[[ Skipping HOST IO Server ]]]"
    else
	@echo "				[[[ Making HOST IO Server ]]]"
	$(MAKE) -C ioproc/server/$(HHOST) install
    endif
    endif
endif

HostAsm:
ifeq ($(HPROC),TRAN)
	@echo "				[[[ Making HOST Assembler ]]]"
	$(MAKE) -C cmds/asm/$(HHOST) install
else
	ifeq ($(HHOST),HELIOSC40)
		@echo "				[[[ Skipping HOST assembler ]]]"
	else
		@echo "				[[[ Making HOST Assembler ]]]"
		$(MAKE) -C cmds/assembler/$(HHOST) install
	endif
endif

HostHelp:
ifeq ($(HHOST),HELIOSC40)
	@echo "				[[[ Skipping HOST help ]]]"
else
	@echo "				[[[ Making HOST Help ]]]"
	$(MAKE) -C cmds/help/$(HHOST) install
endif

HostC:
	@echo "				[[[ Making HOST C Compiler ]]]"
ifeq ($(HPROC),TRAN)
	$(MAKE) -C cmds/cc/compiler/$(HHOST) install
else
 ifeq ($(HPROC),ARM)
	$(MAKE) -C cmds/cc/ncc/cc450/$(HHOST) install
 else
   ifeq ($(HPROC),C40)
	$(MAKE) -C cmds/cc/ncc/cc350/$(HHOST) install
   else
	@echo "				[[[ @@@ Add rule to Make HOST C Compiler ]]]"
   endif
 endif
endif

HostUtil: HostStartM $(HOSTTARGETS) HostEndM


#-----------------------------------------------------------------------------
# Library definition files

LibHdrs: MkDefs Linklibs
	@echo "				[[[ Made library header files ]]]"


# Make all library definition and startup code files:
MkDefs:
	@echo "				[[[ Making library definition files ]]]"
	$(MAKE) -C kernel/$(HPROC) installdef
	$(MAKE) -C util/$(HPROC) installdef
	$(MAKE) -C nucleus/$(HPROC) installdef
	$(MAKE) -C fault/$(HPROC) installdef
	$(MAKE) -C posix/$(HPROC) installdef
	$(MAKE) -C fplib/$(HPROC) installdef
ifndef NUC_LIC
	$(MAKE) -C cmds/cc/clib/$(HPROC) installdef
endif
ifeq ($(HLICENSEE),ABC)
	$(MAKE) -C abclib/$(HPROC) installdef
	$(MAKE) -C abclib/patchlib/$(HPROC) installdef
endif


# Create link library files

Linklibs: 
	@echo "				[[[ Making shared link libraries ]]]"
ifeq ($(HPROC),TRAN)		# use tokenised assembler
	asm -p -o $(LIB)/helios.lib	\
		$(LIB)/kernel.def	\
		$(LIB)/syslib.def	\
		$(LIB)/util.def		\
		$(LIB)/servlib.def	\
		$(LIB)/fault.def	\
		$(LIB)/fplib.def	\
		$(LIB)/posix.def
ifndef NUC_LIC
	asm -p -o $(LIB)/c.lib		\
		$(LIB)/clib.def		\
		$(LIB)/fpclib.def	
endif
else
	cat 	$(LIB)/kernel.def	\
		$(LIB)/syslib.def	\
		$(LIB)/util.def		\
		$(LIB)/servlib.def	\
		$(LIB)/fault.def	\
		$(LIB)/fplib.def	\
		$(LIB)/posix.def	\
		> $(LIB)/helios.lib
	cat	$(HPROD)/lib/clib.def	\
		> $(LIB)/c.lib			
					# note no fpclib
endif


#-----------------------------------------------------------------------------
# The Helios Nucleus
#
# System/Servlib libs, servers and system image
# Fault library made for codes.h, errno.h and signal.h

# Note libraries that are included in the nucleus as std.
PRENUC := Fault Kernel Util

ifeq ($(HLICENSEE),ABC)
# Most libraries are included in ABC's ROMed ARM nucleus
 PRENUC := $(PRENUC) Abclib Posix Clib Fplib Patchlib 
endif

# Build scanlibs before servers so that servers can use them
ifdef SRC_LIC
 PRENUC := $(PRENUC) Scanlibs Servers
 ifdef HFS_LIC
  PRENUC := $(PRENUC) Hfs
 endif
endif
ifdef NUC_LIC
 PRENUC := $(PRENUC) Servtask Posix
endif

# Build servers before Nuc so they can be included in nucleus
Nucleus: Mess2 $(PRENUC) Nuc EndMess2

Nuc:
	@echo "				[[[ Making Nucleus ]]]"
	$(MAKE) -C nucleus/$(HPROC) install 

Mess2:
	@echo "		[[[[ Making **Helios/$(HPROC)** Nucleus ]]]]"

EndMess2:
	@$(HHOSTBIN)/newmotd > ${HPROD}/etc/motd
	@$(HHOSTBIN)/newmotd
	@echo "		[[[[ **Helios/$(HPROC)** Nucleus made successfully ]]]]"


#-----------------------------------------------------------------------------
# For servers not included in the nucleus directory

Servers:
	@echo "				[[[ Making servers and drivers ]]]"
	$(MAKE) -C servers/$(HPROC) install


#-----------------------------------------------------------------------------
# servtask for NUC_LIC

Servtask:
	@echo "				[[[ Making servtask ]]]"
	$(MAKE) -C servers/servtask/$(HPROC) install


#-----------------------------------------------------------------------------
# For Rommed systems Now build the ROM image and ROM disk.
# Make romsys last so any command can also be included in the nucleus or romdisk

romsys:
ifeq ($(HLICENSEE),ABC)
# Helios ARM low Executive and ROM system (ROM disk and ROM image)
	@echo "				[[[ Making ARM Executive and ROM system ]]]"
	$(MAKE) -C kernel/$(HPROC) romsys
else
	@echo "				[[[ ROM resident system not supported for $(HPROC) processor]]]"
endif


#-----------------------------------------------------------------------------
# The Helios Libraries

ifdef NUC_LIC
HELIOSLIBS := Fault Kernel Util Posix
endif

ifdef SRC_LIC
 HELIOSLIBS := Fault Kernel Util Posix Clib Fplib
 ifeq ($(HLICENSEE),ABC)
  HELIOSLIBS := $(HELIOSLIBS) Patchlib Abclib
 endif
endif

# Nucleus should be made last as it may include some of these libraries
HELIOSLIBS := $(HELIOSLIBS) Nuc

Libraries: Scanlibs $(HELIOSLIBS)
	@echo "				[[[ Made Helios Libaries ]]]"

# The kernel
Kernel:
	@echo "				[[[ Making Kernel ]]]"
	$(MAKE) -C kernel/$(HPROC) install

# Utility library
Util:
	@echo "				[[[ Making Util lib ]]]"
	$(MAKE) -C util/$(HPROC) install

# Floating point libraries
Fplib:
	@echo "				[[[ Making FPlib ]]]"
	$(MAKE) -C fplib/$(HPROC) install

# Fault library
Fault:
	@echo "				[[[ Making Fault lib ]]]"
	$(MAKE) -C fault/$(HPROC) install

# Posix library
Posix:
	@echo "				[[[ Making Posix lib ]]]"
	$(MAKE) -C posix/$(HPROC) install

# C language support library
Clib:
	@echo "				[[[ Making Clib ]]]"
	$(MAKE) -C cmds/cc/clib/$(HPROC) install


ifeq ($(HLICENSEE),ABC)
# Patch library
Patchlib:
	@echo "				[[[ Making PatchLIB ]]]"
	$(MAKE) -C abclib/patchlib/$(HPROC) install


# ABClib library
Abclib:
	@echo "				[[[ Making ABClib ]]]"
	$(MAKE) -C abclib/$(HPROC) install
endif


# Build All the Scan Libraries
Scanlibs:
	@echo "				[[[ Making all Scan Libraries ]]]"
	$(MAKE) -C scanlibs/$(HPROC) install

# Scan libraries that can be made individually (not realy needed I know)

# BSD library
Bsd:
	@echo "				[[[ Making BSD lib ]]]"
	$(MAKE) -C scanlibs/bsd/$(HPROC) install

# Curses library
Curses:
	@echo "				[[[ Making Curses lib ]]]"
	$(MAKE) -C scanlibs/curses/$(HPROC) install

# Termcap library
Termcap:
	@echo "				[[[ Making Termcap lib ]]]"
	$(MAKE) -C scanlibs/termcap/$(HPROC) install


# MsWindows graphics library
MsWin: Network
	@echo "				[[[ Making MsWin library ]]]"
	$(MAKE) -C ioproc/mswin/lib/$(HPROC) install

#-----------------------------------------------------------------------------
# Network support
#
# Make even for single processor versions - network makefile will make
# versions of wsh, ps, loaded and run that do not require ns/tfm/RmLib

Network:
	@echo "				[[[ Making Network Software ]]]"
	$(MAKE) -C network/$(HPROC) install


#-----------------------------------------------------------------------------
# Helios Commands
#
# @@@ Should also add all other GNU utilities + other pd suff - steVIe
# Or possibly top level GNU make file

CMDTARGETS := Shell Com Asm Ampp Emacs GMake Tar Testsuite Help Make

ifdef NCC_LIC
 CMDTARGETS := $(CMDTARGETS) Ncc 
endif

ifndef HSINGLEPROC
 CMDTARGETS := $(CMDTARGETS) Cdl
endif

ifneq ($(HPROC),TRAN)
 CMDTARGETS := $(CMDTARGETS) Link
endif

ifeq ($(HPROC),TRAN) # only tranny version bothers with old make
 CMDTARGETS := $(CMDTARGETS) Make
endif

Commands: $(CMDTARGETS)

# NorCroft ANSI C Compiler
Ncc:
	@echo "				[[[ Making Norcroft C Compiler ]]]"
ifeq ($(HPROC),TRAN)
	$(MAKE) -C cmds/cc/compiler/$(HPROC) install
	$(MAKE) -C sa/$(HPROC) install	
					# also make standalone support
else
  ifeq ($(HPROC),C40)
	$(MAKE) -C cmds/cc/ncc/cc350/$(HPROC) install
  else
	$(MAKE) -C cmds/cc/ncc/cc450/$(HPROC) install
  endif
endif

# Shell
Shell:
	@echo "				[[[ Making Shell ]]]"
	$(MAKE) -C cmds/shell/$(HPROC) install

# Make
GMake:
	@echo "				[[[ Making GNU Make ]]]"
	$(MAKE) -C cmds/gnu/gmake/$(HPROC) install

# Tar
Tar:
	@echo "				[[[ Making GNU Tar ]]]"
	$(MAKE) -C cmds/gnu/gtar/$(HPROC) install

Make:
	@echo "				[[[ Making Make ]]]"
	$(MAKE) -C cmds/make/$(HPROC) install

# Helios portable object format Linker
Link:
ifneq ($(HPROC),TRAN)
	@echo "				[[[ Making linker/imdump/objdump]]]"
	$(MAKE) -C cmds/linker/$(HPROC) install
endif

# Transputer Assembler linker
Asm:
ifeq ($(HPROC),TRAN)
	@echo "				[[[ Making Transputer assembler/linker asm ]]]"
	$(MAKE) -C cmds/asm/$(HPROC) install
else
	@echo "				[[[ Making $(HPROC) assembler ]]]"
	$(MAKE) -C cmds/assembler/$(HPROC) install
endif

# Assembler Macro Preprocessor
Ampp:
	@echo "				[[[ Making ampp ]]]"
	$(MAKE) -C cmds/ampp/$(HPROC) install

# Emacs editor
Emacs:
	@echo "				[[[ Making Emacs ]]]"
	$(MAKE) -C cmds/emacs/$(HPROC) install
	$(MAKE) -C cmds/emacs.311/$(HPROC) install

# Component Distribution Language
Cdl:
	@echo "				[[[ Making CDL ]]]"
	$(MAKE) -C cmds/cdl/$(HPROC) install

# General Helios Commands
Com:
	@echo "				[[[ Making General Commands ]]]"
	$(MAKE) -C cmds/com/$(HPROC) install
	$(MAKE) -C cmds/textutil/$(HPROC) install
	$(MAKE) -C cmds/private/$(HPROC) install

Testsuite:
	@echo "				[[[ Making Command Test Suite ]]]"
	$(MAKE) -C cmds/cmdtests/$(HPROC) install


#-----------------------------------------------------------------------------
# Demos and Tutorials

# Demos:
# 	@echo "				[[[ Making Demos ]]]"
# 	$(MAKE) -C demos/$(HPROC) install


#-----------------------------------------------------------------------------
# Public domain software - games/utils/etc (but not GNU)

Public:
	@echo "				[[[ Making Public Domain Programs ]]]"
	$(MAKE) -C cmds/public/$(HPROC) install


# GNU semi-public domain software
Gnu:
	@echo "				[[[ Making Public Domain Programs ]]]"
	$(MAKE) -C cmds/gnu/$(HPROC) install


#-----------------------------------------------------------------------------
# X Window system
# @@@ NOT TESTED - no generic makefile yet

X11:
	@echo "				[[[ Making X11 servers and drivers ]]]"
	$(MAKE) -C X/$(HPROC) install


#-----------------------------------------------------------------------------
# Source Level Debugger

Debugger:
	@echo "				[[[ Making TLA source level debugger ]]]"
	$(MAKE) -C cmds/debugger/$(HPROC) install
	$(MAKE) -C cmds/debugger/lib/$(HPROC) install


#-----------------------------------------------------------------------------
# Ethernet TCP/IP support

Tcpip:
	@echo "				[[[ Making TCP/IP servers and tools ]]]"
	$(MAKE) -C tcpip/$(HPROC) install


#-----------------------------------------------------------------------------
# Helios file system

Hfs:
	@echo "				[[[ Making Helios file system server, drivers and commands ]]]"
	$(MAKE) -C filesys/$(HPROC) install


#-----------------------------------------------------------------------------
# Fortran
# @@@ NOT TESTED - requires BCPL - no generic makefiles yet

Fortran:
	@echo "				[[[ Making Fortran ]]]"
ifeq ($(HPROC), TRAN)
	$(MAKE) -C cmds/bcpl/fortran
else
	$(MAKE) -C cmds/bcpl/fortran/$(HPROC) install
endif


#-----------------------------------------------------------------------------
# Help

Help:
	@echo "				[[[ Making Help System ]]]"
	$(MAKE) -C cmds/help/$(HPROC) install


#-----------------------------------------------------------------------------
# BCPL
# @@@ NOT TESTED - no generic makefiles yet

Bcpl:
	@echo "				[[[ Making BCPL ]]]"
ifeq ($(HPROC), TRAN)
	$(MAKE) -C cmds/bcpl/$(HPROC) install
else
	$(MAKE) -C cmds/bcplterp/$(HPROC) install
					# make interpreter
endif


#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------
# MAKEFILE UTILITIES


#-----------------------------------------------------------------------------
# RCS targets *Only used by Perihelion* 
#
#
#	rcsfreeze	create a global freeze level
#	rcslevel	globally checkout a particular level
#	rcsclean	globally cleanout all files held under rcs (springclean)
#	rcscheck	globally check that all files are check in
#	rcsreport	globally print reports of changes between RCSLEVEL
#			and RCSLEVEL2. RCSLEVEL2 is optional, leaving it out
#			will give reports up to the latest version.
#
#	We pass an RCSLEVEL=xxx parameter to make for rcslevel and rcsfreeze.


RCSMISC :=	. $(ALLINC) oemdoc makeinc $(NCCRCSXTRA) $(FREERCSXTRA) \
		$(IOSRCSXTRA)
# $(HSRC) in last target to get master makefile and Files checked

rcsfreeze:
ifndef RCSLEVEL
	@echo "RCSLEVEL=XXX must be defined for rcsfreeze"
else
	-@$(foreach RCSDIR, $(ALLSRC), echo rcsfreeze $(RCSDIR) @ $(RCSLEVEL); cd $(HSRC)/$(RCSDIR)/RCS; rcsfreeze $(RCSLEVEL); )
	-@$(foreach RCSDIR, $(ALLSRC), echo rcsfreeze $(RCSDIR)/$(HPROC) @ $(RCSLEVEL); cd $(HSRC)/$(RCSDIR)/$(HPROC)/RCS; rcsfreeze $(RCSLEVEL); )
	-@$(foreach RCSDIR, $(ALLHOST), echo rcsfreeze $(RCSDIR) @ $(RCSLEVEL); cd $(HSRC)/$(RCSDIR)/RCS; rcsfreeze $(RCSLEVEL); )
	-@$(foreach RCSDIR, $(ALLIOS) $(STDIOS), echo rcsfreeze $(RCSDIR) @ $(RCSLEVEL); cd $(HSRC)/$(RCSDIR)/RCS; rcsfreeze $(RCSLEVEL); )
	-@$(foreach RCSDIR, $(RCSMISC), echo rcsfreeze $(RCSDIR) @ $(RCSLEVEL); cd $(HSRC)/$(RCSDIR)/RCS; rcsfreeze $(RCSLEVEL); )
endif

rcsunfreeze:
ifndef RCSLEVEL
	@echo "RCSLEVEL=XXX must be defined for rcsunfreeze"
else
	-@$(foreach RCSDIR, $(ALLSRC), echo rcsunfreeze $(RCSDIR) @ $(RCSLEVEL); cd $(HSRC)/$(RCSDIR)/RCS; rcs -n$(RCSLEVEL) *; )
	-@$(foreach RCSDIR, $(ALLSRC), echo rcsunfreeze $(RCSDIR)/$(HPROC) @ $(RCSLEVEL); cd $(HSRC)/$(RCSDIR)/$(HPROC)/RCS; rcs -n$(RCSLEVEL) *; )
	-@$(foreach RCSDIR, $(ALLHOST), echo rcsunfreeze $(RCSDIR) @ $(RCSLEVEL); cd $(HSRC)/$(RCSDIR)/RCS; rcs -n$(RCSLEVEL) *; )
	-@$(foreach RCSDIR, $(ALLIOS) $(STDIOS), echo rcsunfreeze $(RCSDIR) @ $(RCSLEVEL); cd $(HSRC)/$(RCSDIR)/RCS; rcs -n$(RCSLEVEL) *; )
	-@$(foreach RCSDIR, $(RCSMISC), echo rcsunfreeze $(RCSDIR) @ $(RCSLEVEL); cd $(HSRC)/$(RCSDIR)/RCS; rcs -n$(RCSLEVEL) *; )
endif

rcsdiff:
ifndef RCSLEVEL
	@echo "RCSLEVEL=XXX must be defined for rcsdiff"
else
TMP=/tmp
	-@$(foreach RCSDIR, $(ALLSRC), echo rcsdiff $(RCSDIR) @ $(RCSLEVEL); cd $(HSRC)/$(RCSDIR); rcsdiff -r$(RCSLEVEL) RCS/*; )
	-@$(foreach RCSDIR, $(ALLSRC), echo rcsdiff $(RCSDIR)/$(HPROC) @ $(RCSLEVEL); cd $(HSRC)/$(RCSDIR)/$(HPROC); rcsdiff -r$(RCSLEVEL) RCS/*; )
	-@$(foreach RCSDIR, $(ALLHOST), echo rcsdiff $(RCSDIR) @ $(RCSLEVEL); cd $(HSRC)/$(RCSDIR); rcsdiff -r$(RCSLEVEL) RCS/*; )
	-@$(foreach RCSDIR, $(ALLIOS) $(STDIOS), echo rcsdiff $(RCSDIR) @ $(RCSLEVEL); cd $(HSRC)/$(RCSDIR); rcsdiff -r$(RCSLEVEL) RCS/*; )
	-@$(foreach RCSDIR, $(RCSMISC), echo rcsdiff $(RCSDIR) @ $(RCSLEVEL); cd $(HSRC)/$(RCSDIR); rcsdiff -r$(RCSLEVEL) RCS/*; )
endif

rcslevel:
ifndef RCSLEVEL
	@echo "RCSLEVEL=XXX must be defined for rcslevel to work"
else
	-@$(foreach RCSDIR, $(ALLSRC), echo "co $(RCSDIR) -f$(RCSLEVEL)"; cd $(HSRC)/$(RCSDIR); co -f$(RCSLEVEL) $(wildcard $(HSRC)/$(RCSDIR)/RCS/*); )
	-@$(foreach RCSDIR, $(ALLSRC), echo "co $(RCSDIR)/$(HPROC) -f$(RCSLEVEL)"; cd $(HSRC)/$(RCSDIR)/$(HPROC); co -f $(RCSLEVEL) $(wildcard $(HSRC)/$(RCSDIR)/$(HPROC)/RCS/*); )
	-@$(foreach RCSDIR, $(ALLHOST), echo "co $(RCSDIR) -f$(RCSLEVEL)"; cd $(HSRC)/$(RCSDIR); co -f$(RCSLEVEL) $(wildcard $(HSRC)/$(RCSDIR)/RCS/*); )
	-@$(foreach RCSDIR, $(ALLIOS) $(STDIOS), echo "co $(RCSDIR) -f$(RCSLEVEL)"; cd $(HSRC)/$(RCSDIR); co -f$(RCSLEVEL) $(wildcard $(HSRC)/$(RCSDIR)/RCS/*); )
	-@$(foreach RCSDIR, $(RCSMISC), echo "co $(RCSDIR) -f$(RCSLEVEL)"; cd $(HSRC)/$(RCSDIR); co -f$(RCSLEVEL) $(wildcard $(HSRC)/$(RCSDIR)/RCS/*); )
endif

rcsclean:
	-@$(foreach RCSDIR, $(ALLSRC), echo cleaning $(RCSDIR); rcsclean $(wildcard $(RCSDIR)/RCS/*); )
	-@$(foreach RCSDIR, $(ALLSRC), echo cleaning $(RCSDIR)/$(HPROC); rcsclean $(wildcard $(RCSDIR)/$(HPROC)/RCS/*); )
	-@$(foreach RCSDIR, $(ALLHOST), echo cleaning $(RCSDIR); rcsclean $(wildcard $(RCSDIR)/RCS/*); )
	-@$(foreach RCSDIR, $(ALLIOS) $(STDIOS), echo cleaning $(RCSDIR); rcsclean $(wildcard $(RCSDIR)/RCS/*); )
	-@$(foreach RCSDIR, $(RCSMISC), echo cleaning $(RCSDIR); rcsclean $(wildcard $(RCSDIR)/RCS/*); )

rcscheck:
	-@$(foreach RCSDIR, $(ALLSRC), echo checking $(RCSDIR); cd $(HSRC)/$(RCSDIR); rlog -L -h RCS/*; )
	-@$(foreach RCSDIR, $(ALLSRC), echo checking $(RCSDIR)/$(HPROC); rlog -L -h $(wildcard $(RCSDIR)/$(HPROC)/RCS/*); )
	-@$(foreach RCSDIR, $(ALLHOST), echo checking $(RCSDIR); rlog -L -h $(wildcard $(RCSDIR)/RCS/*); )
	-@$(foreach RCSDIR, $(ALLIOS) $(STDIOS), echo checking $(RCSDIR); rlog -L -h $(wildcard $(RCSDIR)/RCS/*); )
	-@$(foreach RCSDIR, $(RCSMISC), echo checking $(RCSDIR); rlog -L -h $(wildcard $(RCSDIR)/RCS/*); )

rcsreport:
	-@$(foreach RCSDIR, $(ALLSRC), echo RCS changes report on $(RCSDIR); rlog -r$(RCSLEVEL)-$(RCSLEVEL2) $(wildcard $(RCSDIR)/RCS/*); )
	-@$(foreach RCSDIR, $(ALLSRC), echo RCS changes report on $(RCSDIR)/$(HPROC); rlog -r$(RCSLEVEL)-$(RCSLEVEL2) $(wildcard $(RCSDIR)/$(HPROC)/RCS/*); )
	-@$(foreach RCSDIR, $(ALLHOST), echo RCS changes report on $(RCSDIR); rlog -r$(RCSLEVEL)-$(RCSLEVEL2) $(wildcard $(RCSDIR)/RCS/*); )
	-@$(foreach RCSDIR, $(ALLIOS) $(STDIOS), echo RCS changes report on $(RCSDIR); rlog -r$(RCSLEVEL)-$(RCSLEVEL2) $(wildcard $(RCSDIR)/RCS/*); )
	-@$(foreach RCSDIR, $(RCSMISC), echo RCS changes report on $(RCSDIR); rlog -r$(RCSLEVEL)-$(RCSLEVEL2) $(wildcard $(RCSDIR)/RCS/*); )

rcsco:
	-@$(foreach RCSDIR, $(ALLSRC), echo check out from $(RCSDIR); co -u $(wildcard $(RCSDIR)/RCS/*); )
	-@$(foreach RCSDIR, $(ALLSRC), echo check out from $(RCSDIR); co -u $(wildcard $(RCSDIR)/$(HPROC)/RCS/*); )
	-@$(foreach RCSDIR, $(ALLHOST), echo check out from (RCSDIR); co -u $(wildcard $(RCSDIR)/RCS/*); )
	-@$(foreach RCSDIR, $(ALLIOS) $(STDIOS), echo check out from $(RCSDIR); co -u $(wildcard $(RCSDIR)/RCS/*); )
	-@$(foreach RCSDIR, $(RCSMISC), echo check out from $(RCSDIR); co -u $(wildcard $(RCSDIR)/RCS/*); )


#-----------------------------------------------------------------------------
# CLEANUP all directories

# Cleanup objects and temporary files

CLEANDIRS := $(ALLSRC)

clean:
	$(foreach CLEANIT, $(CLEANDIRS), $(MAKE) -C $(CLEANIT)/$(HPROC) clean; )
	$(MAKE) -C text clean

QCLEANDIRS := kernel nucleus posix util fault cmds/cc/clib cmds/shell cmds/com
quickclean:
	$(foreach CLEANIT, $(QCLEANDIRS), $(MAKE) -C $(CLEANIT)/$(HPROC) clean; )

NUCCLEANDIRS := kernel nucleus util 
nucclean:
	$(foreach CLEANIT, $(NUCCLEANDIRS), $(MAKE) -C $(CLEANIT)/$(HPROC) clean; )


# Clean up host utilities

hostclean:
	$(foreach CLEANIT, $(HOSTTOOLS), $(MAKE) -C $(CLEANIT)/$(HHOST) clean; )


# Cleanup binaries
binclean:
	-$(RM) $(HPROD)/bin/*
	-$(RM) $(HPROD)/bin/private/*
	-$(RM) $(HPROD)/lib/*
	-$(RM) $(HPROD)/local/bin/*
	-$(RM) $(HPROD)/local/lib/*
	-$(RM) $(HPROD)/local/games/*
#	-$(RM) $(HPROD)/lib/X11/*
#	-$(RM) $(HPROD)/bin/X11/*
#	-$(RM) $(HPROD)/local/bin/X11/*


# Clean out the lot! Cleanup objects, utilities, binaries
realclean: binclean hostclean clean


# Whiter than white, cleaner than clean, that Helios ultraclean (buy some today)
ifeq ($(HLICENSEE), PERIHELION)
ultraclean: realclean rcsclean
endif


#-----------------------------------------------------------------------------
# Backup utilities

# Individual components can also be backed up via their own 'tar' target. This
# tar's its entire contents, including objects and binaries (see $(HHOST).mak)

ifeq ($(HHOST), R140)
tsttar:
	@echo "Testing Helios source backup on floppies"
	dsplit | uncompress | tar tvf -
else
tsttar:
	@echo "Testing Helios source backup"
	cat /hsrc/Helios$(HPROC)src.tar.Z | uncompress | tar tvf -
endif

# Restore tar backup
ifeq ($(HHOST), R140)
gettar:
	@echo "Restoring Helios source backup from floppies"
	dsplit | uncompress | tar xvf -
else
gettar:
	@echo "Restoring Helios source backup"
	cat /hsrc/Helios$(HPROC)src.tar.Z | uncompress | tar xvf -
endif


#-----------------------------------------------------------------------------
# Backup Helios sources via tar
#
# Creates a tar tape/disk containing the Helios sources
# This requires that each makefile conforms to the Helios generic makefile
# system. i.e. they define the SOURCES and XSOURCES variables.
# The backup will only include sources, not objects, binaries or RCS files.
# afterwards 'backedupnames' will hold a list of the source files that have
# been backed up.

# include files and directories that are not referenced in any makefiles,
# or source directories that don't have a generic makefile
# Note that this will include any subdirectories they may contain.
HELPFILES	:= $(foreach HST, $(HOSTSUPPORT), cmds/help/$(HST)/dbmake \
						  cmds/help/$(HST)/stopword.lst)
OTHERFILES	:= $(HELPFILES) $(ODDFILES) $(ALLMISC) $(ALLIOS) include

# beware of host versions of Helios components - these should ONLY have a
# makefile in their directory - the generic makefile should note any extra
# files they may require.
OTHERFILES	:= $(OTHERFILES) $(foreach TOOL, $(ALLHOST), $(TOOL)/makefile)

ifeq ($(HPROC),ARM) # @@@ Bodge until generic makefile written
 OTHERFILES	:= $(OTHERFILES) cmds/cc/ncc
endif

ifdef NUC_LIC
 OTHERFILES	:= $(OTHERFILES) Embedded/esysbuild cmds/com/objed.c \
		   cmds/com/c.c
endif


#srcnames:
#	$(RM) backedupsrcs
#	$(MAKE) -C $(STDIOS) iopsrcnames
#	$(foreach SRCDIR, $(ALLSRC), $(MAKE) -C $(SRCDIR) srcnames; )

# The basic ioserver sources have a generic makefile (courtesy Alan)
# so tar them individually - also gets around problem of tar including all
# subdirectories and hence all IO Server sources wether they have a license
# for them or not!

# all directories that have a generic makefile can do intelligent back up:
srctar:
	@echo "Making Helios/$(HPROC) source backup"
	$(RM) backedupsrcs
	$(MAKE) -C $(STDIOS) iopsrcnames
			# IO Server generic sources
	$(foreach SRCDIR, $(ALLSRC), $(MAKE) -C $(SRCDIR) srcnames; )
ifeq ($(HHOST),R140)
	tar cvhlf - $(OTHERFILES) `cat backedupsrcs` | compress > /hsrc/Helios$(HPROC)src.tar.Z
#	@echo "This will require at least ten formatted floppies"
#	tar cvhlf - $(OTHERFILES) `cat backedupsrcs` | compress | dsplit
endif
ifeq ($(HHOST),HELIOSTRAN)	# HHOST = TRAN?
# Not tested + helios doesn't yet have a 'tr'!
# + may wish to use the -z compress option
# + may wish to write direct to tape
#	cat backedupsrcs | tr " " "\12" > baklist
#	tar cvhlf -  $(OTHERFILES) -T baklist | compress > /helios/tmp/hsrctar.Z
endif
ifeq ($(HHOST),SUN4)
# may wish to write direct to tape
	cat backedupsrcs | tr -s "\12 " "\12" > baklist
	tar FFcvhlf - $(OTHERFILES) -I baklist | compress > Helios$(HPROC)src.tar.Z
endif
ifeq ($(HHOST),RS6000)
	echo $(OTHERFILES) >> backedupsrcs
	cat backedupsrcs | tr -s " " "\n" > baklist
	gtar chfzT Helios$(HPROC)src.tar.Z baklist 
endif
ifeq ($(HHOST),HP)
	echo "Error HP tar cannot accept -I syntax (use a sun)"
endif

# tar everything
tarbakall:
ifeq ($(HHOST),SUN4)
	tar cvhlf /dev/rst0 .
endif
ifeq ($(HHOST),SUN386)
# @@@ not tested
	tar cvhlf /dev/rst8 .
endif
ifdef NEVER
	tar cvhlf - . | compress > Helios$(HPROC)everythingtar.Z
endif


#---------------------------------------------------------------------------
# Create local source tree
#
# Remote/local Helios source tree utility
# Make local empty source tree save for copied makefiles.
# Assumes that you have created root of source tree, copied the
# Helios top level makefile to it and are now running that makefile.

CLIST := $(ALLSRC)

MKANDCP = mkdir $(COMPDIR); mkdir $(COMPDIR)/$(HPROC); \
	cp $(RSRC)/$(COMPDIR)/makefile $(COMPDIR); \
	cp $(RSRC)/$(COMPDIR)/$(HPROC)/makefile $(COMPDIR)/$(HPROC);

makeltree:
ifndef RSRC
	@echo "\$RSRC not defined - get lost sucker - read the makefile comments"
else
	@echo "Creating local Helios source tree"
	cp -r $(RSRC)/makeinc .
	mkdir cmds cmds/cc cmds/public cmds/gnu scanlibs servers tcpip
#	mkdir cmds cmds/cc cmds/public cmds/gnu scanlibs servers demos tcpip
				 # make stub subdirs
	- $(foreach COMPDIR, $(CLIST), $(MKANDCP))
endif


#---------------------------------------------------------------------------
# C40 debug map for entire nucleus


STDKERNEL.OBJECTS = kernel/C40/kmodule.p kernel/C40/kstart.p \
		kernel/C40/queue1.p kernel/C40/sem1.p \
		kernel/C40/port1.p kernel/C40/putmsg1.p \
		kernel/C40/getmsg1.p kernel/C40/kill1.p \
		kernel/C40/link1.p kernel/C40/linkmsg1.p \
		kernel/C40/memory1.p kernel/C40/task.p \
		kernel/C40/event.p

KERNEL.OBJECTS = $(STDKERNEL.OBJECTS) kernel/C40/gexec.p \
		kernel/C40/gslice.p kernel/C40/glinkio.p \
		kernel/C40/romsupp.p kernel/C40/nccsupport.p \
		kernel/C40/c40nccsupp.p kernel/C40/c40exec.p \
		kernel/C40/c40intr.p kernel/C40/c40slice.p \
		kernel/C40/gdebug.p kernel/C40/c40linkio.p \
		kernel/C40/c40dma.p kernel/C40/kend.p


SYSLIB.OBJECTS = nucleus/syslib/C40/sysstart.p nucleus/syslib/C40/desSfn.p \
		nucleus/syslib/C40/des.p nucleus/syslib/C40/device.p \
		nucleus/syslib/C40/info.p nucleus/syslib/C40/ioc.p \
		nucleus/syslib/C40/marshal.p nucleus/syslib/C40/memory.p \
		nucleus/syslib/C40/misc.p nucleus/syslib/C40/pipe.p \
		nucleus/syslib/C40/select.p nucleus/syslib/C40/socket.p \
		nucleus/syslib/C40/stream.p nucleus/syslib/C40/task.p \
		nucleus/syslib/C40/modend.p \
		$(HPROD)/lib/kernel.def $(HPROD)/lib/util.def

SERVLIB.OBJECTS = nucleus/C40/servstart.p nucleus/C40/servlib.p \
		nucleus/C40/modend.p $(HPROD)/lib/kernel.def \
		nucleus/syslib/$(HPROC)/syslib.def $(HPROD)/lib/util.def

UTIL.OBJECTS = util/C40/utilstar.p util/C40/utilasm.p util/C40/misc.p \
		util/C40/string.p util/C40/c40string.p util/C40/modend.p \
		$(HPROD)/lib/kernel.def $(HPROD)/lib/syslib.def

BOOTSTRAP.OBJECT = kernel/C40/c40boot.p

PROCMAN.OBJECTS = nucleus/C40/sstart.o nucleus/C40/procman.o

LOADER.OBJECTS = nucleus/C40/sstart.o nucleus/C40/loader.o

ifeq ($(HPROC),C40)
	# nucleus load point + nucleus header + kernel image header
/tmp/ndebugmap: $(KERNEL.OBJECTS) $(SYSLIB.OBJECTS) $(SERVLIB.OBJECTS) \
		$(UTIL.OBJECTS) $(BOOSTRAP.OBJECT) $(PROCMAN.OBJECTS) \
		$(LOADER.OBJECTS)
	$(OBJDUMP) +12587265 +36 -d $(KERNEL.OBJECTS) \
	+4 $(SYSLIB.OBJECTS) \
	+4 $(SERVLIB.OBJECTS) \
	+4 $(UTIL.OBJECTS) \
	+4 $(BOOTSTRAP.OBJECT) \
	+4 $(PROCMAN.OBJECTS) \
	+4 $(LOADER.OBJECTS) > /tmp/ndebugmap &
endif

#-----------------------------------------------------------------------------
# Emacs Tags

Tags:
	etags -e -t nucleus/*.c nucleus/syslib/*.c kernel/*.c posix/*.c cmds/cc/clib/*.c include/*.h \
		include/sys/*.h scanlibs/bsd/*.[ch]

#----------------------------------------------- Complexity is job security...
