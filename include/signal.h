
/* WARNING: this file is generated automatically from the master codes	*/
/* database, any changes here will be overwritten.			*/

#ifndef __signal_h
#define __signal_h

typedef unsigned long   sigset_t;

struct sigaction {
        void            (*sa_handler)();
        sigset_t        sa_mask;
        int             sa_flags;
};

/* flags */

#ifndef _POSIX_SOURCE
#define SA_SETSIG       1       /* handler set by signal()                */
#define SA_ASYNC        2       /* signal can be delivered asynchronously */
#endif

#define SA_NOCLDSTOP    4       /* do not generate SIGCHLD                */

extern void _ignore_signal_handler(int);
extern void _default_signal_handler(int);
extern void _error_signal_marker(int);

#define SIG_IGN _ignore_signal_handler
#define SIG_DFL _default_signal_handler
#define SIG_ERR _error_signal_marker

#define	SIGZERO		    0		/* no signal			*/
#define	SIGABRT		    1		/* abort			*/
#define	SIGFPE		    2		/* arithmetic exception		*/
#define	SIGILL		    3		/* illegal instruction		*/
#define	SIGINT		    4		/* attention from user		*/
#define	SIGSEGV		    5		/* bad memory access		*/
#define	SIGTERM		    6		/* termination request		*/
#define	SIGSTAK		    7		/* stack overflow		*/
#define	SIGALRM		    8		/* alarm/timeout signal		*/
#define	SIGHUP		    9		/* hangup			*/
#define	SIGPIPE		    10		/* pipe signal			*/
#define	SIGQUIT		    11		/* interactive termination	*/
#define	SIGTRAP		    12		/* trace trap			*/
#define	SIGUSR1		    13		/* user 1			*/
#define	SIGUSR2		    14		/* user 2			*/
#define	SIGCHLD		    15		/* child termination		*/
#define	SIGURG		    16		/* urgent data available	*/
#define	SIGCONT		    17		/* continue			*/
#define	SIGSTOP		    18		/* stop				*/
#define	SIGTSTP		    19		/* interactive stop		*/
#define	SIGTTIN		    20		/* background read		*/
#define	SIGTTOU		    21		/* background write		*/
#define	SIGWINCH	    22		/* window changed		*/
#define	SIGSIB		    23		/* sibling crashed		*/
#define	SIGKILL		    31		/* termination			*/

#ifndef _POSIX_SOURCE
#define NSIG            32
#endif

/* codes for sigprocmask */
#define SIG_BLOCK       1
#define SIG_UNBLOCK     2
#define SIG_SETMASK     3


extern int              kill(int pid, int sig);
extern int              siginitset(sigset_t *set);
extern int              sigfillset(sigset_t *set);
extern int              sigaddset(sigset_t *set, int sig);
extern int              sigdelset(sigset_t *set, int sig);
extern int              sigismember(sigset_t *set, int sig);
extern int              sigaction(int sig, struct sigaction *act, struct sigaction *oact);
extern int              sigprocmask( int how, sigset_t *set, sigset_t *oset);
extern int              sigpending(sigset_t *set);
extern int              sigsuspend(sigset_t *set);
extern unsigned int     alarm(unsigned int sec);
extern int              pause(void);
extern unsigned int     sleep(unsigned int seconds);
extern void             (*signal (int sig, void (*func)(int)))(int);
extern int              raise(int sig);

#ifdef _BSD
/* BSD compatability */

struct sigvec
{
        int     (*sv_handler)();
        int     sv_mask;
        int     sv_onstack;
};

struct sigstack
{
        char *  ss_sp;
        int     ss_onstack;
};

#define sigmask(x) (1U<<(x))

extern int sigvec(int sig, struct sigvec *vec, struct sigvec *ovec);
extern int sigblock(int mask);
extern int sigsetmask(int mask);
extern int sigpause(int mask);
extern int sigstack(struct sigstack *ss,struct sigstack *oss);

extern char *sys_siglist[];

#endif /*_BSD*/

#endif

/*end of signal.h 	    		   				*/

