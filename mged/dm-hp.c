/*
 *			D M - H P . C
 *
 *  Based on dm-tek4109.c
 *
 *  Author -
 *	Mark H. Bowden
 *	Research Institute, RI E47
 *	University of Alabama in Huntsville
 *	Huntsville, AL  35899
 *	(205) 876-1089 Redstone Arsenal
 *	(205) 895-6467 Research Institute
 */

#include "conf.h"

#include <stdio.h>
#include "machine.h"
#include "vmath.h"
#include "mater.h"
#include "./ged.h"
#include "./dm.h"
#include "externs.h"
#include "./solid.h"

typedef unsigned char u_char;

/* Display Manager package interface */
 
#define HPBOUND	1000.0	/* Max magnification in Rot matrix */

int	HP_open();
void	HP_close();
int	HP_input();
void	HP_prolog(), HP_epilog();
void	HP_normal(), HP_newrot();
void	HP_update();
void	HP_puts(), HP_2d_line();
void	HP_light();
int	HP_object();
unsigned HP_cvtvecs(), HP_load();
void	HP_statechange(), HP_viewchange(), HP_colorchange();
void	HP_window(), HP_debug();

struct dm dm_Hp = {
	HP_open, HP_close,
	HP_input,
	HP_prolog, HP_epilog,
	HP_normal, HP_newrot,
	HP_update,
	HP_puts, HP_2d_line,
	HP_light,
	HP_object,
	HP_cvtvecs, HP_load,
	HP_statechange,
	HP_viewchange,
	HP_colorchange,
	HP_window, HP_debug,
	0,				/* no displaylist */
	0,				/* No frame buffer */
	HPBOUND,
	"HP", "Hewlett Packard 2397a"
};

extern struct device_values dm_values;	/* values read from devices */

static vect_t clipmin, clipmax;		/* for vector clipping */
static int oloy = -1;
static int ohiy = -1;
static int ohix = -1;
static int oextra = -1;
static int curx, cury;

#define HP2397A		0
#define HP2627A		1
static char termtype;

#define ESC	'\033'          /* Escape */

static void HPmove(), HPcont();

/*
 * Display coordinate conversion
 */
 
#define XGED_TO_HP(x)   (((x)+2048) * 492 / 4096)
#define XHP_TO_GED(x)   (((x) * 4096 / 492) - 2048)
#define YGED_TO_HP(y)   (((y)+2048) * 25 / 256)
#define YHP_TO_GED(y)   (((y) * 256 / 25) - 2048)

/*
 * HP_open - Fire up the display manager, and the display processor.
 */

HP_open()
{
	char s[16];

	(void)printf("\033*j1A");	/* set tablet on-line */
	(void)printf("\033*s1^");	/* request terminal name */
	(void)fgets(s,16,stdin);	/* read name */
	if (!strcmp(s,"2627A\n")) {
	    termtype = HP2627A;
	    (void)printf("\033*j1C");	/* asynchrous tablet mode */
	} else {
	    termtype = HP2397A;
	    (void)printf("\033*j9F");	/* penpress reports F9 pressed */
	}
	(void)printf("Terminal type: %s\n",s);
	(void)printf("%c*da",ESC);	/* clear graphics memory */
	(void)printf("%c*dc",ESC);	/* graphics display on */
	(void)printf("%c*dk",ESC);	/* graphics cursor on */
	(void)printf("%c*e0b",ESC);	/* background color */
	(void)printf("%c*m6x",ESC);	/* line color */
	(void)printf("%c*n3x",ESC);	/* text color */
	return(0);		/* OK */
}

/*
 *  HP_close - Gracefully release the display.
 */

void
HP_close()
{
	(void)printf("%cH", ESC);	/* cursor home */
	(void)printf("%cJ", ESC);	/* clear screen */
	(void)printf("%c&w6S",ESC);	/* Set Dialog to 12 Lines */
	(void)printf("%c*dD",ESC);	/* graphics off */
	(void)printf("%c*dT",ESC);	/* graph text off */
	(void)printf("%c*dE",ESC);	/* alpha on*/
	(void)fflush(stdout);
}

/*
 * HP_prolog - If something significant has happened, clear screen and redraw
 */

void
HP_prolog()
{
    if (dmaflag) {
	(void)printf("%c*da",ESC);	/* clear graphics memory */
	(void)fflush(stdout);
	point( 0, 0 );			/* Put up the center point */
	(void)printf("\033c");		/* disable keyboard */
    }
}

/*
 * HP_epilog - done drawing
 */

void
HP_epilog()
{
	HPmove(XHP_TO_GED(curx),YHP_TO_GED(cury));
	(void)printf("\033b");			/* enable keyboard */
}

/*
 * HP_object
 *  
 *  Set up for an object, transformed as indicated, and with an
 *  object center as specified.  The ratio of object to screen size
 *  is passed in as a convienience.
 *
 *  Returns 0 if object could be drawn, !0 if object was omitted.
 */

/* ARGSUSED */
int
HP_object( sp, mat, ratio, white )
register struct solid *sp;
mat_t mat;
double ratio;
{
	static vect_t last;
	register struct vlist *vp;
	int color;
	int useful = 0;

	if(  sp->s_soldash )
		printf("%c*m4b",ESC);	/* Dot Dash */
	else	
		printf("%c*m1b",ESC);	/* Solid Line */

	color = sp->s_dmindex;
	printf("%c*m%1dx",ESC,color);
	
	for( vp = sp->s_vlist; vp != VL_NULL; vp = vp->vl_forw )  {
		/* Viewing region is from -1.0 to +1.0 */
		if( vp->vl_draw == 0 )  {
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

			HPmove(	(int)( start[0] * 2047 ),
				(int)( start[1] * 2047 ) );
			HPcont(	(int)( fin[0] * 2047 ),
				(int)( fin[1] * 2047 ) );
			useful = 1;
		}
	}
	(void)printf("%c*m6x",ESC);
	return(useful);
}

/*
 * HP_puts - Output a string into the displaylist.
 */

void
HP_puts( str, x, y, size, color )
register u_char *str;
{
	HPmove(x,y - 29);
	(void)printf("\033*l%s\n",str);
}

/*
 * HP_2d_line
 */

void
HP_2d_line( x1, y1, x2, y2, dashed )
int x1, y1;
int x2, y2;
int dashed;
{
	if( dashed )
		linemod("dotdashed");
	else
		linemod("solid");
	HPmove(x1,y1);
	HPcont(x2,y2);
}

/*
 *			H P _ I N P U T
 *
 * The GED "generic input" structure is filled in.
 *
 * Read first character. If it is a penpress, get cursor position and return
 * indicating no command awaits. Otherwise, put character back on stdin and
 * return indicating that a command does await.
 *
 */

HP_input( cmd_fd, noblock )
{
	int ch;

	if ((ch = getchar()) == '\033') {	/* hp2397a penpress */
	    switch ((ch = getchar())) {		/* what kind of penpress ? */
		case 'q':
		    dm_values.dv_penpress = DV_SLEW;
		    break;
		case 'r':
		    dm_values.dv_penpress = DV_INZOOM;
		    break;
		case 's':
		    dm_values.dv_penpress = DV_OUTZOOM;
		    break;
		default:
		    dm_values.dv_penpress = DV_PICK;
	    }
	    fflush(stdin);
	    printf("\033*s3^");		/* ask terminal for cursor position */
	    scanf("%d,%d",&curx,&cury);
	    dm_values.dv_xpen     = XHP_TO_GED(curx);
	    dm_values.dv_ypen     = YHP_TO_GED(cury);
	    return(0);
	} else if (ch == '+') {		/* hp2627a penpress */
	    scanf("%d,%d",&curx,&cury);
	    dm_values.dv_xpen     = XHP_TO_GED(curx);
	    dm_values.dv_ypen     = YHP_TO_GED(cury);
	    dm_values.dv_penpress = DV_PICK;
	    return(0);
	} else {			/* Not a penpress so */
	    ungetc(ch,stdin);		/* put character back on stdin. */
	    dm_values.dv_penpress = 0;
	    return(1);
	}
/* NOTREACHED */
}

/*
 *  			H P _ C O L O R C H A N G E
 *  
 *  Go through the mater table and assign colors.
 *
 */
void
HP_colorchange()
{
	register struct mater *mp;

	for( mp = MaterHead; mp != MATER_NULL; mp = mp->mt_forw )
		HP_colorit( mp );

	color_soltab();		/* apply colors to the solid table */
}


int
HP_colorit( mp )
register struct mater *mp;
{
	static int i;

	i = (i % 7) + 1;
	mp->mt_dm_int = i;
}

/* Continue motion from last position */
static void
HPcont(x,y)
register int x,y;
{
	int ix,iy;

	ix = XGED_TO_HP(x);
	iy = YGED_TO_HP(y);
	(void)printf("%c*d%d,%do",ESC,ix,iy);    /* move cursr */
	(void)printf("%c*pc",ESC);               /* new point */
}

static void
HPmove(xi,yi)
{
	printf("%c*pa",ESC);	/* pen up */
	HPcont(xi,yi);
	printf("%c*pb",ESC);	/* pen down */
}

static linemod(s)
register char *s;
{
	char  c;

	switch(s[0]){
	case 'l':	
		c = '5';                         /* Long Dashed Line */
		break;
	case 'd':	
		if(s[3] != 'd')c='7';		/* Dot Line   NRTC */
		else c='4';			/* Dot-Dashed Line */
		break;
	case 's':
		if(s[5] != '\0')c='9';		/* Short Dash Line */
		else c='1';			/* Solid Line */
		break;
	default:		/* DAG -- added support for colors */
		c = '1';			/* Solid Line */
		break;
	}
	printf("%c*m%cb",ESC,c);		/* Set Line Mode */
}

static point(xi,yi){
        HPmove(xi,yi);
	HPcont(xi,yi);
}

void
HP_window(w)
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

/*
 * Stubs
 */

unsigned
HP_cvtvecs( sp )
struct solid *sp;
{
}

unsigned
HP_load( addr, count )
unsigned addr, count;
{
}

void
HP_statechange()
{
}

void
HP_viewchange()
{
}

void
HP_restart()
{
}

void
HP_newrot(mat)
mat_t mat;
{
}

void
HP_light( cmd, func )
int cmd;
int func;
{
}

void
HP_normal()
{
}

void
HP_update()
{
}

void
HP_debug(lvl)
{
}
