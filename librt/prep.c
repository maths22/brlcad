/*
 *			P R E P
 *
 *  Manage one-time preparations to be done before actual
 *  ray-tracing can commence.
 *
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
 *	This software is Copyright (C) 1990 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSprep[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>
#ifdef BSD
#include <strings.h>
#else
#include <string.h>
#endif
#include "machine.h"
#include "vmath.h"
#include "db.h"
#include "raytrace.h"
#include "./debug.h"

HIDDEN void	rt_fr_tree();
HIDDEN void	rt_solid_bitfinder();

extern struct resource	rt_uniresource;		/* from shoot.c */

/*
 *  			R T _ P R E P
 *  
 *  This routine should be called just before the first call to rt_shootray().
 *  It should only be called ONCE per execution, unless rt_clean() is
 *  called inbetween.
 */
void
rt_prep(rtip)
register struct rt_i *rtip;
{
	register struct region *regp;
	register struct soltab *stp;
	register int		i;
	vect_t			diag;

	if( rtip->rti_magic != RTI_MAGIC )  rt_bomb("rt_prep:  bad rtip\n");

	if(!rtip->needprep)
		rt_bomb("rt_prep: invoked a second time");
	rtip->needprep = 0;
	if( rtip->nsolids <= 0 )  {
		if( rtip->rti_air_discards > 0 )
			rt_log("rt_prep: %d solids discarded due to air regions\n", rtip->rti_air_discards );
		rt_bomb("rt_prep:  no solids left to prep");
	}

	/* Compute size of model-specific variable-length data structures */
	/* -sizeof(bitv_t) == sizeof(struct partition.pt_solhit) */
	rtip->rti_pt_bytes = sizeof(struct partition) - sizeof(bitv_t) + 1 +
		RT_BITV_BITS2WORDS(rtip->nsolids) * sizeof(bitv_t);
	rtip->rti_bv_bytes = sizeof(bitv_t) *
		( RT_BITV_BITS2WORDS(rtip->nsolids) +
		RT_BITV_BITS2WORDS(rtip->nregions) + 4 );

	/*
	 *  Allocate space for a per-solid bit of rtip->nregions length.
	 */
	for( RT_LIST( stp, soltab, &(rtip->rti_headsolid) ) )  {
		stp->st_regions = (bitv_t *)rt_calloc(
			RT_BITV_BITS2WORDS(rtip->nregions),
			sizeof(bitv_t), "st_regions bitv" );
		stp->st_maxreg = 0;
	}

	/* In case everything is a halfspace, set a minimum space */
	if( rtip->mdl_min[X] >= INFINITY )  {
		VSETALL( rtip->mdl_min, -1 );
	}
	if( rtip->mdl_max[X] <= -INFINITY )  {
		VSETALL( rtip->mdl_max, 1 );
	}

	/*
	 *  Enlarge the model RPP just slightly, to avoid nasty
	 *  effects with a solid's face being exactly on the edge
	 */
	rtip->mdl_min[X] = floor( rtip->mdl_min[X] );
	rtip->mdl_min[Y] = floor( rtip->mdl_min[Y] );
	rtip->mdl_min[Z] = floor( rtip->mdl_min[Z] );
	rtip->mdl_max[X] = ceil( rtip->mdl_max[X] );
	rtip->mdl_max[Y] = ceil( rtip->mdl_max[Y] );
	rtip->mdl_max[Z] = ceil( rtip->mdl_max[Z] );

	/* Compute radius of a model bounding sphere */
	VSUB2( diag, rtip->mdl_max, rtip->mdl_min );
	rtip->rti_radius = 0.5 * MAGNITUDE(diag);

	/*  Build array of region pointers indexed by reg_bit.
	 *  Optimize each region's expression tree.
	 *  Set this region's bit in the bit vector of every solid
	 *  contained in the subtree.
	 */
	rtip->Regions = (struct region **)rt_malloc(
		rtip->nregions * sizeof(struct region *),
		"rtip->Regions[]" );
	for( regp=rtip->HeadRegion; regp != REGION_NULL; regp=regp->reg_forw )  {
		rtip->Regions[regp->reg_bit] = regp;
		rt_optim_tree( regp->reg_treetop, &rt_uniresource );
		rt_solid_bitfinder( regp->reg_treetop, regp->reg_bit,
			&rt_uniresource );
		if(rt_g.debug&DEBUG_REGIONS)  {
			rt_pr_region( regp );
		}
	}
	if(rt_g.debug&DEBUG_REGIONS)  {
		for( RT_LIST( stp, soltab, &(rtip->rti_headsolid) ) )  {
			rt_log("solid %s ", stp->st_name);
			rt_pr_bitv( "regions ref", stp->st_regions,
				stp->st_maxreg);
		}
	}

	/*  Space for array of soltab pointers indexed by solid bit number.
	 *  Include enough extra space for an extra bitv_t's worth of bits,
	 *  to handle round-up.
	 */
	rtip->rti_Solids = (struct soltab **)rt_calloc(
		rtip->nsolids + (1<<BITV_SHIFT), sizeof(struct soltab *),
		"rtip->rti_Solids[]" );
	/*
	 *  Build array of solid table pointers indexed by solid ID.
	 *  Last element for each kind will be found in
	 *	rti_sol_by_type[id][rti_nsol_by_type[id]-1]
	 */
	for( RT_LIST( stp, soltab, &(rtip->rti_headsolid) ) )  {
		rtip->rti_Solids[stp->st_bit] = stp;
		rtip->rti_nsol_by_type[stp->st_id]++;
	}
	/* Find solid type with maximum length (for rt_shootray) */
	rtip->rti_maxsol_by_type = 0;
	for( i=0; i <= ID_MAXIMUM; i++ )  {
		if( rtip->rti_nsol_by_type[i] > rtip->rti_maxsol_by_type )
			rtip->rti_maxsol_by_type = rtip->rti_nsol_by_type[i];
	}
	/* Malloc the storage and zero the counts */
	for( i=0; i <= ID_MAXIMUM; i++ )  {
		if( rtip->rti_nsol_by_type[i] <= 0 )  continue;
		rtip->rti_sol_by_type[i] = (struct soltab **)rt_calloc(
			rtip->rti_nsol_by_type[i],
			sizeof(struct soltab *),
			"rti_sol_by_type[]" );
		rtip->rti_nsol_by_type[i] = 0;
	}
	/* Fill in the array and rebuild the count (aka index) */
	for( RT_LIST( stp, soltab, &(rtip->rti_headsolid) ) )  {
		register int	id;
		id = stp->st_id;
		rtip->rti_sol_by_type[id][rtip->rti_nsol_by_type[id]++] = stp;
	}
	if( rt_g.debug & DEBUG_DB )  {
		for( i=1; i <= ID_MAXIMUM; i++ )  {
			rt_log("%5d %s (%d)\n",
				rtip->rti_nsol_by_type[i],
				rt_functab[i].ft_name,
				i );
		}
	}

	/* Partition space */
	rt_cut_it(rtip);

	/* Plot bounding RPPs */
	if( (rt_g.debug&DEBUG_PLOTBOX) )  {
		FILE	*plotfp;

		if( (plotfp=fopen("rtrpp.plot", "w"))!=NULL) {
			/* Plot solid bounding boxes, in white */
			pdv_3space( plotfp, rtip->rti_pmin, rtip->rti_pmax );
			pl_color( plotfp, 255, 255, 255 );
			for( RT_LIST( stp, soltab, &(rtip->rti_headsolid) ) )  {
				/* Don't draw infinite solids */
				if( stp->st_aradius >= INFINITY )
					continue;
				pdv_3box( plotfp, stp->st_min, stp->st_max );
			}
			(void)fclose(plotfp);
		}
	}

	/* Plot solid outlines */
	if( (rt_g.debug&DEBUG_PLOTBOX) )  {
		FILE		*plotfp;
		register struct soltab	*stp;

		if( (plotfp=fopen("rtsolids.pl", "w")) == NULL)  return;

		pdv_3space( plotfp, rtip->rti_pmin, rtip->rti_pmax );

		for( RT_LIST( stp, soltab, &(rtip->rti_headsolid) ) )  {
			/* Don't draw infinite solids */
			if( stp->st_aradius >= INFINITY )
				continue;

			if( rt_plot_solid( plotfp, rtip, stp ) < 0 )
				rt_log("unable to plot %s\n", stp->st_name);
		}
		(void)fclose(plotfp);
	}
}

/*
 *			R T _ P L O T _ S O L I D
 *
 *  Plot a solid with the same kind of wireframes that MGED would display,
 *  in UNIX-plot form, on the indicated file descriptor.
 *  The caller is responsible for calling pdv_3space().
 *
 *  Returns -
 *	<0	failure
 *	 0	OK
 */
int
rt_plot_solid( fp, rtip, stp )
register FILE		*fp;
struct rt_i		*rtip;
struct soltab		*stp;
{
	register struct rt_vlist	*vp;
	struct rt_list			vhead;
	struct region			*regp;
	struct rt_external		ext;
	struct rt_db_internal		intern;
	int				id = stp->st_id;
	int				rnum;
	struct rt_tess_tol		ttol;
	struct rt_tol			tol;
	matp_t				mat;

	RT_LIST_INIT( &vhead );

	if( db_get_external( &ext, stp->st_dp, rtip->rti_dbip ) < 0 )  {
		rt_log("rt_plot_solid(%s): db_get_external() failure\n",
			stp->st_name);
		return(-1);			/* FAIL */
	}

	if( !(mat = stp->st_matp) )
		mat = (matp_t)rt_identity;
    	RT_INIT_DB_INTERNAL(&intern);
	if( rt_functab[id].ft_import( &intern, &ext, mat ) < 0 )  {
		rt_log("rt_plot_solid(%s):  solid import failure\n",
			stp->st_name );
	    	if( intern.idb_ptr )  rt_functab[id].ft_ifree( &intern );
		db_free_external( &ext );
		return(-1);			/* FAIL */
	}
	RT_CK_DB_INTERNAL( &intern );

	ttol.magic = RT_TESS_TOL_MAGIC;
	ttol.abs = 0.0;
	ttol.rel = 0.01;
	ttol.norm = 0;

	/* XXX These need to be improved */
	tol.magic = RT_TOL_MAGIC;
	tol.dist = 0.005;
	tol.dist_sq = tol.dist * tol.dist;
	tol.perp = 1e-6;
	tol.para = 1 - tol.perp;

	if( rt_functab[id].ft_plot(
		&vhead,
		&intern,
		&ttol,
		&tol
	    ) < 0 )  {
		rt_log("rt_plot_solid(%s): ft_plot() failure\n",
			stp->st_name);
	    	if( intern.idb_ptr )  rt_functab[id].ft_ifree( &intern );
		db_free_external( &ext );
	    	return(-2);
	}
	rt_functab[id].ft_ifree( &intern );
	db_free_external( &ext );

	/* Take color from one region */
	if( (rnum = stp->st_maxreg-1) < 0 ) rnum = 0;
	if( (regp = rtip->Regions[rnum]) != REGION_NULL )  {
		pl_color( fp,
			(int)(255*regp->reg_mater.ma_color[0]),
			(int)(255*regp->reg_mater.ma_color[1]),
			(int)(255*regp->reg_mater.ma_color[2]) );
	}

	for( RT_LIST_FOR( vp, rt_vlist, &vhead ) )  {
		register int	i;
		register int	nused = vp->nused;
		register int	*cmd = vp->cmd;
		register point_t *pt = vp->pt;
		for( i = 0; i < nused; i++,cmd++,pt++ )  {
			switch( *cmd )  {
			case RT_VLIST_POLY_START:
				break;
			case RT_VLIST_POLY_MOVE:
			case RT_VLIST_LINE_MOVE:
				pdv_3move( fp, *pt );
				break;
			case RT_VLIST_POLY_DRAW:
			case RT_VLIST_POLY_END:
			case RT_VLIST_LINE_DRAW:
				pdv_3cont( fp, *pt );
				break;
			default:
				rt_log("rt_plot_solid(%s): unknown vlist cmd x%x\n",
					stp->st_name, *cmd );
			}
		}
	}

	if( RT_LIST_IS_EMPTY( &vhead ) )  {
		rt_log("rt_plot_solid(%s): no vectors to plot?\n",
			stp->st_name);
		return(-3);		/* FAIL */
	}
	RT_FREE_VLIST( &vhead );
	return(0);			/* OK */
}

/*
 *			R T _ C L E A N
 *
 *  Release all the dynamic storage associated with a particular rt_i
 *  structure, except for the database instance information (dir, etc).
 */
void
rt_clean( rtip )
register struct rt_i *rtip;
{
	register struct region *regp;
	register struct soltab *stp;
	int	i;

	if( rtip->rti_magic != RTI_MAGIC )  rt_bomb("rt_clean:  bad rtip\n");

	/*
	 *  Clear out the solid table
	 */
	while( RT_LIST_WHILE( stp, soltab, &(rtip->rti_headsolid) ) )  {
		RT_CHECK_SOLTAB(stp);
		RT_LIST_DEQUEUE( &(stp->l) );
		if( stp->st_id < 0 || stp->st_id >= rt_nfunctab )
			rt_bomb("rt_clean:  bad st_id");
		rt_functab[stp->st_id].ft_free( stp );
		rt_free( (char *)stp->st_regions, "st_regions bitv" );
		if( stp->st_matp )  rt_free( (char *)stp->st_matp, "st_matp");
		stp->st_matp = (matp_t)0;
		stp->st_regions = (bitv_t *)0;
		stp->st_dp = DIR_NULL;		/* was ptr to directory */
		rt_free( (char *)stp, "struct soltab");
	}
	rtip->nsolids = 0;

	/*  
	 *  Clear out the region table
	 */
	for( regp=rtip->HeadRegion; regp != REGION_NULL; )  {
		register struct region *nextregp = regp->reg_forw;

		rt_fr_tree( regp->reg_treetop );
		rt_free( (char *)regp->reg_name, "region name str");
		regp->reg_name = (char *)0;
		rt_free( (char *)regp, "struct region");
		regp = nextregp;
	}
	rtip->HeadRegion = REGION_NULL;
	rtip->nregions = 0;

	/**** The best thing to do would be to hunt down the
	 *  bitv and partition structs and release them, because
	 *  they depend on the number of solids & regions!  XXX
	 */

	/* Clean out the array of pointers to regions, if any */
	if( rtip->Regions )  {
		rt_free( (char *)rtip->Regions, "rtip->Regions[]" );
		rtip->Regions = (struct region **)0;

		/* Free space partitions */
		rt_fr_cut( &(rtip->rti_CutHead) );
		bzero( (char *)&(rtip->rti_CutHead), sizeof(union cutter) );
		rt_fr_cut( &(rtip->rti_inf_box) );
		bzero( (char *)&(rtip->rti_inf_box), sizeof(union cutter) );
	}
	/* rt_g.rtg_CutFree list could be freed, but is bulk allocated, XXX
	 * so cutter structures will hang around.  XXX
	 */
	/* XXX struct seg is also bulk allocated, can't be freed. XXX */

	/* Release partition structs.  XXX How to find them?  resource structs? */

	/* Reset instancing counters in database directory */
	for( i=0; i < RT_DBNHASH; i++ )  {
		register struct directory	*dp;

		dp = rtip->rti_dbip->dbi_Head[i];
		for( ; dp != DIR_NULL; dp = dp->d_forw )
			dp->d_uses = 0;
	}

	/* Free animation structures */
	db_free_anim(rtip->rti_dbip);

	/* Free array of solid table pointers indexed by solid ID */
	for( i=0; i <= ID_MAXIMUM; i++ )  {
		if( rtip->rti_nsol_by_type[i] <= 0 )  continue;
		rt_free( (char *)rtip->rti_sol_by_type[i], "sol_by_type" );
		rtip->rti_sol_by_type[i] = (struct soltab **)0;
	}
	if( rtip->rti_Solids )  {
		rt_free( (char *)rtip->rti_Solids, "rtip->rti_Solids[]" );
		rtip->rti_Solids = (struct soltab **)0;
	}

	/*
	 *  Re-initialize everything important.
	 *  This duplicates the code in rt_dirbuild().
	 */

	rtip->rti_inf_box.bn.bn_type = CUT_BOXNODE;
	VMOVE( rtip->rti_inf_box.bn.bn_min, rtip->mdl_min );
	VMOVE( rtip->rti_inf_box.bn.bn_max, rtip->mdl_max );
	VSETALL( rtip->mdl_min, -0.1 );
	VSETALL( rtip->mdl_max,  0.1 );

	rt_hist_free( &rtip->rti_hist_cellsize );
	rt_hist_free( &rtip->rti_hist_cutdepth );

	rtip->rti_magic = RTI_MAGIC;
	rtip->needprep = 1;
}

/*
 *			R T _ D E L _ R E G T R E E
 *
 *  Remove a region from the linked list.  Used to remove a particular
 *  region from the active database, presumably after some useful
 *  information has been extracted (eg, a light being converted to
 *  implicit type), or for special effects.
 *
 *  Returns -
 *	-1	if unable to find indicated region
 *	 0	success
 */
int
rt_del_regtree( rtip, delregp )
struct rt_i *rtip;
register struct region *delregp;
{
	register struct region *regp;
	register struct region *nextregp;

	regp = rtip->HeadRegion;
	if( rt_g.debug & DEBUG_REGIONS )
		rt_log("Del Region %s\n", delregp->reg_name);

	if( regp == delregp )  {
		rtip->HeadRegion = regp->reg_forw;
		goto zot;
	}

	for( ; regp != REGION_NULL; regp=nextregp )  {
		nextregp=regp->reg_forw;
		if( nextregp == delregp )  {
			regp->reg_forw = nextregp->reg_forw;	/* unlink */
			goto zot;
		}
	}
	rt_log("rt_del_region:  unable to find %s\n", delregp->reg_name);
	return(-1);
zot:
	rt_fr_tree( delregp->reg_treetop );
	rt_free( (char *)delregp->reg_name, "region name str");
	delregp->reg_name = (char *)0;
	rt_free( (char *)delregp, "struct region");
	return(0);
}

/*
 *			R T _ F R  _ T R E E
 *
 *  Free a boolean operation tree.
 *  Pointers to children are nulled out, to prevent any stray references
 *  to this tree node from being sucessful.
 *  XXX should iterate, rather than recurse.
 */
HIDDEN void
rt_fr_tree( tp )
register union tree *tp;
{

	switch( tp->tr_op )  {
	case OP_NOP:
		tp->tr_op = 0;
		rt_free( (char *)tp, "NOP union tree");
		return;
	case OP_SOLID:
		rt_free( tp->tr_a.tu_name, "leaf name" );
		tp->tr_a.tu_name = (char *)0;
		tp->tr_op = 0;
		rt_free( (char *)tp, "leaf tree union");
		return;
	case OP_SUBTRACT:
	case OP_UNION:
	case OP_INTERSECT:
	case OP_XOR:
		rt_fr_tree( tp->tr_b.tb_left );
		tp->tr_b.tb_left = TREE_NULL;
		rt_fr_tree( tp->tr_b.tb_right );
		tp->tr_b.tb_right = TREE_NULL;
		tp->tr_op = 0;
		rt_free( (char *)tp, "binary tree union");
		return;
	default:
		rt_log("rt_fr_tree: bad op x%x\n", tp->tr_op);
		return;
	}
}

/*
 *  			S O L I D _ B I T F I N D E R
 *  
 *  Used to walk the boolean tree, setting bits for all the solids in the tree
 *  to the provided bit vector.  Should be called AFTER the region bits
 *  have been assigned.
 */
HIDDEN void
rt_solid_bitfinder( treep, regbit, resp )
register union tree	*treep;
register int		regbit;
struct resource		*resp;
{
	register union tree	**sp;
	register struct soltab	*stp;
	register union tree	**stackend;

	while( (sp = resp->re_boolstack) == (union tree **)0 )
		rt_grow_boolstack( resp );
	stackend = &(resp->re_boolstack[resp->re_boolslen-1]);
	*sp++ = TREE_NULL;
	*sp++ = treep;
	while( (treep = *--sp) != TREE_NULL ) {
		switch( treep->tr_op )  {
		case OP_NOP:
			break;
		case OP_SOLID:
			stp = treep->tr_a.tu_stp;
			BITSET( stp->st_regions, regbit );
			if( !BITTEST( stp->st_regions, regbit ) )
				rt_bomb("BITSET failure\n");	/* sanity check */
			if( regbit+1 > stp->st_maxreg )  stp->st_maxreg = regbit+1;
			if( rt_g.debug&DEBUG_REGIONS )  {
				rt_pr_bitv( stp->st_name, stp->st_regions,
					stp->st_maxreg );
			}
			break;
		case OP_UNION:
		case OP_INTERSECT:
		case OP_SUBTRACT:
			/* BINARY type */
			/* push both nodes - search left first */
			*sp++ = treep->tr_b.tb_right;
			*sp++ = treep->tr_b.tb_left;
			if( sp >= stackend )  {
				register int off = sp - resp->re_boolstack;
				rt_grow_boolstack( resp );
				sp = &(resp->re_boolstack[off]);
				stackend = &(resp->re_boolstack[resp->re_boolslen-1]);
			}
			break;
		default:
			rt_log("rt_solid_bitfinder:  op=x%x\n", treep->tr_op);
			break;
		}
	}
}

