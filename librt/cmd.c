/*
 *			C M D . C
 *
 *  Read and parse a viewpoint-control command stream.
 *  This module is intended to be common to all programs which
 *  read this type of command stream;  the routines to handle
 *  the various keywords should go in application-specific modules.
 *
 *  Author -
 *	Michael John Muuss
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1987 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCScmd[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <ctype.h>
#include "machine.h"
#include "vmath.h"
#include "raytrace.h"
/*
 *			R T _ R E A D _ C M D
 *
 *  Read one semi-colon terminated string of arbitrary length from
 *  the given file into a dynamicly allocated buffer.
 *  Various commenting and escaping conventions are implemented here.
 *
 *  Returns:
 *	NULL	On EOF
 *	char *	On good read
 */
char *
rt_read_cmd( fp )
register FILE	*fp;
{
	register int	c;
	register char	*buf;
	register int	curpos;
	register int	curlen;

	curpos = 0;
	curlen = 400;
	buf = rt_malloc( curlen, "rt_read_cmd buffer" );

	do  {
		c = fgetc(fp);
		if( c == EOF )  {
			c = '\0';
		} else if( c == '#' )  {
			/* All comments run to the end of the line */
			while( (c = fgetc(fp)) != EOF && c != '\n' )  ;
			continue;
		} else if( c == '\n' )  {
			c = ' ';
		} else if( c == ';' )  {
			c = '\0';
		} else if( c == '\\' )  {
			/*  Backslash takes next character literally.
			 *  EOF detection here is not a problem, next
			 *  pass will detect it.
			 */
			c = fgetc(fp);
		} else if( !isascii(c) )  {
			c = '?';
		}
		if( curpos >= curlen )  {
			curlen *= 2;
			buf = rt_realloc( buf, curlen, "rt_read_cmd buffer" );
		}
		buf[curpos++] = c;
	} while( c != '\0' );
	if( curpos <= 1 )
		return( (char *)0 );		/* EOF */
	return( buf );				/* OK */
}

#define MAXWORDS		64	/* Max # of args per command */

/*
 *			R T _ D O _ C M D
 *
 *  Slice up input buffer into whitespace separated "words",
 *  look up the first word as a command, and if it has the
 *  correct number of args, call that function.
 *  The input buffer is altered in the process.
 *
 *  Expected to return -1 to halt command processing loop.
 *
 *  Based heavily on mged/cmd.c by Chuck Kennedy.
 */
int
rt_do_cmd( rtip, lp, ctp )
struct rt_i		*rtip;			/* FUTURE:  for globbing */
register char		*lp;
register struct command_tab	*ctp;
{
	register int	nwords;			/* number of words seen */
	register struct command_tab *tp;
	char		*cmd_args[MAXWORDS+1];	/* array of ptrs to args */

	while( *lp != '\0' && isspace( *lp ) )
		lp++;

	if( *lp == '\0' )
		return(0);		/* No command */

#ifdef unix
	/* Handle "!" shell escape char so the shell can parse the line */
	if( *lp == '!' )  {
		(void)system( lp+1 );
		return(0);		/* No command */
	}
#endif

	/* some non-space string has been seen, cmd_args[0] is set */
	nwords = 1;
	cmd_args[0] = lp;

	for( ; *lp != '\0'; lp++ )  {
		register char	*lp1;

		if( !isspace( *lp ) )
			continue;	/* skip over current word */

		*lp = '\0';		/* terminate current word */
		lp1 = lp + 1;
		if( *lp1 != '\0' && !isspace( *lp1 ) )  {
			/* Begin next word */
			if( nwords >= MAXWORDS-1 )  {
				rt_log(
					"rt_do_cmd: More than %d arguments, aborting\n",
					MAXWORDS);
				return(-1);	/* ERROR */
			}
			cmd_args[nwords++] = lp1;
		}
	}
	cmd_args[nwords] = (char *)0;	/* safety */

	if( nwords <= 0 )
		return(0);	/* No command to process */

	for( tp = ctp; tp->ct_cmd != (char *)0; tp++ )  {
		if( cmd_args[0][0] != tp->ct_cmd[0] ||
		    strcmp( cmd_args[0], tp->ct_cmd ) != 0 )
			continue;
		if( (nwords >= tp->ct_min) &&
		    (nwords <= tp->ct_max) )  {
			return( tp->ct_func( nwords, cmd_args ) );
		}
		rt_log("rt_do_cmd Usage: %s %s\n\t%s\n",
			tp->ct_cmd, tp->ct_parms, tp->ct_comment );
		return(-1);		/* ERROR */
	}
	fprintf(stderr,"rt_do_cmd(%s):  command not found\n", cmd_args[0]);
	return(-1);			/* ERROR */
}
