/*
	Author:		Gary S. Moss
			U. S. Army Ballistic Research Laboratory
			Aberdeen Proving Ground
			Maryland 21005-5066
			(301)278-6647 or AV-298-6647
*/
/*
	Originally extracted from SCCS archive:
		SCCS id:	@(#) lgt.h	2.2
		Modified: 	1/30/87 at 17:20:43	G S M
		Retrieved: 	2/4/87 at 08:52:50
		SCCS archive:	/vld/moss/src/lgt/s.lgt.h
*/

#define INCL_LGT
#define MAX_COLOR	15
#define MAX_LGTS	10
#define MAX_LGT_NM	16
#define MAX_LN		81
#ifndef TRUE
#define TRUE		1
#define FALSE		0
#endif
#define Toggle(f)	(f) = !(f)
#define Malloc_Bomb( _bytes_ ) \
		fb_log( "\"%s\"(%d) : allocation of %d bytes failed.\n", \
				__FILE__, __LINE__, _bytes_ )

/* Flag (pix_buffered) values for writing pixels to the frame buffer.	*/
#define B_PIO		0	/* Programmed I/O.			*/
#define B_PAGE		1	/* Buffered I/O (DMA paging scheme).	*/
#define B_LINE		2	/* Line-buffered I/O (DMA).		*/
#define Rotate( f )	(f) = (f) + 1 > 2 ? 0 : (f) + 1

#ifndef DEBUG_OFF /* This is from "librt/debug.h", #include <debug.h> would
			be better.					*/
#define DEBUG_OFF	0	/* No debugging */

/* These definitions are each for one bit */

/* Options useful for debugging applications */
#define DEBUG_ALLRAYS	0x00000001	/* Print calls to shootray() */
#define DEBUG_SHOOT	0x00000002	/* Info about shootray() processing */

/* Options useful for debugging the database */
#define DEBUG_DB	0x00000010	/* Database debugging */
#define DEBUG_SOLIDS	0x00000020	/* Print prep'ed solids */
#define DEBUG_REGIONS	0x00000040	/* Print regions & boolean trees */
#define DEBUG_ARB8	0x00000080	/* Print voluminus ARB8 details */
#define DEBUG_SPLINE	0x00000100	/* Splines */

/* Options useful for debugging the library */
#define DEBUG_ROOTS	0x00001000	/* Print rootfinder details */
#define DEBUG_PARTITION	0x00002000	/* Info about bool_weave() */
#define DEBUG_CUT	0x00004000	/* Print space cutting details */
#define DEBUG_BOXING	0x00008000	/* Object/box checking details */
#define DEBUG_MEM	0x00010000	/* Debug dynamic memory operations */
#define DEBUG_TESTING	0x00020000	/* One-shot debugging flag */
#define DEBUG_FDIFF	0x00040000	/* bool/fdiff debugging */

/* These will cause binary debugging output */
#define DEBUG_PLOTBOX	0x80000000	/* Plot(3) bounding boxes to stdout */
#endif

/* Application debugging flags.						*/
#define DEBUG_RGB	0x80000
#define DEBUG_REFRACT	0x100000
#define DEBUG_NORML	0x200000
#define DEBUG_SHADOW	0x400000
#define DEBUG_GAUSS	0x800000
#define DEBUG_OCTREE	0x1000000

/* Light source (LS) specific global information.
	Directions are with respect to the center of the model as calculated
	by 'librt.a'.
 */
typedef struct
	{
	char	name[MAX_LGT_NM];/* Name of entry (i.e. ambient).	*/
	int	beam;	/* Flag denotes gaussian beam intensity.	*/
	int	over;	/* Flag denotes manual overide of position.	*/
	int	rgb[3];	/* Pixel color of LS (0 to 255) for RGB.	*/
	fastf_t	loc[3];	/* Location of LS in model space.		*/
	fastf_t	azim;	/* Azimuthal direction of LS in radians.	*/
	fastf_t	elev;	/* Elevational direction of LS in radians.	*/
	fastf_t	dir[3];	/* Direction vector to LS.			*/
	fastf_t	dist;	/* Distance to LS in from centroid of model.	*/
	fastf_t	energy;	/* Intensity of LS.				*/
	fastf_t	coef[3];/* Color of LS as coefficient (0.0 to 1.0).	*/
	fastf_t	radius;	/* Radius of beam.				*/
	struct soltab	*stp;	/* Solid table pointer to LIGHT solid.	*/
	}
Lgt_Source;
#define LGT_NULL	(Lgt_Source *) NULL

typedef struct
	{
	int	m_noframes;
	int	m_frame_sz;
	int	m_lgts_bool;
	int	m_over_bool;
	int	m_keys_bool;
	FILE	*m_keys_fp;
	fastf_t	m_azim_beg;
	fastf_t m_azim_end;
	fastf_t	m_elev_beg;
	fastf_t m_elev_end;
	fastf_t	m_roll_beg;
	fastf_t m_roll_end;
	fastf_t	m_dist_beg;
	fastf_t m_dist_end;
	fastf_t	m_grid_beg;
	fastf_t m_grid_end;
	fastf_t	m_pers_beg;
	fastf_t m_pers_end;
	}
Movie;
#define IK_INTENSITY	255.0
#define RGB_INVERSE	(1.0 / IK_INTENSITY)
#define EYE_SIZE	12.7
#define TITLE_LEN	72
#define TIMER_LEN	72

extern Lgt_Source	lgts[];
extern Movie		movie;
