/*
 *			D M - P L O T . C
 *
 * An unsatisfying (but useful) hack to allow GED to generate
 * UNIX-plot files that not only contain the drawn objects, but
 * also contain the faceplate display as well.
 * Mostly, a useful hack for making viewgraphs and photographs
 * of an editing session.
 * We assume that the UNIX-plot filter used can at least discard
 * the non-standard extention to specify color (a Gwyn@BRL addition).
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
#include "machine.h"
#include "vmath.h"
#include "mater.h"
#include "raytrace.h"
#include "./ged.h"
#include "./dm.h"
#include "externs.h"
#include "./solid.h"

typedef unsigned char u_char;

/* Display Manager package interface */

#define PLOTBOUND	1000.0	/* Max magnification in Rot matrix */
int	Plot_open();
void	Plot_close();
int	Plot_input();
void	Plot_prolog(), Plot_epilog();
void	Plot_normal(), Plot_newrot();
void	Plot_update();
void	Plot_puts(), Plot_2d_line(), Plot_light();
int	Plot_object();
unsigned Plot_cvtvecs(), Plot_load();
void	Plot_statechange(), Plot_viewchange(), Plot_colorchange();
void	Plot_window(), Plot_debug();

struct dm dm_Plot = {
	Plot_open, Plot_close,
	Plot_input,
	Plot_prolog, Plot_epilog,
	Plot_normal, Plot_newrot,
	Plot_update,
	Plot_puts, Plot_2d_line,
	Plot_light,
	Plot_object,
	Plot_cvtvecs, Plot_load,
	Plot_statechange,
	Plot_viewchange,
	Plot_colorchange,
	Plot_window, Plot_debug,
	0,			/* no displaylist */
	1,			/* play it safe (could be frame buffer) */
	PLOTBOUND,
	"plot", "Screen to UNIX-Plot"
};

extern struct device_values dm_values;	/* values read from devices */

static vect_t clipmin, clipmax;		/* for vector clipping */
static FILE	*up_fp;
static char	ttybuf[BUFSIZ];

/*
 * Display coordinate conversion:
 *  GED is using -2048..+2048,
 *  and we define the PLOT file to use the same space.  Easy!
 */
#define	GED_TO_PLOT(x)	(x)
#define PLOT_TO_GED(x)	(x)

/*
 *			P L O T _ O P E N
 *
 * Fire up the display manager, and the display processor.
 *
 */
Plot_open()
{
	char line[64];

	(void)printf("UNIX-Plot filter [pl-fb]? ");
	(void)gets( line );		/* Null terminated */
	if( feof(stdin) )  quit();
	if( line[0] != '\0' )  {
		if( (up_fp = popen( line, "w" )) == NULL )  {
			perror(line);
			return(1);		/* BAD */
		}
	} else {
		if( (up_fp = popen("pl-fb", "w")) == NULL )  {
			perror("pl-fb");
			return(1);	/* BAD */
		}
	}
	setbuf( up_fp, ttybuf );
	pl_space( up_fp, -2048, -2048, 2048, 2048 );
	return(0);			/* OK */
}

/*
 *  			P L O T _ C L O S E
 *  
 *  Gracefully release the display.
 */
void
Plot_close()
{
	(void)fflush(up_fp);
	pclose(up_fp);			/* close pipe, eat dead children */
}

/*
 *			P L O T _ P R O L O G
 *
 * There are global variables which are parameters to this routine.
 */
void
Plot_prolog()
{
	if( !dmaflag )
		return;

	/* We expect the screen to be blank so far, from last frame flush */

	/* Put the center point up */
	pl_move( up_fp,  0, 0 );
	pl_cont( up_fp,  0, 0 );
}

/*
 *			P L O T _ E P I L O G
 */
void
Plot_epilog()
{
	pl_flush( up_fp );			/* BRL-specific command */
	pl_erase( up_fp );			/* forces drawing */
	(void)fflush( up_fp );
	return;
}

/*
 *  			P L O T _ N E W R O T
 *  Stub.
 */
/* ARGSUSED */
void
Plot_newrot(mat)
mat_t mat;
{
	return;
}

/*
 *  			P L O T _ O B J E C T
 *  
 *  Set up for an object, transformed as indicated, and with an
 *  object center as specified.  The ratio of object to screen size
 *  is passed in as a convienience.
 *
 *  Returns 0 if object could be drawn, !0 if object was omitted.
 */
/* ARGSUSED */
int
Plot_object( sp, mat, ratio, white )
register struct solid *sp;
mat_t mat;
double ratio;
{
	static vect_t			last;
	register struct rt_vlist	*vp;
	int useful = 0;

	if( white )  {
		pl_linmod( up_fp, "longdashed" );
	} else {
		if( sp->s_soldash )
			pl_linmod( up_fp, "dotdashed");
		else
			pl_linmod( up_fp, "solid");
	}

	for( RT_LIST_FOR( vp, rt_vlist, &(sp->s_vlist) ) )  {
		register int	i;
		register int	nused = vp->nused;
		register int	*cmd = vp->cmd;
		register point_t *pt = vp->pt;
		for( i = 0; i < nused; i++,cmd++,pt++ )  {
			static vect_t	start, fin;
			switch( *cmd )  {
			case RT_VLIST_POLY_START:
				continue;
			case RT_VLIST_POLY_MOVE:
			case RT_VLIST_LINE_MOVE:
				/* Move, not draw */
				MAT4X3PNT( last, model2view, *pt );
				continue;
			case RT_VLIST_POLY_DRAW:
			case RT_VLIST_POLY_END:
			case RT_VLIST_LINE_DRAW:
				/* draw */
				MAT4X3PNT( fin, model2view, *pt );
				VMOVE( start, last );
				VMOVE( last, fin );
				break;
			}
			if(
				vclip( start, fin, clipmin, clipmax ) == 0
			)  continue;

			pl_color( up_fp,
				sp->s_color[0],
				sp->s_color[1],
				sp->s_color[2] );
			pl_line( up_fp, 
				(int)( start[0] * 2047 ),
				(int)( start[1] * 2047 ),
				(int)( fin[0] * 2047 ),
				(int)( fin[1] * 2047 ) );
			useful = 1;
		}
	}
	return(useful);
}

/*
 *			P L O T _ N O R M A L
 *
 * Restore the display processor to a normal mode of operation
 * (ie, not scaled, rotated, displaced, etc).
 * Turns off windowing.
 */
void
Plot_normal()
{
	return;
}

/*
 *			P L O T _ U P D A T E
 *
 * Transmit accumulated displaylist to the display processor.
 */
void
Plot_update()
{
	(void)fflush(up_fp);
}

/*
 *			P L O T _ P U T S
 *
 * Output a string into the displaylist.
 * The starting position of the beam is as specified.
 */
/* ARGSUSED */
void
Plot_puts( str, x, y, size, color )
register u_char *str;
{
	switch( color )  {
	case DM_BLACK:
		pl_color( up_fp,  0, 0, 0 );
		break;
	case DM_RED:
		pl_color( up_fp,  255, 0, 0 );
		break;
	case DM_BLUE:
		pl_color( up_fp,  0, 255, 0 );
		break;
	case DM_YELLOW:
		pl_color( up_fp,  255, 255, 0 );
		break;
	case DM_WHITE:
		pl_color( up_fp,  255, 255, 255 );
		break;
	}
	pl_move( up_fp, x,y);
	pl_label( up_fp, str);
}

/*
 *			P L O T _ 2 D _ G O T O
 *
 */
void
Plot_2d_line( x1, y1, x2, y2, dashed )
int x1, y1;
int x2, y2;
int dashed;
{
	pl_color( up_fp,  255, 255, 0 );	/* Yellow */
	if( dashed )
		pl_linmod( up_fp, "dotdashed");
	else
		pl_linmod( up_fp, "solid");
	pl_move( up_fp, x1,y1);
	pl_cont( up_fp, x2,y2);
}

/*
 *			P L O T _ I N P U T
 *
 * Execution must suspend in this routine until a significant event
 * has occured on either the command stream,
 * unless "noblock" is set.
 *
 * Returns:
 *	0 if no command waiting to be read,
 *	1 if command is waiting to be read.
 */
Plot_input( cmd_fd, noblock )
{
	register long readfds;

	/*
	 * Check for input on the keyboard or on the polled registers.
	 *
	 * Suspend execution until either
	 *  1)  User types a full line
	 *  2)  The timelimit on SELECT has expired
	 *
	 * If a RATE operation is in progress (zoom, rotate, slew)
	 * in which we still have to update the display,
	 * do not suspend execution.
	 */
	readfds = (1<<cmd_fd);
	if( noblock )
		readfds = bsdselect( readfds, 0, 0 );
	else
		readfds = bsdselect( readfds, 30*60, 0 );	/* 30 mins */

	dm_values.dv_penpress = 0;

	if( readfds & (1<<cmd_fd) )
		return(1);		/* command awaits */
	else
		return(0);		/* just peripheral stuff */
}

/* 
 *			P L O T _ L I G H T
 */
/* ARGSUSED */
void
Plot_light( cmd, func )
int cmd;
int func;			/* BE_ or BV_ function */
{
	return;
}

/* ARGSUSED */
unsigned
Plot_cvtvecs( sp )
struct solid *sp;
{
	return( 0 );
}

/*
 * Loads displaylist
 */
unsigned
Plot_load( addr, count )
unsigned addr, count;
{
	(void)printf("Plot_load(x%x, %d.)\n", addr, count );
	return( 0 );
}

void
Plot_statechange()
{
}

void
Plot_viewchange()
{
}

void
Plot_colorchange()
{
	color_soltab();		/* apply colors to the solid table */
}

/* ARGSUSED */
void
Plot_debug(lvl)
{
	(void)fflush(up_fp);
	printf("flushed\n");
}

void
Plot_window(w)
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
