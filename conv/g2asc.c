/*
 *		G 2 A S C . C
 *  
 *  This program generates an ASCII data file which contains
 *  a GED database.
 *
 *  Usage:  g2asc < file.g > file.asc
 *  
 *  Author -
 *  	Charles M Kennedy
 *  	Michael J Muuss
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
 
#include <stdio.h>
#include <ctype.h>
#include "db.h"

extern void	exit();
extern int	printf(), fprintf();

char *name();
char *strchop();
#define CH(x)	strchop(x,sizeof(x))

void	idendump(), polyhead(), polydata();
void	soldump(), combdump(), membdump(), arsadump(), arsbdump();
void	materdump(), bspldump(), bsurfdump();

union record	record;		/* GED database record */

main(argc, argv)
char **argv;
{
	/* Read database file */
	while( fread( (char *)&record, sizeof record, 1, stdin ) == 1  &&
	    !feof(stdin) )  {
	    	if( argc > 1 )
			fprintf(stderr,"0%o (%c)\n", record.u_id, record.u_id);
		/* Check record type and skip deleted records */
	    	switch( record.u_id )  {
	    	case ID_FREE:
			continue;
	    	case ID_SOLID:
			(void)soldump();
			continue;
	    	case ID_COMB:
			(void)combdump();
			continue;
	    	case ID_ARS_A:
			(void)arsadump();
	    		continue;
	    	case ID_P_HEAD:
			(void)polyhead();
	    		continue;
	    	case ID_P_DATA:
			(void)polydata();
	    		continue;
	    	case ID_IDENT:
			(void)idendump();
	    		continue;
	    	case ID_MATERIAL:
			materdump();
	    		continue;
	    	case ID_BSOLID:
			bspldump();
	    		continue;
	    	case ID_BSURF:
			bsurfdump();
	    		continue;
	    	default:
			(void)fprintf(stderr,
				"g2asc: unable to convert record type '%c' (0%o), skipping\n",
				record.u_id, record.u_id);
	    		continue;
		}
	}
	exit(0);
}

void
idendump()	/* Print out Ident record information */
{
	(void)printf( "%c %d %.6s\n",
		record.i.i_id,			/* I */
		record.i.i_units,		/* units */
		CH(record.i.i_version)		/* version */
	);
	(void)printf( "%.72s\n",
		CH(record.i.i_title)	/* title or description */
	);

	/* Print a warning message on stderr if versions differ */
	if( strcmp( record.i.i_version, ID_VERSION ) != 0 )  {
		(void)fprintf(stderr,"File is Version %s, Program is version %s\n",
			record.i.i_version, ID_VERSION );
	}
}

void
polyhead()	/* Print out Polyhead record information */
{
	(void)printf("%c ", record.p.p_id );		/* P */
	(void)printf("%.16s", name(record.p.p_name) );	/* unique name */
	(void)printf("\n");			/* Terminate w/ a newline */
}

void
polydata()	/* Print out Polydata record information */
{
	register int i, j;

	(void)printf("%c ", record.q.q_id );		/* Q */
	(void)printf("%d ", record.q.q_count );		/* # of vertices <= 5 */
	for( i = 0; i < 5; i++ )  {			/* [5][3] vertices */
		for( j = 0; j < 3; j++ ) {
			(void)printf("%.12e ", record.q.q_verts[i][j] );
		}
	}
	for( i = 0; i < 5; i++ )  {			/* [5][3] normals */
		for( j = 0; j < 3; j++ ) {
			(void)printf("%.12e ", record.q.q_norms[i][j] );
		}
	}
	(void)printf("\n");			/* Terminate w/ a newline */
}

void
soldump()	/* Print out Solid record information */
{
	register int i;

	(void)printf("%c ", record.s.s_id );	/* S */
	(void)printf("%d ", record.s.s_type );	/* GED primitive type */
	(void)printf("%.16s ", name(record.s.s_name) );	/* unique name */
	(void)printf("%d ", record.s.s_cgtype );/* COMGEOM solid type */
	for( i = 0; i < 24; i++ )
		(void)printf("%.12e ", record.s.s_values[i] ); /* parameters */
	(void)printf("\n");			/* Terminate w/ a newline */
}

void
combdump()	/* Print out Combination record information */
{
	register int i;
	register int length;	/* Keep track of number of members */

	(void)printf("%c ", record.c.c_id );		/* C */
	if( record.c.c_flags == 'R' )			/* set region flag */
		(void)printf("Y ");			/* Y if `R' */
	else
		(void)printf("N ");			/* N if ` ' */
	(void)printf("%.16s ", name(record.c.c_name) );	/* unique name */
	(void)printf("%d ", record.c.c_regionid );	/* region ID code */
	(void)printf("%d ", record.c.c_aircode );	/* air space code */
	(void)printf("%d ", record.c.c_length );	/* # of members */
	(void)printf("%d ", record.c.c_num );		/* COMGEOM region # */
	(void)printf("%d ", record.c.c_material );	/* material code */
	(void)printf("%d ", record.c.c_los );		/* equiv. LOS est. */
	(void)printf("%d %d %d %d ",
		record.c.c_override,
		record.c.c_rgb[0],
		record.c.c_rgb[1],
		record.c.c_rgb[2] );
	if( isascii(record.c.c_matname[0]) && isprint(record.c.c_matname[0]) )  {
		printf("1 ");	/* flag: line 1 follows */
		if( record.c.c_matparm[0] )
			printf("2 ");	/* flag: line 2 follows */
	}
	(void)printf("\n");			/* Terminate w/ a newline */

	if( isascii(record.c.c_matname[0]) && isprint(record.c.c_matname[0]) )  {
		(void)printf("%.32s\n", CH(record.c.c_matname) );
		if( record.c.c_matparm[0] )
			(void)printf("%.60s\n", CH(record.c.c_matparm) );
	}

	length = (int)record.c.c_length;	/* Get # of member records */

	for( i = 0; i < length; i++ )  {
		(void)membdump();
	}
}

void
membdump()	/* Print out Member record information */
{
	register int i;

	/* Read in a member record for processing */
	(void)fread( (char *)&record, sizeof record, 1, stdin );
	(void)printf("%c ", record.M.m_id );		/* M */
	(void)printf("%c ", record.M.m_relation );	/* Boolean oper. */
	(void)printf("%.16s ", name(record.M.m_instname) );	/* referred-to obj. */
	for( i = 0; i < 16; i++ )			/* homogeneous transform matrix */
		(void)printf("%.12e ", record.M.m_mat[i] );
	(void)printf("%d ", record.M.m_num );		/* COMGEOM solid # */
	(void)printf("\n");				/* Terminate w/ nl */
}

void
arsadump()	/* Print out ARS record information */
{
	register int i;
	register int length;	/* Keep track of number of ARS B records */

	(void)printf("%c ", record.a.a_id );	/* A */
	(void)printf("%d ", record.a.a_type );	/* primitive type */
	(void)printf("%.16s ", name(record.a.a_name) );	/* unique name */
	(void)printf("%d ", record.a.a_m );	/* # of curves */
	(void)printf("%d ", record.a.a_n );	/* # of points per curve */
	(void)printf("%d ", record.a.a_curlen );/* # of granules per curve */
	(void)printf("%d ", record.a.a_totlen );/* # of granules for ARS */
	(void)printf("%.12e ", record.a.a_xmax );	/* max x coordinate */
	(void)printf("%.12e ", record.a.a_xmin );	/* min x coordinate */
	(void)printf("%.12e ", record.a.a_ymax );	/* max y coordinate */
	(void)printf("%.12e ", record.a.a_ymin );	/* min y coordinate */
	(void)printf("%.12e ", record.a.a_zmax );	/* max z coordinate */
	(void)printf("%.12e ", record.a.a_zmin );	/* min z coordinate */
	(void)printf("\n");			/* Terminate w/ a newline */
			
	length = (int)record.a.a_totlen;	/* Get # of ARS B records */

	for( i = 0; i < length; i++ )  {
		(void)arsbdump();
	}
}

void
arsbdump()	/* Print out ARS B record information */
{
	register int i;
	
	/* Read in a member record for processing */
	(void)fread( (char *)&record, sizeof record, 1, stdin );
	(void)printf("%c ", record.b.b_id );		/* B */
	(void)printf("%d ", record.b.b_type );		/* primitive type */
	(void)printf("%d ", record.b.b_n );		/* current curve # */
	(void)printf("%d ", record.b.b_ngranule );	/* current granule */
	for( i = 0; i < 24; i++ )  {			/* [8*3] vectors */
		(void)printf("%.12e ", record.b.b_values[i] );
	}
	(void)printf("\n");			/* Terminate w/ a newline */
}

void
materdump()	/* Print out material description record information */
{
	(void)printf( "%c %d %d %d %d %d %d\n",
		record.md.md_id,			/* m */
		record.md.md_flags,			/* UNUSED */
		record.md.md_low,	/* low end of region IDs affected */
		record.md.md_hi,	/* high end of region IDs affected */
		record.md.md_r,
		record.md.md_g,		/* color of regions: 0..255 */
		record.md.md_b );
}

void
bspldump()	/* Print out B-spline solid description record information */
{
	(void)printf( "%c %.16s %d %.12e\n",
		record.B.B_id,		/* b */
		name(record.B.B_name),	/* unique name */
		record.B.B_nsurf,	/* # of surfaces in this solid */
		record.B.B_resolution );	/* resolution of flatness */
}

void
bsurfdump()	/* Print d-spline surface description record information */
{
	register int i;
	register float *vp;
	int nbytes, count;
	float *fp;

	(void)printf( "%c %d %d %d %d %d %d %d %d %d\n",
		record.d.d_id,		/* D */
		record.d.d_order[0],	/* order of u and v directions */
		record.d.d_order[1],	/* order of u and v directions */
		record.d.d_kv_size[0],	/* knot vector size (u and v) */
		record.d.d_kv_size[1],	/* knot vector size (u and v) */
		record.d.d_ctl_size[0],	/* control mesh size (u and v) */
		record.d.d_ctl_size[1],	/* control mesh size (u and v) */
		record.d.d_geom_type,	/* geom type 3 or 4 */
		record.d.d_nknots,	/* # granules of knots */
		record.d.d_nctls );	/* # granules of ctls */
	/* 
	 * The b_surf_head record is followed by
	 * d_nknots granules of knot vectors (first u, then v),
	 * and then by d_nctls granules of control mesh information.
	 * Note that neither of these have an ID field!
	 *
	 * B-spline surface record, followed by
	 *	d_kv_size[0] floats,
	 *	d_kv_size[1] floats,
	 *	padded to d_nknots granules, followed by
	 *	ctl_size[0]*ctl_size[1]*geom_type floats,
	 *	padded to d_nctls granules.
	 *
	 * IMPORTANT NOTE: granule == sizeof(union record)
	 */

	/* Malloc and clear memory for the KNOT DATA and read it */
	nbytes = record.d.d_nknots * sizeof(union record);
	if( (vp = (float *)malloc(nbytes))  == (float *)0 )  {
		(void)fprintf(stderr, "G2ASC: spline knot malloc error\n");
		exit(1);
	}
	fp = vp;
	(void)bzero( (char *)fp, nbytes );
	count = fread( (char *)fp, 1, nbytes, stdin );
	if( count != 1 )  {
		(void)fprintf(stderr, "G2ASC: spline knot read failure\n");
		exit(1);
	}
	/* Print the knot vector information */
	count = record.d.d_kv_size[0] + record.d.d_kv_size[1];
	for( i = 0; i < count; i++ )  {
		(void)printf("%.12e\n", *vp++);
	}
	/* Free the knot data memory */
	(void)free( (char *)fp );

	/* Malloc and clear memory for the CONTROL MESH data and read it */
	nbytes = record.d.d_nctls * sizeof(union record);
	if( (vp = (float *)malloc(nbytes))  == (float *)0 )  {
		(void)fprintf(stderr, "G2ASC: control mesh malloc error\n");
		exit(1);
	}
	fp = vp;
	(void)bzero( (char *)fp, nbytes );
	count = fread( (char *)fp, 1, nbytes, stdin );
	if( count != nbytes )  {
		(void)fprintf(stderr, "G2ASC: control mesh read failure\n");
		exit(1);
	}
	/* Print the control mesh information */
	count = record.d.d_ctl_size[0] * record.d.d_ctl_size[1] *
		record.d.d_geom_type;
	for( i = 0; i < count; i++ )  {
		(void)printf("%.12e\n", *vp++);
	}
	/* Free the control mesh memory */
	(void)free( (char *)fp );
}

#ifdef SYSV

bzero( str, n )
register char *str;
register int n;
{
	while( n-- > 0 )
		*str++ = '\0';
}
#endif

/*
 *			N A M E
 *
 *  Take a database name and null-terminate it,
 *  converting unprintable characters to something printable.
 *  Here we deal with NAMESIZE long names not being null-terminated.
 */
char *name( str )
char *str;
{
	static char buf[NAMESIZE+2];
	register char *ip = str;
	register char *op = buf;
	register int warn = 0;

	while( op < &buf[NAMESIZE] )  {
		if( *ip == '\0' )  break;
		if( isascii(*ip) && isprint(*ip) && !isspace(*ip) )  {
			*op++ = *ip++;
		}  else  {
			*op++ = '@';
			ip++;
			warn = 1;
		}
	}
	*op = '\0';
	if(warn)  {
		(void)fprintf(stderr,
		"g2asc: Illegal char in object name, converted to '%s'\n",
		buf );
	}
	if( op == buf )  {
		/* Null input name */
		(void)fprintf(stderr,
			"g2asc:  NULL object name converted to -=NULL=-\n");
		return("-=NULL=-");
	}
	return(buf);
}

/*
 *			S T R C H O P
 *
 *  Take a string and a length, and null terminate,
 *  converting unprintable characters to something printable.
 */
char *strchop( str, len )
char *str;
{
	static char buf[1024];
	register char *ip = str;
	register char *op = buf;
	register int warn = 0;
	char *ep;

	if( len > sizeof(buf)-2 )  len=sizeof(buf)-2;
	ep = &buf[len];
	while( op < ep )  {
		if( *ip == '\0' )  break;
		if( isascii(*ip) && isprint(*ip) )  {
			*op++ = *ip++;
		}  else  {
			*op++ = '@';
			ip++;
			warn = 1;
		}
	}
	*op = '\0';
	if(warn)  {
		(void)fprintf(stderr,
		"g2asc: Illegal char in string, converted to '%s'\n",
		buf );
	}
	if( op == buf )  {
		/* Null input name */
		(void)fprintf(stderr,
			"g2asc:  NULL string converted to -=STRING=-\n");
		return("-=STRING=-");
	}
	return(buf);
}
