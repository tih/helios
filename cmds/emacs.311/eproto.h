/*	EPROTO:		Global function prototypes and declarations
			MicroEMACS 3.11

                        written by Daniel Lawrence
                        based on code by Dave G. Conroy,
                        	Steve Wilhite and George Jones
*/

/*	Modifications:
	11-Sep-89	Mike Burrow (INMOS)	Added folding.
*/

#if	PROTO

/***	global function prototypes	***/

extern BUFFER *PASCAL NEAR bfind(char *bname, int cflag, int bflag);
extern BUFFER *PASCAL NEAR getcbuf(char *prompt, char *defval, int createflag);
extern BUFFER *PASCAL NEAR getdefb(void);
extern SCREEN *PASCAL NEAR init_screen(char *, BUFFER *);
extern SCREEN *PASCAL NEAR lookup_screen(char *scr_name);
extern SCREEN *PASCAL NEAR index_screen(int scr_num);
extern int PASCAL NEAR screen_index(SCREEN *sp);
extern PASCAL NEAR insert_screen(SCREEN *sp);
extern int PASCAL NEAR select_screen(SCREEN *sp, int announce);
extern PASCAL NEAR free_screen(SCREEN *sp);
#if	VARARG && VARG
extern CDECL NEAR int mlwrite(va_dcl);
#else
extern CDECL NEAR mlwrite(char *fmt, ...);
#endif
extern char *allocate(unsigned nbytes);
extern char *dolock(char *fname);
extern char *getpath(char *filespec);
extern char *gtname(char *filespec);
extern char *PASCAL NEAR bytecopy(char *dst, char *src, int maxlen);
extern char *PASCAL NEAR copystr(char *);
extern char *PASCAL NEAR complete(char *prompt, char *defval, int type, int maxlen);
extern char *PASCAL NEAR envval(int i);
extern char *PASCAL NEAR fixnull(char *s);
extern char *PASCAL NEAR flook(char *fname, int hflag);
extern char *PASCAL NEAR funval(int i);
extern char *PASCAL NEAR getctext(void);
extern char *PASCAL NEAR getffile(char *fspec);
extern char *PASCAL NEAR getfname(KEYTAB *key);
extern char *PASCAL NEAR getkill(void);
extern char *PASCAL NEAR getnfile(void);
extern char *PASCAL NEAR getreg(char *value);
extern char *PASCAL NEAR getval(char *token);
extern char *PASCAL NEAR getwlist(char *buf);
extern char *PASCAL NEAR gtenv(char *vname);
extern char *PASCAL NEAR gtfilename(char *prompt);
extern char *PASCAL NEAR gtfun(char *fname);
extern char *PASCAL NEAR gtusr(char *vname);
extern char *PASCAL NEAR int_asc(int i);
extern char *PASCAL NEAR ltos(int val);
extern char *PASCAL NEAR makename(char *bname, char *fname);
extern char *PASCAL NEAR mklower(char *str);
extern char *PASCAL NEAR mkupper(char *str);
extern char *PASCAL NEAR namval(int index);
extern char *PASCAL NEAR timeset(void);
extern char *PASCAL NEAR token(char *src, char *tok, int size);
extern char *PASCAL NEAR transbind(char *skey);
extern char *PASCAL NEAR trimstr(char *s);
extern char *PASCAL NEAR xlat(char *source, char *lookup, char *trans);
extern char *undolock(char *fname);
extern char *PASCAL NEAR regtostr(char *buf, REGION *region);
extern char PASCAL NEAR lowerc(char ch);
extern char PASCAL NEAR upperc(char ch);
#if	ZTC
extern int (PASCAL NEAR *PASCAL NEAR fncmatch(char *fname))(int, int);
extern int (PASCAL NEAR *PASCAL NEAR getname(char *prompt))(int, int);
#else	/* Sun (and others?) screwed up the prototyping.*/
extern int (PASCAL NEAR *PASCAL NEAR fncmatch(char *fname))(void);
extern int (PASCAL NEAR *PASCAL NEAR getname(char *prompt))(void);
#endif
extern int a1getc(FILE *fp);
extern int PASCAL NEAR asc_int(char *st);
extern int comp_buffer(char *name, int *cpos);
extern int comp_command(char *name, int *cpos);
extern int dolhello(void);
extern int dspram(void);
extern int errormesg(char *mesg, BUFFER *bp, LINE *lp);
extern int lckerror(char *errstr);
extern int lckhello(void);
extern int lock(char *fname);
extern int lockchk(char *fname);
extern int lockrel(void);
extern int lowercase(char *cp);
extern int mousehello(void);
extern int nocrypt(void);
extern int pad(char *s, int len);
extern int PASCAL NEAR absv(int x);
extern int PASCAL NEAR addline(BUFFER *bp, char *text);
extern int PASCAL NEAR amatch(MC *mcptr, int direct, LINE **pcwline, int *pcwoff);
extern int PASCAL NEAR backhunt(int f, int n);
extern int PASCAL NEAR backsearch(int f, int n);
extern int PASCAL NEAR biteq(int bc, BITMAP cclmap);
extern int PASCAL NEAR bktoshell(int f, int n);
extern int PASCAL NEAR boundry(LINE *curline, int curoff, int dir);
extern int PASCAL NEAR cclmake(char **ppatptr, MC *mcptr);
extern int PASCAL NEAR checknext(int chr, int dir);
extern int PASCAL NEAR delins(int dlength, char *instr, int use_rmc);
extern int PASCAL NEAR dispvar(int f, int n);
extern int PASCAL NEAR echochar(int c, int col);
extern int PASCAL NEAR eq(register int bc, register int pc);
extern int PASCAL NEAR ernd(void);
extern int PASCAL NEAR execkey(KEYTAB *key, int f, int n);
extern int PASCAL NEAR expandp(char *srcstr, char *deststr, int maxlength);
extern int PASCAL NEAR fbound(int jump, LINE **pcurline, int *pcuroff, int dir);
extern int PASCAL NEAR fexist(char *fname);
extern int PASCAL NEAR fisearch(int f, int n);
#if	FLABEL
extern int PASCAL NEAR fnclabel(int f, int n);
#endif
extern int PASCAL NEAR forwhunt(int f, int n);
extern int PASCAL NEAR forwsearch(int f, int n);
extern int PASCAL NEAR getcwnum(void);
extern int PASCAL NEAR getgoal(LINE *dlp);
extern int PASCAL NEAR getrawregion(register REGION *rp);/* MJB: 26-Sep-89 */
extern int PASCAL NEAR gettwnum(void);
extern int PASCAL NEAR gettyp(char *token);
extern int PASCAL NEAR getwpos(void);
extern int PASCAL NEAR get_char(void);
extern int PASCAL NEAR indx(char s[], char t[]);/* MJB: 20-Sep-89 */
extern int PASCAL NEAR initlinelist(void);	/* MJB: 15-Sep-89 */
extern int PASCAL NEAR loffset(LINE *lp);	/* MJB: 25-Sep-89 */
#if	DBCS
extern int PASCAL NEAR is2byte(char *sp, char *cp);
#endif
extern int PASCAL NEAR isletter(register unsigned int ch);
extern int PASCAL NEAR islower(register unsigned int ch);
extern int PASCAL NEAR is_num(char *st);
extern int PASCAL NEAR isupper(register unsigned int ch);
extern int PASCAL NEAR lookup_color(char *sp);
extern int PASCAL NEAR mceq(int bc, MC *mt);
extern int PASCAL NEAR mcscanner(int direct, int beg_or_end, int repeats);
extern int PASCAL NEAR mcstr(void);
extern int PASCAL NEAR minleftmarg(LINE *lp);		/* MJB: 17-Oct-89 */
extern int PASCAL NEAR nextch(LINE **pcurline, int *pcuroff, int dir);
extern int PASCAL NEAR pop(BUFFER *popbuffer);
extern int PASCAL NEAR promptpattern(char *prompt);
extern int PASCAL NEAR pushline(LINE *);		/* MJB: 15-Sep-89 */
extern int PASCAL NEAR qreplace(int f, int n);
extern int PASCAL NEAR readpattern(char *prompt, char apat[], int srch);
#if	WINDOW_TEXT
extern void PASCAL NEAR refresh_screen(SCREEN *sp);
#endif
extern int PASCAL NEAR reglines(void);
extern int PASCAL NEAR replaces(int kind, int f, int n);
extern int PASCAL NEAR risearch(int f, int n);
extern int PASCAL NEAR rmcstr(void);
extern int PASCAL NEAR savematch(void);
extern int PASCAL NEAR scanmore(int dir);
extern int PASCAL NEAR scanner(int direct, int beg_or_end, int repeats);
extern int PASCAL NEAR setlower(char *ch, char *val);
extern int PASCAL NEAR setlower(char *ch, char *val);
extern int PASCAL NEAR setupper(char *ch, char *val);
extern int PASCAL NEAR setupper(char *ch, char *val);
extern int PASCAL NEAR setvar(int f, int n);
extern int PASCAL NEAR sindex(char *source, char *pattern);
extern int PASCAL NEAR sreplace(int f, int n);
extern int PASCAL NEAR stol(char *val);
#if	DBCS
extern int PASCAL NEAR stopback(void);
extern int PASCAL NEAR stopforw(void);
#endif
extern int PASCAL NEAR svar(VDESC *var, char *value);
extern int PASCAL NEAR tgetc(void);
extern int PASCAL NEAR tindx(char s[], char t[], int l); /* MJB: 26-Sep-89 */
extern int PASCAL NEAR uneat(void);
extern int release(char *mp);
extern int setkey(KEYTAB *key, char *name);
extern int unlock(char *fname);
extern int uppercase(char *cp);
extern KEYTAB *getbind(int c);
extern LINE *PASCAL NEAR lalloc(int used);
extern LINE *PASCAL NEAR lback(LINE *lp);		/* MJB: 14-Sep-89 */
extern LINE *PASCAL NEAR lforw(LINE *lp);		/* MJB: 14-Sep-89 */
extern LINE *PASCAL NEAR mouseline(WINDOW *wp, int row);
extern LINE *PASCAL NEAR popline(void);			/* MJB: 15-Sep-89 */
extern PASCAL NEAR adjustmode(int kind, int global);
extern PASCAL NEAR anycb(void);
extern PASCAL NEAR apro(int f, int n);
extern PASCAL NEAR backchar(int f, int n);
extern PASCAL NEAR backdel(int f, int n);
extern PASCAL NEAR backline(int f, int n, int raw);
extern PASCAL NEAR backpage(register int f, register int n);
extern PASCAL NEAR backword(int f, int n);
extern PASCAL NEAR bbackline(int f, int n);		/* MJB: 13-Oct-89 */
extern PASCAL NEAR bclear(BUFFER *bp);
extern PASCAL NEAR betawarning(int f, int n);		/* MJB: 30-Oct-89 */
extern PASCAL NEAR bforwline(int f, int n);		/* MJB: 13-Oct-89 */
extern PASCAL NEAR binary(char *key, char *(PASCAL NEAR *tval)(), int tlength);
extern PASCAL NEAR bindtokey(int f, int n);
extern PASCAL NEAR buildlist(int type, char *mstring);
extern PASCAL NEAR capword(int f, int n);
extern PASCAL NEAR cbuf(int f, int n, int bufnum);
extern PASCAL NEAR cbuf1(int f, int n);
extern PASCAL NEAR cbuf10(int f, int n);
extern PASCAL NEAR cbuf11(int f, int n);
extern PASCAL NEAR cbuf12(int f, int n);
extern PASCAL NEAR cbuf13(int f, int n);
extern PASCAL NEAR cbuf14(int f, int n);
extern PASCAL NEAR cbuf15(int f, int n);
extern PASCAL NEAR cbuf16(int f, int n);
extern PASCAL NEAR cbuf17(int f, int n);
extern PASCAL NEAR cbuf18(int f, int n);
extern PASCAL NEAR cbuf19(int f, int n);
extern PASCAL NEAR cbuf2(int f, int n);
extern PASCAL NEAR cbuf20(int f, int n);
extern PASCAL NEAR cbuf21(int f, int n);
extern PASCAL NEAR cbuf22(int f, int n);
extern PASCAL NEAR cbuf23(int f, int n);
extern PASCAL NEAR cbuf24(int f, int n);
extern PASCAL NEAR cbuf25(int f, int n);
extern PASCAL NEAR cbuf26(int f, int n);
extern PASCAL NEAR cbuf27(int f, int n);
extern PASCAL NEAR cbuf28(int f, int n);
extern PASCAL NEAR cbuf29(int f, int n);
extern PASCAL NEAR cbuf3(int f, int n);
extern PASCAL NEAR cbuf30(int f, int n);
extern PASCAL NEAR cbuf31(int f, int n);
extern PASCAL NEAR cbuf32(int f, int n);
extern PASCAL NEAR cbuf33(int f, int n);
extern PASCAL NEAR cbuf34(int f, int n);
extern PASCAL NEAR cbuf35(int f, int n);
extern PASCAL NEAR cbuf36(int f, int n);
extern PASCAL NEAR cbuf37(int f, int n);
extern PASCAL NEAR cbuf38(int f, int n);
extern PASCAL NEAR cbuf39(int f, int n);
extern PASCAL NEAR cbuf4(int f, int n);
extern PASCAL NEAR cbuf40(int f, int n);
extern PASCAL NEAR cbuf5(int f, int n);
extern PASCAL NEAR cbuf6(int f, int n);
extern PASCAL NEAR cbuf7(int f, int n);
extern PASCAL NEAR cbuf8(int f, int n);
extern PASCAL NEAR cbuf9(int f, int n);
extern PASCAL NEAR cex(int f, int n);
extern PASCAL NEAR cinsert(void);
extern PASCAL NEAR clean(void);
extern PASCAL NEAR closefold(int f, int n);		/* MJB: 11-Sep-89 */
extern PASCAL NEAR clrmes(int f, int n);
extern PASCAL NEAR cmdstr(int c, char *seq);
extern PASCAL NEAR copyregion(int f, int n);
extern PASCAL NEAR crypt(char *bptr, unsigned len);
extern PASCAL NEAR ctlxe(int f, int n);
extern PASCAL NEAR ctlxlp(int f, int n);
extern PASCAL NEAR ctlxrp(int f, int n);
extern PASCAL NEAR ctoec(int c);
extern PASCAL NEAR ctrlg(int f, int n);
extern PASCAL NEAR cycle_screens(int f, int n);
extern PASCAL NEAR dcline(int argc, char *argv[], int firstflag);
extern PASCAL NEAR deblank(int f, int n);
extern PASCAL NEAR debug(BUFFER *bp, char *eline, int *skipflag);
extern PASCAL NEAR delbword(int f, int n);
extern PASCAL NEAR delete_screen(int f, int n);
extern PASCAL NEAR delfold(int f, int n);		/* MJB: 21-Sep-89 */
extern PASCAL NEAR delfword(int f, int n);
extern PASCAL NEAR delgmode(int f, int n);
extern PASCAL NEAR delmode(int f, int n);
extern PASCAL NEAR delwind(int f, int n);
extern PASCAL NEAR desbind(int f, int n);
extern PASCAL NEAR desfunc(int f, int n);
extern PASCAL NEAR deskey(int f, int n);
extern PASCAL NEAR desvars(int f, int n);
extern PASCAL NEAR detab(int f, int n);
extern PASCAL NEAR dobuf(BUFFER *bp);
extern PASCAL NEAR docmd(char *cline);
extern PASCAL NEAR dofile(char *fname);
extern PASCAL NEAR ectoc(int c);
extern PASCAL NEAR edinit(char bname[]);
extern PASCAL NEAR editloop(void);
extern PASCAL NEAR endword(int f, int n);
extern PASCAL NEAR enlargewind(int f, int n);
extern PASCAL NEAR entab(int f, int n);
extern PASCAL NEAR enterfold(int f, int n);	/* MJB: 11-Sep-89 */
extern PASCAL NEAR execbuf(int f, int n);
extern PASCAL NEAR execcmd(int f, int n);
extern PASCAL NEAR execfile(int f, int n);
extern PASCAL NEAR execprg(int f, int n);
extern PASCAL NEAR execproc(int f, int n);
extern PASCAL NEAR execute(int c, int f, int n);
extern PASCAL NEAR exitallfolds(int f, int n);	/* MJB: 21-Sep-89 */
extern PASCAL NEAR exitfold(int f, int n);	/* MJB: 11-Sep-89 */
extern PASCAL NEAR ffclose(void);
extern PASCAL NEAR ffgetline(void);
extern PASCAL NEAR ffputline(char buf[], int nbuf);
extern PASCAL NEAR ffropen(char *fn);
extern PASCAL NEAR ffwopen(char *fn, char *mode);
extern PASCAL NEAR fileapp(int f, int n);
extern PASCAL NEAR find_screen(int f, int n);
extern PASCAL NEAR filefind(int f, int n);
extern PASCAL NEAR filename(int f, int n);
extern PASCAL NEAR fileread(int f, int n);
extern PASCAL NEAR filesave(int f, int n);
extern PASCAL NEAR filewrite(int f, int n);
extern PASCAL NEAR fillpara(int f, int n);
extern PASCAL NEAR filter(int f, int n);
extern PASCAL NEAR findvar(char *var, VDESC *vd, int size);
extern PASCAL NEAR fmatch(char ch);
extern PASCAL NEAR forwchar(int f, int n);
extern PASCAL NEAR forwdel(int f, int n);
extern PASCAL NEAR forwline(int f, int n, int raw);
extern PASCAL NEAR forwpage(int f, int n);
extern PASCAL NEAR forwword(int f, int n);
extern PASCAL NEAR freewhile(WHBLOCK *wp);
extern PASCAL NEAR getccol(int bflg);
extern PASCAL NEAR getcmd(void);
extern PASCAL NEAR getfence(int f, int n);
extern PASCAL NEAR getfile(char fname[], int lockfl);
extern PASCAL NEAR getkey(void);
extern PASCAL NEAR getlinenum(BUFFER *bp, LINE *sline);
extern PASCAL NEAR getregion(REGION *rp);
extern PASCAL NEAR getstring(char *prompt, char *buf, int nbuf, int eolchar);
extern PASCAL NEAR gotobob(int f, int n);
extern PASCAL NEAR gotobol(int f, int n);
extern PASCAL NEAR gotobop(int f, int n);
extern PASCAL NEAR gotoeob(int f, int n);
extern PASCAL NEAR gotoeol(int f, int n);
extern PASCAL NEAR gotoeop(int f, int n);
extern PASCAL NEAR gotoline(int f, int n);
extern PASCAL NEAR gotomark(int f, int n);
extern PASCAL NEAR help(int f, int n);
extern PASCAL NEAR ifile(char fname[]);
extern PASCAL NEAR indent(int f, int n);
extern PASCAL NEAR indent_region(int f, int n);
extern PASCAL NEAR initchars(void);
extern PASCAL NEAR initchars(void);
extern PASCAL NEAR insbrace(int n, int c);
extern PASCAL NEAR insfile(int f, int n);
extern PASCAL NEAR inspound(void);
extern PASCAL NEAR void insspace(int f, int n);
extern PASCAL NEAR inword(void);
extern PASCAL NEAR isearch(int dir);
extern PASCAL NEAR ismodeline(WINDOW *wp, int row);
extern PASCAL NEAR istring(int f, int n);
extern PASCAL NEAR void kdelete(void);
extern PASCAL NEAR killbuffer(int f, int n);
extern PASCAL NEAR killpara(int f, int n);
extern PASCAL NEAR killregion(int f, int n);
extern PASCAL NEAR killtext(int f, int n);
extern PASCAL NEAR kinsert(int back, char c);
extern PASCAL NEAR void lchange(register int flag);
extern PASCAL NEAR ldelete(long n, int kflag, int rawmode, int margmode);
extern PASCAL NEAR ldelnewline(int rawmode);
extern PASCAL NEAR void lfree(LINE *lp);
extern PASCAL NEAR linsert(int n, char c, int margmode);
extern PASCAL NEAR linstr(char *instr);
extern PASCAL NEAR listbuffers(int f, int n);
extern PASCAL NEAR list_screens(int f, int n);
extern PASCAL NEAR lnewline(void);
extern PASCAL NEAR long_asc(char buf[], int width, long num);
extern PASCAL NEAR lover(char *ostr);
extern PASCAL NEAR lowerregion(int f, int n);
extern PASCAL NEAR lowerword(int f, int n);
extern PASCAL NEAR lowrite(char c);
extern PASCAL NEAR macarg(char *tok);
extern PASCAL NEAR macrotokey(int f, int n);
extern PASCAL NEAR makefold(int f, int n);		/* MJB: 11-Sep-89 */
extern PASCAL NEAR makelist(int iflag);
extern PASCAL NEAR mouse_screen(void);
extern PASCAL NEAR screenlist(int iflag);
extern PASCAL NEAR makelit(char *s);
extern VOID PASCAL NEAR mcclear(void);
extern PASCAL NEAR meexit(int status);
extern PASCAL NEAR meta(int f, int n);
extern PASCAL NEAR mlerase(void);
extern PASCAL NEAR mlferase(void);
extern PASCAL NEAR mlforce(char *s);
extern PASCAL NEAR mlout(int c);
extern PASCAL NEAR mlputf(int s);
extern PASCAL NEAR mlputi(int i, int r);
extern PASCAL NEAR mlputli(long l, int r);
extern PASCAL NEAR mlputs(char *s);
extern PASCAL NEAR mlreply(char *prompt, char *buf, int nbuf);
extern PASCAL NEAR mltreply(char *prompt, char *buf, int nbuf, int eolchar);
extern PASCAL NEAR mlyesno(char *prompt);
extern PASCAL NEAR modeline(WINDOW *wp);
extern PASCAL NEAR mouseoffset(WINDOW *wp, LINE *lp, int col);
extern PASCAL NEAR movecursor(int row, int col);
extern PASCAL NEAR movemd(int f, int n);
extern PASCAL NEAR movemu(int f, int n);
extern PASCAL NEAR mregdown(int f, int n);
extern PASCAL NEAR mregup(int f, int n);
extern PASCAL NEAR mvdnwind(int f, int n);
extern PASCAL NEAR mvupwind(int f, int n);
extern PASCAL NEAR namebuffer(int f, int n);
extern PASCAL NEAR namedcmd(int f, int n);
extern PASCAL NEAR narrow(int f, int n);
extern PASCAL NEAR newline(int f, int n);
extern PASCAL NEAR new_col_org(int f, int n);
extern PASCAL NEAR new_row_org(int f, int n);
extern PASCAL NEAR newsize(int f, int n);
extern PASCAL NEAR newwidth(int f, int n);
extern PASCAL NEAR nextarg(char *prompt, char *buffer, int size, int terminator);
extern PASCAL NEAR nextbuffer(int f, int n);
extern PASCAL NEAR nextdown(int f, int n);
extern PASCAL NEAR nextup(int f, int n);
extern PASCAL NEAR nextwind(int f, int n);
extern PASCAL NEAR nullproc(int f, int n);
extern PASCAL NEAR onlywind(int f, int n);
extern PASCAL NEAR openfold(int f, int n);		/* MJB: 11-Sep-89 */
extern PASCAL NEAR openline(int f, int n);
extern PASCAL NEAR openoutfolds(void);			/* MJB: 13-Oct-89 */
extern PASCAL NEAR ostring(char *s);
extern PASCAL NEAR outstring(char *s);
extern PASCAL NEAR ovstring(int f, int n);
extern PASCAL NEAR pipecmd(int f, int n);
extern PASCAL NEAR popbuffer(int f, int n);
extern PASCAL NEAR prevwind(int f, int n);
extern PASCAL NEAR putctext(char *iline);
extern PASCAL NEAR putline(int row, int col, char buf[]);
extern PASCAL NEAR quickexit(int f, int n);
extern PASCAL NEAR quit(int f, int n);
extern PASCAL NEAR quote(int f, int n);
extern PASCAL NEAR rdonly(void);
extern PASCAL NEAR readin(char fname[], int lockfl);
extern PASCAL NEAR reeat(int c);
extern PASCAL NEAR reform(char *para);
extern PASCAL NEAR reframe(WINDOW *wp);
extern PASCAL NEAR refresh(int f, int n);
extern PASCAL NEAR remmark(int f, int n);
extern PASCAL NEAR removefold(int f, int n);	/* MJB: 11-Sep-89 */
extern PASCAL NEAR reposition(int f, int n);
extern PASCAL NEAR resetkey(void);
extern PASCAL NEAR resize(int f, int n);
extern PASCAL NEAR resizm(int f, int n);
extern PASCAL NEAR resterr(void);
extern PASCAL NEAR restwnd(int f, int n);
extern VOID PASCAL NEAR rmcclear(void);
extern VOID PASCAL NEAR rvstrcpy(char *rvstr, char *str);
extern PASCAL NEAR savewnd(int f, int n);
extern PASCAL NEAR scwrite(int row, char *outstr, int forg, int bacg);
extern PASCAL NEAR searchffold(int f, int n);	/* MJB: 21-Sep-89 */
extern PASCAL NEAR searchbfold(int f, int n);	/* MJB: 21-Sep-89 */
extern VOID PASCAL NEAR setbit(int bc, BITMAP cclmap);
extern PASCAL NEAR setccol(int pos);
extern PASCAL NEAR setekey(int f, int n);
extern PASCAL NEAR setfillcol(int f, int n);
extern PASCAL NEAR setfoldmarks(int f, int n);
extern PASCAL NEAR setgmode(int f, int n);
extern VOID PASCAL NEAR setjtable(void);
extern PASCAL NEAR setmark(int f, int n);
extern PASCAL NEAR setmod(int f, int n);
extern PASCAL NEAR setwlist(char *wclist);
extern PASCAL NEAR shellprog(char *cmd);
extern PASCAL NEAR showcpos(int f, int n);
extern PASCAL NEAR showfiles(int f, int n);
extern PASCAL NEAR shrinkwind(int f, int n);
extern PASCAL NEAR spal(char *pstr);
extern PASCAL NEAR spawn(int f, int n);
extern PASCAL NEAR spawncli(int f, int n);
extern PASCAL NEAR splitwind(int f, int n);
extern PASCAL NEAR startup(char *sfname);
extern PASCAL NEAR storemac(int f, int n);
extern PASCAL NEAR storeproc(int f, int n);
extern PASCAL NEAR strinc(char *source, char *sub);
extern PASCAL NEAR swapmark(int f, int n);
extern PASCAL NEAR swbuffer(BUFFER *bp);
extern PASCAL NEAR tab(int f, int n);
extern PASCAL NEAR trim(int f, int n);
extern PASCAL NEAR ttclose(void);
extern PASCAL NEAR ttflush(void);
extern PASCAL NEAR ttgetc(void);
extern PASCAL NEAR ttopen(void);
extern PASCAL NEAR ttputc(int c);
extern PASCAL NEAR twiddle(int f, int n);
extern PASCAL NEAR typahead(void);
extern PASCAL NEAR unarg(void);
extern PASCAL NEAR unbindchar(int c);
extern PASCAL NEAR unbindkey(int f, int n);
extern PASCAL NEAR undent_region(int f, int n);
extern PASCAL NEAR unmark(int f, int n);
extern PASCAL NEAR unqname(char *name);
extern PASCAL NEAR updall(WINDOW *wp);
extern PASCAL NEAR update(int force);
extern PASCAL NEAR update_size(void);
extern PASCAL NEAR upddex(void);
extern PASCAL NEAR updext(void);
extern PASCAL NEAR updgar(void);
extern PASCAL NEAR updone(WINDOW *wp);
extern PASCAL NEAR updpos(void);
extern PASCAL NEAR updupd(int force);
extern PASCAL NEAR upmode(void);
extern PASCAL NEAR upperregion(int f, int n);
extern PASCAL NEAR upperword(int f, int n);
extern PASCAL NEAR upscreen(int f, int n);
extern PASCAL NEAR upwind(void);
extern PASCAL NEAR usebuffer(int f, int n);
extern PASCAL NEAR varclean(void);
extern PASCAL NEAR varinit(void);
extern PASCAL NEAR viewfile(int f, int n);
extern PASCAL NEAR vteeol(void);
extern PASCAL NEAR vtfree(void);
extern PASCAL NEAR vtinit(void);
extern PASCAL NEAR vtmove(int row, int col);
extern PASCAL NEAR vtputc(int c);
extern PASCAL NEAR vttidy(void);
extern PASCAL NEAR widen(int f, int n);
extern PASCAL NEAR wordcount(int f, int n);
extern PASCAL NEAR wrapword(int f, int n);
extern PASCAL NEAR writemsg(int f, int n);
extern PASCAL NEAR writeout(char *fn, char *mode);
extern PASCAL NEAR yank(int f, int n);
extern PASCAL NEAR zotbuf(BUFFER *bp);
extern unsigned int PASCAL NEAR chcase(register unsigned int ch);
extern unsigned int PASCAL NEAR getckey(int mflag);
extern unsigned int PASCAL NEAR stock(char *keyname);
extern WINDOW *PASCAL NEAR mousewindow(int row);
extern int PASCAL NEAR wpopup(BUFFER *popbuf);
extern FOLDMARKENT *PASCAL NEAR getftype(char *fname);

#if	CTAGS
extern int PASCAL NEAR tagword(int f, int n);	/* vi-like tagging */
extern int PASCAL NEAR retagword(int f, int n);	/* Try again (if redefined) */
extern int PASCAL NEAR backtagword(int f, int n); /* return from tagged word */
#endif

/* some library redefinitions */

#include <string.h>
#include <stdlib.h>

#else

/***	global function declarations	***/

extern BUFFER *PASCAL NEAR bfind();
extern BUFFER *PASCAL NEAR getcbuf();
extern BUFFER *PASCAL NEAR getdefb();
extern SCREEN *PASCAL NEAR init_screen();
extern SCREEN *PASCAL NEAR lookup_screen();
extern SCREEN *PASCAL NEAR index_screen();
extern int PASCAL NEAR screen_index();
extern PASCAL NEAR insert_screen();
extern int PASCAL NEAR select_screen();
extern PASCAL NEAR free_screen();
extern CDECL NEAR mlwrite();
extern char *allocate();
extern char *dolock();
extern char *getpath();
extern char *gtname();
extern char *PASCAL NEAR bytecopy();
extern char *PASCAL NEAR copystr();
extern char *PASCAL NEAR complete();
extern char *PASCAL NEAR envval();
extern char *PASCAL NEAR fixnull();
extern char *PASCAL NEAR flook();
extern char *PASCAL NEAR funval();
extern char *PASCAL NEAR getctext();
extern char *PASCAL NEAR getffile();
extern char *PASCAL NEAR getfname();
extern char *PASCAL NEAR getkill();
extern char *PASCAL NEAR getnfile();
extern char *PASCAL NEAR getreg();
extern char *PASCAL NEAR getval();
extern char *PASCAL NEAR getwlist();
extern char *PASCAL NEAR gtenv();
extern char *PASCAL NEAR gtfilename();
extern char *PASCAL NEAR gtfun();
extern char *PASCAL NEAR gtusr();
extern char *PASCAL NEAR int_asc();
extern char *PASCAL NEAR ltos();
extern char *PASCAL NEAR makename();
extern char *PASCAL NEAR mklower();
extern char *PASCAL NEAR mkupper();
extern char *PASCAL NEAR namval();
extern char *PASCAL NEAR timeset();
extern char *PASCAL NEAR token();
extern char *PASCAL NEAR transbind();
extern char *PASCAL NEAR trimstr();
extern char *PASCAL NEAR xlat();
extern char *undolock();
extern char *PASCAL NEAR regtostr();
extern char PASCAL NEAR lowerc();
extern char PASCAL NEAR upperc();
extern int (PASCAL NEAR *PASCAL NEAR fncmatch())();
extern int (PASCAL NEAR *PASCAL NEAR getname())();
extern int a1getc();
extern int PASCAL NEAR asc_int();
extern int comp_buffer();
extern int comp_command();
extern int dolhello();
extern int dspram();
extern int errormesg();
extern int lckerror();
extern int lckhello();
extern int lock();
extern int lockchk();
extern int lockrel();
extern int lowercase();
extern int mousehello();
extern int nocrypt();
extern int pad();
extern int PASCAL NEAR absv();
extern int PASCAL NEAR addline();
extern int PASCAL NEAR amatch();
extern int PASCAL NEAR backhunt();
extern int PASCAL NEAR backsearch();
extern int PASCAL NEAR biteq();
extern int PASCAL NEAR bktoshell();
extern int PASCAL NEAR boundry();
extern int PASCAL NEAR cclmake();
extern int PASCAL NEAR checknext();
extern int PASCAL NEAR delins();
extern int PASCAL NEAR dispvar();
extern int PASCAL NEAR echochar();
extern int PASCAL NEAR eq();
extern int PASCAL NEAR ernd();
extern int PASCAL NEAR execkey();
extern int PASCAL NEAR expandp();
extern int PASCAL NEAR fbound();
extern int PASCAL NEAR fexist();
extern int PASCAL NEAR fisearch();
#if	FLABEL
extern int PASCAL NEAR fnclabel();
#endif
extern int PASCAL NEAR forwhunt();
extern int PASCAL NEAR forwsearch();
extern int PASCAL NEAR getcwnum();
extern int PASCAL NEAR getgoal();
extern int PASCAL NEAR gettwnum();
extern int PASCAL NEAR gettyp();
extern int PASCAL NEAR getwpos();
extern int PASCAL NEAR get_char();
extern int PASCAL NEAR indx();			/* MJB: 20-Sep-89 */
extern int PASCAL NEAR initlinelist();		/* MJB: 15-Sep-89 */
extern int PASCAL NEAR loffset();		/* MJB: 25-Sep-89 */
#if	DBCS
extern int PASCAL NEAR is2byte();
#endif
extern int PASCAL NEAR isletter();
extern int PASCAL NEAR islower();
extern int PASCAL NEAR is_num();
extern int PASCAL NEAR isupper();
extern int PASCAL NEAR lookup_color();
extern int PASCAL NEAR mceq();
extern int PASCAL NEAR mcscanner();
extern int PASCAL NEAR mcstr();
extern int PASCAL NEAR minleftmarg();		/* MJB: 17-Oct-89 */
extern int PASCAL NEAR nextch();
extern int PASCAL NEAR pop();
extern int PASCAL NEAR promptpattern();
extern int PASCAL NEAR pushline();		/* MJB: 15-Sep-89 */
extern int PASCAL NEAR qreplace();
extern int PASCAL NEAR readpattern();
#if	WINDOW_TEXT
extern void PASCAL NEAR refresh_screen();
#endif
extern int PASCAL NEAR reglines();
extern int PASCAL NEAR replaces();
extern int PASCAL NEAR risearch();
extern int PASCAL NEAR rmcstr();
extern int PASCAL NEAR savematch();
extern int PASCAL NEAR scanmore();
extern int PASCAL NEAR scanner();
extern int PASCAL NEAR setlower();
extern int PASCAL NEAR setlower();
extern int PASCAL NEAR setupper();
extern int PASCAL NEAR setupper();
extern int PASCAL NEAR setvar();
extern int PASCAL NEAR sindex();
extern int PASCAL NEAR sreplace();
extern int PASCAL NEAR stol();
#if	DBCS
extern int PASCAL NEAR stopback();
extern int PASCAL NEAR stopforw();
#endif
extern int PASCAL NEAR svar();
extern int PASCAL NEAR tgetc();
extern int PASCAL NEAR tindx();		/* MJB: 26-Sep-89 */
extern int PASCAL NEAR uneat();
extern int release();
extern int setkey();
extern int unlock();
extern int uppercase();
extern KEYTAB *getbind();
extern LINE *PASCAL NEAR lalloc();
extern LINE *PASCAL NEAR lback();		/* MJB: 14-Sep-89 */
extern LINE *PASCAL NEAR lforw();		/* MJB: 14-Sep-89 */
extern LINE *PASCAL NEAR mouseline();
extern LINE *PASCAL NEAR popline();		/* MJB: 15-Sep-89 */
extern PASCAL NEAR adjustmode();
extern PASCAL NEAR anycb();
extern PASCAL NEAR apro();
extern PASCAL NEAR backchar();
extern PASCAL NEAR backdel();
extern PASCAL NEAR backline();
extern PASCAL NEAR backpage();
extern PASCAL NEAR backword();
extern PASCAL NEAR bbackline();		/* MJB: 13-Oct-89 */
extern PASCAL NEAR bclear();
extern PASCAL NEAR betawarning();		/* MJB: 30-Oct-89 */
extern PASCAL NEAR bforwline();		/* MJB: 13-Oct-89 */
extern PASCAL NEAR binary();
extern PASCAL NEAR bindtokey();
extern PASCAL NEAR buildlist();
extern PASCAL NEAR capword();
extern PASCAL NEAR cbuf();
extern PASCAL NEAR cbuf1();
extern PASCAL NEAR cbuf10();
extern PASCAL NEAR cbuf11();
extern PASCAL NEAR cbuf12();
extern PASCAL NEAR cbuf13();
extern PASCAL NEAR cbuf14();
extern PASCAL NEAR cbuf15();
extern PASCAL NEAR cbuf16();
extern PASCAL NEAR cbuf17();
extern PASCAL NEAR cbuf18();
extern PASCAL NEAR cbuf19();
extern PASCAL NEAR cbuf2();
extern PASCAL NEAR cbuf20();
extern PASCAL NEAR cbuf21();
extern PASCAL NEAR cbuf22();
extern PASCAL NEAR cbuf23();
extern PASCAL NEAR cbuf24();
extern PASCAL NEAR cbuf25();
extern PASCAL NEAR cbuf26();
extern PASCAL NEAR cbuf27();
extern PASCAL NEAR cbuf28();
extern PASCAL NEAR cbuf29();
extern PASCAL NEAR cbuf3();
extern PASCAL NEAR cbuf30();
extern PASCAL NEAR cbuf31();
extern PASCAL NEAR cbuf32();
extern PASCAL NEAR cbuf33();
extern PASCAL NEAR cbuf34();
extern PASCAL NEAR cbuf35();
extern PASCAL NEAR cbuf36();
extern PASCAL NEAR cbuf37();
extern PASCAL NEAR cbuf38();
extern PASCAL NEAR cbuf39();
extern PASCAL NEAR cbuf4();
extern PASCAL NEAR cbuf40();
extern PASCAL NEAR cbuf5();
extern PASCAL NEAR cbuf6();
extern PASCAL NEAR cbuf7();
extern PASCAL NEAR cbuf8();
extern PASCAL NEAR cbuf9();
extern PASCAL NEAR cex();
extern PASCAL NEAR cinsert();
extern PASCAL NEAR clean();
extern PASCAL NEAR closefold();	/* MJB: 11-Sep-89 */
extern PASCAL NEAR clrmes();
extern PASCAL NEAR cmdstr();
extern PASCAL NEAR copyregion();
extern PASCAL NEAR crypt();
extern PASCAL NEAR ctlxe();
extern PASCAL NEAR ctlxlp();
extern PASCAL NEAR ctlxrp();
extern PASCAL NEAR ctoec();
extern PASCAL NEAR ctrlg();
extern PASCAL NEAR cycle_screens();
extern PASCAL NEAR dcline();
extern PASCAL NEAR deblank();
extern PASCAL NEAR debug();
extern PASCAL NEAR delbword();
extern PASCAL NEAR delete_screen();
extern PASCAL NEAR delfold();		/* MJB: 21-Sep-89 */
extern PASCAL NEAR delfword();
extern PASCAL NEAR delgmode();
extern PASCAL NEAR delmode();
extern PASCAL NEAR delwind();
extern PASCAL NEAR desbind();
extern PASCAL NEAR desfunc();
extern PASCAL NEAR deskey();
extern PASCAL NEAR desvars();
extern PASCAL NEAR detab();
extern PASCAL NEAR dobuf();
extern PASCAL NEAR docmd();
extern PASCAL NEAR dofile();
extern PASCAL NEAR ectoc();
extern PASCAL NEAR edinit();
extern PASCAL NEAR editloop();
extern PASCAL NEAR endword();
extern PASCAL NEAR enlargewind();
extern PASCAL NEAR entab();
extern PASCAL NEAR enterfold();	/* MJB: 11-Sep-89 */
extern PASCAL NEAR execbuf();
extern PASCAL NEAR execcmd();
extern PASCAL NEAR execfile();
extern PASCAL NEAR execprg();
extern PASCAL NEAR execproc();
extern PASCAL NEAR execute();
extern PASCAL NEAR exitallfolds();	/* MJB: 21-Sep-89 */
extern PASCAL NEAR exitfold();		/* MJB: 11-Sep-89 */
extern PASCAL NEAR ffclose();
extern PASCAL NEAR ffgetline();
extern PASCAL NEAR ffputline();
extern PASCAL NEAR ffropen();
extern PASCAL NEAR ffwopen();
extern PASCAL NEAR fileapp();
extern PASCAL NEAR find_screen();
extern PASCAL NEAR filefind();
extern PASCAL NEAR filename();
extern PASCAL NEAR fileread();
extern PASCAL NEAR filesave();
extern PASCAL NEAR filewrite();
extern PASCAL NEAR fillpara();
extern PASCAL NEAR filter();
extern PASCAL NEAR findvar();
extern PASCAL NEAR fmatch();
extern PASCAL NEAR forwchar();
extern PASCAL NEAR forwdel();
extern PASCAL NEAR forwline();
extern PASCAL NEAR forwpage();
extern PASCAL NEAR forwword();
extern PASCAL NEAR freewhile();
extern PASCAL NEAR getccol();
extern PASCAL NEAR getcmd();
extern PASCAL NEAR getfence();
extern PASCAL NEAR getfile();
extern PASCAL NEAR getkey();
extern PASCAL NEAR getlinenum();
extern PASCAL NEAR getrawregion();	/* MJB: 26-Sep-89 */
extern PASCAL NEAR getregion();
extern PASCAL NEAR getstring();
extern PASCAL NEAR gotobob();
extern PASCAL NEAR gotobol();
extern PASCAL NEAR gotobop();
extern PASCAL NEAR gotoeob();
extern PASCAL NEAR gotoeol();
extern PASCAL NEAR gotoeop();
extern PASCAL NEAR gotoline();
extern PASCAL NEAR gotomark();
extern PASCAL NEAR help();
extern PASCAL NEAR ifile();
extern PASCAL NEAR indent();
extern PASCAL NEAR indent_region();
extern PASCAL NEAR initchars();
extern PASCAL NEAR initchars();
extern PASCAL NEAR insbrace();
extern PASCAL NEAR insfile();
extern PASCAL NEAR inspound();
extern PASCAL NEAR void insspace();
extern PASCAL NEAR inword();
extern PASCAL NEAR isearch();
extern PASCAL NEAR isearch();
extern PASCAL NEAR ismodeline();
extern PASCAL NEAR istring();
extern PASCAL NEAR void kdelete();
extern PASCAL NEAR killbuffer();
extern PASCAL NEAR killpara();
extern PASCAL NEAR killregion();
extern PASCAL NEAR killtext();
extern PASCAL NEAR kinsert();
extern PASCAL NEAR void lchange();
extern PASCAL NEAR ldelete();
extern PASCAL NEAR ldelnewline();
extern PASCAL NEAR void lfree();
extern PASCAL NEAR linsert();
extern PASCAL NEAR linstr();
extern PASCAL NEAR listbuffers();
extern PASCAL NEAR list_screens();
extern PASCAL NEAR lnewline();
extern PASCAL NEAR long_asc();
extern PASCAL NEAR lover();
extern PASCAL NEAR lowerregion();
extern PASCAL NEAR lowerword();
extern PASCAL NEAR lowrite();
extern PASCAL NEAR macarg();
extern PASCAL NEAR macrotokey();
extern PASCAL NEAR makefold();		/* MJB: 11-Sep-89 */
extern PASCAL NEAR makelist();
extern PASCAL NEAR mouse_screen();
extern PASCAL NEAR screenlist();
extern PASCAL NEAR makelit();
extern VOID PASCAL NEAR mcclear();
extern PASCAL NEAR meexit();
extern PASCAL NEAR meta();
extern PASCAL NEAR mlerase();
extern PASCAL NEAR mlferase();
extern PASCAL NEAR mlforce();
extern PASCAL NEAR mlout();
extern PASCAL NEAR mlputf();
extern PASCAL NEAR mlputi();
extern PASCAL NEAR mlputli();
extern PASCAL NEAR mlputs();
extern PASCAL NEAR mlreply();
extern PASCAL NEAR mltreply();
extern PASCAL NEAR mlyesno();
extern PASCAL NEAR modeline();
extern PASCAL NEAR mouseoffset();
extern PASCAL NEAR movecursor();
extern PASCAL NEAR movemd();
extern PASCAL NEAR movemu();
extern PASCAL NEAR mregdown();
extern PASCAL NEAR mregup();
extern PASCAL NEAR mvdnwind();
extern PASCAL NEAR mvupwind();
extern PASCAL NEAR namebuffer();
extern PASCAL NEAR namedcmd();
extern PASCAL NEAR narrow();
extern PASCAL NEAR newline();
extern PASCAL NEAR new_col_org();
extern PASCAL NEAR new_row_org();
extern PASCAL NEAR newsize();
extern PASCAL NEAR newwidth();
extern PASCAL NEAR nextarg();
extern PASCAL NEAR nextbuffer();
extern PASCAL NEAR nextdown();
extern PASCAL NEAR nextup();
extern PASCAL NEAR nextwind();
extern PASCAL NEAR nullproc();
extern PASCAL NEAR onlywind();
extern PASCAL NEAR openfold();		/* MJB: 11-Sep-89 */
extern PASCAL NEAR openline();
extern PASCAL NEAR ostring();
extern PASCAL NEAR outstring();
extern PASCAL NEAR ovstring();
extern PASCAL NEAR pipecmd();
extern PASCAL NEAR popbuffer();
extern PASCAL NEAR prevwind();
extern PASCAL NEAR putctext();
extern PASCAL NEAR putline();
extern PASCAL NEAR quickexit();
extern PASCAL NEAR quit();
extern PASCAL NEAR quote();
extern PASCAL NEAR rdonly();
extern PASCAL NEAR readin();
extern PASCAL NEAR reeat();
extern PASCAL NEAR reform();
extern PASCAL NEAR reframe();
extern PASCAL NEAR refresh();
extern PASCAL NEAR remmark();
extern PASCAL NEAR removefold();	/* MJB: 11-Sep-89 */
extern PASCAL NEAR reposition();
extern PASCAL NEAR resetkey();
extern PASCAL NEAR resize();
extern PASCAL NEAR resizm();
extern PASCAL NEAR resterr();
extern PASCAL NEAR restwnd();
extern VOID PASCAL NEAR rmcclear();
extern VOID PASCAL NEAR rvstrcpy();
extern PASCAL NEAR savewnd();
extern PASCAL NEAR scwrite();
extern PASCAL NEAR searchffold();	/* MJB: 21-Sep-89 */
extern PASCAL NEAR searchbfold();	/* MJB: 21-Sep-89 */
extern VOID PASCAL NEAR setbit();
extern PASCAL NEAR setccol();
extern PASCAL NEAR setekey();
extern PASCAL NEAR setfillcol();
extern PASCAL NEAR setfoldmarks();
extern PASCAL NEAR setgmode();
extern VOID PASCAL NEAR setjtable();
extern PASCAL NEAR setmark();
extern PASCAL NEAR setmod();
extern PASCAL NEAR setwlist();
extern PASCAL NEAR shellprog();
extern PASCAL NEAR showfiles();
extern PASCAL NEAR showcpos();
extern PASCAL NEAR shrinkwind();
extern PASCAL NEAR spal();
extern PASCAL NEAR spawn();
extern PASCAL NEAR spawncli();
extern PASCAL NEAR splitwind();
extern PASCAL NEAR startup();
extern PASCAL NEAR storemac();
extern PASCAL NEAR storeproc();
extern PASCAL NEAR strinc();
extern PASCAL NEAR swapmark();
extern PASCAL NEAR swbuffer();
extern PASCAL NEAR tab();
extern PASCAL NEAR trim();
extern PASCAL NEAR ttclose();
extern PASCAL NEAR ttflush();
extern PASCAL NEAR ttgetc();
extern PASCAL NEAR ttopen();
extern PASCAL NEAR ttputc();
extern PASCAL NEAR twiddle();
extern PASCAL NEAR typahead();
extern PASCAL NEAR unarg();
extern PASCAL NEAR unbindchar();
extern PASCAL NEAR unbindkey();
extern PASCAL NEAR undent_region();
extern PASCAL NEAR unmark();
extern PASCAL NEAR unqname();
extern PASCAL NEAR updall();
extern PASCAL NEAR update();
extern PASCAL NEAR update_size();
extern PASCAL NEAR upddex();
extern PASCAL NEAR updext();
extern PASCAL NEAR updgar();
extern PASCAL NEAR updone();
extern PASCAL NEAR updpos();
extern PASCAL NEAR updupd();
extern PASCAL NEAR upmode();
extern PASCAL NEAR upperregion();
extern PASCAL NEAR upperword();
extern PASCAL NEAR upscreen();
extern PASCAL NEAR upwind();
extern PASCAL NEAR usebuffer();
extern PASCAL NEAR varclean();
extern PASCAL NEAR varinit();
extern PASCAL NEAR viewfile();
extern PASCAL NEAR vteeol();
extern PASCAL NEAR vtfree();
extern PASCAL NEAR vtinit();
extern PASCAL NEAR vtmove();
extern PASCAL NEAR vtputc();
extern PASCAL NEAR vttidy();
extern PASCAL NEAR widen();
extern PASCAL NEAR wordcount();
extern PASCAL NEAR wrapword();
extern PASCAL NEAR writemsg();
extern PASCAL NEAR writeout();
extern PASCAL NEAR yank();
extern PASCAL NEAR zotbuf();
extern unsigned int PASCAL NEAR chcase();
extern unsigned int PASCAL NEAR getckey();
extern unsigned int PASCAL NEAR stock();
extern WINDOW *PASCAL NEAR mousewindow();
extern int PASCAL NEAR wpopup();
extern FOLDMARKENT *PASCAL NEAR getftype();

#if	CTAGS
extern int PASCAL NEAR tagword();	/* vi-like tagging */
extern int PASCAL NEAR retagword();	/* Try again (if redefined) */
extern int PASCAL NEAR backtagword();	/* return from tagged word */
#endif

/* some library redefinitions */

#include <string.h>
#include <stdlib.h>

#endif

