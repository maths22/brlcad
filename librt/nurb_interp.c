/* 	N U R B  _ C I N T E R P . C
 *
 * nurb_interp.c - Interpolatopn routines for fitting NURB curves to
 *				existing data.
 *			
 *
 * Author:  Paul R. Stay
 * Source
 * 	VLD  Bldg 247
 * 	The US Army Ballistic Research Laboratory
 * 	Aberdeen Proving Ground, Maryland 21005
 * 
 * Date: Fri. May 14, 1993
 * 
 * Copyright Notice - 
 * 	This software is Copyright (C) 1992 by the United States Army.
 * 	All rights reserved.
 * 
 */

#include "conf.h"

#include <stdio.h>

#include "machine.h"
#include "vmath.h"
#include "raytrace.h"
#include "nurb.h"


/* main routine for interpolation for curves */

rt_nurb_cinterp( crv, order, data, n)
struct cnurb * crv;
fastf_t * data;
int n;
{

	fastf_t * interp_mat;
	fastf_t * nodes;
	struct knot_vector * kv;
	int i,j;

	/* Create Data memory and fill in curve structs */

	interp_mat = (fastf_t *) rt_malloc( n * n * sizeof(fastf_t),
		"rt_nurb_interp: interp_mat");

	nodes = (fastf_t *) rt_malloc( n * sizeof(fastf_t),"rt_nurb_interp:nodes");

	crv->ctl_points = (fastf_t *) rt_malloc( n * 3 * sizeof(fastf_t),
		"solution");

	crv->order = order;
	crv->c_size = n;
	crv->pt_type = RT_NURB_MAKE_PT_TYPE( 3, RT_NURB_PT_XYZ, 0);

	/* First set up Curve data structs */

	kv = &crv->knot;

	/* For now we will assume that all paramerizations are uniform */

	rt_nurb_kvknot( kv, order, 0.0, 1.0, (n - order));
	
	/* Calcualte Nodes at which the data points will be
	 * evaluated in the curve
	 */

	rt_nurb_nodes( nodes, kv, order);

	/* use the node values to create the interpolation matrix
    	 * which is a diagonal matrix
	 */
	
	rt_nurb_interp_mat( interp_mat, kv, nodes, order, n);

	/* Solve the system of equations to get the control points */

	rt_nurb_solve( interp_mat, data, crv->ctl_points, n, 3);

	/* Free up node and interp_mat storage */	

	rt_free( (char *) interp_mat, "rt_nurb_cinterp: interp_mat");
	rt_free( (char *) nodes, "rt_nurb_cinterp: nodes");

	/* All done, The resulting crv now interpolates the data */
}

rt_nurb_nodes( nodes, knots, order)
fastf_t * nodes;
struct knot_vector * knots;
int order;
{
	int i, j;
	fastf_t sum;

	for( i = 0; i < knots->k_size -order; i++)
	{

		sum = 0.0;
		
		for( j = 1; j <= order -1; j++)
		{
			sum += knots->knots[i+j];
		}
		nodes[i] = sum/(order -1);
	}
}

rt_nurb_interp_mat( imat, knots, nodes, order, dim)
fastf_t * imat;
struct knot_vector * knots;
fastf_t * nodes;
int order;
int dim;
{
	int i,j;
	int ptr;
	
	ptr = 0;

	for( i = 0; i < dim; i++)
	for( j = 0; j < dim; j++)
	{
		imat[ptr] = rt_nurb_basis_eval( knots, j, order, nodes[i]);
		ptr++;
	}

	imat[ptr-1] = 1.0;	
}
