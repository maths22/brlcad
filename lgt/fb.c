/*
	Author:		Gary S. Moss
			U. S. Army Ballistic Research Laboratory
			Aberdeen Proving Ground
			Maryland 21005-5066
			(301)278-6647 or AV-298-6647
*/
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif
/*
	Originally extracted from SCCS archive:
		SCCS id:	@(#) fb.c	2.1
		Modified: 	12/10/86 at 16:03:05	G S M
		Retrieved: 	2/4/87 at 08:53:15
		SCCS archive:	/vld/moss/src/lgt/s.fb.c
*/

#include <stdio.h>
#include <fcntl.h>
#include <fb.h>
#include "./extern.h"
#include "./screen.h"
int		zoom;	/* Current zoom factor of frame buffer.		*/
int		fb_Setup();
void		fb_Zoom_Window();

/*	f b _ S e t u p ( )						*/
int
fb_Setup( file, size )
char	*file;
int	size;
	{
#ifdef sgi
		static int	sgi_open = FALSE;
		static FBIO	*sgi_iop;
	if( sgi_open )
		{
		if( file[0] == '\0' || strncmp( file, "/dev/sgi", 8 ) == 0 )
			{
			fbiop = sgi_iop;
			return	1; /* Only open SGI once.		*/
			}
		}
#endif
	size = size > 512 ? 1024 : 512;
	if(	(fbiop = fb_open(	file[0] == '\0' ? NULL : file,
					size, size
					)
		) == FBIO_NULL
	    ||	fb_ioinit( fbiop ) == -1
	    ||	fb_wmap( fbiop, COLORMAP_NULL ) == -1
		)
		return	0;
	(void) fb_setcursor( fbiop, arrowcursor, 16, 16, 0, 0 );
	(void) fb_cursor( fbiop, tracking_cursor, size/2, size/2 );
#ifdef sgi
	if( strncmp( fbiop->if_name, "/dev/sgi", 8 ) == 0 )
		{
		sgi_open = TRUE;
		sgi_iop = fbiop;
		}
#endif
	return	1;
	}

/*	f b _ Z o o m _ W i n d o w ( )					*/
void
fb_Zoom_Window()
	{	register int	xpos, ypos;
	zoom = fb_getwidth( fbiop ) / grid_sz;
	xpos = ypos = grid_sz / 2;
	if( fb_zoom( fbiop, zoom, zoom ) == -1 )
		rt_log( "Can not set zoom <%d,%d>.\n", zoom, zoom );
	if( x_fb_origin >= grid_sz )
		xpos += x_fb_origin;
	if( y_fb_origin >= grid_sz )
		ypos += y_fb_origin;
	if( fb_viewport( fbiop, 0, 0, grid_sz, grid_sz ) == -1 )
		rt_log( "Can not set viewport {<%d,%d>,<%d,%d>}.\n",
			0, 0, grid_sz, grid_sz
			);
	if( fb_window( fbiop, xpos, ypos ) == -1 )
		rt_log( "Can not set window <%d,%d>.\n", xpos, ypos );
	return;
	}
