/*
 *			R L E - F B . C
 *
 *  Decode a Utah Raster Toolkit RLE image, and display on a
 *  BRL libfb(3) framebuffer.
 *
 *  Authors -
 *	Michael John Muuss
 *	Paul R. Stay
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
#include "fb.h"
#include "svfb_global.h"

extern int	optind;
extern char	*optarg;
extern char	*getenv();

extern char	*malloc();

static FILE	*infp = stdin;
static char	*infile;

static int	background[3];
static int	override_background;

unsigned char	*rows[4];		/* Character pointers for rle_getrow */
	
static RGBpixel	*scan_buf;		/* single scanline buffer */
static ColorMap	cmap;

static char	*framebuffer = (char *)0;
static int	screen_width = 0;
static int	screen_height = 0;

static int	crunch;
static int	overlay;
static int	r_debug;

static char	usage[] = "\
Usage: rle-fb [-h -d -v -c -O] [-F framebuffer]  [-C r/g/b]\n\
	[-s squarefilesize] [-w file_width] [-n file_height] [file.rle]\n\
";

/*
 *			G E T _ A R G S
 */
static int
get_args( argc, argv )
register char	**argv;
{
	register int	c;

	while( (c = getopt( argc, argv, "cOdhs:w:n:C:F:" )) != EOF )  {
		switch( c )  {
		case 'O':
			overlay = 1;
			break;
		case 'd':
			r_debug = 1;
			break;
		case 'F':
			framebuffer = optarg;
			break;
		case 'c':
			crunch = 1;
			break;
		case 'h':
			/* high-res */
			screen_height = screen_width = 1024;
			break;
		case 's':
			/* square screen size */
			screen_height = screen_width = atoi(optarg);
			break;
		case 'w':
			screen_width = atoi(optarg);
			break;
		case 'n':
			screen_height = atoi(optarg);
			break;
		case 'C':
			{
				register char *cp = optarg;
				register int *conp = background;

				/* premature null => atoi gives zeros */
				for( c=0; c < 3; c++ )  {
					*conp++ = atoi(cp);
					while( *cp && *cp++ != '/' ) ;
				}
				override_background = 1;
			}
			break;
		case '?':
			return	0;
		}
	}
	if( argv[optind] != NULL )  {
		if( (infp = fopen( (infile=argv[optind]), "r" )) == NULL )  {
			perror(infile);
			return	0;
		}
		optind++;
	} else {
		infile = "-";
	}
	if( argc > ++optind )
		(void) fprintf( stderr, "Excess arguments ignored\n" );

	if( isatty(fileno(infp)) )
		return 0;
	return	1;
}

/*
 *			M A I N
 */
main( argc, argv)
int argc;
char ** argv;
{
	FBIO	*fbp;
	register int i;
	int	x_len, y_len;
	int	xbase;
	int	ncolors;

	if( !get_args( argc, argv ) )  {
		(void)fputs(usage, stderr);
		exit( 1 );
	}

	sv_globals.svfb_fd = infp;
	if( rle_get_setup( &sv_globals ) < 0 )  {
		fprintf(stderr, "rle-fb: Error reading setup information\n");
		exit(1);
	}

	if (r_debug)  {
		fprintf( stderr,"Image bounds\n\tmin %d %d\n\tmax %d %d\n",
			sv_globals.sv_xmin, sv_globals.sv_ymin,
			sv_globals.sv_xmax, sv_globals.sv_ymax );
		fprintf(stderr, "%d color channels\n", sv_globals.sv_ncolors);
		fprintf(stderr,"%d color map channels\n", sv_globals.sv_ncmap);
		if ( sv_globals.sv_alpha )
			fprintf( stderr, "Alpha Channel present in input, ignored.\n");
		for( i=0; i < sv_globals.sv_ncolors; i++ )
			fprintf(stderr,"Background channel %d = %d\n",
				i, sv_globals.sv_bg_color[i] );
		rle_debug(1);
	}

	if( sv_globals.sv_ncmap == 0 )
		crunch = 0;

	/* Only interested in R, G, & B */
	SV_CLR_BIT(sv_globals, SV_ALPHA);
	for (i = 3; i < sv_globals.sv_ncolors; i++)
		SV_CLR_BIT(sv_globals, i);
	ncolors = sv_globals.sv_ncolors > 3 ? 3 : sv_globals.sv_ncolors;

	/* Optional switch of library to overlay mode */
	if( overlay )  {
		sv_globals.sv_background = 1;		/* overlay */
		override_background = 0;
	}

	/* Optional background color override */
	if( override_background )  {
		for( i=0; i<ncolors; i++ )
			sv_globals.sv_bg_color[i] = background[i];
	}

	if( sv_globals.sv_xmax > screen_width ||
	    sv_globals.sv_ymax > screen_height )  {
	    	screen_width = sv_globals.sv_xmax + 1;
	    	screen_height = sv_globals.sv_ymax + 1;
	}

	x_len = sv_globals.sv_xmax - sv_globals.sv_xmin + 1;
	y_len = sv_globals.sv_ymax - sv_globals.sv_ymin + 1;

	/* Pretend saved image origin is at 0, fix in fb_write call */
	xbase = sv_globals.sv_xmin;
	sv_globals.sv_xmax -= xbase;
	sv_globals.sv_xmin = 0;

	if( (fbp = fb_open( framebuffer, screen_width, screen_height )) == FBIO_NULL )
		exit(12);
	screen_width = fb_getwidth(fbp);
	screen_height = fb_getheight(fbp);

	scan_buf = (RGBpixel *)malloc( sizeof(RGBpixel) * screen_width );

	for( i=0; i < ncolors; i++ )
		rows[i] = (unsigned char *)malloc(x_len);
	for( ; i < 3; i++ )
		rows[i] = rows[0];	/* handle monochrome images */

	/*
	 *  Import Utah color map, converting to libfb format.
	 *  Check for old format color maps, where high 8 bits
	 *  were zero, and correct them.
	 *  XXX need to handle < 3 channels of color map, by replication.
	 */
	if( sv_globals.sv_ncmap > 0 )  {
		register int maplen = (1 << sv_globals.sv_cmaplen);
		register int all = 0;
		for( i=0; i<256; i++ )  {
			cmap.cm_red[i] = sv_globals.sv_cmap[i];
			cmap.cm_green[i] = sv_globals.sv_cmap[i+maplen];
			cmap.cm_blue[i] = sv_globals.sv_cmap[i+2*maplen];
			all |= cmap.cm_red[i] | cmap.cm_green[i] |
				cmap.cm_blue[i];
		}
		if( (all & 0xFF00) == 0 && (all & 0x00FF) != 0 )  {
			/*  This is an old (Edition 2) color map.
			 *  Correct by shifting it left 8 bits.
			 */
			for( i=0; i<256; i++ )  {
				cmap.cm_red[i] <<= 8;
				cmap.cm_green[i] <<= 8;
				cmap.cm_blue[i] <<= 8;
			}
			fprintf(stderr,
				"rle-fb: correcting for old style colormap\n");
		}
	}
	if( sv_globals.sv_ncmap > 0 && !crunch )
		(void)fb_wmap( fbp, &cmap );


	for ( i = sv_globals.sv_ymin; i <= sv_globals.sv_ymax; i++)  {
		register unsigned char	*pp = (unsigned char *)scan_buf;
		register rle_pixel	*rp = rows[0];
		register rle_pixel	*gp = rows[1];
		register rle_pixel	*bp = rows[2];
		register int		j;

		if( overlay )  {
			fb_read( fbp, xbase, i, scan_buf, x_len );
			for( j = 0; j < x_len; j++ )  {
				*rp++ = *pp++;
				*gp++ = *pp++;
				*bp++ = *pp++;
			}
			pp = (unsigned char *)scan_buf;
			rp = rows[0];
			gp = rows[1];
			bp = rows[2];
		}

		rle_getrow(&sv_globals, rows );

		/* Grumble, convert from Utah layout */
		if( !crunch )  {
			for ( j = 0; j < x_len; j++)  {
				*pp++ = *rp++;
				*pp++ = *gp++;
				*pp++ = *bp++;
			}
		} else {
			for ( j = 0; j < x_len; j++)  {
				*pp++ = cmap.cm_red[*rp++]>>8;
				*pp++ = cmap.cm_green[*gp++]>>8;
				*pp++ = cmap.cm_blue[*bp++]>>8;
			}
		}
		fb_write( fbp, xbase, i, scan_buf, x_len );
	}
	fb_close( fbp );
	exit(0);
}
