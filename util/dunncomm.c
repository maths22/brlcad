/*
 *			D U N N C O M M . C
 *
 *	Common routines needed for both dunncolor and dunnsnap
 *
 *  Author -
 *	Don Merritt
 *	August 1985
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
#include <fcntl.h>
#include <signal.h>

#ifdef BSD
# include <sys/time.h>
# include <sys/ioctl.h>
struct	sgttyb	tty;
#define TCSETA	TIOCSETP
#define TCGETA	TIOCGETP
#ifndef	XTABS
#define	XTABS	(TAB1 | TAB2)
#endif XTABS
#endif

#ifdef SYSV
struct timeval {
	int	tv_sec;
	int	tv_usec;
};
# include <termio.h>
struct	termio	tty;
#endif SYSV

int fd;
char cmd;
unsigned char	status[4];
unsigned char	values[21];
int	readfds;
int	polaroid = 0;		/* 0 = aux camera, 1 = Polaroid 8x10 */

unsnooze()
{
	printf("\007dunnsnap: request timed out, aborting\n");
	exit(1);
}

/*
 *			D U N N O P E N
 */
void
dunnopen()
{

	/* open the camera device */

	if( (fd = open("/dev/camera", O_RDWR | O_NDELAY)) < 0 
	     || ioctl(fd, TCGETA, &tty) < 0) {
	     	printf("\007dunnopen: can't open /dev/camera\n");
		close(fd);
		exit(10);
	}
	
	/* set up the camera device */
	
#ifdef BSD
	tty.sg_ispeed = tty.sg_ospeed = B9600;
	tty.sg_flags = RAW | EVENP | ODDP | XTABS;
#endif
#ifdef SYSV
	tty.c_cflag = B9600 | CS8;	/* Character size = 8 bits */
	tty.c_cflag &= ~CSTOPB;		/* One stop bit */
	tty.c_cflag |= CREAD;		/* Enable the reader */
	tty.c_cflag &= ~PARENB;		/* Parity disable */
	tty.c_cflag &= ~HUPCL;		/* No hangup on close */
	tty.c_cflag |= CLOCAL;		/* Line has no modem control */

	tty.c_iflag &= ~(BRKINT|ICRNL|INLCR|IXON|IXANY|IXOFF);
	tty.c_iflag |= IGNBRK|IGNPAR;

	tty.c_oflag &= ~(OPOST|ONLCR|OCRNL);	/* Turn off all post-processing */
	tty.c_oflag |= TAB3;		/* output tab expansion ON */
	tty.c_cc[VMIN] = 1;
	tty.c_cc[VTIME] = 0;

	tty.c_lflag &= ~ICANON;		/* Raw mode */
	tty.c_lflag &= ~ISIG;		/* Signals OFF */
	tty.c_lflag &= ~(ECHO|ECHOE|ECHOK);	/* Echo mode OFF */
#endif
	if( ioctl(fd, TCSETA, &tty) < 0 ) {
		perror("/dev/camera");
		exit(20);
	}

	/* Be certain the FNDELAY is off */
	if( fcntl(fd, F_SETFL, 0) < 0 )  {
		perror("/dev/camera");
		exit(21);
	}

	/* Set up alarm clock catcher */
	(void)signal( SIGALRM, unsnooze );
}

/*
 *			M R E A D
 *
 * This function performs the function of a read(II) but will
 * call read(II) multiple times in order to get the requested
 * number of characters.  This can be necessary because pipes
 * and network connections don't deliver data with the same
 * grouping as it is written with.
 */
static int
mread(fd, bufp, n)
int fd;
register char	*bufp;
unsigned	n;
{
	register unsigned	count = 0;
	register int		nread;

	do {
		nread = read(fd, bufp, n-count);
		if(nread == -1)
			return(nread);
		if(nread == 0)
			return((int)count);
		count += (unsigned)nread;
		bufp += nread;
	 } while(count < n);

	return((int)count);
}

/*
 *			G O O D S T A T U S
 *
 *	Checks the status of the Dunn camera and returns 1 for good status
 *	and 0 for bad status.
 *
 */

goodstatus()
{
	struct timeval waittime, *timeout;
	
	timeout = &waittime;
	timeout->tv_sec = 10;
	timeout->tv_usec = 0;
	
	cmd = ';';	/* status request cmd */
	write(fd, &cmd, 1);	
#ifdef BSD
	readfds = 1<<fd;
	select(fd+1, &readfds, (int *)0, (int *)0, timeout);
	if( (readfds & (1<<fd)) ==0 ) {
		printf("\007dunnsnap: status request timed out\n");
		return(0);
	}
#else
	/* Set an alarm, and then just hang a read */
	alarm(timeout->tv_sec);
#endif

	mread(fd, status, 4);
	alarm(0);

	if (status[0]&0x1)  printf("No vertical sync\n");
	if (status[0]&0x2)  printf("8x10 not ready\n");
	if (status[0]&0x4)  printf("Expose in wrong mode\n");
	if (status[0]&0x8)  printf("Aux camera out of film\n");
	if (status[1]&0x1)  printf("B/W mode\n");
	if (status[1]&0x2)  printf("Separate mode\n");
	if (status[2]&0x1)  printf("Y-smoothing off\n");

	if ((status[0]&0xf) == 0x0 &&
	    (status[1]&0x3) == 0x0 &&
	    (status[3]&0x7f)== '\r')
		return 1;	/* status is ok */

	printf("\007dunnsnap: status error from camera\n");
	printf("status[0]= 0x%x [1]= 0x%x [2]= 0x%x [3]= 0x%x\n",
		status[0]&0x7f,status[1]&0x7f,
		status[2]&0x7f,status[3]&0x7f);
	return 0;	/* status is bad or request timed out */
}

/*
 *			H A N G T E N 
 *
 *	Provides a 10 millisecond delay when called
 *
 */
void
hangten()
{
	static struct timeval delaytime = { 0, 10000}; /* set timeout to 10mS*/

#ifdef BSD
	select(0, (int *)0, (int *)0, (int *)0, &delaytime);
#else
	sleep(1);
#endif
}

/*
 *			R E A D Y
 *
 *	Sends a ready test command to the Dunn camera and returns 1 if the
 *	camera is ready or 0 if the camera is not ready after waiting the
 *	number of seconds specified by the argument.
 *
 */
ready(nsecs)
int nsecs;
{
	struct timeval waittime, *timeout;
	register int i;
	timeout = &waittime;
	timeout->tv_sec = nsecs;
	timeout->tv_usec = 0;
	
	cmd = ':';	/* ready test command */
	write(fd, &cmd, 1);
#ifdef BSD
	readfds = 1<<fd;
	select(fd+1, &readfds, (int *)0, (int *)0, timeout);
	if ((readfds & (1<<fd)) != 0) {
		return 0;	/* timeout after n secs */
	}
#else
	alarm(nsecs);
#endif
	status[0] = status[1] = '\0';
	/* This loop is needed to skip leading nulls in input stream */
	do {
		i = read(fd, &status[0], 1);
		if( i != 1 )  {
			printf("dunnsnap:  unexpected EOF %d\n", i);
			return(0);
		}
	} while( status[0] == '\0' );
	(void)read(fd, &status[1], 1);
	alarm(0);

	if((status[0]&0x7f) == 'R' && (status[1]&0x7f) == '\r')
		return 1;	/* camera is ready */

	printf("dunnsnap/ready():  unexpected camera status 0%o 0%o\n",
		status[0]&0x7f, status[1]&0x7f);
	return 0;	/* camera is not ready */
}

/*
 *			G E T E X P O S U R E
 *
 *  Get and print the current exposure
 */
void
getexposure(title)
char *title;
{
	struct timeval waittime;

	waittime.tv_sec = 20;
	waittime.tv_usec = 0;

	if(!ready(20)) {
		printf("dunncolor: (getexposure) camera not ready\n");
		exit(60);
	}

	if(polaroid)
		cmd = '<';	/* req 8x10 exposure values */
	else
		cmd = '=';	/* request AUX exposure values */
	write(fd, &cmd, 1);
#ifdef BSD
	readfds = 1<<fd;
	select(fd+1, &readfds, (int *)0, (int *)0, &waittime);
	if( (readfds&(1<<fd)) == 0) {
		printf("dunncolor:\007 %s request exposure value cmd: timed out\n", title);
		exit(40);
	}
#else
	alarm(waittime.tv_sec);
#endif
	mread(fd, values, 20);
	alarm(0);

	values[20] = '\0';
	printf("dunncolor: %s = %s\n", title, values);
}

/*
 *			S E N D
 *
 */
void
send(color,val)
char color;
int val;
{
	int digit;

	if(val < 0 || val > 255) {
		printf("dunncolor: bad value %d\n",val);
		exit(75);
	}

	if(!ready(20)) {
		printf("dunncolor: 80 camera not ready\n");
		exit(80);
	}

	if( polaroid )
		cmd = 'K';	/* set 8x10 exposure values */
	else
		cmd = 'L';	/* set AUX exposure values */
	write(fd, &cmd, 1);
	hangten();
	write(fd, &color, 1);
	hangten();
	digit = (val/100 + 0x30)&0x7f;
	write(fd, &digit, 1);
	hangten();
	val = val%100;
	digit = (val/10 + 0x30)&0x7f;
	write(fd, &digit, 1);
	hangten();
	digit = (val%10 + 0x30)&0x7f;
	write(fd, &digit,1);
}
