/*
 *			I F _ A B . C
 *
 *  Communicate with an Abekas A60 digital videodisk as if
 *  it was a framebuffer, to ease the task of loading and storing
 *  images.
 *
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
 *
 *	$Header$ (BRL)
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <errno.h>

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

static int	ab_get_reply();
static int	ab_mread();

extern int	fb_sim_readrect(),
		fb_sim_writerect();


_LOCAL_ int	ab_dopen(),
		ab_dclose(),
		ab_dclear(),
		ab_bread(),
		ab_bwrite(),
		ab_cmread(),
		ab_cmwrite(),
		ab_window_set(),
		ab_zoom_set(),
		ab_cmemory_addr(),
		ab_help();

FBIO abekas_interface = {
	ab_dopen,
	ab_dclose,
	fb_null,			/* reset		*/
	ab_dclear,
	ab_bread,
	ab_bwrite,
	ab_cmread,
	ab_cmwrite,
	fb_null,			/* viewport_set		*/
	ab_window_set,
	ab_zoom_set,
	fb_null,			/* curs_set		*/
	ab_cmemory_addr,
	fb_null,			/* cursor_move_screen_addr */
	fb_sim_readrect,
	fb_sim_writerect,
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
#define if_mode		u2.l		/* see MODE_ defines */
#define if_yuv		u3.p		/* buffer for YUV format image */
#define if_rgb		u4.p		/* buffer for RGB format image */
#define if_state	u5.l		/* see STATE_ defines */
#define if_host		u6.p		/* Hostname */

/*
 *  The mode has several independent bits:
 *	Center -vs- lower-left
 *	Output-only (or well-behaved read-write) -vs- conservative read-write
 */
#define MODE_1MASK	(1<<0)
#define MODE_1LOWERLEFT	(0<<0)
#define MODE_1CENTER	(1<<0)

#define MODE_2MASK	(1<<1)
#define MODE_2READFIRST	(0<<1)
#define MODE_2OUTONLY	(1<<1)


struct modeflags {
	char	c;
	long	mask;
	long	value;
	char	*help;
} modeflags[] = {
	{ 'c',	MODE_1MASK, MODE_1CENTER,
		"Center;  default=lower-left " },
	{ 'o',	MODE_2MASK, MODE_2OUTONLY,
		"Output only (in before out); default=always read first" },
	{ '\0', 0, 0, "" }
};

#define STATE_FRAME_WAS_READ	(1<<1)
#define STATE_USER_HAS_READ	(1<<2)
#define STATE_USER_HAS_WRITTEN	(1<<3)


/*
 *			A B _ D O P E N
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
ab_dopen( ifp, file, width, height )
register FBIO	*ifp;
register char	*file;
int		width, height;
{
	register char	*cp;
	register int	i;
	int	mode;

	mode = 0;

	if( file == NULL )  {
		fb_log( "ab_dopen: NULL device string\n" );
		return(-1);
	}

	if( strncmp( file, "/dev/ab", 7 ) != 0 )  {
		fb_log("ab_dopen: bad device '%s'\n", file );
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
			fb_log("ab_dopen: unknown option '%c' ignored\n", *cp);
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
		fb_log("ab_dopen: error in file spec '%s'\n", cp);
		return(-1);
	} else {
		/* Get hostname from environment variable */
		if( (ifp->if_host = getenv("ABEKAS")) == NULL )  {
			fb_log("ab_dopen: hostname not given and ABEKAS environment variable not set\n");
			return(-1);
		}
	}

	/* Process frame number */
	ifp->if_frame = 1;		/* default */
	if( *cp == '#' )  {
		register int	i;

		i = atoi(cp+1);
		if( i < 0 || i >= 50*30 )  {
			fb_log("ab_dopen: frame %d out of range\n", i);
			return(-1);
		}
		ifp->if_frame = i;
	} else if( *cp != '\0' )  {
		fb_log("ab_dopen: error in file spec '%s'\n", cp);
		return(-1);
	}

	/* Allocate memory for YUV and RGB buffers */
	if( (ifp->if_yuv = malloc(720*486*2)) == NULL ||
	    (ifp->if_rgb = malloc(720*486*3)) == NULL )  {
		fb_log("ab_dopen: unable to malloc buffer\n");
		return(-1);
	}

	/* X and Y offsets if centering & non-full size? */

	ifp->if_state = 0;
	if( (ifp->if_mode & MODE_2MASK) == MODE_2READFIRST )  {
		if( ab_readframe(ifp) < 0 )  return(-1);
	}

	return( 0 );			/* OK */
}

/*
 *			A B _ R E A D F R A M E
 */
static int
ab_readframe(ifp)
FBIO	*ifp;
{
	if( ab_yuvio( 0, ifp->if_host, ifp->if_yuv,
	    720*486*2, ifp->if_frame ) != 720*486*2 )  {
		fb_log("ab_readframe(%d): unable to get frame from %s!\n",
			ifp->if_frame, ifp->if_host);
		return(-1);
	}

	/* convert YUV to RGB */

	ifp->if_state |= STATE_FRAME_WAS_READ;
	return(0);			/* OK */
}

/*
 *			A B _ D C L O S E
 */
_LOCAL_ int
ab_dclose( ifp )
FBIO	*ifp;
{
	int	ret = 0;

	if( ifp->if_state & STATE_USER_HAS_WRITTEN )  {

		/* Convert RGB to YUV */

		if( ab_yuvio( 1, ifp->if_host, ifp->if_yuv,
		    720*486*2, ifp->if_frame ) != 720*486*2 )  {
			fb_log("ab_dclose: unable to send frame to A60!\n");
		    	ret = -1;
		}
	}

	/* Free dynamic memory */
	free( ifp->if_yuv );
	ifp->if_yuv = NULL;
	free( ifp->if_rgb );
	ifp->if_rgb = NULL;

	return(ret);
}

/*
 *			A B _ D C L E A R
 */
_LOCAL_ int
ab_dclear( ifp, bgpp )
FBIO		*ifp;
RGBpixel	*bgpp;
{
	register int	r,g,b;
	register int	count;
	register char	*cp;

	/* send a clear package to remote */
	if( bgpp == PIXEL_NULL )  {
		/* Clear to black */
		bzero( ifp->if_rgb, 720*484*3 );
	} else {
		r = (*bgpp)[RED];
		g = (*bgpp)[GRN];
		b = (*bgpp)[BLU];

		cp = ifp->if_rgb;
		for( count = 720*484-1; count >= 0; count-- )  {
			*cp++ = r;
			*cp++ = g;
			*cp++ = b;
		}
	}

	ifp->if_state |= STATE_USER_HAS_WRITTEN;
	return(0);
}

/*
 *			A B _ B R E A D
 */
_LOCAL_ int
ab_bread( ifp, x, y, pixelp, num )
register FBIO	*ifp;
int		x;
register int	y;
RGBpixel	*pixelp;
int		num;
{
	register short		scan_count;	/* # pix on this scanline */
	register char		*cp;
	int			count;
	int			ret;
	int			ybase;

	if( num <= 0 )
		return(0);

	if( x < 0 || x > ifp->if_width ||
	    y < 0 || y > ifp->if_height)
		return(-1);

	if( (ifp->if_state & STATE_FRAME_WAS_READ) == 0 )  {
		if( (ifp->if_state & STATE_USER_HAS_WRITTEN) != 0 )  {
			fb_log("ab_bread:  WARNING out-only mode set & pixels were written.  Subsequent read operation is unsafe\n");
			/* Give him whatever is in the buffer */
		} else {
			/* Read in the frame */
			if( ab_readframe(ifp) < 0 )  return(-1);
		}
	}

	/* Copy from if_rgb[] */
	ybase = y;
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
		x = 0;
		/* Advance upwards */
		if( ++y >= ifp->if_height )
			break;
	}
	ifp->if_state |= STATE_USER_HAS_READ;
	return(ret);
}

/*
 *			A B _ B W R I T E
 */
_LOCAL_ int
ab_bwrite( ifp, x, y, pixelp, num )
register FBIO	*ifp;
int		x, y;
RGBpixel	*pixelp;
int		num;
{
	register short		scan_count;	/* # pix on this scanline */
	register char		*cp;
	int			count;
	int			ret;
	int			ybase;

	if( num <= 0 )
		return(0);

	if( x < 0 || x > ifp->if_width ||
	    y < 0 || y > ifp->if_height)
		return(-1);

	/*
	 *  If "output-only" mode was set and this does not seem to
	 *  be a "well behaved" sequential write, read the frame first.
	 *  Otherwise, just 
	 */
	if( (ifp->if_state & STATE_FRAME_WAS_READ) == 0 &&
	    (ifp->if_state & STATE_USER_HAS_WRITTEN) == 0 )  {
		if( x != 0 || y != 0 )  {
	    		/* Try to read in the frame */
			(void)ab_readframe(ifp);
		} else {
			/* Just clear to black and proceed */
			(void)ab_dclear( ifp, PIXEL_NULL );
		}
	}

	/* Copy from if_rgb[] */
	ybase = y;
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
		x = 0;
		/* Advance upwards */
		if( ++y >= ifp->if_height )
			break;
	}
	ifp->if_state |= STATE_USER_HAS_WRITTEN;
	return(ret);
}

/*
 */
_LOCAL_ int
ab_cmemory_addr( ifp, mode, x, y )
FBIO	*ifp;
int	mode;
int	x, y;
{
	return(-1);
}

/*
 */
_LOCAL_ int
ab_window_set( ifp, x, y )
FBIO	*ifp;
int	x, y;
{
	return(-1);
}

/*
 */
_LOCAL_ int
ab_zoom_set( ifp, x, y )
FBIO	*ifp;
int	x, y;
{
	return(-1);
}

_LOCAL_ int
ab_cmread( ifp, cmap )
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
ab_cmwrite( ifp, cmap )
register FBIO		*ifp;
register ColorMap	*cmap;
{
	return(-1);
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
	unsigned long		addr_tmp;

	bzero((char *)&sinhim, sizeof(sinhim));
	bzero((char *)&sinme, sizeof(sinme));

	if( (rlogin_service = getservbyname("shell", "tcp")) == NULL )  {
		fprintf(stderr,"getservbyname(shell,tcp) fail\n");
		return(-1);
	}
	sinhim.sin_port = rlogin_service->s_port;

	if( atoi( host ) > 0 )  {
		/* Numeric */
		sinhim.sin_family = AF_INET;
#if CRAY && OLDTCP
		addr_tmp = inet_addr(host);
		sinhim.sin_addr = addr_tmp;
#else
		sinhim.sin_addr.s_addr = inet_addr(host);
#endif
	} else {
		if( (hp = gethostbyname(host)) == NULL )  {
			fprintf(stderr,"gethostbyname(%s) fail\n", host);
			return(-1);
		}
		sinhim.sin_family = hp->h_addrtype;
		bcopy(hp->h_addr, (char *)&addr_tmp, hp->h_length);
#		if CRAY && OLDTCP
			sinhim.sin_addr = addr_tmp;
#		else
			sinhim.sin_addr.s_addr = addr_tmp;
#		endif
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

		if( write( netfd, buf, len ) != len )  {
			perror("write()");
			goto err;
		}

		/* Send final go-ahead */
fprintf(stderr,"before write\n");
		if( write( netfd, "\0", 1 ) != 1 )  {
			perror("write()");
			goto err;
		}
fprintf(stderr,"after write\n");
		if( ab_get_reply(netfd) < 0 )  goto err;

		(void)close(netfd);
		return(0);		/* OK */
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
				fprintf(stderr,"cmd buffer overrun\n");
				goto err;
			}
		}
		*cp++ = '\0';
		/* buffer will contain old permission, ??? (size, old name) */
		fprintf(stderr,"got '%s'\n", xmit_buf);
		src_size = 0;
		if( sscanf( xmit_buf, "C%o %d", &perm, &src_size ) != 2 )  {
			fprintf(stderr,"sscanf error\n");
			goto err;
		}

		/* Send go-ahead */
		if( write( netfd, "\0", 1 ) != 1 )  {
			perror("write()");
			goto err;
		}

		/* Read data */
fprintf(stderr,"before ab_mread\n");
		if( (got = ab_mread( netfd, buf, len )) != len )  {
			fprintf(stderr,"ab_mread len=%d, got %d\n", len, got );
			goto err;
		}
fprintf(stderr,"after ab_mread\n");

		/* Send go-ahead */
		if( write( netfd, "\0", 1 ) != 1 )  {
			perror("write()");
			goto err;
		}
		(void)close(netfd);
		return(0);		/* OK */
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
		return(-1);
	}

	if( got == 0 )  {
		fprintf(stderr,"ab_get_reply() unexpected EOF\n");
		return(-2);		/* EOF seen */
	}

	/* got >= 1 */
	if( rep_buf[0] == 0 )
		return(0);		/* OK */

	if( got == 1 )  {
		fprintf(stderr,"ab_get_reply() error reply code, no attached message\n");
		return(-3);
	}

	/* Print error code received from other end */
	fprintf(stderr,"ab_get_reply() error='%s'\n", &rep_buf[1] );
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
