/*
 *			T P _ S Y M B O L
 *
 *	Terminal Independant Graphics Display Package
 *		Mike Muuss  July 31, 1978
 *
 *	This routine is used to plot a string of ASCII symbols
 *  on the plot being generated, using a built-in set of fonts
 *  drawn as vector lists.
 * 
 *	Internally, the basic font resides in a 10x10 unit square.
 *  Externally, each character can be thought to occupy one square
 *  plotting unit;  the 'scale'
 *  parameter allows this to be changed as desired, although scale
 *  factors less than 10.0 are unlikely to be legible.
 *
 */
#include <stdio.h>
#include <math.h>

/*
 *	Motion encoding macros
 *
 * All characters reference absolute points within a 10 x 10 square
 */
#define	brt(x,y)	(11*x+y)
#define drk(x,y)	-(11*x+y)
#define	LAST		0200		/* Marks end of stroke list */
#define	NEGY		0201		/* Denotes negative y stroke */
#define bneg(x,y)	NEGY, brt(x,y)
#define dneg(x,y)	NEGY, drk(x,y)

#if defined(cray) || defined(CRAY2)
#define TINY	int
#else
#define TINY	char		/* must be signed */
#endif

static TINY	*ppindex[256];	/* index to stroke tokens */
extern TINY	pptable[];	/* table of strokes */

/*
 *  Once-only setup routine
 */
static tp_setup()
{
	register TINY	*p;	/* pointer to stroke table */
	register int i;

	p = pptable;		/* pointer to stroke list */

	/* Store start addrs of each stroke list */
	for( i=040-5; i<128; i++)  {
		ppindex[i+128] = ppindex[i] = p;
		while( (*p++ & 0377) != LAST );
	}
	for( i=6; i<040; i++ )  {
		ppindex[i+128] = ppindex[i] = ppindex['?'];
	}
	for( i=1; i<6; i++ )  {
		ppindex[i+128] = ppindex[i] = ppindex[040-1+i];
	}
}

/*
 *			T P _ S Y M B O L
 */
tp_symbol( fp, string, x, y, scale, theta )
char	*string;		/* string of chars to be plotted */
double	x, y;			/* x,y of lower left corner of 1st char */
double	scale;			/* scale factor to change 1x1 char sz */
double	theta;			/* degrees ccw from X-axis */
{
	register unsigned char *cp;
	double	xrot, yrot;		/* x and y rotation factors */
	double	offset;			/* offset of char from given x,y */
	double	x2, y2;			/* char pos in local coord system */
	double	x3, y3;			/* char pos after rotation */
	int	ysign;			/* sign of y motion, either +1 or -1 */

	if( string == NULL || *string == '\0' )
		return;			/* done before begun! */

	/* Check to see if initialization is needed */
	if( ppindex[040] == 0 )  tp_setup();

	/* Apply rotation */
	xrot = cos( 0.0174533 * theta );
	yrot = sin( 0.0174533 * theta );

	/* Draw each character in the input string */
	offset = 0;
	for( cp = (unsigned char *)string ; *cp; cp++, offset += scale )  {
		register TINY	*p;	/* pointer to stroke table */
		register int	stroke;

		pd_move( fp, x+offset*xrot, y+offset*yrot );

		for( p = ppindex[*cp]; ((stroke= *p)&0xFF) != LAST; p++ )  {
			int	draw;

			if( (stroke&0xFF)==NEGY )  {
				ysign = (-1);
				stroke = *++p;
			} else
				ysign = 1;

			/* Detect & process pen control */
			if( stroke < 0 )  {
				stroke = -stroke;
				draw = 0;
			} else
				draw = 1;

			/* stroke co-ordinates in string coord system */
			x2 = (stroke/11) * 0.1 * scale + offset;
			y2 = (ysign * (stroke%11)) * 0.1 * scale;

			/* Plot this stroke */
			x3 = x + x2*xrot - y2*yrot;
			y3 = y + x2*yrot + y2*xrot;
			if( draw )
				pd_cont( fp, x3, y3 );
			else
				pd_move( fp, x3, y3 );
		}
	}
}

/*	tables for markers	*/

static TINY	pptable[] = {

/*	+	*/
	drk(0, 5),
	brt(8, 5),
	drk(4, 8),
	brt(4, 2),
	LAST,

/*	x	*/
	drk(0, 2),
	brt(8, 8),
	drk(0, 8),
	brt(8, 2),
	LAST,

/*	triangle	*/
	drk(0, 2),
	brt(4, 8),
	brt(8, 2),
	brt(0, 2),
	LAST,

/*	square	*/
	drk(0, 2),
	brt(0, 8),
	brt(8, 8),
	brt(8, 2),
	brt(0, 2),
	LAST,

/*	hourglass	*/
	drk(0, 2),
	brt(8, 8),
	brt(0, 8),
	brt(8, 2),
	brt(0, 2),
	LAST,

/*	table for ascii 040, ' '	*/
	LAST,

/*	table for !	*/
	drk(3, 0),
	brt(5, 2),
	brt(5, 0),
	brt(3, 2),
	brt(3, 0),
	drk(4, 4),
	brt(3, 10),
	brt(5, 10),
	brt(4, 4),
	brt(4, 10),
	LAST,

/*	table for "	*/
	drk(1, 10),
	brt(3, 10),
	brt(2, 7),
	brt(1, 10 ),
	drk(5, 10),
	brt(7, 10),
	brt(6, 7),
	brt(5, 10),
	LAST,

/*	table for #	*/
	drk(1, 0),
	brt(3, 9),
	drk(6, 9),
	brt(4, 0),
	drk(6, 3),
	brt(0, 3),
	drk(1, 6),
	brt(7, 6),
	LAST,

/*	table for $	*/
	drk(1, 2),
	brt(1, 1),
	brt(7, 1),
	brt(7, 5),
	brt(1, 5),
	brt(1, 9),
	brt(7, 9),
	brt(7, 8),
	drk(4, 10),
	brt(4, 0),
	LAST,

/*	table for %	*/
	drk(3, 10),
	brt(3, 7),
	brt(0, 7),
	brt(0, 10),
	brt(8, 10),
	brt(0, 0),
	drk(8, 0),
	brt(5, 0),
	brt(5, 3),
	brt(8, 3),
	brt(8, 0),
	LAST,

/*	table for &	*/
	drk(7, 3),
	brt(4, 0),
	brt(1, 0),
	brt(0, 3),
	brt(5, 8),
	brt(4, 10),
	brt(3, 10),
	brt(1, 8),
	brt(8, 0),
	LAST,

/*	table for '	*/
	drk(4, 6),
	brt(5, 10),
	brt(6, 10),
	brt(4, 6),
	LAST,

/*	table for (	*/
	drk(5, 0 ),
	brt(3, 1 ),
	brt(2, 4 ),
	brt(2, 6 ),
	brt(3, 9 ),
	brt(5, 10 ),
	LAST,

/*	table for )	*/
	drk(3, 0 ),
	brt(5, 1 ),
	brt(6, 4 ),
	brt(6, 6 ),
	brt(5, 9 ),
	brt(3, 10 ),
	LAST,

/*	table for *	*/
	drk(4, 2 ),
	brt(4, 8 ),
	drk(6, 7 ),
	brt(2, 3 ),
	drk(6, 3 ),
	brt(2, 7 ),
	drk(1, 5 ),
	brt(7, 5 ),
	LAST,

/*	table for +	*/
	drk(1, 5 ),
	brt(7, 5 ),
	drk(4, 8 ),
	brt(4, 2 ),
	LAST,

/*	table for, 	*/
	drk(5, 0 ),
	brt(3, 2 ),
	brt(3, 0 ),
	brt(5, 2 ),
	brt(5, 0 ),
	bneg(2, 2 ),
	brt(4, 0 ),
	LAST,

/*	table for -	*/
	drk(1, 5 ),
	brt(7, 5 ),
	LAST,

/*	table for .	*/
	drk(5, 0 ),
	brt(3, 2 ),
	brt(3, 0 ),
	brt(5, 2 ),
	brt(5, 0 ),
	LAST,

/*	table for /	*/
	brt(8, 10 ),
	LAST,

/*	table for 0	*/
	drk(8, 10),
	brt(0, 0),
	brt(0, 10),
	brt(8, 10),
	brt(8, 0),
	brt(0, 0),
	LAST,

/*	table for 1	*/
	drk(4, 0 ),
	brt(4, 10 ),
	brt(2, 8 ),
	LAST,

/*	table for 2	*/
	drk(0, 6 ),
	brt(0, 8 ),
	brt(3, 10 ),
	brt(5, 10 ),
	brt(8, 8 ),
	brt(8, 7 ),
	brt(0, 2 ),
	brt(0, 0 ),
	brt(8, 0 ),
	LAST,

/*	table for 3	*/
	drk(0, 10 ),
	brt(8, 10 ),
	brt(8, 5 ),
	brt(0, 5 ),
	brt(8, 5 ),
	brt(8, 0 ),
	brt(0, 0 ),
	LAST,

/*	table for 4	*/
	drk(0, 10 ),
	brt(0, 5 ),
	brt(8, 5 ),
	drk(8, 10 ),
	brt(8, 0 ),
	LAST,

/*	table for 5	*/
	drk(8, 10 ),
	brt(0, 10 ),
	brt(0, 5 ),
	brt(8, 5 ),
	brt(8, 0 ),
	brt(0, 0 ),
	LAST,

/*	table for 6	*/
	drk(0, 10 ),
	brt(0, 0 ),
	brt(8, 0 ),
	brt(8, 5 ),
	brt(0, 5 ),
	LAST,

/*	table for 7	*/
	drk(0, 10 ),
	brt(8, 10 ),
	brt(6, 0 ),
	LAST,

/*	table for 8	*/
	drk(0, 5 ),
	brt(0, 0 ),
	brt(8, 0 ),
	brt(8, 5 ),
	brt(0, 5 ),
	brt(0, 10 ),
	brt(8, 10 ),
	brt(8, 5 ),
	LAST,

/*	table for 9	*/
	drk(8, 5 ),
	brt(0, 5 ),
	brt(0, 10 ),
	brt(8, 10 ),
	brt(8, 0 ),
	LAST,

/*	table for :	*/
	drk(5, 6 ),
	brt(3, 8 ),
	brt(3, 6 ),
	brt(5, 8 ),
	brt(5, 6 ),
	drk(5, 0 ),
	brt(3, 2 ),
	brt(3, 0 ),
	brt(5, 2 ),
	brt(5, 0 ),
	LAST,

/*	table for ;	*/
	drk(5, 6 ),
	brt(3, 8 ),
	brt(3, 6 ),
	brt(5, 8 ),
	brt(5, 6 ),
	drk(5, 0 ),
	brt(3, 2 ),
	brt(3, 0 ),
	brt(5, 2 ),
	brt(5, 0 ),
	bneg(2, 2 ),
	brt(4, 0 ),
	LAST,

/*	table for <	*/
	drk(8, 8 ),
	brt(0, 5 ),
	brt(8, 2 ),
	LAST,

/*	table for =	*/
	drk(0, 7 ),
	brt(8, 7 ),
	drk(0, 3 ),
	brt(8, 3 ),
	LAST,

/*	table for >	*/
	drk(0, 8 ),
	brt(8, 5 ),
	brt(0, 2 ),
	LAST,

/*	table for ?	*/
	drk(3, 0 ),
	brt(5, 2 ),
	brt(5, 0 ),
	brt(3, 2 ),
	brt(3, 0 ),
	drk(1, 7 ),
	brt(1, 9 ),
	brt(3, 10 ),
	brt(5, 10 ),
	brt(7, 9 ),
	brt(7, 7 ),
	brt(4, 5 ),
	brt(4, 3 ),
	LAST,

/*	table for @	*/
	drk(0, 8 ),
	brt(2, 10 ),
	brt(6, 10 ),
	brt(8, 8 ),
	brt(8, 2 ),
	brt(6, 0 ),
	brt(2, 0 ),
	brt(1, 1 ),
	brt(1, 4 ),
	brt(2, 5 ),
	brt(4, 5 ),
	brt(5, 4 ),
	brt(5, 0 ),
	LAST,

/*	table for A	*/
	brt(0, 8 ),
	brt(2, 10 ),
	brt(6, 10 ),
	brt(8, 8 ),
	brt(8, 0 ),
	drk(0, 5 ),
	brt(8, 5 ),
	LAST,

/*	table for B	*/
	brt(0, 10 ),
	brt(5, 10 ),
	brt(8, 9 ),
	brt(8, 6 ),
	brt(5, 5 ),
	brt(0, 5 ),
	brt(5, 5 ),
	brt(8, 4 ),
	brt(8, 1 ),
	brt(5, 0 ),
	brt(0, 0 ),
	LAST,

/*	table for C	*/
	drk(8, 2 ),
	brt(6, 0 ),
	brt(2, 0 ),
	brt(0, 2 ),
	brt(0, 8 ),
	brt(2, 10 ),
	brt(6, 10 ),
	brt(8, 8 ),
	LAST,

/*	table for D	*/
	brt(0, 10 ),
	brt(5, 10 ),
	brt(8, 8 ),
	brt(8, 2 ),
	brt(5, 0 ),
	brt(0, 0 ),
	LAST,

/*	table for E	*/
	drk(8, 0 ),
	brt(0, 0 ),
	brt(0, 10 ),
	brt(8, 10 ),
	drk(0, 5 ),
	brt(5, 5 ),
	LAST,

/*	table for F	*/
	brt(0, 10 ),
	brt(8, 10 ),
	drk(0, 5 ),
	brt(5, 5 ),
	LAST,

/*	table for G	*/
	drk(5, 5 ),
	brt(8, 5 ),
	brt(8, 2 ),
	brt(6, 0 ),
	brt(2, 0 ),
	brt(0, 2 ),
	brt(0, 8 ),
	brt(2, 10 ),
	brt(6, 10 ),
	brt(8, 8 ),
	LAST,

/*	table for H	*/
	brt(0, 10 ),
	drk(8, 10 ),
	brt(8, 0 ),
	drk(0, 6 ),
	brt(8, 6 ),
	LAST,

/*	table for I	*/
	drk(4, 0 ),
	brt(6, 0 ),
	drk(5, 0 ),
	brt(5, 10 ),
	brt(4, 10 ),
	brt(6, 10 ),
	LAST,

/*	table for J	*/
	drk(0, 2 ),
	brt(2, 0 ),
	brt(5, 0 ),
	brt(7, 2 ),
	brt(7, 10 ),
	brt(6, 10 ),
	brt(8, 10 ),
	LAST,

/*	table for K	*/
	brt(0, 10 ),
	drk(0, 5 ),
	brt(8, 10 ),
	drk(3, 7 ),
	brt(8, 0 ),
	LAST,

/*	table for L	*/
	drk(8, 0 ),
	brt(0, 0 ),
	brt(0, 10 ),
	LAST,

/*	table for M	*/
	brt(0, 10 ),
	brt(4, 5 ),
	brt(8, 10 ),
	brt(8, 10 ),
	brt(8, 0 ),
	LAST,

/*	table for N	*/
	brt(0, 10 ),
	brt(8, 0 ),
	brt(8, 10 ),
	LAST,

/*	table for O	*/
	drk(0, 2 ),
	brt(0, 8 ),
	brt(2, 10 ),
	brt(6, 10 ),
	brt(8, 8 ),
	brt(8, 2 ),
	brt(6, 0 ),
	brt(2, 0 ),
	brt(0, 2 ),
	LAST,

/*	table for P	*/
	brt(0, 10 ),
	brt(6, 10 ),
	brt(8, 9 ),
	brt(8, 6 ),
	brt(6, 5 ),
	brt(0, 5 ),
	LAST,

/*	table for Q	*/
	drk(0, 2 ),
	brt(0, 8 ),
	brt(2, 10 ),
	brt(6, 10 ),
	brt(8, 8 ),
	brt(8, 2 ),
	brt(6, 0 ),
	brt(2, 0 ),
	brt(0, 2 ),
	drk(5, 3 ),
	brt(8, 0 ),
	LAST,

/*	table for R	*/
	brt(0, 10 ),
	brt(6, 10 ),
	brt(8, 8 ),
	brt(8, 6 ),
	brt(6, 5 ),
	brt(0, 5 ),
	drk(5, 5 ),
	brt(8, 0 ),
	LAST,

/*	table for S	*/
	drk(0, 1 ),
	brt(1, 0 ),
	brt(6, 0 ),
	brt(8, 2 ),
	brt(8, 4 ),
	brt(6, 6 ),
	brt(2, 6 ),
	brt(0, 7 ),
	brt(0, 9 ),
	brt(1, 10 ),
	brt(7, 10 ),
	brt(8, 9 ),
	LAST,

/*	table for T	*/
	drk(4, 0 ),
	brt(4, 10 ),
	drk(0, 10 ),
	brt(8, 10 ),
	LAST,

/*	table for U	*/
	drk(0, 10 ),
	brt(0, 2 ),
	brt(2, 0 ),
	brt(6, 0 ),
	brt(8, 2 ),
	brt(8, 10 ),
	LAST,

/*	table for V	*/
	drk(0, 10 ),
	brt(4, 0 ),
	brt(8, 10 ),
	LAST,

/*	table for W	*/
	drk(0, 10 ),
	brt(1, 0 ),
	brt(4, 4 ),
	brt(7, 0 ),
	brt(8, 10 ),
	LAST,

/*	table for X	*/
	brt(8, 10 ),
	drk(0, 10 ),
	brt(8, 0 ),
	LAST,

/*	table for Y	*/
	drk(0, 10 ),
	brt(4, 4 ),
	brt(8, 10 ),
	drk(4, 4 ),
	brt(4, 0 ),
	LAST,

/*	table for Z	*/
	drk(0, 10 ),
	brt(8, 10 ),
	brt(0, 0 ),
	brt(8, 0 ),
	LAST,

/*	table for [	*/
	drk(6, 0 ),
	brt(4, 0 ),
	brt(4, 10 ),
	brt(6, 10 ),
	LAST,

/*	table for \	*/
	drk(0, 10 ),
	brt(8, 0 ),
	LAST,

/*	table for ]	*/
	drk(2, 0 ),
	brt(4, 0 ),
	brt(4, 10 ),
	brt(2, 10 ),
	LAST,

/*	table for ^	*/
	drk(4, 0 ),
	brt(4, 10 ),
	drk(2, 8 ),
	brt(4, 10 ),
	brt(6, 8 ),
	LAST,

/*	table for _	*/
	dneg(0, 1),
	bneg(11, 1),
	LAST,

/*	table for ascii 96: accent	*/
	drk(3, 10),
	brt(5, 6),
	brt(4, 10),
	brt(3, 10),
	LAST,

/*	table for a	*/
	drk(0, 5),
	brt(1, 6),
	brt(6, 6),
	brt(7, 5),
	brt(7, 1),
	brt(8, 0),
	drk(7, 1),
	brt(6, 0),
	brt(1, 0),
	brt(0, 1),
	brt(0, 2),
	brt(1, 3),
	brt(6, 3),
	brt(7, 2),
	LAST,

/*	table for b	*/
	brt(0, 10),
	drk(8, 3),
	brt(7, 5),
	brt(4, 6),
	brt(1, 5),
	brt(0, 3),
	brt(1, 1),
	brt(4, 0),
	brt(7, 1),
	brt(8, 3),
	LAST,

/*	table for c	*/
	drk(8, 5),
	brt(7, 6),
	brt(2, 6),
	brt(0, 4),
	brt(0, 4),
	brt(0, 2),
	brt(2, 0),
	brt(7, 0),
	brt(8, 1),
	LAST,

/*	table for d	*/
	drk(8, 0),
	brt(8, 10),
	drk(8, 3),
	brt(7, 5),
	brt(4, 6),
	brt(1, 5),
	brt(0, 3),
	brt(1, 1),
	brt(4, 0),
	brt(7, 1),
	brt(8, 3),
	LAST,

/*	table for e	*/
	drk(0, 4),
	brt(1, 3),
	brt(7, 3),
	brt(8, 4),
	brt(8, 5),
	brt(7, 6),
	brt(1, 6),
	brt(0, 5),
	brt(0, 1),
	brt(1, 0),
	brt(7, 0),
	brt(8, 1),
	LAST,

/*	table for f	*/
	drk(2, 0),
	brt(2, 9),
	brt(3, 10),
	brt(5, 10),
	brt(6, 9),
	drk(1, 5),
	brt(4, 5),
	LAST,

/*	table for g	*/
	drk(8, 6),
	drk(8, 3),
	brt(7, 5),
	brt(4, 6),
	brt(1, 5),
	brt(0, 3),
	brt(1, 1),
	brt(4, 0),
	brt(7, 1),
	brt(8, 3),
	bneg(8, 2),
	bneg(7, 3),
	bneg(1, 3),
	bneg(0, 2),
	LAST,

/*	table for h	*/
	brt(0, 10),
	drk(0, 4),
	brt(2, 6),
	brt(6, 6),
	brt(8, 4),
	brt(8, 0),
	LAST,

/*	table for i	*/
	drk(4, 0),
	brt(4, 6),
	brt(3, 6),
	drk(4, 9),
	brt(4, 8),
	drk(3, 0),
	brt(5, 0),
	LAST,

/*	table for j	*/
	drk(5, 6),
	brt(6, 6),
	bneg(6, 2),
	bneg(5, 3),
	bneg(3, 3),
	bneg(2, 2),
	LAST,

/*	table for k	*/
	brt(2, 0),
	brt(2, 10),
	brt(0, 10),
	drk(2, 4),
	brt(4, 4),
	brt(8, 6),
	drk(4, 4),
	brt(8, 0),
	LAST,

/*	table for l	*/
	drk(3, 10),
	brt(4, 10),
	brt(4, 2),
	brt(5, 0),
	LAST,

/*	table for m	*/
	brt(0, 6),
	drk(0, 5),
	brt(1, 6),
	brt(3, 6),
	brt(4, 5),
	brt(4, 0),
	drk(4, 5),
	brt(5, 6),
	brt(7, 6),
	brt(8, 5),
	brt(8, 0),
	LAST,

/*	table for n	*/
	brt(0, 6),
	drk(0, 4),
	brt(2, 6),
	brt(6, 6),
	brt(8, 4),
	brt(8, 0),
	LAST,

/*	table for o	*/
	drk(8, 3),
	brt(7, 5),
	brt(4, 6),
	brt(1, 5),
	brt(0, 3),
	brt(1, 1),
	brt(4, 0),
	brt(7, 1),
	brt(8, 3),
	LAST,

/*	table for p	*/
	drk(0, 6),
	bneg(0, 3),
	drk(8, 3),
	brt(7, 5),
	brt(4, 6),
	brt(1, 5),
	brt(0, 3),
	brt(1, 1),
	brt(4, 0),
	brt(7, 1),
	brt(8, 3),
	LAST,

/*	table for q	*/
	drk(8, 6),
	drk(8, 3),
	brt(7, 5),
	brt(4, 6),
	brt(1, 5),
	brt(0, 3),
	brt(1, 1),
	brt(4, 0),
	brt(7, 1),
	brt(8, 3),
	bneg(8, 3),
	bneg(9, 3),
	LAST,

/*	table for r	*/
	brt(1, 0),
	brt(1, 6),
	brt(0, 6),
	drk(1, 4),
	brt(3, 6),
	brt(6, 6),
	brt(8, 4),
	LAST,

/*	table for s	*/
	drk(0, 1),
	brt(1, 0),
	brt(7, 0),
	brt(8, 1),
	brt(7, 2),
	brt(1, 4),
	brt(0, 5),
	brt(1, 6),
	brt(7, 6),
	brt(8, 5),
	LAST,

/*	table for t	*/
	drk(7, 1),
	brt(6, 0),
	brt(4, 0),
	brt(3, 1),
	brt(3, 10),
	brt(2, 10),
	drk(1, 5),
	brt(5, 5),
	LAST,

/*	table for u	*/
	drk(0, 6),
	brt(1, 6),
	brt(1, 1),
	brt(2, 0),
	brt(6, 0),
	brt(7, 1),
	brt(7, 6),
	drk(7, 1),
	brt(8, 0),
	LAST,

/*	table for v	*/
	drk(0, 6),
	brt(4, 0),
	brt(8, 6),
	LAST,

/*	table for w	*/
	drk(0, 6),
	brt(0, 5),
	brt(2, 0),
	brt(4, 5),
	brt(6, 0),
	brt(8, 5),
	brt(8, 6),
	LAST,

/*	table for x	*/
	brt(8, 6),
	drk(0, 6),
	brt(8, 0),
	LAST,

/*	table for y	*/
	drk(0, 6),
	brt(0, 1),
	brt(1, 0),
	brt(7, 0),
	brt(8, 1),
	drk(8, 6),
	bneg(8, 2),
	bneg(7, 3),
	bneg(1, 3),
	bneg(0, 2),
	LAST,

/*	table for z	*/
	drk(0, 6),
	brt(8, 6),
	brt(0, 0),
	brt(8, 0),
	LAST,

/*	table for ascii 123, left brace	*/
	drk(6, 10),
	brt(5, 10),
	brt(4, 9),
	brt(4, 6),
	brt(3, 5),
	brt(4, 4),
	brt(4, 1),
	brt(5, 0),
	brt(6, 0),
	LAST,

/*	table for ascii 124, vertical bar	*/
	drk(4, 4),
	brt(4, 0),
	brt(5, 0),
	brt(5, 4),
	brt(4, 4),
	drk(4, 6),
	brt(4, 10),
	brt(5, 10),
	brt(5, 6),
	brt(4, 6),
	LAST,

/*	table for ascii 125, right brace	*/
	drk(2, 0),
	brt(3, 0),
	brt(4, 1),
	brt(4, 4),
	brt(5, 5),
	brt(4, 6),
	brt(4, 9),
	brt(3, 10),
	brt(2, 10),
	LAST,

/*	table for ascii 126, tilde	*/
	drk(0, 5),
	brt(1, 6),
	brt(3, 6),
	brt(5, 4),
	brt(7, 4),
	brt(8, 5),
	LAST,

/*	table for ascii 127, rubout	*/
	drk(0, 2),
	brt(0, 8),
	brt(8, 8),
	brt(8, 2),
	brt(0, 2),
	LAST
};

/*
 *  This FORTRAN interface expects REAL args (single precision).
 */
FSYMBOL( fp, string, x, y, scale, theta )
FILE	**fp;
char	*string;
float	*x, *y;
float	*scale;
float	*theta;
{
	char buf[128];

	tp_strncpy( buf, string, sizeof(buf) );
	tp_symbol( *fp, buf, *x, *y, *scale, *theta );
}

/*
 *			T P _ S T R N C P Y
 *
 *  Make null-terminated copy of a string in output buffer,
 *  being careful not to exceed indicated buffer size
 *  Accept "$" as alternate string-terminator for FORTRAN Holerith constants.
 */
tp_strncpy( out, in, sz )
register char *out;
register char *in;
register int sz;
{
	register int c;

	while( --sz > 0 && (c = *in++) != '\0' && c != '$' )
		*out++ = c;
	*out++ = '\0';
}
