/*		T E A . C
 *
 * Convert the Utah Teapot description from the IEEE CG&A database to the
 * BRL-CAD NMG TNURB format. (Note that this has the closed bottom)
 *
 */

/* Header files which are used for this example */

#include "conf.h"

#include <stdio.h>		/* Direct the output to stdout */
#include "machine.h"		/* BRLCAD specific machine data types */
#include "db.h"			/* BRLCAD data base format */
#include "externs.h"
#include "vmath.h"		/* BRLCAD Vector macros */
#include "nmg.h"
#include "nurb.h"		/* BRLCAD Spline data structures */
#include "raytrace.h"
#include "wdb.h"
#include "../librt/debug.h"	/* rt_g.debug flag settings */

#include "./tea.h"		/* IEEE Data Structures */
#include "./ducks.h"		/* Teapot Vertex data */
#include "./patches.h"		/* Teapot Patch data */

extern dt ducks[DUCK_COUNT];		/* Vertex data of teapot */
extern pt patches[PATCH_COUNT];		/* Patch data of teapot */

char *Usage = "This program ordinarily generates a database on stdout.\n\
	Your terminal probably wouldn't like it.";

static struct shell *s;
static struct model *m;

main(argc, argv) 			/* really has no arguments */
int argc; char *argv[];
{
	struct nmgregion *r;
	char * id_name = "NMG TNURB Example";
	char * tea_name = "UtahTeapot";
	char * uplot_name = "teapot.upl";
	struct rt_list vhead;
	struct rt_tol tol;
	FILE *fp;
	int i;

        tol.magic = RT_TOL_MAGIC;
        tol.dist = 0.005;
        tol.dist_sq = tol.dist * tol.dist;
        tol.perp = 1e-6;
        tol.para = 1 - tol.perp;

	RT_LIST_INIT( &rt_g.rtg_vlfree );

	if (isatty(fileno(stdout))) {
		(void)fprintf(stderr, "%s: %s\n", *argv, Usage);
		return(-1);
	}

	rt_g.debug |= DEBUG_ALLRAYS;	/* Cause core dumps on rt_bomb(), but no extra messages */

	while ((i=getopt(argc, argv, "d")) != EOF) {
		switch (i) {
		case 'd' : rt_g.debug |= DEBUG_MEM | DEBUG_MEM_FULL; break;
		default	:
			(void)fprintf(stderr,
				"Usage: %s [-d] > database.g\n", *argv);
			return(-1);
			break;
		}
	}

	mk_id( stdout, id_name);

	m = nmg_mm();
	NMG_CK_MODEL( m );
	r = nmg_mrsv( m );
	NMG_CK_REGION( r );
	s = RT_LIST_FIRST( shell , &r->s_hd );
	NMG_CK_SHELL( s );

	/* Step through each patch and create a NMG TNURB face
	 * representing the patch then dump them out.
	 */

	for( i = 0; i < PATCH_COUNT; i++)
	{
		dump_patch( patches[i] );
	}

/*	(void)nmg_model_fuse( m , &tol );	*/

	/* Make a vlist for the model */
	RT_LIST_INIT( &vhead );
	nmg_m_to_vlist( &vhead , m , 0 );

	/* Make a UNIX plot file from this vlist */
	if( (fp=fopen( uplot_name , "w" )) == NULL )
	{
		rt_log( "Cannot open plot file: %s\n" , uplot_name );
		perror( "teapot_nmg" );
	}
	else
		rt_vlist_to_uplot( fp , &vhead );

	/* write NMG to output file */
	(void)mk_nmg( stdout , tea_name , m );

	return(0);
}

/* IEEE patch number of the Bi-Cubic Bezier patch and convert it
 * to a B-Spline surface (Bezier surfaces are a subset of B-spline surfaces
 * and output it to a BRLCAD binary format.
 */

dump_patch( patch )
pt patch;
{
	struct vertex *verts[4];
	struct faceuse *fu;
	struct loopuse *lu;
	struct edgeuse *eu;
	int i,j, pt_type;
	fastf_t *mesh=NULL;
	fastf_t *ukv=NULL;
	fastf_t *vkv=NULL;

	/* U and V parametric Direction Spline parameters
	 * Cubic = order 4, 
	 * knot size is Control point + order = 8
	 * control point size is 4
	 * point size is 3
	 */

	for( i=0 ; i<4 ; i++ )
		verts[i] = (struct vertex *)NULL;

	fu = nmg_cface( s , verts , 4 );
	NMG_CK_FACEUSE( fu );

	for( i=0 ; i<4 ; i++ )
	{
		struct vertexuse *vu;
		vect_t uvw;
		point_t pt;
		int k,j;

		switch( i )
		{
			case 0:
				VSET( uvw , 0.0 , 0.0 , 0.0 );
				k = 0;
				j = 0;
				break;
			case 1:
				VSET( uvw , 1.0 , 0.0 , 0.0 );
				k = 3;
				j = 0;
				break;
			case 2:
				VSET( uvw , 1.0 , 1.0 , 0.0 );
				k = 3;
				j = 3;
				break;
			case 3:
				VSET( uvw , 0.0 , 1.0 , 0.0 );
				k = 0;
				j = 3;
				break;
		}

		VSET( pt , ducks[patch[k][j]-1].x * 1000 , ducks[patch[k][j]-1].y * 1000 , ducks[patch[k][j]-1].z * 1000 );
		nmg_vertex_gv( verts[i] , pt );

		for( RT_LIST_FOR( vu , vertexuse , &verts[i]->vu_hd ) )
			nmg_vertexuse_a_cnurb( vu , uvw );
	}

	pt_type = RT_NURB_MAKE_PT_TYPE(3, 2,0); /* see nurb.h for details */

	nmg_face_g_snurb( fu , 4 , 4 , 8 , 8 , ukv , vkv , 4 , 4 , pt_type , mesh );

	NMG_CK_FACE( fu->f_p );
	NMG_CK_FACE_G_SNURB( fu->f_p->g.snurb_p );
	mesh = fu->f_p->g.snurb_p->ctl_points;

	/* Copy the control points */

	for( i = 0; i< 4; i++)
	for( j = 0; j < 4; j++)
	{
		*mesh = ducks[patch[i][j]-1].x * 1000;
		*(mesh+1) = ducks[patch[i][j]-1].y * 1000;
		*(mesh+2) = ducks[patch[i][j]-1].z * 1000;
		mesh += 3;
	}

	/* Both u and v knot vectors are [ 0 0 0 0 1 1 1 1] */
	ukv = fu->f_p->g.snurb_p->u.knots;
	vkv = fu->f_p->g.snurb_p->v.knots;
	/* set the knot vectors */
	for( i=0 ; i<4 ; i++ )
	{
		*(ukv+i) = 0.0;
		*(vkv+i) = 0.0;
	}
	for( i=0 ; i<4 ; i++ )
	{
		*(ukv+4+i) = 1.0;
		*(vkv+4+i) = 1.0;
	}

	/* set eu geometry */
	lu = RT_LIST_FIRST( loopuse , &fu->lu_hd );
	NMG_CK_LOOPUSE( lu );
	for( RT_LIST_FOR( eu , edgeuse , &lu->down_hd ) )
	{
		fastf_t *kv=NULL;
		fastf_t *points=NULL;

		nmg_edge_g_cnurb( eu , 4 , 0 , kv , 2 , pt_type , points );
	}
}
