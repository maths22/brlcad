/*
 *			D M - T E K 4 1 0 9 . C
 *
 * A modified version of the Tektronix 4014 display driver for
 * Tektronix 4109 compatible displays.
 *
 *  Authors -
 *	Michael John Muuss
 *      Glenn E. Martin (NRTC)
 *	Stephen Hunter Willson (NRTC) <willson@nrtc-gremlin.ARPA>
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

/*  Notes:
    You almost certainly want to use this with two terminals.  Otherwise
you can't use the crosshairs to manipulate the image.
    The following keys can be pressed on the 4109 (while the cursor
is showing):
    b -- make objects bigger (zoom in)
    s -- make objects smaller (zoom out)
    Z -- print out current x and y coordinates of crosshair
    ' ' -- pick thing or coordinates pointed at
    . -- slew view

    The cursor speed is set to a moderate rate (5); pressing SHIFT
while moving the cursor increases the rate to fast (10).  This
is different from the factory default.


	Stephen Hunter Willson,
	NRTC
*/




#include <stdio.h>
#include "./machine.h"	/* special copy */
#include "../h/vmath.h"
#include "ged.h"
#include "dm.h"
#include "solid.h"

extern void	perror();

typedef unsigned char u_char;

/* Display Manager package interface */
 
#define TEKBOUND	1000.0	/* Max magnification in Rot matrix */
int	Tek4109_open();
void	Tek4109_close();
int	Tek4109_input();
void	Tek4109_prolog(), Tek4109_epilog();
void	Tek4109_normal(), Tek4109_newrot();
void	Tek4109_update();
void	Tek4109_puts(), Tek4109_2d_line(), Tek4109_light();
int	Tek4109_object();
unsigned Tek4109_cvtvecs(), Tek4109_load();
void	Tek4109_statechange(), Tek4109_viewchange(), Tek4109_colorchange();
void	Tek4109_window(), Tek4109_debug();

struct dm dm_Tek4109 = {
	Tek4109_open, Tek4109_close,
	Tek4109_input,
	Tek4109_prolog, Tek4109_epilog,
	Tek4109_normal, Tek4109_newrot,
	Tek4109_update,
	Tek4109_puts, Tek4109_2d_line,
	Tek4109_light,
	Tek4109_object,
	Tek4109_cvtvecs, Tek4109_load,
	Tek4109_statechange,
	Tek4109_viewchange,
	Tek4109_colorchange,
	Tek4109_window, Tek4109_debug,
	0,				/* no displaylist */
	TEKBOUND,
	"tek4109", "Tektronix 4109"		/* NRTC */
};

struct timeval	{			/* needed for select() */
	long	tv_sec;			/* seconds */
	long	tv_usec;		/* microseconds */
};


extern struct device_values dm_values;	/* values read from devices */

static vect_t clipmin, clipmax;		/* for vector clipping */
static int oloy = -1;
static int ohiy = -1;
static int ohix = -1;
static int oextra = -1;

#define BELL	007
#define	FF	014		/* Form Feed  */
#define SUB	032		/* Turn on graphics cursor */
#define GS	035		/* Enter Graphics Mode (1st vec dark) */
#define ESC	033		/* Escape */
#define US	037		/* Enter Alpha Mode */

static int second_fd;		/* fd of Tektronix if not /dev/tty */
static FILE *outfp;		/* Tektronix device to output on */
static char ttybuf[BUFSIZ];

static void tekmove(), tekcont();

/*
 * Display coordinate conversion:
 *  Tektronix is using 0..4096
 *  GED is using -2048..+2048
 */
 

#define	GED_TO_TEK(x)	(((x)+2048) * 780 / 1024)
#define TEK_TO_GED(x)	(((x) * 1024 / 780) - 2048)

/*
 *			T E K _ O P E N
 *
 * Fire up the display manager, and the display processor.
 *
 */
Tek4109_open()
{
	char line[64], line2[64];

	(void)printf("Output tty [stdout]? ");
	(void)gets( line );		/* Null terminated */
	if( feof(stdin) )  
		quit();
	if( line[0] != '\0' )  {
#if VMS
		if( (outfp = fopen(line,"r+")) == NULL )  {
			if( (outfp = fopen(line,"r+w")) == NULL )  {
				perror(line);
				return(1);		/* BAD */
			}
		}
#else
		if( (outfp = fopen(line,"r+w")) == NULL )  {
			(void)sprintf( line2, "/dev/tty%s%c", line, '\0' );
			if( (outfp = fopen(line2,"r+w")) == NULL )  {
				perror(line);
				return(1);		/* BAD */
			}
		}
#endif
		second_fd = fileno(outfp);
	} 
	else 
	{
#if VMS
		if( (outfp = fopen("SYS$OUTPUT","r+")) == NULL )
#else
		if( (outfp = fopen("/dev/tty","r+w")) == NULL )
#endif
			return(1);	/* BAD */
		second_fd = 0;		/* no second filedes */
	}
	setbuf( outfp, ttybuf );
	fprintf(outfp,"%c%%!0",ESC);	/* Place in TEK mode */
	fprintf(outfp, "%cNF1", ESC);	/* Set dc1/dc3 flow ctrl */
	fprintf(outfp,"%cMCA>B8:",ESC);	/* Set Graphics Font Size */
	fprintf(outfp,"%cLLA>",ESC);	/* Set Dialog to 30 Lines */
	fprintf(outfp,"%cKI0",ESC);	/* Process Delete Characters */
	fprintf(outfp,"%cIJ5:",ESC);	/* Change GIN cursor speed to 5,10 */ 
	fprintf(outfp,"%cTCK4C2F4",ESC);/* Change GIN cursor to yellow */
	fprintf(outfp,"%cML2",ESC);	/* set graphics line index -> 2 (red) 
					   helps reduce alpha/graphics clutter 
					*/
	return(0);			/* OK */
}

/*
 *  			T E K _ C L O S E
 *  
 *  Gracefully release the display.
 */
void
Tek4109_close()
{
	cancel_cursor();
	fprintf(outfp,"%cLZ", ESC);	/* clear screen */
	fprintf(outfp,"%cLLA8",ESC);	/* NRTC - Set Dialog to 24 Lines */
	fprintf(outfp,"%c%%!1",ESC);	/* NRTC - Place in ANSI mode */
	(void)putc(US,outfp);
	(void)fflush(outfp);
	fclose(outfp);
}

/*
 *			T E K _ R E S T A R T
 *
 * Used when the display processor wanders off.
 */
void
Tek4109_restart()
{
	(void)printf("%cTek_restart\n",US);		/* NRTC */
}

/*
 *			T E K _ P R O L O G
 *
 * There are global variables which are parameters to this routine.
 */
void
Tek4109_prolog()
{
	if( !dmaflag )
		return;

	/* If something significant has happened, clear screen and redraw */

	(void)putc(ESC,outfp);	/* Miniature typeface */
	(void)putc(';',outfp);

	(void)fprintf(outfp, "%cLZ", ESC);	/* clear dialog area */
	(void)putc(ESC,outfp);			/* clear screen area */
	(void)putc(FF,outfp);
	(void)fflush(outfp);
	cancel_cursor();
	ohix = ohiy = oloy = oextra = -1;

	/* Put the center point up */
	point( 0, 0 );
}

/*
 *			T E K _ E P I L O G
 */
void
Tek4109_epilog()
{
	if( !dmaflag )
		return;
        tekmove( TITLE_XBASE, SOLID_YBASE );
	(void)putc(US,outfp);
}

/*
 *  			T E K _ N E W R O T
 *  Stub.
 */
/* ARGSUSED */
void
Tek4109_newrot(mat)
mat_t mat;
{
	return;
}

/*
 *  			T E K _ O B J E C T
 *  
 *  Set up for an object, transformed as indicated, and with an
 *  object center as specified.  The ratio of object to screen size
 *  is passed in as a convienience.
 *
 *  Returns 0 if object could be drawn, !0 if object was omitted.
 */
/* ARGSUSED */
int
Tek4109_object( sp, mat, ratio, white )
register struct solid *sp;
mat_t mat;
double ratio;
{
	static vect_t last;
	register struct veclist *vp;
	int nvec;
	int useful = 0;

	if(  sp->s_soldash )
		fprintf(outfp,"%cMV2",ESC);		/* Dot Dash    NRTC */
	else	
		fprintf(outfp,"%cMV0",ESC);		/* Solid Line  NRTC */

	nvec = sp->s_vlen;
	for( vp = sp->s_vlist; nvec-- > 0; vp++ )  {
		/* Viewing region is from -1.0 to +1.0 */
		if( vp->vl_pen == PEN_UP )  {
			/* Move, not draw */
			MAT4X3PNT( last, mat, vp->vl_pnt );
		}  else  {
			static vect_t fin;
			static vect_t start;
			/* draw */
			MAT4X3PNT( fin, mat, vp->vl_pnt );
			VMOVE( start, last );
			VMOVE( last, fin );
			if(
				/* sqrt(1+1) */
				(ratio >= 0.7071)  &&
				vclip( start, fin, clipmin, clipmax ) == 0
			)  continue;

			tekmove(	(int)( start[0] * 2047 ),
				(int)( start[1] * 2047 ) );
			tekcont(	(int)( fin[0] * 2047 ),
				(int)( fin[1] * 2047 ) );
			useful = 1;
		}
	}
	return(useful);
}

/*
 *			T E K _ N O R M A L
 *
 * Restore the display processor to a normal mode of operation
 * (ie, not scaled, rotated, displaced, etc).
 * Turns off windowing.
 */
void
Tek4109_normal()
{
	return;
}

/*
 *			T E K _ U P D A T E
 *
 * Transmit accumulated displaylist to the display processor.
 */
void
Tek4109_update()
{
	if( second_fd )  {
		/* put up graphics cursor */
		(void)putc(ESC,outfp);
		(void)putc(SUB,outfp);
	} else
		(void)putc(US,outfp);		/* Alpha mode */
	(void)fflush(outfp);
}

/*
 *			T E K _ P U T S
 *
 * Output a string into the displaylist.
 * The starting position of the beam is as specified.
 */
/* ARGSUSED */
void
Tek4109_puts( str, x, y, size, color )
register u_char *str;
{
	tekmove(x,y);
	label(str);
}

/*
 *			T E K _ 2 D _ G O T O
 *
 */
void
Tek4109_2d_line( x1, y1, x2, y2, dashed )
int x1, y1;
int x2, y2;
int dashed;
{
	if( dashed )
		linemod("dotdashed");
	else
		linemod("solid");
	tekmove(x1,y1);
	tekcont(x2,y2);
}

/*
 *			G E T _ C U R S O R
 *  
 *  Read the Tektronix cursor.  The Tektronix sends
 *  6 bytes:  The key the user struck, 4 bytes of
 *  encoded position, and a return (newline).
 *  Note this is is complicated if the user types
 *  a return or linefeed.
 *  (The terminal is assumed to be in cooked mode)
 */
static get_cursor()
{
	register char *cp;
	char ibuf[64];
	register int i;
	int hix, hiy, lox, loy;

	/* ASSUMPTION:  Input is line buffered (tty cooked) */
	i = read( second_fd, ibuf, sizeof(ibuf) );
	/* The LAST 6 chars are the string from the tektronix */
	if( i < 6 )  {
		(void)printf("short read of %d\n", i);
		return;		/* Fails if he hits RETURN */
	}
	cp = &ibuf[i-6];
	if( cp[5] != '\n' )  {
		(void)printf("cursor synch?\n");
		(void)printf("saw:%c%c%c%c%c%c\n",
			cp[0], cp[1], cp[2], cp[3], cp[4], cp[5] );
		return;
	}

	/* cp[0] is what user typed, followed by 4pos + NL */
	hix = ((int)cp[1]&037)<<7;
	lox = ((int)cp[2]&037)<<2;
	hiy = ((int)cp[3]&037)<<7;
	loy = ((int)cp[4]&037)<<2;

	/* Tek positioning is 0..4096,
	 * The desired range is -2048 <= x,y <= +2048.
	 */
	dm_values.dv_xpen = TEK_TO_GED(hix|lox);
	dm_values.dv_ypen = TEK_TO_GED(hiy|loy);
	if( dm_values.dv_xpen < -2048 || dm_values.dv_xpen > 2048 )
		dm_values.dv_xpen = 0;
	if( dm_values.dv_ypen < -2048 || dm_values.dv_ypen > 2048 )
		dm_values.dv_ypen = 0;

	switch(cp[0])  {
	case 'Z':
		(void)printf("x=%d,y=%d\n", dm_values.dv_xpen, dm_values.dv_ypen);
		break;		/* NOP */
	case 'b':
		dm_values.dv_penpress = DV_INZOOM;
		break;
	case 's':
		dm_values.dv_penpress = DV_OUTZOOM;
		break;
	case '.':
		dm_values.dv_penpress = DV_SLEW;
		break;
	default:
		(void)printf("s=smaller, b=bigger, .=slew, space=pick/slew\n");
		return;
	case ' ':
		dm_values.dv_penpress = DV_PICK;
		break;
	}
}

/*
 *			T E K _ I N P U T
 *
 * Execution must suspend in this routine until a significant event
 * has occured on either the command stream, or a device event has
 * occured, unless "noblock" is set.
 *
 * The GED "generic input" structure is filled in.
 *
 * Returns:
 *	0 if no command waiting to be read,
 *	1 if command is waiting to be read.
 */
Tek4109_input( cmd_fd, noblock )
{
	static long readfds;
	static struct timeval timeout;

	/*
	 * Check for input on the keyboard or on the polled registers.
	 *
	 * Suspend execution until either
	 *  1)  User types a full line
	 *  2)  A change in peripheral status occurs
	 *  3)  The timelimit on SELECT has expired
	 *
	 * If a RATE operation is in progress (zoom, rotate, slew)
	 * in which the peripherals (rate setting) may not be changed,
	 * but we still have to update the display,
	 * do not suspend execution.
	 */
	if( noblock )
		timeout.tv_sec = 0;
	else
		timeout.tv_sec = 30*60;		/* 30 MINUTES for Tek */
	timeout.tv_usec = 0;

	readfds = (1<<cmd_fd);
	if( second_fd )
		readfds |= (1<<second_fd);
	(void)select( 32, &readfds, 0L, 0L, &timeout );

	dm_values.dv_penpress = 0;
	if( second_fd && readfds & (1<<second_fd) )
		get_cursor();

	if( readfds & (1<<cmd_fd) )
		return(1);		/* command awaits */
	else
		return(0);		/* just peripheral stuff */
}

/* 
 *			T E K _ L I G H T
 */
/* ARGSUSED */
void
Tek4109_light( cmd, func )
int cmd;
int func;			/* BE_ or BV_ function */
{
	return;
}

/* ARGSUSED */
unsigned
Tek4109_cvtvecs( sp )
struct solid *sp;
{
	return( 0 );
}

/*
 * Loads displaylist
 */
unsigned
Tek4109_load( addr, count )
unsigned addr, count;
{
	(void)printf("%cTek_load(x%x, %d.)\n",US, addr, count );
	return( 0 );
}

void
Tek4109_statechange()
{
}

void
Tek4109_viewchange()
{
}

void
Tek4109_colorchange()
{
}

/*
 * Perform the interface functions
 * for the Tektronix 4014-1 or 4109 with Extended Graphics Option.
 * The Extended Graphics Option makes available a field of
 * 10 inches vertical, and 14 inches horizontal, with a resolution
 * of 287 points per inch.
 *
 * The Tektronix is Quadrant I, 4096x4096 (not all visible).
 */

/* The input we see is -2048..+2047 */
/* Continue motion from last position */
static void
tekcont(x,y)
register int x,y;
{
	int hix,hiy,lox,loy,extra;
	int n;

	x = GED_TO_TEK(x);
	y = GED_TO_TEK(y);

	hix=(x>>7) & 037;
	hiy=(y>>7) & 037;
	lox = (x>>2)&037;
	loy=(y>>2)&037;
	extra=x&03+(y<<2)&014;
	n = (abs(hix-ohix) + abs(hiy-ohiy) + 6) / 12;
	if(hiy != ohiy){
		(void)putc(hiy|040,outfp);
		ohiy=hiy;
	}
	if(hix != ohix) {
		if(extra != oextra) {
			(void)putc(extra|0140,outfp);
			oextra=extra;
		}
		(void)putc(loy|0140,outfp);
		(void)putc(hix|040,outfp);
		ohix=hix;
		oloy=loy;
	} else {
		if(extra != oextra) {
			(void)putc(extra|0140,outfp);
			(void)putc(loy|0140,outfp);
			oextra=extra;
			oloy=loy;
		} else if(loy != oloy) {
			(void)putc(loy|0140,outfp);
			oloy=loy;
		}
	}
	(void)putc(lox|0100,outfp);
/*	while(n--)
		(void)putc(0,outfp);  */
}

static void
tekmove(xi,yi)
{
/*	fprintf(outfp,"%cTekmove: x=%d, y=%d \n",US,xi+2048,yi+2048);   */
								/* NRTC */
	(void)putc(GS,outfp);			/* Next vector blank */
	tekcont(xi,yi);
}

static cancel_cursor()
{
	extern unsigned sleep();
	(void)fprintf(outfp, "%cKC", ESC);	/* Cancel crosshairs */
	(void)fflush(outfp);
	sleep(2);	/* Have to wait for terminal reset */
}

static label(s)
register char *s;

{
int	length;				/* NRTC */
char	hi, low;				/* NRTC */
	length= strlen(s);			/* NRTC */
	if ( length <= 0 ) {			/* NRTC */
	   (void)putc(US,outfp);			/* NRTC */
	   ohix = ohiy = oloy = oextra = -1;		/* NRTC */
	   return;				/* NRTC */
	   }					/* NRTC */
	hi = (length>>4) + 64;			/* NRTC */
	low= (length & 15) + 48;		/* NRTC */
	(void)fprintf(outfp,"%cLT%c%c%s",ESC,hi,low,s); /* NRTC */
	(void)putc(US,outfp);			/* NRTC */
	ohix = ohiy = oloy = oextra = -1;
}

/* Line Mode Command - Select Tektronics Preset Line Display 4014 */

static linemod(s)
register char *s;
{
	char  c;

	switch(s[0]){
	case 'l':	
		c = '7';                         /* Long Dashed Line   NRTC */
		break;
	case 'd':	
		if(s[3] != 'd')c='1';		/* Dot Line   NRTC */
		else c='2';			/* Dot-Dashed Line  NRTC  */
		break;
	case 's':
		if(s[5] != '\0')c='3';		/* Short Dash Line  NRTC  */
		else c='0';			/* Solid Line       NRTC  */
		break;
	default:			/* DAG -- added support for colors */
		c = '0';			/* Solid Line        NRTC */
		break;
	}
	fprintf(outfp,"%cMV%c",ESC,c);		/* Set Line Mode  NRTC */
 
}

static point(xi,yi){
        tekmove(xi,yi);
	tekcont(xi,yi);
}

/* ARGSUSED */
void
Tek4109_debug(lvl)
{
}

void
Tek4109_window(w)
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
