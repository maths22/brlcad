#ifndef lint
static char rcsid[] = "$Header$";
#endif
/*				M S R A N . C
 *
 * Minimal Standard RANdom number generator
 *
 * From:
 *	Stephen K. Park and Keith W. Miller
 *	"Random number generators: good ones are hard to find"
 *	CACM vol 31 no 10, Oct 88
 *
 * Author:
 *	Christopher T. Johnson - 90/04/20
 *
 * Source:
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Balistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland 21005-5066
 *
 * Copyright Notice -
 *	This software is Copyright (C) 1991 by the United States Army.
 *	All rights reserved.
 *
 *
 * $Log$
 * Revision 1.2  1991/06/22  02:23:36  cjohnson
 * Sync RCS to source code
 *
 * Revision 1.1  90/04/20  02:05:36  cjohnson
 * Initial revision
 * 
 */

#include <stdio.h>
#include <math.h>
#include "msr.h"

/*	msr_unif_init	Initialize a random number structure.
 *
 * Entry:
 *	setseed	seed to use
 *	method  method to use to generate numbers;
 *
 * Exit:
 *	returns	a pointer to a msr_unif structure.
 *	returns 0 on error.
 *
 * Uses:
 *	None.
 *
 * Calls:
 *	rt_malloc
 *
 * Method:
 *	malloc up a structure with pointers to the numbers
 *	get space for the integer table.
 *	get space for the floating table.
 *	set pointer counters
 *	set seed if one was given and setseed != 1
 *
 */
#define	A	16807
#define M	2147483647
#define DM	2147483647.0
#define Q	127773		/* Q = M / A */
#define R	2836		/* R = M % A */
struct msr_unif *
msr_unif_init(setseed,method)
long setseed;
int method;
{
	struct msr_unif *p;
	p = (struct msr_unif *) rt_malloc(sizeof(struct msr_unif),"msr_unif");
	p->msr_longs = (long *) rt_malloc(MSRMAXTBL*sizeof(long), "msr long table");
	p->msr_doubles=(double *) rt_malloc(MSRMAXTBL*sizeof(double),"msr double table");
	p->msr_seed = 1;
	p->msr_long_ptr = 0;
	p->msr_double_ptr = 0;

	if (setseed&0x7fffffff) p->msr_seed=setseed&0x7fffffff;
	return(p);
}

/*	msr_unif_long_fill	fill a random number table.
 *
 * Use the msrad algorithm to fill a random number table
 * with values from 1 to 2^31-1.  These numbers can (and are) extracted from
 * the random number table via high speed macros and msr_unif_long_fill called
 * when the table is exauseted.
 *
 * Entry:
 *	p	pointer to a msr_unif structure.
 *
 * Exit:
 *	if (!p) returns 1 else returns a value between 1 and 2^31-1
 *
 * Calls:
 *	None.  msran is inlined for speed reasons.
 *
 * Uses:
 *	None.
 *
 * Method:
 *	if (!p) return(1);
 *	if p->msr_longs != NULL 
 *		msr_longs is reloaded with random numbers;
 *		msr_long_ptr is set to MSRMAXTBL
 *	endif
 *	msr_seed is updated.
 */
long
msr_unif_long_fill(p)
struct msr_unif *p;
{
	register long test,work_seed;
	register int i;

	if (!p) return(1);

	work_seed = p->msr_seed;

	if ( p->msr_longs) {
		for (i=0; i < MSRMAXTBL; i++) {
			test = A*(work_seed % Q) - R*(work_seed / Q);
			p->msr_longs[i] = work_seed = (test < 0) ?
			     test+M : test;
		}
		p->msr_long_ptr = MSRMAXTBL;
	}
	test = A*(work_seed % Q) - R*(work_seed / Q);
	p->msr_seed =  (test < 0) ? test+M : test;
	return(p->msr_seed);
}

/*	msr_unif_double_fill	fill a random number table.
 *
 * Use the msrad algorithm to fill a random number table
 * with values from -0.5 to 0.5.  These numbers can (and are) extracted from
 * the random number table via high speed macros and msr_unif_double_fill
 * called when the table is exauseted.
 *
 * Entry:
 *	p	pointer to a msr_unif structure.
 *
 * Exit:
 *	if (!p) returns 0.0 else returns a value between -0.5 and 0.5
 *
 * Calls:
 *	None.  msran is inlined for speed reasons.
 *
 * Uses:
 *	None.
 *
 * Method:
 *	if (!p) return (0.0)
 *	if p->msr_longs != NULL 
 *		msr_longs is reloaded with random numbers;
 *		msr_long_ptr is set to MSRMAXTBL
 *	endif
 *	msr_seed is updated.
 */
double
msr_unif_double_fill(p)
struct msr_unif *p;
{
	register long test,work_seed;
	register int i;

	if (!p) return(0.0);

	work_seed = p->msr_seed;

	if (p->msr_doubles) {
		for (i=0; i < MSRMAXTBL; i++) {
			test = A*(work_seed % Q) - R*(work_seed / Q);
			work_seed = (test < 0) ? test+M : test;
			p->msr_doubles[i] = ( work_seed - M/2) * 1.0/DM;
		}
		p->msr_double_ptr = MSRMAXTBL;
	}
	test = A*(work_seed % Q) - R*(work_seed / Q);
	p->msr_seed = (test < 0) ? test+M : test;

	return((p->msr_seed - M/2) * 1.0/DM);
}

/*	msr_gauss_init	Initialize a random number struct for gaussian 
 *	numbers.
 *
 * Entry:
 *	setseed		Seed to use.
 *	method		method to use to generate numbers (not used)
 *
 * Exit:
 *	Returns a pointer toa msr_guass structure.
 *	returns 0 on error.
 *
 * Calls:
 *	rt_malloc
 *
 * Uses:
 *	None.
 *
 * Method:
 *	malloc up a structure
 *	get table space
 *	set seed and pointer.
 *	if setseed != 0 then seed = setseed
 */
struct msr_gauss *
msr_gauss_init(setseed,method)
long setseed;
int method;
{
	struct msr_gauss *p;
	p = (struct msr_gauss *) rt_malloc(sizeof(struct msr_gauss),"msr_guass");
	p->msr_gausses=(double *) malloc(MSRMAXTBL*sizeof(double));
	p->msr_gauss_doubles=(double *) malloc(MSRMAXTBL*sizeof(double));
	p->msr_gauss_seed = 1;
	p->msr_gauss_ptr = 0;
	p->msr_gauss_dbl_ptr = 0;

	if (setseed&0x7fffffff) p->msr_gauss_seed=setseed&0x7fffffff;
	return(p);
}

/*	msr_gauss_fill	fill a random number table.
 *
 * Use the msrad algorithm to fill a random number table
 * with values from -1.0 to 10.0.  These numbers can (and are) extracted from
 * the random number table via high speed macros and msr_guass_fill
 * called when the table is exauseted.
 *
 * Entry:
 *	p	pointer to a msr_guassstructure.
 *
 * Exit:
 *	if (!p) returns 0.0 else returns a value between -1.0 and 1.0
 *	with a guassian distribution (bell curve)
 *
 * Calls:
 *	MSR_UNIF_CIRCLE to get to uniform random number whos radius is
 *	<= 1.0. I.e. sqrt(v1*v1 + v2*v2) <= 1.0
 *	MSR_UNIF_CIRCLE is a macro which can call msr_unif_double_fill.
 *
 * Uses:
 *	None.
 *
 * Method:
 *	if (!p) return (0.0)
 *	if p->msr_longs != NULL 
 *		msr_longs is reloaded with random numbers;
 *		msr_long_ptr is set to MSRMAXTBL
 *	endif
 *	msr_seed is updated.
 */
double
msr_gauss_fill(p)
struct msr_gauss *p;
{
	register int i;
	/* register */ double v1,v2,r,fac;

	if (!p) return(0.0);

	if (p->msr_gausses) {
		for (i=0; i< MSRMAXTBL-1; ) {
			MSR_UNIF_CIRCLE((struct msr_unif *)p,v1,v2,r);
			if (r<0.00001) continue;
			fac = sqrt(-2.0*log(r)/r);
			p->msr_gausses[i++] = v1*fac;
			p->msr_gausses[i++] = v2*fac;
		}
		p->msr_gauss_ptr = MSRMAXTBL;
	}

	do {
		MSR_UNIF_CIRCLE((struct msr_unif *)p,v1,v2,r);
	} while (r < 0.00001);
	fac = sqrt(-2.0*log(r)/r);
	return(v1*fac);
}
