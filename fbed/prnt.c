/*
	SCCS id:	%Z% %M%	%I%
	Last edit: 	%G% at %U%
	Retrieved: 	%H% at %T%
	SCCS archive:	%P%

	Author:		Gary S. Moss
			U. S. Army Ballistic Research Laboratory
			Aberdeen Proving Ground
			Maryland 21005
			(301)278-6647 or AV-283-6647
 */
#if ! defined( lint )
static
char	sccsTag[] = "%Z% %M%	%I%	last edit %G% at %U%";
#endif
#include <stdio.h>
#include <std.h>
#include <fb.h>
#include "./popup.h"
#include "./extern.h"
#define PIXEL_MOVE()		MvCursor( 7, 1 )
#define PAINT_MOVE()		MvCursor( 26, 1 )
#define STRIDE_MOVE()		MvCursor( 46, 1 )
#define BRUSH_SIZE_MOVE()	MvCursor( 63, 1 )
#define CURSOR_POS_MOVE()	MvCursor( 15, 2 )
#define PROMPT_MOVE()		MvCursor( 1, PROMPT_LINE )
#define ERROR_MOVE()		MvCursor( 1, PROMPT_LINE + 1 )
#define SCROLL_PR_MOVE()	MvCursor( 1, PROMPT_LINE - 1 )
#define SCROLL_DL_MOVE()	MvCursor( 1, BOTTOM_STATUS_AREA + 1 )

static char	*usage[] =
	{
	"",
	"fbed (%I%)",
	"",
	"Usage: fbed [-hp]",
	"",
	"options : -h   use high resolution display",
	"          -p   open the GTCO bit pad",
	0
	};

void	init_Status();
void	prnt_Status(), prnt_Usage(), prnt_Debug(), prnt_Scroll();
void	prnt_Prompt();
void	prnt_Macro();
void	prnt_FBC();

/*	p r n t _ S t a t u s ( )					*/
void
prnt_Status()
	{	Pixel		pixel;
	if( ! tty )
		return;
	fb_Get_Pixel( &pixel );
	PIXEL_MOVE();
	(void) printf( "%3d %3d %3d", pixel.red, pixel.green, pixel.blue );
	PAINT_MOVE();
	(void) printf( "%3d %3d %3d", paint.red, paint.green, paint.blue );
	STRIDE_MOVE();
	(void) printf( "%4d", gain );
	BRUSH_SIZE_MOVE();
	(void) printf( "%4d", brush_sz );
	CURSOR_POS_MOVE();
	(void) SetStandout();
	(void) printf( " line [%4d] column [%4d] ", cursor_pos.p_y, cursor_pos.p_x );
	(void) ClrStandout();
	PROMPT_MOVE();
	(void) fflush( stdout );
	return;
	}
	
static char	*screen_template[] = {
/*        1         2         3         4         5         6         7         8
012345678901234567890123456789012345678901234567890123456789012345678901234567890
 */
"Pixel[           ] Paint[           ] Stride[    ] Brush Size[    ]",
"-- FBED %I% -------------------------------------------------------------------",
0
};		

/*	i n i t _ S t a t u s ( )					*/
void
init_Status()
	{	register char	**p = screen_template;
		register int	template_co;
		char		buf[MAX_LN];
		extern int	CO;
	template_co = Min( CO, MAX_LN );
	if( ! tty )
		return;
	(void) ClrText();
	(void) HmCursor();
	while( *(p+1) )
		{
		(void) strncpy( buf, *p++, template_co );
		buf[template_co-1] = '\0';
		(void) printf( "%s\n\r", buf );
		}
	/* Last line is reverse-video if possible.			*/
	(void) SetStandout();
	(void) strncpy( buf, *p++, template_co );
	buf[template_co-1] = '\0';
	(void) printf( "%s\n\r", buf );
	(void) ClrStandout();
	(void) fflush( stdout );
	return;
	}

/*	p r n t _ U s a g e ( )
	Print usage message.
 */
void
prnt_Usage()
	{	register char	**p = usage;
	while( *p )
		(void) fprintf( stderr, "%s\n", *p++ );
	return;
	}


/*	p r n t _ P i x e l ( )						*/
void
prnt_Pixel( msg, pixelp )
char	*msg;
Pixel	*pixelp;
	{
	prnt_Scroll(	"%s : %03d %03d %03d %03d",
			msg,
			(int) pixelp->red,
			(int) pixelp->green,
			(int) pixelp->blue,
			(int) pixelp->spare
			);
	return;
	}

#include <varargs.h>
/*	p r n t _ S c r o l l ( )					*/
/* VARARGS */
void
prnt_Scroll( fmt, va_alist )
char	*fmt;
va_dcl
	{	extern char	*DL, *CS;
		va_list		ap;
	/* We use the same lock as malloc.  Sys-call or mem lock, really */
	va_start( ap );
	if( tty )
		{
		if( CS != NULL )
			{
			SetScrlReg( TOP_SCROLL_WIN, PROMPT_LINE - 1 );
			SCROLL_PR_MOVE();
			ClrEOL();
			(void) _doprnt( fmt, ap, stdout );
			ResetScrlReg();
			}
		else
		if( DL != NULL )
			{
			SCROLL_DL_MOVE();
			DeleteLn();
			SCROLL_PR_MOVE();
			ClrEOL();
			(void) _doprnt( fmt, ap, stdout );
			}
		else
			(void) _doprnt( fmt, ap, stdout );
		}
	else
		{
		(void) _doprnt( fmt, ap, stdout );
		(void) printf( "\n" );
		}
	va_end( ap );
	return;
	}

/*	p r n t _ D e b u g ( )						*/
/* VARARGS */
void
prnt_Debug( fmt, va_alist )
char	*fmt;
va_dcl
	{	va_list		ap;
	va_start( ap );
	if( tty )
		{
		ERROR_MOVE();
		ClrEOL();
		SetStandout();
		(void) _doprnt( fmt, ap, stdout );
		ClrStandout();
		(void) fflush( stdout );
		}
	else
		{
		(void) _doprnt( fmt, ap, stderr );
		(void) fprintf( stderr, "\n" );
		}
	va_end( ap );
	return;
	}

#include "fb_ik.h"
void
prnt_FBC()
	{	extern struct ik_fbc	ikfbcmem;
	prnt_Scroll(	"viewport:\t\tx [%4d]\ty [%4d]\n",
			ikfbcmem.fbc_xviewport,
			ikfbcmem.fbc_yviewport
			);
	prnt_Scroll(	"view size:\t\tx [%4d]\ty [%4d]\n",
			ikfbcmem.fbc_xsizeview,
			ikfbcmem.fbc_ysizeview
			);
	prnt_Scroll(	"window offsets:\t\tx [%4d]\ty [%4d]\n",
			ikfbcmem.fbc_xwindow,
			ikfbcmem.fbc_ywindow
			);
	prnt_Scroll(	"zoom factor:\t\tx [%4d]\ty [%4d]\n",
			ikfbcmem.fbc_xzoom,
			ikfbcmem.fbc_yzoom
			);
	prnt_Scroll(	"display rate cntrl:\th [%4d]\tv [%4d]\n",
			ikfbcmem.fbc_horiztime,
			ikfbcmem.fbc_nlines
			);
	prnt_Scroll(	"video control:\t\tL [%4d]\tH [%4d]\n",
			ikfbcmem.fbc_Lcontrol,
			ikfbcmem.fbc_Hcontrol
			);
	prnt_Scroll(	"cursor position:\tx [%4d]\ty [%4d]\n",
			ikfbcmem.fbc_xcursor,
			ikfbcmem.fbc_ycursor
			);
	return;
	}

void
prnt_Prompt( msg )
char	*msg;
	{
	PROMPT_MOVE();
	(void) ClrEOL();
	(void) printf( "%s", msg );
	(void) fflush( stdout );
	return;
	}

void
prnt_Macro( bufp )
register char	*bufp;
	{	char	prnt_buf[BUFSIZ];
		register char	*p;
	for( p = prnt_buf; *bufp != '\0'; bufp++ )
		{
		switch( *bufp )
			{
		case ESC :
			*p++ = 'M';
			*p++ = '-';
			break;
		case CR :
			*p++ = '\\';
			*p++ = 'r';
			break;
		case LF :
			*p++ = '\\';
			*p++ = 'n';
			break;
		default :
			*p++ = *bufp;
			break;
			}
		}
	*p = NUL;
	prnt_Scroll( "Macro buffer \"%s\".", prnt_buf );
	return;
	}
