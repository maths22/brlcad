/*
 *			T A B I N T E R P . C
 *
 *  This program is the crucial middle step in the key-frame animation
 *  software.
 *
 *  First, one or more files, on different time scales, are read into
 *  internal "channels".
 *
 *  Secondly, a section of those times is interpolated, and
 *  multi-channel output is produced, on uniform time samples.
 *
 *  This multi-channel output is fed to the next stage to generate
 *  control scripts, etc.
 *
 *  Author -
 *	Michael John Muuss
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1988 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>

#include "machine.h"
#include "vmath.h"
#include "raytrace.h"

struct chan {
	/* INPUTS */
	int	c_ilen;		/* length of input array */
	char	*c_itag;	/* description of input source */
	fastf_t	*c_itime;	/* input time array */
	fastf_t	*c_ival;	/* input value array */
	/* OUTPUTS */
	fastf_t	*c_oval;	/* output value array */
	/* FLAGS */
	int	c_interp;	/* linear or spline? */
#define	INTERP_LINEAR	1
#define	INTERP_SPLINE	2
	int	c_periodic;	/* cyclic end conditions? */
};

int		o_len;		/* length of all output arrays */
fastf_t		*o_time;	/* pointer to output time array */
int		fps;		/* frames/sec of output */

int		nchans;		/* number of chan[] elements in use */
int		max_chans;	/* current size of chan[] array */
struct chan	*chan;		/* ptr to array of chan structs */

extern int	cm_file();
extern int	cm_times();
extern int	cm_interp();

struct command_tab cmdtab[] = {
	"file", "filename chan_num(s)", "load channels from file",
		cm_file,	3, 999,
	"times", "start stop fps", "specify time range and fps rate",
		cm_times,	4, 4,
	"interp", "{linear|spline|cspline} chan_num(s)", "set interpolation type",
		cm_interp,	3, 999,
	(char *)0, (char *)0, (char *)0,
		0,		0, 0	/* END */
};

/*
 *			M A I N
 */
main( argc, argv )
int	argc;
char	**argv;
{
	register char	*buf;
	register int	ret;

	/*
	 * All the work happens in the functions
	 * called by rt_do_cmd().
	 */
	while( (buf = rt_read_cmd( stdin )) != (char *)0 )  {
		fprintf(stderr,"cmd: %s\n", buf );
		ret = rt_do_cmd( 0, buf, cmdtab );
		rt_free( buf, "cmd buf" );
		if( ret < 0 )
			break;
	}

	pr_ichans();

	go();

	output();

	exit(0);	
}

/*
 *			C M _ F I L E
 */
int
cm_file( argc, argv )
int	argc;
char	**argv;
{
	FILE	*fp;
	char	*file;
	char	buf[512];
	int	*cnum;
	int	i;
	int	line;
	char	**iwords;
	int	nlines;		/* number of lines in input file */
	int	nwords;		/* number of words on each input line */
	fastf_t	*times;
	auto double	d;
	int	errors = 0;

	file = argv[1];
	if( (fp = fopen( file, "r" )) == NULL )  {
		perror( file );
		return(0);
	}

	/* First step, count number of lines in file */
	nlines = 0;
	{
		register int	c;

		while( (c = fgetc(fp)) != EOF )  {
			if( c == '\n' )
				nlines++;
		}
	}
	rewind(fp);

	/* Intermediate dynamic memory */
	cnum = (int *)rt_malloc( argc * sizeof(int), "cnum[]");
	nwords = argc - 1;
	iwords = (char **)rt_malloc( (nwords+1) * sizeof(char *), "iwords[]" );

	/* Retained dynamic memory */
	times = (fastf_t *)rt_malloc( nwords * sizeof(fastf_t), "times");

	/* Now, create & allocate memory for each chan */
	for( i = 1; i < nwords; i++ )  {
		sprintf( buf, "File '%s', Column %d", file, i );
		if( (cnum[i] = create_chan( argv[i+1], nlines, buf )) < 0 )
			return(-1);	/* abort */
		/* Share array of times */
		chan[cnum[i]].c_itime = times;
	}

	for( line=0; line < nlines; line++ )  {
		buf[0] = '\0';
		(void)fgets( buf, sizeof(buf), fp );
		i = rt_split_cmd( iwords, nwords+1, buf );
		if( i != nwords )  {
			fprintf(stderr,"File '%s', Line %d:  expected %d columns, got %d\n",
				file, line, nwords, i );
			while( i < nwords )  {
				iwords[i++] = "0.123456789";
				errors++;
			}
		}

		/* Obtain the time from the first column */
		sscanf( iwords[0], "%lf", &d );
		times[line] = d;
		if( line > 0 && times[line-1] > times[line] )  {
		    	fprintf(stderr,"File '%s', Line %d:  time sequence error %g > %g\n",
		    		file, line, times[line-1], times[line] );
			errors++;
		}

		/* Obtain the desired values from the remaining columns,
		 * and assign them to the channels indicated in cnum[]
		 */
		for( i=1; i < nwords; i++ )  {
			if( sscanf( iwords[i], "%lf", &d ) != 1 )  {
			    	fprintf(stderr,"File '%s', Line %d:  scanf failure on '%s'\n",
			    		file, line, iwords[i] );
				d = 0.0;
				errors++;
			}
		    	chan[cnum[i]].c_ival[line] = d;
		}
	}
	fclose(fp);

	/* Free intermediate dynamic memory */
	rt_free( (char *)cnum, "cnum[]");
	rt_free( (char *)iwords, "iwords[]");

	if(errors)
		return(-1);	/* abort */
	return(0);
}

/*
 *			C R E A T E _ C H A N
 */
int
create_chan( num, len, itag )
char	*num;
int	len;
char	*itag;
{
	int	n;

	n = atoi(num);
	if( n < 0 )  return(-1);

	if( n >= max_chans )  {
		if( max_chans <= 0 )  {
			max_chans = 32;
			chan = (struct chan *)rt_malloc(
				max_chans * sizeof(struct chan),
				"chan[]" );
		} else {
			while( n > max_chans )
				max_chans *= 2;
			chan = (struct chan *)rt_realloc( (char *)chan,
				max_chans * sizeof(struct chan),
				"chan[]" );
		}
	}
	for( ; n >= nchans; nchans++ )
		bzero( (char *)&chan[nchans], sizeof(struct chan) );

	chan[n].c_ilen = len;
	chan[n].c_itag = rt_strdup( itag );
	chan[n].c_ival = (fastf_t *)rt_malloc( len * sizeof(fastf_t), "c_ival");
	return(n);
}

/*
 *			P R _ I C H A N S
 *
 *  Print input channel values.
 */
pr_ichans()
{
	register int		ch;
	register struct chan	*cp;
	register int		i;

	for( ch=0; ch < nchans; ch++ )  {
		cp = &chan[ch];
		fprintf(stderr,"--- Channel %d, ilen=%d (%s):\n",
			ch, cp->c_ilen, cp->c_itag );
		for( i=0; i < cp->c_ilen; i++ )  {
			fprintf(stderr," %g\t%g\n", cp->c_itime[i], cp->c_ival[i]);
		}
	}
}

/*
 *			O U T P U T
 */
output()
{
	register int		ch;
	register struct chan	*cp;
	register int		t;

	for( t=0; t < o_len; t++ )  {
		printf("%g", o_time[t]);

		for( ch=0; ch < nchans; ch++ )  {
			cp = &chan[ch];
			if( cp->c_ilen <= 0 )  {
				printf("\t.");
				continue;
			}
			printf("\t%g", cp->c_oval[t] );
		}
		printf("\n");
	}
}

/*
 *			C M _ T I M E S
 */
cm_times( argc, argv )
int	argc;
char	**argv;
{
	double		a, b;
	register int	i;
	int		ch;

	a = atof(argv[1]);
	b = atof(argv[2]);
	fps = atoi(argv[3]);

	if( a >= b )  {
		fprintf(stderr,"times:  %g >= %g\n", a, b );
		return(0);
	}
	if( o_len > 0 )  {
		fprintf(stderr,"times:  already specified\n");
		return(0);	/* ignore */
	}
	o_len = ((b-a) * fps) + 0.999;
	o_len++;	/* one final step to reach endpoint */
	o_time = (fastf_t *)rt_malloc( o_len * sizeof(fastf_t), "o_time[]");

	/*
	 *  Don't use an incremental algorithm, to avoid acrueing error
	 */
	for( i=0; i<o_len; i++ )
		o_time[i] = a + ((double)i)/fps;

	/* Allocate memory for all the output values */
	for( ch = 0; ch < nchans; ch++ )  {
		if( chan[ch].c_ilen <= 0 )
			continue;
		chan[ch].c_oval = (fastf_t *)rt_malloc(
			o_len * sizeof(fastf_t), "c_oval[]");
	}

	return(0);
}

/*
 *			C M _ I N T E R P
 */
cm_interp( argc, argv )
int	argc;
char	**argv;
{
	int	interp = 0;
	int	periodic = 0;
	int	i;
	int	ch;
	struct chan	*chp;

	if( strcmp( argv[1], "linear" ) == 0 )  {
		interp = INTERP_LINEAR;
	} else if( strcmp( argv[1], "spline" ) == 0 )  {
		interp = INTERP_SPLINE;
		periodic = 0;
	} else if( strcmp( argv[1], "cspline" ) == 0 )  {
		interp = INTERP_SPLINE;
		periodic = 1;
	} else {
		fprintf( stderr, "interpolation type '%s' unknown\n", argv[1] );
		interp = INTERP_LINEAR;
	}

	for( i = 2; i < argc; i++ )  {
		ch = atoi( argv[i] );
		chp = &chan[ch];
		if( chp->c_ilen <= 0 )  {
			fprintf(stderr,"error: attempt to set interpolation type on unallocated channel %d\n", ch);
			continue;
		}
		chp->c_interp = interp;
		chp->c_periodic = periodic;
	}
	return(0);
}


/* Perform linear interpolation on each channel */
go()
{
	int	ch;
	struct chan	*chp;

	for( ch=0; ch < nchans; ch++ )  {
		chp = &chan[ch];
		if( chp->c_ilen <= 0 )
			continue;
again:
		switch( chp->c_interp )  {
		default:
			fprintf(stderr,"unknown interpolation type %d\n", chp->c_interp);
			/* FALL THROUGH */
		case INTERP_LINEAR:
			linear_interpolate( chp );
			break;
		case INTERP_SPLINE:
			if( spline( chp ) <= 0 )  {
				fprintf(stderr, "spline failure, switching to linear\n");
				chp->c_interp = INTERP_LINEAR;
				goto again;
			}
			break;
		}
	}
}

/*
 *			L I N E A R _ I N T E R P O L A T E
 *
 *  This routine takes advantage of (and depends on) the fact that
 *  the input and output is sorted in increasing time values.
 */
linear_interpolate( chp )
register struct chan	*chp;
{
	register int	t;		/* output time index */
	register int	i;		/* input time index */

	i = 0;
	for( t=0; t<o_len; t++ )  {
		/* Check for below initial time */
		if( o_time[t] < chp->c_itime[0] )  {
			chp->c_oval[t] = chp->c_ival[0];
			continue;
		}
		/* Check for above final time */
		if( o_time[t] > chp->c_itime[chp->c_ilen-1] )  {
			chp->c_oval[t] = chp->c_ival[chp->c_ilen-1];
			continue;
		}
		/* Find time range in input data.  Could range check? */
		while( i < chp->c_ilen-1 )  {
			if( o_time[t] >= chp->c_itime[i] && 
			    o_time[t] <= chp->c_itime[i+1] )
				break;
			i++;
		}
		/* Perform actual interpolation */
		chp->c_oval[t] = chp->c_ival[i] +
			(o_time[t] - chp->c_itime[i]) *
			(chp->c_ival[i+1] - chp->c_ival[i]) /
			(chp->c_itime[i+1] - chp->c_itime[i]);
	}
}

/* Spline fit technique, derrived from /usr/src/usr.bin/spline.c */

double
rhs(i, chp)
register int		i;
register struct chan	*chp;
{
	register int i_;
	double zz;

	i_ = (i==chp->c_ilen-1) ? 0 : i;
	zz = (chp->c_ival[i] - chp->c_ival[i-1])/
		(chp->c_itime[i] - chp->c_itime[i-1]);
	return( 6 * ((chp->c_ival[i_+1]-chp->c_ival[i_])/
		(chp->c_itime[i+1]-chp->c_itime[i]) - zz));
}

spline( chp )
register struct chan	*chp;
{
	float d,s,u,v;
	double	hi;			/* horiz interval i-1 to i */
	double	hi1;			/* horiz interval i to i+1 */
	float h;
	float D2yi,D2yi1,D2yn1;
	float a;
	int end;
	float corr;
	register int i,j,m;
	float konst = 0.0;		/* derriv. at endpts, non-periodic */
	float *diag, *rrr;
	register int	t;

	/* First, as a quick hack, do linear interpolation to fill in
	 * values off the endpoints, in non-periodic case
	 */
	linear_interpolate( chp );

	if(chp->c_ilen<3) {
		printf("need at least 3 points to spline\n");
		goto bad;
	}

	i = (chp->c_ilen+1)*sizeof(float);
	diag = (float *)rt_malloc((unsigned)i, "diag");
	rrr = (float *)rt_malloc((unsigned)i, "rrr");
	if( !rrr || !diag )  {
		printf("malloc failure\n");
		goto bad;
	}

	if(chp->c_periodic) konst = 0;
	d = 1;
	rrr[0] = 0;
	s = chp->c_periodic?-1:0;
	/* triangularize */
	for( i=0; ++i < chp->c_ilen - !chp->c_periodic; )  {
		hi = chp->c_itime[i]-chp->c_itime[i-1];
		hi1 = (i==chp->c_ilen-1) ?
			chp->c_itime[1] - chp->c_itime[0] :
			chp->c_itime[i+1] - chp->c_itime[i];
		if(hi1*hi<=0) {
			fprintf(stderr,
			    "spline: Horiz. interval changed sign at i=%d, time=%g\n",
			    i, chp->c_itime[i]);
			goto bad;
		}
		u = i==1?0.0:u-s*s/d;
		v = i==1?0.0:v-s*rrr[i-1]/d;
		rrr[i] = rhs(i,chp)-hi*rrr[i-1]/d;
		s = -hi*s/d;
		a = 2*(hi+hi1);
		if(i==1) a += konst*hi;
		if(i==chp->c_ilen-2) a += konst*hi1;
		diag[i] = d = i==1? a:
		    a - hi*hi/d; 
	}
	D2yi = D2yn1 = 0;
	/* back substitute */
	for( i = chp->c_ilen - !chp->c_periodic; --i >= 0; )  {
		end = i==chp->c_ilen-1;
		hi1 = end ? chp->c_itime[1] - chp->c_itime[0]:
			chp->c_itime[i+1] - chp->c_itime[i];
		D2yi1 = D2yi;
		if(i>0){
			hi = chp->c_itime[i]-chp->c_itime[i-1];
			corr = end?2*s+u:0.0;
			D2yi = (end*v+rrr[i]-hi1*D2yi1-s*D2yn1)/
				(diag[i]+corr);
			if(end) D2yn1 = D2yi;
			if(i>1){
				a = 2*(hi+hi1);
				if(i==1) a += konst*hi;
				if(i==chp->c_ilen-2) a += konst*hi1;
				d = diag[i-1];
				s = -s*d/hi; 
			}
		}
		else D2yi = D2yn1;
		if(!chp->c_periodic) {
			if(i==0) D2yi = konst*D2yi1;
			if(i==chp->c_ilen-2) D2yi1 = konst*D2yi;
		}
		if(end) continue;

		/* hi1 is range of time covered in this interval */
#if 0
		/* m will be number of samples in this interval */
		m = hi1 * fps;
		if(m<=0) m = 1;

		/* interpolate, high to low */
		for(j=m;j>0||i==0&&j==0;j--) {
			register int sub;
			register double	x0;	/* fraction from [i+0] */
			register double	x1;	/* fraction from [i+1] */
			register double	yy;

			x1 = (double)j / (double) m;
			x0 = 1 - x1;
			yy = D2yi * (x0 - x0*x0*x0) + D2yi1 * (x1 - x1*x1*x1);
			yy = chp->c_ival[i] * x0 + chp->c_ival[i+1] * x1 - 
				hi1 * hi1 * yy / 6;
			sub = (int)((chp->c_itime[i]+x1*hi1)*fps);
			chp->c_oval[sub] = yy;
		}
#endif
		/* Sweep o_time[], looking for times in this span */
		for( t=0; t<o_len; t++ )  {
			register double	x0;	/* fraction from [i+0] */
			register double	x1;	/* fraction from [i+1] */
			register double	yy;
			register double	cur_t;

			cur_t = o_time[t];
			if( chp->c_periodic )  {
				/* Should be pre-computed once/sline call XXX */
				while( cur_t > chp->c_itime[chp->c_ilen-1] )
					cur_t -= chp->c_itime[0];
				while( cur_t < chp->c_itime[0] );
					cur_t += chp->c_itime[chp->c_ilen-1];
			}
			if( cur_t > chp->c_itime[i+1] )
				continue;
			if( cur_t < chp->c_itime[i] )
				continue;
			x1 = (cur_t - chp->c_itime[i]) /
			    (chp->c_itime[i+1] - chp->c_itime[i]);
			x0 = 1 - x1;
			/* Linear interpolation, with correction */
			yy = D2yi * (x0 - x0*x0*x0) + D2yi1 * (x1 - x1*x1*x1);
			yy = chp->c_ival[i] * x0 + chp->c_ival[i+1] * x1 - 
				hi1 * hi1 * yy / 6;
			chp->c_oval[t] = yy;
		}
	}
	rt_free( (char *)diag, "diag");
	rt_free( (char *)rrr, "rrr" );
	return(1);
bad:
	rt_free( (char *)diag, "diag");
	rt_free( (char *)rrr, "rrr" );
	return(0);
}
