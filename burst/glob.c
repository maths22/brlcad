/*
	Author:		Gary S. Moss
			U. S. Army Ballistic Research Laboratory
			Aberdeen Proving Ground
			Maryland 21005-5066
			(301)278-6651 or AV-298-6651
*/
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif
#include <stdio.h>
#include "machine.h"
#include "vmath.h"
#include "fb.h"
#include "raytrace.h"
#include "./burst.h"
#include "./extern.h"

Colors	colorids;	/* ident range to color mappings for plots */
FBIO	*fbiop;		/* frame buffer specific access from libfb */
FILE	*gridfp=NULL;	/* grid file output stream (2-d shots) */
FILE	*histfp=NULL;	/* histogram output stream (statistics) */
FILE	*outfp=NULL;	/* output stream */
FILE	*plotfp=NULL;	/* 3-D UNIX plot stream (debugging) */
FILE	*shotfp=NULL;	/* input stream for shot positions */
FILE	*tmpfp=NULL;	/* temporary file for logging input */
HmMenu	*mainhmenu;
Ids	airids;		/* burst air idents */
Ids	armorids;	/* burst armor idents */
Ids	critids;	/* critical component idents */
Trie	*cmdtrie = TRIE_NULL;

bool batchmode = false;		/* are we processing batch input now */
bool cantwarhead = false;	/* Bob Wilson's canted warhead stuff */
bool deflectcone = DFL_DEFLECT;	/* cone axis deflects towards normal */
bool dithercells = DFL_DITHER;	/* if true, randomize shot within cell */
bool fatalerror;		/* must abort ray tracing */
bool reportoverlaps = DFL_OVERLAPS;
				/* if true, overlaps are reported */
bool tty = true;		/* if true, full screen display is used */
bool userinterrupt;		/* has the ray trace been interrupted */

char airfile[LNBUFSZ]={0};	/* input file name for burst air ids */
char armorfile[LNBUFSZ]={0};	/* input file name for burst armor ids */
char cmdbuf[LNBUFSZ];
char cmdname[LNBUFSZ];
char colorfile[LNBUFSZ]={0};	/* ident range-to-color file name */
char critfile[LNBUFSZ]={0};	/* input file for critical components */
char errfile[LNBUFSZ]={0};	/* errors/diagnostics log file name */
char fbfile[LNBUFSZ]={0};	/* frame buffer image file name */
char gedfile[LNBUFSZ]={0};	/* MGED data base file name */
char gridfile[LNBUFSZ]={0};	/* saved grid (2-d shots) file name */
char histfile[LNBUFSZ]={0};	/* histogram file name (statistics) */
char objects[LNBUFSZ]={0};	/* list of objects from MGED file */
char outfile[LNBUFSZ]={0};	/* burst output file name */
char plotfile[LNBUFSZ]={0};	/* 3-D UNIX plot file name (debugging) */
char scrbuf[LNBUFSZ];		/* scratch buffer for temporary use */
char scriptfile[LNBUFSZ]={0};	/* shell script file name */
char shotfile[LNBUFSZ];		/* input file of firing coordinates */
char title[TITLE_LEN];		/* title of MGED target description */
char timer[TIMER_LEN];		/* CPU usage statistics */
char tmpfname[TIMER_LEN];	/* temporary file for logging input */

char *cmdptr;

fastf_t	bdist = DFL_BDIST;
fastf_t	burstpoint[3];	/* explicit burst point coordinates */
fastf_t	cellsz = DFL_CELLSIZE;
			/* shotline separation */
fastf_t	conehfangle = DFL_CONEANGLE;
			/* spall cone half angle */
fastf_t	fire[3];	/* explicit firing coordinates (2-D or 3-D) */
fastf_t	gridsoff[3];	/* origin of grid translated by stand-off */
fastf_t	modlcntr[3];	/* centroid of target's bounding RPP */
fastf_t raysolidangle;	/* solid angle per spall sampling ray */
fastf_t	standoff;	/* distance from model origin to grid */
fastf_t	unitconv = 1.0;	/* units conversion factor (mm to "units") */
fastf_t	viewazim = DFL_AZIMUTH;
			/* degrees from X-axis to firing position */
fastf_t	viewelev = DFL_ELEVATION;
			/* degrees from XY-plane to firing position */ 

/* These are the angles and fusing distance used to specify the path of
	the canted warhead in Bob Wilson's simulation.
 */
fastf_t	pitch = 0.0;	/* elevation above path of main penetrator */
fastf_t	yaw = 0.0;	/* deviation right of path of main penetrator */
fastf_t	setback = 0.0;	/* fusing distance for warhead */

int co;			/* columns of text displayable on video screen */
int li;			/* lines of text displayable on video screen */
int firemode = FM_DFLT;	/* mode of specifying shots */
int gridsz = 512;
int gridxfin;
int gridyfin;
int gridxorg;
int gridyorg;
int nbarriers = 0;	    /* no. of barriers allowed to critical comp */
int noverlaps = 0;	    /* no. of overlaps encountered in this view */
int nprocessors;	    /* no. of processors running concurrently */
int nriplevels = 0;	    /* no. of levels of ripping (0 = no ripping) */
int nspallrays = DFL_NRAYS; /* no. of spall rays at each burst point */
int units = DFL_UNITS;	    /* target units (default is millimeters) */
int zoom = 1;		    /* magnification factor on frame buffer */

struct rt_i *rtip = RTI_NULL; /* model specific access from librt */

/* signal handlers */
#if defined( SYSV )
void	(*norml_sig)();	/* active during interactive operation */
void	(*abort_sig)(); /* active during ray tracing only */
#else
int	(*norml_sig)();
int	(*abort_sig)();
#endif
