/*
 *			H T O N D . C
 *
 *  Library routines for conversion between the local host
 *  64-bit ("double precision") representation, and
 *  64-bit IEEE double precision representation, in "network order",
 *  ie, big-endian, the MSB in byte [0], on the left.
 *
 *  As a quick review, the IEEE double precision format is as follows:
 *  sign bit, 11 bits of exponent (bias 1023), and 52 bits of mantissa,
 *  with a hidden leading one (0.1 binary).
 *  When the exponent is 0, IEEE defines a "denormalized number",
 *  which is not supported here.
 *  When the exponent is 2047 (all bits set), and:
 *	all mantissa bits are zero, value is infinity*sign,
 *	mantissa is non-zero, and:
 *		msb of mantissa=0:  signaling NAN
 *		msb of mantissa=1:  quiet NAN
 *
 *  Note that neither the input or output buffers need be word aligned,
 *  for greatest flexability in converting data, even though this
 *  imposes a speed penalty here.
 *
 *  These subroutines operate on a sequential block of numbers,
 *  to save on subroutine linkage execution costs, and to allow
 *  some hope for vectorization.
 *
 *  On brain-damaged machines like the SGI 3-D, where type "double"
 *  allocates only 4 bytes of space, these routines *still* return
 *  8 bytes in the IEEE buffer.
 *
 *  Author -
 *	Michael John Muuss
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Release Status -
 *	Public Domain, Distribution Unlimited
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>

#define	OUT_IEEE_ZERO	{ \
	*out++ = 0; \
	*out++ = 0; \
	*out++ = 0; \
	*out++ = 0; \
	*out++ = 0; \
	*out++ = 0; \
	*out++ = 0; \
	*out++ = 0; \
	continue; } \

#define	OUT_IEEE_NAN	{ /* Signaling NAN */ \
	*out++ = 0xFF; \
	*out++ = 0xF0; \
	*out++ = 0x0B; \
	*out++ = 0xAD; \
	*out++ = 0x0B; \
	*out++ = 0xAD; \
	*out++ = 0x0B; \
	*out++ = 0xAD; \
	continue; } \

/*
 *			H T O N D
 *
 *  Host to Network Doubles
 */
htond( out, in, count )
register unsigned char	*out;
register unsigned char	*in;
int			count;
{
#if	defined(sun) || defined(alliant)
	/*
	 *  First, the case where the system already operates in
	 *  IEEE format internally, using big-endian order.
	 *  These are the lucky ones.
	 */
#	ifdef SYSV
	memcpy( out, in, count*8 );
#	else
	bcopy( in, out, count*8 );
#	endif
	return;
#	define	HTOND	yes
#endif

#if	defined(sgi)
	/*
	 *  Silicon Graphics Iris workstation.
	 *  On the 4-D, a double is a double.
	 *  On the 2-D and 3-D, a double is type converted to a float
	 *  (4 bytes), but IEEE single precision has a different
	 *  number of exponent bits than double precision, so we
	 *  have to engage in gyrations here.
	 */
	register int	i;
	for( i=count-1; i >= 0; i-- )  {
		if( sizeof(double) == 4 )  {
			/* Brain-damaged 3-D case */
			float small;
			long float big;
			register unsigned char *fp = (unsigned char *)&small;
			*fp++ = *in++;
			*fp++ = *in++;
			*fp++ = *in++;
			*fp++ = *in++;
			big = small;		/* H/W cvt to IEEE double */
			fp = (unsigned char *)&big;
			*out++ = *fp++;
			*out++ = *fp++;
			*out++ = *fp++;
			*out++ = *fp++;
			*out++ = *fp++;
			*out++ = *fp++;
			*out++ = *fp++;
			*out++ = *fp++;
		} else {
			/* 4-D case */
			*out++ = *in++;
			*out++ = *in++;
			*out++ = *in++;
			*out++ = *in++;
			*out++ = *in++;
			*out++ = *in++;
			*out++ = *in++;
			*out++ = *in++;
		}
	}
	return;
#	define	HTOND	yes
#endif

#if	defined(vax)
	/*
	 *  Digital Equipment's VAX.
	 *  VAX order is +6, +4, +2, sign|exp|fraction+0
	 *  with 8 bits of exponent, excess 128 base 2, exp=0 => zero.
	 */
	register int	i;
	for( i=count-1; i >= 0; i-- )  {
		register unsigned long left, right, signbit;
		register int exp;

		left  = (in[1]<<24) | (in[0]<<16) | (in[3]<<8) | in[2];
		right = (in[5]<<24) | (in[4]<<16) | (in[7]<<8) | in[6];
		in += 8;

		exp = (left >> 23) & 0xFF;
		signbit = left & 0x80000000;
		if( exp == 0 )  {
			if( signbit )  {
				OUT_IEEE_NAN;
			} else {
				OUT_IEEE_ZERO;
			}
		}
		exp += 1023 - 129;
		/* Round LSB by adding 4, rather than truncating */
#		ifdef ROUNDING
			right = (left<<(32-3)) | ((right+4)>>3);
#		else
			right = (left<<(32-3)) | (right>>3);
#		endif
		left =  ((left & 0x007FFFFF)>>3) | signbit | (exp<<20);
		*out++ = left>>24;
		*out++ = left>>16;
		*out++ = left>>8;
		*out++ = left;
		*out++ = right>>24;
		*out++ = right>>16;
		*out++ = right>>8;
		*out++ = right;
	}
	return;
#	define	HTOND	yes
#endif

#if	defined(ibm) || defined(gould)
	/*
	 *  IBM Format.
	 *  7-bit exponent, base 16.
	 *  No hidden bits in mantissa (56 bits).
	 */
	register int	i;
	for( i=count-1; i >= 0; i-- )  {
		register unsigned long left, right, signbit;
		register int exp;

		left  = (in[0]<<24) | (in[1]<<16) | (in[2]<<8) | in[3];
		right = (in[4]<<24) | (in[5]<<16) | (in[6]<<8) | in[7];
		in += 8;

		exp = (left>>24) & 0x7F;	/* excess 64, base 16 */
		if( left == 0 && right == 0 )
			OUT_IEEE_ZERO;

		signbit = left & 0x80000000;
		left &= 0x00FFFFFF;
		if( signbit )  {
			/* The IBM uses 2's compliment on the mantissa,
			 * and IEEE does not.
			 */
			left  ^= 0xFFFFFFFF;
			right ^= 0xFFFFFFFF;
			if( right & 0x80000000 )  {
				/* There may be a carry */
				right += 1;
				if( (right & 0x80000000) == 0 )  {
					/* There WAS a carry */
					left += 1;
				}
			} else {
				/* There will be no carry to worry about */
				right += 1;
			}
			left &= 0x00FFFFFF;
			exp = (~exp) & 0x7F;
		}
		exp -= (64-32+1);		/* excess 32, base 16, + fudge */
		exp *= 4;			/* excess 128, base 2 */
ibm_normalized:
		if( left & 0x00800000 )  {
			/* fix = 0; */
			exp += 1023-129+1+ 3-0;/* fudge, slide hidden bit */
		} else if( left & 0x00400000 ) {
			/* fix = 1; */
			exp += 1023-129+1+ 3-1;
			left = (left<<1) |
				( (right>>(32-1)) & (0x7FFFFFFF>>(31-1)) );
			right <<= 1;
		} else if( left & 0x00200000 ) {
			/* fix = 2; */
			exp += 1023-129+1+ 3-2;
			left = (left<<2) |
				( (right>>(32-2)) & (0x7FFFFFFF>>(31-2)) );
			right <<= 2;
		} else if( left & 0x00100000 ){ 
			/* fix = 3; */
			exp += 1023-129+1+ 3-3;
			left = (left<<3) |
				( (right>>(32-3)) & (0x7FFFFFFF>>(31-3)) );
			right <<= 3;
		} else {
			/*  Encountered 4 consecutive 0 bits of mantissa,
			 *  attempt to normalize, and loop.
			 *  This case was not expected, but does happen,
			 *  at least on the Gould.
			 */
			exp -= 4;
			left = (left<<4) | (right>>(32-4));
			right <<= 4;
			goto ibm_normalized;
		}

		/* After suitable testing, this check can be deleted */
		if( (left & 0x00800000) == 0 )  {
			fprintf(stderr,"ibm->ieee missing 1, left=x%x\n", left);
			left = (left<<1) | (right>>31);
			right <<= 1;
			goto ibm_normalized;
		}

		/* Having nearly VAX format, shift to IEEE, rounding. */
#		ifdef ROUNDING
			right = (left<<(32-3)) | ((right+4)>>3);
#		else
			right = (left<<(32-3)) | (right>>3);
#		endif
		left =  ((left & 0x007FFFFF)>>3) | signbit | (exp<<20);

		*out++ = left>>24;
		*out++ = left>>16;
		*out++ = left>>8;
		*out++ = left;
		*out++ = right>>24;
		*out++ = right>>16;
		*out++ = right>>8;
		*out++ = right;
	}
	return;
#	define	HTOND	yes
#endif

#if	defined(cray) || defined(CRAY2)
	/*
	 *  Cray version.  Somewhat easier using 64-bit registers.
	 *  15 bit exponent, biased 040000 (octal).  48 mantissa bits.
	 *  No hidden bits.
	 */
	register int	i;
	for( i=count-1; i >= 0; i-- )  {
		register unsigned long word, signbit;
		register int exp;

#ifdef never
		if( (((int)in) & 07) == 0 )
			word = *((float *)in);
		else
#endif
			word  = (in[0]<<56) | (in[1]<<48) | (in[2]<<40) | (in[3]<<32) |
				(in[4]<<24) | (in[5]<<16) | (in[6]<<8) | in[7];
		in += 8;

		if( word == 0 )
			OUT_IEEE_ZERO;
		exp = (word >> 48) & 0x7FFF;
		signbit = word & 0x8000000000000000;
#ifdef redundant
		if( exp <= 020001 || exp >= 060000 )
			OUT_IEEE_NAN;
#endif
		exp += 1023 - 040000 - 1;
		if( (exp & ~0x7FF) != 0 )  {
			fprintf(stderr,"htond:  Cray exponent too large\n");
			OUT_IEEE_NAN;
		}

		word = ((word & 0x00007FFFFFFFFFFF) << (15-11+1)) |
			signbit | (exp<<(64-12));

		*out++ = word>>56;
		*out++ = word>>48;
		*out++ = word>>40;
		*out++ = word>>32;
		*out++ = word>>24;
		*out++ = word>>16;
		*out++ = word>>8;
		*out++ = word;
	}
	return;
#	define	HTOND	yes
#endif

#ifndef	HTOND
# include "htond.c:  Error, no conversion for this machine type"
#endif
}

/*
 *			N T O H D
 *
 *  Network to Host Doubles
 */
ntohd( out, in, count )
register unsigned char	*out;
register unsigned char	*in;
int			count;
{
#if	defined(sun) || defined(alliant)
	/*
	 *  First, the case where the system already operates in
	 *  IEEE format internally, using big-endian order.
	 *  These are the lucky ones.
	 */
	if( sizeof(double) != 8 )
		fprintf(stderr, "ntohd:  sizeof(double) != 8\n");
#	ifdef SYSV
	memcpy( out, in, count*8 );
#	else
	bcopy( in, out, count*8 );
#	endif
	return;
#	define	NTOHD	yes
#endif

#if	defined(sgi)
	/*
	 *  Silicon Graphics Iris workstation.
	 *  See comments in htond() for discussion of the braindamage.
	 */
	register int	i;
	for( i=count-1; i >= 0; i-- )  {
		if( sizeof(double) == 4 )  {
			/* Brain-damaged 3-D case */
			float small;
			long float big;
			register unsigned char *fp = (unsigned char *)&big;
			*fp++ = *in++;
			*fp++ = *in++;
			*fp++ = *in++;
			*fp++ = *in++;
			*fp++ = *in++;
			*fp++ = *in++;
			*fp++ = *in++;
			*fp++ = *in++;
			small = big;		/* H/W cvt to IEEE double */
			fp = (unsigned char *)&small;
			*out++ = *fp++;
			*out++ = *fp++;
			*out++ = *fp++;
			*out++ = *fp++;
		} else {
			/* 4-D case */
			*out++ = *in++;
			*out++ = *in++;
			*out++ = *in++;
			*out++ = *in++;
			*out++ = *in++;
			*out++ = *in++;
			*out++ = *in++;
			*out++ = *in++;
		}
	}
	return;
#	define	NTOHD	yes
#endif

#if	defined(vax)
	/*
	 *  Digital Equipment's VAX.
	 *  VAX order is +6, +4, +2, sign|exp|fraction+0
	 *  with 8 bits of exponent, excess 128 base 2, exp=0 => zero.
	 */
	register int	i;
	for( i=count-1; i >= 0; i-- )  {
		register unsigned long left, right, signbit;
		register int fix, exp;

		left  = (in[0]<<24) | (in[1]<<16) | (in[2]<<8) | in[3];
		right = (in[4]<<24) | (in[5]<<16) | (in[6]<<8) | in[7];
		in += 8;

		exp = (left >> 20) & 0x7FF;
		signbit = left & 0x80000000;
		if( exp == 0 )  {
			*out++ = 0;		/* VAX zero */
			*out++ = 0;
			*out++ = 0;
			*out++ = 0;
			*out++ = 0;
			*out++ = 0;
			*out++ = 0;
			*out++ = 0;
			continue;
		} else if( exp == 0x7FF )  {
vax_undef:		*out++ = 0x80;		/* VAX "undefined" */
			*out++ = 0;
			*out++ = 0;
			*out++ = 0;
			*out++ = 0;
			*out++ = 0;
			*out++ = 0;
			*out++ = 0;
			continue;
		}
		exp += 129 - 1023;
		/* Check for exponent out of range */
		if( (exp & ~0xFF) != 0 )  {
			fprintf(stderr,"ntohd: VAX exponent overflow\n");
			goto vax_undef;
		}
		left = ((left & 0x000FFFFF)<<3) | signbit | (exp<<23) |
			(right >> (32-3));
		right <<= 3;
		out[1] = left>>24;
		out[0] = left>>16;
		out[3] = left>>8;
		out[2] = left;
		out[5] = right>>24;
		out[4] = right>>16;
		out[7] = right>>8;
		out[6] = right;
		out += 8;
	}
	return;
#	define	NTOHD	yes
#endif

#if	defined(ibm) || defined(gould)
	/*
	 *  IBM Format.
	 *  7-bit exponent, base 16.
	 *  No hidden bits in mantissa (56 bits).
	 */
	register int	i;
	for( i=count-1; i >= 0; i-- )  {
		register unsigned long left, right;
		register int fix, exp, signbit;

		left  = (in[0]<<24) | (in[1]<<16) | (in[2]<<8) | in[3];
		right = (in[4]<<24) | (in[5]<<16) | (in[6]<<8) | in[7];
		in += 8;

		exp = ((left >> 20) & 0x7FF);
		signbit = (left & 0x80000000) >> 24;
		if( exp == 0 || exp == 0x7FF )  {
ibm_undef:		*out++ = 0;		/* IBM zero.  No NAN */
			*out++ = 0;
			*out++ = 0;
			*out++ = 0;
			*out++ = 0;
			*out++ = 0;
			*out++ = 0;
			*out++ = 0;
			continue;
		}

		left = (left & 0x000FFFFF) | 0x00100000;/* replace "hidden" bit */

		exp += 129 - 1023 -1;	/* fudge, to make /4 and %4 work */
		fix = exp % 4;		/* 2^4 == 16^1;  get fractional exp */
		exp /= 4;		/* excess 32, base 16 */
		exp += (64-32+1);	/* excess 64, base 16, plus fudge */
		if( (exp & ~0xFF) != 0 )  {
			fprintf(stderr,"ntohd:  IBM exponent overflow\n");
			goto ibm_undef;
		}

		if( fix )  {
			left = (left<<fix) | (right >> (32-fix));
			right <<= fix;
		}

		if( signbit )  {
			/* The IBM actually uses complimented mantissa
			 * and exponent.
			 */
			left  ^= 0xFFFFFFFF;
			right ^= 0xFFFFFFFF;
			if( right & 0x80000000 )  {
				/* There may be a carry */
				right += 1;
				if( (right & 0x80000000) == 0 )  {
					/* There WAS a carry */
					left += 1;
				}
			} else {
				/* There will be no carry to worry about */
				right += 1;
			}
			left &= 0x00FFFFFF;
			exp = (~exp) & 0x7F;
		}


		/*  Not actually required, but for comparison purposes,
		 *  normalize the number.  Remove for production speed.
		 */
		while( (left & 0x00F00000) == 0 && left != 0 )  {
			if( signbit && exp <= 0x41 )  break;

			left = (left << 4) | (right >> (32-4));
			right <<= 4;
			if(signbit)  exp--;
			else exp++;
		}

		*out++ = signbit | exp;
		*out++ = left>>16;
		*out++ = left>>8;
		*out++ = left;
		*out++ = right>>24;
		*out++ = right>>16;
		*out++ = right>>8;
		*out++ = right;
	}
	return;
#	define	NTOHD	yes
#endif

#if	defined(cray) || defined(CRAY2)
	/*
	 *  Cray version.  Somewhat easier using 64-bit registers.
	 *  15 bit exponent, biased 040000 (octal).  48 mantissa bits.
	 *  No hidden bits.
	 */
	register int	i;
	for( i=count-1; i >= 0; i-- )  {
		register unsigned long word, signbit;
		register int exp;

#ifdef never
		if( (((int)in) & 07) == 0 )
			word = *((float *)in);
		else
#endif
			word  = (in[0]<<56) | (in[1]<<48) | (in[2]<<40) | (in[3]<<32) |
				(in[4]<<24) | (in[5]<<16) | (in[6]<<8) | in[7];
		in += 8;

		exp = (word>>(64-12)) & 0x7FF;
		signbit = word & 0x8000000000000000;
		if( exp == 0 )  {
			word = 0;
			goto cray_out;
		}
		if( exp == 0x7FF )  {
			word = 067777<<48;	/* Cray out of range */
			goto cray_out;
		}
		exp += 040000 - 1023 + 1;
		word = ((word & 0x000FFFFFFFFFFFFF) >> (15-11+1)) |
			0x0000800000000000 | signbit | (exp<<(64-16));

cray_out:
		*out++ = word>>56;
		*out++ = word>>48;
		*out++ = word>>40;
		*out++ = word>>32;
		*out++ = word>>24;
		*out++ = word>>16;
		*out++ = word>>8;
		*out++ = word;
	}
	return;
#	define	NTOHD	yes
#endif

#ifndef	NTOHD
# include "ntohd.c:  Error, no conversion for this machine type"
#endif
}
