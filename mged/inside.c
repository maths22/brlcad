/*
 *			I N S I D E 
 *
 *	Given an outside solid and desired thicknesses, finds
 *	an inside solid to produce those thicknesses.
 *
 * Functions -
 *	f_inside	reads all the input required
 *	arbin		finds inside of arbs
 *	tgcin		finds inside of tgcs
 *	ellgin		finds inside of ellgs
 *	torin		finds inside of tors
 *
 *  Authors -
 *	Keith A Applin
 *
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1985 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <signal.h>
#include <stdio.h>
#include <math.h>
#ifdef BSD
#include <strings.h>
#else
#include <string.h>
#endif

#include "machine.h"
#include "vmath.h"
#include "db.h"			/* XXX needed for NAMESIZE */
#include "rtlist.h"
#include "rtgeom.h"
#include "raytrace.h"
#include "./ged.h"
#include "./sedit.h"
#include "externs.h"
#include "./solid.h"
#include "./dm.h"

extern struct rt_db_internal	es_int;	/* from edsol.c */

extern int	args;		/* total number of args available */
extern int	argcnt;		/* holder for number of args added later */
extern int	newargs;	/* number of args from getcmd() */
extern int	numargs;	/* number of args */
extern char	*cmd_args[];	/* array of pointers to args */
extern char	**promp;	/* pointer to a pointer to a char */

static char *p_arb4[] = {
	"Enter thickness for face 123: ",
	"Enter thickness for face 124: ",
	"Enter thickness for face 234: ",
	"Enter thickness for face 134: ",
};

static char *p_arb5[] = {
	"Enter thickness for face 1234: ",
	"Enter thickness for face 125: ",
	"Enter thickness for face 235: ",
	"Enter thickness for face 345: ",
	"Enter thickness for face 145: ",
};

static char *p_arb6[] = {
	"Enter thickness for face 1234: ",
	"Enter thickness for face 2356: ",
	"Enter thickness for face 1564: ",
	"Enter thickness for face 125: ",
	"Enter thickness for face 346: ",
};

static char *p_arb7[] = {
	"Enter thickness for face 1234: ",
	"Enter thickness for face 567: ",
	"Enter thickness for face 145: ",
	"Enter thickness for face 2376: ",
	"Enter thickness for face 1265: ",
	"Enter thickness for face 3475: ",
};

static char *p_arb8[] = {
	"Enter thickness for face 1234: ",
	"Enter thickness for face 5678: ",
	"Enter thickness for face 1485: ",
	"Enter thickness for face 2376: ",
	"Enter thickness for face 1265: ",
	"Enter thickness for face 3478: ",
};

static char *p_tgcin[] = {
	"Enter thickness for base (AxB): ",
	"Enter thickness for top (CxD): ",
	"Enter thickness for side: ",
};


/*	F _ I N S I D E ( ) :	control routine...reads all data
 */
void
f_inside()
{
	register int i;
	struct directory	*dp;
	struct directory	*outdp;
	mat_t newmat;
	int	cgtype;		/* cgtype ARB 4..8 */
	int	p1, p2, p3;
	int	nface;
	fastf_t	thick[6];
	plane_t	planes[6];
	struct rt_db_internal	intern;
	char	*newname;
	struct rt_tol	tol;

	/* XXX These need to be improved */
	tol.magic = RT_TOL_MAGIC;
	tol.dist = 0.005;
	tol.dist_sq = tol.dist * tol.dist;
	tol.perp = 1e-6;
	tol.para = 1 - tol.perp;

	(void)signal( SIGINT, sig2);	/* allow interrupts */

	/* don't use any arguments entered initially 
	 * will explicitly ask for data as needed
	 */
	args = numargs;
	argcnt = 0;

	/* SCHEME:
	 *	if in solid edit, use "edited" solid
	 *	if in object edit, use "key" solid
	 *	else get solid name to use
	 */

	if( state == ST_S_EDIT ) {
		/* solid edit mode */
		/* apply es_mat editing to parameters */
		transform_editing_solid( &intern, es_mat, &es_int, 0 );
		outdp = illump->s_path[illump->s_last];

		(void)printf("Outside solid: ");
		for(i=0; i <= illump->s_last; i++) {
			(void)printf("/%s",illump->s_path[i]->d_namep);
		}
		(void)printf("\n");
	}  else if( state == ST_O_EDIT ) {
		/* object edit mode */
		if( illump->s_Eflag ) {
			(void)printf("Cannot find inside of a processed (E'd) region\n");
			return;
		}
		/* use the solid at bottom of path (key solid) */
		/* apply es_mat and modelchanges editing to parameters */
		mat_mul(newmat, modelchanges, es_mat);
		transform_editing_solid( &intern, newmat, &es_int, 0 );
		outdp = illump->s_path[illump->s_last];

		(void)printf("Outside solid: ");
		for(i=0; i <= illump->s_last; i++) {
			(void)printf("/%s",illump->s_path[i]->d_namep);
		}
		(void)printf("\n");
	} else {
		/* Not doing any editing....ask for outside solid */
		(void)printf("Enter name of outside solid: ");
		argcnt = getcmd(args);
		if( (outdp = db_lookup( dbip,  cmd_args[args], LOOKUP_NOISY )) == DIR_NULL )  
			return;
		args += argcnt;
		if( rt_db_get_internal( &intern, outdp, dbip, rt_identity ) < 0 )
			READ_ERR_return;
	}
	if( intern.idb_type == ID_ARB8 )  {
		/* find the comgeom arb type, & reorganize */
		if( (cgtype = rt_arb_get_cgtype( &intern, &tol )) <= 0 ) {
			rt_log("%s: BAD ARB\n",outdp->d_namep);
			return;
		}

		/* must find new plane equations to account for
		 * any editing in the es_mat matrix or path to this solid.
		 */
		if( rt_arb_calc_planes( planes, intern.idb_ptr, cgtype, &tol ) < 0 )  {
			rt_log("rt_arb_calc_planes(%s): failued\n", outdp->d_namep);
			return;
		}
	}
	/* "intern" is now loaded with the outside solid data */

	/* get the inside solid name */
	argcnt=0;
	while( !argcnt ){		/* no command yet */
		(void)printf("Enter name of the inside solid: ");
		argcnt = getcmd(args);
	}
	if( db_lookup( dbip,  cmd_args[args], LOOKUP_QUIET ) != DIR_NULL ) {
		aexists( cmd_args[args] );
		return;
	}
	if( (int)strlen(cmd_args[args]) >= NAMESIZE )  {
		(void)printf("Names are limited to %d characters\n", NAMESIZE-1);
		return;
	}
	newname = cmd_args[args];

	args += argcnt;

	/* get thicknesses and calculate parameters for newrec */
	switch( intern.idb_type )  {

	case ID_ARB8:
	    {
	    	struct rt_arb_internal *arb =
			(struct rt_arb_internal *)intern.idb_ptr;

		nface = 6;

		switch( cgtype ) {
		case 8:
			promp = p_arb8;
			break;

		case 7:
			promp = p_arb7;
			break;

		case 6:
			promp = p_arb6;
			nface = 5;
			VMOVE( arb->pt[5], arb->pt[6] );
			break;

		case 5:
			promp = p_arb5;
			nface = 5;
			break;

		case 4:
			promp = p_arb4;
			nface = 4;
			VMOVE( arb->pt[3], arb->pt[4] );
			break;
		}

		for(i=0; i<nface; i++) {
			(void)printf("%s",promp[i]);
			if( (argcnt = getcmd( args )) < 0 )
				return;
			thick[i] = atof(cmd_args[args]) * local2base;
			args += argcnt;
		}

		if( arbin(&intern, thick, nface, cgtype, planes) )
			return;
		break;
	    }

	case ID_TGC:
		promp = p_tgcin;
		for(i=0; i<3; i++) {
			(void)printf("%s",promp[i]);
			if( (argcnt = getcmd(args)) < 0 )
				return;
			thick[i] = atof( cmd_args[args] ) * local2base;
			args += argcnt;
		}

		if( tgcin(&intern, thick) )
			return;
		break;

	case ID_ELL:
		(void)printf("Enter desired thickness: ");
		if( (argcnt = getcmd(args)) < 0 )
			return;
		thick[0] = atof( cmd_args[args] ) * local2base;

		if( ellgin(&intern, thick) )
			return;
		break;

	case ID_TOR:
		(void)printf("Enter desired thickness: ");
		if( (argcnt = getcmd(args)) < 0 )
			return;
		thick[0] = atof( cmd_args[args] ) * local2base;

		if( torin(&intern, thick) )
			return;
		break;

	default:
		(void)printf("Cannot find inside for '%s' solid\n",
			rt_functab[intern.idb_type].ft_name );
		return;
	}

	/* don't allow interrupts while we update the database! */
	(void)signal( SIGINT, SIG_IGN);
 
	/* Add to in-core directory */
	if( (dp = db_diradd( dbip,  newname, -1, 0, DIR_SOLID )) == DIR_NULL )  {
	    	ALLOC_ERR_return;
	}
	if( rt_db_put_internal( dp, dbip, &intern ) < 0 )
		WRITE_ERR_return;

	/* Draw the new solid */
	{
		char	*arglist[3];
		arglist[0] = "e";
		arglist[1] = newname;
		f_edit( 2, arglist );
	}
}



/* finds inside arbs */
int
arbin(ip, thick, nface, cgtype, planes)
struct rt_db_internal	*ip;
fastf_t	thick[6];
int	nface;
int	cgtype;		/* # of points, 4..8 */
plane_t	planes[6];
{
	struct rt_arb_internal	*arb = (struct rt_arb_internal *)ip->idb_ptr;
	point_t		center_pt;
	int		i;

	RT_ARB_CK_MAGIC(arb);

	/* find reference point (center_pt[3]) to find direction of normals */
	rt_arb_centroid( center_pt, arb, cgtype );

	/* move new face planes for the desired thicknesses */
	for(i=0; i<nface; i++) {
		if( (planes[i][3] - VDOT(center_pt, &planes[i][0])) > 0.0 )
			thick[i] *= -1.0;
		planes[i][3] += thick[i];
	}

	/* find the new vertices by intersecting the new face planes */
	for(i=0; i<8; i++) {
		if( rt_arb_3face_intersect( arb->pt[i], planes, cgtype, i ) < 0 )  {
			(void)printf("cannot find inside arb\n");
			return(1);
		}
	}
	return(0);
}

/* finds inside of tgc */
int
tgcin(ip, thick)
struct rt_db_internal	*ip;
fastf_t	thick[6];
{
	struct rt_tgc_internal	*tgc = (struct rt_tgc_internal *)ip->idb_ptr;
	fastf_t	mag[5], nmag[5];
	vect_t	unitH, unitA, unitB;
	vect_t	unitC, unitD;
	vect_t	hwork;
	fastf_t	aa[2], ab[2], ba[2], bb[2];
	fastf_t	dt, h1, h2, ht, dtha, dthb;
	fastf_t	s, d4, d5, ctan, t3;
	register int i, j, k;
	double ratio;

	thick[3] = thick[2];
	RT_TGC_CK_MAGIC(tgc);

	mag[0] = MAGNITUDE( tgc->h );
	mag[1] = MAGNITUDE( tgc->a );
	mag[2] = MAGNITUDE( tgc->b );
	mag[3] = MAGNITUDE( tgc->c );
	mag[4] = MAGNITUDE( tgc->d );

	if( (ratio = (mag[2] / mag[1])) < .8 )
		thick[3] = thick[3] / (1.016447 * pow(ratio, .071834));
	VSCALE(unitH, tgc->h, 1.0/mag[0]);
	VSCALE(unitA, tgc->a, 1.0/mag[1]);
	VSCALE(unitB, tgc->b, 1.0/mag[2]);
	VSCALE(unitC, tgc->c, 1.0/mag[3]);
	VSCALE(unitD, tgc->d, 1.0/mag[4]);

	VCROSS(hwork, unitA, unitB);
	if( (dt = VDOT(hwork, unitH)) == 0.0 ) {
		(void)printf("BAD cylinder\n");
		return(1);
	}
	h1 = thick[0] / dt;
	h2 = thick[1] / dt;
	if( (ht = dt * mag[0]) == 0.0 ) {
		(void)printf("Cannot find the inside cylinder\n");
		return(1);
	}
	dtha = VDOT(unitA, tgc->h);
	dthb = VDOT(unitB, tgc->h);
	s = 1.0;
	for(k=0; k<2; k++) {
		if( k )
			s = -1.0;
		d4 = s * dtha + mag[3];
		d5 = s * dthb + mag[4];
		ctan = (mag[1] - d4) / ht;
		t3 = thick[2] * sqrt( (mag[1] - d4)*(mag[1] - d4) + ht*ht ) / ht;
		aa[k] = mag[1] - t3 - (thick[0]*ctan);
		ab[k] = d4 - t3 + (thick[1]*ctan);

		ctan = (mag[2] - d5) / ht;
		t3 = thick[3] * sqrt( (mag[2]-d5)*(mag[2]-d5) + ht*ht ) / ht;
		ba[k] = mag[2] - t3 - (thick[0]*ctan);
		bb[k] = d5 - t3 + (thick[1]*ctan);
	}

	nmag[0] = mag[0] - (h1+h2);
	nmag[1] = ( aa[0] + aa[1] ) * .5;
	nmag[2] = ( ba[0] + ba[1] ) * .5;
	nmag[3] = (ab[0] + ab[1]) * .5;
	nmag[4] = (bb[0] + bb[1]) * .5;
	VSCALE(tgc->h, unitH, nmag[0] );
	VSCALE(tgc->a, unitA, nmag[1] );
	VSCALE(tgc->b, unitB, nmag[2] );
	VSCALE(tgc->c, unitC, nmag[3] );
	VSCALE(tgc->d, unitD, nmag[4] );

	for( i=0; i<3; i++ )  {
		tgc->v[i] += unitH[i]*h1 + .5 *
		    ( (aa[0] - aa[1])*unitA[i] + (ba[0] - ba[1])*unitB[i] );
	}
	return(0);
}


/* finds inside of torus */
int
torin(ip, thick)
struct rt_db_internal	*ip;
fastf_t			thick[6];
{
	struct rt_tor_internal	*tor = (struct rt_tor_internal *)ip->idb_ptr;

	RT_TOR_CK_MAGIC(tor);
	if( thick[0] == 0.0 )
		return(0);

	if( thick[0] < 0 ) {
		if( (tor->r_h - thick[0]) > (tor->r_a + .01) ) {
			(void)printf("cannot do: r2 > r1\n");
			return(1);
		}
	}
	if( thick[0] >= tor->r_h ) {
		(void)printf("cannot do: r2 <= 0\n");
		return(1);
	}

	tor->r_h = tor->r_h - thick[0];
	return(0);
}


/* finds inside ellg */
int
ellgin(ip, thick)
struct rt_db_internal	*ip;
fastf_t	thick[6];
{
	struct rt_ell_internal	*ell = (struct rt_ell_internal *)ip->idb_ptr;
	int i, j, k, order[3];
	fastf_t mag[3], nmag[3];
	fastf_t ratio;

	if( thick[0] <= 0.0 ) 
		return(0);
	thick[2] = thick[1] = thick[0];	/* uniform thickness */

	RT_ELL_CK_MAGIC(ell);
	mag[0] = MAGNITUDE(ell->a);
	mag[1] = MAGNITUDE(ell->b);
	mag[2] = MAGNITUDE(ell->c);

	for(i=0; i<3; i++) {
		order[i] = i;
	}

	for(i=0; i<2; i++) {
		k = i + 1;
		for(j=k; j<3; j++) {
			if(mag[i] < mag[j])
				order[i] = j;
		}
	}

	if( (ratio = mag[order[1]] / mag[order[0]]) < .8 )
		thick[order[1]] = thick[order[1]]/(1.016447*pow(ratio,.071834));
	if( (ratio = mag[order[2]] / mag[order[1]]) < .8 )
		thick[order[2]] = thick[order[2]]/(1.016447*pow(ratio,.071834));

	for(i=0; i<3; i++) {
		if( (nmag[i] = mag[i] - thick[i]) <= 0.0 )
			(void)printf("Warning: new vector [%d] length <= 0 \n", i);
	}
	VSCALE(ell->a, ell->a, nmag[0]/mag[0]);
	VSCALE(ell->b, ell->b, nmag[1]/mag[1]);
	VSCALE(ell->c, ell->c, nmag[2]/mag[2]);
	return(0);
}
