/*	N U R B  _ C 2 . C
 *
 *  Function -
 *	Given parametric u,v values, return the curvature of the
 *	surface.
 *
 *  Author -
 *	Paul Randal Stay
 * 
 *  Source -
 * 	SECAD/VLD Computing Consortium, Bldg 394
 *	The U.S. Army Ballistic Research Laboratory
 * 	Aberdeen Proving Ground, Maryland 21005
 *
 *  Copyright Notice -
 *	This software is Copyright (C) 1990 by the United States Army.
 *	All rights reserved.
 */

#include <stdio.h>
#include <math.h>
#include "machine.h"
#include "vmath.h"
#include "nurb.h"
#include "raytrace.h"

struct curvature *
rt_nurb_curvature(srf, u, v)
struct snurb * srf;
fastf_t u;
fastf_t v;
{

	struct curvature *cvp;
	struct snurb * us, *vs, * uus, * vvs, *uvs;
	fastf_t *ue, *ve, *uue, *vve, *uve, *se;
        fastf_t         E, F, G;                /* First Fundamental Form */
        fastf_t         L, M, N;                /* Second Fundamental form */
        fastf_t         denom;
        fastf_t         wein[4];                /*Weingarten matrix */
        fastf_t         evec[3];
        fastf_t         mean, gauss, discrim;
        vect_t          norm;
	int 		i;

	cvp = (struct curvature *) rt_malloc(sizeof( struct curvature),
		"rt_nurb_curvature: struct curvature");

	us = (struct snurb *) rt_nurb_s_diff(srf, RT_NURB_SPLIT_ROW);
	vs = (struct snurb *) rt_nurb_s_diff(srf, RT_NURB_SPLIT_COL);
	uus = (struct snurb *) rt_nurb_s_diff(us, RT_NURB_SPLIT_ROW);
	vvs = (struct snurb *) rt_nurb_s_diff(vs, RT_NURB_SPLIT_COL);
	uvs = (struct snurb *) rt_nurb_s_diff(vs, RT_NURB_SPLIT_ROW);
	
	se = (fastf_t *) rt_nurb_s_eval(srf, u, v);
	ue = (fastf_t *) rt_nurb_s_eval(us, u,v);
	ve = (fastf_t *) rt_nurb_s_eval(vs, u,v);
	uue = (fastf_t *) rt_nurb_s_eval(uus, u,v);
	vve = (fastf_t *) rt_nurb_s_eval(vvs, u,v);
	uve = (fastf_t *) rt_nurb_s_eval(uvs, u,v);

	rt_nurb_free_snurb( us);
	rt_nurb_free_snurb( vs);
	rt_nurb_free_snurb( uus);
	rt_nurb_free_snurb( vvs);
	rt_nurb_free_snurb( uvs);

	if( RT_NURB_IS_PT_RATIONAL( srf->mesh->pt_type ))
	{
		for( i = 0; i < 3; i++)
		{
			ue[i] = (1.0 / se[3] * ue[i]) -
				(ue[3]/se[3]) * se[0]/se[3];
			ve[i] = (1.0 / se[3] * ve[i]) -
				(ve[3]/se[3]) * se[0]/se[3];
		}
		VCROSS(norm, ue, ve);
		VUNITIZE(norm);
		E = VDOT( ue, ue);
		F = VDOT( ue, ve);
		G = VDOT( ve, ve);
		
		for( i = 0; i < 3; i++)
		{
			uue[i] = (1.0 / se[3] * uue[i]) -
				2 * (uue[3]/se[3]) * uue[i] -
				uue[3]/se[3] * (se[i]/se[3]);

			vve[i] = (1.0 / se[3] * vve[i]) -
				2 * (vve[3]/se[3]) * vve[i] -
				vve[3]/se[3] * (se[i]/se[3]);

			 uve[i] = 1.0 / se[3] * uve[i] +
	                        (-1.0 / (se[3] * se[3])) *
        	                (ve[3] * ue[i] + ue[3] * ve[i] +
                	         uve[3] * se[i]) + 
				(-2.0 / (se[3] * se[3] * se[3])) *
	                        (ve[3] * ue[3] * se[i]);
		}

		L = VDOT( norm, uue);
		M = VDOT( norm, uve);
		N = VDOT( norm, vve);
		
	} else
	{
		VCROSS( norm, ue, ve);
		VUNITIZE( norm );
		E = VDOT( ue, ue);
		F = VDOT( ue, ve);
		G = VDOT( ve, ve);
		
		L = VDOT( norm, uue);
		M = VDOT( norm, uve);
		N = VDOT( norm, vve);
	}

	if( srf->order[0] <= 2 && srf->order[1] <= 2)
	{
		cvp->crv_c1 = cvp->crv_c2 = 0;
		vec_ortho(cvp->crv_pdir, norm);
		goto cleanup;
	}

	denom = ( (E*G) - (F*F) );
	gauss = (L * N - M *M)/denom;
	mean = (G * L + E * N - 2 * F * M) / (2 * denom);
	discrim = sqrt( mean * mean - gauss);
	
	cvp->crv_c1 = mean - discrim;
	cvp->crv_c2 = mean + discrim;

	if( APX_EQ( ( E*G - F*F), 0.0 ))
	{
		rt_log("first fundamental form is singular E = %g F= %g G = %g\n",
			E,F,G);
		goto cleanup;
	}

        wein[0] = ( (G * L) - (F * M))/ (denom);
        wein[1] = ( (G * M) - (F * N))/ (denom);
        wein[2] = ( (E * M) - (F * L))/ (denom);
        wein[3] = ( (E * N) - (F * M))/ (denom);


        if ( APX_EQ( wein[1] , 0.0 ) && APX_EQ( wein[3] - cvp->crv_c1, 0.0) )
        {
                evec[0] = 0.0; evec[1] = 1.0;
        } else
        {
                evec[0] = 1.0;
                if( fabs( wein[1] ) > fabs( wein[3] - cvp->crv_c1) )
                {
                        evec[1] = (cvp->crv_c1 - wein[0]) / wein[1];
                } else
                {
                        evec[1] = wein[2] / ( cvp->crv_c1 - wein[3] );
                }
        }

        VSET( cvp->crv_pdir, 0.0, 0.0, 0.0 );
	cvp->crv_pdir[0] = evec[0] * ue[0] + evec[1] * ve[0];
        cvp->crv_pdir[1] = evec[0] * ue[1] + evec[1] * ve[1];
        cvp->crv_pdir[2] = evec[0] * ue[2] + evec[1] * ve[2];
	VUNITIZE( cvp->crv_pdir);

cleanup:
	rt_free( (char *) se, "rt_nurb_curv:se");
	rt_free( (char *) ue, "rt_nurb_curv:ue");
	rt_free( (char *) ve, "rt_nurb_curv:ve");
	rt_free( (char *) uue, "rt_nurb_curv:uue");
	rt_free( (char *) vve, "rt_nurb_curv:vve");
	rt_free( (char *) uve, "rt_nurb_curv:uve");

	return (struct curvature *) cvp;
}
