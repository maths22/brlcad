#include	"./polyno.h"
#include	"./complex.h"
#include	<math.h>

#define	Abs( a )	((a) >= 0 ? (a) : -(a))

poly	Zpoly = { 0, 0.0 };

/*
 *	polyMul -- multiply two polynomials
 */
poly *
polyMul(mult1,mult2,product)
register poly	*mult1, *mult2, *product;
{
	register int		ct1, ct2;

	*product = Zpoly;

	/* If the degree of the product will be larger than the
	 * maximum size allowed in "polyno.h", then return a null
	 * pointer to indicate failure.
	 */
	if ( (product->dgr = mult1->dgr + mult2->dgr) > MAXP )
		return PM_NULL;

	for ( ct1=0; ct1 <= mult1->dgr; ++ct1 ){
		for ( ct2=0; ct2 <= mult2->dgr; ++ct2 ){
			product->cf[ct1+ct2] += mult1->cf[ct1]*mult2->cf[ct2];
		}
	}
	return product;
}


/*
 *	polyScal -- scale a polynomial
 */
poly *
polyScal(eqn,factor)
register poly	*eqn;
double	factor;
{
	register int		cnt;

	for ( cnt=0; cnt <= eqn->dgr; ++cnt ){
		eqn->cf[cnt] *= factor;
	}
	return eqn;
}


/*
 *	polyAdd -- add two polynomials
 */
poly *
polyAdd(poly1,poly2,sum)
register poly	*poly1, *poly2, *sum;
{
	static poly		tmp;
	register int		i, offset;

	offset = Abs(poly1->dgr - poly2->dgr);

	tmp = Zpoly;

	if ( poly1->dgr >= poly2->dgr ){
		*sum = *poly1;
		for ( i=0; i <= poly2->dgr; ++i ){
			tmp.cf[i+offset] = poly2->cf[i];
		}
	} else {
		*sum = *poly2;
		for ( i=0; i <= poly1->dgr; ++i ){
			tmp.cf[i+offset] = poly1->cf[i];
		}
	}

	for ( i=0; i <= sum->dgr; ++i ){
		sum->cf[i] += tmp.cf[i];
	}
	return sum;
}


/*
 *	polySub -- subtract two polynomials
 */
poly *
polySub(poly1,poly2,diff)
register poly	*poly1, *poly2, *diff;
{
	static poly		tmp;
	register int		i, offset;

	offset = Abs(poly1->dgr - poly2->dgr);

	*diff = Zpoly;
	tmp = Zpoly;

	if ( poly1->dgr >= poly2->dgr ){
		*diff = *poly1;
		for ( i=0; i <= poly2->dgr; ++i ){
			tmp.cf[i+offset] = poly2->cf[i];
		}
	} else {
		diff->dgr = poly2->dgr;
		for ( i=0; i <= poly1->dgr; ++i ){
			diff->cf[i+offset] = poly1->cf[i];
		}
		tmp = *poly2;
	}

	for ( i=0; i <= diff->dgr; ++i ){
		diff->cf[i] -= tmp.cf[i];
	}
	return diff;
}


/*	>>>  s y n D i v ( )  <<<
 *	Divides any polynomial into any other polynomial using synthetic
 *	division.  Both polynomials must have real coefficients.
 */
void
synDiv(dvdend,dvsor,quo,rem)
register poly	*dvdend, *dvsor, *quo, *rem;
{
	register int	div;
	register int	n;

	*quo = *dvdend;
	*rem = Zpoly;

	if ((quo->dgr = dvdend->dgr - dvsor->dgr) < 0)
		quo->dgr = -1;
	if ((rem->dgr = dvsor->dgr - 1) > dvdend->dgr)
		rem->dgr = dvdend->dgr;

	for ( n=0; n <= quo->dgr; ++n){
		quo->cf[n] /= dvsor->cf[0];
		for ( div=1; div <= dvsor->dgr; ++div){
			quo->cf[n+div] -= quo->cf[n] * dvsor->cf[div];
		}
	}
	for ( n=1; n<=(rem->dgr+1); ++n){
		rem->cf[n-1] = quo->cf[quo->dgr+n];
		quo->cf[quo->dgr+n] = 0;
	}
}


/*	>>>  q u a d r a t i c ( )  <<<
 *
 *	Uses the quadratic formula to find the roots (in 'complex' form)
 *	of any quadratic equation with real coefficients.
 */
void
quadratic(quad,root1,root2)
register poly		*quad;
register complex	*root1, *root2;
{
	static double	discrim, denom;

	discrim = quad->cf[1]*quad->cf[1] - 4.0* quad->cf[0]*quad->cf[2];
	denom = 2.0* quad->cf[0];

	if ( discrim >= 0.0 ){
		root1->re = ( -quad->cf[1] + sqrt(discrim) )/ denom;
		root1->im = 0.0;
		root2->re = ( -quad->cf[1] - sqrt(discrim) )/ denom;
		root2->im = 0.0;
	} else {
		discrim = Abs(discrim);
		root1->re = -quad->cf[1] / denom;
		root1->im =  sqrt(discrim) / denom;
		root2->re = -quad->cf[1] / denom;
		root2->im = -sqrt(discrim) / denom;
	}
}

/*
 *			P R N T P O L Y
 */
void
prntpoly(eqn)
register poly	*eqn;
{
	register int	n;

	printf("\nDegree of polynomial = %d\n",eqn->dgr);
	for ( n=0; n<=eqn->dgr; ++n){
		printf(" %g ",eqn->cf[n]);
	}
	printf("\n");
}
