/* 
 *       R E F I N E _ S R F . C
 *
 * Function -
 *     Refines a surface by inserting knots with respect to the row or
 *     columns of the control points resulting in the same surface with new
 *     control points.
 * 
 * Author -
 *     Paul R. Stay
 *
 * Source -
 *     SECAD/VLD Computing Consortium, Bldg 394
 *     The U.S. Army Ballistic Research Laboratory
 *     Aberdeen Proving Ground, Maryland 21005
 *
 * Copyright Notice -
 *     This software is Copyright (C) 1986 by the United States Army.
 *     All rights reserved.
 */

#include <stdio.h>
#include "machine.h"
#include "vmath.h"
#include "nurb.h"

struct snurb *
rt_nurb_s_refine( srf, dir, kv )
struct snurb * srf;			/* Old surface to be refined */
int dir;				/* Direction to refine */
					/* Row = 0, Col = 1 */
struct knot_vector *kv;			/* New knot vector */
{
	register struct snurb * nurb_srf;
	struct oslo_mat *oslo;	/* oslo refinement matrix */
	int i;   

	if (dir == RT_NURB_SPLIT_ROW) {		/* Row (u) direction */

    		nurb_srf = (struct snurb *) rt_malloc( sizeof (struct snurb),
			"rt_nurb_s_refine: row snurb struct");
		
		nurb_srf->next = (struct snurb *) 0;
		nurb_srf->order[0] = srf->order[0];
		nurb_srf->order[1] = srf->order[1];
		
		rt_nurb_kvcopy(&nurb_srf->u_knots, kv);
		rt_nurb_kvcopy(&nurb_srf->v_knots, &srf->v_knots);

		nurb_srf->s_size[0] = srf->s_size[0];
		nurb_srf->s_size[1] = kv->k_size - srf->order[0];
		nurb_srf->pt_type = srf->pt_type;
		nurb_srf->ctl_points = (fastf_t *)
			rt_malloc( sizeof (fastf_t) * 
				nurb_srf->s_size[0] *
				nurb_srf->s_size[1] *
				RT_NURB_EXTRACT_COORDS( nurb_srf->pt_type),
				"rt_nurb_s_refine: row mesh control points");

		oslo = (struct oslo_mat *)
		      rt_nurb_calc_oslo (srf -> order[RT_NURB_SPLIT_ROW], &srf->u_knots, kv);

		for( i = 0; i < nurb_srf->s_size[0]; i++)
		{
			fastf_t * old_mesh_ptr;
			fastf_t * new_mesh_ptr;

			old_mesh_ptr = &srf->ctl_points[
				i * srf->s_size[1] * 
				RT_NURB_EXTRACT_COORDS( srf->pt_type)];
			new_mesh_ptr = &nurb_srf->ctl_points[
				i * nurb_srf->s_size[1] * 
				RT_NURB_EXTRACT_COORDS( nurb_srf->pt_type)];
			rt_nurb_map_oslo( oslo, old_mesh_ptr, new_mesh_ptr,
				RT_NURB_EXTRACT_COORDS( srf->pt_type ),
				RT_NURB_EXTRACT_COORDS( nurb_srf->pt_type ),
				0, kv->k_size - nurb_srf->order[0],
				nurb_srf->pt_type);
		}

		rt_nurb_free_oslo(oslo);

	} else 	{		/* Col (v) direction */

    		nurb_srf = (struct snurb *) rt_malloc( sizeof (struct snurb),
			"rt_nurb_s_refine: row snurb struct");
		
		nurb_srf->next = (struct snurb *) 0;
		nurb_srf->order[0] = srf->order[0];
		nurb_srf->order[1] = srf->order[1];
		
		rt_nurb_kvcopy(&nurb_srf->u_knots, &srf->u_knots);
		rt_nurb_kvcopy(&nurb_srf->v_knots, kv);

		nurb_srf->s_size[0] = kv->k_size - srf->order[1]; 
		nurb_srf->s_size[1] = srf->s_size[1];

		nurb_srf->pt_type = srf->pt_type;
		nurb_srf->ctl_points = (fastf_t *)
			rt_malloc( sizeof (fastf_t) * 
				nurb_srf->s_size[0] *
				nurb_srf->s_size[1] *
				RT_NURB_EXTRACT_COORDS( nurb_srf->pt_type),
				"rt_nurb_s_refine: row mesh control points");

		oslo = (struct oslo_mat *)
		      rt_nurb_calc_oslo (srf->order[RT_NURB_SPLIT_COL], &srf->v_knots, kv);

		for( i = 0; i < nurb_srf->s_size[1]; i++)
		{
			fastf_t * old_mesh_ptr;
			fastf_t * new_mesh_ptr;

			old_mesh_ptr = &srf->ctl_points[
				i * RT_NURB_EXTRACT_COORDS( srf->pt_type)];
			new_mesh_ptr = &nurb_srf->ctl_points[
				i * RT_NURB_EXTRACT_COORDS( nurb_srf->pt_type)];
			rt_nurb_map_oslo( oslo, old_mesh_ptr, new_mesh_ptr,
				srf->s_size[1] * 
				RT_NURB_EXTRACT_COORDS( srf->pt_type ),
				nurb_srf->s_size[1] * 
				RT_NURB_EXTRACT_COORDS( nurb_srf->pt_type ),
				0, kv->k_size - nurb_srf->order[1],
				nurb_srf->pt_type);
		}
		rt_nurb_free_oslo( oslo );
	} 

    return nurb_srf;
}

struct cnurb *
rt_nurb_c_refine( crv, kv )
struct cnurb * crv;
struct knot_vector * kv;
{
	struct oslo_mat * oslo;
	struct cnurb * new_crv;
	int i, coords;

	coords = RT_NURB_EXTRACT_COORDS( crv->pt_type);

	new_crv = ( struct cnurb *) rt_nurb_new_cnurb(
		crv->order, kv->k_size, kv->k_size - crv->order,
		crv->pt_type);

	oslo = (struct oslo_mat *) rt_nurb_calc_oslo(
		crv->order, &crv->knot, kv);

	rt_nurb_map_oslo( oslo, crv->ctl_points, 
		new_crv->ctl_points, 
		coords, coords, 0, 
		kv->k_size - new_crv->order,
		new_crv->pt_type);

	new_crv->knot.k_size = kv->k_size;

	for( i = 0; i < kv->k_size; i++)
		new_crv->knot.knots[i] = kv->knots[i];

	rt_nurb_free_oslo( oslo );

	return new_crv;
}
