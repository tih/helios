/*
	getopt -- public domain version of standard System V routine

	Strictly enforces the System V Command Syntax Standard;
	provided by D A Gwyn of BRL for generic ANSI C implementations
*/
/* $Id: getopt.c,v 1.2 1992/12/07 11:12:52 nickc Exp $ */

#include	<stdio.h>
#include	<string.h>

int	opterr = 1;			/* error => print message */
int	optind = 1;			/* next argv[] index */
char	*optarg = NULL;			/* option parameter if any */

static int
Err(   					/* returns '?' */
	char *	 name,			/* program name argv[0] */
	char *	 mess,			/* specific message */
	int	 c )			/* defective option letter */
	{
	if ( opterr )
		(void) fprintf( stderr,
				"%s: %s -- %c\n",
				name, mess, c
			      );

	return '?';			/* erroneous-option marker */
	}

int
getopt( 				/* returns letter, '?', EOF */
	int		argc,		/* argument count from main */
	char *		argv[],		/* argument vector from main */
	char *		optstring )	/* allowed args, e.g. "ab:c" */
	{
	static int	sp = 1;		/* position within argument */
	register int	osp;		/* saved `sp' for param test */
	register int	c;		/* option letter */
	register char	*cp;		/* -> option in `optstring' */

	optarg = NULL;

	if ( sp == 1 )			/* fresh argument */
	  {
		if ( optind >= argc		/* no more arguments */
		  || argv[optind][0] != '-'	/* no more options */
		  || argv[optind][1] == '\0'	/* not option; stdin */
		   )
			return EOF;
		else if ( strcmp( argv[optind], "--" ) == 0 )
			{
			++optind;	/* skip over "--" */
			return EOF;	/* "--" marks end of options */
			}
	      }
	
	c = argv[optind][sp];		/* option letter */
	osp = sp++;			/* get ready for next letter */

	if ( argv[optind][sp] == '\0' )	/* end of argument */
		{
		++optind;		/* get ready for next try */
		sp = 1;			/* beginning of next argument */
		}

	if ( c == ':'			/* optstring syntax conflict */
	  || (cp = strchr( optstring, c )) == NULL	/* not found */
	   )
		return Err( argv[0], "illegal option", c );

	if ( cp[1] == ':' )		/* option takes parameter */
		{
		if ( osp != 1 )
			return Err( argv[0],
				    "option must not be clustered",
				    c
				  );

		if ( sp != 1 )		/* reset by end of argument */
			return Err( argv[0],
			       "option must be followed by white space",
				    c
				  );

		if ( optind >= argc )
			return Err( argv[0],
				    "option requires an argument",
				    c
				  );

		optarg = argv[optind];	/* make parameter available */
		++optind;		/* skip over parameter */
		}

	return c;
	}

