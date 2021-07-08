/* $Id: debug.c,v 1.1 1995/05/19 11:38:17 nickc Exp $ */
#ifdef __old
#include "cchdr.h"
#include "AEops.h"
#include "cg.h"
#include "util.h"
#include <stdio.h>
#include <stdarg.h>
#else
#include "globals.h"
#include "builtin.h"
#include "store.h"
#include "aeops.h"
#include "cg.h"
#include "codebuf.h"
#include "util.h"
#include "mcdep.h"
#include <stdio.h>
#include <stdarg.h>
#endif

#ifdef __STDC__
#undef va_alist
#define va_alist ...
#endif

#define LAYOUT

#define sizeof_int 4
#define padsize(n,align) (-((-(n)) & (-(align))))

#ifndef CC420
extern int dataloc;
#endif
static int indent = 0;

FILE *dbfile = NULL;

struct stdef
{
	struct stdef	*next;
	int		done;
	TagBinder	*b;
} *stlist;

#ifndef __old

int usrdbgmask;

char dbg_name[4] = "tla";

static int lastdataloc = 0;

#endif


#ifdef __STDC__
void dbprintf( char *fmt, ... )
#else
void dbprintf(fmt, va_alist )
char *fmt;
#endif
{
	va_list a;
	
	va_start(a,fmt);
	
	vfprintf(dbfile,fmt,a);
	
	va_end(a);
}

db_init(name)
char *name;
{
	char dbname[30];
	char *p = dbname;

#ifndef	__old
	if( usrdbg(DBG_ANY) )
	{
		extern int export_statics;
		var_dump_info = 1;
		debug_notify = 5;
		var_sort_locals = 0;
		export_statics = TRUE;
	}
#endif

	strcpy(dbname,name);
	
	while ( *p && (*p != '.') ) p++;
	*p = 0;
	
	strcat(dbname,".dbg");
	
	dbfile = fopen(dbname,"w");

	dbprintf("void=#0;\n");	
	dbprintf("char=#1;\n");
	dbprintf("short=-2;\n");	
	dbprintf("int=-4;\n");	
	dbprintf("long=-4;\n");	

	dbprintf("signed char=-1;\n");
	
	dbprintf("unsigned short=#2;\n");
	dbprintf("unsigned int=#4;\n");
	dbprintf("unsigned long=#4;\n");

	dbprintf("enum=#4;\n");	
	dbprintf("float=.4;\n");
	dbprintf("double=.8;\n");
}

db_tidy()
{
	struct stdef *s = stlist;
#ifdef LAYOUT
	dbprintf("\n");
#endif
	while( s )
	{
		if( !s->done )
		{
			TagBinder *b = s->b;
			TypeExpr *x = global_list3(SU_Other,s_typespec,bitoftype_(s_struct),b);
			dbprintf("__struct_%s=",_symname(bindsym_(b)));
			db_type(x);
			dbprintf(";\n");
		}
		s = s->next;
	}
	fclose(dbfile);
}

void addstdef(b)
TagBinder *b;
{
	struct stdef *s = stlist;
	int done = (tagbindmems_(b) != 0);

	while( s )
	{
		if( s->b == b )
		{
			if( done ) s->done = 1;
			return;
		}
		s = s->next;
	}

	stlist = (struct stdef *)global_list3(SU_Other,stlist,done,b);
	
}

/* still to do here are the divisions into unsigned and long */
int db_type(x)
TypeExpr *x;
{
    SET_BITMAP m;
    int bitoff;
#if 0 /* Tony fix. 3/1/95 */
    TagMemList *l;
#else
    struct ClassMember *	l;
#endif
    TagBinder *b;
    int n, size;
    int padn=0;
            
    switch (h0_(x))
    {   
         case s_typespec:
            m = typespecmap_(x);
            switch (m & -m)    /* LSB - unsigned/long etc. are higher */
            {   
                case bitoftype_(s_char):
                    dbprintf("char");
                    return 1;
                    
                case bitoftype_(s_int):
                    if (m & BITFIELD)
                      cc_rerr("db_type <bit field> illegal - db_type(int) assumed");
                    if (m & bitoftype_(s_short))
                    {
                    	dbprintf("short");
                    	return 2;
                    }
                    else if (m & bitoftype_(s_long))
                    {
                    	dbprintf("long");
                    	return 4;
                    }
		    dbprintf("int");
		    return 4;
                    
                case bitoftype_(s_enum):
#if 0
                    b = typespectagbind_(x);
                    dbprintf("<");
                    if( *symname_(bindsym_(b)) != '<' ) 
                    	dbprintf("$%s;",symname_(bindsym_(b)));
                    for ( n=0; l != 0; l = l->memcdr)
                    {
                        dbprintf("%s:%d;",symname_(l->memsv),n);
                        n++;
                    }
                    dbprintf(">int");
#endif
		    dbprintf("enum");
                    return 4;
                    
                case bitoftype_(s_double):
                    if (m & bitoftype_(s_short)) 
                    { dbprintf("float"); return 4; }
                    else dbprintf("double");
                    return 8;
                    
                case bitoftype_(s_struct):
                    b = typespectagbind_(x);
                    l = tagbindmems_(b);
		    addstdef(b);
                    if( l == 0 )
                    {
                    	dbprintf("$%s",symname_(bindsym_(b)));
			return 0;
                    }
                    
                    dbprintf("{");
                    if( *symname_(bindsym_(b)) != '<' ) 
                    	dbprintf("$%s;",symname_(bindsym_(b)));
                    for (bitoff=n=0; l != 0; l = l->memcdr)
                    {
                        if( l->memsv == NULL ) dbprintf("pad%d:",padn++);
                        else dbprintf("%s:",symname_(l->memsv));
			
                    	if (l->u.membits)
                        {   
                            int k = evaluate(l->u.membits);
                            size = 0;
                            n = padsize(n, alignoftype(te_int));
                            if (k == 0) k = 32-bitoff;  /* rest of int */
                            if (k+bitoff > 32) size=sizeof_int, bitoff = 0; /* ovflow */
                            dbprintf("%%%d,%d",bitoff,k);
                            bitoff += k;
                        }
                        else
                        {   if (bitoff != 0) n += sizeof_int, bitoff = 0;
                            n = padsize(n, alignoftype(l->memtype));
                            size =  db_type(l->memtype);
                        }
                        dbprintf(":%d;",n);
                        n += size;
                    }
                    dbprintf("}");
                    if (bitoff != 0) n += sizeof_int, bitoff = 0;
                    return padsize(n,4);
                    
                case bitoftype_(s_union):
                    b = typespectagbind_(x);
                    l = tagbindmems_(b);
		    addstdef(b);
                    if( l == 0 )
                    {
                    	dbprintf("$%s",symname_(bindsym_(b)));
			return 0;
                    }
                    
                    dbprintf("{");
                    if( *symname_(bindsym_(b)) != '<' ) 
                    	dbprintf("$%s;",symname_(bindsym_(b)));
                    for (n=0; l != 0; l = l->memcdr)
                    {
                    	dbprintf("%s:",symname_(l->memsv));
                        n = max(n, l->u.membits ? sizeof_int : db_type(l->memtype));
                        dbprintf(":0;");
                    }
                    dbprintf("}");
                    return padsize(n,sizeof_int);
              
                case bitoftype_(s_typedefname):
		    dbprintf("%s",symname_(bindsym_(typespecbind_(x))));
                    return sizeoftype(bindtype_(typespecbind_(x)));
                    
                case bitoftype_(s_void):
                    dbprintf("void");
                    return 0;

                default: break;
            }
            /* drop through for now */
        default:
            syserr("db_type(%d,0x%x)", h0_(x), typespecmap_(x));
        case t_subscript:
            n = sizeoftype(typearg_(x));
            if (typesubsize_(x)) n *= evaluate(typesubsize_(x));
            else typesubsize_(x) = globalize_int(1),
                 cc_rerr("size of a [] array required, treated as [1]");
            dbprintf("[%d]",evaluate(typesubsize_(x)));
            db_type(typearg_(x));
            return n;
            
        case t_fnap:
	    dbprintf("()");
	    db_type(typearg_(x));
	    return 4;
	    
        case t_content:
        {
        	TypeExpr *x1 = typearg_(x);
		dbprintf("*");
		if
		(
			(h0_(x1) == s_typespec) &&
			(
				(typespecmap_(x1) & bitoftype_(s_struct)) ||
				(typespecmap_(x1) & bitoftype_(s_union))
			)
		)
		{
			b = typespectagbind_(x1);
			dbprintf("$%s",symname_(bindsym_(b)));
	    	}
		else db_type(x1);
		return 4;
	}
    }
}


db_bindlist(l,arglist)
SynBindList *l;
int arglist;
{
	SynBindList *x;
	int offset = arglist?8:(ssp*4);
	int size;

	for( x = l; x != NULL ; x = x->bindlistcdr )
	{
		Binder *b = x->bindlistcar;
		if( arglist || (bindstg_(b) & bitofstg_(s_auto)) )
		{
			dbprintf("%s:",symname_(bindsym_(b)));
			size = db_type(b->bindtype);
			size = pad_to_word(size);
			if( arglist )
			{
				dbprintf(":%d;",offset);
				if( size < 4 ) size = 4;
				offset += size;
			}
			else
			{
				if( use_vecstack && size > 8 ) size = 4;
				else if( size < 4 ) size = 4;
				offset += size;
				dbprintf(":%d;",-offset);
			}
		}
		else if( bindstg_(b) & bitofstg_(s_static) )
		{
			dbprintf("%s:",symname_(bindsym_(b)));
			db_type(b->bindtype);
			dbprintf(":%d:s;",bindaddr_(b));
		}
	}
}

db_proc(proc,formals,type)
Binder *proc;
SynBindList *formals;
TypeExpr *type;
{
#ifdef LAYOUT
	dbprintf("\n");
#endif
	dbprintf("%s:(",symname_(bindsym_(proc)));
	db_bindlist(formals,1);
	dbprintf(")");
	db_type(type);
	dbprintf(":%d=",dataloc);
#ifndef __old
	lastdataloc = dataloc + 4;
#endif
}

db_blockstart(x)
Cmd *x;
{
	CmdList *cl = cmdblk_cl_(x);
	SynBindList *bl = cmdblk_bl_(x);
	int startline = x->fileline.l;
	int endline = startline;	
	int i;
	
#ifdef LAYOUT
	dbprintf("\n");
	indent++;
	for( i=0; i<indent ; i++ ) dbprintf(" ");
#endif

	dbprintf("{%d",startline);

	while( cl != NULL )
	{
		Cmd *cmd = cmdcar_(cl);
		endline = cmd->fileline.l;
		dbprintf(",%d",endline);
		switch( h0_(cmd) )
		{
		default: break;
		
		case s_if:
			if( h0_(cmd2c_(cmd)) != s_block )
				dbprintf(",%d",cmd2c_(cmd)->fileline.l);
			if( cmd3c_(cmd) != 0 && h0_(cmd3c_(cmd)) != s_block )
				dbprintf(",%d",cmd3c_(cmd)->fileline.l);
			break;
			
		case s_do:	cmd = cmd1c_(cmd);	goto doloop;
		case s_for:	cmd = cmd4c_(cmd);
		doloop:
			if( h0_(cmd) != s_block )
				dbprintf(",%d",cmd->fileline.l);
		}
		cl = cdr_(cl);
	}
	dbprintf(";");
#ifdef LAYOUT
	dbprintf("\n");
	indent++;
	for( i=0; i<indent ; i++ ) dbprintf(" ");
#endif	
	db_bindlist(bl,0);
}

db_blockend(x)
Cmd *x;
{
#ifdef LAYOUT
	indent-=2;
#endif
	dbprintf("};\n");
}

db_static(d)
DeclRhsList *d;
{
	int stat = 0;
	SET_BITMAP stg = d->declstg;

	if( stg & b_fnconst ) return;
	
	switch( stg & -stg )
	{
	case bitofstg_(s_typedef):
		dbprintf("%s=",symname_(d->declname));
		db_type(d->decltype);
		dbprintf(";\n");
		break;
		
        default:
		syserr("db_static(0x%x)", stg);
        case bitofstg_(s_auto):
		break;
	    
        case bitofstg_(s_static): stat = 1;
        case bitofstg_(s_extern):
            if (!(d->declstg & b_undef))
	    {
		dbprintf("%s:",symname_(d->declname));
		db_type(d->decltype);
		dbprintf(":%d:%c;\n",dataloc,stat?'s':'g');
		break;
	    }
            break;
	}
}

#ifndef __old

VoidStar dbg_notefileline(FileLine fl)
{
/*	if( dump_info ) dbprintf("dbg_notefileline %d %s\n",fl.l,fl.f); */
	return NULL;
}

void dbg_locvar(name, fl)
        Binder   *name;
        FileLine fl;
{
#if 0
	if( dump_info ) dbprintf("dbg_locvar %s @ %d\n",symname_(bindsym_(name)),dataloc);
#endif
}

void dbg_proc(Symstr *name, TypeExpr *t, bool ext, FileLine fl)
{
#if 0
	if( dump_info ) dbprintf("dbg_proc %s @ %d\n",symname_(name),dataloc);
#endif
}

void dbg_type(Symstr *name, TypeExpr *t)
{
	if( *symname_(name) != '<' )
	{
		dbprintf("%s=",symname_(name));
		db_type(t);
		dbprintf(";\n");
	}
}

void dbg_topvar(Symstr *name, int32 addr, TypeExpr *t, int stgclass,
                FileLine fl)
{
#if 0
	if( dump_info ) dbprintf("dbg_topvar %s @ %d\n",symname_(name),dataloc);
#endif
	if( dataloc != lastdataloc )
	{
		dbprintf("%s:",symname_(name));
		db_type(t);
		dbprintf(":%d:%c;\n",lastdataloc,stgclass?'g':'s');
		lastdataloc = dataloc;
	}
}

#endif
