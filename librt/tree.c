/*
 *			T R E E
 *
 * Ray Tracing program, GED tree tracer.
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
 *	This software is Copyright (C) 1985 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCStree[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>
#include "../h/machine.h"
#include "../h/vmath.h"
#include "../h/db.h"
#include "../h/raytrace.h"
#include "rtdir.h"
#include "debug.h"

struct rt_g rt_g;
struct rt_i rt_i;	/* eventually, malloc'ed by rt_dir_build */

int rt_pure_boolean_expressions = 0;

HIDDEN union tree *rt_drawobj();
HIDDEN void rt_add_regtree();
HIDDEN union tree *rt_make_bool_tree();

extern int nul_prep(),	nul_print(), nul_norm(), nul_uv();
extern int tor_prep(),	tor_print(), tor_norm(), tor_uv();
extern int tgc_prep(),	tgc_print(), tgc_norm(), tgc_uv();
extern int ell_prep(),	ell_print(), ell_norm(), ell_uv();
extern int arb_prep(),	arb_print(), arb_norm(), arb_uv();
extern int haf_prep(),	haf_print(), haf_norm(), haf_uv();
extern int ars_prep(),  ars_print(), ars_norm(), ars_uv();
extern int rec_prep(),	rec_print(), rec_norm(), rec_uv();
extern int pg_prep(),	pg_print(),  pg_norm(),  pg_uv();
extern int spl_prep(),	spl_print(), spl_norm(), spl_uv();

extern struct seg *nul_shot();
extern struct seg *tor_shot();
extern struct seg *tgc_shot();
extern struct seg *ell_shot();
extern struct seg *arb_shot();
extern struct seg *ars_shot();
extern struct seg *haf_shot();
extern struct seg *rec_shot();
extern struct seg *pg_shot();
extern struct seg *spl_shot();

struct rt_functab rt_functab[] = {
	nul_prep, nul_shot, nul_print, nul_norm, nul_uv, "ID_NULL",
	tor_prep, tor_shot, tor_print, tor_norm, tor_uv, "ID_TOR",
	tgc_prep, tgc_shot, tgc_print, tgc_norm, tgc_uv, "ID_TGC",
	ell_prep, ell_shot, ell_print, ell_norm, ell_uv, "ID_ELL",
	arb_prep, arb_shot, arb_print, arb_norm, arb_uv, "ID_ARB8",
	ars_prep, ars_shot, ars_print, ars_norm, ars_uv, "ID_ARS",
	haf_prep, haf_shot, haf_print, haf_norm, haf_uv, "ID_HALF",
	rec_prep, rec_shot, rec_print, rec_norm, rec_uv, "ID_REC",
	pg_prep,  pg_shot,  pg_print,  pg_norm,  pg_uv,  "ID_POLY",
	spl_prep, spl_shot, spl_print, spl_norm, spl_uv, "ID_BSPLINE",
	nul_prep, nul_shot, nul_print, nul_norm, nul_uv, ">ID_NULL"
};

/*
 *  Hooks for unimplemented routines
 */
#define DEF(func)	func() { rt_log("func unimplemented\n"); return(0); }

DEF(nul_prep); struct seg * DEF(nul_shot); DEF(nul_print); DEF(nul_norm); DEF(nul_uv);

/* To be replaced with code someday */
DEF(haf_prep); struct seg * DEF(haf_shot); DEF(haf_print); DEF(haf_norm); DEF(haf_uv);

/* Map for database solidrec objects to internal objects */
static char idmap[] = {
	ID_NULL,	/* undefined, 0 */
	ID_NULL,	/*  RPP	1 axis-aligned rectangular parallelopiped */
	ID_NULL,	/* BOX	2 arbitrary rectangular parallelopiped */
	ID_NULL,	/* RAW	3 right-angle wedge */
	ID_NULL,	/* ARB4	4 tetrahedron */
	ID_NULL,	/* ARB5	5 pyramid */
	ID_NULL,	/* ARB6	6 extruded triangle */
	ID_NULL,	/* ARB7	7 weird 7-vertex shape */
	ID_NULL,	/* ARB8	8 hexahedron */
	ID_NULL,	/* ELL	9 ellipsoid */
	ID_NULL,	/* ELL1	10 another ellipsoid ? */
	ID_NULL,	/* SPH	11 sphere */
	ID_NULL,	/* RCC	12 right circular cylinder */
	ID_NULL,	/* REC	13 right elliptic sylinder */
	ID_NULL,	/* TRC	14 truncated regular cone */
	ID_NULL,	/* TEC	15 truncated elliptic cone */
	ID_TOR,		/* TOR	16 toroid */
	ID_NULL,	/* TGC	17 truncated general cone */
	ID_TGC,		/* GENTGC 18 supergeneralized TGC; internal form */
	ID_ELL,		/* GENELL 19: V,A,B,C */
	ID_ARB8,	/* GENARB8 20:  V, and 7 other vectors */
	ID_NULL,	/* ARS 21: arbitrary triangular-surfaced polyhedron */
	ID_NULL,	/* ARSCONT 22: extension record type for ARS solid */
	ID_NULL		/* n+1 */
};

HIDDEN char *rt_path_str();

static struct mater_info rt_no_mater;

/*
 *  			R T _ G E T _ T R E E
 *
 *  User-called function to add a tree hierarchy to the displayed set.
 *  
 *  Returns -
 *  	0	Ordinarily
 *	-1	On major error
 */
int
rt_gettree(node)
char *node;
{
	register union tree *curtree;
	register struct directory *dp;
	mat_t	mat;

	if(!rt_i.needprep)
		rt_bomb("rt_gettree called again after rt_prep!");

	mat_idn( mat );

	dp = rt_dir_lookup( node, LOOKUP_NOISY );
	if( dp == DIR_NULL )
		return(-1);		/* ERROR */

	curtree = rt_drawobj( dp, REGION_NULL, 0, mat, &rt_no_mater );
	if( curtree != TREE_NULL )  {
		/*  Subtree has not been contained by a region.
		 *  This should only happen when a top-level solid
		 *  is encountered.  Build a special region for it.
		 */
		register struct region *regionp;	/* XXX */

		GETSTRUCT( regionp, region );
		rt_log("Warning:  Top level solid, region %s created\n",
			rt_path_str(0) );
		if( curtree->tr_op != OP_SOLID )
			rt_bomb("root subtree not Solid");
		regionp->reg_name = rt_strdup(rt_path_str(0));
		rt_add_regtree( regionp, curtree );
	}
	return(0);	/* OK */
}

static vect_t xaxis = { 1.0, 0, 0, 0 };
static vect_t yaxis = { 0, 1.0, 0, 0 };
static vect_t zaxis = { 0, 0, 1.0, 0 };

/*
 *			R T _ A D D _ S O L I D
 */
HIDDEN
struct soltab *
rt_add_solid( rec, name, mat )
union record *rec;
char	*name;
matp_t	mat;
{
	register struct soltab *stp;
	static vect_t v[8];
	static vect_t A, B, C;
	static fastf_t fx, fy, fz;
	FAST fastf_t f;
	register struct soltab *nsp;

	/* Validate that matrix preserves perpendicularity of axis */
	/* by checking that A.B == 0, B.C == 0, A.C == 0 */
	MAT4X3VEC( A, mat, xaxis );
	MAT4X3VEC( B, mat, yaxis );
	MAT4X3VEC( C, mat, zaxis );
	fx = VDOT( A, B );
	fy = VDOT( B, C );
	fz = VDOT( A, C );
	if( ! NEAR_ZERO(fx, 0.0001) ||
	    ! NEAR_ZERO(fy, 0.0001) ||
	    ! NEAR_ZERO(fz, 0.0001) )  {
		rt_log("rt_add_solid(%s):  matrix does not preserve axis perpendicularity.\n  X.Y=%f, Y.Z=%f, X.Z=%f\n",
			name, fx, fy, fz );
		mat_print("bad matrix", mat);
		return( SOLTAB_NULL );		/* BAD */
	}

	/*
	 *  Check to see if this exact solid has already been processed.
	 *  Match on leaf name and matrix.
	 */
	for( nsp = rt_i.HeadSolid; nsp != SOLTAB_NULL; nsp = nsp->st_forw )  {
		register int i;

		if(
			name[0] != nsp->st_name[0]  ||	/* speed */
			name[1] != nsp->st_name[1]  ||	/* speed */
			strcmp( name, nsp->st_name ) != 0
		)
			continue;
		for( i=0; i<16; i++ )  {
			f = mat[i] - nsp->st_pathmat[i];
			if( !NEAR_ZERO(f, 0.0001) )
				goto next_one;
		}
		/* Success, we have a match! */
		if( rt_g.debug & DEBUG_SOLIDS )
			rt_log("rt_add_solid:  %s re-referenced\n",
				name );
		return(nsp);
next_one: ;
	}

	GETSTRUCT(stp, soltab);
	switch( rec->u_id )  {
	case ID_SOLID:
		stp->st_id = idmap[rec->s.s_type];
		/* Convert from database (float) to fastf_t */
		rt_fastf_float( v, rec->s.s_values, 8 );
		break;
	case ID_ARS_A:
		stp->st_id = ID_ARS;
		break;
	case ID_P_HEAD:
		stp->st_id = ID_POLY;
		break;
	case ID_BSOLID:
		stp->st_id = ID_BSPLINE;
		break;
	default:
		rt_log("rt_add_solid:  u_id=x%x unknown\n", rec->u_id);
		free(stp);
		return( SOLTAB_NULL );		/* BAD */
	}
	stp->st_name = name;
	stp->st_specific = (int *)0;

	/* init solid's maxima and minima */
	stp->st_max[X] = stp->st_max[Y] = stp->st_max[Z] = -INFINITY;
	stp->st_min[X] = stp->st_min[Y] = stp->st_min[Z] =  INFINITY;

	if( rt_functab[stp->st_id].ft_prep( v, stp, mat, &(rec->s) ) )  {
		/* Error, solid no good */
		free(stp);
		return( SOLTAB_NULL );		/* BAD */
	}

	/* For now, just link them all onto the same list */
	stp->st_forw = rt_i.HeadSolid;
	rt_i.HeadSolid = stp;

	mat_copy( stp->st_pathmat, mat );

	/* Update the model maxima and minima */
#define TREE_MINMAX(a,b,c)	{ if( (c) < (a) )  a = (c);\
			if( (c) > (b) )  b = (c); }

#define TREE_MM(v)	TREE_MINMAX( rt_i.mdl_min[X], rt_i.mdl_max[X], v[X] ); \
		TREE_MINMAX( rt_i.mdl_min[Y], rt_i.mdl_max[Y], v[Y] ); \
		TREE_MINMAX( rt_i.mdl_min[Z], rt_i.mdl_max[Z], v[Z] )
	TREE_MM( stp->st_min );
	TREE_MM( stp->st_max );

	stp->st_bit = rt_i.nsolids++;
	if(rt_g.debug&DEBUG_SOLIDS)  {
		rt_log("-------------- %s (bit %d) -------------\n",
			stp->st_name, stp->st_bit );
		VPRINT("Bound Sph CENTER", stp->st_center);
		rt_log("Approx Sph Radius = %f\n", stp->st_aradius);
		rt_log("Bounding Sph Radius = %f\n", stp->st_bradius);
		VPRINT("Bound RPP min", stp->st_min);
		VPRINT("Bound RPP max", stp->st_max);
		rt_functab[stp->st_id].ft_print( stp );
	}
	return( stp );
}

/*
 * Note that while GED has a more limited MAXLEVELS, GED can
 * work on sub-trees, while RT must be able to process the full tree.
 * Thus the difference, and the large value here.
 */
#define	MAXLEVELS	64
static struct directory	*path[MAXLEVELS];	/* Record of current path */

struct tree_list {
	union tree *tl_tree;
	int	tl_op;
};

/*
 *			R T _ D R A W _ O B J
 *
 * This routine is used to get an object drawn.
 * The actual processing of solids is performed by rt_add_solid(),
 * but all transformations and region building is done here.
 *
 * NOTE that this routine is used recursively, so no variables may
 * be declared static.
 */
HIDDEN
union tree *
rt_drawobj( dp, argregion, pathpos, old_xlate, materp )
struct directory *dp;
struct region *argregion;
matp_t old_xlate;
struct mater_info *materp;
{
	auto union record rec;		/* local copy of this record */
	register int i;
	auto int j;
	union tree *curtree;		/* ptr to current tree top */
	struct region *regionp;
	union record *members;		/* ptr to array of member recs */
	int subtreecount;		/* number of non-null subtrees */
	struct tree_list *trees;	/* ptr to array of structs */
	struct tree_list *tlp;		/* cur tree_list */
	struct mater_info curmater;

	if( pathpos >= MAXLEVELS )  {
		rt_log("%s: nesting exceeds %d levels\n",
			rt_path_str(MAXLEVELS), MAXLEVELS );
		return(TREE_NULL);
	}
	path[pathpos] = dp;

	/*
	 * Load the first record of the object into local record buffer
	 */
	if( fseek( rt_i.fp, dp->d_addr, 0 ) < 0 ||
	    fread( (char *)&rec, sizeof rec, 1, rt_i.fp ) != 1 )  {
		rt_log("rt_drawobj: %s record read error\n",
			rt_path_str(pathpos) );
		return(TREE_NULL);
	}

	/*
	 *  Draw a solid
	 */
	if( rec.u_id == ID_SOLID || rec.u_id == ID_ARS_A ||
	    rec.u_id == ID_P_HEAD || rec.u_id == ID_BSOLID )  {
		register struct soltab *stp;
		register union tree *xtp;

		if( (stp = rt_add_solid( &rec, dp->d_namep, old_xlate )) ==
		    SOLTAB_NULL )
			return( TREE_NULL );

		/**GETSTRUCT( xtp, union tree ); **/
		if( (xtp=(union tree *)rt_malloc(sizeof(union tree), "solid tree"))
		    == TREE_NULL )
			rt_bomb("rt_drawobj: solid tree malloc failed\n");
		bzero( (char *)xtp, sizeof(union tree) );
		xtp->tr_op = OP_SOLID;
		xtp->tr_a.tu_stp = stp;
		xtp->tr_a.tu_name = rt_strdup(rt_path_str(pathpos));
		xtp->tr_regionp = argregion;
		return( xtp );
	}

	if( rec.u_id != ID_COMB )  {
		rt_log("rt_drawobj:  defective database record, type '%c'\n",
			rec.u_id );
		return(TREE_NULL);			/* ERROR */
	}

	/*
	 *  Process a Combination (directory) node
	 */
	if( rec.c.c_length <= 0 )  {
		rt_log(  "Warning: combination with zero members \"%.16s\".\n",
			rec.c.c_name );
		return(TREE_NULL);
	}
	regionp = argregion;

	/* Handle inheritance of material property */
	curmater = *materp;	/* struct copy */
	if( rec.c.c_override == 1 || rec.c.c_matname[0] != '\0' )  {
		if( argregion != REGION_NULL )  {
			rt_log("Error:  material property spec within region %s\n", argregion->reg_name );
		} else {
			if( rec.c.c_override == 1 )  {
				curmater.ma_override = 1;
				curmater.ma_rgb[0] = rec.c.c_rgb[0];
				curmater.ma_rgb[1] = rec.c.c_rgb[1];
				curmater.ma_rgb[2] = rec.c.c_rgb[2];
			}
			if( rec.c.c_matname[0] != '\0' )  {
				strncpy( curmater.ma_matname, rec.c.c_matname, sizeof(rec.c.c_matname) );
				strncpy( curmater.ma_matparm, rec.c.c_matparm, sizeof(rec.c.c_matparm) );
			}
		}
	}

	/* Handle combinations which are the top of a "region" */
	if( rec.c.c_flags == 'R' )  {
		if( argregion != REGION_NULL )  {
			if( rt_g.debug & DEBUG_REGIONS )
				rt_log("Warning:  region %s within region %s\n",
					rt_path_str(pathpos),
					argregion->reg_name );
/***			argregion = REGION_NULL;	/* override! */
		} else {
			register struct region *nrp;

			/* Ignore "air" regions unless wanted */
			if( rt_i.useair == 0 &&  rec.c.c_aircode != 0 )
				return(TREE_NULL);

			/* Start a new region here */
			GETSTRUCT( nrp, region );
			nrp->reg_forw = REGION_NULL;
			nrp->reg_regionid = rec.c.c_regionid;
			nrp->reg_aircode = rec.c.c_aircode;
			nrp->reg_gmater = rec.c.c_material;
			nrp->reg_name = rt_strdup(rt_path_str(pathpos));
			nrp->reg_mater = curmater;	/* struct copy */
			/* Material property processing in rt_add_regtree() */
			regionp = nrp;
		}
	}

	/* Read all the member records */
	i = sizeof(union record) * rec.c.c_length;
	j = sizeof(struct tree_list) * rec.c.c_length;
	if( (members = (union record *)rt_malloc(i, "member records")) ==
	    (union record *)0  ||
	    (trees = (struct tree_list *)rt_malloc( j, "tree_list array" )) ==
	    (struct tree_list *)0 )
		rt_bomb("rt_drawobj:  malloc failure\n");

	if( fread( (char *)members, sizeof(union record), rec.c.c_length,
	    rt_i.fp ) != rec.c.c_length )  {
		rt_log("rt_drawobj:  %s member read error\n",
			rt_path_str(pathpos) );
		return(TREE_NULL);
	}

	/* Process and store all the sub-trees */
	subtreecount = 0;
	tlp = trees;
	for( i=0; i<rec.c.c_length; i++ )  {
		register struct member *mp;
		auto struct directory *nextdp;
		auto mat_t new_xlate;		/* Accum translation mat */

		mp = &(members[i].M);
		if( (nextdp = rt_dir_lookup( mp->m_instname, LOOKUP_NOISY )) ==
		    DIR_NULL )
			continue;

		/* convert matrix to fastf_t from disk format */
		{
			register int k;
			static mat_t xmat;	/* temporary fastf_t matrix */
			for( k=0; k<4*4; k++ )
				xmat[k] = mp->m_mat[k];
			mat_mul(new_xlate, old_xlate, xmat);
		}

		/* Recursive call */
		if( (tlp->tl_tree = rt_drawobj(
		    nextdp, regionp, pathpos+1, new_xlate, &curmater )
		    ) == TREE_NULL )
			continue;

		if( regionp == REGION_NULL )  {
			register struct region *xrp;
			/*
			 * Found subtree that is not contained in a region;
			 * invent a region to hold JUST THIS SOLID,
			 * and add it to the region chain.  Don't force
			 * this whole combination into this region, because
			 * other subtrees might themselves contain regions.
			 * This matches GIFT's current behavior.
			 */
			rt_log("Warning:  Forced to create region %s\n",
				rt_path_str(pathpos+1) );
			if((tlp->tl_tree)->tr_op != OP_SOLID )
				rt_bomb("subtree not Solid");
			GETSTRUCT( xrp, region );
			xrp->reg_name = rt_strdup(rt_path_str(pathpos+1));
			rt_add_regtree( xrp, (tlp->tl_tree) );
			tlp->tl_tree = TREE_NULL;
			continue;	/* no remaining subtree, go on */
		}

		/* Store operation on subtree */
		switch( mp->m_relation )  {
		default:
			rt_log("%s: bad m_relation '%c'\n",
				regionp->reg_name, mp->m_relation );
			/* FALL THROUGH */
		case UNION:
			tlp->tl_op = OP_UNION;
			break;
		case SUBTRACT:
			tlp->tl_op = OP_SUBTRACT;
			break;
		case INTERSECT:
			tlp->tl_op = OP_INTERSECT;
			break;
		}
		subtreecount++;
		tlp++;
	}

	if( subtreecount <= 0 )  {
		/* Null subtree in region, release region struct */
		if( argregion == REGION_NULL && regionp != REGION_NULL )  {
			rt_free( regionp->reg_name, "unused region name" );
			rt_free( (char *)regionp, "unused region struct" );
		}
		curtree = TREE_NULL;
		goto out;
	}

	/* Build tree representing boolean expression in Member records */
	if( rt_pure_boolean_expressions )  {
		curtree = rt_make_bool_tree( trees, subtreecount, regionp );
	} else {
		register struct tree_list *tstart;

		/*
		 * This is the way GIFT interpreted equations, so we
		 * duplicate it here.  Any expressions between UNIONs
		 * is evaluated first, eg:
		 *	A - B - C u D - E - F
		 * is	(A - B - C) u (D - E - F)
		 * so first we do the parenthesised parts, and then go
		 * back and glue the unions together.
		 * As always, unions are the downfall of free enterprise!
		 */
		tstart = trees;
		tlp = trees+1;
		for( i=subtreecount-1; i>=0; i--, tlp++ )  {
			/* If we went off end, or hit a union, do it */
			if( i>0 && tlp->tl_op != OP_UNION )
				continue;
			if( (j = tlp-tstart) <= 0 )
				continue;
			tstart->tl_tree = rt_make_bool_tree( tstart, j, regionp );
			if(rt_g.debug&DEBUG_REGIONS) rt_pr_tree(tstart->tl_tree, 0);
			/* has side effect of zapping all trees,
			 * so build new first node */
			tstart->tl_op = OP_UNION;
			/* tstart here at union */
			tstart = tlp;
		}
		curtree = rt_make_bool_tree( trees, subtreecount, regionp );
		if(rt_g.debug&DEBUG_REGIONS) rt_pr_tree(curtree, 0);
	}

	if( argregion == REGION_NULL )  {
		/* Region began at this level */
		rt_add_regtree( regionp, curtree );
		curtree = TREE_NULL;		/* no remaining subtree */
	}

	/* Release dynamic memory and return */
out:
	rt_free( (char *)trees, "tree_list array");
	rt_free( (char *)members, "member records" );
	return( curtree );
}

/*
 *			R T _ M A K E _ B O O L _ T R E E
 *
 *  Given a tree_list array, build a tree of "union tree" nodes
 *  appropriately connected together.  Every element of the
 *  tree_list array used is replaced with a TREE_NULL.
 *  Elements which are already TREE_NULL are ignored.
 *  Returns a pointer to the top of the tree.
 */
HIDDEN union tree *
rt_make_bool_tree( tree_list, howfar, regionp )
struct tree_list *tree_list;
int howfar;
struct region *regionp;
{
	register struct tree_list *tlp;
	register int i;
	register struct tree_list *first_tlp;
	register union tree *xtp;
	register union tree *curtree;
	register int inuse;

	if( howfar <= 0 )
		return(TREE_NULL);

	/* Count number of non-null sub-trees to do */
	for( i=howfar, inuse=0, tlp=tree_list; i>0; i--, tlp++ )  {
		if( tlp->tl_tree == TREE_NULL )
			continue;
		if( inuse++ == 0 )
			first_tlp = tlp;
	}
	if( rt_g.debug & DEBUG_REGIONS && first_tlp->tl_op != OP_UNION )
		rt_log("Warning: %s: non-union first operation ignored\n",
			regionp->reg_name);

	/* Handle trivial cases */
	if( inuse <= 0 )
		return(TREE_NULL);
	if( inuse == 1 )  {
		curtree = first_tlp->tl_tree;
		first_tlp->tl_tree = TREE_NULL;
		return( curtree );
	}

	/* Allocate all the tree structs we will need */
	i = sizeof(union tree)*(inuse-1);
	if( (xtp=(union tree *)rt_malloc( i, "tree array")) == TREE_NULL )
		rt_bomb("rt_make_bool_tree: malloc failed\n");
	bzero( (char *)xtp, i );

	curtree = first_tlp->tl_tree;
	first_tlp->tl_tree = TREE_NULL;
	tlp=first_tlp+1;
	for( i=howfar-(tlp-tree_list); i>0; i--, tlp++ )  {
		if( tlp->tl_tree == TREE_NULL )
			continue;

		xtp->tr_b.tb_left = curtree;
		xtp->tr_b.tb_right = tlp->tl_tree;
		xtp->tr_b.tb_regionp = regionp;
		xtp->tr_op = tlp->tl_op;
		curtree = xtp++;
		tlp->tl_tree = TREE_NULL;	/* empty the input slot */
	}
	return(curtree);
}

/*
 *			R T _ P A T H _ S T R
 */
HIDDEN char *
rt_path_str( pos )
int pos;
{
	static char line[MAXLEVELS*(16+2)];
	register char *cp = &line[0];
	register int i;

	if( pos >= MAXLEVELS )  pos = MAXLEVELS-1;
	line[0] = '/';
	line[1] = '\0';
	for( i=0; i<=pos; i++ )  {
		(void)sprintf( cp, "/%s", path[i]->d_namep );
		cp += strlen(cp);
	}
	return( &line[0] );
}

/*
 *			R T _ P R _ R E G I O N
 */
void
rt_pr_region( rp )
register struct region *rp;
{
	rt_log("REGION %s (bit %d)\n", rp->reg_name, rp->reg_bit );
	rt_log("id=%d, air=%d, gift_material=%d, los=%d\n",
		rp->reg_regionid, rp->reg_aircode,
		rp->reg_gmater, rp->reg_los );
	if( rp->reg_mater.ma_override == 1 )
		rt_log("Color %d %d %d\n",
			rp->reg_mater.ma_rgb[0],
			rp->reg_mater.ma_rgb[1],
			rp->reg_mater.ma_rgb[2] );
	if( rp->reg_mater.ma_matname[0] != '\0' )
		rt_log("Material '%s' '%s'\n",
			rp->reg_mater.ma_matname,
			rp->reg_mater.ma_matparm );
	rt_pr_tree( rp->reg_treetop, 0 );
	rt_log("\n");
}

/*
 *			R T _ P R _ T R E E
 */
void
rt_pr_tree( tp, lvl )
register union tree *tp;
int lvl;			/* recursion level */
{
	register int i;

	rt_log("%.8x ", tp);
	for( i=lvl; i>0; i-- )
		rt_log("  ");

	if( tp == TREE_NULL )  {
		rt_log("Null???\n");
		return;
	}

	switch( tp->tr_op )  {

	case OP_SOLID:
		rt_log("SOLID %s (bit %d)",
			tp->tr_a.tu_stp->st_name,
			tp->tr_a.tu_stp->st_bit );
		break;

	default:
		rt_log("Unknown op=x%x\n", tp->tr_op );
		return;

	case OP_UNION:
		rt_log("UNION");
		break;
	case OP_INTERSECT:
		rt_log("INTERSECT");
		break;
	case OP_SUBTRACT:
		rt_log("MINUS");
		break;
	case OP_XOR:
		rt_log("XOR");
		break;
	case OP_NOT:
		rt_log("NOT");
		break;
	}
	rt_log("\n");

	switch( tp->tr_op )  {
	case OP_SOLID:
		break;
	case OP_UNION:
	case OP_INTERSECT:
	case OP_SUBTRACT:
	case OP_XOR:
		/* BINARY type */
		rt_pr_tree( tp->tr_b.tb_left, lvl+1 );
		rt_pr_tree( tp->tr_b.tb_right, lvl+1 );
		break;
	case OP_NOT:
	case OP_GUARD:
		/* UNARY tree */
		rt_pr_tree( tp->tr_b.tb_left, lvl+1 );
		break;
	}
}

/*
 *			R T _ F A S T F _ F L O A T
 *
 *  Convert TO 4xfastf_t FROM 3xfloats (for database) 
 */
void
rt_fastf_float( ff, fp, n )
register fastf_t *ff;
register float *fp;
register int n;
{
	while( n-- )  {
		*ff++ = *fp++;
		*ff++ = *fp++;
		*ff++ = *fp++;
		ff += ELEMENTS_PER_VECT-3;
	}
}

/*
 *  			R T _ F I N D _ S O L I D
 *  
 *  Given the (leaf) name of a solid, find the first occurance of it
 *  in the solid list.  Used mostly to find the light source.
 */
struct soltab *
rt_find_solid( name )
register char *name;
{
	register struct soltab *stp;
	register char *cp;
	for( stp = rt_i.HeadSolid; stp != SOLTAB_NULL; stp = stp->st_forw )  {
		if( *(cp = stp->st_name) == *name  &&
		    strcmp( cp, name ) == 0 )  {
			return(stp);
		}
	}
	return(SOLTAB_NULL);
}


/*
 *  			R T _ A D D _ R E G T R E E
 *  
 *  Add a region and it's boolean tree to all the appropriate places.
 *  The region and treetop are cross-linked, and the region is added
 *  to the linked list of regions.
 *  Positions in the region bit vector are established at this time.
 *  Also, this tree is also included in the overall RootTree.
 */
HIDDEN void
rt_add_regtree( regp, tp )
register struct region *regp;
register union tree *tp;
{
	register union tree *xor;

	/* Cross-reference */
	regp->reg_treetop = tp;
	tp->tr_regionp = regp;
	/* Add to linked list */
	regp->reg_forw = rt_i.HeadRegion;
	rt_i.HeadRegion = regp;

	/* Determine material properties */
	regp->reg_ufunc = 0;
	regp->reg_udata = (char *)0;
	rt_region_color_map(regp);

	regp->reg_bit = rt_i.nregions;	/* Add to bit vectors */
	/* Will be added to rt_i.Regions[] in final prep stage */
	rt_i.nregions++;
	if( rt_g.debug & DEBUG_REGIONS )
		rt_log("Add Region %s\n", regp->reg_name);

	if( rt_i.RootTree == TREE_NULL )  {
		rt_i.RootTree = tp;
		return;
	}
	/**GETSTRUCT( xor, union tree ); **/
	xor = (union tree *)rt_malloc(sizeof(union tree), "rt_add_regtree tree");
	bzero( (char *)xor, sizeof(union tree) );
	xor->tr_b.tb_op = OP_XOR;
	xor->tr_b.tb_left = rt_i.RootTree;
	xor->tr_b.tb_right = tp;
	xor->tr_b.tb_regionp = REGION_NULL;
	rt_i.RootTree = xor;
}

/*
 *			R T _ O P T I M _ T R E E
 */
HIDDEN void
rt_optim_tree( tp )
register union tree *tp;
{
	register union tree *low;

	switch( tp->tr_op )  {
	case OP_SOLID:
		return;
	case OP_SUBTRACT:
		while( (low=tp->tr_b.tb_left)->tr_op == OP_SUBTRACT )  {
			/* Rewrite X - A - B as X - ( A union B ) */
			tp->tr_b.tb_left = low->tr_b.tb_left;
			low->tr_op = OP_UNION;
			low->tr_b.tb_left = low->tr_b.tb_right;
			low->tr_b.tb_right = tp->tr_b.tb_right;
			tp->tr_b.tb_right = low;
		}
		goto binary;
	case OP_UNION:
	case OP_INTERSECT:
	case OP_XOR:
		/* Need to look at 3-level optimizations here, both sides */
		goto binary;
	default:
		rt_log("rt_optim_tree: bad op x%x\n", tp->tr_op);
		return;
	}
binary:
	rt_optim_tree( tp->tr_b.tb_left );
	rt_optim_tree( tp->tr_b.tb_right );
}

/*
 *  			S O L I D _ B I T F I N D E R
 *  
 *  Used to walk the boolean tree, setting bits for all the solids in the tree
 *  to the provided bit vector.  Should be called AFTER the region bits
 *  have been assigned.
 */
HIDDEN void
rt_solid_bitfinder( treep, regbit )
register union tree *treep;
register int regbit;
{
	register struct soltab *stp;

	switch( treep->tr_op )  {
	case OP_SOLID:
		stp = treep->tr_a.tu_stp;
		BITSET( stp->st_regions, regbit );
		if( regbit+1 > stp->st_maxreg )  stp->st_maxreg = regbit+1;
		if( rt_g.debug&DEBUG_REGIONS )  {
			rt_pr_bitv( stp->st_name, stp->st_regions,
				stp->st_maxreg );
		}
		return;
	case OP_UNION:
	case OP_INTERSECT:
	case OP_SUBTRACT:
		/* BINARY type */
		rt_solid_bitfinder( treep->tr_b.tb_left, regbit );
		rt_solid_bitfinder( treep->tr_b.tb_right, regbit );
		return;
	default:
		rt_log("rt_solid_bitfinder:  op=x%x\n", treep->tr_op);
		return;
	}
	/* NOTREACHED */
}

/*
 *  			R T _ P R E P
 *  
 *  This routine should be called just before the first call to rt_shootray().
 *  Right now, it should only be called ONCE per execution.
 */
void
rt_prep()
{
	register struct region *regp;
	register struct soltab *stp;

	if(!rt_i.needprep)
		rt_bomb("second invocation of rt_prep");
	rt_i.needprep = 0;

	/*
	 *  Allocate space for a per-solid bit of rt_i.nregions length.
	 */
	for( stp=rt_i.HeadSolid; stp != SOLTAB_NULL; stp=stp->st_forw )  {
		stp->st_regions = (bitv_t *)rt_malloc(
			BITS2BYTES(rt_i.nregions)+sizeof(bitv_t),
			"st_regions bitv" );
		BITZERO( stp->st_regions, rt_i.nregions );
		stp->st_maxreg = 0;
	}

	/*  Build array of region pointers indexed by reg_bit.
	 *  Optimize each region's expression tree.
	 *  Set this region's bit in the bit vector of every solid
	 *  contained in the subtree.
	 */
	rt_i.Regions = (struct region **)rt_malloc(
		rt_i.nregions * sizeof(struct region *),
		"rt_i.Regions[]" );
	for( regp=rt_i.HeadRegion; regp != REGION_NULL; regp=regp->reg_forw )  {
		rt_i.Regions[regp->reg_bit] = regp;
		rt_optim_tree( regp->reg_treetop );
		rt_solid_bitfinder( regp->reg_treetop, regp->reg_bit );
		if(rt_g.debug&DEBUG_REGIONS)  {
			rt_pr_region( regp );
		}
	}
	if(rt_g.debug&DEBUG_REGIONS)  {
		for( stp=rt_i.HeadSolid; stp != SOLTAB_NULL; stp=stp->st_forw )  {
			rt_log("solid %s ", stp->st_name);
			rt_pr_bitv( "regions ref", stp->st_regions,
				stp->st_maxreg);
		}
	}

	/* Partition space */
	rt_cut_it();
}

/*
 *			R T _ V I E W B O U N D S
 *
 *  Given a model2view transformation matrix, compute the RPP which
 *  encloses all the solids in the model, in view space.
 */
void
rt_viewbounds( min, max, m2v )
matp_t m2v;
register vect_t min, max;
{
	register struct soltab *stp;
	static vect_t xlated;

	max[X] = max[Y] = max[Z] = -INFINITY;
	min[X] = min[Y] = min[Z] =  INFINITY;

	for( stp=rt_i.HeadSolid; stp != 0; stp=stp->st_forw ) {
		MAT4X3PNT( xlated, m2v, stp->st_center );
#define VBMIN(v,t) {FAST fastf_t rt; rt=(t); if(rt<v) v = rt;}
#define VBMAX(v,t) {FAST fastf_t rt; rt=(t); if(rt>v) v = rt;}
		VBMIN( min[X], xlated[0]-stp->st_bradius );		
		VBMAX( max[X], xlated[0]+stp->st_bradius );
		VBMIN( min[Y], xlated[1]-stp->st_bradius );
		VBMAX( max[Y], xlated[1]+stp->st_bradius );
		VBMIN( min[Z], xlated[2]-stp->st_bradius );
		VBMAX( max[Z], xlated[2]+stp->st_bradius );
	}

	/* Provide a slight border */
	min[X] -= fabs(min[X]) * 0.03;
	min[Y] -= fabs(min[Y]) * 0.03;
	min[Z] -= fabs(min[Z]) * 0.03;
	max[X] += fabs(max[X]) * 0.03;
	max[Y] += fabs(max[Y]) * 0.03;
	max[Z] += fabs(max[Z]) * 0.03;
}
