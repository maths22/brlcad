/*
 *			D I R . C
 *
 * Functions -
 *	db_open		Open the database
 *	dir_build	Build directory of object file
 *	dir_print	Print table-of-contents of object file
 *	lookup		Convert an object name into directory pointer
 *	dir_add		Add entry to the directory
 *	strdup		Duplicate a string in dynamic memory
 *	dir_delete	Delete entry from directory
 *	db_delete	Delete entry from database
 *	db_getrec	Get a record from database
 *	db_putrec	Put record to database
 *	db_alloc	Find a contiguous block of database storage
 *	db_grow		Increase size of existing database entry
 *	db_trunc	Decrease size of existing entry, from it's end
 *	f_memprint	Debug, print memory & db free maps
 *	conversions	Builds conversion factors given a local unit
 *	dir_units	Changes the local unit
 *	dir_title	Change the target title
 *
 * Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include	<fcntl.h>
#include	<stdio.h>
#include	<string.h>
#include "ged_types.h"
#include "db.h"
#include "ged.h"
#include "solid.h"
#include "dir.h"
#include "vmath.h"
#include "dm.h"

extern int	read();
extern long	lseek();
extern char	*malloc();

static struct directory *DirHead = DIR_NULL;

static struct mem_map *dbfreep = MAP_NULL; /* map of free granules */

static long	objfdend;		/* End+1 position of object file */
int		objfd;			/* FD of object file */

union record	record;

double	local2base, base2local;		/* unit conversion factors */
int	localunit;			/* local unit currently in effect */
static char *units_str[] = {
	"none",
	"mm",
	"cm",
	"meters",
	"inches",
	"feet",
	"extra"
};

char	cur_title[128];			/* current target title */

char	*filename;			/* Name of database file */
static int read_only = 0;		/* non-zero when read-only */

/*
 *  			D B _ O P E N
 */
db_open( name )
char *name;
{
	if( (objfd = open( name, O_RDWR )) < 0 )  {
		if( (objfd = open( name, O_RDONLY )) < 0 )  {
			perror( name );
			exit(2);		/* NOT finish */
		}
		(void)printf("%s: READ ONLY\n", name);
		read_only = 1;
	}
	filename = name;
}

/*
 *			D I R _ B U I L D
 *
 * This routine reads through the 3d object file and
 * builds a directory of the object names, to allow rapid
 * named access to objects.
 */
void
dir_build()  {
	static long	addr;

	(void)lseek( objfd, 0L, 0 );
	(void)read( objfd, (char *)&record, sizeof record );
	if( record.u_id != ID_IDENT )  {
		(void)printf("Warning:  File is not a proper GED database\n");
		(void)printf("This database should be converted before further use.\n");
		localunit = 0;
		local2base = base2local = 1.0;
	} else {
		if( strcmp( record.i.i_version, ID_VERSION) != 0 )  {
			(void)printf("File is Version %s, Program is version %s\n",
				record.i.i_version, ID_VERSION );
			(void)printf("This database should be converted before further use.\n");
			localunit = 0;
			local2base = base2local = 1.0;
		} else {
			/* get the unit conversion factors */
			localunit = record.i.i_units;
			conversions( record.i.i_units );
		}
		/* save the title */
		cur_title[0] = '\0';
		strcat(cur_title, record.i.i_title);
	}

	(void)lseek( objfd, 0L, 0 );


	while(1)  {
		addr = lseek( objfd, 0L, 1 );
		if( (unsigned)read( objfd, (char *)&record, sizeof record )
				!= sizeof record )
			break;

		if( record.u_id == ID_IDENT )  {
			(void)printf("%s (units=%s)\n",
				record.i.i_title,
				units_str[record.i.i_units] );
			continue;
		}
		if( record.u_id == ID_FREE )  {
			/* Ought to inform db manager of avail. space */
			memfree( &dbfreep, 1, addr/sizeof(union record) );
			continue;
		}
		if( record.u_id == ID_ARS_A )  {
			dir_add( record.a.a_name, addr, DIR_SOLID, record.a.a_totlen+1 );

			/* Skip remaining B type records.	*/
			(void)lseek( objfd,
				(long)(record.a.a_totlen) *
				(long)(sizeof record),
				1 );
			continue;
		}

		if( record.u_id == ID_SOLID )  {
			dir_add( record.s.s_name, addr, DIR_SOLID, 1 );
			continue;
		}
		if( record.u_id == ID_P_HEAD )  {
			union record rec;
			register int nrec;
			register int j;
			nrec = 1;
			while(1) {
				j = read( objfd, (char *)&rec, sizeof(rec) );
				if( j != sizeof(rec) )
					break;
				if( rec.u_id != ID_P_DATA )  {
					lseek( objfd, -(sizeof(rec)), 1 );
					break;
				}
				nrec++;
			}
			dir_add( record.p.p_name, addr, DIR_SOLID, nrec );
			continue;
		}
		if( record.u_id != ID_COMB )  {
			(void)printf( "dir_build:  unknown record %c (0%o)\n",
				record.u_id, record.u_id );
			/* skip this record */
			continue;
		}

		dir_add( record.c.c_name,
			addr,
			record.c.c_flags == 'R' ?
				DIR_COMB|DIR_REGION : DIR_COMB,
			record.c.c_length+1 );
		/* Skip over member records */
		(void)lseek( objfd,
			(long)record.c.c_length * (long)sizeof record,
			1 );
	}

	/* Record current end of objects file */
	objfdend = lseek( objfd, 0L, 1 );
}

/*
 *			D I R _ P R I N T
 *
 * This routine lists the names of all the objects accessible
 * in the object file.
 */
void
dir_print()  {
	struct directory	*dp;
	register char	*cp;		/* -> name char to output */
	register int	count;		/* names listed on current line */
	register int	len;		/* length of previous name */

#define	COLUMNS	((80 + NAMESIZE - 1) / NAMESIZE)

	count = 0;
	len = 0;
	for( dp = DirHead; dp != DIR_NULL; dp = dp->d_forw )  {
		if ( (cp = dp->d_namep)[0] == '\0' )
			continue;	/* empty slot */

		/* Tab to start column. */
		if ( count != 0 )
			do
				(void)putchar( '\t' );
			while ( (len += 8) < NAMESIZE );

		/* Output name and save length for next tab. */
		len = 0;
		do {
			(void)putchar( *cp++ );
			++len;
		}  while ( *cp != '\0' );
		if( dp->d_flags & DIR_COMB )  {
			(void)putchar( '/' );
			++len;
		}
		if( dp->d_flags & DIR_REGION )  {
			(void)putchar( 'R' );
			++len;
		}

		/* Output newline if last column printed. */
		if ( ++count == COLUMNS )  {	/* line now full */
			(void)putchar( '\n' );
			count = 0;
		}
	}
	/* No more names. */
	if ( count != 0 )		/* partial line */
		(void)putchar( '\n' );
#undef	COLUMNS
}

/*
 *			D I R _ L O O K U P
 *
 * This routine takes a name, and looks it up in the
 * directory table.  If the name is present, a pointer to
 * the directory struct element is returned, otherwise
 * NULL is returned.
 *
 * If noisy is non-zero, a print occurs, else only
 * the return code indicates failure.
 */
struct directory *
lookup( str, noisy )
register char *str;
{
	register struct directory *dp;

	for( dp = DirHead; dp != DIR_NULL; dp=dp->d_forw )  {
		if ( strcmp( str, dp->d_namep ) == 0 )
			return(dp);
	}

	if( noisy )
		(void)printf("dir_lookup:  could not find '%s'\n", str );
	return( DIR_NULL );
}

/*
 *			D I R _ A D D
 *
 * Add an entry to the directory
 */
struct directory *
dir_add( name, laddr, flags, len )
register char *name;
long laddr;
{
	register struct directory *dp;

	GETSTRUCT( dp, directory );
	if( dp == DIR_NULL )
		return( DIR_NULL );
	dp->d_namep = strdup( name );
	dp->d_addr = laddr;
	dp->d_flags = flags;
	dp->d_len = len;
	dp->d_forw = DirHead;
	DirHead = dp;
	return( dp );
}

/*
 *			S T R D U P
 *
 * Given a string, allocate enough memory to hold it using malloc(),
 * duplicate the strings, returns a pointer to the new string.
 */
char *
strdup( cp )
register char *cp;
{
	register char	*base;
	register char	*current;

	if( (base = malloc( strlen(cp)+1 )) == (char *)0 )  {
		(void)printf("strdup:  unable to allocate memory");
		return( (char *) 0);
	}

	current = base;
	do  {
		*current++ = *cp;
	}  while( *cp++ != '\0' );

	return(base);
}

/*
 *  			D I R _ D E L E T E
 *
 *  Given a pointer to a directory entry, remove it from the
 *  linked list, and free the associated memory.
 */
void
dir_delete( dp )
register struct directory *dp;
{
	register struct directory *findp;

	if( DirHead == dp )  {
		free( dp->d_namep );
		DirHead = dp->d_forw;
		free( dp );
		return;
	}
	for( findp = DirHead; findp != DIR_NULL; findp = findp->d_forw )  {
		if( findp->d_forw != dp )
			continue;
		free( dp->d_namep );
		findp->d_forw = dp->d_forw;
		free( dp );
		return;
	}
	(void)printf("dir_delete:  unable to find %s\n", dp->d_namep );
}

/*
 *  			D B _ D E L E T E
 *  
 *  Delete the indicated database record(s).
 *  Mark all records with ID_FREE.
 */
static union record zapper;

void
db_delete( dp )
struct directory *dp;
{
	register int i;

	zapper.u_id = ID_FREE;	/* The rest will be zeros */

	for( i=0; i < dp->d_len; i++ )
		db_putrec( dp, &zapper, i );
	memfree( &dbfreep, dp->d_len, dp->d_addr/(sizeof(union record)) );
	dp->d_len = 0;
	dp->d_addr = -1;
}

/*
 *  			D B _ G E T R E C
 *
 *  Retrieve a record from the database,
 *  "offset" granules into this entry.
 */
void
db_getrec( dp, where, offset )
struct directory *dp;
union record *where;
{
	register int i;

	if( offset < 0 || offset >= dp->d_len )  {
		(void)printf("db_getrec(%s):  offset %d exceeds %d\n",
			dp->d_namep, offset, dp->d_len );
		where->u_id = '\0';	/* undefined id */
		return;
	}
	(void)lseek( objfd, dp->d_addr + offset * sizeof(union record), 0 );
	i = read( objfd, (char *)where, sizeof(union record) );
	if( i != sizeof(union record) )  {
		perror("db_getrec");
		(void)printf("db_getrec(%s):  read error.  Wanted %d, got %d\n",
			dp->d_namep, sizeof(union record), i );
		where->u_id = '\0';	/* undefined id */
	}
}

/*
 *  			D B _ G E T M A N Y
 *
 *  Retrieve several records from the database,
 *  "offset" granules into this entry.
 */
void
db_getmany( dp, where, offset, len )
struct directory *dp;
union record *where;
{
	register int i;

	if( offset < 0 || offset+len >= dp->d_len )  {
		(void)printf("db_getmany(%s):  xfer %d..%x exceeds 0..%d\n",
			dp->d_namep, offset, offset+len, dp->d_len );
		where->u_id = '\0';	/* undefined id */
		return;
	}
	(void)lseek( objfd, dp->d_addr + offset * sizeof(union record), 0 );
	i = read( objfd, (char *)where, len * sizeof(union record) );
	if( i != len * sizeof(union record) )  {
		perror("db_getmany");
		(void)printf("db_getmany(%s):  read error.  Wanted %d, got %d\n",
			dp->d_namep, len * sizeof(union record), i );
		where->u_id = '\0';	/* undefined id */
	}
}

/*
 *  			D B _ P U T R E C
 *
 *  Store a single record in the database,
 *  "offset" granules into this entry.
 */
void
db_putrec( dp, where, offset )
register struct directory *dp;
union record *where;
int offset;
{
	register int i;

	if( read_only )  {
		(void)printf("db_putrec on READ-ONLY file\n");
		return;
	}
	if( offset < 0 || offset >= dp->d_len )  {
		(void)printf("db_putrec(%s):  offset %d exceeds %d\n",
			dp->d_namep, offset, dp->d_len );
		return;
	}
	(void)lseek( objfd, dp->d_addr + offset * sizeof(union record), 0 );
	i = write( objfd, (char *)where, sizeof(union record) );
	if( i != sizeof(union record) )  {
		perror("db_putrec");
		(void)printf("db_putrec(%s):  write error\n", dp->d_namep );
	}
}

/*
 *  			D B _ A L L O C
 *  
 *  Find a block of database storage of "count" granules.
 */
void
db_alloc( dp, count )
register struct directory *dp;
int count;
{
	unsigned long addr;
	union record rec;

	if( read_only )  {
		(void)printf("db_alloc on READ-ONLY file\n");
		return;
	}
	if( count <= 0 )  {
		(void)printf("db_alloc(0)\n");
		return;
	}
top:
	if( (addr = memalloc( &dbfreep, count )) == 0L )  {
		/* No contiguous free block, append to file */
		dp->d_addr = objfdend;
		dp->d_len = count;
		objfdend += count * sizeof(union record);
		return;
	}
	dp->d_addr = addr * sizeof(union record);
	dp->d_len = count;
	db_getrec( dp, &rec, 0 );
	if( rec.u_id != ID_FREE )  {
		(void)printf("db_alloc():  addr %ld non-FREE (id %d), skipping\n",
			addr, rec.u_id );
		goto top;
	}
}

/*
 *  			D B _ G R O W
 *  
 *  Increase the database size of an object by "count",
 *  by duplicating in a new area if necessary.
 */
db_grow( dp, count )
register struct directory *dp;
int count;
{
	register int i;
	union record rec;
	struct directory olddir;
	unsigned long addr;

	if( read_only )  {
		(void)printf("db_grow on READ-ONLY file\n");
		return;
	}

	/* Easy case -- see if at end-of-file */
	if( dp->d_addr + dp->d_len * sizeof(union record) == objfdend )  {
		objfdend += count * sizeof(union record);
		dp->d_len += count;
		return;
	}

	/* Try to extend into free space immediately following current obj */
	if( memget( &dbfreep, count, dp->d_addr/sizeof(union record) ) == 0L )
		goto hard;

	/* Check to see if granules are all really availible (sanity check) */
	for( i=0; i < count; i++ )  {
		(void)lseek( objfd, dp->d_addr +
			((dp->d_len + i) * sizeof(union record)), 0 );
		(void)read( objfd, (char *)&rec, sizeof(union record) );
		if( rec.u_id != ID_FREE )  {
			(void)printf("db_grow:  FREE record wasn't?! (id%d)\n",
				rec.u_id);
			goto hard;
		}
	}
	dp->d_len += count;
	return;
hard:
	/* Sigh, got to duplicate it in some new space */
	olddir = *dp;				/* struct copy */
	db_alloc( dp, dp->d_len + count );	/* fixes addr & len */
	for( i=0; i < olddir.d_len; i++ )  {
		db_getrec( &olddir, &rec, i );
		db_putrec( dp, &rec, i );
	}
	/* Release space that original copy consumed */
	db_delete( &olddir );
}

/*
 *  			D B _ T R U N C
 *  
 *  Remove "count" granules from the indicated database entry.
 *  Stomp on them with ID_FREE's.
 *  Later, we will add them to a freelist.
 */
void
db_trunc( dp, count )
register struct directory *dp;
int count;
{
	register int i;

	if( read_only )  {
		(void)printf("db_trunc on READ-ONLY file\n");
		return;
	}
	zapper.u_id = ID_FREE;	/* The rest will be zeros */

	for( i = 0; i < count; i++ )
		db_putrec( dp, &zapper, (dp->d_len - 1) - i );
	dp->d_len -= count;
}

/*
 *			F _ M E M P R I N T
 *  
 *  Debugging aid:  dump memory maps
 */
void
f_memprint()
{
	(void)printf("Display manager free map:\n");
	memprint( &(dmp->dmr_map) );
	(void)printf("Database free granule map:\n");
	memprint( &dbfreep );
}



/*	builds conversion factors given the local unit
 */
conversions( local )
int local;
{

	/* Base unit is MM */
	switch( local ) {

	case ID_NO_UNIT:
		/* no local unit specified ... use the base unit */
		localunit = record.i.i_units = ID_MM_UNIT;
		local2base = 1.0;
		break;

	case ID_MM_UNIT:
		/* local unit is mm */
		local2base = 1.0;
		break;

	case ID_CM_UNIT:
		/* local unit is cm */
		local2base = 10.0;		/* CM to MM */
		break;

	case ID_M_UNIT:
		/* local unit is meters */
		local2base = 1000.0;		/* M to MM */
		break;

	case ID_IN_UNIT:
		/* local unit is inches */
		local2base = 25.4;		/* IN to MM */
		break;

	case ID_FT_UNIT:
		/* local unit is feet */
		local2base = 304.8;		/* FT to MM */
		break;

	default:
		local2base = 1.0;
		localunit = 6;
		break;
	}
	base2local = 1.0 / local2base;
}

/* change the local unit of the description */
dir_units( new_unit )
int new_unit;
{

	if( read_only ) {
		(void)printf("Read only file\n");
		return;
	}

	(void)lseek(objfd, 0L, 0);
	(void)read(objfd, (char *)&record, sizeof record);

	if(record.u_id != ID_IDENT) {
		(void)printf("NOT a proper GED file\n");
		return;
	}

	(void)lseek(objfd, 0L, 0);

	record.i.i_units = new_unit;
	conversions( new_unit );

	(void)write(objfd, (char *)&record, sizeof record);

}



/* change the title of the description */
dir_title( )
{

	if( read_only ) {
		(void)printf("Read only file\n");
		return;
	}

	(void)lseek(objfd, 0L, 0);
	(void)read(objfd, (char *)&record, sizeof record);

	if(record.u_id != ID_IDENT) {
		(void)printf("NOT a proper GED file\n");
		return;
	}

	(void)lseek(objfd, 0L, 0);

	record.i.i_title[0] = '\0';

	strcat(record.i.i_title, cur_title);

	(void)write(objfd, (char *)&record, sizeof record);
}
