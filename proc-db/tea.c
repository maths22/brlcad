/*		T E A . C
 *
 * Convert the Utah Teapot description from the IEEE CG&A database to the
 * BRL-CAD spline format. (Note that this has the closed bottom)
 *
 */

/* Header files which are used for this example */

#include <stdio.h>		/* Direct the output to stdout */
#include "machine.h"		/* BRLCAD specific machine data types */
#include "db.h"			/* BRLCAD data base format */
#include "vmath.h"		/* BRLCAD Vector macros */
#include "../libspl/b_spline.h"		/* BRLCAD Spline data structures */
#include "./tea.h"		/* IEEE Data Structures */
#include "./ducks.h"		/* Teapot Vertex data */
#include "./patches.h"		/* Teapot Patch data */
#include "raytrace.h"
#include "../librt/debug.h"	/* rt_g.debug flag settings */

extern dt ducks[DUCK_COUNT];		/* Vertex data of teapot */
extern pt patches[PATCH_COUNT];		/* Patch data of teapot */

char *Usage = "This program ordinarily generates a database on stdout.\n\
	Your terminal probably wouldn't like it.";

main(argc, argv) 			/* really has no arguments */
int argc; char *argv[];
{
	char * id_name = "Spline Example";
	char * tea_name = "UtahTeapot";
	int i;


	if (isatty(fileno(stdout))) {
		(void)fprintf(stderr, "%s: %s\n", *argv, Usage);
		return(-1);
	}

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


	/* Setup information 
   	 * Database header record
	 *	File name
	 * B-Spline Solid record
	 * 	Name, Number of Surfaces, resolution (not used)
	 *
	 */

	mk_id( stdout, id_name);
	mk_bsolid( stdout, tea_name, PATCH_COUNT, 1.0);

	/* Step through each patch and create a B_SPLINE surface
	 * representing the patch then dump them out.
	 */

	for( i = 0; i < PATCH_COUNT; i++)
	{
		dump_patch( patches[i] );
	}

}

/* IEEE patch number of the Bi-Cubic Bezier patch and convert it
 * to a B-Spline surface (Bezier surfaces are a subset of B-spline surfaces
 * and output it to a BRLCAD binary format.
 */

dump_patch( patch )
pt patch;
{
	struct b_spline * b_patch;
	int i,j;
	fastf_t * mesh_pointer;

	/* U and V parametric Direction Spline parameters
	 * Cubic = order 4, 
	 * knot size is Control point + order = 4
	 * control point size is 4
	 * point size is 3
	 */

	b_patch = (struct b_spline *) spl_new( 4, 4, 8, 8, 4, 4, 3);
	
	/* Now fill in the pieces */

	/* Both u and v knot vectors are [ 0 0 0 0 1 1 1 1] 
	 * spl_kvknot( order, lower parametric value, upper parametric value,
	 * 		Number of interior knots )
 	 */
	

	rt_free((char *)b_patch->u_kv->knots, "dumping u_kv knots I'm about to realloc");
	rt_free((char *)b_patch->u_kv, "dumping u_kv I'm about to realloc");
	b_patch->u_kv = 
	    (struct knot_vec *) spl_kvknot( 4, 0.0, 1.0, 0);

	rt_free((char *)b_patch->v_kv->knots, "dumping v_kv knots I'm about to realloc");
	rt_free((char *)b_patch->v_kv, "dumping v_kv I'm about to realloc");
	b_patch->v_kv = 
	    (struct knot_vec *) spl_kvknot( 4, 0.0, 1.0, 0);

	if (rt_g.debug) {
		rt_ck_malloc_ptr(b_patch, "b_patch");
		rt_ck_malloc_ptr(b_patch->u_kv, "b_patch->u_kv");
		rt_ck_malloc_ptr(b_patch->u_kv->knots,
			"b_patch->u_kv->knots");
		rt_ck_malloc_ptr(b_patch->v_kv, "b_patch->v_kv");
		rt_ck_malloc_ptr(b_patch->v_kv->knots,
			"b_patch->v_kv->knots");
	}

	/* Copy the control points */

	mesh_pointer = b_patch->ctl_mesh->mesh;

	for( i = 0; i< 4; i++)
	for( j = 0; j < 4; j++)
	{
		*mesh_pointer = ducks[patch[i][j]-1].x * 1000;
		*(mesh_pointer+1) = ducks[patch[i][j]-1].y * 1000;
		*(mesh_pointer+2) = ducks[patch[i][j]-1].z * 1000;
		mesh_pointer += 3;
	}

	/* Output the the b_spline through the libwdb interface */
	mk_bsurf( stdout, b_patch);
}
