/*
 *			D B _ A N I M . C
 *
 *  Routines to apply animation directives to geometry database.
 *
 *  Author -
 *	Michael John Muuss
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1987 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSanim[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>
#include "machine.h"
#include "vmath.h"
#include "rtstring.h"
#include "raytrace.h"
#include "./debug.h"


/*
 *			D B _ A D D _ A N I M
 *
 *  Add a user-supplied animate structure to the end of the chain of such
 *  structures hanging from the directory structure of the last node of
 *  the path specifier.  When 'root' is non-zero, this matrix is
 *  located at the root of the tree itself, rather than an arc, and is
 *  stored differently.
 *
 *  In the future, might want to check to make sure that callers directory
 *  references are in the right database (dbip).
 */
int
db_add_anim( dbip, anp, root )
struct db_i *dbip;
register struct animate *anp;
int	root;
{
	register struct animate **headp;
	struct directory	*dp;

	/* Could validate an_type here */

	RT_CK_ANIMATE(anp);
	anp->an_forw = ANIM_NULL;
	if( root )  {
		if( rt_g.debug&DEBUG_ANIM )
			rt_log("db_add_anim(x%x) root\n", anp);
		headp = &(dbip->dbi_anroot);
	} else {
		dp = DB_FULL_PATH_CUR_DIR(&anp->an_path);
		if( rt_g.debug&DEBUG_ANIM )
			rt_log("db_add_anim(x%x) arc %s\n", anp,
				dp->d_namep);
		headp = &(dp->d_animate);
	}

	/* Append to list */
	while( *headp != ANIM_NULL ) {
		RT_CK_ANIMATE(*headp);
		headp = &((*headp)->an_forw);
	}
	*headp = anp;
	return(0);			/* OK */
}

static char	*db_anim_matrix_strings[] = {
	"(nope)",
	"ANM_RSTACK",
	"ANM_RARC",
	"ANM_LMUL",
	"ANM_RMUL",
	"ANM_RBOTH",
	"eh?"
};

/*
 *			D B _ D O _ A N I M
 *
 *  Perform the one animation operation.
 *  Leave results in form that additional operations can be cascaded.
 */
int
db_do_anim( anp, stack, arc, materp )
register struct animate *anp;
mat_t	stack;
mat_t	arc;
struct mater_info	*materp;
{
	mat_t	temp;

	if( rt_g.debug&DEBUG_ANIM )
		rt_log("db_do_anim(x%x) ", anp);
	RT_CK_ANIMATE(anp);
	switch( anp->an_type )  {
	case RT_AN_MATRIX:
		if( rt_g.debug&DEBUG_ANIM )  {
			int	op = anp->an_u.anu_m.anm_op;
			if( op < 0 )  op = 0;
			rt_log("matrix, op=%s (%d)\n",
				db_anim_matrix_strings[op], op);
			if( rt_g.debug&DEBUG_ANIM_FULL )  {
				mat_print("on original arc", arc);
				mat_print("on original stack", stack);
			}
		}
		switch( anp->an_u.anu_m.anm_op )  {
		case ANM_RSTACK:
			mat_copy( stack, anp->an_u.anu_m.anm_mat );
			break;
		case ANM_RARC:
			mat_copy( arc, anp->an_u.anu_m.anm_mat );
			break;
		case ANM_RBOTH:
			mat_copy( stack, anp->an_u.anu_m.anm_mat );
			mat_idn( arc );
			break;
		case ANM_LMUL:
			/* arc = DELTA * arc */
			mat_mul( temp, anp->an_u.anu_m.anm_mat, arc );
			mat_copy( arc, temp );
			break;
		case ANM_RMUL:
			/* arc = arc * DELTA */
			mat_mul( temp, arc, anp->an_u.anu_m.anm_mat );
			mat_copy( arc, temp );
			break;
		default:
			return(-1);		/* BAD */
		}
		if( rt_g.debug&DEBUG_ANIM_FULL )  {
			mat_print("arc result", arc);
			mat_print("stack result", stack);
		}
		break;
	case RT_AN_MATERIAL:
		if( rt_g.debug&DEBUG_ANIM )
			rt_log("property\n");
		/*
		 * if the caller does not care about property, a null
		 * mater pointer is given.
		 */
		if (!materp) break;
		if ((anp->an_u.anu_p.anp_op == RT_ANP_RBOTH) ||
		    (anp->an_u.anu_p.anp_op == RT_ANP_RMATERIAL)) {
			strncpy(materp->ma_matname,
			    rt_vls_addr(&anp->an_u.anu_p.anp_matname), 32);
			materp->ma_matname[31] = '\0';
		}
		if ((anp->an_u.anu_p.anp_op == RT_ANP_RBOTH) ||
		    (anp->an_u.anu_p.anp_op == RT_ANP_RPARAM)) {
			strncpy(materp->ma_matparm,
			    rt_vls_addr(&anp->an_u.anu_p.anp_matparam), 60);
			materp->ma_matparm[59] = '\0';
		}
		if (anp->an_u.anu_p.anp_op == RT_ANP_APPEND) {
			strncat(materp->ma_matparm,
			    rt_vls_addr(&anp->an_u.anu_p.anp_matparam),
			    60-strlen(materp->ma_matparm));
			materp->ma_matparm[59] = '\0';
		}
		break;
	case RT_AN_COLOR:
		if( rt_g.debug&DEBUG_ANIM )
			rt_log("color\n");
		/*
		 * if the caller does not care about property, a null
		 * mater pointer is given.
		 */
		if (!materp) break;
		materp->ma_override = 1;	/* XXX - really override? */
		materp->ma_color[0] =
		    (((double)anp->an_u.anu_c.anc_rgb[0])+0.5)*rt_inv255;
		materp->ma_color[1] =
		    (((double)anp->an_u.anu_c.anc_rgb[1])+0.5)*rt_inv255;
		materp->ma_color[2] =
		    (((double)anp->an_u.anu_c.anc_rgb[2])+0.5)*rt_inv255;
		break;
	default:
		if( rt_g.debug&DEBUG_ANIM )
			rt_log("unknown op\n");
		/* Print something here? */
		return(-1);			/* BAD */
	}
	return(0);				/* OK */
}

/*
 *			D B _ F R E E _ A N I M
 *
 *  Release chain of animation structures
 * XXX really need another subroutine in common,
 * XXX which knows how to free the vls strings, etc.
 */
void
db_free_anim( dbip )
register struct db_i *dbip;
{
	register struct animate *anp;
	register struct directory *dp;
	register int		i;

	/* Rooted animations */
	for( anp = dbip->dbi_anroot; anp != ANIM_NULL; )  {
		register struct animate *nextanp;
		RT_CK_ANIMATE(anp);
		nextanp = anp->an_forw;

		db_free_full_path( &anp->an_path );
		rt_free( (char *)anp, "struct animate");
		anp = nextanp;
	}
	dbip->dbi_anroot = ANIM_NULL;

	/* Node animations */
	for( i=0; i < RT_DBNHASH; i++ )  {
		dp = dbip->dbi_Head[i];
		for( ; dp != DIR_NULL; dp = dp->d_forw )  {
			for( anp = dp->d_animate; anp != ANIM_NULL; )  {
				register struct animate *nextanp;
				RT_CK_ANIMATE(anp);
				nextanp = anp->an_forw;

				db_free_full_path( &anp->an_path );
				rt_free( (char *)anp, "struct animate");
				anp = nextanp;
			}
			dp->d_animate = ANIM_NULL;
		}
	}
}

/*
 *			D B _ P A R S E _ A N I M
 *
 *  A common parser for mged and rt.
 *  Experimental.
 */
int
db_parse_anim( dbip, argc, argv )
struct db_i	*dbip;
int		argc;
char		**argv;
{
	struct db_tree_state	ts;
	struct animate		*anp;
	int	i;
	int	at_root = 0;

	GETSTRUCT( anp, animate );
	anp->magic = ANIMATE_MAGIC;

	if( argv[1][0] == '/' )
		at_root = 1;

	bzero( (char *)&ts, sizeof(ts) );
	ts.ts_dbip = dbip;
	mat_idn( ts.ts_mat );
	anp->an_path.fp_len = anp->an_path.fp_maxlen = 0;
	anp->an_path.fp_names = (struct directory **)0;
	if( db_follow_path_for_state( &ts, &(anp->an_path), argv[1], LOOKUP_NOISY ) < 0 )
		goto bad;

	if( anp->an_path.fp_len > 1 )
		at_root = 0;

	if( strcmp( argv[2], "matrix" ) == 0 )  {
		anp->an_type = RT_AN_MATRIX;
		if( strcmp( argv[3], "rstack" ) == 0 )
			anp->an_u.anu_m.anm_op = ANM_RSTACK;
		else if( strcmp( argv[3], "rarc" ) == 0 )
			anp->an_u.anu_m.anm_op = ANM_RARC;
		else if( strcmp( argv[3], "lmul" ) == 0 )
			anp->an_u.anu_m.anm_op = ANM_LMUL;
		else if( strcmp( argv[3], "rmul" ) == 0 )
			anp->an_u.anu_m.anm_op = ANM_RMUL;
		else if( strcmp( argv[3], "rboth" ) == 0 )
			anp->an_u.anu_m.anm_op = ANM_RBOTH;
		else  {
			rt_log("db_parse_anim:  Matrix op '%s' unknown\n",
				argv[3]);
			goto bad;
		}
		for( i=0; i<16; i++ )
			anp->an_u.anu_m.anm_mat[i] = atof( argv[i+4] );
	} else if( strcmp( argv[2], "material" ) == 0 )  {
		anp->an_type = RT_AN_MATERIAL;
		RT_VLS_INIT( &anp->an_u.anu_p.anp_matname );
		RT_VLS_INIT( &anp->an_u.anu_p.anp_matparam );
		if( strcmp( argv[3], "rboth" ) == 0 )  {
			rt_vls_strcpy( &anp->an_u.anu_p.anp_matname, argv[4] );
			rt_vls_from_argv( &anp->an_u.anu_p.anp_matparam,
				argc-5, &argv[5] );
			anp->an_u.anu_p.anp_op = RT_ANP_RBOTH;
		} else if( strcmp( argv[3], "rmaterial" ) == 0 )  {
			rt_vls_strcpy( &anp->an_u.anu_p.anp_matname, argv[4] );
			anp->an_u.anu_p.anp_op = RT_ANP_RMATERIAL;
		} else if( strcmp( argv[3], "rparam" ) == 0 )  {
			rt_vls_from_argv( &anp->an_u.anu_p.anp_matparam,
				argc-4, &argv[4] );
			anp->an_u.anu_p.anp_op = RT_ANP_RPARAM;
		} else if( strcmp( argv[3], "append" ) == 0 )  {
			rt_vls_from_argv( &anp->an_u.anu_p.anp_matparam,
				argc-4, &argv[4] );
			anp->an_u.anu_p.anp_op = RT_ANP_APPEND;
		} else {
			rt_log("db_parse_anim:  material animation '%s' unknown\n",
				argv[3]);
			goto bad;
		}
	} else if( strcmp( argv[2], "color" ) == 0 )  {
		anp->an_type = RT_AN_COLOR;
		anp->an_u.anu_c.anc_rgb[0] = atoi( argv[3+0] );
		anp->an_u.anu_c.anc_rgb[1] = atoi( argv[3+1] );
		anp->an_u.anu_c.anc_rgb[2] = atoi( argv[3+2] );
	} else {
		rt_log("db_parse_anim:  animation type '%s' unknown\n", argv[2]);
		goto bad;
	}
	if( db_add_anim( dbip, anp, at_root ) < 0 )  {
		goto bad;
	}
	return(0);
bad:
	db_free_full_path( &anp->an_path );
	rt_free( (char *)anp, "animate");
	return(-1);		/* BAD */
}
void
db_write_anim(fop, anp)
FILE *fop;
struct animate *anp;
{
	char *thepath;
	int i;

	RT_CK_ANIMATE(anp);

	thepath  = db_path_to_string(&(anp->an_path));

	fprintf(fop,"anim %s ", thepath);
	rt_free(thepath, "path string");

	switch (anp->an_type) {
	case RT_AN_MATRIX:
		fputs("matrix ",fop);
		switch (anp->an_u.anu_m.anm_op) {
		case ANM_RSTACK:
			fputs("rstack\n", fop);
			break;
		case ANM_RARC:
			fputs("rarc\n", fop);
			break;
		case ANM_LMUL:
			fputs("lmul\n", fop);
			break;
		case ANM_RMUL:
			fputs("rmul\n", fop);
			break;
		case ANM_RBOTH:
			fputs("rboth\n", fop);
			break;
		default:
			fputs("unknown\n",fop);
			rt_log("db_write_anim: unknown matrix operation\n");
		}
		for (i=0; i<16; i++) {
			fprintf(fop, " %8.3f", anp->an_u.anu_m.anm_mat[i]);
			if ((i == 15) || ((i&3) == 3)) {
				fputs("\n",fop);
			}
		}
		break;
	case RT_AN_MATERIAL:
		fputs("material ",fop);
		switch (anp->an_u.anu_p.anp_op) {
		case RT_ANP_RBOTH:
			fputs("rboth ", fop);
			break;
		case RT_ANP_RMATERIAL:
			fputs("rmaterial ", fop);
			break;
		case RT_ANP_RPARAM:
			fputs("rparam ", fop);
			break;
		case RT_ANP_APPEND:
			fputs("append ", fop);
			break;
		default:
			rt_log("db_write_anim: unknown property operation.\n");
			break;
		}
		break;
	case RT_AN_COLOR:
		fprintf(fop,"color %d %d %d", anp->an_u.anu_c.anc_rgb[0],
		    anp->an_u.anu_c.anc_rgb[1], anp->an_u.anu_c.anc_rgb[2]);
		break;
	case RT_AN_SOLID:
		break;
	default:
		rt_log("db_write_anim: Unknown animate type.\n");
	}
	fputs(";\n", fop);
	return;
}
