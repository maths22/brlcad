/*
 * This software is copyrighted as noted below.  It may be freely copied,
 * modified, and redistributed, provided that the copyright notice is 
 * preserved on all copies.
 * 
 * There is no warranty or other guarantee of fitness for this software,
 * it is provided solely "as is".  Bug reports or fixes may be sent
 * to the author, who may or may not act on them as he desires.
 *
 * You may not include this software in a program or other software product
 * without supplying the source, or without informing the end-user that the 
 * source is available for no extra charge.
 *
 * If you modify this software, you should include a notice giving the
 * name of the person performing the modification, the date of modification,
 * and the reason for such modification.
 */
/* 
 * into.c - Put output into a file without destroying it.
 * 
 * Author:	Spencer W. Thomas
 * 		Computer Science Dept.
 * 		University of Utah
 * Date:	Mon Aug 22 1983
 * Copyright (c) 1983 Spencer W. Thomas
 */

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#if defined(vax) && defined(BSD) && BSD < 44
#undef BSD	/* /usr/include/sys/param.h redefines this */
#endif
#include <sys/param.h>			/* for MAXPATHLEN */
#include <sys/stat.h>

#include "rle_config.h"

#ifdef USE_STDLIB_H
#include <stdlib.h>
#else

#ifdef USE_STRING_H
#include <string.h>
#define rindex strrchr
#else
#include <strings.h>
#endif

#endif /* USE_STDLIB_H */

#ifdef S_IFSOCK				/* should work */
# define BSD42
# define FAST_STDIO			/* actually not until 4.3 */
#endif

#ifndef MAXPATHLEN
# define MAXPATHLEN BUFSIZ
#endif

static char temp[] = "intoXXXXXXXX";
static char buf[MAXPATHLEN+1];
short forceflg;				/* overwrite an unwritable file? */

extern int errno;
extern char *sys_errlist[];

void
main(argc, argv)
int argc;
char **argv;
{
    char *cp;
    int c;
    FILE * outf;
#ifdef FAST_STDIO
    char iobuf[BUFSIZ];
    int size;
#endif

    /* Don't allow files named "-f" in order to catch common error */
    if (argc >= 2 && !strcmp(argv[1], "-f"))
    {
	forceflg++;
	argc--, argv++;
    }
    if (argc != 2)
    {
	fprintf(stderr, "Usage: into [ -f ] file\n");
	exit(1);
    }
    if (!forceflg && access(argv[1], 2) < 0 && errno != ENOENT)
    {
	fprintf(stderr, "into: ");
	perror(argv[1]);
	exit(1);
    }

    if ( (cp = rindex( argv[1], '/' )) != NULL )
    {
	c = *++cp;
	*cp = 0;
	strcpy( buf, argv[1] );
	*cp = c;
	strcat( buf, temp );
    }
    else
	strcpy( buf, temp );
    mktemp( buf );

    if ( (outf = fopen( buf, "w" )) == NULL )
    {
	perror(buf);
	exit(1);
    }

#ifdef FAST_STDIO
    while ( (size = fread(iobuf, 1, sizeof iobuf, stdin)) != 0)
	fwrite(iobuf, 1, size, outf);
#else					/* should use unix i/o */
    while ( (c = getchar()) != EOF )
	putc( c, outf );
#endif

    if ( !forceflg && ftell(outf) == 0L )
    {
	fprintf( stderr, "into: empty output, \"%s\" not modified\n", argv[1]);
	unlink( buf );
	exit(1);
    }
    fflush(outf);
    if (ferror(outf))
    {
	fprintf(stderr, "into: %s, \"%s\" not modified\n",
	    sys_errlist[errno], argv[1]);
	unlink(buf);
	exit(1);
    }
    fclose( outf );

    if ( rename( buf, argv[1] ) < 0 )
    {
	fprintf(stderr, "into: rename(%s, %s): ", buf, argv[1]);
	perror("");
    }
    exit( 0 );
}

#ifndef BSD42
rename( file1, file2 )
char *file1, *file2;
{
    struct stat st;

    if ( stat(file2, &st) >= 0 && unlink(file2) < 0 )
	return -1;
    if ( link(file1, file2) < 0 )
	return -1;
    return unlink( file1 );
}
#endif /* !BSD42 */
