/*
	Author:		Gary S. Moss
			U. S. Army Ballistic Research Laboratory
			Aberdeen Proving Ground
			Maryland 21005-5066
			(301)278-6647 or AV-298-6647
*/
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>
#include "machine.h"
#include "vmath.h"
#include "raytrace.h"
#include "./vecmath.h"
#include "./burst.h"
#include "./extern.h"
#define R_GRID	255
#define G_GRID	255
#define B_GRID	0
#define R_BURST	255
#define G_BURST	0
#define B_BURST	0

void
plotInit()
	{	int	x1, y1, z1, x2, y2, z2;
	if( plotfp == NULL )
		return;
	x1 = (int) rtip->mdl_min[X] - 1;
	y1 = (int) rtip->mdl_min[Y] - 1;
	z1 = (int) rtip->mdl_min[Z] - 1;
	x2 = (int) rtip->mdl_max[X] + 1;
	y2 = (int) rtip->mdl_max[Y] + 1;
	z2 = (int) rtip->mdl_max[Z] + 1;
	pl_3space( plotfp, x1, y1, z1, x2, y2, z2 );
	return;
	}

void
plotGrid( r_pt )
register fastf_t	*r_pt;
	{
	if( plotfp == NULL )
		return;
	pl_color( plotfp, R_GRID, G_GRID, B_GRID );
	pl_3point( plotfp, (int) r_pt[X], (int) r_pt[Y], (int) r_pt[Z] );
	return;
	}

void
plotRay( rayp )
register struct xray	*rayp;
	{	int	endpoint[3];
	if( plotfp == NULL )
		return;
	VJOIN1( endpoint, rayp->r_pt, cellsz, rayp->r_dir );
	RES_ACQUIRE( &rt_g.res_syscall );
	pl_color( plotfp, R_BURST, G_BURST, B_BURST );
#if 0
	pl_3line(	plotfp,
			(int) rayp->r_pt[X],
			(int) rayp->r_pt[Y],
			(int) rayp->r_pt[Z],
			endpoint[X],
			endpoint[Y],
			endpoint[Z]
			);
#else
	pl_3point( plotfp, (int) endpoint[X], (int) endpoint[Y], (int) endpoint[Z] );
#endif
	RES_RELEASE( &rt_g.res_syscall );
	return;
	}

void
plotPartition( ihitp, ohitp, rayp, regp )
struct hit		*ihitp;
register struct hit	*ohitp ;
register struct xray	*rayp;
struct region		*regp;
	{
	if( plotfp == NULL )
		return;
#if 0
	if(	regp->reg_mater.ma_rgb[0] == 0
	    &&	regp->reg_mater.ma_rgb[1] == 0
	    &&	regp->reg_mater.ma_rgb[2] == 0
		)
		pl_color( plotfp, 255, 255, 255 );
	else
		pl_color(	plotfp,
				(int) regp->reg_mater.ma_rgb[0],
				(int) regp->reg_mater.ma_rgb[1],
				(int) regp->reg_mater.ma_rgb[2]
				);
#endif
	RES_ACQUIRE( &rt_g.res_syscall );
	pl_3line(	plotfp,
			(int) ihitp->hit_point[X],
			(int) ihitp->hit_point[Y],
			(int) ihitp->hit_point[Z],
			(int) ohitp->hit_point[X],
			(int) ohitp->hit_point[Y],
			(int) ohitp->hit_point[Z]
			);
	RES_RELEASE( &rt_g.res_syscall );
	return;
	}

void
plotShieldComp( rayp, qp )
register struct xray	*rayp;
register Pt_Queue	*qp;
	{	register struct hit	*ohitp;
	if( qp == PT_Q_NULL )
		return;
	plotShieldComp( rayp, qp->q_next );

	/* Fill in hit point and normal.				*/
	ohitp = qp->q_part->pt_outhit;
	VJOIN1( ohitp->hit_point, rayp->r_pt, ohitp->hit_dist, rayp->r_dir );
	colorPartition( qp->q_part->pt_regionp, C_SHIELD );
	plotPartition( qp->q_part->pt_inhit, ohitp, rayp, qp->q_part->pt_regionp );
	return;
	}
