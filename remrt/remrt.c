/*
 *  			R E M R T . C
 *  
 *  Controller for network ray-tracing
 *  Operating as both a network client and server,
 *  starts remote invocations of "rtsrv" via "rsh", then
 *  handles incomming connections offering cycles,
 *  accepting input (solicited and unsolicited) via the pkg.c routines,
 *  and storing the results in files and/or a framebuffer.
 *
 *  Author -
 *	Michael John Muuss
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
#include <signal.h>
#include <errno.h>
#include <netdb.h>

#include <sys/types.h>
#include <sys/time.h>		/* for struct timeval */
#include <sys/socket.h>
#include <netinet/in.h>

#include "fb.h"
#include "pkg.h"

#include "./rtsrv.h"
#include "./remrt.h"

FBIO *fbp = FBIO_NULL;		/* Current framebuffer ptr */
int cur_fbsize;			/* current fb size */

int running = 0;		/* actually working on it */
int detached = 0;		/* continue after EOF */

/*
 * Package Handlers.
 */
int ph_default();	/* foobar message handler */
int ph_pixels();
int ph_print();
int ph_start();
struct pkg_switch pkgswitch[] = {
	{ MSG_START, ph_start, "Startup ACK" },
	{ MSG_MATRIX, ph_default, "Set Matrix" },
	{ MSG_OPTIONS, ph_default, "Set options" },
	{ MSG_LINES, ph_default, "Compute lines" },
	{ MSG_END, ph_default, "End" },
	{ MSG_PIXELS, ph_pixels, "Pixels" },
	{ MSG_PRINT, ph_print, "Log Message" },
	{ 0, 0, (char *)0 }
};

int clients;
int print_on = 1;

#define MAXARGS 48
char *cmd_args[MAXARGS];
int numargs;

#define NFD 32

/* Lists */
struct list {
	struct list	*li_forw;
	struct list	*li_back;
	struct frame	*li_frame;
	int		li_start;
	int		li_stop;
};
struct list *FreeList;

struct frame {
	struct frame	*fr_forw;
	struct frame	*fr_back;
	int		fr_number;	/* frame number */
	int		fr_size;	/* frame size (pixels) */
	char		*fr_picture;	/* ptr to picture buffer */
	struct list	fr_todo;	/* work still to be done */
	long		fr_start;	/* start time */
	long		fr_end;		/* end time */
	int		fr_hyper;	/* hypersampling */
	double		fr_zoomout;	/* perspective */
	/* Current view */
	double		fr_viewsize;
	double		fr_eye_model[3];
	double		fr_mat[16];
};
struct frame FrameHead;
struct frame *FreeFrame;
#define FRAME_NULL	((struct frame *)0)

struct servers {
	struct pkg_conn	*sr_pc;	/* PKC_NULL means slot not in use */
	struct list	sr_work;
	int		sr_speed;	/* # lines to send at once */
	int		sr_started;
#define SRST_NEW	1	/* connected, no model loaded yet */
#define SRST_LOADING	2	/* loading, awaiting ready response */
#define SRST_READY	3	/* loaded, ready */
	struct frame	*sr_curframe;	/* ptr to current frame */
	long		sr_addr;	/* NET order inet addr */
	char		sr_name[32];	/* host name */
} servers[NFD];
#define SERVERS_NULL	((struct servers *)0)

/* Options */
int npts = 64;
int hypersample = 0;
double zoomout = 0;
extern double atof();

/* START */
char start_cmd[256];	/* contains file name & objects */

char ourname[32];
char out_file[256];		/* output file name */

extern char *malloc();

int tcp_listen_fd;

/*
 *			E R R L O G
 *
 *  Log an error.  We supply the newline, and route to user.
 */
/* VARARGS */
void
errlog( str, a, b, c, d, e, f, g, h )
char *str;
{
	char buf[256];		/* a generous output line */

	(void)fprintf( stderr, str, a, b, c, d, e, f, g, h );
}

/*
 *			M A I N
 */
main(argc, argv)
int argc; char **argv;
{
	register int i;
	register struct servers *sp;

	/* Random inits */
	gethostname( ourname, sizeof(ourname) );
	FrameHead.fr_forw = FrameHead.fr_back = &FrameHead;
	for( sp = &servers[0]; sp < &servers[NFD]; sp++ )  {
		sp->sr_work.li_forw = sp->sr_work.li_back = &sp->sr_work;
		sp->sr_pc = PKC_NULL;
	}

	/* Listen for our PKG connections */
	if( (tcp_listen_fd = pkg_initserver("rtsrv", 8, errlog)) < 0 )
		exit(1);
	printf("REMRT listening\n");
	(void)signal( SIGPIPE, SIG_IGN );
	clients = (1<<0);

	while(clients)  {
		(void)signal( SIGINT, SIG_IGN );
		check_input( 30 );	/* delay 30 secs */
	}
	/* Might want to see if any work remains, and if so,
	 * record it somewhere */
	printf("REMRT out of clients\n");
	exit(0);
}

check_input(waittime)
int waittime;
{
	static long ibits, obits, ebits;
	register struct list *lp;
	register int i;
	static struct timeval tv;

	tv.tv_sec = waittime;
	tv.tv_usec = 0;

	ibits = clients|(1<<tcp_listen_fd);
	i=select(32, (char *)&ibits, (char *)0, (char *)0, &tv);
	if( i < 0 )  {
		perror("select");
		return;
	}
	/* First, accept any pending connections */
	if( ibits & (1<<tcp_listen_fd) )  {
		register struct pkg_conn *pc;
		pc = pkg_getclient(tcp_listen_fd, pkgswitch, errlog, 1);
		if( pc != PKC_NULL && pc != PKC_ERROR )
			addclient(pc);
		ibits &= ~(1<<tcp_listen_fd);
	}
	/* Second, give priority to getting traffic off the network */
	for( i=3; i<NFD; i++ )  {
		register struct pkg_conn *pc;
		if( !(ibits&(1<<i)) )  continue;
		pc = servers[i].sr_pc;
		if( pkg_get(pc) < 0 )
			dropclient(pc);
	}
	/* Finally, handle any command input (This can recurse via "read") */
	if( waittime>0 && (ibits & (1<<(fileno(stdin)))) )  {
		interactive(stdin);
	}
}

addclient(pc)
struct pkg_conn *pc;
{
	register struct servers *sp;
	int on = 1, options = 0, fromlen;
	struct sockaddr_in from;
	register struct hostent *hp;
	int fd;

	fd = pc->pkc_fd;

	fromlen = sizeof (from);
	if (getpeername(fd, &from, &fromlen) < 0) {
		perror("getpeername");
		close(fd);
		return;
	}
	if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof (on)) < 0)
		perror("setsockopt (SO_KEEPALIVE)");

	clients |= 1<<fd;

	sp = &servers[fd];
	sp->sr_pc = pc;
	sp->sr_work.li_forw = sp->sr_work.li_back = &(sp->sr_work);
	sp->sr_started = SRST_NEW;
	sp->sr_curframe = FRAME_NULL;
	sp->sr_speed = 3;
	sp->sr_addr = from.sin_addr.s_addr;
	hp = gethostbyaddr(&from.sin_addr, sizeof (struct in_addr),
		from.sin_family);
	if (hp == 0) {
		sprintf( sp->sr_name, "x%x", ntohl(sp->sr_addr) );
	} else {
		strncpy( sp->sr_name, hp->h_name, sizeof(sp->sr_name) );
	}

	printf("%s: ACCEPT\n", sp->sr_name );

	if( start_cmd[0] != '\0' )  {
		send_start(sp);
		send_loglvl(sp);
	}
}

dropclient(pc)
register struct pkg_conn *pc;
{
	register struct list *lhp, *lp;
	register struct servers *sp;
	int fd;

	fd = pc->pkc_fd;
	sp = &servers[fd];
	printf("REMRT closing fd %d %s\n", fd, sp->sr_name);
	if( fd <= 3 || fd >= NFD )  {
		printf("That's unreasonable, forget it!\n");
		return;
	}
	pkg_close(pc);
	clients &= ~(1<<fd);
	sp->sr_pc = PKC_NULL;

	/* Need to remove any work in progress, and re-schedule */
	lhp = &(sp->sr_work);
	while( (lp = lhp->li_forw) != lhp )  {
		DEQUEUE_LIST( lp );
		printf("requeueing fr%d %d..%d\n",
			lp->li_frame->fr_number,
			lp->li_start, lp->li_stop);
		APPEND_LIST( lp, &(lp->li_frame->fr_todo) );
		printf("ToDo:\n");
		pr_list(&(lp->li_frame->fr_todo));
	}
	if(running) schedule();
}

/*
 *  			S T R I N G 2 I N T
 *  
 *  Convert a string to an integer.
 *  A leading "0x" implies HEX.
 *  If needed, octal might be done, but it seems unwise...
 *
 *  For general conversion, this is pretty feeble.  Is more needed?
 */
int
string2int(str)
register char *str;
{
	auto int ret;
	int cnt;

	ret = 0;
	if( str[0] == '0' && str[1] == 'x' )
		cnt = sscanf( str+2, "%x", &ret );
	else
		cnt = sscanf( str, "%d", &ret );
	if( cnt != 1 )
		printf("string2int(%s) = %d?\n", str, ret );
	return(ret);
}

/*
 *			H O S T 2 A D D R
 */
long
host2addr(str)
char *str;
{
	if( str[0] == '0' || atoi(str) > 0 )  {
		/* Numeric */
		register int i;
		if( (i=inet_addr(str)) != 0 )
			return(i);
		return(string2int(str));
	} else {
		register struct hostent *addr;
		long i;
		if ((addr=gethostbyname(str)) == NULL)
			return(0);
		bcopy(addr->h_addr,(char*)&i, sizeof(long));
		return(i);
	}
}
struct servers *
get_server_by_name( str )
char *str;
{
	register long i;
	register struct servers *sp;

	if( *str == '#' )  {
		i = atoi( str+1 );
		if( i < 0 || i > NFD )  return( SERVERS_NULL );
		return( &servers[i] );
	}
	i = host2addr(str);
	for( sp = &servers[0]; sp < &servers[NFD]; sp++ )  {
		if( sp->sr_pc == PKC_NULL )  continue;
		if( sp->sr_addr == i )  return(sp);
	}
	return( SERVERS_NULL );
}

interactive(fp)
FILE *fp;
{
	char buf[BUFSIZ];
	char obuf[256];
	register char *pos;
	register int i;

	pos = buf;

	/* Get first line */
	*pos = '\0';
	(void)fgets( pos, sizeof(buf), fp );
	i = strlen(buf);

	/* If continued, get more */
	while( pos[i-1]=='\n' && pos[i-2]=='\\' )  {
		pos += i-2;	/* zap NL and backslash */
		*pos = '\0';
		printf("-> "); (void)fflush(stdout);
		(void)fgets( pos, sizeof(buf)-strlen(buf), fp );
		i = strlen(pos);
	}

	if( feof(fp) ) {
		register struct servers *sp;

		if( fp != stdin )  return;

		/* Eof on stdin */
		clients &= ~(1<<fileno(fp));
		close( tcp_listen_fd ); tcp_listen_fd = -1;

		/* We might want to wait if something is running? */
		for( sp = &servers[0]; sp < &servers[NFD]; sp++ )  {
			if( sp->sr_pc == PKC_NULL )  continue;
			if( !running )
				dropclient(sp->sr_pc);
			else
				send_restart(sp);
		}
		/* The rest happens when the connections close */
		return;
	}
	if( (i=strlen(buf)) <= 0 )  return;

	/* Feeble allowance for comments */
	if( buf[0] == '#' )  return;

	if( strncmp( buf, "load ", 5 ) == 0 )  {
		register struct servers *sp;
		if( running )  {
			printf("Can't load while running!!\n");
			return;
		}
		/* Really ought to reset here, too */
		if(start_cmd[0] != '\0' )  {
			printf("Was loaded with %s, restarting all\n", start_cmd);
			strncpy( start_cmd, &buf[5], strlen(&buf[5])-1 );
			for( sp = &servers[0]; sp < &servers[NFD]; sp++ )  {
				if( sp->sr_pc == PKC_NULL )  continue;
				send_restart( sp );
			}
			return;
		}
		strncpy( start_cmd, &buf[5], strlen(&buf[5])-1 );
		/* Start any idle servers */
 		for( sp = &servers[0]; sp < &servers[NFD]; sp++ )  {
			if( sp->sr_pc == PKC_NULL )  continue;
			if( sp->sr_started != SRST_NEW )  continue;
			send_start(sp);
		}
		return;
	}

	/* Chop up line */
	if( parse_cmd( buf ) > 0 ) return;	/* was nop */

	if( strcmp( cmd_args[0], "debug" ) == 0 )  {
		register struct servers *sp;
		char rbuf[64];

		sprintf(rbuf, "-x%s", cmd_args[1] );
		for( sp = &servers[0]; sp < &servers[NFD]; sp++ )  {
			if( sp->sr_pc == PKC_NULL )  continue;
			(void)pkg_send( MSG_OPTIONS, rbuf, strlen(buf)+1, sp->sr_pc);
		}
		return;
	}
	if( strcmp( cmd_args[0], "f" ) == 0 )  {
		npts = atoi( cmd_args[1] );
		if( npts < 4 || npts > 8*1024 )
			npts = 64;
		printf("npts=%d, takes effect after next MAT\n", npts);
		return;
	}
	if( strcmp( cmd_args[0], "h" ) == 0 )  {
		hypersample = atoi( cmd_args[1] );
		printf("hypersample=%d, takes effect after next MAT\n", hypersample);
		return;
	}
	if( strcmp( cmd_args[0], "p" ) == 0 )  {
		zoomout = atof( cmd_args[1] );
		printf("zoomount=%f, takes effect after next MAT\n", zoomout);
		return;
	}
	if( strcmp( cmd_args[0], "read" ) == 0 )  {
		register FILE *fp;

		if( numargs < 2 )  return;
		if( (fp = fopen(cmd_args[1], "r")) == NULL )  {
			perror(cmd_args[1]);
			return;
		}
		while( !feof(fp) )  {
			/* do one command from file */
			interactive(fp);
			/* Without delay, see if anything came in */
			check_input(0);
		}
		fclose(fp);
		printf("read file done\n");
		return;
	}
	if( strcmp( cmd_args[0], "detach" ) == 0 )  {
		detached = 1;
		clients &= ~(1<<0);	/* drop stdin */
		close(0);
		return;
	}
	if( strcmp( cmd_args[0], "file" ) == 0 )  {
		if( numargs < 2 )  return;
		strncpy( out_file, cmd_args[1], sizeof(out_file) );
		printf("frames will be recorded in %s.###\n", out_file);
		return;
	}
	if( strcmp( cmd_args[0], "mat" ) == 0 )  {
		register FILE *fp;
		register struct frame *fr;

		GET_FRAME(fr);

		if( numargs >= 3 )  {
			fr->fr_number = atoi(cmd_args[2]);
		} else {
			fr->fr_number = 0;
		}
		if( (fp = fopen(cmd_args[1], "r")) == NULL )  {
			perror(cmd_args[1]);
			return;
		}
		for( i=fr->fr_number; i>=0; i-- )
			if(read_matrix( fp, fr ) < 0 ) break;
		fclose(fp);

		prep_frame(fr);
		APPEND_FRAME( fr, FrameHead.fr_back );
		return;
	}
	if( strcmp( cmd_args[0], "movie" ) == 0 )  {
		register FILE *fp;
		register struct frame *fr;
		struct frame dummy_frame;
		int a,b;

		/* movie mat a b */
		if( numargs < 4 )  {
			printf("usage:  movie matfile startframe endframe\n");
			return;
		}
		if( running )  {
			printf("already running, please wait\n");
			return;
		}
		if( start_cmd[0] == '\0' )  {
			printf("need LOAD before MOVIE\n");
			return;
		}
		a = atoi( cmd_args[2] );
		b = atoi( cmd_args[3] );
		if( (fp = fopen(cmd_args[1], "r")) == NULL )  {
			perror(cmd_args[1]);
			return;
		}
		/* Skip over unwanted beginning frames */
		for( i=0; i<a; i++ )
			if(read_matrix( fp, &dummy_frame ) < 0 ) break;
		for( i=a; i<b; i++ )  {
			GET_FRAME(fr);
			fr->fr_number = i;
			if(read_matrix( fp, fr ) < 0 ) break;
			prep_frame(fr);
			APPEND_FRAME( fr, FrameHead.fr_back );
		}
		fclose(fp);
		printf("Movie ready\n");
		return;
	}
	if( strcmp( cmd_args[0], "add" ) == 0 )  {
		char cmd[128];
		for( i=1; i<numargs; i++ )  {
			sprintf(cmd,
				"rsh %s 'hostname; cd cad/remrt; rtsrv %s;uptime'",
				cmd_args[i], ourname );
			if( vfork() == 0 )  {
				/* 1st level child */
				(void)close(0);
				for(i=3; i<40; i++)  (void)close(i);
				if( vfork() == 0 )  {
					/* worker Child */
					system( cmd );
					_exit(0);
				}
				_exit(0);
			} else {
				(void)wait(0);
			}
			check_input(1);		/* get connections */
		}
		printf("add finished\n");
		return;
	}
	if( strcmp( cmd_args[0], "drop" ) == 0 )  {
		register struct servers *sp;
		if( numargs <= 1 )  return;
		sp = get_server_by_name( cmd_args[1] );
		if( sp == SERVERS_NULL || sp->sr_pc == PKC_NULL )  return;
		dropclient(sp->sr_pc);
		return;
	}
	if( strcmp( cmd_args[0], "restart" ) == 0 )  {
		register struct servers *sp;
		if( numargs <= 1 )  {
			/* Restart all */
			printf("restarting all\n");
			for( sp = &servers[0]; sp < &servers[NFD]; sp++ )  {
				if( sp->sr_pc == PKC_NULL )  continue;
				send_restart( sp );
			}
			return;
		}
		sp = get_server_by_name( cmd_args[1] );
		if( sp == SERVERS_NULL || sp->sr_pc == PKC_NULL )  return;
		send_restart( sp );
		/* The real action takes place when he closes the conn */
		return;
	}
	if( strcmp( cmd_args[0], "stop" ) == 0 )  {
		printf("no more scanlines being scheduled, done soon\n");
		running = 0;
		return;
	}
	if( strcmp( cmd_args[0], "reset" ) == 0 )  {
		register struct frame *fr;
		register struct list *lp;

		if( running )  {
			printf("must STOP before RESET!\n");
			return;
		}
		for( fr = FrameHead.fr_forw; fr != &FrameHead; fr=fr->fr_forw )  {
			if( fr->fr_picture )  free(fr->fr_picture);
			fr->fr_picture = (char *)0;
			/* Need to remove any pending work,
			 * work already assigned will dribble in.
			 */
			while( (lp = fr->fr_todo.li_forw) != &(fr->fr_todo) )  {
				DEQUEUE_LIST( lp );
				FREE_LIST(lp);
			}
			/* We will leave cleanups to schedule() */
		}
		return;
	}
	if( strcmp( cmd_args[0], "attach" ) == 0 )  {
		register struct frame *fr;
		int maxx;

		if( init_fb(cmd_args[1]) < 0 )  return;

		if( (fr = FrameHead.fr_forw) == &FrameHead )  return;

		/* Draw the accumulated image */
		if( fr->fr_picture == (char *)0 )  return;
		size_display(fr->fr_size);
		if( fbp == FBIO_NULL ) return;
		/* Trim to what can be drawn */
		maxx = fr->fr_size;
		if( maxx > fb_getwidth(fbp) )
			maxx = fb_getwidth(fbp);
		for( i=0; i<fr->fr_size; i++ )  {
			fb_write( fbp, 0, i%fb_getwidth(fbp),
				fr->fr_picture + i*fr->fr_size*3,
				maxx );
		}
		return;
	}
	if( strcmp( cmd_args[0], "release" ) == 0 )  {
		if(fbp != FBIO_NULL) fb_close(fbp);
		fbp = FBIO_NULL;
		return;
	}
	if( strcmp( cmd_args[0], "frames" ) == 0 )  {
		register struct frame *fr;

		/* Sumarize frames waiting */
		printf("Frames waiting:\n");
		for(fr=FrameHead.fr_forw; fr != &FrameHead; fr=fr->fr_forw) {
			printf(" %d  ", fr->fr_number);
			printf("size=%d, zoomout=%f, ",
				fr->fr_size, fr->fr_zoomout );
			printf("viewsize = %f, ", fr->fr_viewsize);
			printf("hypersample = %d", fr->fr_hyper);
			if( fr->fr_picture )  printf(" (Pic)");
			printf("\n");
			pr_list( &(fr->fr_todo) );
		}
		return;
	}
	if( strcmp( cmd_args[0], "stat" ) == 0 )  {
		register struct servers *sp;

		if( start_cmd[0] == '\0' )
			printf("No model loaded yet\n");
		else
			printf("\n%s %s\n",
				running ? "RUNNING" : "loaded",
				start_cmd );

		if( fbp != FBIO_NULL )
			printf("Framebuffer is %s\n", fbp->if_name);
		else
			printf("No framebuffer\n");
		if( out_file[0] != '\0' )
			printf("Output file: %s.###\n", out_file );
		printf("Printing of remote messages is %s\n",
			print_on?"ON":"Off" );

		/* Print work assignments */
		printf("Servers:\n");
		for( sp = &servers[0]; sp < &servers[NFD]; sp++ )  {
			if( sp->sr_pc == PKC_NULL )  continue;
			printf("  %s ", sp->sr_name );
			switch( sp->sr_started )  {
			case SRST_NEW:
				printf("IDLE\n"); break;
			case SRST_LOADING:
				printf("(Loading)\n"); break;
			case SRST_READY:
				printf("READY\n"); break;
			default:
				printf("Unknown\n"); break;
			}
			pr_list( &(sp->sr_work) );
		}
		return;
	}
	if( strcmp( cmd_args[0], "clear" ) == 0 )  {
		if( fbp == FBIO_NULL )  return;
		fb_clear( fbp, PIXEL_NULL );
		cur_fbsize = 0;
		return;
	}
	if( strcmp( cmd_args[0], "print" ) == 0 )  {
		register struct servers *sp;
		if( numargs > 1 )
			print_on = atoi(cmd_args[1]);
		else
			print_on = !print_on;	/* toggle */
		for( sp = &servers[0]; sp < &servers[NFD]; sp++ )  {
			if( sp->sr_pc == PKC_NULL )  continue;
			send_loglvl( sp );
		}
		printf("Printing of remote messages is %s\n",
			print_on?"ON":"Off" );
		return;
	}
	if( strcmp( cmd_args[0], "go" ) == 0 )  {
		do_a_frame();
		return;
	}
	if( strcmp( cmd_args[0], "?" ) == 0 )  {
		printf("load db.g trees\n");
		printf("f #		set npts\n");
		printf("p #		set zoomout\n");
		printf("h #		set hypersampling\n");
		printf("read script\n");
		printf("mat file	load matrix from file\n");
		printf("movie file a b\n");
		printf("file name	store frames in file\n");
		printf("add host\n");
		printf("drop host\n");
		printf("restart host\n");
		printf("attach		re-attach to display\n");
		printf("release		release display (continue running)\n");
		printf("detach		stop reading stdin\n");
		printf("stat		show server status\n");
		printf("frames		show frame status\n");
		printf("clear		clear screen\n");
		printf("print [0|1]	enable remote logging\n");
		printf("go		start processing\n");
		printf("stop		cease processing\n");
		printf("reset		zap work queues\n");
	}
	printf("%s: Unknown command, ? for help\n", cmd_args[0]);
}

/* Fill in frame structure after reading MAT */
prep_frame(fr)
register struct frame *fr;
{
	register struct list *lp;

	/* Get local buffer for image */
	fr->fr_size = npts;
	fr->fr_zoomout = zoomout;
	if( fr->fr_picture )  free(fr->fr_picture);
	fr->fr_picture = (char *)0;

	/* Build work list */
	fr->fr_todo.li_forw = fr->fr_todo.li_back = &(fr->fr_todo);
	GET_LIST(lp);
	lp->li_frame = fr;
	lp->li_start = 0;
	lp->li_stop = fr->fr_size-1;	/* last scanline # */
	APPEND_LIST( lp, fr->fr_todo.li_back );
}

do_a_frame()
{
	register struct frame *fr;
	if( running )  {
		printf("already running, please wait or STOP\n");
		return;
	}
	if( start_cmd[0] == '\0' )  {
		printf("need LOAD before GO\n");
		return;
	}
	if( (fr = FrameHead.fr_forw) == &FrameHead )  {
		printf("No frames to do!\n");
		return;
	}
	running = 1;
	schedule();
}

/*
 *  If there is work to do, and a free server, send work
 *  When done, we leave the last finished frame around for attach/release.
 */
schedule()
{
	register struct list *lp;
	register struct servers *sp;
	register struct frame *fr;
	int a,b;
	int fd, cnt;
	char name[256];
	int going;			/* number of servers still going */

	if( start_cmd[0] == '\0' )  return;
	going = 0;

	/* Look for finished frames */
	for( fr = FrameHead.fr_forw; fr != &FrameHead; fr = fr->fr_forw )  {
		if( (lp = fr->fr_todo.li_forw) != &(fr->fr_todo) )
			continue;	/* still work to be done */
		for( sp = &servers[0]; sp < &servers[NFD]; sp++ )  {
			if( sp->sr_pc == PKC_NULL )  continue;
			if( sp->sr_curframe != fr )  continue;
			if( sp->sr_work.li_forw != &(sp->sr_work) )  {
				going++;
				goto next_frame;
			}
		}
		/* Frame has just been completed */
		fr->fr_end = time(0);
		printf("frame %d DONE in %d seconds\n",
			fr->fr_number,
			fr->fr_end - fr->fr_start);
		if( out_file[0] != '\0' )  {
			sprintf(name, "%s.%d", out_file, fr->fr_number);
			cnt = fr->fr_size*fr->fr_size*3;
			if( (fd = creat(name, 0444)) > 0 )  {
				printf("Writing..."); fflush(stdout);
				if( write( fd, fr->fr_picture, cnt ) != cnt ) {
					perror(name);
					exit(3);
				} else
					printf(" %s\n", name);
				close(fd);
			} else {
				perror(name);
			}
		}
		if( fr->fr_picture )  free(fr->fr_picture);
		fr->fr_picture = (char *)0;
		sp->sr_curframe = FRAME_NULL;
		{
			register struct frame *fr2;
			fr2 = fr->fr_forw;
			DEQUEUE_FRAME(fr);
			FREE_FRAME(fr);
			fr = fr2->fr_back;
			continue;
		}
next_frame: ;
	}
	if( running == 0 || FrameHead.fr_forw == &FrameHead )  {
		/* No more work to send out */
		if( going > 0 )
			return;
		/* Nothing to do, and nobody still working */
		running = 0;
		printf("REMRT:  All work done!\n");
		if( detached )  exit(0);
		return;
	}

	/* Find first piece of work */
	if( !running )  return;
	for( fr = FrameHead.fr_forw; fr != &FrameHead; fr = fr->fr_forw)  {
top:
		if( (lp = fr->fr_todo.li_forw) == &(fr->fr_todo) )
			continue;	/* none waiting here */

		/*
		 * Look for a free server to dispatch work to
		 */
		for( sp = &servers[0]; sp < &servers[NFD]; sp++ )  {
			if( sp->sr_pc == PKC_NULL )  continue;
			if( sp->sr_started == SRST_NEW )  {
				/*  advance to state 1 (loading) */
				send_start(sp);
				continue;
			}
			if( sp->sr_started != SRST_READY )
				continue;	/* not running yet */
			if( sp->sr_work.li_forw != &(sp->sr_work) )
				continue;	/* busy */

			a = lp->li_start;
			b = a+sp->sr_speed-1;	/* work increment */
			if( b >= lp->li_stop )  {
				b = lp->li_stop;
				DEQUEUE_LIST( lp );
				FREE_LIST( lp );
				lp = LIST_NULL;
			} else
				lp->li_start = b+1;

			printf("fr%d %d..%d -> %s\n", fr->fr_number,
				a, b, sp->sr_name);
			if( sp->sr_curframe != fr )  {
				sp->sr_curframe = fr;
				send_matrix(sp);
			}
			/* Record newly allocated scanlines */
			GET_LIST(lp);
			lp->li_frame = fr;
			lp->li_start = a;
			lp->li_stop = b;
			APPEND_LIST( lp, &(sp->sr_work) );
			if( a == 0 )
				fr->fr_start = time(0);
			send_do_lines( sp, a, b );
			goto top;
		}
		if( sp >= &servers[NFD] )  return;	/* no free servers */
	}
}

read_matrix( fp, fr )
register FILE *fp;
register struct frame *fr;
{
	register int i;
	char number[128];

	/* Visible part is from -1 to +1 in view space */
	if( fscanf( fp, "%s", number ) != 1 )  goto out;
	fr->fr_viewsize = atof(number);
	if( fscanf( fp, "%s", number ) != 1 )  goto out;
	fr->fr_eye_model[0] = atof(number);
	if( fscanf( fp, "%s", number ) != 1 )  goto out;
	fr->fr_eye_model[1] = atof(number);
	if( fscanf( fp, "%s", number ) != 1 )  goto out;
	fr->fr_eye_model[2] = atof(number);
	for( i=0; i < 16; i++ )  {
		if( fscanf( fp, "%s", number ) != 1 )
			goto out;
		fr->fr_mat[i] = atof(number);
	}
	if( feof(fp) ) {
out:
		printf("EOF on frame file.\n");
		return(-1);
	}
	return(0);	/* OK */
}

parse_cmd( line )
char *line;
{
	register char *lp;
	register char *lp1;

	numargs = 0;
	lp = &line[0];
	cmd_args[0] = &line[0];

	if( *lp=='\0' || *lp == '\n' )
		return(1);		/* NOP */

	/* Handle "!" shell escape char so the shell can parse the line */
	if( *lp == '!' )  {
		(void)system( &line[1] );
		(void)printf("!\n");
		return(1);		/* Don't process command line! */
	}

	/* In case first character is not "white space" */
	if( (*lp != ' ') && (*lp != '\t') && (*lp != '\0') )
		numargs++;		/* holds # of args */

	for( lp = &line[0]; *lp != '\0'; lp++ )  {
		if( (*lp == ' ') || (*lp == '\t') || (*lp == '\n') )  {
			*lp = '\0';
			lp1 = lp + 1;
			if( (*lp1 != ' ') && (*lp1 != '\t') &&
			    (*lp1 != '\n') && (*lp1 != '\0') )  {
				if( numargs >= MAXARGS )  {
					(void)printf("More than %d arguments, excess flushed\n", MAXARGS);
					cmd_args[MAXARGS] = (char *)0;
					return(0);
				}
				cmd_args[numargs++] = lp1;
			}
		}
		/* Finally, a non-space char */
	}
	/* Null terminate pointer array */
	cmd_args[numargs] = (char *)0;
	return(0);
}

ph_default(pc, buf)
register struct pkg_conn *pc;
char *buf;
{
	register int i;

	for( i=0; pc->pkc_switch[i].pks_handler != NULL; i++ )  {
		if( pc->pkc_switch[i].pks_type == pc->pkc_type )  break;
	}
	errlog("ctl: unable to handle %s message: len %d",
		pc->pkc_switch[i].pks_title, pc->pkc_len);
	*buf = '*';
	(void)free(buf);
}

/*
 *			P H _ S T A R T
 *
 *  The server answers our MSG_START with various prints, etc,
 *  and then responds with a MSG_START in return, which indicates
 *  that he is ready to accept work now.
 */
ph_start(pc, buf)
register struct pkg_conn *pc;
char *buf;
{
	register struct servers *sp;

	sp = &servers[pc->pkc_fd];
	if(buf) (void)free(buf);
	if( sp->sr_pc != pc )  {
		printf("unexpected MSG_START from fd %d\n", pc->pkc_fd);
		return;
	}
	if( sp->sr_started != SRST_LOADING )  {
		printf("MSG_START in state %d?\n", sp->sr_started);
	}
	sp->sr_started = SRST_READY;
	printf("%s READY FOR WORK\n", sp->sr_name);
	if(running) schedule();
}

ph_print(pc, buf)
register struct pkg_conn *pc;
char *buf;
{
	if(print_on)
		printf("%s:%s", servers[pc->pkc_fd].sr_name, buf );
	if(buf) (void)free(buf);
}

ph_pixels(pc, buf)
register struct pkg_conn *pc;
char *buf;
{
	int line;
	register int i;
	register struct servers *sp;
	register struct frame *fr;

	sp = &servers[pc->pkc_fd];
	fr = sp->sr_curframe;

	line = ((buf[1]&0xFF)<<8) | (buf[0]&0xFF);

	/* Stash pixels in memory in bottom-to-top .pix order */
	if( fr->fr_picture == (char *)0 )  {
		i = fr->fr_size*fr->fr_size*3+3;
		fr->fr_picture = malloc( i );
		if( fr->fr_picture == (char *)0 )  {
			fprintf(stdout, "ph_pixels: malloc(%d) error\n",i);
			fprintf(stderr, "ph_pixels: malloc(%d) error\n",i);
			abort();
			exit(19);
		}
		bzero( fr->fr_picture, i );
	}
	i = fr->fr_size*3;
	if( pc->pkc_len-2 < i )  i = pc->pkc_len-2;
	bcopy( buf+2, fr->fr_picture + line*fr->fr_size*3, i );

	/* If display attached, also draw it */
	if( fbp != FBIO_NULL )  {
		int maxx;
		maxx = i/3;
		if( maxx > fb_getwidth(fbp) )
			maxx = fb_getwidth(fbp);
		size_display(fr->fr_size);
		fb_write( fbp, 0, line%fb_getheight(fbp), buf+2, maxx );
	}
	if(buf) (void)free(buf);

	/* Remove from work list */
	list_remove( &(sp->sr_work), line );
	if( running && sp->sr_work.li_forw == &(sp->sr_work) )
		schedule();
}

/* Given pointer to head of list of ranges, remove the item that's done */
list_remove( lhp, line )
register struct list *lhp;
{
	register struct list *lp;
	for( lp=lhp->li_forw; lp != lhp; lp=lp->li_forw )  {
		if( lp->li_start == line )  {
			if( lp->li_stop == line )  {
				DEQUEUE_LIST(lp);
				return;
			}
			lp->li_start++;
			return;
		}
		if( lp->li_stop == line )  {
			lp->li_stop--;
			return;
		}
		if( line > lp->li_stop )  continue;
		if( line < lp->li_start ) continue;
		/* Need to split range into two ranges */
		/* (start..line-1) and (line+1..stop) */
		{
			register struct list *lp2;
			printf("splitting range into %d %d %d\n", lp->li_start, line, lp->li_stop);
			GET_LIST(lp2);
			lp2->li_start = line+1;
			lp2->li_stop = lp->li_stop;
			lp->li_stop = line-1;
			APPEND_LIST( lp2, lp );
			return;
		}
	}
}

int
init_fb(name)
char *name;
{
	int res = 512;

	if( fbp != FBIO_NULL )  fb_close(fbp);
	while( npts > res )  res <<= 1;
	if( (fbp = fb_open( name, res, res )) == FBIO_NULL )  {
		printf("fb_open %d failed\n", res);
		return(-1);
	}
	fb_wmap( fbp, COLORMAP_NULL );	/* Standard map: linear */
	cur_fbsize = 0;
	return(0);
}
size_display(n)
register int n;
{
	if( cur_fbsize == n )
		return;
	if( fbp == FBIO_NULL )
		return;
	if( n > fb_getwidth(fbp) )  {
		printf("Warning:  fb not big enough for %d pixels, display truncated\n", n );
		cur_fbsize = n;
		return;
	}
	cur_fbsize = n;

	fb_zoom( fbp, fb_getwidth(fbp)/n, fb_getheight(fbp)/n );
	fb_window( fbp, n/2, n/2 );		/* center of view */
}

send_start(sp)
register struct servers *sp;
{
	printf("Sending model info to %s\n", sp->sr_name);
	if( start_cmd[0] == '\0' || sp->sr_started != SRST_NEW )  return;
	if( pkg_send( MSG_START, start_cmd, strlen(start_cmd)+1, sp->sr_pc ) < 0 )
		dropclient(sp->sr_pc);
	sp->sr_started = SRST_LOADING;
}

send_restart(sp)
register struct servers *sp;
{
	if( pkg_send( MSG_RESTART, "", 0, sp->sr_pc ) < 0 )
		dropclient(sp->sr_pc);
}

send_loglvl(sp)
register struct servers *sp;
{
	if( pkg_send( MSG_LOGLVL, print_on?"1":"0", 1, sp->sr_pc ) < 0 )
		dropclient(sp->sr_pc);
}

send_matrix(sp)
struct servers *sp;
{
	char buf[BUFSIZ];
	register struct frame *fr;

	fr = sp->sr_curframe;
	if( fr->fr_viewsize <= 0 )  return;
	if( fr->fr_zoomout > 0 )  {
		sprintf(buf, "-f%d -h%d -p%f",
			fr->fr_size, fr->fr_hyper, fr->fr_zoomout );
	}  else  {
		sprintf(buf, "-f%d -h%d", fr->fr_size, fr->fr_hyper );
	}

	if( pkg_send( MSG_OPTIONS, buf, strlen(buf)+1, sp->sr_pc ) < 0 )
		dropclient(sp->sr_pc);

	sprintf( buf,
		"%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
		fr->fr_viewsize,
		fr->fr_eye_model[0],
		fr->fr_eye_model[1],
		fr->fr_eye_model[2],
		(fr->fr_mat[0]),
		(fr->fr_mat[1]),
		(fr->fr_mat[2]),
		(fr->fr_mat[3]),
		(fr->fr_mat[4]),
		(fr->fr_mat[5]),
		(fr->fr_mat[6]),
		(fr->fr_mat[7]),
		(fr->fr_mat[8]),
		(fr->fr_mat[9]),
		(fr->fr_mat[10]),
		(fr->fr_mat[11]),
		(fr->fr_mat[12]),
		(fr->fr_mat[13]),
		(fr->fr_mat[14]),
		(fr->fr_mat[15]) );
	if( pkg_send( MSG_MATRIX, buf, strlen(buf)+1, sp->sr_pc ) < 0 )
		dropclient(sp->sr_pc);
}
send_do_lines( sp, start, stop )
register struct servers *sp;
{
	char obuf[128];
	sprintf( obuf, "%d %d", start, stop );
	if( pkg_send( MSG_LINES, obuf, strlen(obuf)+1, sp->sr_pc ) < 0 )
		dropclient(sp->sr_pc);
}

pr_list( lhp )
register struct list *lhp;
{
	register struct list *lp;

	for( lp = lhp->li_forw; lp != lhp; lp = lp->li_forw  )  {
		printf("\t%d..%d frame %d\n",
			lp->li_start, lp->li_stop,
			lp->li_frame->fr_number );
	}
}
