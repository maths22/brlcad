/*
 *			P K G . C
 *
 *  Routines to manage multiplexing and de-multiplexing synchronous
 *  and asynchronous messages across stream connections.
 *
 *  Functions -
 *	pkg_open	Open a network connection to a host
 *	pkg_initserver	Create a network server, and listen for connection
 *	pkg_getclient	As network server, accept a new connection
 *	pkg_close	Close a network connection
 *	pkg_send	Send a message on the connection
 *	pkg_waitfor	Wait for a specific msg, user buf, processing others
 *	pkg_bwaitfor	Wait for specific msg, malloc buf, processing others
 *	pkg_get		Read bytes from connection, assembling message
 *	pkg_block	Wait until a full message has been read
 *
 *  Authors -
 *	Michael John Muuss
 *	Charles M. Kennedy
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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>		/* for struct iovec */
#include <sys/ioctl.h>		/* for FIONBIO */
#include <netinet/in.h>		/* for htons(), etc */
#include <netdb.h>
#include <sys/time.h>		/* for struct timeval */
#include <errno.h>

#include "../h/pkg.h"

extern char *malloc();
extern void perror();
extern int errno;

#define PKG_CK(p)	{if(p==PKC_NULL||p->pkc_magic!=PKG_MAGIC) {\
			fprintf(stderr,"pkg: bad pointer x%x\n",p);abort();}}

/*
 *			P K G _ O P E N
 *
 *  We are a client.  Make a connection to the server.
 *
 *  Returns PKC_NULL on error.
 */
struct pkg_conn *
pkg_open( host, service )
char *host;
char *service;
{
	struct sockaddr_in sinme;		/* Client */
	struct sockaddr_in sinhim;		/* Server */
	register struct hostent *hp;
	register int netfd;
	register struct pkg_conn *pc;

	bzero((char *)&sinhim, sizeof(sinhim));
	bzero((char *)&sinme, sizeof(sinme));

	/* Determine port for service */
	if( (sinhim.sin_port = atoi(service)) > 0 )  {
		sinhim.sin_port = htons(sinhim.sin_port);
	} else {
		register struct servent *sp;
		if( (sp = getservbyname( service, "tcp" )) == NULL )  {
			fprintf(stderr,"pkg_open(%s,%s): unknown service\n",
				host, service );
			return(PKC_NULL);
		}
		sinhim.sin_port = sp->s_port;
	}

	/* Get InterNet address */
	if( atoi( host ) > 0 )  {
		/* Numeric */
		sinhim.sin_family = AF_INET;
		sinhim.sin_addr.s_addr = inet_addr(host);
	} else {
		if( (hp = gethostbyname(host)) == NULL )  {
			fprintf(stderr,"pkg_open(%s,%s): unknown host\n",
				host, service );
			return(PKC_NULL);
		}
		sinhim.sin_family = hp->h_addrtype;
		bcopy(hp->h_addr, (char *)&sinhim.sin_addr, hp->h_length);
	}

	if( (netfd = socket(sinhim.sin_family, SOCK_STREAM, 0)) < 0 )  {
		perror("pkg_open:  client socket");
		return(PKC_NULL);
	}
	sinme.sin_port = 0;		/* let kernel pick it */

	if( bind(netfd, (char *)&sinme, sizeof(sinme)) < 0 )  {
		perror("pkg_open: bind");
		return(PKC_NULL);
	}

	if( connect(netfd, (char *)&sinhim, sizeof(sinhim), 0) < 0 )  {
		perror("pkg_open: client connect");
		return(PKC_NULL);
	}
	if( (pc = (struct pkg_conn *)malloc(sizeof(struct pkg_conn)))==PKC_NULL )  {
		fprintf(stderr,"pkg_open: malloc failure\n");
		return(PKC_NULL);
	}
	pc->pkc_magic = PKG_MAGIC;
	pc->pkc_fd = netfd;
	pc->pkc_left = -1;
	pc->pkc_buf = (char *)0;
	pc->pkc_curpos = (char *)0;

	return(pc);
}

/*
 *  			P K G _ I N I T S E R V E R
 *  
 *  We are now going to be a server for the indicated service.
 *  Hang a LISTEN, and return the fd to select() on waiting for
 *  new connections.
 *
 *  Returns fd to listen on (>=0), -1 on error.
 */
int
pkg_initserver( service, backlog )
char *service;
int backlog;
{
	struct sockaddr_in sinme;
	register struct servent *sp;
	int pkg_listenfd;

	bzero((char *)&sinme, sizeof(sinme));

	/* Determine port for service */
	if( (sp = getservbyname( service, "tcp" )) == NULL )  {
		fprintf(stderr,"pkg_initserver(%s,%d): unknown service\n",
			service, backlog );
		return(-1);
	}

	sinme.sin_port = sp->s_port;
	if( (pkg_listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )  {
		perror("pkg_initserver:  socket");
		return(-1);
	}

	if( bind(pkg_listenfd, &sinme, sizeof(sinme)) < 0 )  {
		perror("pkg_initserver: bind");
		return(-1);
	}

	if( backlog > 5 )  backlog = 5;
	if( listen(pkg_listenfd, backlog) < 0 )  {
		perror("pkg_initserver:  listen");
		return(-1);
	}
	return(pkg_listenfd);
}

/*
 *			P K G _ G E T C L I E N T
 *
 *  Given an fd with a listen outstanding, accept the connection.
 *  When poll == 0, accept is allowed to block.
 *  When poll != 0, accept will not block.
 *
 *  Returns -
 *	>0		ptr to pkg_conn block of new connection
 *	PKC_NULL	accept would block, try again later
 *	PKC_ERROR	fatal error
 */
struct pkg_conn *
pkg_getclient(fd, nodelay)
{
	struct sockaddr_in from;
	register int s2;
	auto int fromlen = sizeof (from);
	auto int onoff;
	register struct pkg_conn *pc;

	if(nodelay)  {
		onoff = 1;
		if( ioctl(fd, FIONBIO, &onoff) < 0 )
			perror("pkg_getclient: FIONBIO 1");
	}
	do  {
		s2 = accept(fd, (char *)&from, &fromlen);
		if (s2 < 0) {
			if(errno == EINTR)
				continue;
			if(errno == EWOULDBLOCK)
				return(PKC_NULL);
			perror("pkg_getclient: accept");
			return(PKC_ERROR);
		}
	}  while( s2 < 0);
	if(nodelay)  {		
		onoff = 0;
		if( ioctl(fd, FIONBIO, &onoff) < 0 )
			perror("pkg_getclient: FIONBIO 2");
		if( ioctl(s2, FIONBIO, &onoff) < 0 )
			perror("pkg_getclient: FIONBIO 3");
	}
	if( (pc = (struct pkg_conn *)malloc(sizeof(struct pkg_conn)))==PKC_NULL )  {
		fprintf(stderr,"pkg_getclient: malloc failure\n");
		return(PKC_ERROR);
	}
	pc->pkc_magic = PKG_MAGIC;
	pc->pkc_fd = s2;
	pc->pkc_left = -1;
	pc->pkc_buf = (char *)0;
	pc->pkc_curpos = (char *)0;
	return(pc);
}

/*
 *  			P K G _ C L O S E
 *  
 *  Gracefully release the connection block and close the connection.
 */
void
pkg_close(pc)
register struct pkg_conn *pc;
{
	PKG_CK(pc);
	(void)close(pc->pkc_fd);
	pc->pkc_fd = -1;		/* safety */
	if( pc->pkc_buf )
		(void)free(pc->pkc_buf);
	pc->pkc_buf = (char *)0;	/* safety */
	(void)free(pc->pkc_buf);
}

/*
 *			P K G _ M R E A D
 *
 * This function performs the function of a read(II) but will
 * call read(II) multiple times in order to get the requested
 * number of characters.  This can be necessary because pipes
 * and network connections don't deliver data with the same
 * grouping as it is written with.  Written by Robert S. Miles, BRL.
 */
int
pkg_mread(fd, bufp, n)
int fd;
register char	*bufp;
int	n;
{
	register int	count = 0;
	register int	nread;

	do {
		nread = read(fd, bufp, (unsigned)n-count);
		if(nread < 0)  {
			perror("pkg_mread");
			return(-1);
		}
		if(nread == 0)
			return((int)count);
		count += (unsigned)nread;
		bufp += nread;
	 } while(count < n);

	return((int)count);
}

/*
 *  			P K G _ S E N D
 *
 *  Send the user's data, prefaced with an identifying header which
 *  contains a message type value.  All header fields are exchanged
 *  in "network order".
 *
 *  Note that the whole message (header + data) should be transmitted
 *  by TCP with only one TCP_PUSH at the end, due to the use of writev().
 *
 *  Returns number of bytes of user data actually sent.
 */
int
pkg_send( type, buf, len, pc )
int type;
char *buf;
int len;
register struct pkg_conn *pc;
{
	static struct iovec cmdvec[2];
	static struct pkg_header hdr;
	struct timeval tv;
	long bits;
	register int i;

	PKG_CK(pc);
	if( len < 0 )  len=0;

	do  {
		/* Finish any partially read message */
		if( pc->pkc_left > 0 )
			if( pkg_block(pc) < 0 )
				return(-1);
#ifdef never
		/* Check socket for more input */
		tv.tv_sec = 0;
		tv.tv_usec = 0;		/* poll */
		bits = 1<<pc->pkc_fd;
		i = select( pc->pkc_fd+1, &bits, (char *)0, (char *)0, &tv );
		if( i > 0 && bits )
			if( pkg_block(pc) < 0 )
				return(-1);
#endif
	} while( pc->pkc_left > 0 );

	hdr.pkg_magic = htons(PKG_MAGIC);
	hdr.pkg_type = htons(type);	/* should see if it's a valid type */
	hdr.pkg_len = htonl(len);

	cmdvec[0].iov_base = (caddr_t)&hdr;
	cmdvec[0].iov_len = sizeof(hdr);
	cmdvec[1].iov_base = (caddr_t)buf;
	cmdvec[1].iov_len = len;

	/*
	 * TODO:  set this FD to NONBIO.  If not all output got sent,
	 * loop in select() waiting for capacity to go out, and
	 * reading input as well.  Prevents deadlocking.
	 */
	if( (i = writev( pc->pkc_fd, cmdvec, (len>0)?2:1 )) != len+sizeof(hdr) )  {
		if( i < 0 )  {
			perror("pkg_send: write");
			return(-1);
		}
		fprintf(stderr,"pkg_send of %d+%d, wrote %d\n",
			sizeof(hdr), len, i);
		return(i-sizeof(hdr));	/* amount of user data sent */
	}
	return(len);
}

/*
 *  			P K G _ W A I T F O R
 *
 *  This routine implements a blocking read on the network connection
 *  until a message of 'type' type is received.  This can be useful for
 *  implementing the synchronous portions of a query/reply exchange.
 *  All messages of any other type are processed by pkg_get() and
 *  handed off to the handler for that type message in pkg_switch[].
 *
 *  Returns the length of the message actually received, or -1 on error.
 */
int
pkg_waitfor( type, buf, len, pc )
int type;
char *buf;
int len;
register struct pkg_conn *pc;
{
	register int i;

	PKG_CK(pc);
again:
	if( pc->pkc_left >= 0 )
		if( pkg_block( pc ) < 0 )
			return(-1);

	if( pkg_gethdr( pc, buf ) < 0 )  return(-1);
	if( pc->pkc_hdr.pkg_type != type )  {
		/* A message of some other type has unexpectedly arrived. */
		if( pc->pkc_hdr.pkg_len > 0 )  {
			pc->pkc_buf = malloc(pc->pkc_hdr.pkg_len+2);
			pc->pkc_curpos = pc->pkc_buf;
		}
		goto again;
	}
	pc->pkc_left = -1;
	if( pc->pkc_hdr.pkg_len == 0 )
		return(0);
	if( pc->pkc_hdr.pkg_len > len )  {
		register char *bp;
		int excess;
		fprintf(stderr,
			"pkg_waitfor:  message %d exceeds buffer %d\n",
			pc->pkc_hdr.pkg_len, len );
		if( (i = pkg_mread( pc->pkc_fd, buf, len )) != len )  {
			fprintf(stderr,
				"pkg_waitfor:  pkg_mread %d gave %d\n", len, i );
			return(-1);
		}
		excess = pc->pkc_hdr.pkg_len - len;	/* size of excess message */
		bp = malloc(excess);
		if( (i = pkg_mread( pc->pkc_fd, bp, excess )) != excess )  {
			fprintf(stderr,
				"pkg_waitfor: pkg_mread of excess, %d gave %d\n",
				excess, i );
			(void)free(bp);
			return(-1);
		}
		(void)free(bp);
		return(len);		/* truncated, but OK */
	}

	/* Read the whole message into the users buffer */
	if( (i = pkg_mread( pc->pkc_fd, buf, pc->pkc_hdr.pkg_len )) != pc->pkc_hdr.pkg_len )  {
		fprintf(stderr,
			"pkg_waitfor:  pkg_mread %d gave %d\n", pc->pkc_hdr.pkg_len, i );
		return(-1);
	}
	return( pc->pkc_hdr.pkg_len );
}

/*
 *  			P K G _ B W A I T F O R
 *
 *  This routine implements a blocking read on the network connection
 *  until a message of 'type' type is received.  This can be useful for
 *  implementing the synchronous portions of a query/reply exchange.
 *  All messages of any other type are processed by pkg_get() and
 *  handed off to the handler for that type message in pkg_switch[].
 *  The buffer to contain the actual message is acquired via malloc().
 *
 *  Returns pointer to message buffer, or NULL.
 */
char *
pkg_bwaitfor( type, pc )
int type;
register struct pkg_conn *pc;
{
	register int i;

	PKG_CK(pc);
	do  {
		/* Finish any unsolicited msg */
		if( pc->pkc_left >= 0 )
			if( pkg_block(pc) < 0 )
				return((char *)0);
		if( pkg_gethdr( pc, (char *)0 ) < 0 )
			return((char *)0);
	}  while( pc->pkc_hdr.pkg_type != type );

	pc->pkc_left = -1;
	if( pc->pkc_hdr.pkg_len == 0 )
		return((char *)0);

	/* Read the whole message into the dynamic buffer */
	if( (i = pkg_mread( pc->pkc_fd, pc->pkc_buf, pc->pkc_hdr.pkg_len )) != pc->pkc_hdr.pkg_len )  {
		fprintf(stderr,
			"pkg_bwaitfor:  pkg_mread %d gave %d\n", pc->pkc_hdr.pkg_len, i );
	}
	return( pc->pkc_buf );
}

/*
 *  			P K G _ G E T
 *
 *  This routine should be called whenever select() indicates that there
 *  is data waiting on the network connection and the user's program is
 *  not blocking on the arrival of a particular message type.
 *  As portions of the message body arrive, they are read and stored.
 *  When the entire message body has been read, it is handed to the
 *  user-provided message handler for that type message, from pkg_switch[].
 *
 *  If this routine is called when there is no message already being
 *  assembled, and there is no input on the network connection, it will
 *  block, waiting for the arrival of the header.
 *
 *  Returns -1 on error, 0 if more data comming, 1 if user handler called.
 *  The user handler is responsible for calling free()
 *  on the message buffer when finished with it.
 */
int
pkg_get(pc)
register struct pkg_conn *pc;
{
	register int i;
	struct timeval tv;
	auto long bits;

	PKG_CK(pc);
	if( pc->pkc_left < 0 )  {
		if( pkg_gethdr( pc, (char *)0 ) < 0 )  return(-1);
		if( pc->pkc_left == 0 )  return( pkg_dispatch(pc) );

		/* See if any message body has arrived yet */
		tv.tv_sec = 0;
		tv.tv_usec = 20000;	/* 20 ms */
		bits = (1<<pc->pkc_fd);
		i = select( pc->pkc_fd+1, (char *)&bits, (char *)0, (char *)0, &tv );
		if( i <= 0 )  return(0);	/* timed out */
		if( !bits )  return(0);		/* no input */
	}

	/* read however much is here already, and remember our position */
	if( pc->pkc_left > 0 )  {
		if( (i = read( pc->pkc_fd, pc->pkc_curpos, pc->pkc_left )) <= 0 )  {
			pc->pkc_left = -1;
			perror("pkg_get: read");
			return(-1);
		}
		pc->pkc_curpos += i;
		pc->pkc_left -= i;
		if( pc->pkc_left > 0 )
			return(0);		/* more is on the way */
	}

	return( pkg_dispatch(pc) );
}

/*
 *			P K G _ D I S P A T C H
 *
 *  Given that a whole message has arrived, send it to the appropriate
 *  User Handler, or else grouse.
 *  Returns -1 on fatal error, 0 on no handler, 1 if all's well.
 */
int
pkg_dispatch(pc)
register struct pkg_conn *pc;
{
	register int i;

	PKG_CK(pc);
	if( pc->pkc_left != 0 )  return(-1);

	/* Whole message received, process it via switchout table */
	for( i=0; i < pkg_swlen; i++ )  {
		register char *tempbuf;

		if( pkg_switch[i].pks_type != pc->pkc_hdr.pkg_type )
			continue;
		/*
		 * NOTICE:  User Handler must free() message buffer!
		 * WARNING:  Handler may recurse back to pkg_get() --
		 * reset all connection state variables first!
		 */
		tempbuf = pc->pkc_buf;
		pc->pkc_buf = (char *)0;
		pc->pkc_curpos = (char *)0;
		pc->pkc_left = -1;		/* safety */
		/* pc->pkc_hdr.pkg_type, pc->pkc_hdr.pkg_len are preserved */
		pkg_switch[i].pks_handler(pc, tempbuf);
		return(1);
	}
	fprintf(stderr,"pkg_get:  no handler for message type %d, len %d\n",
		pc->pkc_hdr.pkg_type, pc->pkc_hdr.pkg_len );
	(void)free(pc->pkc_buf);
	pc->pkc_buf = (char *)0;
	pc->pkc_curpos = (char *)0;
	pc->pkc_left = -1;		/* safety */
	return(0);
}
/*
 *			P K G _ G E T H D R
 *
 *  Get header from a new message.
 *  Returns:
 *	1	when there is some message to go look at
 *	-1	on fatal errors
 */
int
pkg_gethdr( pc, buf )
register struct pkg_conn *pc;
char *buf;
{
	register int i;

	PKG_CK(pc);
	if( pc->pkc_left >= 0 )  return(1);	/* go get it! */

	/*
	 *  At message boundary, read new header.
	 *  This will block until the new header arrives (feature).
	 */
	if( (i = pkg_mread( pc->pkc_fd, &(pc->pkc_hdr),
	    sizeof(struct pkg_header) )) != sizeof(struct pkg_header) )  {
		if(i > 0)
			fprintf(stderr,"pkg_gethdr: header read of %d?\n", i);
		return(-1);
	}
	while( pc->pkc_hdr.pkg_magic != htons(PKG_MAGIC ) )  {
		fprintf(stderr,"pkg_gethdr: skipping noise\n");
		/* Slide over one byte and try again */
		bcopy( ((char *)&pc->pkc_hdr)+1, (char *)&pc->pkc_hdr, sizeof(struct pkg_header)-1);
		if( (i=read( pc->pkc_fd,
		    ((char *)&pc->pkc_hdr)+sizeof(struct pkg_header)-1,
		    1 )) != 1 )  {
			fprintf(stderr,"pkg_gethdr: hdr read=%d?\n",i);
			return(-1);
		}
	}
	pc->pkc_hdr.pkg_type = ntohs(pc->pkc_hdr.pkg_type);	/* host order */
	pc->pkc_hdr.pkg_len = ntohl(pc->pkc_hdr.pkg_len);
	if( pc->pkc_hdr.pkg_len < 0 )  pc->pkc_hdr.pkg_len = 0;
	pc->pkc_buf = (char *)0;
	pc->pkc_left = pc->pkc_hdr.pkg_len;
	if( pc->pkc_left == 0 )  return(1);		/* msg here, no data */

	if( buf )  {
		pc->pkc_buf = buf;
	} else {
		/* Prepare to read message into dynamic buffer */
		pc->pkc_buf = malloc(pc->pkc_hdr.pkg_len+2);
	}
	pc->pkc_curpos = pc->pkc_buf;
	return(1);			/* something ready */
}

/*
 *  			P K G _ B L O C K
 *  
 *  This routine blocks, waiting for one complete message to arrive from
 *  the network.  The actual handling of the message is done with
 *  pkg_get(), which invokes the user-supplied message handler.
 *
 *  This routine can be used in a loop to pass the time while waiting
 *  for a flag to be changed by the arrival of an asynchronous message,
 *  or for the arrival of a message of uncertain type.
 *  
 *  Control returns to the caller after one full message is processed.
 *  Returns -1 on error, etc.
 */
int
pkg_block(pc)
register struct pkg_conn *pc;
{
	register int i;

	PKG_CK(pc);
	if( pc->pkc_left == 0 )  return( pkg_dispatch(pc) );

	/* If no read outstanding, start one. */
	if( pc->pkc_left < 0 )  {
		if( pkg_gethdr( pc, (char *)0 ) < 0 )  return(-1);
		if( pc->pkc_left <= 0 )  return( pkg_dispatch(pc) );
	}

	/* Read the rest of the message, blocking in read() */
	if( pc->pkc_left > 0 && pkg_mread( pc->pkc_fd, pc->pkc_curpos, pc->pkc_left ) != pc->pkc_left )  {
		pc->pkc_left = -1;
		return(-1);
	}
	pc->pkc_left = 0;
	return( pkg_dispatch(pc) );
}
