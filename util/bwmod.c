/*
 *		B W M O D . C
 *
 *  Modify intensities in Black and White files.
 *
 *  Allows any number of add, subtract, multiply, divide, or
 *  exponentiation operations to be performed on a picture.
 *  Keeps track of and reports clipping.
 *
 *  Note that this works on PIX files also but there is no
 *  distinction made between colors.
 *
 *  Author -
 *	Phillip Dykstra
 *	31 July 1986
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

double	atof();

extern int	getopt();
extern char	*optarg;
extern int	optind;

char	*file_name;
FILE	*infp;

char usage[] = "\
Usage: bwmod {-a add -s sub -m mult -d div -A(abs) -e exp} [file.bw]\n";

#define	ADD	1
#define MULT	2
#define	ABS	3
#define	POW	4

#define	BUFLEN	1024

int	numop = 0;		/* number of operations */
int	op[256];		/* operations */
double	val[256];		/* arguments to operations */
double	buf[BUFLEN];		/* working buffer */
unsigned char obuf[BUFLEN];	/* output buffer */

get_args( argc, argv )
register char **argv;
{
	register int c;
	double	d;

	while ( (c = getopt( argc, argv, "a:s:m:d:Ae:" )) != EOF )  {
		switch( c )  {
		case 'a':
			op[ numop ] = ADD;
			val[ numop++ ] = atof(optarg);
			break;
		case 's':
			op[ numop ] = ADD;
			val[ numop++ ] = - atof(optarg);
			break;
		case 'm':
			op[ numop ] = MULT;
			val[ numop++ ] = atof(optarg);
			break;
		case 'd':
			op[ numop ] = MULT;
			d = atof(optarg);
			if( d == 0.0 ) {
				fprintf( stderr, "bwmod: divide by zero!\n" );
				exit( 2 );
			}
			val[ numop++ ] = 1.0 / d;
			break;
		case 'A':
			op[ numop ] = ABS;
			val[ numop++ ] = 0;
			break;
		case 'e':
			op[ numop ] = POW;
			val[ numop++ ] = atof(optarg);
			break;

		default:		/* '?' */
			return(0);
		}
	}

	if( optind >= argc )  {
		if( isatty(fileno(stdin)) )
			return(0);
		file_name = "-";
		infp = stdin;
	} else {
		file_name = argv[optind];
		if( (infp = fopen(file_name, "r")) == NULL )  {
			(void)fprintf( stderr,
				"bwmod: cannot open \"%s\" for reading\n",
				file_name );
			return(0);
		}
	}

	if ( argc > ++optind )
		(void)fprintf( stderr, "bwmod: excess argument(s) ignored\n" );

	return(1);		/* OK */
}

main( argc, argv )
int argc;
char **argv;
{
	int	i, n;
	register double	*bp;
	register double	arg;
	register int j;
	long	value;
	unsigned char ibuf[BUFLEN];
	long	clipped_high, clipped_low;

	if( !get_args( argc, argv ) || isatty(fileno(infp))
	    || isatty(fileno(stdout)) ) {
		(void)fputs(usage, stderr);
		exit( 1 );
	}

	clipped_high = clipped_low = 0;

	while( (n = fread(ibuf, sizeof(*ibuf), BUFLEN, infp)) > 0 ) {
		for( i = 0; i < n; i++ )
			buf[i] = ibuf[i];
		for( i = 0; i < numop; i++ ) {
			arg = val[ i ];
			switch( op[i] ) {
			case ADD:
				bp = &buf[0];
				for( j = n; j > 0; j-- ) {
					*bp++ += arg;
				}
				break;
			case MULT:
				bp = &buf[0];
				for( j = n; j > 0; j-- ) {
					*bp++ *= arg;
				}
				break;
			case POW:
				bp = &buf[0];
				for( j = n; j > 0; j-- ) {
					*bp = pow( *bp, arg );
					bp++;
				}
				break;
			case ABS:
				bp = &buf[0];
				for( j = n; j > 0; j-- ) {
					if( *bp < 0.0 )
						*bp = - *bp;
					bp++;
				}
				break;
			default:
				break;
			}
		}
		for( i = 0; i < n; i++ ) {
			value = buf[i] + 0.5;	/* double -> long */
			if( value > 255 ) {
				obuf[i] = 255;
				clipped_high++;
			} else if( value < 0 ) {
				obuf[i] = 0;
				clipped_low++;
			} else
				obuf[i] = value;
		}
		fwrite( obuf, sizeof(*obuf), n, stdout );
	}

	if( clipped_high != 0 || clipped_low != 0 ) {
		fprintf( stderr, "bwmod: clipped %d high, %d low\n",
			clipped_high, clipped_low );
	}
}
