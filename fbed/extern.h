/*
	SCCS id:	@(#) extern.h	2.2
	Modified: 	1/5/87 at 16:57:32
	Retrieved: 	1/5/87 at 16:58:18
	SCCS archive:	/vld/moss/src/fbed/s.extern.h

	Author:		Gary S. Moss
			U. S. Army Ballistic Research Laboratory
			Aberdeen Proving Ground
			Maryland 21005-5066
			(301)278-6647 or AV-298-6647
*/
#if ! defined( INCL_FB )
#include <fb.h>
#endif
#if ! defined( _VLD_STD_H_ )
#include <std.h>
#endif
#if ! defined( INCL_ASCII )
#include "ascii.h"
#endif
#if ! defined( INCL_FONT )
#include "font.h"
#endif
#if ! defined( INCL_TRY )
#include "try.h"
#endif

typedef struct
	{
	int	p_x;
	int	p_y;
	}
Point;

typedef struct
	{
	Point	r_origin;
	Point	r_corner;
	}
Rectangle;

typedef struct
	{
	RGBpixel  *n_buf;
	int	n_wid;
	int	n_hgt;
	}
Panel;

extern FBIO	*fbp;
extern RGBpixel	*menu_addr;
extern RGBpixel	paint;
extern Point	cursor_pos;
extern Point	image_center;
extern Point	windo_center;
extern Point	windo_anchor;
extern Try	*try_rootp;
extern char	cread_buf[BUFSIZ*10], *cptr;
extern char	macro_buf[];
extern char	*macro_ptr;
extern int	brush_sz;
extern int	gain;
extern int	pad_flag;
extern int	remembering;
extern int	report_status;
extern int	reposition_cursor;
extern int	tty;
extern int	tty_fd;
extern int	zoom_factor;
extern int	LI, CO;

extern Func_Tab	*get_Func_Name();
extern RGBpixel	*get_Fb_Panel();
extern char	*char_To_String();
extern char	*getenv();
extern char	*malloc();
extern int	add_Try();
extern int	fb_Init_Menu();
extern int	getpos();
extern int	get_Input();
extern int	do_Bitpad();
extern void	fb_Get_Pixel();
extern void	pos_close();
extern void	init_Status();
extern void	init_Tty(), restore_Tty();
extern void	prnt_Status(), prnt_Usage(), prnt_Scroll();
extern void	prnt_Rectangle();
extern void	do_Key_Cmd();

#define MAX_LN			81
#define Toggle(f)		(f) = ! (f)
#define De_Bounce_Pen()		while( do_Bitpad( &cursor_pos ) ) ;
#define BOTTOM_STATUS_AREA	2
#define TOP_SCROLL_WIN		(BOTTOM_STATUS_AREA-1)
#define PROMPT_LINE		(LI-2)
#define MACROBUFSZ		(BUFSIZ*10)
#define Malloc_Bomb() \
		fb_log(	"\"%s\"(%d) Malloc() no more space.\n", \
				__FILE__, __LINE__ \
				); \
		return	0;

#ifndef _LOCAL_
/* For production use, set to "static" */
#define _LOCAL_ /**/
#endif
