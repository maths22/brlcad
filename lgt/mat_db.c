/*
	Author:		Gary S. Moss
			U. S. Army Ballistic Research Laboratory
			Aberdeen Proving Ground
			Maryland 21005-5066
			(301)278-6647 or AV-298-6647
*/
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif
/*
	Originally extracted from SCCS archive:
		SCCS id:	@(#) mat_db.c	1.10
		Last edit: 	2/4/87 at 12:18:02
		Retrieved: 	2/4/87 at 12:18:20
		SCCS archive:	/vld/moss/src/libmatdb/s.mat_db.c
*/

#include <stdio.h>
#include "./vecmath.h"
#include "./mat_db.h"
#include "./screen.h"
#include "./extern.h"
static FILE		*matdb_fp;
static Mat_Db_Entry	mat_db_table[MAX_MAT_DB];
static int		mat_db_size = 0;
Mat_Db_Entry		mat_dfl_entry =
				{
				0,		/* Material id.		*/
				4,		/* Shininess.		*/
				0.6,		/* Specular weight.	*/
				0.4,		/* Diffuse weight.	*/
				0.0,		/* Reflectivity.	*/
				0.0,		/* Transmission.	*/
				1.0,		/* Refractive index.	*/
				255, 255, 255,	/* Diffuse RGB values.	*/
				MF_USED,	/* Mode flag.		*/
				"(default)"	/* Material name.	*/
				};
Mat_Db_Entry		mat_nul_entry =
				{
				0,		/* Material id.		*/
				0,		/* Shininess.		*/
				0.0,		/* Specular weight.	*/
				0.0,		/* Diffuse weight.	*/
				0.0,		/* Reflectivity.	*/
				0.0,		/* Transmission.	*/
				0.0,		/* Refractive index.	*/
				0, 0, 0,	/* Diffuse RGB values.	*/
				MF_NULL,	/* Mode flag.		*/
				"(null)"	/* Material name.	*/
				};
_LOCAL_ int	get_Entry(), put_Entry();
_LOCAL_ int	mat_W_Open();

/*	m a t _ O p e n _ D b ( )
	Open file, return file pointer or NULL.
 */
FILE *
mat_Open_Db( mat_db_file )
char	*mat_db_file;
	{	register	Mat_Db_Entry	*entry;
	if( (matdb_fp = fopen( mat_db_file, "r+" )) == (FILE *) NULL )
		return	matdb_fp; /* NULL */
	/* Mark all entries as NULL.					*/
	for( entry = mat_db_table; entry < &mat_db_table[MAX_MAT_DB]; entry++ )
		entry->mode_flag = MF_NULL;
	return	matdb_fp;
	}

/*	m a t _ C l o s e _ D b ( )
	Close material database input file.  Return success or failure.
 */
mat_Close_Db()
	{
	return	fclose( matdb_fp );
	}

/*	m a t _ P r i n t _ D b ( )
	Print material database entry.
 */
mat_Print_Db( material_id )
int		material_id;
	{	register Mat_Db_Entry	*entry;
		register int		stop;
		int			lines =	(PROMPT_LINE-TOP_SCROLL_WIN);
	if( material_id >= MAX_MAT_DB )
		{
		rt_log( "Material data base only has %d entries.\n",
			MAX_MAT_DB
			);
		rt_log( "If this is not enough, notify the support staff.\n" );
		return	-1;
		}
	else
	if( material_id < 0 )
		{
		stop = MAX_MAT_DB - 1;
		material_id = 0;
		}
	else
		stop = material_id;
	for( ; material_id <= stop; material_id++, lines-- )
		{
		entry = &mat_db_table[material_id];
		if( entry->mode_flag == MF_NULL )
			continue;
		if( lines <= 0 && ! do_More( &lines ) )
			break;
		prnt_Scroll( "\n" );
		if( --lines <= 0 && ! do_More( &lines ) )
			break;
		prnt_Scroll( "MATERIAL [%d] %s\n",
				material_id,
				entry->name[0] == '\0' ? "(untitled)" : entry->name
				);
		if( --lines <= 0 && ! do_More( &lines ) )
			break;
		prnt_Scroll( "        shininess\t\t(%d)\n", entry->shine );
		if( --lines <= 0 && ! do_More( &lines ) )
			break;
		prnt_Scroll( "        specular weight\t\t(%g)\n", entry->wgt_specular );
		if( --lines <= 0 && ! do_More( &lines ) )
			break;
		prnt_Scroll( "        diffuse weight\t\t(%g)\n", entry->wgt_diffuse );
		if( --lines <= 0 && ! do_More( &lines ) )
			break;
		prnt_Scroll( "        transparency\t\t(%g)\n", entry->transparency );
		if( --lines <= 0 && ! do_More( &lines ) )
			break;
		prnt_Scroll( "        reflectivity\t\t(%g)\n", entry->reflectivity );
		if( --lines <= 0 && ! do_More( &lines ) )
			break;
		prnt_Scroll( "        refractive index\t(%g)\n", entry->refrac_index );
		if( --lines <= 0 && ! do_More( &lines ) )
			break;
		prnt_Scroll( "        diffuse color\t\t(%d %d %d)\n",
				entry->df_rgb[0],
				entry->df_rgb[1],
				entry->df_rgb[2]
				);
		}
	return	1;
	}

/*	m a t _ R d _ D b ( )
	Read ASCII material database into table, return number of entries.
 */
mat_Rd_Db()
	{	Mat_Db_Entry	*entry;
	mat_db_size = 0;
	for(	entry = mat_db_table;
		entry < &mat_db_table[MAX_MAT_DB]
	     && get_Entry( entry );
		++entry
		)
		mat_db_size++;
	return	mat_db_size;
	}


/*	m a t _ S a v e _ D b ( )
	Write ASCII material database from table.  If 'file' is NULL,
	overwrite input file, otherwise open or create 'file'.  Return success
	or failure.
 */
mat_Save_Db( file )
char	*file;
	{	register Mat_Db_Entry	*entry;
	if( ! mat_W_Open( file ) )
		return	0;
	for(	entry = mat_db_table;
		entry < &mat_db_table[mat_db_size]
	     && put_Entry( entry );
		++entry
		)
		;
	if( entry != &mat_db_table[mat_db_size] )
		return	0;
	(void) fflush( matdb_fp );
	return	1;
	}


/*	m a t _ P u t _ D b _ E n t r y ( )
	Create or overwrite entry in material table.
 */
mat_Put_Db_Entry( material_id )
int	material_id;
	{	register Mat_Db_Entry	*entry;
		static char		input_buf[BUFSIZ];
		int			red, grn, blu;
	if( material_id < 0 )
		return	-1;
	if( material_id < MAX_MAT_DB )
		{
		entry = &mat_db_table[material_id];
		entry->id = material_id;
		}
	else
		{
		(void) fprintf( stderr,
			"Material table full, MAX_DB_ENTRY too small.\n"
				);
		return	0;
		}
	if( get_Input( input_buf, BUFSIZ, "material name : " ) == NULL )
		return	0;
	if( input_buf[0] != '\0' )
		(void) strncpy( entry->name, input_buf, MAX_MAT_NM );
	if( get_Input( input_buf, BUFSIZ, "shine : " ) == NULL )
		return	0;
	if( input_buf[0] != '\0' )
		(void) sscanf( input_buf, "%d", &entry->shine );
	if( get_Input( input_buf, BUFSIZ, "specular weighting [0.0 .. 1.0] : " ) == NULL )
		return	0;
	if( input_buf[0] != '\0' )
#ifdef sgi
		(void) sscanf( input_buf, "%f", &entry->wgt_specular );
#else
		(void) sscanf( input_buf, "%lf", &entry->wgt_specular );
#endif
	if( get_Input( input_buf, BUFSIZ, "diffuse weighting [0.0 .. 1.0] : " ) == NULL )
		return	0;
	if( input_buf[0] != '\0' )
#ifdef sgi
		(void) sscanf( input_buf, "%f", &entry->wgt_diffuse );
#else
		(void) sscanf( input_buf, "%lf", &entry->wgt_diffuse );
#endif
	if( get_Input( input_buf, BUFSIZ, "transparency [0.0 .. 1.0] : " ) == NULL )
		return	0;
	if( input_buf[0] != '\0' )
#ifdef sgi
		(void) sscanf( input_buf, "%f", &entry->transparency );
#else	
		(void) sscanf( input_buf, "%lf", &entry->transparency );
#endif
	if( get_Input( input_buf, BUFSIZ, "reflectivity [0.0 .. 1.0] : " ) == NULL )
		return	0;
	if( input_buf[0] != '\0' )
#ifdef sgi
		(void) sscanf( input_buf, "%f", &entry->reflectivity );
#else
		(void) sscanf( input_buf, "%lf", &entry->reflectivity );
#endif
	if( get_Input( input_buf, BUFSIZ, "refractive index [0.9 .. 5.0] : " ) == NULL )
		return	0;
	if( input_buf[0] != '\0' )
#ifdef sgi
		(void) sscanf( input_buf, "%f", &entry->refrac_index );
#else
		(void) sscanf( input_buf, "%lf", &entry->refrac_index );
#endif
	if( get_Input( input_buf, BUFSIZ, "diffuse RGB values [0 .. 255] : " ) == NULL )
		return	0;
	if(	input_buf[0] != '\0'
	     &&	sscanf( input_buf, "%d %d %d", &red, &grn, &blu ) == 3
		)
		{
		entry->df_rgb[0] = red;
		entry->df_rgb[1] = grn;
		entry->df_rgb[2] = blu;
		}
	entry->mode_flag = MF_USED;
	mat_db_size = Max( mat_db_size, material_id+1 );
	return	1;
	}

/*	m a t _ G e t _ D b _ E n t r y ( )
	Return pointer to entry indexed by id or NULL.
 */
Mat_Db_Entry *
mat_Get_Db_Entry( material_id )
int	material_id;
	{
	if( material_id < 0 )
		return	MAT_DB_NULL;
	if( material_id < mat_db_size )
		return	&mat_db_table[material_id];
	else
		return	MAT_DB_NULL;
	}

_LOCAL_
get_Entry( entry )
register Mat_Db_Entry	*entry;
	{	register char	*ptr;
		int		items;
		int		red, grn, blu, mode;
	if( fgets( entry->name, MAX_MAT_NM, matdb_fp ) == NULL )
		return	0;
	ptr = &entry->name[strlen(entry->name) - 1];
	if( *ptr == '\n' )
		/* Read entire line.					*/
		*ptr = '\0';
	else	/* Skip rest of line.					*/
		while( getc( matdb_fp ) != '\n' )
			;
	if( (items = fscanf( matdb_fp, "%d %d", &entry->id, &entry->shine )) != 2 )
		{
		(void) fprintf( stderr, "Could not read integers (%d read)!\n", items );
		return	0;
		}
	if(	fscanf(	matdb_fp,
#ifdef sgi
			"%f %f %f %f %f",
#else
			"%lf %lf %lf %lf %lf",
#endif
			&entry->wgt_specular,
			&entry->wgt_diffuse,
			&entry->transparency,
			&entry->reflectivity,
			&entry->refrac_index
			) != 5
		)
		{
		(void) fprintf( stderr, "Could not read floats!\n" );
		return	0;
		}
	if( fscanf( matdb_fp, "%d %d %d", &red, &grn, &blu ) != 3 )
		{
		(void) fprintf( stderr, "Could not read chars!\n" );
		return	0;
		}
	entry->df_rgb[0] = red;
	entry->df_rgb[1] = grn;
	entry->df_rgb[2] = blu;
	if( fscanf( matdb_fp, "%d", &mode ) != 1 )
		{
		(void) fprintf( stderr,
				"get_Entry(): Could not read mode_flag!\n"
				);
		return	0;
		}
	entry->mode_flag = mode;
	while( getc( matdb_fp ) != '\n' )
		; /* Gobble rest of line.				*/
	return	1;
	}

_LOCAL_
put_Entry( entry )
register Mat_Db_Entry	*entry;
	{
	if( entry->mode_flag == MF_NULL )
		entry = &mat_nul_entry;
	(void) fprintf( matdb_fp, "%s\n", entry->name );
	(void) fprintf( matdb_fp, "%d\n%d\n", entry->id, entry->shine );
	(void) fprintf(	matdb_fp,
			"%f\n%f\n%f\n%f\n%f\n",
			entry->wgt_specular,
			entry->wgt_diffuse,
			entry->transparency,
			entry->reflectivity,
			entry->refrac_index
			);
	(void) fprintf(	matdb_fp,
			"%u %u %u\n",
			(unsigned) entry->df_rgb[0],
			(unsigned) entry->df_rgb[1],
			(unsigned) entry->df_rgb[2]
			);
	(void) fprintf( matdb_fp, "%u\n", (unsigned) entry->mode_flag );
	return	1;
	}

_LOCAL_
mat_W_Open( file )
char	*file;
	{
	if( file != NULL )
		{	register FILE	*fp;
		if( (fp = fopen( file, "w+" )) == NULL )
			return	0;
		else
			{
			matdb_fp = fp;
			setbuf( matdb_fp, malloc( BUFSIZ ) );
			}
		}
	/* else default to last database accessed.			*/
	if( matdb_fp == NULL )
		return	0;
	return	fseek( matdb_fp, 0L, 0 ) == 0;
	}
