/*
 *  select some number of doubles
 */
#include <stdio.h>
#include <math.h>

double	buf[1024];

static char usage[] = "\
Usage: dsel num\n\
       dsel skip keep ...\n";

main( argc, argv )
int	argc;
char	**argv;
{
	int	nskip;	/* number to skip */
	int	nkeep;	/* number to keep */

	if( argc < 1 || isatty(fileno(stdin)) || isatty(fileno(stdout)) ) {
		fprintf( stderr, usage );
		exit( 1 );
	}

	if( argc == 2 ) {
		keep( atoi(argv[1]) );
		exit( 0 );
	}

	while( argc > 1 ) {
		nskip = atoi(argv[1]);
		argc--;
		argv++;
		if( nskip > 0 )
			skip( nskip );

		if( argc > 1 ) {
			nkeep = atoi(argv[1]);
			argc--;
			argv++;
		} else {
			nkeep = HUGE;
		}

		if( nkeep <= 0 )
			exit( 0 );
		keep( nkeep );
	}
}

skip( num )
register int num;
{
	register int	n, m;

	while( num > 0 ) {
		n = num > 1024 ? 1024 : num;
		if( (m = fread( buf, sizeof(*buf), n, stdin )) == 0 )
			exit( 0 );
		num -= m;
	}
}

keep( num )
register int num;
{
	register int	n, m;

	while( num > 0 ) {
		n = num > 1024 ? 1024 : num;
		if( (m = fread( buf, sizeof(*buf), n, stdin )) == 0 )
			exit( 0 );
		fwrite( buf, sizeof(*buf), m, stdout );
		num -= n;
	}
}
