/*
 * Copyright (c) 1983 The Regents of the University of California.
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
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1983 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifdef lint
static char sccsid[] = "@(#)rexecd.c	5.7 (Berkeley) 1/4/89";
#endif /* not lint */

#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <netinet/in.h>

#include <stdio.h>
#include <errno.h>
#include <pwd.h>
#include <signal.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <sys/wait.h>
#include <posix.h>

#ifdef __HELIOS
#include <process.h>
#endif
  
#ifdef NEW_SYSTEM
#include <bsd.h>
#endif
  
extern	errno;

/*
 * remote execute server:
 *	username\0
 *	password\0
 *	command\0
 *	data
 */
/*ARGSUSED*/
char	username[20] = "USER=";
char	homedir[64] = "HOME=";
char	shell[64] = "SHELL=";
char	*envinit[] =
	    {homedir, shell, "PATH=:/usr/ucb:/bin:/usr/bin", username, 0};

#ifdef __HELIOS
int pid;
void do_stderr(int s, int pv0);
#endif

struct	sockaddr_in asin = { AF_INET };

void
error( char * fmt, ... )
{
  char 		buf[BUFSIZ];
  va_list	args;

  
  buf[0] = 1;

  va_start( args, fmt );
  
  (void) vsprintf( buf + 1, fmt, args );

  va_end( args );
  
  (void) write( 2, buf, strlen( buf ) );
}

int
getstr(
	char *buf,
	int cnt,
	char *err )
{
	char c;

	do {
		if (read(0, &c, 1) != 1)
			exit(1);
		*buf++ = c;
		if (--cnt == 0) {
			error("%s too long\n", err,0,0);
			exit(1);
		}
	} while (c != 0);
}

void
doit(f, fromp)
	int f;
	struct sockaddr_in *fromp;
{
	static char cmdbuf[NCARGS+1];
	char *cp, *namep;
	char user[16], pass[16];
	struct passwd *pwd;
	int s;
	u_short port;
	int pv[2];
#ifndef __HELIOS
	int ready, readfrom, cc;
	static char buf[BUFSIZ];
	char sig;
#endif	
	int one = 1;

	(void) signal(SIGINT, SIG_DFL);
	(void) signal(SIGQUIT, SIG_DFL);
	(void) signal(SIGTERM, SIG_DFL);
#ifdef DEBUG
	{ int t = open("/dev/tty", 2);
	  if (t >= 0) {
		ioctl(t, TIOCNOTTY, (char *)0);
		(void) close(t);
	  }
	}
#endif
	dup2(f, 0);
	dup2(f, 1);
	dup2(f, 2);
	(void) alarm(60);
	port = 0;
	for (;;) {
		char c;
		if (read(f, &c, 1) != 1)
			exit(1);
		if (c == 0)
			break;
		port = port * 10 + c - '0';
	}
	(void) alarm(0);
	if (port != 0) {
		s = socket(AF_INET, SOCK_STREAM, 0);
		if (s < 0)
			exit(1);
		if (bind(s, (struct sockaddr *)&asin, sizeof (asin)) < 0)
			exit(1);
		(void) alarm(60);
		fromp->sin_port = htons(port);
		if (connect(s, (struct sockaddr *)fromp, sizeof (*fromp)) < 0)
			exit(1);
		(void) alarm(0);
	}
	getstr(user, sizeof(user), "username");
	getstr(pass, sizeof(pass), "password");
	getstr(cmdbuf, sizeof(cmdbuf), "command");
	setpwent();
	pwd = getpwnam(user);
	if (pwd == NULL) {
		error("Login incorrect.\n");
		exit(1);
	}
	endpwent();
	if (*pwd->pw_passwd != '\0') {
#ifdef __HELIOS
		namep = pass;
#else
		namep = crypt(pass, pwd->pw_passwd);
#endif
		if (strcmp(namep, pwd->pw_passwd)) {
			error("Password incorrect.\n");
			exit(1);
		}
	}
	if (chdir(pwd->pw_dir) < 0) {
		error("No remote directory.\n");
		exit(1);
	}
	(void) write(2, "\0", 1);
#ifdef __HELIOS
	if( port )
	{
		(void) pipe(pv);
		Fork(2000,do_stderr,2*sizeof(int),s,pv[0]);
		ioctl(pv[1], FIONBIO, (char *)&one);
		dup2(pv[1], 2);
	}
#else
	if (port) {
		(void) pipe(pv);
		pid = fork();
		if (pid == -1)  {
			error("Try again.\n");
			exit(1);
		}
		if (pid) {
			(void) close(0); (void) close(1); (void) close(2);
			(void) close(f); (void) close(pv[1]);
			readfrom = (1<<s) | (1<<pv[0]);
			ioctl(pv[1], FIONBIO, (char *)&one);
			/* should set s nbio! */
			do {
				ready = readfrom;
				(void) select(16, &ready, (fd_set *)0,
				    (fd_set *)0, (struct timeval *)0);
				if (ready & (1<<s)) {
					if (read(s, &sig, 1) <= 0)
						readfrom &= ~(1<<s);
					else
						killpg(pid, sig);
				}
				if (ready & (1<<pv[0])) {
					cc = read(pv[0], buf, sizeof (buf));
					if (cc <= 0) {
						shutdown(s, 1+1);
						readfrom &= ~(1<<pv[0]);
					} else
						(void) write(s, buf, cc);
				}
			} while (readfrom);
			exit(0);
		}
		setpgrp(0, getpid());
		(void) close(s); (void)close(pv[0]);
		dup2(pv[1], 2);
	}
#endif
	if (*pwd->pw_shell == '\0')
		pwd->pw_shell = "/bin/sh";
	if (f > 2)
		(void) close(f);
	(void) setgid((gid_t)pwd->pw_gid);
	initgroups(pwd->pw_name, pwd->pw_gid);
	(void) setuid((uid_t)pwd->pw_uid);
#ifndef __HELIOS
	environ = envinit;
#endif
	strncat(homedir, pwd->pw_dir, sizeof(homedir)-6);
	strncat(shell, pwd->pw_shell, sizeof(shell)-7);
	strncat(username, pwd->pw_name, sizeof(username)-6);
	cp = rindex(pwd->pw_shell, '/');
	if (cp)
		cp++;
	else
		cp = pwd->pw_shell;
#ifdef __HELIOS
	if((pid = vfork()) == 0 )
	{
		close(pv[0]);
		close(pv[1]);
		close(3);
		execle(pwd->pw_shell, cp, "-c", cmdbuf, 0, envinit);
		perror(pwd->pw_shell);
		_exit(1);		
	}
	wait(NULL);
#else
	execl(pwd->pw_shell, cp, "-c", cmdbuf, 0);
	perror(pwd->pw_shell);
	exit(1);
#endif
}

#ifdef __HELIOS

void do_stderr(int s, int pv0)
{
	int readfrom, ready;
	char sig;
	static char buf[BUFSIZ];
	int cc;
		
	readfrom = (1<<s) | (1<<pv0);
			
	/* should set s nbio! */
	do {
		ready = readfrom;
		(void) select(16, (int *)&ready, (int *)0,
		    (int *)0, (struct timeval *)0);
		if (ready & (1<<s)) {
			if (read(s, &sig, 1) <= 0)
				readfrom &= ~(1<<s);
			else
				kill(-pid, sig);
		}
		if (ready & (1<<pv0)) {
			cc = read(pv0, buf, sizeof (buf));
			if (cc <= 0) {
				shutdown(s, 1+1);
				readfrom &= ~(1<<pv0);
			} else
				(void) write(s, buf, cc);
		}
	} while (readfrom);
}
#endif

int
main(argc, argv)
	int argc;
	char **argv;
{
	struct sockaddr_in from;
	int fromlen;

	fromlen = sizeof (from);
	if (getpeername(0, (struct sockaddr *)&from, &fromlen) < 0) {
		fprintf(stderr, "%s: ", argv[0]);
		perror("getpeername");
		exit(1);
	}
	doit(0, &from);
}

