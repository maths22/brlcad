/*
	SCCS id:	@(#) empty.c	2.3
	Modified: 	1/5/87 at 16:52:54
	Retrieved: 	1/5/87 at 16:58:01
	SCCS archive:	/vld/moss/src/fbed/s.empty.c

	Author:		Gary S. Moss
			U. S. Army Ballistic Research Laboratory
			Aberdeen Proving Ground
			Maryland 21005-5066
			(301)278-6647 or AV-298-6647
*/
#if ! defined( lint )
static
char	sccsTag[] = "@(#) empty.c 2.3, modified 1/5/87 at 16:52:54, archive /vld/moss/src/fbed/s.empty.c";
#endif
#ifdef BSD
#include <sys/types.h>
#include <sys/time.h>
#endif

#ifdef SYSV
#ifdef VLDSYSV
/* BRL SysV under 4.2 */
#define select	_select
struct timeval
	{
	long	tv_sec;		/* seconds */
	long	tv_usec;	/* and microseconds */
	};
#else
/* Regular SysV */
#ifdef sgi
#include <bsd/sys/types.h>
#include <bsd/sys/time.h>
#include "fb.h"
#endif sgi
#endif VLDSYSV
#endif SYSV

#ifndef FD_ZERO
/* 4.2 does not define these */
#define	FD_SET(n, p)	((p)->fds_bits[0] |= (n) == 0 ? 1 : (1 << (n)))
#define FD_ZERO(p)	(p)->fds_bits[0] = 0
/** typedef	struct fd_set { fd_mask	fds_bits[1]; } fd_set; **/
#endif FD_ZERO

/*	e m p t y ( )
	Examine file descriptor for input with no time delay.
	Return 1 if input is pending on file descriptor 'fd'.
	Return 0 if no input or error.
 */
int
empty( fd )
int	fd;
	{
#ifdef sgi
		extern FBIO	*fbp;
	if( fbp != FBIO_NULL && strncmp( fbp->if_name, "/dev/sgi", 8 ) == 0 )
		return	sgi_Empty();
	else
#endif
		{	static struct timeval	timeout = { 0L, 600L };
			fd_set		readfds;
			register int	nfound;
		FD_ZERO( &readfds );
		FD_SET( fd, &readfds );
		nfound = select( fd+1, &readfds, (fd_set *)0, (fd_set *)0, &timeout );
		return	nfound == -1 ? 1 : (nfound == 0);
		}
	}
