#ifndef lint
static char rcsid[] = "$Header$";
#endif
#include <stdio.h>
#define	DLEVEL	1
extern int Debug;
typedef struct Cubic {
	double	x,A,B,C,D;
} C;
/*	tonescale	Given a raw pixel value, return a scaled value
 *
 * This is normally used to map the output devices characteristics to
 * the input intinsites.  There can be a diffrent map for each color.
 *
 * Entry:
 *	map	pointer to a 256 byte map
 *	Slope	Slope of a line
 *	B	offset of line.
 *	eqptr	Null or the pointer to a equation for generating a curve
 *
 * Exit:
 *	map	is filled using eqptr 
 *
 * Uses:
 *	EqCubics	x,A,B,C,D of a set of cubics for a curve
 *
 * Calls:
 *	eq_line	given x return y; requires EqLineSlope and EqLineB
 *	eqptr	if not null.
 *
 * Method:
 *	straight-forward.
 *
 * Author:
 *	Christopher T. Johnson	- 90/03/22
 *
 * Change History:
 *	ctj 90/04/04 - change to use a standard cubic line format for the
 *	tone scale.  If eqptr is null then Set EqCubic to evaluate to a line.
 *
 * $Log$
 */
extern struct Cubic *EqCubics;
int eq_cubic();
void
tonescale(map,Slope,B,eqptr)
unsigned char	*map;
float		Slope,
		B;
int 		(*eqptr)();
{
	int i,result;

	if (!eqptr) {
		eqptr=eq_cubic;
		EqCubics = (struct Cubic *)malloc(2*sizeof(struct Cubic));
		EqCubics[0].x = 0.0;
		EqCubics[0].A = B;
		EqCubics[0].B = Slope;
		EqCubics[1].x = 256.0;
		EqCubics[0].C = EqCubics[0].D =	EqCubics[1].A =
		    EqCubics[1].B = EqCubics[1].C = EqCubics[1].D = 0.0;
	}

	for (i=0;i<256;i++) {
		result=eqptr(i);
		if (result<0) {
			if (Debug >= DLEVEL) {
				fprintf(stderr,"tonescale: y=%d, x=%d\n",
				    result, i);
			}
			result=0;
		} else if (result > 255) {
			if (Debug >= DLEVEL) {
				fprintf(stderr,"tonescale: y=%d, x=%d\n",
				    result, i);
			}
			result=255;
		}

		map[i] = result;
	}
}
static struct Cubic	*EqCubics;
/* eq_cubic	default tone scale alorithm
 *
 * impliment
 *	y = A+B*(X-x)+C*(X-x)^2+D*(X-x)^3
 * as a default tonescale algorithm;
 *
 * Entry:
 *	x	x value for equation.
 *
 * Exit:
 *	returns	y in the range 0-255  reqires a clipping.
 *
 * Calls:
 *	none.
 *
 * Uses:
 *	EqCubic		list of Cubic equations.
 *
 * Method:
 *	straight-forward.
 *
 * Author:
 *	Christopher T. Johnson - 90/03/22
 */
eq_cubic(x)
int x;
{
	int y;
	struct Cubic *p = EqCubics;

	if (!p) {
		fprintf(stderr,"eq_cubic called with no cubics!\n");
		return(x);
	}
	while (x >= (p+1)->x) p++;
	
	y = ((p->D * (x - p->x) + p->C) * (x - p->x) + p->B)
	    * (x - p->x) + p->A;

	if (y<0) y = 0;
	if (y>255) y = 255;
	return(y);
}
/* cubic_init	initialize a cubic list given a set of points.
 *
 * Entry:
 *	n	number of points
 *	x	list of x points.
 *	y	list of y points.
 *
 * Exit:
 *	EqCubic	is set to a list of cubics
 *
 * Calls:
 *	none.
 *
 * Uses:
 *	none.
 *
 * Method:
 *	diagnal solution to cubics taken from Lee Butler's book.
 */
cubic_init(n,x,y)
int n;
int *x,*y;
{
	int i,j;
	double *h,*alpha,*mi,*z,*l;

	h = (double *) malloc(n*sizeof(double));
	alpha = (double *) malloc(n*sizeof(double));
	mi = (double *) malloc(n*sizeof(double));
	z = (double *) malloc(n*sizeof(double));
	l = (double *) malloc(n*sizeof(double));

	EqCubics = (struct Cubic *) malloc(n*sizeof(struct Cubic));

	for (i=0; i<n-1; i++) {
		h[i] = x[i+1] - x[i];
		EqCubics[i].x = x[i];
		EqCubics[i].A = y[i];
	}

	EqCubics[i].x = x[i];
	EqCubics[i].A = y[i];

	for (i=1; i<n-1; i++) {
		alpha[i] = 3.0*(y[i+1]*h[i-1] - y[i]*(x[i+1]-x[i-1]) +
		    y[i-1]*h[i]) / (h[i-1]*h[i]);
	}

	z[0] = 0;
	mi[0] = 0;

	for (i=1; i<n-1; i++) {
		l[i] = 2.0*(x[i+1] - x[i-1]) - h[i-1]*mi[i-1];
		mi[i] = h[i]/l[i];
		z[i] = (alpha[i]-h[i-1]*z[i-1]) / l[i];
	}

	l[i] = 1.0;
	z[i] = 0.0;
	EqCubics[i].C = z[i];

	for (i=n-2; i>=0; i--) {
		EqCubics[i].C = z[i] - mi[i]*EqCubics[i+1].C;
		EqCubics[i].B = (y[i+1] - y[i])/h[i] -
		    h[i]*(EqCubics[i+1].C + 2.0*EqCubics[i].C)/3.0;
		EqCubics[i].D = (EqCubics[i+1].C - EqCubics[i].C)/(3.0*h[i]);
	}

	free(h);
	free(alpha);
	free(mi);
	free(z);
	free(l);
}
