/*
	Author:		Gary S. Moss
			U. S. Army Ballistic Research Laboratory
			Aberdeen Proving Ground
			Maryland 21005-5066
			(301)278-6651 or AV-298-6651
*/
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#ifndef DEBUG
#define NDEBUG
#define STATIC static
#else
#define STATIC
#endif

#include <assert.h>

#include <stdio.h>
#include <time.h>
#include <string.h>

#include <Sc/Sc.h>

#include "machine.h"
#include "vmath.h"
#include "raytrace.h"

#include "./vecmath.h"
#include "./ascii.h"
#include "./extern.h"


#define MAX_COLS	128

#define PHANTOM_ARMOR	111

STATIC fastf_t getNormThickness();

int
doMore( linesp )
int	*linesp;
	{	register int	ret = true;
	if( ! tty )
		return	true;
	TcSaveTty( HmTtyFd );
	TcSetRaw( HmTtyFd );
	TcClrEcho( HmTtyFd );
	ScSetStandout();
	prompt( "-- More -- " );
	ScClrStandout();
	(void) fflush( stdout );
	switch( HmGetchar() )
		{
	case 'q' :
	case 'n' :
		ret = false;
		break;
	case LF :
	case CR :
		*linesp = 1;
		break;
	default :
		*linesp = (PROMPT_Y-SCROLL_TOP);
		break;
		}
	TcResetTty( HmTtyFd );
	return	ret;
	}

STATIC int
f_Nerror( ap )
struct application	*ap;
	{
	rt_log( "Couldn't compute thickness or exit point%s.\n",
		"along normal direction" );
	V_Print( "\tpnt", ap->a_ray.r_pt, rt_log );
	V_Print( "\tdir", ap->a_ray.r_dir, rt_log );
	ap->a_rbeam = 0.0;
	VSET( ap->a_vvec, 0.0, 0.0, 0.0 );
	return	0;
	}

/*	f_Normal()

	Shooting from surface of object along reversed entry normal to
	compute exit point along normal direction and normal thickness.
	Exit point returned in "a_vvec", thickness returned in "a_rbeam".
 */
STATIC int
f_Normal( ap, pt_headp )
struct application	*ap;
struct partition	*pt_headp;
	{	register struct partition	*pp = pt_headp->pt_forw;
		register struct partition	*cp;
		register struct xray		*rp = &ap->a_ray;
		register struct hit		*ohitp;
	for(	cp = pp->pt_forw;
		cp != pt_headp && SameCmp( pp->pt_regionp, cp->pt_regionp );
		cp = cp->pt_forw
		)
		;
	ohitp = cp->pt_back->pt_outhit;
	ap->a_rbeam = ohitp->hit_dist - pp->pt_inhit->hit_dist;
	VJOIN1( ap->a_vvec, rp->r_pt, ohitp->hit_dist, rp->r_dir );
#ifdef DEBUG
	rt_log( "f_Normal: thickness=%g dout=%g din=%g\n",
		ap->a_rbeam*unitconv, ohitp->hit_dist, pp->pt_inhit->hit_dist );
#endif
	return	1;
	}

#include <errno.h>
/* These aren't defined in BSD errno.h.					*/
extern int	errno;
extern int	sys_nerr;
extern char	*sys_errlist[];

void
locPerror( msg )
char    *msg;
	{
	if( errno > 0 && errno < sys_nerr )
		rt_log( "%s: %s\n", msg, sys_errlist[errno] );
	else
		rt_log( "BUG: errno not set, shouldn't call perror.\n" );
	return;
	}

int
notify( str, mode )
char    *str;
int	mode;
	{       register int    i;
		static int      lastlen = -1;
		register int    len;
		static char	buf[LNBUFSZ] = { 0 };
		register char	*p;
	if( ! tty )
		return	false;
	switch( mode )
		{
	case NOTIFY_APPEND :
		p = buf + lastlen;
		break;
	case NOTIFY_DELETE :
		for( p = buf+lastlen; p > buf && *p != NOTIFY_DELIM; p-- )
			;
		break;
	case NOTIFY_ERASE :
		p = buf;
		break;
		}
	if( str != NULL )
		{
		if( p > buf )
			*p++ = NOTIFY_DELIM;
		(void) strcpy( p, str );
		}
	else
		*p = NUL;
	(void) ScMvCursor( PROMPT_X, PROMPT_Y );
	len = strlen( buf );
	if( len > 0 )
		{
		(void) ScSetStandout();
		(void) fputs( buf, stdout );
		(void) ScClrStandout();
		}

	/* Blank out remainder of previous command. */
	for( i = len; i < lastlen; i++ )
		(void) putchar( ' ' );
	(void) ScMvCursor( PROMPT_X, PROMPT_Y );
	(void) fflush( stdout );
	lastlen = len;
	return	true;
	}

/*
	void prntAspectInit( void )

	Burst Point Library and Shotline file: header record for each view.
	Ref. Figure 20., Line Number 1 and Figure 19., Line Number 1 of ICD.
 */
void
prntAspectInit()
	{
	if(	outfile[0] != NUL
	    &&	fprintf(outfp,
			"%c % 8.4f % 8.4f\n",
			PB_ASPECT_INIT,
			viewazim*DEGRAD, /* attack azimuth in degrees */
			viewelev*DEGRAD	 /* attack elevation in degrees */
			) < 0
		)
		{
		rt_log( "Write failed to file (%s)!\n", outfile );
		locPerror( "fprintf" );
		exitCleanly( 1 );
		}
	if(	shotlnfile[0] != NUL
	    &&	fprintf(shotlnfp,
			"%c % 8.4f % 8.4f % 7.2f % 7.2f %7.2f %7.2f %7.2f\n",
			PS_ASPECT_INIT,
			viewazim*DEGRAD, /* attack azimuth in degrees */
			viewelev*DEGRAD, /* attack elevation in degrees */
			cellsz*unitconv, /* shotline separation */
			gridrt*unitconv, /* maximum Y'-coordinate of target */
			gridlf*unitconv, /* minimum Y'-coordinate of target */
			gridup*unitconv, /* maximum Z'-coordinate of target */
			griddn*unitconv  /* minimum Z'-coordinate of target */
			) < 0
		)
		{
		rt_log( "Write failed to file (%s)!\n", shotlnfile );
		locPerror( "fprintf" );
		exitCleanly( 1 );
		}
	return;
	}

/*
	void prntCellIdent( struct application *ap, struct partition *pt_headp )

	Burst Point Library and Shotline file: information about shotline.
	Ref. Figure 20., Line Number 2 and Figure 19., Line Number 2 of ICD.
 */
void
prntCellIdent( ap, pt_headp )
register struct application	*ap;
struct partition		*pt_headp;
	{	fastf_t projarea;	/* projected area */
	/* Convert to user units before squaring cell size. */
	projarea = cellsz*unitconv;
	projarea *= projarea;

	if(	outfile[0] != NUL
	    &&	fprintf(outfp,
			"%c % 7.2f % 7.2f % 4.1f % 10.2f\n",
			PB_CELL_IDENT,
			ap->a_uvec[X]*unitconv,
			 	/* horizontal coordinate of shotline (Y') */
			ap->a_uvec[Y]*unitconv,
			 	/* vertical coordinate of shotline (Z') */
			bdist*unitconv, /* BDIST */
			/* projected area associated with burst point */
			projarea
			) < 0
		)
		{
		rt_log( "Write failed to file (%s)!\n", outfile );
		locPerror( "fprintf" );
		exitCleanly( 1 );
		}
	if(	shotlnfile[0] != NUL
	    &&	fprintf(shotlnfp,
			"%c % 8.2f % 8.2f\n",
			PS_CELL_IDENT,
			ap->a_uvec[X]*unitconv,
			 	/* horizontal coordinate of shotline (Y') */
			ap->a_uvec[Y]*unitconv
			 	/* vertical coordinate of shotline (Z') */
			) < 0
		)
		{
		rt_log( "Write failed to file (%s)!\n", shotlnfile );
		locPerror( "fprintf" );
		exitCleanly( 1 );
		}
	return;
	}

/*
	void prntSeg( struct application *ap, struct partition *cpp, int space )

	Burst Point Library and Shotline file: information about each component
	hit along path of main penetrator (shotline).
	Ref. Figure 20., Line Number 3 and Figure 19., Line Number 2 of ICD.
 */
void
prntSeg( ap, cpp, space )
register struct application *ap;
register struct partition *cpp;		/* component partition */
int space;
	{	fastf_t cosobliquity;	/* cosine of obliquity at entry */
		fastf_t	cosrotation;	/* cosine of rotation angle */
		fastf_t	entryangle;	/* obliquity angle at entry */
		fastf_t exitangle;	/* obliquity angle at exit */
		fastf_t los;		/* line-of-sight thickness */
		fastf_t normthickness;	/* normal thickness */
		fastf_t	rotangle;	/* rotation angle */
		fastf_t sinfbangle;	/* sine of fall back angle */
		register struct hit *ihitp;
		register struct hit *ohitp;
		register struct soltab *stp;

	/* fill in entry hit point and normal */
	stp = cpp->pt_inseg->seg_stp;
	ihitp = cpp->pt_inhit;
	RT_HIT_NORM( ihitp, stp, &(ap->a_ray) );
	
	/* check for flipped normal and fix */
	if( cpp->pt_inflip )
		{
		ScaleVec( ihitp->hit_normal, -1.0 );
		cpp->pt_inflip = 0;
		}
fixed_entry_normal:
	/* This *should* give negative of desired result, but make sure. */
	cosobliquity = Dot( ap->a_ray.r_dir, ihitp->hit_normal );
	if( cosobliquity < 0.0 )
		cosobliquity = -cosobliquity;
	else 
		{ 
#ifdef DEBUG 
		rt_log( "prntSeg: fixed flipped entry normal.\n" );
		rt_log( "cosine of angle of obliquity is %12.9f\n",
			cosobliquity );
		rt_log( "\tregion name '%s' solid name '%s'\n",
			cpp->pt_regionp->reg_name,
			stp->st_name );
#endif
		ScaleVec( ihitp->hit_normal, -1.0 );
		goto fixed_entry_normal;
		}
	cosrotation = Dot( ihitp->hit_normal, xaxis );
	sinfbangle = Dot( ihitp->hit_normal, zaxis );
	rotangle = AproxEq( cosrotation, 1.0, COS_TOL ) ?
			0.0 : acos( cosrotation ) * DEGRAD;
	los = (cpp->pt_outhit->hit_dist-ihitp->hit_dist)*unitconv;
#ifdef DEBUG
	rt_log( "prntSeg: los=%g dout=%g din=%g\n",
		los, cpp->pt_outhit->hit_dist, ihitp->hit_dist );
#endif

	if(	outfile[0] != NUL
	    &&	fprintf( outfp,
			"%c % 8.2f % 8.2f %4d % 7.3f % 7.3f % 7.3f\n",
			PB_RAY_INTERSECT,
			ihitp->hit_dist*unitconv,
					/* X'-coordinate of intersection */
			los,		/* LOS thickness of component */
			cpp->pt_regionp->reg_regionid,
					/* component code number */
			sinfbangle,	/* sine of fallback angle */
			rotangle,	/* rotation angle in degrees */
			cosobliquity	/* cosine of obliquity angle at entry */
			) < 0
		)
		{
		rt_log( "Write failed to file (%s)!\n", outfile );
		locPerror( "fprintf" );
		exitCleanly( 1 );
		}
	if( shotlnfile[0] == NUL )
		return;
	entryangle = AproxEq( cosobliquity, 1.0, COS_TOL ) ?
			0.0 : acos( cosobliquity ) * DEGRAD;
	if(	(normthickness =
		 getNormThickness( ap, cpp, cosobliquity )) <= 0.0
	    &&	fatalerror )
		{
		rt_log( "Couldn't compute normal thickness.\n" );
		rt_log( "\tshotline coordinates <%g,%g>\n",
			ap->a_uvec[X]*unitconv,
			ap->a_uvec[Y]*unitconv
			);
		rt_log( "\tregion name '%s' solid name '%s'\n",
			cpp->pt_regionp->reg_name,
			stp->st_name );
		return;
		}
	/* fill in exit hit point and normal */
	stp = cpp->pt_outseg->seg_stp;
	ohitp = cpp->pt_outhit;
	RT_HIT_NORM( ohitp, stp, &(ap->a_ray) );
	
	/* check for flipped normal and fix */
	if( cpp->pt_outflip )
		{
		ScaleVec( ohitp->hit_normal, -1.0 );
		cpp->pt_outflip = 0;
		}
fixed_exit_normal:
	/* This *should* give negative of desired result, but make sure. */
	cosobliquity = Dot( ap->a_ray.r_dir, ohitp->hit_normal );
	if( cosobliquity < 0.0 )
		cosobliquity = -cosobliquity;
	else 
		{ 
#ifdef DEBUG 
		rt_log( "prntSeg: fixed flipped exit normal.\n" );
		rt_log( "cosine of angle of obliquity is %12.9f\n",
			cosobliquity );
		rt_log( "\tregion name '%s' solid name '%s'\n",
			cpp->pt_regionp->reg_name,
			stp->st_name );
#endif
		ScaleVec( ohitp->hit_normal, -1.0 );
		goto fixed_exit_normal;
		}
	exitangle = AproxEq( cosobliquity, 1.0, COS_TOL ) ?
			0.0 : acos( cosobliquity ) * DEGRAD;
	if( fprintf( shotlnfp,
	       "%c % 8.2f % 7.3f % 7.3f %4d % 8.2f % 8.2f %2d % 7.2f % 7.2f\n",
			PS_SHOT_INTERSECT,
			ihitp->hit_dist*unitconv,
					/* X'-coordinate of intersection */
			sinfbangle,	/* sine of fallback angle */
			rotangle,	/* rotation angle in degrees */
			cpp->pt_regionp->reg_regionid,
					/* component code number */
			normthickness*unitconv,
					/* normal thickness of component */
			los,		/* LOS thickness of component */
			space,		/* space code */
			entryangle,	/* entry obliquity angle in degrees */
			exitangle	/* exit obliquity angle in degrees */
			) < 0
		)
		{
		rt_log( "Write failed to file (%s)!\n", shotlnfile );
		locPerror( "fprintf" );
		exitCleanly( 1 );
		}
	return;
	}

/*
	void prntRayHeader( fastf_t *raydir, fastf_t *shotdir, unsigned rayno )

	Burst Point Library: information about burst ray.  All angles are
	WRT the shotline coordinate system, represented by X', Y' and Z'.
	Ref. Figure 20., Line Number 19 of ICD.
 */
void
prntRayHeader( raydir, shotdir, rayno )
fastf_t	*raydir;	/* burst ray direction vector */
fastf_t *shotdir;	/* shotline direction vector */
unsigned rayno;		/* ray number for this burst point */
	{	double cosxr;	 /* cosine of angle between X' and raydir */
		double cosyr;	 /* cosine of angle between Y' and raydir */
		fastf_t azim;	 /* ray azim in radians */
		fastf_t sinelev; /* sine of ray elevation */
	if( outfile[0] == NUL )
		return;
	cosxr = -Dot( shotdir, raydir ); /* shotdir is reverse of X' */
	cosyr = Dot( gridhor, raydir );
	azim = atan2( cosyr, cosxr );
	sinelev = Dot( gridver, raydir );
	if(	fprintf( outfp,
			"%c % 6.3f % 6.3f % 6u\n",
			PB_RAY_HEADER,
			azim,   /* ray azimuth angle WRT shotline (radians). */
			sinelev, /* sine of ray elevation angle WRT shotline. */
			rayno
			) < 0
		)
		{
		rt_log( "Write failed to file (%s)!\n", outfile );
		locPerror( "fprintf" );
		exitCleanly( 1 );
		}
	return;
	}

/*
	void prntRegionHdr( struct application *ap, struct partition *pt_headp,
				struct partition *pp )

	Burst Point Libary: intersection along burst ray.
	Ref. Figure 20., Line Number 20 of ICD.
 */
void
prntRegionHdr( ap, pt_headp, pp )
struct application *ap;
struct partition *pt_headp;
struct partition *pp;
	{	fastf_t	cosobliquity;
		fastf_t normthickness;
		register struct hit *ihitp = pp->pt_inhit;
		register struct hit *ohitp = pp->pt_outhit;
		register struct region *regp = pp->pt_regionp;
		register struct soltab	*stp = pp->pt_inseg->seg_stp;
		register struct xray *rayp = &ap->a_ray;
	/* fill in entry/exit normals and hit points */
	RT_HIT_NORM( ihitp, stp, rayp );
	Check_Iflip( pp, ihitp->hit_normal, rayp->r_dir );
	stp = pp->pt_outseg->seg_stp;
	RT_HIT_NORM( ohitp, stp, rayp );
	Check_Oflip( pp, ohitp->hit_normal, rayp->r_dir );

	/* calculate cosine of obliquity angle */
fixed_normal:
	cosobliquity = Dot( ap->a_ray.r_dir, ihitp->hit_normal );
	if( cosobliquity < 0.0 )
		cosobliquity = -cosobliquity;
	else
		{
#if DEBUG
		rt_log( "prntRegionHdr: fixed flipped entry normal.\n" );
		rt_log( "region name '%s'\n", regp->reg_name );
#endif
		ScaleVec( ihitp->hit_normal, -1.0 );
		goto    fixed_normal;
		}
#if DEBUG
	if( cosobliquity - COS_TOL > 1.0 )
		{
		rt_log( "cosobliquity=%12.8f\n", cosobliquity );
		rt_log( "normal=<%g,%g,%g>\n",
			ihitp->hit_normal[X],
			ihitp->hit_normal[Y],
			ihitp->hit_normal[Z]
			);
		rt_log( "ray direction=<%g,%g,%g>\n",
			ap->a_ray.r_dir[X],
			ap->a_ray.r_dir[Y],
			ap->a_ray.r_dir[Z]
			);
		rt_log( "region name '%s'\n", regp->reg_name );
		}
#endif
	if( outfile[0] == NUL )
		return;

	
	/* Now we must find normal thickness through component. */
	normthickness = getNormThickness( ap, pp, cosobliquity );
 	RES_ACQUIRE( &rt_g.res_syscall );		/* lock */
	if(	fprintf( outfp,
			"%c % 10.3f % 9.3f % 9.3f %4d %4d % 6.3f\n",
			PB_REGION_HEADER,
			ihitp->hit_dist*unitconv, /* distance from burst pt. */
			(ohitp->hit_dist - ihitp->hit_dist)*unitconv, /* LOS */
			normthickness*unitconv,	  /* normal thickness */
			pp->pt_forw == pt_headp ?
				EXIT_AIR : pp->pt_forw->pt_regionp->reg_aircode,
			regp->reg_regionid,
			cosobliquity
			) < 0
		)
		{
		RES_RELEASE( &rt_g.res_syscall );	/* unlock */
		rt_log( "Write failed to file (%s)!\n", outfile );
		locPerror( "fprintf" );
		exitCleanly( 1 );
		}
	RES_RELEASE( &rt_g.res_syscall );	/* unlock */
	return;
	}

/*
	fastf_t getNormThickness( struct application *ap, struct partition *pp,
					fast_t cosobliquity )

	Given a partition structure with entry hit point and normal filled in,
	the current application structure and the cosine of the obliquity at
	entry to the component, return the normal thickness through the
	component at the given hit point.

 */
STATIC fastf_t
getNormThickness( ap, pp, cosobliquity )
register struct application *ap;
register struct partition *pp;
fastf_t cosobliquity;
	{	struct application a_thick;
		register struct hit *ihitp = pp->pt_inhit;
		register struct region *regp = pp->pt_regionp;
	a_thick = *ap;
	a_thick.a_hit = f_Normal;
	a_thick.a_miss = f_Nerror;
	a_thick.a_level++;     
	a_thick.a_uptr = regp->reg_name;
	a_thick.a_user = regp->reg_regionid;
	CopyVec( a_thick.a_ray.r_pt, ihitp->hit_point );
	if( AproxEq( cosobliquity, 1.0, COS_TOL ) )
		{ /* Trajectory was normal to surface, so no need
			to shoot another ray.  We will use the
			f_Normal routine to make sure we are
			consistant in our calculations, even
			though it requires some unnecessary vector
			math. */
		CopyVec( a_thick.a_ray.r_dir, ap->a_ray.r_dir );
#ifdef DEBUG
		rt_log( "getNormThickness: using existing partitions.\n" );
#endif
		(void) f_Normal( &a_thick, pp->pt_back );
		} 
	else     
		{ /* need to shoot ray */
#ifdef DEBUG
		rt_log( "getNormThickness: ray tracing.\n" );
#endif
		Scale2Vec( ihitp->hit_normal, -1.0, a_thick.a_ray.r_dir );
		if( rt_shootray( &a_thick ) == -1 && fatalerror )
			{ /* Fatal error in application routine. */
			rt_log( "Fatal error: raytracing aborted.\n" );
			return	0.0;
			}
		}
	return	a_thick.a_rbeam;
	}

/*
	void prntShieldComp( struct application *ap, struct partition *pt_headp,
				Pt_Queue *qp )
 */
void
prntShieldComp( ap, pt_headp, qp )
struct application *ap;
struct partition *pt_headp;
register Pt_Queue *qp;
	{
	if( outfile[0] == NUL )
		return;
	if( qp == PT_Q_NULL )
		return;
	prntShieldComp( ap, pt_headp, qp->q_next );
	prntRegionHdr( ap, pt_headp, qp->q_part );
	return;
	}
void
prntColors( colorp, str )
register Colors	*colorp;
char	*str;
	{
	rt_log( "%s:\n", str );
	for(	colorp = colorp->c_next;
		colorp != COLORS_NULL;
		colorp = colorp->c_next )
		{
		rt_log( "\t%d..%d\t%d,%d,%d\n",
			(int)colorp->c_lower,
			(int)colorp->c_upper,
			(int)colorp->c_rgb[0],
			(int)colorp->c_rgb[1],
			(int)colorp->c_rgb[2]
			);
		}
	return;
	}

/*
	void prntFiringCoords( register fastf_t *vec )

	If the user has asked for grid coordinates to be saved, write
	them to the output stream 'gridfp'.
 */
void
prntFiringCoords( vec )
register fastf_t *vec;
	{
	if( gridfile[0] == '\0' )
		return;
	assert( gridfp != (FILE *) NULL );
	if( fprintf( gridfp, "%7.2f %7.2f\n", vec[X]*unitconv, vec[Y]*unitconv )
		< 0 )
		{
		rt_log( "Write failed to file (%s)!\n", gridfile );
		locPerror( "fprintf" );
		exitCleanly( 1 );
		}
	return;
	}

void
prntGridOffsets( x, y )
int	x, y;
	{
	if( ! tty )
		return;
	(void) ScMvCursor( GRID_X, GRID_Y );
	(void) printf( "[% 4d:% 4d,% 4d:% 4d]",
			x, gridxfin, y, gridyfin
			);
	(void) fflush( stdout );
	return;
	}

void
prntIdents( idp, str )
register Ids	*idp;
char	*str;
	{
	rt_log( "%s:\n", str );
	for( idp = idp->i_next ; idp != IDS_NULL; idp = idp->i_next )
		{
		if( idp->i_lower == idp->i_upper )
			rt_log( "\t%d\n", (int) idp->i_lower );
		else
			rt_log( "\t%d..%d\n",
				(int)idp->i_lower,
				(int)idp->i_upper
				);
		}
	return;
	}

/**/
void
prntPagedMenu( menu )
register char	**menu;
	{	register int	done = false;
		int		lines =	(PROMPT_Y-SCROLL_TOP);
	if( ! tty )
		{
		for( ; *menu != NULL; menu++ )
			rt_log( "%s\n", *menu );
		return;
		}
	for( ; *menu != NULL && ! done;  )
		{
		for( ; lines > 0 && *menu != NULL; menu++, --lines )
			rt_log( "%-*s\n", co, *menu );
		if( *menu != NULL )
			done = ! doMore( &lines );
		prompt( "" );
		}
	(void) fflush( stdout );
	return;
	}

/*
	void prntPhantom( struct hit *hitp, int space, fastf_t los )

	Output "phantom armor" pseudo component.  This component has no
	surface normal or thickness, so many zero fields are used for
	conformity with the normal component output formats.
 */
/*ARGSUSED*/
void
prntPhantom( hitp, space, los )
struct hit *hitp;	/* ptr. to phantom's intersection information */
int space;		/* space code behind phantom */
fastf_t	los;		/* LOS of space */
	{
	if(	outfile[0] != NUL
	    &&	fprintf( outfp,
			"%c % 8.2f % 8.2f %4d % 7.3f % 7.3f % 7.3f\n",
			PB_RAY_INTERSECT,
			hitp->hit_dist*unitconv,
				/* X' coordinate of intersection */
			0.0,	/* LOS thickness of component */
			PHANTOM_ARMOR, /* component code number */
			0.0,	/* sine of fallback angle */
			0.0,	/* rotation angle (degrees) */
			0.0 /* cosine of obliquity angle at entry */
			) < 0
		)
		{
		rt_log( "Write failed to file!\n", outfile );
		locPerror( "fprintf" );
		exitCleanly( 1 );
		}
	if(	shotlnfile[0] != NUL
	    &&	fprintf( shotlnfp,
	       "%c % 8.2f % 7.3f % 7.3f %4d % 8.2f % 8.2f %2d % 7.2f % 7.2f\n",
			PS_SHOT_INTERSECT,
			hitp->hit_dist*unitconv,
					/* X'-coordinate of intersection */
			0.0,		/* sine of fallback angle */
			0.0,		/* rotation angle in degrees */
			PHANTOM_ARMOR,	/* component code number */
			0.0,		/* normal thickness of component */
			0.0,		/* LOS thickness of component */
			space,		/* space code */
			0.0,		/* entry obliquity angle in degrees */
			0.0		/* exit obliquity angle in degrees */
			) < 0
		)
		{
		rt_log( "Write failed to file (%s)!\n", shotlnfile );
		locPerror( "fprintf" );
		exitCleanly( 1 );
		}
	return;
	}

#include <varargs.h>
/* VARARGS */
void
prntScr( fmt, va_alist )
char	*fmt;
va_dcl
	{	va_list		ap;
	va_start( ap );
	if( tty )
		{
		TcClrTabs( HmTtyFd );
		if( ScSetScrlReg( SCROLL_TOP, SCROLL_BTM ) )
			{
			(void) ScMvCursor( 1, SCROLL_BTM );
			(void) ScClrEOL();
			(void) _doprnt( fmt, ap, stdout );
			(void) ScClrScrlReg();
			}
		else
		if( ScDL != NULL )
			{
			(void) ScMvCursor( 1, SCROLL_TOP );
			(void) ScDeleteLn();
			(void) ScMvCursor( 1, SCROLL_BTM );
			(void) ScClrEOL();
			(void) _doprnt( fmt, ap, stdout );
			}
		else
			{
			(void) _doprnt( fmt, ap, stdout );
			(void) fputs( "\n", stdout );
			}
		(void) fflush( stdout );
		}
	else
		{
		(void) _doprnt( fmt, ap, stderr );
		(void) fputs( "\n", stderr );
		}
	va_end( ap );
	return;
	}



/*
	void	prntTimer( char *str )
 */
void
prntTimer( str )
char    *str;
	{
	(void) rt_read_timer( timer, TIMER_LEN-1 );
	if( tty )
		{
		(void) ScMvCursor( TIMER_X, TIMER_Y );
		if( str == NULL )
			(void) printf( "%s", timer );
		else
			(void) printf( "%s:\t%s", str, timer );
		(void) ScClrEOL();
		(void) fflush( stdout );
		}
	else
		rt_log( "%s:\t%s\n", str == NULL ? "(null)" : str, timer );
	return;
	}

void
prntTitle( title )
char	*title;
	{
	if( ! tty || rt_g.debug )
		rt_log( "%s\n", title == NULL ? "(null)" : title );
	return;
	}

static char	*usage[] =
	{
	"Usage: burst [-b]",
	"\tThe -b option suppresses the screen display (for batch jobs).",
	NULL
	};
void
prntUsage()
	{	register char   **p = usage;
	while( *p != NULL )
		(void) fprintf( stderr, "%s\n", *p++ );
	return;
	}

void
prompt( str )
char    *str;
	{
	(void) ScMvCursor( PROMPT_X, PROMPT_Y );
	if( str == (char *) NULL )
		(void) ScClrEOL();
	else
		{
		(void) ScSetStandout();
		(void) fputs( str, stdout );
		(void) ScClrStandout();
		}
	(void) fflush( stdout );
	return;
	}

int
qAdd( pp, qpp )
struct partition	*pp;
Pt_Queue		**qpp;
	{	Pt_Queue	*newq;
	RES_ACQUIRE( &rt_g.res_syscall );
	if( (newq = (Pt_Queue *) malloc( sizeof(Pt_Queue) )) == PT_Q_NULL )
		{
		Malloc_Bomb( sizeof(Pt_Queue) );
		RES_RELEASE( &rt_g.res_syscall );
		return	0;
		}
	RES_RELEASE( &rt_g.res_syscall );
	newq->q_next = *qpp;
	newq->q_part = pp;
	*qpp = newq;
	return	1;
	}

void
qFree( qp )
Pt_Queue	*qp;
	{
	if( qp == PT_Q_NULL )
		return;
	qFree( qp->q_next );
	RES_ACQUIRE( &rt_g.res_syscall );
	free( (char *) qp );
	RES_RELEASE( &rt_g.res_syscall );
	return;
	}

void
warning( str )
char	*str;
	{
	if( tty )
		HmError( str );
	else
		prntScr( str );
	return;
	}
