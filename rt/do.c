/*
 *			D O . C 
 *
 *  Routines that process the various commands, and manage
 *  the overall process of running the raytracing process.
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
 *	This software is Copyright (C) 1987 by the United States Army.
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
#include "./material.h"
#include "./mathtab.h"
#include "./rdebug.h"
#include "../librt/debug.h"

#ifdef HEP
# include <synch.h>
# undef stderr
# define stderr stdout
# define PARALLEL 1
#endif

extern double	atof();
extern char	*sbrk();

extern int	rdebug;			/* RT program debugging (not library) */

/***** Variables shared with viewing model *** */
extern FILE	*outfp;			/* optional pixel output file */
extern int	hex_out;		/* Binary or Hex .pix output file */
extern double	azimuth, elevation;
extern mat_t	view2model;
extern mat_t	model2view;
/***** end of sharing with viewing model *****/

extern void	grid_setup();
extern void	worker();

/***** variables shared with worker() ******/
extern struct application ap;
extern int	hypersample;		/* number of extra rays to fire */
extern point_t	eye_model;		/* model-space location of eye */
extern int	width;			/* # of pixels in X */
extern int	height;			/* # of lines in Y */
extern mat_t	Viewrotscale;
extern fastf_t	viewsize;
extern char	*scanbuf;		/* For optional output buffering */
extern int	incr_mode;		/* !0 for incremental resolution */
extern int	incr_level;		/* current incremental level */
extern int	incr_nlevel;		/* number of levels */
extern int	parallel;		/* Trying to use multi CPUs */
extern int	npsw;
extern struct resource resource[];
/***** end variables shared with worker() */

/***** variables shared with do.c *****/
extern int	pix_start;		/* pixel to start at */
extern int	pix_end;		/* pixel to end at */
extern int	nobjs;			/* Number of cmd-line treetops */
extern char	**objtab;		/* array of treetop strings */
extern char	*beginptr;		/* sbrk() at start of program */
extern int	matflag;		/* read matrix from stdin */
extern int	desiredframe;		/* frame to start at */
extern int	curframe;		/* current frame number */
extern char	*outputfile;		/* name of base of output file */
/***** end variables shared with do.c *****/

/*
 *			O L D _ W A Y
 * 
 *  Determine if input file is old or new format, and if
 *  old format, handle process *  Returns 0 if new way, 1 if old way (and all done).
 *  Note that the rewind() will fail on ttys, pipes, and sockets (sigh).
 */
int
old_way( fp )
FILE *fp;
{
	viewsize = -42.0;
	if( old_frame( fp ) < 0 || viewsize <= 0.0 )  {
		rewind( fp );
		return(0);		/* Not old way */
	}
	rt_log("Interpreting command stream in old format\n");

	def_tree( ap.a_rt_i );		/* Load the default trees */

	curframe = 0;
	do {
		do_frame( curframe++ );
	}  while( old_frame( fp ) >= 0 && viewsize > 0.0 );
	return(1);			/* old way, all done */
}

/*
 *			O L D _ F R A M E
 *
 *  Acquire particulars about a frame, in the old format.
 *  Returns -1 if unable to acquire info, 0 if successful.
 */
int
old_frame( fp )
FILE *fp;
{
	register int i;
	char number[128];

	/* Visible part is from -1 to +1 in view space */
	if( fscanf( fp, "%s", number ) != 1 )  return(-1);
	viewsize = atof(number);
	if( fscanf( fp, "%s", number ) != 1 )  return(-1);
	eye_model[X] = atof(number);
	if( fscanf( fp, "%s", number ) != 1 )  return(-1);
	eye_model[Y] = atof(number);
	if( fscanf( fp, "%s", number ) != 1 )  return(-1);
	eye_model[Z] = atof(number);
	for( i=0; i < 16; i++ )  {
		if( fscanf( fp, "%s", number ) != 1 )
			return(-1);
		Viewrotscale[i] = atof(number);
	}
	return(0);		/* OK */
}

/*
 *			C M _ S T A R T
 *
 *  Process "start" command in new format input stream
 */
cm_start( argc, argv )
int	argc;
char	**argv;
{
	char ibuf[512];
	int frame;

	frame = atoi(argv[1]);
	if( frame >= desiredframe )  {
		curframe = frame;
		return(0);
	}

	/* Skip over unwanted frames -- find next frame start */
	while( read_cmd( stdin, ibuf, sizeof(ibuf) ) >= 0 )  {
		register char *cp;

		cp = ibuf;
		while( *cp && isspace(*cp) )  cp++;	/* skip spaces */
		if( strncmp( cp, "start", 5 ) != 0 )  continue;
		while( *cp && !isspace(*cp) )  cp++;	/* skip keyword */
		while( *cp && isspace(*cp) )  cp++;	/* skip spaces */
		frame = atoi(cp);
		if( frame >= desiredframe )  {
			curframe = frame;
			return(0);
		}
	}
	return(-1);		/* EOF */
}

cm_vsize( argc, argv )
int	argc;
char	**argv;
{
	viewsize = atof( argv[1] );
	return(0);
}

cm_eyept( argc, argv )
int	argc;
char	**argv;
{
	register int i;

	for( i=0; i<3; i++ )
		eye_model[i] = atof( argv[i+1] );
	return(0);
}

cm_vrot( argc, argv )
int	argc;
char	**argv;
{
	register int i;

	for( i=0; i<16; i++ )
		Viewrotscale[i] = atof( argv[i+1] );
	return(0);
}

cm_end( argc, argv )
int	argc;
char	**argv;
{
	struct rt_i *rtip = ap.a_rt_i;

	if( rtip->HeadRegion == REGION_NULL )  {
		def_tree( rtip );		/* Load the default trees */
	}

	/* If no matrix or az/el specified yet, use params from cmd line */
	if( Viewrotscale[15] <= 0.0 )
		do_ae( azimuth, elevation );

	if( do_frame( curframe ) < 0 )  return(-1);
	return(0);
}

cm_tree( argc, argv )
int	argc;
char	**argv;
{
	register struct rt_i *rtip = ap.a_rt_i;
	char outbuf[132];
	register int i;

	if( argc <= 1 )  {
		def_tree( rtip );		/* Load the default trees */
		return(0);
	}
	rt_prep_timer();
	for( i=1; i < argc; i++ )  {
		if( rt_gettree(rtip, argv[i]) < 0 )
			fprintf(stderr,"rt_gettree(%s) FAILED\n", argv[i]);
	}
	(void)rt_read_timer( outbuf, sizeof(outbuf) );
	fprintf(stderr,"GETTREE: %s\n", outbuf);
	return(0);
}

cm_multiview( argc, argv )
int	argc;
char	**argv;
{
	register struct rt_i *rtip = ap.a_rt_i;
	static int a[] = {
		-35,
		  0,  90, 135, 180, 225, 270, 315,
		  0,  90, 135, 180, 225, 270, 315
	};
	static int e[] = {
		-25,
		-30, -30, -30, -30, -30, -30, -30,
		  0,   0,   0,   0,   0,   0,   0
	};

	if( rtip->HeadRegion == REGION_NULL )  {
		def_tree( rtip );		/* Load the default trees */
	}
	for( curframe=0; curframe<(sizeof(a)/sizeof(a[0])); curframe++ )  {
		do_ae( (double)a[curframe], (double)e[curframe] );
		(void)do_frame( curframe );
	}
	return(-1);	/* end RT by returning an error */
}

/*
 *			C M _ A N I M
 *
 *  Experimental animation code
 *
 *  Usage:  anim <path> <type> args
 */
cm_anim( argc, argv )
int	argc;
char	**argv;
{
	struct rt_i *rtip = ap.a_rt_i;
	struct animate *anp;
	struct directory **dir;
	int i;
	int	at_root = 0;

	if( argv[1][0] == '/' )
		at_root = 1;
	if( (i = rt_plookup( rtip, &dir, argv[1], LOOKUP_NOISY )) <= 0 )
		return(-1);		/* error */
	if( i > 1 )
		at_root = 0;

	GETSTRUCT( anp, animate );
	anp->an_path = dir;
	anp->an_pathlen = i;

	if( strcmp( argv[2], "matrix" ) == 0 )  {
		anp->an_type = AN_MATRIX;
		if( strcmp( argv[3], "rstack" ) == 0 )
			anp->an_u.anu_m.anm_op = ANM_RSTACK;
		else if( strcmp( argv[3], "rarc" ) == 0 )
			anp->an_u.anu_m.anm_op = ANM_RARC;
		else if( strcmp( argv[3], "lmul" ) == 0 )
			anp->an_u.anu_m.anm_op = ANM_LMUL;
		else if( strcmp( argv[3], "rmul" ) == 0 )
			anp->an_u.anu_m.anm_op = ANM_RMUL;
		else if( strcmp( argv[3], "rboth" ) == 0 )
			anp->an_u.anu_m.anm_op = ANM_RBOTH;
		else  {
			fprintf(stderr,"cm_anim:  Matrix op %s unknown\n",
				argv[3]);
			goto bad;
		}
		for( i=0; i<16; i++ )
			anp->an_u.anu_m.anm_mat[i] = atof( argv[i+4] );
	} else {
		fprintf(stderr,"cm_anim:  type %s unknown\n", argv[2]);
		goto bad;
	}
	if( rt_add_anim( rtip, anp, at_root ) < 0 )  {
		fprintf(stderr,"cm_anim:  %s %s failed\n", argv[1], argv[2]);
		goto bad;
	}
	return(0);
bad:
	rt_free( (char *)dir, "directory []");
	rt_free( (char *)anp, "animate");
	return(-1);		/* BAD */
}

/*
 *			C M _ C L E A N
 *
 *  Clean out results of last rt_prep(), and start anew.
 */
cm_clean( argc, argv )
int	argc;
char	**argv;
{
	register struct region *regp;

	/* The linkage here needs to be much better for things
	 * like rtrad, etc. XXXX
	 */
	for( regp=ap.a_rt_i->HeadRegion; regp != REGION_NULL; regp=regp->reg_forw )
		mlib_free( regp );
	rt_clean( ap.a_rt_i );
	if(rdebug&RDEBUG_RTMEM)
		rt_prmem( "After rt_clean" );
	return(0);
}

/*
 *			D E F _ T R E E
 *
 *  Load default tree list, from command line.
 */
def_tree( rtip )
register struct rt_i	*rtip;
{
	char outbuf[132];
	register int i;

	rt_prep_timer();
	for( i=0; i < nobjs; i++ )  {
		if( rt_gettree(rtip, objtab[i]) < 0 )
			fprintf(stderr,"rt_gettree(%s) FAILED\n", objtab[i]);
	}
	(void)rt_read_timer( outbuf, sizeof(outbuf) );
	fprintf(stderr,"GETTREE: %s\n", outbuf);
}

/*
 *			D O _ F R A M E
 *
 *  Do all the actual work to run a frame.
 *
 *  Returns -1 on error, 0 if OK.
 */
do_frame( framenumber )
int framenumber;
{
	char outbuf[132];
	char framename[128];		/* File name to hold current frame */
	struct rt_i *rtip = ap.a_rt_i;
	static double utime;
	register struct region *regp;

	fprintf(stderr, "\n...................Frame %5d...................\n",
		framenumber);
	set_priority( width*height*(hypersample+1) );
	if( rtip->needprep )  {
		/* Allow RT library to prepare itself */
		rt_prep_timer();
		rt_prep(rtip);

		/* Initialize the material library for all regions */
		for( regp=rtip->HeadRegion; regp != REGION_NULL; regp=regp->reg_forw )  {
			if( mlib_setup( regp ) < 0 )  {
				rt_log("mlib_setup failure on %s\n", regp->reg_name);
			} else {
				if(rdebug&RDEBUG_MATERIAL)
					((struct mfuncs *)(regp->reg_mfuncs))->
						mf_print( regp, regp->reg_udata );
				/* Perhaps this should be a function? */
			}
		}
		(void)rt_read_timer( outbuf, sizeof(outbuf) );
		fprintf(stderr, "PREP: %s\n", outbuf );
	}

	if( parallel && resource[0].re_seg == SEG_NULL )  {
		register int x;
		/* 
		 *  Get dynamic memory to keep from having to call
		 *  malloc(), because the portability of doing sbrk()
		 *  sys-calls when running in parallel mode is unknown.
		 */
		for( x=0; x<npsw; x++ )  {
			rt_get_seg(&resource[x]);
			rt_get_pt(rtip, &resource[x]);
			rt_get_bitv(rtip, &resource[x]);
		}
		fprintf(stderr,"Additional dynamic memory used=%d. bytes\n",
			sbrk(0)-beginptr );
		beginptr = sbrk(0);
	}

	fprintf(stderr,"shooting at %d solids in %d regions\n",
		rtip->nsolids, rtip->nregions );
	if( rtip->HeadSolid == SOLTAB_NULL )  {
		fprintf(stderr,"rt ERROR: No solids\n");
		exit(3);
	}
	fprintf(stderr,"model X(%g,%g), Y(%g,%g), Z(%g,%g)\n",
		rtip->mdl_min[X], rtip->mdl_max[X],
		rtip->mdl_min[Y], rtip->mdl_max[Y],
		rtip->mdl_min[Z], rtip->mdl_max[Z] );

	if(rdebug&RDEBUG_RTMEM)
		rt_g.debug |= DEBUG_MEM;	/* Just for the tracing */

	if( outputfile != (char *)0 )  {
#ifdef CRAY_COS
		/* Dots in COS file names make them permanant files. */
		sprintf( framename, "F%d", framenumber );
		if( (outfp = fopen( framename, "w" )) == NULL )  {
			perror( framename );
			if( matflag )  return(0);
			return(-1);
		}
		/* Dispose to shell script starts with "!" */
		if( framenumber <= 0 || outputfile[0] == '!' )  {
			sprintf( framename, outputfile );
		}  else  {
			sprintf( framename, "%s.%d", outputfile, framenumber );
		}
#else
		if( framenumber <= 0 )  {
			sprintf( framename, outputfile );
		}  else  {
			sprintf( framename, "%s.%d", outputfile, framenumber );
		}
		if( (outfp = fopen( framename, "w" )) == NULL )  {
			perror( framename );
			if( matflag )  return(0);	/* OK */
			return(-1);			/* Bad */
		}
#endif CRAY_COS
		fprintf(stderr,"Output file is '%s'\n", framename);
	}

	grid_setup();
	fprintf(stderr,"Beam radius=%g mm, divergance=%g mm/1mm\n",
		ap.a_rbeam, ap.a_diverge );

	/* initialize lighting */
	view_2init( &ap );

	rtip->nshots = 0;
	rtip->nmiss_model = 0;
	rtip->nmiss_tree = 0;
	rtip->nmiss_solid = 0;
	rtip->nmiss = 0;
	rtip->nhits = 0;
	rtip->rti_nrays = 0;

	fprintf(stderr,"\n");
	fflush(stdout);
	fflush(stderr);

	/*
	 *  Compute the image
	 *  It may prove desirable to do this in chunks
	 */
	rt_prep_timer();
	if( incr_mode )  {
		for( incr_level = 0; incr_level < incr_nlevel; incr_level++ )  {
			do_run( 0, (1<<incr_level)*(1<<incr_level)-1 );
			view_end( &ap );
		}
	} else {
		do_run( pix_start, pix_end );
	}
	utime = rt_read_timer( outbuf, sizeof(outbuf) );

	/*
	 *  End of application.  Done outside of timing section.
	 *  Typically, writes any remaining results out.
	 */
	view_end( &ap );

	if(rdebug&RDEBUG_RTMEM)
		rt_g.debug &= ~DEBUG_MEM;	/* Stop until next frame */

	/*
	 *  All done.  Display run statistics.
	 */
	fprintf(stderr,"SHOT: %s\n", outbuf );
	fprintf(stderr,"Additional dynamic memory used=%d. bytes\n",
		sbrk(0)-beginptr );
		beginptr = sbrk(0);
	fprintf(stderr,"%ld solid/ray intersections: %ld hits + %ld miss\n",
		rtip->nshots, rtip->nhits, rtip->nmiss );
	fprintf(stderr,"pruned %.1f%%:  %ld model RPP, %ld dups skipped, %ld solid RPP\n",
		rtip->nshots>0?((double)rtip->nhits*100.0)/rtip->nshots:100.0,
		rtip->nmiss_model, rtip->nmiss_tree, rtip->nmiss_solid );
	fprintf(stderr,
		"Frame %5d: %8d pixels in %10.2f sec = %10.2f pixels/sec\n",
		framenumber,
		width*height, utime, (double)(width*height)/utime );
	fprintf(stderr,
		"Frame %5d: %8d rays   in %10.2f sec = %10.2f rays/sec (RTFM)\n",
		framenumber,
		rtip->rti_nrays, utime, (double)(rtip->rti_nrays)/utime );

	if( outfp != NULL )  {
#ifdef CRAY_COS
		int status;
		char dn[16];
		char message[128];

		strncpy( dn, outfp->ldn, sizeof(outfp->ldn) );	/* COS name */
#endif CRAY_COS
		(void)fclose(outfp);
		outfp = NULL;
#ifdef CRAY_COS
		status = 0;
		if( hex_out )  {
			(void)DISPOSE( &status, "DN      ", dn,
				"TEXT    ", framename,
				"NOWAIT  " );
		} else {
			/* Binary out */
			(void)DISPOSE( &status, "DN      ", dn,
				"TEXT    ", framename,
				"NOWAIT  ",
				"DF      ", "BB      " );
		}
		sprintf(message,
			"%s Dispose,dn='%s',text='%s'.  stat=0%o",
			(status==0) ? "Good" : "---BAD---",
			dn, framename, status );
		fprintf(stderr, "%s\n", message);
		remark(message);	/* Send to log files */
#else
		/* Protect finished product */
		if( outputfile != (char *)0 )
			chmod( framename, 0444 );
#endif CRAY_COS
	}

#ifdef STAT_PARALLEL
	lock_pr();
	res_pr();
#endif STAT_PARALLEL

	fprintf(stderr,"\n");
	return(0);		/* OK */
}

/*
 *			D O _ A E
 *
 *  Compute the rotation specified by the azimuth and
 *  elevation parameters.  First, note that these are
 *  specified relative to the GIFT "front view", ie,
 *  model (X,Y,Z) is view (Z,X,Y):  looking down X axis.
 *  Then, a positive azimuth represents rotating the *model*
 *  around the Y axis, or, rotating the *eye* in -Y.
 *  A positive elevation represents rotating the *model*
 *  around the X axis, or, rotating the *eye* in -X.
 *  This is the "Gwyn compatable" azim/elev interpretation.
 *  Note that GIFT azim/elev values are the negatives of
 *  this interpretation.
 */
do_ae( azim, elev )
double azim, elev;
{
	vect_t	temp;
	vect_t	diag;
	mat_t	toEye;
	struct rt_i *rtip = ap.a_rt_i;

	if( rtip->mdl_max[X] >= INFINITY || rtip->mdl_max[X] <= -INFINITY )
		rt_bomb("do_ae called before rt_gettree");

	mat_idn( Viewrotscale );
	mat_angles( Viewrotscale, 270.0-elev, 0.0, 270.0+azim );
	fprintf(stderr,
		"Viewing %g azimuth, %g elevation off of front view\n",
		azim, elev);

	/* Look at the center of the model */
	mat_idn( toEye );
	toEye[MDX] = -(rtip->mdl_max[X]+rtip->mdl_min[X])/2;
	toEye[MDY] = -(rtip->mdl_max[Y]+rtip->mdl_min[Y])/2;
	toEye[MDZ] = -(rtip->mdl_max[Z]+rtip->mdl_min[Z])/2;

	/* Fit a sphere to the model RPP, diameter is viewsize,
	 * unless viewsize command used to override.
	 */
	VSUB2( diag, rtip->mdl_max, rtip->mdl_min );
	if( viewsize <= 0 )
		viewsize = MAGNITUDE( diag );
	fprintf(stderr,"view size = %g\n", viewsize);

	Viewrotscale[15] = 0.5*viewsize;	/* Viewscale */
	mat_mul( model2view, Viewrotscale, toEye );
	mat_inv( view2model, model2view );
	VSET( temp, 0, 0, 1.414 );
	MAT4X3PNT( eye_model, view2model, temp );
}
