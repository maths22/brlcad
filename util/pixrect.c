/*
 *		P I X R E C T . C
 *
 * Remove a portion of a potentially huge pix file.
 *
 *  Author -
 *	Phillip Dykstra
 *	2 Oct 1985
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

char *malloc();

int	xnum, ynum;		/* Number of pixels in new map */
int	xorig, yorig;		/* Bottom left corner to extract from */
int	linelen;
char	*buf;			/* output scanline buffer, malloc'd */

main(argc, argv)
int argc; char **argv;
{
	FILE	*ifp, *ofp;
	int	row;
	int	error;
	long	offset;

	if (argc < 3) {
		printf("usage: pixrect infile outfile (I prompt!)\n");
		exit( 1 );
	}
	if ((ifp = fopen(argv[1], "r")) == NULL) {
		printf("pixrect: can't open %s\n", argv[1]);
		exit( 2 );
	}
	if ((ofp = fopen(argv[2], "w")) == NULL) {
		printf("pixrect: can't open %s\n", argv[1]);
		exit( 3 );
	}

	/* Get info */
	printf( "Area to extract (x, y) in pixels " );
	scanf( "%d%d", &xnum, &ynum );
	printf( "Origin to extract from (0,0 is lower left) " );
	scanf( "%d%d", &xorig, &yorig );
	printf( "Scan line length of input file " );
	scanf( "%d", &linelen );

	buf = malloc( xnum * 3 );

	/* Move all points */
	for (row = 0+yorig; row < ynum+yorig; row++) {
		offset = row * 3 * linelen + (3 * xorig);
		error = fseek(ifp, offset, 0);
		error = fread(buf, sizeof(*buf), 3*xnum, ifp);
		error = fwrite(buf, sizeof(*buf), 3*xnum, ofp);
	}
}
