/*
 *			F B Z O O M . C
 *
 * Function -
 *	Dynamicly modify Ikonas Zoom and Window parameters,
 *	using VI and/or EMACS-like keystrokes on a regular terminal.
 *
 *  Authors -
 *	Bob Suckling
 *	Michael John Muuss
 *	Gary S. Moss
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
#include "fb.h"

extern int	getopt();
extern char	*optarg;
extern int	optind;

/* Zoom rate and limits */
#define MinZoom		(1)

/* Pan limits */
#define MaxXPan		(fb_getwidth(fbp)-1)
#define MaxYPan		(fb_getheight(fbp)-1)
#define MinPan		(0)

static int PanFactor;			/* Speed with whitch to pan.	*/
static int xPan, yPan;			/* Pan Location.		*/
static int xZoom, yZoom;		/* Zoom Factor.			*/
static int new_xPan, new_yPan;
static int new_xZoom, new_yZoom;

static int	scr_width = 0;		/* screen size */
static int	scr_height = 0;
static int	toggle_pan = 0;		/* Reverse sense of pan commands? */
static char	*framebuffer = NULL;
static FBIO	*fbp;

static char usage[] = "\
Usage: fbzoom [-h] [-F framebuffer]\n\
	[-{sS} squarescrsize] [-{wW} scr_width] [-{nN} scr_height]\n";

main(argc, argv )
int argc;
char **argv;
{
	if( ! pars_Argv( argc, argv ) ) {
		(void)fputs(usage, stderr);
		exit(1);
	}
	if( (fbp = fb_open( framebuffer, scr_width, scr_height )) == NULL )
		exit(1);

	if( optind+4 == argc ) {
		xPan = atoi( argv[optind+0] );
		yPan = atoi( argv[optind+1] );
		xZoom = atoi( argv[optind+2] );
		yZoom = atoi( argv[optind+3] );
		fb_view(fbp, xPan, yPan, xZoom, yZoom);
	}

#if 0
	xZoom = 1;
	yZoom = 1;
	xPan = fb_getwidth(fbp)/2;
	yPan = fb_getheight(fbp)/2;
#else
	fb_getview(fbp, &xPan, &yPan, &xZoom, &yZoom);
#endif

	/* Set RAW mode */
	save_Tty( 0 );
	set_Raw( 0 );
	clr_Echo( 0 );

	PanFactor = fb_getwidth(fbp)/16;
	if( PanFactor < 2 )  PanFactor = 2;

	new_xPan = xPan;
	new_yPan = yPan;
	new_xZoom = xZoom;
	new_yZoom = yZoom;
	do  {
		/* Clip values against Min/Max */
		if (new_xPan > MaxXPan) new_xPan = MaxXPan;
		if (new_xPan < MinPan) new_xPan = MinPan;
		if (new_yPan > MaxYPan) new_yPan = MaxYPan;
		if (new_yPan < MinPan) new_yPan = MinPan;
		if (xZoom < MinZoom) xZoom = MinZoom;
		if (yZoom < MinZoom) yZoom = MinZoom;

		if( new_xPan != xPan || new_yPan != yPan
		  || new_xZoom != xZoom || new_yZoom != yZoom ) {
		  	/* values have changed, write them */
			if( fb_view(fbp, new_xPan, new_yPan,
			    new_xZoom, new_yZoom) >= 0 ) {
				/* good values, save them */
				xPan = new_xPan;
				yPan = new_yPan;
				xZoom = new_xZoom;
				yZoom = new_yZoom;
			} else {
				/* bad values, reset to old ones */
				new_xPan = xPan;
				new_yPan = yPan;
				new_xZoom = xZoom;
				new_yZoom = yZoom;
			}
		}
#if 0
		(void) fprintf( stdout,
				"Zoom: %2d %2d  Center Pixel: %4d %4d            \r",
				xZoom, yZoom, xPan, yPan );
#else
		(void) fprintf( stdout,
				"Center Pixel: %4d %4d   Zoom: %2d %2d            \r",
				xPan, yPan, xZoom, yZoom );
#endif
		(void) fflush( stdout );
	}  while( doKeyPad() );

	reset_Tty( 0 );
	(void) fb_view( fbp, xPan, yPan, xZoom, yZoom );
	(void) fb_close( fbp );
	(void) fprintf( stdout,  "\n");	/* Move off of the output line.	*/
	return	0;
}

char help[] = "\r\n\
Both VI and EMACS motions work.\r\n\
b ^V	zoom Bigger (*2)\r\n\
s	zoom Smaller (*0.5)\r\n\
+	zoom Bigger (+1)\r\n\
-	zoom Smaller (-1)\r\n\
h B 	move Right (1)\r\n\
j P	move Up (1)\r\n\
k N	move Down (1)\r\n\
l F	move Left (1)\r\n\
H ^B	move Right (many)\r\n\
J ^P	move Up (many)\r\n\
K ^N	move Down (many)\r\n\
L ^F	move Left (many)\r\n\
c	goto Center\r\n\
z	zoom 1 1\r\n\
r	Reset to normal\r\n\
q	Exit\r\n\
RETURN	Exit\r\n";

#define ctl(x)	(x&037)

doKeyPad()
{ 
	register int ch;

	if( (ch = getchar()) == EOF )
		return	0;		/* done */
	ch &= ~0x80;			/* strip off parity bit */
	switch( ch ) {
	default :
		(void) fprintf( stdout,
				"\r\n'%c' bad -- Type ? for help\r\n",
				ch );
	case '?' :
		(void) fprintf( stdout, "\r\n%s", help );
		break;

	case 'T' :
		toggle_pan = 1 - toggle_pan;
		break;
	case '\r' :				/* Done, leave "as is" */
	case '\n' :
	case 'q' :
	case 'Q' :				
		return	0;

	case 'c' :				/* Reset Pan (Center) */
	case 'C' :
		new_xPan = fb_getwidth(fbp)/2;
		new_yPan = fb_getheight(fbp)/2;
		break;
	case 'z' :				/* Reset Zoom */
	case 'Z' :
		new_xZoom = 1;
		new_yZoom = 1;
		break;
	case 'r' :				/* Reset Pan and Zoom */
	case 'R' :
		new_xZoom = 1;
		new_yZoom = 1;
		new_xPan = fb_getwidth(fbp)/2;
		new_yPan = fb_getheight(fbp)/2;
		break;

	case ctl('v') :
	case 'b' :				/* zoom BIG binary */
		new_xZoom *= 2;
		new_yZoom *= 2;
		break;
	case '+' :				/* zoom BIG incr */
		new_xZoom++;
		new_yZoom++;
		break;
	case 's' :				/* zoom small binary */
		new_xZoom /= 2;
		new_yZoom /= 2;
		break;
	case '-' :				/* zoom small incr */
		--new_xZoom;
		--new_yZoom;
		break;

	case '>' :				/* X Zoom */
		new_xZoom *= 2;
		break;
	case '.' :
		++new_xZoom;
		break;
	case '<' :
		new_xZoom /= 2;
		break;
	case ',' :
		--new_xZoom;
		break;

	case ')' :				/* Y Zoom */
		new_yZoom *= 2;
		break;
	case '0' :
		++new_yZoom;
		break;
	case '(' :
		new_yZoom /= 2;
		break;
	case '9' :
		--new_yZoom;
		break;

	case 'F' :
	case 'l' :				/* move RIGHT.	*/
		new_xPan += 1 - 2 * toggle_pan;
		break;
	case ctl('f') :
	case 'L' :
		new_xPan += PanFactor * (1 - 2 * toggle_pan);
		break;
	case 'N' :
	case 'j' :				/* move DOWN.	*/
		new_yPan -= 1 - 2 * toggle_pan;
		break;
	case ctl('n') :
	case 'J' :
		new_yPan -= PanFactor * (1 - 2 * toggle_pan);
		break;
	case 'P' :
	case 'k' :				/* move UP.	*/
		new_yPan += 1 - 2 * toggle_pan;
		break;
	case ctl('p') :
	case 'K' :
		new_yPan += PanFactor * (1 - 2 * toggle_pan);
		break;
	case 'B' :
	case 'h' :				/* move LEFT.	*/
		new_xPan -= 1 - 2 * toggle_pan;
		break;
	case ctl('b') :
	case 'H' :
		new_xPan -= PanFactor * (1 - 2 * toggle_pan);
		break;
	}
	return	1;		/* keep going */
}

/*	p a r s _ A r g v ( )
 */
int
pars_Argv( argc, argv )
register char	**argv;
{
	register int	c;

	while( (c = getopt( argc, argv, "hTF:s:S:w:W:n:N:" )) != EOF )  {
		switch( c )  {
		case 'h':
			/* high-res */
			scr_height = scr_width = 1024;
			break;
		case 'T':
			/* reverse the sense of pan commands */
			toggle_pan = 1;
			break;
		case 'F':
			framebuffer = optarg;
			break;
		case 's':
		case 'S':
			scr_height = scr_width = atoi(optarg);
			break;
		case 'w':
		case 'W':
			scr_width = atoi(optarg);
			break;
		case 'n':
		case 'N':
			scr_height = atoi(optarg);
			break;

		default:		/* '?' */
			return(0);
		}
	}
	return	1;
}
