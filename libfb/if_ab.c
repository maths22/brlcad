/*
 *			I F _ A B . C
 *
 *  Communicate with an Abekas A60 digital videodisk as if it was
 *  a framebuffer, to ease the task of loading and storing images.
 *
 *  Authors -
 *	Michael John Muuss
 *	Phillip Dykstra
 *
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1989 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <time.h>

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifdef BSD
#include <strings.h>
#else
#include <string.h>
#endif

#include "fb.h"
#include "./fblocal.h"

static void	ab_log();
static int	ab_get_reply();
static int	ab_mread();


_LOCAL_ int	ab_open(),
		ab_close(),
		ab_clear(),
		ab_read(),
		ab_write(),
		ab_rmap(),
		ab_wmap(),
		ab_window(),
		ab_zoom(),
		ab_cursor(),
		ab_help();

FBIO abekas_interface = {
	ab_open,
	ab_close,
	fb_null,			/* reset */
	ab_clear,
	ab_read,
	ab_write,
	ab_rmap,
	ab_wmap,
	fb_null,			/* viewport */
	ab_window,
	ab_zoom,
	fb_null,			/* setcursor */
	ab_cursor,
	fb_null,			/* scursor */
	fb_sim_readrect,
	fb_sim_writerect,
	fb_null,			/* flush */
	ab_close,			/* free */
	ab_help,
	"Abekas A60 Videodisk, via Ethernet",
	720,				/* max width */
	486,				/* max height */
	"/dev/ab",
	720,				/* current/default width */
	486,				/* current/default height */
	-1,				/* file descriptor */
	PIXEL_NULL,			/* page_base */
	PIXEL_NULL,			/* page_curp */
	PIXEL_NULL,			/* page_endp */
	-1,				/* page_no */
	0,				/* page_ref */
	0L,				/* page_curpos */
	0L,				/* page_pixels */
	0				/* debug */
};

#define if_frame	u1.l		/* frame number on A60 disk */
#define if_mode		u2.l		/* see MODE_ and STATE_ defines */
#define if_yuv		u3.p		/* YUV image, 4th quadrant */
#define if_rgb		u4.p		/* RGB image, 1st quadrant */
#define if_host		u5.p		/* Hostname */
#define if_xyoff	u6.l		/* (x<<16) | y pixel offsets */

/*
 *  The mode has several independent bits:
 *	Center -vs- lower-left
 *	Output-only (or well-behaved read-write) -vs- conservative read-write
 *
 *  Also, the state bits ride in some of the upper bits.
 */
#define MODE_1MASK	(1<<0)
#define MODE_1CENTER	(0<<0)
#define MODE_1LOWERLEFT	(1<<0)

#define MODE_2MASK	(1<<1)
#define MODE_2READFIRST	(0<<1)
#define MODE_2OUTONLY	(1<<1)

#define MODE_3MASK	(1<<2)
#define MODE_3QUIET	(0<<2)
#define MODE_3VERBOSE	(1<<2)

#define STATE_FRAME_WAS_READ	(1<<8)
#define STATE_USER_HAS_READ	(1<<9)
#define STATE_USER_HAS_WRITTEN	(1<<10)

static struct modeflags {
	char	c;
	long	mask;
	long	value;
	char	*help;
} modeflags[] = {
	{ 'l',	MODE_1MASK, MODE_1LOWERLEFT,
		"Lower left;  default=center" },
	{ 'o',	MODE_2MASK, MODE_2OUTONLY,
		"Output only (in before out); default=always read first" },
	{ 'v',	MODE_3MASK, MODE_3VERBOSE,
		"Verbose logging; default=quiet" },
	{ '\0', 0, 0, "" }
};


/*
 *			A B _ O P E N
 *
 *  The device name is expected to have a fairly rigid format:
 *
 *	/dev/abco@host#300
 *
 *  ie, options follow first (if any),
 *  "@host" gives target host (else use environment variable),
 *  "#300" gives frame number (else use default frame).
 *
 *  It is intentional that the frame number is last in the sequence;
 *  this should make changing it easier with tcsh, etc.
 *
 */
_LOCAL_ int
ab_open( ifp, file, width, height )
register FBIO	*ifp;
register char	*file;
int		width, height;
{
	register char	*cp;
	register int	i;
	int	mode;

	mode = 0;

	if( file == NULL )  {
		fb_log( "ab_open: NULL device string\n" );
		return(-1);
	}

	if( strncmp( file, "/dev/ab", 7 ) != 0 )  {
		fb_log("ab_open: bad device '%s'\n", file );
		return(-1);
	}

	/* Process any options */
	for( cp = &file[7]; *cp != '\0'; cp++)  {
		register struct	modeflags *mfp;

		if( *cp == '@' || *cp == '#' )  break;
		for( mfp = modeflags; mfp->c != '\0'; mfp++ )  {
			if( mfp->c != *cp )  continue;
			mode = (mode & ~(mfp->mask)) | mfp->value;
			break;
		}
		if( mfp->c == '\0' )  {
			fb_log("ab_open: unknown option '%c' ignored\n", *cp);
		}
	}
	ifp->if_mode = mode;

	/* Process host name */
	if( *cp == '@' )  {
		register char	*ep;

		cp++;			/* advance over '@' */

		/* Measure length, allocate memory for string */
		for( ep=cp; *ep != '\0' && *ep != '#'; ep++ ) /* NULL */ ;
		ifp->if_host = malloc(ep-cp+2);

		ep = ifp->if_host;
		for( ; *cp != '\0' && *cp != '#'; )
			*ep++ = *cp++;
		*ep++ = '\0';
	} else if( *cp != '#' && *cp != '\0' )  {
		fb_log("ab_open: error in file spec '%s'\n", cp);
		return(-1);
	} else {
		/* Get hostname from environment variable */
		if( (ifp->if_host = getenv("ABEKAS")) == NULL )  {
			fb_log("ab_open: hostname not given and ABEKAS environment variable not set\n");
			return(-1);
		}
	}

	/* Process frame number */
	ifp->if_frame = 1;		/* default */
	if( *cp == '#' )  {
		register int	i;

		i = atoi(cp+1);
		if( i < 0 || i >= 50*30 )  {
			fb_log("ab_open: frame %d out of range\n", i);
			return(-1);
		}
		ifp->if_frame = i;
	} else if( *cp != '\0' )  {
		fb_log("ab_open: error in file spec '%s'\n", cp);
		return(-1);
	}

	/* Allocate memory for YUV and RGB buffers */
	if( (ifp->if_yuv = malloc(720*486*2)) == NULL ||
	    (ifp->if_rgb = malloc(720*486*3)) == NULL )  {
		fb_log("ab_open: unable to malloc buffer\n");
		return(-1);
	}

	/* X and Y offsets if centering & non-full size */
	ifp->if_xyoff = 0;
	if( (ifp->if_mode & MODE_1MASK) == MODE_1CENTER )  {
		if( width < ifp->if_max_width )  {
			i = (ifp->if_max_width - width)/2;
			ifp->if_xyoff = i<<16;
		}
		if( height < ifp->if_max_height )  {
			i = (ifp->if_max_height - height)/2;
			ifp->if_xyoff |= i;
		}
	}

	/* If not in output-only mode, read the frame first */
	if( (ifp->if_mode & MODE_2MASK) == MODE_2READFIRST )  {
		if( ab_readframe(ifp) < 0 )  return(-1);
	}

	ab_log(ifp, "ab_open");

	return( 0 );			/* OK */
}

/*
 *			A B _ L O G
 *
 *  If verbose mode is enabled, print the time and a message
 */
static void ab_log(ifp, str)
FBIO	*ifp;
char	*str;
{
	long		now;
	struct tm	*tmp;

	if( (ifp->if_mode & MODE_3MASK) != MODE_3VERBOSE )  return;

	(void)time( &now );
	tmp = localtime( &now );
	fb_log("%2.2d:%2.2d:%2.2d %s frame %d: %s\n",
		tmp->tm_hour, tmp->tm_min, tmp->tm_sec,
		ifp->if_host, ifp->if_frame, str );
}

/*
 *			A B _ R E A D F R A M E
 */
static int
ab_readframe(ifp)
FBIO	*ifp;
{
	register int	y;

	ab_log(ifp, "Reading frame");
	if( ab_yuvio( 0, ifp->if_host, ifp->if_yuv,
	    720*486*2, ifp->if_frame ) != 720*486*2 )  {
		fb_log("ab_readframe(%d): unable to get frame from %s!\n",
			ifp->if_frame, ifp->if_host);
		return(-1);
	}

	/* convert YUV to RGB */
	ab_log(ifp, "Converting YUV to RGB");
	for( y=0; y < 486; y++ )  {
		ab_yuv_to_rgb(
		    &ifp->if_rgb[(486-1-y)*720*3],
		    &ifp->if_yuv[y*720*2],
		    720 );
	}
	ab_log(ifp, "Conversion done");

	ifp->if_mode |= STATE_FRAME_WAS_READ;
	return(0);			/* OK */
}

/*
 *			A B _ C L O S E
 */
_LOCAL_ int
ab_close( ifp )
FBIO	*ifp;
{
	int	ret = 0;

	if( ifp->if_mode & STATE_USER_HAS_WRITTEN )  {
		register int y;		/* in Abekas coordinates */

		/* Convert RGB to YUV */
		ab_log(ifp, "Converting RGB to YUV");
		for( y=0; y < 486; y++ )  {
			ab_rgb_to_yuv(
			    &ifp->if_yuv[y*720*2],
			    &ifp->if_rgb[(486-1-y)*720*3],
			    720 );
		}
		ab_log(ifp, "Writing frame");

		if( ab_yuvio( 1, ifp->if_host, ifp->if_yuv,
		    720*486*2, ifp->if_frame ) != 720*486*2 )  {
			fb_log("ab_close: unable to send frame to A60!\n");
		    	ret = -1;
		}
		ab_log(ifp, "Transmission done");
	}

	/* Free dynamic memory */
	free( ifp->if_yuv );
	ifp->if_yuv = NULL;
	free( ifp->if_rgb );
	ifp->if_rgb = NULL;

	ab_log(ifp, "ab_close");
	return(ret);
}

/*
 *			A B _ C L E A R
 */
_LOCAL_ int
ab_clear( ifp, bgpp )
FBIO		*ifp;
RGBpixel	*bgpp;
{
	register int	r,g,b;
	register int	count;
	register char	*cp;

	if( bgpp == PIXEL_NULL )  {
		/* Clear to black */
		bzero( ifp->if_rgb, 720*486*3 );
	} else {
		r = (*bgpp)[RED];
		g = (*bgpp)[GRN];
		b = (*bgpp)[BLU];

		cp = ifp->if_rgb;
		for( count = 720*486-1; count >= 0; count-- )  {
			*cp++ = r;
			*cp++ = g;
			*cp++ = b;
		}
	}

	ifp->if_mode |= STATE_USER_HAS_WRITTEN;
	return(0);
}

/*
 *			A B _ R E A D
 */
_LOCAL_ int
ab_read( ifp, x, y, pixelp, count )
register FBIO	*ifp;
int		x;
register int	y;
RGBpixel	*pixelp;
int		count;
{
	register short		scan_count;	/* # pix on this scanline */
	register char		*cp;
	int			ret;
	int			xoff, yoff;

	if( count <= 0 )
		return(0);

	if( x < 0 || x > ifp->if_width ||
	    y < 0 || y > ifp->if_height)
		return(-1);

	if( (ifp->if_mode & STATE_FRAME_WAS_READ) == 0 )  {
		if( (ifp->if_mode & STATE_USER_HAS_WRITTEN) != 0 )  {
			fb_log("ab_read:  WARNING out-only mode set & pixels were written.  Subsequent read operation is unsafe\n");
			/* Give him whatever is in the buffer */
		} else {
			/* Read in the frame */
			if( ab_readframe(ifp) < 0 )  return(-1);
		}
	}

	xoff = ifp->if_xyoff>>16;
	yoff = ifp->if_xyoff & 0xFFFF;

	/* Copy from if_rgb[] */
	x += xoff;
	y += yoff;
	ret = 0;
	cp = (char *)(pixelp);

	while( count )  {
		if( y >= ifp->if_height )
			break;

		if ( count >= ifp->if_width-x )
			scan_count = ifp->if_width-x;
		else
			scan_count = count;

		bcopy( &ifp->if_rgb[(y*720+x)*3], cp, scan_count*3 );
		cp += scan_count * 3;
		ret += scan_count;
		count -= scan_count;
		x = xoff;
		/* Advance upwards */
		if( ++y >= ifp->if_height )
			break;
	}
	ifp->if_mode |= STATE_USER_HAS_READ;
	return(ret);
}

/*
 *			A B _ W R I T E
 */
_LOCAL_ int
ab_write( ifp, x, y, pixelp, count )
register FBIO	*ifp;
int		x, y;
RGBpixel	*pixelp;
int		count;
{
	register short		scan_count;	/* # pix on this scanline */
	register char		*cp;
	int			ret;
	int			xoff, yoff;

	if( count <= 0 )
		return(0);

	if( x < 0 || x > ifp->if_width ||
	    y < 0 || y > ifp->if_height)
		return(-1);

	/*
	 *  If "output-only" mode was set and this does not seem to
	 *  be a "well behaved" sequential write, read the frame first.
	 *  Otherwise, just 
	 */
	if( (ifp->if_mode & STATE_FRAME_WAS_READ) == 0 &&
	    (ifp->if_mode & STATE_USER_HAS_WRITTEN) == 0 )  {
		if( x != 0 || y != 0 )  {
	    		/* Try to read in the frame */
			(void)ab_readframe(ifp);
		} else {
			/* Just clear to black and proceed */
			(void)ab_clear( ifp, PIXEL_NULL );
		}
	}

	xoff = ifp->if_xyoff>>16;
	yoff = ifp->if_xyoff & 0xFFFF;

	/* Copy from if_rgb[] */
	x += xoff;
	y += yoff;
	ret = 0;
	cp = (char *)(pixelp);

	while( count )  {
		if( y >= ifp->if_height )
			break;

		if ( count >= ifp->if_width-x )
			scan_count = ifp->if_width-x;
		else
			scan_count = count;

		bcopy( cp, &ifp->if_rgb[(y*720+x)*3], scan_count*3 );
		cp += scan_count * 3;
		ret += scan_count;
		count -= scan_count;
		x = xoff;
		/* Advance upwards */
		if( ++y >= ifp->if_height )
			break;
	}
	ifp->if_mode |= STATE_USER_HAS_WRITTEN;
	return(ret);
}

/*
 */
_LOCAL_ int
ab_cursor( ifp, mode, x, y )
FBIO	*ifp;
int	mode;
int	x, y;
{
	return(-1);
}

/*
 */
_LOCAL_ int
ab_window( ifp, x, y )
FBIO	*ifp;
int	x, y;
{
	return(-1);
}

/*
 */
_LOCAL_ int
ab_zoom( ifp, x, y )
FBIO	*ifp;
int	x, y;
{
	return(-1);
}

_LOCAL_ int
ab_rmap( ifp, cmap )
register FBIO		*ifp;
register ColorMap	*cmap;
{
	register int	i;

	for( i = 0; i < 256; i++ ) {
		cmap->cm_red[i] = i<<8;
		cmap->cm_green[i] = i<<8;
		cmap->cm_blue[i] = i<<8;
	}
	return(0);
}

_LOCAL_ int
ab_wmap( ifp, cmap )
register FBIO		*ifp;
register ColorMap	*cmap;
{
	/* Just pretend it worked OK */
	return(0);
}

/*
 *			A B _ H E L P
 */
_LOCAL_ int
ab_help( ifp )
FBIO	*ifp;
{
	struct	modeflags *mfp;

	fb_log( "Abekas A60 Ethernet Interface\n" );
	fb_log( "Device: %s\n", ifp->if_name );
	fb_log( "Maximum width height: %d %d\n",
		ifp->if_max_width,
		ifp->if_max_height );
	fb_log( "Default width height: %d %d\n",
		ifp->if_width,
		ifp->if_height );
	fb_log( "A60 Host: %s\n", ifp->if_host );
	fb_log( "Frame: %d\n", ifp->if_frame );
	fb_log( "Usage: /dev/ab[options][@host][#framenumber]\n" );
	for( mfp = modeflags; mfp->c != '\0'; mfp++ ) {
		fb_log( "   %c   %s\n", mfp->c, mfp->help );
	}

	return(0);
}

/*
 *			A B _ Y U V I O
 *
 *  Input or output a full frame image from the Abekas using the RCP
 *  protocol.
 *
 *  Returns -
 *	-1	error
 *	len	successful count
 */
ab_yuvio( output, host, buf, len, frame )
int	output;		/* 0=read(input), 1=write(output) */
char	*host;
char	*buf;
int	len;
int	frame;		/* frame number */
{
	struct sockaddr_in	sinme;		/* Client */
	struct sockaddr_in	sinhim;		/* Server */
	struct servent		*rlogin_service;
	struct hostent		*hp;
	char			xmit_buf[128];
	int			n;
	int			netfd;
	int			got;

	bzero((char *)&sinhim, sizeof(sinhim));
	bzero((char *)&sinme, sizeof(sinme));

	if( (rlogin_service = getservbyname("shell", "tcp")) == NULL )  {
		fb_log("getservbyname(shell,tcp) fail\n");
		return(-1);
	}
	sinhim.sin_port = rlogin_service->s_port;

	if( atoi( host ) > 0 )  {
		/* Numeric */
		sinhim.sin_family = AF_INET;
		sinhim.sin_addr.s_addr = inet_addr(host);
	} else {
		if( (hp = gethostbyname(host)) == NULL )  {
			fb_log("gethostbyname(%s) fail\n", host);
			return(-1);
		}
		sinhim.sin_family = hp->h_addrtype;
		bcopy(hp->h_addr, (char *)&sinhim.sin_addr, hp->h_length);
	}

	if( (netfd = socket(sinhim.sin_family, SOCK_STREAM, 0)) < 0 )  {
		perror("socket()");
		return(-1);
	}
	sinme.sin_port = 0;		/* let kernel pick it */

	if( bind(netfd, (char *)&sinme, sizeof(sinme)) < 0 )  {
		perror("bind()");
		goto err;
	}

	if( connect(netfd, (char *)&sinhim, sizeof(sinhim)) < 0 )  {
		perror("connect()");
		goto err;
	}

	/*
	 *  Connection established.  Now, speak the RCP protocol.
	 */

	/* Indicate that standard error is not connected */
	if( write( netfd, "\0", 1 ) != 1 )  {
		perror("write()");
		goto err;
	}

	/* Write local and remote user names, null terminated */
	if( write( netfd, "a60libfb\0", 9 ) != 9 )  {
		perror("write()");
		goto err;
	}
	if( write( netfd, "a60libfb\0", 9 ) != 9 )  {
		perror("write()");
		goto err;
	}
	if( ab_get_reply(netfd) < 0 )  goto err;

	if( output )  {
		/* Output from buffer to A60 */
		/* Send command, null-terminated */
		sprintf( xmit_buf, "rcp -t %d.yuv", frame );
		n = strlen(xmit_buf)+1;		/* include null */
		if( write( netfd, xmit_buf, n ) != n )  {
			perror("write()");
			goto err;
		}
		if( ab_get_reply(netfd) < 0 )  goto err;

		/* Send Access flags, length, old name */
		sprintf( xmit_buf, "C0664 %d %d.yuv\n", len, frame );
		n = strlen(xmit_buf);
		if( write( netfd, xmit_buf, n ) != n )  {
			perror("write()");
			goto err;
		}
		if( ab_get_reply(netfd) < 0 )  goto err;

		if( (got = write( netfd, buf, len )) != len )  {
			perror("write()");
			goto err;
		}

		/* Send final go-ahead */
		if( (got = write( netfd, "\0", 1 )) != 1 )  {
			fb_log("go-ahead write got %d\n", got);
			perror("write()");
			goto err;
		}
		if( (got = ab_get_reply(netfd)) < 0 )  {
			fb_log("get_reply got %d\n", got);
			goto err;
		}

		(void)close(netfd);
		return(len);		/* OK */
	} else {
		register char	*cp;
		int		perm;
		int		src_size;
		/* Input from A60 into buffer */
		/* Send command, null-terminated */
		sprintf( xmit_buf, "rcp -f %d.yuv", frame );
		n = strlen(xmit_buf)+1;		/* include null */
		if( write( netfd, xmit_buf, n ) != n )  {
			perror("write()");
			goto err;
		}

		/* Send go-ahead */
		if( write( netfd, "\0", 1 ) != 1 )  {
			perror("write()");
			goto err;
		}

		/* Read up to a newline */
		cp = xmit_buf;
		for(;;)  {
			if( read( netfd, cp, 1 ) != 1 )  {
				perror("read()");
				goto err;
			}
			if( *cp == '\n' )  break;
			cp++;
			if( (cp - xmit_buf) >= sizeof(xmit_buf) )  {
				fb_log("cmd buffer overrun\n");
				goto err;
			}
		}
		*cp++ = '\0';
		/* buffer will contain old permission, size, old name */
		src_size = 0;
		if( sscanf( xmit_buf, "C%o %d", &perm, &src_size ) != 2 )  {
			fb_log("sscanf error\n");
			goto err;
		}

		/* Send go-ahead */
		if( write( netfd, "\0", 1 ) != 1 )  {
			perror("write()");
			goto err;
		}

		/* Read data */
		if( (got = ab_mread( netfd, buf, len )) != len )  {
			fb_log("ab_mread len=%d, got %d\n", len, got );
			goto err;
		}

		/* Send go-ahead */
		if( write( netfd, "\0", 1 ) != 1 )  {
			perror("write()");
			goto err;
		}
		(void)close(netfd);
		return(len);		/* OK */
	}

err:
	(void)close(netfd);
	return(-1);
}

static int
ab_get_reply(fd)
int	fd;
{
	char	rep_buf[128];
	int	got;

	if( (got = read( fd, rep_buf, sizeof(rep_buf) )) < 0 )  {
		perror("ab_get_reply()/read()");
		fb_log("ab_get_reply() read error\n");
		return(-1);
	}

	if( got == 0 )  {
		fb_log("ab_get_reply() unexpected EOF\n");
		return(-2);		/* EOF seen */
	}

	/* got >= 1 */
	if( rep_buf[0] == 0 )
		return(0);		/* OK */

	if( got == 1 )  {
		fb_log("ab_get_reply() error reply code, no attached message\n");
		return(-3);
	}

	/* Print error code received from other end */
	fb_log("ab_get_reply() error='%s'\n", &rep_buf[1] );
	return(-4);
}

/*
 *			M R E A D
 *
 * Internal.
 * This function performs the function of a read(II) but will
 * call read(II) multiple times in order to get the requested
 * number of characters.  This can be necessary because pipes
 * and network connections don't deliver data with the same
 * grouping as it is written with.  Written by Robert S. Miles, BRL.
 */
static int
ab_mread(fd, bufp, n)
int	fd;
register char	*bufp;
int	n;
{
	register int	count = 0;
	register int	nread;

	do {
		nread = read(fd, bufp, (unsigned)n-count);
		if(nread < 0)  {
			perror("ab_mread");
			return(-1);
		}
		if(nread == 0)
			return((int)count);
		count += (unsigned)nread;
		bufp += nread;
	 } while(count < n);

	return((int)count);
}

/*************************************************************************
 *************************************************************************
 *  Herein lies the conversion between YUV and RGB
 *************************************************************************
 *************************************************************************
 */
/*  A 4:2:2 framestore uses 2 bytes per pixel.  The even bytes (from 0)
 *  hold Cb and Y, the odd bytes Cr and Y.  Thus a scan line has:
 *      Cb Y Cr Y Cb Y Cr Y ...
 *  If we are at an even pixel, we use the Cr value following it.  If
 *  we are at an odd pixel, we use the Cb value following it.
 *
 *  Y:      16 .. 235 range, offset by 16
 *  U, V: -112 .. +112 range, centered on 128
 */

#define	VDOT(a,b)	(a[0]*b[0]+a[1]*b[1]+a[2]*b[2])
#define	V5DOT(a,b)	(a[0]*b[0]+a[1]*b[1]+a[2]*b[2]+a[3]*b[3]+a[4]*b[4])
#define	floor(d)	(d>=0?(int)d:((int)d==d?d:(int)(d-1.0)))
#define	CLIP(out,in)		{ register int t; \
		if( (t = (in)) < 0 )  (out) = 0; \
		else if( t >= 255 )  (out) = 255; \
		else (out) = t; }

#define	LINE_LENGTH	720
#define	FRAME_LENGTH	486

static double	y_weights[] = {  0.299,   0.587,   0.114 };
static double	u_weights[] = { -0.1686, -0.3311,  0.4997 };
static double	v_weights[] = {  0.4998, -0.4185, -0.0813 };

static double	y_filter[] = { -0.05674, 0.01883, 1.07582, 0.01883, -0.05674 };
static double	u_filter[] = {  0.14963, 0.22010, 0.26054, 0.22010,  0.14963 };
static double	v_filter[] = {  0.14963, 0.22010, 0.26054, 0.22010,  0.14963 };

static double	ybuf[724];
static double	ubuf[724];
static double	vbuf[724];

/* RGB to YUV */
ab_rgb_to_yuv( yuv_buf, rgb_buf, len )
unsigned char *yuv_buf;
unsigned char *rgb_buf;
int	len;
{
	register unsigned char *cp;
	register double	*yp, *up, *vp;
	register int	i;
	static int	first=1;

	if(first)  {
		/* SETUP */
		for( i = 0; i < 5; i++ ) {
			y_filter[i] *= 219.0/255.0;
			u_filter[i] *= 224.0/255.0;
			v_filter[i] *= 224.0/255.0;
		}
		first = 0;
	}

	/* Matrix RGB's into separate Y, U, and V arrays */
	yp = &ybuf[2];
	up = &ubuf[2];
	vp = &vbuf[2];
	cp = rgb_buf;
	for( i = len; i; i-- ) {
		*yp++ = VDOT( y_weights, cp );
		*up++ = VDOT( u_weights, cp );
		*vp++ = VDOT( v_weights, cp );
		cp += 3;
	}

	/* filter, scale, and sample YUV arrays */
	yp = ybuf;
	up = ubuf;
	vp = vbuf;
	cp = yuv_buf;
	for( i = len/2; i; i-- ) {
		*cp++ = V5DOT(u_filter,up) + 128.0;	/* u */
		*cp++ = V5DOT(y_filter,yp) + 16.0;	/* y */
		*cp++ = V5DOT(v_filter,vp) + 128.0;	/* v */
		yp++;
		*cp++ = V5DOT(y_filter,yp) + 16.0;	/* y */
		yp++;
		up += 2;
		vp += 2;
	}
}

/* YUV to RGB */
ab_yuv_to_rgb( rgb_buf, yuv_buf, len )
unsigned char *rgb_buf;
unsigned char *yuv_buf;
{
	register unsigned char *rgbp;
	register unsigned char *yuvp;
	register double	y, u, v;
	register int	pixel;
	int		last;

	/* Input stream looks like:  uy  vy  uy  vy  */

	rgbp = rgb_buf;
	yuvp = yuv_buf;
	last = len/2;
	for( pixel = last; pixel; pixel-- ) {
		/* even pixel, get y and next v */
		if( pixel == last ) {
			u = (*yuvp++ - 128.0) * 255.0/224.0;
		}
		y = (*yuvp++ - 16.0) * 255.0/219.0;
		v = (*yuvp++ - 128.0) * 255.0/224.0;

		CLIP( *rgbp++, y + 1.4026 * v);			/* R */
		CLIP( *rgbp++, y - 0.3444 * u - 0.7144 * v);	/* G */
		CLIP( *rgbp++, y + 1.7730 * u);			/* B */

		/* odd pixel, got v already, get y and next u */
		y = (*yuvp++ - 16.0) * 255.0/219.0;
		if( pixel != 1 ) {
			u = (*yuvp++ - 128.0) * 255.0/224.0;
		}

		CLIP( *rgbp++, y + 1.4026 * v);			/* R */
		CLIP( *rgbp++, y - 0.3444 * u - 0.7144 * v);	/* G */
		CLIP( *rgbp++, y + 1.7730 * u);			/* B */
	}
}
