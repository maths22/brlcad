/*
 *			N M G . H
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
 *	Any program that uses this header file must also include
 *	these other header files, in this order:
 *		<stdio.h>
 *		<math.h>
 *		"machine.h"
 *		"vmath.h"
 *		"nmg.h"		(This file)
 *		"raytrace.h"
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

#ifndef SEEN_RTLIST_H
#include "rtlist.h"
#endif

#ifndef NULL
#define NULL 0
#endif

#define	NMG_EXTERN(type_and_name,args)	RT_EXTERN(type_and_name,args)


#define DEBUG_PL_ANIM	0x00000001	/* 1 mged animated evaluation */
#define DEBUG_PL_SLOW	0x00000002	/* 2 add delays to animation */

#define DEBUG_COMBINE	0x00000008	/* 4 combine() */
#define DEBUG_CUTLOOP	0x00000010	/* 5 cutting loops in two */
#define DEBUG_POLYSECT	0x00000020	/* 6 combine() */
#define DEBUG_PLOTEM	0x00000040	/* 7 combine() */
#define DEBUG_BOOL	0x00000080	/* 8 combine() */
#define DEBUG_CLASSIFY	0x00000100	/* 9 combine() */
#define DEBUG_BOOLEVAL	0x00000200	/* 10 boolean evaluation steps */
#define DEBUG_GRAZING	0x00000400	/* 11 combine() */
#define DEBUG_MESH	0x00000800	/* 12 combine() */
#define DEBUG_MESH_EU	0x00001000	/* 13 combine() */
#define DEBUG_POLYTO	0x00002000	/* 14 combine() */
#define DEBUG_LABEL_PTS 0x00004000	/* 15 label points in plot files */
#define DEBUG_INS	0x00008000	/* 16 nmg_tbl table insert */
#define DEBUG_NMGRT     0x00010000	/* 17 ray tracing */

#define DEBUG_FINDEU	0x00020000	/* 18 findeu (find edge[use]) */
#define DEBUG_CMFACE	0x00040000	/* 19 nmg_cmface() */
#define DEBUG_GRAPHCL	0x00080000	/* 20 graphic classification */
#define NMG_DEBUG_FORMAT \
"\020\024GRAPHCL\023CMFACE\022FINDEU\021NMGRT\020TBL_INS\
\017LABEL_PTS\016POLYTO\015MESH_EU\014MESH\
\013GRAZING\
\012BOOLEVAL\011CLASSIFY\
\010BOOL\7PLOTEM\6POLYSECT\5CUTLOOP\4COMBINE\3XXX\2PL_SLOW\1PL_ANIM"

/* Boolean operations */
#define NMG_BOOL_SUB 1		/* subtraction */
#define NMG_BOOL_ADD 2		/* addition/union */
#define NMG_BOOL_ISECT 4	/* intsersection */

/* Boolean classifications */
#define NMG_CLASS_AinB		0
#define NMG_CLASS_AonBshared	1
#define NMG_CLASS_AonBanti	2
#define NMG_CLASS_AoutB		3
#define NMG_CLASS_BinA		4
#define NMG_CLASS_BonAshared	5
#define NMG_CLASS_BonAanti	6
#define NMG_CLASS_BoutA		7

/* orientations available.  All topological elements are orientable. */
#define OT_NONE     0    /* no orientation */
#define OT_SAME     1    /* orientation same */
#define OT_OPPOSITE 2    /* orientation opposite */
#define OT_UNSPEC   3    /* orientation unspecified */
#define OT_BOOLPLACE 4   /* object is intermediate data for boolean ops */



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
#define TBL_INS_UNIQUE	 7	/* insert item into list, if not present */

struct nmg_ptbl {
	int	end;	/* index into buffer of first available location */
	int	blen;	/* # of (long *)'s worth of storage at *buffer */
	long  **buffer;	/* data storage area */
};

/* For those routines that have to "peek" a little */
#define NMG_TBL_BASEADDR(p)	((p)->buffer)
#define NMG_TBL_END(p)		((p)->end)
#define NMG_TBL_GET(p,i)	((p)->buffer[(i)])

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
	if( !(_ptr) || *((long *)(_ptr)) != (_magic) )  { \
		rt_badmagic( (long *)(_ptr), _magic, _str, __FILE__, __LINE__ ); \
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
#define NMG_CK_LIST(_p)		NMG_CKMAG(_p, RT_LIST_HEAD_MAGIC, "rt_list")

#define NMG_TEST_LOOPUSE(_p) \
	if (!(_p)->up.magic_p || !(_p)->l.forw || !(_p)->l.back || \
	    !(_p)->l_p || !(_p)->lumate_p || !(_p)->down.magic_p) { \
		rt_log("at %d in %s BAD loopuse member pointer\n", \
			__LINE__, __FILE__); nmg_pr_lu(_p, (char *)NULL); \
			rt_bomb("Null pointer\n"); }

#define NMG_TEST_EDGEUSE(_p) \
	if (!(_p)->l.forw || !(_p)->l.back || !(_p)->eumate_p || \
	    !(_p)->radial_p || !(_p)->e_p || !(_p)->vu_p || \
	    !(_p)->up.magic_p ) { \
		rt_log("in %s at %d Bad edgeuse member pointer\n",\
			 __FILE__, __LINE__);  nmg_pr_eu(_p, (char *)NULL); \
			rt_bomb("Null pointer\n"); \
	} else if ((_p)->vu_p->up.eu_p != (_p) || \
	(_p)->eumate_p->vu_p->up.eu_p != (_p)->eumate_p) {\
	    	rt_log("in %s at %d edgeuse lost vertexuse\n",\
	    		 __FILE__, __LINE__); rt_bomb("bye");}

/*
 *	N O T I C E !
 *
 *	We rely on the fact that the first long in a struct is the magic
 *	number (which is used to identify the struct type).
 *	This may be either a long, or an rt_list structure, which
 *	starts with a magic number.
 *
 *	To these ends, there is a standard ordering for fields in "object-use"
 *	structures.  That ordering is:
 *		1) magic number, or rt_list structure
 *		2) pointer to parent
 *		5) pointer to mate
 *		6) pointer to geometry
 *		7) pointer to attributes
 *		8) pointer to child(ren)
 */


/*
 *			M O D E L
 */
#if !defined(MODEL_DEFINED)
#define MODEL_DEFINED
struct model {
	long			magic;
	struct model_a		*ma_p;
	struct rt_list		r_hd;	/* list of regions */
	long			index;	/* struct # in this model */
	char			*manifolds; /*  structure 1-3manifold table */
	long			maxindex; /* # of structs so far */
};
#endif /* !MODEL_DEFINED */

struct model_a {
	long			magic;
	long			index;	/* struct # in this model */
};

/*
 *			R E G I O N
 */
#if !defined(NMGREGION_DEFINED)
#define NMGREGION_DEFINED
struct nmgregion {
	struct rt_list		l;	/* regions, in model's r_hd list */
	struct model   		*m_p;	/* owning model */
	struct nmgregion_a	*ra_p;	/* attributes */
	struct rt_list		s_hd;	/* list of shells in region */
	long			index;	/* struct # in this model */
};
#endif /* !NMGREGION_DEFINED */

struct nmgregion_a {
	long			magic;
	point_t			min_pt;	/* minimums of bounding box */
	point_t			max_pt;	/* maximums of bounding box */
	long			index;	/* struct # in this model */
};

/*
 *			S H E L L
 */
struct shell {
	struct rt_list		l;	/* shells, in region's s_hd list */
	struct nmgregion	*r_p;	/* owning region */
	struct shell_a		*sa_p;	/* attribs */

	struct rt_list		fu_hd;	/* list of face uses in shell */
	struct rt_list		lu_hd;	/* loop uses (edge groups) in shell */
	struct rt_list		eu_hd;	/* wire list (shell has wires) */
	struct vertexuse	*vu_p;	/* internal ptr to single vertexuse */
	long			index;	/* struct # in this model */
};

struct shell_a {
	long			magic;
	point_t			min_pt;	/* minimums of bounding box */
	point_t			max_pt;	/* maximums of bounding box */
	long			index;	/* struct # in this model */
};

/*
 *			F A C E
 *
 *  Note: there will always be exactly two faceuse's using a face.
 *  To find them, go up fu_p for one, then across fumate_p to other.
 */
struct face {
	long	   		magic;
	struct faceuse		*fu_p;	/* Ptr up to one use of this face */
	struct face_g		*fg_p;	/* geometry */
	long			index;	/* struct # in this model */
};

struct face_g {
	long			magic;
	plane_t			N;	/* Plane equation (incl normal) */
	point_t			min_pt;	/* minimums of bounding box */
	point_t			max_pt;	/* maximums of bounding box */
	long			index;	/* struct # in this model */
};

struct faceuse {
	struct rt_list		l;	/* fu's, in shell's fu_hd list */
	struct shell		*s_p;	/* owning shell */
	struct faceuse		*fumate_p;    /* opposite side of face */
	int			orientation;  /* rel to face geom defn */
	struct face		*f_p;	/* face definition and attributes */
	struct faceuse_a	*fua_p;	/* attributess */
	struct rt_list		lu_hd;	/* list of loops in face-use */
	long			index;	/* struct # in this model */
};

struct faceuse_a {
	long			magic;
	long			index;	/* struct # in this model */
};

/*
 *			L O O P
 *
 *  To find all the uses of this loop, use lu_p for one loopuse,
 *  then go down and find an edge,
 *  then wander around either eumate_p or radial_p from there.
 *
 *  Normally, down_hd heads a doubly linked list of edgeuses.
 *  But, before using it, check RT_LIST_FIRST_MAGIC(&lu->down_hd)
 *  for the magic number type.
 *  If this is a self-loop on a single vertexuse, then get the vertex pointer
 *  with vu = RT_LIST_FIRST(vertexuse, &lu->down_hd)
 *
 *  This is an especially dangerous storage efficiency measure ("hack"),
 *  because the list that the vertexuse structure belongs to is headed,
 *  not by a superior element type, but by the vertex structure.
 *  When a loopuse needs to point down to a vertexuse, rip off the
 *  forw pointer.  Take careful note that this is just a pointer,
 *  **not** the head of a linked list (single, double, or otherwise)!
 *  Exercise great care!
 *
 *  The edges of an exterior (OT_SAME) loop occur in counter-clockwise
 *  order, as viewed from the normalward side (outside).
 */
#define RT_LIST_SET_DOWN_TO_VERT(_hp,_vu)	{ \
	(_hp)->forw = &((_vu)->l); (_hp)->back = (struct rt_list *)NULL; }

struct loop {
	long			magic;
	struct loopuse		*lu_p;	/* Ptr to one use of this loop */
	struct loop_g		*lg_p;  /* Geometry */
	long			index;	/* struct # in this model */
};

struct loop_g {
	long			magic;
	point_t			min_pt;	/* minimums of bounding box */
	point_t			max_pt;	/* maximums of bounding box */
	long			index;	/* struct # in this model */
};

struct loopuse {
	struct rt_list		l;	/* lu's, in fu's lu_hd, or shell's lu_hd */
	union {
		struct faceuse  *fu_p;	/* owning face-use */
		struct shell	*s_p;
		long		*magic_p;
	} up;
	struct loopuse		*lumate_p; /* loopuse on other side of face */
	int			orientation;  /* OT_SAME=outside loop */
	struct loop		*l_p;	/* loop definition and attributes */
	struct loopuse_a	*lua_p;	/* attributes */
	struct rt_list		down_hd; /* eu list or vu pointer */
	long			index;	/* struct # in this model */
};

struct loopuse_a {
	long			magic;
	long			index;	/* struct # in this model */
};

/*
 *			E D G E
 *
 *  To find all the uses of this edge, use eu_p for one edge,
 *  then wander around either eumate_p or radial_p from there.
 */
struct edge {
	long			magic;
	struct edgeuse		*eu_p;	/* Ptr to one use of this edge */
	struct edge_g		*eg_p;  /* geometry */
	long			is_real;/* artifact or modeled edge */
	long			index;	/* struct # in this model */
};

struct edge_g {
	long			magic;
	long			usage;	/* # of uses of this geometry */
	point_t			e_pt;	/* parametric equation of the edge */
	vect_t			e_dir;
	long			index;	/* struct # in this model */
};

struct edgeuse {
	struct rt_list		l;	/* cw/ccw edges in loop or wire edges in shell */
	union {
		struct loopuse	*lu_p;
		struct shell	*s_p;
		long	        *magic_p; /* for those times when we're not sure */
	} up;
	struct edgeuse		*eumate_p;  /* eu on other face or other end of wire*/
	struct edgeuse		*radial_p;  /* eu on radially adj. fu (null if wire)*/
	struct edge		*e_p;	    /* edge definition and attributes */
	struct edgeuse_a	*eua_p;	    /* parametric space geom */
	int	  		orientation;/* compared to geom (null if wire) */
	struct vertexuse	*vu_p;	    /* first vu of eu in this orient */
	long			index;	/* struct # in this model */
};

struct edgeuse_a {
	long			magic;
	long			index;	/* struct # in this model */
};

/*
 *			V E R T E X
 *
 *  The vertex and vertexuse structures are connected in a way different
 *  from the superior kinds of topology elements.
 *  The vertex structure heads a linked list that all vertexuse's
 *  that use the vertex are linked onto.
 */
struct vertex {
	long			magic;
	struct rt_list		vu_hd;	/* heads list of vu's of this vertex */
	struct vertex_g		*vg_p;	/* geometry */
	long			index;	/* struct # in this model */
};

struct vertex_g {
	long			magic;
	point_t			coord;	/* coordinates of vertex in space */
	long			index;	/* struct # in this model */
};

struct vertexuse {
	struct rt_list		l;	/* list of all vu's on a vertex */
	union {
		struct shell	*s_p;	/* no fu's or eu's on shell */
		struct loopuse	*lu_p;	/* loopuse contains single vertex */
		struct edgeuse	*eu_p;	/* eu causing this vu */
		long		*magic_p; /* for those times when we're not sure */
	} up;
	struct vertex		*v_p;	/* vertex definition and attributes */
	struct vertexuse_a	*vua_p;	/* Attributes */
	long			index;	/* struct # in this model */
};

struct vertexuse_a {
	long			magic;
	vect_t			N;	/* (opt) surface Normal at vertexuse */
	long			index;	/* struct # in this model */
};


/*
 * storage allocation and de-allocation support
 */

#if __STDC__ && !alliant && !apollo
#   define NMG_GETSTRUCT(p,str) \
	p = (struct str *)rt_calloc(1,sizeof(struct str), "getstruct " #str)
#else
#   define NMG_GETSTRUCT(p,str) \
	p = (struct str *)rt_calloc(1,sizeof(struct str), "getstruct str")
#endif

#define NMG_INCR_INDEX(_p,_m)	\
	NMG_CK_MODEL(_m); (_p)->index = ((_m)->maxindex)++

#define GET_MODEL_A(p,m)    {NMG_GETSTRUCT(p, model_a); NMG_INCR_INDEX(p,m);}
#define GET_REGION(p,m)	    {NMG_GETSTRUCT(p, nmgregion); NMG_INCR_INDEX(p,m);}
#define GET_REGION_A(p,m)   {NMG_GETSTRUCT(p, nmgregion_a); NMG_INCR_INDEX(p,m);}
#define GET_SHELL(p,m)	    {NMG_GETSTRUCT(p, shell); NMG_INCR_INDEX(p,m);}
#define GET_SHELL_A(p,m)    {NMG_GETSTRUCT(p, shell_a); NMG_INCR_INDEX(p,m);}
#define GET_FACE(p,m)	    {NMG_GETSTRUCT(p, face); NMG_INCR_INDEX(p,m);}
#define GET_FACE_G(p,m)	    {NMG_GETSTRUCT(p, face_g); NMG_INCR_INDEX(p,m);}
#define GET_FACEUSE(p,m)    {NMG_GETSTRUCT(p, faceuse); NMG_INCR_INDEX(p,m);}
#define GET_FACEUSE_A(p,m)  {NMG_GETSTRUCT(p, faceuse_a); NMG_INCR_INDEX(p,m);}
#define GET_LOOP(p,m)	    {NMG_GETSTRUCT(p, loop); NMG_INCR_INDEX(p,m);}
#define GET_LOOP_G(p,m)	    {NMG_GETSTRUCT(p, loop_g); NMG_INCR_INDEX(p,m);}
#define GET_LOOPUSE(p,m)    {NMG_GETSTRUCT(p, loopuse); NMG_INCR_INDEX(p,m);}
#define GET_LOOPUSE_A(p,m)  {NMG_GETSTRUCT(p, loopuse_a); NMG_INCR_INDEX(p,m);}
#define GET_EDGE(p,m)	    {NMG_GETSTRUCT(p, edge); NMG_INCR_INDEX(p,m);}
#define GET_EDGE_G(p,m)	    {NMG_GETSTRUCT(p, edge_g); (p)->usage = 1; NMG_INCR_INDEX(p,m);}
#define GET_EDGEUSE(p,m)    {NMG_GETSTRUCT(p, edgeuse); NMG_INCR_INDEX(p,m);}
#define GET_EDGEUSE_A(p,m)  {NMG_GETSTRUCT(p, edgeuse_a); NMG_INCR_INDEX(p,m);}
#define GET_VERTEX(p,m)	    {NMG_GETSTRUCT(p, vertex); NMG_INCR_INDEX(p,m);}
#define GET_VERTEX_G(p,m)   {NMG_GETSTRUCT(p, vertex_g); NMG_INCR_INDEX(p,m);}
#define GET_VERTEXUSE(p,m)  {NMG_GETSTRUCT(p, vertexuse); NMG_INCR_INDEX(p,m);}
#define GET_VERTEXUSE_A(p,m) {NMG_GETSTRUCT(p, vertexuse_a); NMG_INCR_INDEX(p,m);}

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
#define FREE_EDGE_G(p)	    if (--((p)->usage) <= 0)  FREESTRUCT(p, edge_g)
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

#ifndef MAXVAL
# define MAXVAL(_a, _b) ((_a) > (_b) ? (_a) : (_b))
#endif
#ifndef MINVAL
# define MINVAL(_a, _b) ((_a) < (_b) ? (_a) : (_b))
#endif

#define EUPRINT(_s, _eu)	nmg_euprint( (_s), (_eu) )
#define PLPRINT(_s, _pl) rt_log("%s %gx + %gy + %gz = %g\n", (_s), \
	(_pl)[0], (_pl)[1], (_pl)[2], (_pl)[3])


struct nmg_boolstruct {
	struct nmg_ptbl	ilist;		/* vertexuses on intersection line */
	fastf_t		tol;
	point_t		pt;		/* line of intersection */
	vect_t		dir;
	int		coplanar;
	char		*vertlist;
	int		vlsize;
	struct model	*model;
};

#define PREEXIST 1
#define NEWEXIST 2


#define VU_PREEXISTS(_bs, _vu) { chkidxlist((_bs), (_vu)); \
	(_bs)->vertlist[(_vu)->index] = PREEXIST; }

#define VU_NEW(_bs, _vu) { chkidxlist((_bs), (_vu)); \
	(_bs)->vertlist[(_vu)->index] = NEWEXIST; }


struct nmg_struct_counts {
	/* Actual structure counts (Xuse, then X) */
	long	model;
	long	model_a;
	long	region;
	long	region_a;
	long	shell;
	long	shell_a;
	long	faceuse;
	long	faceuse_a;
	long	face;
	long	face_g;
	long	loopuse;
	long	loopuse_a;
	long	loop;
	long	loop_g;
	long	edgeuse;
	long	edgeuse_a;
	long	edge;
	long	edge_g;
	long	vertexuse;
	long	vertexuse_a;
	long	vertex;
	long	vertex_g;
	/* Abstractions */
	long	max_structs;
	long	face_loops;
	long	face_edges;
	long	face_lone_verts;
	long	wire_loops;
	long	wire_loop_edges;
	long	wire_edges;
	long	wire_lone_verts;
	long	shells_of_lone_vert;
};

/*
 *  For use with tables subscripted by NMG structure "index" values,
 *  traditional test and set macros.
 *  A value of zero indicates unset, a value of one indicates set.
 *  test-and-set returns TRUE if value was unset;  in the process,
 *  value has become set.  This is often used to detect the first
 *  time an item is used, so an alternative name is given, for clarity.
 *  Note that the somewhat simpler auto-increment form
 *	( (tab)[(p)->index]++ == 0 )
 *  is not used, to avoid the possibility of integer overflow from
 *  repeated test-and-set operations on one item.
 */
#define NMG_INDEX_VALUE(_tab,_index)	((_tab)[_index])
#define NMG_INDEX_TEST(_tab,_p)		( (_tab)[(_p)->index] )
#define NMG_INDEX_SET(_tab,_p)		{(_tab)[(_p)->index] = 1;}
#define NMG_INDEX_TEST_AND_SET(_tab,_p)	\
	( (_tab)[(_p)->index] == 0 ? ((_tab)[(_p)->index] = 1) : 0 )
#define NMG_INDEX_IS_SET(_tab,_p)	NMG_INDEX_TEST(_tab,_p)
#define NMG_INDEX_FIRST_TIME(_tab,_p)	NMG_INDEX_TEST_AND_SET(_tab,_p)
#define NMG_INDEX_ASSIGN(_tab,_p,_val)	{(_tab)[(_p)->index] = _val;}
#define NMG_INDEX_OR(_tab,_p,_val)	{(_tab)[(_p)->index] |= _val;}
#define NMG_INDEX_AND(_tab,_p,_val)	{(_tab)[(_p)->index] &= _val;}

/* flags for manifold-ness */
#define NMG_3MANIFOLD	16
#define NMG_2MANIFOLD	4
#define NMG_1MANIFOLD	2
#define NMG_0MANIFOLD	1
/* #define NMG_DANGLING	8 /* NMG_2MANIFOLD + 4th bit for special cond */

#define NMG_SET_MANIFOLD(_t,_p,_v) NMG_INDEX_OR(_t, _p, _v)
#define NMG_MANIFOLDS(_t, _p)	   NMG_INDEX_VALUE(_t, (_p)->index)
#define NMG_CP_MANIFOLD(_t, _p, _q) (_t)[(_p)->index] = (_t)[(_q)->index]

#endif
