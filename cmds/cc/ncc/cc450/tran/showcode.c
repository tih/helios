/*{{{  Includes */
/* $Id: showcode.c,v 1.2 1995/08/04 11:19:48 nickc Exp $ */
#ifdef __old
#include "cchdr.h"
#include "util.h"
#include "xpuops.h"
#include "xrefs.h"
#include "cg.h"
#include "AEops.h"
#include <stdarg.h>
#define g_list4(t,a1,a2,a3,a4) global_list4(a1,a2,a3,a4)
#define g_list5(t,a1,a2,a3,a4,a5) global_list5(a1,a2,a3,a4,a5)
#define g_list6(t,a1,a2,a3,a4,a5,a6) global_list6(a1,a2,a3,a4,a5,a6)
#define Glob_Alloc(s) GlobAlloc(s)
#else
#include "globals.h"
#include "builtin.h"
#include "store.h"
#include "util.h"
#include "xpuops.h"
#include "xrefs.h"
#include "cg.h"
#include "aeops.h"
#include "codebuf.h"
#include "mcdep.h"
#include <stdarg.h>
#define g_list4(t,a1,a2,a3,a4) global_list4(t,a1,a2,a3,a4)
#define g_list5(t,a1,a2,a3,a4,a5) global_list5(t,a1,a2,a3,a4,a5)
#define g_list6(t,a1,a2,a3,a4,a5,a6) global_list6(t,a1,a2,a3,a4,a5,a6)
#define Glob_Alloc(s) GlobAlloc(SU_Other, s)
#endif
/*}}}*/
/*{{{  Externs, static, defines etc */

/* My debug things.  Tony 4/1/95 */
#if 0
#define TDebug_(statement)	statement
#else
#define TDebug_(statement)
#endif

#ifdef __STDC__
int trace( char *str, ... );
int aprintf( char *str, ... );
#else
#define va_alist __va_alist__
#define va_dcl   char *__va_alist__
#endif

#define COMMONDATA 0		/* make all externs common, not data */

/* Assembler directives */

#define asm_code	0xd5
#define asm_bss		0xd8
#define asm_blkb        0xe1
#define asm_blkw        0xe2
#define asm_init        0xe3
#define asm_align       0xe4
#define asm_word        0xe5
#define asm_byte        0xe6
#define asm_module      0xe7
#define asm_global      0xe8
#define asm_data        0xe9
#define asm_common	0xea
#define asm_size	0xeb
#define asm_ref		0xec

#define c_ext	'_'
#define c_code	'.'

typedef struct Stub {
	struct Stub	*next;
	int		type;
	int		valid;
	Symstr		*sym;
} Stub;

#define type_ext	1
#define type_pv		2
#define type_fp		3

/* memory access checking stub types. Tony (22/12/94) */
#define type_ldnl	4	/* load non-local long	*/
#define type_stnl	5	/* store non-local long	*/
#define type_ls		6	/* load short		*/
#define type_lsx	7	/* load short and extend*/
#define type_ss		8	/* store short		*/
#define type_lb		9	/* load byte		*/
#define type_lbx	10	/* load byte and extend	*/
#define	type_sb		11	/* store byte		*/

int32	mac_stubs;
#define no_mac_stub_(v)		((mac_stubs & (v)) == 0)
#define set_mac_stub_(v)	(mac_stubs |= (v))

#define	MAC_LDNLW	1
#define MAC_LDNLS	2
#define MAC_LDNLSX	4
#define MAC_LDNLB	8
#define MAC_LDNLBX	16
#define MAC_STNLW	32
#define MAC_STNLS	64
#define MAC_STNLB	128

/*
 * Set when a load or store is safe (assumed always when no memory access checking)
 * Tony 3/1/95.
 */

int	safe_ldnl;
int	safe_stnl;

#define check_load_memory_access	(memory_access_checks && !safe_ldnl)
#define check_store_memory_access	(memory_access_checks && !safe_stnl)

#if 1
#define safe_load()	safe_ldnl = TRUE
#define unsafe_load()	safe_ldnl = (memory_access_checks)?FALSE:TRUE
#define safe_store()	safe_stnl = TRUE
#define unsafe_store()	safe_stnl = (memory_access_checks)?FALSE:TRUE
#else
void safe_load ()
{
	cc_msg ("loads are safe\n");

	safe_ldnl = TRUE;
}
void unsafe_load ()
{
	if (memory_access_checks)
	{
		cc_msg ("loads are unsafe\n");

		safe_ldnl = FALSE;
	}
	else
	{
		cc_msg ("loads are still safe\n");

		safe_ldnl = TRUE;
	}
}
void safe_store ()
{
	cc_msg ("stores are safe\n");

	safe_stnl = TRUE;
}
void unsafe_store ()
{
	if (memory_access_checks)
	{
		cc_msg ("stores are unsafe\n");

		safe_stnl = FALSE;
	}
	else
	{
		cc_msg ("stores are still safe\n");

		safe_stnl = TRUE;
	}
}
#endif

typedef struct DataCode {
	struct DataCode	*next;
	int		type;
	int		dest;
	union {
		LabelNumber	*l;
		Symstr		*s;
		int		i;
	} src;
	int size;
} DataCode;

/* DataCode types */
#define DC_MOVE		0
#define DC_FUNC		1
#define DC_STRING	2
#define DC_STATIC	3
#define DC_EXTERN	4
#define DC_EXTFUNC	5

typedef struct FileName {
	struct FileName	*next;
	LabelNumber	*lab;
	char		*name;	
} FileName;

FileName *filenamelist = NULL;

extern int maxssp;
extern int maxvsp;
extern int vsp;
extern int maxcall;
extern int ncalls;
extern Block *topblock;
extern Literal *litpoolhead;

#ifdef __old
extern DataInit *datainitp;
extern DataInit *datainitq;
extern int dataloc;
#else
#ifndef CC420
DataInit *datainitp;
DataInit *datainitq;
int dataloc;
#endif
#endif

static LabelNumber *infolab = NULL;
LabelNumber *litlab = NULL;

extern Block *start_new_basic_block();

int nodata = FALSE;		/* Made false (rather than not defined at all).*/
				/* Tony 23/1/95.			       */
int export_statics = FALSE;

#if 0
/*
 * Replaced by pragma check library_module.
 * Note that I've replaced BOTH flags with the single pragma which
 * may or may not be correct.  Tony 23/1/95.
 */
int nomodule = FALSE;		/* Made false (rather than not defined at all. */
int libfile = FALSE;
#endif

int fnstack;

Stub *stubhead, *stubend;
struct FPLIB fplib;
struct DEBUG debug;
struct BUILTIN builtin;

/* forward references */
void dumplits();

void putop (Xop);
void putfc (Xop, int);
void putfcc (Xop, int, char *);


void genstub(int, Symstr *, int);
#define genstub_(t,s,v)		genstub ((int)(t), (Symstr *)(s), (int)(v))

void dataword (int, int);
#define dataword_(s,w)		dataword ((int)(s), (int)(w))

void dosym (Symstr *, int);
#define dosym_(s,e)		dosym ((Symstr *)(s), (int)(e))

void putword (long);
#define putword_(w)		putword ((long)(w))

void aputc(char);
#define aputc_(c)		aputc ((char)(c))

Binder *lib_binder();
LabelNumber *genpvstub();
LabelNumber *filenamelabel();

static FILE *outstream;
static bool tokenize;
/*}}}*/
/*{{{  asm_header */

#if CC420
extern char *sourcefile;
#define pp_cisname sourcefile
#endif

void asm_header()
{
	int i = strlen(pp_cisname);
	char *cisname;

	while( i>=0 && pp_cisname[i] != '/' ) i--;
	cisname = &pp_cisname[i+1];

#if defined(DBX) || defined(TARGET_HAS_DEBUGGER)
#ifdef __old
	if( dump_info ) db_init(cisname);
#else
	if( usrdbg(DBG_ANY) )
                db_init(cisname);
#endif
#endif

	if( asmstream != NULL )
	{
	    outstream = asmstream, tokenize = FALSE;
	}
	else 
	{
	    outstream = objstream, tokenize = TRUE;
	}

#if 0
	if( !libfile && !nomodule )
#else
	if (!library_module)
#endif
	{
		if( tokenize )
		{
			StringSegList s;
			align();
			aputc_( asm_module ); aprintf("-1");
			aprintf("\n.ModStart:");
			putword_( 0x60f160f1 );
			aprintf("%c.ModEnd-.ModStart",asm_word);
			s.strsegbase = cisname;
			s.strseglen = strlen(cisname);
			cdr_((List *)&s) = NULL;
			putstring( &s );
			aputc_( asm_bss ); aputc_( 32-(s.strseglen+1) );
			aprintf("%cmodnum",asm_word);
			aputc_( asm_word ); aputc_( '1' );
			aprintf("%c.MaxData", asm_word );
			aputc_( asm_init );
		}
		else
		{		
		        align();
			aprintf("\tmodule\t-1\n");
			aprintf(".ModStart:\n");
			aprintf("\tword\t#60f160f1\n");
			aprintf("\tword\t.ModEnd-.ModStart\n");
			aprintf("\tblkb\t31,\"%s\" byte 0\n",cisname);
			aprintf("\tword\tmodnum\n");
			aprintf("\tword\t1\n");
			aprintf("\tword\t.MaxData\n");
			aprintf("\tinit\n");
		}
	}
	else
	{
		cc_msg ("library module ... no header\n");
	}
	
	stubhead = stubend = NULL;

	/* initialise fplib binders */
	fplib.real32op = lib_binder("real32op");
	fplib.real32rem = lib_binder("real32rem");
	fplib.real32gt = lib_binder("real32gt");
	fplib.real32eq = lib_binder("real32eq");

	fplib.real64op = lib_binder("real64op");
	fplib.real64rem = lib_binder("real64rem");
	fplib.real64gt = lib_binder("real64gt");
	fplib.real64eq = lib_binder("real64eq");

	fplib.real32toreal64 = lib_binder("real32toreal64");
	fplib.real64toreal32 = lib_binder("real64toreal32");
	fplib.int32toreal32 = lib_binder("int32toreal32");
	fplib.int32toreal64 = lib_binder("int32toreal64");
	fplib.real32toint32 = lib_binder("real32toint32");
	fplib.real64toint32 = lib_binder("real64toint32");

	/* debugging/stackcheck entries */
	debug.stackerror	= lib_binder("_stack_error");
	debug.notify_entry	= lib_binder("_notify_entry");
	debug.notify_return	= lib_binder("_notify_return");
	debug.notify_command	= lib_binder("_notify_command");

	/* memory checking ... Tony (22/12/94) */
	debug.memcheck_loadword		= lib_binder("checkloadword");
	debug.memcheck_storeword	= lib_binder("checkstoreword");
	debug.memcheck_loadshort	= lib_binder("checkloadshort");
	debug.memcheck_loadshortext	= lib_binder("checkloadshortext");
	debug.memcheck_storeshort	= lib_binder("checkstoreshort");
	debug.memcheck_loadbyte		= lib_binder("checkloadbyte");
	debug.memcheck_loadbyteext	= lib_binder("checkloadbyteext");
	debug.memcheck_storebyte	= lib_binder("checkstorebyte");
	
	/* builtins */
#ifdef NEW_OPERATE
	builtin._operate_void	= lib_binder("_operate_void");
	builtin._operate_word	= lib_binder("_operate_word");
#else
	builtin._operate	= lib_binder("_operate");
#endif
	builtin._direct		= lib_binder("_direct");
	builtin._setpri		= lib_binder("_setpri");	
	builtin._stackframe	= lib_binder("_stackframe");	

	/* memory checking ... Tony (22/12/94) */
	debug.memcheck_loadword	= lib_binder("checkloadword");
	debug.memcheck_storeword= lib_binder("checkstoreword");

	/*
	 * The following is used to keep track of which stubs I have
	 * already set up for the memory access functions. Tony (22/12/94).
	 */
	mac_stubs = 0;

	/*
	 * We assume that loads/stores are unsafe, but as we always check
	 * the memory_access_checks pragma is set, this is only relevant
	 * if we are performing memory access checking.
	 * Note that at this point the pragma's haven't been set up yet.
	 */
	safe_ldnl = safe_stnl = FALSE;
}
/*}}}*/
/*{{{  lib_binder */

Binder *lib_binder(name)
char *name;
{
	Symstr *s = sym_insert(name, s_identifier);
	return global_mk_binder(0,s,bitofstg_(s_extern),
		g_list6(SU_Other, t_fnap, te_int, 0, 1, 100, 0));
}

void asm_trailer()
{
#if defined(DBX) || defined(TARGET_HAS_DEBUGGER)
#ifdef __old
	if( dump_info ) db_tidy();
#else
	if( usrdbg(DBG_ANY) )
                db_tidy();        /* XXX NC 19 May 1995 */
#endif
#endif
	genfilenames();
	makestubs();
	if( !nodata ) gendata();
#if 0
	if( !libfile && !nomodule )
#else
	if (!library_module)
#endif
	{
		if( tokenize )
		{
			aprintf("%c.MaxData 0",asm_data);
			align();
			aprintf(".ModEnd:\n");
		}
		else
		{
			aprintf("\tdata\t.MaxData 0\n");
			align();
			aprintf(".ModEnd:\n");
		}
	}
}
/*}}}*/
/*{{{  obj_makestubs */

#ifndef __old

int suppress_module = 0;

void obj_makestubs() {}

#endif
/*}}}*/
/*{{{  Tracing */

static int tracelevel = 0;

struct traceproc
{
	struct traceproc *last;
	char 		 *proc;
	int		 level;
} *tracestack = NULL;

/*{{{  _push_trace */
void _push_trace(str)
char *str;
{
	struct traceproc *t = (struct traceproc *)malloc(sizeof(struct traceproc));
	trace("{ %s() [%d:%d] TOS[%d %d]",str,ida,fda,istackmodes[ida],istacklengths[ida]);
	tracelevel++;
	if( t == NULL ) return;
	t->last = tracestack;
	t->proc = str;
	t->level = tracelevel;
	tracestack = t;	
}
/*}}}*/
/*{{{  _pop_trace */

void _pop_trace() 
{
	struct traceproc *t = tracestack;
	tracelevel--;
	if( tracelevel <= 0 ) tracelevel = 0;
	if( t == NULL ) trace("} ??????");
	else
	{
		trace("} %s [%d:%d] TOS[%d %d]",t->proc,ida,fda,istackmodes[ida],istacklengths[ida]);
/*		trace("} %s [%d:%d]",t->proc,ida,fda); */
		tracestack = t->last;
		free(t);
	}
}
/*}}}*/
/*{{{  trace */

#ifndef __STDC__
void trace(str,va_alist)
char *str;
va_dcl;
#else
void trace( char *str, ... )
#endif
{
	int i = tracelevel;
	va_list a;
	if( tokenize ) return;
	va_start(a, str);
	aprintf("-- ");
	while(i--) putc(' ',outstream);	
	_vfprintf(outstream,str,a);
        va_end(a);
	putc('\n',outstream);
}
/*}}}*/
/*}}}*/
/*{{{  peepxmask */
/* A side effect of various load and cast operations is chains	*/
/* of sign-extend and mask operations. This routine attempts to	*/
/* simplify these. It is easier to peephole these out here than	*/
/* to try and avoid generating them in the first place.		*/

bool peepxmask(Tcode *t)
{
	Tcode *tn = t->next;
	Tcode *tnn = NULL;
	int x;

	if( t->op == p_xword ) x = t->opd.value;
	else if( t->op == p_mask ) x = 8|t->opd.value;

	x<<=4;

	if( tn != NULL )
	{
		if( tn->op == p_xword ) x |= tn->opd.value;
		else if( tn->op == p_mask ) x |= 8|tn->opd.value;
		else return FALSE;
		
		tnn = tn->next;
	}
	x<<=4;

	if( tnn != NULL )
	{
		if( tnn->op == p_xword ) x |= tnn->opd.value;
		else if( tnn->op == p_mask ) x |= 8|tnn->opd.value;
	}

	switch(x)
	{
	case 0x1a0:	/* xword 1; mask 2;			*/
	case 0xa00:	/* mask 2;				*/
	case 0x900:	/* mask 1;				*/
	case 0x200:	/* xword 2;				*/
	case 0x100:	/* xword 1;				*/
		return FALSE;

	case 0x910:	/* mask 1; xword 1;			*/
	case 0xa20:	/* mask 2; xword 2;			*/
			/* On T9 a xbword or xsword will do	*/
		if( lxxAllowed ) return TRUE;
		else return FALSE;
		

	case 0x1a2:	/* xword 1; mask 2; xword 2; -> xword 1	*/
		tnn->opd.value = t->opd.value;
		/* drop through */

	case 0x2a2:	/* xword 2; mask 2; xword 2; -> xword 2 */
	case 0x191:	/* xword 1; mask 1; xword 1; -> xword 1 */
	case 0xa29:	/* mask 2; xword 2; mask 1; -> mask 1	*/
	case 0xa2a:	/* mask 2; xword 2; mask 2; -> mask 2	*/
		t->op = p_noop;
		tn->op = p_noop;
trace("peephole %s %d; %s %d; %s %d; -> %s %d",
		(x&0x800)?"mask":"xword",(x>>8)&3,
		(x&0x80)?"mask":"xword",(x>>4)&3,
		(x&0x8)?"mask":"xword",x&3,
		(x&0x8)?"mask":"xword",tnn->opd.value);		
		return TRUE;
		
	case 0x291:	/* xword 2; mask 1; xword 1; -> mask 1; xword 1; */
		t->op = p_noop;
trace("peephole xword 2; mask 1; xword 1; -> mask 1; xword 1");
		return TRUE;

	case 0x190:	/* xword 1; mask 1; -> mask 1;		*/
	case 0x2a0:	/* xword 2; mask 2; -> mask 2;		*/
	case 0x290:	/* xword 2; mask 1; -> mask 1;		*/
	case 0x990:	/* mask 1; mask 1; -> mask 1		*/
	case 0xaa0:	/* mask 2; mask 2; -> mask 2		*/
	case 0xa90:	/* mask 2; mask 1; -> mask 1		*/
		t->op = p_noop;
trace("peephole %s %d; %s %d; -> %s %d",
		(x&0x800)?"mask":"xword",(x>>8)&3,
		(x&0x80)?"mask":"xword",(x>>4)&3,
		(x&0x80)?"mask":"xword",(x>>4)&3);
		return TRUE;

	default:
		cc_msg("Curious xword/mask sequence: %x\n",x);
		return FALSE;
	}
}
/*}}}*/
/*{{{  gencode */

gencode(name,external)
Symstr *name;
int external;
{
	/*
	 * While in gencode (), we do not need to check non-local loads and
	 * stores as we are assuming that they only access the module table
	 * and hence are valid. Tony (22/12/94)
	 */
	safe_load (); safe_store ();
	
	optimise();		/* invoke optimiser */
	        
	infolab = litlab = NULL;
		
	genstub_( type_ext, name , 0);

#if 0
	if( !libfile && (external || export_statics) ) 
#else
	if( !library_module && (external || export_statics) ) 
#endif
	{
		adddata( 0, name, LIT_FUNC, external, 0);
		dataloc += 4;
	}

	fnstack = 0;
	maxssp += maxcall;

	align();
	
	if( dump_info )
	{
		if( tokenize )
		{
			putword_( 0x60f760f7 );
			aprintf("%c.e.%s-.%s",asm_word,symname_(name),symname_(name));
			aprintf("%c%d%c%d%cmodnum",asm_word,maxssp,asm_word,maxvsp,asm_word);
			aprintf("%c_%s%c%c\n",asm_word,symname_(name),asm_bss,8);
		}
		else aprintf("\tword #60f760f7,.e.%s-.%s,%d,%d,modnum,_%s,0,0\n",
				symname_(name),symname_(name),
				maxssp,maxvsp,symname_(name));
	}
	
	if( !no_stack_checks || debug_notify > 0 || profile_option >= 1 ) 
	{
		infolab = nextlabel();
		infolab->refs=1;
		putlab(infolab);
	}

	if( proc_names )
	{
		if( tokenize )
		{
			StringSegList s;
			putword_( 0x60f360f3 );
			aprintf("%c.%s",asm_word,symname_(name));
			s.strsegbase = symname_(name);
			s.strseglen = strlen(s.strsegbase);
			cdr_((List *)&s) = NULL;
			putstring( &s );
			align();
		}
		else aprintf("\tword #60f360f3,.%s byte \"%s\",0 align\n",
				symname_(name),symname_(name));
	}

	if( tokenize ) aprintf("\n.%s:",symname_(name));
	else aprintf(".%s:\n",symname_(name));

	/* generate a stack check if required. This code assumes that the */
	/* scalar and vector stacks occupy the same block and grow	  */
	/* towards eachother.						  */
	if( !no_stack_checks && use_vecstack )
	{
		LabelNumber *l;
		putfc( f_ldl, 1 );
		putfc( f_ldnl, 1 );
		if( maxvsp != 0 ) putfc( f_ldnlp, maxvsp );
		putfc( f_ldlp, -(maxssp+STACK_SAFETY) );
		putop( op_gt );
		putfl( f_cj, l=nextlabel());
		l->refs++;
		putldpi(infolab);
		putfc( f_ldl, 1 );
		putsym( f_call, bindsym_(debug.stackerror), c_code );
		putlab(l);
		genstub_( type_ext, bindsym_(debug.stackerror), 1 );
	}

	if( debug_notify >= 1 || profile_option >= 1 )
	{
		putfc( f_ldlp, 0 );
		putldpi(infolab);
		putfc( f_ldl, 1 );
		putsym( f_call, bindsym_(debug.notify_entry), c_code );
		genstub_( type_ext, bindsym_(debug.notify_entry), 1 );
	}

	if( maxssp > 0 ) putfc( f_ajw, -maxssp );
	
	if( modtab != NULL ) 		/* load up module table if needed */
	{
		putfc( f_ldl, maxssp + 1 );
		putfc( f_ldnl, 0 );
		putfc( f_stl, maxssp - modtab->reallocal );
	}
	if( (modtab != NULL && ncalls>0) || maxvsp>0 || memory_access_checks)
					/* if we use vstack		 */
	{				/* build new descriptor		 */
					/* ... OR if we are checking mem */
					/* accesses. Tony (22/12/94)	 */
		putfc( f_ldl, maxssp + 1 );
		putfc( f_ldnl, 1 );
		if( maxvsp > 0 ) putfc( f_ldnlp, maxvsp );
		putfc( f_stl, maxssp - modtab->reallocal + 1);
	}
		
	if( owndata != NULL )		/* and own data pointer		*/
	{
		if( modtab != NULL ) 
		{
			putfc( f_ldl, maxssp - modtab->reallocal );
		}
		else {
			putfc( f_ldl, maxssp + 1 ); 
			putfc( f_ldnl, 0 );
		}
		ldnl_modnum();
                putfc( f_stl, maxssp - owndata->reallocal );
	}

	/* check here whether a literal pool local has been allocated 	*/
	/* and whether any literals have been put in it.		*/
	if( litpool != NULL && litpoolhead != NULL )
	{
		if( litlab == NULL ) litlab = nextlabel();
		putldpi(litlab);
		putfc( f_stl, maxssp - litpool->reallocal );
	}
	
	if( real32op != NULL )
	{
		if( modtab != NULL ) 
		{
			putfc( f_ldl, maxssp - modtab->reallocal );
		}
		else {
			putfc( f_ldl, maxssp + 1 ); 
			putfc( f_ldnl, 0 );
		}
		if( tokenize )
		{
			aprintf("%c@_real32op",0x80|f_ldnl);
			aprintf("%c_real32op",0x80|f_ldnl);
		}
		else
		{
			aprintf("\tldnl\t@_real32op\n");
			aprintf("\tldnl\t_real32op\n");
		}
                putfc( f_stl, maxssp - real32op->reallocal );
	}

	if( real64op != NULL )
	{
		if( modtab != NULL ) 
		{
			putfc( f_ldl, maxssp - modtab->reallocal );
		}
		else {
			putfc( f_ldl, maxssp + 1 ); 
			putfc( f_ldnl, 0 );
		}
		if( tokenize )
		{
			aprintf("%c@_real64op",0x80|f_ldnl);
			aprintf("%c_real64op",0x80|f_ldnl);
		}
		else
		{
			aprintf("\tldnl\t@_real64op\n");
			aprintf("\tldnl\t_real64op\n");
		}
                putfc( f_stl, maxssp - real64op->reallocal );
	}

	if( mshortvar != NULL )
	{
		putfc( f_ldc, 0xFFFF );
		putfc( f_stl, maxssp - mshortvar->reallocal );
	}
	if( xshortvar != NULL )
	{
		putfc( f_ldc, 0x8000 );
		putfc( f_stl, maxssp - xshortvar->reallocal );
	}

	unsafe_load (); unsafe_store ();
	
	outputCode(topblock);

    if( dump_info )
    {
	aprintf(".e.%s:\n",symname_(name));
    }

    dumplits();

}
/*}}}*/
/*{{{  outputCode */

outputCode(b)
Block *b;
{
	LabelNumber *pendinglab = NULL;
	LabelNumber *pendinglab2 = NULL;
	Xop pendop = -1;

	while(b != NULL )
	{
		if(debugging(DEBUG_CG))
			trace("doing block %d(%d) %s jump %s pending op %s lab %d lab2 %d",
				b->lab->name,b->lab->refs,
				(b->flags&BlkFlg_alive)?"alive":"dead",
				fName(b->jump),fName(pendop),
				(pendop==-1||pendop==p_noop)?-1:pendinglab->name,
				pendop!=f_cj?-1:pendinglab2->name);

		if( b->flags & BlkFlg_alive )
		{
			/* for the first real block in the procedure,	*/
			/* fake a jump to it. This is to get the generation */
			/* of its label correct.			*/
			if( pendop == -1 )
			{
				pendop = f_j;
				pendinglab = b->lab;
			}
			switch( pendop )
			{
			case f_cj:
				/* if we are about to do		*/
				/* cj x ; j y; x:			*/
				/* actually generate 			*/
				/* eqc 0; cj y; x:			*/
				if( pendinglab2 == b->lab )
				{
					putfc( f_eqc, 0 );
					putfl( f_cj, pendinglab );
					if( b->lab->refs > 1 ) putlab(b->lab);
					break;
				}
				/* if both branches go the same way, just jump */
				if( pendinglab == pendinglab2 )
				{
					putfl( f_j, pendinglab);
					putlab(b->lab);
					break;
				}
				/* else put out a normal cj	*/
				putfl( f_cj, pendinglab2 );
				/* & drop through to do jump */
			case p_j:
			case f_j:
				if( pendinglab == b->lab )
				{
					/* when we have a pending jump to the */
					/* block we are about to generate,    */
					/* only output the label if there is  */
					/* more than 1 ref to it.	      */
					if( pendinglab->refs > 1 ) putlab(b->lab);
					break;
				}
				else if( pendop == p_j )
				{
					putfc( f_ldc, 0 );
					putfl( f_cj, pendinglab );
				}
				else putfl( f_j, pendinglab );
				putlab(b->lab);
				break;
				
			case p_noop:
				putlab(b->lab);
				break;
			}
			
			pendop = p_noop;
			pendinglab = NULL;

			outputBlock(b);

			switch( b->jump )
			{
			case f_cj:
				pendop = f_cj;
				pendinglab = b->succ1;
				pendinglab2 = b->succ2;
				break;
			case f_j:
			case p_j:
				pendop = b->jump;
				pendinglab = b->succ1;
				break;
				
			case p_case:
				{
					int i, size = b->operand2.tabsize;
					LabelNumber **tab = b->operand1.table;
					for( i = 0; i<size; i++ )
					if( tokenize ) aprintf("%c4%c..%d",asm_size,0x80|f_j,tab[i]->name);
					else aprintf("\tsize 4 j ..%d\n",tab[i]->name);
					pendinglab = NULL;
					pendop = p_noop;
					break;
				}
			}
		}
		b = b->next;
	}
	
	switch( pendop )
	{
	case p_noop: break;
	
	case f_j:
		putfl( f_j, pendinglab );
		break;
		
	case p_j:
		putfc( f_ldc, 0 );
		putfl( f_cj, pendinglab );
		break;
		
	case f_cj:
		putfl( f_cj, pendinglab2 );
		putfl( f_j, pendinglab );
		break;
	}	
}
/*}}}*/
/*{{{  outputBlock */

static int fpoptab[4] = { op_fpldnlmuldb, 
			  op_fpldnlmulsn, 
			  op_fpldnladddb, 
			  op_fpldnladdsn };

outputBlock(b)
Block *b;
{
  Tcode *t;

/*  trace ("@OutputBlock (): op codes are ..."); */
  
  for (t = b->code; t != NULL; t = t->next)
  {
     Xop op = t -> op;
     char *comment = NULL;

/*     trace ("\top: 0x%lx\t%s", (long)op, (op == p_infoline)?"(p_infoline)":""); */
     
     switch (op)
     {  /* Jumps never get here */

         case f_opr: /* Operations */
         {
            int x = 0;
            switch( t->opd.value )
            {
            case op_fpldnlsn: x |= 1;
            case op_fpldnldb: 
            	if( t->next != NULL )
            	{
            		Tcode *tn = t->next;
            		    if( tn->op == f_opr &&
            		    (tn->opd.value == op_fpadd || tn->opd.value == op_fpmul ) )
            		{
            			if( tn->opd.value == op_fpadd ) x |= 2;
				t->opd.value = fpoptab[x];
				tn->op = p_noop;
            		}
            	}
            	break;

            case op_lsx:	/* load short and sign extend */
            	if( t->next != NULL )
		{
			Tcode *tn = t->next;
			if( tn->op == p_xword && tn->opd.value == 2)
			{
trace("peephole lsx; mask 2 -> lsx");
				tn->op = p_noop;
			}
		}
		break;

            case op_lbx:	/* load byte and sign extend */
            	if( t->next != NULL )
		{
			Tcode *tn = t->next;
			if( tn->op == p_xword && tn->opd.value == 1)
			{
trace("peephole lbx; mask 2 -> lbx");
				tn->op = p_noop;
			}
		}
		break;

	    case op_ls:		/* load short */
            	if( t->next != NULL )
		{
			Tcode *tn = t->next;
			if( tn->op == p_mask && tn->opd.value == 2)
			{
trace("peephole ls; mask 2 -> ls");
				tn->op = p_noop;
			}
		}
		break;

	    case op_lb:		/* load byte */
            	if( t->next != NULL )
		{
			Tcode *tn = t->next;
			if( tn->op == p_mask && tn->opd.value == 1)
			{
trace("peephole lb; mask 1 -> lb");
				tn->op = p_noop;
			}
		}
		break;
#if 0
		/*
		 * NickG mentioned changing the code here, but do
		 * I need to?  Tony 9/1/95.
		 */
	    case op_ss:		/* store short (Tony 5/1/95) */
		TDebug_(cc_msg ("case op_ss in output block\n"));

		break;

	    case op_sb:		/* store byte (Tony 5/1/95) */
		TDebug_(cc_msg ("case op_sb in output block\n"));

		break;		
#endif
            }
            if( fpuNotAllowed ) switch( t->opd.value )
            {
		case op_fpurp          : t->opd.value = op_fprp; break;
		case op_fpurm          : t->opd.value = op_fprm; break;
		case op_fpurz          : t->opd.value = op_fprz; break;
		case op_fpur32tor64    : t->opd.value = op_fpr32tor64; break;
		case op_fpur64tor32    : t->opd.value = op_fpr64tor32; break;
		case op_fpuexpdec32    : t->opd.value = op_fpexpdec32; break;
		case op_fpuexpinc32    : t->opd.value = op_fpexpinc32; break;
		case op_fpuabs         : t->opd.value = op_fpabs; break;
/* ????		case op_fpunoround     : t->opd.value = op_fpnoround; break; */
		case op_fpuchki32      : t->opd.value = op_fpchki32; break;
		case op_fpuchki64      : t->opd.value = op_fpchki64; break;
		case op_fpudivby2      : t->opd.value = op_fpdivby2; break;
		case op_fpumulby2      : t->opd.value = op_fpmulby2; break;
		case op_fpurn          : t->opd.value = op_fprn; break;
		case op_fpuseterr      : t->opd.value = op_fpseterr; break;
		case op_fpuclrerr      : t->opd.value = op_fpclrerr; break;
            }
            putop( t -> opd.value );
            break;
	 }
	 
         case p_ldvl:
         case p_stvl:
         case p_ldvlp:
         {
		VLocal *v = t->opd.local;
		int offset = 0;
		Binder *b = v->b;
		Symstr *s = NULL;
		char *name = NULL;

		if( b != NULL ) s = bindsym_(b);
		if( s != NULL ) name = symname_(s);
		if( name == NULL )
		{
			if( v == mshortvar ) name = "0xFFFF";
			else if( v == xshortvar ) name = "0x8000";
			else if( v == litpool ) name = "..litpool";
			else name = "..temp";
		}
		
		switch( v->type )
		{
		case v_vec:
			if( op != p_ldvlp )
			    syserr("Gencode: attempt to access array directly");
			op = f_ldl;
			offset = fnstack + maxssp - v->reallocal;
			break;
			
		case v_var:
			op = (op == p_ldvl) ? f_ldl:
			     (op == p_stvl) ? f_stl: f_ldlp;
			offset = fnstack + maxssp - v->reallocal;
			break;

		case v_arg:
			op = (op == p_ldvl) ? f_ldl:
			     (op == p_stvl) ? f_stl: f_ldlp;
			offset = v->reallocal;
			break;
		}
		if( op != f_ldlp || t->next == NULL )
		{
			if( op == f_ldl && t->next != NULL &&
			    t->next->op == p_stvl &&
			    /*v->type == t->next->opd.local->type &&*/
			    v->reallocal == t->next->opd.local->reallocal )
			{
trace("peephole out ldl %d stl %d",v->reallocal, t->next->opd.local->reallocal );
				t = t->next;
			}
			else
			{
				
				putfcc( op, offset, name );
			}
			break;
		}
		t->op = f_ldlp;
		t->opd.value = offset;
		comment = name;

		/* drop through into ldlp case */
	}

		/* try a little peephole optimisation here		*/
	case f_ldlp:
		if( t->next != NULL )
		{
			Tcode *next = t->next;

			switch(next->op)
			{
				/* ldlp m ldnl n -> ldl m+n		*/
			case f_ldnl:
				next->op = f_ldl;
				next->opd.value += t->opd.value;
				break;	
			
				/* ldlp m stnl n -> stl m+n		*/
			case f_stnl:
				next->op = f_stl;
				next->opd.value += t->opd.value;
				break;	
			
				/* ldlp m ldnlp n -> ldlp m+n		*/
			case f_ldnlp:
				next->op = f_ldlp;
				next->opd.value += t->opd.value;
				break;	
			
			default:
				putfcc( t->op, t->opd.value, comment );
				break;
			}
		}
		break;
        
        case p_infoline:
        	if( t->opd.line->l == 0 ) break;
		if( t->opd.line->l > 100000 )
		{
			trace("strange line number in showcode %d",t->opd.line->l);
			break;
		}
               	if( ! tokenize ) 
               		aprintf("-- Line %d (%s)\n", t -> opd.line -> l,
                        	                     t -> opd.line -> f);
                        	                     
		if( debug_notify >= 5 || profile_option >= 2 )
		{
			LabelNumber *l = filenamelabel(t->opd.line->f);

			putldpi(l);
			putfc( f_ldc, t -> opd.line -> l );
			
			if( modtab ) /* &module table */
				putfc( f_ldlp, fnstack + maxssp - modtab->reallocal);
			else putfc( f_ldl, fnstack + maxssp + 1 );

			putsym( f_call, bindsym_(debug.notify_command), c_code );
			genstub_( type_ext, bindsym_(debug.notify_command), 1 );
		}
               break;
	case p_setv1:
		putfc( f_ldl, maxssp - modtab->reallocal + 1);
		putfc( f_ldnlp, -(t->opd.value) );
		break;
	case p_setv2:	
		putfc( f_stl, maxssp - (t->opd.local)->reallocal );
		break;

	case p_ldx:
		loaddataptr(t->opd.binder);
		putsym( f_ldnl, bindsym_(t->opd.binder), c_ext );
		break;

	case p_stx:
		loaddataptr(t->opd.binder);
		putsym( f_stnl, bindsym_(t->opd.binder), c_ext );
		break;

	case p_ldxp:
		loaddataptr(t->opd.binder);
#if 1
		{
			int offset = 0;
			Binder *b = t->opd.binder;
			op = f_ldnlp;

			/* peephole optimiser, convert 			*/
			/*   ldnlp _X {ldnlp n} ld/stnl m   into	*/
			/*   ld/stnl _X+n+m				*/
			while( t->next != NULL && t->next->op == f_ldnlp )
			{
				t = t->next;
				offset += t->opd.value;
			}
			if( t->next != NULL && t->next->op == f_ldnl )
			{
				t = t->next;
				offset += t->opd.value;
				op = t->op;
			}
			else if( t->next != NULL && t->next->op == f_stnl )
			{
				t = t->next;
				offset += t->opd.value;
				op = t->op;
			}
			if( offset == 0 ) putsym( op, bindsym_(b), c_ext);
			else putsymoff( op, bindsym_(b), c_ext, offset);
		}
#else
		putsym( f_ldnlp, bindsym_(t->opd.binder), c_ext );
#endif
		break;

	case p_lds:
		loaddataptr(t->opd.binder);
		putfd( f_ldnl, t->opd.binder);
		break;

	case p_sts:
		loaddataptr(t->opd.binder);
		putfd( f_stnl, t->opd.binder);
		break;

	case p_ldsp:
		loaddataptr(t->opd.binder);
#if 1
		{
			int offset = 0;
			Binder *b = t->opd.binder;
			op = f_ldnlp;

			/* peephole optimiser, convert 			*/
			/*   ldnlp _X {ldnlp n} ld/stnl m   into	*/
			/*   ld/stnl _X+n+m				*/
			while( t->next != NULL && t->next->op == f_ldnlp )
			{
				t = t->next;
				offset += t->opd.value;
			}
			if( t->next != NULL && t->next->op == f_ldnl )
			{
				t = t->next;
				offset += t->opd.value;
				op = t->op;
			}
			else if( t->next != NULL && t->next->op == f_stnl )
			{
				t = t->next;
				offset += t->opd.value;
				op = t->op;
			}
			if( offset == 0 ) putfd( op, b );
			else putfdoff( op, b, offset);
		}
#else
		putfd( f_ldnlp, t->opd.binder);
#endif
		break;

	case p_ldpi:
		putldpi( t->opd.label );
		break;

	case p_ldpf:
		if( tokenize ) aprintf("%c.%s-2",0x80|f_ldc,symname_(bindsym_(t->opd.binder)));
		else aprintf("\tldc\t.%s-2\n",symname_(bindsym_(t->opd.binder)));
		putop( op_ldpi );
		break;

	case p_fnstack:
		fnstack += t->opd.value;
		if( t->opd.value ) putfc( f_ajw, -t->opd.value );
		break;

	case p_call:
		if( modtab ) /* &module table */
			putfc( f_ldlp, fnstack + maxssp - modtab->reallocal);
		else putfc( f_ldl, fnstack + maxssp + 1 );

		putsym( f_call, t->opd.sym, c_code );
		genstub_( type_ext, t->opd.sym, 1 );
		break;

	case p_callpv:
	{
		int local = ((VLocal *)bindxx_(t->opd.binder))->reallocal;
		if( modtab ) /* &module table */
			putfc( f_ldlp, fnstack + maxssp - modtab->reallocal);
		else putfc( f_ldl, fnstack + maxssp + 1 );
		putfl( f_call, genpvstub(fnstack+maxssp+4-local) );
		break;
	}
	
	case p_fpcall:
	{
		int local = t->opd.local->reallocal;
		putfl( f_call, genpvstub(maxssp+4-local) );
		break;
	}

	case p_ret:
		if( debug_notify >= 1 || profile_option >= 1 )
		{
			putldpi(infolab);
			if( modtab ) /* &module table */
				putfc( f_ldlp, fnstack + maxssp - modtab->reallocal);
			else putfc( f_ldl, fnstack + maxssp + 1 );
			
			putsym( f_call, bindsym_(debug.notify_return), c_code );
			genstub_( type_ext, bindsym_(debug.notify_return), 1 );
		}

		if( maxssp > 0 ) putfc( f_ajw, maxssp);
		putop( op_ret );
		break;
	
	case p_j: 
		putfc(f_ldc,0);
	        putfl(f_cj,t -> opd.label);
	case p_noop:
	case p_noop2:	
	        break;

	case p_stackframe:
		putfc( f_ldlp, fnstack + maxssp );
		break;

	case p_xword:
	{
		int width;

		if( peepxmask(t) ) continue;

		width = t->opd.value;
		if( lxxAllowed && !memory_access_checks )
		{
			int xwop = op_xbword;
			if( width == 2 ) xwop = op_xsword;
			putop( xwop );
		}
		else
		{
			if( width == 1 ) putfc( f_ldc, 0x80 );
			else
			{
				if( xshortvar != NULL ) 
					putfcc( f_ldl,
					fnstack + maxssp - xshortvar->reallocal,
					"0x8000");
				else putfc( f_ldc, 0x8000 ); 
			}
			putop( op_xword );
		}
		break;
	}

	case p_mask:
	{
		int width;

		if( peepxmask(t) ) continue;

		width = t->opd.value;
		
		if( width == 1 ) putfc( f_ldc, 0xFF );
		else
		{
			if( mshortvar != NULL ) 
				putfcc( f_ldl, 
					fnstack + maxssp - mshortvar->reallocal,
					"0xFFFF");
			else putfc( f_ldc, 0xFFFF );
		}
		putop( op_and );
		break;
	}
		
	/* Here we look for some simple peephole optimisations		*/
	case f_ldnlp:
		if( t->next != NULL )
		{
			Tcode *next = t->next;
			switch(next->op)
			{
				/* ldnlp m ldnl n -> ldnl m+n		*/
				/* ldnlp m stnl n -> stnl m+n		*/
				/* ldnlp m ldnlp n -> ldnlp m+n		*/
			case f_ldnl:
			case f_ldnlp:
			case f_stnl:
				next->opd.value += t->opd.value;
				/* go round main loop here so we pick up */
				/* ldnlp chains.			 */
				continue;
			}
		}

         default:     /* Primaries which take constants */ 
		putfc( t->op, t->opd.value );
		break;
       }
   }
   /* Flag this block as output */
   b -> lab -> refs = 0;
   
}
/*}}}*/
/*{{{  loaddataptr */
		
loaddataptr(b)
Binder *b;
{
	/*
	 * While in loaddataptr (), we do not need to check non-local loads
	 * and stores as we are assuming that they only access the module
	 * table and hence are valid. Tony (22/12/94)
	 */
	safe_load (); safe_store ();

	if( (bindstg_(b) & bitofstg_(s_static)) && owndata != NULL )
	{
		putfcc( f_ldl, fnstack + maxssp - owndata->reallocal, "static data" );
	}
	else if ( modtab != NULL )
	{
		putfcc( f_ldl, fnstack + maxssp - modtab->reallocal, "module table" );
		if( bindstg_(b) & bitofstg_(s_static) ) ldnl_modnum();
		else putat( f_ldnl, bindsym_(b));
	}
	else {
		putfcc( f_ldl, fnstack + maxssp + 1, "display");
		putfcc( f_ldnl, 0, "module table" );
		if( bindstg_(b) & bitofstg_(s_static) ) ldnl_modnum();
		else putat( f_ldnl, bindsym_(b));
	}

	unsafe_load (); unsafe_store ();
}
/*}}}*/
/*{{{  dumplits */

#ifdef OLDLITS
/*{{{  defunct? */
void dumplits()
{
	Literal *l = litpoolhead;
	if( l!=NULL) trace("Literals");
	for(;l != NULL; l = l->next ) 
	{
		switch( l->type )
		{
		case lit_string:
		{
			putlab( l->lab );
			putstring( (StringSegList *)l -> v.s );
			break;
		}
		case lit_floatSn:
		{ /* BYTE SEX MAY HURT HERE ... CHECK AND BEWARE */
		        if (debugging(DEBUG_CG))
		           trace("Single precision float");
			align();
			putlab( l->lab );
			putword_(l -> v.fb.val);
			break;
		}	
		case lit_floatDb:
		{ /* BYTE SEX MAY HURT HERE ... CHECK AND BEWARE */
		        if (debugging(DEBUG_CG))
		           trace("Double precision float");
			align();
			putlab( l->lab );
			putword_(l -> v.db.lsd);
			putword_(l -> v.db.msd);
			break;
		}	
		default:
			syserr("Unexpected literal type (%d)",l->type);
			break;
		}
	}
}
/*}}}*/
#else
void dumplits()
{
	Literal *l = litpoolhead;
	if( l != NULL )
	{
		trace("Literals");
		if( litlab == NULL ) litlab = nextlabel();
		litlab->refs++;
		align();
		putlab( litlab );
	}
	for(;l != NULL; l = l->next ) 
	{
		switch( l->type )
		{
		case lit_integer:
			putword_( l->v.i );
			break;

		case lit_string:
			putstring( (StringSegList *)l -> v.s );
			align();
			break;

		case lit_floatSn:
			/* BYTE SEX MAY HURT HERE ... CHECK AND BEWARE */
		        if (debugging(DEBUG_CG))trace("Single precision float");
			putword_(l -> v.fb.val);
			break;

		case lit_floatDb:
			/* BYTE SEX MAY HURT HERE ... CHECK AND BEWARE */
		        if (debugging(DEBUG_CG))trace("Double precision float");
			putword_(l -> v.db.lsd);
			putword_(l -> v.db.msd);
			break;

		default:
			syserr("Unexpected literal type (%d)",l->type);
			break;
		}
	}
}

#endif
/*}}}*/
/*{{{  Calling Stubs  */
/*{{{  genldnlstub */
/* Tony (22/12/94) */
LabelNumber * genldnlstub (mtp_offset)
int	mtp_offset;
{
	Stub *		s = stubhead;
	LabelNumber *	l;

	/* only generate one stub for each offset value */
	TDebug_(cc_msg ("genldnlstub (%d): checking stub list ...\n", mtp_offset));
	
	while (s != NULL)
	{
		TDebug_(cc_msg ("\t[type: %d, sym: %d]\n", s->type, (int)(s->sym)));
	    
		if ((s->type == type_ldnl) && ((int)s->sym == mtp_offset))
		{
			/* already done, so return this label */
			TDebug_(cc_msg ("genldnlstub (%d): found previous entry (%d)\n",
				    mtp_offset, ((LabelNumber *)(s->valid))->name));
			
			return (LabelNumber *)s->valid;
		}
		s = s->next;
	}

	/* new mtp_offset, so add to list */
	l = nextlabel ();
	genstub_(type_ldnl, mtp_offset, l);

	TDebug_(cc_msg ("genldnlstub (%d): new entry (%d)\n",
		    mtp_offset, l->name));
			
	return l;
}
/*}}} */
/*{{{  genstnlstub */
/* Tony (22/12/94) */
LabelNumber * genstnlstub (mtp_offset)
int	mtp_offset;
{
	Stub *		s = stubhead;
	LabelNumber *	l;

	/* only generate one stub for each offset value */

	TDebug_(cc_msg ("genstnlstub (%d): checking stub list ...\n", mtp_offset));

	while (s != NULL)
	{
		TDebug_(cc_msg ("\t[type: %d, sym: %d]\n", s->type, (int)(s->sym)));
	    
		if ((s->type == type_stnl) && ((int)s->sym == mtp_offset))
		{
			/* already done, so return this label */
			TDebug_(cc_msg ("genstnlstub (%d): found previous entry (%d)\n",
				    mtp_offset, ((LabelNumber *)(s->valid))->name));
			
			return (LabelNumber *)s->valid;
		}
		s = s->next;
	}

	/* new mtp offset, so add to list */
	l = nextlabel ();
	genstub_(type_stnl, mtp_offset, l);

	TDebug_(cc_msg ("genstnlstub (%d): new entry (%d)\n",
		    mtp_offset, l->name));
			
	return l;
}
/*}}}*/
/*{{{  genlbstub */
LabelNumber * genlbstub (mtp_offset)
int	mtp_offset;
{
	Stub *		s = stubhead;
	LabelNumber *	l;

	/* only generate one stub for each offset value */

	TDebug_(cc_msg ("genlbstub (%d): checking stub list ...\n", mtp_offset));

	while (s != NULL)
	{
		TDebug_(cc_msg ("\t[type: %d, sym: %d]\n", s->type, (int)(s->sym)));
	    
		if ((s->type == type_lb) && ((int)s->sym == mtp_offset))
		{
			/* already done, so return this label */
			TDebug_(cc_msg ("genlbstub (%d): found previous entry (%d)\n",
				    mtp_offset, ((LabelNumber *)(s->valid))->name));
			
			return (LabelNumber *)s->valid;
		}
		s = s->next;
	}

	/* new mtp offset, so add to list */
	l = nextlabel ();
	genstub_(type_lb, mtp_offset, l);

	TDebug_(cc_msg ("genlbstub (%d): new entry (%d)\n",
		    mtp_offset, l->name));
			
	return l;

}
/*}}}*/
/*{{{  genlbxstub */
LabelNumber * genlbxstub (mtp_offset)
int	mtp_offset;
{
	Stub *		s = stubhead;
	LabelNumber *	l;

	/* only generate one stub for each offset value */

	TDebug_(cc_msg ("genlbxstub (%d): checking stub list ...\n", mtp_offset));

	while (s != NULL)
	{
		TDebug_(cc_msg ("\t[type: %d, sym: %d]\n", s->type, (int)(s->sym)));
	    
		if ((s->type == type_lbx) && ((int)s->sym == mtp_offset))
		{
			/* already done, so return this label */
			TDebug_(cc_msg ("genlbxstub (%d): found previous entry (%d)\n",
				    mtp_offset, ((LabelNumber *)(s->valid))->name));
			
			return (LabelNumber *)s->valid;
		}
		s = s->next;
	}

	/* new mtp offset, so add to list */
	l = nextlabel ();
	genstub_(type_lbx, mtp_offset, l);

	TDebug_(cc_msg ("genlbxstub (%d): new entry (%d)\n",
		    mtp_offset, l->name));
			
	return l;

}
/*}}}*/
/*{{{  gensbstub */
LabelNumber * gensbstub (mtp_offset)
int	mtp_offset;
{
	Stub *		s = stubhead;
	LabelNumber *	l;

	/* only generate one stub for each offset value */

	TDebug_(cc_msg ("gensbstub (%d): checking stub list ...\n", mtp_offset));

	while (s != NULL)
	{
		TDebug_(cc_msg ("\t[type: %d, sym: %d]\n", s->type, (int)(s->sym)));
	    
		if ((s->type == type_sb) && ((int)s->sym == mtp_offset))
		{
			/* already done, so return this label */
			TDebug_(cc_msg ("gensbstub (%d): found previous entry (%d)\n",
				    mtp_offset, ((LabelNumber *)(s->valid))->name));
			
			return (LabelNumber *)s->valid;
		}
		s = s->next;
	}

	/* new mtp offset, so add to list */
	l = nextlabel ();
	genstub_(type_sb, mtp_offset, l);

	TDebug_(cc_msg ("gensbstub (%d): new entry (%d)\n",
		    mtp_offset, l->name));
			
	return l;

}
/*}}}*/
/*{{{  genlsstub */
LabelNumber * genlsstub (mtp_offset)
int	mtp_offset;
{
	Stub *		s = stubhead;
	LabelNumber *	l;

	/* only generate one stub for each offset value */

	TDebug_(cc_msg ("genlsstub (%d): checking stub list ...\n", mtp_offset));

	while (s != NULL)
	{
		TDebug_(cc_msg ("\t[type: %d, sym: %d]\n", s->type, (int)(s->sym)));
	    
		if ((s->type == type_ls) && ((int)s->sym == mtp_offset))
		{
			/* already done, so return this label */
			TDebug_(cc_msg ("genlsstub (%d): found previous entry (%d)\n",
				    mtp_offset, ((LabelNumber *)(s->valid))->name));
			
			return (LabelNumber *)s->valid;
		}
		s = s->next;
	}

	/* new mtp offset, so add to list */
	l = nextlabel ();
	genstub_(type_ls, mtp_offset, l);

	TDebug_(cc_msg ("genlsstub (%d): new entry (%d)\n",
		    mtp_offset, l->name));
			
	return l;

}
/*}}}*/
/*{{{  genlsxstub */
LabelNumber * genlsxstub (mtp_offset)
int	mtp_offset;
{
	Stub *		s = stubhead;
	LabelNumber *	l;

	/* only generate one stub for each offset value */

	TDebug_(cc_msg ("genlsxstub (%d): checking stub list ...\n", mtp_offset));

	while (s != NULL)
	{
		TDebug_(cc_msg ("\t[type: %d, sym: %d]\n", s->type, (int)(s->sym)));
	    
		if ((s->type == type_lsx) && ((int)s->sym == mtp_offset))
		{
			/* already done, so return this label */
			TDebug_(cc_msg ("genlsxstub (%d): found previous entry (%d)\n",
				    mtp_offset, ((LabelNumber *)(s->valid))->name));
			
			return (LabelNumber *)s->valid;
		}
		s = s->next;
	}

	/* new mtp offset, so add to list */
	l = nextlabel ();
	genstub_(type_lsx, mtp_offset, l);

	TDebug_(cc_msg ("genlsxstub (%d): new entry (%d)\n",
		    mtp_offset, l->name));
			
	return l;

}
/*}}}*/
/*{{{  genssstub */
LabelNumber * genssstub (mtp_offset)
int	mtp_offset;
{
	Stub *		s = stubhead;
	LabelNumber *	l;

	/* only generate one stub for each offset value */

	TDebug_(cc_msg ("genssstub (%d): checking stub list ...\n", mtp_offset));

	while (s != NULL)
	{
		TDebug_(cc_msg ("\t[type: %d, sym: %d]\n", s->type, (int)(s->sym)));
	    
		if ((s->type == type_ss) && ((int)s->sym == mtp_offset))
		{
			/* already done, so return this label */
			TDebug_(cc_msg ("genssstub (%d): found previous entry (%d)\n",
				    mtp_offset, ((LabelNumber *)(s->valid))->name));
			
			return (LabelNumber *)s->valid;
		}
		s = s->next;
	}

	/* new mtp offset, so add to list */
	l = nextlabel ();
	genstub_(type_ss, mtp_offset, l);

	TDebug_(cc_msg ("genssstub (%d): new entry (%d)\n",
		    mtp_offset, l->name));
			
	return l;

}
/*}}}*/
/*{{{  genpvstub */
LabelNumber *genpvstub( local )
int local;
{
	Stub *s = stubhead;
	LabelNumber *l;
	
	/* only generate one stub for each offset value */

	while( s != NULL )
	{
		if( (s->type == type_pv) && ((int)s->sym == local) )
		{
			return (LabelNumber *)s->valid;
		}
		s = s->next;
	}

	l = nextlabel();
	genstub_( type_pv, local, l );
	return l;
}
/*}}}*/
/*{{{  genstub */

void genstub( type, sym , valid)
int type;
Symstr *sym;
int valid;
{
	Stub *s = stubhead;

	TDebug_(
		switch (type)
		{
		case type_ext:
		    cc_msg ("genstub (type: %d, sym name: %s, valid: %d)\n",
			    type, symname_(sym), valid);
		    break;
		    
		default:
		    cc_msg ("genstub (type: %d, sym value: %d, valid: %d)\n",
			    type, (int)sym, valid);
		    break;
		}
	)
	
	/*
	 * The following messes up the stubs for the memory checking functions,
	 * so rather than add 2-8 more conditions, I've replaced the existing
	 * check with the inverse (I hope).  Tony 4/1/95.
	 */
	/* if (type != type_pv)  */
	if (type == type_ext || type == type_fp)
	{
	    while( s != NULL )
	    {
		/* slightly dodgy comparison	*/
		if( (s->type!=type_pv) && s->sym == sym ) 
		{
		    if( s->valid ) s->valid = valid;
		    return;
		}
		s = s->next;
	    }
	}

	/* there is no existing stub for this one, add it */
	s = Glob_Alloc(sizeof(Stub));
	s->next = NULL;
	s->type = type;
	s->valid = valid;
	s->sym = sym;
	if( stubend!=NULL ) stubend->next = s;
	if( stubhead==NULL ) stubhead = s;
	stubend = s;
}
/*}}}*/
/*}}}*/ /* Calling stubs ... */
/*{{{  makestubs */

makestubs()
{
	Stub *s = stubhead;

	/*
	 * For now I'm going to assume that all loads/stores in stubs
	 * are safe.  Tony 4/1/95.
	 */
	safe_load (); safe_store ();

	if( s!=NULL ) trace("Stubs");
	while( s != NULL )
	{
		switch( s->type )
		{
		case type_pv:
		{
			int local = (int)(s->sym);
			align();
			aprintf("..%d:\n",((LabelNumber *)(s->valid))->name);
			putfc( f_ldl, local);
			putop( op_gcall );
			break;
		}
		case type_ext:
#if 0
			if( s->valid && nomodule==0 && !libfile )
#else
			if( s->valid && !library_module)
#endif
			{
				align();
				aprintf(".%s:\n",symname_(s->sym));
				putfc( f_ldl, 1 );
				putfc( f_ldnl, 0 );
				putat( f_ldnl, s->sym );
				putsym( f_ldnl, s->sym, c_ext );
				putop( op_gcall );
			}
			break;

		/* memory access checking stubs */
		case type_ldnl:
		{
			int	mtp_offset = (int)(s->sym);

			trace ("\tload memory access checking code ... ");

			align ();
			aprintf ("..%d:\n", ((LabelNumber *)(s->valid))->name);
			
			/*
			 * The call used to get here put all the things we're interested
			 * in into the workspace, so simply read them back in.
			 * Tony 4/1/95.
			 */
			putfcc (f_ldl, 1, "load address (retrieved from stack)");
			putfcc (f_ldlp, mtp_offset, "load module table pointer");

			/*
			 * now registers look like ...
			 *
			 * A.	mtp pointer
			 * B.	address (to check)
			 * C.	return address (ignored)
			 */

			/* generate call to memory checking function */
			trace ("\t... call to checkloadword function ... ");
			putsym (f_call, bindsym_(debug.memcheck_loadword), c_code);

			/*
			 * After the call has finished, we need to restore B and C to their
			 * original state.  We use the old value for A in the workspace to
			 * temporarily hold the value loaded, and push the B and C.
			 */
			putfcc (f_stl, 1, "store value returned from function");
			putfcc (f_ldl, 3, "push old C reg");
			putfcc (f_ldl, 2, "push old B reg");
			putfcc (f_ldl, 1, "push new A reg");

			/* return to wherever the code was called from */
			putop (op_ret);

			/* make sure we have a stub for this function */
			if (no_mac_stub_(MAC_LDNLW))
			{
				genstub_(type_ext, bindsym_(debug.memcheck_loadword), 1);

				set_mac_stub_(MAC_LDNLW);
			}

			trace ("\t... end of load memory access checking");

			break;
		}
		case type_stnl:
		{
			int	mtp_offset = (int)(s->sym);

			trace ("\tstore memory access checking code ... ");

			align ();
			aprintf ("..%d:\n", ((LabelNumber *)(s->valid))->name);

			/*
			 * The call used to get here put all the things we're interested
			 * in into the workspace, so simply read them back in.
			 * Tony 4/1/95.
			 */
			putfcc (f_ldl, 2, "value to store (retrieved from stack)");
			putfcc (f_ldl, 1, "store address (retrieved from stack)");
			putfcc (f_ldlp, mtp_offset, "load module table pointer");

			/*
			 * now registers look like ...
			 *
			 * A.	mtp pointer
			 * B.	address to check
			 * C.	value to store
			 */

			/* generate call to memory checking function */
			trace ("\t... call to checkstoreword function ... ");
			putsym (f_call, bindsym_(debug.memcheck_storeword), c_code);

			/*
			 * After the call has finished, we need to put the old value for
			 * C into A.
			 */
			putfcc (f_ldl, 3, "push old C reg into A");

			/* return to wherever the code was called from */
			putop (op_ret);

			/* make sure we have a stub for this function */
			if (no_mac_stub_(MAC_STNLW))
			{
				genstub_(type_ext, bindsym_(debug.memcheck_storeword), 1);

				set_mac_stub_(MAC_STNLW);
			}

			trace ("\t... end of store memory access checking");

			break;
		}
		case type_lb:
		{
			int	mtp_offset = (int)(s->sym);

			trace ("\tload memory access checking code ... ");

			align ();
			aprintf ("..%d:\n", ((LabelNumber *)(s->valid))->name);
			
			/*
			 * The call used to get here put all the things we're interested
			 * in into the workspace, so simply read them back in.
			 * Tony 4/1/95.
			 */
			putfcc (f_ldl, 1, "load address (retrieved from stack)");
			putfcc (f_ldlp, mtp_offset, "load module table pointer");

			/*
			 * now registers look like ...
			 *
			 * A.	mtp pointer
			 * B.	address (to check)
			 * C.	return address (ignored)
			 */

			/* generate call to memory checking function */
			trace ("\t... call to checkloadbyte function ... ");
			putsym (f_call, bindsym_(debug.memcheck_loadbyte), c_code);

			/*
			 * After the call has finished, we need to restore B and C to their
			 * original state.  We use the old value for A in the workspace to
			 * temporarily hold the value loaded, and push the B and C.
			 */
			putfcc (f_stl, 1, "store value returned from function");
			putfcc (f_ldl, 3, "push old C reg");
			putfcc (f_ldl, 2, "push old B reg");
			putfcc (f_ldl, 1, "push new A reg");

			/* return to wherever the code was called from */
			putop (op_ret);

			/* make sure we have a stub for this function */
			if (no_mac_stub_(MAC_LDNLB))
			{
				genstub_(type_ext, bindsym_(debug.memcheck_loadbyte), 1);

				set_mac_stub_(MAC_LDNLB);
			}

			trace ("\t... end of load memory access checking");

			break;
		}
		case type_lbx:
		{
			int	mtp_offset = (int)(s->sym);

			trace ("\tload memory access checking code ... ");

			align ();
			aprintf ("..%d:\n", ((LabelNumber *)(s->valid))->name);
			
			/*
			 * The call used to get here put all the things we're interested
			 * in into the workspace, so simply read them back in.
			 * Tony 4/1/95.
			 */
			putfcc (f_ldl, 1, "load address (retrieved from stack)");
			putfcc (f_ldlp, mtp_offset, "load module table pointer");

			/*
			 * now registers look like ...
			 *
			 * A.	mtp pointer
			 * B.	address (to check)
			 * C.	return address (ignored)
			 */

			/* generate call to memory checking function */
			trace ("\t... call to checkloadbyteext function ... ");
			putsym (f_call, bindsym_(debug.memcheck_loadbyteext), c_code);

			/*
			 * After the call has finished, we need to restore B and C to their
			 * original state.  We use the old value for A in the workspace to
			 * temporarily hold the value loaded, and push the B and C.
			 */
			putfcc (f_stl, 1, "store value returned from function");
			putfcc (f_ldl, 3, "push old C reg");
			putfcc (f_ldl, 2, "push old B reg");
			putfcc (f_ldl, 1, "push new A reg");

			/* return to wherever the code was called from */
			putop (op_ret);

			/* make sure we have a stub for this function */
			if (no_mac_stub_(MAC_LDNLBX))
			{
				genstub_(type_ext, bindsym_(debug.memcheck_loadbyteext), 1);

				set_mac_stub_(MAC_LDNLBX);
			}

			trace ("\t... end of load memory access checking");

			break;
		}
		case type_sb:
		{
			int	mtp_offset = (int)(s->sym);

			trace ("\tstore memory access checking code ... ");

			align ();
			aprintf ("..%d:\n", ((LabelNumber *)(s->valid))->name);

			/*
			 * The call used to get here put all the things we're interested
			 * in into the workspace, so simply read them back in.
			 * Tony 4/1/95.
			 */
			putfcc (f_ldl, 2, "value to store (retrieved from stack)");
			putfcc (f_ldl, 1, "store address (retrieved from stack)");
			putfcc (f_ldlp, mtp_offset, "load module table pointer");

			/*
			 * now registers look like ...
			 *
			 * A.	mtp pointer
			 * B.	address to check
			 * C.	value to store
			 */

			/* generate call to memory checking function */
			trace ("\t... call to checkstorebyte function ... ");
			putsym (f_call, bindsym_(debug.memcheck_storebyte), c_code);

			/*
			 * After the call has finished, we need to put the old value for
			 * C into A.
			 */
			putfcc (f_ldl, 3, "push old C reg into A");

			/* return to wherever the code was called from */
			putop (op_ret);

			/* make sure we have a stub for this function */
			if (no_mac_stub_(MAC_STNLB))
			{
				genstub_(type_ext, bindsym_(debug.memcheck_storebyte), 1);

				set_mac_stub_(MAC_STNLB);
			}

			trace ("\t... end of store memory access checking");
			break;
		}
		case type_ls:
		{
			int	mtp_offset = (int)(s->sym);

			trace ("\tload memory access checking code ... ");

			align ();
			aprintf ("..%d:\n", ((LabelNumber *)(s->valid))->name);
			
			/*
			 * The call used to get here put all the things we're interested
			 * in into the workspace, so simply read them back in.
			 * Tony 4/1/95.
			 */
			putfcc (f_ldl, 1, "load address (retrieved from stack)");
			putfcc (f_ldlp, mtp_offset, "load module table pointer");

			/*
			 * now registers look like ...
			 *
			 * A.	mtp pointer
			 * B.	address (to check)
			 * C.	return address (ignored)
			 */

			/* generate call to memory checking function */
			trace ("\t... call to checkloadshort function ... ");
			putsym (f_call, bindsym_(debug.memcheck_loadshort), c_code);

			/*
			 * After the call has finished, we need to restore B and C to their
			 * original state.  We use the old value for A in the workspace to
			 * temporarily hold the value loaded, and push the B and C.
			 */
			putfcc (f_stl, 1, "store value returned from function");
			putfcc (f_ldl, 3, "push old C reg");
			putfcc (f_ldl, 2, "push old B reg");
			putfcc (f_ldl, 1, "push new A reg");

			/* return to wherever the code was called from */
			putop (op_ret);

			/* make sure we have a stub for this function */
			if (no_mac_stub_(MAC_LDNLS))
			{
				genstub_(type_ext, bindsym_(debug.memcheck_loadshort), 1);

				set_mac_stub_(MAC_LDNLS);
			}

			trace ("\t... end of load memory access checking");
			break;
		}
		case type_lsx:
		{
			int	mtp_offset = (int)(s->sym);

			trace ("\tload memory access checking code ... ");

			align ();
			aprintf ("..%d:\n", ((LabelNumber *)(s->valid))->name);
			
			/*
			 * The call used to get here put all the things we're interested
			 * in into the workspace, so simply read them back in.
			 * Tony 4/1/95.
			 */
			putfcc (f_ldl, 1, "load address (retrieved from stack)");
			putfcc (f_ldlp, mtp_offset, "load module table pointer");

			/*
			 * now registers look like ...
			 *
			 * A.	mtp pointer
			 * B.	address (to check)
			 * C.	return address (ignored)
			 */

			/* generate call to memory checking function */
			trace ("\t... call to checkloadshortext function ... ");
			putsym (f_call, bindsym_(debug.memcheck_loadshortext), c_code);

			/*
			 * After the call has finished, we need to restore B and C to their
			 * original state.  We use the old value for A in the workspace to
			 * temporarily hold the value loaded, and push the B and C.
			 */
			putfcc (f_stl, 1, "store value returned from function");
			putfcc (f_ldl, 3, "push old C reg");
			putfcc (f_ldl, 2, "push old B reg");
			putfcc (f_ldl, 1, "push new A reg");

			/* return to wherever the code was called from */
			putop (op_ret);

			/* make sure we have a stub for this function */
			if (no_mac_stub_(MAC_LDNLSX))
			{
				genstub_(type_ext, bindsym_(debug.memcheck_loadshortext), 1);

				set_mac_stub_(MAC_LDNLSX);
			}

			trace ("\t... end of load memory access checking");
			break;
		}
		case type_ss:
		{
			int	mtp_offset = (int)(s->sym);

			trace ("\tstore memory access checking code ... ");

			align ();
			aprintf ("..%d:\n", ((LabelNumber *)(s->valid))->name);

			/*
			 * The call used to get here put all the things we're interested
			 * in into the workspace, so simply read them back in.
			 * Tony 4/1/95.
			 */
			putfcc (f_ldl, 2, "value to store (retrieved from stack)");
			putfcc (f_ldl, 1, "store address (retrieved from stack)");
			putfcc (f_ldlp, mtp_offset, "load module table pointer");

			/*
			 * now registers look like ...
			 *
			 * A.	mtp pointer
			 * B.	address to check
			 * C.	value to store
			 */

			/* generate call to memory checking function */
			trace ("\t... call to checkstoreshort function ... ");
			putsym (f_call, bindsym_(debug.memcheck_storeshort), c_code);

			/*
			 * After the call has finished, we need to put the old value for
			 * C into A.
			 */
			putfcc (f_ldl, 3, "push old C reg into A");

			/* return to wherever the code was called from */
			putop (op_ret);

			/* make sure we have a stub for this function */
			if (no_mac_stub_(MAC_STNLS))
			{
				genstub_(type_ext, bindsym_(debug.memcheck_storeshort), 1);

				set_mac_stub_(MAC_STNLS);
			}

			trace ("\t... end of store memory access checking");

			break;
		}
		default:
			syserr("Unknown stub type: %d",s->type);	
		}
		s = s->next;
	}

	unsafe_load (); unsafe_store ();
}
/*}}}*/
/*}}}*/
/*{{{  File Names */

/*{{{  filename */
LabelNumber *filenamelabel(name)
char *name;
{
	FileName *f = filenamelist;
	char *s;
	while( f != NULL )
	{
		if( strcmp(name,f->name) == 0 ) return f->lab;
		f = f->next;
	}
	f = (FileName *)Glob_Alloc(sizeof(FileName));
	s = (char *)Glob_Alloc(pad_to_word(strlen(name)+1));
	f->next = filenamelist;
	f->lab = nextlabel();
	f->name = s;
	strcpy(s,name);
	filenamelist = f;
	return f->lab;
}
/*}}}*/
/*{{{  genfilenames */
genfilenames()
{
	FileName *f = filenamelist;
	if( f!=NULL ) trace("File Names");
	while( f != NULL )
	{
		f->lab->refs=1;
		align();
		putlab(f->lab);
		if( tokenize )
		{
			StringSegList s;
			putword_( 0x60f960f9 );
			aprintf("%cmodnum",asm_word);
			s.strsegbase = f->name;
			s.strseglen = strlen(f->name);
			cdr_((List *)&s) = NULL;
			putstring( &s );
		}
		else
		{
			aprintf("\tword #60f960f9, modnum ");
			aprintf("byte \"%s\",0\n",f->name);
		}
		f = f->next;
	}	
}
/*}}}*/
/*}}}*/
/*{{{  Instruction Encoding */
/*{{{  pfsize */
#define TargetBytesPerWord 4

int pfsize(n)
int n;
{
        int pfx = 0;

        if( n < 0 )
        {
                pfx = 1;
                n = (~n) >> 4;
        }

        for(; pfx <= TargetBytesPerWord*2; pfx++)
        {
                n = n>>4;
                if( n == 0 ) return pfx+1;
        }
        return pfx;
}
/*}}}*/
/*{{{  encode */
#define pbyte(x) (*pbytefn)(x)

encode(op,arg,pbytefn)
int op;
int arg;
void (*pbytefn)();
{

	op <<= 4;
        if ( arg < 0 ) encodestep( (~arg)>>4, TRUE, pbytefn);
        else if ( arg > 15 ) encodestep( arg>>4, FALSE, pbytefn);
        pbytefn( op | ( arg & 0xf ) );
}
/*}}}*/
/*{{{  encodestep */
encodestep( arg, negative, pbytefn)
int arg, negative;
void (*pbytefn)();
{
        if( arg > 15 )
        {
                encodestep( arg>>4, negative, pbytefn);
                pbytefn( (0x20 | ((negative ? ~arg : arg) & 0xf)) );
        }
        else pbytefn((negative ? 0x60 : 0x20 ) | (arg & 0xf) );
}
/*}}}*/
/*}}}*/
/*{{{  Instruction Generation */
/*{{{  putsym */
putsym(op,s,pch)
Xop op;
Symstr *s;
char pch;
{
#if 0
	/*
	 * Supposition - we don't need to check the validity of symbols as
	 * this will be handled by the assembler.  Either the symbol is valid,
	 * and hence will have a valid address, or it won't be and hence will
	 * not be known about ?  Tony 13/1/95
	 */

	/*
	 * We trap all non-local loads and stores here, if memory checking
	 * is enabled. Tony (22/12/94)
	 * Convert the ldnl into a ldnlp, and call putsym again with the same values,
	 * then output a call in the normal way.
	 */
	if (op == f_ldnl && check_load_memory_access)
	{
		trace ("\tmemory access checking ... converting ldnl %c%s",
				pch, symname_(s));
		putsym (f_ldnlp, s, pch);
		putflc (f_call, genldnlstub (fnstack + maxssp - modtab->reallocal + 4), "load word stub");

		return;
	}
	if (op == f_stnl && check_store_memory_access)
	{
		trace ("\tmemory access checking ... converting stnl %c%s",
				pch, symname_(s));
		putsym (f_ldnlp, s, pch);
		putflc (f_call, genstnlstub (fnstack + maxssp - modtab->reallocal + 4), "store word stub");

		return;
	}
#endif
	if( tokenize ) aprintf("%c%c%s",0x80|op,pch,symname_(s));
	else aprintf("\t%s\t%c%s\n",fName(op),pch,symname_(s));
}
/*}}}*/
/*{{{  putsymoff */
putsymoff(op,s,pch,off)
Xop op;
Symstr *s;
char pch;
int off;
{
	if( tokenize ) aprintf("%c%c%s+%d",0x80|op,pch,symname_(s),off);
	else aprintf("\t%s\t%c%s+%d\n",fName(op),pch,symname_(s),off);
}
/*}}}*/
/*{{{  putat */
putat(op,s)
Xop op;
Symstr *s;
{
	if( tokenize ) aprintf("%c@_%s",0x80|op,symname_(s));
        else aprintf("\t%s\t@_%s\n",fName(op),symname_(s));
}
/*}}}*/
/*{{{  putfc */

void putfc(op,val)
Xop op;
int val;
{    
	/*
	 * We trap all non-local loads and stores here, if memory checking
	 * is enabled. Tony (22/12/94)
	 */
	if (op == f_ldnl && check_load_memory_access)
	{
		/*
		 * If the offset (= val) is 0, A reg already contains the pointer
		 * address (?). Tony 4/1/95.
		 */
		trace ("\tmemory access checking ... converting ldnl %d", val);
#if 1
		if (val != 0)
		{
			putfc (f_ldnlp, val);
		}
#else
		putfc (f_ldnlp, val);
#endif
		putflc (f_call, genldnlstub (fnstack + maxssp - modtab->reallocal + 4), "load word stub");

		return;
	}
	if (op == f_stnl && check_store_memory_access)
	{
		/*
		 * If the offset (= val) is 0, A reg already contains the pointer
		 * address (?). Tony 4/1/95.
		 */
		trace ("\tmemory access checking ... converting stnl %d", val);
#if 1
		if (val != 0)
		{
			putfc (f_ldnlp, val);
		}
#else
		putfc (f_ldnlp, val);
#endif
		/*
		 * The module table offset is incremented by a further 4 to take into
		 * account the words pushed onto the stack by the "call".  Tony 4/1/95.
		 */
		putflc (f_call, genstnlstub (fnstack + maxssp - modtab->reallocal + 4), "store word stub");

		return;
	}

	if( op == f_ajw && val == 0 ) return;
	if( tokenize )
	{
		aputc_( asm_code );
		aputc_( pfsize(val) );
		encode( op, val, aputc );
	}
	else aprintf("\t%s\t%d\n",fName(op),val);
}
/*}}}*/
/*{{{  putfcc */

void putfcc(op,val,comment)
Xop op;
int val;
char *comment;
{    
	/*
	 * We trap all non-local loads and stores here, if memory checking
	 * is enabled. Tony (22/12/94)
	 */
	if (op == f_ldnl && check_load_memory_access)
	{
		/*
		 * If the offset (= val) is 0, A reg already contains the pointer
		 * address (?). Tony 4/1/95.
		 */
		trace ("\tmemory access checking ... converting ldnl %d", val);
#if 1
		if (val != 0)
		{
			putfcc (f_ldnlp, val, comment);
		}
#else
		putfcc (f_ldnlp, val, comment);
#endif
		putflc (f_call, genldnlstub (fnstack + maxssp - modtab->reallocal + 4), "load word stub");

		return;
	}
	if (op == f_stnl && check_store_memory_access)
	{
		/*
		 * If the offset (= val) is 0, A reg already contains the pointer	
		 * address (?). Tony 4/1/95.
		 */
		trace ("\tmemory access checking ... converting stnl %d", val);
#if 1
		if (val != 0)
		{
			putfcc (f_ldnlp, val, comment);
		}
#else
		putfcc (f_ldnlp, val, comment);
#endif
		/*
		 * The module table offset is incremented by a further 4 to take into
		 * account the words pushed onto the stack by the "call".  Tony 4/1/95.
		 */
		putflc (f_call, genstnlstub (fnstack + maxssp - modtab->reallocal + 4), "store word stub");

		return;
	}

	if( op == f_ajw && val == 0 ) return;
	if( tokenize )
	{
		aputc_( asm_code );
		aputc_( pfsize(val) );
		encode( op, val, aputc );
	}
	else aprintf("\t%s\t%d\t\t-- %s\n",fName(op),val,comment?comment:"");
}
/*}}}*/
/*{{{  putfd */

putfd(op,b)
Xop op;
Binder *b;
{
	/*
	 * We trap all non-local loads and stores here, if memory checking
	 * is enabled. Tony (22/12/94)
	 * Convert the ldnl into a ldnlp, and call putfd again with the same values,
	 * then output a call in the normal way.
	 */
	if (op == f_ldnl && check_load_memory_access)
	{
		trace ("\tmemory access checking ... converting ldnl ..dataseg+%d",
				bindaddr_(b) >> 2);
		putfd (f_ldnlp, b);
		putflc (f_call, genldnlstub (fnstack + maxssp - modtab->reallocal + 4), "load word stub");

		return;
	}
	if (op == f_stnl && check_store_memory_access)
	{
		trace ("\tmemory access checking ... converting stnl ..dataseg+%d",
				bindaddr_(b) >> 2);
		putfd (f_ldnlp, b);
		putflc (f_call, genstnlstub (fnstack + maxssp - modtab->reallocal + 4), "store word stub");

		return;
	}

	if( !tokenize )
	{
		if( nodata )
			aprintf("\t%s\t_%s\n",fName(op),symname_(bindsym_(b)));
		else aprintf("\t%s\t..dataseg+%d\t-- %s\n",fName(op),bindaddr_(b)>>2,symname_(bindsym_(b)));
	}
	else
	{
		if( nodata )
			aprintf("%c_%s",0x80|op,symname_(bindsym_(b)));
		else aprintf("%c..dataseg+%d",0x80|op,bindaddr_(b)>>2);
	}
}
/*}}}*/
/*{{{  putfdoff */

putfdoff(op,b,off)
Xop op;
Binder *b;
int off;
{
	if( !tokenize )
	{
		if( nodata )
			aprintf("\t%s\t_%s+%d\n",fName(op),symname_(bindsym_(b)),off);
		else aprintf("\t%s\t..dataseg+%d\t-- %s+%d\n",
			fName(op),(bindaddr_(b)>>2)+off,
			symname_(bindsym_(b)),off);
	}
	else
	{
		if( nodata )
			aprintf("%c_%s+%d",0x80|op,symname_(bindsym_(b)),off);
		else aprintf("%c..dataseg+%d",0x80|op,(bindaddr_(b)>>2)+off);
	}
}
/*}}}*/
/*{{{  putop */

void putop( op )
Xop op;
{
	if (check_load_memory_access)
	{
		switch (op)
		{
		case op_lb:
			/*
			 * Replace lb with a call to checkloadbyte ().  Tony 5/1/95/
			 */
			trace ("\tmemory access checking ... converting lb");
			putflc (f_call, genlbstub (fnstack + maxssp - modtab->reallocal + 4), "load byte stub");

			return;
		case op_lbx:
			/*
			 * Replace lbx with a call to checkloadbyteext ().  Tony 6/1/95
			 */
			trace ("\tmemory access checking ... converting lbx");
			putflc (f_call, genlbxstub (fnstack + maxssp - modtab->reallocal + 4), "load byte extend stub");

			return;
		case op_ls:
			/*
			 * Replace ls with a call to checkloadshort ().  Tony 5/1/95.
			 */
			trace ("\tmemory access checking ... converting ls");
			putflc (f_call, genlsstub (fnstack + maxssp - modtab->reallocal + 4), "load short stub");

			return;
		case op_lsx:
			/*
			 * Replace lsx with a call to checkloadshortext ().  Tony 5/1/95.
			 */
			trace ("\tmemory access checking ... converting lsx");
			putflc (f_call, genlsxstub (fnstack + maxssp - modtab->reallocal + 4), "load short extend stub");

			return;

		default:
			break;
		}
	}
	if (check_store_memory_access)
	{
		switch (op)
		{
		case op_sb:
			/*
			 * Replace sb with a call to checkstorebyte ().  Tony 5/1/95.
			 */
			trace ("\tmemory access checking ... converting sb");
			putflc (f_call, gensbstub (fnstack + maxssp - modtab->reallocal + 4), "store byte stub");

			return;
		case op_ss:
			/*
			 * Replace ss with a call to checkstoreshort ().  Tony 5/1/95.
			 */
			trace ("\tmemory access checking ... converting ss");
			putflc (f_call, genssstub (fnstack + maxssp - modtab->reallocal + 4), "store short stub");

			return;
		default:
			break;
		}
	}

	if( tokenize )
	{
		aputc_( asm_code );
		aputc_( pfsize(op) );
		encode( f_opr, op, aputc );
	}
	else aprintf("\t%s\n", opName(op));
}
/*}}}*/
/*{{{  putlab */

putlab(l)
LabelNumber *l;
{
        if( l->refs == 0 ) return;
	/* @@@@ note that aligning all labels makes dhrystone go a little */
	/* faster, but the code is larger.				  */
        if( (l->block != DUFF_ADDR) && ((l->block->flags & BlkFlg_align) != 0)) 
        	align();
        if( tokenize ) aprintf("\n..%d:",l->name);
	else aprintf("..%d: -- %d refs\n",l->name, l->refs);
}
/*}}}*/
/*{{{  putfl */

putfl(op,l)
Xop op;
LabelNumber *l;
{
	if( tokenize ) aprintf("%c..%d",0x80|op,l->name);
	else aprintf("\t%s\t..%d\n",fName(op),l->name);
}
/*}}}*/
/*{{{  putflc */

putflc(op,l,comment)
Xop op;
LabelNumber *l;
char *comment;
{
	if( tokenize ) aprintf("%c..%d",0x80|op,l->name);
	else aprintf("\t%s\t..%d\t\t-- %s\n",fName(op),l->name,comment?comment:"");
}
/*}}}*/
/*{{{  putldpi */

putldpi(l)
LabelNumber *l;
{
	if( tokenize ) 
	{
		aprintf("%c..%d-2",0x80|f_ldc,l->name);
		putop( op_ldpi );
	}
	else aprintf("\tldc\t..%d-2\n\tldpi\n",l->name);
}
/*}}}*/
/*{{{  align */

align()
{
	if( tokenize ) aputc_( asm_align );
	else aprintf("\talign\n");
}
/*}}}*/
/*{{{  ldnl_modnum */

ldnl_modnum()
{
	if( tokenize ) aprintf("%cmodnum",0x80|f_ldnl);
	else aprintf("\tldnl\tmodnum\n");	
}
/*}}}*/
/*{{{  putstring */
putstring(s)
StringSegList *s;
{
	while( s != NULL )
	{
		char * str = s->strsegbase;
		int size = s->strseglen;

		if( tokenize ) while( size != 0 ) 
		{
			int i;
			int bsize = size > 128 ? 128 : size;
	
			aputc_( asm_code );
			aputc_( bsize );
			for( i = 0; i < bsize; i++ ) aputc_(*str++);
			size -= bsize;			
		}
		else while( size != 0 )
		{
			int i;
			int bsize = size > 128 ? 128 : size;
	
			aprintf("\tbyte\t\"");
			for( i = 0; i < bsize; i++ ) sputc(*str++);
			aprintf("\"\n");		

			size -= bsize;
		}
		s = (StringSegList *)cdr_((List *)s);
	}
	
	if ( tokenize ) 
	{
		aputc_( asm_bss );
		aputc_( 1 );
	}
	else aprintf("\tbyte\t0\n");
}
/*}}}*/
/*{{{  putword */

void putword (word)
long word;
{
	if( tokenize )
	{
		aputc_( asm_code ); aputc_( 4 );
		aputc_( (word>>0)&0xff  );
		aputc_( (word>>8)&0xff  );
		aputc_( (word>>16)&0xff );
		aputc_( (word>>24)&0xff );
	}
	else aprintf("\tbyte\t#%02x,#%02x,#%02x,#%02x\n",
		(word>>0)&0xff,
		(word>>8)&0xff,
		(word>>16)&0xff,
		(word>>24)&0xff);
}
/*}}}*/
/*{{{  aprintf */

#ifndef __STDC__
void aprintf(str, va_alist)
char *str;
va_dcl;
#else
void aprintf(char *str, ... )
#endif
{
	va_list a;
	va_start(a, str);
	_vfprintf(outstream,str,a);
        va_end(a);
}
/*}}}*/
/*{{{  sputc */

sputc(c)
char c;
{
	switch( c )
	{
	case 0x07: aputc_('\\'); aputc_('a'); return;		
	case '\b': aputc_('\\'); aputc_('b'); return;
	case '\f': aputc_('\\'); aputc_('f'); return;	
	case '\n': aputc_('\\'); aputc_('n'); return;
	case '\r': aputc_('\\'); aputc_('r'); return;
	case '\t': aputc_('\\'); aputc_('t'); return;
	case 0x0b: aputc_('\\'); aputc_('v'); return;	
	case '\\': aputc_('\\'); aputc_('\\'); return;
	case '\?': aputc_('\\'); aputc_('\?'); return;	
	case '\"': aputc_('\\'); aputc_('\"'); return;
	case '\'': aputc_('\\'); aputc_('\''); return;	
	default: 
		if( isprint(c) ) aputc_(c);
		else aprintf("\\x0%02x",c&0xff);
		return;
	}
}
/*}}}*/
/*{{{  aputc */

void aputc(c)
char c;
{
	putc(c,outstream);
}
/*}}}*/
/*}}}*/
/*{{{  Data Initialisation */
/* data initialisation */
/*{{{  statics */

#define dvec_max 64
static unsigned char dvec[dvec_max];
static int vpos;
static LabelNumber *datalab;
static int labset;
static int datapos, datastart;
static int zeroes, datasize;
static Symstr *pendsym;
static int sympos;
static int symext;
DataCode *codelist0;
DataCode *codelist1;
/*}}}*/
/*{{{  init stack */

/* layout of init routine stack */
#define init_dataseg	0
#define init_module	1
#define init_ret	2
#define init_display	3
#define init_pri	4
/*}}}*/
/*{{{  gendata */

gendata()
{
	LabelNumber *database;
	int i;
	DataInit *d = datainitp;
	DataCode *c;

	if( d == NULL ) return;
	else trace("Data Initialization");
	
	/* Assume that loads/stores are safe.  Tony 4/1/95 */
	safe_load (); safe_store ();

	codelist0 = NULL;
	codelist1 = NULL;
	datalab = NULL;
	pendsym = NULL;
	sympos = zeroes = datasize = datastart = datapos = 0;

	(datalab=nextlabel())->refs++;
	labset = 0;

	while( d != NULL )
	{
		switch( d->sort ) 
		{
		case LIT_LABEL:
		        dosym_(d->rpt,d->len);
			break;

		case LIT_FUNC:
			codelist0 = g_list4(SU_Other,  codelist0, DC_FUNC, datapos, d->rpt );
#if 0
			if( !libfile )
#else
			if (!library_module)
#endif
			{
				dosym_(d->rpt,d->len);
				dataword_(LIT_NUMBER,0);
			}
			break;
			
		case LIT_HH:
		case LIT_BBH:
		case LIT_HBB:
		case LIT_STRING:
		case LIT_NUMBER:
			i = d->rpt;
			while( i-- ) dataword_(d->sort,d->val);
			break;

		case LIT_FPNUM:
		{
			FloatCon *f = (FloatCon *)d->val;
			i = d->rpt;
			while (i--)
				if (d->len == 4)
					dataword_(LIT_NUMBER,f->floatbin.fb.val);
				else {
					dataword_(LIT_NUMBER,f->floatbin.db.lsd);
					dataword_(LIT_NUMBER,f->floatbin.db.msd);
				}
			break;
		}

		case LIT_ADCON:
			if( (Symstr *)(d->len)==bindsym_(codesegment))
			{
				codelist0 = g_list4(SU_Other,  codelist0, DC_STRING, 
					datapos, d->val );
				dataword_(LIT_NUMBER,0);
				break;
			}
			else if( (Symstr *)(d->len)==bindsym_(datasegment))
			{
				codelist0 = g_list4(SU_Other,  codelist0, DC_STATIC,
					datapos, d->val );
				dataword_(LIT_NUMBER,0);
				break;
			}
			else {
				/* this is either an external data ref or one to
				   a function. Detect the latter by searching
				   Stubs list. In either case we must generate
				   into codelist1 to wait until external module
				   has initialised.
				*/
				Stub *s = stubhead;
				while( s != NULL )
				{
				    if( (s->type==type_ext) && 
						(s->sym == (Symstr *)d->len) )
				    {
					/* external function pointer */
					if( s->valid )
					    codelist1 = g_list4(SU_Other,  codelist1,
					      DC_EXTFUNC, datapos, d->len );
					/* local function pointer */
					else
					    codelist0 = g_list4(SU_Other,  codelist0,
					     DC_FUNC, datapos, d->len );
					break;
				    }
				    else s = s->next;
				}
				if( s == NULL )
				{
					/* external data pointer */
					codelist1 = g_list5(SU_Other,  codelist1, DC_EXTERN,
						datapos, d->len, d->val );
				}
				dataword_(LIT_NUMBER,0);
			}

		default:
			break;
		}
		d = (DataInit *)cdr_((List *)d);
	}

	dosym_(NULL,1);

	if( datasize > 0 ) genmove();
	
	/* Now generate the init routine */

	{
		LabelNumber *code0;
		LabelNumber *done;

		/* output init routine header */
		align();
		if( tokenize ) aputc_( asm_init );
		else aprintf("\tinit\n");
		putfc( f_ajw, -(init_ret) );
		putfc( f_ldl, init_display );
		putfc( f_ldnl, 0 );
		ldnl_modnum();
		putfc( f_stl, init_module );
		putfc( f_ldl, init_module );
		if( tokenize ) aprintf("%c..dataseg",0x80|f_ldnlp);
		else aprintf("\tldnlp\t..dataseg\n");
		putfc( f_stl, init_dataseg );

		code0=nextlabel();
		code0->refs++;

		if( codelist0 == NULL ) done = code0;
		else done = nextlabel(), done->refs++;
		
		putfc( f_ldl, init_pri );
		putfl( f_cj, code0 );

		putcodelist(codelist1);

		if( codelist0 == NULL )
		{
			putlab( done );
		}
		else
		{
			putfl( f_j, done );
			putlab( code0 );
			putcodelist(codelist0);
			putlab( done );
		}
		
		putfc( f_ajw, init_ret );
		putop( op_ret );
	}

	unsafe_load (); unsafe_store ();
}
/*}}}*/
/*{{{  putcodelist */

putcodelist(codelist)
DataCode *codelist;
{	
	DataCode *c = codelist;
	while( c != NULL )
	{
		switch( c->type )
		{
		case DC_MOVE:		/* constant data initialisation */
			putldpi( c->src.l );
			putfc( f_ldl, init_dataseg );
			if (c->dest != 0) putfc( f_adc, c->dest );
			putfc( f_ldc, c->size );
			putop( op_move );
			break;

		case DC_FUNC:		/* function pointer */
			if( tokenize ) aprintf("%c.%s-2",0x80|f_ldc,symname_(c->src.s));
			else aprintf("\tldc\t.%s-2\n",symname_(c->src.s));
			putop( op_ldpi ); 
#if COMMONDATA
			putfc( f_ldl, init_module  );
			putsym(f_stnl, c->src.s, c_ext);
#else
#if 0
			if( !libfile ) 
#else
			if (!library_module)
#endif
			{	/* normal files, store by offset into dataseg */
				putfc( f_ldl, init_dataseg );
				putfc( f_stnl, c->dest>>2 );
			}
			else {	/* library files, store by name	into module */
				putfc( f_ldl, init_module  );
				putsym(f_stnl, c->src.s, c_ext);
			}
#endif
			break;

		case DC_STRING:		/* own code reference (typically a string) */
			if( tokenize ) aprintf("%c..%d-2",0x80|f_ldc,c->src.i);
			else aprintf("\tldc\t..%d-2\n",c->src.i);
			putop( op_ldpi );
			putfc( f_ldl, init_dataseg );
			putfc( f_stnl, c->dest>>2 );
			break;

		case DC_STATIC:		/* own data reference */
			putfc( f_ldl, init_dataseg );
			putfc( f_ldnlp, c->src.i>>2 );
			if( c->src.i & 0x3 ) putfc( f_adc, c->src.i & 0x3 );
			putfc( f_ldl, init_dataseg );
			putfc( f_stnl, c->dest>>2 );
			break;

		case DC_EXTERN:		/* external data reference */
			putfc( f_ldl, init_display );
			putat( f_ldnl, c->src.s );
			putsym( f_ldnlp, c->src.s, c_ext );
			if( c->size != 0 )
				if( c->size % 4 == 0 ) 
					putfc( f_ldnlp, c->size>>2);
				else putfc( f_adc, c->size );
			putfc( f_ldl, init_dataseg );
			putfc( f_stnl, c->dest>>2 );
			break;

		case DC_EXTFUNC:	/* external function reference */
			putfc( f_ldl, init_display );
			putat( f_ldnl, c->src.s );
			putsym( f_ldnl, c->src.s, c_ext );
			putfc( f_ldl, init_dataseg );
			putfc( f_stnl, c->dest>>2 );
			break;
		}
		c = c->next;
	}
}
/*}}}*/
/*{{{  codeloc */

int32 codeloc(void)
{
	LabelNumber *l = nextlabel();
	l->refs++;
	putlab( l );
	return l->name;
}
/*}}}*/
/*{{{  codeseg_stringsegs */

/* this is only called between procedures */
void codeseg_stringsegs(s,i)
StringSegList *s;
int i;
{	
	putstring( s );
}
/*}}}*/
/*{{{  dataword */

/* the following code is meant to be host byte sex independant */
void dataword(sort,w)
int sort,w;
{
	char *c = (char *)&w;
	short *s = (short *)&w;
	
	switch ( sort )
	{
	case LIT_BBBB:
		databyte(c[0]);
		databyte(c[1]);
		databyte(c[2]);
		databyte(c[3]);
		break;

	case LIT_BBH:
		databyte(c[0]);
		databyte(c[1]);
		databyte(s[1]&0xff);
		databyte((s[1]>>8)&0xff);
		break;

	case LIT_HBB:
		databyte(s[0]&0xff);
		databyte((s[0]>>8)&0xff);
		databyte(c[2]);
		databyte(c[3]);
		break;

	case LIT_HH:
		databyte(s[0]&0xff);
		databyte((s[0]>>8)&0xff);
		databyte(s[1]&0xff);
		databyte((s[1]>>8)&0xff);
		break;

	case LIT_NUMBER:
		databyte((w>>0)&0xff);
		databyte((w>>8)&0xff);
		databyte((w>>16)&0xff);
		databyte((w>>24)&0xff);
		break;

	default:
		syserr("Unexpected data sort: %x",sort);
	}
}
/*}}}*/
/*{{{  databyte */

databyte(b)
int b;
{
	if( b == 0 ) { zeroes++; datapos++; return; }


	/* the following constant may need tuning */
	if( zeroes <= 16 ) 
	{
		while( zeroes ) databyte1(0), zeroes-- ;
	}
	else
	{	/* generate a move */
		genmove();
	}

	if( datasize == 0 )		/* first non zero byte ? */
		datastart = datapos;


	databyte1(b);
	datapos++;
}
/*}}}*/
/*{{{  databyte1 */

databyte1(b)
int b;
{
	if( vpos == dvec_max ) flushdata();
	dvec[vpos++] = b;
	datasize++;
}
/*}}}*/
/*{{{  flushdata */

flushdata()
{
	int i;
	if( vpos )
	{
		if( !labset ) putlab( datalab ), labset++;

		if( tokenize )
		{
			aputc_( asm_code );
			aputc_( vpos );
			for ( i = 0 ; i < vpos ; i++ ) aputc_( dvec[i] );
		}
		else
		{
			aprintf("\n\tbyte\t");
			for ( i = 0 ; i < vpos-1 ; i++ )
			{
				aprintf("%d,",dvec[i]);
			}
			aprintf("%d\n",dvec[vpos-1]);
		}
	}
	vpos = 0;
}
/*}}}*/
/*{{{  genmove */

genmove()
{
	if( datasize == 0 ) return;
	flushdata();
	codelist0 = g_list5(SU_Other,  codelist0, DC_MOVE, datastart, datalab, datasize );
	(datalab=nextlabel())->refs++;
	labset = datasize = zeroes = 0;
}
/*}}}*/
/*{{{  dosym */

void dosym(s,ext)
Symstr *s;
int ext;
{
	if( ((pendsym != NULL) && (ext||export_statics)) || 
		(s == bindsym_(datasegment)) )
	{
		if( pendsym == bindsym_(datasegment) )
		{
			if( tokenize ) aprintf("%c..dataseg %d",asm_data,(datapos-sympos)>>2);
			else aprintf("\tdata\t..dataseg %d\n",(datapos-sympos)>>2);
		}
		else if( pendsym != NULL )
		{
			if( symext )
			{
				if( tokenize ) aprintf("%c_%s",asm_global,symname_(pendsym));
				else aprintf("\tglobal\t_%s\n",symname_(pendsym));
			}
#if COMMONDATA
			if( tokenize ) aprintf("%c_%s %d\n",asm_common,
					symname_(pendsym),(datapos-sympos)>>2);
			else aprintf("\tcommon\t_%s %d\n",
					symname_(pendsym),(datapos-sympos)>>2);
#else
			if( tokenize ) aprintf("%c_%s %d\n",asm_data,
					symname_(pendsym),(datapos-sympos)>>2);
			else aprintf("\tdata\t_%s %d\n",
					symname_(pendsym),(datapos-sympos)>>2);

#endif				

		}
		pendsym = s;
		sympos = datapos;
		symext = ext;
	}
}
/*}}}*/
/*}}}*/
/*{{{  fName */

char *fName( f )
int f;
{
   switch ( f )
   {
    case f_j: return("j");
    case f_ldlp: return("ldlp");
    case f_ldnl: return("ldnl");
    case f_ldc: return("ldc");
    case f_ldnlp: return("ldnlp");
    case f_ldl: return("ldl");
    case f_adc: return("adc");
    case f_call: return("call");
    case f_cj: return("cj");
    case f_ajw: return("ajw");
    case f_eqc: return("eqc");
    case f_stl: return("stl");
    case f_stnl: return("stnl");
    case f_opr: return("opr");

    /* internal codes, used for debug output */
    case p_infoline	: return("infoline");
    case p_call		: return("call");
    case p_callpv	: return("callpv");
    case p_fnstack	: return("fnstack");
    case p_ldx		: return("ldx");
    case p_stx		: return("stx");
    case p_ldxp		: return("ldxp");
    case p_ret		: return("ret");
    case p_ldpi		: return("ldpi");
    case p_ldpf		: return("ldpf");
    case p_lds		: return("lds");
    case p_sts		: return("sts");
    case p_ldsp		: return("ldsp");
    case p_j		: return("ldc 0 cj");
    case p_noop		: return("noop");
    case p_ldvl		: return("ldvl");
    case p_stvl		: return("stvl");
    case p_ldvlp	: return("ldvlp");
    case p_xword	: return("xword");
    case p_mask		: return("mask");
    default: return ("INVALID FUNCTION");
   }
}
/*}}}*/
/*{{{  opName */

char *opName( op ) 
int op;
{
   switch (op)
   {
    case op_rev: return("rev");
    case op_lb: return("lb");
    case op_bsub: return("bsub");
    case op_endp: return("endp");
    case op_diff: return("diff");
#if 0
    case op_add: return("add");
#else
    case op_add: return("sum");
#endif
    case op_gcall: return("gcall");
    case op_in: return("in");
    case op_prod: return("prod");
    case op_gt: return("gt");
    case op_wsub: return("wsub");
    case op_out: return("out");
    case op_sub: return("sub");
    case op_startp: return("startp");
    case op_outbyte: return("outbyte");
    case op_outword: return("outword");
    case op_seterr: return("seterr");
    case op_resetch: return("resetch");
    case op_csub0: return("csub0");
    case op_stopp: return("stopp");
    case op_ladd: return("ladd");
    case op_stlb: return("stlb");
    case op_sthf: return("sthf");
    case op_norm: return("norm");
    case op_ldiv: return("ldiv");
    case op_ldpi: return("ldpi");
    case op_stlf: return("stlf");
    case op_xdble: return("xdble");
    case op_ldpri: return("ldpri");
    case op_rem: return("rem");
    case op_ret: return("ret");
    case op_lend: return("lend");
    case op_ldtimer: return("ldtimer");
    case op_testerr: return("testerr");
    case op_testpranal: return("testpranal");
    case op_tin: return("tin");
    case op_div: return("div");
    case op_dist: return("dist");
    case op_disc: return("disc");
    case op_diss: return("diss");
    case op_lmul: return("lmul");
    case op_not: return("not");
    case op_xor: return("xor");
    case op_bcnt: return("bcnt");
    case op_lshr: return("lshr");
    case op_lshl: return("lshl");
    case op_lsum: return("lsum");
    case op_lsub: return("lsub");
    case op_runp: return("runp");
    case op_xword: return("xword");
    case op_sb: return("sb");
    case op_gajw: return("gajw");
    case op_savel: return("savel");
    case op_saveh: return("saveh");
    case op_wcnt: return("wcnt");
    case op_shr: return("shr");
    case op_shl: return("shl");
    case op_mint: return("mint");
    case op_alt: return("alt");
    case op_altwt: return("altwt");
    case op_altend: return("altend");
    case op_and: return("and");
    case op_enbt: return("enbt");
    case op_enbc: return("enbc");
    case op_enbs: return("enbs");
    case op_move: return("move");
    case op_or: return("or");
    case op_csngl: return("csngl");
    case op_ccnt1: return("ccnt1");
    case op_talt: return("talt");
    case op_ldiff: return("ldiff");
    case op_sthb: return("sthb");
    case op_taltwt: return("taltwt");
    case op_sum: return("sum");
    case op_mul: return("mul");
    case op_sttimer: return("sttimer");
    case op_stoperr: return("stoperr");
    case op_cword: return("cword");
    case op_clrhalterr: return("clrhalterr");
    case op_sethalterr: return("sethalterr");
    case op_testhalterr: return("testhalterr");
    case op_dup: return("dup");
    case op_move2dinit: return("move2dinit");
    case op_move2dall: return("move2dall");
    case op_move2dnonzero: return("move2dnonzero");
    case op_move2dzero: return("move2dzero");
    case op_unpacksn: return("unpacksn");
    case op_postnormsn: return("postnormsn");
    case op_roundsn: return("roundsn");
    case op_ldinf: return("ldinf");
    case op_fmul: return("fmul");
    case op_cflerr: return("cflerr");
    case op_crcword: return("crcword");
    case op_crcbyte: return("crcbyte");
    case op_bitcnt: return("bitcnt");
    case op_bitrevword: return("bitrevword");
    case op_bitrevnbits: return("bitrevnbits");
    case op_wsubdb: return("wsubdb");
    case op_fpldndbi: return("fpldndbi");
    case op_fpchkerr: return("fpchkerr");
    case op_fpstnldb: return("fpstnldb");
    case op_fpldnlsni: return("fpldnlsni");
    case op_fpadd: return("fpadd");
    case op_fpstnlsn: return("fpstnlsn");
    case op_fpsub: return("fpsub");
    case op_fpldnldb: return("fpldnldb");
    case op_fpmul: return("fpmul");
    case op_fpdiv: return("fpdiv");
    case op_fpldnlsn: return("fpldnlsn");
    case op_fpremfirst: return("fpremfirst");
    case op_fpremstep: return("fpremstep");
    case op_fpnan: return("fpnan");
    case op_fpordered: return("fpordered");
    case op_fpnotfinite: return("fpnotfinite");
    case op_fpgt: return("fpgt");
    case op_fpeq: return("fpeq");
    case op_fpi32tor32: return("fpi32tor32");
    case op_fpi32tor64: return("fpi32tor64");
    case op_fpb32tor64: return("fpb32tor64");
    case op_fptesterr: return("fptesterr");
    case op_fprtoi32: return("fprtoi32");
    case op_fpstnli32: return("fpstnli32");
    case op_fpldzerosn: return("fpldzerosn");
    case op_fpldzerodb: return("fpldzerodb");
    case op_fpint: return("fpint");
    case op_fpdup: return("fpdup");
    case op_fprev: return("fprev");
    case op_fpldnladddb: return("fpldnladddb");
    case op_fpldnlmuldb: return("fpldnlmuldb");
    case op_fpldnladdsn: return("fpldnladdsn");
    case op_fpentry: return("fpentry");
    case op_fpldnlmulsn: return("fpldnlmulsn");
    case op_fpusqrtfirst: return("fpusqrtfirst");
    case op_fpusqrtstep: return("fpusqrtstep");
    case op_fpusqrtlast: return("fpusqrtlast");
    case op_fpurp: return("fpurp");
    case op_fpurm: return("fpurm");
    case op_fpurz: return("fpurz");
    case op_fpur32tor64: return("fpur32tor64");
    case op_fpur64tor32: return("fpur64tor32");
    case op_fpuexpdec32: return("fpuexpdec32");
    case op_fpuexpinc32: return("fpuexpinc32");
    case op_fpuabs: return("fpuabs");
    case op_fpunoround: return("fpunoround");
    case op_fpuchki32: return("fpuchki32");
    case op_fpuchki64: return("fpuchki64");
    case op_fpudivby2: return("fpudivby2");
    case op_fpumulby2: return("fpumulby2");
    case op_fpurn: return("fpurn");
    case op_fpuseterr: return("fpuseterr");
    case op_fpuclrerr: return("fpuclrerr");
    /* T425 instructions */
    case op_start: return "start";
    case op_testhardchan: return "testhardchan";
    case op_testldd: return "testldd";
    case op_testlde: return "testlde";
    case op_testlds: return "testlds";
    case op_teststd: return "teststd";
    case op_testste: return "testste";
    case op_teststs: return "teststs";
    case op_break: return "break";
    case op_clrj0break: return "clrj0break";
    case op_setj0break: return "setj0break";
    case op_testj0break: return "testj0break";
    case op_timerdisableh: return "timerdisableh";
    case op_timerdisablel: return "timerdisablel";
    case op_timerenableh: return "timerenableh";
    case op_timerenablel: return "timerenablel";
    case op_ldmemstartval: return "ldmemstartval";
    case op_pop: return "pop";
    case op_lddevid: return "lddevid";
    /* T9000 instructions */
    case op_gtu:	return "gtu";
    case op_fprange:	return "fprange";
    case op_fpge:	return "fpge";
    case op_fplg:	return "fplg";
    case op_settimeslice:return "settimeslice";
    case op_ldflags:	return "ldflags";
    case op_stflags:	return "stflags";
    case op_xbword:	return "xbword";
    case op_lbx:	return "lbx";
    case op_cb:		return "cb";
    case op_cbu:	return "cbu";
    case op_insphdr:	return "insphdr";
    case op_readbfr:	return "readbfr";
    case op_ldconf:	return "ldconf";
    case op_stconf:	return "stconf";
    case op_ldcnt:	return "ldcnt";
    case op_ssub:	return "ssub";
    case op_ldth:	return "ldth";
    case op_ldchstatus:	return "ldchstatus";
    case op_intdis:	return "intdis";
    case op_intenb:	return "intenb";
    case op_cir:	return "cir";
    case op_ss:		return "ss";
    case op_chantype:	return "chantype";
    case op_ls:		return "ls";
    case op_fpseterr:	return "fpseterr";
    case op_ciru:	return "ciru";
    case op_fprem:	return "fprem";
    case op_fprn:	return "fprn";
    case op_fpdivby2:	return "fpdivby2";
    case op_fpmulby2:	return "fpmulby2";
    case op_fpsqrt:	return "fpsqrt";
    case op_fprp:	return "fprp";
    case op_fprm:	return "fprm";
    case op_fprz:	return "fprz";
    case op_fpr32tor64:	return "fpr32tor64";
    case op_fpr64tor32:	return "fpr64tor32";
    case op_fpexpdec32:	return "fpexpdec32";
    case op_fpexpinc32:	return "fpexpinc32";
    case op_fpabs:	return "fpabs";
    case op_fpclrerr:	return "fpclrerr";
    case op_fpadddbsn:	return "fpadddbsn";
    case op_fpchki32:	return "fpchki32";
    case op_fpchki64:	return "fpchki64";
    case op_devlb:	return "devlb";
    case op_devsb:	return "devsb";
    case op_devls:	return "devls";
    case op_devss:	return "devss";
    case op_devlw:	return "devlw";
    case op_devsw:	return "devsw";
    case op_xsword:	return "xsword";
    case op_lsx:	return "lsx";
    case op_cs:		return "cs";
    case op_csu:	return "csu";
    case op_fpstall:	return "fpstall";
    case op_fpldall:	return "fpldall";
    case op_stshadow:	return "stshadow";
    case op_ldshadow:	return "ldshadow";
    case op_tret:	return "tret";
    case op_goprot:	return "goprot";
    case op_selth:	return "selth";
    case op_syscall:	return "syscall";
    case op_swapgstatus:return "swapgstatus";
    case op_wait:	return "wait";
    case op_signal:	return "signal";
    case op_timeslice:	return "timeslice";
    case op_insertqueue:return "insertqueue";
    case op_swaptimer:	return "swaptimer";
    case op_swapqueue:	return "swapqueue";
    case op_stopch:	return "stopch";
    case op_vout:	return "vout";
    case op_vin:	return "vin";
    case op_swapbfr:	return "swapbfr";
    case op_sethdr:	return "sethdr";
    case op_setchmode:	return "setchmode";
    case op_initvlcb:	return "initvlcb";
    case op_writehdr:	return "writehdr";
    case op_readhdr:	return "readhdr";
    case op_disg:	return "disg";
    case op_enbg:	return "enbg";
    case op_grant:	return "grant";
    case op_stmove2dinit:return "stmove2dinit";
    case op_causeerror:	return "causeerror";
    case op_unmkrc:	return "unmkrc";
    case op_mkrc:	return "mkrc";
    case op_irdsq:	return "irdsq";
    case op_erdsq:	return "erdsq";
    case op_stresptr:	return "stresptr";
    case op_ldresptr:	return "ldresptr";
    case op_devmove:	return "devmove";
    case op_icl:	return "icl";
    case op_fdcl:	return "fdcl";
    case op_ica:	return "ica";
    case op_fdca:	return "fdca";
    case op_nop:	return "nop";
    case op_ldprodid:	return "ldprodid";

    /* internal codes, used for debug output */
    case p_infoline	: return("infoline");
    case p_call		: return("call");
    case p_callpv	: return("callpv");
    case p_fnstack	: return("fnstack");
    case p_ldx		: return("ldx");
    case p_stx		: return("stx");
    case p_ldxp		: return("ldxp");
    case p_ret		: return("ret");
    case p_ldpi		: return("ldpi");
    case p_ldpf		: return("ldpf");
    case p_lds		: return("lds");
    case p_sts		: return("sts");
    case p_ldsp		: return("ldsp");
    case p_j		: return("ldc 0 cj");
    case p_noop		: return("noop");
    case p_ldvl		: return("ldvl");
    case p_stvl		: return("stvl");
    case p_ldvlp	: return("ldvlp");
    default: return ("INVALID OP");
   }
}
/*}}}*/
