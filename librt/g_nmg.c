/*
 *			G _ N M G . C
 *
 *  Purpose -
 *	Intersect a ray with a 
 *
 *  Authors -
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1990 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSnmg[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>
#include "machine.h"
#include "vmath.h"
#include "db.h"
#include "rtlist.h"
#include "nmg.h"
#include "raytrace.h"
#include "./debug.h"

/* rt_nmg_internal is just "model", from nmg.h */

struct nmg_specific {
	vect_t	nmg_V;
};

/*
 *  			R T _ N M G _ P R E P
 *  
 *  Given a pointer to a GED database record, and a transformation matrix,
 *  determine if this is a valid NMG, and if so, precompute various
 *  terms of the formula.
 *  
 *  Returns -
 *  	0	NMG is OK
 *  	!0	Error in description
 *  
 *  Implicit return -
 *  	A struct nmg_specific is created, and it's address is stored in
 *  	stp->st_specific for use by nmg_shot().
 */
int
rt_nmg_prep( stp, ip, rtip )
struct soltab		*stp;
struct rt_db_internal	*ip;
struct rt_i		*rtip;
{
	struct model		*m;
	register struct nmg_specific	*nmg;

	RT_CK_DB_INTERNAL(ip);
	m = (struct model *)ip->idb_ptr;
	NMG_CK_MODEL(m);
}

/*
 *			R T _ N M G _ P R I N T
 */
void
rt_nmg_print( stp )
register struct soltab *stp;
{
	register struct nmg_specific *nmg =
		(struct nmg_specific *)stp->st_specific;
}

/*
 *  			R T _ N M G _ S H O T
 *  
 *  Intersect a ray with a nmg.
 *  If an intersection occurs, a struct seg will be acquired
 *  and filled in.
 *  
 *  Returns -
 *  	0	MISS
 *	>0	HIT
 */
int
rt_nmg_shot( stp, rp, ap, seghead )
struct soltab		*stp;
register struct xray	*rp;
struct application	*ap;
struct seg		*seghead;
{
	register struct nmg_specific *nmg =
		(struct nmg_specific *)stp->st_specific;
	register struct seg *segp;

	return(2);			/* HIT */
}

#define RT_NMG_SEG_MISS(SEG)	(SEG).seg_stp=RT_SOLTAB_NULL

/*
 *			R T _ N M G _ V S H O T
 *
 *  Vectorized version.
 */
void
rt_nmg_vshot( stp, rp, segp, n, resp)
struct soltab	       *stp[]; /* An array of solid pointers */
struct xray		*rp[]; /* An array of ray pointers */
struct  seg            segp[]; /* array of segs (results returned) */
int		  	    n; /* Number of ray/object pairs */
struct resource         *resp; /* pointer to a list of free segs */
{
	rt_vstub( stp, rp, segp, n, resp );
}

/*
 *  			R T _ N M G _ N O R M
 *  
 *  Given ONE ray distance, return the normal and entry/exit point.
 */
void
rt_nmg_norm( hitp, stp, rp )
register struct hit	*hitp;
struct soltab		*stp;
register struct xray	*rp;
{
	register struct nmg_specific *nmg =
		(struct nmg_specific *)stp->st_specific;

	VJOIN1( hitp->hit_point, rp->r_pt, hitp->hit_dist, rp->r_dir );
}

/*
 *			R T _ N M G _ C U R V E
 *
 *  Return the curvature of the nmg.
 */
void
rt_nmg_curve( cvp, hitp, stp )
register struct curvature *cvp;
register struct hit	*hitp;
struct soltab		*stp;
{
	register struct nmg_specific *nmg =
		(struct nmg_specific *)stp->st_specific;

 	cvp->crv_c1 = cvp->crv_c2 = 0;

	/* any tangent direction */
 	vec_ortho( cvp->crv_pdir, hitp->hit_normal );
}

/*
 *  			R T _ N M G _ U V
 *  
 *  For a hit on the surface of an nmg, return the (u,v) coordinates
 *  of the hit point, 0 <= u,v <= 1.
 *  u = azimuth
 *  v = elevation
 */
void
rt_nmg_uv( ap, stp, hitp, uvp )
struct application	*ap;
struct soltab		*stp;
register struct hit	*hitp;
register struct uvcoord	*uvp;
{
	register struct nmg_specific *nmg =
		(struct nmg_specific *)stp->st_specific;
}

/*
 *		R T _ N M G _ F R E E
 */
void
rt_nmg_free( stp )
register struct soltab *stp;
{
	register struct nmg_specific *nmg =
		(struct nmg_specific *)stp->st_specific;

	rt_free( (char *)nmg, "nmg_specific" );
}

/*
 *			R T _ N M G _ C L A S S
 */
int
rt_nmg_class()
{
	return(0);
}

/*
 *			R T _ N M G _ P L O T
 */
int
rt_nmg_plot( vhead, ip, abs_tol, rel_tol, norm_tol )
struct rt_list		*vhead;
struct rt_db_internal	*ip;
double			abs_tol;
double			rel_tol;
double			norm_tol;
{
	LOCAL struct model	*m;

	RT_CK_DB_INTERNAL(ip);
	m = (struct model *)ip->idb_ptr;
	NMG_CK_MODEL(m);

	return(-1);
}

/*
 *			R T _ N M G _ T E S S
 *
 *  Returns -
 *	-1	failure
 *	 0	OK.  *r points to nmgregion that holds this tessellation.
 */
int
rt_nmg_tess( r, m, ip, abs_tol, rel_tol, norm_tol )
struct nmgregion	**r;
struct model		*m;
struct rt_db_internal	*ip;
double			abs_tol;
double			rel_tol;
double			norm_tol;
{
	LOCAL struct model	*m;

	RT_CK_DB_INTERNAL(ip);
	m = (struct model *)ip->idb_ptr;
	NMG_CK_MODEL(m);

	return(-1);
}

/* Format of db.h nmg.N_nstructs info, sucked straight from nmg_struct_counts */
#define NMG_O(m)	offsetof(struct nmg_struct_counts, m)

struct rt_imexport rt_nmg_structs_fmt[] = {
	"%d",	NMG_O(model),		1,
	"%d",	NMG_O(model_a),		1,
	"%d",	NMG_O(region),		1,
	"%d",	NMG_O(region_a),	1,
	"%d",	NMG_O(shell),		1,
	"%d",	NMG_O(shell_a),		1,
	"%d",	NMG_O(faceuse),		1,
	"%d",	NMG_O(faceuse_a),	1,
	"%d",	NMG_O(face),		1,
	"%d",	NMG_O(face_g),		1,
	"%d",	NMG_O(loopuse),		1,
	"%d",	NMG_O(loopuse_a),	1,
	"%d",	NMG_O(loop),		1,
	"%d",	NMG_O(loop_g),		1,
	"%d",	NMG_O(edgeuse),		1,
	"%d",	NMG_O(edgeuse_a),	1,
	"%d",	NMG_O(edge),		1,
	"%d",	NMG_O(edge_g),		1,
	"%d",	NMG_O(vertexuse),	1,
	"%d",	NMG_O(vertexuse_a),	1,
	"%d",	NMG_O(vertex),		1,
	"%d",	NMG_O(vertex_g),	1,
	"",	0,			0
};

/*
 *			R T _ N M G _ I M P O R T
 *
 *  Import an NMG from the database format to the internal format.
 *  Apply modeling transformations as well.
 */
int
rt_nmg_import( ip, ep, mat )
struct rt_db_internal	*ip;
struct rt_external	*ep;
register mat_t		mat;
{
	LOCAL struct model		*m;
	union record			*rp;
	struct nmg_struct_counts	cntbuf;
	struct rt_external		count_ext;

	RT_CK_EXTERNAL( ep );
	rp = (union record *)ep->ext_buf;
	/* Check record type */
	if( rp->u_id != DBID_NMG )  {
		rt_log("rt_nmg_import: defective record\n");
		return(-1);
	}

	bzero( (char *)&cntbuf, sizeof(cntbuf) );
	RT_INIT_EXTERNAL(&count_ext);
	count_ext.ext_nbytes = sizeof(rp->nmg.N_structs);
	count_ext.ext_buf = rp->nmg.N_structs;
	if( rt_struct_import( (genptr_t)&cntbuf, rt_nmg_structs_fmt, &count_ext ) <= 0 )  {
		rt_log("rt_struct_import failure\n");
		return(-1);
	}
	nmg_pr_struct_counts( &cntbuf, "After import" );

return(-1);
#if 0
	RT_INIT_DB_INTERNAL( ip );
	ip->idb_type = ID_NMG;
	ip->idb_ptr = rt_malloc( sizeof(struct model), "model");
	m = (struct model *)ip->idb_ptr;
	m->magic = RT_NMG_INTERNAL_MAGIC;

	return(0);			/* OK */
#endif
}

/* ---------------------------------------------------------------------- */

typedef	long		index_t;
struct disk_rt_list  {
	index_t		forw;
	index_t		back;
};

/* ---------------------------------------------------------------------- */

struct disk_model {
	index_t			ma_p;
	struct disk_rt_list	r_hd;
};
struct rt_imexport rt_fmt_disk_model[] = {
	"%d",	offsetof(struct disk_model, ma_p),		1,
	"%d",	offsetof(struct disk_model, r_hd),		2,
	"",	0,						0
};

struct disk_model_a {
	long			magic;
};
struct rt_imexport rt_fmt_disk_model_a[] = {
	"%d",	offsetof(struct disk_model_a, magic),		1,
	"",	0,						0
};

struct disk_nmgregion {
	struct disk_rt_list	l;
	index_t   		m_p;
	index_t			ra_p;
	struct disk_rt_list	s_hd;
};
struct rt_imexport rt_fmt_disk_nmgregion[] = {
	"%d",	offsetof(struct disk_nmgregion, l),		2,
	"%d",	offsetof(struct disk_nmgregion, m_p),		1,
	"%d",	offsetof(struct disk_nmgregion, ra_p),		1,
	"%d",	offsetof(struct disk_nmgregion, s_hd),		2,
	"",	0,						0
};

struct disk_nmgregion_a {
	fastf_t			min_pt[3];
	fastf_t			max_pt[3];
};
struct rt_imexport rt_fmt_nmgregion_a[] = {
	"%f",	offsetofarray(struct nmgregion_a, min_pt),	3,
	"%f",	offsetofarray(struct nmgregion_a, max_pt),	3,
	"",	0,						0
};

struct disk_shell {
	struct disk_rt_list	l;
	index_t			r_p;
	index_t			sa_p;
	struct disk_rt_list	fu_hd;
	struct disk_rt_list	lu_hd;
	struct disk_rt_list	eu_hd;
	index_t			vu_p;
};
struct rt_imexport rt_fmt_disk_shell[] = {
	"%d",	offsetof(struct disk_shell, l),			2,
	"%d",	offsetof(struct disk_shell, r_p),		1,
	"%d",	offsetof(struct disk_shell, sa_p),		1,
	"%d",	offsetof(struct disk_shell, fu_hd),		2,
	"%d",	offsetof(struct disk_shell, lu_hd),		2,
	"%d",	offsetof(struct disk_shell, eu_hd),		2,
	"%d",	offsetof(struct disk_shell, vu_p),		1,
	"",	0,						0
};

struct disk_shell_a {
	fastf_t			min_pt[3];
	fastf_t			max_pt[3];
};
struct rt_imexport rt_fmt_disk_shell_a[] = {
	"%f",	offsetofarray(struct disk_shell_a, min_pt),	3,
	"%f",	offsetofarray(struct disk_shell_a, max_pt),	3,
	"",	0,						0
};

struct disk_face {
	index_t			fu_p;
	index_t			fg_p;
};
struct rt_imexport rt_fmt_disk_face[] = {
	"%d",	offsetof(struct disk_face, fu_p),		1,
	"%d",	offsetof(struct disk_face, fg_p),		1,
	"",	0,						0
};

struct disk_face_g {
	fastf_t			N[4];
	fastf_t			min_pt[3];
	fastf_t			max_pt[3];
};
struct rt_imexport rt_fmt_disk_face_g[] = {
	"%f",	offsetofarray(struct disk_face_g, N),		4,
	"%f",	offsetofarray(struct disk_face_g, min_pt),	3,
	"%f",	offsetofarray(struct disk_face_g, max_pt),	3,
	"",	0,						0
};

struct disk_faceuse {
	struct disk_rt_list	l;
	index_t			s_p;
	index_t			fumate_p;
	int			orientation;
	index_t			f_p;
	index_t			fua_p;
	struct disk_rt_list	lu_hd;
};
struct rt_imexport rt_fmt_faceuse[] = {
	"%d",	offsetof(struct faceuse, l),			2,
	"%d",	offsetof(struct faceuse, s_p),			1,
	"%d",	offsetof(struct faceuse, fumate_p),		1,
	"%d",	offsetof(struct faceuse, orientation),		1,
	"%d",	offsetof(struct faceuse, f_p),			1,
	"%d",	offsetof(struct faceuse, fua_p),		1,
	"%d",	offsetof(struct faceuse, lu_hd),		2,
	"",	0,						0
};

struct disk_faceuse_a {
	long			magic;
};
struct rt_imexport rt_fmt_faceuse_a[] = {
	"%d",	offsetof(struct faceuse_a, magic),		1,
	"",	0,						0
};

struct disk_loop {
	index_t			lu_p;
	index_t			lg_p;
};
struct rt_imexport rt_fmt_disk_loop[] = {
	"%d",	offsetof(struct disk_loop, lu_p),		1,
	"%d",	offsetof(struct disk_loop, lg_p),		1,
	"",	0,						0
};

struct disk_loop_g {
	fastf_t			min_pt[3];
	fastf_t			max_pt[3];
};
struct rt_imexport rt_fmt_disk_loop_g[] = {
	"%f",	offsetofarray(struct disk_loop_g, min_pt),	3,
	"%f",	offsetofarray(struct disk_loop_g, max_pt),	3,
	"",	0,						0
};

struct disk_loopuse {
	struct disk_rt_list	l;
	index_t			up;
	index_t			lumate_p;
	int			orientation;
	index_t			l_p;
	index_t			lua_p;
	struct disk_rt_list	down_hd;
};
struct rt_imexport rt_fmt_disk_loopuse[] = {
	"%d",	offsetof(struct disk_loopuse, l),		2,
	"%d",	offsetof(struct disk_loopuse, up),		1,
	"%d",	offsetof(struct disk_loopuse, lumate_p),	1,
	"%d",	offsetof(struct disk_loopuse, orientation),	1,
	"%d",	offsetof(struct disk_loopuse, l_p),		1,
	"%d",	offsetof(struct disk_loopuse, lua_p),		1,
	"%d",	offsetof(struct disk_loopuse, down_hd),		2,
	"",	0,						0
};

struct disk_loopuse_a {
	long			magic;
};
struct rt_imexport rt_fmt_disk_loopuse_a[] = {
	"%d",	offsetof(struct loopuse_a, magic),		1,
	"",	0,						0
};

struct disk_edge {
	index_t			eu_p;
	index_t			eg_p;
};
struct rt_imexport rt_fmt_disk_edge[] = {
	"%d",	offsetof(struct disk_edge, eu_p),		1,
	"%d",	offsetof(struct disk_edge, eg_p),		1,
	"",	0,						0
};

struct disk_edge_g {
	long			magic;
};
struct rt_imexport rt_fmt_disk_edge_g[] = {
	"%d",	offsetof(struct disk_edge_g, magic),		1,
	"",	0,						0
};

struct disk_edgeuse {
	struct disk_rt_list	l;
	index_t			up;
	index_t			eumate_p;
	index_t			radial_p;
	index_t			e_p;
	index_t			eua_p;
	int	  		orientation;
	index_t			vu_p;
};
struct rt_imexport rt_fmt_edgeuse[] = {
	"%d",	offsetof(struct edgeuse, l),			2,
	"%d",	offsetof(struct edgeuse, up),			1,
	"%d",	offsetof(struct edgeuse, eumate_p),		1,
	"%d",	offsetof(struct edgeuse, radial_p),		1,
	"%d",	offsetof(struct edgeuse, e_p),			1,
	"%d",	offsetof(struct edgeuse, eua_p),		1,
	"%d",	offsetof(struct edgeuse, orientation),		1,
	"%d",	offsetof(struct edgeuse, vu_p),			1,
	"",	0,						0
};

struct disk_edgeuse_a {
	long			magic;
};
struct rt_imexport rt_fmt_edgeuse_a[] = {
	"%d",	offsetof(struct edgeuse_a, magic),		1,
	"",	0,						0
};

struct disk_vertex {
	struct disk_rt_list	vu_hd;
	index_t			vg_p;
};
struct rt_imexport rt_fmt_disk_vertex[] = {
	"%d",	offsetof(struct disk_vertex, vu_hd),		2,
	"%d",	offsetof(struct disk_vertex, vg_p),		1,
	"",	0,						0
};

struct disk_vertex_g {
	fastf_t			coord[3];
};
struct rt_imexport rt_fmt_disk_vertex_g[] = {
	"%f",	offsetofarray(struct disk_vertex_g, coord),	3,
	"",	0,						0
};

struct disk_vertexuse {
	struct disk_rt_list	l;
	index_t			up;
	index_t			v_p;
	index_t			vua_p;
};
struct rt_imexport rt_fmt_vertexuse[] = {
	"%d",	offsetof(struct vertexuse, l),			2,
	"%d",	offsetof(struct vertexuse, up),			1,
	"%d",	offsetof(struct vertexuse, v_p),		1,
	"%d",	offsetof(struct vertexuse, vua_p),		1,
	"",	0,						0
};

struct disk_vertexuse_a {
	fastf_t			N[3];
};
struct rt_imexport rt_fmt_vertexuse_a[] = {
	"%f",	offsetofarray(struct vertexuse_a, N),		3,
	"",	0,						0
};

/* ---------------------------------------------------------------------- */
#define NSTRUCTS	22
int	rt_disk_sizes[NSTRUCTS] = {
	sizeof(struct disk_model),
	sizeof(struct disk_model_a),
	sizeof(struct disk_nmgregion),
	sizeof(struct disk_nmgregion_a),
	sizeof(struct disk_shell),
	sizeof(struct disk_shell_a),
	sizeof(struct disk_faceuse),
	sizeof(struct disk_faceuse_a),
	sizeof(struct disk_face),
	sizeof(struct disk_face_g),
	sizeof(struct disk_loopuse),
	sizeof(struct disk_loopuse_a),
	sizeof(struct disk_loop),
	sizeof(struct disk_loop_g),
	sizeof(struct disk_edgeuse),
	sizeof(struct disk_edgeuse_a),
	sizeof(struct disk_edge),
	sizeof(struct disk_edge_g),
	sizeof(struct disk_vertexuse),
	sizeof(struct disk_vertexuse_a),
	sizeof(struct disk_vertex),
	sizeof(struct disk_vertex_g)
};
/* ---------------------------------------------------------------------- */

int
rt_nmg_magic_to_kind( magic )
register long	magic;
{
	switch(magic)  {
	case NMG_MODEL_MAGIC:
		return 0;
	case NMG_MODEL_A_MAGIC:
		return 1;
	case NMG_REGION_MAGIC:
		return 2;
	case NMG_REGION_A_MAGIC:
		return 3;
	case NMG_SHELL_MAGIC:
		return 4;
	case NMG_SHELL_A_MAGIC:
		return 5;
	case NMG_FACEUSE_MAGIC:
		return 6;
	case NMG_FACEUSE_A_MAGIC:
		return 7;
	case NMG_FACE_MAGIC:
		return 8;
	case NMG_FACE_G_MAGIC:
		return 9;
	case NMG_LOOPUSE_MAGIC:
		return 10;
	case NMG_LOOPUSE_A_MAGIC:
		return 11;
	case NMG_LOOP_MAGIC:
		return 12;
	case NMG_LOOP_G_MAGIC:
		return 13;
	case NMG_EDGEUSE_MAGIC:
		return 14;
	case NMG_EDGEUSE_A_MAGIC:
		return 15;
	case NMG_EDGE_MAGIC:
		return 16;
	case NMG_EDGE_G_MAGIC:
		return 17;
	case NMG_VERTEXUSE_MAGIC:
		return 18;
	case NMG_VERTEXUSE_A_MAGIC:
		return 19;
	case NMG_VERTEX_MAGIC:
		return 20;
	case NMG_VERTEX_G_MAGIC:
		return 21;
	}
	/* default */
	rt_log("magic = x%x\n", magic);
	rt_bomb("rt_nmg_magic_to_kind: bad magic");
	return -1;
}

int
rt_nmg_index_of_struct( p )
register long	*p;
{
	switch(*p)  {
	case NMG_MODEL_MAGIC:
		return ((struct model *)p)->index;
	case NMG_MODEL_A_MAGIC:
		return ((struct model_a *)p)->index;
	case NMG_REGION_MAGIC:
		return ((struct nmgregion *)p)->index;
	case NMG_REGION_A_MAGIC:
		return ((struct nmgregion_a *)p)->index;
	case NMG_SHELL_MAGIC:
		return ((struct shell *)p)->index;
	case NMG_SHELL_A_MAGIC:
		return ((struct shell_a *)p)->index;
	case NMG_FACEUSE_MAGIC:
		return ((struct faceuse *)p)->index;
	case NMG_FACEUSE_A_MAGIC:
		return ((struct faceuse_a *)p)->index;
	case NMG_FACE_MAGIC:
		return ((struct face *)p)->index;
	case NMG_FACE_G_MAGIC:
		return ((struct face_g *)p)->index;
	case NMG_LOOPUSE_MAGIC:
		return ((struct loopuse *)p)->index;
	case NMG_LOOPUSE_A_MAGIC:
		return ((struct loopuse_a *)p)->index;
	case NMG_LOOP_MAGIC:
		return ((struct loop *)p)->index;
	case NMG_LOOP_G_MAGIC:
		return ((struct loop_g *)p)->index;
	case NMG_EDGEUSE_MAGIC:
		return ((struct edgeuse *)p)->index;
	case NMG_EDGEUSE_A_MAGIC:
		return ((struct edgeuse_a *)p)->index;
	case NMG_EDGE_MAGIC:
		return ((struct edge *)p)->index;
	case NMG_EDGE_G_MAGIC:
		return ((struct edge_g *)p)->index;
	case NMG_VERTEXUSE_MAGIC:
		return ((struct vertexuse *)p)->index;
	case NMG_VERTEXUSE_A_MAGIC:
		return ((struct vertexuse_a *)p)->index;
	case NMG_VERTEX_MAGIC:
		return ((struct vertex *)p)->index;
	case NMG_VERTEX_G_MAGIC:
		return ((struct vertex_g *)p)->index;
	}
	/* default */
	rt_log("magicp = x%x, magic = x%x\n", p, *p);
	rt_bomb("rt_nmg_index_of_struct: bad magic");
	return -1;
}

struct nmg_exp_counts {
	long	new_subscript;
	long	per_struct_index;
	int	kind;
};

#define REINDEX(p)	(ecnt[nmg_index_of_struct((long *)(p))].new_subscript)
#define INDEX(o,i,elem)		(o)->elem = REINDEX((i)->elem)
#define INDEXL(oo,ii,elem)	{ \
	(oo)->elem.forw = REINDEX((ii)->elem.forw); \
	(oo)->elem.back = REINDEX((ii)->elem.back); }

rt_nmg_edisk( op, ip, ecnt, index )
genptr_t	op;		/* base of disk array */
genptr_t	ip;		/* ptr to in-memory structure */
struct nmg_exp_counts	*ecnt;
int		index;
{
	int	oindex;		/* index in op */

	oindex = ecnt[index].per_struct_index;
	switch(ecnt[index].kind)  {
	case 0:
		/* model */
		{
			struct model	*m = (struct model *)ip;
			struct disk_model	*d;
			d = &((struct disk_model *)op)[oindex];
			NMG_CK_MODEL(m);
			INDEX( d, m, ma_p );
			INDEXL( d, m, r_hd );
		}
		return;
	case 1:
		/* model_a */
		{
			struct model_a	*ma = (struct model_a *)ip;
			struct disk_model_a	*d;
			d = &((struct disk_model_a *)op)[oindex];
			NMG_CK_MODEL_A(ma);
			d->magic = ma->magic;	/* NOOP */
		}
		return;
	case 2:
		/* region */
		{
			struct nmgregion	*r = (struct nmgregion *)ip;
			struct disk_nmgregion	*d;
			d = &((struct disk_nmgregion *)op)[oindex];
			NMG_CK_REGION(r);
			INDEXL( d, r, l );
			INDEX( d, r, m_p );
			INDEX( d, r, ra_p );
			INDEXL( d, r, s_hd );
		}
		return;
	case 3:
		/* region_a */
		{
			struct nmgregion_a	*r = (struct nmgregion_a *)ip;
			struct disk_nmgregion_a	*d;
			d = &((struct disk_nmgregion_a *)op)[oindex];
			NMG_CK_REGION_A(r);
			VMOVE( d->min_pt, r->min_pt );
			VMOVE( d->max_pt, r->max_pt );
		}
		return;
	case 4:
		/* shell */
		{
			struct shell	*s = (struct shell *)ip;
			struct disk_shell	*d;
			d = &((struct disk_shell *)op)[oindex];
			NMG_CK_SHELL(s);
			INDEXL( d, s, l );
			INDEX( d, s, r_p );
			INDEX( d, s, sa_p );
			INDEXL( d, s, fu_hd );
			INDEXL( d, s, lu_hd );
			INDEXL( d, s, eu_hd );
			INDEX( d, s, vu_p );
		}
		return;
	case 5:
		/* shell_a */
		{
			struct shell_a	*sa = (struct shell_a *)ip;
			struct disk_shell_a	*d;
			d = &((struct disk_shell_a *)op)[oindex];
			NMG_CK_SHELL_A(sa);
			VMOVE( d->min_pt, sa->min_pt );
			VMOVE( d->max_pt, sa->max_pt );
		}
		return;
	case 6:
		/* faceuse */
		{
			struct faceuse	*fu = (struct faceuse *)ip;
			struct disk_faceuse	*d;
			d = &((struct disk_faceuse *)op)[oindex];
			NMG_CK_FACEUSE(fu);
			INDEXL( d, fu, l );
			INDEX( d, fu, s_p );
			INDEX( d, fu, fumate_p );
			d->orientation = fu->orientation;
			INDEX( d, fu, f_p );
			INDEX( d, fu, fua_p );
			INDEXL( d, fu, lu_hd );
		}
		return;
	case 7:
		/* faceuse_a */
		{
			struct faceuse_a	*fua = (struct faceuse_a *)ip;
			struct disk_faceuse_a	*d;
			d = &((struct disk_faceuse_a *)op)[oindex];
			NMG_CK_FACEUSE_A(fua);
			d->magic = fua->magic;	/* NOOP */
		}
		return;
	case 8:
		/* face */
		{
			struct face	*f = (struct face *)ip;
			struct disk_face	*d;
			d = &((struct disk_face *)op)[oindex];
			NMG_CK_FACE(f);
			INDEX( d, f, fu_p );
			INDEX( d, f, fg_p );
		}
		return;
	case 9:
		/* face_g */
		{
			struct face_g	*fg = (struct face_g *)ip;
			struct disk_face_g	*d;
			d = &((struct disk_face_g *)op)[oindex];
			NMG_CK_FACE_G(fg);
			VMOVEN( d->N, fg->N, 4 );
			VMOVE( d->min_pt, fg->min_pt );
			VMOVE( d->max_pt, fg->max_pt );
		}
		return;
	case 10:
		/* loopuse */
		{
			struct loopuse	*lu = (struct loopuse *)ip;
			struct disk_loopuse	*d;
			d = &((struct disk_loopuse *)op)[oindex];
			NMG_CK_LOOPUSE(lu);
			INDEXL( d, lu, l );
			d->up = REINDEX(lu->up.magic_p);
			INDEX( d, lu, lumate_p );
			d->orientation = lu->orientation;
			INDEX( d, lu, l_p );
			INDEX( d, lu, lua_p );
			INDEXL( d, lu, down_hd );
		}
		return;
	case 11:
		/* loopuse_a */
		{
			struct loopuse_a	*lua = (struct loopuse_a *)ip;
			struct disk_loopuse_a	*d;
			d = &((struct disk_loopuse_a *)op)[oindex];
			NMG_CK_LOOPUSE_A(lua);
			d->magic = lua->magic;	/* NOOP */
		}
		return;
	case 12:
		/* loop */
		{
			struct loop	*l = (struct loop *)ip;
			struct disk_loop	*d;
			d = &((struct disk_loop *)op)[oindex];
			NMG_CK_LOOP(l);
			INDEX( d, l, lu_p );
			INDEX( d, l, lg_p );
		}
		return;
	case 13:
		/* loop_g */
		{
			struct loop_g	*lg = (struct loop_g *)ip;
			struct disk_loop_g	*d;
			d = &((struct disk_loop_g *)op)[oindex];
			NMG_CK_LOOP_G(lg);
			VMOVE( d->min_pt, lg->min_pt );
			VMOVE( d->max_pt, lg->max_pt );
		}
		return;
	case 14:
		/* edgeuse */
		{
			struct edgeuse	*eu = (struct edgeuse *)ip;
			struct disk_edgeuse	*d;
			d = &((struct disk_edgeuse *)op)[oindex];
			NMG_CK_EDGEUSE(eu);
			INDEXL( d, eu, l );
			d->up = REINDEX(eu->up.magic_p);
			INDEX( d, eu, eumate_p );
			INDEX( d, eu, radial_p );
			INDEX( d, eu, e_p );
			INDEX( d, eu, eua_p );
			d->orientation = eu->orientation;
			INDEX( d, eu, vu_p );
		}
		return;
	case 15:
		/* edgeuse_a */
		{
			struct edgeuse_a	*eua = (struct edgeuse_a *)ip;
			struct disk_edgeuse_a	*d;
			d = &((struct disk_edgeuse_a *)op)[oindex];
			NMG_CK_EDGEUSE_A(eua);
			d->magic = eua->magic;	/* NOOP */
		}
		return;
	case 16:
		/* edge */
		{
			struct edge	*e = (struct edge *)ip;
			struct disk_edge	*d;
			d = &((struct disk_edge *)op)[oindex];
			NMG_CK_EDGE(e);
			INDEX( d, e, eu_p );
			INDEX( d, e, eg_p );
		}
		return;
	case 17:
		/* edge_g */
		{
			struct edge_g	*eg = (struct edge_g *)ip;
			struct disk_edge_g	*d;
			d = &((struct disk_edge_g *)op)[oindex];
			NMG_CK_EDGE_G(eg);
			d->magic = eg->magic;	/* NOOP */
		}
		return;
	case 18:
		/* vertexuse */
		{
			struct vertexuse	*vu = (struct vertexuse *)ip;
			struct disk_vertexuse	*d;
			d = &((struct disk_vertexuse *)op)[oindex];
			NMG_CK_VERTEXUSE(vu);
			INDEXL( d, vu, l );
			d->up = REINDEX(vu->up.magic_p);
			INDEX( d, vu, v_p );
			INDEX( d, vu, vua_p );
		}
		return;
	case 19:
		/* vertexuse_a */
		{
			struct vertexuse_a	*vua = (struct vertexuse_a *)ip;
			struct disk_vertexuse_a	*d;
			d = &((struct disk_vertexuse_a *)op)[oindex];
			NMG_CK_VERTEXUSE_A(vua);
			VMOVEN( d->N, vua->N, 4 );
		}
		return;
	case 20:
		/* vertex */
		{
			struct vertex	*v = (struct vertex *)ip;
			struct disk_vertex	*d;
			d = &((struct disk_vertex *)op)[oindex];
			NMG_CK_VERTEX(v);
			INDEXL( d, v, vu_hd );
			INDEX( d, v, vg_p );
		}
		return;
	case 21:
		/* vertex_g */
		{
			struct vertex_g	*vg = (struct vertex_g *)ip;
			struct disk_vertex_g	*d;
			d = &((struct disk_vertex_g *)op)[oindex];
			NMG_CK_VERTEX_G(vg);
			VMOVE( d->coord, vg->coord );
		}
		return;
	}
}

/*
 *			R T _ N M G _ E X P O R T
 *
 *  The name is added by the caller, in the usual place.
 */
int
rt_nmg_export( ep, ip, local2mm )
struct rt_external	*ep;
struct rt_db_internal	*ip;
double			local2mm;
{
	struct model			*m;
	union record			*rp;
	struct nmg_struct_counts	cntbuf;
	struct rt_external		count_ext;
	long				**ptrs;
	struct nmg_exp_counts		*ecnt;
	int				i;
	int				subscript;
	int				indices[NSTRUCTS];
	genptr_t			disk_arrays[NSTRUCTS];
	int				kind;

	RT_CK_DB_INTERNAL(ip);
	if( ip->idb_type != ID_NMG )  return(-1);
	m = (struct model *)ip->idb_ptr;
	NMG_CK_MODEL(m);

	bzero( (char *)&cntbuf, sizeof(cntbuf) );
	ptrs = nmg_m_struct_count( &cntbuf, m );
	nmg_pr_struct_counts( &cntbuf, "Counts in rt_nmg_export" );

	/* Collect overall new subscripts, and structure-specific indices */
	ecnt = (struct nmg_exp_counts *)rt_calloc( m->maxindex,
		sizeof(struct nmg_exp_counts), "ecnt[]" );
	for( i = 0; i < NSTRUCTS; i++ )
		indices[i] = 0;
	subscript = 1;
	for( i = m->maxindex-1; i >= 0; i-- )  {
		if( ptrs[i] == (long *)0 )  continue;
		kind = rt_nmg_magic_to_kind( *(ptrs[i]) );
		ecnt[i].per_struct_index = indices[kind]++;
		ecnt[i].kind = kind;
	}
	/* Assign new subscripts to ascending guys of same kind */
	for( kind = 0; kind < NSTRUCTS; kind++ )  {
		for( i=0; i < m->maxindex-1; i++ )  {
			if( ecnt[i].kind != kind )  continue;
			ecnt[i].new_subscript = subscript++;
		}
	}

	for( i = 0; i < NSTRUCTS; i++ )  {
		rt_log("%d of kind %d\n", indices[i], i);
		if( indices[i] <= 0 )  {
			disk_arrays[i] = GENPTR_NULL;
			continue;
		}
		disk_arrays[i] = (genptr_t)rt_calloc( indices[i],
			rt_disk_sizes[i], "disk_arrays[i]" );
	}

	/* Convert all the structures to their disk versions */
	for( i = m->maxindex-1; i >= 0; i-- )  {
		if( ptrs[i] == (long *)0 )  continue;
		kind = ecnt[i].kind;
		rt_nmg_edisk( disk_arrays[kind], ptrs[i], ecnt, i );
	}

	RT_INIT_EXTERNAL(ep);
	ep->ext_nbytes = sizeof(union record);
	ep->ext_buf = (genptr_t)rt_calloc( 1, ep->ext_nbytes, "nmg external");
	rp = (union record *)ep->ext_buf;
	rp->nmg.N_id = DBID_NMG;
	rp->nmg.N_count = 0;		/* XXX for now */

	RT_INIT_EXTERNAL(&count_ext);
	if( rt_struct_export( &count_ext, (genptr_t)&cntbuf, rt_nmg_structs_fmt ) < 0 )  {
		rt_log("rt_struct_export() failure\n");
		return(-1);
	}
	if( count_ext.ext_nbytes > sizeof(rp->nmg.N_structs) )
		rt_bomb("nmg.N_structs overflow");
	bcopy( count_ext.ext_buf, rp->nmg.N_structs, count_ext.ext_nbytes );
	db_free_external( &count_ext );

	return(0);
}

/*
 *			R T _ N M G _ D E S C R I B E
 *
 *  Make human-readable formatted presentation of this solid.
 *  First line describes type of solid.
 *  Additional lines are indented one tab, and give parameter values.
 */
int
rt_nmg_describe( str, ip, verbose, mm2local )
struct rt_vls		*str;
struct rt_db_internal	*ip;
int			verbose;
double			mm2local;
{
	register struct model	*m =
		(struct model *)ip->idb_ptr;
	char	buf[256];

	NMG_CK_MODEL(m);
	rt_vls_strcat( str, "truncated general nmg (NMG)\n");

#if 0
	sprintf(buf, "\tV (%g, %g, %g)\n",
		m->v[X] * mm2local,
		m->v[Y] * mm2local,
		m->v[Z] * mm2local );
	rt_vls_strcat( str, buf );
#endif

	return(0);
}

/*
 *			R T _ N M G _ I F R E E
 *
 *  Free the storage associated with the rt_db_internal version of this solid.
 */
void
rt_nmg_ifree( ip )
struct rt_db_internal	*ip;
{
	register struct model	*m;

	RT_CK_DB_INTERNAL(ip);
	m = (struct model *)ip->idb_ptr;
	NMG_CK_MODEL(m);
	nmg_km( m );

	ip->idb_ptr = GENPTR_NULL;	/* sanity */
}
