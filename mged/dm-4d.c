/*
 *			D M - I R I S . C
 *
 *  Uses library -lgl2
 *
 *  NON-displaylist version.
 *
 *  MGED display manager for the IRIS.
 *  Based on dm-mer.c
 *
 *  Display structure -
 *	ROOT LIST = { wrld }
 *	wrld = { face, Mvie, Medi }
 *	face = { faceplate stuff, 2d lines and text }
 *	Mvie = { model2view matrix, view }
 *	view = { (all non-edit solids) }
 *	Medi = { model2objview matrix, edit }
 *	edit = { (all edit solids) }
 *
 *  Authors -
 *	Michael John Muuss
 *	Paul R. Stay
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1985 by the United States Army.
 *	All rights reserved.
 *
 *  Now with BUTTONS and KNOBS! - Ron Natalie, BRL,  3 Oct, 1986
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <termio.h>
#undef VMIN		/* is used in vmath.h, too */
#include <ctype.h>

#include "../h/machine.h"	/* special copy */
#include "vmath.h"
#include "mater.h"
#include "./ged.h"
#include "./dm.h"
#include "./solid.h"

#include <gl/gl.h>		/* SGI IRIS library */
#include <gl/device.h>		/* SGI IRIS library */
#include <gl/immed.h>

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


#define	IR_BUTTONS	32
#define	IR_KNOBS	8


static char ir_title[] = "BRL MGED";
static int big_txt = 0;		/* Text window size flag - hack for Ir_puts */
static int cueing_on = 1;	/* Depth cueing flag - for colormap work */
static int zclipping_on = 1;	/* Z Clipping flag */
static int perspective_on = 0;	/* Perspective flag */
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
	IRBOUND,
	"sgi", "SGI 4d"
};

extern struct device_values dm_values;	/* values read from devices */

static int	ir_debug;		/* 2 for basic, 3 for full */
static int	ir_buffer;

long gr_id;
long win_l, win_b, win_r, win_t;
long winx_size, winy_size;

/* Map +/-2048 GED space into -1.0..+1.0 :: x/2048*/
#define GED2IRIS(x)	(((float)(x))*0.00048828125)
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

#define map_entry(x)	((cueing_on) ? ((x) * 16) : ((x) + 16))


/*
 *			I R _ O P E N
 *
 * Fire up the display manager, and the display processor.
 *
 */
int
Ir_open()
{
	register int i;
	Matrix	m;

	foreground();
	prefposition( 0, 10, 0, 10);
	winopen("dummy");
	prefposition( 276, 1276, 12, 1012 );
	if( (gr_id = winopen( "BRL MGED" )) == -1 )  {
		printf( "No more graphics ports available.\n" );
		return	-1;
	}

	/* setupt the global variables for the window. */
	getsize( &winx_size, &winy_size);
	getorigin( &win_l, & win_b );

	win_r = win_l + winx_size;
	win_t = win_b + winy_size;

	onemap();			/* one color map */
	doublebuffer();			/* half of whatever we have */
	gconfig();

	winattach( );

	color(BLACK);
	frontbuffer(1);
	clear();
	frontbuffer(0);
	
	ir_buffer = 0;

/* 	swapinterval( 5 );*/

	qdevice(LEFTMOUSE);
	qdevice(MIDDLEMOUSE);
	qdevice(RIGHTMOUSE);
	tie(MIDDLEMOUSE, MOUSEX, MOUSEY);

#if IR_BUTTONS
	ir_dbtext("dm_ir");
	/*
	 *  Enable all the buttons in the button table.
	 */
	for(i = 0; i < IR_BUTTONS; i++)
		qdevice(i+SWBASE);

	/*
	 *  For all possible button presses, build a table
	 *  of MGED function to SGI button/light mappings.
	 */
	for( i=0; i < IR_BUTTONS; i++ )  {
		register int j;
		if( (j = bmap[i]) != 0 )
			invbmap[j] = i;
	}

	/* Enable the pf1 key for depthcue switching */
	qdevice(F1KEY);
	qdevice(F2KEY);	/* pf2 for Z clipping */
	qdevice(F3KEY);	/* pf3 for perspective */
#endif
#if IR_KNOBS
	/*
	 *  Turn on the dials and initialize them for -2048 to 2047
	 *  range with a dead spot at zero (Iris knobs are 1024 units
	 *  per rotation).
	 */
	for(i = DIAL0; i <= DIAL8; i++) {
		qdevice(i);
		setvaluator(i, 0, -2048-NOISE, 2047+NOISE);
	}
	ir_dbtext(ir_title);
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

	return(0);
}

/*
 *  			I R _ C L O S E
 *  
 *  Gracefully release the display.
 */
void
Ir_close()
{
	if(cueing_on) depthcue(0);

	lampoff( 0xf );

	color(BLACK);
	clear();
	frontbuffer(1);
	clear();

	greset();
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
	color(BLACK);
	ortho2( -1.0,1.0, -1.0,1.0);	/* L R Bot Top */
	kblights();
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

	gflush();
	swapbuffers();

	ir_buffer = (ir_buffer==0)?1:0;

	/* give Graphics pipe time to work */
	color(BLACK);
	clear();
}

/*
 *  			I R _ N E W R O T
 *  Stub.
 */
void
Ir_newrot(mat)
mat_t mat;
{
	im_setup;
	
}

/*
 *  			I R _ O B J E C T
 *  
 *  Set up for an object, transformed as indicated, and with an
 *  object center as specified.  The ratio of object to screen size
 *  is passed in as a convienience.  Mat is model2view.
 *
 *  Returns 0 if object could be drawn, !0 if object was omitted.
 */
int
Ir_object( sp, m, ratio, white )
register struct solid *sp;
register fastf_t *m;
double ratio;
{
	register struct vlist *vp;
	im_setup;
	register int nvec;
	mat_t	mtmp, newm;

	/* This section has the potential of being speed up since a new
	 * matrix is loaded for each object. Even though its the same
	 * matrix.
	 */

	if( perspective_on ) {
		mat_mul( newm, perspect_mat, m );
		m = newm;
	} else if( ! zclipping_on ) {
		mat_mul( newm, nozclip_mat, m );
		m = newm;
	}

	im_dirty_matrixstack;
	GEWAIT;
	
	im_outshort(GEloadmm);

	im_outfloat( *m++);
	im_outfloat( *m++);
	im_outfloat( *m++);
	im_outfloat( *m++);
	GEWAIT;
	im_outfloat( *m++);
	im_outfloat( *m++);
	im_outfloat( *m++);
	im_outfloat( *m++);
	GEWAIT;
	im_outfloat( *m++);
	im_outfloat( *m++);
	im_outfloat( *m++);
	im_outfloat( *m++);
	GEWAIT;
	im_outfloat( *m++);
	im_outfloat( *m++);
	im_outfloat( *m++);
	im_last_outfloat( *m++);

	/*
	 * IMPORTANT DEPTHCUEING NOTE:  The IRIS screen +Z points in
	 * (i.e. increasing Z is further away).  In order to reconcile
	 * this with mged (which expects +Z to point out), the color
	 * ramps will be flipped to compensate.  This seemed to be the
	 * lessor of the evils at the time.
	 * Also note that the depthcueing shaderange() routine wanders
	 * outside it's allotted range due to roundoff errors.  A buffer
	 * entry is kept on each end of the shading curves, and the
	 * highlight mode uses the *next* to the brightest entry --
	 * otherwise it can (and does) fall off the shading ramp.
	 */
	
	GEWAIT;
	im_passcmd(2, FBClinestyle);
	im_last_outshort( 0xFFFF );

	if( white || sp->s_materp == (char *)0 ) {
		ovec = nvec = map_entry(DM_WHITE);
		/* Use the *next* to the brightest white entry */
		if(cueing_on) shaderange(nvec+1, nvec+1, 0, 768);
	} else {
		if((nvec =
		  map_entry(((struct mater *)sp->s_materp)->mt_dm_int))
		  != ovec) {
			/* Use only the middle 14 to allow for roundoff...
			 * Pity the poor fool who has defined a black object.
			 * The code will use the "reserved" color map entries
			 * to display it when in depthcued mode.
			 */
		  	if(cueing_on) shaderange(nvec+1, nvec+14, 0, 768);
		  	ovec = nvec;
		  }
	}

	im_color( nvec );

	for( vp = sp->s_vlist; vp != VL_NULL; vp = vp->vl_forw )  {
		/* Viewing region is from -1.0 to +1.0 */
		if( vp->vl_draw == 0 )  {
			/* Move, not draw */
			im_outshort( GEmove | GEPA_3F );
		}  else  {
			/* draw */
			im_outshort( GEdraw | GEPA_3F );
		}
		im_out3F( vp->vl_pnt[X], vp->vl_pnt[Y], vp->vl_pnt[Z] );

	}

	GEWAIT;

	im_passcmd(2, FBClinestyle);
	im_last_outshort( 0xFFFF );

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
	gflush();		/* Flush any pending output */
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
{
	im_setup;

	/*
	 * HACK ALERT!  The following is a quick first attempt at turning
	 * off all text except vertices in the small graphics window.
	 */
	if( (big_txt == 0) || ((strlen(str) == 1) && (colour == DM_WHITE))) {
		im_cmov2( GED2IRIS(x), GED2IRIS(y));

		im_color( map_entry(colour) );

		charstr( str );
	}
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
	im_setup;
	register int nvec;
	if((nvec = map_entry(DM_YELLOW)) != ovec) {
	  	if(cueing_on) shaderange(nvec, nvec, 0, 768);
	  	ovec = nvec;
	}
	
	im_color( nvec );

	if( dashed )  {
		GEWAIT;
		im_passcmd(2, FBClinestyle);
		im_last_outshort( 0xFFFF );
	}
	im_move2( GED2IRIS(x1), GED2IRIS(y1));
	im_draw2( GED2IRIS(x2), GED2IRIS(y2));

	if( dashed )  {
		GEWAIT;
		im_passcmd(2, FBClinestyle);
		im_last_outshort( 0xFFFF );
	}
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

	n = blkqread( values, NVAL );	/* n is # of shorts returned */
	if( ir_debug ) printf("blkqread gave %d\n", n);

	for( valp = values; n > 0; n -= 2, valp += 2 )  {
		ret = *valp;
		if( ir_debug ) printf("qread ret=%d, val=%d\n", ret, valp[1]);

#if IR_BUTTONS
		if((ret >= SWBASE && ret < SWBASE+IR_BUTTONS)
		  || ret == F1KEY || ret == F2KEY || ret == F3KEY ) {
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
				else
					ir_dbtext(ir_title);
				continue;
				
			}
#if IR_KNOBS
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
				/* zap the knobs */
				for(i = 0; i < 8; i++)  {
					setvaluator(DIAL0+i, 0,
					  -2048-NOISE, 2047+NOISE);
					knobs[i] = 0;
				}
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
				kblights();
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
				kblights();
				continue;
			} else if(ret == F3KEY)  {
				if(!valp[1]) continue; /* Ignore release */
				/*  Help mode */
				if(button0)  {
					ir_dbtext("Perspect");
					continue;
				}
				/* toggle perspective viewing */
				perspective_on = perspective_on ? 0 : 1;
				dmaflag = 1;
				kblights();
				continue;
			}
			/*
			 * If button being depressed (as opposed to
			 * being released) either display the cute
			 * message or process the button depending
			 * on whether button0 is also being held down.
			 */
			i = bmap[ret - SWBASE];
			if(valp[1])
				if(button0 && valp[1])
					 ir_dbtext(label_button(i));
			else
				button(i);
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
		switch( ret )  {
		case LEFTMOUSE:
			if( valp[1] && dm_values.dv_penpress != DV_PICK )
				dm_values.dv_penpress = DV_OUTZOOM;
			break;
		case MIDDLEMOUSE:
			/* Will also get MOUSEX and MOUSEY hits */
			if( valp[1] ) {
				dm_values.dv_penpress = DV_PICK;
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
			break;
		case REDRAW:
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
	setdblights( lights );
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
		unqdevice( MOUSEY );	/* constant tracking OFF */
		break;
		
	case ST_S_PICK:
	case ST_O_PICK:
	case ST_O_PATH:
		qdevice( MOUSEY );	/* constant tracking ON */
		break;
	case ST_O_EDIT:
	case ST_S_EDIT:
		unqdevice( MOUSEY );	/* constant tracking OFF */
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
 * Color Map table
 */
#define NSLOTS		4080	/* The mostest possible - may be fewer */
static int ir_nslots=0;		/* how many we have, <= NSLOTS */
static int slotsused;		/* how many actually used */
static struct rgbtab {
	unsigned char	r;
	unsigned char	g;
	unsigned char	b;
} ir_rgbtab[NSLOTS];

/*
 *  			I R _ C O L O R C H A N G E
 *  
 *  Go through the mater table, and allocate color map slots.
 *	8 bit system gives 4 or 8,
 *	24 bit system gives 12 or 24.
 */
void
Ir_colorchange()
{
	register struct mater *mp;
	register int i;
	register int nramp;

	ir_nslots = getplanes();
	if(cueing_on && (ir_nslots < 7)) {
		printf("Too few bitplanes: depthcueing disabled\n");
		cueing_on = 0;
	}
	ir_nslots = 1<<ir_nslots;
	if(cueing_on) {
		ir_nslots = ir_nslots / 16 - 1;	/* peel off reserved ones */
		depthcue(1);
	} else {
		ir_nslots -= 16;	/* peel off the reserved entries */
		depthcue(0);
	}
	if( ir_nslots > NSLOTS )  ir_nslots = NSLOTS;

	if( ir_debug )  printf("colorchange\n");
	ovec = -1;	/* Invalidate the old colormap entry */
	/* Program the builtin colors */
	ir_rgbtab[0].r=0; ir_rgbtab[0].g=0; ir_rgbtab[0].b=0;/* Black */
	ir_rgbtab[1].r=255; ir_rgbtab[1].g=0; ir_rgbtab[1].b=0;/* Red */
	ir_rgbtab[2].r=0; ir_rgbtab[2].g=0; ir_rgbtab[2].b=255;/* Blue */
	ir_rgbtab[3].r=255; ir_rgbtab[3].g=255;ir_rgbtab[3].b=0;/*Yellow */
	ir_rgbtab[4].r = ir_rgbtab[4].g = ir_rgbtab[4].b = 255; /* White */

	slotsused = 5;
	for( mp = MaterHead; mp != MATER_NULL; mp = mp->mt_forw )
		ir_colorit( mp );

	color_soltab();		/* apply colors to the solid table */

	for( i=0; i < slotsused; i++ )  {
		gen_color( i, ir_rgbtab[i].r, ir_rgbtab[i].g, ir_rgbtab[i].b);
	}

	/* Re-send the tree, with the new colors attached */
	Ir_viewchange( DM_CHGV_REDO, SOLID_NULL );
	color(WHITE);	/* undefinied after gconfig() */
}

int
ir_colorit( mp )
struct mater *mp;
{
	register struct rgbtab *rgb;
	register int i;
	register int r,g,b;

	r = mp->mt_r;
	g = mp->mt_g;
	b = mp->mt_b;
	if( (r == 255 && g == 255 && b == 255) ||
	    (r == 0 && g == 0 && b == 0) )  {
		mp->mt_dm_int = DM_WHITE;
		return;
	}

	/* First, see if this matches an existing color map entry */
	rgb = ir_rgbtab;
	for( i = 0; i < slotsused; i++, rgb++ )  {
		if( rgb->r == r && rgb->g == g && rgb->b == b )  {
			 mp->mt_dm_int = i;
			 return;
		}
	}

	/* If slots left, create a new color map entry, first-come basis */
	if( slotsused < ir_nslots )  {
		rgb = &ir_rgbtab[i=(slotsused++)];
		rgb->r = r;
		rgb->g = g;
		rgb->b = b;
		mp->mt_dm_int = i;
		return;
	}
	mp->mt_dm_int = DM_YELLOW;	/* Default color */
}

/*
 *  I R _ D B T E X T
 *
 *  Call dbtext to print cute messages on the button box,
 *  if you have one.  Has to shift everythign to upper case
 *  since the box goes off the deep end with lower case.
 */
 
ir_dbtext(str)
	register char *str;
{
#if IR_BUTTONS
  	register i;
	char	buf[9];
	register char *cp;

	for(i = 0, cp = buf; i < 8 && *str; i++, cp++, str++)
		*cp = islower(*str) ?  toupper(*str) : *str;
	*cp = 0;
	dbtext(buf);
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
 *	maps the mater color entries into the appropriate colormap entry
 *	for both depthcued and non-depthcued mode.  In depthcued mode,
 *	gen_color also generates the colormap ramps.  Note that in depthcued
 *	mode, DM_BLACK uses map entry 0, and does not generate a ramp for it.
 *	Non depthcued mode skips the first 16 colormap entries.  Also note
 * 	that the colormap ramps are generated from bright to dim.
 */
gen_color(c)
int c;
{
	if(cueing_on) {
		
		/*  Not much sense in making a ramp for DM_BLACK.  Besides
		 *  which, doing so, would overwrite the bottom 16 color
		 *  map entries, which is a no, no.
		 */
		if( c != DM_BLACK) {
			register int i;
			fastf_t r_inc, g_inc, b_inc;
			fastf_t red, green, blue;

			r_inc = ir_rgbtab[c].r/16;
			g_inc = ir_rgbtab[c].g/16;
			b_inc = ir_rgbtab[c].b/16;

			for(i = 0, red = ir_rgbtab[c].r,
			  green = ir_rgbtab[c].g, blue = ir_rgbtab[c].b;
			  i < 16;
			  i++, red -= r_inc, green -= g_inc, blue -= b_inc)
				mapcolor(c * 16 + i, (short)red, (short)green,
				  (short)blue);
		}
	} else {
		mapcolor(c+16, ir_rgbtab[c].r, ir_rgbtab[c].g, ir_rgbtab[c].b);
	}
}

/*
 *  Update the PF key lights.
 */
static int
kblights()
{
	char	lights;

#ifdef never
	lights = (cueing_on)
		| (zclipping_on << 1)
		| (perspective_on << 2);

	lampon(lights);
	lampoff(lights^0xf);
#endif
}

/*
 *  Compute a perspective matrix.
 *  Reference: SGI Graphics Reference Appendix C
 */
#include <math.h>
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
