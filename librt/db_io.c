/*
 *			D B _ I O . C
 *
 * Functions -
 *	db_getmrec	Read all records into malloc()ed memory chunk
 *	db_get		Get records from database
 *	db_put		Put records to database
 *
 *
 *  Authors -
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

/*
 *  			D B _ G E T M R E C
 *
 *  Retrieve all records in the database pertaining to an object,
 *  and place them in malloc()'ed storage, which the caller is
 *  responsible for free()'ing.
 *
 *  Returns -
 *	union record *		OK
 *	(union record *)0	failure
 */
union record *
db_getmrec( dbip, dp )
struct db_i	*dbip;
struct directory *dp;
{
	union record	*where;

	if( dbip->dbi_magic != DBI_MAGIC )  rt_bomb("db_getmrec:  bad dbip\n");
	if(rt_g.debug&DEBUG_DB) rt_log("db_getmrec(%s) x%x, x%x\n",
		dp->d_namep, dbip, dp );

	if( dp->d_addr < 0 )
		return( (union record *)0 );	/* was dummy DB entry */
	if( (where = (union record *)rt_malloc(
		dp->d_len * sizeof(union record),
		dp->d_namep)
	    ) == ((union record *)0) )
		return( (union record *)0 );

	if( db_read( dbip, (char *)where,
	    (long)dp->d_len * sizeof(union record),
	    dp->d_addr ) < 0 )  {
		rt_free( (char *)where, dp->d_namep );
		return( (union record *)0 );	/* VERY BAD */
	}
	return( where );
}

/*
 *  			D B _ G E T
 *
 *  Retrieve 'len' records from the database,
 *  "offset" granules into this entry.
 *
 *  Returns -
 *	 0	OK
 *	-1	failure
 */
int
db_get( dbip, dp, where, offset, len )
struct db_i	*dbip;
struct directory *dp;
union record	*where;
int		offset;
int		len;
{

	if( dbip->dbi_magic != DBI_MAGIC )  rt_bomb("db_get:  bad dbip\n");
	if(rt_g.debug&DEBUG_DB) rt_log("db_get(%s) x%x, x%x x%x off=%d len=%d\n",
		dp->d_namep, dbip, dp, where, offset, len );

	if( dp->d_addr < 0 )  {
		where->u_id = '\0';	/* undefined id */
		return(-1);
	}
	if( offset < 0 || offset+len > dp->d_len )  {
		rt_log("db_get(%s):  xfer %d..%x exceeds 0..%d\n",
			dp->d_namep, offset, offset+len, dp->d_len );
		where->u_id = '\0';	/* undefined id */
		return(-1);
	}

	if( db_read( dbip, (char *)where, (long)len * sizeof(union record),
	    dp->d_addr + offset * sizeof(union record) ) < 0 )  {
		where->u_id = '\0';	/* undefined id */
		return(-1);
	}
	return(0);			/* OK */
}

/*
 *  			D B _ P U T
 *
 *  Store 'len' records to the database,
 *  "offset" granules into this entry.
 *
 *  Returns -
 *	 0	OK
 *	-1	failure
 */
int
db_put( dbip, dp, where, offset, len )
struct db_i	*dbip;
struct directory *dp;
union record	*where;
int		offset;
int		len;
{

	if( dbip->dbi_magic != DBI_MAGIC )  rt_bomb("db_put:  bad dbip\n");
	if(rt_g.debug&DEBUG_DB) rt_log("db_put(%s) x%x, x%x x%x off=%d len=%d\n",
		dp->d_namep, dbip, dp, where, offset, len );

	if( dbip->dbi_read_only )  {
		rt_log("db_put(%s):  READ-ONLY file\n",
			dbip->dbi_filename);
		return(-1);
	}
	if( offset < 0 || offset+len > dp->d_len )  {
		rt_log("db_put(%s):  xfer %d..%x exceeds 0..%d\n",
			dp->d_namep, offset, offset+len, dp->d_len );
		return(-1);
	}

	if( db_write( dbip, (char *)where, (long)len * sizeof(union record),
	    dp->d_addr + offset * sizeof(union record) ) < 0 )  {
		return(-1);
	}
	return(0);
}

/*
 *			D B _ R E A D
 *
 *  Reads 'count' bytes at file offset 'offset' into buffer at 'addr'.
 *  A wrapper for the UNIX read() sys-call that takes into account
 *  syscall semaphores, stdio-only machines, and in-memory buffering.
 *
 *  Returns -
 *	 0	OK
 *	-1	failure
 */
int
db_read( dbip, addr, count, offset )
struct db_i	*dbip;
genptr_t	addr;
long		count;		/* byte count */
long		offset;		/* byte offset from start of file */
{
	register int	got;

	if( dbip->dbi_magic != DBI_MAGIC )  rt_bomb("db_read:  bad dbip\n");
	if(rt_g.debug&DEBUG_DB)  {
		rt_log("db_read(dbip=x%x, addr=x%x, count=%d., offset=%d.)\n",
			dbip, addr, count, offset );
	}
	if( count <= 0 || offset < 0 )  {
		return(-1);
	}
	if( dbip->dbi_inmem )  {
#if defined(SYSV)
		memcpy( addr, dbip->dbi_inmem + offset, count );
#else
		bcopy( dbip->dbi_inmem + offset, addr, count );
#endif
		return(0);
	}
	RES_ACQUIRE( &rt_g.res_syscall );
#if unix
	(void)lseek( dbip->dbi_fd, offset, 0 );
	got = read( dbip->dbi_fd, addr, count );
#else
	(void)fseek( dbip->dbi_fp, offset, 0 );
	got = fread( addr, count, 1, dbip->dbi_fp );
#endif
	RES_RELEASE( &rt_g.res_syscall );
	if( got != count )  {
		perror("db_read");
		rt_log("db_read(%s):  read error.  Wanted %d, got %d bytes\n",
			dbip->dbi_filename, count, got );
		return(-1);
	}
	return(0);			/* OK */
}

/*
 *			D B _ W R I T E
 *
 *  Writes 'count' bytes into at file offset 'offset' from buffer at 'addr'.
 *  A wrapper for the UNIX write() sys-call that takes into account
 *  syscall semaphores, stdio-only machines, and in-memory buffering.
 *
 *  Returns -
 *	 0	OK
 *	-1	failure
 */
int
db_write( dbip, addr, count, offset )
struct db_i	*dbip;
genptr_t	addr;
long		count;
long		offset;
{
	register int	got;

	if( dbip->dbi_magic != DBI_MAGIC )  rt_bomb("db_write:  bad dbip\n");
	if(rt_g.debug&DEBUG_DB)  {
		rt_log("db_write(dbip=x%x, addr=x%x, count=%d., offset=%d.)\n",
			dbip, addr, count, offset );
	}
	if( dbip->dbi_read_only )  {
		rt_log("db_write(%s):  READ-ONLY file\n",
			dbip->dbi_filename);
		return(-1);
	}
	if( count <= 0 || offset < 0 )  {
		return(-1);
	}
	if( dbip->dbi_inmem )  {
		rt_log("db_write() in memory?\n");
		return(-1);
	}
	RES_ACQUIRE( &rt_g.res_syscall );
#if unix
	(void)lseek( dbip->dbi_fd, offset, 0 );
	got = write( dbip->dbi_fd, addr, count );
#else
	(void)fseek( dbip->dbi_fp, offset, 0 );
	got = fwrite( addr, count, 1, dbip->dbi_fp );
#endif
	RES_RELEASE( &rt_g.res_syscall );
	if( got != count )  {
		perror("db_write");
		rt_log("db_write(%s):  write error.  Wanted %d, got %d bytes\n",
			dbip->dbi_filename, count, got );
		return(-1);
	}
	return(0);			/* OK */
}
