/*
 *			R T . C 
 *
 *  Ray Tracing main program, using RT library.
 *  Invoked by MGED for quick pictures.
 *  Is linked with each of several "back ends":
 *	view.c, viewpp.c, viewray.c, viewcheck.c, etc
 *  to produce different executable programs:
 *	rt, rtpp, rtray, rtcheck, etc.
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
extern char	*sbrk();

extern char	usage[];

int		rdebug;			/* RT program debugging (not library) */

/***** Variables shared with viewing model *** */
FBIO		*fbp = FBIO_NULL;	/* Framebuffer handle */
FILE		*outfp = NULL;		/* optional pixel output file */
extern int	hex_out;		/* Binary or Hex .pix output file */
extern double	AmbientIntensity;	/* Ambient light intensity */
extern double	azimuth, elevation;
extern int	lightmodel;		/* Select lighting model */
mat_t		view2model;
mat_t		model2view;
extern int	use_air;		/* Handling of air in librt */
/***** end of sharing with viewing model *****/

/***** variables shared with worker() ******/
struct application ap;
vect_t		left_eye_delta;
extern int	width;			/* # of pixels in X */
extern int	height;			/* # of lines in Y */
extern int	incr_mode;		/* !0 for incremental resolution */
extern int	incr_nlevel;		/* number of levels */
extern int	npsw;			/* number of worker PSWs to run */
/***** end variables shared with worker() *****/

/***** variables shared with do.c *****/
extern int	pix_start;		/* pixel to start at */
extern int	pix_end;		/* pixel to end at */
extern int	nobjs;			/* Number of cmd-line treetops */
extern char	**objtab;		/* array of treetop strings */
char		*beginptr;		/* sbrk() at start of program */
extern int	matflag;		/* read matrix from stdin */
extern int	desiredframe;		/* frame to start at */
extern int	curframe;		/* current frame number */
extern char	*outputfile;		/* name of base of output file */
extern int	interactive;		/* human is watching results */
/***** end variables shared with do.c *****/

extern char	*framebuffer;		/* desired framebuffer */

extern int	cm_start();
extern int	cm_vsize();
extern int	cm_eyept();
extern int	cm_lookat_pt();
extern int	cm_vrot();
extern int	cm_end();
extern int	cm_multiview();
extern int	cm_anim();
extern int	cm_tree();
extern int	cm_clean();
extern int	cm_set();

static struct command_tab cmdtab[] = {
	"start", "frame number", "start a new frame",
		cm_start,	2, 2,
	"viewsize", "size in mm", "set view size",
		cm_vsize,	2, 2,
	"eye_pt", "xyz of eye", "set eye point",
		cm_eyept,	4, 4,
	"lookat_pt", "x y z [yflip]", "set eye look direction, in X-Y plane",
		cm_lookat_pt,	4, 5,
	"viewrot", "4x4 matrix", "set view direction from matrix",
		cm_vrot,	17,17,
	"end", 	"", "end of frame setup, begin raytrace",
		cm_end,		1, 1,
	"multiview", "", "produce stock set of views",
		cm_multiview,	1, 1,
	"anim", 	"path type args", "specify articulation animation",
		cm_anim,	4, 999,
	"tree", 	"treetop(s)", "specify alternate list of tree tops",
		cm_tree,	1, 999,
	"clean", "", "clean articulation from previous frame",
		cm_clean,	1, 1,
	"set", 	"", "show or set parameters",
		cm_set,		1, 999,
	(char *)0, (char *)0, (char *)0,
		0,		0, 0	/* END */
};

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

#ifdef BSD
	setlinebuf( stderr );
#else
#	if defined( SYSV ) && !defined( sgi ) && !defined(CRAY2)
		(void) setvbuf( stderr, (char *) NULL, _IOLBF, BUFSIZ );
#	endif
#endif

	beginptr = sbrk(0);
	width = height = 512;
	azimuth = 35.0;			/* GIFT defaults */
	elevation = 25.0;

	if ( !get_args( argc, argv ) )  {
		(void)fputs(usage, stderr);
		exit(1);
	}
	if( optind >= argc )  {
		fprintf(stderr,"rt:  MGED database not specified\n");
		(void)fputs(usage, stderr);
		exit(1);
	}

	if( incr_mode )  {
		x = height;
		if( x < width )  x = width;
		incr_nlevel = 1;
		while( (1<<incr_nlevel) < x )
			incr_nlevel++;
		height = width = 1<<incr_nlevel;
		fprintf(stderr, "incremental resolution, nlevels = %d, width=%d\n",
			incr_nlevel, width);
	}

	if( pix_start == -1 )  {
		pix_start = 0;
		pix_end = height * width - 1;
	}
	/*
	 *  Handle parallel initialization, if applicable.
	 */
#ifndef PARALLEL
	npsw = 1;			/* force serial */
#endif
	if( npsw > 1 )  {
		rt_g.rtg_parallel = 1;
		fprintf(stderr,"rt:  running with %d processors\n", npsw );
	} else
		rt_g.rtg_parallel = 0;
	RES_INIT( &rt_g.res_syscall );
	RES_INIT( &rt_g.res_worker );
	RES_INIT( &rt_g.res_stats );
	RES_INIT( &rt_g.res_results );
	/*
	 *  Do not use rt_log() or rt_malloc() before this point!
	 */

	if( rt_g.debug )  {
		rt_printb( "librt rt_g.debug", rt_g.debug, DEBUG_FORMAT );
		rt_log("\n");
	}
	if( rdebug )  {
		rt_printb( "rt rdebug", rdebug, RDEBUG_FORMAT );
		rt_log("\n");
	}

	title_file = argv[optind];
	title_obj = argv[optind+1];
	nobjs = argc - optind - 1;
	objtab = &(argv[optind+1]);

	if( nobjs <= 0 )  {
		fprintf(stderr,"rt: no objects specified\n");
		exit(1);
	}

	/* Build directory of GED database */
	if( (rtip=rt_dirbuild(title_file, idbuf, sizeof(idbuf))) == RTI_NULL ) {
		fprintf(stderr,"rt:  rt_dirbuild failure\n");
		exit(2);
	}
	ap.a_rt_i = rtip;
	fprintf(stderr, "db title:  %s\n", idbuf);
	rtip->useair = use_air;

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
	} else if( outputfile == (char *)0 || strcmp( outputfile, "-") == 0 )  {
		outputfile = (char *)0;
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
	} else if( !isatty(fileno(stdin)) && old_way( stdin ) )  {
		; /* All is done */
	} else {
		register char	*buf;
		register int	ret;
		/*
		 * New way - command driven.
		 * Process sequence of input commands.
		 * All the work happens in the functions
		 * called by rt_do_cmd().
		 */
		while( (buf = rt_read_cmd( stdin )) != (char *)0 )  {
			if( rdebug&RDEBUG_PARSE )
				fprintf(stderr,"cmd: %s\n", buf );
			ret = rt_do_cmd( rtip, buf, cmdtab );
			rt_free( buf, "cmd buf" );
			if( ret < 0 )
				break;
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
