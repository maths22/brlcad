/*
 *		A S C 2 G . C
 *  
 *  This program generates a GED database from an
 *  ASCII GED data file.
 *
 *  Usage:  asc2g < file.asc > file.g
 *  
 *  Authors -
 *  	Charles M Kennedy
 *  	Michael J Muuss
 *	Susanne Muuss, J.D.	 Converted to libwdb, Oct. 1990 
 *
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
#include "machine.h"
#include "vmath.h"
#include "rtlist.h"
#include "db.h"
#include "wdb.h"
#include "externs.h"


#define BUFSIZE			(8*1024)	/* input line buffer size */
#define TYPELEN			10
#define NAMELEN			20

void		identbld(), polyhbld(), polydbld(), pipebld(), particlebld();
void		solbld(), combbld(), membbld(), arsabld(), arsbbld();
void		materbld(), bsplbld(), bsurfbld(), zap_nl();
char		*nxt_spc();

static union record	record;			/* GED database record */
static char 		buf[BUFSIZE];		/* Record input buffer */
char			name[NAMESIZE + 2];
int 			debug;

main(argc, argv)
char **argv;
{
	if( argc > 1 )
		debug = 1;

	/* Read ASCII input file, each record on a line */
	while( ( fgets( buf, BUFSIZE, stdin ) ) != (char *)0 )  {

		/* Clear the output record -- vital! */
		(void)bzero( (char *)&record, sizeof(record) );

		/* Check record type */
		if( debug )
			(void)fprintf(stderr,"rec %c\n", buf[0] );
		switch( buf[0] )  {
		case ID_SOLID:
			solbld();
			continue;

		case ID_COMB:
			combbld();
			continue;

		case ID_MEMB:
			membbld();
			continue;

		case ID_ARS_A:
			arsabld();
			continue;

		case ID_ARS_B:
			arsbbld();
			continue;

		case ID_P_HEAD:
			polyhbld();
			continue;

		case ID_P_DATA:
			polydbld();
			continue;

		case ID_IDENT:
			identbld();
			continue;

		case ID_MATERIAL:
			materbld();
			continue;

		case ID_BSOLID:
			bsplbld();
			continue;

		case ID_BSURF:
			bsurfbld();
			continue;

		case DBID_PIPE:
			pipebld();
			continue;

		case DBID_PARTICLE:
			particlebld();
			continue;

		default:
			(void)fprintf(stderr,"asc2g: bad record type '%c' (0%o), skipping\n", buf[0], buf[0]);
			(void)fprintf(stderr,"%s\n", buf );
			continue;
		}
	}
	exit(0);
}

/*		S O L B L D
 *
 * This routine parses a solid record and determines which libwdb routine
 * to call to replicate this solid.  Simple primitives are expected.
 */

void
solbld()
{
	register char *cp;
	register char *np;
	register int i;

	char	s_id;			/* id code for the record */
	char	s_type;			/* id for the type of primitive */
	short	s_cgtype;		/* comgeom solid type */
	fastf_t	val[24];		/* array of values/parameters for solid */
	point_t	center;			/* center; used by many solids */
	point_t pnts[9];		/* array of points for the arbs */
	point_t	norm;
	point_t	min, max;
	vect_t	a, b, c, d, n, r1, r2;	/* various vectors required */
	vect_t	height;			/* height vector for tgc */
	double	dd, rad1, rad2;

	cp = buf;
	s_id = *cp++;
	cp = nxt_spc( cp );		/* skip the space */

	s_type = (char)atoi( cp );
	cp = nxt_spc( cp );

	np = name;
	while( *cp != ' ' )  {
		*np++ = *cp++;
	}
	*np = '\0';
	
	cp = nxt_spc( cp );
	s_cgtype = (short)atoi( cp );

	for( i = 0; i < 24; i++ )  {
		cp = nxt_spc( cp );
		val[i] = atof( cp );
	}

	/* Switch on the record type to make the solids. */

	switch( s_type ) {

		case TOR:
			VSET(center, val[0], val[1], val[2]);
			VSET(n, val[3], val[4], val[5]);
			rad1 = MAGNITUDE(&val[6]);
			rad2 = MAGNITUDE(n);
			VUNITIZE(n);

			/* Prevent illegal torii from floating point fuzz */
			if( rad2 > rad1 )  rad2 = rad1;

			mk_tor(stdout, name, center, n, rad1, rad2);
			break;

		case GENTGC:
			VSET(center, val[0], val[1], val[2]);
			VSET(height, val[3], val[4], val[5]);
			VSET(a, val[6], val[7], val[8]);
			VSET(b, val[9], val[10], val[11]);
			VSET(c, val[12], val[13], val[14]);
			VSET(d, val[15], val[16], val[17]);
			
			mk_tgc(stdout, name, center, height, a, b, c, d);
			break;

		case GENELL:
			VSET(center, val[0], val[1], val[2]);
			VSET(a, val[3], val[4], val[5]);
			VSET(b, val[6], val[7], val[8]);
			VSET(c, val[9], val[10], val[11]);

			mk_ell(stdout, name, center, a, b, c);
			break;

		case GENARB8:
			VSET(pnts[0], val[0], val[1], val[2]);
			VSET(pnts[1], val[3], val[4], val[5]);
			VSET(pnts[2], val[6], val[7], val[8]);
			VSET(pnts[3], val[9], val[10], val[11]);
			VSET(pnts[4], val[12], val[13], val[14]);
			VSET(pnts[5], val[15], val[16], val[17]);
			VSET(pnts[6], val[18], val[19], val[20]);
			VSET(pnts[7], val[21], val[22], val[23]);

			/* Convert from vector notation to absolute points */
			for( i=1; i<8; i++ )  {
				VADD2( pnts[i], pnts[i], pnts[0] );
			}

			mk_arb8(stdout, name, pnts);
			break;

		case HALFSPACE:
			VSET(norm, val[0], val[1], val[2]);
			dd = val[3];

			mk_half(stdout, name, norm, dd);
			break;

		default:
			fprintf(stderr, "asc2g: bad solid %s s_type= %d, skipping\n",
				name, s_type);
	}

}


/*			C O M B B L D
 *
 *  This routine builds combinations.
 */

void
combbld()
{
	register char 	*cp;
	register char 	*np;
	int 		temp_nflag, temp_pflag;

	char		override;
	char		id;		/* == ID_COMB */
	char		reg_flags;	/* region flag */
	int		is_reg;
	short		regionid;
	short		aircode;
	short		length;		/* number of members expected */
	short		num;		/* Comgeom reference number: DEPRECATED */
	short		material;	/* GIFT material code */
	short		los;		/* LOS estimate */
	unsigned char	rgb[3];		/* Red, green, blue values */
	char		matname[32];	/* String of material name */
	char		matparm[60];	/* String of material parameters */
	char		inherit;	/* Inheritance property */


	/* Set all flags initially. */

	override = 0;
	temp_nflag = temp_pflag = 0;	/* indicators for optional fields */

	cp = buf;
	id = *cp++;			/* ID_COMB */
	cp = nxt_spc( cp );		/* skip the space */

	reg_flags = *cp++;
	cp = nxt_spc( cp );

	np = name;
	while( *cp != ' ' )  {
		*np++ = *cp++;
	}
	*np = '\0';
	
	cp = nxt_spc( cp );

	regionid = (short)atoi( cp );
	cp = nxt_spc( cp );
	aircode = (short)atoi( cp );
	cp = nxt_spc( cp );
	length = (short)atoi( cp );
	cp = nxt_spc( cp );
	num = (short)atoi( cp );
	cp = nxt_spc( cp );
	material = (short)atoi( cp );
	cp = nxt_spc( cp );
	los = (short)atoi( cp );
	cp = nxt_spc( cp );
	override = (char)atoi( cp );
	cp = nxt_spc( cp );

	rgb[0] = (unsigned char)atoi( cp );
	cp = nxt_spc( cp );
	rgb[1] = (unsigned char)atoi( cp );
	cp = nxt_spc( cp );
	rgb[2] = (unsigned char)atoi( cp );
	cp = nxt_spc( cp );

	temp_nflag = atoi( cp );
	cp = nxt_spc( cp );
	temp_pflag = atoi( cp );

	cp = nxt_spc( cp );
	inherit = atoi( cp );

	if( reg_flags == 'Y' )
		is_reg = 1;
	else
		is_reg = 0;

	if( temp_nflag )  {
		fgets( buf, BUFSIZE, stdin );
		zap_nl();
		strncpy( matname, buf, sizeof(matname)-1 );
	}
	if( temp_pflag )  {
		fgets( buf, BUFSIZE, stdin );
		zap_nl();
		strncpy( matparm, buf, sizeof(matparm)-1 );
	}

	if( mk_rcomb(stdout, name, length, is_reg,
		temp_nflag ? matname : (char *)0,
		temp_pflag ? matparm : (char *)0,
		override ? rgb : (char *)0,
		regionid, aircode, material, los, inherit) < 0 )  {
			fprintf(stderr,"asc2g: mk_rcomb fail\n");
			exit(1);
	}

}


/*		M E M B B L D
 *
 *  This routine invokes libwdb to build a member of a combination.
 */

void
membbld()
{
	register char 	*cp;
	register char 	*np;
	register int 	i;
	char		id;
	char		relation;	/* boolean operation */
	char		inst_name[NAMESIZE];
	fastf_t		mat[16];	/* transformation matrix */

	cp = buf;
	id = *cp++;
	cp = nxt_spc( cp );		/* skip the space */

	relation = *cp++;
	cp = nxt_spc( cp );

	np = inst_name;
	while( *cp != ' ' )  {
		*np++ = *cp++;
	}
	*np = '\0';

	cp = nxt_spc( cp );

	for( i = 0; i < 16; i++ )  {
		mat[i] = atof( cp );
		cp = nxt_spc( cp );
	}

	mk_memb(stdout, inst_name, mat, relation );
}


/*		A R S B L D
 *
 * This routine builds ARS's.
 */

void
arsabld()
{


	register char *cp;
	register char *np;
	point_t	      max[3];
	point_t	      min[3];

	cp = buf;
	record.a.a_id = *cp++;
	cp = nxt_spc( cp );		/* skip the space */

	record.a.a_type = (char)atoi( cp );
	cp = nxt_spc( cp );

	np = record.a.a_name;
	while( *cp != ' ' )  {
		*np++ = *cp++;
	}
	cp = nxt_spc( cp );

	record.a.a_m = (short)atoi( cp );
	cp = nxt_spc( cp );
	record.a.a_n = (short)atoi( cp );
	cp = nxt_spc( cp );
	record.a.a_curlen = (short)atoi( cp );
	cp = nxt_spc( cp );
	record.a.a_totlen = (short)atoi( cp );
	cp = nxt_spc( cp );

	record.a.a_xmax = atof( cp );
	cp = nxt_spc( cp );
	record.a.a_xmin = atof( cp );
	cp = nxt_spc( cp );
	record.a.a_ymax = atof( cp );
	cp = nxt_spc( cp );
	record.a.a_ymin = atof( cp );
	cp = nxt_spc( cp );
	record.a.a_zmax = atof( cp );
	cp = nxt_spc( cp );
	record.a.a_zmin = atof( cp );

	/* Write out the record */
	(void)fwrite( (char *)&record, sizeof record, 1, stdout );

}

/*		A R S B L D
 *
 * This is the second half of the ARS-building.  It builds the ARS B record.
 */

void
arsbbld()
{
	register char *cp;
	register int i;
	point_t	      pnt[8];		/* need 8 points */

	cp = buf;
	record.b.b_id = *cp++;
	cp = nxt_spc( cp );		/* skip the space */

	record.b.b_type = (char)atoi( cp );
	cp = nxt_spc( cp );
	record.b.b_n = (short)atoi( cp );
	cp = nxt_spc( cp );
	record.b.b_ngranule = (short)atoi( cp );

	for( i = 0; i < 24; i++ )  {
		cp = nxt_spc( cp );
		record.b.b_values[i] = atof( cp );
	}

	/* Write out the record */
	(void)fwrite( (char *)&record, sizeof record, 1, stdout );

}


/*		Z A P _ N L
 *
 * This routine removes newline characters from the buffer and substitutes
 * in NULL.
 */

void
zap_nl()
{
	register char *bp;

	bp = &buf[0];

	while( *bp != '\0' )  {
		if( *bp == '\n' )
			*bp = '\0';
		bp++;
	}
}


/*		I D E N T B L D
 *
 * This routine makes an ident record.  It calls libwdb to do this.
 */

void
identbld()
{
	register char	*cp;
	register char	*np;
	char		id;		/* a freebie */
	char		units;		/* libwdb doesn't take this?! */
	char		version[6];
	char		title[72];

	cp = buf;
	id = *cp++;
	cp = nxt_spc( cp );		/* skip the space */

	/* Note that there is no provision for handing libwdb the units.  Just
	 * ignore.
	 */

	units = (char)atoi( cp );
	cp = nxt_spc( cp );

	/* Note that there is no provision for handing libwdb the version either.
	 * However, this is automatically provided when needed.
	 */

	np = version;
	while( *cp != '\n' && *cp != '\0' )  {
		*np++ = *cp++;
	}
	*np = '\0';

	if( strcmp( version, ID_VERSION ) != 0 )  {
		fprintf(stderr, "WARNING:  input file version (%s) is not %s\n",
			version, ID_VERSION);
	}

	(void)fgets( buf, BUFSIZE, stdin);
	zap_nl();
	(void)strncpy( title, buf, sizeof(title)-1 );

	mk_id(stdout, title);
}


/*		P O L Y H B L D
 *
 *  This routine builds the record headder for a polysolid.
 */

void
polyhbld()
{

	/* Headder for polysolid */

	register char	*cp;
	register char	*np;
	char		id;

	cp = buf;
	id = *cp++;
	cp = nxt_spc( cp );		/* skip the space */

	np = name;
	while( *cp != '\n' && *cp != '\0' )  {
		*np++ = *cp++;
	}

	mk_polysolid(stdout, name);
}

/*		P O L Y D B L D
 *
 * This routine builds a polydata record using libwdb.
 */

void
polydbld()
{
	register char	*cp;
	register int	i, j;
	char		id;
	char		count;		/* number of vertices */
	fastf_t		verts[5][3];	/* vertices for the polygon */
	fastf_t		norms[5][3];	/* normals at each vertex */

	cp = buf;
	id = *cp++;
	cp = nxt_spc( cp );		/* skip the space */

	count = (char)atoi( cp );

	for( i = 0; i < 5; i++ )  {
		for( j = 0; j < 3; j++ )  {
			cp = nxt_spc( cp );
			verts[i][j] = atof( cp );
		}
	}

	for( i = 0; i < 5; i++ )  {
		for( j = 0; j < 3; j++ )  {
			cp = nxt_spc( cp );
			norms[i][j] = atof( cp );
		}
	}

	mk_poly(stdout, count, verts, norms);
}


/*		M A T E R B L D
 *
 * The need for this is being phased out. Leave alone.
 */

void
materbld()
{

	register char *cp;
	register char *np;

	cp = buf;
	record.md.md_id = *cp++;
	cp = nxt_spc( cp );		/* skip the space */

	record.md.md_flags = (char)atoi( cp );
	cp = nxt_spc( cp );
	record.md.md_low = (short)atoi( cp );
	cp = nxt_spc( cp );
	record.md.md_hi = (short)atoi( cp );
	cp = nxt_spc( cp );
	record.md.md_r = (unsigned char)atoi( cp);
	cp = nxt_spc( cp );
	record.md.md_g = (unsigned char)atoi( cp);
	cp = nxt_spc( cp );
	record.md.md_b = (unsigned char)atoi( cp);

	/* Write out the record */
	(void)fwrite( (char *)&record, sizeof record, 1, stdout );
}

/*		B S P L B L D
 *
 *  This routine builds B-splines using libwdb.
 */

void
bsplbld()
{
	register char	*cp;
	register char	*np;
	char		id;
	short		nsurf;		/* number of surfaces */
	fastf_t		resolution;	/* resolution of flatness */
	
	cp = buf;
	id = *cp++;
	cp = nxt_spc( cp );		/* skip the space */

	np = name;
	while( *cp != ' ' )  {
		*np++ = *cp++;
	}
	*np = '\0';
	cp = nxt_spc( cp );

	nsurf = (short)atoi( cp );
	cp = nxt_spc( cp );
	resolution = atof( cp );

	mk_bsolid(stdout, name, nsurf, resolution);
}

/* 		B S U R F B L D
 *
 * This routine builds d-spline surface descriptions using libwdb.
 */

void
bsurfbld()
{

/* HELP! This involves mk_bsurf(filep, bp) where bp is a ptr to struct */

	register char	*cp;
	register int	i;
	register float	*vp;
	int		nbytes, count;
	float		*fp;

	cp = buf;
	record.d.d_id = *cp++;
	cp = nxt_spc( cp );		/* skip the space */

	record.d.d_order[0] = (short)atoi( cp );
	cp = nxt_spc( cp );
	record.d.d_order[1] = (short)atoi( cp );
	cp = nxt_spc( cp );
	record.d.d_kv_size[0] = (short)atoi( cp );
	cp = nxt_spc( cp );
	record.d.d_kv_size[1] = (short)atoi( cp );
	cp = nxt_spc( cp );
	record.d.d_ctl_size[0] = (short)atoi( cp );
	cp = nxt_spc( cp );
	record.d.d_ctl_size[1] = (short)atoi( cp );
	cp = nxt_spc( cp );
	record.d.d_geom_type = (short)atoi( cp );
	cp = nxt_spc( cp );
	record.d.d_nknots = (short)atoi( cp );
	cp = nxt_spc( cp );
	record.d.d_nctls = (short)atoi( cp );

	record.d.d_nknots = 
		ngran( record.d.d_kv_size[0] + record.d.d_kv_size[1] );

	record.d.d_nctls = 
		ngran( record.d.d_ctl_size[0] * record.d.d_ctl_size[1] 
			* record.d.d_geom_type);

	/* Write out the record */
	(void)fwrite( (char *)&record, sizeof record, 1, stdout );

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
		(void)fprintf(stderr, "asc2g: spline knot malloc error\n");
		exit(1);
	}
	fp = vp;
	(void)bzero( (char *)vp, nbytes );
	/* Read the knot vector information */
	count = record.d.d_kv_size[0] + record.d.d_kv_size[1];
	for( i = 0; i < count; i++ )  {
		fgets( buf, BUFSIZE, stdin );
		(void)sscanf( buf, "%f", vp++);
	}
	/* Write out the information */
	(void)fwrite( (char *)fp, nbytes, 1, stdout );

	/* Free the knot data memory */
	(void)free( (char *)fp );

	/* Malloc and clear memory for the CONTROL MESH data and read it */
	nbytes = record.d.d_nctls * sizeof(union record);
	if( (vp = (float *)malloc(nbytes))  == (float *)0 )  {
		(void)fprintf(stderr, "asc2g: control mesh malloc error\n");
		exit(1);
	}
	fp = vp;
	(void)bzero( (char *)vp, nbytes );
	/* Read the control mesh information */
	count = record.d.d_ctl_size[0] * record.d.d_ctl_size[1] *
		record.d.d_geom_type;
	for( i = 0; i < count; i++ )  {
		fgets( buf, BUFSIZE, stdin );
		(void)sscanf( buf, "%f", vp++);
	}
	/* Write out the information */
	(void)fwrite( (char *)fp, nbytes, 1, stdout );

	/* Free the control mesh memory */
	(void)free( (char *)fp );
}

/*		P I P E B L D
 *
 *  This routine reads pipe data from standard in, constructs a doublely
 *  linked list of pipe segments, and sends this list to mk_pipe().
 */

void
pipebld()
{

	char			name[NAMELEN];
	char			ident;
	char			type[TYPELEN];
	int			ret;
	fastf_t			id;
	fastf_t			od;
	point_t			start;
	point_t			bendcenter;
	register char		*cp;
	register char		*np;
	struct wdb_pipeseg	*sp;
	struct wdb_pipeseg	head;

	/* Process the first buffer */

	cp = buf;
	ident = *cp++;			/* not used later */
	cp = nxt_spc( cp );		/* skip spaces */

	np = name;
	while( *cp != '\n' )  {
		*np++ = *cp++;
	}
	*np = '\0';			/* null terminate the string */


	/* Read data lines and process */

	RT_LIST_INIT( &head.l );
	do{
		fgets( buf, BUFSIZE, stdin);
		(void)sscanf( buf, "%s %le %le %le %le %le %le %le %le", type, 
				&id, &od,
				&start[0],
				&start[1],
				&start[2],
				&bendcenter[0],
				&bendcenter[1],
				&bendcenter[2]);

		if( (sp = (struct wdb_pipeseg *)malloc(sizeof(struct wdb_pipeseg) ) )
			== WDB_PIPESEG_NULL)  {
				printf("asc2g: malloc failure for pipe\n");
				exit(-1);
		}

		sp->ps_id = id;
		sp->ps_od = od;
		VMOVE(sp->ps_start, start);

		/* Identify type */
		if( (ret = (strcmp( type, "end" ))) == 0)  {
			sp->ps_type = WDB_PIPESEG_TYPE_END;
		} else if( (ret = (strcmp( type, "linear" ))) == 0)  {
			sp->ps_type = WDB_PIPESEG_TYPE_LINEAR;
		} else if( (ret = (strcmp( type, "bend"))) == 0)  {
			sp->ps_type = WDB_PIPESEG_TYPE_BEND;
			VMOVE(sp->ps_bendcenter, bendcenter);
		} else  {
			fprintf(stderr, "asc2g: no pipe type %s\n", type);
		}

		RT_LIST_INSERT( &head.l, &sp->l);
	} while( (ret = (strcmp (type , "end"))) != 0);

	mk_pipe(stdout, name, &head);
	mk_freemembers( &head );
	free( sp );
}

/*			P A R T I C L E B L D
 *
 * This routine reads particle data from standard in, and constructs the
 * parameters required by mk_particle.
 */

void
particlebld()
{

	char		name[NAMELEN];
	char		ident;
	char		type[TYPELEN];
	point_t		vertex;
	vect_t		height;
	double		vrad;
	double		hrad;
	register char	*cp;
	register char	*np;


	/* Read all the information out of the existing buffer.  Note that
	 * particles fit into one granule.
	 */

	(void)sscanf(buf, "%c %s %le %le %le %le %le %le %le %le",
		&ident, name,
		&vertex[0],
		&vertex[1],
		&vertex[2],
		&height[0],
		&height[1],
		&height[2],
		&vrad, &hrad);

	mk_particle( stdout, name, vertex, height, vrad, hrad);
}


char *
nxt_spc( cp)
register char *cp;
{
	while( *cp != ' ' && *cp != '\t' && *cp !='\0' )  {
		cp++;
	}
	if( *cp != '\0' )  {
		cp++;
	}
	return( cp );
}

ngran( nfloat )
{
	register int gran;
	/* Round up */
	gran = nfloat + ((sizeof(union record)-1) / sizeof(float) );
	gran = (gran * sizeof(float)) / sizeof(union record);
	return(gran);
}
