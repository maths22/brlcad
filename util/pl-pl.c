/*
 *			P L - P L . C
 *
 *  Plot smasher.
 *  Gets rid of (floating point, flush, 3D, color, text).
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
 *	This software is Copyright (C) 1988 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>

void	putshort(), putieee(), getstring();
void	putargs(), getargs();
void	doscale();

#define	TBAD	0	/* no such command */
#define TNONE	1	/* no arguments */
#define TSHORT	2	/* Vax 16-bit short */
#define	TIEEE	3	/* IEEE 64-bit floating */
#define	TCHAR	4	/* unsigned chars */
#define	TSTRING	5	/* linefeed terminated string */

struct uplot {
	int	targ;	/* type of args */
	int	narg;	/* number or args */
	char	*desc;	/* description */
	int	t3d;	/* non-zero if 3D */
};
struct uplot uerror = { 0, 0, 0 };
struct uplot letters[] = {
/*A*/	{ 0, 0, 0, 0 },
/*B*/	{ 0, 0, 0, 0 },
/*C*/	{ TCHAR, 3, "color", 0 },
/*D*/	{ 0, 0, 0, 0 },
/*E*/	{ 0, 0, 0, 0 },
/*F*/	{ TNONE, 0, "flush", 0 },
/*G*/	{ 0, 0, 0, 0 },
/*H*/	{ 0, 0, 0, 0 },
/*I*/	{ 0, 0, 0, 0 },
/*J*/	{ 0, 0, 0, 0 },
/*K*/	{ 0, 0, 0, 0 },
/*L*/	{ TSHORT, 6, "3line", 1 },
/*M*/	{ TSHORT, 3, "3move", 1 },
/*N*/	{ TSHORT, 3, "3cont", 1 },
/*O*/	{ TIEEE, 3, "d_3move", 1 },
/*P*/	{ TSHORT, 3, "3point", 1 },
/*Q*/	{ TIEEE, 3, "d_3cont", 1 },
/*R*/	{ 0, 0, 0, 0 },
/*S*/	{ TSHORT, 6, "3space", 1 },
/*T*/	{ 0, 0, 0, 0 },
/*U*/	{ 0, 0, 0, 0 },
/*V*/	{ TIEEE, 6, "d_3line", 1 },
/*W*/	{ TIEEE, 6, "d_3space", 1 },
/*X*/	{ TIEEE, 3, "d_3point", 1 },
/*Y*/	{ 0, 0, 0, 0 },
/*Z*/	{ 0, 0, 0, 0 },
/*[*/	{ 0, 0, 0, 0 },
/*\*/	{ 0, 0, 0, 0 },
/*]*/	{ 0, 0, 0, 0 },
/*^*/	{ 0, 0, 0, 0 },
/*_*/	{ 0, 0, 0, 0 },
/*`*/	{ 0, 0, 0, 0 },
/*a*/	{ TSHORT, 3, "arc", 0 },
/*b*/	{ 0, 0, 0, 0 },
/*c*/	{ TSHORT, 3, "circle", 0 },
/*d*/	{ 0, 0, 0, 0 },
/*e*/	{ TNONE, 0, "erase", 0 },
/*f*/	{ TSTRING, 1, "linmod", 0 },
/*g*/	{ 0, 0, 0, 0 },
/*h*/	{ 0, 0, 0, 0 },
/*i*/	{ TIEEE, 3, "d_circle", 0 },
/*j*/	{ 0, 0, 0, 0 },
/*k*/	{ 0, 0, 0, 0 },
/*l*/	{ TSHORT, 4, "line", 0 },
/*m*/	{ TSHORT, 2, "move", 0 },
/*n*/	{ TSHORT, 2, "cont", 0 },
/*o*/	{ TIEEE, 2, "d_move", 0 },
/*p*/	{ TSHORT, 2, "point", 0 },
/*q*/	{ TIEEE, 2, "d_cont", 0 },
/*r*/	{ TIEEE, 3, "d_arc", 0 },
/*s*/	{ TSHORT, 4, "space", 0 },
/*t*/	{ TSTRING, 1, "label", 0 },
/*u*/	{ 0, 0, 0, 0 },
/*v*/	{ TIEEE, 4, "d_line", 0 },
/*w*/	{ TIEEE, 4, "d_space", 0 },
/*x*/	{ TIEEE, 2, "d_point", 0 },
/*y*/	{ 0, 0, 0, 0 },
/*z*/	{ 0, 0, 0, 0 }
};

double	getieee();
int	verbose;
double	arg[6];			/* parsed plot command arguments */
double	sp[6];			/* space command */
char	strarg[512];		/* string buffer */
double	cx, cy, cz;		/* center */
double	scale = 0;
int	seenscale = 0;

int	nofloat = 1;
int	noflush = 1;
int	nocolor = 1;
int	no3d = 1;

static char usage[] = "\
Usage: pl-pl [-S] < unix_plot > unix_plot\n";

main( argc, argv )
int	argc;
char	**argv;
{
	register int	c;
	struct	uplot *up;

	while( argc > 1 ) {
		if( strcmp(argv[1], "-v") == 0 ) {
			verbose++;
		} else if( strcmp(argv[1], "-S") == 0 ) {
			scale = 1;
		} else
			break;

		argc--;
		argv++;
	}
	if( isatty(fileno(stdin)) ) {
		fprintf( stderr, usage );
		exit( 1 );
	}

	while( (c = getchar()) != EOF ) {
		/* look it up */
		if( c < 'A' || c > 'z' ) {
			up = &uerror;
		} else {
			up = &letters[ c - 'A' ];
		}

		if( up->targ == TBAD ) {
			fprintf( stderr, "Bad command '%c' (0x%02x)\n", c, c );
			continue;
		}

		if( up->narg > 0 )
			getargs( up );

		/* check for space command */
		switch( c ) {
		case 's':
		case 'w':
			sp[0] = arg[0];
			sp[1] = arg[1];
			sp[2] = 0;
			sp[3] = arg[2];
			sp[4] = arg[3];
			sp[5] = 0;
			if( scale )
				doscale();
			break;
		case 'S':
		case 'W':
			sp[0] = arg[0];
			sp[1] = arg[1];
			sp[2] = arg[2];
			sp[3] = arg[3];
			sp[4] = arg[4];
			sp[5] = arg[5];
			if( scale )
				doscale();
			break;
		}

		/* do it */
		switch( c ) {
		case 's':
		case 'm':
		case 'n':
		case 'p':
		case 'l':
		case 'c':
		case 'a':
		case 'f':
		case 'e':
			/* The are as generic as unix plot gets */
			putchar( c );
			putargs( up );
			break;

		case 't': /* XXX vector lists */
			putchar( c );
			putargs( up );
			break;

		case 'C':
			if( nocolor == 0 ) {
				putchar( c );
				putargs( up );
			}
			break;

		case 'F':
			if( noflush == 0 ) {
				putchar( c );
				putargs( up );
			}
			break;

		case 'S':
		case 'M':
		case 'N':
		case 'P':
		case 'L':
			if( no3d )
				putchar( c + 'a' - 'A' );
			else
				putchar( c );
			putargs( up );
			break;

		case 'w':
		case 'o':
		case 'q':
		case 'x':
		case 'v':
		case 'i':
		case 'r':
			/* 2d floating */
			if( nofloat ) {
				/* to 2d integer */
				if( c == 'w' )
					putchar( 's' );
				else if( c == 'o' )
					putchar( 'm' );
				else if( c == 'q' )
					putchar( 'n' );
				else if( c == 'x' )
					putchar( 'p' );
				else if( c == 'v' )
					putchar( 'l' );
				else if( c == 'i' )
					putchar( 'c' );
				else if( c == 'r' )
					putchar( 'a' );
			} else {
				putchar( c );
			}
			putargs( up );
			break;

		case 'W':
		case 'O':
		case 'Q':
		case 'X':
		case 'V':
			/* 3d floating */
			if( nofloat ) {
				if( no3d ) {
					/* to 2d integer */
					if( c == 'W' )
						putchar( 's' );
					else if( c == 'O' )
						putchar( 'm' );
					else if( c == 'Q' )
						putchar( 'n' );
					else if( c == 'X' )
						putchar( 'p' );
					else if( c == 'V' )
						putchar( 'l' );
				} else {
					/* to 3d integer */
					if( c == 'W' )
						putchar( 'S' );
					else if( c == 'O' )
						putchar( 'M' );
					else if( c == 'Q' )
						putchar( 'N' );
					else if( c == 'X' )
						putchar( 'P' );
					else if( c == 'V' )
						putchar( 'L' );
				}
			} else {
				if( no3d ) {
					/* to 2d floating */
					putchar( c + 'a' - 'A' );
				} else {
					/* to 3d floating */
					putchar( c );
				}
			}
			putargs( up );
			break;
		}

		if( verbose )
			printf( "%s\n", up->desc );
	}

	if( !seenscale ) {
		fprintf( stderr, "pl-pl: warning no space command\n" );
	}

	return(0);
}

/*** Input args ***/

void
getargs( up )
struct uplot *up;
{
	int	i;

	for( i = 0; i < up->narg; i++ ) {
		switch( up->targ ) {
			case TSHORT:
				arg[i] = getshort();
				break;
			case TIEEE:
				arg[i] = getieee();
				break;
			case TSTRING:
				getstring();
				break;
			case TCHAR:
				arg[i] = getchar();
				break;
			case TNONE:
			default:
				arg[i] = 0;	/* ? */
				break;
		}
	}
}

void
getstring()
{
	int	c;
	char	*cp;

	cp = strarg;
	while( (c = getchar()) != '\n' && c != EOF )
		*cp++ = c;
	*cp = 0;
}

getshort()
{
	register long	v, w;

	v = getchar();
	v |= (getchar()<<8);	/* order is important! */

	/* worry about sign extension - sigh */
	if( v <= 0x7FFF )  return(v);
	w = -1;
	w &= ~0x7FFF;
	return( w | v );
}

double
getieee()
{
	char	in[8];
	double	d;

	fread( in, 8, 1, stdin );
	ntohd( &d, in, 1 );
	return	d;
}

/*** Output args ***/

void
putargs( up )
struct uplot *up;
{
	int	i;

	for( i = 0; i < up->narg; i++ ) {
		if( no3d && ((i % 3) == 2) && up->t3d )
			continue;	/* skip z coordinate */
		/* gag me with a spoon... */
		if( scale && (up->targ == TSHORT || up->targ == TIEEE) ) {
			if( up->t3d ) {
				if( i % 3 == 0 )
					arg[i] = (arg[i] - cx) * scale;
				else if( i % 3 == 1 )
					arg[i] = (arg[i] - cy) * scale;
				else
					arg[i] = (arg[i] - cz) * scale;
			} else {
				if( i % 2 == 0 )
					arg[i] = (arg[i] - cx) * scale;
				else
					arg[i] = (arg[i] - cy) * scale;
			}
		}
		switch( up->targ ) {
			case TSHORT:
				if( arg[i] > 32767 ) arg[i] = 32767;
				if( arg[i] < -32767 ) arg[i] = -32767;
				putshort( (short)arg[i] );
				break;
			case TIEEE:
				if( nofloat ) {
					if( arg[i] > 32767 ) arg[i] = 32767;
					if( arg[i] < -32767 ) arg[i] = -32767;
					putshort( (short)arg[i] );
				} else
					putieee( arg[i] );
				break;
			case TSTRING:
				printf( "%s\n", strarg );
				break;
			case TCHAR:
				putchar( arg[i] );
				break;
			case TNONE:
			default:
				break;
		}
	}
}

void
putshort( s )
short s;
{
	/* For the sake of efficiency, we trust putc()
	 * to write only one byte
	 */
	putchar( s );
	putchar( s>>8 );
}

void
putieee( d )
double	d;
{
	char	out[8];

	htond( out, &d, 1 );
	fwrite( out, 1, 8, stdout );
}

void
doscale()
{
	double	dx, dy, dz;
	double	max;

	cx = (sp[3] + sp[0]) / 2.0;
	cy = (sp[4] + sp[1]) / 2.0;
	cz = (sp[5] + sp[2]) / 2.0;

	dx = (sp[3] - sp[0]) / 2.0;
	dy = (sp[4] - sp[1]) / 2.0;
	dz = (sp[5] - sp[2]) / 2.0;

	max = dx;
	if( dy > max ) max = dy;
	if( dz > max ) max = dz;

	scale = 32767.0 / max;

	seenscale++;
}
