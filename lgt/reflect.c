/*
	Author:		Gary S. Moss
			U. S. Army Ballistic Research Laboratory
			Aberdeen Proving Ground
			Maryland 21005-5066
			(301)278-6647 or AV-298-6647

	Thanks to Edwin O. Davisson and Robert Shnidman for contributions
	to the refraction algorithm.
*/
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <math.h>
#ifdef BSD
#include <strings.h>
#else
#include <string.h>
#endif
#include "./extern.h"
#include "./vecmath.h"
#include "./mat_db.h"
#include "./tree.h"
#include "./screen.h"
#define TWO_PI		6.28318530717958647692528676655900576839433879875022
#define RI_AIR		1.0  /* refractive index of air */

/* These are used by the hidden-line drawing routines. */
#define COSTOL		0.91
#define is_Odd(_a)	((_a)&01)
#define ARCTAN_87	19.08


#ifndef FLIPPED_NORMALS_BUG
#define FLIPPED_NORMALS_BUG	FALSE /* Keep an eye out for dark spots. */
#endif
#define Fix_Iflip( _pp, _normal, _rdir, _stp )\
	{\
	if( _pp->pt_inflip )\
		{\
		VREVERSE( _normal, _normal );\
		_pp->pt_inflip = 0;\
		}\
	Check_Iflip( _pp, _normal, _rdir, _stp );\
	}

#if FLIPPED_NORMALS_BUG
#define Check_Iflip( _pp, _normal, _rdir, _stp )\
	{ fastf_t	f = Dot( _rdir, _normal );\
	if( f >= 0.0 )\
		{\
		if( ! _pp->pt_inflip && (rt_g.debug&DEBUG_NORML) )\
			{\
			V_Print( "Fixed flipped entry normal, was",\
				_normal, rt_log );\
			rt_log( "Solid type %d\n", _stp->st_id );\
			}\
		VREVERSE( _normal, _normal );\
		}\
	}
#else
#define Check_Iflip( _pp, _normal, _rdir, _stp )	;
#endif

#define Fix_Oflip( _pp, _normal, _rdir, _stp )\
	{\
	if( _pp->pt_outflip )\
		{\
		VREVERSE( _normal, _normal );\
		_pp->pt_outflip = 0;\
		}\
	Check_Oflip( _pp, _normal, _rdir, _stp );\
	}

#if FLIPPED_NORMALS_BUG
#define Check_Oflip( _pp, _normal, _rdir, _stp )\
	{	fastf_t	f = Dot( _rdir, _normal );\
	if( f <= 0.0 )\
		{\
		if( ! _pp->pt_outflip && (rt_g.debug&DEBUG_NORML) )\
			{\
			V_Print( "Fixed flipped exit normal, was",\
				_normal, rt_log );\
			rt_log( "Solid type %d\n", _stp->st_id );\
			}\
		VREVERSE( _normal, _normal );\
		}\
	}
#else
#define Check_Oflip( _pp, _normal, _rdir, _stp )	;
#endif

/* default material property entry */
static Mat_Db_Entry	mat_tmp_entry =
				{
				0,		/* Material id. */
				4,		/* Shininess. */
				0.6,		/* Specular weight. */
				0.4,		/* Diffuse weight. */
				0.0,		/* Reflectivity. */
				0.0,		/* Transmission. */
				1.0,		/* Refractive index. */
				255, 255, 255,	/* Diffuse RGB values. */
				MF_USED,	/* Mode flag. */
				"(default)"	/* Material name. */
				};

/* Collect statistics on refraction. */
static int		refrac_missed;
static int		refrac_inside;
static int		refrac_total;

/* Collect statistics on shadowing. */
static int		hits_shadowed;
static int		hits_lit;

/* Local communication with render_Scan(). */
static int curr_scan;	  /* current scan line number */
static int last_scan;	  /* last scan */
static int nworkers;	  /* number of workers now running */
static int a_gridsz;	  /* grid size taking anti-aliasing into account */
static fastf_t	a_cellsz; /* cell size taking anti-aliasing into account */
static fastf_t	grid_dh[3], grid_dv[3];
static struct application ag;	/* global application structure */

/* Bit map for hidden line drawing. */
#ifndef BITSPERBYTE
#define BITSPERBYTE	8
#endif
#define HL_BITVBITS		(sizeof(bitv_t)*BITSPERBYTE)
#define HL_BITVMASK(_x)	((_x) == 0 ? 1 : 1L<<(_x)%HL_BITVBITS)
#define HL_BITVWORD(_x,_y)	hl_bits[_y][(_x)/HL_BITVBITS]
#define HL_SETBIT(_x,_y)	HL_BITVWORD(_x,_y) |= HL_BITVMASK(_x)
#define HL_CLRBIT(_x,_y)	HL_BITVWORD(_x,_y) &= ~HL_BITVMASK(_x)
#define HL_TSTBIT(_x,_y)	(HL_BITVWORD(_x,_y) & HL_BITVMASK(_x))
#define ZeroPixel(_p)		((_p)[RED]==0 && (_p)[GRN]==0 && (_p)[BLU]==0)
static bitv_t	hl_bits[1024][1024/HL_BITVBITS];
static short		*hl_regmap = NULL;
static unsigned short	*hl_dstmap = NULL;

/* Is object behind me.  Using values smaller than .01 cause bogus shadows
	on single-precision math library machines like the IRIS 3030.  This
	is because in shooting from a surface to the light source, we
	actually nick the surface because of floating point inaccuracies
	in the root finder.  BEHIND_ME_TOL must be larger than the distance
	to the exit from these thin segments.
 */
#define BEHIND_ME_TOL	0.01
#define PT_EMPTY	0
#define PT_OHIT		1
#define PT_BEHIND	2

#define Get_Partition( ap, pp, pt_headp, func )\
	{	int	failure;\
	for(	pp = pt_headp->pt_forw;\
		(failure=PT_EMPTY, pp != pt_headp)\
	    &&	(failure=PT_OHIT, pp->pt_outhit != (struct hit *) NULL)\
	    &&	(failure=PT_BEHIND, pp->pt_outhit->hit_dist < BEHIND_ME_TOL);\
		pp = pp->pt_forw\
		)\
		{ struct partition *pt_back = pp->pt_back;\
		DEQUEUE_PT( pp );\
		FREE_PT( pp, ap->a_resource );\
		pp = pt_back;\
		}\
	switch( failure )\
		{\
	case PT_EMPTY :\
		return	ap->a_miss( ap );\
	case PT_OHIT :\
		rt_log( "BUG:%s: Bad partition returned by rt_shootray!\n",\
			func );\
		rt_log( "\tlevel=%d grid=<%d,%d>\n",\
			ap->a_level, ap->a_x, ap->a_y );\
		return	ap->a_miss( ap );\
	case PT_BEHIND :\
		break;\
		}\
	}

STATIC fastf_t		myIpow();
STATIC fastf_t		correct_Lgt();
STATIC fastf_t		*mirror_Reflect();

/* "Hit" application routines to pass to "rt_shootray()".		*/
STATIC int		f_Model(), f_Probe(), f_Shadow(), f_HL_Hit(), f_Region();
/* "Miss" application routines to pass to "rt_shootray()".		*/
STATIC int		f_Backgr(), f_Error(), f_Lit(), f_HL_Miss(), f_R_Miss();
/* "Overlap" application routines to pass to "rt_shootray()".		*/
STATIC int		f_Overlap(), f_NulOverlap();

STATIC int		refract();

STATIC void		model_Reflectance();
STATIC void		glass_Refract();
STATIC void		view_pix(), view_bol(), view_eol(), view_end();

void			cons_Vector();
void			render_Model();
void			render_Scan();

#if defined( cray ) && 0
#define BYTE_OFFSET(p)	(((long)(p)&0xE000000000000000)>>61)
#define WORD_ADDR(p)	((long)(p)&0xFFFFFF)
/* Work around for loop-optimization bug.				*/
incr_Ptr( pp )
RGBpixel	**pp;
	{
	(*pp)++;
	return;
	}
#endif

/*
	void getCellSize( int gsize )
 */
STATIC void
getCellSize( gsize )
int gsize;
	{
	if( save_view_flag )
		{ /* Saved view from GED, match view size. */
		if( rel_perspective != 0.0 )
			/* Animation sequence, perspective gridding. */
			cell_sz = EYE_SIZE / (fastf_t) gsize;
		else
			cell_sz = view_size / (fastf_t) gsize;
		}
	else
		cell_sz = view_size/ (fastf_t) gsize;
	return;
	}

/*
	void getCenter( void )

 */
STATIC int
getCenter()
	{
	switch( type_grid )
		{
	case GT_RPP_CENTERED :
		modl_cntr[X] = (rt_ip->mdl_max[X] + rt_ip->mdl_min[X]) / 2.0;
		modl_cntr[Y] = (rt_ip->mdl_max[Y] + rt_ip->mdl_min[Y]) / 2.0;
		modl_cntr[Z] = (rt_ip->mdl_max[Z] + rt_ip->mdl_min[Z]) / 2.0;
		break;
	case GT_ORG_CENTERED :
		VSETALL( modl_cntr, 0.0 );
		break;
	default :
		rt_log( "Illegal grid type %d\n", type_grid );
		return	FALSE;
		}
	return	TRUE;
	}

/*	r e n d e r _ M o d e l ( )					*/
void
render_Model( frame )
int	frame;
	{	int	x;
	(void) signal( SIGINT, abort_sig );
	if( npsw > 1 )
		pix_buffered = B_LINE;

	if( aperture_sz < 1 )
		aperture_sz = 1;
	if( ir_mapping & IR_OCTREE )
		{
		ag.a_hit = f_IR_Model;
		ag.a_miss = f_IR_Backgr;
		}
	else
	if( query_region )
		{
		ag.a_hit = f_Region;
		ag.a_miss = f_R_Miss;
		ag.a_overlap = report_overlaps ? f_Overlap : f_NulOverlap;
		ag.a_onehit = TRUE;
		}
	else
	if( hiddenln_draw )
		{
		if( (hl_regmap = (short *)
			malloc( (unsigned)(grid_sz*grid_sz)*sizeof(short) ))
			== (short *) NULL
			)
			rt_log( "Warning, no memory for region map.\n" );
		if( (hl_dstmap = (unsigned short *)
			malloc( (unsigned)
				(grid_sz*grid_sz)*sizeof(unsigned short) ))
			== (unsigned short *) NULL
			)
			rt_log( "Warning, no memory for distance map.\n" );
		ag.a_hit = f_HL_Hit;
		ag.a_miss = f_HL_Miss;
		ag.a_overlap = report_overlaps ? f_Overlap : f_NulOverlap;
		ag.a_onehit = TRUE;
		}
	else
		{
		ag.a_hit = f_Model;
		ag.a_miss = f_Backgr;
		ag.a_overlap = report_overlaps ? f_Overlap : f_NulOverlap;
		ag.a_onehit = ! (max_bounce > 0);
		}
	ag.a_rt_i = rt_ip;
	ag.a_rbeam = modl_radius / grid_sz;
	ag.a_diverge = 0.0;

	/* Compute center of view. */
	if( ! getCenter() )
		return;

	/* Compute light source positions.				*/
	if( ! setup_Lgts( frame ) )
		{
		(void) signal( SIGINT, norml_sig );
		return;
		}
	/* Compute grid vectors of magnitude of one cell.
		These will be the delta vectors between adjacent cells.
	 */
	a_gridsz = anti_aliasing ? grid_sz * aperture_sz : grid_sz;
	if( force_cellsz )
		a_cellsz = anti_aliasing ? cell_sz / aperture_sz : cell_sz;
	else
		{
		getCellSize( a_gridsz );
		a_cellsz = cell_sz;
		}

	if( rt_g.debug & DEBUG_CELLSIZE )
		rt_log( "Cell size is %g mm.\n", cell_sz );
	Scale2Vec( grid_hor, a_cellsz, grid_dh );
	Scale2Vec( grid_ver, a_cellsz, grid_dv );

	/* Statistics for refraction tuning.				*/ 
	refrac_missed = 0;
	refrac_inside = 0;
	refrac_total = 0;

	/* Statistics for shadowing.					*/
	hits_shadowed = 0;
	hits_lit = 0;

	fatal_error = FALSE;

	/* Get starting and ending scan line number.			*/
	if( grid_x_fin >= fb_getwidth( fbiop ) )
		grid_x_fin = fb_getwidth( fbiop ) - 1;
	if( grid_y_fin >= fb_getheight( fbiop ) )
		grid_y_fin = fb_getheight( fbiop ) - 1;
	curr_scan = grid_y_org;
	last_scan = grid_y_fin;

	if( tty )
		{
		rt_prep_timer();
		(void) sprintf( FRAME_NO_PTR, "%04d", frame_no );
		prnt_Event( "Raytracing..." );
		SCROLL_DL_MOVE();
		(void) fflush( stdout );
		}
	if( ! rt_g.rtg_parallel )
		{
		/*
		 * SERIAL case -- one CPU does all the work.
		 */
		render_Scan();
		view_end();
		(void) signal( SIGINT, norml_sig );
		return;
		}
	/*
	 *  Parallel case.
	 */
	nworkers = 0;
	rt_parallel( render_Scan, npsw );

	/* ensure that all the workers are REALLY dead */
	for( x = 0; nworkers > 0; x++ )
		;
	if( x > 0 )
		rt_log( "render_Model: termination took %d extra loops\n",
			x );	

	view_end();
	(void) signal( SIGINT, norml_sig );
	return;
	}

void
render_Scan()
	{	fastf_t		grid_y_inc[3], grid_x_inc[3];
		RGBpixel	scanbuf[1024];
		register int	com;
		int		cpu;	/* local CPU number */
		
	/* Must have local copy of application structure for parallel
		threads of execution, so make copy.			*/
		struct application	a;

	RES_ACQUIRE( &rt_g.res_worker );
	cpu = nworkers++;
	RES_RELEASE( &rt_g.res_worker );

	resource[cpu].re_cpu = cpu;
#ifdef RESOURCE_MAGIC
	resource[cpu].re_magic = RESOURCE_MAGIC;
#endif

	for( ; ! user_interrupt; )
		{
		RES_ACQUIRE( &rt_g.res_worker );
		com = curr_scan++;
		RES_RELEASE( &rt_g.res_worker );

		if( com > last_scan )
			break;
		a = ag; /* Eeek, look out for those struct copy bugs. */
		assert( a.a_hit == ag.a_hit );
		assert( a.a_miss == ag.a_miss );
		assert( a.a_overlap == ag.a_overlap );
		assert( a.a_rt_i == ag.a_rt_i );
		assert( a.a_rbeam == ag.a_rbeam );
		assert( a.a_diverge == ag.a_diverge );
		a.a_x = grid_x_org;
		a.a_y = com;
		a.a_onehit = FALSE;
		a.a_resource = &resource[cpu];
		a.a_purpose = "render_Scan";
		if( anti_aliasing )
			{
			a.a_x *= aperture_sz;
			a.a_y *= aperture_sz;
			}
		for(	;
			! user_interrupt
		     &&	a.a_y < (com+1) * aperture_sz;
			view_eol( &a, (RGBpixel *) scanbuf ), a.a_y++
			)
			{
			view_bol( &a );

			/* Compute vectors from center to origin (bottom-left) of grid.	*/
			Scale2Vec( grid_dv, (fastf_t)(-a_gridsz/2)+a.a_y, grid_y_inc );
			Scale2Vec( grid_dh, (fastf_t)(-a_gridsz/2)+a.a_x, grid_x_inc );
			for(	;
				! user_interrupt
			     &&	a.a_x < (grid_x_fin+1) * aperture_sz;
				view_pix( &a, scanbuf ), a.a_x++
				)
				{	fastf_t		aim_pt[3];
				if( rel_perspective == 0.0 )
					{ /* Parallel rays emanating from grid.	*/
					Add2Vec( grid_loc, grid_y_inc, aim_pt );
					Add2Vec( aim_pt, grid_x_inc, a.a_ray.r_pt );
					VREVERSE( a.a_ray.r_dir, lgts[0].dir );
					}
				else	
				/* Fire a ray at model from the zeroth point-light-
					source position "lgts[0].loc" through each
					grid cell. The closer the source is to the
					grid, the more perspective there will be;
				 */
					{
					VMOVE( a.a_ray.r_pt, lgts[0].loc );
					/* Compute ray direction.		*/
					Add2Vec( grid_loc, grid_y_inc, aim_pt );
					AddVec( aim_pt, grid_x_inc );
					Diff2Vec( aim_pt, lgts[0].loc, a.a_ray.r_dir );
					VUNITIZE( a.a_ray.r_dir );
					}
				a.a_level = 0;	 /* Recursion level (bounces).	*/
				if( ir_mapping & IR_OCTREE )
					{
					if( ir_shootray_octree( &a ) == -1 )
						{
						/* Fatal error in application routine.	*/
						rt_log( "Fatal error: raytracing aborted.\n" );
						return;
						}
					}
				else
				if( rt_shootray( &a ) == -1 && fatal_error )
					{
					/* Fatal error in application routine.	*/
					rt_log( "Fatal error: raytracing aborted.\n" );
					return;
					}
				AddVec( grid_x_inc, grid_dh );
				}
			}
		}
	RES_ACQUIRE( &rt_g.res_worker );
	nworkers--;
	RES_RELEASE( &rt_g.res_worker );
	return;
	}

/*ARGSUSED*/
STATIC int
f_R_Miss( ap )
register struct application *ap;
	{
	prnt_Scroll( "Missed model.\n" );
	return	0;
	}

STATIC int
f_Region( ap, pt_headp )
register struct application *ap;
struct partition *pt_headp;
	{	register struct partition	*pp;
		register struct region		*regp;
		register struct soltab		*stp;
		register struct xray		*rp;
		register struct hit		*ihitp;
	Get_Partition( ap, pp, pt_headp, "f_Region" );
	regp = pp->pt_regionp;
	stp = pp->pt_inseg->seg_stp;
	rp = &ap->a_ray;
	ihitp = pp->pt_inhit;
	if( ihitp->hit_dist < BEHIND_ME_TOL )
		{ /* We are inside a solid, so slice it. */
		VJOIN1( ihitp->hit_point, rp->r_pt, ihitp->hit_dist,
			rp->r_dir );
		VSCALE( ihitp->hit_normal, rp->r_dir, -1.0 );
		}
	else
		{
		RT_HIT_NORM( ihitp, stp, rp );
		Fix_Iflip( pp, ihitp->hit_normal, ap->a_ray.r_dir, stp );
		}
	prnt_Scroll(	"Hit region \"%s\"\n", regp->reg_name );
	prnt_Scroll(	"\timpact point: <%8.2f,%8.2f,%8.2f>\n",
			ihitp->hit_point[X],
			ihitp->hit_point[Y],
			ihitp->hit_point[Z]
			);
	prnt_Scroll(	"\tsurface normal: <%8.2f,%8.2f,%8.2f>\n",
			ihitp->hit_normal[X],
			ihitp->hit_normal[Y],
			ihitp->hit_normal[Z]
			);
	prnt_Scroll(	"\tid: %d, aircode: %d, material: %d, LOS: %d\n",
			regp->reg_regionid,
			regp->reg_aircode,
			regp->reg_gmater,
			regp->reg_los
			);
	if( regp->reg_mater.ma_matname[0] != '\0' )
		{
		prnt_Scroll(	"\tmaterial info \"%.32s\"\n",
				regp->reg_mater.ma_matname
				);
		if( regp->reg_mater.ma_override )
			prnt_Scroll(	"\t\t\tcolor: {%5.2f,%5.2f,%5.2f}\n",
					regp->reg_mater.ma_color[0],
					regp->reg_mater.ma_color[1],
					regp->reg_mater.ma_color[2]
					);
		if( regp->reg_mater.ma_matparm[0] != '\0' )
			prnt_Scroll(	"\t\t\tparameters: \"%.60s\"\n",
					regp->reg_mater.ma_matparm
					);
		}
	return	1;
	}

STATIC int
f_HL_Miss( ap )
register struct application *ap;
	{
	VSETALL( ap->a_color, 0.0 );
	if( hl_regmap != NULL )
		hl_regmap[ap->a_y*grid_sz+ap->a_x] = 0;
	if( hl_dstmap != NULL )
		hl_dstmap[ap->a_y*grid_sz+ap->a_x] = 0;
	return	0;
	}

STATIC int
f_HL_Hit( ap, pt_headp )
register struct application *ap;
struct partition *pt_headp;
	{	register struct partition	*pp;
		register struct soltab		*stp;
		register struct hit		*ihitp;
	Get_Partition( ap, pp, pt_headp, "f_HL_Hit" );
	stp = pp->pt_inseg->seg_stp;
	ihitp = pp->pt_inhit;
	RT_HIT_NORM( ihitp, stp, &(ap->a_ray) );
	Fix_Iflip( pp, ihitp->hit_normal, ap->a_ray.r_dir, stp );
	ap->a_color[RED] = (ihitp->hit_normal[X] + 1.0) / 2.0;
	ap->a_color[GRN] = (ihitp->hit_normal[Y] + 1.0) / 2.0;
	ap->a_color[BLU] = (ihitp->hit_normal[Z] + 1.0) / 2.0;
	if( rt_g.debug )
		{
		V_Print( "normal", ihitp->hit_normal, rt_log );
		V_Print( "acolor", ap->a_color, rt_log );
		}
	if( hl_regmap != NULL )
		hl_regmap[ap->a_y*grid_sz+ap->a_x] =
			pp->pt_regionp->reg_regionid;
	if( hl_dstmap != NULL )
		hl_dstmap[ap->a_y*grid_sz+ap->a_x] =
			(unsigned short) ihitp->hit_dist;
	return	1;
	}

#define MA_WHITESP ", \t"	/* seperators for parameter list */
#define MA_MID	   "mid"
#define MA_LENPARM 512 /* Must be 1 byte longer than length of ma_matparm
			field of mater_info struct in raytrace.h; last
			time I looked it was 60, but I anticipate that
			we will outgrow that soon.  A defined constant
			in raytrace.h would be nice, but no-o-o-o. */
/*
	bool getMaMID( struct mater_info *map, int *id )

	This is a kludge to permit material ids to be assigned to groups
	in 'mged'.  Since the mater_info struct can be inherited down the
	tree, we will use it.  If the string MA_MID=<digits> appears in the
	ma_matparm array, we will assign those digits to the material id
	of this region.
 */
STATIC bool
getMaMID( map, id )
struct mater_info *map;
int *id;
	{	char matparm[MA_LENPARM];
		char *name;
		char *value;
		register char *p;
	if( map->ma_matname[0] == '\0' )
		return	false;
	/* guard against changes to length of ma_matparm field */
	if( sizeof(map->ma_matparm) > MA_LENPARM-1 )
		{
		rt_log( "BUG: Must lengthen MA_LENPARM to be %u bytes.\n",
			sizeof( map->ma_matparm ) );
		return	false;
		}
	/* copy parameter string to scratch buffer and null terminate */
	(void) strncpy( matparm, map->ma_matparm, sizeof(map->ma_matparm) );
	matparm[sizeof(map->ma_matparm)] = '\0';
	/* get <name>=<value> string */
	if( (name = strtok( matparm, MA_WHITESP )) == NULL )
		return	false;
	do
		{
		/* break it down into name and value */
		for( p = name; *p != '\0'; p++ )
			if( *p == '=' )
				{
				value = p+1;
				*p = '\0';
				break;
				}
		/* if we have a material id, get it and return */
		if(	strcmp( name, MA_MID ) == 0
		    &&	sscanf( value, "%d", id ) == 1 )
			return	true;
		}
	/* keep trying for rest of parameter string */
	while( (name = strtok( NULL, MA_WHITESP )) != NULL );
	return	false;		
	}


/*	f _ M o d e l ( )
	'Hit' application specific routine for 'rt_shootray()' from
	observer or a bounced ray.

 */
STATIC int
f_Model( ap, pt_headp )
register struct application *ap;
struct partition *pt_headp;
	{	register struct partition	*pp;
		register Mat_Db_Entry		*entry;
		register struct soltab		*stp;
		register struct hit		*ihitp;
		register struct xray		*rp = &ap->a_ray;
		int				material_id;
		fastf_t				rgb_coefs[3];
	Get_Partition( ap, pp, pt_headp, "f_Model" );
	stp = pp->pt_inseg->seg_stp;
	ihitp = pp->pt_inhit;
	if( ihitp->hit_dist < BEHIND_ME_TOL )
		{ /* We are inside a solid, so slice it. */
		VJOIN1( ihitp->hit_point, rp->r_pt, ihitp->hit_dist,
			rp->r_dir );
		VSCALE( ihitp->hit_normal, rp->r_dir, -1.0 );
		}
	else
		{
		RT_HIT_NORM( ihitp, stp, rp );
		Fix_Iflip( pp, ihitp->hit_normal, ap->a_ray.r_dir, stp );
		}
#if 0
	{	register struct hit	*ohitp;
	stp = pp->pt_outseg->seg_stp;
	ohitp = pp->pt_outhit;
	RT_HIT_NORM( ohitp, stp, &(ap->a_ray) );
	Fix_Oflip( pp, ohitp->hit_normal, ap->a_ray.r_dir, stp );
	}
#endif

	/* See if we hit a light source.				*/
	{	register int	i;
	for(	i = 1;
		i < lgt_db_size && stp != lgts[i].stp;
		i++
		)
		;
	if( i < lgt_db_size && lgts[i].energy > 0.0 )
		{ /* Maximum light coming from light source.		*/
		ap->a_color[0] = lgts[i].rgb[0];
		ap->a_color[1] = lgts[i].rgb[1];
		ap->a_color[2] = lgts[i].rgb[2];
		return	1;
		}
	}

	/* Get material id as index into material database.		*/
	if( ! getMaMID( &pp->pt_regionp->reg_mater, &material_id ) )
		material_id = (int)(pp->pt_regionp->reg_gmater);

	/* Get material database entry.					*/
	if( ir_mapping )
		{ /* We are mapping temperatures into an octree.	*/
			Trie		*triep;
			Octree		*octreep;
			int		fahrenheit;
		if( ! ir_Chk_Table() )
			{
			fatal_error = TRUE;
			return	-1;
			}
		entry = &mat_tmp_entry;
		if( ir_mapping & IR_READONLY )
			{	int	ir_level = 0;
			RES_ACQUIRE( &rt_g.res_worker );
			octreep = find_Octant(	&ir_octree,
						ihitp->hit_point,
						&ir_level
						);
			RES_RELEASE( &rt_g.res_worker );
			}
		else
		if( ir_mapping & IR_EDIT )
			{
			if( ir_offset )
				{	RGBpixel	pixel;
					int	x = ap->a_x + x_fb_origin - ir_mapx;
					int	y = ap->a_y + y_fb_origin - ir_mapy;
				/* Map temperature from IR image using offsets.	*/
				RES_ACQUIRE( &rt_g.res_stats );
				if(	x < 0 || y < 0
				    ||	fb_read( fbiop, x, y, pixel, 1 ) == -1
					)
					fahrenheit = AMBIENT-1;
				else
					fahrenheit = pixel_To_Temp( (RGBpixel *) pixel );
				RES_RELEASE( &rt_g.res_stats );
				}
			else
			if( ir_doing_paint )
				/* User specified temp. of current rectangle.	*/
				fahrenheit = ir_paint;
			else
				/* Unknown temperature, use out-of-band value.	*/
				fahrenheit = AMBIENT-1;
			RES_ACQUIRE( &rt_g.res_worker );
			triep = add_Trie( pp->pt_regionp->reg_name, &reg_triep );
			octreep = add_Region_Octree(	&ir_octree,
							ihitp->hit_point,
							triep,
							fahrenheit,
							0
							);
			if( octreep != OCTREE_NULL )
				append_Octp( triep, octreep );
			else
			if( fatal_error )
				{
				RES_RELEASE( &rt_g.res_worker );
				return	-1;
				}
			RES_RELEASE( &rt_g.res_worker );
			}
		if( octreep != OCTREE_NULL )
			{	register int	index;
			index = octreep->o_temp - ir_min;
			index = index < 0 ? 0 : index;
			COPYRGB( entry->df_rgb, ir_table[index] );
			}
		}
	else
	/* Get material attributes from database.		*/
	if(	(entry = mat_Get_Db_Entry( material_id )) == MAT_DB_NULL
	   || ! (entry->mode_flag & MF_USED)
		)
		entry = &mat_dfl_entry;
	/* If texture mapping replace color with texture map look up. */
	if( strncmp( TEX_KEYWORD, entry->name, TEX_KEYLEN ) == 0 )
		{	struct uvcoord		uv;
			Mat_Db_Entry		loc_entry;
		/* Solid has a frame buffer image map.		*/
		rt_functab[stp->st_id].ft_uv( ap, stp, ihitp, &uv );
		loc_entry = *entry;
		if( tex_Entry( &uv, &loc_entry ) )
			entry = &loc_entry;
		}
	if( lgts[0].energy < 0.0 )
		{	fastf_t	f = RGB_INVERSE;
			/* Scale RGB values to coeffs (0.0 .. 1.0 )	*/
		/* Negative intensity on ambient light is a flag meaning
			do not calculate lighting effects at all, but
			produce a flat, pseudo-color mapping.
		 */
		ap->a_color[0] = entry->df_rgb[0] * f;
		ap->a_color[1] = entry->df_rgb[1] * f;
		ap->a_color[2] = entry->df_rgb[2] * f;
		return	1;
		}

	/* Compute contribution from this surface.			*/
	{	fastf_t	f;
		register int	i;
		auto fastf_t	view_dir[3];

	/* Calculate view direction.					*/
	VREVERSE( view_dir, ap->a_ray.r_dir );

	rgb_coefs[0] = rgb_coefs[1] = rgb_coefs[2] = 0.0;
	if( (f = 1.0 - (entry->reflectivity + entry->transparency)) > 0.0 )
		{
		for( i = 0; i < lgt_db_size; i++ )
			{ /* All light sources.				*/
			if( lgts[i].energy > 0.0 )
				{
				ap->a_user = i;
				model_Reflectance( ap, pp, entry, &lgts[i],
							view_dir
							);
				VJOIN1(	rgb_coefs, rgb_coefs, f, ap->a_color );
				if( rt_g.debug & DEBUG_SHADOW )
					{
					rt_log( "\t\tcontribution from light %d:\n", i );
					V_Print( "\t\treflectance coeffs", ap->a_color, rt_log );
					V_Print( "\t\taccumulated coeffs", rgb_coefs, rt_log );
					}
				}
			}
		}
	}

	/* Recursion is used to do multiple bounces for mirror reflections
		and transparency.  The level of recursion is stored in
		'a_level' and is controlled via the variable 'max_bounce'.
	 */
	if( ap->a_level + 1 <= max_bounce )
		{
		ap->a_user = material_id;
		if( entry->reflectivity > 0.0 )
			{	register fastf_t *mirror_coefs =
						mirror_Reflect( ap, pp );
			/* Compute mirror reflection.			*/
			VJOIN1(	rgb_coefs,
				rgb_coefs,
				entry->reflectivity,
				mirror_coefs
				);
			if( rt_g.debug & DEBUG_RGB )
				{
				V_Print( "mirror", mirror_coefs, rt_log );
				V_Print( "rgb_coefs", rgb_coefs, rt_log );
				}
			}
		if( entry->transparency > 0.0 )
			{
			glass_Refract( ap, pp, entry );
			/* Compute transmission through glass.		*/
			VJOIN1( rgb_coefs,
				rgb_coefs,
				entry->transparency,
				ap->a_color
				);
			if( rt_g.debug & DEBUG_RGB )
				{
				V_Print( "glass", ap->a_color, rt_log );
				V_Print( "rgb_coefs", rgb_coefs, rt_log );
				}
			}
		}
	/* Pass result in application struct.				*/
	VMOVE( ap->a_color, rgb_coefs );
	if( rt_g.debug & DEBUG_RGB )
		{
		V_Print( "ap->a_color", ap->a_color, rt_log );
		}
	return	1;
	}

/*	c o r r e c t _ L g t ( )
	Shoot a ray to the light source to determine if surface
	is shadowed, return corrected light source intensity.
 */
STATIC fastf_t
correct_Lgt( ap, pp, lgt_entry )
register struct application	*ap;
register struct partition	*pp;
register Lgt_Source		*lgt_entry;
	{	fastf_t	energy_attenuation;
		fastf_t	lgt_dir[3];
	if( ! shadowing )
		{
		energy_attenuation = 1.0;
		if( ! lgt_entry->beam )
			return	lgt_entry->energy;
		}
	/* Vector to light src from surface contact pt. */
	Diff2Vec( lgt_entry->loc, pp->pt_inhit->hit_point, lgt_dir );
	VUNITIZE( lgt_dir );
	if( shadowing )
		{	struct application	ap_hit;
		/* Set up appl. struct for 'rt_shootray()' to light src. */
		ap_hit = *ap; /* Watch out for struct copy bugs. */
		assert( ap_hit.a_overlap == ap->a_overlap );
		assert( ap_hit.a_level == ap->a_level );
		ap_hit.a_onehit = FALSE; /* Go all the way to the light. */
		ap_hit.a_hit = f_Shadow; /* Handle shadowed pixels. */
		ap_hit.a_miss = f_Lit;   /* Handle illuminated pixels. */
		ap_hit.a_level++;	 /* Increment recursion level. */
	
		if( rt_g.debug & DEBUG_SHADOW )
			{
			rt_log( "\tcorrect_Lgt()\n" );
			V_Print( "\t\tlgt source location",
				lgt_entry->loc, rt_log );
			}
		/* Set up ray direction to light source. */
		VMOVE( ap_hit.a_ray.r_dir, lgt_dir );

		/* Set up ray origin at surface contact point. */
		VMOVE( ap_hit.a_ray.r_pt, pp->pt_inhit->hit_point );
	
		if( rt_g.debug & DEBUG_SHADOW )
			{
			V_Print( "\t\tdir. of ray to light",
				ap_hit.a_ray.r_dir, rt_log );
			V_Print( "\t\torigin of ray to lgt",
				ap_hit.a_ray.r_pt, rt_log );
			}
		/* Fetch attenuated lgt intensity into "ap_hit.a_diverge". */
		(void) rt_shootray( &ap_hit );
		energy_attenuation = ap_hit.a_diverge;
		if( energy_attenuation == 0.0 )
			/* Shadowed by opaque object(s). */
			return	energy_attenuation;
		}

	/* Light is either full intensity or attenuated by transparent
		object(s). */
	if( lgt_entry->beam )
		/* Apply gaussian intensity distribution.		*/
		{	fastf_t lgt_cntr[3];
			fastf_t ang_dist, rel_radius;
			fastf_t	cos_angl;
			fastf_t	gauss_Wgt_Func();
		if( lgt_entry->stp == SOLTAB_NULL )
			cons_Vector( lgt_cntr, lgt_entry->azim, lgt_entry->elev );
		else
			{
			Diff2Vec( lgt_entry->loc, modl_cntr, lgt_cntr );
			VUNITIZE( lgt_cntr );
			}
		cos_angl = Dot( lgt_cntr, lgt_dir );
		if( NEAR_ZERO( cos_angl, EPSILON ) )
			/* Negligable intensity.			*/
			return	0.0;
		ang_dist = sqrt( 1.0 - Sqr( cos_angl ) );
		rel_radius = lgt_entry->radius / pp->pt_inhit->hit_dist;
		if( rt_g.debug & DEBUG_RGB )
			{
			rt_log( "\t\tcos. of angle to lgt center = %g\n", cos_angl );
			rt_log( "\t\t	   angular distance = %g\n", ang_dist );
			rt_log( "\t\t	    relative radius = %g\n", rel_radius );
			rt_log( "\t\t	relative distance = %g\n", ang_dist/rel_radius );
			}
		/* Return weighted and attenuated light intensity.	*/
		return	gauss_Wgt_Func( ang_dist/rel_radius ) *
			lgt_entry->energy * energy_attenuation;
		}
	else	/* Return attenuated light intensity.			*/
		return	lgt_entry->energy * energy_attenuation;
	}

/*	m i r r o r _ R e f l e c t ( )					*/
STATIC fastf_t *
mirror_Reflect( ap, pp )
register struct application	*ap;
register struct partition	*pp;
	{	fastf_t			r_dir[3];
		struct application	ap_hit;
	ap_hit = *ap;		/* Same as initial application.		*/
	ap_hit.a_onehit = FALSE;
	ap_hit.a_level++;	/* Increment recursion level.		*/

	if( rt_g.debug & DEBUG_RGB )
		{
		rt_log( "\tmirror_Reflect: level %d grid <%d,%d>\n",
			ap_hit.a_level, ap_hit.a_x, ap_hit.a_y
			);
		rt_log( "\t\tOne hit flag is %s\n",
			ap_hit.a_onehit ? "ON" : "OFF" );
		}
	/* Calculate reflected incident ray.				*/
	VREVERSE( r_dir, ap->a_ray.r_dir );

	{	fastf_t	f = 2.0	* Dot( r_dir, pp->pt_inhit->hit_normal );
		fastf_t tmp_dir[3];
	Scale2Vec( pp->pt_inhit->hit_normal, f, tmp_dir );
	Diff2Vec( tmp_dir, r_dir, ap_hit.a_ray.r_dir );
	}
	/* Set up ray origin at surface contact point.			*/
	VMOVE( ap_hit.a_ray.r_pt, pp->pt_inhit->hit_point );
	(void) rt_shootray( &ap_hit );
	return	ap_hit.a_color;
	}

/*	g l a s s _ R e f r a c t ( )					*/
STATIC void
glass_Refract( ap, pp, entry )
register struct application	*ap;
register struct partition	*pp;
register Mat_Db_Entry		*entry;
	{	struct application	ap_hit;	/* To shoot ray beyond.	*/
		struct application	ap_ref; /* For getting thru.	*/
	/* Application structure for refracted ray.			*/
	ap_ref = *ap;
	ap_ref.a_hit =  f_Probe;	/* Find exit from glass.	*/
	ap_ref.a_miss = f_Error;	/* Bad news.			*/
	assert( ap_ref.a_overlap == ap->a_overlap );
	ap_ref.a_onehit = TRUE;
	ap_ref.a_level++;		/* Increment recursion level.	*/

	/* Application structure for exiting ray.			*/
	ap_hit = *ap;
	ap_hit.a_onehit = FALSE;
	ap_hit.a_level++;

	if( rt_g.debug & DEBUG_REFRACT )
		{
		rt_log( "\tEntering glass_Refract(), level %d grid <%d,%d>\n",
			ap->a_level, ap->a_x, ap->a_y
			);
		V_Print( "\t\tincident ray pnt", ap->a_ray.r_pt, rt_log );
		V_Print( "\t\tincident ray dir", ap->a_ray.r_dir, rt_log );
		}
	refrac_total++;

	/* Guard against zero refractive index. */
	if( entry->refrac_index < 0.001 )
		entry->refrac_index = 1.0;

	if( entry->refrac_index == RI_AIR )
		{ /* No refraction necessary.				*/
			struct partition	*pt_headp = pp->pt_back;
		if( rt_g.debug & DEBUG_REFRACT )
			rt_log( "\t\tNo refraction on entry.\n" );
		/* Ray direction stays the same, and so does ray origin,
			because we are using existing partitions with
			hit distances relative to the ray origin.
		 */
		VMOVE( ap_hit.a_ray.r_dir, ap->a_ray.r_dir );
		VMOVE( ap_hit.a_ray.r_pt, ap->a_ray.r_pt );
		if( pp->pt_forw != pt_headp )
			{
			/* We have more partitions, so use them, but first
				toss out the current one because we must
				always pass the hit routine the head of the
				partition chain, so we can detect the end
				of the circular-doubly-linked list.
			 */
			DEQUEUE_PT( pp );
			FREE_PT( pp, ap->a_resource );
			f_Model( &ap_hit, pt_headp );
			VMOVE( ap->a_color, ap_hit.a_color );
			if( rt_g.debug & DEBUG_REFRACT )
				{
				V_Print( "\t\tf_Model returned coeffs",
					ap->a_color, rt_log );
				}
			return;	
			}
		else
			{
			f_Backgr( &ap_hit );
			VMOVE( ap->a_color, ap_hit.a_color );
			if( rt_g.debug & DEBUG_REFRACT )
				{
				rt_log( "\t\tOne hit flag is %s\n",
					ap->a_onehit ? "ON" : "OFF" );
				V_Print( "\t\tf_Backgr returned coeffs",
					ap->a_color, rt_log );
				}
			return;
			}
		}
	else
		/* Set up ray-trace to find new exit point.		*/
		{ /* Calculate refraction at entrance.			*/
		if( pp->pt_inhit->hit_dist < 0.0 )
			{
			if( rt_g.debug & DEBUG_REFRACT )
				rt_log( "\t\tRefracting inside solid.\n" );
			VMOVE( ap_ref.a_ray.r_pt, ap->a_ray.r_pt );
			VMOVE( ap_ref.a_ray.r_dir, ap->a_ray.r_dir );
			goto	inside_ray;
			}
		if( ! refract(	ap->a_ray.r_dir,   /* Incident ray.	*/
				pp->pt_inhit->hit_normal,
				RI_AIR,		   /* Air ref. index.	*/
				entry->refrac_index,
				ap_ref.a_ray.r_dir /* Refracted ray.	*/
				)
			)
			{ /* Past critical angle, reflected back out.	*/
			VMOVE( ap_hit.a_ray.r_pt, pp->pt_inhit->hit_point );
			VMOVE( ap_hit.a_ray.r_dir, ap_ref.a_ray.r_dir );
			if( rt_g.debug & DEBUG_REFRACT )
				rt_log( "\t\tPast critical angle on entry!\n" );
			goto	exiting_ray;
			}
		}
	/* Fire from entry point.					*/
	VMOVE( ap_ref.a_ray.r_pt, pp->pt_inhit->hit_point );

inside_ray :
	/* Find exit of refracted ray, exit normal (reversed) returned in
		ap_ref.a_uvec, and exit point returned in ap_ref.a_color.
	 */
	if( ! rt_shootray( &ap_ref ) )
		{ /* Refracted ray missed, solid!  This should not occur,
			but if it does we will skip the refraction.
		   */
		if( rt_g.debug & DEBUG_REFRACT )
			{
			rt_log( "\t\tRefracted ray missed:\n" );
			V_Print( "\t\trefracted ray pnt", ap_ref.a_ray.r_pt, rt_log );
			V_Print( "\t\trefracted ray dir", ap_ref.a_ray.r_dir, rt_log );
			}
		refrac_missed++;
		VMOVE( ap_hit.a_ray.r_pt, pp->pt_outhit->hit_point );
		VMOVE( ap_hit.a_ray.r_dir, ap->a_ray.r_dir );
		goto	exiting_ray;
		}
	else
		{
		if( rt_g.debug & DEBUG_REFRACT )
			rt_log( "\t\tRefracted ray hit.\n" );
		}

	/* Calculate refraction at exit.				*/
	if( ap_ref.a_level <= max_bounce )
		{
		/* Reversed exit normal in a_uvec.			*/
		if( ! refract(	ap_ref.a_ray.r_dir,
				ap_ref.a_uvec,
				entry->refrac_index,
				RI_AIR,
				ap_hit.a_ray.r_dir
				)
			)
			{ /* Past critical angle, internal reflection.	*/
			if( rt_g.debug & DEBUG_REFRACT )
				rt_log( "\t\tInternal reflection, recursion level (%d)\n", ap_ref.a_level );
			ap_ref.a_level++;
			VMOVE( ap_ref.a_ray.r_dir, ap_hit.a_ray.r_dir );
			/* Refracted ray exit point in a_color.		*/
			VMOVE( ap_ref.a_ray.r_pt, ap_ref.a_color );
			goto	inside_ray;
			}
		}
	else
		{ /* Exceeded max bounces, total absorbtion of light.	*/
		ap->a_color[0] = ap->a_color[1] = ap->a_color[2] = 0.0;
		if( rt_g.debug & DEBUG_REFRACT )
			rt_log( "\t\tExceeded max bounces with internal reflections, recursion level (%d)\n", ap_ref.a_level );
		refrac_inside++;
		return;
		}
	/* Refracted ray exit point in a_color.				*/
	VMOVE( ap_hit.a_ray.r_pt, ap_ref.a_color );

exiting_ray :
	/* Shoot from exit point in direction of refracted ray.		*/
	if( rt_g.debug & DEBUG_REFRACT )
		{
		rt_log( "\t\tExiting ray from glass.\n" );
		V_Print( "\t\t   ray origin", ap_hit.a_ray.r_pt, rt_log );
		V_Print( "\t\tray direction", ap_hit.a_ray.r_dir, rt_log );
		rt_log( "\t\tOne hit flag is %s\n", ap_hit.a_onehit ? "ON" : "OFF" );
		}
	(void) rt_shootray( &ap_hit );
	VMOVE( ap->a_color, ap_hit.a_color );
	return;
	}

/*	f _ B a c k g r ( )
	'Miss' application specific routine for 'rt_shootray()' from
	observer or a bounced ray.
 */
STATIC int
f_Backgr( ap )
register struct application *ap;
	{	register int	i;
	/* Base-line color is same as background.			*/
	VMOVE( ap->a_color, bg_coefs );

	if( rt_g.debug & DEBUG_RGB )
		{
		rt_log( "\tRay missed model.\n" );
		V_Print( "\tbackground coeffs", ap->a_color, rt_log );
		}

	/* If this is a reflection, we may see each light source.	*/
	if( ap->a_level )
		{	Mat_Db_Entry	*mdb_entry;
		if( (mdb_entry = mat_Get_Db_Entry( ap->a_user ))
			== MAT_DB_NULL
		   || ! (mdb_entry->mode_flag & MF_USED)
			)
			mdb_entry = &mat_dfl_entry;
		for( i = 1; i < lgt_db_size; i++ )
			{	auto fastf_t		real_l_1[3];
				register fastf_t	specular;
				fastf_t			cos_s;
			if( lgts[i].energy <= 0.0 )
				continue;
			Diff2Vec( lgts[i].loc, ap->a_ray.r_pt, real_l_1 );
			VUNITIZE( real_l_1 );
			if(	(cos_s = Dot( ap->a_ray.r_dir, real_l_1 ))
				> 0.0
			    &&	cos_s <= 1.0
				)
				{
				specular = mdb_entry->wgt_specular *
					lgts[i].energy *
					myIpow( cos_s, mdb_entry->shine );
				/* Add reflected light source.		*/
				VJOIN1(	ap->a_color,
					ap->a_color,
					specular,
					lgts[i].coef );
				}
			}
		}
	if( rt_g.debug & DEBUG_RGB )
		{
		V_Print( "coeffs returned from background", ap->a_color, rt_log );
		}
	return	0;
	}

/*	f _ E r r o r ( )						*/
/*ARGSUSED*/
STATIC int
f_Error( ap )
register struct application *ap;
	{
	if( rt_g.debug & DEBUG_RGB )
		rt_log( "f_Error()\n" );
	return	0;
	}

/*	f _ L i t ( )
	'Miss' application specific routine for 'rt_shootray()' to
	light source for shadowing.  Return full intensity in "ap->a_diverge".
 */
STATIC int
f_Lit( ap )
register struct application *ap;
	{	
	if( rt_g.debug & DEBUG_SHADOW )
		rt_log( "\t\tSurface is illuminated.\n" );
	ap->a_diverge = 1.0;
	hits_lit++;
	return	0;
	}

/*	f _ P r o b e ( )						*/
STATIC int
f_Probe( ap, pt_headp )
register struct application *ap;
struct partition *pt_headp;
	{	register struct partition	*pp;
		register struct hit		*hitp;
		register struct soltab		*stp;
	if( rt_g.debug & DEBUG_RGB )
		rt_log( "f_Probe()\n" );
	Get_Partition( ap, pp, pt_headp, "f_Probe" );
	stp = pp->pt_outseg->seg_stp;
	hitp = pp->pt_outhit;
	RT_HIT_NORM( hitp, stp, &(ap->a_ray) );
	Fix_Oflip( pp, hitp->hit_normal, ap->a_ray.r_dir, stp );
	VMOVE( ap->a_uvec, hitp->hit_normal );
	VMOVE( ap->a_color, hitp->hit_point );
	if( ! pp->pt_outflip )
		{ /* For refraction, want exit normal to point inward.	*/
		VREVERSE( ap->a_uvec, ap->a_uvec );
		}
	return	1;
	}

/*	r e f r a c t ( )
	Compute the refracted ray 'v_2' from the incident ray 'v_1' with
	the refractive indices 'ri_2' and 'ri_1' respectively.

	Using Snell's Law

		theta_1 = angle of v_1 with surface normal
		theta_2 = angle of v_2 with reversed surface normal
		ri_1 * sin( theta_1 ) = ri_2 * sin( theta_2 )

		sin( theta_2 ) = ri_1/ri_2 * sin( theta_1 )
		
	The above condition is undefined for ri_1/ri_2 * sin( theta_1 )
	being greater than 1, and this represents the condition for total
	reflection, the 'critical angle' is the angle theta_1 for which
	ri_1/ri_2 * sin( theta_1 ) equals 1.
 */
STATIC int
refract( v_1, norml, ri_1, ri_2, v_2 )
register fastf_t	*v_1, *norml;
fastf_t			ri_1, ri_2;
register fastf_t	*v_2;
	{	fastf_t	w[3], u[3];	/* Intermediate vectors.	*/
		fastf_t	beta;		/* Intermediate scalar.		*/
	if( rt_g.debug & DEBUG_REFRACT )
		{
		V_Print( "\tEntering refract(), incident ray", v_1, rt_log );
		V_Print( "\t\tentrance normal", norml, rt_log );
		rt_log( "\t\trefractive indices leaving:%g, entering:%g\n", ri_1, ri_2 );
		}
	if( ri_2 < 0.001 || ri_1 < 0.001 )
		{ /* User probably forgot to specify refractive index.	*/
		rt_log( "\tBUG: Zero or negative refractive index, should have been caught earlier.\n" );
		VMOVE( v_2, v_1 ); /* Just return ray unchanged.	*/
		return	1;
		}
	beta = ri_1 / ri_2;
	Scale2Vec( v_1, beta, w );	
	CrossProd( w, norml, u );
	/*	|w X norml| = |w||norml| * sin( theta_1 )
		        |u| = ri_1/ri_2 * sin( theta_1 ) = sin( theta_2 )
	 */
	if( (beta = Dot( u, u )) > 1.0 ) /* beta = sin( theta_2 )^^2.	*/
		{ /* Past critical angle, total reflection.
			Calculate reflected (bounced) incident ray.
		   */
		VREVERSE( u, v_1 );
		beta = 2.0 * Dot( u, norml );
		Scale2Vec( norml, beta, w );
		Diff2Vec( w, u, v_2 );
		if( rt_g.debug & DEBUG_REFRACT )
			{
			V_Print( "\tdeflected refracted ray", v_2, rt_log );
			}
		return	0;
		}
	else
		{
		/* 1 - beta = 1 - sin( theta_2 )^^2
			    = cos( theta_2 )^^2.
		       beta = -1.0 * cos( theta_2 ) - Dot( w, norml ).
		 */
		beta = -sqrt( 1.0 - beta ) - Dot( w, norml );
		Scale2Vec( norml, beta, u );
		Add2Vec( w, u, v_2 );
		if( rt_g.debug & DEBUG_REFRACT )
			{
			V_Print( "\trefracted ray", v_2, rt_log );
			}
		return	1;
		}
	/*NOTREACHED*/
	}

/*	f _ S h a d o w ( )
	'Hit' application specific routine for 'rt_shootray()' to
	light source for shadowing. Returns attenuated light intensity in
	"ap->a_diverge".
 */
STATIC int
f_Shadow( ap, pt_headp )
register struct application *ap;
struct partition *pt_headp;
	{	register struct partition	*pp;
		register Mat_Db_Entry		*entry;
	Get_Partition( ap, pp, pt_headp, "f_Shadow" );
	if( rt_g.debug & DEBUG_SHADOW )
		{	register struct hit	*ihitp, *ohitp;
			register struct soltab	*istp, *ostp;
		rt_log( "Shadowed by :\n" );
		istp = pp->pt_inseg->seg_stp;
		ihitp = pp->pt_inhit;
		ostp = pp->pt_outseg->seg_stp;
		ohitp = pp->pt_outhit;
		RT_HIT_NORM( ihitp, istp, &(ap->a_ray) );
		Fix_Iflip( pp, ihitp->hit_normal, ap->a_ray.r_dir, istp );
		RT_HIT_NORM( ohitp, ostp, &(ap->a_ray) );
		Fix_Oflip( pp, ohitp->hit_normal, ap->a_ray.r_dir, ostp );
		V_Print( "entry normal", ihitp->hit_normal, rt_log );
		V_Print( "entry point", ihitp->hit_point, rt_log );
		rt_log( "partition[start %g end %g]\n",
			ihitp->hit_dist, ohitp->hit_dist
			);
		rt_log( "solid name (%s)\n", pp->pt_inseg->seg_stp->st_name );
		}
	ap->a_diverge = 1.0;
	if( pp->pt_inseg->seg_stp == lgts[ap->a_user].stp )
		{ /* Have hit the EXPLICIT light source, no shadow.	*/
		if( rt_g.debug & DEBUG_SHADOW )
			rt_log( "Unobstructed path to explicit light.\n" );
		return	ap->a_miss( ap );
		}
	for( ; pp != pt_headp; pp = pp->pt_forw )
		{
		if( pp->pt_inseg->seg_stp == lgts[ap->a_user].stp )
			/* Have hit the EXPLICIT light source.		*/
			break;
		if(	(entry =
			mat_Get_Db_Entry( (int)(pp->pt_regionp->reg_gmater) ))
				== MAT_DB_NULL
		   || ! (entry->mode_flag & MF_USED)
			)
			entry = &mat_dfl_entry;
		if( (ap->a_diverge -= 1.0 - entry->transparency) <= 0.0 )
			/* Light is totally eclipsed.			*/
			{
			ap->a_diverge = 0.0;
			break;
			}
		}
	if( ap->a_diverge != 1.0 )
		/* Light source is obstructed, object shadowed.		*/
		{
		if( rt_g.debug & DEBUG_SHADOW )
			rt_log( "Lgt source obstructed, object shadowed\n" );
		hits_shadowed++;
		return	1;
		}
	else	/* Full intensity of light source.			*/
		{
		if( rt_g.debug & DEBUG_SHADOW )
			rt_log( "Full intensity of light source, no shadow\n" );
		return	ap->a_miss( ap );
		}
	}

/*	m o d e l _ R e f l e c t a n c e ( )
	This is the heart of the lighting model which is based on a model
	developed by Bui-Tuong Phong, [see Wm M. Newman and R. F. Sproull,
	"Principles of Interactive Computer Graphics", 	McGraw-Hill, 1979]
	
	Er = Ra(m)*cos(Ia) + Rd(m)*cos(Il) + W(Il,m)*cos(s)^^n
	where,
 
	Er	is the energy reflected in the observer's direction.
	Ra	is the diffuse reflectance coefficient at the point
		of intersection due to ambient lighting.
	Ia	is the angle of incidence associated with the ambient
		light source (angle between ray direction (negated) and
		surface normal).
	Rd	is the diffuse reflectance coefficient at the point
		of intersection due to lighting.
	Il	is the angle of incidence associated with the
		light source (angle between light source direction and
		surface normal).
	m	is the material identification code.
	W	is the specular reflectance coefficient,
		a function of the angle of incidence, range 0.0 to 1.0,
		for the material.
	s	is the angle between the reflected ray and the observer.
	n	'Shininess' of the material,  range 1 to 10.

	The RGB result is returned implicitly in "ap->a_color".
 */
STATIC void
model_Reflectance( ap, pp, mdb_entry, lgt_entry, view_dir )
register struct application	*ap;
struct partition		*pp;
Mat_Db_Entry			*mdb_entry;
register Lgt_Source		*lgt_entry;
fastf_t				*view_dir;
	{	/* Compute attenuation of light source intensity.	*/
		register fastf_t	*norml = pp->pt_inhit->hit_normal;
		register fastf_t	ff;		/* temp */
		fastf_t			lgt_energy;
		fastf_t			cos_il; /* Cos. incident angle.	*/
		auto fastf_t		lgt_dir[3];

	if( rt_g.debug & DEBUG_RGB )
		rt_log( "\nmodel_Reflectance(): level %d grid <%d,%d>\n",
			ap->a_level, ap->a_x, ap->a_y
			);

	if( ap->a_user == 0 )		/* Ambient lighting.		*/
		{
		lgt_energy = lgt_entry->energy;
		VMOVE( lgt_dir, view_dir );
		}
	else
		{	
		/* Compute attenuated light intensity due to shadowing.	*/
		if( (lgt_energy = correct_Lgt( ap, pp, lgt_entry )) == 0.0 )
			{
			/* Shadowed by an opaque object.		*/
			ap->a_color[0] = ap->a_color[1] = ap->a_color[2] = 0.0;
			return;
			}
		/* Direction unit vector to light source from hit pt.	*/
		Diff2Vec( lgt_entry->loc, pp->pt_inhit->hit_point, lgt_dir );
		VUNITIZE( lgt_dir );
		}

	/* Calculate diffuse reflectance from light source.		*/
	if( (cos_il = Dot( norml, lgt_dir )) < 0.0 )
		cos_il = 0.0;
	/* Facter in light source intensity and diffuse weighting.	*/
	ff = cos_il * lgt_energy * mdb_entry->wgt_diffuse;
	/* Facter in light source color.				*/
	Scale2Vec( lgt_entry->coef, ff, ap->a_color );
	if( rt_g.debug & DEBUG_RGB )
		{
		rt_log( "\tDiffuse reflectance:\n" );
		V_Print( "\tsurface normal", norml, rt_log );
		V_Print( "\t dir. of light", lgt_dir, rt_log );
		rt_log( "\t cosine of incident angle = %g\n", cos_il );
		rt_log( "\tintensity of light source = %g\n", lgt_energy );
		rt_log( "\t diffuse weighting coeff. = %g\n", mdb_entry->wgt_diffuse );
		V_Print( "\tdiffuse coeffs", ap->a_color, rt_log );
		}
	/* Facter in material color (diffuse reflectance coeffs)	*/
	ff = RGB_INVERSE; /* Scale RGB values to coeffs (0.0 .. 1.0 )	*/
	ap->a_color[0] *= mdb_entry->df_rgb[0] * ff;
	ap->a_color[1] *= mdb_entry->df_rgb[1] * ff;
	ap->a_color[2] *= mdb_entry->df_rgb[2] * ff;

	if( ap->a_user != 0 )
		/* Calculate specular reflectance, if not ambient light.
		 	Reflected ray = (2 * cos(i) * Normal) - Incident ray.
		 	Cos(s) = dot product of Reflected ray with Incident ray.
		 */
		{	auto fastf_t		lgt_reflect[3], tmp_dir[3];
			register fastf_t	specular;
			fastf_t			cos_s;
		ff = 2.0 * cos_il;
		Scale2Vec( norml, ff, tmp_dir );
		Diff2Vec( tmp_dir, lgt_dir, lgt_reflect );
		if( rt_g.debug & DEBUG_RGB )
			{
			rt_log( "\tSpecular reflectance:\n" );
			V_Print( "\t           dir of eye", view_dir, rt_log );
			V_Print( "\tdir reflected lgt ray", lgt_reflect, rt_log );
			}
		if(	(cos_s = Dot( view_dir, lgt_reflect )) > 0.0
		    &&	cos_s <= 1.0
			)
			{ /* We have a significant specular component.	*/
			specular = mdb_entry->wgt_specular * lgt_energy *
					myIpow( cos_s, mdb_entry->shine );
			/* Add specular component.			*/
			VJOIN1( ap->a_color, ap->a_color, specular, lgt_entry->coef );
			if( rt_g.debug & DEBUG_RGB )
				{
				rt_log( "\tcosine of specular angle = %g\n", cos_s );
				rt_log( "\t      specular component = %g\n", specular );
				V_Print( "\tdiff+spec coeffs", ap->a_color, rt_log );
				}
			}
		else
		if( cos_s > 1.0 )
			{	struct soltab	*stp = pp->pt_inseg->seg_stp;
			rt_log( "\"%s\"(%d) : solid \"%s\" type %d cos(s)=%g grid <%d,%d>!\n",
				__FILE__, __LINE__, stp->st_name, stp->st_id,
				cos_s, ap->a_x, ap->a_y
				);
			V_Print( "Surface normal", norml, rt_log );
			}
		}
	return;
	}

/*	c o n s _ V e c t o r ( )
	Construct a direction vector out of azimuth and elevation angles
	in radians, allocating storage for it and returning its address.
 */
void
cons_Vector( vec, azim, elev )
register fastf_t	*vec;
fastf_t	azim, elev;
	{ /* Store cosine of the elevation to save calculating twice.	*/
		fastf_t	cosE;
	cosE = cos( elev );
	vec[0] = cos( azim ) * cosE;
	vec[1] = sin( azim ) * cosE;
	vec[2] = sin( elev );
	return;
	}

/*	a b o r t _ R T ( )						*/
/*ARGSUSED*/
#if STD_SIGNAL_DECLS
void
#else
int
#endif
abort_RT( sig )
int	sig;
	{
	RES_ACQUIRE( &rt_g.res_syscall );
	(void) signal( SIGINT, abort_RT );
	(void) fb_flush( fbiop );
	user_interrupt = 1;
	RES_RELEASE( &rt_g.res_syscall );
#if STD_SIGNAL_DECLS
	return;
#else
	return	sig;
#endif
	}

/*	i p o w ( )
	Integer exponent pow() function.
	Returns 'd' to the 'n'th power.
 */
STATIC fastf_t
myIpow( d, n )
register fastf_t	d;
register int	n;
	{	register fastf_t	result = 1.0;
	if( d == 0.0 )
		return	0.0;
	while( n-- > 0 )
		result *= d;
	return	result;
	}

hl_Dst_Diff( x0, y0, x1, y1, maxdist )
register int	x0, y0, x1, y1;
register unsigned short	maxdist;
	{	short	distance;
	distance = hl_dstmap[y0*grid_sz+x0] - hl_dstmap[y1*grid_sz+x1];
	distance = Abs( distance );
#if FALSE
	if( y0 == grid_sz/2 )
	rt_log( "hl_Dst_Diff({<%4d,%4d>,<%4d,%4d>}) %4d-%4d=%4d > %d?\n",
		x0, y0, x1, y1,
		hl_dstmap[y0*grid_sz+x0],
		hl_dstmap[y1*grid_sz+x1],
		distance,
		maxdist
		);
#endif
	return distance > maxdist;
	}

hl_Reg_Diff( x0, y0, x1, y1 )
register int	x0, y0, x1, y1;
	{
#if FALSE
	rt_log( "hl_Reg_Diff({<%4d,%4d>,<%4d,%4d>}) %4d != %4d\n",
		x0, y0, x1, y1,
		hl_regmap[y0*grid_sz+x0],
		hl_regmap[y1*grid_sz+x1]
		);
#endif
	return	hl_regmap[y0*grid_sz+x0] != hl_regmap[y1*grid_sz+x1];
	}

hl_Norm_Diff( pix1, pix2 )
register RGBpixel	*pix1, *pix2;
	{	fastf_t	dir1[3], dir2[3];
		static fastf_t	conv = 2.0/255.0;
#if 0
#ifdef cray
	rt_log( "hl_Norm_Diff(0x%x,0x%x)\n", pix1, pix2 );
#endif
	rt_log( "hl_Norm_Diff(<%d,%d,%d>,<%d,%d,%d>)\n",
		(*pix1)[0], (*pix1)[1], (*pix1)[2],
		(*pix2)[0], (*pix2)[1], (*pix2)[2] );
#endif
	if( ZeroPixel( *pix1 ) )
		{
		if( ZeroPixel( *pix2 ) )
			return	0;
		else
			return	1;
		}
	else
	if( ZeroPixel( *pix2 ) )
		return	1;
	dir1[X] = (*pix1)[RED] * conv;
	dir1[Y] = (*pix1)[GRN] * conv;
	dir1[Z] = (*pix1)[BLU] * conv;
	dir2[X] = (*pix2)[RED] * conv;
	dir2[Y] = (*pix2)[GRN] * conv;
	dir2[Z] = (*pix2)[BLU] * conv;
	dir1[X] -= 1.0;
	dir1[Y] -= 1.0;
	dir1[Z] -= 1.0;
	dir2[X] -= 1.0;
	dir2[Y] -= 1.0;
	dir2[Z] -= 1.0;
	return	Dot( dir1, dir2 ) < COSTOL;
	}


void
prnt_Pixel( pixelp, x, y )
register RGBpixel	*pixelp;
int	x, y;
	{
	rt_log( "Pixel:<%3d,%3d,%3d>(%4d,%4d)\n",
		(*pixelp)[RED],
		(*pixelp)[GRN],
		(*pixelp)[BLU],
		x, y
		);
	return;
	}

hl_Postprocess()
	{	static RGBpixel	black_pixel = { 0, 0, 0 };
		static RGBpixel	white_pixel = { 255, 255, 255 };
		register RGBpixel *fgp = (RGBpixel *)
				(reverse_video ? black_pixel : white_pixel);
		register RGBpixel *bgp = (RGBpixel *)
				(reverse_video ? white_pixel : black_pixel);
		static RGBpixel bufa[1024], bufb[1024];
		register int	x, y;
		unsigned short	maxdist = (cell_sz*ARCTAN_87)+2;
	prnt_Event( "Making hidden-line drawing..." );
	for( y = 0; y < grid_sz && ! user_interrupt; y++ )
		{	static RGBpixel	*rpixp;
			static RGBpixel	*lpixp;
			
		rpixp = is_Odd(y) ? bufb : bufa;
		lpixp = is_Odd(y) ? bufa : bufb;
		(void) fb_seek( fbiop, 0, y );
		for(	x = 0;
			x < grid_sz && ! user_interrupt;
			x++
			)
			{
			if( fb_rpixel( fbiop, rpixp[x] ) == -1 )
				{
				fb_log(
			"hl_Postprocess: Failed to read pixel <%d,%d>\n",
					x, y );
				return;
				}
			/*prnt_Pixel( rpixp[x], x, y );*/
			if( x == 0 )
				HL_SETBIT( x, y );
			else
			if( y == 0 )
				if( hl_Norm_Diff( rpixp[x], rpixp[x-1] ) )
					HL_CLRBIT( x, y );
				else
					HL_SETBIT( x, y );
			else
			if(  (hl_regmap != NULL &&
				(hl_Reg_Diff( x, y, x-1, y )
			     ||	 hl_Reg_Diff( x, y, x, y-1 )))
			  || (hl_dstmap != NULL &&
				(hl_Dst_Diff( x, y, x-1, y, maxdist )
			     ||	 hl_Dst_Diff( x, y, x, y-1, maxdist )))
			  ||	hl_Norm_Diff( rpixp[x], rpixp[x-1] )
			  ||	hl_Norm_Diff( rpixp[x], lpixp[x] )
				)
				HL_CLRBIT( x, y );
			else
				HL_SETBIT( x, y );
			}
		}
	for( y = 0; y < grid_sz && ! user_interrupt; y++ )
		{
		(void) fb_seek( fbiop, 0, y );
		for( x = 0; x < grid_sz && ! user_interrupt; x++ )
			{
			if( HL_TSTBIT( x, y ) )
				{
				FB_WPIXEL( fbiop, *fgp );
				}
			else
				{
				FB_WPIXEL( fbiop, *bgp );
				}
			}
		}
	(void) fb_flush( fbiop );
	return;
	}

/*	v i e w _ p i x ( )						*/
STATIC void
view_pix( ap, scanbuf )
register struct application	*ap;
RGBpixel		scanbuf[];
	{	RGBpixel	pixel;
		int		x = ap->a_x + x_fb_origin;
		int		y = ap->a_y + y_fb_origin;
	if( rt_g.debug && tty )
		{
		RES_ACQUIRE( &rt_g.res_syscall );
		(void) sprintf( GRID_PIX_PTR, " [%04d-", ap->a_x/aperture_sz );
		prnt_Timer( (char *) NULL );
		IDLE_MOVE();
		(void) fflush( stdout );
		RES_RELEASE( &rt_g.res_syscall );
		}
	/* Clip relative intensity on each gun to range 0.0 to 1.0;
		then scale to RGB values.				*/
	pixel[RED] = ap->a_color[0] > 1.0 ? 255 :
			(ap->a_color[0] < 0.0 ? 0 : ap->a_color[0] * 255);
	pixel[GRN] = ap->a_color[1] > 1.0 ? 255 :
			(ap->a_color[1] < 0.0 ? 0 : ap->a_color[1] * 255);
	pixel[BLU] = ap->a_color[2] > 1.0 ? 255 :
			(ap->a_color[2] < 0.0 ? 0 : ap->a_color[2] * 255);
	if( anti_aliasing )
		{
		x = ap->a_x / aperture_sz + x_fb_origin;
		y = ap->a_y / aperture_sz + y_fb_origin;
		pixel[RED] = (int)((fastf_t) pixel[RED] / sample_sz);
		pixel[GRN] = (int)((fastf_t) pixel[GRN] / sample_sz);
		pixel[BLU] = (int)((fastf_t) pixel[BLU] / sample_sz);
		if( ap->a_x % aperture_sz || ap->a_y % aperture_sz )
			{	RGBpixel	tpixel;
			/* Read accumulator pixel per buffering scheme.	*/
			switch( pix_buffered )
				{
			case B_PIO :
				if( fb_read( fbiop, x, y, tpixel, 1 ) == -1 )
					{
					rt_log( "Read failed from pixel <%d,%d>.\n",
						x, y
						);
					return;
					}
				break;
			case B_PAGE :
				if(	fb_seek( fbiop, x, y ) == -1
				    ||	fb_rpixel( fbiop, tpixel ) == -1
					)
					{
					rt_log( "Read failed from pixel <%d,%d>.\n",
						x, y
						);
					return;
					}
				break;
			case B_LINE :
				COPYRGB( tpixel, scanbuf[x] );
				break;
			default :
				rt_log( "unknown buffering scheme %d\n",
					pix_buffered );
				return;
				}
			/* Add current RGB values to accumulator pixel.	*/
			pixel[RED] += tpixel[RED];
			pixel[GRN] += tpixel[GRN];
			pixel[BLU] += tpixel[BLU];
			}
		}
	/* Write out pixel, depending on buffering scheme.		*/
	if( query_region )
		return;
	switch( pix_buffered )
		{
	case B_PIO :
		if( fb_write( fbiop, x, y, pixel, 1 ) != -1 )
			/* Programmed I/O to frame buffer (if possible).*/
			return;
		break;
	case B_PAGE :
		/* Buffered writes to frame buffer.			*/
		if( fb_seek( fbiop, x, y ) != -1 )
			{ /* WARNING: no error checking. */
			FB_WPIXEL( fbiop, pixel );
			return;
			}
		break;
	case B_LINE :
		COPYRGB( scanbuf[x], pixel );
		return;
	default :
		rt_log( "unknown buffering scheme %d\n",
			pix_buffered );
		return;
		}
	rt_log( "Write failed to pixel <%d,%d>.\n", x, y );
	return;
	}

/*	v i e w _ b o l ( )						*/
STATIC void
view_bol( ap )
register struct application	*ap;
	{	int	x = grid_x_org + x_fb_origin;
		int	y = ap->a_y/aperture_sz + y_fb_origin;
	if( tracking_cursor )
		{
		RES_ACQUIRE( &rt_g.res_stats );
		(void) fb_cursor( fbiop, 1, x, y );
		RES_RELEASE( &rt_g.res_stats );
		}
	if( tty )
		{
		RES_ACQUIRE( &rt_g.res_stats );
		(void) sprintf( GRID_SCN_PTR, "%04d-", ap->a_y/aperture_sz );
		(void) sprintf( GRID_PIX_PTR, " [%04d-", ap->a_x/aperture_sz );
		update_Screen();
		RES_RELEASE( &rt_g.res_stats );
		}
	return;
	}

/*	v i e w _ e o l ( )						*/
STATIC void
view_eol( ap, scanbuf )
register struct application	*ap;
RGBpixel			scanbuf[];
	{	int	x = grid_x_org + x_fb_origin;
		int	y = ap->a_y/aperture_sz + y_fb_origin;
		int	ct = (ap->a_x - grid_x_org)/aperture_sz;
	/* Reset horizontal pixel position.				*/
	ap->a_x = grid_x_org * aperture_sz;

	if( tty )
		{
		RES_ACQUIRE( &rt_g.res_stats );
		prnt_Timer( (char *) NULL );
		IDLE_MOVE();
		(void) fflush( stdout );
		RES_RELEASE( &rt_g.res_stats );
		}
	else
		{	char	grid_y[5];
		RES_ACQUIRE( &rt_g.res_stats );
		(void) sprintf( grid_y, "%04d", ap->a_y/aperture_sz );
		prnt_Timer( grid_y );
		RES_RELEASE( &rt_g.res_stats );
		}
	if( query_region )
		return;
	if( pix_buffered == B_LINE )
		{
		RES_ACQUIRE( &rt_g.res_stats );
		if( strcmp( fb_file, "/dev/remote" ) == 0 )
			{	char	ystr[5];
			(void) sprintf( ystr, "%04d", ap->a_y );
			if(	write( 1, ystr, 4 ) != 4
			    ||	write(	1,
					(char *)(scanbuf+x),
					ct*sizeof(RGBpixel)
					) != ct*sizeof(RGBpixel)
				)
				{
				rt_log( "Write of scan line %d failed.\n", ap->a_y );
				perror( "write" );
				}
			}
		else
		if( fb_write( fbiop, x, y, scanbuf+x, ct ) == -1 )
			rt_log( "Write of scan line (%d) failed.\n", ap->a_y );
		RES_RELEASE( &rt_g.res_stats );
		}
	return;
	}

/*	v i e w _ e n d ( )						*/
STATIC void
view_end()
	{
	if( pix_buffered == B_PAGE )
		fb_flush( fbiop );
	if( rt_g.debug & DEBUG_REFRACT )
		rt_log( "Refraction stats : hits=%d misses=%d inside=%d total=%d\n",
			refrac_total-(refrac_missed+refrac_inside),
			refrac_missed, refrac_inside, refrac_total
			);
	if( rt_g.debug & DEBUG_SHADOW )
		rt_log( "Shadowing stats : lit=%d shadowed=%d total=%d\n",
			hits_lit, hits_shadowed, hits_lit+hits_shadowed
			);
	if( hiddenln_draw )
		{
		if( ! user_interrupt )
			hl_Postprocess();
		if( hl_regmap != NULL )
			{
			free( (char *) hl_regmap );
			hl_regmap = NULL;
			}
		if( hl_dstmap != NULL )
			{
			free( (char *) hl_dstmap );
			hl_dstmap = NULL;
			}
		}
	prnt_Timer( "VIEW" );
	return;
	}

#define	SIGMA	1/(sqrt(2)*1.13794)
/*	g a u s s_ w g t _ f u n c ( ) by Douglas A. Gwyn
	Gaussian weighting function.

	r = distance from center of beam
	B = beam finite radius (contains 90% of the total energy)

	R = r / B	definition

	I = intensity of beam at desired position
	  =  e^(- R^2 / log10(e)) / (log10(e) * Pi)

	Note that the intensity at the center of the beam is not 1 but
	rather log(10)/Pi.  This is because the intensity is normalized
	so that its integral over the infinite plane is 1.
 */
fastf_t
gauss_Wgt_Func( R )
fastf_t	R;
	{
	return	exp( - Sqr( R ) / LOG10E ) / (LOG10E * PI);
	}

STATIC int
f_Overlap( ap, pp, reg1, reg2 )
register struct application	*ap;
register struct partition	*pp;
struct region			*reg1, *reg2;
	{	point_t	pt;
		fastf_t	depth = pp->pt_outhit->hit_dist - pp->pt_inhit->hit_dist;
	if( depth < OVERLAPTOL )
		return	1;
	VJOIN1( pt, ap->a_ray.r_pt, pp->pt_inhit->hit_dist,
		ap->a_ray.r_dir );
	rt_log( "OVERLAP:\n" );
	rt_log( "reg=%s sol=%s,\n",
		reg1->reg_name, pp->pt_inseg->seg_stp->st_name
		);
	rt_log( "reg=%s sol=%s,\n",
		reg2->reg_name, pp->pt_outseg->seg_stp->st_name
		);
	rt_log( "(x%d y%d lvl%d) depth %gmm at (%g,%g,%g)\n",
		ap->a_x, ap->a_y, ap->a_level,
		depth,
		pt[X], pt[Y], pt[Z]
		);
	return	1;
	}

STATIC int
/*ARGSUSED*/
f_NulOverlap( ap, pp, reg1, reg2 )
struct application	*ap;
struct partition	*pp;
struct region		*reg1, *reg2;
	{
	return	1;
	}
