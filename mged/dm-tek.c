/*
 *			D M - T E K . C
 *
 * An unsatisfying (but useful) hack to allow GED to display
 * it's images on Tektronix 4014 compatible displays.
 * Mostly, a precursor for BLIT and RasterTek code.
 *
 * Mike Muuss, BRL, 16-Jan-85.
 *  
 * Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include "ged_types.h"
#include "ged.h"
#include "dm.h"
#include "vmath.h"
#include "solid.h"

typedef unsigned char u_char;

/* Display Manager package interface */

#define TEKBOUND	1000.0	/* Max magnification in Rot matrix */
void	Tek_open(), Tek_close(), Tek_restart();
int	Tek_input();
void	Tek_prolog(), Tek_epilog();
void	Tek_normal(), Tek_newrot();
void	Tek_update();
void	Tek_puts(), Tek_2d_line(), Tek_light();
int	Tek_object();
unsigned Tek_cvtvecs(), Tek_load();

struct dm dm_Tek = {
	Tek_open, Tek_close, Tek_restart,
	Tek_input,
	Tek_prolog, Tek_epilog,
	Tek_normal, Tek_newrot,
	Tek_update,
	Tek_puts, Tek_2d_line,
	Tek_light,
	Tek_object,
	Tek_cvtvecs, Tek_load,
	TEKBOUND,
	"tek", "Tektronix 4014"
};

struct timeval	{			/* needed for _select() */
	long	tv_sec;			/* seconds */
	long	tv_usec;		/* microseconds */
};


extern struct device_values dm_values;	/* values read from devices */

/**** Begin global display information, used by dm.c *******/
extern int	inten_offset;		/* Intensity offset */
extern int	inten_scale;		/* Intensity scale */
extern int	xcross;
extern int	ycross;			/* tracking cross position */
extern mat_t	rot;			/* viewing rotation */
extern int	windowbounds[6];	/* X hi,lo;  Y hi,lo;  Z hi,lo */
/**** End global display information ******/

/**** Global mode information ****/
extern int	regdebug;		/* toggled by "X" command */
extern int	adcflag;		/* A/D cursor on/off */
/**** End Global mode information ****/

static vect_t clipmin, clipmax;		/* for vector clipping */

#define BELL	007
#define	FF	014
#define SUB	032		/* Turn on graphics cursor */
#define GS	035		/* Enter Graphics Mode (1st vec dark) */
#define ESC	033
#define US	037		/* Enter Alpha Mode */

static int second_fd;		/* fd of Tektronix if not /dev/tty */
static FILE *outfp;		/* Tektronix device to output on */
static char ttybuf[BUFSIZ];

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
void
Tek_open()
{
	char line[64], line2[64];

	printf("Output tty [stdout]? ");
	(void)gets( line );		/* Null terminated */
	if( feof(stdin) )  quit();
	if( line[0] != '\0' )  {
		if( (outfp = fopen(line,"r+w")) == NULL )  {
			sprintf( line2, "/dev/tty%s%c", line, '\0' );
			if( (outfp = fopen(line2,"r+w")) == NULL )  {
				perror(line);
				outfp = fopen("/dev/null","r+w");
			}
		}
		second_fd = fileno(outfp);
	} else {
		outfp = fopen("/dev/tty","r+w");
		second_fd = 0;		/* no second filedes */
	}
	setbuf( outfp, ttybuf );
}

/*
 *  			T E K _ C L O S E
 *  
 *  Gracefully release the display.
 */
void
Tek_close()
{
	putc(US,outfp);
	fflush(outfp);
	fclose(outfp);
}

/*
 *			T E K _ R E S T A R T
 *
 * Used when the display processor wanders off.
 */
void
Tek_restart()
{
	printf("Tek_restart\n");
}

/*
 *			T E K _ P R O L O G
 *
 * There are global variables which are parameters to this routine.
 */
void
Tek_prolog()
{
	register int i;

	if( !dmaflag )
		return;

	/* If something significant has happened, clear screen and redraw */
	erase();
	/* Miniature typeface */
	putc(ESC,outfp);
	putc(';',outfp);

	/* Put the center point up */
	point( 0, 0 );

#ifdef never
	/* Draw the tracking cross */
#define CLIP(v)	if(v < -2048) v = -2048; else if(v > 2047) v = 2047;
	i = xcross-75;  CLIP(i);
	move( i, ycross );
	i = xcross+75;  CLIP(i);
	cont(i, ycross );

	i = ycross-75;  CLIP(i);
	move( xcross, i );
	i = ycross+75;  CLIP(i);
	cont( xcross, i );
#undef CLIP
#endif
	/* Compute the clipping bounds */
	clipmin[0] = windowbounds[1] / 2048.;
	clipmin[1] = windowbounds[3] / 2048.;
	clipmin[2] = windowbounds[5] / 2048.;
	clipmax[0] = windowbounds[0] / 2047.;
	clipmax[1] = windowbounds[2] / 2047.;
	clipmax[2] = windowbounds[4] / 2047.;
}

/*
 *			T E K _ E P I L O G
 */
void
Tek_epilog()
{
	if( !dmaflag )
		return;
	move( TITLE_XBASE, SOLID_YBASE );
	putc(US,outfp);
}

/*
 *  			T E K _ N E W R O T
 *  Stub.
 */
void
Tek_newrot(mat)
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
int
Tek_object( sp, mat, ratio, white )
register struct solid *sp;
mat_t mat;
double ratio;
{
	static vect_t last;
	register struct veclist *vp;
	int nvec;
	int useful = 0;

	if(  sp->s_soldash )
		putc('b',outfp);	/* Dot dash */
	else
		putc('`',outfp);	/* Solid */

	nvec = sp->s_vlen;
	for( vp = sp->s_vlist; nvec-- > 0; vp++ )  {
		/* Viewing region is from -1.0 to +1.0 */
		if( vp->vl_pen == PEN_UP )  {
			/* Move, not draw */
			MAT4X3PNT( last, mat, vp->vl_pnt );
		}  else  {
			static vect_t finish;
			static vect_t start;
			/* draw */
			MAT4X3PNT( finish, mat, vp->vl_pnt );
			VMOVE( start, last );
			VMOVE( last, finish );
			if(
#ifdef later
				/* sqrt(1+1) */
				(ratio >= 0.7071)  &&
#endif
				vclip( start, finish, clipmin, clipmax ) == 0
			)  continue;

			move(	(int)( start[0] * 2047 ),
				(int)( start[1] * 2047 ) );
			cont(	(int)( finish[0] * 2047 ),
				(int)( finish[1] * 2047 ) );
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
Tek_normal()
{
	return;
}

/*
 *			T E K _ U P D A T E
 *
 * Transmit accumulated displaylist to the display processor.
 */
void
Tek_update()
{
	if( second_fd )  {
		/* put up graphics cursor */
		putc(ESC,outfp);
		putc(SUB,outfp);
	} else
		putc(US,outfp);		/* Alpha mode */
	fflush(outfp);
}

/*
 *			T E K _ P U T S
 *
 * Output a string into the displaylist.
 * The starting position of the beam is as specified.
 */
void
Tek_puts( str, x, y, size, color )
register u_char *str;
{
	move(x,y);
	label(str);
}

/*
 *			T E K _ 2 D _ G O T O
 *
 */
void
Tek_2d_line( x1, y1, x2, y2, dashed )
int x1, y1;
int x2, y2;
int dashed;
{
	if( dashed )
		linemod("dotdashed");
	else
		linemod("solid");
	move(x1,y1);
	cont(x2,y2);
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
		printf("short read of %d\n", i);
		return;		/* Fails if he hits RETURN */
	}
	cp = &ibuf[i-6];
	if( cp[5] != '\n' )  {
		printf("cursor synch?\n");
		printf("saw:%c%c%c%c%c%c\n",
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
		printf("x=%d,y=%d\n", dm_values.dv_xpen, dm_values.dv_ypen);
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
		printf("s=smaller, b=bigger, .=slew, space=pick/slew\n");
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
Tek_input( cmd_fd, noblock )
{
	static long readfds;
	static struct timeval timeout;
	register int i, j;

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
	(void)_select( 32, &readfds, 0L, 0L, &timeout );

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
void
Tek_light( cmd, func )
int cmd;
int func;			/* BE_ or BV_ function */
{
	return;
}

unsigned
Tek_cvtvecs( sp )
register struct solid *sp;
{
	return( 0 );
}

/*
 * Loads displaylist
 */
unsigned
Tek_load( addr, count )
unsigned addr, count;
{
	return( count );
}
/*
 *	This program performs the interpretation function
 * for the Tektronix 4014-1 with Extended Graphics Option.
 * The device independant requests which the TIG-PACK routines
 * output are mapped by this program into 4014 commands.
 *
 * The Extended Graphics Option makes available a field of
 * 10 inches vertical, and 14 inches horizontal, with a resolution
 * of 287 points per inch.
 *
 * The Tektronix is Quadrant I, 4096x4096 (not all visible).
 */
/*** pilfered, for now *** */
/*	@(#)subr.c	1.1	*/
float obotx = 0.;
float oboty = 0.;
float botx = 0.;
float boty = 0.;
float scalex = 1.;
float scaley = 1.;
int scaleflag;

int oloy = -1;
int ohiy = -1;
int ohix = -1;
int oextra = -1;

/* The input we see is -2048..+2047 */
cont(x,y)
register int x,y;
{
	int hix,hiy,lox,loy,extra;
	int n;

	x = GED_TO_TEK(x);
	y = GED_TO_TEK(y);

#ifdef never
	x = (x-obotx)*scalex + botx;
	y = (y-oboty)*scaley + boty;
#endif
	hix=(x>>7) & 037;
	hiy=(y>>7) & 037;
	lox = (x>>2)&037;
	loy=(y>>2)&037;
	extra=x&03+(y<<2)&014;
	n = (abs(hix-ohix) + abs(hiy-ohiy) + 6) / 12;
	if(hiy != ohiy){
		putc(hiy|040,outfp);
		ohiy=hiy;
	}
	if(hix != ohix){
		if(extra != oextra){
			putc(extra|0140,outfp);
			oextra=extra;
		}
		putc(loy|0140,outfp);
		putc(hix|040,outfp);
		ohix=hix;
		oloy=loy;
	}
	else{
		if(extra != oextra){
			putc(extra|0140,outfp);
			putc(loy|0140,outfp);
			oextra=extra;
			oloy=loy;
		}
		else if(loy != oloy){
			putc(loy|0140,outfp);
			oloy=loy;
		}
	}
	putc(lox|0100,outfp);
	while(n--)
		putc(0,outfp);
}

move(xi,yi){

	putc(GS,outfp);			/* Next vector blank */
	cont(xi,yi);
}

erase(){
	extern unsigned sleep();	/* DAG -- was missing */
	putc(ESC,outfp);
	putc(FF,outfp);
	ohix = ohiy = oloy = oextra = -1;
	fflush(outfp);

	/* If 2 FD's, it's probably a BLIT, otherwise assume real Tek */
	if( !second_fd )
		sleep(2);
}

label(s)
register char *s;
{
	putc(US,outfp);
	for( ; *s; s++ )
		putc(*s,outfp);
	ohix = ohiy = oloy = oextra = -1;
}

linemod(s)
char *s;
{
	int c;				/* DAG -- was char */
	putc(ESC,outfp);
	switch(s[0]){
	case 'l':	
		c = 'd';
		break;
	case 'd':	
		if(s[3] != 'd')c='a';
		else c='b';
		break;
	case 's':
		if(s[5] != '\0')c='c';
		else c='`';
		break;
	default:			/* DAG -- added support for colors */
		c = '`';
		break;
	}
	putc(c,outfp);
}

point(xi,yi){
	move(xi,yi);
	cont(xi+1,yi+1);
}
