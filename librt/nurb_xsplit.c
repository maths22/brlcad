/*	 	N U R B _ S P L I T . C
 *
 * Function -
 * 	Subdivide a nurb surface by inserting a multiple knot of
 * 	of the surface order in a given direction and return the 
 *	resulting surfaces.
 *
 * Author-
 *	Paul Randal Stay
 *
 * Source
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The US Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland 21005
 *
 *
 * Copyright Notice -
 * 	This software if Copyright (C) 1990 by the United States Arym.
 *	All Rights Reserved.
 */

#include <stdio.h>
#include "machine.h"
#include "vmath.h"
#include "nurb.h"

/* Algorithm
 *
 * 	Given a parametric direction (u or v) look at the direction 
 * knot vector and insert a multiple knot of parametric direction surface
 * order. This is somewhat different than rt_nurb_split in that the surface is 
 * give a parametric value at which to split the surface. 
 * rt_nurb_kvmult does the right thing in inserting a multiple knot with the 
 * correct amount. Separate the surface and return the two resulting surface.
 */

struct snurb *
rt_nurb_s_xsplit( srf,param, dir)
struct snurb * srf;
fastf_t param;
int dir;
{
	struct knot_vector new_kv;
	fastf_t value;
	struct oslo_mat * oslo;
	int i;
	int k_index;
	struct snurb * srf1, * srf2;

	if ( dir == RT_NURB_SPLIT_ROW )
	{
		rt_nurb_kvmult(&new_kv, &srf->u_knots, srf->order[0], param);

		k_index = srf->order[0];

		oslo = ( struct oslo_mat *) 
			rt_nurb_calc_oslo( srf->order[RT_NURB_SPLIT_ROW], &srf->u_knots, &new_kv);

		srf1 = (struct snurb *) rt_malloc( sizeof( struct snurb),
			"rt_nurb_s_xsplit: row snurb struct");
		srf1->order[0]  = srf->order[0];
		srf1->order[1]  = srf->order[1];
		srf1->dir = RT_NURB_SPLIT_ROW;
		rt_nurb_kvextract(&srf1->u_knots, &new_kv, 0, k_index + srf1->order[0]);
		rt_nurb_kvcopy(&srf1->v_knots, &srf->v_knots);
		
		srf1->pt_type = srf->pt_type;
		srf1->s_size[0] = srf1->v_knots.k_size - 
			srf1->order[1];
		srf1->s_size[1] = srf1->u_knots.k_size - 
			srf1->order[0];

		srf1->ctl_points = (fastf_t *)
			rt_malloc( sizeof(fastf_t) * srf1->s_size[0] *
				srf1->s_size[1] * 
				RT_NURB_EXTRACT_COORDS( srf1->pt_type),
				"rt_nurb_s_xsplit: srf1 row mesh control points");

		srf2 = (struct snurb *) rt_malloc( sizeof( struct snurb),
			"rt_nurb_s_xsplit: row snurb struct");
		srf2->order[0]  = srf->order[0];
		srf2->order[1]  = srf->order[1];
		srf2->dir = RT_NURB_SPLIT_ROW;
		rt_nurb_kvextract(&srf2->u_knots, &new_kv, k_index, new_kv.k_size);
		rt_nurb_kvcopy(&srf2->v_knots, &srf->v_knots);
		
		srf2->pt_type = srf->pt_type;
		srf2->s_size[0] = srf2->v_knots.k_size - 
			srf2->order[1];
		srf2->s_size[1] = srf2->u_knots.k_size - 
			srf2->order[0];

		srf2->ctl_points = (fastf_t *)
			rt_malloc( sizeof(fastf_t) * srf2->s_size[0] *
				srf2->s_size[1] * 
				RT_NURB_EXTRACT_COORDS( srf2->pt_type),
				"rt_nurb_s_xsplit: srf2 row mesh control points");

		for( i = 0; i < srf->s_size[0]; i++)
		{
			fastf_t * old_mesh_ptr;
			fastf_t * new_mesh_ptr;

			old_mesh_ptr = &srf->ctl_points[
				i * srf->s_size[1] * 
				RT_NURB_EXTRACT_COORDS( srf->pt_type)];
			new_mesh_ptr = &srf1->ctl_points[
				i * srf1->s_size[1] * 
				RT_NURB_EXTRACT_COORDS( srf1->pt_type)];
			rt_nurb_map_oslo( oslo, old_mesh_ptr, new_mesh_ptr,
				RT_NURB_EXTRACT_COORDS( srf->pt_type ),
				RT_NURB_EXTRACT_COORDS( srf1->pt_type ),
				0, k_index, srf1->pt_type);
			new_mesh_ptr = &srf2->ctl_points[
				i * srf2->s_size[1] * 
				RT_NURB_EXTRACT_COORDS( srf2->pt_type)];
			rt_nurb_map_oslo( oslo, old_mesh_ptr, new_mesh_ptr,
				RT_NURB_EXTRACT_COORDS( srf->pt_type ),
				RT_NURB_EXTRACT_COORDS( srf2->pt_type ),
				k_index, new_kv.k_size - srf2->order[0], 
				srf2->pt_type);
		}
	}
	else 
	{
		rt_nurb_kvmult(&new_kv, &srf->v_knots, srf->order[RT_NURB_SPLIT_COL], param);

		k_index = srf->order[1];

		oslo = ( struct oslo_mat *) 
			rt_nurb_calc_oslo( srf->order[RT_NURB_SPLIT_COL], &srf->v_knots, &new_kv);

		srf1 = (struct snurb *) rt_malloc( sizeof( struct snurb),
			"rt_nurb_s_xsplit: row snurb struct");
		srf1->order[0]  = srf->order[0];
		srf1->order[1]  = srf->order[1];
		srf1->dir = RT_NURB_SPLIT_COL;
		rt_nurb_kvextract(&srf1->v_knots, &new_kv, 0, k_index + srf1->order[RT_NURB_SPLIT_COL]);
		rt_nurb_kvcopy(&srf1->u_knots, &srf->u_knots);
		
		srf1->pt_type = srf->pt_type;
		srf1->s_size[0] = srf1->v_knots.k_size - 
			srf1->order[1];
		srf1->s_size[1] = srf1->u_knots.k_size - 
			srf1->order[0];

		srf1->ctl_points = (fastf_t *)
			rt_malloc( sizeof(fastf_t) * srf1->s_size[0] *
				srf1->s_size[1] * 
				RT_NURB_EXTRACT_COORDS( srf1->pt_type),
				"rt_nurb_split: srf1 row mesh control points");

		srf2 = (struct snurb *) rt_malloc( sizeof( struct snurb),
			"rt_nurb_split: row snurb struct");
		srf2->order[0]  = srf->order[0];
		srf2->order[1]  = srf->order[1];
		srf2->dir = RT_NURB_SPLIT_COL;
		rt_nurb_kvextract(&srf2->v_knots, &new_kv, k_index, new_kv.k_size);
		rt_nurb_kvcopy(&srf2->u_knots, &srf->u_knots);
		
		srf2->pt_type = srf->pt_type;
		srf2->s_size[0] = srf2->v_knots.k_size - 
			srf2->order[1];
		srf2->s_size[1] = srf2->u_knots.k_size - 
			srf2->order[0];

		srf2->ctl_points = (fastf_t *)
			rt_malloc( sizeof(fastf_t) * srf2->s_size[0] *
				srf2->s_size[1] * 
				RT_NURB_EXTRACT_COORDS( srf2->pt_type),
				"rt_nurb_s_xsplit: srf2 row mesh control points");

		for( i = 0; i < srf->s_size[1]; i++)
		{
			fastf_t * old_mesh_ptr;
			fastf_t * new_mesh_ptr;

			old_mesh_ptr = &srf->ctl_points[
				i * RT_NURB_EXTRACT_COORDS( srf->pt_type)];
			new_mesh_ptr = &srf1->ctl_points[
				i * RT_NURB_EXTRACT_COORDS( srf1->pt_type)];
			rt_nurb_map_oslo( oslo, old_mesh_ptr, new_mesh_ptr,
				srf->s_size[1] *
				RT_NURB_EXTRACT_COORDS( srf->pt_type ),
				srf1->s_size[1] * 
				RT_NURB_EXTRACT_COORDS( srf1->pt_type ),
				0, k_index, srf1->pt_type);
			new_mesh_ptr = &srf2->ctl_points[
				i * RT_NURB_EXTRACT_COORDS( srf2->pt_type)];
			rt_nurb_map_oslo( oslo, old_mesh_ptr, new_mesh_ptr,
				srf->s_size[1] *
				RT_NURB_EXTRACT_COORDS( srf->pt_type ),
				srf2->s_size[1] *
				RT_NURB_EXTRACT_COORDS( srf2->pt_type ),
				k_index, new_kv.k_size - srf2->order[1], 
				srf2->pt_type);
		}
	}
	
	srf1->next = srf2;

	rt_free( (char *) new_kv.knots, "rt_nurb_s_xsplit: new_kv.knots");

	rt_nurb_free_oslo(oslo);

	return (struct snurb *) srf1;
}

/* rt_nurb_c_xsplit()
 * Split a NURB curve by inserting a multiple knot and return
 * the result of the two curves.
 *
 * Algorithm
 *
 * Insert a multiple knot of the curve order. A parameter is give for the
 * knot value for which the curve will be split.
 */

struct cnurb *
rt_nurb_c_xsplit( crv, param)
struct cnurb * crv;
fastf_t param;
{
	struct knot_vector new_kv;
	fastf_t value;
	struct oslo_mat * oslo;
	int i;
	int k_index;
	struct cnurb * crv1, * crv2;
	int coords;

	coords = RT_NURB_EXTRACT_COORDS( crv->pt_type ),

	k_index = crv->order;
	rt_nurb_kvmult(&new_kv, &crv->knot, crv->order, param);

	oslo = ( struct oslo_mat *) 
		rt_nurb_calc_oslo( crv->order, &crv->knot, &new_kv);

	crv1 = (struct cnurb *) rt_malloc( sizeof( struct cnurb),
		"rt_nurb_c_xsplit: cnurb struct");

	crv1->next = (struct cnurb *)0;
	crv1->order  = crv->order;
	rt_nurb_kvextract(&crv1->knot, &new_kv, 0, k_index + crv->order);
	crv1->pt_type = crv->pt_type;
	crv1->c_size = crv1->knot.k_size - crv1->order;
	crv1->ctl_points = (fastf_t *)
		rt_malloc( sizeof(fastf_t) * crv1->c_size *
			RT_NURB_EXTRACT_COORDS( crv1->pt_type),
			"rt_nurb_c_xsplit: crv1 control points");

	crv2 = (struct cnurb *) rt_malloc( sizeof( struct cnurb),
		"rt_nurb_c_xsplit: crv2 cnurb struct");

	crv2->next = (struct cnurb *)0;
	crv2->order  = crv->order;
	rt_nurb_kvextract(&crv2->knot, &new_kv, k_index, new_kv.k_size);
	crv2->pt_type = crv->pt_type;
	crv2->c_size = crv2->knot.k_size - crv2->order;
	crv2->ctl_points = (fastf_t *)
		rt_malloc( sizeof(fastf_t) * crv2->c_size *
			RT_NURB_EXTRACT_COORDS( crv2->pt_type),
			"rt_nurb_c_xsplit: crv2 row mesh control points");

	rt_nurb_map_oslo( oslo, crv->ctl_points, crv1->ctl_points,
		coords, coords, 0, k_index, crv->pt_type );

	rt_nurb_map_oslo( oslo, crv->ctl_points, crv2->ctl_points,
		coords, coords, k_index, new_kv.k_size - crv2->order, 
		crv2->pt_type );

	rt_nurb_free_oslo( oslo );

	rt_free( (char *) new_kv.knots, "rt_nurb_c_xsplit: new_kv.knots");

	crv1->next = crv2;
	return (struct cnurb *) crv1;
}
