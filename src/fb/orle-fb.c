/*
 *			O R L E - F B . C
 *
 *  Display an old RLE format image on a framebuffer
 *
 *  Author -
 *	Gary S. Moss
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1986-2004 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static const char RCSid[] = "@(#)$Id$ (BRL)";
#endif

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif



#include <stdio.h>
#ifdef USE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include "machine.h"
#include "externs.h"			/* For getopt() */
#include "fb.h"
#include "orle.h"

static char	*usage[] =
	{
"Usage: orle-fb [-Otdv] [-b (rgbwBG)] [-F Frame_buffer] [-p X Y] [file.rle]",
"",
"If no rle file is specifed, rle-fb will read its standard input.",
"If the environment variable FB_FILE is set, its value will be used",
"	to specify the framebuffer file or device to write to.",
0
	};

static FBIO	*fbp;
static FILE	*fp;
static RGBpixel	bgpixel;
static int	bgflag = 0;
static int	olflag = 0;
static int	pars_Argv(int argc, register char **argv);
static int	xlen = -1, ylen = -1;
static int	xpos = -1, ypos = -1;
static void	prnt_Cmap(ColorMap *cmap);
static void	prnt_Usage(void);
static int	width = 512;
static int	height = 512;
static int	topdown = 0;
static int	pixels_per_buffer;
static char	*fb_file = (char *)NULL;

void		fill_Buffer(register char *dest, register char *src, register int scan_bytes, register int repeat);

/*	m a i n ( )							*/
int
main(int argc, char **argv)
{	register int	y;
		register int	lines_per_buffer;
		register unsigned char *scanbuf;
		static RGBpixel	bg_scan[8192+1];
		static ColorMap	cmap;
		int		get_flags;

	fp = stdin;
	if( ! pars_Argv( argc, argv ) || isatty(fileno(fp)) )
		{
		prnt_Usage();
		return	1;
		}
	if( bgflag )  {
		/* User has supplied his own background */
		if( rle_rhdr( fp, &get_flags, RGBPIXEL_NULL ) == -1 )
			return 1;
	} else {
		if( rle_rhdr( fp, &get_flags, bgpixel ) == -1 )
		return	1;
	}

	rle_rlen( &xlen, &ylen );
	if( xpos < 0 || ypos < 0 )
		rle_rpos( &xpos, &ypos );
	else
		rle_wpos( xpos, ypos, 0 );

	/* Automatic selection of high res. device.			*/
	if( xpos + xlen > 512 || ypos + ylen > 512 )
		width = height = 1024;
	if( xpos + xlen > width )
		xlen = width - xpos;
	if( ypos + ylen > height )
		ylen = height - ypos;
	rle_wlen( xlen, ylen, 0 );

	if( (fbp = fb_open( fb_file, width, height )) == NULL )  {
		exit(12);
	}

	if( topdown )
		pixels_per_buffer = width * height;
	else
		pixels_per_buffer = width * 64;
	scanbuf = RGBPIXEL_NULL;
	while( scanbuf == RGBPIXEL_NULL && pixels_per_buffer > 0 )  {
		scanbuf = (unsigned char *)malloc(pixels_per_buffer*sizeof(RGBpixel));
		if( scanbuf == RGBPIXEL_NULL )  {
			pixels_per_buffer >>= 1;
			continue;
		}
		break;
	}
	if( scanbuf == RGBPIXEL_NULL )  {
		fprintf(stderr," rle-fb:  unable to malloc pixel buffer\n");
		return(1);
	}

	lines_per_buffer = pixels_per_buffer / width;	/* # of full scanlines in buffer */
	pixels_per_buffer = lines_per_buffer * width;

	if( rle_verbose )
		(void) fprintf( stderr,
				"Background is %d %d %d\n",
				bgpixel[RED], bgpixel[GRN], bgpixel[BLU]
				);

	/* If color map provided, use it, else go with standard map. */
	if( ! (get_flags & NO_COLORMAP) )
		{
		if( rle_verbose && ! olflag )
			(void) fprintf( stderr,
					"Loading saved color map from file\n"
					);
		if( rle_rmap( fp, &cmap ) == -1 )
			return	1;
		if( rle_verbose )
			prnt_Cmap( &cmap );
		if( ! olflag )
			{
			/* User-supplied map */
			if( rle_verbose )
				(void) fprintf( stderr,
					"Writing color map to framebuffer\n"
						);
			if( fb_wmap( fbp, &cmap ) == -1 )
				return	1;
			}
		}
	else
	if( ! olflag )
		{
		if( rle_verbose )
			(void) fprintf( stderr,
					"Creating standard color map\n"
					);
		if( fb_wmap( fbp, COLORMAP_NULL ) == -1 )
			return	1;
		}
	/* Fill a DMA buffer buffer with background */
	if( ! olflag && (get_flags & NO_BOX_SAVE) )
		{	register int	i;
			register RGBpixel	*to;
		to = bg_scan;
		for( i = 0; i < width; i++,to++ )  {
			COPYRGB( *to, bgpixel );
			}
		}

#ifndef SIMPLE
	{	register int	page_fault = 1;
		register int	dirty_flag = 1;
		int		ymax = ypos + (ylen-1);
		int		start_y = 0;
	for( y = 0; y < width; y++ )  {
		if( page_fault )  {
			start_y = y;
			if( olflag )  {
				/* Overlay - read cluster from fb.	*/
				if( fb_read( fbp, 0, y, scanbuf, pixels_per_buffer ) == -1 )
					return	1;
			} else
			if( (get_flags & NO_BOX_SAVE) && dirty_flag )
				fill_Buffer(	(char *) scanbuf,
						(char *) bg_scan,
						width*sizeof(RGBpixel),
						lines_per_buffer
						);
			dirty_flag = 0;
			page_fault = 0;
		}
		if( y >= ypos && y <= ymax )  {
			if( rle_decode_ln( fp,
			    scanbuf[(y%lines_per_buffer)*width*sizeof(RGBpixel)] ) == -1 )
				break;		/* not return */
			dirty_flag = 1;
		}
		page_fault = ((y%lines_per_buffer)==(lines_per_buffer-1));
		if( page_fault )  {
			if( fb_write( fbp, 0, start_y, scanbuf, pixels_per_buffer ) == -1 )
				return	1;
		}
	}
	if( page_fault == 0 )  {
		/* Write out the residue, a short buffer */
		if( fb_write( fbp, 0, start_y, scanbuf, (y-start_y)*width ) == -1 )
			return(1);
	}
	} /* end block */
#else
	/* Simplified version, for testing */
	for( y = 0; y < height; y++ )  {
		if( olflag )  {
			/* Overlay - read cluster from fb.	*/
			if( fb_read( fbp, 0, y, scanbuf, width ) == -1 )
				return	1;
		} else {
			if( (get_flags & NO_BOX_SAVE) )
				fill_Buffer(	(char *) scanbuf,
				(char *) bg_scan,
				width*sizeof(RGBpixel),
				1 );
		}
		if( y <= ypos+ylen && y >= ypos )
			if( rle_decode_ln( fp, scanbuf ) == -1 )
				return	1;
		if( fb_write( fbp, 0, y, scanbuf, width ) == -1 )
			return	1;
	}
#endif

	/* Write background pixel in agreed-upon place */
	(void)fb_write( fbp, 1, 1, bgpixel, 1 );

	fb_close( fbp );
	return	0;
	}

/*	f i l l _ B u f f e r ( )
	Fill cluster buffer from scanline (as fast as possible).
 */
void
fill_Buffer(register char *dest, register char *src, register int scan_bytes, register int repeat)
{	register int	i;
	for( i = 0; i < repeat; ++i )
		{
		bcopy( src, dest, scan_bytes );
		dest += scan_bytes;
		}
	return;
	}

/*	p a r s _ A r g v ( )						*/
static int
pars_Argv(int argc, register char **argv)
{	register int	c;
	/* Parse options.						*/
	while( (c = getopt( argc, argv, "tOF:b:dp:v" )) != EOF )
		{
		switch( c )
			{
		case 't':
			/* Top-down mode */
			topdown = 1;
			break;
		case 'O' : /* Overlay mode.				*/
			olflag = 1;
			break;
		case 'b' : /* User-specified background.		*/
			bgflag = optarg[0];
			switch( bgflag )
				{
			case 'r':
				bgpixel[RED] = 255;
				break;
			case 'g':
				bgpixel[GRN] = 255;
				break;
			case 'b':
				bgpixel[BLU] = 255;
				break;
			case 'w':
				bgpixel[RED] =
				bgpixel[GRN] =
				bgpixel[BLU] = 255;
				break;
			case 'B':		/* Black */
				break;
			case 'G':		/* 18% grey, for alignments */
				bgpixel[RED] =
				bgpixel[GRN] =
				bgpixel[BLU] = 255.0 * 0.18;
				break;
			default:
				(void) fprintf( stderr,
						"Background '%c' unknown\n",
						bgflag
						);
				bgflag = 0;
				break;
				} /* End switch */
			break;
		case 'd' :
			rle_debug = 1;
			break;
		case 'p' :
			if( argc - optind < 2 )
				{
				(void) fprintf( stderr,
				"-p option requires an X and Y argument!\n"
						);
				return	0;
				}
			xpos = atoi( optarg );
			ypos = atoi( argv[optind++] );
			break;
		case 'v' :
			rle_verbose = 1;
			break;
		case 'F' : fb_file = optarg;
			break;
		case '?' :
			return	0;
			} /* end switch */
		} /* end while */

	if( argv[optind] != NULL )
		if( (fp = fopen( argv[optind], "r" )) == NULL )
			{
			(void) fprintf( stderr,
					"Can't open %s for reading!\n",
					argv[optind]
					);
			return	0;
			}
	if( argc > ++optind )
		{
		(void) fprintf( stderr, "Too many arguments!\n" );
		return	0;
		}
	return	1;
	}

/*	p r n t _ U s a g e ( )
	Print usage message.
 */
static void
prnt_Usage(void)
{	register char	**p = usage;
	while( *p )
		(void) fprintf( stderr, "%s\n", *p++ );
	return;
	}

static void
prnt_Cmap(ColorMap *cmap)
{	register unsigned short	*cp;
		register int	i;
	(void) fprintf( stderr, "\t\t\t_________ Color map __________\n" );
	(void) fprintf( stderr, "Red segment :\n" );
	for( i = 0, cp = cmap->cm_red; i < 16; ++i, cp += 16 )
		{
		(void) fprintf( stderr,
	"%3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d\n",
	/* 1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16 */
				cp[0],
				cp[1],
				cp[2],
				cp[3],
				cp[4],
				cp[5],
				cp[6],
				cp[7],
				cp[8],
				cp[9],
				cp[10],
				cp[11],
				cp[12],
				cp[13],
				cp[14],
				cp[15]
				);
		}
	(void) fprintf( stderr, "Green segment :\n" );
	for( i = 0, cp = cmap->cm_green; i < 16; ++i, cp += 16 )
		{
		(void) fprintf( stderr, 
	"%3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d\n",
	/* 1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16 */
				cp[0],
				cp[1],
				cp[2],
				cp[3],
				cp[4],
				cp[5],
				cp[6],
				cp[7],
				cp[8],
				cp[9],
				cp[10],
				cp[11],
				cp[12],
				cp[13],
				cp[14],
				cp[15]
				);
		}
	(void) fprintf( stderr, "Blue segment :\n" );
	for( i = 0, cp = cmap->cm_blue; i < 16; ++i, cp += 16 )
		{
		(void) fprintf( stderr,
	"%3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d\n",
	/* 1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16 */
				cp[0],
				cp[1],
				cp[2],
				cp[3],
				cp[4],
				cp[5],
				cp[6],
				cp[7],
				cp[8],
				cp[9],
				cp[10],
				cp[11],
				cp[12],
				cp[13],
				cp[14],
				cp[15]
				);
		}
	return;
	}
