/*
 *			D B _ L O O K U P . C
 *
 * Functions -
 *	db_dirhash	Compute hashing function
 *	db_lookup	Convert an object name into directory pointer
 *	db_diradd	Add entry to the directory
 *	db_dirdelete	Delete entry from directory
 *	db_rename	Change name string of a directory entry
 *	db_pr_dir	Print contents of database directory
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
static const char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include "conf.h"

#include <stdio.h>
#ifdef USE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "machine.h"
#include "vmath.h"
#include "db.h"
#include "raytrace.h"

#include "./debug.h"


/*
 *			D B _ I S _ D I R E C T O R Y _ N O N _ E M P T Y
 *
 *  Returns -
 *	0	if the in-memory directory is empty
 *	1	if the in-memory directory has entries,
 *		which implies that a db_scan() has already been performed.
 */
int
db_is_directory_non_empty(CONST struct db_i	*dbip)
{
	register int	i;

	RT_CK_DBI(dbip);

	for (i = 0; i < RT_DBNHASH; i++)  {
		if( dbip->dbi_Head[i] != DIR_NULL )
			return 1;
	}
	return 0;
}

/*
 *			D B _ G E T _ D I R E C T O R Y _ S I Z E
 *
 *  Return the number of "struct directory" nodes in the given database.
 */
int
db_get_directory_size( dbip )
CONST struct db_i	*dbip;
{
	register struct directory *dp;
	register int	count = 0;
	int		i;

	RT_CK_DBI(dbip);

	for (i = 0; i < RT_DBNHASH; i++)  {
		for (dp = dbip->dbi_Head[i]; dp != DIR_NULL; dp = dp->d_forw)
			count++;
	}
	return count;
}

/*
 *			D B _ D I R H A S H
 *  
 *  Internal function to return pointer to head of hash chain
 *  corresponding to the given string.
 */
int
db_dirhash(str)
CONST char *str;
{
	register CONST unsigned char *s = (unsigned char *)str;
	register long sum;
	register int i;

	sum = 0;
	/* BSD namei hashing starts i=0, discarding first char.  why? */
	for( i=1; *s; )
		sum += *s++ * i++;

	return( RT_DBHASH(sum) );
}


/*
 *			D B _ L O O K U P
 *
 * This routine takes a name and looks it up in the
 * directory table.  If the name is present, a pointer to
 * the directory struct element is returned, otherwise
 * NULL is returned.
 *
 * If noisy is non-zero, a print occurs, else only
 * the return code indicates failure.
 *
 *  Returns -
 *	struct directory	if name is found
 *	DIR_NULL		on failure
 */
struct directory *
db_lookup( dbip, name, noisy )
CONST struct db_i	*dbip;
register CONST char	*name;
int			noisy;
{
	register struct directory *dp;
	register char	n0 = name[0];
	register char	n1 = name[1];

	RT_CK_DBI(dbip);

	dp = dbip->dbi_Head[db_dirhash(name)];
	for(; dp != DIR_NULL; dp=dp->d_forw )  {
		register char	*this;
		if(
			n0 == *(this=dp->d_namep)  &&	/* speed */
			n1 == this[1]  &&	/* speed */
			strcmp( name, this ) == 0
		)  {
			if(rt_g.debug&DEBUG_DB) bu_log("db_lookup(%s) x%x\n", name, dp);
			return(dp);
		}
	}

	if(noisy || rt_g.debug&DEBUG_DB) bu_log("db_lookup(%s) failed: %s does not exist\n", name, name);
	return( DIR_NULL );
}

/*
 *			D B _ D I R A D D
 *
 * Add an entry to the directory.
 * Try to make the regular path through the code as fast as possible,
 * to speed up building the table of contents.
 */
struct directory *
db_diradd( dbip, name, laddr, len, flags, ptr )
register struct db_i	*dbip;
register CONST char	*name;
long			laddr;
int			len;
int			flags;
genptr_t		ptr;		/* unused client_data from db_scan() */
{
	struct directory **headp;
	register struct directory *dp;

	RT_CK_DBI(dbip);

	if(rt_g.debug&DEBUG_DB)  {
		bu_log("db_diradd(dbip=x%x, name='%s', addr=x%x, len=%d, flags=x%x)\n",
			dbip, name, laddr, len, flags );
	}

	if( strchr( name, '/' ) != NULL )  {
		bu_log("db_diradd() object named '%s' is illegal, ignored\n", name );
		return DIR_NULL;
	}

	/* Compute hash only once */
	headp = &(dbip->dbi_Head[db_dirhash(name)]);

	/* Use inline version of db_lookup here, to save re-hash */
	{
		register char	n0 = name[0];
		register char	n1 = name[1];
		for(dp = *headp; dp != DIR_NULL; dp=dp->d_forw )  {
			register char	*this;
			if(
				n0 == *(this=dp->d_namep)  &&	/* speed */
				n1 == this[1]  &&	/* speed */
				strcmp( name, this ) == 0
			)  {
				/* Name exists in directory already */
				char		local[NAMESIZE+2+2];
				register int	c;

				/* Shift right two characters */
				/* Don't truncate to NAMESIZE, name is just internal */
				strncpy( local+2, name, NAMESIZE );
				local[1] = '_';			/* distinctive separater */
				local[NAMESIZE+2] = '\0';	/* ensure null termination */

				for( c = 'A'; c <= 'Z'; c++ )  {
					local[0] = c;
					if( db_lookup( dbip, local, 0 ) == DIR_NULL )
						break;
				}
				if( c > 'Z' )  {
					bu_log("db_diradd: Duplicate of name '%s', ignored\n",
						local );
					return( DIR_NULL );
				}
				bu_log("db_diradd: Duplicate of '%s', given temporary name '%s'\n",
					name, local );
				/* Use recursion to simplify the code */
				return db_diradd( dbip, local, laddr, len, flags, ptr );
			}
		}
	}

	/* 'name' not found in directory, add it */
	BU_GETSTRUCT( dp, directory );		/* calls bu_malloc */
	dp->d_magic = RT_DIR_MAGIC;
	dp->d_namep = bu_strdup( name );	/* calls bu_malloc */
	dp->d_un.file_offset = laddr;
	dp->d_flags = flags & ~(RT_DIR_INMEM);
	dp->d_len = len;
	dp->d_forw = *headp;
	BU_LIST_INIT( &dp->d_use_hd );
	*headp = dp;
	return( dp );
}

/*
 *			D B _ I N M E M
 *
 *  Transmogrify an existing directory entry to be an in-memory-only
 *  one, stealing the external representation from 'ext'.
 */
void
db_inmem( dp, ext, flags )
struct directory	*dp;
struct bu_external	*ext;
int			flags;
{
	BU_CK_EXTERNAL(ext);
	RT_CK_DIR(dp);

	if( dp->d_flags & RT_DIR_INMEM )
		bu_free( dp->d_un.ptr, "db_inmem() ext ptr" );
	dp->d_un.ptr = ext->ext_buf;
	dp->d_len = ext->ext_nbytes / 128;	/* DB_MINREC granule size */
	dp->d_flags = flags | RT_DIR_INMEM;

	/* Empty out the external structure, but leave it w/valid magic */
	ext->ext_buf = (genptr_t)NULL;
	ext->ext_nbytes = 0;
}

/*
 *  			D B _ D I R D E L E T E
 *
 *  Given a pointer to a directory entry, remove it from the
 *  linked list, and free the associated memory.
 *
 *  It is the responsibility of the caller to have released whatever
 *  structures have been hung on the d_use_hd bu_list, first.
 *
 *  Returns -
 *	 0	on success
 *	-1	on failure
 */
int
db_dirdelete( dbip, dp )
register struct db_i		*dbip;
register struct directory	*dp;
{
	register struct directory *findp;
	register struct directory **headp;

	RT_CK_DBI(dbip);
	RT_CK_DIR(dp);

	headp = &(dbip->dbi_Head[db_dirhash(dp->d_namep)]);

	if( dp->d_flags & RT_DIR_INMEM )
	{
		if( dp->d_un.ptr != NULL )
			bu_free( dp->d_un.ptr, "db_dirdelete() inmem ptr" );
	}

	if( *headp == dp )  {
		bu_free( dp->d_namep, "dir name" );
		*headp = dp->d_forw;
		bu_free( (char *)dp, "struct directory" );
		return(0);
	}
	for( findp = *headp; findp != DIR_NULL; findp = findp->d_forw )  {
		if( findp->d_forw != dp )
			continue;
		bu_free( dp->d_namep, "dir name" );
		findp->d_forw = dp->d_forw;
		bzero( (char *)dp, sizeof(struct directory) );	/* sanity */
		bu_free( (char *)dp, "struct directory" );
		return(0);
	}
	return(-1);
}

/*
 *			D B _ R E N A M E
 *
 *  Change the name string of a directory entry.
 *  Because of the hashing function, this takes some extra work.
 *
 *  Returns -
 *	 0	on success
 *	-1	on failure
 */
int
db_rename( dbip, dp, newname )
register struct db_i		*dbip;
register struct directory	*dp;
CONST char			*newname;
{
	register struct directory *findp;
	register struct directory **headp;

	RT_CK_DBI(dbip);
	RT_CK_DIR(dp);

	/* Remove from linked list */
	headp = &(dbip->dbi_Head[db_dirhash(dp->d_namep)]);
	if( *headp == dp )  {
		/* Was first on list, dequeue */
		*headp = dp->d_forw;
	} else {
		for( findp = *headp; findp != DIR_NULL; findp = findp->d_forw )  {
			if( findp->d_forw != dp )
				continue;
			/* Dequeue */
			findp->d_forw = dp->d_forw;
			goto out;
		}
		return(-1);		/* ERROR: can't find */
	}

out:
	/* Effect new name */
	bu_free( dp->d_namep, "d_namep" );
	dp->d_namep = bu_strdup( newname );

	/* Add to new linked list */
	headp = &(dbip->dbi_Head[db_dirhash(newname)]);
	dp->d_forw = *headp;
	*headp = dp;
	return(0);
}

/*
 *			D B _ P R _ D I R
 *
 *  For debugging, print the entire contents of the database directory.
 */
void
db_pr_dir( dbip )
register CONST struct db_i	*dbip;
{
	register CONST struct directory *dp;
	register char		*flags;
	register int		i;

	RT_CK_DBI(dbip);

	bu_log("db_pr_dir(x%x):  Dump of directory for file %s [%s]\n",
		dbip, dbip->dbi_filename,
		dbip->dbi_read_only ? "READ-ONLY" : "Read/Write" );

	bu_log("Title = %s\n", dbip->dbi_title);
	/* units ? */

	for( i = 0; i < RT_DBNHASH; i++ )  {
		for( dp = dbip->dbi_Head[i]; dp != DIR_NULL; dp=dp->d_forw )  {
			if( dp->d_flags & DIR_SOLID )
				flags = "SOL";
			else if( (dp->d_flags & (DIR_COMB|DIR_REGION)) ==
			    (DIR_COMB|DIR_REGION) )
				flags = "REG";
			else if( (dp->d_flags & (DIR_COMB|DIR_REGION)) ==
			    DIR_COMB )
				flags = "COM";
			else
				flags = "Bad";
			bu_log("x%.8x %s %s=x%.8x len=%.5d use=%.2d nref=%.2d %s",
				dp,
				flags,
				dp->d_flags & RT_DIR_INMEM ? "  ptr " : "d_addr",
				dp->d_addr,
				dp->d_len,
				dp->d_uses,
				dp->d_nref,
				dp->d_namep );
			if( dp->d_animate )
				bu_log(" anim=x%x\n", dp->d_animate );
			else
				bu_log("\n");
		}
	}
}
