/*
 *			P I X H A L V E . C
 *
 *  Reduce the resolution of a .pix file by one half in each direction,
 *  using a 5x5 pyramid filter.
 *
 *  Author -
 *	Michael John Muuss
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1990 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include "externs.h"

extern int	getopt();
extern char	*optarg;
extern int	optind;

static char	*file_name;
static FILE	*infp;

static int	fileinput = 0;		/* file or pipe on input? */
static int	autosize = 0;		/* !0 to autosize input */

static int	file_width = 512;	/* default input width */
static int	file_height = 512;	/* default input height */

static char usage[] = "\
Usage: pixhalve [-h] [-a]\n\
	[-s squaresize] [-w file_width] [-n file_height] [file.pix]\n";

#include "./asize.c"

get_args( argc, argv )
register char **argv;
{
	register int c;

	while ( (c = getopt( argc, argv, "ahs:w:n:" )) != EOF )  {
		switch( c )  {
		case 'a':
			autosize = 1;
			break;
		case 'h':
			/* high-res */
			file_height = file_width = 1024;
			autosize = 0;
			break;
		case 's':
			/* square file size */
			file_height = file_width = atoi(optarg);
			autosize = 0;
			break;
		case 'w':
			file_width = atoi(optarg);
			autosize = 0;
			break;
		case 'n':
			file_height = atoi(optarg);
			autosize = 0;
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
			perror(file_name);
			(void)fprintf( stderr,
				"pixhalve: cannot open \"%s\" for reading\n",
				file_name );
			return(0);
		}
		fileinput++;
	}

	if ( argc > ++optind )
		(void)fprintf( stderr, "pixhalve: excess argument(s) ignored\n" );

	return(1);		/* OK */
}

int	*rlines[5];
int	*glines[5];
int	*blines[5];

main( argc, argv )
int	argc;
char	**argv;
{
	char	*inbuf;
	char	*outbuf;
	int	*rout, *gout, *bout;
	int	out_width;
	int	i;

	if ( !get_args( argc, argv ) )  {
		(void)fputs(usage, stderr);
		exit( 1 );
	}

	/* autosize input? */
	if( fileinput && autosize ) {
		int	w, h;
		if( image_size(file_name, 3, &w, &h) ) {
			file_width = w;
			file_height = h;
		}
	}
	out_width = file_width/2;

	/* Allocate 1-scanline input & output buffers */
	inbuf = malloc( 3*file_width+8 );
	outbuf = malloc( 3*(out_width+2)+8 );

	/* Allocate 5 integer arrays for each color */
	/* each width+2 elements wide */
	for( i=0; i<5; i++ )  {
		rlines[i] = (int *)calloc( (file_width+4)+1, sizeof(long) );
		glines[i] = (int *)calloc( (file_width+4)+1, sizeof(long) );
		blines[i] = (int *)calloc( (file_width+4)+1, sizeof(long) );
	}

	/* Allocate an integer array for each color, for output */
	rout = (int *)malloc( out_width * sizeof(long) + 8 );
	gout = (int *)malloc( out_width * sizeof(long) + 8 );
	bout = (int *)malloc( out_width * sizeof(long) + 8 );

	/* Prime the pumps with the bottom 5 lines */
	for( i=0; i<5; i++ )  {
		if( fread( inbuf, 3, file_width, infp ) != file_width )  {
			perror(file_name);
			fprintf(stderr, "pixhalve:  fread error\n");
			exit(1);
		}
		separate( &rlines[i][2], &glines[i][2], &blines[i][2],
			inbuf, file_width );
	}

	for(;;)  {
		filter( rout, rlines, out_width );
		filter( gout, glines, out_width );
		filter( bout, blines, out_width );
		combine( outbuf, rout, gout, bout, out_width );
		if( fwrite( outbuf, 3, out_width, stdout ) != out_width )  {
			perror("stdout");
			exit(2);
		}

		/* Ripple down two scanlines, and acquire two more */
		if( fread( inbuf, 3, file_width, infp ) != file_width )  {
			exit(0);
		}
		ripple( rlines, 5 );
		ripple( glines, 5 );
		ripple( blines, 5 );
		separate( &rlines[4][2], &glines[4][2], &blines[4][2],
			inbuf, file_width );

		if( fread( inbuf, 3, file_width, infp ) != file_width )  {
			exit(0);
		}
		ripple( rlines, 5 );
		ripple( glines, 5 );
		ripple( blines, 5 );
		separate( &rlines[4][2], &glines[4][2], &blines[4][2],
			inbuf, file_width );

	}
}

separate( rop, gop, bop, cp, num )
register int	*rop;
register int	*gop;
register int	*bop;
register unsigned char	*cp;
int		num;
{
	register int 	i;

	for( i = num-1; i >= 0; i-- )  {
		*rop++ = *cp++;
		*gop++ = *cp++;
		*bop++ = *cp++;
	}
}

combine( cp, rip, gip, bip, num )
register unsigned char	*cp;
register int		*rip;
register int		*gip;
register int		*bip;
int			num;
{
	register int 	i;

	for( i = num-1; i >= 0; i-- )  {
		*cp++ = *rip++;
		*cp++ = *gip++;
		*cp++ = *bip++;
	}
}

/*
 *  Barrel shift all the pointers down, with [0] going back to the top.
 */
ripple( array, num )
int	*array[];
int	num;
{
	register int	i;
	int		*temp;

	temp = array[0];
	for( i=0; i < num-1; i++ )
		array[i] = array[i+1];
	array[num-1] = temp;
}

filter( op, lines, num )
int	*op;
int	*lines[];
int	num;
{
	register int	i;
	register int	j;
	register int	*a, *b, *c, *d, *e;

	a = lines[0];
	b = lines[1];
	c = lines[2];
	d = lines[3];
	e = lines[4];

#	include "noalias.h"
	for( i=0; i < num; i++ )  {
		j = i*2;
		op[i] = (
			  a[j+0] + 2*a[j+1] + 4*a[j+2] + 2*a[j+3] +   a[j+4] +
			2*b[j+0] + 4*b[j+1] + 8*b[j+2] + 4*b[j+3] + 2*b[j+4] +
			4*c[j+0] + 8*c[j+1] +16*c[j+2] + 8*c[j+3] + 4*c[j+4] +
			2*d[j+0] + 4*d[j+1] + 8*d[j+2] + 4*d[j+3] + 2*d[j+4] +
			  e[j+0] + 2*e[j+1] + 4*e[j+2] + 2*e[j+3] +   e[j+4]
			) / 100;
	}
}
