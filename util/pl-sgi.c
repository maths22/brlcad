/*
 *			T I R I S . C
 *
 *  SGI Iris 3-D Unix Plot display program.
 *
 *  Authors -
 *	Paul R. Stay
 *	Gary S. Moss
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

#ifdef sgi
# include "gl.h"
# include "device.h"

#define Min( x1, x2 )	((x1) < (x2) ? (x1) : (x2))
#define Max( x1, x2 )	((x1) > (x2) ? (x1) : (x2))

Matrix	*viewmat;	/* current viewing projection */
Matrix	viewortho;	/* ortho viewing projection */
Matrix	viewpersp;	/* perspective viewing projection */
Matrix	centermat;	/* center screen matrix */
Coord	viewsize;
#endif sgi

main()
{
#ifdef sgi
	Coord	max[3], min[3];

	if( isatty(fileno(stdin)) ) {
		fprintf( stderr, "Usage: tiris < unixplot\n" );
		exit( 1 );
	}

	/* In case if there is no space command */
	min[0] = min[1] = min[2] = -32768.0;
	max[0] = max[1] = max[2] = 32768.0;

	init_display();

	makeobj( 1 );
	color( 3000 );
	uplot( max, min );
	closeobj( 1 );

	/* scale to the largest X, Y, or Z interval */
	viewsize = max[0] - min[0];
	viewsize = Max( viewsize, max[1] - min[1] );
	viewsize = Max( viewsize, max[2] - min[2] );
	viewsize /= 2.0;

	/* set up and save the viewing projection matrix */
	if( ismex() ) {
		ortho( -viewsize, viewsize, -viewsize, viewsize,
			-32768.0, 32768.0 );
		getmatrix( viewortho );
		perspective( 900, 1.0, 0.01, 1.0e10 );
		/*polarview( 1.414, 0, 0, 0 );*/
		getmatrix( viewpersp );
	} else {
		/* Try to make up for the rectangular display surface */
		ortho( -1.33*viewsize, 1.33*viewsize, -viewsize, viewsize,
			-32768.0, 32768.0 );
		getmatrix( viewortho );
		perspective( 900, 1.0, 0.01, 1.0e10 );
		/*polarview( 1.414, 0, 0, 0 );*/
		getmatrix( viewpersp );
	}
	viewmat = (Matrix *)viewortho;

	/* set up and save the initial rot/trans/scale matrix */
	ortho( -1.0, 1.0, -1.0, 1.0, -1.0, 1.0 );	/* ident on stack XXX */
	translate( -(max[0]+min[0])/2.0, -(max[1]+min[1])/2.0,
		-(max[2]+min[2])/2.0 );
	getmatrix( centermat );

	view_loop();
#else
	printf( "tiris: this is an SGI Iris specific program\n" );
#endif
}

#ifdef sgi
/* 
 *	V I E W _ L O O P
 */

#define	ROTX	DIAL6
#define	ROTY	DIAL4
#define	ROTZ	DIAL2
#define	TRANX	DIAL7
#define	TRANY	DIAL5
#define	TRANZ	DIAL3
#define	ZOOM	DIAL1

#define MARGIN	4			/* # pixels margin to screen edge */
#define WINDIM	768
#define BANNER	20
#define WIN_L	(1024-WINDIM-MARGIN)
#define WIN_R	(1024-1-MARGIN)
#define WIN_B	MARGIN
#define WIN_T	(WINDIM-BANNER-MARGIN)

#define MAP_RESERVED	16		/* # slots reserved by MEX */
#define MAP_TOL		15		/* pixel delta across all channels */

static int map_size;			/* # of color map slots available */

view_loop()
{
	Matrix          m, ident;
	Device          event;
	short           val;
	int             end_it = 0;
	int 		obj_num = 1;
	float		fval;

	/* Initial translate/rotate/scale matrix */
	loadmatrix( centermat );
	getmatrix( m );

	/* Make an identity matrix */
	ident[0][0] = ident[1][1] = ident[2][2] = ident[3][3] = 1.0;
	ident[1][0] = ident[1][2] = ident[1][3] = 0.0;
	ident[2][0] = ident[0][1] = ident[2][3] = 0.0;
	ident[3][0] = ident[3][1] = ident[3][2] = 0.0;

/*	depthcue(1);*/
	cursoff();

	while (1) {
		Coord		fval;

		if (qtest())
			event = qread(&val);
		fval = val;

		loadmatrix ( ident );

		switch (event) {
		case ROTX:
			fval *= 5.0;
			rotate( (Angle) fval, 'x' );
			setvaluator(ROTX, 0, -360, 360);
			break;
		case ROTY:
			fval *= 5.0;
			rotate( (Angle) -fval, 'y' );
			setvaluator(ROTY, 0, -360, 360);
			break;
		case ROTZ:
			fval *= 5.0;
			rotate( (Angle) -fval, 'z' );
			setvaluator(ROTZ, 0, -360, 360);
			break;
		case TRANX:
			fval *= viewsize/200.0;
			translate( fval, 0.0, 0.0 );
			setvaluator(TRANX, 0, -10, 10);
			break;
		case TRANY:
			fval *= viewsize/200.0;
			translate( 0.0, fval, 0.0 );
			setvaluator(TRANY, 0, -10, 10);
			break;
		case TRANZ:
			fval *= viewsize/200.0;
			translate( 0.0, 0.0, fval );
			setvaluator(TRANZ, 0, -10, 10);
			break;
		case ZOOM:
			fval = 1.0 + fval / 500.0;
			scale( fval, fval, fval );
			setvaluator(ZOOM, 1, -1000, 1000);
			break;
		case LEFTMOUSE:
		case SW3:
			loadmatrix( centermat );
			getmatrix( m );
			loadmatrix( ident );
			break;
		case MIDDLEMOUSE:
			end_it = 1;
			break;
		case SW0:
			viewmat = (Matrix *)viewortho;
			break;
		case SW1:
			viewmat = (Matrix *)viewpersp;
			break;
		}
		event = 0;
		qreset();
		if (end_it == 1)
			break;

		/* combine new rot/trans with old */
		multmatrix( m );
		getmatrix( m );

		/* set up total viewing transformation */
		loadmatrix( viewmat );
		multmatrix( m );

		/* draw the object */
		color(BLACK);
		clear();
		callobj(obj_num);
		swapbuffers();
	}
	/*depthcue( 0 );*/
	curson();
	greset();
	tpon();
	gexit();
}

init_display()
{
	int	i;
	short	r, g, b;

	if( ismex() )
		{
		prefposition( WIN_L, WIN_R, WIN_B, WIN_T );
		foreground();
		if( winopen( "UNIX plot display" ) == -1 )
			{
			printf( "No more graphics ports available.\n" );
			return	1;
			}
		wintitle( "UNIX plot display" );
		/* Free window of position constraint.			*/
		winconstraints();
		tpoff();
		doublebuffer();
		gconfig();	/* Must be called after singlebuffer().	*/
		winattach();

		/*
		 * Deal with the SGI hardware color map
		 */
		map_size = 1<<getplanes(); /* 10 or 12, depending on ismex() */
	
		/* The first 8 entries of the colormap are "known" colors */
		mapcolor( 0, 000, 000, 000 );	/* BLACK */
		mapcolor( 1, 255, 000, 000 );	/* RED */
		mapcolor( 2, 000, 255, 000 );	/* GREEN */
		mapcolor( 3, 255, 255, 000 );	/* YELLOW */
		mapcolor( 4, 000, 000, 255 );	/* BLUE */
		mapcolor( 5, 255, 000, 255 );	/* MAGENTA */
		mapcolor( 6, 000, 255, 000 );	/* CYAN */
		mapcolor( 7, 255, 255, 255 );	/* WHITE */

		/* Use fixed color map with 10x10x10 color cube */
		for( i = 0; i < map_size-MAP_RESERVED; i++ )
			mapcolor( 	i+MAP_RESERVED,
					(short)((i % 10) + 1) * 25,
					(short)(((i / 10) % 10) + 1) * 25,
					(short)((i / 100) + 1) * 25
					);
		}
	else
		{
		ginit();
		tpoff();
		doublebuffer();
		onemap();
		gconfig();

		for( b = 0; b < 16; b++ ) {
			for( g = 0; g < 16; g++ ) {
				for( r = 0; r < 16; r++ ) {
					mapcolor( b*256+g*16+r, r<<4, g<<4, b<<4 );
				}
			}
		}
		}

	qdevice(LEFTMOUSE);
	qdevice(MIDDLEMOUSE);

	qdevice( SW0 );
	qdevice( SW1 );
	qdevice( SW2 );

	for (i = DIAL0; i < DIAL8; i++) {
		qdevice(i);
	}

	setvaluator(ROTX, 0, -360, 360);
	setvaluator(ROTY, 0, -360, 360);
	setvaluator(ROTZ, 0, -360, 360);
	setvaluator(TRANX, 0, -10, 10);
	setvaluator(TRANY, 0, -10, 10);
	setvaluator(TRANZ, 0, -10, 10);
	setvaluator(ZOOM, 1, -1000, 1000);
	noise( ROTX, 5 );
	noise( ROTY, 5 );
	noise( ROTZ, 5 );
	noise( TRANX, 5 );
	noise( TRANY, 5 );
	noise( TRANZ, 5 );
	noise( ZOOM, 5 );
}


/*
 *  Iris 3-D Unix plot reader
 */

/* Map RGB onto 10x10x10 color cube, giving index in range 0..999 */
#define MAP_RESERVED	16		/* # slots reserved by MEX */
#define COLOR_APPROX(r,g,b)	\
	((r/26)+ (g/26)*10 + (b/26)*100 + MAP_RESERVED)

/*
 *  UNIX-Plot integers are 16bit VAX order (little-endian) 2's complement.
 */
#define	geti(x)	{ (x) = getchar(); (x) |= (short)(getchar()<<8); }
#define	getb()	(getchar())

uplot( max, min )
Coord max[3], min[3];
{
	register int	c;
	int	x, y, z, x1, y1, z1, x2, y2, z2;
	int	r, g, b;
	long	l;

	while( (c = getchar()) != EOF ) {
		switch( c ) {
		case 'e':
			l = getcolor();
			color( BLACK );
			clear();
			color( l );
			break;
		case 'p':
			geti(x);
			geti(y);
			pnti( x, y, 0 );
			break;
		case 'm':
			geti(x);
			geti(y);
			movei( x, y, 0 );
			break;
		case 'n':
			geti(x);
			geti(y);
			drawi( x, y, 0 );
			break;
		case 'c':
			/* CIRCLE! XXX */
			break;
		case 'l':
			geti(x1);
			geti(y1);
			geti(x2);
			geti(y2);
			movei( x1, y1, 0 );
			drawi( x2, y2, 0 );
			break;
		case 's':
			geti(x1);
			geti(y1);
			geti(x2);
			geti(y2);
			min[0] = x1; min[1] = y1;
			max[0] = x2; max[1] = y2;
			min[2] = -32768.0; max[2] = 32768.0;
			break;
		case 'a':
			geti(x);
			geti(y);
			geti(x1);
			geti(y1);
			geti(x2);
			geti(y2);
			/* ARC XXX */
			break;
		case 'f':
		case 't':
			eat_string();
			break;
		case 'M':
			geti(x);
			geti(y);
			geti(z);
			movei( x, y, z );
			break;
		case 'N':
			geti(x);
			geti(y);
			geti(z);
			drawi( x, y, z );
			break;
		case 'P':
			geti(x);
			geti(y);
			geti(z);
			pnti( x, y, z );
			break;
		case 'L':
			geti(x1);
			geti(y1);
			geti(z1);
			geti(x2);
			geti(y2);
			geti(z2);
			movei( x1, y1, z1 );
			drawi( x2, y2, z2 );
			break;
		case 'S':
			geti(x1);
			geti(y1);
			geti(z1);
			geti(x2);
			geti(y2);
			geti(z2);
			min[0] = x1; min[1] = y1; min[2] = z1;
			max[0] = x2; max[1] = y2; max[2] = z2;
			break;
		case 'C':
			r = getb();
			g = getb();
			b = getb();
			if( ismex() )
				color( COLOR_APPROX(r,g,b) );
			else
				color( (b&0xf0)<<4 | (g&0xf0) | (r>>4) );
			break;
		default:
			fprintf( stderr, "uplot: unrecognized command '%c'\n", c );
			break;
		}
	}
}

eat_string()
{
	int	c;

	while( (c = getchar()) != '\n' && c != EOF )
		;
}
#endif sgi
