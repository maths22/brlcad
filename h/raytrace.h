/*
 *			R A Y T R A C E . H
 *
 *  All the data structures and manifest constants
 *  necessary for interacting with the BRL-CAD LIBRT ray-tracing library.
 *
 *  Note that this header file defines many internal data structures,
 *  as well as the external (interface) data structures.  These are
 *  provided for the convenience of applications builders.  However,
 *  the internal data structures are subject to change.
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
 *
 *  $Header$
 */

#ifndef SEEN_RTLIST_H
# include "rtlist.h"
#endif

#ifndef RAYTRACE_H
#define RAYTRACE_H seen

#define RAYTRACE_H_VERSION	"@(#)$Header$ (BRL)"

/*
 *  It is necessary to have a representation of 1.0/0.0, or "infinity"
 *  that fits within the dynamic range of the machine being used.
 *  This constant places an upper bound on the size object which
 *  can be represented in the model.
 */
#if defined(vax) || (defined(sgi) && !defined(mips))
#	define INFINITY	(1.0e20)	/* VAX limit is 10**37 */
#else
#	define INFINITY	(1.0e40)	/* IBM limit is 10**75 */
#endif

/*
 *  Unfortunately, to prevent divide-by-zero, some tolerancing
 *  needs to be introduced.
 *
 *  RT_LEN_TOL is the shortest length, in mm, that can be stood
 *  as the dimensions of a primitive.
 *  Can probably become at least SMALL.
 *
 *  Dot products smaller than RT_DOT_TOL are considered to have
 *  a dot product of zero, i.e., the angle is effectively zero.
 *  This is used to check vectors that should be perpendicular.
 *  asin(0.1   ) = 5.73917 degrees
 *  asin(0.01  ) = 0.572967
 *  asin(0.001 ) = 0.0572958 degrees
 *  asin(0.0001) = 0.00572958 degrees
 *
 *  sin(0.01 degrees) = sin(0.000174 radians) = 0.000174533
 *
 *  Many TGCs at least, in existing databases, will fail the
 *  perpendicularity test if DOT_TOL is much smaller than 0.001,
 *  which establishes a 1/20th degree tolerance.
 *  The intent is to eliminate grossly bad primitives, not pick nits.
 *
 *  RT_PCOEF_TOL is a tolerance on polynomial coefficients to prevent
 *  the root finder from having heartburn.
 */
#define RT_LEN_TOL	(1.0e-8)
#define RT_DOT_TOL	(0.001)
#define RT_PCOEF_TOL	(1.0e-10)

/*
 *  Macros for providing function prototypes, regardless of whether
 *  the compiler understands them or not.
 *  It is vital that the argument list given for "args" be enclosed
 *  in parens.
 */
#if __STDC__
#	define	RT_EXTERN(type_and_name,args)	extern type_and_name args
#	define	RT_ARGS(args)			args
#else
#	define	RT_EXTERN(type_and_name,args)	extern type_and_name()
#	define	RT_ARGS(args)			()
#endif

/*
 * Handy memory allocator
 */

/* Acquire storage for a given struct, eg, GETSTRUCT(ptr,structname); */
#if __STDC__ && !alliant && !apollo
# define GETSTRUCT(p,str) \
	p = (struct str *)rt_calloc(1,sizeof(struct str), "getstruct " #str)
# define GETUNION(p,unn) \
	p = (union unn *)rt_calloc(1,sizeof(union unn), "getstruct " #unn)
#else
# define GETSTRUCT(p,str) \
	p = (struct str *)rt_calloc(1,sizeof(struct str), "getstruct str")
# define GETUNION(p,unn) \
	p = (union unn *)rt_calloc(1,sizeof(union unn), "getstruct unn")
#endif

/*
 *			X R A Y
 *
 * All necessary information about a ray.
 * Not called just "ray" to prevent conflicts with VLD stuff.
 */
struct xray {
	point_t		r_pt;		/* Point at which ray starts */
	vect_t		r_dir;		/* Direction of ray (UNIT Length) */
	fastf_t		r_min;		/* entry dist to bounding sphere */
	fastf_t		r_max;		/* exit dist from bounding sphere */
};
#define RAY_NULL	((struct xray *)0)


/*
 *			H I T
 *
 *  Information about where a ray hits the surface
 *
 * Important Note:  Surface Normals always point OUT of a solid.
 */
struct hit {
	fastf_t		hit_dist;	/* dist from r_pt to hit_point */
	point_t		hit_point;	/* Intersection point */
	vect_t		hit_normal;	/* Surface Normal at hit_point */
	vect_t		hit_vpriv;	/* PRIVATE vector for xxx_*() */
	genptr_t	hit_private;	/* PRIVATE handle for xxx_shot() */
	int		hit_surfno;	/* solid-specific surface indicator */
};
#define HIT_NULL	((struct hit *)0)


/*
 *			C U R V A T U R E
 *
 *  Information about curvature of the surface at a hit point.
 *  The principal direction pdir has unit length and principal curvature c1.
 *  |c1| <= |c2|, i.e. c1 is the most nearly flat principle curvature.
 *  A POSITIVE curvature indicates that the surface bends TOWARD the
 *  (outward pointing) normal vector at that point.
 *  c1 and c2 are the inverse radii of curvature.
 *  The other principle direction is implied: pdir2 = normal x pdir1.
 */
struct curvature {
	vect_t		crv_pdir;	/* Principle direction */
	fastf_t		crv_c1;		/* curvature in principle dir */
	fastf_t		crv_c2;		/* curvature in other direction */
};
#define CURVE_NULL	((struct curvature *)0)

/*
 *  Use this macro after having computed the normal, to
 *  compute the curvature at a hit point.
 */
#define RT_CURVE( curvp, hitp, stp )  { \
	register int id = (stp)->st_id; \
	RT_CHECK_SOLTAB(stp); \
	if( id <= 0 || id > ID_MAXIMUM )  { \
		rt_log("stp=x%x, id=%d.\n", stp, id); \
		rt_bomb("RT_CURVE:  bad st_id"); \
	} \
	rt_functab[id].ft_curve( curvp, hitp, stp ); }

/*
 *			U V C O O R D
 *
 *  Mostly for texture mapping, information about parametric space.
 */
struct uvcoord {
	fastf_t		uv_u;		/* Range 0..1 */
	fastf_t		uv_v;		/* Range 0..1 */
	fastf_t		uv_du;		/* delta in u */
	fastf_t		uv_dv;		/* delta in v */
};
#define RT_HIT_UVCOORD( ap, stp, hitp, uvp )  { \
	register int id = (stp)->st_id; \
	RT_CHECK_SOLTAB(stp); \
	if( id <= 0 || id > ID_MAXIMUM )  { \
		rt_log("stp=x%x, id=%d.\n", stp, id); \
		rt_bomb("RT_UVCOORD:  bad st_id"); \
	} \
	rt_functab[id].ft_uv( ap, stp, hitp, uvp ); }


/*
 *			S E G
 *
 * Intersection segment.
 *
 * Includes information about both endpoints of intersection.
 * Contains forward link to additional intersection segments
 * if the intersection spans multiple segments (eg, shooting
 * a ray through a torus).
 */
struct seg {
	struct rt_list	l;
	struct hit	seg_in;		/* IN information */
	struct hit	seg_out;	/* OUT information */
	struct soltab	*seg_stp;	/* pointer back to soltab */
};
#define RT_SEG_NULL	((struct seg *)0)
#define RT_SEG_MAGIC	0x98bcdef1

#define RT_CHECK_SEG(_p)	{ \
	if( (_p)->l.magic != RT_SEG_MAGIC )  { \
		rt_log("RT_CHECK_SEG(x%x) magic was x%x, s/b x%x, %s line %d\n", \
			(_p), (_p)->l.magic, RT_SEG_MAGIC, \
			__FILE__, __LINE__ ); \
		rt_bomb("bad seg ptr\n"); \
	} }

#define RT_GET_SEG(p,res)    { \
	while( !RT_LIST_LOOP((p),seg,&((res)->re_seg.l)) || !(p) ) \
		rt_get_seg(res); \
	RT_LIST_DEQUEUE( &((p)->l) ); \
	(p)->l.forw = (p)->l.back = RT_LIST_NULL; \
	res->re_segget++; }

#define RT_FREE_SEG(p,res)  { \
	RT_CHECK_SEG(p); \
	RT_LIST_INSERT( &((res)->re_seg.l), &((p)->l) ); \
	res->re_segfree++; }

#define RT_FREE_SEG_LIST( _segheadp, _res )	{ \
	register struct seg *_a; \
	while( RT_LIST_LOOP( _a, seg, &((_segheadp)->l) ) )  { \
		RT_LIST_DEQUEUE( &(_a->l) ); \
		RT_FREE_SEG( _a, _res ); \
	} }


/*
 *			S O L T A B
 *
 * Internal information used to keep track of solids in the model
 * Leaf name and Xform matrix are unique identifier.
 */
struct soltab {
	struct rt_list	l;		/* links, headed by rti_headsolid */
	int		st_id;		/* Solid ident */
	vect_t		st_center;	/* Centroid of solid */
	fastf_t		st_aradius;	/* Radius of APPROXIMATING sphere */
	fastf_t		st_bradius;	/* Radius of BOUNDING sphere */
	genptr_t	st_specific;	/* -> ID-specific (private) struct */
	struct directory *st_dp;	/* Directory entry of solid */
	char		*st_name;	/* Leaf name of solid */
	vect_t		st_min;		/* min X, Y, Z of bounding RPP */
	vect_t		st_max;		/* max X, Y, Z of bounding RPP */
	int		st_bit;		/* solids bit vector index (const) */
	int		st_maxreg;	/* highest bit set in st_regions */
	bitv_t		*st_regions;	/* bit vect of region #'s (const) */
	mat_t		st_pathmat;	/* Xform matrix on path */
};
#define st_name		st_dp->d_namep
#define RT_SOLTAB_NULL	((struct soltab *)0)
#define	SOLTAB_NULL	RT_SOLTAB_NULL	/* backwards compat */
#define RT_SOLTAB_MAGIC	0x92bfcde0

#define RT_CHECK_SOLTAB(_p)	{ \
	if( (_p)->l.magic != RT_SOLTAB_MAGIC )  { \
		rt_log("RT_CHECK_SOLTAB(x%x) magic was x%x, s/b x%x, %s line %d\n", \
			(_p), (_p)->l.magic, RT_SOLTAB_MAGIC, \
			__FILE__, __LINE__ ); \
		rt_bomb("bad soltab ptr\n"); \
	} }

/*
 *  Values for Solid ID.
 */
#define ID_NULL		0	/* Unused */
#define ID_TOR		1	/* Toroid */
#define ID_TGC		2	/* Generalized Truncated General Cone */
#define ID_ELL		3	/* Ellipsoid */
#define ID_ARB8		4	/* Generalized ARB.  V + 7 vectors */
#define ID_ARS		5	/* ARS */
#define ID_HALF		6	/* Half-space */
#define ID_REC		7	/* Right Elliptical Cylinder [TGC special] */
#define ID_POLY		8	/* Polygonal facted object */
#define ID_BSPLINE	9	/* B-spline object */
#define ID_SPH		10	/* Sphere */
#define	ID_STRINGSOL	11	/* String-defined solid */
#define ID_EBM		12	/* Extruded bitmap solid */
#define ID_VOL		13	/* 3-D Volume */
#define ID_ARBN		14	/* ARB with N faces */
#define ID_PIPE		15	/* Pipe (wire) solid */
#define ID_PARTICLE	16	/* Particle system solid */

#define ID_MAXIMUM	16	/* Maximum defined ID_xxx value */

/*
 *			F U N C T A B
 *
 *  Object-oriented interface to geometry modules.
 *  Table is indexed by ID_xxx value of particular solid.
 */
struct rt_functab {
	char	*ft_name;
	int	ft_use_rpp;
	int	(*ft_prep) RT_ARGS((struct soltab *stp, union record *rec,
			struct rt_i *rtip));
	int 	(*ft_shot) RT_ARGS((struct soltab *stp, struct xray *rp,
			struct application *ap, struct seg *seghead));
	void	(*ft_print) RT_ARGS((struct soltab *stp));
	void	(*ft_norm) RT_ARGS((struct hit *hitp, struct soltab *stp,
			struct xray *rp));
	void	(*ft_uv) RT_ARGS((struct application *ap, struct soltab *stp,
			struct hit *hitp, struct uvcoord *uvp));
	void	(*ft_curve) RT_ARGS((struct curvature *cvp, struct hit *hitp,
			struct soltab *stp));
	int	(*ft_classify)();
	void	(*ft_free) RT_ARGS((struct soltab *stp));
	int	(*ft_plot) RT_ARGS((union record *rp, mat_t mat,
			struct vlhead *vhead, struct directory *dp,
			double abs_tol, double rel_tol, double norm_tol));
	void	(*ft_vshot) RT_ARGS((struct soltab *stp[], struct xray *rp[],
			struct seg segp[], int n, struct resource *resp));
	int	(*ft_tessellate) RT_ARGS((struct nmgregion **r,
			struct model *m, union record *rp,
			mat_t mat, struct directory *dp,
			double abs_tol, double rel_tol, double norm_tol));
};
extern struct rt_functab rt_functab[];
extern int rt_nfunctab;


/*
 *			T R E E
 *
 *  Binary trees representing the Boolean operations between solids.
 */
#define MKOP(x)		((x))

#define OP_SOLID	MKOP(1)		/* Leaf:  tr_stp -> solid */
#define OP_UNION	MKOP(2)		/* Binary: L union R */
#define OP_INTERSECT	MKOP(3)		/* Binary: L intersect R */
#define OP_SUBTRACT	MKOP(4)		/* Binary: L subtract R */
#define OP_XOR		MKOP(5)		/* Binary: L xor R, not both*/
#define OP_REGION	MKOP(6)		/* Leaf: tr_stp -> combined_tree_state */
#define OP_NOP		MKOP(7)		/* Leaf with no effect */
/* Internal to library routines */
#define OP_NOT		MKOP(8)		/* Unary:  not L */
#define OP_GUARD	MKOP(9)		/* Unary:  not L, or else! */
#define OP_XNOP		MKOP(10)	/* Unary:  L, mark region */

union tree {
	int	tr_op;		/* Operation */
	struct tree_node {
		int		tb_op;		/* non-leaf */
		struct region	*tb_regionp;	/* ptr to containing region */
		union tree	*tb_left;
		union tree	*tb_right;
	} tr_b;
	struct tree_leaf {
		int		tu_op;		/* leaf, OP_SOLID */
		struct region	*tu_regionp;	/* ptr to containing region */
		struct soltab	*tu_stp;
		char		*tu_name;	/* full path name of leaf */
	} tr_a;
};
/* Things which are in the same place in both structures */
#define tr_regionp	tr_a.tu_regionp

#define TREE_NULL	((union tree *)0)

/*
 *			M A T E R _ I N F O
 */
struct mater_info {
	float	ma_color[3];		/* explicit color:  0..1  */
	char	ma_override;		/* non-0 ==> ma_color is valid */
	char	ma_cinherit;		/* DB_INH_LOWER / DB_INH_HIGHER */
	char	ma_minherit;		/* DB_INH_LOWER / DB_INH_HIGHER */
	char	ma_matname[32];		/* Material name */
	char	ma_matparm[60];		/* String Material parms */
};

/*
 *			R E G I O N
 *
 *  The region structure.
 */
struct region  {
	char		*reg_name;	/* Identifying string */
	union tree	*reg_treetop;	/* Pointer to boolean tree */
	short		reg_bit;	/* constant index into Regions[] */
	short		reg_regionid;	/* Region ID code.  If <=0, use reg_aircode */
	short		reg_aircode;	/* Region ID AIR code */
	short		reg_gmater;	/* GIFT Material code */
	short		reg_los;	/* equivalent LOS estimate ?? */
	struct region	*reg_forw;	/* linked list of all regions */
	struct mater_info reg_mater;	/* Real material information */
	genptr_t	reg_mfuncs;	/* User appl. funcs for material */
	genptr_t	reg_udata;	/* User appl. data for material */
	short		reg_transmit;	/* flag:  material transmits light */
	short		reg_instnum;	/* instance number, from d_uses */
};
#define REGION_NULL	((struct region *)0)

/*
 *  			P A R T I T I O N
 *
 *  Partitions of a ray.  Passed from rt_shootray() into user's
 *  a_hit() function.
 *
 *  Only the hit_dist field of pt_inhit and pt_outhit are valid
 *  when a_hit() is called;  to compute both hit_point and hit_normal,
 *  use RT_HIT_NORM() macro;  to compute just hit_point, use
 *  VJOIN1( hitp->hit_point, rp->r_pt, hitp->hit_dist, rp->r_dir );
 *
 *  NOTE:  rt_get_pt allows enough storage at the end of the partition
 *  for a bit vector of "rt_i.nsolids" bits in length.
 *
 *  NOTE:  The number of solids in a model can change from frame to frame
 *  due to the effect of animations, so partition structures can be expected
 *  to change length over the course of a single application program.
 */
#define RT_HIT_NORM( hitp, stp, rayp )  { \
	register int id = (stp)->st_id; \
	RT_CHECK_SOLTAB(stp); \
	if( id <= 0 || id > ID_MAXIMUM ) { \
		rt_log("stp=x%x, id=%d. hitp=x%x, rayp=x%x\n", stp, id, hitp, rayp); \
		rt_bomb("RT_HIT_NORM:  bad st_id");\
	} \
	rt_functab[id].ft_norm(hitp, stp, rayp); }

struct partition {
	long		pt_magic;		/* sanity check */
	struct seg	*pt_inseg;		/* IN seg ptr (gives stp) */
	struct hit	*pt_inhit;		/* IN hit pointer */
	struct seg	*pt_outseg;		/* OUT seg pointer */
	struct hit	*pt_outhit;		/* OUT hit ptr */
	struct region	*pt_regionp;		/* ptr to containing region */
	struct partition *pt_forw;		/* forwards link */
	struct partition *pt_back;		/* backwards link */
	char		pt_inflip;		/* flip inhit->hit_normal */
	char		pt_outflip;		/* flip outhit->hit_normal */
	int		pt_len;			/* rti_pt_bytes when created */
	bitv_t		pt_solhit[1];		/* VAR bit array:solids hit */
};
#define PT_NULL		((struct partition *)0)
#define PT_MAGIC	0x87687681

#define RT_CHECK_PT(_p)	{ if( (_p)->pt_magic != PT_MAGIC )  { \
				rt_log("RT_CHECK_PT(x%x) magic was x%x, s/b x%x, %s line %d\n", \
					(_p), (_p)->pt_magic, PT_MAGIC, \
					__FILE__, __LINE__ ); \
				rt_bomb("bad partition ptr\n"); \
			} }

#define COPY_PT(ip,out,in)	{ \
	bcopy((char *)in, (char *)out, ip->rti_pt_bytes); }

/* Initialize all the bits to FALSE, clear out structure */
#define GET_PT_INIT(ip,p,res)	{\
	GET_PT(ip,p,res); \
	bzero( ((char *) (p)), (ip)->rti_pt_bytes ); \
	(p)->pt_len = (ip)->rti_pt_bytes; \
	(p)->pt_magic = PT_MAGIC; }

#define GET_PT(ip,p,res)   { \
	while( ((p) = res->re_parthead.pt_forw) == &(res->re_parthead) || \
	    !(p) || (p)->pt_len != (ip)->rti_pt_bytes ) \
		rt_get_pt(ip, res); \
	(p)->pt_magic = PT_MAGIC; \
	DEQUEUE_PT(p); \
	res->re_partget++; }

#define FREE_PT(p,res)  { \
			RT_CHECK_PT(p); \
			(p)->pt_magic = 0; /* sanity */ \
			APPEND_PT( (p), &(res->re_parthead) ); \
			res->re_partfree++; }

#define RT_FREE_PT_LIST( _headp, _res )		{ \
		register struct partition *_pp, *_zap; \
		for( _pp = (_headp)->pt_forw; _pp != (_headp);  )  { \
			_zap = _pp; \
			_pp = _pp->pt_forw; \
			FREE_PT(_zap, _res); \
		} \
		(_headp)->pt_forw = (_headp)->pt_back = (_headp); \
	}

/* Insert "new" partition in front of "old" partition */
#define INSERT_PT(new,old)	{ \
	(new)->pt_back = (old)->pt_back; \
	(old)->pt_back = (new); \
	(new)->pt_forw = (old); \
	(new)->pt_back->pt_forw = (new);  }

/* Append "new" partition after "old" partition */
#define APPEND_PT(new,old)	{ \
	(new)->pt_forw = (old)->pt_forw; \
	(new)->pt_back = (old); \
	(old)->pt_forw = (new); \
	(new)->pt_forw->pt_back = (new);  }

/* Dequeue "cur" partition from doubly-linked list */
#define DEQUEUE_PT(cur)	{ \
	(cur)->pt_forw->pt_back = (cur)->pt_back; \
	(cur)->pt_back->pt_forw = (cur)->pt_forw;  }

/*
 *  Bit vectors
 */
union bitv_elem {
	union bitv_elem	*be_next;
	bitv_t		be_v[2];
};
#define BITV_NULL	((union bitv_elem *)0)

#define GET_BITV(ip,p,res)  { \
			while( ((p)=res->re_bitv) == BITV_NULL ) \
				rt_get_bitv(ip,res); \
			res->re_bitv = (p)->be_next; \
			p->be_next = BITV_NULL; \
			res->re_bitvget++; }

#define FREE_BITV(p,res)  { \
			(p)->be_next = res->re_bitv; \
			res->re_bitv = (p); \
			res->re_bitvfree++; }

/*
 *  Bit-string manipulators for arbitrarily long bit strings
 *  stored as an array of bitv_t's.
 *  BITV_SHIFT and BITV_MASK are defined in machine.h
 */
#define BITS2BYTES(nbits) (((nbits)+BITV_MASK)/8)	/* conservative */
#define BITTEST(lp,bit)	\
	((lp[bit>>BITV_SHIFT] & (((bitv_t)1)<<(bit&BITV_MASK)))?1:0)
#define BITSET(lp,bit)	\
	(lp[bit>>BITV_SHIFT] |= (((bitv_t)1)<<(bit&BITV_MASK)))
#define BITCLR(lp,bit)	\
	(lp[bit>>BITV_SHIFT] &= ~(((bitv_t)1)<<(bit&BITV_MASK)))
#define BITZERO(lp,nbits) bzero((char *)lp, BITS2BYTES(nbits))

#define RT_BITV_BITS2WORDS(_nb)	(((_nb)+BITV_MASK)>>BITV_SHIFT)

/*
 *  Macros to efficiently find all the bits set in a bit vector.
 *  Counts words down, counts bits in words going up, for speed & portability.
 *  It does not matter if the shift causes the sign bit to smear to the right.
 */
#define RT_BITV_LOOP_START(_bitv, _lim)	\
{ \
	register int		_b;	/* Current bit-in-word number */  \
	register bitv_t		_val;	/* Current word value */  \
	register int		_wd;	/* Current word number */  \
	for( _wd=RT_BITV_BITS2WORDS(_lim)-1; _wd>=0; _wd-- )  {  \
		_val = (_bitv)[_wd];  \
		for(_b=0; _val!=0 && _b < BITV_MASK+1; _b++, _val >>= 1 ) { \
			if( !(_val & 1) )  continue;

#define RT_BITV_LOOP_END	\
		} /* end for(_b) */ \
	} /* end for(_wd) */ \
} /* end block */

/* The two registers are combined when needed;  the assumption is
 * that "one" bits are relatively infrequent with respect to zero bits.
 */
#define RT_BITV_LOOP_INDEX	((_wd << BITV_SHIFT) | _b)

/*
 *			C U T
 *
 *  Structure for space subdivision.
 *
 *  cn_type is an integer for efficiency of access in rt_shootray()
 *  on non-word addressing machines.
 */
union cutter  {
#define CUT_CUTNODE	1
#define CUT_BOXNODE	2
	int	cut_type;
	union cutter *cut_forw;		/* Freelist forward link */
	struct cutnode  {
		int	cn_type;
		int	cn_axis;	/* 0,1,2 = cut along X,Y,Z */
		fastf_t	cn_point;	/* cut through axis==point */
		union cutter *cn_l;	/* val < point */
		union cutter *cn_r;	/* val >= point */
	} cn;
	struct boxnode  {
		int	bn_type;
		fastf_t	bn_min[3];
		fastf_t	bn_max[3];
		struct soltab **bn_list;
		short	bn_len;		/* # of solids in list */
		short	bn_maxlen;	/* # of ptrs allocated to list */
	} bn;
};
#define CUTTER_NULL	((union cutter *)0)


/*
 *			M E M _ M A P
 *
 *  These structures are used to manage internal resource maps.
 *  Typically these maps describe some kind of memory or file space.
 */
struct mem_map {
	struct mem_map	*m_nxtp;	/* Linking pointer to next element */
	unsigned	 m_size;	/* Size of this free element */
	unsigned long	 m_addr;	/* Address of start of this element */
};
#define MAP_NULL	((struct mem_map *) 0)


/*
 *  The directory is organized as forward linked lists hanging off of
 *  one of RT_DBNHASH headers in the db_i structure.
 */
#define	RT_DBNHASH		128	/* size of hash table */

#if	((RT_DBNHASH)&((RT_DBNHASH)-1)) != 0
#define	RT_DBHASH(sum)	((unsigned)(sum) % (RT_DBNHASH))
#else
#define	RT_DBHASH(sum)	((unsigned)(sum) & ((RT_DBNHASH)-1))
#endif

/*
 *			D B _ I
 *
 *  One of these structures is used to describe each separate instance
 *  of a model database file.
 */
struct db_i  {
	int			dbi_magic;	/* magic number */
	struct directory	*dbi_Head[RT_DBNHASH];
	int			dbi_fd;		/* UNIX file descriptor */
	FILE			*dbi_fp;	/* STDIO file descriptor */
	long			dbi_eof;	/* End+1 pos after db_scan() */
	long			dbi_nrec;	/* # records after db_scan() */
	int			dbi_localunit;	/* unit currently in effect */
	double			dbi_local2base;
	double			dbi_base2local;	/* unit conversion factors */
	char			*dbi_title;	/* title from IDENT rec */
	char			*dbi_filename;	/* file name */
	int			dbi_read_only;	/* !0 => read only file */
	struct mem_map		*dbi_freep;	/* map of free granules */
	char			*dbi_inmem;	/* ptr to in-memory copy */
	char			*dbi_shmaddr;	/* ptr to memory-mapped file */
	struct animate		*dbi_anroot;	/* heads list of anim at root lvl */
};
#define DBI_NULL	((struct db_i *)0)
#define DBI_MAGIC	0x57204381

#define RT_CHECK_DBI(_p) { if( (_p)->dbi_magic != DBI_MAGIC )  { \
				rt_log("RT_CHECK_DBI(x%x) magic was x%x, s/b x%x, %s line %d\n", \
					(_p), (_p)->dbi_magic, DBI_MAGIC, \
					__FILE__, __LINE__ ); \
				rt_bomb("bad db_i pointer\n"); \
			} }

/*
 *			D I R E C T O R Y
 */
struct directory  {
	char		*d_namep;		/* pointer to name string */
	long		d_addr;			/* disk address in obj file */
	struct directory *d_forw;		/* link to next dir entry */
	struct animate	*d_animate;		/* link to animation */
	long		d_uses;			/* # uses, from instancing */
	long		d_len;			/* # of db granules used */
	long		d_nref;			/* # times ref'ed by COMBs */
	short		d_flags;		/* flags */
};
#define DIR_NULL	((struct directory *)0)

#define DIR_SOLID	0x1		/* this name is a solid */
#define DIR_COMB	0x2		/* combination */
#define DIR_REGION	0x4		/* region */

/* Args to db_lookup() */
#define LOOKUP_NOISY	1
#define LOOKUP_QUIET	0

/*
 *			D B _ F U L L _ P A T H
 *
 *  For collecting paths through the database tree
 */
struct db_full_path {
	int		fp_len;
	int		fp_maxlen;
	struct directory **fp_names;	/* array of dir pointers */
};
#define DB_FULL_PATH_POP(_pp)	{(_pp)->fp_len--;}
#define DB_FULL_PATH_CUR_DIR(_pp)	((_pp)->fp_names[(_pp)->fp_len-1])

/*
 *			D B _ T R E E _ S T A T E
 *
 *  State for database tree walker db_walk_tree()
 *  and related user-provided handler routines.
 */
struct db_tree_state {
	struct db_i	*ts_dbip;
	int		ts_sofar;		/* Flag bits */

	int		ts_regionid;	/* GIFT compat region ID code*/
	int		ts_aircode;	/* GIFT compat air code */
	int		ts_gmater;	/* GIFT compat material code */
	struct mater_info ts_mater;	/* material properties */

	mat_t		ts_mat;		/* transform matrix */

	int		ts_stop_at_regions;	/* else stop at solids */
	int		(*ts_region_start_func)();
	union tree *	(*ts_region_end_func)();
	union tree *	(*ts_leaf_func)();
};
#define TS_SOFAR_MINUS	1		/* Subtraction encountered above */
#define TS_SOFAR_INTER	2		/* Intersection encountered above */
#define TS_SOFAR_REGION	4		/* Region encountered above */

/*
 *			C O M B I N E D _ T R E E _ S T A T E
 */
struct combined_tree_state {
	struct db_tree_state	cts_s;
	struct db_full_path	cts_p;
};

/*
 *			A N I M A T E
 *
 *  Each one of these structures specifies an arc in the tree that
 *  is to be operated on for animation purposes.  More than one
 *  animation operation may be applied at any given arc.  The directory
 *  structure points to a linked list of animate structures
 *  (built by rt_anim_add()), and the operations are processed in the
 *  order given.
 */
struct anim_mat {
	short		anm_op;			/* ANM_RSTACK, ANM_RARC... */
	mat_t		anm_mat;		/* Matrix */
};
#define ANM_RSTACK	1			/* Replace stacked matrix */
#define ANM_RARC	2			/* Replace arc matrix */
#define ANM_LMUL	3			/* Left (root side) mul */
#define ANM_RMUL	4			/* Right (leaf side) mul */
#define ANM_RBOTH	5			/* Replace stack, arc=Idn */

struct animate {
	struct animate	*an_forw;		/* forward link */
	struct directory **an_path;		/* pointer to path array */
	short		an_pathlen;		/* 0 = root */
	short		an_type;		/* AN_MATRIX, AN_COLOR... */
	union animate_specific {
		struct anim_mat anu_m;
	}		an_u;
};
#define AN_MATRIX	1			/* Matrix animation */
#define AN_PROPERTY	2			/* Material property anim */
#define AN_COLOR	3			/* Material color anim */
#define AN_SOLID	4			/* Solid parameter anim */

#define ANIM_NULL	((struct animate *)0)

/*
 *			R E S O U R C E
 *
 *  One of these structures is allocated per processor.
 *  To prevent excessive competition for free structures,
 *  memory is now allocated on a per-processor basis.
 *  The application structure a_resource element specifies
 *  the resource structure to be used;  if uniprocessing,
 *  a null a_resource pointer results in using the internal global
 *  structure, making initial application development simpler.
 *
 *  Applications are responsible for filling the resource structure
 *  with zeros before letting librt use them.
 *
 *  Note that if multiple models are being used, the partition and bitv
 *  structures (which are variable length) will require there to be
 *  ncpus * nmodels resource structures, the selection of which will
 *  be the responsibility of the application.
 */
struct resource {
	struct seg 	re_seg;		/* Head of segment freelist */
	long		re_seglen;
	long		re_segget;
	long		re_segfree;
	struct partition re_parthead;	/* Head of freelist */
	long		re_partlen;
	long		re_partget;
	long		re_partfree;
	union bitv_elem *re_bitv;	/* head of freelist */
	long		re_bitvlen;
	long		re_bitvget;
	long		re_bitvfree;
	union tree	**re_boolstack;	/* Stack for rt_booleval() */
	long		re_boolslen;	/* # elements in re_boolstack[] */
	int		re_cpu;		/* processor number, for ID */
	float		*re_randptr;	/* ptr into random number table */
	long		re_magic;	/* Magic number */
};
#define RESOURCE_NULL	((struct resource *)0)
#define RESOURCE_MAGIC	0x83651835
#define RT_RESOURCE_CHECK(_resp)	\
	{if((_resp)->re_magic != RESOURCE_MAGIC) {\
		rt_log("resp=x%x, magic s/b x%x was x%x, %s line %d\n", \
			(_resp), RESOURCE_MAGIC, (_resp)->re_magic, \
			__FILE__, __LINE__ ); \
		rt_bomb("bad resource struct"); } }

/*
 *			S T R U C T P A R S E
 *
 *  Definitions and data structures needed for routines that assign values
 *  to elements of arbitrary data structures, the layout of which is
 *  described by tables of "structparse" structures.
 *
 *  The general problem of word-addressed hardware
 *  where (int *) and (char *) have different representations
 *  is handled in the parsing routines that use sp_offset,
 *  because of the limitations placed on compile-time initializers.
 */
#if __STDC__
#	define offsetofarray(_t, _m)	offsetof(_t, _m)
#else
#	define offsetof(_t, _m)		(int)(&(((_t *)0)->_m))
#	define offsetofarray(_t, _m)	(int)( (((_t *)0)->_m))
#endif

struct structparse {
	char		*sp_fmt;		/* "indir" or "%f", etc */
	char		*sp_name;		/* Element's symbolic name */
	int		sp_offset;		/* Byte offset in struct */
	void		(*sp_hook)();		/* Optional hooked function, or indir ptr */
};
#define FUNC_NULL	((void (*)())0)

/*
 *			H I S T O G R A M
 */
struct histogram  {
	int		hg_min;		/* minimum value */
	int		hg_max;		/* maximum value */
	int		hg_nbins;	/* # of bins in hg_bins[] */
	int		hg_clumpsize;	/* (max-min+1)/nbins+1 */
	long		hg_nsamples;	/* total number of samples */
	long		*hg_bins;	/* array of counters */
};
#define RT_HISTOGRAM_TALLY( _hp, _val )	{ \
	if( (_val) <= (_hp)->hg_min )  { \
		(_hp)->hg_bins[0]++; \
	} else if( (_val) >= (_hp)->hg_max )  { \
		(_hp)->hg_bins[(_hp)->hg_nbins]++; \
	} else { \
		(_hp)->hg_bins[((_val)-(_hp)->hg_min)/(_hp)->hg_clumpsize]++; \
	} \
	(_hp)->hg_nsamples++;  }

/*
 *			A P P L I C A T I O N
 *
 *  This structure is the only parameter to rt_shootray().
 *
 *  When calling rt_shootray(), these fields are mandatory:
 *	a_ray.r_pt	Starting point of ray to be fired
 *	a_ray.r_dir	UNIT VECTOR with direction to fire in (dir cosines)
 *	a_hit		Routine to call when something is hit
 *	a_miss		Routine to call when ray misses everything
 *
 *  Note that rt_shootray() returns the (int) return of the a_hit()/a_miss()
 *  function called.
 */
struct application  {
	/* THESE ELEMENTS ARE MANDATORY */
	struct xray	a_ray;		/* Actual ray to be shot */
	int		(*a_hit)();	/* called when shot hits model */
	int		(*a_miss)();	/* called when shot misses */
	int		(*a_overlap)();	/* called when overlaps occur */
	int		a_level;	/* recursion level (for printing) */
	int		a_onehit;	/* flag to stop on first hit */
	struct rt_i	*a_rt_i;	/* this librt instance */
	struct resource	*a_resource;	/* dynamic memory resources */
	int		a_zero1;	/* must be zero (sanity check) */
	/* THE FOLLOWING ROUTINES ARE MAINLINE & APPLICATION SPECIFIC */
	int		a_x;		/* Screen X of ray, if applicable */
	int		a_y;		/* Screen Y of ray, if applicable */
	char		*a_purpose;	/* Debug string:  purpose of ray */
	int		a_user;		/* application-specific value */
	genptr_t	a_uptr;		/* application-specific pointer */
	fastf_t		a_rbeam;	/* initial beam radius (mm) */
	fastf_t		a_diverge;	/* slope of beam divergance/mm */
	fastf_t		a_color[9];	/* application-specific color */
	vect_t		a_uvec;		/* application-specific vector */
	vect_t		a_vvec;		/* application-specific vector */
	fastf_t		a_refrac_index;	/* current index of refraction */
	fastf_t		a_cumlen;	/* cumulative length of ray */
	int		a_zero2;	/* must be zero (sanity check) */
};
#define RT_AFN_NULL	((int (*)())0)

#define RT_AP_CHECK(_ap)	\
	{if((_ap)->a_zero1||(_ap)->a_zero2) \
		rt_bomb("corrupt application struct"); }

/*
 *			R T _ G
 *
 *  Definitions for librt.a which are global to the library
 *  regardless of how many different models are being worked on
 */
struct rt_g {
	int		debug;		/* non-zero for debug, see debug.h */
	union cutter	*rtg_CutFree;	/* cut Freelist */
	/*  Definitions necessary to interlock in a parallel environment */
	int		rtg_parallel;	/* !0 = trying to use multi CPUs */
	int		res_syscall;	/* lock on system calls */
	int		res_worker;	/* lock on work to do */
	int		res_stats;	/* lock on statistics */
	int		res_results;	/* lock on result buffer */
	int		res_model;	/* lock on model growth (splines) */
	struct vlist	*rtg_vlFree;	/* vlist freelist */
	int		rtg_logindent;	/* rt_log() indentation level */
	int		NMG_debug;	/* debug bits for NMG's see nmg.h */
};
extern struct rt_g rt_g;

/*
 *			R T _ I
 *
 *  Definitions for librt which are specific to the
 *  particular model being processed, one copy for each model.
 *  Initially, a pointer to this is returned from rt_dirbuild().
 */
struct rt_i {
	struct region	**Regions;	/* ptrs to regions [reg_bit] */
	struct rt_list	rti_headsolid;	/* list of active solids */
	struct region	*HeadRegion;	/* ptr of list of regions in model */
	struct db_i	*rti_dbip;	/* prt to Database instance struct */
	vect_t		mdl_min;	/* min corner of model bounding RPP */
	vect_t		mdl_max;	/* max corner of model bounding RPP */
	long		nregions;	/* total # of regions participating */
	long		nsolids;	/* total # of solids participating */
	long		nshots;		/* # of calls to ft_shot() */
	long		nmiss_model;	/* rays missed model RPP */
	long		nmiss_tree;	/* rays missed sub-tree RPP */
	long		nmiss_solid;	/* rays missed solid RPP */
	long		nmiss;		/* solid ft_shot() returned a miss */
	long		nhits;		/* solid ft_shot() returned a hit */
	int		needprep;	/* needs rt_prep */
	int		useair;		/* "air" regions are used */
	int		rti_nrays;	/* # calls to rt_shootray() */
	union cutter	rti_CutHead;	/* Head of cut tree */
	union cutter	rti_inf_box;	/* List of infinite solids */
	int		rti_pt_bytes;	/* length of partition struct */
	int		rti_bv_bytes;	/* length of BITV array */
	long		rti_magic;	/* magic # for integrity check */
	vect_t		rti_pmin;	/* for plotting, min RPP */
	vect_t		rti_pmax;	/* for plotting, max RPP */
	fastf_t		rti_pconv;	/* scale from rti_pmin */
	int		rti_nlights;	/* number of light sources */
	int		rti_cut_maxlen;	/* max len RPP list in 1 cut bin */
	int		rti_cut_nbins;	/* number of cut bins (leaves) */
	int		rti_cut_totobj;	/* # objs in all bins, total */
	int		rti_cut_maxdepth;/* max depth of cut tree */
	struct soltab	**rti_sol_by_type[ID_MAXIMUM+1];
	int		rti_nsol_by_type[ID_MAXIMUM+1];
	int		rti_maxsol_by_type;
	int		rti_air_discards;/* # of air regions discarded */
	struct histogram rti_hist_cellsize; /* occupancy of cut cells */
	struct histogram rti_hist_cutdepth; /* depth of cut tree */
	struct soltab	**rti_Solids;	/* ptrs to soltab [st_bit] */
	double		rti_radius;	/* radius of model bounding sphere */
};
#define RTI_NULL	((struct rt_i *)0)
#define RTI_MAGIC	0x01016580	/* magic # for integrity check */

#define RT_CHECK_RTI(_p) { if( (_p)->rti_magic != RTI_MAGIC )  { \
				rt_log("RT_CHECK_RTI(x%x) magic was x%x, s/b x%x, %s line %d\n", \
					(_p), (_p)->rti_magic, RTI_MAGIC, \
					__FILE__, __LINE__ ); \
				rt_bomb("bad rt_i pointer\n"); \
			} }

/*
 *			V L I S T
 *
 *  Definitions for handling lists of vectors (really verticies, or points)
 *  in 3-space.
 *  Intented for common handling of wireframe display information.
 *  XXX For the moment, allocated with individual malloc() calls.
 *  XXX For the moment, only hold one point per structure.
 *  It is debatable whether these should be single or double precision.
 */
struct vlist {
	point_t		vl_pnt;		/* coordinates in space */
	int		vl_draw;	/* 1=draw, 0=move */
	struct vlist	*vl_forw;	/* next structure in list */
};
#define VL_NULL		((struct vlist *)0)

struct vlhead {
	struct vlist	*vh_first;
	struct vlist	*vh_last;
};

struct vlblock {
	int			count;
	struct color_vlhead	*cvp;
};

struct color_vlhead {
	long			rgb;
	struct vlhead		head;
};

/* Values for vl_draw */
#define VL_CMD_LINE_MOVE	0
#define VL_CMD_LINE_DRAW	1
#define VL_CMD_POLY_START	2	/* vl_pnt has surface normal */
#define VL_CMD_POLY_MOVE	3	/* move to first poly vertex */
#define VL_CMD_POLY_DRAW	4	/* subsequent poly vertex */
#define VL_CMD_POLY_END		5	/* last vert (repeats 1st), draw poly */

#define GET_VL(p)	{ \
			if( ((p) = rt_g.rtg_vlFree) == VL_NULL )  { \
				(p) = (struct vlist *)rt_malloc(sizeof(struct vlist), "vlist"); \
			} else { \
				rt_g.rtg_vlFree = (p)->vl_forw; \
			} }

/* Free an entire chain of vlist structs */
#define FREE_VL(p)	{ register struct vlist *_vp = (p); \
			while( _vp->vl_forw != VL_NULL ) _vp=_vp->vl_forw; \
			_vp->vl_forw = rt_g.rtg_vlFree; \
			rt_g.rtg_vlFree = (p);  }

#define ADD_VL(hd,pnt,draw)  { \
			register struct vlist *_vp; \
			GET_VL(_vp); \
			VMOVE( _vp->vl_pnt, pnt ); \
			_vp->vl_draw = draw; \
			_vp->vl_forw = VL_NULL; \
			if( (hd)->vh_first == VL_NULL ) { \
				(hd)->vh_first = (hd)->vh_last = _vp; \
			} else { \
				(hd)->vh_last->vl_forw = _vp; \
				(hd)->vh_last = _vp; \
			} }

/*
 *  Replacements for definitions from ../h/vmath.h
 */
#undef VPRINT
#undef HPRINT
#define VPRINT(a,b)	rt_log("%s (%g, %g, %g)\n", a, (b)[0], (b)[1], (b)[2])
#define HPRINT(a,b)	rt_log("%s (%g, %g, %g, %g)\n", a, (b)[0], (b)[1], (b)[2], (b)[3])

/*
 *			C O M M A N D _ T A B
 *
 *  Table for driving generic command-parsing routines
 */
struct command_tab {
	char	*ct_cmd;
	char	*ct_parms;
	char	*ct_comment;
	int	(*ct_func)();
	int	ct_min;		/* min number of words in cmd */
	int	ct_max;		/* max number of words in cmd */
};

/*****************************************************************
 *                                                               *
 *          Applications interface to the RT library             *
 *                                                               *
 *****************************************************************/

RT_EXTERN(void rt_bomb, (char *str) );	/* Fatal error */
RT_EXTERN(void rt_log, (char *, ... ) );	/* Log message */
					/* Read named MGED db, build toc */
RT_EXTERN(struct rt_i *rt_dirbuild, (char *filename, char *buf, int len) );
					/* Prepare for raytracing */
RT_EXTERN(void rt_prep, (struct rt_i *rtip) );
					/* Shoot a ray */
RT_EXTERN(int rt_shootray, (struct application *ap) );
					/* Get expr tree for object */
RT_EXTERN(int rt_gettree, (struct rt_i *rtip, char *node) );
					/* Print seg struct */
RT_EXTERN(void rt_pr_seg, (struct seg *segp) );
					/* Print the partitions */
RT_EXTERN(void rt_pr_partitions, (struct rt_i *rtip,
	struct partition *phead, char *title) );
					/* Find solid by leaf name */
RT_EXTERN(void rt_printb, (char *s, unsigned long v, char *bits) );
					/* Print a bit vector */
RT_EXTERN(struct soltab *rt_find_solid, (struct rt_i *rtip, char *name) );
					/* Parse arbitrary data structure */
RT_EXTERN(void rt_structparse, (char *cp, struct structparse *tab, char *base ) );
					/* Print arbitrary data structure */
RT_EXTERN(void rt_structprint, (char *title, struct structparse *tab, char *base ) );
RT_EXTERN(char *rt_read_cmd, (FILE *fp) );	/* Read semi-colon terminated line */
					/* do cmd from string via cmd table */
RT_EXTERN(int rt_do_cmd, (struct rt_i *rtip, char *lp, struct command_tab *ctp) );
					/* Start the timer */
RT_EXTERN(void rt_prep_timer, (void) );
					/* Read timer, return time + str */
RT_EXTERN(double rt_read_timer, (char *str, int len) );
					/* Plot a solid */
RT_EXTERN(int rt_plot_solid, (FILE *fp, struct rt_i *rtip, struct soltab *stp) );
					/* Release storage assoc with rt_i */
RT_EXTERN(void rt_clean, (struct rt_i *rtip) );

/* The matrix math routines */
RT_EXTERN(double mat_atan2, (double y, double x) );
RT_EXTERN(void mat_zero, (mat_t m) );
RT_EXTERN(void mat_idn, (mat_t m) );
RT_EXTERN(void mat_copy, (mat_t dest, mat_t src) );
RT_EXTERN(void mat_mul, (mat_t dest, mat_t a, mat_t b) );
RT_EXTERN(void matXvec, (vect_t dest, mat_t m, vect_t src) );
RT_EXTERN(void mat_inv, (mat_t dest, mat_t src) );
/* XXX these two need mat_ on their names */
RT_EXTERN(void vtoh_move, (hvect_t dest, vect_t src) );
RT_EXTERN(void htov_move, (vect_t dest, hvect_t src) );
RT_EXTERN(void mat_print, (char *title, mat_t m) );
RT_EXTERN(void mat_trn, (mat_t dest, mat_t src) );
RT_EXTERN(void mat_ae, (mat_t dest, double azimuth, double elev) );
/* XXX new name */
RT_EXTERN(void ae_vec, (fastf_t *azp, fastf_t *elp, vect_t src) );
RT_EXTERN(void mat_angles, (mat_t dest, double alpha, double beta, double ggamma) );
/* XXX new name */
RT_EXTERN(void eigen2x2, (fastf_t *val1, fastf_t *val2,
	vect_t vec1, vect_t vec2, double a, double b, double c) );
RT_EXTERN(void mat_fromto, (mat_t dest, vect_t from, vect_t to) );
RT_EXTERN(void mat_xrot, (mat_t dest, double sinx, double cosx) );
RT_EXTERN(void mat_yrot, (mat_t dest, double siny, double cosy) );
RT_EXTERN(void mat_zrot, (mat_t dest, double sinz, double cosz) );
RT_EXTERN(void mat_lookat, (mat_t dest, vect_t dir, int yflip) );
/* XXX new names */
RT_EXTERN(void vec_ortho, (vect_t dest, vect_t src) );
RT_EXTERN(void vec_perp, (vect_t dest, vect_t src) );

/*****************************************************************
 *                                                               *
 *  Internal routines in the RT library.			 *
 *  These routines are *not* intended for Applications to use.	 *
 *  The interface to these routines may change significantly	 *
 *  from release to release of this software.			 *
 *                                                               *
 *****************************************************************/

					/* visible malloc() */
RT_EXTERN(char *rt_malloc, (unsigned int cnt, char *str) );
					/* visible free() */
RT_EXTERN(void rt_free, (char *ptr, char *str) );
					/* visible realloc() */
RT_EXTERN(char *rt_realloc, (char *ptr, unsigned int cnt, char *str) );
					/* visible calloc() */
RT_EXTERN(char *rt_calloc, (unsigned nelem, unsigned elsize, char *str) );
					/* Duplicate str w/malloc */
RT_EXTERN(char *rt_strdup, (char *cp) );

					/* Weave segs into partitions */
RT_EXTERN(void rt_boolweave, (struct seg *out_hd, struct seg *in_hd,
	struct partition *PartHeadp, struct application *ap) );
					/* Eval booleans over partitions */
RT_EXTERN(int rt_boolfinal, (struct partition *InputHdp,
	struct partition *FinalHdp,
	fastf_t startdist, fastf_t enddist,
	bitv_t *regionbits, struct application *ap) );
					/* Eval bool tree node */
RT_EXTERN(int rt_booleval, (union tree *treep, struct partition *partp,
	 struct region **trueregp, struct resource *resp) );

RT_EXTERN(void rt_grow_boolstack, (struct resource *res) );
					/* Approx Floating compare */
RT_EXTERN(int rt_fdiff, (double a, double b) );
					/* Relative Difference */
RT_EXTERN(double rt_reldiff, (double a, double b) );
					/* Print a soltab */
RT_EXTERN(void rt_pr_soltab, (struct soltab *stp) );
					/* Print a region */
RT_EXTERN(void rt_pr_region, (struct region *rp) );
					/* Print an expr tree */
RT_EXTERN(void rt_pr_tree, (union tree *tp, int lvl) );
					/* Print a partition */
RT_EXTERN(void rt_pr_pt, (struct rt_i *rtip, struct partition *pp) );
					/* Print a bit vector */
RT_EXTERN(void rt_pr_bitv, (char *str, bitv_t *bv, int len) );
					/* Print a hit point */
RT_EXTERN(void rt_pr_hit, (char *str, struct hit *hitp) );
					/* convert dbfloat->fastf_t */
/* XXX these next two should be dbfloat_t, but that means
 * XXX including db.h in absolutely everything.  No way.
 */
RT_EXTERN(void rt_fastf_float, (fastf_t *ff, float *fp, int n) );
					/* convert dbfloat mat->fastf_t */
RT_EXTERN(void rt_mat_dbmat, (fastf_t *ff, float *dbp) );
					/* storage obtainers */
RT_EXTERN(void rt_get_seg, (struct resource *res) );
RT_EXTERN(void rt_get_pt, (struct rt_i *rtip, struct resource *res) );
RT_EXTERN(void rt_get_bitv, (struct rt_i *rtip, struct resource *res) );
					/* malloc rounder */
RT_EXTERN(int rt_byte_roundup, (int nbytes) );
					/* logical OR on bit vectors */
RT_EXTERN(void rt_bitv_or, (bitv_t *out, bitv_t *in, int nbits) );
					/* space partitioning */
RT_EXTERN(void rt_cut_it, (struct rt_i *rtip) );
					/* print cut node */
RT_EXTERN(void rt_pr_cut, (union cutter *cutp, int lvl) );
					/* free a cut tree */
RT_EXTERN(void rt_fr_cut, (union cutter *cutp) );
					/* regionid-driven color override */
RT_EXTERN(void rt_region_color_map, (struct region *regp) );
					/* process ID_MATERIAL record */
RT_EXTERN(void rt_color_addrec, () );
					/* extend a cut box */
RT_EXTERN(void rt_cut_extend, (union cutter *cutp, struct soltab *stp) );
					/* find RPP of one region */
RT_EXTERN(int rt_rpp_region, (struct rt_i *rtip, char *reg_name,
	fastf_t *min_rpp, fastf_t *max_rpp) );

/* The database library */

/* db_anim.c */
RT_EXTERN(int db_add_anim, (struct db_i *dbip, struct animate *anp, int root) );
RT_EXTERN(int db_do_anim, (struct animate *anp, mat_t stack, mat_t arc,
	struct mater_info *materp) );
RT_EXTERN(void db_free_anim, (struct db_i *dbip) );

/* db_path.c */
RT_EXTERN(void db_add_node_to_full_path, (struct db_full_path *pp,
	struct directory *dp) );
RT_EXTERN(void db_dup_full_path, (struct db_full_path *newp,
	struct db_full_path *oldp) );
RT_EXTERN(char *db_path_to_string, (struct db_full_path *pp) );
RT_EXTERN(void db_free_full_path, (struct db_full_path *pp) );

/* db_open.c */
					/* open an existing model database */
RT_EXTERN(struct db_i *db_open, ( char *name, char *mode ) );
					/* create a new model database */
RT_EXTERN(struct db_i *db_create, ( char *name ) );
					/* close a model database */
RT_EXTERN(void db_close, ( struct db_i *dbip ) );
/* db_io.c */
					/* malloc & read records */
RT_EXTERN(union record *db_getmrec, ( struct db_i *, struct directory *dp ) );
					/* get several records from db */
RT_EXTERN(int db_get, (struct db_i *, struct directory *dp, union record *where,
	int offset, int len ) );
					/* put several records into db */
RT_EXTERN(int db_put, ( struct db_i *, struct directory *dp, union record *where,
	int offset, int len ) );
/* db_scan.c */
					/* read db (to build directory) */
RT_EXTERN(int db_scan, ( struct db_i *, int (*handler)() ) );
					/* update db unit conversions */
RT_EXTERN(void db_conversions, ( struct db_i *, int units ) );
/* db_lookup.c */
					/* convert name to directory ptr */
RT_EXTERN(struct directory *db_lookup,( struct db_i *, char *name, int noisy ) );
					/* add entry to directory */
RT_EXTERN(struct directory *db_diradd, ( struct db_i *, char *name, long laddr,
	int len, int flags ) );
					/* delete entry from directory */
RT_EXTERN(int db_dirdelete, ( struct db_i *, struct directory *dp ) );
/* db_alloc.c */
					/* allocate "count" granules */
RT_EXTERN(int db_alloc, ( struct db_i *, struct directory *dp, int count ) );
					/* grow by "count" granules */
RT_EXTERN(int db_grow, ( struct db_i *, struct directory *dp, int count ) );
					/* truncate by "count" */
RT_EXTERN(int db_trunc, ( struct db_i *, struct directory *dp, int count ) );
					/* delete "recnum" from entry */
RT_EXTERN(int db_delrec, ( struct db_i *, struct directory *dp, int recnum ) );
					/* delete all granules assigned dp */
RT_EXTERN(int db_delete, ( struct db_i *, struct directory *dp ) );
					/* write FREE records from 'start' */
RT_EXTERN(int db_zapper, ( struct db_i *, struct directory *dp, int start ) );

/* machine.c */
					/* change to new "nice" value */
RT_EXTERN(void rt_pri_set, ( int nval ) );
					/* get CPU time limit */
RT_EXTERN(int rt_cpuget, (void) );
					/* set CPU time limit */
RT_EXTERN(void rt_cpuset, (int sec) );
					/* find # of CPUs available */
RT_EXTERN(int rt_avail_cpus, (void) );
					/* run func in parallel */
RT_EXTERN(void rt_parallel, ( void (*func)(), int ncpu ) );

/* memalloc.c */
RT_EXTERN(unsigned long memalloc, (struct mem_map **pp, unsigned size) );
RT_EXTERN(unsigned long memget, (struct mem_map **pp, unsigned int size,
	unsigned int place) );
RT_EXTERN(void memfree, (struct mem_map **pp, unsigned size, unsigned long addr) );
RT_EXTERN(void mempurge, (struct mem_map **pp) );
RT_EXTERN(void memprint, (struct mem_map **pp) );

RT_EXTERN(struct vlblock *rt_vlblock_init, () );
RT_EXTERN(void rt_vlblock_free, (struct vlblock *vbp) );
RT_EXTERN(struct vlhead *rt_vlblock_find, (struct vlblock *vbp,
	int r, int g, int b) );

/* plane.c */
RT_EXTERN(int rt_3pts_distinct, (point_t a, point_t b, point_t c, double dist_tol_sq) );
RT_EXTERN(int rt_mk_plane_3pts, (plane_t plane, point_t a, point_t b, point_t c, double dist_tol_sq) );
RT_EXTERN(int rt_mkpoint_3planes, (point_t pt, plane_t a, plane_t b, plane_t c) );
RT_EXTERN(int rt_isect_ray_plane, (fastf_t *dist, point_t pt, vect_t dir, plane_t plane) );
RT_EXTERN(int rt_isect_2planes, (point_t pt, vect_t  dir, plane_t a, plane_t b, vect_t  rpp_min) );
RT_EXTERN(int rt_isect_2lines, (fastf_t *t, fastf_t *u, point_t p, vect_t d, point_t a, vect_t c) );
RT_EXTERN(int rt_isect_line_lseg, (fastf_t *t, point_t p, vect_t d, point_t a, point_t b) );
RT_EXTERN(double rt_dist_line_point, (point_t pt, vect_t dir, point_t a) );
RT_EXTERN(double rt_dist_line_origin, (point_t pt, vect_t dir) );
RT_EXTERN(double rt_area_of_triangle, (point_t a, point_t b, point_t c) );
RT_EXTERN(int rt_isect_pt_lseg, (fastf_t *dist, point_t a, point_t b, point_t p, fastf_t tolsq) );
RT_EXTERN(double rt_dist_pt_lseg, (point_t pca, point_t a, point_t b, point_t p) );

/* CxDiv, CxSqrt */
extern void rt_pr_roots();		/* print complex roots */

/*
 *  Constants provided and used by the RT library.
 */
extern CONST double rt_pi;
extern CONST double rt_twopi;
extern CONST double rt_halfpi;
extern CONST double rt_invpi;
extern CONST double rt_inv2pi;
extern CONST double rt_inv255;
extern CONST double rt_degtorad;
extern CONST double rt_radtodeg;

/*
 *  System library routines used by the RT library.
 */
#if __STDC__ && !apollo
/*	NOTE:  Nested includes, gets malloc(), offsetof(), etc */
#	include <stdlib.h>
#	include <stddef.h>
#else
extern char	*malloc();
extern char	*calloc();
extern char	*realloc();
/**extern void	free(); **/
#endif

#endif /* RAYTRACE_H */
