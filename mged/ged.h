/*
 *			G E D . H
 *
 * This file contains all of the definitions local to
 * the GED graphics editor.
 *
 *	     V E R Y   I M P O R T A N T   N O T I C E ! ! !
 *
 *  Many people in the computer graphics field use post-multiplication,
 *  (thanks to Newman and Sproull) with row vectors, ie:
 *
 *		view_vec = model_vec * T
 *
 *  However, in the GED system, the more traditional representation
 *  of column vectors is used (ref: Gwyn).  Therefore, when transforming
 *  a vector by a matrix, pre-multiplication is used, ie:
 *
 *		view_vec = model2view_mat * model_vec
 *
 *  Furthermore, additional transformations are multiplied on the left, ie:
 *
 *		vec'  =  T1 * vec
 *		vec'' =  T2 * T1 * vec  =  T2 * vec'
 *
 *  The most notable implication of this is the location of the
 *  "delta" (translation) values in the matrix, ie:
 *
 *        x'     ( R0   R1   R2   Dx )      x
 *        y' =  (  R4   R5   R6   Dy  )  *  y
 *        z'    (  R8   R9   R10  Dz  )     z
 *        w'     (  0    0    0   1/s)      w
 *
 *  This of course requires that the rotation portion be computed
 *  using somewhat different formulas (see buildHrot for both kinds).
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
#if USE_PROTOTYPES
#	define	MGED_EXTERN(type_and_name,args)	extern type_and_name args
#	define	MGED_ARGS(args)			args
#else
#	define	MGED_EXTERN(type_and_name,args)	extern type_and_name()
#	define	MGED_ARGS(args)			()
#endif

extern double	degtorad, radtodeg;	/* Defined in usepen.c */

/*
 * All GED files are stored in a fixed base unit (MM).
 * These factors convert database unit to local (or working) units.
 */
extern struct db_i	*dbip;			/* defined in ged.c */
#define	base2local	(dbip->dbi_base2local)
#define local2base	(dbip->dbi_local2base)
#define localunit	(dbip->dbi_localunit)	/* current local unit (index) */
#define	cur_title	(dbip->dbi_title)	/* current model title */
extern char		*local_unit[];			/* titles.c */

extern int		dmaflag;		/* !0 forces screen update */

/* Tolerances */
extern double		mged_abs_tol;		/* abs surface tolerance */
extern double		mged_rel_tol;		/* rel surface tolerance */
extern double		mged_nrm_tol;		/* surface normal tolerance */

/* default region codes       defined in mover.c */
extern int	item_default;
extern int	air_default;
extern int	mat_default;
extern int	los_default;

/*
 *  Definitions.
 *
 *  Solids are defined in "model space".
 *  The screen is in "view space".
 *  The visible part of view space is -1.0 <= x,y,z <= +1.0
 *
 *  The transformation from the origin of model space to the
 *  origin of view space (the "view center") is contained
 *  in the matrix "toViewcenter".  The viewing rotation is
 *  contained in the "Viewrot" matrix.  The viewscale factor
 *  (for [15] use) is kept in the float "Viewscale".
 *
 *  model2view = Viewscale * Viewrot * toViewcenter;
 *
 *  model2view is the matrix going from model space coordinates
 *  to the view coordinates, and view2model is the inverse.
 *  It is recomputed by new_mats() only.
 *
 * CHANGE matrix.  Defines the change between the un-edited and the
 * present state in the edited solid or combination.
 *
 * model2objview = modelchanges * model2view
 *
 *  For object editing and solid edit, model2objview translates
 *  from model space to view space with all the modelchanges too.
 *
 *  These are allocated storage in dozoom.c
 */
extern fastf_t	Viewscale;		/* dist from center to edge of RPP */
extern mat_t	Viewrot;
extern mat_t	toViewcenter;
extern mat_t	model2view, view2model;
extern mat_t	model2objview, objview2model;
extern mat_t	modelchanges;		/* full changes this edit */
extern mat_t	incr_change;		/* change(s) from last cycle */
				
#define VIEWSIZE	(2*Viewscale)	/* Width of viewing cube */
#define VIEWFACTOR	(1/Viewscale)	/* 2.0 / VIEWSIZE */

/*
 * Identity matrix.  Handy to have around. - initialized in e1.c
 */
extern mat_t	identity;

/* defined in buttons.c */
extern fastf_t	acc_sc_sol;	/* accumulate solid scale factor */
extern fastf_t	acc_sc[3];	/* accumulate local object scale factors */
extern mat_t	acc_rot_sol;	/* accumulate solid rotations */

/* defined in dodraw.c */
extern int	no_memory;	/* flag indicating memory for drawing is used up */

/* defined in menu.c */
extern int	menuflag;	/* flag indicating if a menu item is selected */

/*
 * These variables are global for the benefit of
 * the display portion of dozoom. - defined in adc.c
 */
extern fastf_t	curs_x;		/* cursor X position */
extern fastf_t	curs_y;		/* cursor Y position */
extern fastf_t	c_tdist;	/* Cursor tick distance */
extern fastf_t	angle1;		/* Angle to solid wiper */
extern fastf_t	angle2;		/* Angle to dashed wiper */

/* defined in ged.c */
extern FILE *infile;
extern FILE *outfile;
/*
 *	GED functions referenced in more than one source file:
 */
extern void		dir_build(), buildHrot(), dozoom(),
			pr_schain();
extern void		eraseobj(), mged_finish(), slewview(),
			mmenu_init(), moveHinstance(), moveHobj(), pr_solid(),
			quit(), refresh(), rej_sedit(), sedit(),
			sig2(), dir_print(),
			usepen(), setview(),
			adcursor(), mmenu_display(),
			col_item(), col_putchar(), col_eol(), col_pr4v();
extern void		sedit_menu();
extern void		attach(), release(), get_attached();
extern void		(*cur_sigint)();	/* Current SIGINT status */
extern void		aexists(), f_quit();
extern int		clip(), getname(), use_pen();
extern struct directory	*combadd(), **dir_getspace();
extern void		ellipse();

/* memalloc.c */
MGED_EXTERN(unsigned long memalloc, (struct mem_map **pp, unsigned size) );
MGED_EXTERN(unsigned long memget, (struct mem_map **pp, unsigned int size,
	unsigned int place) );
MGED_EXTERN(void memfree, (struct mem_map **pp, unsigned size, unsigned long addr) );
MGED_EXTERN(void mempurge, (struct mem_map **pp) );
MGED_EXTERN(void memprint, (struct mem_map **pp) );

/* The matrix math routines */
MGED_EXTERN(double mat_atan2, (double y, double x) );
MGED_EXTERN(void mat_zero, (mat_t m) );
MGED_EXTERN(void mat_idn, (mat_t m) );
MGED_EXTERN(void mat_copy, (mat_t dest, mat_t src) );
MGED_EXTERN(void mat_mul, (mat_t dest, mat_t a, mat_t b) );
MGED_EXTERN(void matXvec, (vect_t dest, mat_t m, vect_t src) );
MGED_EXTERN(void mat_inv, (mat_t dest, mat_t src) );
/* XXX these two need mat_ on their names */
MGED_EXTERN(void vtoh_move, (hvect_t dest, vect_t src) );
MGED_EXTERN(void htov_move, (vect_t dest, hvect_t src) );
MGED_EXTERN(void mat_print, (char *title, mat_t m) );
MGED_EXTERN(void mat_trn, (mat_t dest, mat_t src) );
MGED_EXTERN(void mat_ae, (mat_t dest, double azimuth, double elev) );
/* XXX new name */
MGED_EXTERN(void ae_vec, (fastf_t *azp, fastf_t *elp, vect_t src) );
MGED_EXTERN(void mat_angles, (mat_t dest, double alpha, double beta, double ggamma) );
/* XXX new name */
MGED_EXTERN(void eigen2x2, (fastf_t *val1, fastf_t *val2,
	vect_t vec1, vect_t vec2, double a, double b, double c) );
MGED_EXTERN(void mat_fromto, (mat_t dest, vect_t from, vect_t to) );
MGED_EXTERN(void mat_xrot, (mat_t dest, double sinx, double cosx) );
MGED_EXTERN(void mat_yrot, (mat_t dest, double siny, double cosy) );
MGED_EXTERN(void mat_zrot, (mat_t dest, double sinz, double cosz) );
MGED_EXTERN(void mat_lookat, (mat_t dest, vect_t dir, int yflip) );
/* XXX new names */
MGED_EXTERN(void vec_ortho, (vect_t dest, vect_t src) );
MGED_EXTERN(void vec_perp, (vect_t dest, vect_t src) );

/* buttons.c */
MGED_EXTERN(void button, (int bnum) );
MGED_EXTERN(void press, (char *str) );
MGED_EXTERN(char *label_button, (int bnum) );
MGED_EXTERN(int not_state, (int desired, char *str) );
MGED_EXTERN(int chg_state, (int from, int to, char *str) );
MGED_EXTERN(void state_err, (char *str) );

#ifndef	NULL
#define	NULL		0
#endif

/*
 * "Standard" flag settings
 */
#define UP	0
#define DOWN	1

/*
 * Pointer to solid in solid table to be illuminated. - defined in usepen.c
 */
extern struct solid	*illump;/* == 0 if none, else points to ill. solid */
extern int	sedraw;		/* apply solid editing changes */

/* defined in buttons.c */
extern int	adcflag;	/* angle/distance cursor in use */

/* defined in chgview.c */
extern int	inpara;		/* parameter input from keyboard flag */
extern int	newedge;	/* new edge for arb editing */

/* defined in usepen.c */
extern int	ipathpos;	/* path index of illuminated element */

#define RARROW		001
#define UARROW		002
#define SARROW		004
#define	ROTARROW	010	/* Object rotation enabled */
extern int	movedir;	/* RARROW | UARROW | SARROW | ROTARROW */

extern int	edobj;		/* object editing options */

/* Flags for line type decisions */
#define ROOT	0
#define INNER	1

/*
 *  Editor States
 */
extern int state;			/* (defined in dozoom.c) */
extern char *state_str[];		/* identifying strings */
#define ST_VIEW		1		/* Viewing only */
#define ST_S_PICK	2		/* Picking for Solid Edit */
#define ST_S_EDIT	3		/* Solid Editing */
#define ST_O_PICK	4		/* Picking for Object Edit */
#define ST_O_PATH	5		/* Path select for Object Edit */
#define ST_O_EDIT	6		/* Object Editing */

#define MIN(a,b)	if( (b) < (a) )  a = b
#define MAX(a,b)	if( (b) > (a) )  a = b

#ifndef GETSTRUCT
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
#endif

#define	MAXLINE		10240	/* Maximum number of chars per line */

/*
 *  Helpful macros to inform the user of trouble encountered in
 *  library routines, and bail out.
 *  They are intended to be used mainly in top-level command processing
 *  routines, and therefore include a "return" statement and curley brackets.
 *  Thus, they should only be used in void functions.
 *  The word "return" is not in upper case in these macros,
 *  to enable editor searches for the word "return" to succeed.
 */
/* For errors from db_get() or db_getmrec() */
#define READ_ERR_return		{ \
	(void)printf("Database read error, aborting\n"); \
	return;  }

/* For errors from db_put() */
#define WRITE_ERR_return	{ \
	(void)printf("Database write error, aborting.\n"); \
	ERROR_RECOVERY_SUGGESTION; \
	return;  }

/* For errors from db_diradd() or db_alloc() */
#define ALLOC_ERR_return	{ \
	(void)printf("\
An error has occured while adding a new object to the database.\n"); \
	ERROR_RECOVERY_SUGGESTION; \
	return;  }

/* For errors from db_delete() or db_dirdelete() */
#define DELETE_ERR_return(_name)	{  \
	(void)printf("\
An error has occured while deleting '%s' from the database.\n", _name); \
	ERROR_RECOVERY_SUGGESTION; \
	return;  }

/* A verbose message to attempt to soothe and advise the user */
#define	ERROR_RECOVERY_SUGGESTION	\
	(void)printf("\
The in-memory table of contents may not match the status of the on-disk\n\
database.  The on-disk database should still be intact.  For safety,\n\
you should exit MGED now, and resolve the I/O problem, before continuing.\n")
