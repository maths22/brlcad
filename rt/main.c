/*
 *			R T . C 
 *
 *  Ray Tracing main program, using RT library.
 *  Invoked by MGED for quick pictures.
 *  Is linked with each of three "back ends" (view.c, viewpp.c, viewray.c)
 *  to produce three executable programs:  rt, rtpp, rtray.
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
 *	This software is Copyright (C) 1985,1987 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSrt[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include "machine.h"
#include "vmath.h"
#include "raytrace.h"
#include "fb.h"
#include "./mathtab.h"
#include "./rdebug.h"
#include "../librt/debug.h"

extern int	getopt();
extern char	*optarg;
extern int	optind;

extern char	usage[];

extern double	atof();
extern char	*sbrk();

int		rdebug;			/* RT program debugging (not library) */

/***** Variables shared with viewing model *** */
FBIO		*fbp = FBIO_NULL;	/* Framebuffer handle */
FILE		*outfp = NULL;		/* optional pixel output file */
int		hex_out = 0;		/* Binary or Hex .pix output file */
double		AmbientIntensity = 0.4;	/* Ambient light intensity */
double		azimuth, elevation;
int		lightmodel;		/* Select lighting model */
mat_t		view2model;
mat_t		model2view;
/***** end of sharing with viewing model *****/

/***** variables shared with worker() ******/
struct application ap;
int		stereo = 0;		/* stereo viewing */
vect_t		left_eye_delta;
int		hypersample=0;		/* number of extra rays to fire */
int		perspective=0;		/* perspective view -vs- parallel */
vect_t		dx_model;		/* view delta-X as model-space vect */
vect_t		dy_model;		/* view delta-Y as model-space vect */
point_t		eye_model;		/* model-space location of eye */
int		width;			/* # of pixels in X */
int		height;			/* # of lines in Y */
mat_t		Viewrotscale;
fastf_t		viewsize=0;
fastf_t		zoomout=1;		/* >0 zoom out, 0..1 zoom in */
char		*scanbuf;		/* For optional output buffering */
int		incr_mode;		/* !0 for incremental resolution */
int		incr_level;		/* current incremental level */
int		incr_nlevel;		/* number of levels */
int		parallel=0;		/* Trying to use multi CPUs */
int		npsw = MAX_PSW;		/* number of worker PSWs to run */
struct resource	resource[MAX_PSW];	/* memory resources */
/***** end variables shared with worker() *****/

/***** variables shared with do.c *****/
int		pix_start = -1;		/* pixel to start at */
int		pix_end;		/* pixel to end at */
int		nobjs;			/* Number of cmd-line treetops */
char		**objtab;		/* array of treetop strings */
char		*beginptr;		/* sbrk() at start of program */
int		matflag = 0;		/* read matrix from stdin */
int		desiredframe = 0;	/* frame to start at */
int		curframe = 0;		/* current frame number */
char		*outputfile = (char *)0;/* name of base of output file */
/***** end variables shared with do.c *****/

static char	*framebuffer;		/* desired framebuffer */

#define MAX_WIDTH	(8*1024)

/*
 *			G E T _ A R G S
 */
get_args( argc, argv )
register char **argv;
{
	register int c;
	register int i;

	while( (c=getopt( argc, argv, "SH:F:D:MA:x:X:s:f:a:e:l:O:o:p:P:Bb:n:w:i" )) != EOF )  {
		switch( c )  {
		case 'i':
			incr_mode = 1;
			break;
		case 'S':
			stereo = 1;
			break;
		case 'H':
			hypersample = atoi( optarg );
			break;
		case 'F':
			framebuffer = optarg;
			break;
		case 'D':
			desiredframe = atoi( optarg );
			break;
		case 'M':
			matflag = 1;
			break;
		case 'A':
			AmbientIntensity = atof( optarg );
			break;
		case 'x':
			sscanf( optarg, "%x", &rt_g.debug );
			fprintf(stderr,"librt rt_g.debug=x%x\n", rt_g.debug);
			break;
		case 'X':
			sscanf( optarg, "%x", &rdebug );
			fprintf(stderr,"rt rdebug=x%x\n", rdebug);
			break;

		case 's':
			/* Square size -- fall through */
		case 'f':
			/* "Fast" -- arg's worth of pixels */
			i = atoi( optarg );
			if( i < 2 || i > MAX_WIDTH )
				fprintf(stderr,"squaresize=%d out of range\n", i);
			else
				width = height = i;
			break;
		case 'n':
			i = atoi( optarg );
			if( i < 2 || i > MAX_WIDTH )
				fprintf(stderr,"height=%d out of range\n", i);
			else
				height = i;
			break;
		case 'w':
			i = atoi( optarg );
			if( i < 2 || i > MAX_WIDTH )
				fprintf(stderr,"width=%d out of range\n", i);
			else
				width = i;
			break;

		case 'a':
			/* Set azimuth */
			azimuth = atof( optarg );
			matflag = 0;
			break;
		case 'e':
			/* Set elevation */
			elevation = atof( optarg );
			matflag = 0;
			break;
		case 'l':
			/* Select lighting model # */
			lightmodel = atoi( optarg );
			break;
		case 'O':
			/* Output pixel file name, Hex format */
			outputfile = optarg;
			hex_out = 1;
			break;
		case 'o':
			/* Output pixel file name, binary format */
			outputfile = optarg;
			hex_out = 0;
			break;
		case 'p':
			perspective = 1;
			zoomout = atof( optarg );
			if( zoomout <= 0 )  zoomout = 1;
			break;
		case 'P':
			/* Number of parallel workers */
			npsw = atoi( optarg );
			if( npsw < 1 || npsw > MAX_PSW )  {
				fprintf(stderr,"npsw out of range 1..%d\n", MAX_PSW);
				npsw = 1;
			}
			break;
		case 'B':
			/*  Remove all intentional random effects
			 *  (dither, etc) for benchmarking.
			 */
			mathtab_constant();
			break;
		case 'b':
			/* Specify a single pixel to be done */
			{
				int xx, yy;
				(void)sscanf( optarg, "%d%d", &xx, &yy );
				fprintf(stderr,"only pixel %d %d\n", xx, yy);
				if( xx * yy >= 0 )  {
					pix_start = yy * width + xx;
					pix_end = pix_start;
				}
			}
			break;
		default:		/* '?' */
			fprintf(stderr,"unknown option %c\n", c);
			return(0);	/* BAD */
		}
	}
	return(1);			/* OK */
}

/*
 *			M A I N
 */
main(argc, argv)
int argc;
char **argv;
{
	static struct rt_i *rtip;
	static vect_t temp;
	char *title_file, *title_obj;	/* name of file and first object */
	register int x,y;
	char outbuf[132];
	char idbuf[132];		/* First ID record info */
	char cbuf[512];			/* Input command buffer */

#ifdef BSD
	setlinebuf( stderr );
#endif BSD
#if defined( SYSV ) && !defined( sgi ) && !defined(CRAY2)
	(void) setvbuf( stderr, (char *) NULL, _IOLBF, BUFSIZ );
#endif SYSV

	beginptr = sbrk(0);
	width = height = 512;
	azimuth = -35.0;			/* GIFT defaults */
	elevation = -25.0;

#ifdef cray
	npsw = 1;			/* >1 on GOS crashes system */
#endif cray

	if ( !get_args( argc, argv ) )  {
		(void)fputs(usage, stderr);
		exit(1);
	}
	if( optind >= argc )  {
		fprintf(stderr,"rt:  MGED database not specified\n");
		(void)fputs(usage, stderr);
		exit(1);
	}
	if( pix_start == -1 )  {
		pix_start = 0;
		pix_end = height * width - 1;
	}

	if( incr_mode )  {
		x = height;
		if( x < width )  x = width;
		incr_nlevel = 1;
		while( (1<<incr_nlevel) < x )
			incr_nlevel++;
		rt_log("incremental resolution, nlevels = %d\n", incr_nlevel);
	}

	RES_INIT( &rt_g.res_syscall );
	RES_INIT( &rt_g.res_worker );
	RES_INIT( &rt_g.res_stats );
	RES_INIT( &rt_g.res_results );

#ifndef PARALLEL
	npsw = 1;		/* serial */
#endif
	if( npsw > 1 )
		parallel = 1;
	if( parallel )
		fprintf(stderr,"rt:  running with %d processors\n", npsw );

	title_file = argv[optind];
	title_obj = argv[optind+1];
	nobjs = argc - optind - 1;
	objtab = &(argv[optind+1]);

	/* Build directory of GED database */
	if( (rtip=rt_dirbuild(title_file, idbuf, sizeof(idbuf))) == RTI_NULL ) {
		fprintf(stderr,"rt:  rt_dirbuild failure\n");
		exit(2);
	}
	ap.a_rt_i = rtip;
	fprintf(stderr, "db title:  %s\n", idbuf);

	/* 
	 *  Initialize application.
	 */
	if( view_init( &ap, title_file, title_obj, outputfile!=(char *)0 ) != 0 )  {
		/* Framebuffer is desired */
		register int xx, yy;
		xx = yy = 512;		/* SGI users may want 768 */
		while( xx < width )
			xx <<= 1;
		while( yy < width )
			yy <<= 1;
		if( (fbp = fb_open( framebuffer, xx, yy )) == FBIO_NULL )  {
			fprintf(stderr,"rt:  can't open frame buffer\n");
			exit(12);
		}
		/* ALERT:  The library wants zoom before window! */
		fb_zoom( fbp, fb_getwidth(fbp)/width, fb_getheight(fbp)/height );
		fb_window( fbp, width/2, height/2 );
	} else if( outputfile == (char *)0 )  {
		if( isatty(fileno(stdout)) )  {
			fprintf(stderr,"rt:  attempting to send binary output to terminal, aborting\n");
			exit(14);
		}
		outfp = stdout;
	}
	fprintf(stderr,"initial dynamic memory use=%d.\n",sbrk(0)-beginptr );
	beginptr = sbrk(0);

	if( !matflag )  {
		def_tree( rtip );		/* Load the default trees */
		do_ae( azimuth, elevation );
		(void)do_frame( curframe );
	} else if( old_way( stdin ) )  {
		; /* All is done */
	} else {
		/*
		 * New way - command driven.
		 * Process sequence of input commands.
		 * All the work happens in the functions
		 * called by do_cmd().
		 */
		while( read_cmd( stdin, cbuf, sizeof(cbuf) ) >= 0 )  {
			if( rdebug&RDEBUG_PARSE )
				fprintf(stderr,"cmd: %s\n", cbuf );
			if( do_cmd( cbuf ) < 0 )  break;
		}
		if( curframe < desiredframe )  {
			fprintf(stderr,
				"rt:  Desired frame %d not reached, last was %d\n",
				desiredframe, curframe);
		}
	}

	/* Release the framebuffer, if any */
	if( fbp != FBIO_NULL )
		fb_close(fbp);

	return(0);
}
