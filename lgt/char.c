/*
	Authors:	Paul R. Stay
			Gary S. Moss
			Doug A. Gwyn

			U. S. Army Ballistic Research Laboratory
			Aberdeen Proving Ground
			Maryland 21005-5066
			(301)278-6647 or AV-298-6647
*/
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif
/*
	Originally extracted from SCCS archive:

		SCCS id:	@(#) char.c	2.1
		Modified: 	12/10/86 at 16:04:27	G S M
		Retrieved: 	2/4/87 at 08:53:52
		SCCS archive:	/vld/moss/src/lgt/s.char.c
*/
/* 
	char.c - routines for displaying a string on a frame buffer.
 */

#include <stdio.h>
#include <fb.h>
#include "./font.h"
#include "./extern.h"
#define BUFFSIZ 200
_LOCAL_ int	bitx();
_LOCAL_ void	do_char();

void
do_line( xpos, ypos, line )
int		xpos, ypos;
register char	*line;
	{	register int    currx;
		register int    char_count, char_id;
		register int	len = strlen( line );
	if( ffdes == NULL )
		{
		rt_log( "ERROR: do_line() called before get_Font().\n" );
		return;
		}
	currx = xpos;

	for( char_count = 0; char_count < len; char_count++ )
		{
		char_id = (int) line[char_count] & 0377;

		/* Since there is no valid space in font, skip to the right
			using the width of the digit zero.
		 */
		if( char_id == ' ' )
			{
			currx += (SWABV(dir['0'].width) + 2) / aperture_sz;
			continue;
			}

		/* locate the bitmap for the character in the file */
		if( fseek( ffdes, (long)(SWABV(dir[char_id].addr)+offset), 0 )
			== EOF
			)
			{
			rt_log( "fseek() to %ld failed.\n",
				(long)(SWABV(dir[char_id].addr) + offset)
				);
			return;
			}

		/* Read in the dimensions for the character */
		width = dir[char_id].right + dir[char_id].left;
		height = dir[char_id].up + dir[char_id].down;

		if( currx + width > fb_getwidth( fbiop ) - 1 )
			break;		/* won't fit on screen */

		do_char( char_id, currx, ypos );
		currx += (SWABV(dir[char_id].width) + 2) / aperture_sz;
    		}
	return;
	}

/*	d o _ c h a r ( )
	Outputs pixel representation of a chararcter by reading a row of a
	bitmap from the character font file.  The file pointer is assumed
	to be in the correct position.
 */
_LOCAL_ void
do_char( c, xpos, ypos )
int		c;
register int	xpos, ypos;
	{	int     	up = dir[c].up / aperture_sz;
		int		left = dir[c].left / aperture_sz;
		static char	bitbuf[BUFFSIZ][BUFFSIZ];
		static RGBpixel	pixel;
		register int    h, i, j, k, x;
	for( k = 0; k < height; k++ )
		{
		/* Read row, rounding width up to nearest byte value. */
		if( fread( bitbuf[k], width/8+(width % 8 == 0 ? 0 : 1), 1, ffdes )
			!= 1 )
			{
			rt_log( "\"%s\" (%d) read of character from font failed.\n",
				__FILE__, __LINE__
				);
			return;
			}
		}
	for( k = 0; k < height; k += aperture_sz, ypos-- )
		{
		x = xpos - left;
		for( j = 0; j < width; j += aperture_sz, x++ )
			{	register int	sum;
				fastf_t		weight;
			/* The bitx routine extracts the bit value.
				Can't just use the j-th bit because
				the bytes are backwards. */
			sum = 0;
			for( i = 0; i < aperture_sz; i++ )
				for( h = 0; h < aperture_sz; h++ )
					sum += bitx(	bitbuf[k+i],
							((j+h)&~7) + (7-((j+h)&7))
							) != 0;
			weight = (fastf_t) sum / sample_sz;
			if( fb_seek( fbiop, x, ypos + up ) == -1 )
				continue;
			if( fb_rpixel( fbiop, pixel ) == -1 )
				{
				rt_log( "\"%s\" (%d) read of pixel from <%d,%d> failed.\n",
					__FILE__, __LINE__, x, ypos
					);
				return;
				}
			pixel[RED] = pixel[RED]*(1.0-weight) + FONTCOLOR_RED*weight;
			pixel[GRN] = pixel[GRN]*(1.0-weight) + FONTCOLOR_GRN*weight;
			pixel[BLU] = pixel[BLU]*(1.0-weight) + FONTCOLOR_BLU*weight;
			if( fb_seek( fbiop, x, ypos + up ) == -1 )
				continue;
			if( fb_wpixel( fbiop, pixel ) == -1 )
				{
				rt_log( "\"%s\" (%d) write of pixel to <%d,%d> failed.\n",
					__FILE__, __LINE__, x, ypos
					);
				return;
				}
			}
		}
	return;
	}

/*	b i t x ( )
	Extract a bit field from a bit string.
 */
_LOCAL_ int
bitx( bitstring, posn )
register char *bitstring;
register int posn;
	{
#if defined( vax )
   	register field;

   	asm("extzv	r10,$1,(r11),r8");
	return field;
#else
	for( ; posn >= 8; posn -= 8, bitstring++ )
		;
#if defined( CANT_DO_ZERO_SHIFT )
	if( posn == 0 )
		return	(int)(*bitstring) & 1;
	else
#endif
	return	(int)(*bitstring) & (1<<posn);
#endif
	}
