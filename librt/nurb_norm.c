/*		N U R B _ N O R M . C
 *
 *  Function-
 *  	Calulate and return the normal of a surface given the
 *	U,V parametric values.
 *
 *  Author -
 *	Paul R. Stay
 *
 *  Source -
 *     SECAD/VLD Computing Consortium, Bldg 394
 *     The U.S. Army Ballistic Research Laboratory
 *     Aberdeen Proving Ground, Maryland 21005
 *
 * Copyright Notice -
 *     This software is Copyright (C) 1991 by the United States Army.
 *     All rights reserved.
 */

#include <stdio.h>
#include "machine.h"
#include "vmath.h"
#include "nurb.h"

/* If the order of the surface is linear either direction
 * than approximate it.
 */

fastf_t *
rt_nurb_s_norm(srf, u, v)
struct snurb * srf;
fastf_t u, v;
{
	struct snurb *usrf, *vsrf;
	fastf_t * se, * ue, *ve;
	point_t uvec, vvec;
	fastf_t * norm;
	fastf_t p;
	int i;

	/* Case (linear, lienar) find the normal from the polygon */
	if( srf->order[0] == 2 && srf->order[1] == 2 )
	{
		/* Find the correct span to get the normal */
		se = (fastf_t *) rt_nurb_s_eval( srf, u, v);
		
		p = 0.0;
		for( i = 0; i < srf->u_knots->k_size -1; i++)
		{
			if ( srf->u_knots->knots[i] <= u 
				&& u < srf->u_knots->knots[i+1] )
			{
				p = srf->u_knots->knots[i];

				if (u == p)
					p = srf->u_knots->knots[i+1];
				if ( u == p && i > 1)
					p = srf->u_knots->knots[i-1];
			}
		}

		ue = (fastf_t *)rt_nurb_s_eval( srf, p, v);

		p = 0.0;
		for( i = 0; i < srf->v_knots->k_size -1; i++)
		{
			if ( srf->v_knots->knots[i] < u 
				&& srf->v_knots->knots[i+1] )
			{
				p = srf->v_knots->knots[i];
				if (v == p)
					p = srf->v_knots->knots[i+1];
				if ( v == p && i > 1)
					p = srf->v_knots->knots[i-1];
			}
		}

		ve = (fastf_t *) rt_nurb_s_eval( srf, u, p);		
	
		if( EXTRACT_RAT(srf->mesh->pt_type))
		{
			ue[0] = ue[0] / ue[3];
			ue[1] = ue[1] / ue[3];
			ue[2] = ue[2] / ue[3];
			ue[3] = ue[3] / ue[3];

			ve[0] = ve[0] / ve[3];
			ve[1] = ve[1] / ve[3];
			ve[2] = ve[2] / ve[3];
			ve[3] = ve[3] / ve[3];

		}

		VSUB2(uvec, se, ue);
		VSUB2(vvec, se, ve);

		norm = (fastf_t *) rt_malloc( 3 * sizeof( fastf_t ),
				"normal");
		VCROSS( norm, uvec, vvec);
		VUNITIZE( norm );

		rt_free( (char *) se, "rt_nurb_norm: se");
		rt_free( (char *) ue, "rt_nurb_norm: ue");
		rt_free( (char *) ve, "rt_nurb_norm: ve");

		return norm;		

	}
	/* Case (linear, > linear) Use the linear direction to approximate
	 * the tangent to the surface 
	 */
	if( srf->order[0] == 2 && srf->order[1] > 2 )
	{
		se = (fastf_t *)rt_nurb_s_eval( srf, u, v);
		
		norm = (fastf_t *) rt_malloc( 3 * sizeof(fastf_t),"normal");
		p = 0.0;
		for( i = 0; i < srf->u_knots->k_size -1; i++)
		{
			if ( srf->u_knots->knots[i] <= u 
				&& u < srf->u_knots->knots[i+1] )
			{
				p = srf->u_knots->knots[i];

				if (u == p)
					p = srf->u_knots->knots[i+1];
				if ( u == p && i > 1)
					p = srf->u_knots->knots[i-1];
			}
		}

		ue = (fastf_t *) rt_nurb_s_eval( srf, p, v);
		
		vsrf = (struct snurb *) rt_nurb_s_diff(srf, COL);

		ve = (fastf_t *) rt_nurb_s_eval(vsrf, u, v);

		if( EXTRACT_RAT(srf->mesh->pt_type) )
		{
			fastf_t w, inv_w;
			
			w = se[3];
			inv_w = 1.0 / w;

			ve[0] = (inv_w * ve[0]) -
				ve[3] / (w * w) * se[0];
			ve[1] = (inv_w * ve[1]) -
				ve[3] / (w * w) * se[1];
			ve[2] = (inv_w * ve[2]) -
				ve[3] / (w * w) * se[2];

			ue[0] = ue[0] / ue[3];
			ue[1] = ue[1] / ue[3];
			ue[2] = ue[2] / ue[3];
			ue[3] = ue[3] / ue[3];

			se[0] = se[0] / se[3];
			se[1] = se[1] / se[3];
			se[2] = se[2] / se[3];
			se[3] = se[3] / se[3];
		}
		
		VSUB2(uvec, se, ue);
		
		VCROSS(norm, uvec, ve);
		VUNITIZE(norm);

		rt_free((char *) se, "se");
		rt_free((char *) ue, "ue");
		rt_free((char *) ve, "ve");
		rt_nurb_free_snurb(vsrf);
		return norm;
	}
	if( srf->order[1] == 2 && srf->order[0] > 2 )
	{
		se = (fastf_t *) rt_nurb_s_eval( srf, u, v);
		
		norm = (fastf_t *) rt_malloc( 3 * sizeof(fastf_t),"normal");
		p = 0.0;
		for( i = 0; i < srf->v_knots->k_size -1; i++)
		{
			if ( srf->v_knots->knots[i] <= v 
				&& v < srf->v_knots->knots[i+1] )
			{
				p = srf->v_knots->knots[i];

				if (v == p)
					p = srf->u_knots->knots[i+1];
				if ( v == p && i > 1)
					p = srf->v_knots->knots[i-1];
			}
		}

		ve = (fastf_t *) rt_nurb_s_eval( srf, p, v);

		usrf = (struct snurb *) rt_nurb_s_diff(srf, ROW);
		ue = (fastf_t *) rt_nurb_s_eval(usrf, u, v);

		if( EXTRACT_RAT(srf->mesh->pt_type) )
		{
			fastf_t w, inv_w;
			
			w = se[3];
			inv_w = 1.0 / w;

			ue[0] = (inv_w * ue[0]) -
				ue[3] / (w * w) * se[0];
			ue[1] = (inv_w * ue[1]) -
				ue[3] / (w * w) * se[1];
			ue[2] = (inv_w * ue[2]) -
				ue[3] / (w * w) * se[2];

			ve[0] = ve[0] / ve[3];
			ve[1] = ve[1] / ve[3];
			ve[2] = ve[2] / ve[3];
			ve[3] = ve[3] / ve[3];

			se[0] = se[0] / se[3];
			se[1] = se[1] / se[3];
			se[2] = se[2] / se[3];
			se[3] = se[3] / se[3];
		}

		VSUB2(vvec, se, ve);
		
		VCROSS(norm, ue, vvec);
		VUNITIZE(norm);		

		rt_free((char *) se, "se");
		rt_free((char *) ve, "ve");
		rt_free((char *) ue, "ue");
		rt_nurb_free_snurb(usrf);
		return norm;
	}
	
	/* Case Non Rational (order > 2, order > 2) */
	if( !EXTRACT_RAT(srf->mesh->pt_type))
	{

		norm = (fastf_t *) rt_malloc( sizeof( fastf_t) * 3, 
			"rt_nurb_norm: fastf_t for norm");

		usrf = (struct snurb *) rt_nurb_s_diff( srf, ROW);
		vsrf = (struct snurb *) rt_nurb_s_diff( srf, COL);

		ue = (fastf_t *) rt_nurb_s_eval(usrf, u,v);
		ve = (fastf_t *) rt_nurb_s_eval(vsrf, u,v);
		
		VCROSS( norm, ue, ve);
		VUNITIZE( norm);

		rt_free( (char *) ue, "rt_nurb_norm: ue");
		rt_free( (char *) ve, "rt_nurb_norm: ve");
		rt_nurb_free_snurb(usrf);
		rt_nurb_free_snurb(vsrf);
		
		return norm;
	}

	/* Case Rational (order > 2, order > 2) */
	if( EXTRACT_RAT(srf->mesh->pt_type))
	{
		fastf_t w, inv_w;
		vect_t unorm, vnorm;
		int i;

		norm = (fastf_t *) rt_malloc( sizeof( fastf_t) * 3, 
			"rt_nurb_norm: fastf_t for norm");

		se = (fastf_t *) rt_nurb_s_eval(srf, u, v);

		usrf = (struct snurb *) rt_nurb_s_diff( srf, ROW);
		vsrf = (struct snurb *) rt_nurb_s_diff( srf, COL);

		ue = (fastf_t *) rt_nurb_s_eval(usrf, u,v);

		ve = (fastf_t *) rt_nurb_s_eval(vsrf, u,v);
		
		w = se[3];
		inv_w = 1.0 / w;

		for(i = 0; i < 3; i++)
		{
			unorm[i] = (inv_w * ue[i]) -
				ue[3] / (w*w) * se[i];
			vnorm[i] = (inv_w * ve[i]) -
				ve[3] / (w*w) * se[i];
		}

		VCROSS( norm, unorm, vnorm);
		VUNITIZE( norm);

		rt_free( (char *) se, "rt_nurb_norm: se");
		rt_free( (char *) ue, "rt_nurb_norm: ue");
		rt_free( (char *) ve, "rt_nurb_norm: ve");
		rt_nurb_free_snurb(usrf);
		rt_nurb_free_snurb(vsrf);
		
		return norm;
	}
}

