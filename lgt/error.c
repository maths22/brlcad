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
 *			E R R O R
 *
 *  Ray Tracing library and Framebuffer library, error handling routines.
 *
 *  Functions -
 *	rt_bomb		Called upon fatal RT library error.
 *	rt_log		Called to log RT library events.
 *	fb_log		Called to log FB library events.
 *
 *	Idea originated by Mike John Muuss
 */

#include <stdio.h>
#include <varargs.h>
#include "machine.h"
#include "vmath.h"
#include "raytrace.h"
#include "./lgt.h"
#include "./screen.h"
#include "./extern.h"
extern int	_doprnt();
/*
 *  		R T _ B O M B
 *  
 *  Abort the RT library
 */
void
rt_bomb(str)
char *str;
	{
	rt_log( "%s (librt.a) : Fatal error, aborting!\n", str );
	(void) fflush( stdout );
	prnt_Timer( "DUMP" );
	if( pix_buffered == B_PAGE )
		(void) fb_flush( fbiop ); /* Write out buffered image.	*/
	(void) abort();			  /* Should dump.		*/
	exit(12);
	}

#if defined( cray ) && ! defined( CRAY2 )
/* VARARGS */
void
rt_log(fmt, a,b,c,d,e,f,g,h,i)
char *fmt;
	{
	RES_ACQUIRE( &rt_g.res_syscall );		/* lock */
	if( tty && (err_file[0] == '\0' || ! strcmp( err_file, "/dev/tty" )) )
		{ /* Only move cursor and scroll if newline is output.	*/
			static int	newline = 1;
		if( CS != NULL )
			{
			(void) SetScrlReg( TOP_SCROLL_WIN, PROMPT_LINE - 1 );
			if( newline )
				{
				SCROLL_PR_MOVE();
				(void) ClrEOL();
				}
			(void) fprintf( stdout, fmt, a,b,c,d,e,f,g,h,i );
			(void) ResetScrlReg();
			}
		else
		if( DL != NULL )
			{
			if( newline )
				{
				SCROLL_DL_MOVE();
				(void) DeleteLn();
				SCROLL_PR_MOVE();
				(void) ClrEOL();
				}
			(void) fprintf( stdout, fmt, a,b,c,d,e,f,g,h,i );
			}
		else
			(void) fprintf( stdout, fmt, a,b,c,d,e,f,g,h,i );
		(void) fflush( stdout );
		/* End of line detected by existance of a newline.	*/
		newline = fmt[strlen( fmt )-1] == '\n';
		hmredraw();
		}
	else
		(void) fprintf( stderr, fmt, a,b,c,d,e,f,g,h,i );
	RES_RELEASE( &rt_g.res_syscall );		/* unlock */
	}
#else
/*
 *  		R T _  L O G
 *  
 *  Log an RT library event
 */
/* VARARGS */
void
rt_log( fmt, va_alist )
char	*fmt;
va_dcl
	{	va_list		ap;
	/* We use the same lock as malloc.  Sys-call or mem lock, really */
	RES_ACQUIRE( &rt_g.res_syscall );		/* lock */
	va_start( ap );
	if( tty && (err_file[0] == '\0' || ! strcmp( err_file, "/dev/tty" )) )
		{ /* Only move cursor and scroll if newline is output.	*/
			static int	newline = 1;
		if( CS != NULL )
			{
			(void) SetScrlReg( TOP_SCROLL_WIN, PROMPT_LINE - 1 );
			if( newline )
				{
				SCROLL_PR_MOVE();
				(void) ClrEOL();
				}
			(void) _doprnt( fmt, ap, stdout );
			(void) ResetScrlReg();
			}
		else
		if( DL != NULL )
			{
			if( newline )
				{
				SCROLL_DL_MOVE();
				(void) DeleteLn();
				SCROLL_PR_MOVE();
				(void) ClrEOL();
				}
			(void) _doprnt( fmt, ap, stdout );
			}
		else
			(void) _doprnt( fmt, ap, stdout );
		(void) fflush( stdout );
		/* End of line detected by existance of a newline.	*/
		newline = fmt[strlen( fmt )-1] == '\n';
		hmredraw();
		}
	else
		{
		(void) _doprnt( fmt, ap, stderr );
#ifdef sun
		(void) fflush( stderr );
#endif
		}
	va_end( ap );
	RES_RELEASE( &rt_g.res_syscall );		/* unlock */
	return;
	}
#endif

#if defined( cray ) && ! defined( CRAY2 )
/* VARARGS */
void
fb_log(fmt, a,b,c,d,e,f,g,h,i)
char *fmt;
	{
	RES_ACQUIRE( &rt_g.res_syscall );		/* lock */
	fprintf(stderr, fmt, a,b,c,d,e,f,g,h,i );
	RES_RELEASE( &rt_g.res_syscall );		/* unlock */
	hmredraw();
	}
#else
/*
 *		F B _ L O G
 *  
 *  Log an FB library event
 */
/* VARARGS */
void
fb_log( fmt, va_alist )
char	*fmt;
va_dcl
	{	va_list		ap;
	/* We use the same lock as malloc.  Sys-call or mem lock, really */
	RES_ACQUIRE( &rt_g.res_syscall );		/* lock */
	va_start( ap );
	if( tty && (err_file[0] == '\0' || ! strcmp( err_file, "/dev/tty" )) )
		{ /* Only move cursor and scroll if newline is output.	*/
			static int	newline = 1;
		if( CS != NULL )
			{
			(void) SetScrlReg( TOP_SCROLL_WIN, PROMPT_LINE - 1 );
			if( newline )
				{
				SCROLL_PR_MOVE();
				(void) ClrEOL();
				}
			(void) _doprnt( fmt, ap, stdout );
			(void) ResetScrlReg();
			}
		else
		if( DL != NULL )
			{
			if( newline )
				{
				SCROLL_DL_MOVE();
				(void) DeleteLn();
				SCROLL_PR_MOVE();
				(void) ClrEOL();
				}
			(void) _doprnt( fmt, ap, stdout );
			}
		else
			(void) _doprnt( fmt, ap, stdout );
		(void) fflush( stdout );
		/* End of line detected by existance of a newline.	*/
		newline = fmt[strlen( fmt )-1] == '\n';
		hmredraw();
		}
	else
		{
		(void) _doprnt( fmt, ap, stderr );
#ifdef sun
		(void) fflush( stderr );
#endif
		}
	va_end( ap );
	RES_RELEASE( &rt_g.res_syscall );		/* unlock */
	return;
	}
#endif

