/*
 *			D M - O G L . C
 *
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

#include "conf.h"

#include <stdio.h>
#include <math.h>
#include <termio.h>
#undef VMIN		/* is used in vmath.h, too */
#include <ctype.h>

#include <sys/types.h>
#include <sys/time.h>

#include <X11/X.h>
#ifdef HAVE_XOSDEFS_H
#include <X11/Xfuncproto.h>
#include <X11/Xosdefs.h>
#endif
#if defined(linux)
#	undef   X_NOT_STDC_ENV
#	undef   X_NOT_POSIX
#endif
#define XLIB_ILLEGAL_ACCESS	/* necessary on facist SGI 5.0.1 */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "tcl.h"
#include "tk.h"
#include <X11/extensions/XI.h>
#include <X11/extensions/XInput.h>

#include <GL/glx.h>
#include <GL/gl.h>
#include <gl/device.h>

#include "machine.h"
#include "externs.h"
#include "bu.h"
#include "vmath.h"
#include "mater.h"
#include "raytrace.h"
#include "dm.h"
#include "dm-ogl.h"
#include "solid.h"

#define VIEWFACTOR      (1/(*dmp->dmr_vp))
#define VIEWSIZE        (2*(*dmp->dmr_vp))

/* these are from /usr/include/gl.h could be device dependent */
#define XMAXSCREEN	1279
#define YMAXSCREEN	1023
#define YSTEREO		491	/* subfield height, in scanlines */
#define YOFFSET_LEFT	532	/* YSTEREO + YBLANK ? */

extern Tk_Window tkwin;
void	Ogl_configure_window_shape();
void    Ogl_establish_perspective();
void    Ogl_set_perspective();
void	Ogl_establish_lighting();
void	Ogl_establish_zbuffer();
int     Ogl_irisX2ged();
int     Ogl_irisY2ged();
#ifdef IR_KNOBS
int     Ogl_irlimit();			/* provides knob dead spot */
int     Ogl_add_tol();
#endif

static int	Ogl_setup();
static void	Ogl_gen_color();
static void     Ogl_colorit();
static void     Ogl_load_startup();
static XVisualInfo *Ogl_set_visual();
static void     print_cmap();

/* Flags indicating whether the ogl and sgi display managers have been
 * attached.
 * These are necessary to decide whether or not to use direct rendering
 * with gl.
 */
char  ogl_ogl_used = 0;
char  ogl_sgi_used = 0;

/* Display Manager package interface */
#define IRBOUND	4095.9	/* Max magnification in Rot matrix */

#define PLOTBOUND	1000.0	/* Max magnification in Rot matrix */
static int     Ogl_init();
static int	Ogl_open();
static void	Ogl_close();
static void     Ogl_input();
static void	Ogl_prolog(), Ogl_epilog();
static void	Ogl_normal(), Ogl_newrot();
static void	Ogl_update();
static void	Ogl_puts(), Ogl_2d_line(), Ogl_light();
static int	Ogl_object();
static unsigned Ogl_cvtvecs(), Ogl_load();
static void	Ogl_viewchange(), Ogl_colorchange();
static void	Ogl_window(), Ogl_debug(), Ogl_selectargs();

struct dm dm_ogl = {
  Ogl_init,
  Ogl_open, Ogl_close,
  Ogl_input,
  Ogl_prolog, Ogl_epilog,
  Ogl_normal, Ogl_newrot,
  Ogl_update,
  Ogl_puts, Ogl_2d_line,
  Ogl_light,
  Ogl_object, Ogl_cvtvecs, Ogl_load,
  0,
  Ogl_viewchange,
  Ogl_colorchange,
  Ogl_window, Ogl_debug, 0, 0,
  0,				/* no displaylist */
  0,				/* multi-window */
  IRBOUND,
  "ogl", "X Windows with OpenGL graphics",
  0,				/* mem map */
  0,
  0,
  0,
  0,
  0,
  0
};


/* ogl stuff */
struct ogl_vars head_ogl_vars;
int perspective_table[] = {
	30, 45, 60, 90 };

static fastf_t default_viewscale = 1000.0;
static double	xlim_view = 1.0;	/* args for glOrtho*/
static double	ylim_view = 1.0;

/* lighting parameters */
static float amb_three[] = {0.3, 0.3, 0.3, 1.0};

static float light0_position[] = {100.0, 200.0, 100.0, 0.0};
static float light1_position[] = {100.0, 30.0, 100.0, 0.0};
static float light2_position[] = {-100.0, 20.0, 20.0, 0.0};
static float light3_position[] = {0.0, -100.0, -100.0, 0.0};

static float light0_diffuse[] = {0.70, 0.70, 0.70, 1.0}; /* white */
static float light1_diffuse[] = {0.60, 0.10, 0.10, 1.0}; /* red */
static float light2_diffuse[] = {0.10, 0.30, 0.10, 1.0}; /* green */
static float light3_diffuse[] = {0.10, 0.10, 0.30, 1.0}; /* blue */

/* functions */
void
Ogl_do_linewidth(dmp)
struct dm *dmp;
{
	glLineWidth((GLfloat) ((struct ogl_vars *)dmp->dmr_vars)->mvars.linewidth);
#if 0
	dmaflag = 1;
#endif
}


void
Ogl_do_fog(dmp)
struct dm *dmp;
{
	glHint(GL_FOG_HINT, ((struct ogl_vars *)dmp->dmr_vars)->mvars.fastfog ? GL_FASTEST : GL_NICEST);
#if 0
	dmaflag = 1;
#endif
}

/* get rid of when no longer needed */
#define USE_RAMP	(((struct ogl_vars *)dmp->dmr_vars)->mvars.cueing_on || ((struct ogl_vars *)dmp->dmr_vars)->mvars.lighting_on)
#define CMAP_BASE	32
#define CMAP_RAMP_WIDTH	16
#define MAP_ENTRY(x)	( USE_RAMP ? \
			((x) * CMAP_RAMP_WIDTH + CMAP_BASE) : \
			((x) + CMAP_BASE) )

/********************************************************************/

/*
 *  Mouse coordinates are in absolute screen space, not relative to
 *  the window they came from.  Convert to window-relative,
 *  then to MGED-style +/-2048 range.
 */
int
Ogl_irisX2ged(dmp, x)
struct dm *dmp;
register int x;
{
  return ((x/(double)((struct ogl_vars *)dmp->dmr_vars)->width - 0.5) * 4095);
}

int
Ogl_irisY2ged(dmp, y)
struct dm *dmp;
register int y;
{
  return ((0.5 - y/(double)((struct ogl_vars *)dmp->dmr_vars)->height) * 4095);
}

static int
Ogl_init(dmp, argc, argv)
struct dm *dmp;
int argc;
char *argv[];
{
  dmp->dmr_vars = bu_calloc(1, sizeof(struct ogl_vars), "Ogl_init: ogl_vars");
  ((struct ogl_vars *)dmp->dmr_vars)->devmotionnotify = LASTEvent;
  ((struct ogl_vars *)dmp->dmr_vars)->devbuttonpress = LASTEvent;
  ((struct ogl_vars *)dmp->dmr_vars)->devbuttonrelease = LASTEvent;
  ((struct ogl_vars *)dmp->dmr_vars)->perspective_angle = 3;
  ((struct ogl_vars *)dmp->dmr_vars)->aspect = 1.0;
  ((struct ogl_vars *)dmp->dmr_vars)->ovec = -1;

  /* initialize the modifiable variables */
  ((struct ogl_vars *)dmp->dmr_vars)->mvars.cueing_on = 1; /* Depth cueing flag - for colormap work */
  ((struct ogl_vars *)dmp->dmr_vars)->mvars.zclipping_on = 1; /* Z Clipping flag */
  ((struct ogl_vars *)dmp->dmr_vars)->mvars.zbuffer_on = 1; /* Hardware Z buffer is on */
  ((struct ogl_vars *)dmp->dmr_vars)->mvars.linewidth = 1; /* Line drawing width */
  ((struct ogl_vars *)dmp->dmr_vars)->mvars.dummy_perspective = 1;
  ((struct ogl_vars *)dmp->dmr_vars)->mvars.fastfog = 1;
  ((struct ogl_vars *)dmp->dmr_vars)->mvars.fogdensity = 1.0;

  if(dmp->dmr_vars)
    return TCL_OK;

  return TCL_ERROR;
}

/*
 *			O G L _ O P E N
 *
 * Fire up the display manager, and the display processor.
 *
 */
static int
Ogl_open(dmp)
struct dm *dmp;
{
  return Ogl_setup(dmp);
}

/*XXX Just experimenting */
static void
Ogl_load_startup(dmp)
struct dm *dmp;
{
  char *filename;

  bzero((void *)&head_ogl_vars, sizeof(struct ogl_vars));
  BU_LIST_INIT( &head_ogl_vars.l );

  if((filename = getenv("DM_OGL_RCFILE")) != (char *)NULL )
    Tcl_EvalFile(interp, filename);
}

/*
 *  			O G L _ C L O S E
 *  
 *  Gracefully release the display.
 */
static void
Ogl_close(dmp)
struct dm *dmp;
{
  if(((struct ogl_vars *)dmp->dmr_vars)->glxc != NULL){
#if 0
    glDrawBuffer(GL_FRONT);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    /*	glClearDepth(0.0);*/
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawBuffer(GL_BACK);
#endif
    glXDestroyContext(((struct ogl_vars *)dmp->dmr_vars)->dpy,
		      ((struct ogl_vars *)dmp->dmr_vars)->glxc);
  }

  if(((struct ogl_vars *)dmp->dmr_vars)->xtkwin != NULL)
    Tk_DestroyWindow(((struct ogl_vars *)dmp->dmr_vars)->xtkwin);

#if 0
  if(dmp->dmr_app_close)
    dmp->dmr_app_close(((struct ogl_vars *)dmp->dmr_vars)->app_vars);
#endif

  if(((struct ogl_vars *)dmp->dmr_vars)->l.forw != BU_LIST_NULL)
    BU_LIST_DEQUEUE(&((struct ogl_vars *)dmp->dmr_vars)->l);

  bu_free(dmp->dmr_vars, "Ogl_close: ogl_vars");

  if(BU_LIST_IS_EMPTY(&head_ogl_vars.l))
    Tk_DeleteGenericHandler(dmp->dmr_eventhandler, (ClientData)NULL);
}

/*
 *			O G L _ P R O L O G
 *
 * There are global variables which are parameters to this routine.
 */
static void
Ogl_prolog(dmp)
struct dm *dmp;
{
  GLint mm; 
  char i;
  char *str = "a";
  GLfloat fogdepth;

  if (((struct ogl_vars *)dmp->dmr_vars)->mvars.debug)
    Tcl_AppendResult(interp, "Ogl_prolog\n", (char *)NULL);

  if (!glXMakeCurrent(((struct ogl_vars *)dmp->dmr_vars)->dpy,
		      ((struct ogl_vars *)dmp->dmr_vars)->win,
		      ((struct ogl_vars *)dmp->dmr_vars)->glxc)){
    Tcl_AppendResult(interp, "Ogl_prolog: Couldn't make context current\n", (char *)NULL);
    return;
  }

  if (!((struct ogl_vars *)dmp->dmr_vars)->mvars.doublebuffer){
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    /*			return;*/
  }

  if (((struct ogl_vars *)dmp->dmr_vars)->face_flag){
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    ((struct ogl_vars *)dmp->dmr_vars)->face_flag = 0;
    if (((struct ogl_vars *)dmp->dmr_vars)->mvars.cueing_on){
      glEnable(GL_FOG);
#if 1
      /*XXX Need to do something with Viewscale */
      fogdepth = 2.2 * (*dmp->dmr_vp); /* 2.2 is heuristic */
#else
      fogdepth = 2.2; /* 2.2 is heuristic */
#endif
      glFogf(GL_FOG_END, fogdepth);
#if 1
/*XXX Need to do something with Viewscale */
      fogdepth = (GLfloat) (0.5*((struct ogl_vars *)dmp->dmr_vars)->mvars.fogdensity/
			    (*dmp->dmr_vp));
#else
      fogdepth = (GLfloat) 0.5*((struct ogl_vars *)dmp->dmr_vars)->mvars.fogdensity;
#endif
      glFogf(GL_FOG_DENSITY, fogdepth);
      glFogi(GL_FOG_MODE, ((struct ogl_vars *)dmp->dmr_vars)->mvars.perspective_mode ? GL_EXP : GL_LINEAR);
    }
    if (((struct ogl_vars *)dmp->dmr_vars)->mvars.lighting_on){
      glEnable(GL_LIGHTING);
    }
  }
	
  glLineWidth((GLfloat) ((struct ogl_vars *)dmp->dmr_vars)->mvars.linewidth);
}

/*
 *			O G L _ E P I L O G
 */
static void
Ogl_epilog(dmp)
struct dm *dmp;
{
  if (((struct ogl_vars *)dmp->dmr_vars)->mvars.debug)
    Tcl_AppendResult(interp, "Ogl_epilog\n", (char *)NULL);

  /*
   * A Point, in the Center of the Screen.
   * This is drawn last, to always come out on top.
   */

  glColor3ub( (short)((struct ogl_vars *)dmp->dmr_vars)->rgbtab[4].r,
	      (short)((struct ogl_vars *)dmp->dmr_vars)->rgbtab[4].g,
	      (short)((struct ogl_vars *)dmp->dmr_vars)->rgbtab[4].b );
  glBegin(GL_POINTS);
  glVertex2i(0,0);
  glEnd();
  /* end of faceplate */

  if(((struct ogl_vars *)dmp->dmr_vars)->mvars.doublebuffer ){
    glXSwapBuffers(((struct ogl_vars *)dmp->dmr_vars)->dpy,
		   ((struct ogl_vars *)dmp->dmr_vars)->win);
    /* give Graphics pipe time to work */
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  /* Prevent lag between events and updates */
  XSync(((struct ogl_vars *)dmp->dmr_vars)->dpy, 0);

  if(((struct ogl_vars *)dmp->dmr_vars)->mvars.debug){
    int error;
    struct bu_vls tmp_vls;

    bu_vls_init(&tmp_vls);
    bu_vls_printf(&tmp_vls, "ANY ERRORS?\n");

    while((error = glGetError())!=0){
      bu_vls_printf(&tmp_vls, "Error: %x\n", error);
    }

    Tcl_AppendResult(interp, bu_vls_addr(&tmp_vls), (char *)NULL);
    bu_vls_free(&tmp_vls);
  }

  return;
}

/*
 *  			O G L _ N E W R O T
 *  load new rotation matrix onto top of stack
 */
static void
Ogl_newrot(dmp, mat, which_eye)
struct dm *dmp;
mat_t mat;
int which_eye;
{
	register fastf_t *mptr;
	GLfloat gtmat[16], view[16];
	GLfloat *gtmatp;
	mat_t	newm;
	int	i;

	
	if (((struct ogl_vars *)dmp->dmr_vars)->mvars.debug)
	  Tcl_AppendResult(interp, "Ogl_newrot()\n", (char *)NULL);

	if(((struct ogl_vars *)dmp->dmr_vars)->mvars.debug){
	  struct bu_vls tmp_vls;

	  bu_vls_init(&tmp_vls);
	  bu_vls_printf(&tmp_vls, "which eye = %d\t", which_eye);
	  bu_vls_printf(&tmp_vls, "newrot matrix = \n");
	  bu_vls_printf(&tmp_vls, "%g %g %g %g\n", mat[0], mat[4], mat[8],mat[12]);
	  bu_vls_printf(&tmp_vls, "%g %g %g %g\n", mat[1], mat[5], mat[9],mat[13]);
	  bu_vls_printf(&tmp_vls, "%g %g %g %g\n", mat[2], mat[6], mat[10],mat[14]);
	  bu_vls_printf(&tmp_vls, "%g %g %g %g\n", mat[3], mat[7], mat[11],mat[15]);

	  Tcl_AppendResult(interp, bu_vls_addr(&tmp_vls), (char *)NULL);
	  bu_vls_free(&tmp_vls);
	}

	switch(which_eye)  {
	case 0:
		/* Non-stereo */
		break;
	case 1:
		/* R eye */
		glViewport(0,  0, (XMAXSCREEN)+1, ( YSTEREO)+1); 
		glScissor(0,  0, (XMAXSCREEN)+1, (YSTEREO)+1);
		Ogl_puts( "R", 2020, 0, 0, DM_RED );
		break;
	case 2:
		/* L eye */
		glViewport(0,  0+YOFFSET_LEFT, ( XMAXSCREEN)+1, ( YSTEREO+YOFFSET_LEFT)-( YOFFSET_LEFT)+1); 
		glScissor(0,  0+YOFFSET_LEFT, ( XMAXSCREEN)+1, ( YSTEREO+YOFFSET_LEFT)-( YOFFSET_LEFT)+1);
		break;
	}

	mptr = mat;

	gtmat[0] = *(mptr++) * ((struct ogl_vars *)dmp->dmr_vars)->aspect;
	gtmat[4] = *(mptr++) * ((struct ogl_vars *)dmp->dmr_vars)->aspect;
	gtmat[8] = *(mptr++) * ((struct ogl_vars *)dmp->dmr_vars)->aspect;
	gtmat[12] = *(mptr++) * ((struct ogl_vars *)dmp->dmr_vars)->aspect;

	gtmat[1] = *(mptr++) * ((struct ogl_vars *)dmp->dmr_vars)->aspect;
	gtmat[5] = *(mptr++) * ((struct ogl_vars *)dmp->dmr_vars)->aspect;
	gtmat[9] = *(mptr++) * ((struct ogl_vars *)dmp->dmr_vars)->aspect;
	gtmat[13] = *(mptr++) * ((struct ogl_vars *)dmp->dmr_vars)->aspect;

	gtmat[2] = *(mptr++);
	gtmat[6] = *(mptr++);
	gtmat[10] = *(mptr++);
	gtmat[14] = *(mptr++);

	gtmat[3] = *(mptr++);
	gtmat[7] = *(mptr++);
	gtmat[11] = *(mptr++);
	gtmat[15] = *(mptr++);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef( 0.0, 0.0, -1.0 );
	glMultMatrixf( gtmat );

	/* Make sure that new matrix is applied to the lights */
	if (((struct ogl_vars *)dmp->dmr_vars)->mvars.lighting_on ){
		glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
		glLightfv(GL_LIGHT1, GL_POSITION, light1_position);
		glLightfv(GL_LIGHT2, GL_POSITION, light2_position);
		glLightfv(GL_LIGHT3, GL_POSITION, light3_position);

	}
}



/*
 *  			O G L _ O B J E C T
 *  
 *  Set up for an object, transformed as indicated, and with an
 *  object center as specified.  The ratio of object to screen size
 *  is passed in as a convienience.
 *
 *  Returns 0 if object could be drawn, !0 if object was omitted.
 */

/* ARGSUSED */
static int
Ogl_object( dmp, sp, mat, ratio, white_flag )
struct dm *dmp;
register struct solid *sp;
mat_t mat;
double ratio;
int white_flag;
{
	register struct rt_vlist	*vp;
	register int nvec;
	register float	*gtvec;
	register float material[4];
	char	gtbuf[16+3*sizeof(double)];
	int first;
	int i,j;

	if (((struct ogl_vars *)dmp->dmr_vars)->mvars.debug)
	  Tcl_AppendResult(interp, "Ogl_Object()\n", (char *)NULL);

	/*
	 *  It is claimed that the "dancing vector disease" of the
	 *  4D GT processors is due to the array being passed to v3f()
	 *  not being quad-word aligned (16-byte boundary).
	 *  This hack ensures that the buffer has this alignment.
	 *  Note that this requires gtbuf to be 16 bytes longer than needed.
	 */
	gtvec = (float *)((((int)gtbuf)+15) & (~0xF));

	if (sp->s_soldash)
		glEnable(GL_LINE_STIPPLE);		/* set dot-dash */

	if (white_flag && ((struct ogl_vars *)dmp->dmr_vars)->mvars.cueing_on)
		glDisable(GL_FOG);	

	if( ((struct ogl_vars *)dmp->dmr_vars)->mvars.rgb )  {
		register short	r, g, b;
		if( white_flag )  {
			r = g = b = 230;
		} else {
			r = (short)sp->s_color[0];
			g = (short)sp->s_color[1];
			b = (short)sp->s_color[2];
		}

		if(((struct ogl_vars *)dmp->dmr_vars)->mvars.lighting_on)
		{

			/* Ambient = .2, Diffuse = .6, Specular = .2 */

			material[0] = 	.2 * ( r / 255.0);
			material[1] = 	.2 * ( g / 255.0);
			material[2] = 	.2 * ( b / 255.0);
			material[3] = 1.0;
			glMaterialfv(GL_FRONT, GL_AMBIENT, material);
			glMaterialfv(GL_FRONT, GL_SPECULAR, material);

			material[0] *= 3.0;
			material[1] *= 3.0;
			material[2] *= 3.0;
			glMaterialfv(GL_FRONT, GL_DIFFUSE, material);

		} else {
			glColor3ub( r,  g,  b );
		}
	} else {
		if (white_flag){
		  ((struct ogl_vars *)dmp->dmr_vars)->ovec = MAP_ENTRY(DM_WHITE);
		  glIndexi(((struct ogl_vars *)dmp->dmr_vars)->ovec);
		} else if( (nvec = MAP_ENTRY( sp->s_dmindex )) !=
			   ((struct ogl_vars *)dmp->dmr_vars)->ovec) {
		  glIndexi(nvec);
		  ((struct ogl_vars *)dmp->dmr_vars)->ovec = nvec;
		}

		if (((struct ogl_vars *)dmp->dmr_vars)->mvars.lighting_on){
		  material[0] = ((struct ogl_vars *)dmp->dmr_vars)->ovec -
		    CMAP_RAMP_WIDTH + 2;
		  material[1] = ((struct ogl_vars *)dmp->dmr_vars)->ovec - CMAP_RAMP_WIDTH/2;
		  material[2] = ((struct ogl_vars *)dmp->dmr_vars)->ovec - 1;
		  glMaterialfv(GL_FRONT, GL_COLOR_INDEXES, material);
		}
	}


	/* Viewing region is from -1.0 to +1.0 */
	first = 1;
	for( BU_LIST_FOR( vp, rt_vlist, &(sp->s_vlist) ) )  {
		register int	i;
		register int	nused = vp->nused;
		register int	*cmd = vp->cmd;
		register point_t *pt = vp->pt;
		for( i = 0; i < nused; i++,cmd++,pt++ )  {
			switch( *cmd )  {
			case RT_VLIST_LINE_MOVE:
				/* Move, start line */
				if( first == 0 )
					glEnd();
				first = 0;
				glBegin(GL_LINE_STRIP);
				glVertex3dv( *pt );
				break;
			case RT_VLIST_LINE_DRAW:
				/* Draw line */
				glVertex3dv( *pt );
				break;
			case RT_VLIST_POLY_START:
				/* Start poly marker & normal */
				if( first == 0 )
					glEnd();
				glBegin(GL_POLYGON);
				/* Set surface normal (vl_pnt points outward) */
				VMOVE( gtvec, *pt );
				glNormal3fv(gtvec);
				break;
			case RT_VLIST_POLY_MOVE:
				/* Polygon Move */
				glVertex3dv( *pt );
				break;
			case RT_VLIST_POLY_DRAW:
				/* Polygon Draw */
				glVertex3dv( *pt );
				break;
			case RT_VLIST_POLY_END:
				/* Draw, End Polygon */
				glVertex3dv( *pt );
				glEnd();
				first = 1;
				break;
			case RT_VLIST_POLY_VERTNORM:
				/* Set per-vertex normal.  Given before vert. */
				VMOVE( gtvec, *pt );
				glNormal3fv(gtvec);
				break;
			}
		}
	}
	if( first == 0 ) glEnd();

	if (sp->s_soldash)
		glDisable(GL_LINE_STIPPLE);	/* restore solid lines */

	if (white_flag && ((struct ogl_vars *)dmp->dmr_vars)->mvars.cueing_on){
		glEnable(GL_FOG);
	}

	return(1);	/* OK */

}

/*
 *			O G L _ N O R M A L
 *
 * Restore the display processor to a normal mode of operation
 * (ie, not scaled, rotated, displaced, etc).
 * Turns off windowing.
 */
static void
Ogl_normal(dmp)
struct dm *dmp;
{
	GLint mm; 

	if (((struct ogl_vars *)dmp->dmr_vars)->mvars.debug)
	  Tcl_AppendResult(interp, "Ogl_normal\n", (char *)NULL);

	if( ((struct ogl_vars *)dmp->dmr_vars)->mvars.rgb )  {
	  glColor3ub( 0,  0,  0 );
	} else {
	  ((struct ogl_vars *)dmp->dmr_vars)->ovec = MAP_ENTRY(DM_BLACK);
	  glIndexi( ((struct ogl_vars *)dmp->dmr_vars)->ovec );
	}

	if (!((struct ogl_vars *)dmp->dmr_vars)->face_flag){
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadMatrixd( ((struct ogl_vars *)dmp->dmr_vars)->faceplate_mat );
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		((struct ogl_vars *)dmp->dmr_vars)->face_flag = 1;
		if(((struct ogl_vars *)dmp->dmr_vars)->mvars.cueing_on)
			glDisable(GL_FOG);
		if (((struct ogl_vars *)dmp->dmr_vars)->mvars.lighting_on)
			glDisable(GL_LIGHTING);

	}

	return;
}

/*
 *			O G L _ U P D A T E
 *
 * Transmit accumulated displaylist to the display processor.
 */
static void
Ogl_update(dmp)
struct dm *dmp;
{
  if (((struct ogl_vars *)dmp->dmr_vars)->mvars.debug)
    Tcl_AppendResult(interp, "Ogl_update()\n", (char *)NULL);

    XFlush(((struct ogl_vars *)dmp->dmr_vars)->dpy);
}


/*
 *			XOGL _ P U T S
 *
 * Output a string.
 * The starting position of the beam is as specified.
 */
static void
Ogl_puts( dmp, str, x, y, size, colour )
struct dm *dmp;
register char *str;
int x,y,size, colour;
{
  if (((struct ogl_vars *)dmp->dmr_vars)->mvars.debug)
    Tcl_AppendResult(interp, "Ogl_puts()\n", (char *)NULL);

	
  /*	glRasterPos2f( GED2IRIS(x),  GED2IRIS(y));*/
  if( ((struct ogl_vars *)dmp->dmr_vars)->mvars.rgb )  {
    glColor3ub( (short)((struct ogl_vars *)dmp->dmr_vars)->rgbtab[colour].r,
		(short)((struct ogl_vars *)dmp->dmr_vars)->rgbtab[colour].g,
		(short)((struct ogl_vars *)dmp->dmr_vars)->rgbtab[colour].b );
  } else {
    ((struct ogl_vars *)dmp->dmr_vars)->ovec = MAP_ENTRY(colour);
    glIndexi( ((struct ogl_vars *)dmp->dmr_vars)->ovec );
  }


  /*	glRasterPos2i( x,  y);*/
  glRasterPos2f( GED2IRIS(x),  GED2IRIS(y));
  glListBase(((struct ogl_vars *)dmp->dmr_vars)->fontOffset);
  glCallLists(strlen( str ), GL_UNSIGNED_BYTE,  str );
}


/*
 *			O G L _ 2 D _ L I N E
 *
 */
static void
Ogl_2d_line( dmp, x1, y1, x2, y2, dashed )
struct dm *dmp;
int x1, y1;
int x2, y2;
int dashed;
{
	register int nvec;

	if (((struct ogl_vars *)dmp->dmr_vars)->mvars.debug)
	  Tcl_AppendResult(interp, "Ogl_2d_line()\n", (char *)NULL);

	if( ((struct ogl_vars *)dmp->dmr_vars)->mvars.rgb )  {
		/* Yellow */

		glColor3ub( (short)255,  (short)255,  (short) 0 );
	} else {
	  if((nvec = MAP_ENTRY(DM_YELLOW)) !=
	     ((struct ogl_vars *)dmp->dmr_vars)->ovec) {
	    glIndexi(nvec);
	    ((struct ogl_vars *)dmp->dmr_vars)->ovec = nvec;
	  }
	}
	
/*	glColor3ub( (short)255,  (short)255,  (short) 0 );*/

	if(((struct ogl_vars *)dmp->dmr_vars)->mvars.debug){
	  GLfloat pmat[16];
	  struct bu_vls tmp_vls;

	  bu_vls_init(&tmp_vls);
	  glGetFloatv(GL_PROJECTION_MATRIX, pmat);
	  bu_vls_printf(&tmp_vls, "projection matrix:\n");
	  bu_vls_printf(&tmp_vls, "%g %g %g %g\n", pmat[0], pmat[4], pmat[8],pmat[12]);
	  bu_vls_printf(&tmp_vls, "%g %g %g %g\n", pmat[1], pmat[5], pmat[9],pmat[13]);
	  bu_vls_printf(&tmp_vls, "%g %g %g %g\n", pmat[2], pmat[6], pmat[10],pmat[14]);
	  bu_vls_printf(&tmp_vls, "%g %g %g %g\n", pmat[3], pmat[7], pmat[11],pmat[15]);
	  glGetFloatv(GL_MODELVIEW_MATRIX, pmat);
	  bu_vls_printf(&tmp_vls, "modelview matrix:\n");
	  bu_vls_printf(&tmp_vls, "%g %g %g %g\n", pmat[0], pmat[4], pmat[8],pmat[12]);
	  bu_vls_printf(&tmp_vls, "%g %g %g %g\n", pmat[1], pmat[5], pmat[9],pmat[13]);
	  bu_vls_printf(&tmp_vls, "%g %g %g %g\n", pmat[2], pmat[6], pmat[10],pmat[14]);
	  bu_vls_printf(&tmp_vls, "%g %g %g %g\n", pmat[3], pmat[7], pmat[11],pmat[15]);

	  Tcl_AppendResult(interp, bu_vls_addr(&tmp_vls), (char *)NULL);
	  bu_vls_free(&tmp_vls);
	}

	if( dashed )
		glEnable(GL_LINE_STIPPLE);
		
	glBegin(GL_LINES); 
	 glVertex2f( GED2IRIS(x1),  GED2IRIS(y1));
	 glVertex2f( GED2IRIS(x2),  GED2IRIS(y2));
	glEnd();

	if( dashed )		/* restore solid */
		glDisable(GL_LINE_STIPPLE); 
}


/*
 *			O G L _ I N P U T
 *
 * Execution must suspend in this routine until a significant event
 * has occured on either the command stream,
 * unless "noblock" is set.
 *
 *
 * Implicit Return -
 *	If any files are ready for input, their bits will be set in 'input'.
 *	Otherwise, 'input' will be all zeros.
 *
 * DEPRECATED 
 *
 */
/* ARGSUSED */
static void
Ogl_input( dmp, input, noblock )
struct dm *dmp;
fd_set		*input;
int		noblock;
{
    return;
}

/* 
 *			O G L _ L I G H T
 */
/* ARGSUSED */
static void
Ogl_light( dmp, cmd, func )
struct dm *dmp;
int cmd;
int func;			/* BE_ or BV_ function */
{
	return;
}

/* ARGSUSED */
static unsigned
Ogl_cvtvecs( dmp, sp )
struct dm *dmp;
struct solid *sp;
{
	return( 0 );
}

/*
 * Loads displaylist
 */
static unsigned
Ogl_load( dmp, addr, count )
struct dm *dmp;
unsigned addr, count;
{
  struct bu_vls tmp_vls;

  bu_vls_init(&tmp_vls);
  bu_vls_printf(&tmp_vls, "Ogl_load(x%x, %d.)\n", addr, count );
  Tcl_AppendResult(interp, bu_vls_addr(&tmp_vls), (char *)NULL);
  bu_vls_free(&tmp_vls);
  return( 0 );
}

static void
Ogl_viewchange(dmp)
struct dm *dmp;
{
}

static void
Ogl_colorchange(dmp)
struct dm *dmp;
{
	register int i;
	register int nramp;
	XColor celltest;
	int count = 0;
	Colormap a_cmap;

	if( ((struct ogl_vars *)dmp->dmr_vars)->mvars.debug )
	  Tcl_AppendResult(interp, "colorchange\n", (char *)NULL);

	/* Program the builtin colors */
	((struct ogl_vars *)dmp->dmr_vars)->rgbtab[0].r=0; 
	((struct ogl_vars *)dmp->dmr_vars)->rgbtab[0].g=0; 
	((struct ogl_vars *)dmp->dmr_vars)->rgbtab[0].b=0;/* Black */
	((struct ogl_vars *)dmp->dmr_vars)->rgbtab[1].r=255; 
	((struct ogl_vars *)dmp->dmr_vars)->rgbtab[1].g=0; 
	((struct ogl_vars *)dmp->dmr_vars)->rgbtab[1].b=0;/* Red */
	((struct ogl_vars *)dmp->dmr_vars)->rgbtab[2].r=0; 
	((struct ogl_vars *)dmp->dmr_vars)->rgbtab[2].g=0; 
	((struct ogl_vars *)dmp->dmr_vars)->rgbtab[2].b=255;/* Blue */
	((struct ogl_vars *)dmp->dmr_vars)->rgbtab[3].r=255; 
	((struct ogl_vars *)dmp->dmr_vars)->rgbtab[3].g=255;
	((struct ogl_vars *)dmp->dmr_vars)->rgbtab[3].b=0;/*Yellow */
	((struct ogl_vars *)dmp->dmr_vars)->rgbtab[4].r =
	  ((struct ogl_vars *)dmp->dmr_vars)->rgbtab[4].g =
	  ((struct ogl_vars *)dmp->dmr_vars)->rgbtab[4].b = 255; /* White */
	((struct ogl_vars *)dmp->dmr_vars)->uslots = 5;

	if( ((struct ogl_vars *)dmp->dmr_vars)->mvars.rgb )  {
		if(((struct ogl_vars *)dmp->dmr_vars)->mvars.cueing_on) {
			glEnable(GL_FOG);
		} else {
			glDisable(GL_FOG);
		}

		glColor3ub( (short)255,  (short)255,  (short)255 );

		/* apply region-id based colors to the solid table */
		dmp->dmr_cfunc();

		return;
	}

	if(USE_RAMP && (((struct ogl_vars *)dmp->dmr_vars)->index_size < 7)) {
	  Tcl_AppendResult(interp, "Too few bitplanes: depthcueing and lighting disabled\n",
			   (char *)NULL);
	  ((struct ogl_vars *)dmp->dmr_vars)->mvars.cueing_on = 0;
	  ((struct ogl_vars *)dmp->dmr_vars)->mvars.lighting_on = 0;
	}
	/* number of slots is 2^indexsize */
	((struct ogl_vars *)dmp->dmr_vars)->nslots =
	  1<<((struct ogl_vars *)dmp->dmr_vars)->index_size;
	if( ((struct ogl_vars *)dmp->dmr_vars)->nslots > NSLOTS )
	  ((struct ogl_vars *)dmp->dmr_vars)->nslots = NSLOTS;
	if(USE_RAMP) {
	  /* peel off reserved ones */
	  ((struct ogl_vars *)dmp->dmr_vars)->nslots =
	    (((struct ogl_vars *)dmp->dmr_vars)->nslots - CMAP_BASE) / CMAP_RAMP_WIDTH;
	} else {
	  ((struct ogl_vars *)dmp->dmr_vars)->nslots -= CMAP_BASE;	/* peel off the reserved entries */
	}

	((struct ogl_vars *)dmp->dmr_vars)->ovec = -1;	/* Invalidate the old colormap entry */

	/* apply region-id based colors to the solid table */
	dmp->dmr_cfunc();

	/* best to do this before the colorit */
	if (((struct ogl_vars *)dmp->dmr_vars)->mvars.cueing_on && ((struct ogl_vars *)dmp->dmr_vars)->mvars.lighting_on){
		((struct ogl_vars *)dmp->dmr_vars)->mvars.lighting_on = 0;
		glDisable(GL_LIGHTING);
	}

	/* Map the colors in the solid table to colormap indices */
	Ogl_colorit(dmp);

	for( i=0; i < ((struct ogl_vars *)dmp->dmr_vars)->uslots; i++ )  {
		Ogl_gen_color( dmp, i,
			       ((struct ogl_vars *)dmp->dmr_vars)->rgbtab[i].r,
			       ((struct ogl_vars *)dmp->dmr_vars)->rgbtab[i].g,
			       ((struct ogl_vars *)dmp->dmr_vars)->rgbtab[i].b);
	}

	/* best to do this after the colorit */
	if (((struct ogl_vars *)dmp->dmr_vars)->mvars.cueing_on){
		glEnable(GL_FOG);
	} else {
		glDisable(GL_FOG);
	}

	((struct ogl_vars *)dmp->dmr_vars)->ovec = MAP_ENTRY(DM_WHITE);
	glIndexi( ((struct ogl_vars *)dmp->dmr_vars)->ovec );

}

/* ARGSUSED */
static void
Ogl_debug(dmp, lvl)
struct dm *dmp;
{
  ((struct ogl_vars *)dmp->dmr_vars)->mvars.debug = lvl;
  XFlush(((struct ogl_vars *)dmp->dmr_vars)->dpy);
  Tcl_AppendResult(interp, "flushed\n", (char *)NULL);
}

static void
Ogl_window(w)
register int w[];
{
}

/* the font used depends on the size of the window opened */
#define FONTBACK	"-adobe-courier-medium-r-normal--10-100-75-75-m-60-iso8859-1"
#define FONT5	"5x7"
#define FONT6	"6x10"
#define FONT7	"7x13"
#define FONT8	"8x13"
#define FONT9	"9x15"

static int
Ogl_setup( dmp )
struct dm *dmp;
{
  static count = 0;
  char *cp, symbol;
  XGCValues gcv;
  XColor a_color;
  Visual *a_visual;
  int a_screen, num, i, success;
  int major, minor;
  Colormap  a_cmap;
  XVisualInfo *vip;
  int dsize, use, dbfr, rgba, red, blue, green, alpha, index;
  GLfloat backgnd[4];
  long supplied;
  int j, k;
  int ndevices;
  int nclass = 0;
  XDeviceInfoPtr olist, list;
  XDevice *dev;
  XEventClass e_class[15];
  XInputClassInfo *cip;
  struct bu_vls str;
  Display *tmp_dpy;

  bu_vls_init(&str);

  /* Only need to do this once for this display manager */
  if(!count)
    Ogl_load_startup(dmp);

  if(BU_LIST_IS_EMPTY(&head_ogl_vars.l))
    Tk_CreateGenericHandler(dmp->dmr_eventhandler, (ClientData)NULL);

  BU_LIST_APPEND(&head_ogl_vars.l, &((struct ogl_vars *)dmp->dmr_vars)->l);

  bu_vls_printf(&dmp->dmr_pathName, ".dm_ogl%d", count++);

  /* this is important so that Ogl_configure_notify knows to set
   * the font */
  ((struct ogl_vars *)dmp->dmr_vars)->fontstruct = NULL;

  if((tmp_dpy = XOpenDisplay(dmp->dmr_dname)) == NULL){
    bu_vls_free(&str);
    return TCL_ERROR;
  }

  ((struct ogl_vars *)dmp->dmr_vars)->width = DisplayWidth(tmp_dpy, DefaultScreen(tmp_dpy)) - 20;
  ((struct ogl_vars *)dmp->dmr_vars)->height = DisplayHeight(tmp_dpy, DefaultScreen(tmp_dpy)) - 20;

  /* Make window square */
  if(((struct ogl_vars *)dmp->dmr_vars)->height < ((struct ogl_vars *)dmp->dmr_vars)->width)
    ((struct ogl_vars *)dmp->dmr_vars)->width = ((struct ogl_vars *)dmp->dmr_vars)->height;
  else
    ((struct ogl_vars *)dmp->dmr_vars)->height = ((struct ogl_vars *)dmp->dmr_vars)->width;

  XCloseDisplay(tmp_dpy);

  /* Make xtkwin a toplevel window */
  ((struct ogl_vars *)dmp->dmr_vars)->xtkwin =
    Tk_CreateWindowFromPath(interp, tkwin, bu_vls_addr(&dmp->dmr_pathName), dmp->dmr_dname);

  /* Open the display - XXX see what NULL does now */
  if( ((struct ogl_vars *)dmp->dmr_vars)->xtkwin == NULL ) {
    Tcl_AppendResult(interp, "dm-Ogl: Failed to open ", bu_vls_addr(&dmp->dmr_pathName),
		     "\n", (char *)NULL);
    return TCL_ERROR;
  }

  bu_vls_strcpy(&str, "init_ogl ");
  bu_vls_printf(&str, "%s\n", bu_vls_addr(&dmp->dmr_pathName));

  if(Tcl_Eval(interp, bu_vls_addr(&str)) == TCL_ERROR){
    bu_vls_free(&str);
    return TCL_ERROR;
  }

  ((struct ogl_vars *)dmp->dmr_vars)->dpy = Tk_Display(((struct ogl_vars *)dmp->dmr_vars)->xtkwin);

  /* must do this before MakeExist */
  if ((vip=Ogl_set_visual(dmp, ((struct ogl_vars *)dmp->dmr_vars)->xtkwin))==NULL){
    Tcl_AppendResult(interp, "Ogl_open: Can't get an appropriate visual.\n", (char *)NULL);
    return TCL_ERROR;
  }

#if 1
  Tk_GeometryRequest(((struct ogl_vars *)dmp->dmr_vars)->xtkwin,
		     ((struct ogl_vars *)dmp->dmr_vars)->width,
		     ((struct ogl_vars *)dmp->dmr_vars)->height);

  Tk_ResizeWindow(((struct ogl_vars *)dmp->dmr_vars)->xtkwin,
		  ((struct ogl_vars *)dmp->dmr_vars)->width,
		  ((struct ogl_vars *)dmp->dmr_vars)->height);
#endif
  Tk_MoveToplevelWindow(((struct ogl_vars *)dmp->dmr_vars)->xtkwin, 1276 - 976, 0);
  Tk_MakeWindowExist(((struct ogl_vars *)dmp->dmr_vars)->xtkwin);

  ((struct ogl_vars *)dmp->dmr_vars)->win = Tk_WindowId(((struct ogl_vars *)dmp->dmr_vars)->xtkwin);

  a_screen = Tk_ScreenNumber(((struct ogl_vars *)dmp->dmr_vars)->xtkwin);

  /* open GLX context */
  /* If the sgi display manager has been used, then we must use
   * an indirect context. Otherwise use direct, since it is usually
   * faster.
   */
  if ((((struct ogl_vars *)dmp->dmr_vars)->glxc =
       glXCreateContext(((struct ogl_vars *)dmp->dmr_vars)->dpy, vip, 0,
			ogl_sgi_used ? GL_FALSE : GL_TRUE))==NULL) {
    Tcl_AppendResult(interp, "Ogl_open: couldn't create glXContext.\n", (char *)NULL);
    return TCL_ERROR;
  }
  /* If we used an indirect context, then as far as sgi is concerned,
   * gl hasn't been used.
   */
  ((struct ogl_vars *)dmp->dmr_vars)->is_direct =
    (char) glXIsDirect(((struct ogl_vars *)dmp->dmr_vars)->dpy,
		       ((struct ogl_vars *)dmp->dmr_vars)->glxc);
  Tcl_AppendResult(interp, "Using ", ((struct ogl_vars *)dmp->dmr_vars)->is_direct ?
		   "a direct" : "an indirect", " OpenGL rendering context.\n", (char *)NULL);
  /* set ogl_ogl_used if the context was ever direct */
  ogl_ogl_used = (((struct ogl_vars *)dmp->dmr_vars)->is_direct || ogl_ogl_used);

  /*
   * Take a look at the available input devices. We're looking
   * for "dial+buttons".
   */
  olist = list =
    (XDeviceInfoPtr) XListInputDevices (((struct ogl_vars *)dmp->dmr_vars)->dpy, &ndevices);

  /* IRIX 4.0.5 bug workaround */
  if( list == (XDeviceInfoPtr)NULL ||
      list == (XDeviceInfoPtr)1 )  goto Done;

  for(j = 0; j < ndevices; ++j, list++){
    if(list->use == IsXExtensionDevice){
      if(!strcmp(list->name, "dial+buttons")){
	if((dev = XOpenDevice(((struct ogl_vars *)dmp->dmr_vars)->dpy,
			      list->id)) == (XDevice *)NULL){
	  Tcl_AppendResult(interp, "Glx_open: Couldn't open the dials+buttons\n", (char *)NULL);
	  goto Done;
	}

	for(cip = dev->classes, k = 0; k < dev->num_classes;
	    ++k, ++cip){
	  switch(cip->input_class){
#if IR_BUTTONS
	  case ButtonClass:
	    DeviceButtonPress(dev, ((struct ogl_vars *)dmp->dmr_vars)->devbuttonpress,
			      e_class[nclass]);
	    ++nclass;
	    DeviceButtonRelease(dev, ((struct ogl_vars *)dmp->dmr_vars)->devbuttonrelease,
				e_class[nclass]);
	    ++nclass;
	    break;
#endif
#if IR_KNOBS
	  case ValuatorClass:
	    DeviceMotionNotify(dev, ((struct ogl_vars *)dmp->dmr_vars)->devmotionnotify,
			       e_class[nclass]);
	    ++nclass;
	    break;
#endif
	  default:
	    break;
	  }
	}

	XSelectExtensionEvent(((struct ogl_vars *)dmp->dmr_vars)->dpy,
			      ((struct ogl_vars *)dmp->dmr_vars)->win, e_class, nclass);
	goto Done;
      }
    }
  }
Done:
  XFreeDeviceList(olist);

  if (!glXMakeCurrent(((struct ogl_vars *)dmp->dmr_vars)->dpy,
		      ((struct ogl_vars *)dmp->dmr_vars)->win,
		      ((struct ogl_vars *)dmp->dmr_vars)->glxc)){
    Tcl_AppendResult(interp, "Ogl_open: Couldn't make context current\n", (char *)NULL);
    return TCL_ERROR;
  }

  /* display list (fontOffset + char) will displays a given ASCII char */
  if ((((struct ogl_vars *)dmp->dmr_vars)->fontOffset = glGenLists(128))==0){
    Tcl_AppendResult(interp, "dm-ogl: Can't make display lists for font.\n", (char *)NULL);
    return TCL_ERROR;
  }

  /* do viewport, ortho commands and initialize font*/
  Ogl_configure_window_shape(dmp);

  /* Lines will be solid when stippling disabled, dashed when enabled*/
  glLineStipple( 1, 0xCF33);
  glDisable(GL_LINE_STIPPLE);

  backgnd[0] = backgnd[1] = backgnd[2] = backgnd[3] = 0.0;
  glFogi(GL_FOG_MODE, GL_LINEAR);
  glFogf(GL_FOG_START, 0.0);
  glFogf(GL_FOG_END, 2.0);
  if (((struct ogl_vars *)dmp->dmr_vars)->mvars.rgb)
    glFogfv(GL_FOG_COLOR, backgnd);
  else
    glFogi(GL_FOG_INDEX, CMAP_RAMP_WIDTH - 1);
#if 1
  /*XXX Need to do something about VIEWFACTOR */
  glFogf(GL_FOG_DENSITY, VIEWFACTOR);
#else
  glFogf(GL_FOG_DENSITY, 1.0);
#endif

	

  /* Initialize matrices */
  /* Leave it in model_view mode normally */
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-1.0, 1.0, -1.0, 1.0, 0.0, 2.0);
  glGetDoublev(GL_PROJECTION_MATRIX, ((struct ogl_vars *)dmp->dmr_vars)->faceplate_mat);
  glPushMatrix();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity(); 
  glTranslatef( 0.0, 0.0, -1.0); 
  glPushMatrix();
  glLoadIdentity();
  ((struct ogl_vars *)dmp->dmr_vars)->face_flag = 1;	/* faceplate matrix is on top of stack */
		
  Tk_MapWindow(((struct ogl_vars *)dmp->dmr_vars)->xtkwin);
  return TCL_OK;
}

/* currently, get a double buffered rgba visual that works with Tk and
 * OpenGL
 */
static XVisualInfo *
Ogl_set_visual(dmp, tkwin)
struct dm *dmp;
Tk_Window tkwin;
{
  XVisualInfo *vip, vitemp, *vibase, *maxvip;
  int good[40];
  int num, i, j;
  int use, rgba, dbfr, stereo;
  int m_stereo, m_double, m_rgba;
  int tries, baddepth;

  /* m_stereo - try to get stereo 
   * m_double - try to get double buffered
   * m_rgba  - try to get rgb
   */

#if 0
  /*XXX Need to do something with this */
  if( mged_variables.eye_sep_dist )  {
    m_stereo = 1;
  } else {
    m_stereo = 0;
  }
#endif

  m_double = 1;
  m_rgba = 1;
	
  /* Try to satisfy the above desires with a color visual of the
   * greatest depth */

  vibase = XGetVisualInfo(((struct ogl_vars *)dmp->dmr_vars)->dpy,
			  0, &vitemp, &num);

  while (1) {
    for (i=0, j=0, vip=vibase; i<num; i++, vip++){
      /* requirements */
      glXGetConfig(((struct ogl_vars *)dmp->dmr_vars)->dpy,
		   vip, GLX_USE_GL, &use);
      if (!use)
	continue;
      /* desires */
      if (m_rgba){
	glXGetConfig(((struct ogl_vars *)dmp->dmr_vars)->dpy,
		     vip, GLX_RGBA, &rgba);
	if (!rgba)
	  continue;
      } else if (vip->class != PseudoColor) {
	/* if index mode, accept only read/write*/
	continue;
      }
      if ( m_stereo ) {
	glXGetConfig(((struct ogl_vars *)dmp->dmr_vars)->dpy,
		     vip, GLX_STEREO, &stereo);
	if (!stereo)
	  continue;
      }
      if (m_double ) {
	glXGetConfig(((struct ogl_vars *)dmp->dmr_vars)->dpy,
		     vip, GLX_DOUBLEBUFFER,&dbfr);
	if (!dbfr)
	  continue;
      }
			
      /* this visual meets criteria */
      good[j++] = i;
    }

    /* j = number of acceptable visuals under consideration */
    if (j >= 1){
      baddepth = 1000;
      tries = 0;
      while (tries < j) {
	maxvip = vibase + good[0];
	for (i=1; i<j; i++) {
	  vip = vibase + good[i];
	  if ((vip->depth > maxvip->depth)&&(vip->depth < baddepth)){
	    maxvip = vip;
	  }
	}

	/* make sure Tk handles it */
	if (maxvip->class == PseudoColor)
	  ((struct ogl_vars *)dmp->dmr_vars)->cmap =
	    XCreateColormap(((struct ogl_vars *)dmp->dmr_vars)->dpy,
			    RootWindow(((struct ogl_vars *)dmp->dmr_vars)->dpy,
				       maxvip->screen),
			    maxvip->visual, AllocAll);
	else
	  ((struct ogl_vars *)dmp->dmr_vars)->cmap =
	    XCreateColormap(((struct ogl_vars *)dmp->dmr_vars)->dpy,
			    RootWindow(((struct ogl_vars *)dmp->dmr_vars)->dpy,
				       maxvip->screen),
			    maxvip->visual, AllocNone);

	if (Tk_SetWindowVisual(tkwin, maxvip->visual, maxvip->depth,
			       ((struct ogl_vars *)dmp->dmr_vars)->cmap)){
	  ((struct ogl_vars *)dmp->dmr_vars)->mvars.doublebuffer = m_double;
	  glXGetConfig(((struct ogl_vars *)dmp->dmr_vars)->dpy, maxvip, GLX_DEPTH_SIZE,
		       &((struct ogl_vars *)dmp->dmr_vars)->mvars.depth);
	  if (((struct ogl_vars *)dmp->dmr_vars)->mvars.depth > 0)
	    ((struct ogl_vars *)dmp->dmr_vars)->mvars.zbuf = 1;
	  ((struct ogl_vars *)dmp->dmr_vars)->mvars.rgb = m_rgba;
	  if (!m_rgba){
	    glXGetConfig(((struct ogl_vars *)dmp->dmr_vars)->dpy,
			 maxvip, GLX_BUFFER_SIZE,
			 &((struct ogl_vars *)dmp->dmr_vars)->index_size);
	  }
	  ((struct ogl_vars *)dmp->dmr_vars)->stereo_is_on = m_stereo;
	  return (maxvip); /* sucess */
	} else { 
	  /* retry with lesser depth */
	  baddepth = maxvip->depth;
	  tries ++;
	  XFreeColormap(((struct ogl_vars *)dmp->dmr_vars)->dpy,
			((struct ogl_vars *)dmp->dmr_vars)->cmap);
	}
      }
    }

    /* if no success at this point, relax a desire and try again */
    if ( m_stereo ){
      m_stereo = 0;
      Tcl_AppendResult(interp, "Stereo not available.\n", (char *)NULL);
    } else if (m_rgba) {
      m_rgba = 0;
      Tcl_AppendResult(interp, "RGBA not available.\n", (char *)NULL);
    } else if (m_double) {
      m_double = 0;
      Tcl_AppendResult(interp, "Doublebuffering not available. \n", (char *)NULL);
    } else {
      return(NULL); /* failure */
    }
  }
}

/* 
 *			O G L _ C O N F I G U R E _ W I N D O W _ S H A P E
 *
 *  Either initially, or on resize/reshape of the window,
 *  sense the actual size of the window, and perform any
 *  other initializations of the window configuration.
 *
 * also change font size if necessary
 */
void
Ogl_configure_window_shape(dmp)
struct dm *dmp;
{
  int		npix;
  GLint mm; 
  XWindowAttributes xwa;
  XFontStruct	*newfontstruct;

  XGetWindowAttributes( ((struct ogl_vars *)dmp->dmr_vars)->dpy,
			((struct ogl_vars *)dmp->dmr_vars)->win, &xwa );
  ((struct ogl_vars *)dmp->dmr_vars)->height = xwa.height;
  ((struct ogl_vars *)dmp->dmr_vars)->width = xwa.width;
	
  glViewport(0,  0, (((struct ogl_vars *)dmp->dmr_vars)->width),
	     (((struct ogl_vars *)dmp->dmr_vars)->height));
  glScissor(0,  0, (((struct ogl_vars *)dmp->dmr_vars)->width)+1,
	    (((struct ogl_vars *)dmp->dmr_vars)->height)+1);

  if( ((struct ogl_vars *)dmp->dmr_vars)->mvars.zbuffer_on )
    Ogl_establish_zbuffer(dmp);

  Ogl_establish_lighting(dmp);

#if 0
  glDrawBuffer(GL_FRONT_AND_BACK);

  glClearColor(0.0, 0.0, 0.0, 0.0);
  if (((struct ogl_vars *)dmp->dmr_vars)->mvars.zbuffer_on)
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  else
    glClear( GL_COLOR_BUFFER_BIT);

  if (((struct ogl_vars *)dmp->dmr_vars)->mvars.doublebuffer)
    glDrawBuffer(GL_BACK);
  else
    glDrawBuffer(GL_FRONT);

  /*CJXX*/
  glFlush();
#endif
  /*CJXX this might cause problems in perspective mode? */
  glGetIntegerv(GL_MATRIX_MODE, &mm);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho( -xlim_view, xlim_view, -ylim_view, ylim_view, 0.0, 2.0 );
  glMatrixMode(mm);
  ((struct ogl_vars *)dmp->dmr_vars)->aspect =
    (fastf_t)((struct ogl_vars *)dmp->dmr_vars)->height/
    (fastf_t)((struct ogl_vars *)dmp->dmr_vars)->width;


  /* First time through, load a font or quit */
  if (((struct ogl_vars *)dmp->dmr_vars)->fontstruct == NULL) {
    if ((((struct ogl_vars *)dmp->dmr_vars)->fontstruct =
	 XLoadQueryFont(((struct ogl_vars *)dmp->dmr_vars)->dpy,
			FONT9)) == NULL ) {
      /* Try hardcoded backup font */
      if ((((struct ogl_vars *)dmp->dmr_vars)->fontstruct =
	   XLoadQueryFont(((struct ogl_vars *)dmp->dmr_vars)->dpy,
			  FONTBACK)) == NULL) {
	Tcl_AppendResult(interp, "dm-Ogl: Can't open font '", FONT9,
			 "' or '", FONTBACK, "'\n", (char *)NULL);
	return;
      }
    }
    glXUseXFont( ((struct ogl_vars *)dmp->dmr_vars)->fontstruct->fid,
		 0, 127, ((struct ogl_vars *)dmp->dmr_vars)->fontOffset);
  }
		

  /* Always try to choose a the font that best fits the window size.
   */

  if (((struct ogl_vars *)dmp->dmr_vars)->width < 582) {
    if (((struct ogl_vars *)dmp->dmr_vars)->fontstruct->per_char->width != 5) {
      if ((newfontstruct = XLoadQueryFont(((struct ogl_vars *)dmp->dmr_vars)->dpy,
					  FONT5)) != NULL ) {
	XFreeFont(((struct ogl_vars *)dmp->dmr_vars)->dpy,
		  ((struct ogl_vars *)dmp->dmr_vars)->fontstruct);
	((struct ogl_vars *)dmp->dmr_vars)->fontstruct = newfontstruct;
	glXUseXFont( ((struct ogl_vars *)dmp->dmr_vars)->fontstruct->fid,
		     0, 127, ((struct ogl_vars *)dmp->dmr_vars)->fontOffset);
      }
    }
  } else if (((struct ogl_vars *)dmp->dmr_vars)->width < 679) {
    if (((struct ogl_vars *)dmp->dmr_vars)->fontstruct->per_char->width != 6){
      if ((newfontstruct = XLoadQueryFont(((struct ogl_vars *)dmp->dmr_vars)->dpy,
					  FONT6)) != NULL ) {
	XFreeFont(((struct ogl_vars *)dmp->dmr_vars)->dpy,
		  ((struct ogl_vars *)dmp->dmr_vars)->fontstruct);
	((struct ogl_vars *)dmp->dmr_vars)->fontstruct = newfontstruct;
	glXUseXFont( ((struct ogl_vars *)dmp->dmr_vars)->fontstruct->fid,
		     0, 127, ((struct ogl_vars *)dmp->dmr_vars)->fontOffset);
      }
    }
  } else if (((struct ogl_vars *)dmp->dmr_vars)->width < 776) {
    if (((struct ogl_vars *)dmp->dmr_vars)->fontstruct->per_char->width != 7){
      if ((newfontstruct = XLoadQueryFont(((struct ogl_vars *)dmp->dmr_vars)->dpy,
					  FONT7)) != NULL ) {
	XFreeFont(((struct ogl_vars *)dmp->dmr_vars)->dpy,
		  ((struct ogl_vars *)dmp->dmr_vars)->fontstruct);
	((struct ogl_vars *)dmp->dmr_vars)->fontstruct = newfontstruct;
	glXUseXFont( ((struct ogl_vars *)dmp->dmr_vars)->fontstruct->fid,
		     0, 127, ((struct ogl_vars *)dmp->dmr_vars)->fontOffset);
      }
    }
  } else if (((struct ogl_vars *)dmp->dmr_vars)->width < 873) {
    if (((struct ogl_vars *)dmp->dmr_vars)->fontstruct->per_char->width != 8){
      if ((newfontstruct = XLoadQueryFont(((struct ogl_vars *)dmp->dmr_vars)->dpy,
					  FONT8)) != NULL ) {
	XFreeFont(((struct ogl_vars *)dmp->dmr_vars)->dpy,
		  ((struct ogl_vars *)dmp->dmr_vars)->fontstruct);
	((struct ogl_vars *)dmp->dmr_vars)->fontstruct = newfontstruct;
	glXUseXFont( ((struct ogl_vars *)dmp->dmr_vars)->fontstruct->fid,
		     0, 127, ((struct ogl_vars *)dmp->dmr_vars)->fontOffset);
      }
    }
  } else {
    if (((struct ogl_vars *)dmp->dmr_vars)->fontstruct->per_char->width != 9){
      if ((newfontstruct = XLoadQueryFont(((struct ogl_vars *)dmp->dmr_vars)->dpy,
					  FONT9)) != NULL ) {
	XFreeFont(((struct ogl_vars *)dmp->dmr_vars)->dpy,
		  ((struct ogl_vars *)dmp->dmr_vars)->fontstruct);
	((struct ogl_vars *)dmp->dmr_vars)->fontstruct = newfontstruct;
	glXUseXFont( ((struct ogl_vars *)dmp->dmr_vars)->fontstruct->fid,
		     0, 127, ((struct ogl_vars *)dmp->dmr_vars)->fontOffset);
      }
    }
  }
}

void	
Ogl_establish_lighting(dmp)
struct dm *dmp;
{
  if (!((struct ogl_vars *)dmp->dmr_vars)->mvars.lighting_on) {
    /* Turn it off */
    glDisable(GL_LIGHTING);
    if (!((struct ogl_vars *)dmp->dmr_vars)->mvars.rgb)
      Ogl_colorchange(dmp);
  } else {
    /* Turn it on */

    if (!((struct ogl_vars *)dmp->dmr_vars)->mvars.rgb){
      if (((struct ogl_vars *)dmp->dmr_vars)->mvars.cueing_on){
	((struct ogl_vars *)dmp->dmr_vars)->mvars.cueing_on = 0;
	glDisable(GL_FOG);
      } 
      Ogl_colorchange(dmp);
    }

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb_three);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);

    /* light positions specified in Ogl_newrot */

    glLightfv(GL_LIGHT0, GL_SPECULAR, light0_diffuse);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, light1_diffuse);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_diffuse);
    glLightfv(GL_LIGHT2, GL_SPECULAR, light2_diffuse);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, light2_diffuse);
    glLightfv(GL_LIGHT3, GL_SPECULAR, light3_diffuse);
    glLightfv(GL_LIGHT3, GL_DIFFUSE, light3_diffuse);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_LIGHT3);
    glEnable(GL_LIGHT2);
  }

#if 0
  dmaflag = 1;
#endif
}	

void	
Ogl_establish_zbuffer(dmp)
struct dm *dmp;
{
  if( ((struct ogl_vars *)dmp->dmr_vars)->mvars.zbuf == 0 ) {
    Tcl_AppendResult(interp, "dm-Ogl: This machine has no Zbuffer to enable\n",
		     (char *)NULL);
    ((struct ogl_vars *)dmp->dmr_vars)->mvars.zbuffer_on = 0;
  }

  if (((struct ogl_vars *)dmp->dmr_vars)->mvars.zbuffer_on)  {
    glEnable(GL_DEPTH_TEST);
  } else {
    glDisable(GL_DEPTH_TEST);
  }

#if 0
  dmaflag = 1;
#endif	
}

void
Ogl_establish_perspective(dmp)
struct dm *dmp;
{
  struct bu_vls vls;

  bu_vls_init(&vls);
  bu_vls_printf( &vls, "set perspective %d\n",
		 ((struct ogl_vars *)dmp->dmr_vars)->mvars.perspective_mode ?
		 perspective_table[((struct ogl_vars *)dmp->dmr_vars)->perspective_angle] :
		 -1 );
  Tcl_Eval(interp, bu_vls_addr(&vls));
  bu_vls_free(&vls);
#if 0
  dmaflag = 1;
#endif
}

/*
  This routine will toggle the perspective_angle if the
  dummy_perspective value is 0 or less. Otherwise, the
  perspective_angle is set to the value of (dummy_perspective - 1).
*/
void
Ogl_set_perspective(dmp)
struct dm *dmp;
{
  /* set perspective matrix */
  if(((struct ogl_vars *)dmp->dmr_vars)->mvars.dummy_perspective > 0)
    ((struct ogl_vars *)dmp->dmr_vars)->perspective_angle =
      ((struct ogl_vars *)dmp->dmr_vars)->mvars.dummy_perspective <= 4 ?
      ((struct ogl_vars *)dmp->dmr_vars)->mvars.dummy_perspective - 1: 3;
  else if (--((struct ogl_vars *)dmp->dmr_vars)->perspective_angle < 0) /* toggle perspective matrix */
    ((struct ogl_vars *)dmp->dmr_vars)->perspective_angle = 3;

  if(((struct ogl_vars *)dmp->dmr_vars)->mvars.perspective_mode){
    struct bu_vls vls;

    bu_vls_init(&vls);
    bu_vls_printf( &vls, "set perspective %d\n",
		   perspective_table[((struct ogl_vars *)dmp->dmr_vars)->perspective_angle] );
    Tcl_Eval(interp, bu_vls_addr(&vls));
    bu_vls_free(&vls);
  }

  /*
     Just in case the "!" is used with the set command. This
     allows us to toggle through more than two values.
   */
  ((struct ogl_vars *)dmp->dmr_vars)->mvars.dummy_perspective = 1;

#if 0
  dmaflag = 1;
#endif
}

static void
Ogl_colorit(dmp)
struct dm *dmp;
{
  register struct solid	*sp;
  register struct rgbtab *rgb;
  register int i;
  register int r,g,b;

  if( ((struct ogl_vars *)dmp->dmr_vars)->mvars.rgb )  return;

  FOR_ALL_SOLIDS(sp, &dmp->dmr_hp->l)  {
    r = sp->s_color[0];
    g = sp->s_color[1];
    b = sp->s_color[2];
    if( (r == 255 && g == 255 && b == 255) ||
	(r == 0 && g == 0 && b == 0) )  {
      sp->s_dmindex = DM_WHITE;
      continue;
    }

    /* First, see if this matches an existing color map entry */
    rgb = ((struct ogl_vars *)dmp->dmr_vars)->rgbtab;
    for( i = 0; i < ((struct ogl_vars *)dmp->dmr_vars)->uslots; i++, rgb++ )  {
      if( rgb->r == r && rgb->g == g && rgb->b == b )  {
	sp->s_dmindex = i;
	goto next;
      }
    }

    /* If slots left, create a new color map entry, first-come basis */
    if( ((struct ogl_vars *)dmp->dmr_vars)->uslots <
	((struct ogl_vars *)dmp->dmr_vars)->nslots )  {
      rgb =
	&((struct ogl_vars *)dmp->dmr_vars)->rgbtab[i=
	 (((struct ogl_vars *)dmp->dmr_vars)->uslots++)];
      rgb->r = r;
      rgb->g = g;
      rgb->b = b;
      sp->s_dmindex = i;
      continue;
    }
    sp->s_dmindex = DM_YELLOW;	/* Default color */
next:		
    ;
  }
}


#if IR_KNOBS
ogl_dbtext(str)
{
  Tcl_AppendResult(interp, "dm-ogl: You pressed Help key and '",
		   str, "'\n", (char *)NULL);
}
/*
 *			I R L I M I T
 *
 * Because the dials are so sensitive, setting them exactly to
 * zero is very difficult.  This function can be used to extend the
 * location of "zero" into "the dead zone".
 */
int
Ogl_irlimit(i)
int i;
{
  if( i > NOISE )
    return( i-NOISE );
  if( i < -NOISE )
    return( i+NOISE );
  return(0);
}

int
Ogl_add_tol(i)
int i;\
{
  if( i > 0 )
    return( i + NOISE );
  if( i < 0 )
    return( i - NOISE );
  return(0);
}
#endif

/*			G E N _ C O L O R
 *
 *	maps a given color into the appropriate colormap entry
 *	for both depthcued and non-depthcued mode.  In depthcued mode,
 *	Ogl_gen_color also generates the colormap ramps.  Note that in depthcued
 *	mode, DM_BLACK uses map entry 0, and does not generate a ramp for it.
 *	Non depthcued mode skips the first CMAP_BASE colormap entries.
 *
 *	This routine is not called at all if ((struct ogl_vars *)dmp->dmr_vars)->mvars.rgb is set.
 */
static void
Ogl_gen_color(dmp, c)
struct dm *dmp;
int c;
{
  if(USE_RAMP) {

    /*  Not much sense in making a ramp for DM_BLACK.  Besides
     *  which, doing so, would overwrite the bottom color
     *  map entries, which is a no-no.
     */
    if( c != DM_BLACK) {
      register int i, j;
      fastf_t r_inc, g_inc, b_inc;
      fastf_t red, green, blue;
      XColor cells[CMAP_RAMP_WIDTH];

      red = r_inc =
	((struct ogl_vars *)dmp->dmr_vars)->rgbtab[c].r * (256/CMAP_RAMP_WIDTH);
      green = g_inc =
	((struct ogl_vars *)dmp->dmr_vars)->rgbtab[c].g * (256/CMAP_RAMP_WIDTH);
      blue = b_inc =
	((struct ogl_vars *)dmp->dmr_vars)->rgbtab[c].b * (256/CMAP_RAMP_WIDTH);

#if 0
      red = rgbtab[c].r * 256;
      green = rgbtab[c].g * 256;
      blue = rgbtab[c].b * 256;
#endif
			

      if (((struct ogl_vars *)dmp->dmr_vars)->mvars.cueing_on){
	for(i = 0, j = MAP_ENTRY(c) + CMAP_RAMP_WIDTH - 1; 
	    i < CMAP_RAMP_WIDTH;
	    i++, j--, red += r_inc, green += g_inc, blue += b_inc){
	  cells[i].pixel = j;
	  cells[i].red = (short)red;
	  cells[i].green = (short)green;
	  cells[i].blue = (short)blue;
	  cells[i].flags = DoRed|DoGreen|DoBlue;
	}
      } else { /* ((struct ogl_vars *)dmp->dmr_vars)->mvars.lighting_on */ 
	for(i = 0, j = MAP_ENTRY(c) - CMAP_RAMP_WIDTH + 1; 
	    i < CMAP_RAMP_WIDTH;
	    i++, j++, red += r_inc, green += g_inc, blue += b_inc){
	  cells[i].pixel = j;
	  cells[i].red = (short)red;
	  cells[i].green = (short)green;
	  cells[i].blue = (short)blue;
	  cells[i].flags = DoRed|DoGreen|DoBlue;
	}
      }
      XStoreColors(((struct ogl_vars *)dmp->dmr_vars)->dpy,
		   ((struct ogl_vars *)dmp->dmr_vars)->cmap, cells, CMAP_RAMP_WIDTH);
    }
  } else {
    XColor cell, celltest;

    cell.pixel = c + CMAP_BASE;
    cell.red = ((struct ogl_vars *)dmp->dmr_vars)->rgbtab[c].r * 256;
    cell.green = ((struct ogl_vars *)dmp->dmr_vars)->rgbtab[c].g * 256;
    cell.blue = ((struct ogl_vars *)dmp->dmr_vars)->rgbtab[c].b * 256;
    cell.flags = DoRed|DoGreen|DoBlue;
    XStoreColor(((struct ogl_vars *)dmp->dmr_vars)->dpy,
		((struct ogl_vars *)dmp->dmr_vars)->cmap, &cell);
  }
}

static void
print_cmap(dmp)
struct dm *dmp;
{
  int i;
  XColor cell;

  for (i=0; i<112; i++){
    cell.pixel = i;
    XQueryColor(((struct ogl_vars *)dmp->dmr_vars)->dpy,
		((struct ogl_vars *)dmp->dmr_vars)->cmap, &cell);
    printf("%d  = %d %d %d\n",i,cell.red,cell.green,cell.blue);
  }
}
