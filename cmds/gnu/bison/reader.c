/* Input parser for bison
   Copyright (C) 1984, 1986 Bob Corbett and Free Software Foundation, Inc.

BISON is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY.  No author or distributor accepts responsibility to anyone
for the consequences of using it or for whether it serves any
particular purpose or works at all, unless he says so in writing.
Refer to the BISON General Public License for full details.

Everyone is granted permission to copy, modify and redistribute BISON,
but only under the conditions described in the BISON General Public
License.  A copy of this license is supposed to have been given to you
along with BISON so you can know your rights and responsibilities.  It
should be in a file named COPYING.  Among other things, the copyright
notice and this notice must be preserved on all copies.

 In other words, you are welcome to use, share and improve this program.
 You are forbidden to forbid anyone else to use, share and improve
 what you give them.   Help stamp out software-hoarding!  */

/* read in the grammar specification and record it in the format described in gram.h.
  All guards are copied into the fguard file and all actions into faction,
  in each case forming the body of a C function (yyguard or yyaction)
  which contains a switch statement to decide which guard or action to execute.

The entry point is reader().  */

#include <stdio.h>
#include <ctype.h>
#include "files.h"
#include "new.h"
#include "symtab.h"
#include "lex.h"
#include "gram.h"

static char *rcsid = "$Header: /dsl/HeliosRoot/Helios/cmds/gnu/bison/RCS/reader.c,v 1.1 1990/08/28 11:35:21 james Exp $";

#define   LTYPESTR   "\n#ifndef YYLTYPE\ntypedef\n  struct yyltype\n\
    {\n      int timestamp;\n      int first_line;\n      int first_column;\n\
      int last_line;\n      int last_column;\n      char *text;\n   }\n\
  yyltype;\n\n#define YYLTYPE yyltype\n#endif\n\n"

/* Number of slots allocated (but not necessarily used yet) in `rline'  */
int rline_allocated;

extern int definesflag;
extern bucket *symval;
extern int numval;

typedef
  struct symbol_list
    {
      struct symbol_list *next;
      bucket *sym;
      bucket *ruleprec;
    }
  symbol_list;



int lineno;
/* bucket *symval; */
symbol_list *grammar;
int start_flag;
bucket *startval;
char **tags;

static int typed;  /* nonzero if %union has been seen.  */

static int lastprec;  /* incremented for each %left, %right or %nonassoc seen */

static int gensym_count;  /* incremented for each generated symbol */

static bucket *errtoken;

static FILE *fattrs1;

reader()
{

  start_flag = 0;
  startval = NULL;  /* start symbol not specified yet. */

  translations = 0;  /* initially assume token number translation not needed.  */

  nsyms = 1;
  nvars = 0;
  nrules = 0;
  nitems = 0;
  rline_allocated = 10;
  rline = NEW2(rline_allocated, short);

  typed = 0;
  lastprec = 0;

  gensym_count = 0;

  semantic_parser = 0;
  pure_parser = 0;

  grammar = NULL;

  /* fattrs1 = ftable;  JF use fattrs instead  /* Unless/until fattrs is opened, use ftable instead.  */
  fattrs1 = fattrs;

  init_lex();
  lineno = 1;

  /* initialize the symbol table.  */
  tabinit();
  /* construct the error token */
  errtoken = getsym("error");
  errtoken->class = STOKEN;
  /* construct a token that represents all undefined literal tokens. */
  /* it is always token number 2.  */
  getsym("$illegal.")->class = STOKEN;
  /* Read the declaration section.  Copy %{ ... %} groups to ftable or fattrs file.
     Also notice any %token, %left, etc. found there.  */
  fprintf(ftable, "\n/*  A Bison parser, made from %s  */\n\n", infile);
  read_declarations();
  /* output the definition of YYLTYPE into the fattrs or ftable file.  */
  output_ltype();
  /* start writing the guard and action files, if they are needed.  */
  output_headers();
  /* read in the grammar, build grammar in list form.  write out guards and actions.  */
  readgram();
  /* write closing delimiters for actions and guards.  */
  output_trailers();
  /* assign the symbols their symbol numbers.
     Write #defines for the token symbols into fdefines if requested.  */
  packsymbols();
  /* convert the grammar into the format described in gram.h.  */
  packgram();
  /* free the symbol table data structure
     since symbols are now all referred to by symbol number.  */
  free_symtab();
}



/* read from finput until %% is seen.  Discard the %%.
Handle any % declarations,
and copy the contents of any %{ ... %} groups to ftable or fattrs.  */

read_declarations ()
{
  register int c;
  register int tok;

  for (;;)
    {
      c = skip_white_space();

      if (c == '%')
   {
     tok = parse_percent_token();

     switch (tok)
       {
       case TWO_PERCENTS:
         return;

       case PERCENT_LEFT_CURLY:
         copy_definition();
         break;

       case TOKEN:
         parse_token_decl (STOKEN, SNTERM);
         break;
   
       case NTERM:
         parse_token_decl (SNTERM, STOKEN);
         break;
   
       case TYPE:
         parse_type_decl();
         break;
   
       case START:
         parse_start_decl();
         break;
   
       case UNION:
         parse_union_decl();
         break;
   
       case LEFT:
         parse_assoc_decl(LEFT_ASSOC);
         break;

       case RIGHT:
         parse_assoc_decl(RIGHT_ASSOC);
         break;

       case NONASSOC:
         parse_assoc_decl(NON_ASSOC);
         break;

       case SEMANTIC_PARSER:
         semantic_parser = 1;
         open_extra_files();
         fattrs1 = fattrs;
         break;

       case PURE_PARSER:
         pure_parser = 1;
         break;

       default:
         fatal("junk after % in definition section");
       }
   }
      else if (c == EOF)
        fatal("no input grammar");
      else/* JF changed msg */
        fatals("Unrecognized char '%c' in declaration section",c);

    }
}


/* copy the contents of a %{ ... %} into the definitions file.
The %{ has already been read.  Return after reading the %}.  */
copy_definition ()
{
  register int c;
  register int match;
  register int ended;
  register int after_percent;  /* -1 while reading a character if prev char was % */

  fprintf(fattrs1, "#line %d \"%s\"\n", lineno, infile);

  after_percent = 0;

  c = getc(finput);

  for (;;)
    {
      switch (c)
   {
   case '\n':
     putc(c, fattrs1);
     lineno++;
     break;

   case '%':
          after_percent = -1;
     break;
         
   case '\'':
   case '"':
     match = c;
     putc(c, fattrs1);
     c = getc(finput);

     while (c != match)
       {
         if (c == EOF || c == '\n')
      fatal("unterminated string");

         putc(c, fattrs1);
         
         if (c == '\\')
      {
        c = getc(finput);
        if (c == EOF || c == '\n')
          fatal("unterminated string");
        putc(c, fattrs1);
        if (c == '\n')
          lineno++;
      }

         c = getc(finput);
       }

     putc(c, fattrs1);
     break;

   case '/':
     putc(c, fattrs1);
     c = getc(finput);
     if (c != '*')
       continue;

     putc(c, fattrs1);
     c = getc(finput);

     ended = 0;
     while (!ended)
       {
         if (c == '*')
      {
        while (c == '*')
          {
            putc(c, fattrs1);
            c = getc(finput);
          }

        if (c == '/')
          {
            putc(c, fattrs1);
            ended = 1;
          }
      }
         else if (c == '\n')
      {
        lineno++;
        putc(c, fattrs1);
        c = getc(finput);
      }
         else if (c == EOF)
      fatal("unterminated comment in %{ definition");
         else
      {
        putc(c, fattrs1);
        c = getc(finput);
      }
       }

     break;

   case EOF:
     fatal("unterminated %{ definition");

   default:
     putc(c, fattrs1);
   }

      c = getc(finput);

      if (after_percent)
   {
     if (c == '}')
       return;
     putc('%', fattrs1);
   }
      after_percent = 0;

    }

}



/* parse what comes after %token or %nterm.
For %token, what_is is STOKEN and what_is_not is SNTERM.
For %nterm, the arguments are reversed.  */

parse_token_decl (what_is, what_is_not)
     int what_is, what_is_not;
{
/*   register int start_lineno; JF */
  register int token = 0;
  register int prev;
  register char *typename = 0;
  int k;
  extern char token_buffer[];

/*   start_lineno = lineno; JF */

  for (;;)
    {
      if(ungetc(skip_white_space(), finput) == '%')
   return;

/*      if (lineno != start_lineno)
   return; JF */

      /* we have not passed a newline, so the token now starting is in this declaration */
      prev = token;

      if ((token = lex()) == TYPENAME)
   {
     k = strlen(token_buffer);
     if (typename) free (typename);
     typename = NEW2(k + 1, char);
     strcpy(typename, token_buffer);
   }
      else if (token == IDENTIFIER)
   {
     if (symval->class == what_is_not)
       fatals("symbol %s redefined", symval->tag);
     symval->class = what_is;
     if (what_is == SNTERM)
       symval->value = nvars++;

     if (typename)
       {
         if (symval->type_name == NULL)
      symval->type_name = typename;
         else
      fatals("type redeclaration for %s", symval->tag);
       }
   }
      else if (prev == IDENTIFIER && token == NUMBER)
        {
     symval->user_token_number = numval;
     translations = 1;
        }
      else
   fatal("illegal text in %token or %nterm declaration");
    }

}



/* parse what comes after %start */

parse_start_decl ()
{
  if (start_flag)
    fatal("multiple start declarations");
  start_flag = 1;
  if (lex() != IDENTIFIER)
    fatal("illegal start declaration");
  startval = symval;
}



/* read in a %type declaration and record its information for get_type_name to access */

parse_type_decl ()
{
  register int k;
  register char *name;
/*   register int start_lineno; JF */

  extern char token_buffer[];

  if (lex() != TYPENAME)
    fatal("ill-formed %type declaration");

  k = strlen(token_buffer);
  name = NEW2(k + 1, char);
  strcpy(name, token_buffer);

/*   start_lineno = lineno; */

  for (;;)
    {
      register int t;

      if(ungetc(skip_white_space(), finput) == '%')
   return;

/*       if (lineno != start_lineno)
   return; JF */

      /* we have not passed a newline, so the token now starting is in this declaration */

      t = lex();

      switch (t)
   {

   case COMMA:
     break;

   case IDENTIFIER:
     if (symval->type_name == NULL)
       symval->type_name = name;
     else
       fatals("type redeclaration for %s", symval->tag);

     break;

   default:
     fatal("illegal %type declaration");
   }
    }
}



/* read in a %left, %right or %nonassoc declaration and record its information.  */
/* assoc is either LEFT_ASSOC, RIGHT_ASSOC or NON_ASSOC.  */

parse_assoc_decl (assoc)
int assoc;
{
  register int k;
  register char *name = NULL;
/*  register int start_lineno; JF */
  register int prev = 0;   /* JF added = 0 to keep lint happy */

  extern char token_buffer[];

  lastprec++;  /* assign a new precedence level.  */

/*   start_lineno = lineno; */

  for (;;)
    {
      register int t;

      if(ungetc(skip_white_space(), finput) == '%')
   return;

      /* if (lineno != start_lineno)
   return; JF */

      /* we have not passed a newline, so the token now starting is in this declaration */

      t = lex();

      switch (t)
   {

   case TYPENAME:
     k = strlen(token_buffer);
     name = NEW2(k + 1, char);
     strcpy(name, token_buffer);
     break;

   case COMMA:
     break;

   case IDENTIFIER:
     symval->prec = lastprec;
     symval->assoc = assoc;
     if (symval->class == SNTERM)
       fatals("symbol %s redefined", symval->tag);
     symval->class = STOKEN;
     if (name)
       { /* record the type, if one is specified */
         if (symval->type_name == NULL)
      symval->type_name = name;
         else
      fatals("type redeclaration for %s", symval->tag);
       }
     break;

   case NUMBER:
     if (prev == IDENTIFIER)
            {
         symval->user_token_number = numval;
         translations = 1;
            }
          else     
       fatal("illegal text in association declaration");
     break;

   case SEMICOLON:
     return;

   default:
     fatal("malformatted association declaration");
   }

      prev = t;

    }
}



/* copy the union declaration into ftable or fattrs, where it is made into the
definition of YYSTYPE, the type of elements of the parser value stack.  */

parse_union_decl()
{
  register int c;
  register int count;
  register int in_comment;

  if (typed)
    fatal("multiple %union declarations");

  typed = 1;

  fprintf(fattrs1, "\n#line %d \"%s\"\n", lineno, infile);
  fprintf(fattrs1, "typedef union");
  if( definesflag )
  {
     fprintf(fdefines, "\n#line %d \"%s\"\n", lineno, infile);
     fprintf(fdefines, "typedef union");
  }

  count = 0;
  in_comment = 0;

  c = getc(finput);

  while (c != EOF)
    {
      putc(c, fattrs1);
      if( definesflag ) putc(c, fdefines);
      switch (c)
   {
   case '\n':
     lineno++;
     break;

   case '/':
     c = getc(finput);
     if (c != '*')
       ungetc(c, finput);
     else
       {
         putc('*', fattrs1);
         if( definesflag ) putc('*', fdefines);
         c = getc(finput);
         in_comment = 1;
         while (in_comment)
      {
        if (c == EOF)
          fatal("unterminated comment");

        putc(c, fattrs1);
        if( definesflag ) putc(c, fdefines);
        if (c == '*')
          {
            c = getc(finput);
            if (c == '/')
         {
           putc('/', fattrs1);
           if( definesflag ) putc('/', fdefines);
           in_comment = 0;
         }
          }
        else
          c = getc(finput);
      }
       }
     break;


   case '{':
     count++;
     break;

   case '}':
     count--;
     if (count == 0)
       {
         fprintf(fattrs1, " YYSTYPE;\n");
         if( definesflag ) fprintf(fdefines, " YYSTYPE;\n");
         /* JF don't choke on trailing semi */
         c=skip_white_space();
         if(c!=';') ungetc(c,finput);
         return;
       }
   }

      c = getc(finput);
    }
}

/* that's all of parsing the declaration section */


output_ltype()
{
  fprintf(fattrs1, LTYPESTR);/* JF added YYABORT() */
  fprintf(fattrs1, "#define\tYYACCEPT\treturn(0)\n");
  fprintf(fattrs1, "#define\tYYABORT\treturn(1)\n");
  fprintf(fattrs1, "#define\tYYERROR\treturn(1)\n");
}



/* Get the data type (alternative in the union) of the value for symbol n in rule rule.  */

char *
get_type_name(n, rule)
int n;
symbol_list *rule;
{
  static char *msg = "illegal $ value";

  register int i;
  register symbol_list *rp;

  if (n < 0)
    fatal(msg);

  rp = rule;
  i = 0;

  while (i < n)
    {
      rp = rp->next;
      if (rp == NULL || rp->sym == NULL)
   fatal(msg);
      i++;
    }

  return (rp->sym->type_name);
}



/* after %guard is seen in the input file,
copy the actual guard into the guards file.
If the guard is followed by an action, copy that into the actions file.
stack_offset is the number of values in the current rule so far,
which says where to find $0 with respect to the top of the stack,
for the simple parser in which the stack is not popped until after the guard is run.  */

copy_guard(rule, stack_offset)
symbol_list *rule;
int stack_offset;
{
  register int c;
  register int n;
  register int count;
  register int match;
  register int ended;
  register char *type_name;
  extern char token_buffer[];

  /* offset is always 0 if parser has already popped the stack pointer */
  if (semantic_parser) stack_offset = 0;

  fprintf(fguard, "\ncase %d:\n", nrules);
  fprintf(fguard, "#line %d \"%s\"\n", lineno, infile);
  putc('{', fguard);

  count = 0;
  c = getc(finput);

  while (count > 0 || c != ';' && c != '{')
    {
      switch (c)
   {
   case '\n':
     putc(c, fguard);
     lineno++;
     break;

   case '{':
     putc(c, fguard);
     count++;
     break;

   case '}':
     putc(c, fguard);
     if (count > 0)
       count--;
     else
       fatal("unmatched right brace ('}')");

   case '\'':
   case '"':
     match = c;
     putc(c, fguard);
     c = getc(finput);

     while (c != match)
       {
         if (c == EOF || c == '\n')
      fatal("unterminated string");

         putc(c, fguard);
         
         if (c == '\\')
      {
        c = getc(finput);
        if (c == EOF || c == '\n')
          fatal("unterminated string");
        putc(c, fguard);
        if (c == '\n')
          lineno++;
      }

         c = getc(finput);
       }

     putc(c, fguard);
     break;

   case '/':
     putc(c, fguard);
     c = getc(finput);
     if (c != '*')
       continue;

     putc(c, fguard);
     c = getc(finput);

     ended = 0;
     while (!ended)
       {
         if (c == '*')
      {
        while (c == '*')
          {
            putc(c, fguard);
            c = getc(finput);
          }

        if (c == '/')
          {
            putc(c, fguard);
            ended = 1;
          }
      }
         else if (c == '\n')
      {
        lineno++;
        putc(c, fguard);
        c = getc(finput);
      }
         else if (c == EOF)
      fatal("unterminated comment");
         else
      {
        putc(c, fguard);
        c = getc(finput);
      }
       }

     break;

   case '$':
     c = getc(finput);
     type_name = NULL;

     if (c == '<')
       {
         register char *cp = token_buffer;

         while ((c = getc(finput)) != '>' && c > 0)
      *cp++ = c;
         *cp = 0;
         type_name = token_buffer;

         c = getc(finput);
       }

     if (c == '$')
          {
         fprintf(fguard, "yyval");
         if (!type_name) type_name = rule->sym->type_name;
         if (type_name)
      fprintf(fguard, ".%s", type_name);
         if(!type_name && typed)   /* JF */
      fprintf(stderr,"%s:%d:  warning:  $$ of '%s' has no declared type.\n",infile,lineno,rule->sym->tag);
       }

     else if (isdigit(c) || c == '-')
       {
         register int sign = 1;

         if (c == '-')
      {
        c = getc(finput);
        sign = -1;
      }
         n = 0;
         while (isdigit(c))
      {
        n = 10*n + (c - '0');
        c = getc(finput);
      }

         n *= sign;

         if (!type_name && n > 0)
      type_name = get_type_name(n, rule);

         fprintf(fguard, "yyvsp[%d]", n - stack_offset);
         if (type_name)
      fprintf(fguard, ".%s", type_name);
         if(!type_name && typed)   /* JF */
      fprintf(stderr,"%s:%d: warning:  $%d of '%s' has no declared type.\n",infile,lineno,n,rule->sym->tag);
         continue;
       }
     else
       fatals("$%c is illegal",c);   /* JF changed style */

     break;

   case '@':
     c = getc(finput);
     n = 0;
     while (isdigit(c))
       {
         n = 10*n + (c - '0');
         c = getc(finput);
       }

     if (n < 1)
       fatal("illegal @-construct");

     fprintf(fguard, "yylsp[%d]", n - stack_offset);

     continue;

   case EOF:
     fatal("unterminated %guard clause");

   default:
     putc(c, fguard);
   }

      c = getc(finput);
    }

  fprintf(fguard, ";\n    break;}");
  if (c == '{')
    copy_action(rule, stack_offset);
  else if (c == '=')
    {
      c = getc(finput);
      if (c = '{')
   copy_action(rule, stack_offset);
    }

}



/* Assuming that a { has just been seen, copy everything up to the matching }
into the actions file.
stack_offset is the number of values in the current rule so far,
which says where to find $0 with respect to the top of the stack.  */

copy_action(rule, stack_offset)
symbol_list *rule;
int stack_offset;
{
  register int c;
  register int n;
  register int count;
  register int match;
  register int ended;
  register char *type_name;
  extern char token_buffer[];

  /* offset is always 0 if parser has already popped the stack pointer */
  if (semantic_parser) stack_offset = 0;

  fprintf(faction, "\ncase %d:\n", nrules);
  fprintf(faction, "#line %d \"%s\"\n", lineno, infile);
  putc('{', faction);

  count = 1;
  c = getc(finput);

  while (count > 0)
    {
      while (c != '}')
        {
          switch (c)
       {
       case '\n':
         putc(c, faction);
         lineno++;
         break;

       case '{':
         putc(c, faction);
         count++;
         break;

       case '\'':
       case '"':
         match = c;
         putc(c, faction);
         c = getc(finput);

         while (c != match)
      {
        if (c == EOF || c == '\n')
          fatal("unterminated string");

        putc(c, faction);

        if (c == '\\')
          {
            c = getc(finput);
            if (c == EOF)
         fatal("unterminated string");
            putc(c, faction);
            if (c == '\n')
         lineno++;
          }

        c = getc(finput);
      }

         putc(c, faction);
         break;

       case '/':
         putc(c, faction);
         c = getc(finput);
         if (c != '*')
      continue;

         putc(c, faction);
         c = getc(finput);

         ended = 0;
         while (!ended)
      {
        if (c == '*')
          {
            while (c == '*')
              {
           putc(c, faction);
           c = getc(finput);
         }

            if (c == '/')
         {
           putc(c, faction);
           ended = 1;
         }
          }
        else if (c == '\n')
          {
            lineno++;
            putc(c, faction);
            c = getc(finput);
          }
        else if (c == EOF)
          fatal("unterminated comment");
        else
          {
            putc(c, faction);
            c = getc(finput);
          }
      }

         break;

       case '$':
         c = getc(finput);
         type_name = NULL;

         if (c == '<')
      {
        register char *cp = token_buffer;

        while ((c = getc(finput)) != '>' && c > 0)
          *cp++ = c;
        *cp = 0;
        type_name = token_buffer;

        c = getc(finput);
      }
         if (c == '$')
      {
        fprintf(faction, "yyval");
        if (!type_name) type_name = get_type_name(0, rule);
        if (type_name)
          fprintf(faction, ".%s", type_name);
        if(!type_name && typed)   /* JF */
          fprintf(stderr,"%s:%d:  warning:  $$ of '%s' has no declared type.\n",infile,lineno,rule->sym->tag);
      }
         else if (isdigit(c) || c == '-')
      {
        register int sign = 1;

        if (c == '-')
          {
            c = getc(finput);
            sign = -1;
          }
        n = 0;
        while (isdigit(c))
          {
            n = 10*n + (c - '0');
            c = getc(finput);
          }
        n *= sign;
        if (!type_name && n > 0)
          type_name = get_type_name(n, rule);

        fprintf(faction, "yyvsp[%d]", n - stack_offset);
        if (type_name)
          fprintf(faction, ".%s", type_name);
        if(!type_name && typed)   /* JF */
          fprintf(stderr,"%s:%d: warning:  $%d of '%s' has no declared type.\n",infile,lineno,n,rule->sym->tag);
        continue;
      }
         else
      fatal("$%c is illegal",c);   /* JF changed format */

         break;

       case '@':
         c = getc(finput);
         n = 0;

         while (isdigit(c))
      {
        n = 10*n + (c - '0');
        c = getc(finput);
      }

         if (n < 1)
      fatal("illegal @-construct");

         fprintf(faction, "yylsp[%d]", n - stack_offset);

         continue;

       case EOF:
         fatal("unmatched '{'");

       default:
         putc(c, faction);
       }

          c = getc(finput);
        }

      /* above loop exits when c is '}' */

      if (--count)
        {
     putc(c, faction);
     c = getc(finput);
   }
    }

  fprintf(faction, ";\n    break;}");
}



/* generate a dummy symbol, a nonterminal,
whose name cannot conflict with the user's names. */

bucket *
gensym()
{
  register bucket *sym;

  extern char token_buffer[];
  sprintf (token_buffer, "@%d", ++gensym_count);
  sym = getsym(token_buffer);
  sym->class = SNTERM;
  sym->value = nvars++;
  return (sym);
}



/* Parse the input grammar into a one symbol_list structure.
Each rule is represented by a sequence of symbols: the left hand side
followed by the contents of the right hand side, followed by a null pointer
instead of a symbol to terminate the rule.
The next symbol is the lhs of the following rule.

All guards and actions are copied out to the appropriate files,
labelled by the rule number they apply to.  */

readgram()
{
  register int t;
  register bucket *lhs;
  register symbol_list *p;
  register symbol_list *p1;

  symbol_list *crule;   /* points to first symbol_list of current rule.  */
         /* its symbol is the lhs of the rule.   */
  symbol_list *crule1;  /* points to the symbol_list preceding crule.  */

  p1 = NULL;

  t = lex();

  while (t != TWO_PERCENTS && t != ENDFILE)
    {
      if (t == IDENTIFIER || t == BAR)
   {
     register int actionflag = 0;
     int rulelength = 0;  /* number of symbols in rhs of this rule so far  */
     int xactions = 0;   /* JF for error checking */
     bucket *first_rhs = 0;

     if (t == IDENTIFIER)
       {
         lhs = symval;
    
         t = lex();
         if (t != COLON)
      fatal("ill-formed rule");
       }

     if (nrules == 0)
       {
         if (t == BAR)
      fatal("grammar starts with vertical bar");

         if (!start_flag)
      startval = lhs;
       }

     /* start a new rule and record its lhs.  */

     nrules++;
     nitems++;

     record_rule_line ();

     p = NEW(symbol_list);
     p->sym = lhs;

     crule1 = p1;
     if (p1)
       p1->next = p;
     else
       grammar = p;

     p1 = p;
     crule = p;

     /* mark the rule's lhs as a nonterminal if not already so.  */

     if (lhs->class == SUNKNOWN)
       {
         lhs->class = SNTERM;
         lhs->value = nvars;
         nvars++;
       }
     else if (lhs->class == STOKEN)
       fatals("rule given for %s, which is a token", lhs->tag);

     /* read the rhs of the rule.  */

     for (;;)
       {
         t = lex();

         if (! (t == IDENTIFIER || t == LEFT_CURLY)) break;

         /* if next token is an identifier, see if a colon follows it.
       If one does, exit this rule now.  */
         if (t == IDENTIFIER)
      {
        register bucket *ssave;
        register int t1;

        ssave = symval;
        t1 = lex();
        unlex(t1);
        symval = ssave;
        if (t1 == COLON) break;

        if(!first_rhs)   /* JF */
           first_rhs = symval;
        /* not followed by colon => process as part of this rule's rhs.  */
        if (actionflag)
          {
            register bucket *sdummy;

            /* if this symbol was preceded by an action, */
            /* make a dummy nonterminal to replace that action in this rule */
            /* and make another rule to associate the action to the dummy.  */
            /* Since the action was written out with this rule's number, */
            /* we must write give the new rule this number */
            /* by inserting the new rule before it.  */

            /* make a dummy nonterminal, a gensym.  */
            sdummy = gensym();

            /* make a new rule, whose body is empty, before the current one.  */
            /* so that the action just read can belong to it.  */
            nrules++;
            nitems++;
            record_rule_line ();
            p = NEW(symbol_list);
            if (crule1)
         crule1->next = p;
            else grammar = p;
            p->sym = sdummy;
            crule1 = NEW(symbol_list);
            p->next = crule1;
            crule1->next = crule;
            
            /* insert the dummy generated by that rule into this rule.  */
            nitems++;
            p = NEW(symbol_list);
            p->sym = sdummy;
            p1->next = p;
            p1 = p;

            actionflag = 0;
          }
        nitems++;
        p = NEW(symbol_list);
        p->sym = symval;
        p1->next = p;
        p1 = p;
      }
         else /* handle an action.  */
      {
        copy_action(crule, rulelength);
        actionflag = 1;
        xactions++;   /* JF */
      }
         rulelength++;
       }

     /* Put an empty link in the list to mark the end of this rule  */
     p = NEW(symbol_list);
     p1->next = p;
     p1 = p;

     if (t == PREC)
       {
         t = lex();
         crule->ruleprec = symval;
         t = lex();
       }
     if (t == GUARD)
       {
         if (! semantic_parser)
      fatal("%guard present but %semantic_parser not specified");

         copy_guard(crule, rulelength);
         t = lex();
       }
     else if (t == LEFT_CURLY)
       {
         if (actionflag) fatal("two actions at end of one rule");
         copy_action(crule, rulelength);
         t = lex();
       }
     /* JF if we'd end up using default, get a warning */
     else if(!xactions && first_rhs && lhs->type_name!=first_rhs->type_name) {
       if(lhs->type_name == 0 || first_rhs->type_name == 0 ||
                  strcmp(lhs->type_name,first_rhs->type_name))
         fprintf(stderr,"%s:%d:  warning: type clash ('%s' '%s') on default action\n",
              infile,lineno,lhs->type_name,first_rhs->type_name);
     }
     if (t == SEMICOLON)
       t = lex();
   }
      /* these things can appear as alternatives to rules.  */
      else if (t == TOKEN)
   {
     parse_token_decl(STOKEN, SNTERM);
     t = lex();
   }
      else if (t == NTERM)
   {
     parse_token_decl(SNTERM, STOKEN);
     t = lex();
   }
      else if (t == TYPE)
   {
     t = get_type();
   }
      else if (t == UNION)
   {
     parse_union_decl();
     t = lex();
   }
      else if (t == START)
   {
     parse_start_decl();
     t = lex();
   }
      else
   fatal("illegal input");
    }

  if (nrules == 0)
    fatal("no input grammar");

  if (typed == 0)/* JF put out same default YYSTYPE as YACC does */
    fprintf(fattrs1, "#ifndef YYSTYPE\n#define YYSTYPE int\n#endif\n");

  ntokens = nsyms - nvars;
}


record_rule_line ()
{
  /* Record each rule's source line number in rline table.  */

  if (nrules >= rline_allocated)
    {
      rline_allocated = nrules * 2;
      rline = (short *) realloc (rline,
             rline_allocated * sizeof (short));
      if (rline == 0)
   {
     fprintf (stderr, "bison: memory exhausted\n");
     exit (1);
   }
    }
  rline[nrules] = lineno;
}


/* read in a %type declaration and record its information for get_type_name to access */

int
get_type()
{
  register int k;
  register int t;
  register char *name;

  extern char token_buffer[];

  t = lex();

  if (t != TYPENAME)
    fatal("ill-formed %type declaration");

  k = strlen(token_buffer);
  name = NEW2(k + 1, char);
  strcpy(name, token_buffer);

  for (;;)
    {
      t = lex();

      switch (t)
   {
   case SEMICOLON:
     return (lex());

   case COMMA:
     break;

   case IDENTIFIER:
     if (symval->type_name == NULL)
       symval->type_name = name;
     else
       fatals("type redeclaration for %s", symval->tag);

     break;

   default:
     return (t);
   }
    }
}



/* assign symbol numbers, and write definition of token names into fdefines.
Set up vectors tags and sprec of names and precedences of symbols.  */

packsymbols()
{
  register bucket *bp;
  register int tokno = 1;
  register int i;
  register int last_user_token_number;

  /* int lossage = 0; JF set but not used */

  tags = NEW2(nsyms + 1, char *);
  tags[0] = "$";

  sprec = NEW2(nsyms, short);
  sassoc = NEW2(nsyms, short);

  max_user_token_number = 255;
  last_user_token_number = 255;

  for (bp = firstsymbol; bp; bp = bp->next)
    {
      if (bp->class == SUNKNOWN)
   {
     fprintf(stderr, "symbol %s used, not defined as token, and no rules for it\n",
           bp->tag);
     /* lossage = 1; JF not used */
     bp->class = SNTERM;
   }

      if (bp->class == SNTERM)
   {
     bp->value += ntokens;
   }
      else
   {
     if (translations && !(bp->user_token_number))
       bp->user_token_number = ++last_user_token_number;
     if (bp->user_token_number > max_user_token_number)
       max_user_token_number = bp->user_token_number;
     bp->value = tokno++;
   }

      tags[bp->value] = bp->tag;
      sprec[bp->value] = bp->prec;
      sassoc[bp->value] = bp->assoc;

    }

  if (translations)
    {
      register int i;

      token_translations = NEW2(max_user_token_number+1, short);

      /* initialize all entries for literal tokens to 2,
    the internal token number for $illegal., which represents all illegal inputs.  */
      for (i = 0; i <= max_user_token_number; i++)
        token_translations[i] = 2;      
    }

  for (bp = firstsymbol; bp; bp = bp->next)
    {
      if (bp->value >= ntokens) continue;
      if (translations)
   {
     if (token_translations[bp->user_token_number] != 2)
       {
          /* JF made this a call to fatals() */
         fatals( "tokens %s and %s both assigned number %d",
               tags[token_translations[bp->user_token_number]],
               bp->tag,
               bp->user_token_number);
       }
     token_translations[bp->user_token_number] = bp->value;
   }
    }

  error_token_number = errtoken->value;

  output_token_defines(ftable);

  if (startval->class == SUNKNOWN)
    fatals("the start symbol %s is undefined", startval->tag);
  else if (startval->class == STOKEN)
    fatals("the start symbol %s is a token", startval->tag);

  start_symbol = startval->value;

  if (definesflag)
    {
      output_token_defines(fdefines);

      if (semantic_parser)
   for (i = ntokens; i < nsyms; i++)
     {
       /* don't make these for dummy nonterminals made by gensym.  */
       if (*tags[i] != '@')
         fprintf(fdefines, "#define\tNT%s\t%d\n", tags[i], i);
     }

      fclose(fdefines);
      fdefines = NULL;
    }
}
      

output_token_defines(file)
FILE *file;
{
  bucket *bp;

  for (bp = firstsymbol; bp; bp = bp->next)
    {
      if (bp->value >= ntokens) continue;

      /* For named tokens, but not literal ones, define the name.  */
      /* The value is the user token number.  */

      if ('\'' != *tags[bp->value] && bp != errtoken)
   {
     register char *cp = tags[bp->value];
     register char c;

     /* Don't #define nonliteral tokens whose names contain periods.  */

     while ((c = *cp++) && c != '.');
     if (!c)
       {
              fprintf(file, "#define\t%s\t%d\n", tags[bp->value],
             (translations ? bp->user_token_number : bp->value));
         if (semantic_parser)
                fprintf(file, "#define\tT%s\t%d\n", tags[bp->value],
               bp->value);
       }
   }
    }

  putc('\n', file);
}



/* convert the rules into the representation using rrhs, rlhs and ritems.  */

packgram()
{
  register int itemno;
  register int ruleno;
  register symbol_list *p;
/*  register bucket *bp; JF unused */

  bucket *ruleprec;

  ritem = NEW2(nitems + 1, short);
  rlhs = NEW2(nrules, short) - 1;
  rrhs = NEW2(nrules, short) - 1;
  rprec = NEW2(nrules, short) - 1;
  rassoc = NEW2(nrules, short) - 1;

  itemno = 0;
  ruleno = 1;

  p = grammar;
  while (p)
    {
      rlhs[ruleno] = p->sym->value;
      rrhs[ruleno] = itemno;
      ruleprec = p->ruleprec;

      p = p->next;
      while (p && p->sym)
   {
     ritem[itemno++] = p->sym->value;
     /* a rule gets the precedence and associativity of the last token in it.  */
          if (p->sym->class == STOKEN)
       {
         rprec[ruleno] = p->sym->prec;
         rassoc[ruleno] = p->sym->assoc;
       }
     if (p) p = p->next;
   }

      /* if this rule has a %prec, specified symbol's precedence replaces the default */
      if (ruleprec)
   {
          rprec[ruleno] = ruleprec->prec;
          rassoc[ruleno] = ruleprec->assoc;
   }

      ritem[itemno++] = -ruleno;
      ruleno++;

      if (p) p = p->next;
    }

  ritem[itemno] = 0;
}
