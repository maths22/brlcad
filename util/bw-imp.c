/*
 *			B W - I M P R E S S . C
 *
 *  Authors -
 *	Douglas A. Gwyn
 *	Michael John Muuss
 *
 *  Borrows heavily from Steve Hawley's & Geoffrey Cooper's
 *  "traceconv" program.
 *
 *  Notes -
 *	The image is printed upside down to simplify the arithmetic,
 *	due to the organization of the input file.
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

#include	<stdio.h>

typedef int bool;
#define true 1
#define false 0

extern char	*optarg;
extern int	optind;

extern void	exit();
extern int	getopt();

typedef unsigned char u_char;
static u_char	halftone[8][8] =	/* halftone pattern */
{
	6,   7,   8,   9,  10,  11,  12,  13,
	40,  42,  44,  46,  49,  51,  54,  14,
	37, 112, 118, 124, 130, 137,  57,  15,
	36, 106, 208, 219, 230, 145,  60,  16,
	34, 101, 197, 254, 243, 152,  63,  17,
	32,  96, 187, 178, 169, 160,  66,  18,
	30,  91,  86,  82,  78,  74,  70,  19,
	29,  27,  26,  25,  23,  22,  21,  20
};

static u_char	dither[8][8] =		/* dither pattern */
{
	6,  51,  14,  78,   8,  57,  16,  86,
	118,  22, 178,  34, 130,  25, 197,  37,
	18,  96,  10,  63,  20, 106,  12,  70,
	219,  42, 145,  27, 243,  46, 160,  30,
	9,  60,  17,  91,   7,  54,  15,  82,
	137,  26, 208,  40, 124,  23, 187,  36,
	21, 112,  13,  74,  19, 101,  11,  66,
	254,  49, 169,  32, 230,  44, 152,  29
};

static u_char	(*pattern)[8] = dither;	/* -> dither or halftone */

static FILE	*infp;			/* input file handle */
static char	*file_name = "-";	/* name of input file, for banner */

static int	height;			/* input height */
static int	width;			/* input width */

static int	thresh = -1;		/* Threshold */

#define MAXWIDTH	2600
long		swath[32][MAXWIDTH/32];	/* assumes long has 32 bits */
unsigned char	line[MAXWIDTH];		/* grey-scale input buffer */

static int	im_mag;			/* magnification (1, 2 or 4) */
static int	im_width;		/* image size (in Imagen dots) */
static int	im_wpatches;		/* # 32-bit patches width */
static int	im_hpatches;		/* # 32-bit patches height */

bool	get_args();
bool	im_close();
bool	im_header();
void	im_write();

char usage[] = "\
Usage: bw-impress [-h -D] [-s squaresize] [-W width] [-H height]\n\
	[-t thresh] [file]\n";

bool
get_args( argc, argv )
register char	**argv;
{
	register int	c;

	while ( (c = getopt( argc, argv, "hDs:H:W:t:" )) != EOF )  {
		switch( c )  {
		case 'h':
			/* high-res */
			height = width = 1024;
			break;
		case 'D':
			/* halftone instead of dither */
			pattern = halftone;
			break;
		case 's':
			/* square size */
			height = width = atoi(optarg);
			break;
		case 'H':
			height = atoi(optarg);
			break;
		case 'W':
			width = atoi(optarg);
			break;
		case 't':
			thresh = atoi(optarg);
			break;

		default:		/* '?' */
			return false;
		}
	}

	if( optind >= argc )  {
		if( isatty(fileno(stdin)) )
			return(false);
		file_name = "-";
		infp = stdin;
	} else {
		file_name = argv[optind];
		if( (infp = fopen(file_name, "r")) == NULL )  {
			(void)fprintf( stderr,
				"bw-impress: cannot open \"%s\" for reading\n",
				file_name );
			return false;
		}
	}

	if ( argc > ++optind )
		(void)fprintf( stderr, "bw-impress: excess argument(s) ignored\n" );

	return true;
}

main( argc, argv )
int		argc;
char		*argv[];
{
	register int 	y;
	int		flags;

	height = width = 512;		/* Defaults */

	if ( !get_args( argc, argv ) || isatty(fileno(stdout)) )  {
		(void)fputs(usage, stderr);
		return 1;
	}

	if( thresh >= 0 )  {
		/* Each pixel in gives one bit out, depending on thresh */
		im_mag = 1;
	} else {
		if( width > 512 )  im_mag = 2;
		else if( width > 256 )  im_mag = 4;
		else if( width > 128 )  im_mag = 8;
		else im_mag = 16;
	}
	im_width  = (width * im_mag) & (~31);
	im_wpatches = (im_width+31) / 32;
	im_hpatches = ((height * im_mag)+31) / 32;
	if( im_wpatches*32 > 2560 )  {
		fprintf(stderr,"bw-impress:  output too wide\n");
		return(1);
	}

	if ( !im_header() )
		return 1;

	for( y = 0; y < height; y += 32/im_mag )  {
		if( feof(infp) )
			return 1;
		im_write(y);
	}

	if ( !im_close() )
	    return 1;

	return 0;
}

bool
im_header()
{

	(void)printf( "@document(language impress, prerasterization on, Name \"%s\")",
	file_name
	    );

	(void)putchar(205);		/* SET_HV_SYSTEM (whole page) */
	(void)putchar(0x54);
	(void)putchar(135);		/* SET_ABS_H (left margin) */
	(void)putchar(0);
	(void)putchar(192);
	(void)putchar(137);		/* SET_ABS_V (top margin) */
	(void)putchar(2);
	(void)putchar(32);
	(void)putchar(205);		/* SET_HV_SYSTEM (set origin) */
	(void)putchar(0x74);
	(void)putchar(206);		/* SET_ADV_DIRS (normal raster scan) */
	(void)putchar(0);
	(void)putchar(236);		/* SET_MAGNIFICATION */
	(void)putchar(0);		/* x 1 */
	(void)putchar(235);		/* BITMAP */
	(void)putchar(3);		/* opaque (store) */
	(void)putchar(im_wpatches);	/* hsize (# patches across) */
	(void)putchar(im_hpatches);	/* vsize (# patches down) */

	return true;
}

void
im_write(y)
int y;
{
	int y1;

	/* Process one 32-bit high output swath */
	for ( y1 = 0; y1 < 32; y1 += im_mag )  {
		int x;

		/* Obtain a single line of 8-bit pixels */
		if( fread( line, 1, width, infp ) != width )  {
#ifdef BSD
			bzero( line, width );
#else
			memset( line, '\0', width );
#endif
		}

		/* construct im_mag scans of Imagen swath */
		for ( x = 0; x < im_width; x += 32 )  {
			int my;

			for ( my = 0; my < im_mag; ++my )  {
				register long	b = 0L;	/* image bits */
				int x1;

				for ( x1 = 0; x1 < 32; x1 += im_mag )  {
					register int	level =
					    line[width-1-((x + x1) / im_mag)];
					register int mx;

					if( im_mag <= 1 )  {
						b <<= 1;
						if( level < thresh )
							b |= 1L;
						continue;
					}
					for ( mx = 0; mx < im_mag; ++mx )  {
						register int pgx, pgy;	/* page position */
						b <<= 1;

						/* Compute Dither */
						pgx = x + x1 + mx;
						pgy = y + y1 + my;
						/* ameliorate grid regularity */
						if( pattern == halftone &&
						    (pgy % 16) >= 8 )
							pgx += 4;

						if( level < pattern[pgx % 8][pgy % 8] )
							b |= 1L;
					}
				}
				swath[y1 + my][x / 32] = b;
			}
		}
	}

	/* output the swath */
	{
		register int x, y;
		for ( x = 0; x < im_wpatches; ++x )  {
			for ( y = 0; y < 32; ++y )  {
				register long	b = swath[y][x];

				(void)putchar( (int)(b >> 24) & 0xFF );
				(void)putchar( (int)(b >> 16) & 0xFF );
				(void)putchar( (int)(b >> 8) & 0xFF );
				(void)putchar( (int)b & 0xFF );
			}
		}
	}
}

bool
im_close()
{
/*	(void)putchar( 219 );		/* ENDPAGE */

	(void)fflush( stdout );
	return true;
}
