/*
 *  			P L R O T . C
 *  
 *  Rotate, Translate, and Scale a Unixplot file.
 *
 *  Author -
 *	Phillip Dykstra
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1986 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>
#include "machine.h"
#include "vmath.h"

#ifdef SYSV
#define bzero(p,cnt)	memset(p,'\0',cnt);
#endif

#define	UPPER_CASE(c)	((c)-32)
#define	COPY(n)	{fread(cbuf,1,n,fp); fwrite(cbuf,1,n,stdout);}
#define	SKIP(n)	{fread(cbuf,1,n,fp);}

#define	putsi(s) {putchar(s); putchar((s)>>8);}

extern int	getopt();
extern char	*optarg;
extern int	optind;

double	getdouble();

char	cbuf[48];		/* COPY and SKIP macro buffer */

mat_t	rmat;			/* rotation matrix to be applied */
double	scale;			/* scale factor to be applied */
int	doscale;

int	rpp;			/* indicates new center and space values */
double	centx, centy, centz;	/* new center of rotation values */
double	viewsize = 2.0;

void	dofile(), copy_string();
void	two_coord_out(), three_coord_out();
void	two_dcoord_out(), three_dcoord_out();

static char usage[] = "\
Usage: plrot [options] [file1 ... fileN] > file.plot\n\
   -x# -y# -z#    Rotation about axis in degrees\n\
   -X# -Y# -Z#    Translation along axis\n\
   -s#            Scale factor\n\
   -a# -e#        Azimuth/Elevation from front view\n\
                  (usually first, in this order, implies -M)\n\
   -g		  MGED front view to display coordinates (usually last)\n\
   -M             Autoscale space command like RT model RPP\n";

get_args( argc, argv )
register char **argv;
{
	register int c;
	mat_t	tmp, m;

	mat_idn( rmat );
	scale = 1.0;

	while ( (c = getopt( argc, argv, "Mga:e:x:y:z:X:Y:Z:s:" )) != EOF )  {
		switch( c )  {
		case 'M':
			/* model RPP! */
			rpp++;
			break;
		case 'g':
			mat_angles( tmp, -90.0, 0.0, -90.0 );
			mat_copy( m, rmat );
			mat_mul( rmat, tmp, m );
			break;
		case 'a':
			mat_angles( tmp, 0.0, 0.0, -atof(optarg) );
			mat_copy( m, rmat );
			mat_mul( rmat, tmp, m );
			rpp++;
			break;
		case 'e':
			mat_angles( tmp, 0.0, -atof(optarg), 0.0 );
			mat_copy( m, rmat );
			mat_mul( rmat, tmp, m );
			rpp++;
			break;
		case 'x':
			mat_angles( tmp, atof(optarg), 0.0, 0.0 );
			mat_copy( m, rmat );
			mat_mul( rmat, tmp, m );
			break;
		case 'y':
			mat_angles( tmp, 0.0, atof(optarg), 0.0 );
			mat_copy( m, rmat );
			mat_mul( rmat, tmp, m );
			break;
		case 'z':
			mat_angles( tmp, 0.0, 0.0, atof(optarg) );
			mat_copy( m, rmat );
			mat_mul( rmat, tmp, m );
			break;
		case 'X':
			mat_idn( tmp );
			tmp[3] = atof(optarg);
			mat_copy( m, rmat );
			mat_mul( rmat, tmp, m );
			break;
		case 'Y':
			mat_idn( tmp );
			tmp[7] = atof(optarg);
			mat_copy( m, rmat );
			mat_mul( rmat, tmp, m );
			break;
		case 'Z':
			mat_idn( tmp );
			tmp[11] = atof(optarg);
			mat_copy( m, rmat );
			mat_mul( rmat, tmp, m );
			break;
		case 's':
			scale *= atof(optarg);
			doscale++;
			break;
		default:		/* '?' */
			return(0);	/* Bad */
		}
	}

	if( isatty(fileno(stdout))
	  || (isatty(fileno(stdin)) && (optind >= argc)) )  {
		return(0);	/* Bad */
	}

	return(1);		/* OK */
}

main( argc, argv )
int	argc;
char	**argv;
{
	FILE	*fp;

	if( !get_args( argc, argv ) )  {
		(void)fputs(usage, stderr);
		exit( 1);
	}

	if( optind < argc ) {
		while( optind < argc ) {
			if( fp != NULL && fp != stdin )
				fclose( fp );
			if( strcmp(argv[optind], "-") == 0 )
				fp = stdin;
			else if( (fp = fopen(argv[optind],"r")) == NULL ) {
				fprintf( stderr, "plrot: can't open \"%s\"\n", argv[optind] );
				continue;
			}
			dofile( fp );
			optind++;
		}
	} else {
		dofile( stdin );
	}
}

void
dofile( fp )
FILE	*fp;
{
	register int	c;

	while( (c = getc(fp)) != EOF ) {
		switch( c ) {
		/* One of a kind functions */
		case 'e':
		case 'F':
			putchar( c );
			break;
		case 'f':
		case 't':
			putchar( c );
			copy_string( fp );
			break;
		case 'C':
			putchar( c );
			COPY(3);
			break;
		case 'c':	/* map x, y? */
			putchar( c );
			COPY(6);
			break;
		case 'a':	/* map points? */
			putchar( c );
			COPY(12);
			break;
		case 'p':
		case 'm':
		case 'n':
			putchar( UPPER_CASE(c) );
			two_coord_out( fp, rmat );
			break;
		case 'l':
			putchar( 'L' );
			two_coord_out( fp, rmat );
			two_coord_out( fp, rmat );
			break;
		case 'P':
		case 'M':
		case 'N':
			putchar( c );
			three_coord_out( fp, rmat );
			break;
		case 'L':
			putchar( c );
			three_coord_out( fp, rmat );
			three_coord_out( fp, rmat );
			break;
		case 's':
			{
				/* SPACE transform */
				int	minx, miny, minz, maxx, maxy, maxz;

				minx = getshort(fp);

				miny = getshort(fp);
				maxx = getshort(fp);
				maxy = getshort(fp);
				minz = -1;
				maxz = 1;
				if( rpp ) {
					short	v;

					model_rpp( (double)minx, (double)miny, (double)minz,
						(double)maxx, (double)maxy, (double)maxz );

					/* output viewsize cube */
					v = viewsize/2.0 + 0.5;
					pl_3space( stdout, -v, -v, -v, v, v, v );
				} else {
					pl_3space( stdout, minx, miny, -1, maxx, maxy, 1 );
				}
			}
			break;
		case 'S':
			if( rpp ) {
				int	minx, miny, minz, maxx, maxy, maxz;
				short	v;

				minx = getshort(fp);
				miny = getshort(fp);
				minz = getshort(fp);
				maxx = getshort(fp);
				maxy = getshort(fp);
				maxz = getshort(fp);

				model_rpp( (double)minx, (double)miny, (double)minz,
					(double)maxx, (double)maxy, (double)maxz );

				/* output viewsize cube */
				v = viewsize/2.0 + 0.5;
				pl_3space( stdout, -v, -v, -v, v, v, v );
			} else {
				/* leave it unchanged */
				putchar(c);
				COPY(6*2);
			}
			/* Third option: rotate and rebound? */
			/* 4th, universal space? */
			break;
		/* 2D and 3D IEEE */
		case 'w':
			{
				/* SPACE transform */
				double	minx, miny, minz, maxx, maxy, maxz;

				minx = getdouble(fp);
				miny = getdouble(fp);
				maxx = getdouble(fp);
				maxy = getdouble(fp);
				minz = -1.0;
				maxz = 1.0;
				if( rpp ) {
					double	v;

					model_rpp( minx, miny, minz,
						maxx, maxy, maxz );

					/* output viewsize cube */
					v = viewsize/2.0;
					pd_3space( stdout, -v, -v, -v, v, v, v );
				} else {
					pd_3space( stdout, minx, miny, -1.0, maxx, maxy, 1.0 );
				}
			}
			break;
		case 'W':
			if( rpp ) {
				double	minx, miny, minz, maxx, maxy, maxz;
				double	v;

				minx = getdouble(fp);
				miny = getdouble(fp);
				minz = getdouble(fp);
				maxx = getdouble(fp);
				maxy = getdouble(fp);
				maxz = getdouble(fp);

				model_rpp( minx, miny, minz,
					maxx, maxy, maxz );

				/* output viewsize cube */
				v = viewsize/2.0;
				pd_3space( stdout, -v, -v, -v, v, v, v );
			} else {
				/* leave it unchanged */
				putchar(c);
				COPY(6*8);
			}
			break;
		case 'i':
			putchar(c);
			COPY(3*8);
			break;
		case 'r':
			putchar(c);
			COPY(6*8);
			break;
		case 'x':
		case 'o':
		case 'q':
			putchar( UPPER_CASE(c) );
			two_dcoord_out( fp, rmat );
			break;
		case 'v':
			putchar( 'V' );
			two_dcoord_out( fp, rmat );
			two_dcoord_out( fp, rmat );
			break;
		case 'X':
		case 'O':
		case 'Q':
			putchar( c );
			three_dcoord_out( fp, rmat );
			break;
		case 'V':
			putchar( c );
			three_dcoord_out( fp, rmat );
			three_dcoord_out( fp, rmat );
			break;
		default:
			fprintf( stderr, "plrot: unrecognized command '%c' (0x%x)\n", c, c );
			fprintf( stderr, "plrot: ftell = %d\n", ftell(fp) );
			putchar( c );
			break;
		}
	}
}

void
copy_string( fp )
FILE	*fp;
{
	int	c;

	while( (c = putchar(getc(fp))) != '\n' && c != EOF )
		;
}

model_rpp( minx, miny, minz, maxx, maxy, maxz )
double	minx, miny, minz, maxx, maxy, maxz;
{
	float	dx, dy, dz;

	centx = (maxx + minx) / 2.0;
	centy = (maxy + miny) / 2.0;
	centz = (maxz + minz) / 2.0;

	dx = maxx - minx;
	dy = maxy - miny;
	dz = maxz - minz;
	viewsize = sqrt( dx*dx + dy*dy + dz*dz );

	doscale++;

	return( 1 );
}

/******* Coordinate Transforms / Output *******/

void
two_coord_out( fp, m )
FILE	*fp;
mat_t	m;
{
	double	p1[4];
	short	p2[4];

	p1[0] = getshort(fp);	/* get X */
	p1[1] = getshort(fp);	/* and Y */
	p1[2] = 0;		/* no Z  */
	p1[3] = 1.0;

	if( doscale ) {
		p1[0] = (p1[0] - centx) * scale;
		p1[1] = (p1[1] - centy) * scale;
		p1[2] = (p1[2] - centz) * scale;
	}

	MAT4X4PNT( p2, m, p1 );

	putsi(p2[0]);		/* put X */
	putsi(p2[1]);		/* and Y */
	putsi(p2[2]);		/* and Z */
}

void
three_coord_out( fp, m )
FILE	*fp;
mat_t	m;
{
	double	p1[4];
	short	p2[4];

	p1[0] = getshort(fp);	/* get X */
	p1[1] = getshort(fp);	/* and Y */
	p1[2] = getshort(fp);	/* and Z */
	p1[3] = 1.0;

	if( doscale ) {
		p1[0] = (p1[0] - centx) * scale;
		p1[1] = (p1[1] - centy) * scale;
		p1[2] = (p1[2] - centz) * scale;
	}

	MAT4X4PNT( p2, m, p1 );

	putsi(p2[0]);		/* put X */
	putsi(p2[1]);		/* and Y */
	putsi(p2[2]);		/* and Z */
}

void
two_dcoord_out( fp, m )
FILE	*fp;
mat_t	m;
{
	char	buf[2*8];
	double	p1[4], p2[4];

	fread( buf, 1, 2*8, fp );
	ntohd( p1, buf, 2 );
	p1[2] = 0;		/* no Z  */
	p1[3] = 1.0;

	if( doscale ) {
		p1[0] = (p1[0] - centx) * scale;
		p1[1] = (p1[1] - centy) * scale;
		p1[2] = (p1[2] - centz) * scale;
	}

	MAT4X4PNT( p2, m, p1 );

	htond( buf, p2, 3 );
	fwrite( buf, 1, 3*8, stdout );
}

void
three_dcoord_out( fp, m )
FILE	*fp;
mat_t	m;
{
	char	buf[3*8];
	double	p1[4], p2[4];

	fread( buf, 1, 3*8, fp );
	ntohd( p1, buf, 3 );
	p1[3] = 1.0;

/*fprintf( stderr, "Before [%g %g %g %g]\n", p1[0], p1[1], p1[2], p1[3] );*/
	if( doscale ) {
		p1[0] = (p1[0] - centx) * scale;
		p1[1] = (p1[1] - centy) * scale;
		p1[2] = (p1[2] - centz) * scale;
	}

	MAT4X4PNT( p2, m, p1 );

/*fprintf( stderr, "After [%g %g %g %g]\n", p2[0], p2[1], p2[2], p2[3] );*/
	htond( buf, p2, 3 );
	fwrite( buf, 1, 3*8, stdout );
}

getshort( fp )
FILE	*fp;
{
	register long	v, w;

	v = getc(fp);
	v |= (getc(fp)<<8);	/* order is important! */

	/* worry about sign extension - sigh */
	if( v <= 0x7FFF )  return(v);
	w = -1;
	w &= ~0x7FFF;
	return( w | v );
}

double
getdouble( fp )
FILE	*fp;
{
	double	d;
	char	buf[8];
	fread( buf, 8, 1, fp );
	ntohd( &d, buf, 1 );
	return( d );
}
