/* Copyright (C) 1989, 1997, 1998 Aladdin Enterprises.  All rights reserved. */

/*$Id: ansi2knr.c,v 1.1.1.1 2004/03/11 18:57:30 dizzy Exp $*/
/* Convert ANSI C function definitions to K&R ("traditional C") syntax */

/*
ansi2knr is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY.  No author or distributor accepts responsibility to anyone for the
consequences of using it or for whether it serves any particular purpose or
works at all, unless he says so in writing.  Refer to the GNU General Public
License (the "GPL") for full details.

Everyone is granted permission to copy, modify and redistribute ansi2knr,
but only under the conditions described in the GPL.  A copy of this license
is supposed to have been given to you along with ansi2knr so you can know
your rights and responsibilities.  It should be in a file named COPYLEFT,
or, if there is no file named COPYLEFT, a file named COPYING.  Among other
things, the copyright notice and this notice must be preserved on all
copies.

We explicitly state here what we believe is already implied by the GPL: if
the ansi2knr program is distributed as a separate set of sources and a
separate executable file which are aggregated on a storage medium together
with another program, this in itself does not bring the other program under
the GPL, nor does the mere fact that such a program or the procedures for
constructing it invoke the ansi2knr executable bring any other part of the
program under the GPL.
*/

/*
 * Usage:
	ansi2knr [--filename FILENAME] [INPUT_FILE [OUTPUT_FILE]]
 * --filename provides the file name for the #line directive in the output,
 * overriding input_file (if present).
 * If no input_file is supplied, input is read from stdin.
 * If no output_file is supplied, output goes to stdout.
 * There are no error messages.
 *
 * ansi2knr recognizes function definitions by seeing one or more
 * identifiers at the left margin, possibly including function pointer
 * types, eventually followed by a left parenthesis with a right
 * parenthesis after that and with a left brace as the first token on
 * the next line.  ansi2knr will recognize a multi-line header provided
 * that no intervening line ends with a left or right brace or a
 * semicolon.  These algorithms ignore whitespace and comments.
 * The following constructs will confuse it:
 *	- Any other construct that starts at the left margin and
 *	    follows the above syntax (such as a macro or function call).
 *	- Some macros that tinker with the syntax of the function header.
 */

/*
 * The original and principal author of ansi2knr is L. Peter Deutsch
 * <ghost@aladdin.com>.  Other authors are noted in the change history
 * that follows (in reverse chronological order):
	rac 2000-08-02 modified to not force the return values to be
		on the preceding line, to transform prototypes, to
		remove the "void" single parameter restriction, and to
		handle function pointers with ANSI style parameters.
	lpd 1998-11-09 added further hack to recognize identifier(void)
		as being a procedure
	lpd 1998-10-23 added hack to recognize lines consisting of
		identifier1(identifier2) as *not* being procedures
	lpd 1997-12-08 made input_file optional; only closes input and/or
		output file if not stdin or stdout respectively; prints
		usage message on stderr rather than stdout; adds
		--filename switch (changes suggested by
		<ceder@lysator.liu.se>)
	lpd 1996-01-21 added code to cope with not HAVE_CONFIG_H and with
		compilers that don't understand void, as suggested by
		Tom Lane
	lpd 1996-01-15 changed to require that the first non-comment token
		on the line following a function header be a left brace,
		to reduce sensitivity to macros, as suggested by Tom Lane
		<tgl@sss.pgh.pa.us>
	lpd 1995-06-22 removed #ifndefs whose sole purpose was to define
		undefined preprocessor symbols as 0; changed all #ifdefs
		for configuration symbols to #ifs
	lpd 1995-04-05 changed copyright notice to make it clear that
		including ansi2knr in a program does not bring the entire
		program under the GPL
	lpd 1994-12-18 added conditionals for systems where ctype macros
		don't handle 8-bit characters properly, suggested by
		Francois Pinard <pinard@iro.umontreal.ca>;
		removed --varargs switch (this is now the default)
	lpd 1994-10-10 removed CONFIG_BROKETS conditional
	lpd 1994-07-16 added some conditionals to help GNU `configure',
		suggested by Francois Pinard <pinard@iro.umontreal.ca>;
		properly erase prototype args in function parameters,
		contributed by Jim Avera <jima@netcom.com>;
		correct error in writeblanks (it shouldn't erase EOLs)
	lpd 1989-xx-xx original version
 */

/* Most of the conditionals here are to make ansi2knr work with */
/* or without the GNU configure machinery. */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <ctype.h>

#if HAVE_CONFIG_H

/*
   For properly autoconfiguring ansi2knr, use AC_CONFIG_HEADER(config.h).
   This will define HAVE_CONFIG_H and so, activate the following lines.
 */

# if STDC_HEADERS || HAVE_STRING_H
#  include <string.h>
# else
#  include <strings.h>
# endif

#else /* not HAVE_CONFIG_H */

/* Otherwise do it the hard way */

# ifdef BSD
#  include <strings.h>
# else
#  ifdef VMS
    extern int strlen(), strncmp();
#  else
#   include <string.h>
#  endif
# endif

#endif /* not HAVE_CONFIG_H */

#if STDC_HEADERS
# include <stdlib.h>
#else
/*
   malloc and free should be declared in stdlib.h,
   but if you've got a K&R compiler, they probably aren't.
 */
# ifdef MSDOS
#  include <malloc.h>
# else
#  ifdef VMS
     extern char *malloc();
     extern void free();
#  else
     extern char *malloc();
     extern int free();
#  endif
# endif

#endif

/*
 * The ctype macros don't always handle 8-bit characters correctly.
 * Compensate for this here.
 */
#ifdef isascii
#  undef HAVE_ISASCII		/* just in case */
#  define HAVE_ISASCII 1
#else
#endif
#if STDC_HEADERS || !HAVE_ISASCII
#  define is_ascii(c) 1
#else
#  define is_ascii(c) isascii(c)
#endif

#define is_space(c) (is_ascii(c) && isspace(c))
#define is_alpha(c) (is_ascii(c) && isalpha(c))
#define is_alnum(c) (is_ascii(c) && isalnum(c))

/* Scanning macros */
#define isidchar(ch) (is_alnum(ch) || (ch) == '_')
#define isidfirstchar(ch) (is_alpha(ch) || (ch) == '_')

/* Forward references */
static char *skipspace();
static int writeblanks();
static int test1();
static int convert1();

static char *startfn;
static char *endfn;

/* The main program */
int
main(argc, argv)
    int argc;
    char *argv[];
{	FILE *in = stdin;
	FILE *out = stdout;
	char *filename = 0;
#define bufsize 5000			/* arbitrary size */
	char *buf;
	char *line;
	char *more;
	char *usage =
	  "Usage: ansi2knr [--filename FILENAME] [INPUT_FILE [OUTPUT_FILE]]\n";
	/*
	 * In previous versions, ansi2knr recognized a --varargs switch.
	 * If this switch was supplied, ansi2knr would attempt to convert
	 * a ... argument to va_alist and va_dcl; if this switch was not
	 * supplied, ansi2knr would simply drop any such arguments.
	 * Now, ansi2knr always does this conversion, and we only
	 * check for this switch for backward compatibility.
	 */
	int convert_varargs = 1;

	while ( argc > 1 && argv[1][0] == '-' ) {
	  if ( !strcmp(argv[1], "--varargs") ) {
	    convert_varargs = 1;
	    argc--;
	    argv++;
	    continue;
	  }
	  if ( !strcmp(argv[1], "--filename") && argc > 2 ) {
	    filename = argv[2];
	    argc -= 2;
	    argv += 2;
	    continue;
	  }
	  fprintf(stderr, "Unrecognized switch: %s\n", argv[1]);
	  fprintf(stderr, usage);
	  exit(1);
	}
	switch ( argc )
	   {
	default:
		fprintf(stderr, usage);
		exit(0);
	case 3:
		out = fopen(argv[2], "w");
		if ( out == NULL ) {
		  fprintf(stderr, "Cannot open output file %s\n", argv[2]);
		  exit(1);
		}
		/* falls through */
	case 2:
		in = fopen(argv[1], "r");
		if ( in == NULL ) {
		  fprintf(stderr, "Cannot open input file %s\n", argv[1]);
		  exit(1);
		}
		if ( filename == 0 )
		  filename = argv[1];
		/* falls through */
	case 1:
		break;
	   }
	if ( filename )
	  fprintf(out, "#line 1 \"%s\"\n", filename);
	buf = malloc(bufsize);
	line = buf;
	while ( fgets(line, (unsigned)(buf + bufsize - line), in) != NULL )
	   {
test:		line += strlen(line);
		switch ( test1(buf) )
		   {
		case 3:			/* a function pointer */
		case 2:			/* a function prototype */
			convert1(buf, out, 1, convert_varargs);
			break;
		case 1:			/* a function */
			/* Check for a { at the start of the next line. */
			more = ++line;
f:			if ( line >= buf + (bufsize - 1) ) /* overflow check */
			  goto wl;
			if ( fgets(line, (unsigned)(buf + bufsize - line), in) == NULL )
			  goto wl;
			switch ( *skipspace(more, 1) )
			  {
			  case '{':
			    /* Definitely a function */
			    convert1(buf, out, 0, convert_varargs);
			    fputs(more, out);
			    break;
			  case 0:
			    /* The next line was blank or a comment: */
			    /* keep scanning for a non-comment. */
			    line += strlen(line);
			    goto f;
			  default:
			    /* buf isn't a function header, but */
			    /* more might be. */
			    fputs(buf, out);
			    strcpy(buf, more);
			    line = buf;
			    goto test;
			  }
			break;
		case -1:		/* maybe the start of a function */
			if ( line != buf + (bufsize - 1) ) /* overflow check */
			  continue;
			/* falls through */
		default:		/* not a function */
wl:			fputs(buf, out);
			break;
		   }
		line = buf;
	   }
	if ( line != buf )
	  fputs(buf, out);
	free(buf);
	if ( out != stdout )
	  fclose(out);
	if ( in != stdin )
	  fclose(in);
	return 0;
}


/* Skip over space and comments, in either direction. */
static char *
skipspace(p, dir)
    char *p;
    int dir;			/* 1 for forward, -1 for backward */
{	if (*p=='\0') return p;
	for ( ; ; )
	   {	while ( is_space(*p) )
		  p += dir;
		if ( !(*p == '/' && p[dir] == '*') )
		  break;
		p += dir;  p += dir;
		while ( !(*p == '*' && p[dir] == '/') )
		   {	if ( *p == 0 )
			  return p;	/* multi-line comment?? */
			p += dir;
		   }
		p += dir;  p += dir;
	   }
	return p;
}

/*
 * Write blanks over part of a string.
 * Don't overwrite end-of-line characters.
 */
static int
writeblanks(start, end)
    char *start;
    char *end;
{	char *p;
	for ( p = start; p < end; p++ )
	  if ( *p != '\r' && *p != '\n' )
	    *p = ' ';
	return 0;
}

/*
 * Test whether the string in buf is a function definition.
 * The string may contain and/or end with a newline.
 * Return as follows:
 *	0 - definitely not a function definition;
 *	1 - definitely a function definition;
 *	2 - definitely a function prototype;
 *	3 - definitely a function pointer;
 *	-1 - may be the beginning of a function definition,
 *		append another line and look again.
 */
static int
test1(buf)
    char *buf;
{	char *p;
	char *bend;
	int contin;

	if ( !isalpha(*buf) )
	  return 0;		/* no name at left margin */
	bend = skipspace(buf + strlen(buf) - 1, -1);
	switch ( *bend )
	   {
	   case ';': contin = 2; break;
	   case ')': contin = 1; break;
	   case '{':
	   case '}':
	     return 0;		/* not a function */
	   default: contin = -1;
	   }

	/* find start and end of function name */
	startfn=buf;
	while (*startfn!='\0' && *startfn!='(')
	  {
	    startfn++;
	    startfn = skipspace(startfn, 1);
	  }
	if (*startfn=='\0')
	    return 0;		/* not a function */
	startfn--;
	p = startfn;
	startfn = skipspace(startfn,-1);
	endfn = startfn+1;
	while (isidchar(*startfn)) startfn--;
	startfn++;
	if (startfn>=endfn)
	    return 0;
	
	p = skipspace(endfn, 1);
#if 0
/*                   v         v */
/* typedef __ssize_t __io_read_  */
fprintf(stderr,"found %s\n",buf);
fprintf(stderr,"func=");
{ int i; for (i=0; i<endfn-startfn; i++) fprintf(stderr,"%c",startfn[i]); }
fprintf(stderr,"\n");
fprintf(stderr,"p=%s\n",p);
#endif
	p++; /* skip ( */
	p = skipspace(p, 1);
	if (*p=='*')  /* maybe a function pointer */
	  {
	    startfn = p;
	    while (*p!='\0' && *p!=')')
	        p++;
	    if (*p=='\0')
		return -1;  /* wraps to the next line */
	    endfn = p-1;
	    if (contin!=-1) contin = 3;
 	  }
	else
	    if (!isidfirstchar(*p))
	        return 0;  /* ok... no parameters anyway */
	
	/* Check that the apparent function name isn't a keyword. */
	/* We only need to check for keywords that could be followed */
	/* by a left parenthesis (which, unfortunately, is most of them). */
	if (contin != 3)
	{  static char *words[] =
	     {	"asm", "auto", "case", "char", "const", "double",
		"extern", "float", "for", "if", "int", "long",
		"register", "return", "short", "signed", "sizeof",
		"static", "switch", "typedef", "unsigned",
		"void", "volatile", "while", 0
	     };
	  char **key = words;
	  char *kp;
	  int len = endfn - startfn;

	  while ( (kp = *key) != 0 )
	    {	if ( strlen(kp) == len && !strncmp(kp, startfn, len) )
		{
		  fprintf(stderr,"it was a keyword\n");
		  return 0;	/* name is a keyword */
		}
		key++;
	    }
	}
	/*
	 * If the last significant character was a ), we need to count
	 * parentheses, because it might be part of a formal parameter
	 * that is a procedure.
	 */
	if (contin > 0) {
	    int level = 0;

	    for (p = endfn; *p; p = skipspace(p + 1, 1))
		level += (*p == '(' ? 1 : *p == ')' ? -1 : 0);
	    if (level > 0)
		contin = -1;
	}
	return contin;
}

/* Convert a recognized function definition or header to K&R syntax. */
static int
convert1(buf, out, header, convert_varargs)
    char *buf;
    FILE *out;
    int header;			/* Boolean */
    int convert_varargs;	/* Boolean */
{
	char *p;
	/*
	 * The breaks table contains pointers to the beginning and end
	 * of each argument.
	 */
	char **breaks;
	unsigned num_breaks = 2;	/* for testing */
	char **btop;
	char **bp;
	char **ap;
	char *vararg = 0;

	for (p=buf; p<startfn; p++)
	  fprintf(out,"%c",*p);
	
        while (*(endfn++) != '(');
	
top:	p = endfn;
	breaks = (char **)malloc(sizeof(char *) * num_breaks * 2);
	if ( breaks == 0 )
	   {	/* Couldn't allocate break table, give up */
		fprintf(stderr, "Unable to allocate break table!\n");
		fputs(startfn, out);
		return -1;
	   }
	btop = breaks + num_breaks * 2 - 2;
	bp = breaks;
	/* Parse the argument list */
	do
	   {	int level = 0;
		char *lp = NULL;
		char *rp;
		char *end = NULL;

		if ( bp >= btop )
		   {	/* Filled up break table. */
			/* Allocate a bigger one and start over. */
			free((char *)breaks);
			num_breaks <<= 1;
			goto top;
		   }
		*bp++ = p;
		/* Find the end of the argument */
		for ( ; end == NULL; p++ )
		   {	switch(*p)
			   {
			   case ',':
				if ( !level ) end = p;
				break;
			   case '(':
				if ( !level ) lp = p;
				level++;
				break;
			   case ')':
				if ( --level < 0 ) end = p;
				else rp = p;
				break;
			   case '/':
				p = skipspace(p, 1) - 1;
				break;
			   default:
				;
			   }
		   }
		/* Erase any embedded function pointer parameters. */
		if ( lp )
		  writeblanks(lp + 1, rp);
		p--;			/* back up over terminator */
		/* Find the name being declared. */
		/* This is complicated because of procedure and */
		/* array modifiers. */
		for ( ; ; )
		   {	p = skipspace(p - 1, -1);
			switch ( *p )
			   {
			   case ']':	/* skip array dimension(s) */
			   case ')':	/* skip procedure args OR name */
			   {	int level = 1;
				while ( level )
				 switch ( *--p )
				   {
				   case ']': case ')': level++; break;
				   case '[': case '(': level--; break;
				   case '/': p = skipspace(p, -1) + 1; break;
				   default: ;
				   }
			   }
				if ( *p == '(' && *skipspace(p + 1, 1) == '*' )
				   {	/* We found the name of the function pointer */
					while ( !isidfirstchar(*p) )
					  p = skipspace(p, 1) + 1;
					goto found;
				   }
				break;
			   default:
				goto found;
			   }
		   }
found:		if ( *p == '.' && p[-1] == '.' && p[-2] == '.' )
		  {	if ( convert_varargs )
			  {	*bp++ = "va_alist";
				vararg = p-2;
			  }
			else
			  {	p++;
				if ( bp == breaks + 1 )	/* sole argument */
				  writeblanks(breaks[0], p);
				else
				  writeblanks(bp[-1] - 1, p);
				bp--;
			  }
		   }
		else
		   {	while ( isidchar(*p) ) p--;
			*bp++ = p+1;
		   }
		p = end;
	   }
	while ( *p++ == ',' );
	*bp = p;
	/* Make a special check for 'void' arglist */
	if ( bp == breaks+2 )
	   {	p = skipspace(breaks[0], 1);
		if ( !strncmp(p, "void", 4) )
		   {	p = skipspace(p+4, 1);
			if ( p == breaks[2] - 1 )
			   {	bp = breaks;	/* yup, pretend arglist is empty */
				writeblanks(breaks[0], p + 1);
			   }
		   }
	   }
	/* Put out the function name and left parenthesis. */
	for (p=startfn; p != endfn; p++) putc(*p, out);
	/* Put out the declaration. */
	if ( header )
	  {	fputs(");", out);
		for ( p = breaks[0]; *p; p++ )
		  if ( *p == '\r' || *p == '\n' )
		    putc(*p, out);
	  }
	else
	  {	for ( ap = breaks+1; ap < bp; ap += 2 )
		  {	p = *ap;
			while ( isidchar(*p) )
			  putc(*p, out), p++;
			if ( ap < bp - 1 )
			  fputs(", ", out);
		  }
		fputs(")  ", out);
		/* Put out the argument declarations */
		for ( ap = breaks+2; ap <= bp; ap += 2 )
		  (*ap)[-1] = ';';
		if ( vararg != 0 )
		  {	*vararg = 0;
			fputs(breaks[0], out);		/* any prior args */
			fputs("va_dcl", out);		/* the final arg */
			fputs(bp[0], out);
		  }
		else
		  fputs(breaks[0], out);
	  }
	free((char *)breaks);
	return 0;
}
