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
#include "externs.h"

extern int	getopt();
extern char	*optarg;
extern int	optind;
char *progname = "(noname)";

char	*file_name;

char usage[] = "\
Usage: bwmod [-c] {-a add -s sub -m mult -d div -A -e exp -r root} [file.bw]\n";

#define	ADD	1
#define MULT	2
#define	ABS	3
#define	POW	4
#define	BUFLEN	(8192*2)	/* usually 2 pages of memory, 16KB */

int	numop = 0;		/* number of operations */
int	op[256];		/* operations */
double	val[256];		/* arguments to operations */
unsigned char ibuf[BUFLEN];	/* input buffer */

#define MAPBUFLEN 256
int mapbuf[MAPBUFLEN];		/* translation buffer/lookup table */
int char_arith = 0;

get_args( argc, argv )
register char **argv;
{
	register int c;
	double	d;

	while ( (c = getopt( argc, argv, "a:s:m:d:Ae:r:c" )) != EOF )  {
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
				(void)fprintf( stderr, "bwmod: divide by zero!\n" );
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
		case 'r':
			op[ numop ] = POW;
			d = atof(optarg);
			if( d == 0.0 ) {
				(void)fprintf( stderr, "bwmod: zero root!\n" );
				exit( 2 );
			}
			val[ numop++ ] = 1.0 / d;
			break;
		case 'c':
			char_arith = !char_arith; break;
		default:		/* '?' */
			return(0);
		}
	}

	if( optind >= argc )  {
		if( isatty((int)fileno(stdin)) )
			return(0);
		file_name = "-";
	} else {
		file_name = argv[optind];
		if( freopen(file_name, "r", stdin) == NULL )  {
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

void mk_trans_tbl()
{
	register int j, i;
	register double d;

	/* create translation map */
	for (j = 0; j < MAPBUFLEN ; ++j) {
		d = j;
		for (i=0 ; i < numop ; i++) {
			switch (op[i]) {
			case ADD : d += val[i]; break;
			case MULT: d *= val[i]; break;
			case POW : d = pow( d, val[i]); break;
			case ABS : if (d < 0.0) d = - d; break;
			default  : (void)fprintf(stderr, "%s: error in op\n", progname);
				   exit(-1);
				   break;
			}
		}
		if (d > 255.0) {
			mapbuf[j] = 256;
		} else if (d < 0.0) {
			mapbuf[j] = -1;
		} else
			mapbuf[j] = d + 0.5;
	}
}
void mk_char_trans_tbl()
{
	register int j, i;
	register signed char d;

	/* create translation map */
	for (j = 0; j < MAPBUFLEN ; ++j) {
		d = j;
		for (i=0 ; i < numop ; i++) {
			switch (op[i]) {
			case ADD : d += val[i]; break;
			case MULT: d *= val[i]; break;
			case POW : d = pow( d, val[i]); break;
			case ABS : if (d < 0.0) d = - d; break;
			default  : (void)fprintf(stderr, "%s: error in op\n", progname);
				   exit(-1);
				   break;
			}
		}
		mapbuf[j] = d & 0x0ff;
	}
}
int main( argc, argv )
int argc;
char **argv;
{
	register unsigned char	*p, *q;
	register int		tmp;
	int	 		n;
	unsigned long		clip_high, clip_low;
	
	progname = *argv;

	if( !get_args( argc, argv ) || isatty((int)fileno(stdin))
	    || isatty((int)fileno(stdout)) ) {
		(void)fputs(usage, stderr);
		exit( 1 );
	}

	if (char_arith)
		mk_char_trans_tbl();
	else
		mk_trans_tbl();

	clip_high = clip_low = 0L;
	while ( (n=read(0, (void *)ibuf, (unsigned)sizeof(ibuf))) > 0) {
		/* translate */
		for (p = ibuf, q = &ibuf[n] ; p < q ; ++p) {
			tmp = mapbuf[*p];
			if (tmp > 255) { ++clip_high; *p = 255; }
			else if (tmp < 0) { ++clip_low; *p = 0; }
			else *p = tmp;
		}
		/* output */
		if (write(1, (void *)ibuf, (unsigned)n) != n) {
			(void)fprintf(stderr, "%s: Error writing stdout\n",
				progname);
			exit(-1);
		}
	}

	if( clip_high != 0 || clip_low != 0 ) {
		(void)fprintf( stderr, "bwmod: clipped %lu high, %lu low\n",
			clip_high, clip_low );
	}
	return(0);
}
