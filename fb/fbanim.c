/*
 *			F B A N I M . C
 *
 * Function -
 *	Dynamicly modify framebuffer Zoom and Window parameters,
 *	to flip betwen sub-images, giving an inexpensive animation
 *	effect.
 *
 *  Author -
 *	Michael John Muuss
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
#include "fb.h"

#if defined(BSD)
#include <sys/time.h>		/* for struct timeval */
#define HAS_TIMEVAL	yes
#endif

#if defined(sgi)
#	if !defined(mips) || defined(SGI4D_Rel2)
		/* 3D systems, and Rel2 4D systems. */
#		include <bsd/sys/types.h>
#		include <bsd/sys/time.h>
#	else
		/* Rel3 4D systems got it right */
#		include <sys/types.h>
#		include <sys/time.h>
#	endif
#	define HAS_TIMEVAL yes
#endif

#ifdef HAS_TIMEVAL
struct timeval	tv;
#endif

extern int	getopt();
extern char	*optarg;
extern int	optind;

void		newframe();

FBIO	*fbp;
int	screen_width;		/* Number of pixels/line in frame buffer */
int	screen_height;
int	verbose = 0;
int	rocking = 0;

int	subimage_width;		/* subimage width */
int	subimage_height;		/* subimage height */
int nframes;			/* number of frames */
int im_line;			/* Number of images across the screen */
int fps;			/* frames/sec */
int passes = 100;		/* limit on number of passes */
int inverse;			/* for old 4th quadrant sequences */

char Usage[] = "\
Usage: fbanim [-h -i -r -v] [-p passes]\n\
	[-S squarescrsize] [-W scr_width] [-N scr_height]\n\
	[-s square_subimage_size] [-w subimage_width] [-n subimage_height]\n\
	subimage_width nframes [fps]\n";

get_args( argc, argv )
register char **argv;
{
	register int c;

	while ( (c = getopt( argc, argv, "s:w:n:hirvp:S:W:N:" )) != EOF )  {
		switch( c )  {
		case 's':
			subimage_width = subimage_height = atoi(optarg);
			break;
		case 'w':
			subimage_width = atoi(optarg);
			break;
		case 'n':
			subimage_height = atoi(optarg);
			break;
		case 'i':
			inverse = 1;
			break;
		case 'p':
			passes = atoi(optarg);
			if(passes<1)  passes=1;
			break;
		case 'r':
			rocking = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'h':
			/* high-res screen */
			screen_width = screen_height = 1024;
			break;
		case 'S':
			screen_height = screen_width = atoi(optarg);
			break;
		case 'W':
			screen_width = atoi(optarg);
			break;
		case 'N':
			screen_height = atoi(optarg);
			break;

		default:		/* '?' */
			return(0);
		}
	}

	if( optind+1 >= argc )	/* two mandatory positional args */
		return(0);
	return(1);		/* OK */
}

main(argc, argv )
char **argv;
{
	register int i;

	if ( !get_args( argc, argv ) )  {
		(void)fputs(Usage, stderr);
		exit( 1 );
	}

	/* If not given with -s & -n, use (old) positional param (compat) */
	if( subimage_width <= 0 || subimage_height <= 0 )  {
		subimage_width = subimage_height = atoi(argv[optind]);
		if( subimage_width == 0 ) {
			fprintf(stderr,"fbanim: must specify image size\n");
			exit( 2 );
		}
	}
	nframes = atoi(argv[optind+1]);
	if( optind+2 >= argc )
		fps = 8;
	else
		fps = atoi(argv[optind+2]);

#ifdef HAS_TIMEVAL
	if( fps <= 1 )  {
		tv.tv_sec = fps ? 1 : 4;
		tv.tv_usec = 0;
	} else {
		tv.tv_sec = 0;
		tv.tv_usec = 1000000/fps;
	}
#endif

	if( (fbp = fb_open( NULL, screen_width, screen_height )) == NULL )  {
		fprintf(stderr,"fbanim: fb_open failed\n");
		exit(12);
	}
	screen_width = fb_getwidth(fbp);
	screen_height = fb_getheight(fbp);

	im_line = screen_width/subimage_width;	/* number of images across line */

	fb_zoom( fbp, screen_width/subimage_width, screen_height/subimage_height );

	while(passes-- > 0)  {
		if( !rocking )  {
			/* Play from start to finish, over and over */
			for( i=0; i<nframes; i++ )
				newframe(i);
		} else {
			/* Play from start to finish and back */
			for( i=0; i<nframes; i++ )
				newframe(i);
			while(i-->0)
				newframe(i);
		}
	}
	fb_close( fbp );
}

void
newframe(i)
register int i;
{
	register int	xPan, yPan;		/* Pan Location */

	xPan = (i%im_line)*subimage_width+subimage_width/2;
	yPan = (i/im_line)*subimage_height+subimage_height/2;
	if( inverse )
		yPan = screen_width - yPan;
	if( verbose )  {
		printf("%3d: %3d %3d\n", i, xPan, yPan);
		fflush( stdout );
	}
	fb_window( fbp, xPan, yPan );
#ifdef HAS_TIMEVAL
	(void)select( 0, 0, 0, 0, &tv );
#else
	(void)sleep(1);	/* best I can do, sorry */
#endif
}
