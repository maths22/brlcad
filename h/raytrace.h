/*
 *			R A Y T R A C E . H
 *
 * All the structures necessary for dealing with the RT ray tracer library.
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

#ifdef HEP
/* full means resource free, empty means resource busy */
#define	RES_ACQUIRE(ptr)	(void)Daread(ptr)	/* wait full set empty */
#define RES_RELEASE(ptr)	(void)Daset(ptr,3)	/* set full */
#else
#define RES_ACQUIRE(ptr)	;
#define RES_RELEASE(ptr)	;
#endif

/*
 * Handy memory allocator
 */

/* Acquire storage for a given struct, eg, GETSTRUCT(ptr,structname); */
#define GETSTRUCT(p,str) \
	p = (struct str *)rt_malloc(sizeof(struct str), "getstruct str"); \
	if( p == (struct str *)0 ) \
		exit(17); \
	bzero( (char *)p, sizeof(struct str));


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
	vect_t		hit_vpriv;	/* private vector for xxx_*() */
	char		*hit_private;	/* private handle for xxx_shot() */
};
#define HIT_NULL	((struct hit *)0)



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
	struct hit	seg_in;		/* IN information */
	struct hit	seg_out;	/* OUT information */
	struct soltab	*seg_stp;	/* pointer back to soltab */
	struct seg	*seg_next;	/* non-zero if more segments */
};
#define SEG_NULL	((struct seg *)0)

#define GET_SEG(p)    {	RES_ACQUIRE(&rt_g.res_seg); \
			while( ((p)=rt_g.FreeSeg) == SEG_NULL ) \
				rt_get_seg(); \
			rt_g.FreeSeg = (p)->seg_next; \
			p->seg_next = SEG_NULL; \
			RES_RELEASE(&rt_g.res_seg); }
#define FREE_SEG(p)   {	RES_ACQUIRE(&rt_g.res_seg); \
			(p)->seg_next = rt_g.FreeSeg; rt_g.FreeSeg = (p); \
			RES_RELEASE(&rt_g.res_seg); }


/*
 *			S O L T A B
 *
 * Internal information used to keep track of solids in the model
 * Leaf name and Xform matrix are unique identifier.
 */
struct soltab {
	int		st_id;		/* Solid ident */
	vect_t		st_center;	/* Centroid of solid */
	fastf_t		st_aradius;	/* Radius of APPROXIMATING sphere */
	fastf_t		st_bradius;	/* Radius of BOUNDING sphere */
	int		*st_specific;	/* -> ID-specific (private) struct */
	struct soltab	*st_forw;	/* Linked list of solids */
	char		*st_name;	/* Leaf name of solid */
	vect_t		st_min;		/* min X, Y, Z of bounding RPP */
	vect_t		st_max;		/* max X, Y, Z of bounding RPP */
	int		st_bit;		/* solids bit vector index (const) */
	int		st_maxreg;	/* highest bit set in st_regions */
	bitv_t		*st_regions;	/* bit vect of region #'s (const) */
	mat_t		st_pathmat;	/* Xform matrix on path */
};
#define SOLTAB_NULL	((struct soltab *)0)

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

struct rt_functab {
	int		(*ft_prep)();
	struct seg 	*((*ft_shot)());
	int		(*ft_print)();
	int		(*ft_norm)();
	int		(*ft_uv)();
	char		*ft_name;
};
extern struct rt_functab rt_functab[];

#define EPSILON		0.0001
#define NEAR_ZERO(f)	( ((f) > -EPSILON) && ((f) < EPSILON) )
#define INFINITY	(1.0e20)


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
/* Internal */
#define OP_NOT		MKOP(6)		/* Unary:  not L */
#define OP_GUARD	MKOP(7)		/* Unary:  not L, or else! */
#define OP_XNOP		MKOP(8)		/* Unary:  L, mark region */

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
 *			R E G I O N
 *
 *  The region structure.
 */
struct region  {
	char		*reg_name;	/* Identifying string */
	union tree	*reg_treetop;	/* Pointer to boolean tree */
	short		reg_bit;	/* constant index into Regions[] */
	short		reg_regionid;	/* Region ID code;  index to ? */
	short		reg_aircode;	/* ?? */
	short		reg_material;	/* Material */
	short		reg_los;	/* equivalent LOS estimate ?? */
	struct region	*reg_forw;	/* linked list of all regions */
	char		*reg_materp;	/* material structure */
};
#define REGION_NULL	((struct region *)0)


/*
 *  			P A R T I T I O N
 *
 *  Partitions of a ray
 *
 *  NOTE:  rt_get_pt allows enough storage at the end of the partition
 *  for a bit vector of "rt_i.nsolids" bits in length.
 */
struct partition {
	struct seg	*pt_inseg;		/* IN seg ptr (gives stp) */
	struct hit	*pt_inhit;		/* IN hit pointer */
	struct seg	*pt_outseg;		/* OUT seg pointer */
	struct hit	*pt_outhit;		/* OUT hit ptr */
	struct region	*pt_regionp;		/* ptr to containing region */
	struct partition *pt_forw;		/* forwards link */
	struct partition *pt_back;		/* backwards link */
	char		pt_inflip;		/* flip inhit->hit_normal */
	char		pt_outflip;		/* flip outhit->hit_normal */
	long		pt_solhit[2];		/* VAR bit array:solids hit */
};
#define PT_NULL	((struct partition *)0)

#define PT_BYTES	(sizeof(struct partition) + \
			 BITS2BYTES(rt_i.nsolids) + sizeof(bitv_t))

#define COPY_PT(out,in)	bcopy((char *)in, (char *)out, PT_BYTES)

/* Initialize all the bits to FALSE, clear out structure */
#define GET_PT_INIT(p)	\
	{ GET_PT(p); bzero( ((char *) (p)), PT_BYTES ); }

#define GET_PT(p)   { RES_ACQUIRE(&rt_g.res_pt); \
			while( ((p)=rt_g.FreePart) == PT_NULL ) \
				rt_get_pt(); \
			rt_g.FreePart = (p)->pt_forw; \
			RES_RELEASE(&rt_g.res_pt); }
#define FREE_PT(p) { RES_ACQUIRE(&rt_g.res_pt); \
			(p)->pt_forw = rt_g.FreePart; rt_g.FreePart = (p); \
			RES_RELEASE(&rt_g.res_pt); }

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

#define GET_BITV(p)    {	RES_ACQUIRE(&rt_g.res_bitv); \
			while( ((p)=rt_i.FreeBitv) == BITV_NULL ) \
				rt_get_bitv(); \
			rt_i.FreeBitv = (p)->be_next; \
			p->be_next = BITV_NULL; \
			RES_RELEASE(&rt_g.res_bitv); }
#define FREE_BITV(p)   {	RES_ACQUIRE(&rt_g.res_bitv); \
			(p)->be_next = rt_i.FreeBitv; rt_i.FreeBitv = (p); \
			RES_RELEASE(&rt_g.res_bitv); }

/*
 *  Bit-string manipulators for arbitrarily long bit strings
 *  stored as an array of bitv_t's.
 *  BITV_SHIFT and BITV_MASK are defined in machine.h
 */
#define BITS2BYTES(nbits) (((nbits)+BITV_MASK)/8)	/* conservative */
#define BITTEST(lp,bit)	(lp[bit>>BITV_SHIFT] & (1<<(bit&BITV_MASK)))
#define BITSET(lp,bit)	(lp[bit>>BITV_SHIFT] |= (1<<(bit&BITV_MASK)))
#define BITCLR(lp,bit)	(lp[bit>>BITV_SHIFT] &= ~(1<<(bit&BITV_MASK)))
#define BITZERO(lp,nbits) bzero((char *)lp, BITS2BYTES(nbits))

/*
 *		A P P L I C A T I O N
 *
 * Note:  When calling rt_shootray(), these fields are mandatory:
 *	a_ray.r_pt	Starting point of ray to be fired
 *	a_ray.r_dir	UNIT VECTOR with direction to fire in (dir cosines)
 *	a_hit		Routine to call when something is hit
 *	a_miss		Routine to call when ray misses everything
 *
 * Also note that rt_shootray() returns the (int) return of a_hit()/a_miss().
 */
struct application  {
	/* THESE ELEMENTS ARE MANDATORY */
	struct xray a_ray;	/* Actual ray to be shot */
	int	(*a_hit)();	/* routine to call when shot hits model */
	int	(*a_miss)();	/* routine to call when shot misses */
	int	a_level;	/* recursion level (for printing) */
	int	a_onehit;	/* flag to stop on first hit */
	/* THE FOLLOWING ROUTINES ARE MAINLINE & APPLICATION SPECIFIC */
	int	a_x;		/* Screen X of ray, where applicable */
	int	a_y;		/* Screen Y of ray, where applicable */
	int	a_user;		/* application-specific value */
	point_t	a_color;	/* application-specific color */
	vect_t	a_uvec;		/* application-specific vector */
};

/*
 *  Definitions for librt.a which are global to the library
 *  regardless of how many different models are being worked on
 */
struct rt_g {
	int		debug;		/* non-zero for debug, see debug.h */
	struct seg 	*FreeSeg;	/* Head of segment freelist */
	struct partition *FreePart;	/* Head of freelist */
	/*  Definitions necessary to interlock in a parallel environment */
	int		res_pt;		/* lock on free partition structs */
	int		res_seg;	/* lock on free seg structs */
	int		res_malloc;	/* lock on memory allocation */
	int		res_printf;	/* lock on printing */
	int		res_bitv;	/* lock on bitvectors */
};
extern struct rt_g rt_g;

/*
 *  Definitions for librt.a which are specific to the
 *  particular model being processed;  ultimately,
 *  there could be several instances of this structure.
 */
struct rt_i {
	struct region	**Regions;	/* ptrs to regions [reg_bit] */
	struct soltab	*HeadSolid;	/* ptr to list of solids in model */
	struct region	*HeadRegion;	/* ptr of list of regions in model */
	union tree	*RootTree;	/* ptr to total tree (non-region) */
	long		nregions;	/* total # of regions participating */
	int		fd;		/* fd of database */
	long		nsolids;	/* total # of solids participating */
	long		nshots;		/* # of calls to ft_shot() */
	long		nmiss_model;	/* rays missed model RPP */
	long		nmiss_tree;	/* rays missed sub-tree RPP */
	long		nmiss_solid;	/* rays missed solid RPP */
	long		nmiss;		/* solid ft_shot() returned a miss */
	long		nhits;		/* solid ft_shot() returned a hit */
	vect_t		mdl_min;	/* min corner of model bounding RPP */
	vect_t		mdl_max;	/* max corner of model bounding RPP */
	union bitv_elem *FreeBitv;	/* head of freelist */
	int		needprep;	/* needs rt_prep */
	char		*file;		/* name of file */
};
extern struct rt_i rt_i;	/* Eventually, will be a return value */

/*
 *  Global routines to interface with the RT library.
 */
extern void rt_bomb();			/* Fatal error */
extern void rt_log();			/* Log message */

extern int rt_gettree();		/* Get expr tree for object */
extern void rt_prep();			/* Prepare for raytracing */
extern int rt_shootray();		/* Shoot a ray */
extern void rt_prep_timer();		/* Start the timer */
extern double rt_read_timer();		/* Read timer, return time + str */
extern int rt_dirbuild();		/* Read named GED db, build toc */
extern void rt_pr_seg();			/* Print seg struct */
extern void rt_pr_partitions();		/* Print the partitions */
extern void rt_viewbounds();		/* Find bounding view-space RPP */
extern struct soltab *rt_find_solid();	/* Find solid by leaf name */

/* RT Storage allocators */
extern char *rt_malloc();			/* visible malloc() */
extern void rt_free();			/* visible free() */
extern char *rt_strdup();			/* Duplicate str w/malloc */

/* The matrix math routines */
extern void mat_zero(), mat_idn(), mat_copy(), mat_mul(), matXvec();
extern void mat_inv(), mat_trn(), mat_ae(), mat_angles();
extern void vtoh_move(), htov_move(), mat_print();

/*
 *  Internal routines in RT library.
 *  Not intended for general use.
 */
extern struct directory *rt_dir_lookup();/* Look up name in toc */
extern struct directory *rt_dir_add();	/* Add name to toc */
extern void rt_bool_weave();		/* Weave segs into partitions */
extern void rt_bool_final();		/* Eval booleans over partitions */
extern int rt_bool_eval();		/* Eval bool tree node */
extern int rt_fdiff();			/* Approx Floating compare */
extern double rt_reldiff();		/* Relative Difference */
extern void rt_pr_region();		/* Print a region */
extern void rt_pr_tree();		/* Print an expr tree */
extern void rt_pr_pt();			/* Print a partition */
extern void rt_pr_bitv();		/* Print a bit vector */
extern void rt_pr_hit();		/* Print a hit point */
extern void rt_fastf_float();		/* convert float->fastf_t */
extern void rt_get_seg();		/* storage obtainer */
extern void rt_get_pt();
extern void rt_get_bitv();
extern int rt_byte_roundup();		/* malloc rounder */
extern void rt_bitv_or();		/* logical OR on bit vectors */
extern void rt_cut_it();		/* space partitioning */
extern void rt_pr_cut();		/* print cut node */
extern void rt_draw_box();		/* unix-plot an RPP */

/* CxDiv, CxSqrt */
extern void rt_pr_roots();		/* print complex roots */

/*
 *  Library routines used by the RT library.
 */
extern long	lseek();
extern int	read(), write();
extern char	*malloc();
extern void	free();
