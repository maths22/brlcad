/*
 *  			E X T E R N S . H
 *
 *  Declarations for C library routines and UNIX system calls.
 *  Inspired by the ANSI C header file <stdlib.h>
 *  Not claimed to be complete (yet)
 *  
 *  Authors -
 *	Michael John Muuss
 *	Charles M. Kennedy
 *	Phillip Dykstra
 *
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Distribution Status -
 *	Public Domain, Distribution Unlimitied.
 *
 *  $Header$
 */
#ifndef EXTERNS_H
#define EXTERNS_H

/* We need sys/select.h under Irix 5 to get fd_set. */

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

/* Some additional fd_set manipulation routines */

#ifndef FD_OR
#define FD_OR(x, a, b) { register int _i; for (_i = 0; _i < FD_SETSIZE; _i++) \
	if (FD_ISSET(_i, a) || FD_ISSET(_i, b)) FD_SET(_i, x); \
	else FD_CLR(_i, x); }
#endif

#ifndef FD_AND
#define FD_AND(x, a, b) { register int _i; for (_i = 0; _i < FD_SETSIZE; _i++)\
	if (FD_ISSET(_i, a) && FD_ISSET(_i, b)) FD_SET(_i, x); \
	else FD_CLR(_i, x); }
#endif

#ifndef FD_MOVE
#define FD_MOVE(a, b) { register int _i; for (_i = 0; _i < FD_SETSIZE; _i++) \
	if (FD_ISSET(_i, b)) FD_SET(_i, a); else FD_CLR(_i, a); }
#endif

#ifndef FD_OREQ
#define FD_OREQ(a, b) { register int _i; for (_i = 0; _i < FD_SETSIZE; _i++) \
	if (FD_ISSET(_i, b)) FD_SET(_i, a); }
#endif

#ifndef FD_ANDEQ
#define FD_ANDEQ(a, b) { register int _i; for (_i = 0; _i < FD_SETSIZE; _i++) \
	if (!FD_ISSET(_i, a) || !FD_ISSET(_i, b)) FD_CLR(_i, a) }
#endif


/* Here, we want to include unistd.h if we have it to get the definitions
   of things such as off_t.  If we don't have it, make some good guesses. */

#ifdef HAVE_UNISTD_H
#	include <unistd.h>		/* For many important definitions */
#endif

#ifdef HAVE_STDLIB_H
#	include <stdlib.h>
#	if defined(__stardent)
		extern FILE	*popen( const char *, const char * );
		extern FILE	*fdopen( int, const char * );
#	endif
#else

#if !defined(OFF_T) && !defined(HAVE_OFF_T)
#	define	off_t	long
#endif

#ifndef HAVE_UNISTD_H    /* We will have already included many of these in unistd.h */

/*
 *	System calls
 */
extern int	close();
extern int	dup();
#ifndef CRAY1		/* Horrid XMP UNICOS 4.0.7 /bin/cc bug if you define this */
extern void	exit();
#endif

extern int	execl();
extern int	fork();
extern int	getuid();

extern int	open();
extern off_t	lseek();
extern int	nice();
extern int	pipe();
extern int	read();
extern unsigned	sleep();
extern void	sync();
extern int	unlink();
extern int	wait();
extern int	write();


/*
 *	C Library Routines
 */
extern void	perror();
extern void	free();

extern char	*malloc();
extern char	*calloc();
extern char	*getenv();
extern char	*realloc();
extern char	*tempnam();
extern char	*strcpy();
extern char	*strcat();
extern char	*strncat();
extern char	*mktemp();

extern int	atoi();
extern int	qsort();
extern int	strcmp();

extern long	time();

#endif

/*
 *	STDIO Library Routine supplements
 */
#if defined(alliant) ||  defined(__stardent)
	extern FILE	*popen(); /* Not declared in stdio.h */
#endif
#if defined(__stardent)
	extern FILE	*fdopen();
#endif

/*
 *	Math Library Routines
 */
extern double	atof();			/* Should be in math.h or stdlib.h */

#endif /* __STDC__ */

#if defined(alliant) && !defined(__STDC__)
extern double   modf();
#endif


/*
 *  Now, define all the routines found in BRL-CAD's libsysv
 */
#if USE_PROTOTYPES
extern void	port_setlinebuf( FILE *fp );
#if !defined(__stardent) && !defined(__bsdi__)
extern int	getopt( int argc, char **argv, char *optstr );
#endif
extern char	*re_comp( char *s );
extern int	re_exec( char *s );

#else
extern void	port_setlinebuf();
extern int	getopt();
extern char	*re_comp();
extern int	re_exec();
#endif

/* getopt munchies */
extern char	*optarg;
extern int	optind;
extern int	opterr;

/* sys_errlist and errno */
#ifndef HAVE_SYS_ERRLIST_DECL
extern int	errno;
extern int	sys_nerr;
extern char *	sys_errlist[];
#endif

#ifndef HAVE_SBRK_DECL
extern char *	sbrk();
extern int	brk();
#endif

   /* IRIX 5 string.h is unwilling to define strdup */
#if (IRIX >= 5)
extern char *	strdup(const char *s);
#endif

#endif /* EXTERNS_H */
