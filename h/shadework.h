/*
 *			M A T E R I A L . H
 */

#ifdef cray
/*
 * Cray has a problem taking the address of an arbitrary character within
 * a structure.  int pointers have to be used instead.
 * There is some matching hackery in the invididual tables.
 */
typedef int	*mp_off_ty;
#else
typedef char	*mp_off_ty;
#endif

/*
 *			M A T P A R S E
 */
struct matparse {
	char		*mp_name;
	mp_off_ty	mp_offset;
	char		*mp_fmt;
};

/*
 *			M F U N C S
 *
 *  The interface to the various material property & texture routines.
 */
struct mfuncs {
	char		*mf_name;	/* Keyword for material */
	int		mf_magic;	/* To validate structure */
	struct mfuncs	*mf_forw;	/* Forward link */
	int		mf_inputs;	/* shadework inputs needed */
	int		(*mf_setup)();	/* Routine for preparing */
	int		(*mf_render)();	/* Routine for rendering */
	int		(*mf_print)();	/* Routine for printing */
	int		(*mf_free)();	/* Routine for releasing storage */
};
#define MF_MAGIC	0x55968058
#define MF_NULL		((struct mfuncs *)0)

/*
 *  mf_inputs lists what optional shadework fields are needed.
 *  dist, point, color, & default(trans,reflect,ri) are always provided
 */
#define MFI_NORMAL	0x01		/* Need normal */
#define MFI_UV		0x02		/* Need uv */
#define MFI_LIGHT	0x04		/* Need light visibility */


#define SW_NLIGHTS	16		/* Max # of light sources */

/*
 *			S H A D E W O R K
 */
struct shadework {
	fastf_t		sw_transmit;
	fastf_t		sw_reflect;
	fastf_t		sw_refrac_index;
	fastf_t		sw_color[3];	/* shaded color */
	fastf_t		sw_basecolor[3]; /* base color */
	struct hit	sw_hit;		/* ray hit (dist,point,normal) */
	struct uvcoord	sw_uv;
	fastf_t		sw_intensity[3*SW_NLIGHTS]; /* light intensities */
	char		*sw_visible[SW_NLIGHTS]; /* visibility flags/ptrs */
	int		sw_xmitonly;	/* flag: need sw_transmit only */
};
