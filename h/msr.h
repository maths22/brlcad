/*	$Header$
 *
 * Define random number structures and constants.
 * Also define a set of macros to access the random number tables
 * and to limit the area/volume that a set of random numbers inhabit.
 *
 *	$Log$
 * 
 */
/*
 * NOTE!!! MSRMAXTBL must be an even number!!!
 */
#define	MSRMAXTBL	4096	/* Size of random number tables. */

struct msr_unif {
	long	msr_seed;
	int	msr_double_ptr;
	double	*msr_doubles;
	int	msr_long_ptr;
	long	*msr_longs;
};
/*
 * NOTE!!! The order of msr_gauss and msr_unif MUST match in the
 * first three entries as msr_gauss is passed as a msr_unif in
 * msr_gauss_fill.
 */
struct msr_gauss {
	long	msr_gauss_seed;
	int	msr_gauss_ptr;
	double	*msr_gausses;
};

#define	MSR_UNIF_LONG(_p)	(((_p)->msr_long_ptr ) ? \
	(_p)->msr_longs[--(_p)->msr_long_ptr] : \
	msr_unif_fill_long(_p))
#define MSR_UNIF_DOUBLE(_p)	(((_p)->msr_double_ptr) ? \
	(_p)->msr_longs[--(_p)->msr_double_ptr] : \
	msr_unif_double_fill(_p))

#define MSR_UNIF_CIRCLE(_p,_x,_y,_r) { \
	do { \
		(_x) = 2.0*MSR_UNIF_DOUBLE((_p)) - 1.0; \
		(_y) = 2.0*MSR_UNIF_DOUBLE((_p)) - 1.0; \
		(_r) = (_x)*(_x)+(_y)*(_y); \
	} while ((_r) >= 1.0);  }

#define	MSR_UNIF_SPHERE(_p,_x,_y,_z,_r) { \
	do { \
		(_x) = 2.0*MSR_UNIF_DOUBLE(_p) - 1.0; \
		(_y) = 2.0*MSR_UNIF_DOUBLE(_p) - 1.0; \
		(_z) = 2.0*MSR_UNIF_DOUBLE(_p) - 1.0; \
		(_r) = (_x)*(_x)+(_y)*(_y)+(_z)*(_z);\
	} while ((_r) >= 1.0) }

#define	MSR_GAUSS_DOUBLE(_p)	(((_p)->msr_gauss_ptr) ? \
	(_p)->msr_gausses[--(_p)->msr_gauss_ptr)] : \
	msr_gause_fill(_p))
