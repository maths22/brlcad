/*
 *			R T
 *
 * Ray Tracing program
 *
 * Author -
 *	Michael John Muuss
 *
 *	U. S. Army Ballistic Research Laboratory
 *	March 27, 1984
 *
 * $Revision$
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include "vmath.h"
#include "ray.h"
#include "debug.h"

extern int null_prep(),	null_print();
extern int tor_prep(),	tor_print();
extern int tgc_prep(),	tgc_print();
extern int ellg_prep(),	ellg_print();
extern int arb8_prep(),	arb8_print();
extern int half_prep(),	half_print();
extern int ars_prep();
extern int rec_print();

extern struct seg *null_shot();
extern struct seg *tor_shot();
extern struct seg *tgc_shot();
extern struct seg *ellg_shot();
extern struct seg *arb8_shot();
extern struct seg *half_shot();
extern struct seg *rec_shot();

struct functab functab[] = {
	null_prep,	null_shot,	null_print,	"ID_NULL",
	tor_prep,	tor_shot,	tor_print,	"ID_TOR",
	tgc_prep,	tgc_shot,	tgc_print,	"ID_TGC",
	ellg_prep,	ellg_shot,	ellg_print,	"ID_ELL",
	arb8_prep,	arb8_shot,	arb8_print,	"ID_ARB8",
	ars_prep,	arb8_shot,	arb8_print,	"ID_ARS",
	half_prep,	half_shot,	half_print,	"ID_HALF",
	null_prep,	rec_shot,	rec_print,	"ID_REC",
	null_prep,	null_shot,	null_print,	">ID_NULL"
};

/*
 *  Hooks for unimplemented routines
 */

static char unimp[] = "Unimplemented Routine";

#define UNIMPLEMENTED(type) \
	DEF(type/**/_prep) \
	struct seg * DEF(type/**/_shot) \
	DEF(type/**/_print)

#define DEF(func)	func() { printf("func unimplemented\n"); }

UNIMPLEMENTED(null);
UNIMPLEMENTED(half);

double timer_print();

extern struct partition *bool_regions();

int debug = DEBUG_OFF;
int view_only;		/* non-zero if computation is for viewing only */
int lightmodel;		/* Select lighting model */

long nsolids;		/* total # of solids participating */
long nregions;		/* total # of regions participating */
long nshots;		/* # of ray-meets-solid "shots" */
long nmiss;		/* # of ray-misses-solid's-sphere "shots" */
int outfd;		/* fd of optional pixel output file */
FILE *outfp;		/* used to write .PP files */

struct soltab *HeadSolid = SOLTAB_NULL;

struct seg *FreeSeg = SEG_NULL;		/* Head of freelist */

struct seg *HeadSeg = SEG_NULL;

char usage[] = "\
Usage:  rt [options] model.vg object [objects]\n\
Options:  -f[#] -x# -aAz -eElev -A%Ambient -l# [-o model.pix]\n";

/* Used for autosizing */
static fastf_t xbase, ybase, zbase;
static fastf_t deltas;
extern double atof();

double AmbientIntensity = 0.1;		/* Ambient light intensity */
vect_t l0vec;		/* 0th light vector */
vect_t l1vec;		/* 1st light vector */
vect_t l2vec;		/* 2st light vector */

static char ttyObuf[4096];

main(argc, argv)
int argc;
char **argv;
{
	register struct ray *rayp;
	register int xscreen, yscreen;
	static int npts;		/* # of points to shoot: x,y */
	static mat_t view2model;
	static mat_t model2view;
	static mat_t mat1, mat2;	/* temporary matrices */
	static vect_t tempdir;
	static struct partition *PartHeadp, *pp;
	static fastf_t distsq;
	static double azimuth, elevation;
	static int matflag = 0;		/* read matrix from stdin */

	npts = 512;
	azimuth = -35.0;			/* GIFT defaults */
	elevation = -25.0;

	if( argc < 1 )  {
		printf(usage);
		exit(1);
	}

	argc--; argv++;
	while( argv[0][0] == '-' )  {
		switch( argv[0][1] )  {
		case 'M':
			matflag = 1;
			break;
		case 'A':
			AmbientIntensity = atof( &argv[0][2] );
			break;
		case 'x':
			sscanf( &argv[0][2], "%x", &debug );
			printf("debug=x%x\n", debug);
			break;
		case 'f':
			/* "Fast" -- just a few pixels.  Or, arg's worth */
			npts = atoi( &argv[0][2] );
			if( npts < 2 || npts > 1024 )  {
				npts = 50;
			}
			break;
		case 'a':
			/* Set azimuth */
			azimuth = atof( &argv[0][2] );
			matflag = 0;
			break;
		case 'e':
			/* Set elevation */
			elevation = atof( &argv[0][2] );
			matflag = 0;
			break;
		case 'l':
			/* Select lighting model # */
			lightmodel = atoi( &argv[0][2] );
			break;
		case 'o':
			/* Output pixel file name */
			if( (outfd = creat( argv[1], 0444 )) <= 0 )  {
				perror( argv[1] );
				exit(10);
			}
			argc--; argv++;
			break;
		default:
			printf("rt:  Option '%c' unknown\n", argv[0][1]);
			printf(usage);
			break;
		}
		argc--; argv++;
	}

	if( argc < 2 )  {
		printf(usage);
		exit(2);
	}

	if( lightmodel != 4 )
		view_only = 1;

	if( lightmodel == 3 || lightmodel == 4 )
		if( outfd > 0 )
			outfp = fdopen( outfd, "w" );
		else
			bomb("No output file specified");

	if( lightmodel == 3 )  {
		fprintf(outfp, "%s: %s (RT)\n", argv[0], argv[1] );
		fprintf(outfp, "%10d%10d", (int)azimuth, (int)elevation );
		fprintf(outfp, "%10d%10d\n", npts, npts );
	}

	/* 4.2 BSD stdio debugging assist */
	if( debug )
		setbuffer( stdout, ttyObuf, sizeof(ttyObuf) );

	timer_prep();		/* Start timing preparations */

	/* Build directory of GED database */
	dir_build( argv[0] );
	argc--; argv++;

	/* Load the desired portion of the model */
	while( argc > 0 )  {
		get_tree(argv[0]);
		argc--; argv++;
	}
	(void)timer_print("PREP");

	if( HeadSolid == 0 )  bomb("No solids");

	/* Set up the online display and/or the display file */
	if( !(debug&DEBUG_QUICKIE) )  {
		dev_setup(npts);
	}

	timer_prep();	/* start timing actual run */

	/*
	 * Determine the view
	 */
	GETSTRUCT(rayp, ray);

	VSET( tempdir, 0, 0, -1 );
	if( !matflag )  {
		/*
		 * Unrotated view is TOP.
		 * Rotation of 270,0,270 takes us to a front view.
		 * Standard GIFT view is -35 azimuth, -25 elevation off front.
		 */
		mat_idn( model2view );
		mat_angles( model2view, 270.0-elevation, 0.0, 270.0+azimuth );
		printf("Viewing %f azimuth, %f elevation off of front view\n",
			azimuth, elevation);
		mat_inv( view2model, model2view );

		if( !(debug&DEBUG_QUICKIE) )  {
			autosize( view2model, npts );
		} else {
			xbase = -3;
			ybase = -3;
			zbase = -10;
			deltas = 1;
			npts = 8;
			VSET( tempdir, 0, 0, 1 );
		}
	}  else  {
		static int i;

		/* Visible part is from -1 to +1 */
		for( i=0; i < 16; i++ )
			scanf( "%f", &model2view[i] );

		xbase = ybase = zbase = -1;
		deltas = 2.0 / ((double)npts);
		mat_inv( view2model, model2view );
	}

	MAT4X3VEC( rayp->r_dir, view2model, tempdir );
	VUNITIZE( rayp->r_dir );

	VSET( tempdir, 	xbase, ybase, zbase );
	MAT4X3PNT( rayp->r_pt, view2model, tempdir );

	printf("Ambient light at %f%%\n", AmbientIntensity * 100.0 );

	/* Determine the Light location(s) in model space, xlate to view */
	/* 0:  Blue, at left edge, 1/2 high */
	tempdir[0] = 2 * (xbase);
	tempdir[1] = (2/2) * (ybase);
	tempdir[2] = 2 * (zbase + npts*deltas);
	MAT4X3VEC( l0vec, view2model, tempdir );
	VUNITIZE(l0vec);

	/* 1: Red, at right edge, 1/2 high */
	tempdir[0] = 2 * (xbase + npts*deltas);
	tempdir[1] = (2/2) * (ybase);
	tempdir[2] = 2 * (zbase + npts*deltas);
	MAT4X3VEC( l1vec, view2model, tempdir );
	VUNITIZE(l1vec);

	/* 2:  Grey, behind, and overhead */
	tempdir[0] = 2 * (xbase + (npts/2)*deltas);
	tempdir[1] = 2 * (ybase + npts*deltas);
	tempdir[2] = 2 * (zbase + (npts/2)*deltas);
	MAT4X3VEC( l2vec, view2model, tempdir );
	VUNITIZE(l2vec);

	fflush(stdout);

	for( yscreen = npts-1; yscreen >= 0; yscreen--)  {
		for( xscreen = 0; xscreen < npts; xscreen++)  {
			VSET( tempdir,
				xbase + xscreen * deltas,
				ybase + (npts-yscreen-1) * deltas,
				zbase +  2*npts*deltas );
			MAT4X3PNT( rayp->r_pt, view2model, tempdir );

			shootray( rayp );
			/* Implicit return of HeadSeg chain */

			if( HeadSeg == SEG_NULL )  {
				wbackground( xscreen, yscreen );
				continue;
			}

			/*
			 *  All intersections of the ray with the model have
			 *  been computed.  Evaluate the boolean functions.
			 */
			PartHeadp = bool_regions( HeadSeg );
			if( PartHeadp->pt_forw == PartHeadp )  {
				wbackground( xscreen, yscreen );
			}  else  {
				/*
				 * Hand final partitioned intersection list
				 * to application.
				 */
				viewit( PartHeadp, rayp, xscreen, yscreen );
			}

			/*
			 * Processing of this ray is complete.
			 * Release resources.
			 *
			 * Free up Seg memory.
			 */
			while( HeadSeg != SEG_NULL )  {
				register struct seg *hsp;	/* XXX */

				hsp = HeadSeg->seg_next;
				FREE_SEG( HeadSeg );
				HeadSeg = hsp;
			}
			/* Free up partition list */
			for( pp = PartHeadp->pt_forw; pp != PartHeadp;  )  {
				register struct partition *newpp;
				newpp = pp;
				pp = pp->pt_forw;
				FREE_PART(newpp);
			}
			if( debug )  fflush(stdout);
		}
		/* End of scan line */
		dev_eol( yscreen );
	}

	dev_end();

	/*
	 *  All done.  Display run statistics.
	 */
	{
		FAST double utime;
		utime = timer_print("SHOT");
		printf("%d solids, %d regions\n",
			nsolids, nregions );
		printf("%d output rays in %f sec = %f rays/sec\n",
			npts*npts, utime, (double)(npts*npts/utime) );
		printf("%d solids shot at, %d shots pruned\n",
			nshots, nmiss );
		printf("%d total shots in %f sec = %f shots/sec\n",
			nshots+nmiss, utime, (double)((nshots+nmiss)/utime) );
	}
	return(0);
}

autosize( rot, npts )
matp_t rot;
int npts;
{
	register struct soltab *stp;
	static fastf_t xmin, xmax;
	static fastf_t ymin, ymax;
	static fastf_t zmin, zmax;
	static vect_t xlated;
	static mat_t invrot;		/* model2view */

	mat_inv( invrot, rot );		/* Inverse rotation matrix */

	/* init maxima and minima */
	xmax = ymax = zmax = -100000000.0;
	xmin = ymin = zmin =  100000000.0;

	for( stp=HeadSolid; stp != 0; stp=stp->st_forw ) {
		FAST fastf_t rad;

		rad = sqrt(stp->st_radsq);
		MAT4X3PNT( xlated, invrot, stp->st_center );
#define MIN(v,t) {FAST fastf_t rt; rt=(t); if(rt<v) v = rt;}
#define MAX(v,t) {FAST fastf_t rt; rt=(t); if(rt>v) v = rt;}
		MIN( xmin, xlated[0]-rad );
		MAX( xmax, xlated[0]+rad );
		MIN( ymin, xlated[1]-rad );
		MAX( ymax, xlated[1]+rad );
		MIN( zmin, xlated[2]-rad );
		MAX( zmax, xlated[2]+rad );
	}

	/* Provide a slight border */
	xmin -= xmin * 0.03;
	ymin -= ymin * 0.03;
	zmin -= zmin * 0.03;
	xmax *= 1.03;
	ymax *= 1.03;
	zmax *= 1.03;

	xbase = xmin;
	ybase = ymin;
	zbase = zmin;

	deltas = (xmax-xmin)/npts;
	MAX( deltas, (ymax-ymin)/npts );
	printf("X(%f,%f), Y(%f,%f), Z(%f,%f)\n",
		xmin, xmax, ymin, ymax, zmin, zmax );
	printf("Deltas=%f (units between rays)\n", deltas );
}

bomb(str)
char *str;
{
	fflush(stdout);
	fprintf(stderr,"\nrt: %s.  FATAL ERROR.\n", str);
	exit(12);
}

pr_seg(segp)
register struct seg *segp;
{
	printf("%.8x: SEG %s (%f,%f) bin=%d\n",
		segp,
		segp->seg_stp->st_name,
		segp->seg_in.hit_dist,
		segp->seg_out.hit_dist,
		segp->seg_stp->st_bin );
}
