/*			N M G . H
 *
 *  Author -
 *	Lee A. Butler
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1989 by the United States Army.
 *	All rights reserved.
 *
 *  Definition of data structures for "Non-Manifold Geometry Modelling."
 *  Developed from "Non-Manifold Geometric Boundary Modeling" by 
 *  Kevin Weiler, 5/7/87 (SIGGraph 1989 Course #20 Notes)
 *
 *  Note -
 *	Any program that uses this header file must also include <stdio.h>
 */
#ifndef NMG_H
#define NMG_H seen

/* make sure all the prerequisite include files have been included
 */
#ifndef MACHINE_H
#include "machine.h"
#endif

#ifndef VMATH_H
#include "vmath.h"
#endif

#ifndef NULL
#define NULL 0
#endif

#define DEBUG_INS	0x00000001	/* 1 nmg_tbl table insert */
#define DEBUG_FINDEU	0x00000002	/* 2 findeu (find edge[use]) */
#define DEBUG_CMFACE	0x00000004	/* 3 nmg_cmface() */
#define DEBUG_COMBINE	0x00000008	/* 4 combine() */
#define DEBUG_CUTLOOP	0x00000010	/* 5 cutting loops in two */
#define DEBUG_POLYSECT	0x00000020	/* 6 combine() */
#define DEBUG_PLOTEM	0x00000040	/* 7 combine() */
#define DEBUG_BOOL	0x00000080	/* 8 combine() */
#define DEBUG_CLASSIFY	0x00000100	/* 9 combine() */
#define DEBUG_SUBTRACT	0x00000200	/* 10 combine() */
#define DEBUG_GRAZING	0x00000400	/* 11 combine() */
#define DEBUG_MESH	0x00000800	/* 12 combine() */
#define DEBUG_MESH_EU	0x00001000	/* 13 combine() */
#define DEBUG_POLYTO	0x00002000	/* 14 combine() */
#define DEBUG_LABEL_PTS 0x00004000	/* label points in plot files */

#define NMG_DEBUG_FORMAT \
"\020\017LABEL_PTS\016POLYTO\015MESH_EU\014MESH\013GRAZING\
\012SUBTRACT\011CLASSIFY\
\010BOOL\7PLOTEM\6POLYSECT\5CUTLOOP\4COMBINE\3CMFACE\2FINDEU\1TBL_INS"

/* Boolean operations */
#define NMG_BOOL_SUB 1		/* subtraction */
#define NMG_BOOL_ADD 2		/* addition/union */
#define NMG_BOOL_ISECT 4	/* intsersection */

/* orientations available.  All topological elements are orientable. */
#define OT_NONE     '\0'    /* no orientation */
#define OT_SAME     '\1'    /* orientation same */
#define OT_OPPOSITE '\2'    /* orientation opposite */
#define OT_UNSPEC   '\3'    /* orientation unspecified */
#define OT_BOOLPLACE '\4'   /* object is intermediate data for boolean ops */

/* support for pointer tables.  Our table is currently un-ordered, and is
 * merely a list of objects.  The support routine nmg_tbl manipulates the
 * list structure for you.  Objects to be referenced (inserted, deleted,
 * searched for) are passed as a "pointer to long" to the support routine.
 */
#define TBL_INIT 0	/* initialize list pointer struct & get storage */
#define TBL_INS	 1	/* insert an item (long *) into a list */
#define TBL_LOC  2	/* locate a (long *) in an existing list */
#define TBL_FREE 3	/* deallocate buffer associated with a list */
#define TBL_RST	 4	/* empty a list, but keep storage on hand */
#define TBL_CAT  5	/* catenate one list onto another */
#define TBL_RM	 6	/* remove all occurrences of an item from a list */

struct nmg_ptbl {
	int	end;	/* index into buffer of first available location */
	int	blen;	/* # of (long *)'s worth of storage at *buffer */
	long  **buffer;	/* data storage area */
};



/*
 *  Magic Numbers.
 */
#define NMG_MODEL_MAGIC 	12121212
#define NMG_MODEL_A_MAGIC	0x68652062
#define NMG_REGION_MAGIC	23232323
#define NMG_REGION_A_MAGIC	0x696e6720
#define NMG_SHELL_MAGIC 	71077345	/* shell oil */
#define NMG_SHELL_A_MAGIC	0x65207761
#define NMG_FACE_MAGIC		45454545
#define NMG_FACE_G_MAGIC	0x726b6e65
#define NMG_FACEUSE_MAGIC	56565656
#define NMG_FACEUSE_A_MAGIC	0x20476f64
#define NMG_LOOP_MAGIC		67676767
#define NMG_LOOP_G_MAGIC	0x6420224c
#define NMG_LOOPUSE_MAGIC	78787878
#define NMG_LOOPUSE_A_MAGIC	0x68657265
#define NMG_EDGE_MAGIC		33333333
#define NMG_EDGE_G_MAGIC	0x6c696768
#define NMG_EDGEUSE_MAGIC	90909090
#define NMG_EDGEUSE_A_MAGIC	0x20416e64
#define NMG_VERTEX_MAGIC	123123
#define NMG_VERTEX_G_MAGIC	727737707
#define NMG_VERTEXUSE_MAGIC	12341234
#define NMG_VERTEXUSE_A_MAGIC	0x69676874

/* macros to check/validate a structure pointer
 */
#define NMG_CKMAG(_ptr, _magic, _str)	\
	if( !(_ptr) )  { \
		rt_log("ERROR: NMG null %s ptr, file %s, line %d\n", \
			_str, __FILE__, __LINE__ ); \
		rt_bomb("NULL NMG pointer"); \
	} else if( (_ptr)->magic != (_magic) )  { \
		rt_log("ERROR: NMG bad %s ptr x%x, s/b x%x, was %s(x%x), file %s, line %d\n", \
			_str, _ptr, _magic, \
			nmg_identify_magic( (_ptr)->magic ), \
			 (_ptr)->magic, __FILE__, __LINE__ ); \
		rt_bomb("Bad NMG pointer"); \
	}

#define NMG_CK_MODEL(_p)	NMG_CKMAG(_p, NMG_MODEL_MAGIC, "model")
#define NMG_CK_MODEL_A(_p)	NMG_CKMAG(_p, NMG_MODEL_A_MAGIC, "model_a")
#define NMG_CK_REGION(_p)	NMG_CKMAG(_p, NMG_REGION_MAGIC, "region")
#define NMG_CK_REGION_A(_p)	NMG_CKMAG(_p, NMG_REGION_A_MAGIC, "region_a")
#define NMG_CK_SHELL(_p)	NMG_CKMAG(_p, NMG_SHELL_MAGIC, "shell")
#define NMG_CK_SHELL_A(_p)	NMG_CKMAG(_p, NMG_SHELL_A_MAGIC, "shell_a")
#define NMG_CK_FACE(_p)		NMG_CKMAG(_p, NMG_FACE_MAGIC, "face")
#define NMG_CK_FACE_G(_p)	NMG_CKMAG(_p, NMG_FACE_G_MAGIC, "face_g")
/*
#define NMG_CK_FACE_G(_p)	{ \
NMG_CKMAG(_p, NMG_FACE_G_MAGIC, "face_g") \
if ( (_p)->N[X] == 0.0 && (_p)->N[Y] == 0.0 && (_p)->N[Z] == 0.0 && \
(_p)->N[H] != 0.0) { \
rt_log( \
"ERROR: in file %s, line %d\nbad NMG plane equation %fX + %fY + %fZ = %f\n", \
__FILE__, __LINE__, \
(_p)->N[X], (_p)->N[Y], (_p)->N[Z], (_p)->N[H]); \
rt_bomb("Bad NMG geometry\n"); \
} }
*/
#define NMG_CK_FACEUSE(_p)	NMG_CKMAG(_p, NMG_FACEUSE_MAGIC, "faceuse")
#define NMG_CK_FACEUSE_A(_p)	NMG_CKMAG(_p, NMG_FACEUSE_A_MAGIC, "faceuse_a")
#define NMG_CK_LOOP(_p)		NMG_CKMAG(_p, NMG_LOOP_MAGIC, "loop")
#define NMG_CK_LOOP_G(_p)	NMG_CKMAG(_p, NMG_LOOP_G_MAGIC, "loop_g")
#define NMG_CK_LOOPUSE(_p)	NMG_CKMAG(_p, NMG_LOOPUSE_MAGIC, "loopuse")
#define NMG_CK_LOOPUSE_A(_p)	NMG_CKMAG(_p, NMG_LOOPUSE_A_MAGIC, "loopuse_a")
#define NMG_CK_EDGE(_p)		NMG_CKMAG(_p, NMG_EDGE_MAGIC, "edge")
#define NMG_CK_EDGE_G(_p)	NMG_CKMAG(_p, NMG_EDGE_G_MAGIC, "edge_g")
#define NMG_CK_EDGEUSE(_p)	NMG_CKMAG(_p, NMG_EDGEUSE_MAGIC, "edgeuse")
#define NMG_CK_EDGEUSE_A(_p)	NMG_CKMAG(_p, NMG_EDGEUSE_A_MAGIC, "edgeuse_a")
#define NMG_CK_VERTEX(_p)	NMG_CKMAG(_p, NMG_VERTEX_MAGIC, "vertex")
#define NMG_CK_VERTEX_G(_p)	NMG_CKMAG(_p, NMG_VERTEX_G_MAGIC, "vertex_g")
#define NMG_CK_VERTEXUSE(_p)	NMG_CKMAG(_p, NMG_VERTEXUSE_MAGIC, "vertexuse")
#define NMG_CK_VERTEXUSE_A(_p)	NMG_CKMAG(_p, NMG_VERTEXUSE_A_MAGIC, "vertexuse_a")

#define NMG_TEST_LOOPUSE(_p) \
	if (!(_p)->up.magic_p || !(_p)->next || !(_p)->last || \
	    !(_p)->l_p || !(_p)->lumate_p || !(_p)->down.magic_p) { \
		rt_log("at %d in %s BAD loopuse member pointer\n", \
			__LINE__, __FILE__); nmg_pr_lu(_p, (char *)NULL); \
			rt_bomb("Null pointer\n"); }

#define NMG_TEST_EDGEUSE(_p) \
	if (!(_p)->next || !(_p)->last || !(_p)->eumate_p || \
	    !(_p)->radial_p || !(_p)->e_p || !(_p)->vu_p || \
	    !(_p)->up.magic_p ) { \
		rt_log("in %s at %d Bad edgeuse member pointer\n",\
			 __FILE__, __LINE__);  nmg_pr_eu(_p, (char *)NULL); \
			rt_bomb("Null pointer\n"); \
	} else if ((_p)->vu_p->up.eu_p != (_p) || \
	(_p)->eumate_p->vu_p->up.eu_p != (_p)->eumate_p) {\
	    	rt_log("in %s at %d edgeuse lost vertexuse\n",\
	    		 __FILE__, __LINE__); rt_bomb("bye");}

/*	W A R N I N G !
 *
 *	We rely on the fact that the first object in a struct is the magic
 *	number (which we use to identify the struct type).  It is also
 *	important that the next/last fields have the same offset in all
 *	structs that have them.
 *
 *	To these ends, there is a standard ordering for fields in "object-use"
 *	structures.  That ordering is:
 *		1) magic number
 *		2) pointer to parent
 *		3) pointer to next
 *		4) pointer to last
 *		5) pointer to mate
 *		6) pointer to geometry
 *		7) pointer to attributes
 *		8) pointer to child(ren)
 */

/*  M O D E L
 */
struct model {
    long		magic;
    struct model_a	*ma_p;
    struct nmgregion	*r_p;		/* list of regions in model space */
};

struct model_a {
	long magic;
};

/*  R E G I O N
 */
struct nmgregion {
    long	    magic;
    struct model    *m_p;	/* owning model */
    struct nmgregion *next,
		    *last;	/* regions in model list of regions */
    struct nmgregion_a *ra_p;	/* attributes */
    struct shell    *s_p;	/* list of shells in region */
};

struct nmgregion_a {
	long	magic;
	point_t	min_pt;	/* minimums of bounding box */
	point_t	max_pt;	/* maximums of bounding box */
};

/*  S H E L L
 */
struct shell {
    long	    magic;
    struct nmgregion   *r_p;	    /* owning region */
    struct shell    *next,
		    *last;	    /* shells in region's list of shells */
    struct shell_a  *sa_p;	    /* attribs */

    struct faceuse	    *fu_p;  /* list of face uses in shell */
    struct loopuse	    *lu_p;  /* loop uses (edge groups) in shell */
    struct edgeuse	    *eu_p;  /* wire list (shell has wires) */
    struct vertexuse    *vu_p;  /* shell is single vertex */
};

struct shell_a {
    long	magic;
    point_t	min_pt;	/* minimums of bounding box */
    point_t	max_pt;	/* maximums of bounding box */
};

/*  F A C E
 */
struct face {
    long	    magic;
    struct faceuse  *fu_p;  /* list of uses of this face. use fu mate field */
    struct face_g   *fg_p;  /* geometry */
};

struct face_g {
    long	magic;
    unsigned	ref_cnt;
    plane_t	N;	/* Surface Normal (N[0]x + N[1]y + N[2]z + N[3] = 0)*/
    point_t	min_pt;	/* minimums of bounding box */
    point_t	max_pt;	/* maximums of bounding box */
};

struct faceuse { /* Note: there will always be exactly two uses of a face */
    long	    magic;
    struct shell    *s_p;	    /* owning shell */
    struct faceuse  *next,
		    *last,	    /* fu's in shell's list of fu's */
		    *fumate_p;	    /* opposite side of face */
    char	    orientation;    /* compared to face geom definition */
    struct face     *f_p;	    /* face definition and attributes */
    struct faceuse_a *fua_p;	    /* attributess */
    struct loopuse  *lu_p;	    /* list of loops in face-use */
};

struct faceuse_a {
    long    magic;
};

/*  L O O P
 */
struct loop {
    long	    magic;
    struct loopuse  *lu_p;  /* list of uses of this loop. -
					use eu_mate eulu fields */
    struct loop_g   *lg_p;  /* Geoometry */
};

struct loop_g {
    long    magic;
    point_t	min_pt;	/* minimums of bounding box */
    point_t	max_pt;	/* maximums of bounding box */
};

struct loopuse {
    long	    magic;
    union {
	struct faceuse  *fu_p;	    /* owning face-use */
	struct shell	*s_p;
	long		*magic_p;
    } up;
    struct loopuse  *next,
		    *last,	    /* lu's in fu's list of lu's */
		    *lumate_p;	    /* loopuse on other side of face */
    char	    orientation;    /* OT_SAME=outside loop */
    struct loop     *l_p;	    /* loop definition and attributes */
    struct loopuse_a *lua_p;	    /* attributes */
    union {
	struct edgeuse	    *eu_p;  /* list of eu's in lu */
	struct vertexuse    *vu_p; /* loop is a single vertex */
	long		    *magic_p; /* for those times when we're not sure */
    } down;
};

struct loopuse_a {
    long    magic;
};

/*  E D G E
 */
struct edge {
    long	    magic;
    struct edgeuse  *eu_p;  /* list of uses of this edge -
				    use eu radial/mate fields */
    struct edge_g   *eg_p;  /* geometry */
};

struct edge_g {
    long    magic;
};

struct edgeuse {
    long		magic;
    union {
	struct loopuse	*lu_p;
	struct shell	*s_p;
	long	        *magic_p; /* for those times when we're not sure */
    } up;
    struct edgeuse	*next,	    /* either clockwise/ccw edges in loop */
    			*last,	    /* or list of edges/faces in shell */
    			*eumate_p,  /* eu on other face or other end of wire*/
		  	*radial_p;  /* eu on radially adj. fu (null if wire)*/
    struct edge 	*e_p;	    /* edge definition and attributes */
    struct edgeuse_a	*eua_p;	    /* parametric space geom */
    char	  	orientation;/* compared to geom (null if wire) */
    struct vertexuse	*vu_p;	    /* starting vu of eu in this orient */
};

struct edgeuse_a {
    long    magic;
};

/*  V E R T E X
 */
struct vertex {
    long		magic;
    struct vertexuse	*vu_p;	/* list of uses of this vertex - 
					    use vu_next fields */
    struct vertex_g	*vg_p;	/* geometry */
};

struct vertex_g {
    long	magic;
    point_t	coord;	/* coordinates of vertex in space */
};

struct vertexuse {
    long		magic;
    union {
	struct shell	*s_p;	/* no fu's or eu's on shell */
	struct loopuse	*lu_p;	/* loopuse contains single vertex */
	struct edgeuse	*eu_p;	/* eu causing this vu */
	long		*magic_p; /* for those times when we're not sure */
    } up;
    struct vertexuse	*next,   /* list of all vu's of vertex */
			*last;
    struct vertex	*v_p;	    /* vertex definition and attributes */
    struct vertexuse_a	*vua_p;     /* Attributes */
};

struct vertexuse_a {
    long	magic;
    vect_t	N;	/* optional surface Normal at vertexuse */
};


/*
 * model storage allocation and de-allocation support
 */
extern char *rt_calloc();

#if __STDC__ && !alliant && !apollo
#   define NMG_GETSTRUCT(p,str) \
	p = (struct str *)rt_calloc(1,sizeof(struct str), "getstruct " #str)
#else
#   define NMG_GETSTRUCT(p,str) \
	p = (struct str *)rt_calloc(1,sizeof(struct str), "getstruct str")
#endif

#define GET_MODEL(p)	    NMG_GETSTRUCT(p, model)
#define GET_MODEL_A(p)	    NMG_GETSTRUCT(p, model_a)
#define GET_REGION(p)	    NMG_GETSTRUCT(p, nmgregion)
#define GET_REGION_A(p)     NMG_GETSTRUCT(p, nmgregion_a)
#define GET_SHELL(p)	    NMG_GETSTRUCT(p, shell)
#define GET_SHELL_A(p)	    NMG_GETSTRUCT(p, shell_a)
#define GET_FACE(p)	    NMG_GETSTRUCT(p, face)
#define GET_FACE_G(p)	    NMG_GETSTRUCT(p, face_g)
#define GET_FACEUSE(p)	    NMG_GETSTRUCT(p, faceuse)
#define GET_FACEUSE_A(p)    NMG_GETSTRUCT(p, faceuse_a)
#define GET_LOOP(p)	    NMG_GETSTRUCT(p, loop)
#define GET_LOOP_G(p)	    NMG_GETSTRUCT(p, loop_g)
#define GET_LOOPUSE(p)	    NMG_GETSTRUCT(p, loopuse)
#define GET_LOOPUSE_A(p)    NMG_GETSTRUCT(p, loopuse_a)
#define GET_EDGE(p)	    NMG_GETSTRUCT(p, edge)
#define GET_EDGE_G(p)	    NMG_GETSTRUCT(p, edge_g)
#define GET_EDGEUSE(p)	    NMG_GETSTRUCT(p, edgeuse)
#define GET_EDGEUSE_A(p)    NMG_GETSTRUCT(p, edgeuse_a)
#define GET_VERTEX(p)	    NMG_GETSTRUCT(p, vertex)
#define GET_VERTEX_G(p)     NMG_GETSTRUCT(p, vertex_g)
#define GET_VERTEXUSE(p)    NMG_GETSTRUCT(p, vertexuse)
#define GET_VERTEXUSE_A(p)  NMG_GETSTRUCT(p, vertexuse_a)

#if __STDC__ && !alliant && !apollo
# define FREESTRUCT(ptr, str) \
	{ bzero((char *)(ptr), sizeof(struct str)); \
	  rt_free((char *)(ptr), "freestruct " #str); }
#else
# define FREESTRUCT(ptr, str) \
	{ bzero((char *)(ptr), sizeof(struct str)); \
	  rt_free((char *)(ptr), "freestruct str"); }
#endif

#define FREE_MODEL(p)	    FREESTRUCT(p, model)
#define FREE_MODEL_A(p)	    FREESTRUCT(p, model_a)
#define FREE_REGION(p)	    FREESTRUCT(p, nmgregion)
#define FREE_REGION_A(p)    FREESTRUCT(p, nmgregion_a)
#define FREE_SHELL(p)	    FREESTRUCT(p, shell)
#define FREE_SHELL_A(p)	    FREESTRUCT(p, shell_a)
#define FREE_FACE(p)	    FREESTRUCT(p, face)
#define FREE_FACE_G(p)	    FREESTRUCT(p, face_g)
#define FREE_FACEUSE(p)	    FREESTRUCT(p, faceuse)
#define FREE_FACEUSE_A(p)   FREESTRUCT(p, faceuse_a)
#define FREE_LOOP(p)	    FREESTRUCT(p, loop)
#define FREE_LOOP_G(p)	    FREESTRUCT(p, loop_g)
#define FREE_LOOPUSE(p)	    FREESTRUCT(p, loopuse)
#define FREE_LOOPUSE_A(p)   FREESTRUCT(p, loopuse_a)
#define FREE_EDGE(p)	    FREESTRUCT(p, edge)
#define FREE_EDGE_G(p)	    FREESTRUCT(p, edge_g)
#define FREE_EDGEUSE(p)	    FREESTRUCT(p, edgeuse)
#define FREE_EDGEUSE_A(p)   FREESTRUCT(p, edgeuse_a)
#define FREE_VERTEX(p)	    FREESTRUCT(p, vertex)
#define FREE_VERTEX_G(p)    FREESTRUCT(p, vertex_g)
#define FREE_VERTEXUSE(p)   FREESTRUCT(p, vertexuse)
#define FREE_VERTEXUSE_A(p) FREESTRUCT(p, vertexuse_a)

#if defined(SYSV) && !defined(bzero)
#	define bzero(str,n)		memset( str, '\0', n )
#	define bcopy(from,to,count)	memcpy( to, from, count )
#endif

/* insert a node into the head of a doubly linked list */
#define DLLINS(_listp, _nodep) {\
	if (_listp) { /* link node into existing list */ \
		_nodep->next = _listp; _nodep->last = _listp->last; \
		_listp->last->next = _nodep; _listp->last = _nodep; \
	} else { /* make node the entire list */\
		_nodep->next = _nodep->last = _nodep; \
	} \
	_listp = _nodep; }

/* remove a node from a doubly linked list and put it's pointer in nodep.
 * we leave the next and last pointers of the extracted node pointing to the
 * node.  If it was the last node in the list, we don't actually remove it, 
 * but we do still copy the pointer.
 */
#define DLLRM(_listp, _nodep) if (( (_nodep)=(_listp) ) != (_listp)->next) { \
				(_listp) = (_listp)->next; \
				(_nodep)->next->last = (_nodep)->last; \
				(_nodep)->last->next = (_nodep)->next; \
				(_nodep)->next = (_nodep)->last = (_nodep); }


/* compare value to min/max and do appropriate assignments */
#define MINMAX(_a, _b, _c) { if (_a < _b) _b = _a; if (_a > _c) _c = _a; }

/* compare two extents and if they overlap, return non-zero */
#define NMG_EXTENT_OVERLAP(_l1, _h1, _l2, _h2) \
    (! ((_l1)[0] > (_h2)[0] || (_l1)[1] > (_h2)[1] || (_l1)[2] > (_h2)[2] || \
	(_l2)[0] > (_h1)[0] || (_l2)[1] > (_h1)[1] || (_l2)[2] > (_h1)[2]) )

/* two edges share same vertices */
#define EDGESADJ(_e1, _e2) (((_e1)->vu_p->v_p == (_e2)->vu_p->v_p && \
		 (_e1)->eumate_p->vu_p->v_p == (_e2)->eumate_p->vu_p->v_p) || \
		 ((_e1)->vu_p->v_p == (_e2)->eumate_p->vu_p->v_p && \
		 (_e1)->eumate_p->vu_p->v_p == (_e2)->vu_p->v_p ) )

/* Minimum distance from a point to a plane */
#define NMG_DIST_PT_PLANE(_pt, _pl) (VDOT(_pt, _pl) - (_pl)[H])
/*#define NMG_DIST_PT_PLANE(_pt, _pl) pnt_pln_dist(_pt, _pl) */
/* Believe it or not, not every system has these macros somewhere
 * in the include files
 */
#ifndef MAXVAL
# define MAXVAL(_a, _b) ((_a) > (_b) ? (_a) : (_b))
#endif
#ifndef MINVAL
# define MINVAL(_a, _b) ((_a) < (_b) ? (_a) : (_b))
#endif

/*
 *  Support Function Declarations
 *
 */
#if __STDC__
extern struct model	*nmg_mmr();
extern struct model	*nmg_mm();
extern struct shell 	*nmg_msv(struct nmgregion *r_p);
extern struct nmgregion	*nmg_mrsv(struct model *m);
extern struct vertexuse	*nmg_mvu(struct vertex *v, long *upptr);
extern struct vertexuse	*nmg_mvvu(long *upptr);
extern struct edgeuse	*nmg_me(struct vertex *v1, struct vertex *v2, struct shell *s);
extern struct edgeuse	*nmg_meonvu(struct vertexuse *vu);
extern struct edgeuse	*nmg_eins(struct edgeuse *eu);
extern struct loopuse	*nmg_ml(struct shell *s);
extern struct loopuse	*nmg_mlv(long *magic, struct vertex *v);
extern struct faceuse	*nmg_mf(struct loopuse *lu1);
extern struct faceuse	*nmg_cface(struct shell *s, struct vertex **vt,	int n);
extern struct faceuse	*nmg_cmface(struct shell *s, struct vertex **vt, int n);
extern struct edgeuse	*nmg_eusplit(struct vertex *v, struct edgeuse *oldeu);
extern struct edge	*nmg_esplit(struct vertex *v, struct edge *e);
extern char		*nmg_identify_magic(long magic);
extern int		nmg_tbl(struct nmg_ptbl *b, int func, long *p);
extern void		nmg_movevu(struct vertexuse *vu, struct vertex *v);
extern void		nmg_kfu(struct faceuse *fu1);
extern void		nmg_klu(struct loopuse *lu1);
extern void		nmg_evu(struct edgeuse *eu);
extern void		nmg_kvu(struct vertexuse *vu);
extern void		nmg_ks(struct shell *s);
extern void		nmg_kr(struct nmgregion *r);
extern void		nmg_km(struct model *m);
extern void		nmg_pr_m(struct model *m);
extern void		nmg_pr_r(struct nmgregion *r, char *h);
extern void		nmg_pr_s(struct shell *s, char *h);
extern void		nmg_pr_fg(struct face_g *fg, char *h);
extern void		nmg_pr_f(struct face *f, char *h);
extern void		nmg_pr_fu(struct faceuse *fu, char *h);
extern void		nmg_pr_l(struct loop *l, char *h);
extern void		nmg_pr_lu(struct loopuse *lu, char *h);
extern void		nmg_pr_e(struct edge *e, char *h);
extern void		nmg_pr_eu(struct edgeuse *eu, char *h);
extern void		nmg_pr_vg(struct vertex_g *vg, char *h);
extern void		nmg_pr_v(struct vertex *v, char *h);
extern void		nmg_pr_vu(struct vertexuse *vu, char *h);
extern void		nmg_unglueedge(struct edgeuse *eu);
extern void		nmg_moveeu(struct edgeuse *eudst, struct edgeuse *eusrc);
extern void 		nmg_moveltof(struct faceuse *fu, struct shell *s);
extern void		nmg_face_g(struct faceuse *fu, plane_t p);
extern void		nmg_face_bb(struct face *f);
extern void		nmg_vertex_gv(struct vertex *v, pointp_t pt);
extern void		nmg_loop_g(struct loop *l);
extern void		nmg_shell_a(struct shell *s);
extern void		nmg_jv(struct vertex *v1, struct vertex *v2);
extern void		nmg_moveltof(struct faceuse *fu, struct shell *s);
extern void		nmg_pl_fu(FILE *fp, struct faceuse *fu, 
					struct nmg_ptbl *b, unsigned char R,
					unsigned char G, unsigned char B);
extern void		nmg_pl_lu(FILE *fp, struct loopuse *fu, 
					struct nmg_ptbl *b, unsigned char R,
					unsigned char G, unsigned char B);
extern void		nmg_pl_eu(FILE *fp, struct edgeuse *eu, 
					struct nmg_ptbl *b, unsigned char R,
					unsigned char G, unsigned char B);
extern void		nmg_pl_s(FILE *fp, struct shell *s);
extern void		nmg_pl_r(FILE *fp, struct nmgregion *r);
extern void		nmg_pl_m(FILE *fp, struct model *m);
extern struct vertexuse	*nmg_find_vu_in_face(point_t pt, struct faceuse *fu, fastf_t tol);
extern void		nmg_mesh_faces(struct faceuse *fu1, struct faceuse *fu2);
extern void		nmg_isect_faces(struct faceuse *fu1, struct faceuse *fu2);
extern struct nmgregion	*nmg_do_bool(struct shell *s1, struct shell *s2, int oper, fastf_t tol);
extern int		nmg_ck_closed_surf(struct shell *s);
extern void		nmg_m_to_g(FILE *fp, struct model *m);
extern void		nmg_r_to_g(FILE *fp, struct nmgregion *r);
extern void		nmg_s_to_g(FILE *fp, struct shell *s, RGBpixel rgb);
extern struct shell	*polytonmg(FILE *fd, struct nmgregion *r, point_t min, point_t max);
extern void		nmg_pr_orient(char o, char *h);

#else
extern struct model	*nmg_mmr();
extern struct model	*nmg_mm();
extern struct shell 	*nmg_msv();
extern struct nmgregion	*nmg_mrsv();
extern struct vertexuse	*nmg_mvu();
extern struct vertexuse	*nmg_mvvu();
extern struct edgeuse	*nmg_me();
extern struct edgeuse	*nmg_meonvu();
extern struct edgeuse	*nmg_eins();
extern struct loopuse	*nmg_ml();
extern struct loopuse	*nmg_mlv();
extern struct faceuse	*nmg_mf();
extern struct faceuse	*nmg_cface();
extern struct faceuse	*nmg_cmface();
extern struct edgeuse	*nmg_eusplit();
extern struct edge	*nmg_esplit();
extern char		*nmg_identify_magic();
extern int		nmg_tbl();
extern void		nmg_movevu();
extern void		nmg_kfu();
extern void		nmg_klu();
extern void		nmg_keu();
extern void		nmg_kvu();
extern void		nmg_ks();
extern void		nmg_kr();
extern void		nmg_km();
extern void		nmg_pr_m();
extern void		nmg_pr_r();
extern void		nmg_pr_s();
extern void		nmg_pr_fg();
extern void		nmg_pr_f();
extern void		nmg_pr_fu();
extern void		nmg_pr_l();
extern void		nmg_pr_lu();
extern void		nmg_pr_e();
extern void		nmg_pr_eu();
extern void		nmg_pr_vg();
extern void		nmg_pr_v();
extern void		nmg_pr_vu();
extern void		nmg_unglueedge();
extern void		nmg_moveeu();
extern void 		nmg_moveltof();
extern void		nmg_face_g();
extern void		nmg_face_bb();
extern void		nmg_vertex_gv();
extern void		nmg_loop_g();
extern void		nmg_shell_a();
extern void		nmg_jv();
extern void		nmg_moveltof();
extern void		nmg_pl_s();
extern void		nmg_pl_r();
extern void		nmg_pl_m();
extern void		nmg_pl_fu();
extern void		nmg_pl_lu();
extern void		nmg_pl_eu();
extern struct vertexuse	*nmg_find_vu_in_face();
extern void		nmg_mesh_faces();
extern void		nmg_isect_faces();
extern struct nmgregion	*nmg_do_bool();
extern int		nmg_ck_closed_surf();
extern void		nmg_m_to_g();
extern void		nmg_r_to_g();
extern void		nmg_s_to_g();
extern struct shell	*polytonmg();
extern void		nmg_pr_orient();
extern void		nmg_pl_isect();

#endif
#define nmg_mev(_v, _u)	nmg_me((_v), (struct vertex *)NULL, (_u))

#endif
