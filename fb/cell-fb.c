/*
 *			C E L L - F B . C
 *
 *	Original author:	Gary S. Moss
 *				(301) 278-2979 or AV 298-2979
 *
 *	Modifications by:	Paul J. Tanenbaum
 *				(301) 278-6691 or AV 298-6691
 *
 *	Both of whom are at:	U. S. Army Ballistic Research Laboratory
 *				Aberdeen Proving Ground
 *				Maryland 21005-5066
 */

#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>
#include "machine.h"
#include "vmath.h"
#include <sys/time.h>
#include "fb.h"

/* Macros without arguments */
#ifndef STATIC
#define STATIC static
#endif

#ifndef true
#define false	0
#define true	1
#endif

#define LORES		512
#define HIRES		1024
#define	SNUG_FIT	1
#define	LOOSE_FIT	2
#define MAX_LINE	133
#define PI		3.14159265358979323846264338327950288419716939937511
#define HFPI		(PI/2.0)
#define NEG_INFINITY	-10000000.0
#define POS_INFINITY	10000000.0
#define MAX_COLORTBL	11
#define WHITE		colortbl[0]
#define BACKGROUND	colortbl[MAX_COLORTBL]
#define	OPT_STRING	"CF:N:S:W:X:b:c:d:ef:ghikl:m:p:s:v:?"


/* Macros with arguments */
#ifndef Min
#define Min(a, b)		((a) < (b) ? (a) : (b))
#define Max(a, b)		((a) > (b) ? (a) : (b))
#define MinMax(m, M, a)    { m = Min(m, a); M = Max(M, a); }
#endif

/*
 * Translate between different coordinate systems at play:
 *	H,V	The units of the input file.  (from GIFT)
 *	C	The relative cell number, within the input
 *	VP	The pixel within the viewport (a sub-rectangle of the screen)
 *		Includes offsetting for the "key" area within the viewport.
 *	SCR	The pixel on the screen.  Framebuffer coordinates for LIBFB.
 *		Includes offsetting the viewport anywhere on the screen.
 */

#define H2CX(_h)	( ((_h) - xmin) / cell_size )
#define V2CY(_v)	( ((_v) - ymin) / cell_size )

#define CX2VPX(_cx)	( (_cx) * (wid + grid_flag)+1.0 )
#define CY2VPY(_cy)	( (_cy) * (hgt + grid_flag)+key_height )

#define VPX2SCRX(_vp_x)	( (_vp_x) + xorigin )
#define VPY2SCRY(_vp_y)	( (_vp_y) + yorigin )

/* --- */

#define SCRX2VPX(_scr_x) ( (_scr_x) - xorigin )
#define SCRY2VPY(_scr_y) ( (_scr_y) - yorigin )

#define VPX2CX(_vp_x)	( (_vp_x) / (wid+grid_flag)-1.0 )
#define VPY2CY(_vp_y)	( (_vp_y) / (hgt+grid_flag)-key_height )

#define CX2H(_cx)	( (_cx) * cell_size + xmin )
#define CY2V(_cy)	( (_cy) * cell_size + ymin )

/* --- */

#define H2SCRX(_h)	VPX2SCRX( CX2VPX( H2CX(_h) ) )
#define V2SCRY(_v)	VPY2SCRY( CY2VPY( V2CY(_v) ) )

#define SCRX2H(_s_x)	CX2H( VPX2CX( SCRX2VPX(_s_x) ) )
#define SCRY2V(_s_y)	CY2V( VPY2CY( SCRY2VPY(_s_y) ) )

/* Absolute value */
#define Abs(_a)	((_a) < 0.0 ? -(_a) : (_a))

/* Debug flags */
#define		CFB_DBG_MINMAX		0x01
#define		CFB_DBG_GRID		0x02

/* Data structure definitions */
typedef int		bool;
typedef union
{
    double	v_scalar;
    RGBpixel	v_color;
}			cell_val;
typedef struct
{
    double	c_x;
    double	c_y;
    cell_val	c_val;
}			Cell;

/* Global variables */
static Cell	*grid;

static char	*usage[] = {
	"",
	"cell-fb ($Revision$)",
	"",
	"Usage: cell-fb [options] [file]",
	"Options:",
	" -C            Use first 3 fields as r, g, and b",
	" -F dev        Use frame-buffer device `dev'",
	" -N n          Set frame-buffer height to `n' pixels",
	" -S n          Set frame-buffer height and width to `n' pixels",
	" -W n          Set frame-buffer width to `n' pixels",
	" -X n          Set debug flag to hexadecimal value `n' (default is 0)",
	" -b n          Ignore values not equal to `n'",
	" -c n          Assume cell size of `n' user units (default is 100)",
	" -d \"m n\"      Expect input in interval [m, n] (default is [0, 1])",
	" -e            Erase frame buffer before displaying picture",
	" -f n          Display field `n' of cell data",
	" -g            Leave space between cells",
	" -h            Use high-resolution frame buffer (sames as -S 1024)",
	" -i            Round values (default is to interpolate colors)",
	" -k            Display color key",
	" -l \"a e\"      Write log information to stdout",
	" -m \"n r g b\"  Map value `n' to color ``r g b''",
	" -p \"x y\"      Offset picture from bottom-left corner of display",
	" -s \"w h\"      Set cell width and height in pixels",
	" -v n          Display view number `n' (default is all views)",
	0
};
static char	fbfile[MAX_LINE] = { 0 };/* Name of frame-buffer device */
static char	infile[MAX_LINE] = { 0 };/* Name of input stream */

static double	az;			/* To dump to log file */
static double	bool_val;		/* Only value displayed for -b option */
static double	cell_size = 100.0;	/* Size of cell in user units */
static double	el;			/* To dump to log file */
static double	key_height = 0;		/* How many cell heights for key? */
static double	xmin;			/* Extrema of coordinates	*/
static double	ymin;			/* in user units		*/
static double	xmax;			/* (set in read_Cell_Data())	*/
static double	ymax;			/*				*/
static double	dom_min = 0.0;		/* Extrema of data to plot	*/
static double	dom_max = 1.0;		/*				*/
static double	dom_cvt = 10.0;		/* To convert domain to [0, 10] */

static bool	boolean_flag = false;	/* Show only one value? */
static bool	color_flag = false;	/* Interpret fields as R, G, B? */
static bool	erase_flag = false;	/* Erase frame buffer first? */
static bool	grid_flag = false;	/* Leave space between cells? */
static bool	hires_flag = false;	/* Force high-res frame buffer? */
static bool	interp_flag = true;	/* Ramp between colortbl entries? */
static bool	key_flag = false;	/* Display color-mapping key? */
static bool	log_flag = false;	/* Make a log file? */

static int	compute_fb_height;	/* User supplied height?  Else what? */
static int	compute_fb_width;	/* User supplied width?  Else what? */
static int	debug_flag = 0;		/* Control diagnostic prints */
static int	fb_height = -1;		/* Height of frame buffer in pixels */
static int	fb_width = -1;		/* Width of frame buffer in pixels */
static int	field = 1;		/* The field that is of interest */
static int	wid = 10, hgt = 10;	/* Number of pixels per cell, H & V */
static int	xorigin = 0, yorigin = 0;/* Pixel location of image low lft */
static int	view_flag = 0;		/* The view that is of interest */

static long	maxcells = 10000;	/* Max number of cells in the image */

static FBIO	*fbiop = FBIO_NULL;	/* Frame-buffer device */

static FILE	*filep;			/* Input stream */

static RGBpixel	colortbl[12] =		/* The map: value --> R, G, B */
{
    { 255, 255, 255 },		/* white */
    { 100, 100, 140 },		/* blue grey */
    {   0,   0, 255 },		/* blue */
    {   0, 120, 255 },		/* light blue */
    { 100, 200, 140 },		/* turquoise */
    {   0, 150,   0 },		/* dark green */
    {   0, 225,   0 },		/* green */
    { 255, 255,   0 },		/* yellow */
    { 255, 160,   0 },		/* tangerine */
    { 255, 100, 100 },		/* pink */
    { 255,   0,   0 },		/* red */
    {   0,   0,   0 }		/* black */
};			

STATIC bool	get_OK();
STATIC bool	pars_Argv();
STATIC long	read_Cell_Data();
STATIC void	init_Globs();
STATIC void	prnt_Usage();
STATIC void	val_To_RGB();
STATIC void	log_Run();

main (argc, argv)

int	argc;
char	**argv;

{	
    static long	ncells;

    if (! pars_Argv(argc, argv))
    {
	prnt_Usage();
	exit (1);
    }
    if ((grid = (Cell *) malloc(sizeof(Cell) * maxcells)) == NULL)
    {
	fb_log("cell-fb: couldn't allocate space for %d cells\n", maxcells);
	exit (1);
    }

    do
    {
	init_Globs();
	if ((ncells = read_Cell_Data()) == 0)
	{
	    fb_log("cell-fb: failed to read view\n");
	    exit (1);
	}
	fb_log("Displaying %ld cells\n", ncells);
	if (! display_Cells(ncells))
	{
	    fb_log("cell-fb: failed to display %ld cells\n", ncells);
	    exit (1);
	}
	if (log_flag)
	    log_Run();
    } while ((view_flag == 0) && ! feof(filep) && get_OK());
}

STATIC long read_Cell_Data()
{	
    static char		linebuf[MAX_LINE];
    static char		format[MAX_LINE];
    register int	past_data = false;
    static int		past_header = false;
    int			i;
    register Cell	*gp = grid;
    int			view_ct = 1;

    /* Build the format for sscanf() */
    (void) strcpy(format, "%lf %lf");
    if (color_flag)
	(void) strcat(format, " %d %d %d");
    else
    {
	for (i = 1; i < field; i++)
	    (void) strcat(format, " %*lf");	/* Skip to field of interest */
	(void) strcat(format, " %lf");
    }

    /* EOF encountered before we found the desired view? */
    if (! past_header && fgets(linebuf, MAX_LINE, filep) == NULL)
	return (0);

    /* Read the data */
    do
    {	
	double		x, y;
	int		r, g, b;
	cell_val	value;

	/* Have we run out of room for the cells?  If so reallocate memory */
	if (gp - grid >= maxcells)
	{	
	    long	ncells = gp - grid;

	    maxcells *= 2;
	    if ((grid = (Cell *) realloc(grid, sizeof(Cell) * maxcells))
		== NULL)
	    {
		fb_log("Cannot allocate space for %d cells\n", maxcells);
		return (0); /* failure */
	    }
	    gp = grid + ncells;
#ifdef DEBUG
	    fb_log("maxcells increased to %ld\n", maxcells);
#endif
	}

	/* Read in a line of input */
	while ((color_flag &&
		(sscanf(linebuf, format, &x, &y, &r, &g, &b) != 5))
	    || (! color_flag &&
		(sscanf(linebuf, format, &x, &y, &value.v_scalar) != 3)))
	{
	    if (past_header)
		past_data = true;
	    if(feof(filep) ||	fgets(linebuf, MAX_LINE, filep) == NULL)
		return (gp - grid);
	}
	if (color_flag)
	{
	    value.v_color[RED] = r;
	    value.v_color[GRN] = g;
	    value.v_color[BLU] = b;
	}
	if (past_data)
	    if ((view_flag == 0) || (view_flag == view_ct++))
		return (gp - grid);
	    else	/* Not the selected view, read the next one. */
	    {
		past_data = false;
		continue;
	    }
	past_header = true;

	/* If user has selected a view, only store values for that view. */
	if ((view_flag == 0) || (view_flag == view_ct))
	{
	    MinMax(xmin, xmax, x);
	    MinMax(ymin, ymax, y);
	    if (debug_flag & CFB_DBG_MINMAX)
	    {
		fprintf(stderr, "xmin=%g, xmax=%g, ymin=%g, ymax=%g\n",
		    xmin, xmax, ymin, ymax);
		fflush(stderr);
	    }
	    gp->c_x = x;
	    gp->c_y = y;
	    if (color_flag)
	    {
		COPYRGB(gp->c_val.v_color, value.v_color);
	    }
	    else
		gp->c_val.v_scalar = value.v_scalar;
	    gp++;
	}
    } while (fgets(linebuf, MAX_LINE, filep) != NULL);
    return (gp - grid);
}

STATIC bool get_OK()
{	
    int		c;
    FILE	*infp;

    if ((infp = fopen("/dev/tty", "r")) == NULL)
    {
	fb_log("Cannot open /dev/tty for reading\n");
	return (false);
    }
    (void) fputs("Another view follows.  Display ? [y/n](y) ", stderr);
    (void) fflush(stdout);
    switch ((c = getc(infp)))
    {
	case '\n':
	    break;
	default:
	    while (getc(infp) != '\n')
		; /* Read until user hits <RETURN>. */
	    break;
    }
    (void) fclose(infp);
    if (c == 'n')
	return (false);
    return (true);
}

STATIC void init_Globs()
{
    xmin = POS_INFINITY;
    ymin = POS_INFINITY;
    xmax = NEG_INFINITY;
    ymax = NEG_INFINITY;
    return;
}

STATIC bool display_Cells (ncells)

long	ncells;

{	
    register Cell	*gp, *ep = &grid[ncells];
    static int		zoom;
    static RGBpixel	*buf = 0;
    static RGBpixel	pixel;
    double		lasty = NEG_INFINITY;
    double		dx, dy;
    register int	y0 = 0, y1;

    if (compute_fb_height)
    {
	dy = ((ymax - ymin) / cell_size + 1.0) * hgt;
	if (compute_fb_height == SNUG_FIT)
	    fb_height = dy + (key_flag * 2 * hgt) + yorigin;
	else if (dy > LORES)	/* LOOSE_FIT */
	    fb_height = HIRES;
	else
	    fb_height = LORES;
    }
    if (compute_fb_width)
    {
	dx = ((xmax - xmin) / cell_size + 1.0) * wid;
	if (compute_fb_width == SNUG_FIT)
	    fb_width = dx + xorigin;
	else if (dx > LORES)	/* LOOSE_FIT */
	    fb_width = HIRES;
	else
	    fb_width = LORES;
    }

    zoom = 1;
    if ((fbiop = fb_open((fbfile[0] != '\0') ? fbfile : NULL, fb_width, fb_height))
	== FBIO_NULL)
	return (false);
    if (compute_fb_height || compute_fb_width)  {
	(void) fprintf(stderr, "fb_size requested: %d %d\n", fb_width, fb_height);
    	fb_width = fb_getwidth(fbiop);
    	fb_height = fb_getheight(fbiop);
	(void) fprintf(stderr, "fb_size  obtained: %d %d\n", fb_width, fb_height);
    }
    if (fb_wmap(fbiop, COLORMAP_NULL) == -1)
	fb_log("Cannot initialize color map\n");
    if (fb_zoom(fbiop, zoom, zoom) == -1)
	fb_log("Cannot set zoom <%d,%d>\n", zoom, zoom);
    if (erase_flag && fb_clear(fbiop, BACKGROUND) == -1)
	fb_log("Cannot clear frame buffer\n");

    if ((buf = (RGBpixel *) malloc(sizeof(RGBpixel) * fb_width)) == NULL)
    {
	fb_log("cell-fb: couldn't allocate space for %d pixels\n", fb_width);
	exit (1);
    }

    for (gp = grid; gp < ep; gp++)
    {	
	register int	x0, x1;

	/* Whenever Y changes, write out row of cells. */
	if (lasty != gp->c_y)
	{
	    /* If first time, nothing to write out. */
	    if (lasty != NEG_INFINITY)
	    {
		if (debug_flag & CFB_DBG_GRID)
		{
		    fprintf(stderr, "%d = V2SCRY(%g)\n", V2SCRY(lasty), lasty);
		    fflush(stderr);
		}
		for(y0 = V2SCRY(lasty), y1 = y0 + hgt; y0 < y1; y0++)
		    if (fb_write(fbiop, 0, y0, buf, fb_width) == -1)
		    {
			fb_log("Couldn't write to <%d,%d>\n", 0, y0);
			(void) fb_close(fbiop);
			return (false);
		    }
	    }

	    /* Clear buffer. */
	    for (x0 = 0; x0 < fb_width; x0++)
	    {
		COPYRGB(buf[x0], BACKGROUND);
	    }

	     /* Draw grid line between rows of cells. */
	    if (grid_flag && (lasty != NEG_INFINITY))
	    {
		if (fb_write(fbiop, 0, y0, buf, fb_width) == -1)
		{
		    fb_log("Couldn't write to <%d,%d>\n", 0, y0);
		    (void) fb_close(fbiop);
		    return (false);
		}
		if (debug_flag & CFB_DBG_GRID)
		{
		    fprintf(stderr, "Writing grid row at %d\n", y0);
		    fflush(stderr);
		}
	    }
	    lasty = gp->c_y;
	}
	val_To_RGB(gp->c_val, pixel);
	for (x0 = H2SCRX(gp->c_x), x1 = x0 + wid; x0 < x1;  x0++)
	{
	    COPYRGB(buf[x0], pixel);
	}
    }

    /* Write out last row of cells. */
    for (y0 = V2SCRY(lasty), y1 = y0 + hgt; y0 < y1;  y0++)
	if (fb_write(fbiop, 0, y0, buf, fb_width) == -1)
	{
	    fb_log("Couldn't write to <%d,%d>\n", 0, y0);
	    (void) fb_close(fbiop);
	    return (false);
	}
    /* Draw color key. */
    if (key_flag)
    {	
	register int	i, j;
	double		base;

	/* Clear buffer. */
	for (i = 0; i < fb_width; i++)
	{
	    COPYRGB(buf[i], BACKGROUND);
	}
	base = (xmin+xmax) / 2 - 5 * cell_size;
	base = H2SCRX(base);
	for (i = 0; i <= 10; i++)
	{	
	    cell_val	cv;

	    cv.v_scalar = i / 10.0;

	    val_To_RGB(cv, pixel);
	    for (j = 0; j < wid; j++)
	    {	
		int offset = i * (wid+grid_flag);
		register int index = base + offset + j;
		COPYRGB(buf[index], pixel);
	    }
	}

	for (i = yorigin; i < yorigin+hgt; i++)
	    if (fb_write(fbiop, 0, i, buf, fb_width) == -1)
	    {
		fb_log("Couldn't write to <%d,%d>\n", 0, i);
		(void) fb_close(fbiop);
		return (false);
	    }
    }
    (void) fb_close(fbiop);
    return (true);
}

STATIC void val_To_RGB (cv, rgb)

cell_val	cv;
RGBpixel	rgb;

{
    double	val;

    if (color_flag)
    {
	COPYRGB(rgb, cv.v_color);
	return;
    }
    val = (cv.v_scalar - dom_min) * dom_cvt;
    if ((boolean_flag && (cv.v_scalar != bool_val))
	|| (val < 0.0) || (val > 10.0))
    {
	COPYRGB(rgb, BACKGROUND);
    }
    else if (val == 0.0)
    {
	COPYRGB(rgb, WHITE);
    }
    else
    {	
	int		index;
	double		rem;
	double		res;

	if (interp_flag)
	{
	    index = val + 0.01; /* convert to range [0 to 10] */
	    if ((rem = val - (double) index) < 0.0) /* remainder */
		rem = 0.0;
	    res = 1.0 - rem;
	    rgb[RED] = res*colortbl[index][RED]
			+ rem*colortbl[index+1][RED];
	    rgb[GRN] = res*colortbl[index][GRN]
			+ rem*colortbl[index+1][GRN];
	    rgb[BLU] = res*colortbl[index][BLU]
			+ rem*colortbl[index+1][BLU];
	}
	else
	{
	    index = val + 0.51;
	    COPYRGB(rgb, colortbl[index]);
	}
    }
    return;
}

STATIC bool pars_Argv (argc, argv)

register int	argc;
register char	**argv;

{	
    register int	c;
    extern int		optind;
    extern char		*optarg;

    /* Parse options. */
    while ((c = getopt(argc, argv, OPT_STRING)) != EOF)
    {
	switch (c)
	{
	    case 'C':
		color_flag = true;
		break;
	    case 'F':
		(void) strncpy(fbfile, optarg, MAX_LINE);
		break;
	    case 'N':
		if (sscanf(optarg, "%d", &fb_height) < 1)
		{
		    fb_log("Invalid frame-buffer height: '%s'\n", optarg);
		    return (false);
		}
		if (fb_height < -1)
		{
		    fb_log("Frame-buffer height out of range: %d\n", fb_height);
		    return (false);
		}
		break;
	    case 'W':
		if (sscanf(optarg, "%d", &fb_width) < 1)
		{
		    fb_log("Invalid frame-buffer width: '%s'\n", optarg);
		    return (false);
		}
		if (fb_width < -1)
		{
		    fb_log("Frame-buffer width out of range: %d\n", fb_width);
		    return (false);
		}
		break;
	    case 'S':
		if (sscanf(optarg, "%d", &fb_height) < 1)
		{
		    fb_log("Invalid frame-buffer dimension: '%s'\n", optarg);
		    return (false);
		}
		if (fb_height < -1)
		{
		    fb_log("Frame-buffer dimensions out of range: %d\n",
			fb_height);
		    return (false);
		}
		fb_width = fb_height;
		break;
	    case 'X':
		if (sscanf(optarg, "%x", &debug_flag) < 1)
		{
		    fb_log("Invalid debug flag: '%s'\n", optarg);
		    return (false);
		}
		break;
	    case 'b':
		if (sscanf(optarg, "%lf", &bool_val) != 1)
		{
		    fb_log("Invalid boolean value: '%s'\n", optarg);
		    return (false);
		}
		boolean_flag = true;
		break;
	    case 'c':
		if (sscanf(optarg, "%lf", &cell_size) != 1)
		{
		    fb_log("Invalid cell size: '%s'\n", optarg);
		    return (false);
		}
		if (cell_size <= 0)
		{
		    fb_log("Cell size out of range: %d\n", cell_size);
		    return (false);
		}
		break;
	    case 'd':
		if (sscanf(optarg, "%lf %lf", &dom_min, &dom_max) < 2)
		{
		    fb_log("Invalid domain for input: '%s'\n", optarg);
		    return (false);
		}
		if (dom_min >= dom_max)
		{
		    fb_log("Bad domain for input: [%lf, %lf]\n",
			dom_min, dom_max);
		    return (false);
		}
		dom_cvt = 10.0 / (dom_max - dom_min);
		break;
	    case 'e':
		erase_flag = true;
		break;
	    case 'f':
		if (sscanf(optarg, "%d", &field) != 1)
		{
		    fb_log("Invalid field: '%s'\n", optarg);
		    return (false);
		}
		break;
	    case 'g':
		grid_flag = true;
		break;
	    case 'h':
		hires_flag = true;
		break;
	    case 'i':
		interp_flag = false;
		break;
	    case 'k':
		key_flag = true;
		key_height = 2.5;
		break;
	    case 'l':
		if (sscanf(optarg, "%f%f", &az, &el) != 2)
		{
		    fb_log("Invalid view: '%s'\n", optarg);
		    return (false);
		}
		log_flag = true;
		if (view_flag == 0)
		    view_flag = 1;
		break;

	    case 'm':
		{	
		    double	value;
		    RGBpixel	rgb;
		    int		red, grn, blu;
		    int		index;

		    if (sscanf(optarg, "%lf %d %d %d", &value, &red, &grn, &blu)
			< 4)
		    {
			fb_log("Invalid color-mapping: '%s'\n",
			    optarg);
			return (false);
		    }
		    value *= 10.0;
		    index = value + 0.01;
		    if (index < 0 || index > MAX_COLORTBL)
		    {
			fb_log("Value out of range (%s)\n", optarg);
			return (false);
		    }
		    rgb[RED] = red;
		    rgb[GRN] = grn;
		    rgb[BLU] = blu;
		    COPYRGB(colortbl[index], rgb);
		    break;
		}
	    case 'p':
		switch (sscanf(optarg, "%d %d", &xorigin, &yorigin))
		{
		    case 2: break;
		    case 1: yorigin = xorigin; break;
		    default:
			fb_log("Invalid offset: '%s'\n", optarg);
			return (false);
		}
		break;
	    case 's':
		switch (sscanf(optarg, "%d %d", &wid, &hgt))
		{
		    case 2: break;
		    case 1: hgt = wid; break;
		    default:
			fb_log("Invalid cell scale: '%s'\n", optarg);
			return (false);
		}
		break;
	    case 'v':
		if (sscanf(optarg, "%d", &view_flag) < 1)
		{
		    fb_log("Invalid view number: '%s'\n", optarg);
		    return (false);
		}
		if (view_flag == 0)
		    log_flag = false;
		break;
	    case '?':
		return (false);
	}
    }

    if (argc == optind + 1)
    {
	if ((filep = fopen(argv[optind], "r")) == NULL)
	{
	    fb_log("Cannot open file '%s'\n", argv[optind]);
	    return (false);
	}
    }
    else if (argc != optind)
    {
	fb_log("Too many arguments!\n");
	return (false);
    }
    else
	filep = stdin;

    compute_fb_height = (fb_height == -1) ? SNUG_FIT :
			(fb_height == 0) ? LOOSE_FIT : false;
    compute_fb_width = (fb_width == -1) ? SNUG_FIT :
			(fb_width == 0) ? LOOSE_FIT : false;
    return (true);
}

/*	prnt_Usage() --	Print usage message. */
STATIC void prnt_Usage()
{	
    register char	**p = usage;

    while (*p)
	fb_log("%s\n", *p++);
    return;
}

STATIC void log_Run()
{
    long                clock;
    struct tm           *tempus;
    static char         *mon_nam[] =
                        { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                          "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    mat_t		model2hv;		/* model to h,v matrix */
    mat_t		hv2model;		/* h,v tp model matrix */
    quat_t		orient;			/* orientation */
    point_t		hv_eye;			/* eye position in h,v coords */
    point_t		m_eye;			/* eye position in model coords */
    fastf_t		hv_viewsize;		/* size of view in h,v coords */
    fastf_t		m_viewsize;		/* size of view in model coords. */

    /* Current date and time get printed in header comment */
    (void) time(&clock);
    tempus = localtime(&clock);

    (void) printf(
	"# Log information produced by CELL-FB %02d %s %4d at %02d%02d\n",
	tempus -> tm_mday, mon_nam[tempus -> tm_mon],
	tempus -> tm_year + 1900, tempus -> tm_hour, tempus -> tm_min);
    (void) printf("az_el: %f %f\n", az, el);
    (void) printf("view_extrema: %f %f %f %f\n",
	SCRX2H(0), SCRX2H(fb_width), SCRY2V(0), SCRY2V(fb_height));
    (void) printf("fb_size: %d %d\n", fb_width, fb_height);

	/* Produce the orientation, the model eye_pos, and the model
	 * view size for input into rtregis.
 	 * First use the azimuth and elevation to produce the model2hv
	 * matrix and use that to find the orientation.
	 */

	mat_idn( model2hv );
	mat_idn( hv2model );

	/* Print out the "view" just to keep rtregis from belly-aching */

	printf("View: %g azimuth, %g elevation\n", az, el);

	/** mat_ae( model2hv, az, el ); **/
	/* Formula from rt/do.c */
	mat_angles( model2hv, 270.0+el, 0.0, 270.0-az );
	model2hv[15] = 25.4;		/* input is in inches */
	mat_inv( hv2model, model2hv);

	quat_mat2quat( orient, model2hv );

	printf("Orientation: %g, %g, %g, %g\n", V4ARGS(orient) );

	/* Now find the eye position in h, v space.  Note that the eye
	 * is located at the center of the image; in this case, the center
	 * of the screen space, i.e., the framebuffer. )
	 * Also find the hv_viewsize at this time.
	 */
	hv_viewsize = SCRX2H( fb_width - 1 ) - SCRX2H( 0 );
	hv_eye[0] = SCRX2H( fb_width/2 + 0.5 );
	hv_eye[1] = SCRY2V( fb_height/2 + 0.5 );
	hv_eye[2] = hv_viewsize/2;

printf("SCRX2H(fb_width -1) = %g; SCRX2H(0) = %g; hv_viewsize= %g\n",
	SCRX2H(fb_width - 1), SCRX2H(0), hv_viewsize);

	/* Now find the model eye_position and report on that */

printf("hv_eye= %g, %g, %g\n", V3ARGS(hv_eye) );

	MAT4X3PNT( m_eye, hv2model, hv_eye );

	printf("Eye_pos: %g, %g, %g\n", V3ARGS(m_eye) );

	/* Now find the view size in model coordinates and print that
	 * as well.
	 */

	m_viewsize = hv_viewsize/hv2model[15];
	printf("Size: %g\n", m_viewsize);

}
