/*
 *  Authors -
 *	John R. Anderson
 *
 *  Source -
 *	SLAD/BVLD/VMB
 *	The U. S. Army Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1994 by the United States Army
 *	in all countries except the USA. All rights reserved.
 */

/*	This routine loops through all the directory entries and calls
	appropriate routines to convert trimmed surface entities to BRLCAD
	NMG TNURBS	*/

#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include "conf.h"

#include <stdio.h>
#include "machine.h"
#include "vmath.h"
#include "nmg.h"
#include "raytrace.h"
#include "nurb.h"
#include "./iges_struct.h"
#include "./iges_extern.h"
#include "wdb.h"

struct nurb_verts
{
	point_t pt;
	vect_t uvw;
	struct vertex *v;
};

void
set_edge_vertices_m( fu , nverts , nurb_v , vert_count , entityno )
struct faceuse *fu;
int nverts;
struct nurb_verts *nurb_v;
int *vert_count;
int entityno;
{
	int entity_type;
	int i;
	double x,y,z;
	point_t pt;
printf( "Setting edge vertices in model space: vert_count=%d\n" , *vert_count );

	if( dir[entityno]->param <= pstart )
	{
		printf( "Illegal parameter pointer for entity D%07d (%s)\n" ,
				dir[entityno]->direct , dir[entityno]->name );
		return;
	}

	if( *vert_count == nverts-1 )
		return;

	Readrec( dir[entityno]->param );
	Readint( &entity_type , "" );
	switch( entity_type )
	{
		case 102:
			{
				int curve_count;
				int *curve_list;

				Readint( &curve_count , "Number of curves: " );
				curve_list = (int *)rt_calloc( curve_count , sizeof( int ) , "set_edge_vertices_m: curve_list" );
				for( i=0 ; i<curve_count ; i++ )
					Readint( &curve_list[i] , "\tCurve DE: " );
				for( i=0 ; i<curve_count ; i++ )
					set_edge_vertices_m( fu , nverts , nurb_v , vert_count , (curve_list[i]-1)/2 );
				rt_free( (char *)curve_list , "set_edge_vertices_m: curve_list" );
			}
			break;
		case 110:
			Readdbl( &x , "X1: " );
			Readdbl( &y , "Y1: " );
			Readdbl( &z , "Z1: " );
			if( *vert_count == 0 )
			{
				VSET( pt , x , y , z );
				MAT4X3PNT( nurb_v[0].pt , *dir[entityno]->rot , pt );
				nmg_vertex_gv( nurb_v[0].v , nurb_v[0].pt );
			}
			Readdbl( &x , "X2: " );
			Readdbl( &y , "Y2: " );
			Readdbl( &z , "Z2: " );
			(*vert_count)++;
			VSET( pt , x , y , z )
			MAT4X3PNT( nurb_v[*vert_count].pt , *dir[entityno]->rot , pt );
			nmg_vertex_gv( nurb_v[*vert_count].v , nurb_v[*vert_count].pt );
			break;
		case 100:
			Readdbl( &z , "Z: " );
			Readdbl( &x , "X arc center: " );
			Readdbl( &y , "Y arc center: " );
			Readdbl( &x , "X1: " );
			Readdbl( &y , "Y1: " );
			if( *vert_count == 0 )
			{
				VSET( pt , x , y , z );
				MAT4X3PNT( nurb_v[0].pt , *dir[entityno]->rot , pt );
				nmg_vertex_gv( nurb_v[0].v , nurb_v[0].pt );
			}
			Readdbl( &x , "X2: " );
			Readdbl( &y , "Y2: " );
			(*vert_count)++;
			VSET( pt , x , y , z );
			MAT4X3PNT( nurb_v[*vert_count].pt , *dir[entityno]->rot , pt );
			nmg_vertex_gv( nurb_v[*vert_count].v , nurb_v[*vert_count].pt );
			break;
		case 126:
			{
				int num_pts;
				int degree;
				int j;
				double a;

				Readint( &i , "Upper index for sum: " );
				num_pts = i+1;
				Readint( &degree , "Degree: " );

				/* skip properties */
				for( i=0 ; i<4 ; i++ )
					Readint( &j , "" );

				/* skip knot vector */
				for( i=0 ; i<num_pts+degree ; i++ )
					Readdbl( &a , "" );

				/* skip weights */
				for( i=0 ; i<num_pts ; i++ )
					Readdbl( &a , "" );

				/* get first vertex */
				Readdbl( &x , "X1: " );
				Readdbl( &y , "Y1: " );
				Readdbl( &z , "Z1: " );
				if( *vert_count == 0 )
				{
					VSET( pt , x , y , z );
					MAT4X3PNT( nurb_v[0].pt , *dir[entityno]->rot , pt );
					nmg_vertex_gv( nurb_v[0].v , nurb_v[0].pt );
				}

				/* skip to last vertex */
				for( i=1 ; i<num_pts-1 ; i++ )
				{
					Readdbl( &x , "XMIDDLE: " );
					Readdbl( &y , "YMIDDLE: " );
					Readdbl( &z , "ZMIDDLE: " );
				}

				/* get last vertex */
				Readdbl( &x , "X2: " );
				Readdbl( &y , "Y2: " );
				Readdbl( &z , "Z2: " );
				(*vert_count)++;
				VSET( pt , x , y , z );
				MAT4X3PNT( nurb_v[*vert_count].pt , *dir[entityno]->rot , pt );
				nmg_vertex_gv( nurb_v[*vert_count].v , nurb_v[*vert_count].pt );
			}
			break;
	}
}

void
set_edge_vertices_p( fu , nverts , nurb_v , vert_count , entityno )
struct faceuse *fu;
int nverts;
struct nurb_verts *nurb_v;
int *vert_count;
int entityno;
{
	int entity_type;
	int i;
	double x,y,z;
	struct vertexuse *vu;
	struct edgeuse *eu;
	struct loopuse *lu;
	point_t pt;
printf( "Setting edge vertices in parameter space: (vert_count=%d)\n" , *vert_count );

	if( dir[entityno]->param <= pstart )
	{
		printf( "Illegal parameter pointer for entity D%07d (%s)\n" ,
				dir[entityno]->direct , dir[entityno]->name );
		return;
	}

	if( *vert_count == nverts-1 )
		return;

	Readrec( dir[entityno]->param );
	Readint( &entity_type , "" );
	switch( entity_type )
	{
		case 102:
			{
				int curve_count;
				int *curve_list;

				Readint( &curve_count , "Number of curves: " );
				curve_list = (int *)rt_calloc( curve_count , sizeof( int ) , "set_edge_vertices_m: curve_list" );
				for( i=0 ; i<curve_count ; i++ )
					Readint( &curve_list[i] , "\tCurve DE: " );
				for( i=0 ; i<curve_count ; i++ )
					set_edge_vertices_p( fu , nverts , nurb_v , vert_count , (curve_list[i]-1)/2 );
				rt_free( (char *)curve_list , "set_edge_vertices_m: curve_list" );
			}
			break;
		case 110:
			Readdbl( &x , "U1: " );
			Readdbl( &y , "V1: " );
			Readdbl( &z , "W1: " );
			if( *vert_count == 0 )
			{
				VSET( pt , x , y , z );
				MAT4X3PNT( nurb_v[0].uvw , *dir[entityno]->rot , pt );
				for( RT_LIST_FOR( vu , vertexuse , &nurb_v[0].v->vu_hd ) )
					nmg_vertexuse_a_cnurb( vu , nurb_v[0].uvw );
			}
			Readdbl( &x , "U2: " );
			Readdbl( &y , "V2: " );
			Readdbl( &z , "W2: " );
			(*vert_count)++;
			VSET( pt , x , y , z );
			MAT4X3PNT( nurb_v[*vert_count].uvw , *dir[entityno]->rot , pt );
			for( RT_LIST_FOR( vu , vertexuse , &nurb_v[*vert_count].v->vu_hd ) )
				nmg_vertexuse_a_cnurb( vu , nurb_v[*vert_count].uvw );
			for( RT_LIST_FOR( lu , loopuse , &fu->lu_hd ) )
			{
				for( RT_LIST_FOR( eu , edgeuse , &lu->down_hd ) )
				{
					if( (eu->vu_p->v_p == nurb_v[*vert_count].v &&
					    eu->eumate_p->vu_p->v_p == nurb_v[*vert_count-1].v ) ||
					    (eu->vu_p->v_p == nurb_v[*vert_count-1].v &&
					    eu->eumate_p->vu_p->v_p == nurb_v[*vert_count].v ) )
							nmg_edge_g_cnurb_plinear( eu );
				}
			}
			break;
		case 100:
			{
				fastf_t kv[8];
				fastf_t pts[5*4];
				fastf_t pts1[5*4];
				fastf_t radius;
				fastf_t cx,cy,x1,y1;
				fastf_t ang1,ang2;
				fastf_t fact1=1.048816;
				vect_t tangent;
				int pt_type;

				/* don't use automatic aggregate initialization (Ultrix) */
				kv[0] = 0.0;
				kv[1] = 0.0;
				kv[2] = 0.0;
				kv[3] = 1.0;
				kv[4] = 2.0;
				kv[5] = 3.0;
				kv[6] = 3.0;
				kv[7] = 3.0;

				pt_type = RT_NURB_MAKE_PT_TYPE( 4 , 2 , 1 );

				Readdbl( &z , "W: " );
				Readdbl( &cx , "U arc center: " );
				Readdbl( &cy , "V arc center: " );
				Readdbl( &x1 , "U1: " );
				Readdbl( &y1 , "V1: " );
				if( *vert_count == 0 )
				{
					VSET( pt , x , y , z );
					MAT4X3PNT( nurb_v[0].uvw , *dir[entityno]->rot , pt );
					for( RT_LIST_FOR( vu , vertexuse , &nurb_v[0].v->vu_hd ) )
						nmg_vertexuse_a_cnurb( vu , nurb_v[0].uvw );
				}
				Readdbl( &x , "U2: " );
				Readdbl( &y , "V2: " );
				(*vert_count)++;
				VSET( pt , x , y , z );
				MAT4X3PNT( nurb_v[*vert_count].uvw , *dir[entityno]->rot , pt );
				for( RT_LIST_FOR( vu , vertexuse , &nurb_v[*vert_count].v->vu_hd ) )
					nmg_vertexuse_a_cnurb( vu , nurb_v[*vert_count].uvw );

				radius = sqrt( (x-cx)*(x-cx) + (y-cy)*(y-cy) );
				VSET( &pts[0] , x1 , y1 , z );
				VSET( &pts[4*4] , x , y , z );
				ang1 = atan2( y1-cy , x1-cx );
				ang2 = atan2( y-cy , x-cx );
				while( ang2 <= ang1 )
					ang2 += rt_twopi;
				VSET( &pts[2*4] , cx + fact1*radius*cos( 0.5*(ang1+ang2) ) , cy + fact1*radius*sin( 0.5*(ang1+ang2) ) , 0.0 );
				VSET( tangent , -sin( ang1 ) , cos( ang1 ) , 0.0 );
				VJOIN1( &pts[1*4] , &pts[0] , 0.5*sqrt(2.0)*radius , tangent );
				VSET( tangent , sin( ang2 ) , -cos( ang2 ) , 0.0 );
				VJOIN1( &pts[3*4] , &pts[4*4] , 0.5*sqrt(2.0)*radius , tangent );
				for( i=0 ; i<5 ; i++ )
					pts[i*4+3] = 1.0;
				pts[2*4+3] = 2.0;
				for( i=0 ; i<5 ; i++ )
				{
					pts1[i*4+3] = pts[i*4+3];
					MAT4X3PNT( &pts1[i*4] , *dir[entityno]->rot , &pts[i*4] );
				}
				for( RT_LIST_FOR( lu , loopuse , &fu->lu_hd ) )
				{
					for( RT_LIST_FOR( eu , edgeuse , &lu->down_hd ) )
					{
						if( (eu->vu_p->v_p == nurb_v[*vert_count].v &&
						    eu->eumate_p->vu_p->v_p == nurb_v[*vert_count-1].v ) ||
						    (eu->vu_p->v_p == nurb_v[*vert_count-1].v &&
						    eu->eumate_p->vu_p->v_p == nurb_v[*vert_count].v ) )
								nmg_edge_g_cnurb( eu, 3, 8, kv, 5, pt_type, pts1  );
					}
				}
			}
			break;
		case 126:
			{
				int num_pts;
				int num_knots;
				int degree;
				int rational;
				int coords;
				int pt_type;
				fastf_t *kv;
				fastf_t *points;
				int j;
				double a;

				Readint( &i , "Upper index for sum: " );
				num_pts = i+1;
				Readint( &degree , "Degree: " );

				/* properties */
				Readint( &j , "" );
				Readint( &j , "" );
				Readint( &j , "" );
				rational = !j;
				Readint( &j , "" );

				pt_type = RT_NURB_MAKE_PT_TYPE( 3+rational , 2 , rational );
				coords = RT_NURB_EXTRACT_COORDS( pt_type );

				/* knot vector */
				num_knots = num_pts + degree;
				kv = (fastf_t *)rt_calloc( num_knots , sizeof( fastf_t ) , "set_edge_vertices_p: kv" );
				for( i=0 ; i<num_knots ; i++ )
				{
					Readdbl( &a , "" );
					kv[i] = a;
				}

				points = (fastf_t *)rt_calloc( num_pts*coords , sizeof( fastf_t ) , "set_edge_vertices_p: points" );
				/* weights */
				for( i=0 ; i<num_pts ; i++ )
				{
					Readdbl( &a , "" );
					if( rational )
						points[i*coords+3] = a;
				}

				/* get first vertex */
				Readdbl( &x , "U1: " );
				Readdbl( &y , "V1: " );
				Readdbl( &z , "W1: " );
				VSET( pt , x , y , z );
				MAT4X3PNT( &points[0] , *dir[entityno]->rot , pt );
				if( *vert_count == 0 )
				{
					VMOVE( nurb_v[0].uvw , &points[0] )
					for( RT_LIST_FOR( vu , vertexuse , &nurb_v[0].v->vu_hd ) )
						nmg_vertexuse_a_cnurb( vu , nurb_v[0].uvw );
				}

				/* middle vertices */
				for( i=1 ; i<num_pts-1 ; i++ )
				{
					Readdbl( &x , "UMIDDLE: " );
					Readdbl( &y , "VMIDDLE: " );
					Readdbl( &z , "WMIDDLE: " );
					VSET( pt , x , y , z );
					MAT4X3PNT( &points[i*coords] , *dir[entityno]->rot , pt );
					VSET( &points[i*coords] , x , y , z );
				}

				/* get last vertex */
				Readdbl( &x , "U2: " );
				Readdbl( &y , "V2: " );
				Readdbl( &z , "W2: " );
				VSET( pt , x , y , z );
				MAT4X3PNT( &points[num_pts-1] , *dir[entityno]->rot , pt );
				(*vert_count)++;
				VMOVE( nurb_v[*vert_count].uvw , &points[num_pts-1] );
				for( RT_LIST_FOR( vu , vertexuse , &nurb_v[*vert_count].v->vu_hd ) )
					nmg_vertexuse_a_cnurb( vu , nurb_v[*vert_count].uvw );
				for( RT_LIST_FOR( lu , loopuse , &fu->lu_hd ) )
				{
					for( RT_LIST_FOR( eu , edgeuse , &lu->down_hd ) )
					{
						if( (eu->vu_p->v_p == nurb_v[*vert_count].v &&
						    eu->eumate_p->vu_p->v_p == nurb_v[*vert_count-1].v ) ||
						    (eu->vu_p->v_p == nurb_v[*vert_count-1].v &&
						    eu->eumate_p->vu_p->v_p == nurb_v[*vert_count].v ) )
								nmg_edge_g_cnurb( eu, degree+1, num_knots,
									kv, num_pts, pt_type, points );
					}
				}
			}
			break;
	}
}

void
Set_loop_vertices( fu , nverts , nurb_v , pcurve , mcurve )
struct faceuse *fu;
int nverts;
struct nurb_verts *nurb_v;
int pcurve,mcurve;
{
	int vert_count;

	/* first do model coordinates */
	vert_count = 0;
	set_edge_vertices_m( fu , nverts , nurb_v , &vert_count , mcurve );

	/* now do parametric space */
	vert_count = 0;
	set_edge_vertices_p( fu , nverts , nurb_v , &vert_count , pcurve );
}

void
Set_vertices( fu , nverts , nurb_v , on_surf_de , entityno )
struct faceuse *fu;
int nverts;
struct nurb_verts *nurb_v;
int on_surf_de;
int entityno;
{
	int entity_type;
	int surf_de;
	int param_curve_de,model_curve_de;
	int i;

	/* Acquiring Data */

	if( dir[entityno]->param <= pstart )
	{
		printf( "Illegal parameter pointer for entity D%07d (%s)\n" ,
				dir[entityno]->direct , dir[entityno]->name );
		return;
	}

	Readrec( dir[entityno]->param );
	Readint( &entity_type , "" );
	if( entity_type != 142 )
	{
		printf( "Expected Curve on a Paremetric Surface, found entity type %d\n" , entity_type );
		return;
	}
	Readint( &i , "Curve created by method %d: " );
	Readint( &surf_de , "Curve is on surface %d: " );
	if( surf_de != on_surf_de )
		printf( "Curve is on surface at DE %d, should be on surface at DE %d\n", surf_de, on_surf_de );

	Readint( &param_curve_de , "Parametric Curve DE: " );
	Readint( &model_curve_de , "Model Curve DE: " );

	Set_loop_vertices( fu , nverts , nurb_v , (param_curve_de-1)/2 , (model_curve_de-1)/2 );
}

int
Count_verts( entityno )
int entityno;
{
	int entity_type;
	int vert_count=0;
	int curve_count;
	int *comp_curve_des;
	int i;

	/* Acquiring Data */

	if( dir[entityno]->param <= pstart )
	{
		printf( "Illegal parameter pointer for entity D%07d (%s)\n" ,
				dir[entityno]->direct , dir[entityno]->name );
		return(0);
	}

	Readrec( dir[entityno]->param );
	Readint( &entity_type , "" );

	switch( entity_type )
	{
		case 110:
		case 100:
		case 126:
			vert_count = 1;
			break;
		case 102:
			Readint( &curve_count , "Number of curves on composite: " );
			comp_curve_des = (int *)rt_calloc( curve_count , sizeof( int ) , "Count_verts: comp_curve_des" );
			for( i=0 ; i<curve_count ; i++ )
				Readint( &comp_curve_des[i] , "\tCurve DE: " );

			for( i=0 ; i<curve_count ; i++ )
				vert_count += Count_verts( (comp_curve_des[i]-1)/2 );
			break;
	}

	return( vert_count );
}

int
Get_curve_verts( entityno, on_surf_de , nurb_v )
int entityno;
struct nurb_verts **nurb_v;
{
	int entity_type;
	int vert_count=0;
	int i;
	int surf_de,param_curve_de,model_curve_de;

	/* Acquiring Data */

	if( dir[entityno]->param <= pstart )
	{
		printf( "Illegal parameter pointer for entity D%07d (%s)\n" ,
				dir[entityno]->direct , dir[entityno]->name );
		return(0);
	}

	Readrec( dir[entityno]->param );
	Readint( &entity_type , "" );
	if( entity_type != 142 )
	{
		printf( "Expected Curve on a Paremetric Surface, found entity type %d\n" , entity_type );
		return( 0 );
	}
	Readint( &i , "Curve created by method %d: " );
	Readint( &surf_de , "Curve is on surface %d: " );
	if( surf_de != on_surf_de )
		printf( "Curve is on surface at DE %d, should be on surface at DE %d\n", surf_de, on_surf_de );

	Readint( &param_curve_de , "Parametric Curve DE: " );
	Readint( &model_curve_de , "Model Curve DE: " );

	vert_count = Count_verts( (model_curve_de-1)/2 );

	(*nurb_v) = (struct nurb_verts *)rt_calloc( vert_count , sizeof( struct nurb_verts ) , "Get_curve_verts: (*nurb_v)" );

	return( vert_count );
}

int
Get_nurb_surf( entityno, u_order, v_order, n_u_knots, n_v_knots, ukv, vkv, n_rows, n_cols, pt_type, mesh )
int entityno;
int *u_order,*v_order,*n_u_knots,*n_v_knots,*n_rows,*n_cols,*pt_type;
fastf_t **ukv,**vkv,**mesh;
{
	int entity_type;
	int i;
	int num_knots;
	int num_pts;
	int rational;
	double a;
	fastf_t u_min,u_max,v_min,v_max;

	/* Acquiring Data */

	if( dir[entityno]->param <= pstart )
	{
		printf( "Illegal parameter pointer for entity D%07d (%s)\n" ,
				dir[entityno]->direct , dir[entityno]->name );
		return(0);
	}

	Readrec( dir[entityno]->param );
	Readint( &entity_type , "" );
	if( entity_type != 128 )
	{
		rt_log( "Only B-Spline surfaces allowed for faces (found type %d)\n", entity_type );
		return( 1 );
	}
	Readint( &i , "Upper index of first Sum: " );
	*n_rows = i+1;
	Readint( &i , "Upper index of second sum: " );
	*n_cols = i+1;
	Readint( &i , "Degree in first direction: " );
	*u_order = i+1;
	Readint ( &i , "Degree in second direction: " );
	*v_order = i+1;
	Readint( &i , "Closed in first direction: " );
	Readint( &i , "Closed in second direction: " );
	Readint( &i , "Polynomial: " );
	rational = !i;
	*pt_type = RT_NURB_MAKE_PT_TYPE( 3+rational , 2 , rational );
	Readint( &i , "Periodic in first direction: " );
	Readint( &i , "Periodic in second direction: " );
	*n_u_knots = (*n_rows)+(*u_order);
	*ukv = (fastf_t *)rt_calloc( *n_u_knots , sizeof( fastf_t ) , "Get_nurb_surf: *ukv" );
	for( i=0 ; i<*n_u_knots ; i++ )
	{
		Readdbl( &a , "U_knot Vector: " );
		(*ukv)[i] = a;
	}
	*n_v_knots = (*n_cols)+(*v_order);
	(*vkv) = (fastf_t *)rt_calloc( *n_v_knots , sizeof( fastf_t ) , "Get_nurb_surf: *vkv" );
	for( i=0 ; i<*n_v_knots ; i++ )
	{
		Readdbl( &a , "V_knot Vector: " );
		(*vkv)[i] = a;
	}
	num_pts = (*n_rows)*(*n_cols);
	*mesh = (fastf_t *)rt_calloc( num_pts*(3+rational) , sizeof( fastf_t ) , "Get_nurb_surf: *mesh" );
	for( i=0 ; i<num_pts ; i++ )
	{
		Readdbl( &a , "Weight: " );
		if( rational )
			(*mesh)[i*4+3] = a;
	}
	for( i=0 ; i<num_pts ; i++ )
	{
		Readdbl( &a , "X: " );
		(*mesh)[i*(3+rational)] = a;
		Readdbl( &a , "Y: " );
		(*mesh)[i*(3+rational)+1] = a;
		Readdbl( &a , "Z: " );
		(*mesh)[i*(3+rational)+2] = a;
	}
	Readdbl( &a , "u_min: " );
	u_min = a;
	Readdbl( &a , "u_max: " );
	u_max = a;
	Readdbl( &a , "v_min: " );
	v_min = a;
	Readdbl( &a , "v_max: " );
	v_max = a;

	if( u_min != 0.0 || u_max != 1.0 )
	{
		num_knots = (*n_rows)+(*u_order);
		for( i=0 ; i<num_knots ; i++ )
			(*ukv)[i] = ((*ukv)[i]-u_min)/(u_max-u_min);
	}
	if( v_min != 0.0 || v_max != 1.0 )
	{
		num_knots = (*n_cols)+(*v_order);
		for( i=0 ; i<num_knots ; i++ )
			(*vkv)[i] = ((*vkv)[i]-v_min)/(v_max-v_min);
	}

	return( 0 );
}

struct faceuse *
trim_surf( entityno , s )
int entityno;
struct shell *s;
{
	struct faceuse *fu;
	struct nurb_verts *nurb_v;
	struct vertex ***verts;
	int sol_num;
	int surf_de;
	int nverts;
	int u_order,v_order,n_u_knots,n_v_knots,n_rows,n_cols,pt_type;
	fastf_t *ukv,*vkv,*mesh;
	int coords;
	int has_outer_boundary,inner_loop_count,outer_loop;
	int *inner_loop;
	int i;

	/* Acquiring Data */

	if( dir[entityno]->param <= pstart )
	{
		printf( "Illegal parameter pointer for entity D%07d (%s)\n" ,
				dir[entityno]->direct , dir[entityno]->name );
		return(0);
	}

	Readrec( dir[entityno]->param );
	Readint( &sol_num , "" );
	if( sol_num != 144 )
	{
		printf( "Expected Trimmed Surface Entity found type %d\n" );
		return( (struct faceuse *)NULL );
	}
	Readint( &surf_de , "Surface DE = " );
	Readint( &has_outer_boundary , "Has outer boundary = " );
	Readint( &inner_loop_count , "Inner loops = " );
	Readint( &outer_loop , "Outer loop = " );
	if( inner_loop_count )
	{
		inner_loop = (int *)rt_calloc( inner_loop_count , sizeof( int ) , "trim_surf: innerloop" );
		for( i=0 ; i<inner_loop_count ; i++ )
			Readint( &inner_loop[i] , "\tInner loop = " );
	}

	if( Get_nurb_surf( (surf_de-1)/2, &u_order, &v_order, &n_u_knots, &n_v_knots, &ukv, &vkv,
				&n_rows, &n_cols, &pt_type, &mesh ) )
	{
		rt_free( (char *)inner_loop , "trim_surf: inner_loop" );
		return( (struct faceuse *)NULL );
	}
	coords = RT_NURB_EXTRACT_COORDS( pt_type );

	if( !has_outer_boundary )
	{
printf( "No outer boundary use rectangle\n" );
		nverts = 4;
		nurb_v = (struct nurb_verts *)rt_calloc( nverts , sizeof( struct nurb_verts ) , "trim_surf: nurb_v " );
	}
	else
		nverts = Get_curve_verts( (outer_loop-1)/2 , entityno*2+1 , &nurb_v );
printf( "Outer boundary has %d vertices\n" , nverts );

	verts = (struct vertex ***)rt_calloc( nverts , sizeof( struct vertex **) , "trim_surf: verts" );
	if( has_outer_boundary )
	{
		for( i=0 ; i<nverts ; i++ )
		{
			verts[i] = &nurb_v[i].v;
printf( "pt ( %f %f %f ) vertex pointer x%x\n" , V3ARGS( nurb_v[i].pt ) , verts[i] );
		}
	}
	fu = nmg_cmface( s , verts , nverts );
printf( "Made faceuse x%x\n" , fu );
printf( "Calling nmg_face_g_snurb:\n" ) ;
printf( "\tu_order = %d, v_order = %d, num_u_knots = %d, num_v_knots = %d\n" , u_order,v_order,n_u_knots,n_v_knots );
printf( "\tU knots:\n" );
for( i=0 ; i<n_u_knots ; i++ )
	printf( "\t\t%f\n" , ukv[i] );
printf( "\tV knots:\n" );
for( i=0 ; i<n_v_knots ; i++ )
	printf( "\t\t%f\n" , vkv[i] );
printf( "\tControl mesh is %dX%d (%d coords per point):\n" , n_rows,n_cols,coords );
for( i=0 ; i<n_rows ; i++ )
{
	int j;

	printf( "\t\tRow %d:\n\t\t\t" , i );
	for( j=0 ; j<n_cols ; j++ )
	{
		if( coords == 3 )
			printf( " ( %f, %f, %f)" , V3ARGS( &mesh[i*n_cols*3+j*3] ) );
		else if( coords ==4 )
			printf( " ( %f, %f, %f, %f)" , V4ARGS( &mesh[i*n_cols*4+j*4] ) );
		else
		{
			printf( "Coords = %d!!!!!!\n" , coords );
			exit( 1 );
		}
		
	}
	putchar( '\n' );
}
	nmg_face_g_snurb( fu, u_order, v_order, n_u_knots, n_v_knots, ukv, vkv, n_rows, n_cols, pt_type, mesh );

	if( !has_outer_boundary )
	{
		struct vertexuse *vu;

		VMOVE( nurb_v[0].pt , &mesh[0] );
		nmg_vertex_gv( nurb_v[0].v , nurb_v[0].pt );
		VSET( nurb_v[0].uvw , 0, 0, 0 );
		for( RT_LIST_FOR( vu , vertexuse , &nurb_v[0].v->vu_hd ) )
			nmg_vertexuse_a_cnurb( vu , nurb_v[0].uvw );

		VMOVE( nurb_v[1].pt , &mesh[(n_rows-1)*coords] );
		nmg_vertex_gv( nurb_v[1].v , nurb_v[1].pt );
		VSET( nurb_v[1].uvw , 1, 0, 0 );
		for( RT_LIST_FOR( vu , vertexuse , &nurb_v[1].v->vu_hd ) )
			nmg_vertexuse_a_cnurb( vu , nurb_v[1].uvw );

		VMOVE( nurb_v[2].pt , &mesh[(n_rows-1)*(n_cols-1)*coords] );
		nmg_vertex_gv( nurb_v[2].v , nurb_v[2].pt );
		VSET( nurb_v[2].uvw , 1, 1, 0 );
		for( RT_LIST_FOR( vu , vertexuse , &nurb_v[2].v->vu_hd ) )
			nmg_vertexuse_a_cnurb( vu , nurb_v[2].uvw );

		VMOVE( nurb_v[3].pt , &mesh[((n_rows-1)*(n_cols-2)+1)*coords] );
		nmg_vertex_gv( nurb_v[3].v , nurb_v[3].pt );
		VSET( nurb_v[3].uvw , 0, 1, 0 );
		for( RT_LIST_FOR( vu , vertexuse , &nurb_v[3].v->vu_hd ) )
			nmg_vertexuse_a_cnurb( vu , nurb_v[3].uvw );
	}
	else
		Set_vertices( fu , nverts , nurb_v , surf_de , (outer_loop-1)/2 );

	if( inner_loop_count )	
		rt_free( (char *)inner_loop , "trim_surf: inner_loop" );
	return( fu );
}

void
Convtrimsurfs()
{

	int i,convsurf=0,totsurfs=0;
	struct model *m;
	struct nmgregion *r;
	struct shell *s;
	struct faceuse *fu;

	printf( "\n\nConverting Trimmed Surface entities:\n" );

	m = nmg_mm();
	r = nmg_mrsv( m );
	s = RT_LIST_FIRST( shell , &r->s_hd );

	for( i=0 ; i<totentities ; i++ )
	{
		if( dir[i]->type == 144 )
		{
			totsurfs++;
			fu = trim_surf( i , s );
			if( fu )
			{
				nmg_face_bb( fu->f_p , &tol );
				convsurf++;
			}
		}
	}
	printf( "Converted %d Trimmed Sufaces successfully out of %d total Trimmed Sufaces\n" , convsurf , totsurfs );

	(void)nmg_model_fuse( m , &tol );

	mk_nmg( fdout , "Trimmed_surf" , m );
	nmg_km( m );
}
