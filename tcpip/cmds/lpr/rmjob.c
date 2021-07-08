/*
 * Copyright (c) 1983 Regents of the University of California.
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

#ifndef __HELIOS
#ifndef lint
static char sccsid[] = "@(#)rmjob.c	5.4 (Berkeley) 2/20/89";
#endif /* not lint */
#else
static char *rcsid = "$Header: /hsrc/tcpip/cmds/lpr/RCS/rmjob.c,v 1.3 1992/02/25 10:57:16 craig Exp $";
#endif

/*
 * rmjob - remove the specified jobs from the queue.
 */

#include "lp.h"

#include <syslib.h>
#include <dirent.h>
#include "printcap.h"

/*
 * Stuff for handling lprm specifications
 */
extern char	*user[];		/* users to process */
extern int	users;			/* # of users in user array */
extern int	requ[];			/* job number of spool entries */
extern int	requests;		/* # of spool requests */
extern char	*person;		/* name of person doing lprm */

char	root[] = "root";
int	all = 0;		/* eliminate all files (root only) */

#ifndef __HELIOS
int	cur_daemon;		/* daemon's pid */
#else
char	cur_daemon_name [256];	/* daemon's name */
#endif

#ifndef __HELIOS
char	current[40];		/* active control file name */
#else
char	cur_file[40];		/* active control file name */
#endif

#ifndef __HELIOS
int	iscf();
#endif

int lockchk (char *) ;
void process (char *) ;
int chk (char *) ;
int isowner (char *, char *) ;
void chkremote (void) ;

void rmjob()
{
  register int i, nitems;
  int assasinated = 0;
  struct direct **files;

  extern int startdaemon (char *) ;

  if ((i = pgetent(line, printer)) < 0)
    fatal("cannot open printer description file");
  else if (i == 0)
    fatal("unknown printer");
  if ((SD = pgetstr("sd", &bp)) == NULL)
    SD = DEFSPOOL;
  if ((LO = pgetstr("lo", &bp)) == NULL)
    LO = DEFLOCK;
  if ((LP = pgetstr("lp", &bp)) == NULL)
    LP = DEFDEVLP;
  if ((RP = pgetstr("rp", &bp)) == NULL)
    RP = DEFLP;
  RM = pgetstr("rm", &bp);

  /*
   * Figure out whether the local machine is the same as the remote 
   * machine entry (if it exists).  If not, then ignore the local
   * queue information.
   */

   if (RM != (char *) NULL) {
    char name[256];
    struct hostent *hp;

    /* get the standard network name of the local host */
    gethostname(name, sizeof(name));
    name[sizeof(name)-1] = '\0';
    hp = gethostbyname(name);
    if (hp == (struct hostent *) NULL) {
#ifndef __HELIOS
        printf("unable to get network name for local machine %s",
#else
        printf("%s %s\n", ERR_LOCAL_NAME,
#endif
      name);
        goto localcheck_done;
    } else strcpy(name, hp->h_name);

    /* get the standard network name of RM */
    hp = gethostbyname(RM);
    if (hp == (struct hostent *) NULL) {
#ifndef __HELIOS
        printf("unable to get hostname for remote machine %s", RM);
#else
        printf("%s %s\n", ERR_REMOTE_NAME, RM);
#endif
        goto localcheck_done;
    }

    /* if printer is not on local machine, ignore LP */
    if (strcmp(name, hp->h_name) != 0) *LP = '\0';
  }
localcheck_done:

  /*
   * If the format was `lprm -' and the user isn't the super-user,
   *  then fake things to look like he said `lprm user'.
   */
  if (users < 0) {
    if (getuid() == 0)
      all = 1;  /* all files in local queue */
    else {
      user[0] = person;
      users = 1;
    }
  }
  if (!strcmp(person, "-all")) {
    if (from == host)
      fatal("The login name \"-all\" is reserved");
    all = 1;  /* all those from 'from' */
    person = root;
  }

  if (chdir(SD) < 0)
    fatal("cannot chdir to spool directory");
#ifndef __HELIOS
  if ((nitems = scandir(".", &files, iscf, NULL)) < 0)
    fatal("cannot access spool directory");
#else
  if ((nitems = scan_dir (SD, &files, &iscf, NULL)) < 0)
    fatal("cannot access spool directory");
#endif    

  if (nitems) {
    /*
     * Check for an active printer daemon (in which case we
     *  kill it if it is reading our file) then remove stuff
     *  (after which we have to restart the daemon).
     */
#ifndef __HELIOS     
    if (lockchk(LO) && chk(current)) {
      assasinated = kill(cur_daemon, SIGINT) == 0;
      if (!assasinated)
        fatal("cannot kill printer daemon");
    }
#else
    debugf ("rmjob: entering lockchk() && chk()") ;
    if (lockchk(LO) && chk(cur_file))
    {  
      assasinated = 
#ifdef OLDCODE
      (kill_task (cur_daemon_name, SIGINT) >= 0) ;
#else
      (kill_task (cur_daemon_name) >= 0) ;
#endif      
      if (!assasinated)
      {
        debugf ("rmjob: kill_task failed") ;
        fatal("cannot kill printer daemon");
      }
    }
#endif

#ifdef OLDCODE
    if (lockchk(LO) && chk(cur_file)) {
      debugf ("rmjob: daemon should be killed here") ;
/*
-- crf : take the easy way out ... i.e. pretend that assasination failed
*/
      assasinated = 0 ;
      if (!assasinated)
        fatal("cannot kill printer daemon");
    }
#endif /* OLDCODE */

    /*
     * process the files
     */
    for (i = 0; i < nitems; i++)
      process(files[i]->d_name);
#ifdef __HELIOS
    debugf ("rmjob: scan_freeing") ;        
    if (scan_free (files) < 1)
      fatal ("error freeing memory") ;
#endif
  }

  chkremote();
  /*
   * Restart the printer daemon if it was killed
   */
  if (assasinated && !startdaemon(printer))
    fatal("cannot restart printer daemon\n");
  exit(0);
}

/*
 * Process a lock file: collect the pid of the active
 *  daemon and the file name of the active spool entry.
 * Return boolean indicating existence of a lock file.
 */
int lockchk(char *s)
{
  register FILE *fp;
  register int i, n;

#ifndef __HELIOS
  if ((fp = fopen(s, "r")) == NULL)
    if (errno == EACCES)
      fatal("can't access lock file");
    else
      return(0);
#else
/*
-- crf : dangling else ...
*/
  if ((fp = fopen(s, "r")) == NULL)
  {
    if (errno == EACCES)
      fatal("can't access lock file");
    else
      return(0);
  }
#endif      
  
  if (!getline(fp)) {
    (void) fclose(fp);
    return(0);    /* no daemon present */
  }


#ifndef __HELIOS
  cur_daemon = atoi(line);
  if (kill(cur_daemon, 0) < 0) {
    (void) fclose(fp);
    return(0);    /* no daemon present */
  }
#else
  strcpy (cur_daemon_name, line) ;
  debugf ("daemon name = %s", cur_daemon_name) ;  
  if (Locate (NULL, cur_daemon_name) == NULL) 
  {
    (void) fclose(fp);
    return(0);    /* no daemon present */
  }
#endif

#ifndef __HELIOS
  for (i = 1; (n = fread(current, sizeof(char), sizeof(current), fp)) <= 0; i++) {
#else
  for (  i = 1; 
         (n = fread(cur_file, sizeof(char), sizeof(cur_file), fp)) <= 0; 
         i++
       ) {
#endif    
    if (i > 5) {
      n = 1;
      break;
    }
    sleep(i);
  }
#ifndef __HELIOS
  current[n-1] = '\0';
#else
  cur_file[n-1] = '\0';
#endif  
  (void) fclose(fp);
  return(1);
}

/*
 * Process a control file.
 */
void process(char *file)
{
  FILE *cfp;

  if (!chk(file))
    return;
  if ((cfp = fopen(file, "r")) == NULL)
    fatal("cannot open %s", file);
  while (getline(cfp)) {
    switch (line[0]) {
    case 'U':  /* unlink associated files */
      if (from != host)
        printf("%s: ", host);
      printf(unlink(line+1) ? "cannot dequeue %s\n" :
        "%s dequeued\n", line+1);
    }
  }
  (void) fclose(cfp);
  if (from != host)
    printf("%s: ", host);
  printf(unlink(file) ? "cannot dequeue %s\n" : "%s dequeued\n", file);
}

/*
 * Do the dirty work in checking
 */
int chk(char *file)
{
  register int *r, n;
  register char **u, *cp;
  FILE *cfp;

  /*
   * Check for valid cf file name (mostly checking current).
   */
  if (strlen(file) < 7 || file[0] != 'c' || file[1] != 'f')
    return(0);

  if (all && (from == host || !strcmp(from, file+6)))
    return(1);

  /*
   * get the owner's name from the control file.
   */
  if ((cfp = fopen(file, "r")) == NULL)
    return(0);
  while (getline(cfp)) {
    if (line[0] == 'P')
      break;
  }
  (void) fclose(cfp);
  if (line[0] != 'P')
    return(0);

  if (users == 0 && requests == 0)
#ifndef __HELIOS
    return(!strcmp(file, current) && isowner(line+1, file));
#else
    return(!strcmp(file, cur_file) && isowner(line+1, file));
#endif    
  /*
   * Check the request list
   */
  for (n = 0, cp = file+3; isdigit(*cp); )
    n = n * 10 + (*cp++ - '0');
  for (r = requ; r < &requ[requests]; r++)
    if (*r == n && isowner(line+1, file))
      return(1);
  /*
   * Check to see if it's in the user list
   */
  for (u = user; u < &user[users]; u++)
    if (!strcmp(*u, line+1) && isowner(line+1, file))
      return(1);
  return(0);
}

/*
 * If root is removing a file on the local machine, allow it.
 * If root is removing a file from a remote machine, only allow
 * files sent from the remote machine to be removed.
 * Normal users can only remove the file from where it was sent.
 */

int isowner(char *owner, char *file)
{
#ifndef __HELIOS
  if (!strcmp(person, root) && (from == host || !strcmp(from, file+6)))
#else
  if (
       (!strcmp(person, root)) && 
       (from == host || !strncmp(from, file+6, 2))
     )
#endif  
    return(1);
#ifndef __HELIOS
  if (!strcmp(person, owner) && !strcmp(from, file+6))
#else
  if (
       (!strcmp(person, owner)) && 
       (!strncmp(from, file+6, 2))
     )
#endif  
    return(1);
  if (from != host)
    printf("%s: ", host);
  printf("%s: Permission denied\n", file);
  return(0);
}

/*
 * Check to see if we are sending files to a remote machine. If we are,
 * then try removing files on the remote machine.
 */
void chkremote()
{
  register char *cp;
  register int i, rem;
  char buf[BUFSIZ];

/*
-- crf: force remote operation
*/
#ifndef __HELIOS
  if (*LP || RM == NULL)
    return;  /* not sending to a remote machine */
#endif

  /*
   * Flush stdout so the user can see what has been deleted
   * while we wait (possibly) for the connection.
   */
  fflush(stdout);

  sprintf(buf, "\5%s %s", RP, all ? "-all" : person);
  cp = buf;
  for (i = 0; i < users; i++) {
    cp += strlen(cp);
    *cp++ = ' ';
    strcpy(cp, user[i]);
  }
  for (i = 0; i < requests; i++) {
    cp += strlen(cp);
    (void) sprintf(cp, " %d", requ[i]);
  }
  strcat(cp, "\n");
  rem = getport(RM);
  if (rem < 0) {
    if (from != host)
      printf("%s: ", host);
    printf("connection to %s is down\n", RM);
  } else {
    i = strlen(buf);
    if (write(rem, buf, i) != i)
      fatal("Lost connection");
    while ((i = read(rem, buf, sizeof(buf))) > 0)
      (void) fwrite(buf, 1, i, stdout);
    (void) close(rem);
  }
}

/*
-- crf : iscf() is now in common.c
*/

#ifndef __HELIOS
/*
 * Return 1 if the filename begins with 'cf'
 */
iscf(d)
  struct direct *d;
{
  return(d->d_name[0] == 'c' && d->d_name[1] == 'f');
}
#endif
