/*
 *			M O V E R . C
 *
 * Functions -
 *	moveHobj	used to update position of an object in objects file
 *	moveinstance	Given a COMB and an object, modify all the regerences
 *	combadd		Add an instance of an object to a combination
 *
 *  Author -
 *	Michael John Muuss
 *
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1985 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include	<string.h>
#include "ged_types.h"
#include "../h/db.h"
#include "ged.h"
#include "objdir.h"
#include "solid.h"
#include "../h/vmath.h"

extern int	printf();

/* default region ident codes */
int	item_default = 1000;
int	air_default = 0;
int	mat_default = 1;
int	los_default = 100;

/*
 *			M O V E H O B J
 *
 * This routine is used when the object to be moved is
 * the top level in its reference path.  The object itself
 * is relocated.
 */
void
moveHobj( dp, xlate )
register struct directory *dp;
matp_t xlate;
{
	vect_t	work;			/* Working vector */
	register int i;
	register float *p;		/* -> to vector to be worked on */
	static float *area_end;		/* End of area to be processed */
	union record record;

	db_getrec( dp, &record, 0 );

	switch( record.u_id )  {

	case ID_ARS_A:
		/* 1st B type record is special:  Vertex point */
		db_getrec( dp, &record, 1 );

		/* displace the base vector */
		MAT4X3PNT( work, xlate, &record.b.b_values[0] );
		VMOVE( &record.b.b_values[0], work );

		/* Transform remaining vectors */
		for( p = &record.b.b_values[1*3]; p < &record.b.b_values[8*3];
								p += 3) {
			MAT4X3VEC( work, xlate, p );
			VMOVE( p, work );
		}
		db_putrec( dp, &record, 1 );

		/* Process all the remaining B records */
		for( i = 2; i < dp->d_len; i++ )  {
			db_getrec( dp, &record, i );
			/* Transform remaining vectors */
			for( p = &record.b.b_values[0*3];
			     p < &record.b.b_values[8*3]; p += 3) {
				MAT4X3VEC( work, xlate, p );
				VMOVE( p, work );
			}
			db_putrec( dp, &record, i );
		}
		break;

	case ID_B_SPL_HEAD:
		for( i = 1; i < dp->d_len; i++) {
			db_getrec( dp, &record, i);
			if( record.u_id != ID_B_SPL_CTL )
				continue;
			for( p = &record.l.l_pts[0*3];
			     p < &record.l.l_pts[8*3]; p += 3) {
				MAT4X3VEC( work, xlate, p );
				VMOVE( p, work );
			}
			db_putrec( dp, &record, i);
		}
		break;

	case ID_SOLID:
		/* Displace the vertex (V) */
		MAT4X3PNT( work, xlate, &record.s.s_values[0] );
		VMOVE( &record.s.s_values[0], work );

		switch( record.s.s_type )  {

		case GENARB8:
			if(record.s.s_cgtype < 0)
				record.s.s_cgtype = -record.s.s_cgtype;
			area_end = &record.s.s_values[8*3];
			goto common;

		case GENTGC:
			area_end = &record.s.s_values[6*3];
			goto common;

		case GENELL:
			area_end = &record.s.s_values[4*3];
			goto common;

		case TOR:
			area_end = &record.s.s_values[8*3];
			/* Fall into COMMON section */

		common:
			/* Transform all the vectors */
			for( p = &record.s.s_values[1*3]; p < area_end;
			     p += 3) {
				MAT4X3VEC( work, xlate, p );
				VMOVE( p, work );
			}
			break;

		default:
			(void)printf("moveobj:  can't move obj type %d\n",
				record.s.s_type );
			return;		/* ERROR */
		}
		db_putrec( dp, &record, 0 );
		break;

	default:
		(void)printf("MoveHobj -- bad disk record\n");
		return;			/* ERROR */

	case ID_COMB:
		/*
		 * Move all the references within a combination
		 */
		for( i=1; i < dp->d_len; i++ )  {
			static mat_t temp;

			db_getrec( dp, &record, i );
			mat_mul( temp, xlate, record.M.m_mat );
			mat_copy( record.M.m_mat, temp );
			db_putrec( dp, &record, i );
		}
	}
	return;
}

/*
 *			M O V E H I N S T A N C E
 *
 * This routine is used when an instance of an object is to be
 * moved relative to a combination, as opposed to modifying the
 * co-ordinates of member solids.  Input is a pointer to a COMB,
 * a pointer to an object within the COMB, and modifications.
 */
void
moveHinstance( cdp, dp, xlate )
struct directory *cdp;
struct directory *dp;
matp_t xlate;
{
	register int i;
	union record record;
	mat_t temp;			/* Temporary for mat_mul */

	for( i=1; i < cdp->d_len; i++ )  {
		db_getrec( cdp, &record, i );

		/* Check for match */
		if( strcmp( dp->d_namep, record.M.m_instname ) == 0 )  {
			/* Apply the Homogeneous Transformation Matrix */
			mat_mul(temp, xlate, record.M.m_mat);
			mat_copy( record.M.m_mat, temp );

			db_putrec( cdp, &record, i );
			return;
		}
	}
	(void)printf( "moveinst:  couldn't find %s/%s\n",
		cdp->d_namep, dp->d_namep );
	return;				/* ERROR */
}

/*
 *			C O M B A D D
 *
 * Add an instance of object 'dp' to combination 'name'.
 * If the combination does not exist, it is created.
 * Flag is 'r' (region), or 'g' (group).
 */
struct directory *
combadd( objp, combname, region_flag, relation, ident, air )
register struct directory *objp;
char *combname;
int region_flag;			/* true if adding region */
int relation;				/* = UNION, SUBTRACT, INTERSECT */
int ident;				/* "Region ID" */
int air;				/* Air code */
{
	register struct directory *dp;
	union record record;

	/*
	 * Check to see if we have to create a new combination
	 */
	if( (dp = lookup( combname, LOOKUP_QUIET )) == DIR_NULL )  {

		/* Update the in-core directory */
		dp = dir_add( combname, -1, DIR_COMB, 2 );
		if( dp == DIR_NULL )
			return DIR_NULL;
		db_alloc( dp, 2 );

		/* Generate the disk record */
		record.c.c_id = ID_COMB;
		record.c.c_length = 1;
		record.c.c_flags = record.c.c_aircode = 0;
		record.c.c_regionid = -1;
		record.c.c_material = record.c.c_los = 0;
		NAMEMOVE( combname, record.c.c_name );
		if( region_flag ) {       /* creating a region */
			record.c.c_flags = 'R';
			record.c.c_regionid = ident;
			record.c.c_aircode = air;
			record.c.c_material = mat_default;
			record.c.c_los = los_default;
			(void)printf("Creating region id=%d, air=%d, mat=%d, los=%d\n",
				ident, air, mat_default, los_default );
		}

		/* finished with combination record - write it out */
		db_putrec( dp, &record, 0 );

		/* create first member record */
		(void)strcpy( record.M.m_instname, objp->d_namep );

		record.M.m_id = ID_MEMB;
		record.M.m_relation = relation;
		mat_idn( record.M.m_mat );

		db_putrec( dp, &record, 1 );
		return( dp );
	}

	/*
	 * The named combination already exists.  Fetch the header record,
	 * and verify that this is a combination.
	 */
	db_getrec( dp, &record, 0 );
	if( record.u_id != ID_COMB )  {
		(void)printf("%s:  not a combination\n", combname );
		return DIR_NULL;
	}

	if( region_flag ) {
		if( record.c.c_flags != 'R' ) {
			(void)printf("%s: not a region\n",combname);
			return DIR_NULL;
		}
	}
	record.c.c_length++;
	db_putrec( dp, &record, 0 );
	db_grow( dp, 1 );

	/* Fill in new Member record */
	record.M.m_id = ID_MEMB;
	record.M.m_relation = relation;
	mat_idn( record.M.m_mat );
	(void)strcpy( record.M.m_instname, objp->d_namep );

	db_putrec( dp, &record, dp->d_len-1 );
	return( dp );
}
