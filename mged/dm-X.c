/*
 *			D M - X . C
 *
 *  An X Window System interface for MGED.
 *  X11R2.  Color support is yet to be implemented.
 *
 *  Author -
 *	Phillip Dykstra
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1988 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include "machine.h"
#include "vmath.h"
#include "mater.h"
#include "raytrace.h"
#include "./ged.h"
#include "./dm.h"
#include "externs.h"
#include "./solid.h"
#include <X11/X.h>
#define XLIB_ILLEGAL_ACCESS	/* necessary on facist SGI 5.0.1 */
#include <X11/Xlib.h>
#include <X11/Xutil.h>

static void	label();
static void	draw();
static void	checkevents();
static int	xsetup();

/* Display Manager package interface */

#define PLOTBOUND	1000.0	/* Max magnification in Rot matrix */
int	X_open();
void	X_close();
int	X_input();
void	X_prolog(), X_epilog();
void	X_normal(), X_newrot();
void	X_update();
void	X_puts(), X_2d_line(), X_light();
int	X_object();
unsigned X_cvtvecs(), X_load();
void	X_statechange(), X_viewchange(), X_colorchange();
void	X_window(), X_debug();

struct dm dm_X = {
	X_open, X_close,
	X_input,
	X_prolog, X_epilog,
	X_normal, X_newrot,
	X_update,
	X_puts, X_2d_line,
	X_light,
	X_object,	X_cvtvecs, X_load,
	X_statechange,
	X_viewchange,
	X_colorchange,
	X_window, X_debug,
	0,				/* no displaylist */
	0,				/* multi-window */
	PLOTBOUND,
	"X", "X Window System (X11)"
};

extern struct device_values dm_values;	/* values read from devices */

static vect_t clipmin, clipmax;		/* for vector clipping */
static int	height, width;

static Display	*dpy;			/* X display pointer */
static Window	win;			/* X window */
static GC	gc;			/* X Graphics Context */
static unsigned long	black,gray,white,yellow,red,blue;
static XFontStruct *fontstruct;		/* X Font */

static int	no_faceplate = 0;

/*
 * Display coordinate conversion:
 *  GED is using -2048..+2048,
 *  X is 0..width,0..height
 */
#define	GED_TO_Xx(x)	(((x)/4096.0+0.5)*width)
#define	GED_TO_Xy(x)	((0.5-(x)/4096.0)*height)
/*XXX
#define X_TO_GED(x)	(x)
*/

/*
 *			X _ O P E N
 *
 * Fire up the display manager, and the display processor.
 *
 */
X_open()
{
	char	line[64];
	char	hostname[80];
	char	display[80];
	char	*envp;

	/* get or create the default display */
	if( (envp = getenv("DISPLAY")) == NULL ) {
		/* Env not set, use local host */
		gethostname( hostname, 80 );
		(void)sprintf( display, "%s:0", hostname );
		envp = display;
	}

	(void)printf("X Display [%s]? ", envp );
	(void)gets( line );		/* Null terminated */
	if( feof(stdin) )  quit();

	if( line[0] != '\0' ) {
		if( xsetup(line) ) {
			return(1);		/* BAD */
		}
	} else {
		if( xsetup(envp) ) {
			return(1);	/* BAD */
		}
	}

	return(0);			/* OK */
}

/*
 *  			X _ C L O S E
 *  
 *  Gracefully release the display.
 */
void
X_close()
{
	XCloseDisplay( dpy );
}

/*
 *			X _ P R O L O G
 *
 * There are global variables which are parameters to this routine.
 */
void
X_prolog()
{
	if( !dmaflag )
		return;

	XClearWindow( dpy, win );

	/* Put the center point up */
	draw( 0, 0, 0, 0 );
}

/*
 *			X _ E P I L O G
 */
void
X_epilog()
{
	XFlush( dpy );
	return;
}

/*
 *  			X _ N E W R O T
 *  Stub.
 */
/* ARGSUSED */
void
X_newrot(mat)
mat_t mat;
{
	return;
}

/*
 *  			X _ O B J E C T
 *  
 *  Set up for an object, transformed as indicated, and with an
 *  object center as specified.  The ratio of object to screen size
 *  is passed in as a convienience.
 *
 *  Returns 0 if object could be drawn, !0 if object was omitted.
 */
/* ARGSUSED */
int
X_object( sp, mat, ratio, white )
register struct solid *sp;
mat_t mat;
double ratio;
{
	static vect_t   pnt;
	register struct rt_vlist	*vp;
	int useful = 0;
	XSegment segbuf[1024];		/* XDrawSegments list */
	XSegment *segp;			/* current segment */
	XGCValues gcv;
	int	nseg;			/* number of segments */
	int	x, y;
	int	lastx = 0;
	int	lasty = 0;

	if( sp->s_soldash ) {
		XSetLineAttributes( dpy, gc, 1, LineOnOffDash, CapButt, JoinMiter );
	} else {
		XSetLineAttributes( dpy, gc, 1, LineSolid, CapButt, JoinMiter );
	}

	nseg = 0;
	segp = segbuf;
	for( RT_LIST_FOR( vp, rt_vlist, &(sp->s_vlist) ) )  {
		register int	i;
		register int	nused = vp->nused;
		register int	*cmd = vp->cmd;
		register point_t *pt = vp->pt;

		/* Viewing region is from -1.0 to +1.0 */
		/* 2^31 ~= 2e9 -- dynamic range of a long int */
		/* 2^(31-11) = 2^20 ~= 1e6 */
		/* Integerize and let the X server do the clipping */
		for( i = 0; i < nused; i++,cmd++,pt++ )  {
			static vect_t	start, fin;
			switch( *cmd )  {
			case RT_VLIST_POLY_START:
				continue;
			case RT_VLIST_POLY_MOVE:
			case RT_VLIST_LINE_MOVE:
				/* Move, not draw */
				MAT4X3PNT( pnt, mat, *pt );
				if( pnt[0] < -1e6 || pnt[0] > 1e6 ||
				    pnt[1] < -1e6 || pnt[1] > 1e6 )
					continue; /* omit this point (ugh) */
				pnt[0] *= 2047;
				pnt[1] *= 2047;
				x = GED_TO_Xx(pnt[0]);
				y = GED_TO_Xy(pnt[1]);
				lastx = x;
				lasty = y;
				continue;
			case RT_VLIST_POLY_DRAW:
			case RT_VLIST_POLY_END:
			case RT_VLIST_LINE_DRAW:
				/* draw */
				MAT4X3PNT( pnt, mat, *pt );
				if( pnt[0] < -1e6 || pnt[0] > 1e6 ||
				    pnt[1] < -1e6 || pnt[1] > 1e6 )
					continue; /* omit this point (ugh) */
				/* Integerize and let the X server do the clipping */
				/*XXX Color */
				gcv.foreground = black;
				XChangeGC( dpy, gc, GCForeground, &gcv );

				pnt[0] *= 2047;
				pnt[1] *= 2047;
				x = GED_TO_Xx(pnt[0]);
				y = GED_TO_Xy(pnt[1]);

				segp->x1 = lastx;
				segp->y1 = lasty;
				segp->x2 = x;
				segp->y2 = y;
				nseg++;
				segp++;
				lastx = x;
				lasty = y;
				useful = 1;
				if( nseg == 1024 ) {
					XDrawSegments( dpy, win, gc, segbuf, nseg );
					if( white ) {
						int	i;
						/* XXX - width and height don't work on Sun! */
						/* Thus the following gross hack */
						segp = segbuf;
						for( i = 0; i < nseg; i++ ) {
							segp->x1++;
							segp->y1++;
							segp->x2++;
							segp->y2++;
							segp++;
						}
						XDrawSegments( dpy, win, gc, segbuf, nseg );
					}
					nseg = 0;
					segp = segbuf;
				}
				break;
			}
		}
	}
	if( nseg ) {
		XDrawSegments( dpy, win, gc, segbuf, nseg );
		if( white ) {
			int	i;
			/* XXX - width and height don't work on Sun! */
			/* Thus the following gross hack */
			segp = segbuf;
			for( i = 0; i < nseg; i++ ) {
				segp->x1++;
				segp->y1++;
				segp->x2++;
				segp->y2++;
				segp++;
			}
			XDrawSegments( dpy, win, gc, segbuf, nseg );
		}
	}

	return(useful);
}

/*
 *			X _ N O R M A L
 *
 * Restore the display processor to a normal mode of operation
 * (ie, not scaled, rotated, displaced, etc).
 * Turns off windowing.
 */
void
X_normal()
{
	return;
}

/*
 *			X _ U P D A T E
 *
 * Transmit accumulated displaylist to the display processor.
 */
void
X_update()
{
	XFlush( dpy );
}

/*
 *			X _ P U T S
 *
 * Output a string into the displaylist.
 * The starting position of the beam is as specified.
 */
/* ARGSUSED */
void
X_puts( str, x, y, size, color )
register char *str;
{
	XGCValues gcv;
	unsigned long fg;

	switch( color )  {
	case DM_BLACK:
		fg = black;
		break;
	case DM_RED:
		fg = red;
		break;
	case DM_BLUE:
		fg = blue;
		break;
	default:
	case DM_YELLOW:
		fg = yellow;
		break;
	case DM_WHITE:
		fg = gray;
		break;
	}
	gcv.foreground = fg;
	XChangeGC( dpy, gc, GCForeground, &gcv );
	label( (double)x, (double)y, str );
}

/*
 *			X _ 2 D _ G O T O
 *
 */
void
X_2d_line( x1, y1, x2, y2, dashed )
int x1, y1;
int x2, y2;
int dashed;
{
	XGCValues gcv;

	gcv.foreground = yellow;
	XChangeGC( dpy, gc, GCForeground, &gcv );
	if( dashed ) {
		XSetLineAttributes( dpy, gc, 1, LineOnOffDash, CapButt, JoinMiter );
	} else {
		XSetLineAttributes( dpy, gc, 1, LineSolid, CapButt, JoinMiter );
	}
	draw( x1, y1, x2, y2 );
}

/*
 *			X _ I N P U T
 *
 * Execution must suspend in this routine until a significant event
 * has occured on either the command stream,
 * unless "noblock" is set.
 *
 * Returns:
 *	0 if no command waiting to be read,
 *	1 if command is waiting to be read.
 */
X_input( cmd_fd, noblock )
{
	register long readfds;
	register int	i;

	/*
	 * Check for input on the keyboard, mouse, or window system.
	 *
	 * Suspend execution until either
	 *  1)  User types a full line
	 *  2)  Mouse or Window input arrives
	 *  3)  The timelimit on SELECT has expired
	 *
	 * If a RATE operation is in progress (zoom, rotate, slew)
	 * in which we still have to update the display,
	 * do not suspend execution.
	 */
	do {
		readfds = 0;
		i = XPending( dpy );
		if( i > 0 ) {
			/* Don't select if we have some input! */
			break;
		}
		readfds = (1<<cmd_fd) | (1<<dpy->fd);
		if( noblock ) {
			readfds = bsdselect( readfds, 0, 0 );
		} else {
			readfds = bsdselect( readfds, 30*60, 0 );
		}
		if( readfds != 0 )
			break;
	} while( noblock == 0 );

	/* "rest" state */
	dm_values.dv_buttonpress = 0;
	dm_values.dv_flagadc = 0;
	dm_values.dv_penpress = 0;

	if( (i != 0) || (readfds & (1<<dpy->fd)) )
		checkevents();

	if( readfds & (1<<cmd_fd) )
		return(1);		/* command awaits */
	else
		return(0);		/* just peripheral stuff */
}

/* 
 *			X _ L I G H T
 */
/* ARGSUSED */
void
X_light( cmd, func )
int cmd;
int func;			/* BE_ or BV_ function */
{
	return;
}

/* ARGSUSED */
unsigned
X_cvtvecs( sp )
struct solid *sp;
{
	return( 0 );
}

/*
 * Loads displaylist
 */
unsigned
X_load( addr, count )
unsigned addr, count;
{
	(void)printf("X_load(x%x, %d.)\n", addr, count );
	return( 0 );
}

void
X_statechange( a, b )
int	a, b;
{
	/*
	 *  Based upon new state, possibly do extra stuff,
	 *  including enabling continuous tablet tracking,
	 *  object highlighting
	 */
	switch( b )  {
	case ST_VIEW:
		/* constant tracking OFF */
		XSelectInput( dpy, win, ExposureMask|ButtonPressMask|KeyPressMask );
		break;
	case ST_S_PICK:
	case ST_O_PICK:
	case ST_O_PATH:
		/* constant tracking ON */
		XSelectInput( dpy, win, PointerMotionMask|ExposureMask|ButtonPressMask|KeyPressMask );
		break;
	case ST_O_EDIT:
	case ST_S_EDIT:
		/* constant tracking OFF */
		XSelectInput( dpy, win, ExposureMask|ButtonPressMask|KeyPressMask );
		break;
	default:
		(void)printf("X_statechange: unknown state %s\n", state_str[b]);
		break;
	}
	/*X_viewchange( DM_CHGV_REDO, SOLID_NULL );*/
}

void
X_viewchange()
{
}

void
X_colorchange()
{
	color_soltab();		/* apply colors to the solid table */
}

/* ARGSUSED */
void
X_debug(lvl)
{
	XFlush( dpy );
	printf("flushed\n");
}

void
X_window(w)
register int w[];
{
	/* Compute the clipping bounds */
	clipmin[0] = w[1] / 2048.;
	clipmin[1] = w[3] / 2048.;
	clipmin[2] = w[5] / 2048.;
	clipmax[0] = w[0] / 2047.;
	clipmax[1] = w[2] / 2047.;
	clipmax[2] = w[4] / 2047.;
}

/*********XXX**********/
/*
 *  Called for 2d_line, and dot at center of screen.
 */
static void
draw( x1, y1, x2, y2 )
int	x1, y1;		/* from point */
int	x2, y2;		/* to point */
{
	int	sx1, sy1, sx2, sy2;

	sx1 = GED_TO_Xx( x1 );
	sy1 = GED_TO_Xy( y1 );
	sx2 = GED_TO_Xx( x2 );
	sy2 = GED_TO_Xy( y2 );

	if( sx1 == sx2 && sy1 == sy2 )
		XDrawPoint( dpy, win, gc, sx1, sy1 );
	else
		XDrawLine( dpy, win, gc, sx1, sy1, sx2, sy2 );
}

static void
label( x, y, str )
double	x, y;
char	*str;
{
	int	sx, sy;

	sx = GED_TO_Xx( x );
	sy = GED_TO_Xy( y );
	/* point is center of text? - seems like what MGED wants... */
	/* The following makes the menu look good, the rest bad */
	/*sy += (fontstruct->max_bounds.ascent + fontstruct->max_bounds.descent)/2;*/

	XDrawString( dpy, win, gc, sx, sy, str, strlen(str) );
}

#define	FONT	"6x10"

static XWMHints xwmh = {
	(InputHint|StateHint),		/* flags */
	False,				/* input */
	NormalState,			/* initial_state */
	0,				/* icon pixmap */
	0,				/* icon window */
	0, 0,				/* icon location */
	0,				/* icon mask */
	0				/* Window group */
};

static int
xsetup( name )
char	*name;
{
	char	hostname[80];
	char	display[80];
	char	*envp, *cp;
	char	line[80];
	unsigned long	bd, bg, fg, bw;
	XSizeHints xsh;
	XEvent	event;
	XGCValues gcv;
	XColor a_color;
	int a_screen;
	Colormap  a_cmap;

	width = height = 512;

	if( name == NULL || *name == '\0' ) {
		if( (envp = getenv("DISPLAY")) == NULL ) {
			/* Env not set, use local host */
			gethostname( hostname, 80 );
			sprintf( display, "%s:0", hostname );
			envp = display;
		}
	} else {
		envp = name;
	}

	/* Open the display - XXX see what NULL does now */
	if( (dpy = XOpenDisplay( envp )) == NULL ) {
		fprintf( stderr, "dm-X: Can't open X display\n" );
		return -1;
	}

#if 0
	/* Load the font to use */
	/* Answering this extra question all the time is irritating */
	(void)printf("Font [6x10]? ");
	(void)gets( line );		/* Null terminated */
	if( line[0] != NULL )
		cp = line;
	else
#endif
		cp = FONT;
#ifndef CRAY2
/* The Cray2 never returns from this call.  sigh.
 * Use the default font until the Cray libX11 is fixed.
 */
	if( (fontstruct = XLoadQueryFont(dpy, cp)) == NULL ) {
		fprintf( stderr, "dm-X: Can't open font\n" );
		return -1;
	}
#endif

	/* Get color map inddices for the colors we use. */
	black = BlackPixel( dpy, DefaultScreen(dpy) );
	white = WhitePixel( dpy, DefaultScreen(dpy) );

	a_screen = DefaultScreen(dpy);
	a_cmap = DefaultColormap(dpy, a_screen);
	a_color.red = 255<<8;
	a_color.green=0;
	a_color.blue=0;
	a_color.flags = DoRed | DoGreen| DoBlue;
	if ( ! XAllocColor(dpy, a_cmap, &a_color)) {
		fprintf( stderr, "dm-X: Can't Allocate red\n");
		return -1;
	}
	red = a_color.pixel;
	if ( red == white ) red = black;

	a_color.red = 200<<8;
	a_color.green=200<<8;
	a_color.blue=0<<8;
	a_color.flags = DoRed | DoGreen| DoBlue;
	if ( ! XAllocColor(dpy, a_cmap, &a_color)) {
		fprintf( stderr, "dm-X: Can't Allocate yellow\n");
		return -1;
	}
	yellow = a_color.pixel;
	if (yellow == white) yellow = black;

	a_color.red = 0;
	a_color.green=0;
	a_color.blue=255<<8;
	a_color.flags = DoRed | DoGreen| DoBlue;
	if ( ! XAllocColor(dpy, a_cmap, &a_color)) {
		fprintf( stderr, "dm-X: Can't Allocate blue\n");
		return -1;
	}
	blue = a_color.pixel;
	if (blue == white) blue = black;

	a_color.red = 128<<8;
	a_color.green=128<<8;
	a_color.blue=128<<8<<8;
	a_color.flags = DoRed | DoGreen| DoBlue;
	if ( ! XAllocColor(dpy, a_cmap, &a_color)) {
		fprintf( stderr, "dm-X: Can't Allocate gray\n");
		return -1;
	}
	gray = a_color.pixel;
	if (gray == white) gray = black;

	/* Select border, background, foreground colors,
	 * and border width.
	 */
	bd = BlackPixel( dpy, DefaultScreen(dpy) );
	bg = WhitePixel( dpy, DefaultScreen(dpy) );
	fg = BlackPixel( dpy, DefaultScreen(dpy) );
	bw = 1;

	/* Fill in XSizeHints struct to inform window
	 * manager about initial size and location.
	 */
	xsh.flags = (PSize);
	xsh.height = height + 10;
	xsh.width = width + 10;
	xsh.x = xsh.y = 0;

	win = XCreateSimpleWindow( dpy, DefaultRootWindow(dpy),
		xsh.x, xsh.y, xsh.width, xsh.height,
		bw, bd, bg );
	if( win == 0 ) {
		fprintf( stderr, "dm-X: Can't create window\n" );
		return -1;
	}

	/* Set standard properties for Window Managers */
	XSetStandardProperties( dpy, win, "MGED", "MGED", None, NULL, 0, &xsh );
	XSetWMHints( dpy, win, &xwmh );

	/* Create a Graphics Context for drawing */
#ifndef CRAY2
	gcv.font = fontstruct->fid;
#endif
	gcv.foreground = fg;
	gcv.background = bg;
#ifndef CRAY2
	gc = XCreateGC( dpy, win, (GCFont|GCForeground|GCBackground), &gcv );
#else
	gc = XCreateGC( dpy, win, (GCForeground|GCBackground), &gcv );
#endif

	XSelectInput( dpy, win, ExposureMask|ButtonPressMask|KeyPressMask );
	XMapWindow( dpy, win );

	while( 1 ) {
		XNextEvent( dpy, &event );
		if( event.type == Expose && event.xexpose.count == 0 ) {
			XWindowAttributes xwa;

			/* remove other exposure events */
			while( XCheckTypedEvent(dpy, Expose, &event) ) ;

			if( XGetWindowAttributes( dpy, win, &xwa ) == 0 )
				break;

			width = xwa.width;
			height = xwa.height;
			break;
		}
	}
	return	0;
}

/*
 *  Only called when we *know* there is at least one event to process.
 *  (otherwise we would block in XNextEvent)
 */
static void
checkevents()
{
	XEvent	event;
	KeySym	key;
	char keybuf[4];
	int cnt;
	XComposeStatus compose_stat;

	while( XPending( dpy ) > 0 ) {
		XNextEvent( dpy, &event );
		if( event.type == Expose ) {
			if( event.xexpose.count == 0 ) {
				XWindowAttributes xwa;
				XGetWindowAttributes( dpy, win, &xwa );
				height = xwa.height;
				width = xwa.width;
				rt_vls_printf( &dm_values.dv_string, "refresh\n");
			}
		} else if( event.type == MotionNotify ) {
			int	x, y;
			x = (event.xmotion.x/(double)width - 0.5) * 4095;
			y = (0.5 - event.xmotion.y/(double)height) * 4095;
			rt_vls_printf( &dm_values.dv_string, "M 0 %d %d\n",
				x, y );
		} else if( event.type == ButtonPress ) {
			/* There may also be ButtonRelease events */
			int	x, y;
			/* In MGED this is a "penpress" */
			x = (event.xbutton.x/(double)width - 0.5) * 4095;
			y = (0.5 - event.xbutton.y/(double)height) * 4095;
			switch( event.xbutton.button ) {
			case Button1:
				/* Left mouse: Zoom out */
				rt_vls_strcat( &dm_values.dv_string, "zoom 0.5\n");
				break;
			case Button2:
				/* Middle mouse, up to down transition */
				rt_vls_printf( &dm_values.dv_string, "M 1 %d %d\n",
					x, y);
				break;
			case Button3:
				/* Right mouse: Zoom in */
				rt_vls_strcat( &dm_values.dv_string, "zoom 2\n");
				break;
			}
		} else if( event.type == ButtonRelease ) {
			int	x, y;
			x = (event.xbutton.x/(double)width - 0.5) * 4095;
			y = (0.5 - event.xbutton.y/(double)height) * 4095;
			switch( event.xbutton.button ) {
			case Button1:
				/* Left mouse: Zoom out.  Do nothing more */
				break;
			case Button2:
				/* Middle mouse, down to up transition */
				rt_vls_printf( &dm_values.dv_string, "M 0 %d %d\n",
					x, y);
				break;
			case Button3:
				/* Right mouse: Zoom in.  Do nothing more. */
				break;
			}
		} else if( event.type == KeyPress ) {
		    register int i;
			/* Turn these into MGED "buttonpress" or knob functions */

		    cnt = XLookupString(&event.xkey, keybuf, sizeof(keybuf),
						&key, &compose_stat);

		    for(i=0 ; i < cnt ; i++){

			switch( *keybuf ) {
			case '?':
				fprintf( stderr, "\nKey Help Menu:\n\
0	Zero 'knobs'\n\
x	Increase xrot\n\
y	Increase yrot\n\
z	Increase zrot\n\
X	Increase Xslew\n\
Y	Increase Yslew\n\
Z	Increase Zslew\n\
f	Front view\n\
t	Top view\n\
b	Bottom view\n\
l	Left view\n\
r	Right view\n\
R	Rear view\n\
3	35,25 view\n\
4	45,45 view\n\
F	Toggle faceplate\n\
" );
				break;
			case 'F':
				/* Toggle faceplate on/off */
				no_faceplate = !no_faceplate;
				rt_vls_strcat( &dm_values.dv_string,
					no_faceplate ?
					"set faceplate=0\n" :
					"set faceplate=1\n" );
				break;
			case '0':
				dm_values.dv_xjoy = 0;
				dm_values.dv_yjoy = 0;
				dm_values.dv_zjoy = 0;
				dm_values.dv_xslew = 0;
				dm_values.dv_yslew = 0;
				dm_values.dv_zslew = 0;
				break;
			case 'x':
				/* 6 degrees per unit */
				dm_values.dv_xjoy += 0.1;
				break;
			case 'y':
				dm_values.dv_yjoy += 0.1;
				break;
			case 'z':
				dm_values.dv_zjoy += 0.1;
				break;
			case 'X':
				/* viewsize per unit */
				dm_values.dv_xslew += 0.1;
				break;
			case 'Y':
				dm_values.dv_yslew += 0.1;
				break;
			case 'Z':
				dm_values.dv_zslew += 0.1;
				break;
			case 'f':
				rt_vls_strcat( &dm_values.dv_string,
					"press front\n");
				break;
			case 't':
				rt_vls_strcat( &dm_values.dv_string,
					"press top\n");
				break;
			case 'b':
				rt_vls_strcat( &dm_values.dv_string,
					"press bottom\n");
				break;
			case 'l':
				rt_vls_strcat( &dm_values.dv_string,
					"press left\n");
				break;
			case 'r':
				rt_vls_strcat( &dm_values.dv_string,
					"press right\n");
				break;
			case 'R':
				rt_vls_strcat( &dm_values.dv_string,
					"press rear\n");
				break;
			case '3':
				rt_vls_strcat( &dm_values.dv_string,
					"press 35,25\n");
				break;
			case '4':
				rt_vls_strcat( &dm_values.dv_string,
					"press 45,45\n");
				break;
			default:
				printf("dm-X: The key '%c' is not defined\n", key);
				break;
			}
		    }
		} else
			fprintf( stderr, "Unknown event type\n" );
	}
}
