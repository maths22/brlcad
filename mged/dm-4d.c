/*
 *			D M - 4 D . C
 *
 *  This version for the SGI 4-D Iris, both regular and GT versions.
 *
 *  Uses library -lgl
 *
 *  Authors -
 *	Paul R. Stay
 *	Michael John Muuss
 *	Robert J. Reschly
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
#include <math.h>
#include <termio.h>
#undef VMIN		/* is used in vmath.h, too */
#include <ctype.h>

#include "machine.h"
#include "vmath.h"
#include "raytrace.h"
#include "./ged.h"
#include "./dm.h"
#include "externs.h"
#include "./solid.h"

#include <gl/gl.h>		/* SGI IRIS library */
#include <gl/device.h>		/* SGI IRIS library */
#include <gl/get.h>		/* SGI IRIS library */
#include <sys/types.h>
#include <sys/invent.h>

extern inventory_t	*getinvent();

/* Display Manager package interface */

#define IRBOUND	4095.9	/* Max magnification in Rot matrix */

int	Ir_open();
void	Ir_close();
int	Ir_input();
void	Ir_prolog(), Ir_epilog();
void	Ir_normal(), Ir_newrot();
void	Ir_update();
void	Ir_puts(), Ir_2d_line(), Ir_light();
int	Ir_object();
unsigned Ir_cvtvecs(), Ir_load();
void	Ir_statechange(), Ir_viewchange(), Ir_colorchange();
void	Ir_window(), Ir_debug();


static char ir_title[] = "BRL MGED";
static int cueing_on = 1;	/* Depth cueing flag - for colormap work */
static int zclipping_on = 1;	/* Z Clipping flag */
static int zbuffer_on = 1;	/* Hardware Z buffer is on */
static int perspective_mode = 0;	/* Perspective flag */
static int perspective_angle =3;	/* Angle of perspective */
static int lighting_on = 0;	/* Lighting model on */
static int no_faceplate = 0;	/* Don't draw faceplate */
static int ovec = -1;		/* Old color map entry number */
static mat_t perspect_mat;
static mat_t nozclip_mat;
static int kblights();
static int persp_mat();

#ifdef IR_BUTTONS
/*
 * Map SGI Button numbers to MGED button functions.
 * The layout of this table is suggestive of the actual button box layout.
 */
#define SW_HELP_KEY	SW0
#define SW_ZERO_KEY	SW3
#define HELP_KEY	0
#define ZERO_KNOBS	0
static unsigned char bmap[IR_BUTTONS] = {
             HELP_KEY,    BV_ADCURSOR, BV_RESET,    ZERO_KNOBS,
BE_O_SCALE,  BE_O_XSCALE, BE_O_YSCALE, BE_O_ZSCALE, 0,           BV_VSAVE,
BE_O_X,      BE_O_Y,      BE_O_XY,     BE_O_ROTATE, 0,           BV_VRESTORE,
BE_S_TRANS,  BE_S_ROTATE, BE_S_SCALE,  BE_MENU,     BE_O_ILLUMINATE, BE_S_ILLUMINATE,
BE_REJECT,   BV_BOTTOM,   BV_TOP,      BV_REAR,     BV_45_45,    BE_ACCEPT,
             BV_RIGHT,    BV_FRONT,    BV_LEFT,     BV_35_25
};
/* Inverse map for mapping MGED button functions to SGI button numbers */
static unsigned char invbmap[BV_MAXFUNC+1];

/* bit 0 == switchlight 0 */
static unsigned long lights;
#endif

#ifdef IR_KNOBS
static int irlimit();			/* provides knob dead spot */
#define NOISE 32		/* Size of dead spot on knob */
/*
 *  Labels for knobs in help mode.
 */
char	*kn1_knobs[] = {
	/* 0 */ "adc <1",	/* 1 */ "zoom", 
	/* 2 */ "adc <2",	/* 3 */ "adc dist",
	/* 4 */ "adc y",	/* 5 */ "y slew",
	/* 6 */ "adc x",	/* 7 */	"x slew"
};
char	*kn2_knobs[] = {
	/* 0 */ "unused",	/* 1 */	"zoom",
	/* 2 */ "z rot",	/* 3 */ "z slew",
	/* 4 */ "y rot",	/* 5 */ "y slew",
	/* 6 */ "x rot",	/* 7 */	"x slew"
};
#endif

/*
 * SGI Color Map table
 */
#define NSLOTS		4080	/* The mostest possible - may be fewer */
static int ir_nslots=0;		/* how many we have, <= NSLOTS */
static int slotsused;		/* how many actually used */
static struct rgbtab {
	unsigned char	r;
	unsigned char	g;
	unsigned char	b;
} ir_rgbtab[NSLOTS];

struct dm dm_4d = {
	Ir_open, Ir_close,
	Ir_input,
	Ir_prolog, Ir_epilog,
	Ir_normal, Ir_newrot,
	Ir_update,
	Ir_puts, Ir_2d_line,
	Ir_light,
	Ir_object,
	Ir_cvtvecs, Ir_load,
	Ir_statechange,
	Ir_viewchange,
	Ir_colorchange,
	Ir_window, Ir_debug,
	0,			/* no "displaylist", per. se. */
	0,			/* multi-window */
	IRBOUND,
	"sgi", "SGI 4d"
};

extern struct device_values dm_values;	/* values read from devices */

static int	ir_debug;		/* 2 for basic, 3 for full */
static int	ir_is_gt;		/* 0 for non-GT machines */
static int	ir_has_zbuf;		/* 0 if no Z buffer */
static int	ir_has_rgb;		/* 0 if mapped mode must be used */

long gr_id;
long win_l, win_b, win_r, win_t;
long winx_size, winy_size;

#if IR_WIDGETS
# include "forms.h"
# include "./mged_widgets.h"

static int win_focus = 0;

void button_call(obj, val)
OBJECT *obj;
long val;
{	
	short ev[4];


	ev[0] = (short)val;
	ev[1] = get_button(obj);
/*	(void)fprintf(stdout, "button %d\n", ev[1]); */
	ev[2] = getvaluator(MOUSEX);
	ev[3] = getvaluator(MOUSEY);

	lbr_qenter(ev);
}

void toggle_call(obj, val)
OBJECT *obj;
long val;
{
	short ev[4];

	ev[0] = (short)val;
	ev[1] = 1;
	ev[2] = getvaluator(MOUSEX);
	ev[3] = getvaluator(MOUSEY);

	lbr_qenter(ev);
	ev[1] = 0;
	lbr_qenter(ev);
}

void dial_call(obj, dial_dev)
OBJECT *obj;
long dial_dev;
{
	float	dial_val, min, max;
	short	valuator;

	dial_val = get_dial_value(obj);
	get_dial_bounds(obj, &min, &max);
	valuator = getvaluator(dial_dev);

	/* setvaluator will cause an event to be queued if the device
	 * has been queued.  Thus there is no reason for us to queue
	 * a "fake" dial event, setvaluator will do it for us ;-).
	 */
	setvaluator((short)dial_dev, (short)dial_val,
		(short)min, (short)max);
}

void help_call(obj, val)
OBJECT *obj;
long val;
{
	short ev[4];

	ev[0] = val;
	ev[1] = get_button(obj);
	ev[2] = getvaluator(MOUSEX);
	ev[3] = getvaluator(MOUSEY);

	(void)fprintf(stdout, "help button %d\n", ev[1]);
	
	lbr_qenter(ev);
}


#endif


/* Map +/-2048 GED space into -1.0..+1.0 :: x/2048*/
#define GED2IRIS(x)	(((float)(x))*0.00048828125)

/*
 *  Mouse coordinates are in absolute screen space, not relative to
 *  the window they came from.  Convert to window-relative,
 *  then to MGED-style +/-2048 range.
 */
static int
irisX2ged(x)
register int x;
{
	if( x <= win_l )  return(-2048);
	if( x >= win_r )  return(2047);
	x -= win_l;
	x = ( x / (double)winx_size)*4096.0;
	x -= 2048;
	return(x);
}

static int
irisY2ged(y)
register int y;
{
	
	if( y <= win_b )  return(-2048);
	if( y >= win_t )  return(2047);
	y -= win_b;
	y = ( y / (double)winy_size)*4096.0;
	y -= 2048;
	return(y);
}

/* 
 *			I R _ C O N F I G U R E _ W I N D O W _ S H A P E
 *
 *  Either initially, or on resize/reshape of the window,
 *  sense the actual size of the window, and perform any
 *  other initializations of the window configuration.
 */
static void
Ir_configure_window_shape()
{
	int		npix;

	getsize( &winx_size, &winy_size);
	getorigin( &win_l, & win_b );
	win_r = win_l + winx_size;
	win_t = win_b + winy_size;

	/* Write enable all the bloody bits after resize! */
	viewport(0, winx_size, 0, winy_size);

	if( ir_has_zbuf && zbuffer_on ) 
	{
		zbuffer(1);
		lsetdepth(0, 0x07fffff );
	}

	/* Clear out image from windows underneath */
	frontbuffer(1);
	ir_clear_to_black();
	frontbuffer(0);
	ir_clear_to_black();
	
	switch( getmonitor() )  {
	default:
		/*
		 *  Set an 8 minute screensaver blanking, which will light up
		 *  the screen again if it was dark, and will protect it otherwise.
		 *  4D/60 has a hardware botch limiting the time to 2**15 frames.
		 */
		blanktime( (long) 32767L );
		break;
	case NTSC:
		/* Only use the central square part */
		npix = YMAX170-30;
		winx_size = npix * 4 / 3;	/* NTSC aspect ratio */
		winy_size = npix;
		win_l = (XMAX170 - winx_size)/2;
		win_r = win_l + winx_size;
		win_b = (YMAX170-winy_size)/2;
		win_t = win_b + winy_size;
		viewport( (XMAX170 - npix)/2, npix + (XMAX170 - npix)/2,
			(YMAX170-npix)/2, npix + (YMAX170-npix)/2 );
		linewidth(3);
		blanktime(0);	/* don't screensave while recording video! */
		break;
	case PAL:
		/* Only use the central square part */
		npix = YMAXPAL-30;
		winx_size = npix;
		winy_size = npix;
		win_l = (XMAXPAL - winx_size)/2;
		win_r = win_l + winx_size;;
		win_b = (YMAXPAL-winy_size)/2;
		win_t = win_b + winy_size;
		viewport( (XMAXPAL - npix)/2, npix + (XMAXPAL - npix)/2,
			(YMAXPAL-npix)/2, npix + (YMAXPAL-npix)/2 );
		linewidth(3);
		blanktime(0);	/* don't screensave while recording video! */
		break;
	}
}

#define CMAP_BASE	32
#define CMAP_RAMP_WIDTH	16
#define MAP_ENTRY(x)	((cueing_on) ? \
			((x) * CMAP_RAMP_WIDTH + CMAP_BASE) : \
			((x) + CMAP_BASE) )


/*
 *			I R _ O P E N
 *
 *  Fire up the display manager, and the display processor. Note that
 *  this brain-damaged version of the MEX display manager gets terribly
 *  confused if you try to close your last window.  Tough. We go ahead
 *  and close the window.  Ignore the "ERR_CLOSEDLASTWINDOW" error
 *  message. It doesn't hurt anything.  Silly MEX.
 */

int
Ir_open()
{
	register int	i;
	Matrix		m;
	inventory_t	*inv;
#if IR_WIDGETS
	int		use_widgets=0;
#endif
	int		win_size=1000;
	int		win_o_x=272;
	int		win_o_y=12;

	/*
	 *  Take inventory of the hardware
	 */
	while( (inv = getinvent()) != (inventory_t *)0 )  {
		if( inv->class != INV_GRAPHICS )  continue;
		switch( inv->type )  {
		case INV_GMDEV:
			ir_is_gt = 1;
			ir_has_zbuf = 1;
			ir_has_rgb = 1;
			break;
		case INV_GR1BOARD:	/* Persoanl Iris */
			if ( inv->state & INV_GR1RE2 )
				ir_is_gt = 1;
			if(inv->state & INV_GR1ZBUF24 )
				ir_has_zbuf = 1;
			if(inv->state & INV_GR1BIT24 )
				ir_has_rgb = 1;
			break;
		}
	}
	endinvent();		/* frees internal inventory memory */
#if 0
	printf("4D: gt=%d, zbuf=%d, rgb=%d\n", ir_is_gt, ir_has_zbuf, ir_has_rgb);
#endif

	/* Start out with the usual window */
	foreground();
#if 1
	if (mged_variables.sgi_win_size > 0)
		win_size = mged_variables.sgi_win_size;

	if (mged_variables.sgi_win_origin[0] > 0 ||
	    mged_variables.sgi_win_origin[0] > 0) {
	    	win_o_x = mged_variables.sgi_win_origin[0];
	    	win_o_y = mged_variables.sgi_win_origin[1];
	}

	prefposition( win_o_x, win_o_x+win_size, win_o_y, win_o_y+win_size);
#else
	prefposition( 376, 1276, 12, 912 );		/* Old, larger size */
	prefposition( 376, 376+900, 112, 112+900 );	/* new, smaller size */
#endif
	if( (gr_id = winopen( "BRL MGED" )) == -1 )  {
		printf( "No more graphics ports available.\n" );
		return	-1;
	}
	keepaspect(1,1);	/* enforce 1:1 aspect ratio */
	winconstraints();	/* remove constraints on the window size */

	/*
	 *  If monitor is in special mode, close window and re-open.
	 *  winconstraints() does not work, and getmonitor() can't
	 *  be called before a window is open.
	 */
	switch( getmonitor() )  {
	case HZ30:
	case HZ30_SG:
		/* Dunn camera, etc. */
		/* Use already established prefposition */
		break;
	default:
	case HZ60:
		/* Regular hi-res monitor */
		/* Use already established prefposition */
#if IR_WIDGETS
		use_widgets = 1;
#endif
		break;
	case NTSC:
		/* Television */
		winclose(gr_id);
		prefposition( 0, XMAX170, 0, YMAX170 );
		foreground();
		if( (gr_id = winopen( "BRL MGED" )) == -1 )  {
			printf( "No more graphics ports available.\n" );
			return	-1;
		}
		break;
	case PAL:
		/* Television */
		winclose(gr_id);
		prefposition( 0, XMAXPAL, 0, YMAXPAL );
		foreground();
		if( (gr_id = winopen( "BRL MGED" )) == -1 )  {
			printf( "No more graphics ports available.\n" );
			return	-1;
		}
		break;
	}

	/*
	 *  Configure the operating mode of the pixels in this window.
	 *  Do not output graphics, clear screen, etc, until *after*
	 *  the call to gconfig().
	 */
	if( ir_has_rgb )  {
		RGBmode();
	} else {
		/* one indexed color map of 4096 entries */
		onemap();
	}
	doublebuffer();
	gconfig();

	/*
	 * Establish GL library operating modes
	 */
	/* Don't draw polygon edges */
	glcompat( GLC_OLDPOLYGON, 0 );
	/* Z-range mapping */
#if 0
	/* Optional:  Z from 0 to 0x007fffff */
	glcompat( GLC_ZRANGEMAP, 1 );
#else
	/* Z range from getgdesc(GD_ZMIN)
	 * to getgdesc(GD_ZMAX).
	 * Hardware specific.  This is the default mode.
	 */
	glcompat( GLC_ZRANGEMAP, 0 );
#endif

	Ir_configure_window_shape();

	/* Enable qdev() input from various devices */
#if IR_WIDGETS

	fl_usrqdevice(LEFTMOUSE);
	fl_usrqdevice(MIDDLEMOUSE);
	fl_usrqdevice(RIGHTMOUSE);
#else
	qdevice(LEFTMOUSE);
	qdevice(MIDDLEMOUSE);
	qdevice(RIGHTMOUSE);
	tie(MIDDLEMOUSE, MOUSEX, MOUSEY);
#endif
#if IR_KNOBS || IR_WIDGETS
	/*
	 *  Turn on the dials and initialize them for -2048 to 2047
	 *  range with a dead spot at zero (Iris knobs are 1024 units
	 *  per rotation).
	 */
	for(i = DIAL0; i < DIAL8; i++)
		setvaluator(i, 0, -2048-NOISE, 2047+NOISE);
	for(i = DIAL0; i < DIAL8; i++)
# if IR_WIDGETS
		fl_usrqdevice(i);
# else
		qdevice(i);
# endif

#endif
#if IR_BUTTONS || IR_WIDGETS
	/*
	 *  Enable all the buttons in the button table.
	 */
	for(i = 0; i < IR_BUTTONS; i++)
#if IR_WIDGETS
		fl_usrqdevice(i+SWBASE);
#else
		qdevice(i+SWBASE);
#endif
	/*
	 *  For all possible button presses, build a table
	 *  of MGED function to SGI button/light mappings.
	 */
	for( i=0; i < IR_BUTTONS; i++ )  {
		register int j;
		if( (j = bmap[i]) != 0 )
			invbmap[j] = i;
	}
# if 0
	ir_dbtext(ir_title);
# else
	dbtext("");
# endif
#endif

#if IR_WIDGETS
	fl_usrqdevice(F1KEY);	/* pf1 key for depthcue switching */
	fl_usrqdevice(F2KEY);	/* pf2 for Z clipping */
	fl_usrqdevice(F3KEY);	/* pf3 for perspective */
	fl_usrqdevice(F4KEY);	/* pf4 for Z buffering */
	fl_usrqdevice(F5KEY);	/* pf5 for lighting */
	fl_usrqdevice(F6KEY);	/* pf6 for changing perspective */
	fl_usrqdevice(F7KEY);	/* pf7 for no faceplate */
#else
	qdevice(F1KEY);	/* pf1 key for depthcue switching */
	qdevice(F2KEY);	/* pf2 for Z clipping */
	qdevice(F3KEY);	/* pf3 for perspective */
	qdevice(F4KEY);	/* pf4 for Z buffering */
	qdevice(F5KEY);	/* pf5 for lighting */
	qdevice(F6KEY);	/* pf6 for changing perspective */
	qdevice(F7KEY);	/* pf7 for no faceplate */
#endif
	while( getbutton(LEFTMOUSE)||getbutton(MIDDLEMOUSE)||getbutton(RIGHTMOUSE) )  {
		printf("IRIS_open:  mouse button stuck\n");
		sleep(1);
	}

	/* Line style 0 is solid.  Program line style 1 as dot-dashed */
	deflinestyle( 1, 0xCF33 );
	setlinestyle( 0 );

	/* Compute some viewing matricies */
	mat_idn( nozclip_mat );
	nozclip_mat[10] = 1.0e-20;
	persp_mat( perspect_mat, 90.0, 1.0, 0.01, 1.0e10, 1.0 );

#if IR_WIDGETS
	if (use_widgets) {
		create_the_forms();

		show_forms(Button_Box, PLACE_SIZE, TRUE, "Soft Buttons");
		show_forms(Dials, PLACE_SIZE, TRUE, "Soft Dials");
	}
#endif

	return(0);
}

/*
 *  			I R _ C L O S E
 *  
 *  Gracefully release the display.  Well, mostly gracefully -- see
 *  the comments in the open routine.
 */
void
Ir_close()
{
	if(cueing_on) depthcue(0);

	lampoff( 0xf );

	frontbuffer(1);
	ir_clear_to_black();
	frontbuffer(0);

#if IR_WIDGETS
	hide_form(Button_Box);
	hide_form(Dials);
#endif
	winclose(gr_id);
	return;
}

/*
 *			I R _ P R O L O G
 *
 * Define the world, and include in it instances of all the
 * important things.  Most important of all is the object "faceplate",
 * which is built between dmr_normal() and dmr_epilog()
 * by dmr_puts and dmr_2d_line calls from adcursor() and dotitles().
 */
void
Ir_prolog()
{

	ortho2( -1.0,1.0, -1.0,1.0);	/* L R Bot Top */

	if( !dmaflag )
		return;
}

/*
 *			I R _ N O R M A L
 *
 * Restore the display processor to a normal mode of operation
 * (ie, not scaled, rotated, displaced, etc).
 * Turns off windowing.
 */
void
Ir_normal()
{
	if( ir_has_rgb )  {
		RGBcolor( (short)0, (short)0, (short)0 );
	} else {
		color(BLACK);
	}

	ortho2( -1.0,1.0, -1.0,1.0);	/* L R Bot Top */
}

/*
 *			I R _ E P I L O G
 */
void
Ir_epilog()
{
	/*
	 * A Point, in the Center of the Screen.
	 * This is drawn last, to always come out on top.
	 */
	Ir_2d_line( 0, 0, 0, 0, 0 );
	/* End of faceplate */

	swapbuffers();

	/* give Graphics pipe time to work */
	ir_clear_to_black();
}

/*
 *  			I R _ N E W R O T
 *  Stub.
 */
void
Ir_newrot(mat)
mat_t mat;
{
}

/*
 *  			I R _ O B J E C T
 *  
 *  Set up for an object, transformed as indicated, and with an
 *  object center as specified.  The ratio of object to screen size
 *  is passed in as a convienience.  Mat is model2view.
 *
 *  Returns -
 *	 0 if object could be drawn
 *	!0 if object was omitted.
 *
 *  IMPORTANT COORDINATE SYSTEM NOTE:
 *
 *  MGED uses a right handed coordinate system where +Z points OUT of
 *  the screen.  The Silicon Graphics uses a left handed coordinate
 *  system where +Z points INTO the screen.
 *  This difference in convention is handled here.
 *  The conversion is accomplished by concatenating a matrix multiply
 *  by
 *            (  1    0    0   0  )
 *            (  0    1    0   0  )
 *            (  0    0   -1   0  )
 *            (  0    0    0   1  )
 *
 *  However, this is actually implemented by straight-line code which
 *  flips the sign on the entire third row.
 *
 *  Note that through BRL-CAD Release 3.7 this was handled by flipping
 *  the direction of the shade ramps.  Now, with the Z-buffer being used,
 *  the correct solution is important.
 */
int
Ir_object( sp, m, ratio, white )
register struct solid *sp;
register fastf_t *m;
double ratio;
{
	register struct rt_vlist	*vp;
	register int nvec;
	register float	*gtvec;
	char	gtbuf[16+3*sizeof(double)];
	register fastf_t *mptr;
	Matrix gtmat;
	int first;
	int i,j;	
	mat_t	mtmp, newm;

	/* It seems that this needs to be done before the loadmatrix() */
	if( ir_is_gt && lighting_on )  {
		/* Separate projection matrix from
		 * modeling and viewing xforms.
		 */
		mmode(MVIEWING);
		/* Select the material */
		lmbind(MATERIAL, 20);
	}


	/*
	 *  It is claimed that the "dancing vector disease" of the
	 *  4D GT processors is due to the array being passed to v3f()
	 *  not being quad-word aligned (16-byte boundary).
	 *  This hack ensures that the buffer has this alignment.
	 *  Note that this requires gtbuf to be 16 bytes longer than needed.
	 */
	gtvec = (float *)((((int)gtbuf)+15) & (~0xF));

	/* This section has the potential of being speed up since a new
	 * matrix is loaded for each object. Even though its the same
	 * matrix.
	 */
	if( perspective_mode ) {
		mat_mul( newm, perspect_mat, m );
		m = newm;
	} else if( ! zclipping_on ) {
		mat_mul( newm, nozclip_mat, m );
		m = newm;
	}

	mptr = m;
	for(i= 0; i < 4; i++)
	for(j= 0; j < 4; j++)
		gtmat[j][i] = *(mptr++);

	/*
	 *  Convert between MGED's right handed coordinate system
	 *  where +Z comes out of the screen to the Silicon Graphics's
	 *  left handed coordinate system, where +Z goes INTO the screen.
	 */
	gtmat[0][2] = -gtmat[0][2];
	gtmat[1][2] = -gtmat[1][2];
	gtmat[2][2] = -gtmat[2][2];
	gtmat[3][2] = -gtmat[3][2];

	loadmatrix( gtmat );


	/*
	 * IMPORTANT DEPTHCUEING NOTE
	 *
	 * Also note that the depthcueing shaderange() routine wanders
	 * outside it's allotted range due to roundoff errors.  A buffer
	 * entry is kept on each end of the shading curves, and the
	 * highlight mode uses the *next* to the brightest entry --
	 * otherwise it can (and does) fall off the shading ramp.
	 */
	if (sp->s_soldash)
		setlinestyle( 1 );		/* set dot-dash */

	if( ir_has_rgb )  {
		register short	r, g, b;
		if( white )  {
			r = g = b = 230;
		} else {
			r = (short)sp->s_color[0];
			g = (short)sp->s_color[1];
			b = (short)sp->s_color[2];
		}
		if(cueing_on)  {
			/* RGBrange marked obsolete, use lRGBrange! */
			lRGBrange(
				r/10, g/10, b/10,
				r, g, b,
				0, 0x007fffff );
			/* XXX should investigate lRGBrange, lshaderange, lsetdepth */
		}
		RGBcolor( r, g, b );
	} else {
		if( white ) {
			ovec = nvec = MAP_ENTRY(DM_WHITE);
			/* Use the *next* to the brightest white entry */
			if(cueing_on) lshaderange(nvec+1, nvec+1, 0, 0x007fffff);

			color( nvec );
		} else {
			if( (nvec = MAP_ENTRY( sp->s_dmindex )) != ovec) {
				/* Use only the middle 14 to allow for roundoff...
				 * Pity the poor fool who has defined a black object.
				 * The code will use the "reserved" color map entries
				 * to display it when in depthcued mode.
				 */
			  	if(cueing_on) lshaderange(nvec+1, nvec+14, 0, 0x007fffff);
				color( nvec );
			  	ovec = nvec;
			  }
		}
	}

	/* Viewing region is from -1.0 to +1.0 */
	first = 1;
	for( RT_LIST_FOR( vp, rt_vlist, &(sp->s_vlist) ) )  {
		register int	i;
		register int	nused = vp->nused;
		register int	*cmd = vp->cmd;
		register point_t *pt = vp->pt;
		for( i = 0; i < nused; i++,cmd++,pt++ )  {
			switch( *cmd )  {
			case RT_VLIST_LINE_MOVE:
				/* Move, start line */
				if( first == 0 )
					endline();
				first = 0;
				bgnline();
				v3d( *pt );
				break;
			case RT_VLIST_LINE_DRAW:
				/* Draw line */
				v3d( *pt );
				break;
			case RT_VLIST_POLY_START:
				/* Start poly marker & normal */
				if( first == 0 )
					endline();
				/* concave(TRUE); */
				bgnpolygon();
				/* Set surface normal (vl_pnt points outward) */
				VMOVE( gtvec, *pt );
				n3f(gtvec);
				break;
			case RT_VLIST_POLY_MOVE:
				/* Polygon Move */
				v3d( *pt );
				break;
			case RT_VLIST_POLY_DRAW:
				/* Polygon Draw */
				v3d( *pt );
				break;
			case RT_VLIST_POLY_END:
				/* Draw, End Polygon */
				v3d( *pt );
				endpolygon();
				first = 1;
				break;
			}
		}
	}
	if( first == 0 ) endline();
	if( lighting_on )  {
		/* Return to no-lighting mode */
		mmode(MSINGLE);
	}

	if (sp->s_soldash)
		setlinestyle(0);		/* restore solid lines */

	return(1);	/* OK */
}

/*
 *			I R _ U P D A T E
 *
 * Transmit accumulated displaylist to the display processor.
 * Last routine called in refresh cycle.
 */
void
Ir_update()
{
	if( !dmaflag )
		return;
}

/*
 *			I R _ P U T S
 *
 * Output a string.
 * The starting position of the beam is as specified.
 */
void
Ir_puts( str, x, y, size, colour )
register char *str;
int x,y,size, colour;
{
	if( no_faceplate )  return;

	cmov2( GED2IRIS(x), GED2IRIS(y));
	if( ir_has_rgb )  {
		RGBcolor( (short)ir_rgbtab[colour].r,
			(short)ir_rgbtab[colour].g,
			(short)ir_rgbtab[colour].b );
	} else {
		color( MAP_ENTRY(colour) );
	}
	charstr( str );
}

/*
 *			I R _ 2 D _ L I N E
 *
 */
void
Ir_2d_line( x1, y1, x2, y2, dashed )
int x1, y1;
int x2, y2;
int dashed;
{
	register int nvec;

	if( no_faceplate )  return;

	if( ir_has_rgb )  {
		/* Yellow */
		if(cueing_on)  {
			lRGBrange(
				255, 255, 0,
				255, 255, 0,
				0, 0x007fffff );
		}
		RGBcolor( (short)255, (short)255, (short) 0 );
	} else {
		if((nvec = MAP_ENTRY(DM_YELLOW)) != ovec) {
		  	if(cueing_on) lshaderange(nvec, nvec, 0, 0x007fffff);
			color( nvec );
		  	ovec = nvec;
		}
	}

	if( dashed )
		setlinestyle(1);	/* into dot-dash */

	move2( GED2IRIS(x1), GED2IRIS(y1));
	draw2( GED2IRIS(x2), GED2IRIS(y2));

	if( dashed )
		setlinestyle(0);	/* restore to solid */
}

/*
 *			I R _ I N P U T
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
Ir_input( cmd_fd, rateflg )
{
	static int cnt;
	register int i;

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
	/* System V on IRIS defines select() for graphics library.
	 * Also note that we need to frequently check qtest() for devices.
	 * Under MEX, need to swap buffers, for -other- programs to get
	 * a chance.
	 */
	do  {
		cnt = 0;
		i = qtest();
		if( i != 0 )
			break;
#ifdef NONBLOCK
		clearerr( stdin );
		cnt = 1;
		(void)ioctl( cmd_fd, FIONBIO, &cnt );
		cnt = 1;
#else
		if( rateflg )  {
			cnt = bsdselect( (long)(1<<cmd_fd), 0, 0 );
		}  else  {
			/* 1/20th second */
			cnt = bsdselect( (long)(1<<cmd_fd), 0, 50000 );
		}
#endif
		if( cnt != 0 )
			break;
		/* If Rate operation, return immed., else wait. */
		/* For MEX in double buffered mode, we really ought to
		 * do a swapbuffers() here, but that means arranging to
		 * have something sensible in the back buffer already,
		 * and at the moment, it's black.  We could leave it stale,
		 * but that would be just as bad.  For now, punt.
		 */
	} while( rateflg == 0 );

	/*
	 * Set device interface structure for GED to "rest" state.
	 * First, process any messages that came in.
	 */
	dm_values.dv_buttonpress = 0;
	dm_values.dv_flagadc = 0;
	dm_values.dv_penpress = 0;

	if( i != 0 )
		checkevents();

	if( cnt > 0 )
		return(1);		/* command awaits */
	else
		return(0);		/* just peripheral stuff */
}
/*
 *  C H E C K E V E N T S
 *
 *  Look at events to check for button, dial, and mouse movement.
 */
checkevents()  {
#define NVAL 48
	short values[NVAL];
	register short *valp;
	register int ret;
	register int n;
	static	button0  = 0;	/*  State of button 0 */
	static	knobs[8];	/*  Save values of dials  */
	static	pending_middlemouse = 0;	/* state variable */

#if IR_WIDGETS
	n = fl_usrblkqread( values, NVAL );/* n is # of shorts returned */
	if( ir_debug ) printf("fl_usrblkqread gave %d\n", n);
	for (valp = values ; n > 0  ; n -= 4, valp += 4 ) {
		ret = *valp;

#else
	n = blkqread( values, NVAL );	/* n is # of shorts returned */
	if( ir_debug ) printf("blkqread gave %d\n", n);
	for( valp = values; n > 0; n -= 2, valp += 2 )  {

		ret = *valp;
		if( ir_debug ) printf("qread ret=%d, val=%d\n", ret, valp[1]);
#endif
#if IR_BUTTONS || IR_WIDGETS
		if((ret >= SWBASE && ret < SWBASE+IR_BUTTONS)
		  || ret == F1KEY || ret == F2KEY || ret == F3KEY
		  || ret == F4KEY || ret == F5KEY || ret == F6KEY
		  || ret == F7KEY
		) {
			register int	i;

			/*
			 *  Switch 0 is help key.
			 *  Holding down key 0 and pushing another
			 *  button or turning the knob will display
			 *  what it is in the button box displya.
			 */
			if(ret == SW_HELP_KEY)  {
				button0 = valp[1];
#if IR_KNOBS
				/*
				 *  Save and restore dial settings
				 *  so that when the user twists them
				 *  while holding down button 0 he
				 *  really isn't changing them.
				 */
				for(i = 0; i < 8; i++)
					if(button0)
						 knobs[i] =
						   getvaluator(DIAL0+i);
					else setvaluator(DIAL0+i,knobs[i],
						-2048-NOISE, 2047+NOISE);
#endif
				if(button0)
					ir_dbtext("Help Key");
# if 0
				else
					ir_dbtext(ir_title);
# endif
				continue;
				
			}

#if IR_KNOBS || IR_WIDGETS
			/*
			 *  If button 1 is pressed, reset run away knobs.
			 */
			if(ret == SW_ZERO_KEY)  {
				if(!valp[1]) continue; /* Ignore release */
				/*  Help mode */
				if(button0)  {
					ir_dbtext("ZeroKnob");
					continue;
				}
#if IR_WIDGETS
				/* zero the widget knobs */
#endif
#if IR_KNOBS
				/* zap the knobs */
				for(i = 0; i < 8; i++)  {
					setvaluator(DIAL0+i, 0,
					  -2048-NOISE, 2047+NOISE);
					knobs[i] = 0;
				}
#endif
				continue;
			}
#endif
			/*
			 *  If PFkey 1 is pressed, toggle depthcueing.
			 */
			if(ret == F1KEY)  {
				if(!valp[1]) continue; /* Ignore release */
				/*  Help mode */
				if(button0)  {
					ir_dbtext("Depthcue");
					continue;
				}
				/* toggle depthcueing and remake colormap */
				cueing_on = cueing_on ? 0 : 1;
				Ir_colorchange();
				continue;
			} else if(ret == F2KEY)  {
				if(!valp[1]) continue; /* Ignore release */
				/*  Help mode */
				if(button0)  {
					ir_dbtext("Z clip");
					continue;
				}
				/* toggle zclipping */
				zclipping_on = zclipping_on ? 0 : 1;
				dmaflag = 1;
				continue;
			} else if(ret == F3KEY)  {
				if(!valp[1]) continue; /* Ignore release */
				/*  Help mode */
				if(button0)  {
					ir_dbtext("Perspect");
					continue;
				}
				perspective_mode = 1-perspective_mode;
				dmaflag = 1;
				continue;
			} else if(ret == F4KEY)  {
				if(!valp[1]) continue; /* Ignore release */
				/*  Help mode */
				if(button0)  {
					ir_dbtext("Zbuffing");
					continue;
				}
				if( ir_has_zbuf == 0 )  {
					printf("dm-4d: This machine has no Zbuffer to enable\n");
					continue;
				}
				/* toggle zbuffer status */
				zbuffer_on = zbuffer_on ? 0 : 1;
				zbuffer( zbuffer_on );
				if( zbuffer_on) lsetdepth(0, 0x007fffff);
				dmaflag = 1;
				continue;
			} else if(ret == F5KEY)  {
				if(!valp[1]) continue; /* Ignore release */
				/*  Help mode */
				if(button0)  {
					ir_dbtext("Lighting");
					continue;
				}
				/* toggle status */
				if( lighting_on )  {
					/* Turn it off */
					mmode(MSINGLE);
					lmbind(MATERIAL,0);
					lmbind(LMODEL,0);
					lighting_on = 0;
				} else {
					/* Turn it on */
					if( cueing_on )  {
						/* Has to be off for lighting */
						cueing_on = 0;
						Ir_colorchange();
					}

					/* Define material properties */
					make_materials();

					lmbind(LMODEL, 2);	/* infinite */

					lmbind(LIGHT2,2);
					lmbind(LIGHT3,3);
					lmbind(LIGHT4,4);
					lmbind(LIGHT5,5);

					/* RGB color commands & lighting */
#if 1
					/* Good for debugging */
					/* Material color does not apply,
					 * when lighting is on */
					lmcolor( LMC_COLOR );	/* default */
#else
					/* Good for looking.
					 * RGBcolor() values go to emissions
					 * durring lighting calculations.
					 */
					lmcolor( LMC_EMISSION );
#endif

					lighting_on = 1;
				}
				dmaflag = 1;
				continue;
			} else if (ret == F6KEY) {
				if (!valp[1]) continue; /* Ignore release */
				/* Help mode */
				if (button0) {
					ir_dbtext("P-Angle");
					continue;
				}
				/* toggle perspective matrix */
				if (--perspective_angle < 0) perspective_angle = 3;
				switch (perspective_angle) {
				case 3:
					persp_mat( perspect_mat, 90.0,
					    1.0, 0.01, 1.0e10, 1.0 );
					break;
				case 2:
					persp_mat( perspect_mat, 60.0,
					    1.0, 0.01, 1.0e10, 1.0 );
					break;
				case 1:
					persp_mat( perspect_mat, 45.0,
					    1.0, 0.01, 1.0e10, 1.0 );
					break;
				case 0:
					persp_mat( perspect_mat, 30.0,
					    1.0, 0.01, 1.0e10, 1.0 );
					break;
				}
				dmaflag = 1;
				continue;
			} else if (ret == F7KEY) {
				if (!valp[1]) continue; /* Ignore release */
				/* Help mode */
				if (button0) {
					ir_dbtext("NoFace");
					continue;
				}
				/* Toggle faceplate on/off */
				no_faceplate = !no_faceplate;
				dmaflag = 1;
				continue;
			}
			/*
			 * If button being depressed (as opposed to
			 * being released) either display the cute
			 * message or process the button depending
			 * on whether button0 is also being held down.
			 */
			i = bmap[ret - SWBASE];



			if(!valp[1]) continue;
			if(button0) {
				 ir_dbtext(label_button(i));
			} else {
				/* An actual button press */
#				if 0
					/* old way -- an illegal upcall */
					button(i);
#				else
					/* better way -- queue a string command */
					rt_vls_strcat( &dm_values.dv_string,
						"press " );
					rt_vls_strcat( &dm_values.dv_string,
						label_button(i) );
					rt_vls_strcat( &dm_values.dv_string,
						"\n" );
#				endif
			}
			continue;
		}
#endif
#if IR_KNOBS
		/*  KNOBS, uh...er...DIALS  */
		/*	6	7
		 *	4	5
		 *	2	3
		 *	0	1
		 */
		if(ret >= DIAL0 && ret <= DIAL8)  {
			int	setting;	
			/*  Help MODE */
			if(button0)  {
				ir_dbtext(
		(adcflag ? kn1_knobs:kn2_knobs)[ret-DIAL0]);
				continue;
			}
			/* Make a dead zone around 0 */
			setting = irlimit(valp[1]);
			switch(ret)  {
			case DIAL0:
				if(adcflag) {
					dm_values.dv_1adc = setting;
					dm_values.dv_flagadc =1;
				}
				break;
			case DIAL1:
				dm_values.dv_zoom = setting / 2048.0;
				break;
			case DIAL2:
				if(adcflag) {
					dm_values.dv_2adc = setting;
					dm_values.dv_flagadc =1;
				} else {
					dm_values.dv_zjoy = setting/2048.0;
				}
				break;
			case DIAL3:
				if(adcflag) {
					dm_values.dv_distadc = setting;
					dm_values.dv_flagadc =1;
				}
				else dm_values.dv_zslew = setting/2048.0;
				break;
			case DIAL4:
				if(adcflag) {
					dm_values.dv_yadc = setting;
					dm_values.dv_flagadc =1;
				} else {
					dm_values.dv_yjoy = setting/2048.0;
				}
				break;
			case DIAL5:
				dm_values.dv_yslew = setting/2048.0;
				break;
			case DIAL6:
				if(adcflag) {
					dm_values.dv_xadc = setting;
					dm_values.dv_flagadc =1;
				} else {
					dm_values.dv_xjoy = setting/2048.0;
				}
				break;
			case DIAL7:
				dm_values.dv_xslew = setting/2048.0;
				break;
			}
			continue;
		}
#endif
#if IR_WIDGETS
		switch( ret ) {
		case LEFTMOUSE:
			if (win_focus && valp[1] &&
			    dm_values.dv_penpress != DV_PICK )
				dm_values.dv_penpress = DV_OUTZOOM;
			break;
		case MIDDLEMOUSE:
			if (win_focus) {
				dm_values.dv_xpen = irisX2ged( (int)valp[2] );
				dm_values.dv_ypen = irisY2ged( (int)valp[3] );
				dm_values.dv_penpress = DV_PICK;
			}
			break;
		case RIGHTMOUSE:
			if( win_focus && valp[1] && 
			    dm_values.dv_penpress != DV_PICK )
				dm_values.dv_penpress = DV_INZOOM;
			break;
		case REDRAW:
			/* Window may have moved */
			Ir_configure_window_shape();
			dmaflag = 1;
			refresh();		/* to fix back buffer */
			dmaflag = 1;
			break;
		case INPUTCHANGE:
			/* Means we got or lost the input focus. */
			win_focus = (valp[1] == gr_id);
			break;
		case WMREPLY:
			/* This guy speaks, but has nothing to say */
			break;
		default:
			printf("IRIS device %d gave %d?\n", ret, valp[1]);
			break;
		}

#else
		switch( ret )  {
		case LEFTMOUSE:
			if( valp[1] && dm_values.dv_penpress != DV_PICK )
				dm_values.dv_penpress = DV_OUTZOOM;
			break;
		case MIDDLEMOUSE:
			/* Will also get MOUSEX and MOUSEY hits */
			if( valp[1] ) {
				pending_middlemouse = 1;
				/* Don't signal DV_PICK until MOUSEY event */
			}
			break;
		case RIGHTMOUSE:
			if( valp[1] && dm_values.dv_penpress != DV_PICK )
				dm_values.dv_penpress = DV_INZOOM;
			break;
		case MOUSEX:
			dm_values.dv_xpen = irisX2ged( (int)valp[1] );
			break;
		case MOUSEY:
			dm_values.dv_ypen = irisY2ged( (int)valp[1] );
			/*
			 *  Thanks to the tie() call, when a MIDDLEMOUSE
			 *  event is signalled, it will be (eventually)
			 *  followed by a MOUSEX and MOUSEY event.
			 *  The MOUSEY event may not be in the same
			 *  blkqread() buffer, owing to time delays in
			 *  the outputting of the tie()'ed events.
			 *  So, don't signal DV_PICK on MIDDLEMOUSE;
			 *  instead, set the flag, and patiently wait until
			 *  the subsequent MOUSEY event has arrived, which
			 *  may need multiple trips through this subroutine.
			 */
			if( pending_middlemouse )  {
				dm_values.dv_penpress = DV_PICK;
				pending_middlemouse = 0;
			}
			break;
		case REDRAW:
			/* Window may have moved */
			Ir_configure_window_shape();
			dmaflag = 1;
			refresh();		/* to fix back buffer */
			dmaflag = 1;
			break;
		case INPUTCHANGE:
			/* Means we got or lost the keyboard.  Ignore */
			break;
		case WMREPLY:
			/* This guy speaks, but has nothing to say */
			break;
		default:
			printf("IRIS device %d gave %d?\n", ret, valp[1]);
			break;
		}
#endif
	}
}

/* 
 *			I R _ L I G H T
 *
 * This function must keep both the light hardware, and the software
 * copy of the lights up to date.  Note that requests for light changes
 * may not actually cause the lights to be changed, depending on
 * whether the buttons are being used for "view" or "edit" functions
 * (although this is not done in the present code).
 */
void
Ir_light( cmd, func )
int cmd;
int func;			/* BE_ or BV_ function */
{
	register unsigned short bit;
#ifdef IR_BUTTONS
	/* Check for BE_ function not assigned to a button */
	if( (bit = invbmap[func]) == 0 && cmd != LIGHT_RESET )
		return;
	switch( cmd )  {
	case LIGHT_RESET:
		lights = 0;
		break;
	case LIGHT_ON:
		lights |= 1<<bit;
		break;
	case LIGHT_OFF:
		lights &= ~(1<<bit);
		break;
	}

	/* Update the lights box. */
#if !defined(__sgi)	/* This bombs, on early Irix 4.0 releases */
	setdblights( lights );
#endif
#endif
}

/*
 *			I R _ C V T V E C S
 *
 */
unsigned
Ir_cvtvecs( sp )
register struct solid *sp;
{
	return( 0 );	/* No "displaylist" consumed */
}

/*
 * Loads displaylist from storage[]
 */
unsigned
Ir_load( addr, count )
unsigned addr, count;
{
	return( 0 );		/* FLAG:  error */
}

void
Ir_statechange( a, b )
{
	if( ir_debug ) printf("statechange %d %d\n", a, b );
	/*
	 *  Based upon new state, possibly do extra stuff,
	 *  including enabling continuous tablet tracking,
	 *  object highlighting
	 */
	switch( b )  {
	case ST_VIEW:
#if IR_WIDGETS
		fl_usrunqdevice( MOUSEY );	/* constant tracking OFF */
#else
		unqdevice( MOUSEY );	/* constant tracking OFF */
#endif
		/* This should not affect the tie()'d MOUSEY events */
		break;
		
	case ST_S_PICK:
	case ST_O_PICK:
	case ST_O_PATH:
		/*  Have all changes of MOUSEY generate an event */
#if IR_WIDGETS
		fl_usrqdevice( MOUSEY );	/* constant tracking ON */
#else
		qdevice( MOUSEY );	/* constant tracking ON */
#endif
		break;
	case ST_O_EDIT:
	case ST_S_EDIT:
#if IR_WIDGETS
		fl_usrunqdevice( MOUSEY );	/* constant tracking OFF */
#else
		unqdevice( MOUSEY );	/* constant tracking OFF */
#endif
		break;
	default:
		(void)printf("Ir_statechange: unknown state %s\n", state_str[b]);
		break;
	}
	Ir_viewchange( DM_CHGV_REDO, SOLID_NULL );
}

void
Ir_viewchange( cmd, sp )
register int cmd;
register struct solid *sp;
{
	if( ir_debug ) printf("viewchange( %d, x%x )\n", cmd, sp );
	switch( cmd )  {
	case DM_CHGV_ADD:
		break;
	case DM_CHGV_REDO:
		break;
	case DM_CHGV_DEL:
		break;
	case DM_CHGV_REPL:
		return;
	case DM_CHGV_ILLUM:
		break;
	}
}

void
Ir_debug(lvl)
{
	ir_debug = lvl;
}

void
Ir_window(w)
int w[];
{
}


/*
 *  			I R _ C O L O R C H A N G E
 *  
 *  Go through the solid table, and allocate color map slots.
 *	8 bit system gives 4 or 8,
 *	24 bit system gives 12 or 24.
 */
void
Ir_colorchange()
{
	register int i;
	register int nramp;

	if( ir_debug )  printf("colorchange\n");

	/* Program the builtin colors */
	ir_rgbtab[0].r=0; ir_rgbtab[0].g=0; ir_rgbtab[0].b=0;/* Black */
	ir_rgbtab[1].r=255; ir_rgbtab[1].g=0; ir_rgbtab[1].b=0;/* Red */
	ir_rgbtab[2].r=0; ir_rgbtab[2].g=0; ir_rgbtab[2].b=255;/* Blue */
	ir_rgbtab[3].r=255; ir_rgbtab[3].g=255;ir_rgbtab[3].b=0;/*Yellow */
	ir_rgbtab[4].r = ir_rgbtab[4].g = ir_rgbtab[4].b = 255; /* White */
	slotsused = 5;

	if( ir_has_rgb )  {
		if(cueing_on) {
			depthcue(1);
		} else {
			depthcue(0);
		}

		RGBcolor( (short)255, (short)255, (short)255 );

		/* apply region-id based colors to the solid table */
		color_soltab();

		return;
	}

	ir_nslots = getplanes();
	if(cueing_on && (ir_nslots < 7)) {
		printf("Too few bitplanes: depthcueing disabled\n");
		cueing_on = 0;
	}
	ir_nslots = 1<<ir_nslots;
	if( ir_nslots > NSLOTS )  ir_nslots = NSLOTS;
	if(cueing_on) {
		/* peel off reserved ones */
		ir_nslots = (ir_nslots - CMAP_BASE) / CMAP_RAMP_WIDTH;
		depthcue(1);
	} else {
		ir_nslots -= CMAP_BASE;	/* peel off the reserved entries */
		depthcue(0);
	}

	ovec = -1;	/* Invalidate the old colormap entry */

	/* apply region-id based colors to the solid table */
	color_soltab();

	/* Map the colors in the solid table to colormap indices */
	ir_colorit();

	for( i=0; i < slotsused; i++ )  {
		gen_color( i, ir_rgbtab[i].r, ir_rgbtab[i].g, ir_rgbtab[i].b);
	}

	color(WHITE);	/* undefinied after gconfig() */
}

int
ir_colorit()
{
	register struct solid	*sp;
	register struct rgbtab *rgb;
	register int i;
	register int r,g,b;

	if( ir_has_rgb )  return;

	FOR_ALL_SOLIDS( sp )  {
		r = sp->s_color[0];
		g = sp->s_color[1];
		b = sp->s_color[2];
		if( (r == 255 && g == 255 && b == 255) ||
		    (r == 0 && g == 0 && b == 0) )  {
		    	sp->s_dmindex = DM_WHITE;
			continue;
		}

		/* First, see if this matches an existing color map entry */
		rgb = ir_rgbtab;
		for( i = 0; i < slotsused; i++, rgb++ )  {
			if( rgb->r == r && rgb->g == g && rgb->b == b )  {
				sp->s_dmindex = i;
				goto next;
			}
		}

		/* If slots left, create a new color map entry, first-come basis */
		if( slotsused < ir_nslots )  {
			rgb = &ir_rgbtab[i=(slotsused++)];
			rgb->r = r;
			rgb->g = g;
			rgb->b = b;
			sp->s_dmindex = i;
			continue;
		}
		sp->s_dmindex = DM_YELLOW;	/* Default color */
next:		;
	}
}

/*
 *  I R _ D B T E X T
 *
 *  Used to call dbtext to print cute messages on the button box,
 *  if you have one.  Has to shift everythign to upper case
 *  since the box goes off the deep end with lower case.
 *
 *  Because not all SGI button boxes have text displays,
 *  this now needs to go to stdout in order to be useful.
 */
 
ir_dbtext(str)
	register char *str;
{
#if IR_BUTTONS
  	register i;
	char	buf[9];
	register char *cp;

# if 0
	for(i = 0, cp = buf; i < 8 && *str; i++, cp++, str++)
		*cp = islower(*str) ?  toupper(*str) : *str;
	*cp = 0;
	dbtext(buf);
# else
	printf("dm-4d: You pressed Help key and '%s'\n", str);
# endif
#else
	return;
#endif
}

#if IR_KNOBS
/*
 *			I R L I M I T
 *
 * Because the dials are so sensitive, setting them exactly to
 * zero is very difficult.  This function can be used to extend the
 * location of "zero" into "the dead zone".
 */
static 
int irlimit(i)
	int i;
{
	if( i > NOISE )
		return( i-NOISE );
	if( i < -NOISE )
		return( i+NOISE );
	return(0);
}
#endif

/*			G E N _ C O L O R
 *
 *	maps a given color into the appropriate colormap entry
 *	for both depthcued and non-depthcued mode.  In depthcued mode,
 *	gen_color also generates the colormap ramps.  Note that in depthcued
 *	mode, DM_BLACK uses map entry 0, and does not generate a ramp for it.
 *	Non depthcued mode skips the first CMAP_BASE colormap entries.
 *
 *	This routine is not called at all if ir_has_rgb is set.
 */
gen_color(c)
int c;
{
	if(cueing_on) {
		
		/*  Not much sense in making a ramp for DM_BLACK.  Besides
		 *  which, doing so, would overwrite the bottom color
		 *  map entries, which is a no-no.
		 */
		if( c != DM_BLACK) {
			register int i;
			fastf_t r_inc, g_inc, b_inc;
			fastf_t red, green, blue;

			r_inc = ir_rgbtab[c].r/16;
			g_inc = ir_rgbtab[c].g/16;
			b_inc = ir_rgbtab[c].b/16;

			red = ir_rgbtab[c].r;
			green = ir_rgbtab[c].g;
			blue = ir_rgbtab[c].b;

			for(i = 15; i >= 0;
			  i--, red -= r_inc, green -= g_inc, blue -= b_inc)
				mapcolor( MAP_ENTRY(c) + i,
					(short)red,
					(short)green,
					(short)blue );
		}
	} else {
		mapcolor(c+CMAP_BASE,
			ir_rgbtab[c].r, ir_rgbtab[c].g, ir_rgbtab[c].b);
	}
}

#ifdef never
/*
 *  Update the PF key lights.
 */
static int
kblights()
{
	char	lights;

	lights = (cueing_on)
		| (zclipping_on << 1)
		| (perspective_mode << 2)
		| (zbuffer_on << 3);

	lampon(lights);
	lampoff(lights^0xf);
}
#endif
/*
 *  Compute a perspective matrix.
 *  Reference: SGI Graphics Reference Appendix C
 */
static int
persp_mat( m, fovy, aspect, near, far, backoff )
mat_t	m;
fastf_t	fovy, aspect, near, far, backoff;
{
	mat_t	m2, tran;

	fovy *= 3.1415926535/180.0;

	mat_idn( m2 );
	m2[5] = cos(fovy/2.0) / sin(fovy/2.0);
	m2[0] = m2[5]/aspect;
	m2[10] = -(far+near) / (far-near);	/* negate Z! */
	m2[11] = -2*far*near / (far-near);
	m2[14] = -1;
	m2[15] = 0;

	mat_idn( tran );
	tran[11] = -backoff;
	mat_mul( m, m2, tran );
}

ir_clear_to_black()
{
	if( zbuffer_on )  {
		zfunction( ZF_LEQUAL );
		if( ir_has_rgb )  {
			czclear( 0, 0x7fffff );
		} else {
			czclear( BLACK, 0x7fffff );
		}
	} else {
		if( ir_has_rgb )  {
			RGBcolor( (short)0, (short)0, (short)0 );
		} else {
			color(BLACK);
		}
		clear();
	}
}

#if 0
/* Handy fakeouts when we don't want to link with -lmpc */
usinit()	{ printf("usinit\n"); }
usnewlock()	{ printf("usnewlock\n"); }
taskcreate()	{ printf("taskcreate\n"); }
#endif

/*
 *  Some initial lighting model stuff
 *  Really, MGED needs to derrive it's lighting from the database,
 *  but for now, this hack will suffice.
 *
 *  For materials, the definitions are:
 *	ALPHA		opacity.  1.0=opaque
 *	AMBIENT		ambient reflectance of the material  0..1
 *	DIFFUSE		diffuse reflectance of the material  0..1
 *	SPECULAR	specular reflectance of the material  0..1
 *	EMISSION	emission color ???
 *	SHININESS	specular scattering exponent, integer 0..128
 */
static float material_default[] = {
			ALPHA,		1.0,
			AMBIENT,	0.2, 0.2, 0.2,
			DIFFUSE,	0.8, 0.8, 0.8,
			EMISSION,	0.0, 0.0, 0.0,
			SHININESS,	0.0,
			SPECULAR,	0.0, 0.0, 0.0,
			LMNULL   };

/* Something like the RT default phong material */
static float material_rtdefault[] = {
			ALPHA,		1.0,	
			AMBIENT,	0.2, 0.2, 0.2,	/* 0.4 in rt */
			DIFFUSE,	0.6, 0.6, 0.6,
			SPECULAR,	0.2, 0.2, 0.2,
			EMISSION,	0.0, 0.0, 0.0,
			SHININESS,	10.0,
			LMNULL   };

/* This was the "default" material in the demo */
static float material_xdefault[] = {AMBIENT, 0.35, 0.25,  0.1,
			DIFFUSE, 0.1, 0.5, 0.1,
			SPECULAR, 0.0, 0.0, 0.0,
			SHININESS, 5.0,
			LMNULL   };

static float mat_brass[] = {AMBIENT, 0.35, 0.25,  0.1,
			DIFFUSE, 0.65, 0.5, 0.35,
			SPECULAR, 0.0, 0.0, 0.0,
			SHININESS, 5.0,
			LMNULL   };

static float mat_shinybrass[] = {AMBIENT, 0.25, 0.15, 0.0,
			DIFFUSE, 0.65, 0.5, 0.35,
			SPECULAR, 0.9, 0.6, 0.0,
			SHININESS, 10.0,
			LMNULL   };

static float mat_pewter[] = {AMBIENT, 0.0, 0.0,  0.0,
			DIFFUSE, 0.6, 0.55 , 0.65,
			SPECULAR, 0.9, 0.9, 0.95,
			SHININESS, 10.0,
			LMNULL   };

static float mat_silver[] = {AMBIENT, 0.4, 0.4,  0.4,
			DIFFUSE, 0.3, 0.3, 0.3,
			SPECULAR, 0.9, 0.9, 0.95,
			SHININESS, 30.0,
			LMNULL   };

static float mat_gold[] = {AMBIENT, 0.4, 0.2, 0.0,
			DIFFUSE, 0.9, 0.5, 0.0,
			SPECULAR, 0.7, 0.7, 0.0,
			SHININESS, 10.0,
			LMNULL   };

static float mat_shinygold[] = {AMBIENT, 0.4, 0.2,  0.0,
			DIFFUSE, 0.9, 0.5, 0.0,
			SPECULAR, 0.9, 0.9, 0.0,
			SHININESS, 20.0,
			LMNULL   };

static float mat_plaster[] = {AMBIENT, 0.2, 0.2,  0.2,
			DIFFUSE, 0.95, 0.95, 0.95,
			SPECULAR, 0.0, 0.0, 0.0,
			SHININESS, 1.0,
			LMNULL   };

static float mat_redplastic[] = {AMBIENT, 0.3, 0.1, 0.1,
			DIFFUSE, 0.5, 0.1, 0.1,
			SPECULAR, 0.45, 0.45, 0.45,
			SHININESS, 30.0,
			LMNULL   };

static float mat_greenplastic[] = {AMBIENT, 0.1, 0.3, 0.1,
			DIFFUSE, 0.1, 0.5, 0.1,
			SPECULAR, 0.45, 0.45, 0.45,
			SHININESS, 30.0,
			LMNULL   };

static float mat_blueplastic[] = {AMBIENT, 0.1, 0.1, 0.3,
			DIFFUSE, 0.1, 0.1, 0.5,
			SPECULAR, 0.45, 0.45, 0.45,
			SHININESS, 30.0,
			LMNULL   };

static float mat_greenflat[] = {
			EMISSION,   0.0, 0.4, 0.0,
			AMBIENT,    0.0, 0.0, 0.0,
			DIFFUSE,    0.0, 0.0, 0.0,
			SPECULAR,   0.0, 0.6, 0.0,
			SHININESS, 10.0,
			LMNULL
			};

static float mat_greenshiny[]= {
			EMISSION, 0.0, 0.4, 0.0,
			AMBIENT,  0.1, 0.25, 0.1,
			DIFFUSE,  0.5, 0.5, 0.5,
			SPECULAR,  0.25, 0.9, 0.25,
			SHININESS, 10.0,
			LMNULL
			};

static float mat_blueflat[] = {
		       EMISSION, 0.0, 0.0, 0.4,
		       AMBIENT,  0.1, 0.25, 0.1,
		       DIFFUSE,  0.0, 0.5, 0.5,
		       SPECULAR,  0.0, 0.0, 0.9,
		       SHININESS, 10.0,
		       LMNULL
		       };

static float mat_blueshiny[] = {
			EMISSION, 0.0, 0.0, 0.6,
			AMBIENT,  0.1, 0.25, 0.5,
			DIFFUSE,  0.5, 0.5, 0.5,
			SPECULAR,  0.5, 0.0, 0.0,
			SHININESS, 10.0,
			LMNULL
			};

static float mat_redflat[] = {
			EMISSION, 0.60, 0.0, 0.0,
			AMBIENT,  0.1, 0.25, 0.1,
			DIFFUSE,  0.5, 0.5, 0.5,
			SPECULAR,  0.5, 0.0, 0.0,
			SHININESS, 1.0,
			LMNULL
			};

static float mat_redshiny[] = {
			EMISSION, 0.60, 0.0, 0.0,
			AMBIENT,  0.1, 0.25, 0.1,
			DIFFUSE,  0.5, 0.5, 0.5,
			SPECULAR,  0.5, 0.0, 0.0,
			SHININESS, 10.0,
			LMNULL
			};

static float mat_beigeshiny[] = {
			EMISSION, 0.5, 0.5, 0.6,
			AMBIENT,  0.35, 0.35, 0.0,
			DIFFUSE,  0.5, 0.5, 0.0,
			SPECULAR,  0.5, 0.5, 0.0,
			SHININESS, 10.0,
			LMNULL
			};

/*
 *  Meanings of the parameters:
 *	AMBIENT		ambient light associated with this source ???, 0..1
 *	LCOLOR		light color, 0..1
 *	POSITION	position of light.  w=0 for infinite lights
 */
static float default_light[] = {
			AMBIENT,	0.0, 0.0, 0.0, 
			LCOLOR,		1.0, 1.0, 1.0, 
			POSITION,	0.0, 0.0, 1.0, 0.0,
			LMNULL};
		    
static float white_inf_light[] = {AMBIENT, 0.0, 0.0, 0.0, 
			   LCOLOR,   0.70, 0.70, 0.70, 
			   POSITION, 10.0, 50.0, 50.0, 0.0, 
			   LMNULL};

static float red_inf_light[] = {AMBIENT, 0.0, 0.0, 0.0, 
			   LCOLOR,   0.5, 0.1, 0.1, 
			   POSITION, -100.0, 50.0, 0.0, 0.0, 
			   LMNULL};

static float blue_inf_light[] = {AMBIENT, 0.0, 0.0, 0.0, 
			   LCOLOR,   0.1, 0.1, 0.5, 
			   POSITION, 0.0, 50.0, 0.0, 0.0, 
			   LMNULL};

static float green_inf_light[] = {AMBIENT, 0.0, 0.0, 0.0, 
			   LCOLOR,   0.1, 0.5, 0.1, 
			   POSITION, 100.0, 50.0, 0.0, 0.0, 
			   LMNULL};

static float orange_inf_light[] = {AMBIENT, 0.0, 0.0, 0.0, 
			    LCOLOR,	0.35, 0.175, 0.0, 
			    POSITION, -50.0, 50.0, 10.0, 0.0, 
			    LMNULL};

static float white_local_light[] = {AMBIENT, 0.0, 0.0, 0.0, 
			     LCOLOR,   0.75, 0.75, 0.75, 
			     POSITION, 0.0, 10.0, 10.0, 5.0, 
			     LMNULL};
			   
/*
 *  Lighting model parameters
 *	AMBIENT		amount of ambient light present in the scene, 0..1
 *	ATTENUATION	fixed and variable attenuation factor, 0..1
 *	LOCALVIEWER	1=eye at (0,0,0), 0=eye at (0,0,+inf)
 */
static float	default_lmodel[] = {
			AMBIENT,	0.2,  0.2,  0.2,
			ATTENUATION,	1.0, 0.0, 
			LOCALVIEWER,	0.0, 
			LMNULL};

static float infinite[] = {AMBIENT, 0.3,  0.3, 0.3, 
	            LOCALVIEWER, 0.0, 
	            LMNULL};

static float local[] = {AMBIENT, 0.3,  0.3, 0.3, 
	         LOCALVIEWER, 1.0, 
	         ATTENUATION, 1.0, 0.0, 
	         LMNULL};


make_materials()
{
    /* define material properties */
    lmdef (DEFMATERIAL, 1, 0, material_default);

    lmdef (DEFMATERIAL, 2, 0, mat_brass);
    lmdef (DEFMATERIAL, 3, 0, mat_shinybrass);
    lmdef (DEFMATERIAL, 4, 0, mat_pewter);
    lmdef (DEFMATERIAL, 5, 0, mat_silver);
    lmdef (DEFMATERIAL, 6, 0, mat_gold);
    lmdef (DEFMATERIAL, 7, 0, mat_shinygold);
    lmdef (DEFMATERIAL, 8, 0, mat_plaster);
    lmdef (DEFMATERIAL, 9, 0, mat_redplastic);
    lmdef (DEFMATERIAL, 10, 0, mat_greenplastic);
    lmdef (DEFMATERIAL, 11, 0, mat_blueplastic);

    lmdef (DEFMATERIAL, 12, 0, mat_greenflat);
    lmdef (DEFMATERIAL, 13, 0, mat_greenshiny);

    lmdef (DEFMATERIAL, 14, 0, mat_blueflat);
    lmdef (DEFMATERIAL, 15, 0, mat_blueshiny);

    lmdef (DEFMATERIAL, 16, 0, mat_redflat);
    lmdef (DEFMATERIAL, 17, 0, mat_redshiny);

    lmdef (DEFMATERIAL, 18, 0, mat_beigeshiny);

	lmdef( DEFMATERIAL, 19, 0, material_xdefault );
	lmdef( DEFMATERIAL, 20, 0, material_rtdefault );

    lmdef (DEFLIGHT, 1, 0, default_light);
    lmdef (DEFLIGHT, 2, 0, white_inf_light);
    lmdef (DEFLIGHT, 3, 0, red_inf_light);
    lmdef (DEFLIGHT, 4, 0, green_inf_light);
    lmdef (DEFLIGHT, 5, 0, blue_inf_light);
    lmdef (DEFLIGHT, 6, 0, orange_inf_light);
    lmdef (DEFLIGHT, 7, 0, white_local_light);

    lmdef (DEFLMODEL, 1, 0, default_lmodel);
    lmdef (DEFLMODEL, 2, 0, infinite);
    lmdef (DEFLMODEL, 3, 0, local);


}
