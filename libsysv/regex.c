/*
 *			R E G E X . C
 *
 *  An interface to the System-V regular expression subroutines which
 *  present the Berkeley (BSD) names and semantics, so that all
 *  regular expression code can be programmed using the Berkeley interface.
 *
 *	last edit:	04-Nov-1987	D A Gwyn
 *
 *  Author -
 *	D A Gwyn
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Distribution Status -
 *	Public Domain, Distribution Unlimitied.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#if defined(BSD)
#ifndef lint
char	regex_dummy;      /* some systems can't handle empty object modules */
#else
		/* Nothing further to do on BSD systems */
#endif
#else

extern struct re_msg
	{
	int	number;
	char	*message;
	}	re_msgtab[];

static char	*re_err;		/* sneak path to error string */

/*	Handle error from <regexp.h>	*/

static void
re_error( n )
	register int		n;	/* error number */
	{
	register struct re_msg	*mp;

	for ( mp = re_msgtab; mp->number > 0; ++mp )
		if ( mp->number == n )
			break;

	re_err = mp->message;
	}

/* macros for <regexp.h> */
#define INIT		register char	*re_str = instring;
#define GETC()		(*re_str == '\0' ? '\0' : (int)*re_str++)
#define UNGETC( c )	--re_str
#define PEEKC()		((int)*re_str)
#define RETURN( c )	return (char *)0
#define ERROR( n )	re_error( n )

/* change the following global extern variables for safety's sake */
#define braelist re_braelist
#define braslist re_braslist
#define	ebra	re_ebra
#define	sed	re_sed
#define	nbra	re_nbra
#define	loc1	re_loc1
#define	loc2	re_loc2
#define	locs	re_locs
#define nodelim	re_nodelim
#define	circf	re_circf
#define low	re_low
#define size	re_size
#define bittab	re_bittab

/* change the following global extern functions for safety's sake */
#define	compile	re_cmpl			/* avoid truncated name collision! */
#define	step	re_step
#define	advance	re_advance
#define getrnge	re_getrnge
#if !defined(sgi) && !defined(i386)
#	define ecmp	re_ecmp
#endif

#include	<regexp.h>

#define	ESIZE	512
static char	re_buf[ESIZE];		/* compiled r.e. */

/*	Compile regular expression	*/

char	*
re_comp( s )				/* returns 0 or ptr to error message */
	char	*s;
	{
	re_err = (char *)0;

	if ( s != (char *)0 && *s != '\0' )
		(void)compile( s, re_buf, &re_buf[ESIZE], '\0' );
	else if ( re_buf[0] == '\0' )
		ERROR( 41 );		/* no remembered search string */
	/* else use remembered search string from previous call */

	return re_err;
	}

/*	Test for match against compiled expression	*/

int
re_exec( s )				/* returns 1 if s matches, else 0 */
	char	*s;
	{
	locs = 0;			/* ??? */
	return step( s, re_buf );
	}
#endif
