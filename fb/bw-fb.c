/*
 *			B W - F B . C
 *
 * Write a black and white (.bw) image to the framebuffer.
 * From an 8-bit/pixel, pix order file (i.e. Bottom UP, left to right).
 *
 * This allows an offset into both the display and source file.
 * The color planes to be loaded are also selectable.
 *
 *  Author -
 *	Phillip Dykstra
 *	15 Aug 1985
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1986 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include "fb.h"

extern int	getopt();
extern char	*optarg;
extern int	optind;

#define	MAX_LINE	(16*1024)	/* Largest output scan line length */

char	ibuf[MAX_LINE];
RGBpixel obuf[MAX_LINE];

int	fileinput = 0;		/* file of pipe on input? */

int	file_width = 512;	/* default input width */
int	file_height = 512;	/* default input height */
int	scr_width = 0;		/* screen tracks file if not given */
int	scr_height = 0;
int	file_xoff, file_yoff;
int	scr_xoff, scr_yoff;
int	clear = 0;
int	zoom = 0;
int	inverse = 0;
int	redflag   = 0;
int	greenflag = 0;
int	blueflag  = 0;

char	*framebuffer = NULL;
char	*file_name;
int	infd;
FBIO	*fbp;

char	usage[] = "\
Usage: bw-fb [-h -i -c -z -R -G -B] [-F framebuffer]\n\
	[-s squarefilesize] [-w file_width] [-n file_height]\n\
	[-x file_xoff] [-y file_yoff] [-X scr_xoff] [-Y scr_yoff]\n\
	[-S squarescrsize] [-W scr_width] [-N scr_height] [file.bw]\n";

get_args( argc, argv )
register char **argv;
{
	register int c;

	while ( (c = getopt( argc, argv, "hiczRGBF:s:w:n:x:y:X:Y:S:W:N:" )) != EOF )  {
		switch( c )  {
		case 'h':
			/* high-res */
			file_height = file_width = 1024;
			scr_height = scr_width = 1024;
			break;
		case 'i':
			inverse = 1;
			break;
		case 'c':
			clear = 1;
			break;
		case 'z':
			zoom = 1;
			break;
		case 'R':
			redflag = 1;
			break;
		case 'G':
			greenflag = 1;
			break;
		case 'B':
			blueflag = 1;
			break;
		case 'F':
			framebuffer = optarg;
			break;
		case 's':
			/* square file size */
			file_height = file_width = atoi(optarg);
			break;
		case 'w':
			file_width = atoi(optarg);
			break;
		case 'n':
			file_height = atoi(optarg);
			break;
		case 'x':
			file_xoff = atoi(optarg);
			break;
		case 'y':
			file_yoff = atoi(optarg);
			break;
		case 'X':
			scr_xoff = atoi(optarg);
			break;
		case 'Y':
			scr_yoff = atoi(optarg);
			break;
		case 'S':
			scr_height = scr_width = atoi(optarg);
			break;
		case 'W':
			scr_width = atoi(optarg);
			break;
		case 'N':
			scr_height = atoi(optarg);
			break;

		default:		/* '?' */
			return(0);
		}
	}

	if( optind >= argc )  {
		if( isatty(fileno(stdin)) )
			return(0);
		file_name = "-";
		infd = 0;
	} else {
		file_name = argv[optind];
		if( (infd = open(file_name, 0)) == NULL )  {
			(void)fprintf( stderr,
				"bw-fb: cannot open \"%s\" for reading\n",
				file_name );
			return(0);
		}
		fileinput++;
	}

	if ( argc > ++optind )
		(void)fprintf( stderr, "bw-fb: excess argument(s) ignored\n" );

	return(1);		/* OK */
}

main( argc, argv )
int argc; char **argv;
{
	register int	x, y, n;
	int	xout, yout;		/* number of sceen output lines */

	if ( !get_args( argc, argv ) )  {
		(void)fputs(usage, stderr);
		exit( 1 );
	}

	/* If no color planes were selected, load them all */
	if( redflag == 0 && greenflag == 0 && blueflag == 0 )
		redflag = greenflag = blueflag = 1;

	/* If screen size was not set, track the file size */
	if( scr_width == 0 )
		scr_width = file_width;
	if( scr_height == 0 )
		scr_height = file_height;

	/* Open Display Device */
	if ((fbp = fb_open( framebuffer, scr_width, scr_height )) == NULL ) {
		fprintf( stderr, "fb_open failed\n");
		exit( 3 );
	}

	/* Get the screen size we were given */
	scr_width = fb_getwidth(fbp);
	scr_height = fb_getheight(fbp);

	/* compute pixels output to screen */
	xout = scr_width - scr_xoff;
	if( xout < 0 ) xout = 0;
	if( xout > (file_width-file_xoff) ) xout = (file_width-file_xoff);
	yout = scr_height - scr_yoff;
	if( yout < 0 ) yout = 0;
	if( yout > (file_height-file_yoff) ) yout = (file_height-file_yoff);
	if( xout > MAX_LINE ) {
		fprintf( stderr, "bw-fb: can't output %d pixel lines.\n", xout );
		exit( 2 );
	}

	if( clear ) {
		fb_clear( fbp, PIXEL_NULL );
		fb_wmap( fbp, COLORMAP_NULL );
	}
	if( zoom ) {
		/* Zoom in, and center the file */
		fb_zoom( fbp, scr_width/xout, scr_height/yout );
		if( inverse )
			fb_window( fbp, scr_xoff+xout/2, scr_height-1-(scr_yoff+yout/2) );
		else
			fb_window( fbp, scr_xoff+xout/2, scr_yoff+yout/2 );
	}

	if( file_yoff != 0 ) skipbytes( infd, file_yoff*file_width );

	for( y = scr_yoff; y < scr_yoff + yout; y++ ) {
		if( file_xoff != 0 )
			skipbytes( infd, file_xoff );
		n = mread( infd, &ibuf[0], xout );
		if( n <= 0 ) break;
		/*
		 * If we are not loading all color planes, we have
		 * to do a pre-read.
		 */
		if( redflag == 0 || greenflag == 0 || blueflag == 0 ) {
			if( inverse )
				n = fb_read( fbp, scr_xoff, scr_height-1-y,
					obuf, xout );
			else
				n = fb_read( fbp, scr_xoff, y, obuf, xout );
			if( n < 0 )  break;
		}
		for( x = 0; x < xout; x++ ) {
			if( redflag )
				obuf[x][RED] = ibuf[x];
			if( greenflag )
				obuf[x][GRN] = ibuf[x];
			if( blueflag )
				obuf[x][BLU] = ibuf[x];
		}
		if( inverse )
			fb_write( fbp, scr_xoff, scr_height-1-y, obuf, xout );
		else
			fb_write( fbp, scr_xoff, y, obuf, xout );

		/* slop at the end of the line? */
		if( xout < file_width-file_xoff )
			skipbytes( infd, file_width-file_xoff-xout );
	}

	fb_close( fbp );
	exit( 0 );
}

/*
 * Throw bytes away.  Use reads into ibuf buffer if a pipe, else seek.
 */
skipbytes( fd, num )
int	fd;
long	num;
{
	int	n, try;

	if( fileinput ) {
		(void)lseek( fd, num, 1 );
		return 0;
	}
	
	while( num > 0 ) {
		try = num > MAX_LINE ? MAX_LINE : num;
		n = read( fd, ibuf, try );
		if( n <= 0 ){
			return -1;
		}
		num -= n;
	}
	return	0;
}

/*
 * "Multiple try" read.
 *  Will keep reading until either an error occurs
 *  or the requested number of bytes is read.  This
 *  is important for pipes.
 */
mread( fd, bp, num )
int	fd;
register char	*bp;
register int	num;
{
	register int	n;
	int	count;

	count = 0;

	while( num > 0 ) {
		n = read( fd, bp, num );
		if( n < 0 )
			return	-1;
		if( n == 0 )
			return count;
		bp += n;
		count += n;
		num -= n;
	}
	return count;
}
