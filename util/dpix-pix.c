/* Minmax reads a binary input file, finds the minimum and maximum values
 * read, and linearly interpolates these values between 0 and 255.
 * S. Muuss, J.D., Feb 04, 1990.
 */

#include <stdio.h>

#define NUM	(1024 * 16)	/* Note the powers of 2 -- v. efficient */

main(argc, argv)
int	argc;
char	*argv[];

{
	int		count;			/* count of items */
	int		got;			/* count of bytes */
	int		fd;			/* UNIX file descriptor */
	double		d[NUM];
	register double	*dp;			/* ptr to d */
	register double *ep;
	double		m;			/* slope */
	double		b;			/* intercept */
	unsigned char	c[NUM];


	if( (fd = open(argv[1], 0)) < 0 )  {
		perror(argv[1]);
		exit(1);
	}

	/* Note that the minimum is set to 1.0e20, the computer's working
	 * equivalent of positive infinity.  Thus any subsequent value
	 * must be larger. Likewise, the maximun is set to -1.0e20, the
	 * equivalent of negative infinity, and any values must thus be
	 * bigger than it.
	 */
	{
		register double	min, max;		/* high usage items */

		min = 1.0e20;
		max = -1.0e20;

		while(1)  {
			got = read( fd, (char *)&d[0], NUM*sizeof(d[0]) );
			if( got <= 0 )
				break;
			count = got / sizeof(d[0]);
			ep = &d[count];
			for(dp = &d[0]; dp < ep;)  {
				register double val;
				if( (val = *dp++) < min )
					min = val;
				else if ( val > max )
					max = val;
			}
		}

		lseek( fd, 0L, 0 );		/* rewind(fp); */
	

		/* This section uses the maximum and the minimum values found to
		 * compute the m and the b of the line as specified by the
		 * equation y = mx + b.
		 */
		fprintf(stderr, "min=%lf, max=%lf\n", min, max);
		if (max < min)  {
			printf("MINMAX: max less than min!\n");
			exit(1);
		}

		m = (255 - 0)/(max - min);
		b = (-255 * min)/(max - min);
	}

	while (1)  {
		register char	*cp;		/* ptr to c */
		register double	mm;		/* slope */
		register double	bb;		/* intercept */

		mm = m;
		bb = b;

		got = read( fd, (char *)&d[0], NUM*sizeof(d[0]) );
		if (got <=  0 )
			break;
		count = got / sizeof(d[0]);
		ep = &d[count];
		for(dp = &d[0], cp = &c[0]; dp < ep;)  {
			*cp++ = mm * (*dp++) + bb;
		}

		/* fd 1 is stdout */
		got = write( 1, (char *)&c[0], count*sizeof(c[0]) );
		if( got != count*sizeof(c[0]) )  {
			perror("write");
			exit(2);
		}
	}

	exit(0);
}
