/*
 *			T P _ M A R K E R
 *
 *	This routine places a specified character (either from
 * the ASCII set, or one of the 5 special marker characters)
 * centered about the current pen position (instead of above & to
 * the right of the current position, as characters usually go).
 * Calling sequence:
 *
 *	char c		is the character to be used for a marker,
 *			or one of the following special markers -
 *				1 = plus
 *				2 = an "x"
 *				3 = a triangle
 *				4 = a square
 *				5 = an hourglass
 *  Author -
 *	Mike Muuss
 *	August 04, 1978
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

#include <stdio.h>

#include "machine.h"
#include "vmath.h"
#include "./tig.h"

void
tp_2marker( fp, c, x, y, scale )
FILE	*fp;
register int c;
double	x, y;
double	scale;
{
	char	mark_str[4];

	mark_str[0] = (char)c;
	mark_str[1] = '\0';

	/* Draw the marker */
	tp_2symbol( fp, mark_str,
		(x - scale*0.5), (y - scale*0.5),
		scale, 0.0 );
}

void
F(f2mark, F2MARK)( fp, c, x, y, scale )
FILE	**fp;
int	*c;
float	*x, *y;
float	*scale;
{
	tp_2marker( *fp, *c, *x, *y, *scale );
}

void
tp_3marker( fp, c, x, y, z, scale )
FILE	*fp;
register int c;
double	x, y, z;
double	scale;
{
	char	mark_str[4];
	mat_t	mat;
	vect_t	p;

	mark_str[0] = (char)c;
	mark_str[1] = '\0';
	mat_idn( mat );
	VSET( p, x - scale*0.5, y - scale*0.5, z );
	tp_3symbol( fp, mark_str, p, mat, scale );
}

void
F(f3mark, F3MARK)( fp, c, x, y, z, scale )
FILE	**fp;
int	*c;
float	*x, *y, *z;
float	*scale;
{
	tp_3marker( *fp, *c, *x, *y, *z, *scale );
}
