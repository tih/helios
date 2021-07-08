/*
 * Copyright (c) 1981 Regents of the University of California.
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
 *
 *	@(#)curses.h	5.4 (Berkeley) 6/30/88
 */

#ifndef WINDOW

# ifdef __HELIOS
#  define __system_io		/* to access FILE structure pos field */
#  define sysbase void
#  define FILEHANDLE void *
# endif

# include	<stdio.h>
# include	<sgtty.h>
# include	<termcap.h>

# define	bool	char
# define	reg	register

# ifndef TRUE
#  define	TRUE	(1)
#  define	FALSE	(0)
# endif

# define	ERR	(0)
# define	OK	(1)

# define	_ENDLINE	001
# define	_FULLWIN	002
# define	_SCROLLWIN	004
# define	_FLUSH		010
# define	_FULLLINE	020
# define	_IDLINE		040
# define	_STANDOUT	0200
# define	_NOCHANGE	-1

# define	_puts(s)	tputs(s, 0, _putchar)

typedef	struct sgttyb	SGTTY;

/*
 * Capabilities from termcap
 */

extern bool     AM, BS, CA, DA, DB, EO, HC, HZ, IN, MI, MS, NC, NS, OS, UL,
		XB, XN, XT, XS, XX;
extern char	*AL, *BC, *BT, *CD, *CE, *CL, *CM, *CR, *CS, *DC, *DL,
		*DM, *DO, *ED, *EI, *K0, *K1, *K2, *K3, *K4, *K5, *K6,
		*K7, *K8, *K9, *HO, *IC, *IM, *IP, *KD, *KE, *KH, *KL,
		*KR, *KS, *KU, *LL, *MA, *ND, *NL, *RC, *SC, *SE, *SF,
		*SO, *SR, *TA, *TE, *TI, *UC, *UE, *UP, *US, *VB, *VS,
		*VE, *AL_PARM, *DL_PARM, *UP_PARM, *DOWN_PARM,
		*LEFT_PARM, *RIGHT_PARM;
extern char	PC;

/*
 * From the tty modes...
 */

extern bool	GT, NONL, UPPERCASE, normtty, _pfast;

struct _win_st {
	short		_cury, _curx;
	short		_maxy, _maxx;
	short		_begy, _begx;
	short		_flags;
	short		_ch_off;
	bool		_clear;
	bool		_leave;
	bool		_scroll;
	char		**_y;
	short		*_firstch;
	short		*_lastch;
	struct _win_st	*_nextp, *_orig;
};

# define	WINDOW	struct _win_st

extern bool	My_term, _echoit, _rawmode, _endwin;

extern char	*Def_term, ttytype[];

extern int	LINES, COLS, _tty_ch, _res_flg;

extern SGTTY	_tty;

extern WINDOW	*stdscr, *curscr;

/*
 *	Define VOID to stop lint from generating "null effect"
 * comments.
 */
# ifdef lint
int	__void__;
# define	VOID(x)	(__void__ = (int) (x))
# else
# define	VOID(x)	(x)
# endif

/*
 * psuedo functions for standard screen
 */
# define	addch(ch)	VOID(waddch(stdscr, ch))
# define	getch()		VOID(wgetch(stdscr))
# define	addbytes(da,co)	VOID(waddbytes(stdscr, da,co))
# define	addstr(str)	VOID(waddbytes(stdscr, str, strlen(str)))
# define	getstr(str)	VOID(wgetstr(stdscr, str))
# define	move(y, x)	VOID(wmove(stdscr, y, x))
# define	clear()		VOID(wclear(stdscr))
# define	erase()		VOID(werase(stdscr))
# define	clrtobot()	VOID(wclrtobot(stdscr))
# define	clrtoeol()	VOID(wclrtoeol(stdscr))
# define	insertln()	VOID(winsertln(stdscr))
# define	deleteln()	VOID(wdeleteln(stdscr))
# define	refresh()	VOID(wrefresh(stdscr))
# define	inch()		VOID(winch(stdscr))
# define	insch(c)	VOID(winsch(stdscr,c))
# define	delch()		VOID(wdelch(stdscr))
# define	standout()	VOID(wstandout(stdscr))
# define	standend()	VOID(wstandend(stdscr))

/*
 * mv functions
 */
# define	mvwaddch(win,y,x,ch)	VOID(wmove(win,y,x)==ERR?ERR:waddch(win,ch))
# define	mvwgetch(win,y,x)	VOID(wmove(win,y,x)==ERR?ERR:wgetch(win))
# define	mvwaddbytes(win,y,x,da,co) \
		VOID(wmove(win,y,x)==ERR?ERR:waddbytes(win,da,co))
# define	mvwaddstr(win,y,x,str) \
		VOID(wmove(win,y,x)==ERR?ERR:waddbytes(win,str,strlen(str)))
# define 	mvwgetstr(win,y,x,str)  VOID(wmove(win,y,x)==ERR?ERR:wgetstr(win,str))
# define	mvwinch(win,y,x)	VOID(wmove(win,y,x) == ERR ? ERR : winch(win))
# define	mvwdelch(win,y,x)	VOID(wmove(win,y,x) == ERR ? ERR : wdelch(win))
# define	mvwinsch(win,y,x,c)	VOID(wmove(win,y,x) == ERR ? ERR:winsch(win,c))
# define	mvaddch(y,x,ch)		mvwaddch(stdscr,y,x,ch)
# define	mvgetch(y,x)		mvwgetch(stdscr,y,x)
# define	mvaddbytes(y,x,da,co)	mvwaddbytes(stdscr,y,x,da,co)
# define	mvaddstr(y,x,str)	mvwaddstr(stdscr,y,x,str)
# define 	mvgetstr(y,x,str)       mvwgetstr(stdscr,y,x,str)
# define	mvinch(y,x)		mvwinch(stdscr,y,x)
# define	mvdelch(y,x)		mvwdelch(stdscr,y,x)
# define	mvinsch(y,x,c)		mvwinsch(stdscr,y,x,c)

/*
 * psuedo functions
 */

# define	clearok(win,bf)	 (win->_clear = bf)
# define	leaveok(win,bf)	 (win->_leave = bf)
# define	scrollok(win,bf) (win->_scroll = bf)
# define	flushok(win,bf)	 (bf ? (win->_flags |= _FLUSH):(win->_flags &= ~_FLUSH))
# define	getyx(win,y,x)	 y = win->_cury, x = win->_curx
# define	winch(win)	 (win->_y[win->_cury][win->_curx] & 0177)

# define raw()	 	(_tty.sg_flags|=RAW, _pfast=_rawmode=TRUE, stty(_tty_ch,&_tty))
# define noraw()	(_tty.sg_flags&=~RAW,_rawmode=FALSE,_pfast=!(_tty.sg_flags&CRMOD),stty(_tty_ch,&_tty))
# define cbreak() 	(_tty.sg_flags |= CBREAK, _rawmode = TRUE, stty(_tty_ch,&_tty))
# define nocbreak() 	(_tty.sg_flags &= ~CBREAK,_rawmode=FALSE,stty(_tty_ch,&_tty))
# define crmode() 	cbreak()	/* backwards compatability */
# define nocrmode() 	nocbreak()	/* backwards compatability */
# define echo()	 	(_tty.sg_flags |= ECHO, _echoit = TRUE, stty(_tty_ch, &_tty))
# define noecho() 	(_tty.sg_flags &= ~ECHO, _echoit = FALSE, stty(_tty_ch, &_tty))
# define nl()	 	(_tty.sg_flags |= CRMOD,_pfast = _rawmode,stty(_tty_ch, &_tty))
# define nonl()	 	(_tty.sg_flags &= ~CRMOD, _pfast = TRUE, stty(_tty_ch, &_tty))
# define savetty()	((void) gtty(_tty_ch, &_tty), _res_flg = (int) _tty.sg_flags)
# define resetty() 	(_tty.sg_flags = _res_flg, (void) stty(_tty_ch, &_tty))

# define	erasechar()	(_tty.sg_erase)
# define	killchar()	(_tty.sg_kill)
# define 	baudrate()	(_tty.sg_ospeed)

/*
 * Used to be in unctrl.h.
 */
# define	unctrl(c)	_unctrl[(c) & 0177]
extern char *_unctrl[];

/*
 * Defs to provide ANSI compatibility
 */

WINDOW	*initscr(void);
void	endwin(void);
int	wrefresh(WINDOW *win);
WINDOW	*newwin(int nline, int ncols, int begy, int begx);
int	mvwin(WINDOW *win, int by, int bx);
WINDOW	*subwin(WINDOW *win, int nlines, int ncols, int begy, int begx);
void	delwin(WINDOW *win);
int	waddch(WINDOW *win, char c);
int	waddstr(WINDOW *win, char *str);
void	box(WINDOW *win, char vert, char hor);
void	werase(WINDOW *win);
int	wclear(WINDOW *win);
void	wclrtobot(WINDOW *win);
void	wclrtoeol(WINDOW *win);
int	wdelch(WINDOW *win);
int	wdeleteln(WINDOW *win);
int	winsch(WINDOW *win, char ch);
void	winsertln(WINDOW *win);
int	wmove(WINDOW *win, int y, int x);
void	overlay(WINDOW *srcwin, WINDOW *dstwin);
void	overwrite(WINDOW *srcwin, WINDOW *dstwin);
int	printw(char *fmt, ...);
int	wprintw(WINDOW *win, char *fmt, ...);
int	mvprintw(int y, int x, char *fmt, ...);
int	mvwprintw(WINDOW *win, int y, int x, char *fmt, ...);
int	scroll(WINDOW *win);
void	touchwin(WINDOW *win);
void	touchline(WINDOW *win, int y, int ex, int sx);
char	wgetch(WINDOW *win);
int	wgetstr(WINDOW *win, char *str);
int 	scanw(char *fmt, ...);
int	wscanw(WINDOW *win, char *fmt, ...);
int	mvscanw(int y, int x, char *fmt, ...);
int	mvwscanw(WINDOW *win, int y, int x, char *fmt, ...);
void	idlok(WINDOW *win, bool bf);
char	*longname(char *bp, char *def);
char	*getcap(char *name);
void	touchoverlap(WINDOW *win1, WINDOW *win2);
void	gettmode(void);

/*
 * Non XOPEN calls
 */
int	waddbytes(WINDOW *win, char *bytes, int count);
char	*wstandout(WINDOW *win);
char	*wstandend(WINDOW *win);
extern int stty( int, SGTTY * );
extern int gtty( int, SGTTY * );
extern void _id_subwins( WINDOW * );
extern int setterm( char * type );
extern int  _sscans(WINDOW *win, char *fmt, ...);

#endif /* WINDOW */
