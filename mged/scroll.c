/*
 *			S C R O L L . C
 *
 * Functions -
 *	scroll_display		Add a list of items to the display list
 *	scroll_select		Called by usepen() for pointing
 *
 * Authors -
 *	Bill Mermagen Jr.
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
#include "raytrace.h"
#include "./ged.h"
#include "./titles.h"
#include "./scroll.h"
#include "./dm.h"

static int	scroll_top;	/* screen loc of the first menu item */

static int	scroll_enabled = 0;

struct scroll_item *scroll_array[6];	/* Active scroll bar definitions */

/************************************************************************
 *									*
 *	First part:  scroll bar definitions				*
 *									*
 ************************************************************************/

static void sl_tol();

struct scroll_item scr_menu[] = {
	{ "xslew",	sl_tol,		&dm_values.dv_xslew },
	{ "yslew",	sl_tol,		&dm_values.dv_yslew },
	{ "zslew",	sl_tol,		&dm_values.dv_zslew },
	{ "zoom",	sl_tol,		&dm_values.dv_zoom },
	{ "xrot",	sl_tol,		&dm_values.dv_xjoy },
	{ "yrot",	sl_tol,		&dm_values.dv_yjoy },
	{ "zrot",	sl_tol,		&dm_values.dv_zjoy },
	{ "",		(void (*)())NULL, 0 }
};

static void	sl_xadc(), sl_yadc(), sl_1adc(), sl_2adc(), sl_distadc();
static double	sld_xadc, sld_yadc, sld_1adc, sld_2adc, sld_distadc;

struct scroll_item sl_adc_menu[] = {
	{ "xadc",	sl_xadc,	&sld_xadc },
	{ "yadc",	sl_yadc,	&sld_yadc },
	{ "ang 1",	sl_1adc,	&sld_1adc },
	{ "ang 2",	sl_2adc,	&sld_2adc },
	{ "tick",	sl_distadc,	&sld_distadc },
	{ "",		(void (*)())NULL, 0 }
};


/************************************************************************
 *									*
 *	Second part: Event Handlers called from menu items by buttons.c *
 *									*
 ************************************************************************/


/*
 *			S L _ H A L T _ S C R O L L
 *
 *  Reset all scroll bars to the zero position.
 */
void sl_halt_scroll()
{
	struct scroll_item      **m;
	register struct scroll_item     *mptr;

	/* Re-init scrollers */
	for( m = &scroll_array[0]; *m != SCROLL_NULL; m++ )  {
		for( mptr = *m; mptr->scroll_string[0] != '\0'; mptr++ ){
			*(mptr->scroll_val) = 0.0;
		}
	}

	dmaflag = 1;
}

/*
 *			S L _ T O G G L E _ S C R O L L
 */
void sl_toggle_scroll()
{

	scroll_enabled = (scroll_enabled == 0) ? 1 : 0;
	if( scroll_enabled )  {
		scroll_array[0] = scr_menu;
	} else {
		scroll_array[0] = SCROLL_NULL;	
		scroll_array[1] = SCROLL_NULL;	
	}
	dmaflag++;
}

/************************************************************************
 *									*
 *	Third part:  event handlers called from tables, above		*
 *									*
 *  Where the floating point value pointed to by scroll_val		*
 *  in the range -1.0 to +1.0 is the only desired result,		*
 *  everything can bel handled by sl_tol().				*
 *  When conversion to integer is required, an individual handler	*
 *  is required for each slider.					*
 *									*
 ************************************************************************/
#define SL_TOL	0.015		/* size of dead spot, 0.015 = 32/2047 */

static void sl_tol( mptr )
register struct scroll_item     *mptr;
{

	if( *mptr->scroll_val < -SL_TOL )   {
		*(mptr->scroll_val) += SL_TOL;
	} else if( *mptr->scroll_val > SL_TOL )   {
		*(mptr->scroll_val) -= SL_TOL;
	} else {
		*(mptr->scroll_val) = 0.0;
	}
}

static void sl_xadc( mptr )
register struct scroll_item     *mptr;
{
	dm_values.dv_xadc = *(mptr->scroll_val) * 2047;
}

static void sl_yadc( mptr )
register struct scroll_item     *mptr;
{
	dm_values.dv_yadc = *(mptr->scroll_val) * 2047;
}

static void sl_1adc( mptr )
register struct scroll_item     *mptr;
{
	dm_values.dv_1adc = *(mptr->scroll_val) * 2047;
}

static void sl_2adc( mptr )
register struct scroll_item     *mptr;
{
	dm_values.dv_2adc = *(mptr->scroll_val) * 2047;
}

static void sl_distadc( mptr )
register struct scroll_item     *mptr;
{
	dm_values.dv_distadc = *(mptr->scroll_val) * 2047;
}


/************************************************************************
 *									*
 *	Fourth part:  general-purpose interface mechanism		*
 *									*
 ************************************************************************/

/*
 *			S C R O L L _ D I S P L A Y
 *
 *  
 */
void
scroll_display( y_top )
int y_top;
{ 
	register int		y;
	struct scroll_item	*mptr;
	struct scroll_item	**m;
	int		xpos;

	/* XXX this should be driven by the button event */
	if( adcflag && scroll_enabled )
		scroll_array[1] = sl_adc_menu;
	else
		scroll_array[1] = SCROLL_NULL;

	y = y_top;
	scroll_top = y - SCROLL_DY / 2;

	for( m = &scroll_array[0]; *m != SCROLL_NULL; m++ )  {
	   for( mptr = *m; mptr->scroll_string[0] != '\0';
	     mptr++, y += SCROLL_DY )  {
	     	xpos = *(mptr->scroll_val) * 2047;
		dmp->dmr_puts( mptr->scroll_string, xpos, y, 0, DM_RED );
		dmp->dmr_2d_line(XMAX, y+(SCROLL_DY/2), MENUXLIM, y+(SCROLL_DY/2), 0);
	     }
	}
	if( y == y_top )  return;	/* no active menus */

	dmp->dmr_2d_line( MENUXLIM, scroll_top-1, MENUXLIM, y-(SCROLL_DY/2), 0 );
	dmp->dmr_2d_line( MENUXLIM, scroll_top, XMAX, scroll_top, 0 );
}

/*
 *			S C R O L L _ S E L E C T
 *
 *  Called with Y coordinate of pen in menu area.
 *
 * Returns:	1 if menu claims these pen co-ordinates,
 *		0 if pen is BELOW scroll
 *		-1 if pen is ABOVE scroll	(error)
 */
int
scroll_select( pen_x, pen_y )
int		pen_x;
register int	pen_y;
{ 
	register int		yy;
	struct scroll_item	**m;
	register struct scroll_item     *mptr;

	if( !scroll_enabled )  return(0);	/* not enabled */

	if( pen_y > scroll_top )
		return(-1);	/* pen above menu area */

	/*
	 * Start at the top of the list and see if the pen is
	 * above here.
	 */
	yy = scroll_top;

	for( m = &scroll_array[0]; *m != SCROLL_NULL; m++ )  {
		for( mptr = *m; mptr->scroll_string[0] != '\0'; mptr++ )  {
			yy += SCROLL_DY;
			if( pen_y <= yy )
				continue;	/* pen below this item */

			/* Record the location of scroll marker */
			*(mptr->scroll_val) = pen_x/2047.0;

			/* See if hooked function has been specified */
			if( mptr->scroll_func == ((void (*)())0) )  continue;

			(*(mptr->scroll_func))(mptr);
			dmaflag = 1;
			return( 1 );		/* scroll claims pen value */
		}
	}
	return( 0 );		/* pen below scroll area */
}
