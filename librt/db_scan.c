/*
 *			D B _ S C A N . C
 *
 * Functions -
 *	db_scan		Sequentially read database, send objects to handler()
 *	db_ident	Update database IDENT record
 *
 *
 *  Author -
 *	Michael John Muuss
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1988 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#ifdef BSD
#include <strings.h>
#else
#include <string.h>
#endif

#include "machine.h"
#include "vmath.h"
#include "raytrace.h"
#include "db.h"

#include "./debug.h"

#define DEBUG_PR(aaa, rrr) 	{\
	if(rt_g.debug&DEBUG_DB) rt_log("db_scan x%x %c (0%o)\n", \
		aaa,rrr.u_id,rrr.u_id ); }

/*
 *			D B _ S C A N
 *
 *  This routine sequentially reads through the model database file and
 *  builds a directory of the object names, to allow rapid
 *  named access to objects.
 *
 *  Note that some multi-record database items include length fields.
 *  These length fields are not used here.
 *  Instead, the sizes of multi-record items are determined by
 *  reading ahead and computing the actual size.
 *  This prevents difficulties arising from external "adjustment" of
 *  the number of records without corresponding adjustment of the length fields.
 *  In the future, these length fields will be phased out.
 *
 *  The handler will be called with a variety of args.
 *  The handler is responsible for handling name strings of exactly
 *  NAMESIZE chars.
 *  The most common example of such a function is db_diradd().
 *
 * Returns -
 *	 0	Success
 *	-1	Fatal Error
 */
int
db_scan( dbip, handler )
register struct db_i	*dbip;
int			(*handler)();
{
	union record	record;		/* Initial record, holds name */
	union record	rec2;		/* additional record(s) */
	register long	addr;		/* start of current rec */
	register long	here;		/* intermediate positions */
	register int	nrec;		/* # records for this solid */
	register int	totrec;		/* # records for database */
	register int	j;

	if(rt_g.debug&DEBUG_DB) rt_log("db_scan( x%x, x%x )\n", dbip, handler);
	if( dbip->dbi_magic != DBI_MAGIC )  rt_bomb("db_scan:  bad dbip\n");

	/* In a portable way, read the header (even if not rewound) */
	rewind( dbip->dbi_fp );
	if( fread( (char *)&record, sizeof record, 1, dbip->dbi_fp ) != 1  ||
	    record.u_id != ID_IDENT )  {
		rt_log("db_scan ERROR:  File is lacking a proper MGED database header\n");
	    	return(-1);
	}
	rewind( dbip->dbi_fp );

	here = addr = -1;
	totrec = 0;
	while(1)  {
		nrec = 0;
		if( (addr = ftell(dbip->dbi_fp)) == EOF )  {
			rt_log("db_scan:  ftell() failure\n");
			return(-1);
		}

		if( fread( (char *)&record, sizeof record, 1, dbip->dbi_fp ) != 1
		    || feof(dbip->dbi_fp) )
			break;
		DEBUG_PR( addr, record );

		nrec++;
		switch( record.u_id )  {
		case ID_IDENT:
			if( strcmp( record.i.i_version, ID_VERSION) != 0 )  {
				rt_log("db_scan WARNING: File is Version %s, Program is version %s\n",
					record.i.i_version, ID_VERSION );
			}
			/* Record first IDENT records title string */
			if( dbip->dbi_title == (char *)0 )  {
				dbip->dbi_title = rt_strdup( record.i.i_title );
				dbip->dbi_localunit = record.i.i_units;
				/* conversions() XXX */
				dbip->dbi_local2base = 1.0;
				dbip->dbi_base2local = 1.0;
			}
			break;
		case ID_FREE:
			/* Inform db manager of avail. space */
			memfree( &(dbip->dbi_freep), (unsigned)1,
				addr/sizeof(union record) );
			break;
		case ID_ARS_A:
			while(1) {
				here = ftell( dbip->dbi_fp );
				if( fread( (char *)&rec2, sizeof(rec2),
				    1, dbip->dbi_fp ) != 1 )
					break;
				DEBUG_PR( here, rec2 );
				if( rec2.u_id != ID_ARS_B )  {
					fseek( dbip->dbi_fp, here, 0 );
					break;
				}
				nrec++;
			}
			handler( dbip, record.a.a_name, addr, nrec,
				DIR_SOLID );
			break;
		case ID_ARS_B:
			rt_log("db_scan ERROR: Unattached ARS 'B' record\n");
			break;
		case ID_SOLID:
			handler( dbip, record.s.s_name, addr, nrec,
				DIR_SOLID );
			break;
		case ID_MATERIAL:
			/* This is common to RT and MGED */
			rt_color_addrec( &record, addr );
			break;
		case ID_P_HEAD:
			while(1) {
				here = ftell( dbip->dbi_fp );
				if( fread( (char *)&rec2, sizeof(rec2),
				    1, dbip->dbi_fp ) != 1 )
					break;
				DEBUG_PR( here, rec2 );
				if( rec2.u_id != ID_P_DATA )  {
					fseek( dbip->dbi_fp, here, 0 );
					break;
				}
				nrec++;
			}
			handler( dbip, record.p.p_name, addr, nrec,
				DIR_SOLID );
			break;
		case ID_P_DATA:
			rt_log("db_scan ERROR: Unattached P_DATA record\n");
			break;
		case ID_BSOLID:
			while(1) {
				/* Find and skip subsequent BSURFs */
				here = ftell( dbip->dbi_fp );
				if( fread( (char *)&rec2, sizeof(rec2),
				    1, dbip->dbi_fp ) != 1 )
					break;
				DEBUG_PR( here, rec2 );
				if( rec2.u_id != ID_BSURF )  {
					fseek( dbip->dbi_fp, here, 0 );
					break;
				}

				/* Just skip over knots and control mesh */
				j = (rec2.d.d_nknots + rec2.d.d_nctls);
				nrec += j+1;
				while( j-- > 0 )
					fread( (char *)&rec2, sizeof(rec2), 1, dbip->dbi_fp );
			}
			handler( dbip, record.B.B_name, addr, nrec,
				DIR_SOLID );
			break;
		case ID_BSURF:
			rt_log("db_scan ERROR: Unattached B-spline surface record\n");

			/* Just skip over knots and control mesh */
			j = (record.d.d_nknots + record.d.d_nctls);
			while( j-- > 0 )
				fread( (char *)&rec2, sizeof(rec2), 1, dbip->dbi_fp );
			break;
		case ID_MEMB:
			rt_log("db_scan ERROR: Unattached combination MEMBER record\n");
			break;
		case ID_COMB:
			while(1) {
				here = ftell( dbip->dbi_fp );
				if( fread( (char *)&rec2, sizeof(rec2),
				    1, dbip->dbi_fp ) != 1 )
					break;
				DEBUG_PR( here, rec2 );
				if( rec2.u_id != ID_MEMB )  {
					fseek( dbip->dbi_fp, here, 0 );
					break;
				}
				nrec++;
			}
			handler( dbip, record.c.c_name, addr, nrec,
				record.c.c_flags == 'R' ?
					DIR_COMB|DIR_REGION : DIR_COMB );
			break;
		default:
			rt_log("db_scan ERROR:  bad record %c (0%o), addr=x%x\n",
				record.u_id, record.u_id, addr );
			/* skip this record */
			break;
		}
		totrec += nrec;
	}
	dbip->dbi_nrec = totrec;
	dbip->dbi_eof = ftell( dbip->dbi_fp );
	rewind( dbip->dbi_fp );

	return( 0 );			/* OK */
}

#ifdef never
db_suckin()
{
	/*
	 * Obtain in-core copy of database, rather than doing lots of
	 * random-access reads.  Here, "addr" is really "nrecords".
	 */
	if( (rtip->rti_db = (union record *)rt_malloc(
	    addr*sizeof(union record), "in-core database"))
	    == (union record *)0 )
	    	rt_bomb("in-core database malloc failure");
	rewind(dbip->dbi_fp);
	if( fread( (char *)rtip->rti_db, sizeof(union record), addr,
	    dbip->dbi_fp) != addr )  {
	    	rt_log("rt_dirbuild:  problem reading db on 2nd pass\n");
	    	goto bad;
	}
}
#endif


/*
 *			D B _ I D E N T
 *
 *  Update the IDENT record with new title and units.
 *  To permit using db_get and db_put, a custom directory entry is crafted.
 *
 * Returns -
 *	 0	Success
 *	-1	Fatal Error
 */
int
db_ident( dbip, title, units )
struct db_i	*dbip;
char		*title;
int		units;
{
	struct directory	dir;
	union record		rec;

	if( dbip->dbi_magic != DBI_MAGIC )  rt_bomb("db_ident:  bad dbip\n");
	if(rt_g.debug&DEBUG_DB) rt_log("db_ident( x%x, %s, %d )\n",
		dbip, title, units );

	if( dbip->dbi_read_only )
		return(-1);

	dir.d_namep = "/IDENT/";
	dir.d_addr = 0L;
	dir.d_len = 1;
	if( db_get( dbip, &dir, &rec, 0, 1 ) < 0 ||
	    rec.u_id != ID_IDENT )
		return(-1);

	rec.i.i_title[0] = '\0';
	(void)strncpy(rec.i.i_title, title, sizeof(rec.i.i_title)-1 );

	if( dbip->dbi_title )
		rt_free( dbip->dbi_title, "dbi_title" );
	dbip->dbi_title = rt_strdup( title );
	dbip->dbi_localunit = rec.i.i_units = units;

	return( db_put( dbip, &dir, &rec, 0, 1 ) );
}
