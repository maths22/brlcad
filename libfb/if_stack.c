/*
 *			I F _ S T A C K . C
 *
 *  Allows multiple frame buffers to be ganged together.
 *
 *  Authors -
 *	Phillip Dykstra
 *
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *
 * Copyright Notice -
 *	This software is Copyright (C) 1986 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include "fb.h"
#include "./fblocal.h"

_LOCAL_ int	stk_open(),
		stk_close(),
		stk_reset(),
		stk_clear(),
		stk_read(),
		stk_write(),
		stk_rmap(),
		stk_wmap(),
		stk_viewport(),
		stk_window(),
		stk_zoom(),
		stk_setcursor(),
		stk_cursor(),
		stk_scursor(),
		stk_readrect(),
		stk_writerect(),
		stk_flush(),
		stk_free(),
		stk_help();

/* This is the ONLY thing that we normally "export" */
FBIO stk_interface =  {
	stk_open,		/* device_open		*/
	stk_close,		/* device_close		*/
	stk_reset,		/* device_reset		*/
	stk_clear,		/* device_clear		*/
	stk_read,		/* buffer_read		*/
	stk_write,		/* buffer_write		*/
	stk_rmap,		/* colormap_read	*/
	stk_wmap,		/* colormap_write	*/
	stk_viewport,		/* viewport_set		*/
	stk_window,		/* window_set		*/
	stk_zoom,		/* zoom_set		*/
	stk_setcursor,		/* curs_set		*/
	stk_cursor,		/* cursor_move_memory_addr */
	stk_scursor,		/* cursor_move_screen_addr */
	stk_readrect,		/* readrect		*/
	stk_writerect,		/* writerect		*/
	stk_flush,		/* flush output		*/
	stk_free,		/* free resources	*/
	stk_help,		/* help function	*/
	"Multiple Device Stacker", /* device description */
	1024*32,		/* max width		*/
	1024*32,		/* max height		*/
	"/dev/stack",		/* short device name	*/
	4,			/* default/current width  */
	4,			/* default/current height */
	-1,			/* file descriptor	*/
	PIXEL_NULL,		/* page_base		*/
	PIXEL_NULL,		/* page_curp		*/
	PIXEL_NULL,		/* page_endp		*/
	-1,			/* page_no		*/
	0,			/* page_dirty		*/
	0L,			/* page_curpos		*/
	0L,			/* page_pixels		*/
	0			/* debug		*/
};

/* List of interface struct pointers, one per dev */
#define	MAXIF	32
struct	stkinfo {
	FBIO	*if_list[MAXIF];
};
#define	SI(ptr) ((struct stkinfo *)((ptr)->u1.p))
#define	SIL(ptr) ((ptr)->u1.p)		/* left hand side version */

_LOCAL_ int
stk_open( ifp, file, width, height )
FBIO	*ifp;
char	*file;
int	width, height;
{
	int	i;
	char	*cp;
	char	devbuf[80];

	/* Check for /dev/stack */
	if( strncmp(file,ifp->if_name,strlen("/dev/stack")) != 0 ) {
		fb_log( "stack_dopen: Bad device %s\n", file );
		return(-1);
	}

	if( (SIL(ifp) = (char *)calloc( 1, sizeof(struct stkinfo) )) == NULL )  {
		fb_log("stack_dopen:  stkinfo malloc failed\n");
		return(-1);
	}

	cp = &file[strlen("/dev/stack")];
	while( *cp != NULL && *cp != ' ' && *cp != '\t' )
		cp++;	/* skip suffix */

	/* special check for a possibly user confusing case */
	if( *cp == NULL ) {
		fb_log("stack_dopen: No devices specified\n");
		fb_log("Usage: /dev/stack device_one; device_two; [etc]\n");
		return(-1);
	}

	/* Chamelion mode:  be any size he wants */
	ifp->if_width = width;
	ifp->if_height = height;
	i = 0;
	while( i < MAXIF && *cp != NULL ) {
		register char	*dp;
		register FBIO	*fbp;

		while( *cp != NULL && (*cp == ' ' || *cp == '\t' || *cp == ';') )
			cp++;	/* skip blanks and separators */
		if( *cp == NULL )
			break;
		dp = devbuf;
		while( *cp != NULL && *cp != ';' )
			*dp++ = *cp++;
		*dp = NULL;
		if( (fbp = fb_open(devbuf, ifp->if_width, ifp->if_height)) != FBIO_NULL )  {
			/* Use first default size found */
			if( ifp->if_width == 0 )
				ifp->if_width = fbp->if_width;
			if( ifp->if_height == 0 )
				ifp->if_height = fbp->if_height;

			/* Track the minimum of all the sizes availible */
			if( fbp->if_width < ifp->if_width )
				ifp->if_width = fbp->if_width;
			if( fbp->if_height < ifp->if_height )
				ifp->if_height = fbp->if_height;
			if( fbp->if_max_width < ifp->if_max_width )
				ifp->if_max_width = fbp->if_max_width;
			if( fbp->if_max_height < ifp->if_max_height )
				ifp->if_max_height = fbp->if_max_height;
			SI(ifp)->if_list[i++] = fbp;
		}
	}
	if( i > 0 )
		return(0);
	else
		return(-1);
}

_LOCAL_ int
stk_close( ifp )
FBIO	*ifp;
{
	register FBIO **ip = SI(ifp)->if_list;

	while( *ip != (FBIO *)NULL ) {
		fb_close( (*ip) );
		ip++;
	}

	return(0);
}

_LOCAL_ int
stk_reset( ifp )
FBIO	*ifp;
{
	register FBIO **ip = SI(ifp)->if_list;

	while( *ip != (FBIO *)NULL ) {
		fb_reset( (*ip) );
		ip++;
	}

	return(0);
}

_LOCAL_ int
stk_clear( ifp, pp )
FBIO	*ifp;
RGBpixel	*pp;
{
	register FBIO **ip = SI(ifp)->if_list;

	while( *ip != (FBIO *)NULL ) {
		fb_clear( (*ip), pp );
		ip++;
	}

	return(0);
}

_LOCAL_ int
stk_read( ifp, x, y, pixelp, count )
FBIO	*ifp;
int	x, y;
RGBpixel	*pixelp;
int	count;
{
	register FBIO **ip = SI(ifp)->if_list;

	if( *ip != (FBIO *)NULL ) {
		fb_read( (*ip), x, y, pixelp, count );
	}

	return(count);
}

_LOCAL_ int
stk_write( ifp, x, y, pixelp, count )
FBIO	*ifp;
int	x, y;
RGBpixel	*pixelp;
int	count;
{
	register FBIO **ip = SI(ifp)->if_list;

	while( *ip != (FBIO *)NULL ) {
		fb_write( (*ip), x, y, pixelp, count );
		ip++;
	}

	return(count);
}

/*
 *			S T K _ R E A D R E C T
 */
_LOCAL_ int
stk_readrect( ifp, xmin, ymin, width, height, pp )
FBIO	*ifp;
int	xmin, ymin;
int	width, height;
RGBpixel	*pp;
{
	register FBIO **ip = SI(ifp)->if_list;

	if( *ip != (FBIO *)NULL ) {
		(void)fb_readrect( (*ip), xmin, ymin, width, height, pp );
	}

	return( width*height );
}

_LOCAL_ int
stk_writerect( ifp, xmin, ymin, width, height, pp )
FBIO	*ifp;
int	xmin, ymin;
int	width, height;
RGBpixel	*pp;
{
	register FBIO **ip = SI(ifp)->if_list;

	while( *ip != (FBIO *)NULL ) {
		(void)fb_writerect( (*ip), xmin, ymin, width, height, pp );
		ip++;
	}

	return( width*height );
}

_LOCAL_ int
stk_rmap( ifp, cmp )
FBIO	*ifp;
ColorMap	*cmp;
{
	register FBIO **ip = SI(ifp)->if_list;

	if( *ip != (FBIO *)NULL ) {
		fb_rmap( (*ip), cmp );
	}

	return(0);
}

_LOCAL_ int
stk_wmap( ifp, cmp )
FBIO	*ifp;
ColorMap	*cmp;
{
	register FBIO **ip = SI(ifp)->if_list;

	while( *ip != (FBIO *)NULL ) {
		fb_wmap( (*ip), cmp );
		ip++;
	}

	return(0);
}

_LOCAL_ int
stk_viewport( ifp, left, top, right, bottom )
FBIO	*ifp;
int	left, top, right, bottom;
{
	register FBIO **ip = SI(ifp)->if_list;

	while( *ip != (FBIO *)NULL ) {
		fb_viewport( (*ip), left, top, right, bottom );
		ip++;
	}

	return(0);
}

_LOCAL_ int
stk_window( ifp, x, y )
FBIO	*ifp;
int	x, y;
{
	register FBIO **ip = SI(ifp)->if_list;

	while( *ip != (FBIO *)NULL ) {
		fb_window( (*ip), x, y );
		ip++;
	}

	return(0);
}

_LOCAL_ int
stk_zoom( ifp, x, y )
FBIO	*ifp;
int	x, y;
{
	register FBIO **ip = SI(ifp)->if_list;

	while( *ip != (FBIO *)NULL ) {
		fb_zoom( (*ip), x, y );
		ip++;
	}

	return(0);
}

_LOCAL_ int
stk_setcursor( ifp, bits, xbits, ybits, xorig, yorig )
FBIO	*ifp;
unsigned char *bits;
int	xbits, ybits;
int	xorig, yorig;
{
	register FBIO **ip = SI(ifp)->if_list;

	while( *ip != (FBIO *)NULL ) {
		fb_setcursor( (*ip), bits, xbits, ybits, xorig, yorig );
		ip++;
	}

	return(0);
}

_LOCAL_ int
stk_cursor( ifp, mode, x, y )
FBIO	*ifp;
int	mode;
int	x, y;
{
	register FBIO **ip = SI(ifp)->if_list;

	while( *ip != (FBIO *)NULL ) {
		fb_cursor( (*ip), mode, x, y );
		ip++;
	}

	return(0);
}

_LOCAL_ int
stk_scursor( ifp, mode, x, y )
FBIO	*ifp;
int	mode;
int	x, y;
{
	register FBIO **ip = SI(ifp)->if_list;

	while( *ip != (FBIO *)NULL ) {
		fb_scursor( (*ip), mode, x, y );
		ip++;
	}

	return(0);
}

_LOCAL_ int
stk_flush( ifp )
FBIO	*ifp;
{
	register FBIO **ip = SI(ifp)->if_list;

	while( *ip != (FBIO *)NULL ) {
		fb_flush( (*ip) );
		ip++;
	}

	return(0);
}

_LOCAL_ int
stk_free( ifp )
FBIO	*ifp;
{
	register FBIO **ip = SI(ifp)->if_list;

	while( *ip != (FBIO *)NULL ) {
		fb_free( (*ip) );
		ip++;
	}

	return(0);
}

_LOCAL_ int
stk_help( ifp )
FBIO	*ifp;
{
	register FBIO **ip = SI(ifp)->if_list;
	int	i;

	fb_log("Device: /dev/stack\n");
	fb_log("Usage: /dev/stack device_one; device_two; [etc]\n");

	i = 0;
	while( *ip != (FBIO *)NULL ) {
		fb_log("=== Current stack device #%d ===\n", i++);
		fb_help( (*ip) );
		ip++;
	}

	return(0);
}
