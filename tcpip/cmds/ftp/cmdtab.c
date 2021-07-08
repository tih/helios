/*
 * Copyright (c) 1985, 1989 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifdef lint
static char sccsid[] = "@(#)cmdtab.c	5.8.1.2 (Berkeley) 3/1/89";
#endif /* not lint */

#include "ftp_var.h"

/*
 * User FTP -- Command Tables.
 */
int	setascii(), setbell(), setbinary(), setdebug(), setform();
int	setglob(), sethash(), setmode(), setpeer(), setport();
int	setprompt(), setstruct();
int	settenex(), settrace(), settype(), setverbose();
int	disconnect(), syst();
int	cd(), lcd(), MyDelete(), mdelete(), user();
int	ls(), mls(), get(), mget(), help(), append(), put(), mput();
int	quit(), renamefile(), status();
int	quote(), rmthelp(), shell();
int	pwd(), makedir(), removedir(), setcr();
int	account(), doproxy(), reset(), setcase(), setntrans(), setnmap();
int	setsunique(), setrunique(), cdup(), macdef(), domacro();
int	sizecmd(), modtime(), rmtstatus();

char	accounthelp[] =	"send account command to remote server";
char	appendhelp[] =	"append to a file";
char	asciihelp[] =	"set ascii transfer type";
char	beephelp[] =	"beep when command completed";
char	binaryhelp[] =	"set binary transfer type";
char	casehelp[] =	"toggle mget upper/lower case id mapping";
char	cdhelp[] =	"change remote working directory";
char	cduphelp[] = 	"change remote working directory to parent directory";
char	connecthelp[] =	"connect to remote tftp";
char	crhelp[] =	"toggle carriage return stripping on ascii gets";
char	deletehelp[] =	"delete remote file";
char	debughelp[] =	"toggle/set debugging mode";
char	dirhelp[] =	"list contents of remote directory";
char	disconhelp[] =	"terminate ftp session";
char	domachelp[] = 	"execute macro";
char	formhelp[] =	"set file transfer format";
char	globhelp[] =	"toggle metacharacter expansion of local file names";
char	hashhelp[] =	"toggle printing `#' for each buffer transferred";
char	helphelp[] =	"print local help information";
char	lcdhelp[] =	"change local working directory";
char	lshelp[] =	"list contents of remote directory";
char	macdefhelp[] =  "define a macro";
char	mdeletehelp[] =	"delete multiple files";
char	mdirhelp[] =	"list contents of multiple remote directories";
char	mgethelp[] =	"get multiple files";
char	mkdirhelp[] =	"make directory on the remote machine";
char	mlshelp[] =	"nlist contents of multiple remote directories";
char	modtimehelp[] = "show last modification time of remote file";
char	modehelp[] =	"set file transfer mode";
char	mputhelp[] =	"send multiple files";
char	nlisthelp[] =	"nlist contents of remote directory";
char	nmaphelp[] =	"set templates for default file name mapping";
char	ntranshelp[] =	"set translation table for default file name mapping";
char	porthelp[] =	"toggle use of PORT cmd for each data connection";
char	prompthelp[] =	"force interactive prompting on multiple commands";
char	proxyhelp[] =	"issue command on alternate connection";
char	pwdhelp[] =	"print working directory on remote machine";
char	quithelp[] =	"terminate ftp session and exit";
char	quotehelp[] =	"send arbitrary ftp command";
char	receivehelp[] =	"receive file";
char	remotehelp[] =	"get help from remote server";
char	renamehelp[] =	"rename file";
char	rmdirhelp[] =	"remove directory on the remote machine";
char	rmtstatushelp[]="show status of remote machine";
char	runiquehelp[] = "toggle store unique for local files";
char	resethelp[] =	"clear queued command replies";
char	sendhelp[] =	"send one file";
char	shellhelp[] =	"escape to the shell";
char	sizecmdhelp[] = "show size of remote file";
char	statushelp[] =	"show current status";
char	structhelp[] =	"set file transfer structure";
char	suniquehelp[] = "toggle store unique on remote machine";
char	systemhelp[] =  "show remote system type";
char	tenexhelp[] =	"set tenex file transfer type";
char	tracehelp[] =	"toggle packet tracing";
char	typehelp[] =	"set file transfer type";
char	userhelp[] =	"send new user information";
char	verbosehelp[] =	"toggle verbose mode";

struct cmd cmdtab[] = {
	{ "!",		shellhelp,	0,	0,	0,	shell },
	{ "$",		domachelp,	1,	0,	0,	domacro },
	{ "account",	accounthelp,	0,	1,	1,	account},
	{ "append",	appendhelp,	1,	1,	1,	put },
	{ "ascii",	asciihelp,	0,	1,	1,	setascii },
	{ "bell",	beephelp,	0,	0,	0,	setbell },
	{ "binary",	binaryhelp,	0,	1,	1,	setbinary },
	{ "bye",	quithelp,	0,	0,	0,	quit },
	{ "case",	casehelp,	0,	0,	1,	setcase },
	{ "cd",		cdhelp,		0,	1,	1,	cd },
	{ "cdup",	cduphelp,	0,	1,	1,	cdup },
	{ "close",	disconhelp,	0,	1,	1,	disconnect },
	{ "cr",		crhelp,		0,	0,	0,	setcr },
	{ "delete",	deletehelp,	0,	1,	1,	MyDelete },
	{ "debug",	debughelp,	0,	0,	0,	setdebug },
	{ "dir",	dirhelp,	1,	1,	1,	ls },
	{ "disconnect",	disconhelp,	0,	1,	1,	disconnect },
	{ "form",	formhelp,	0,	1,	1,	setform },
	{ "get",	receivehelp,	1,	1,	1,	get },
	{ "glob",	globhelp,	0,	0,	0,	setglob },
	{ "hash",	hashhelp,	0,	0,	0,	sethash },
	{ "help",	helphelp,	0,	0,	1,	help },
	{ "image",	binaryhelp,	0,	1,	1,	setbinary },
	{ "lcd",	lcdhelp,	0,	0,	0,	lcd },
	{ "ls",		lshelp,		1,	1,	1,	ls },
	{ "macdef",	macdefhelp,	0,	0,	0,	macdef },
	{ "mdelete",	mdeletehelp,	1,	1,	1,	mdelete },
	{ "mdir",	mdirhelp,	1,	1,	1,	mls },
	{ "mget",	mgethelp,	1,	1,	1,	mget },
	{ "mkdir",	mkdirhelp,	0,	1,	1,	makedir },
	{ "mls",	mlshelp,	1,	1,	1,	mls },
	{ "mode",	modehelp,	0,	1,	1,	setmode },
	{ "modtime",	modtimehelp,	0,	1,	1,	modtime },
	{ "mput",	mputhelp,	1,	1,	1,	mput },
	{ "nmap",	nmaphelp,	0,	0,	1,	setnmap },
	{ "nlist",	nlisthelp,	1,	1,	1,	ls },
	{ "ntrans",	ntranshelp,	0,	0,	1,	setntrans },
	{ "open",	connecthelp,	0,	0,	1,	setpeer },
	{ "prompt",	prompthelp,	0,	0,	0,	setprompt },
	{ "proxy",	proxyhelp,	0,	0,	1,	doproxy },
	{ "sendport",	porthelp,	0,	0,	0,	setport },
	{ "put",	sendhelp,	1,	1,	1,	put },
	{ "pwd",	pwdhelp,	0,	1,	1,	pwd },
	{ "quit",	quithelp,	0,	0,	0,	quit },
	{ "quote",	quotehelp,	1,	1,	1,	quote },
	{ "recv",	receivehelp,	1,	1,	1,	get },
	{ "remotehelp",	remotehelp,	0,	1,	1,	rmthelp },
	{ "rstatus",	rmtstatushelp,	0,	1,	1,	rmtstatus },
	{ "rhelp",	remotehelp,	0,	1,	1,	rmthelp },
	{ "rename",	renamehelp,	0,	1,	1,	renamefile },
	{ "reset",	resethelp,	0,	1,	1,	reset },
	{ "rmdir",	rmdirhelp,	0,	1,	1,	removedir },
	{ "runique",	runiquehelp,	0,	0,	1,	setrunique },
	{ "send",	sendhelp,	1,	1,	1,	put },
	{ "size",	sizecmdhelp,	1,	1,	1,	sizecmd },
	{ "status",	statushelp,	0,	0,	1,	status },
	{ "struct",	structhelp,	0,	1,	1,	setstruct },
	{ "sunique",	suniquehelp,	0,	0,	1,	setsunique },
	{ "system",	systemhelp,	0,	1,	1,	syst },
	{ "tenex",	tenexhelp,	0,	1,	1,	settenex },
	{ "trace",	tracehelp,	0,	0,	0,	settrace },
	{ "type",	typehelp,	0,	1,	1,	settype },
	{ "user",	userhelp,	0,	1,	1,	user },
	{ "verbose",	verbosehelp,	0,	0,	0,	setverbose },
	{ "?",		helphelp,	0,	0,	1,	help },
	{ 0 },
};

int	NCMDS = (sizeof (cmdtab) / sizeof (cmdtab[0])) - 1;
