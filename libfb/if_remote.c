/*
 *		I F _ R E M O T E . C
 *
 *  Remote libfb interface.
 *
 *  Duplicates the functions in libfb via communication
 *  with a remote server (rfbd).
 *
 *  Authors -
 *	Phillip Dykstra
 *	Gary S. Moss
 *
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1986 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>

#ifdef BSD
#include <sys/types.h>
#include <sys/uio.h>		/* for struct iovec */
#include <netinet/in.h>		/* for htons(), etc */
#endif

#if BSD >= 43
# include <sys/socket.h>
#endif

#ifdef BSD
#include <strings.h>
#else
#include <string.h>
#endif

#include "pkg.h"
#include "./pkgtypes.h"
#include "fb.h"
#include "./fblocal.h"

#define NET_LONG_LEN	4	/* # bytes to network long */

#define MAX_HOSTNAME	128
#define	PCP(ptr)	((struct pkg_conn *)((ptr)->u1.p))
#define	PCPL(ptr)	((ptr)->u1.p)	/* left hand side version */

/* Package Handlers. */
static void	pkgerror();	/* error message handler */
static struct pkg_switch pkgswitch[] = {
	{ MSG_ERROR, pkgerror, "Error Message" },
	{ 0, NULL, NULL }
};

_LOCAL_ int	rem_open(),
		rem_close(),
		rem_clear(),
		rem_read(),
		rem_write(),
		rem_rmap(),
		rem_wmap(),
		rem_window(),
		rem_zoom(),
		rem_cursor(),
		rem_scursor(),
		rem_readrect(),
		rem_writerect(),
		rem_flush(),
		rem_free(),
		rem_help();

FBIO remote_interface = {
	rem_open,
	rem_close,
	fb_null,			/* fb_reset */
	rem_clear,
	rem_read,
	rem_write,
	rem_rmap,
	rem_wmap,
	fb_null,			/* fb_viewport */
	rem_window,
	rem_zoom,
	fb_null,			/* fb_setcursor */
	rem_cursor,
	rem_scursor,
	rem_readrect,
	rem_writerect,
	rem_flush,
	rem_free,
	rem_help,
	"Remote Device Interface",	/* should be filled in	*/
	1024,				/* " */
	1024,				/* " */
	"host:[dev]",
	512,
	512,
	-1,
	PIXEL_NULL,
	PIXEL_NULL,
	PIXEL_NULL,
	-1,
	0,
	0L,
	0L,
	0
};

void	pkg_queue(), flush_queue();
static	struct pkg_conn *pcp;

/* from getput.c */
extern unsigned short fbgetshort();
extern unsigned long fbgetlong();
extern char *fbputshort(), *fbputlong();

/* True if the non-null string s is all digits */
static int
numeric( s )
register char *s;
{
	if( s == (char *)0 || *s == 0 )
		return	0;

	while( *s ) {
		if( *s < '0' || *s > '9' )
			return 0;
		s++;
	}

	return 1;
}

/*
 *  Break up a file specification into its component parts.
 *  We try to be infinitely flexible here which makes this complicated.
 *  Handle any of the following:
 *
 *	File			Host		Port		Dev
 *	0			localhost	0		NULL
 *	0:[dev]			localhost	0		dev
 *	:0			localhost	0		NULL
 *	host:[dev]		host		remotefb	dev
 *	host:0			host		0		NULL
 *	host:0:[dev]		host		0		dev
 *
 *  Return -1 on error, else 0.
 */
parse_file( file, host, portp, device )
char *file;	/* input file spec */
char *host;	/* host part */
int  *portp;	/* port number */
char *device;	/* device part */
{
	int	port;
	char	prefix[256];
	char	*rest;
	char	*dev;
	char	*colon;

	if( numeric(file) ) {
		/* 0 */
		port = atoi(file);
		strcpy( host, "localhost" );
		dev = "";
		goto done;
	}
	if( (colon = strchr(file, ':')) != NULL ) {
		strncpy( prefix, file, colon-file );
		prefix[colon-file] = NULL;
		rest = colon+1;
		if( numeric(prefix) ) {
			/* 0:[dev] */
			port = atoi(prefix);
			strcpy( host, "localhost" );
			dev = rest;
			goto done;
		} else {
			/* :[dev] or host:[dev] */
			strcpy( host, prefix );
			if( numeric(rest) ) {
				/* :0 or host:0 */
				port = atoi(rest);
				dev = "";
				goto done;
			} else {
				/* check for [host]:0:[dev] */
				if( (colon = strchr(rest, ':')) != NULL ) {
					strncpy( prefix, rest, colon-rest );
					prefix[colon-rest] = NULL;
					if( numeric(prefix) ) {
						port = atoi(prefix);
						dev = colon+1;
						goto done;
					} else {
						/* No port given! */
						dev = rest;
						port = 5558;	/*XXX*/
						goto done;
					}
				} else {
					/* No port given */
					dev = rest;
					port = 5558;		/*XXX*/
					goto done;
				}
			}
		}
	}
	/* bad file spec */
	return -1;

done:
	/* Default hostname */
	if( strlen(host) == 0 ) {
		strcpy( host, "localhost" );
	}
	/* Magic port number mapping */
	if( port < 0 )
		return -1;
	if( port < 1024 )
		port += 5559;
	/*
	 * In the spirit of X, let "unix" be an alias for the "localhost".
	 * Eventually this may invoke UNIX Domain PKG (if we can figure
	 * out what to do about socket pathnames).
	 */
	if( strcmp(host,"unix") == 0 )
		strcpy( host, "localhost" );

	/* copy out port and device */
	*portp = port;
	strcpy( device, dev );

	return( 0 );
}

/*
 * Open a connection to the remotefb.
 * We send NET_LONG_LEN bytes of mode, NET_LONG_LEN bytes of size, then the
 *  devname (or NULL if default).
 */
_LOCAL_ int
rem_open( ifp, file, width, height )
register FBIO	*ifp;
register char	*file;
int	width, height;
{
	register int	i;
	struct pkg_conn *pc;
	char	buf[128];
	char	hostname[MAX_HOSTNAME];
	char	portname[MAX_HOSTNAME];
	char	device[MAX_HOSTNAME];
	int	port;

	hostname[0] = NULL;
	portname[0] = NULL;
	port = 0;

	if( file == NULL || parse_file(file, hostname, &port, device) < 0 ) {
		/* too wild for our tastes */
		fb_log( "rem_open: bad device name \"%s\"\n",
			file == NULL ? "(null)" : file );
		return	-1;
	}
	/*printf("hostname = \"%s\", port = %d, device = \"%s\"\n", hostname, port, device );*/

	if( port != 5558 ) {
		sprintf(portname, "%d", port);
		if( (pc = pkg_open( hostname, portname, 0, 0, 0, pkgswitch, fb_log )) == PKC_ERROR ) {
			fb_log(	"rem_open: can't connect to fb server on host \"%s\", port \"%s\".\n",
				hostname, portname );
			return	-1;
		}
	} else
	if( (pc = pkg_open( hostname, "remotefb", 0, 0, 0, pkgswitch, fb_log )) == PKC_ERROR &&
	    (pc = pkg_open( hostname, "5558", 0, 0, 0, pkgswitch, fb_log )) == PKC_ERROR ) {
		fb_log(	"rem_open: can't connect to remotefb server on host \"%s\".\n",
			hostname );
		return	-1;
	}
	PCPL(ifp) = (char *)pc;			/* stash in u1 */
	ifp->if_fd = pc->pkc_fd;		/* unused */

#if BSD >= 43
	{
		int	n;
		int	val = 32767;
		n = setsockopt( pc->pkc_fd, SOL_SOCKET,
			SO_SNDBUF, (char *)&val, sizeof(val) );
		if( n < 0 )  perror("setsockopt: SO_SNDBUF");
	}
#endif

	(void)fbputlong( width, &buf[0*NET_LONG_LEN] );
	(void)fbputlong( height, &buf[1*NET_LONG_LEN] );
	(void) strcpy( &buf[8], device );
	pkg_send( MSG_FBOPEN, buf, strlen(device)+2*NET_LONG_LEN, pc );

	/* return code, max_width, max_height, width, height as longs */
	pkg_waitfor( MSG_RETURN, buf, sizeof(buf), pc );
	ifp->if_max_width = fbgetlong( &buf[1*NET_LONG_LEN] );
	ifp->if_max_height = fbgetlong( &buf[2*NET_LONG_LEN] );
	ifp->if_width = fbgetlong( &buf[3*NET_LONG_LEN] );
	ifp->if_height = fbgetlong( &buf[4*NET_LONG_LEN] );
	if( fbgetlong( &buf[0*NET_LONG_LEN] ) != 0 )
		return(-1);		/* fail */
	return( 0 );			/* OK */
}

_LOCAL_ int
rem_close( ifp )
FBIO	*ifp;
{
	char	buf[NET_LONG_LEN+1];

	/* send a close package to remote */
	pkg_send( MSG_FBCLOSE, (char *)0, 0, PCP(ifp) );
	pkg_waitfor( MSG_RETURN, buf, NET_LONG_LEN, PCP(ifp) );
	pkg_close( PCP(ifp) );
	return( fbgetlong( &buf[0*NET_LONG_LEN] ) );
}

_LOCAL_ int
rem_free( ifp )
FBIO	*ifp;
{
	char	buf[NET_LONG_LEN+1];

	/* send a free package to remote */
	pkg_send( MSG_FBFREE, (char *)0, 0, PCP(ifp) );
	pkg_waitfor( MSG_RETURN, buf, NET_LONG_LEN, PCP(ifp) );
	pkg_close( PCP(ifp) );
	return( fbgetlong( &buf[0*NET_LONG_LEN] ) );
}

_LOCAL_ int
rem_clear( ifp, bgpp )
FBIO	*ifp;
RGBpixel	*bgpp;
{
	char	buf[NET_LONG_LEN+1];

	/* send a clear package to remote */
	if( bgpp == PIXEL_NULL )  {
		buf[0] = buf[1] = buf[2] = 0;	/* black */
	} else {
		buf[0] = (*bgpp)[RED];
		buf[1] = (*bgpp)[GRN];
		buf[2] = (*bgpp)[BLU];
	}
	pkg_send( MSG_FBCLEAR, buf, 3, PCP(ifp) );
	pkg_waitfor( MSG_RETURN, buf, NET_LONG_LEN, PCP(ifp) );
	return( fbgetlong( buf ) );
}

/*
 *  Send as longs:  x, y, num
 */
_LOCAL_ int
rem_read( ifp, x, y, pixelp, num )
register FBIO	*ifp;
int		x, y;
RGBpixel	*pixelp;
int		num;
{
	int	ret;
	char	buf[3*NET_LONG_LEN+1];

	if( num <= 0 )
		return(0);
	/* Send Read Command */
	(void)fbputlong( x, &buf[0*NET_LONG_LEN] );
	(void)fbputlong( y, &buf[1*NET_LONG_LEN] );
	(void)fbputlong( num, &buf[2*NET_LONG_LEN] );
	pkg_send( MSG_FBREAD, buf, 3*NET_LONG_LEN, PCP(ifp) );

	/* Get response;  0 len means failure */
	pkg_waitfor( MSG_RETURN, (char *)pixelp,
			num*sizeof(RGBpixel), PCP(ifp) );
	if( (ret=PCP(ifp)->pkc_len) == 0 )  {
		fb_log( "rem_read: read %d at <%d,%d> failed.\n",
			num, x, y );
		return(-1);
	}
	return( ret/sizeof(RGBpixel) );
}

/*
 * As longs, x, y, num
 */
_LOCAL_ int
rem_write( ifp, x, y, pixelp, num )
register FBIO	*ifp;
int		x, y;
RGBpixel	*pixelp;
int		num;
{
	int	ret;
	char	buf[3*NET_LONG_LEN+1];

	/* Send Write Command */
	(void)fbputlong( x, &buf[0*NET_LONG_LEN] );
	(void)fbputlong( y, &buf[1*NET_LONG_LEN] );
	(void)fbputlong( num, &buf[2*NET_LONG_LEN] );
	pkg_2send( MSG_FBWRITE+MSG_NORETURN,
		buf, 3*NET_LONG_LEN,
		(char *)pixelp, num*sizeof(RGBpixel),
		PCP(ifp) );

	return	num;	/* No error return, sacrificed for speed. */
}

/*
 *			R E M _ R E A D R E C T
 */
_LOCAL_ int
rem_readrect( ifp, xmin, ymin, width, height, pp )
FBIO	*ifp;
int	xmin, ymin;
int	width, height;
RGBpixel	*pp;
{
	int	num;
	int	ret;
	char	buf[4*NET_LONG_LEN+1];

	num = width*height;
	if( num <= 0 )
		return(0);
	/* Send Read Command */
	(void)fbputlong( xmin, &buf[0*NET_LONG_LEN] );
	(void)fbputlong( ymin, &buf[1*NET_LONG_LEN] );
	(void)fbputlong( width, &buf[2*NET_LONG_LEN] );
	(void)fbputlong( height, &buf[3*NET_LONG_LEN] );
	pkg_send( MSG_FBREADRECT, buf, 4*NET_LONG_LEN, PCP(ifp) );

	/* Get response;  0 len means failure */
	pkg_waitfor( MSG_RETURN, (char *)pp,
			num*sizeof(RGBpixel), PCP(ifp) );
	if( (ret=PCP(ifp)->pkc_len) == 0 )  {
		fb_log( "rem_rectread: read %d at <%d,%d> failed.\n",
			num, xmin, ymin );
		return(-1);
	}
	return( ret/sizeof(RGBpixel) );
}

/*
 *			R E M _ W R I T E R E C T
 */
_LOCAL_ int
rem_writerect( ifp, xmin, ymin, width, height, pp )
FBIO	*ifp;
int	xmin, ymin;
int	width, height;
RGBpixel	*pp;
{
	int	num;
	int	ret;
	char	buf[4*NET_LONG_LEN+1];

	num = width*height;
	if( num <= 0 )
		return(0);

	/* Send Write Command */
	(void)fbputlong( xmin, &buf[0*NET_LONG_LEN] );
	(void)fbputlong( ymin, &buf[1*NET_LONG_LEN] );
	(void)fbputlong( width, &buf[2*NET_LONG_LEN] );
	(void)fbputlong( height, &buf[3*NET_LONG_LEN] );
	pkg_2send( MSG_FBWRITERECT+MSG_NORETURN,
		buf, 4*NET_LONG_LEN,
		(char *)pp, num*sizeof(RGBpixel),
		PCP(ifp) );

	return(num);	/* No error return, sacrificed for speed. */
}

/*
 *  32-bit longs: mode, x, y
 */
_LOCAL_ int
rem_cursor( ifp, mode, x, y )
FBIO	*ifp;
int	mode;
int	x, y;
{
	char	buf[3*NET_LONG_LEN+1];
	
	/* Send Command */
	(void)fbputlong( mode, &buf[0*NET_LONG_LEN] );
	(void)fbputlong( x, &buf[1*NET_LONG_LEN] );
	(void)fbputlong( y, &buf[2*NET_LONG_LEN] );
	pkg_send( MSG_FBCURSOR, buf, 3*NET_LONG_LEN, PCP(ifp) );
	pkg_waitfor( MSG_RETURN, buf, NET_LONG_LEN, PCP(ifp) );
	return( fbgetlong( buf ) );
}

/*
 *  32-bit longs: mode, x, y
 */
_LOCAL_ int
rem_scursor( ifp, mode, x, y )
FBIO	*ifp;
int	mode;
int	x, y;
{
	char	buf[3*NET_LONG_LEN+1];
	
	/* Send Command */
	(void)fbputlong( mode, &buf[0*NET_LONG_LEN] );
	(void)fbputlong( x, &buf[1*NET_LONG_LEN] );
	(void)fbputlong( y, &buf[2*NET_LONG_LEN] );
	pkg_send( MSG_FBSCURSOR, buf, 3*NET_LONG_LEN, PCP(ifp) );
	pkg_waitfor( MSG_RETURN, buf, NET_LONG_LEN, PCP(ifp) );
	return( fbgetlong( buf ) );
}

/*
 *	x,y
 */
_LOCAL_ int
rem_window( ifp, x, y )
FBIO	*ifp;
int	x, y;
{
	char	buf[3*NET_LONG_LEN+1];
	
	/* Send Command */
	(void)fbputlong( x, &buf[0*NET_LONG_LEN] );
	(void)fbputlong( y, &buf[1*NET_LONG_LEN] );
	pkg_send( MSG_FBWINDOW, buf, 2*NET_LONG_LEN, PCP(ifp) );
	pkg_waitfor( MSG_RETURN, buf, NET_LONG_LEN, PCP(ifp) );
	return( fbgetlong( buf ) );
}

/*
 *	x,y
 */
_LOCAL_ int
rem_zoom( ifp, x, y )
FBIO	*ifp;
int	x, y;
{
	char	buf[3*NET_LONG_LEN+1];

	/* Send Command */
	(void)fbputlong( x, &buf[0*NET_LONG_LEN] );
	(void)fbputlong( y, &buf[1*NET_LONG_LEN] );
	pkg_send( MSG_FBZOOM, buf, 2*NET_LONG_LEN, PCP(ifp) );
	pkg_waitfor( MSG_RETURN, buf, NET_LONG_LEN, PCP(ifp) );
	return( fbgetlong( &buf[0*NET_LONG_LEN] ) );
}

#define REM_CMAP_BYTES	(256*3*2)

_LOCAL_ int
rem_rmap( ifp, cmap )
register FBIO		*ifp;
register ColorMap	*cmap;
{
	register int	i;
	char	buf[NET_LONG_LEN+1];
	char	cm[REM_CMAP_BYTES+4];

	pkg_send( MSG_FBRMAP, (char *)0, 0, PCP(ifp) );
	pkg_waitfor( MSG_DATA, cm, REM_CMAP_BYTES, PCP(ifp) );
	for( i = 0; i < 256; i++ ) {
		cmap->cm_red[i] = fbgetshort( cm+2*(0+i) );
		cmap->cm_green[i] = fbgetshort( cm+2*(256+i) );
		cmap->cm_blue[i] = fbgetshort( cm+2*(512+i) );
	}
	pkg_waitfor( MSG_RETURN, buf, NET_LONG_LEN, PCP(ifp) );
	return( fbgetlong( &buf[0*NET_LONG_LEN] ) );
}

_LOCAL_ int
rem_wmap( ifp, cmap )
register FBIO		*ifp;
register ColorMap	*cmap;
{
	register int	i;
	char	buf[NET_LONG_LEN+1];
	char	cm[REM_CMAP_BYTES+4];

	if( cmap == COLORMAP_NULL )
		pkg_send( MSG_FBWMAP, (char *)0, 0, PCP(ifp) );
	else {
		for( i = 0; i < 256; i++ ) {
			(void)fbputshort( cmap->cm_red[i], cm+2*(0+i) );
			(void)fbputshort( cmap->cm_green[i], cm+2*(256+i) );
			(void)fbputshort( cmap->cm_blue[i], cm+2*(512+i) );
		}
		pkg_send( MSG_FBWMAP, cm, REM_CMAP_BYTES, PCP(ifp) );
	}
	pkg_waitfor( MSG_RETURN, buf, NET_LONG_LEN, PCP(ifp) );
	return( fbgetlong( &buf[0*NET_LONG_LEN] ) );
}

_LOCAL_ int
rem_flush( ifp )
FBIO	*ifp;
{
	char	buf[NET_LONG_LEN+1];

	/* send a flush package to remote */
	pkg_send( MSG_FBFLUSH, (char *)0, 0, PCP(ifp) );
	pkg_waitfor( MSG_RETURN, buf, NET_LONG_LEN, PCP(ifp) );
	return( fbgetlong( &buf[0*NET_LONG_LEN] ) );
}

/*
 *			R E M _ H E L P
 */
_LOCAL_ int
rem_help( ifp )
FBIO	*ifp;
{
	char	buf[1*NET_LONG_LEN+1];

	fb_log( "Remote Interface:\n" );

	/* Send Command */
	(void)fbputlong( 0L, &buf[0*NET_LONG_LEN] );
	pkg_send( MSG_FBHELP, buf, 1*NET_LONG_LEN, PCP(ifp) );
	pkg_waitfor( MSG_RETURN, buf, NET_LONG_LEN, PCP(ifp) );
	return( fbgetlong( &buf[0*NET_LONG_LEN] ) );
}

/*
 *			P K G E R R O R
 *
 *  This is where we come on asynchronous error or log messages.
 *  We are counting on the remote machine now to prefix his own
 *  name to messages, so we don't touch them ourselves.
 */
static void
pkgerror(pcpp, buf)
struct pkg_conn *pcpp;
char *buf;
{
	fb_log( "%s", buf );
	free(buf);
}
