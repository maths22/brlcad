/*
 *			C H G T R E E
 *
 * This module contains functions which change the tree structure
 * of the model, and delete solids or combinations or combination elements.
 *
 * Functions -
 *
 * Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include	<math.h>
#include	<stdio.h>
#include "ged_types.h"
#include "db.h"
#include "sedit.h"
#include "ged.h"
#include "dir.h"
#include "solid.h"
#include "dm.h"
#include "vmath.h"

extern int	atoi();

extern int	numargs;	/* number of args */
extern char	*cmd_args[];	/* array of pointers to args */

/* Rename an object */
/* Format: n oldname newname	*/
void
f_name()
{
	register struct directory *dp;
	union record record;

	if( (dp = lookup( cmd_args[1], LOOKUP_NOISY )) == DIR_NULL )
		return;

	if( lookup( cmd_args[2], LOOKUP_QUIET ) != DIR_NULL )  {
		aexists( cmd_args[2] );
		return;
	}

	dp->d_namep = strdup( cmd_args[2] );
	db_getrec( dp, &record, 0 );

	NAMEMOVE( cmd_args[2], record.c.c_name );
	db_putrec( dp, &record, 0 );
	(void)printf("done\n");
}

/* Copy a solid */
/* Format: c oldname newname	*/
void
f_copy()
{
	register struct directory *proto;
	register struct directory *dp;
	union record record;
	int i, ngran;

	if( (proto = lookup( cmd_args[1], LOOKUP_NOISY )) == DIR_NULL )
		return;

	if( lookup( cmd_args[2], LOOKUP_QUIET ) != DIR_NULL )  {
		aexists( cmd_args[2] );
		return;
	}

	db_getrec( proto, &record, 0 );

	if( record.u_id != ID_SOLID && record.u_id != ID_ARS_A ) {
		(void)printf("%s: not a solid\n", proto->d_namep );
		return;
	}

	/*
	 * Update the in-core directory
	 */
	if( (dp = dir_add( cmd_args[2], -1, DIR_SOLID, 0 )) == DIR_NULL )
		return;
	db_alloc( dp, proto->d_len );

	/*
	 * Update the disk record
	 */
	if(record.u_id == ID_ARS_A)  {
		NAMEMOVE( cmd_args[2], record.a.a_name );
		ngran = record.a.a_totlen;
		db_putrec( dp, &record, 0 );

		/* Process the rest of the ARS (b records)  */
		for( i = 0; i < ngran; i++ )  {
			db_getrec( proto, &record, i+1 );
			if( i == 0 )  {
				record.b.b_values[0] = -toViewcenter[MDX];
				record.b.b_values[1] = -toViewcenter[MDY];
				record.b.b_values[2] = -toViewcenter[MDZ];
			}
			db_putrec( dp, &record, i+1 );
		}
	}  else  {
		NAMEMOVE( cmd_args[2], record.s.s_name );
		record.s.s_values[0] = -toViewcenter[MDX];
		record.s.s_values[1] = -toViewcenter[MDY];
		record.s.s_values[2] = -toViewcenter[MDZ];
		db_putrec( dp, &record, 0 );
	}
	(void)printf("done\n");
}

/* Create an instance of something */
/* Format: i object combname instname [op]	*/
void
f_instance()
{
	register struct directory *dp;
	char oper;

	if( (dp = lookup( cmd_args[1], LOOKUP_NOISY )) == DIR_NULL )
		return;

	oper = UNION;
	if( numargs == 5 )
		oper = cmd_args[4][0];
	if(oper != UNION && oper != SUBTRACT &&	oper != INTERSECT) {
		(void)printf("bad operation: %c\n", oper );
		return;
	}
	if( combadd( dp, cmd_args[2], cmd_args[3], '\0', oper, 0, 0 ) ==
	    DIR_NULL )
		return;
	(void)printf("done\n");
}

/* add solids to a region or create the region */
/* and then add solids */
/* Format: r regionname opr1 sol1 opr2 sol2 ... oprn soln */
void
f_region()
{
	register struct directory *dp;
	union record record;
	int i;
	int ident, air;
	char oper;

	ident = air = 0;
	/* Check for even number of arguments */
	if( numargs & 01 )  {
		printf("error in number of args!\n");
		return;
	}
	/* Get operation and solid name for each solid */
	for( i = 2; i < numargs; i += 2 )  {
		if( cmd_args[i][1] != '\0' )  {
			(void)printf("bad operation: %s skip member: %s\n",
				cmd_args[i], cmd_args[i+1] );
			continue;
		}
		oper = cmd_args[i][0];
		if( (dp = lookup( cmd_args[i + 1], LOOKUP_NOISY )) == DIR_NULL )  {
			(void)printf("skipping %s\n", cmd_args[i + 1] );
			continue;
		}

		if(oper != UNION && oper != SUBTRACT &&	oper != INTERSECT) {
			(void)printf("bad operation: %c skip member: %s\n",
				oper, dp->d_namep );
			continue;
		}

		db_getrec( dp, &record, 0 );
		if( record.u_id == ID_COMB ) {
			if( record.c.c_flags == 'R' ) {
				(void)printf(
				     "Note: %s is a region\n",
				     dp->d_namep );
			}
		}

		if( combadd( dp, cmd_args[1], (char *)NULL, 'r', oper, ident,
							air ) == DIR_NULL )  {
			(void)printf("error in combadd\n");
			return;
		}
	}
	(void)printf("done\n");
}

/* Remove an object or several from the description */
/* Format: k object1 object2 .... objectn	*/
void
f_kill()
{
	register struct directory *dp;
	register int i;

	for( i = 1; i < numargs; i++ )  {
		if( (dp = lookup( cmd_args[i], LOOKUP_NOISY )) != DIR_NULL )  {
			eraseobj( dp );
			db_delete( dp );
			dir_delete( dp );
		}
	}
	dmaflag = 1;
	(void)printf("done\n");
}

/* Grouping command */
/* Format: g groupname object1 object2 .... objectn	*/
void
f_group()
{
	register struct directory *dp;
	register int i;

	/* get objects to add to group */
	for( i = 2; i < numargs; i++ )  {
		if( (dp = lookup( cmd_args[i], LOOKUP_NOISY)) != DIR_NULL )  {
			if( combadd( dp, cmd_args[1], (char *)NULL, 'g',
				UNION, 0, 0) == DIR_NULL )
				return;
		}
		else
			(void)printf("skip member %s\n", cmd_args[i]);
	}
	(void)printf("done\n");
}

/* Delete members of a combination */
/* Format: D comb memb1 memb2 .... membn	*/
void
f_delmem()
{
	register struct directory *dp;
	register int i, rec;
	union record record;

	if( (dp = lookup( cmd_args[1], LOOKUP_NOISY )) == DIR_NULL )
		return;

	/* Examine all the Member records, one at a time */
	for( rec = 1; rec < dp->d_len; rec++ )  {
		db_getrec( dp, &record, rec );
top:
		/* Compare this member to each command arg */
		for( i = 2; i < numargs; i++ )  {
			if( strcmp( cmd_args[i], record.M.m_instname ) != 0 &&
			    strcmp( cmd_args[i], record.M.m_brname ) != 0 )
				continue;
			printf("deleting member %s\n", cmd_args[i] );

			/* If deleting last member, just truncate */
			if( rec == dp->d_len-1 ) {
				db_trunc(dp, 1);
				continue;
			}

			db_getrec( dp, &record, dp->d_len-1 );	/* last one */
			db_putrec( dp, &record, rec );		/* xch */
			db_trunc( dp, 1 );
			goto top;
		}
	}
	(void)printf("done\n");
}
