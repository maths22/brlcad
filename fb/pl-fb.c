/*
 *			I K P L O T . C
 *
 *	Program to take UNIX plot data and output on IKONAS display.
 *
 *	Joseph C. Pistritto JCP@BRL
 *	US Army Ballistic Research Laboratory
 *
Function:
	Reads device-independent plot data from specified input file;
	for each frame, builds an image file containing raster data then
	sends the frame output to the output device.

	Edge-limiting is done here; use "rot" if clipping is desired.

Method:
	Inputs vector data and builds a rasterization descriptor for
	each visible stroke.  (Strokes are limited to frame boundaries.)
	X goes down the page, Y goes from left to right.  To obtain a
	different orientation, pre-process data with the "rot" filter.
	(Quadrant 1 graphics devices)

	The frame image file of SCANS scans is considered artificially
	divided into BANDS bands, each containing SPB scans.  Each band
	has a linked list of descriptors for not-yet-rasterized strokes
	that start in the band.

	Space for descriptors is obtained via "malloc".  When no more
	space is available, the image file is updated as follows, then
	"malloc" is tried again ("must" work the second time):

	Each band in increasing X order becomes "active"; if no
	descriptors exist for the band it is skipped, otherwise its
	existing raster data is re-read from the image file into a
	buffer and each descriptor is processed to rasterize its stroke.
	If the stroke terminates in the band its descriptor is freed,
	otherwise the descriptor is linked into the following band's
	list.  When the descriptor list for the active band becomes
	empty (must happen), the band's raster data is flushed back to
	the image file and the next band becomes active.  This process
	continues until all vectors have been input and rasterized.

Acknowledgment:
	Based rather heavily on Doug Gwyn's Versatec PLOT rasterizer VPL.C
	which was
	Based very loosely on Mike Muuss's Versatec TIGpack interpreter.
*/

#define STATIC	/* nothing, for debugging */

#include	<signal.h>
#include	<stdio.h>

/*
	Raster device model and image terminology as used herein:

Frames are delimited in plot data by "erase" codes; each frame looks
like:


			    top of frame
	---------------------------------------------------------
	|-----> plot						|
	||	X axis						|
 ^	||							|
 |	|v							|
	|plot							|
	|Y axis
	|			|				|
 ^	|			v				|
 |	|			.				|
	|scan line ->...........................................|
	|			.				|
	---------------------------------------------------------
			    bottom of frame


Each plot-mode scan line consists of exactly BYTES bytes of plot data.

Each scan line is composed of four bytes of color data, for the Red,
Green, and Blue DACS, (the fourth byte is unused here, and is set to 0)
for each pixels, times the number of pixels desired (512 in LORES,
1024 in HIRES)

*/

/*	IKONAS Device Parameters				 */

#define	PIXELSIZE	4		/* # bytes per pixel */

/* the following parameter should be tweaked for fine-tuning */
#define SPB		16		/* scan lines per band */
#define	X_CHAR_SIZE	(8)		/* pixels per char horizontal */
#define	Y_CHAR_SIZE	(10)		/* pixels per char vertical */

#define	CLEAR	0			/* value for no intensity */


/*	Program constants computed from device parameters:	*/

#define BANDS	(Nscanlines / SPB)		/* # of "bands" */
#define BYTES	(Npixels * PIXELSIZE)	/* max data bytes per scan */
#define XMAX	(Npixels - 1)
#define YMAX	(Nscanlines - 1)

/*	Data structure definitions:	*/

#ifdef pdp11
typedef char	tiny;			/* for very small numbers */
#else
typedef	short	tiny;			/* for very small numbers */
#endif

typedef int	bool;			/* boolean data type */
#define false	0
#define true	1

char	color[PIXELSIZE] = { 255, 255, 255, 0 };

typedef struct
	{
	short		x;
	short		y;
	}	coords; 		/* Cartesian coordinates */

typedef struct descr
	{
	struct descr	*next;		/* next in list, or NULL */
	coords		pixel;		/* starting scan, nib */
	tiny		xsign;		/* 0 or +1 */
	tiny		ysign;		/* -1, 0, or +1 */
	bool		ymajor; 	/* true iff Y is major dir. */
	short		major;		/* major dir delta (nonneg) */
	short		minor;		/* minor dir delta (nonneg) */
	short		e;		/* DDA error accumulator */
	short		de;		/* increment for `e' */
	char		col[PIXELSIZE];	/* COLOR of this vector */
	}	stroke; 		/* rasterization descriptor */

/*	Global data allocations:	*/

STATIC struct
	{
	short		left;		/* window edges */
	short		bottom;
	short		right;
	short		top;
	}	space;	 		/* plot scale data */

STATIC struct
	{
	short		left;		/* 3d window edges */
	short		bottom;
	short		front;
	short		right;
	short		top;
	short		back;
	}	space3;

struct	relvect {
	char	x,y;			/* x, y values (255,255 is end) */
};

#define	END	-1,-1			/* end of stroke description */
#define	NIL	0,0
#define min(a,b)	((a)<(b)?(a):(b))
/*
 *  These character sets are taken from the Motorola MC6575 Pattern Generator,
 *  page 5-119 of 'The Complete Motorola Microcomputer Data Library'
 */
STATIC struct vectorchar {
	char		ascii;		/* ASCII character emulated */
	struct	relvect	r[10];		/* maximum # of vectors 1 char */
} charset[] = {
/*ASCII <1>    <2>    <3>    <4>    <5>    <6>    <7>    <8>    <9>    <10> */
   '0', 5,0,   1,0,   0,1,   0,7,   6,1,   6,7,   5,8,   1,8,   END,   NIL,
   '1', 1,2,   3,0,   3,8,   5,8,   1,8,   END,   NIL,   NIL,   NIL,   NIL,
   '2', 0,1,   1,0,   5,0,   6,1,   6,2,   4,4,   2,4,   0,6,   0,8,   6,8,
   '3', 1,0,   5,0,   6,1,   6,4,   2,4,   6,4,   6,7,   5,8,   1,8,   END,
   '4', 5,8,   5,0,   0,5,   0,6,   6,6,   END,   NIL,   NIL,   NIL,   NIL,
   '5', 6,0,   0,0,   0,3,   4,3,   6,5,   6,6,   4,8,   1,8,   0,7,   END,
   '6', 5,0,   2,0,   0,2,   0,7,   1,8,   5,8,   6,7,   6,5,   5,4,   1,4,
   '7', 0,1,   0,0,   6,0,   6,1,   2,5,   2,8,   END,   NIL,   NIL,   NIL,
   '8', 1,0,   5,0,   6,1,   6,7,   5,8,   1,8,   0,7,   0,1,   0,4,   6,4,
   '9', 1,8,   4,8,   6,6,   6,1,   5,0,   1,0,   0,1,   0,3,   1,4,   6,4,
   'A', 0,8,   0,2,   2,0,   4,0,   6,2,   6,8,   6,5,   0,5,   END,   NIL,
   'B', 6,5,   6,7,   5,8,   0,8,   0,0,   5,0,   6,1,   6,3,   5,4,   0,4,
   'C', 6,1,   5,0,   2,0,   0,2,   0,6,   2,8,   5,8,   6,7,   END,   NIL,
   'D', 0,0,   4,0,   6,2,   6,6,   4,8,   0,8,   0,0,   END,   NIL,   NIL,
   'E', 6,0,   0,0,   0,4,   3,4,   0,4,   0,8,   6,8,   END,   NIL,   NIL,
   'F', 6,0,   0,0,   0,4,   3,4,   0,4,   0,8,   END,   NIL,   NIL,   NIL,
   'G', 6,1,   5,0,   2,0,   0,2,   0,6,   2,8,   5,8,   6,7,   6,5,   3,5,
   'H', 0,0,   0,8,   0,4,   6,4,   6,0,   6,8,   END,   NIL,   NIL,   NIL,
   'I', 1,0,   5,0,   3,0,   3,8,   1,8,   5,8,   END,   NIL,   NIL,   NIL,
   'J', 2,0,   6,0,   4,0,   4,7,   3,8,   1,8,   0,7,   END,   NIL,   NIL,
   'K', 0,0,   0,8,   0,6,   6,0,   2,4,   6,8,   END,   NIL,   NIL,   NIL,
   'L', 0,0,   0,8,   6,8,   END,   NIL,   NIL,   NIL,   NIL,   NIL,   NIL,
   'M', 0,8,   0,0,   3,3,   3,4,   3,3,   6,0,   6,8,   END,   NIL,   NIL,
   'N', 0,8,   0,0,   6,6,   6,8,   6,0,   END,   NIL,   NIL,   NIL,   NIL,
   'O', 0,6,   0,2,   2,0,   4,0,   6,2,   6,6,   4,8,   2,8,   0,6,   END,
   'P', 0,8,   0,0,   5,0,   6,1,   6,3,   5,4,   0,4,   END,   NIL,   NIL,
   'Q', 6,6,   6,2,   4,0,   2,0,   0,2,   0,6,   2,8,   4,8,   4,6,   6,8,
   'R', 0,8,   0,0,   5,0,   6,1,   6,3,   5,4,   0,4,   2,4,   6,8,   END,
   'S', 6,1,   5,0,   1,0,   0,1,   0,4,   5,4,   6,5,   6,7,   5,8,   0,8,
   'T', 0,0,   6,0,   3,0,   3,8,   END,   NIL,   NIL,   NIL,   NIL,   NIL,
   'U', 0,0,   0,7,   1,8,   5,8,   6,7,   6,0,   END,   NIL,   NIL,   NIL,
   'V', 0,0,   0,2,   3,8,   6,2,   6,0,   END,   NIL,   NIL,   NIL,   NIL,
   'W', 0,0,   0,8,   3,5,   3,4,   3,5,   6,8,   6,0,   END,   NIL,   NIL,
   'X', 0,0,   6,8,   3,4,   0,8,   6,0,   END,   NIL,   NIL,   NIL,   NIL,
   'Y', 0,0,   0,1,   3,4,   3,8,   3,4,   6,1,   6,0,   END,   NIL,   NIL,
   'Z', 0,0,   6,0,   6,1,   0,7,   0,8,   6,8,   END,   NIL,   NIL,   NIL,
   '+', 0,4,   6,4,   3,4,   3,1,   3,7,   END,   NIL,   NIL,   NIL,   NIL,
   '-', 0,4,   6,4,   END,   NIL,   NIL,   NIL,   NIL,   NIL,   NIL,   NIL,
   '/', 0,7,   6,1,   END,   NIL,   NIL,   NIL,   NIL,   NIL,   NIL,   NIL,
   '(', 4,0,   2,2,   2,6,   4,8,   END,   NIL,   NIL,   NIL,   NIL,   NIL,
   ')', 2,0,   4,2,   4,6,   2,8,   END,   NIL,   NIL,   NIL,   NIL,   NIL,
   '<', 4,0,   0,4,   4,8,   END,   NIL,   NIL,   NIL,   NIL,   NIL,   NIL,
   '>', 2,0,   6,4,   2,8,   END,   NIL,   NIL,   NIL,   NIL,   NIL,   NIL,

/* Some needed chars, hastily drawn -MJM */
   '.', 4,7,   3,7,   3,6,   4,6,   4,7,   END,   NIL,   NIL,   NIL,   NIL,
   ',', 4,6,   3,6,   3,5,   4,5,   4,8,   END,   NIL,   NIL,   NIL,   NIL,

   NULL
};

STATIC int	Nscanlines = 512;
STATIC int	Npixels = 512;

STATIC long	delta;			/* larger window dimension */
STATIC long	deltao2;		/* delta / 2 */

struct band  {
	stroke	*first;
	stroke	*last;
};
STATIC struct band	*band;		/* array of descriptor lists */
STATIC struct band	*bandEnd;

STATIC char	*buffer;		/* ptr to active band buffer */
STATIC long	buffersize;		/* active band buffer bytes */
STATIC short	ystart = 0;		/* active band starting scan */
STATIC short	debug  = 0;
STATIC short	overlay = 0;		/* !0 to overlay on existing image */

STATIC int	sigs[] =		/* signals to be caught */
	{
	SIGHUP,
	SIGINT,
	SIGQUIT,
	SIGTERM
	};

 
/*	Externals:	*/

STATIC FILE	*pfin;		/* input file FIO block ptr */
extern void	free();
extern char	*malloc();
extern int	close(), creat(), getpid(), kill(), open(), read(),
		write();

extern int	ikhires;		/* for ik library */
extern int	ikfd;			/* from ik library */

/*	Local subroutines:	*/

STATIC int	DoFile(), Foo();
STATIC stroke	*Allocate(), *Dequeue();
STATIC bool	BuildStr(), GetCoords(),
		OutBuild(), RdBand(), WrBand();
STATIC void	Catch(), FreeUp(), InitDesc(), Queue(),
		Requeue(),
		Raster(), SetSigs();

/*
 *  M A I N
 *
 *	Parse arguments, valid ones are:
 *		name of file to plot (instead of STDIN)
 *		-d for debugging statements
 *
 *	Default (no arguments) action is to plot STDIN on STDOUT
 *	as an IKONAS
 */
main(argc, argv)
int argc;
char **argv;
{
	register int	i;
	register char	*filename = NULL;
	char		*cp;
	
	Nscanlines = Npixels = 512;
	for(i = 1; i < argc; i++)
		if( argv[i][0] == '-' )
			switch( argv[i][1] )  {
				
			case 'd':
				debug = 1;
				break;

			case 'O':
			case 'o':
				overlay = 1;
				break;

			case 'h':
				ikhires = 1;
				Nscanlines = Npixels = 1024;
				break;

			default:
				fprintf(stderr, "unknown option %s\n",
					argv[i] );
			}
		else
			filename = argv[i];

	/*
	 * Handle image-size specific initializations
	 */
	if( (Nscanlines % SPB) != 0 )  {
		fprintf(stderr, "Nscanlines % SPB != 0\n");
		exit(1);
	}
	space.left = space.right = 0;
	space.right = Npixels;
	space.top = Nscanlines;
	delta = Nscanlines;
	deltao2 = Nscanlines/2;

	buffersize = SPB*Npixels*PIXELSIZE;
	if( (buffer = malloc(buffersize)) == (char *)0)  {
		fprintf(stderr,"ikplot:  malloc error\n");
		exit(1);
	}
	band = (struct band *)malloc(BANDS*sizeof(struct band));
	if( band == (struct band *)0 )  {
		fprintf(stderr,"ikplot: malloc error2\n");
		exit(1);
	}
	bzero( (char *)band, BANDS*sizeof(struct band) );
	bandEnd = &band[BANDS];

	if( ikopen() < 0 )  {
		fprintf(stderr,"ikplot: ikopen failed\n");
		exit(1);
	}

	/*
	 *  Plot the selected filename -- note
	 *  with no arguments, we plot STDIN.
	 */
	if( debug )
		fprintf(stderr, "ikplot output of %s\n", filename);
		
	if ( filename == NULL || filename[0] == 0 )
		pfin = stdin;
	else if( (pfin = fopen( filename, "r" )) == NULL )
		return Foo( -2 );

	SetSigs();			/* set signal catchers */

	(void)DoFile( );	/* plot it */
}


/*
	DoFile - process UNIX plot file onto IKONAS

	This routine reads UNIX plot records from the specified file
	and controls the entry of the strokes into the descriptor lists.
	Strokes are limited (not clipped) to fit the frame.

	Upon end of file or erase, plot data is copied to the device.
	Returns status code:
		   < 0	=> catastrophe
		   = 0	=> complete success
		   > 0	=> line limit hit
*/
STATIC int
DoFile( )	/* returns vpl status code */
{
	register bool	plotted;	/* false => empty frame image */
	register int	c;		/* input character */
	coords		newpos; 	/* current input coordinates */
	coords		virpos; 	/* virtual pen position */

	/* process each frame into a raster image file */

	for ( ; ; )			/* for each frame */
	{
		InitDesc();		/* empty descriptor lists */

		virpos.x = virpos.y = 0;
		plotted = false;

		for ( ; ; )		/* read until EOF or erase */
		{
			switch ( c = getc( pfin ) )
			{	/* record type */
			case EOF:
			case 'e':	/* erase */
				if( debug )
					fprintf( stderr,"Erase or EOF\n");
					
				if ( plotted )
				{
					/* flush strokes */
					if ( !OutBuild() )
						return Foo( -6 );

				}

				if ( c == EOF )
					return Foo( 0 );/* success */

				(void)lseek( ikfd, 0L, 0 );	/* top */
				break;	/* next frame */

			case 'f':	/* linemod */
				if(debug)
					fprintf( stderr,"linemod\n");
				/* ignore for time being */
				while ( (c = getc( pfin )) != EOF
				     && c != '\n'
				      )
					;	/* eat string */
				continue;

			case 'L':
			case 'M':
				if ( !Get3Coords( &newpos ) )
					return Foo( -8 );
				virpos = newpos;
				if( c == 'M'  )  {
					if( debug )
						fprintf( stderr,"Move3\n");
					continue;
				}
				if( debug )
					fprintf( stderr,"Line3\n");

			case 'N':	/* continue3 */
			case 'P':	/* point3 */
				if ( !Get3Coords( &newpos ) )
					return Foo( -9 );
				if ( c == 'P' )  {
					if( debug )
						fprintf( stderr,"point3\n");
					virpos = newpos;
				} else
					if( debug )
						fprintf( stderr,"cont3\n");

				if ( !BuildStr( &virpos, &newpos ) )
					return Foo( -10 );
				plotted = true;
				virpos = newpos;
				continue;
			
			case 'l':	/* line */
			case 'm':	/* move */
				if ( !GetCoords( &newpos ) )
					return Foo( -8 );
				virpos = newpos;
				if ( c == 'm' )  {
					if( debug )
						fprintf( stderr,"move\n");
					continue;
				}
				/* line: fall through */
				if( debug )
					fprintf( stderr,"line\n");

			case 'n':	/* cont */
			case 'p':	/* point */
				if ( !GetCoords( &newpos ) )
					return Foo( -9 );
				if ( c == 'p' )  {
					if( debug )
						fprintf( stderr,"point\n");
					virpos = newpos;
				} else
					if( debug )
						fprintf( stderr,"cont\n");

				if ( !BuildStr( &virpos, &newpos ) )
					return Foo( -10 );
				plotted = true;
				virpos = newpos;
				continue;

			case 'S':
				if( debug )
					fprintf( stderr,"space3\n");
				if( fread( (char *)&space3,
					   (int)sizeof space3, 1, pfin)
					   != 1
				  )
				  	return Foo( -11 );
				/* turn 3 space into 2 space */
				space.right = space3.right;
				space.left  = space3.left;
				space.top   = space3.top;
				space.bottom= space3.bottom;
				goto spacend;
				
			case 's':	/* space */
				if( debug )
					fprintf( stderr,"space\n");
				if ( fread( (char *)&space,
					    (int)sizeof space, 1, pfin
					  ) != 1
				   )
					return Foo( -11 );

spacend:
				delta = (long)space.right
				      - (long)space.left;
				deltao2 = (long)space.top
					- (long)space.bottom;
				if ( deltao2 > delta )
					delta = deltao2;
				deltao2 = (delta + 1L) / 2L;

				continue;

			case 'C':	/* color */
				if( fread( &color[0], 1, 3, pfin) != 3 )
					return Foo( -11 );
				if( debug )
					fprintf( stderr,"Color is R%d G%d B%d\n",
						color[0], color[1], color[2]);
				continue;
						
			case 't':	/* label */
				if( debug )
					fprintf( stderr,"label: ");

				newpos = virpos;			
				while ( (c = getc( pfin )) != EOF && c != '\n'
				      )  {
					/* vectorize the characters */
					put_vector_char( c, &newpos);
				
					if( debug )
						putc( c, stderr );
				      }

				plotted = true;
				virpos = newpos;
				continue;
				
			default:
				if( debug )
					fprintf( stderr,"action %c ignored\n", c);
					
				return Foo( -12 );	/* bad input */
			}
			break;
		}		/* next input record */
	}			/* next frame */
}

/*
 *	PutVectorChar - Put vectors corresponding to this character out
 * 	into plotting space
 *	Update position to reflect character width.
 */
put_vector_char( c, pos )
register char	c;
register coords	*pos;
{
	static coords	start, end;
	register struct vectorchar	*vc;
	register struct relvect		*rv;

	if( c >= 'a' && c <= 'z' )
		c = c - 'a' + 'A';	/* xlate to upper case */
	
	for( vc = &charset[0]; vc->ascii; vc++)
		if( vc->ascii == c )
			break;

	if( !vc->ascii )  {
		/* Character not found -- space over 1/2 char */
		pos->x += X_CHAR_SIZE/2;
		return;
	}

	/* have the correct character entry - start plotting */
	start.x = vc->r[0].x + pos->x;
	start.y = vc->r[0].y + pos->y - Y_CHAR_SIZE;

	for( rv = &vc->r[1]; (rv < &vc->r[10]) && rv->x >= 0; rv++ )  {
		end.x = rv->x + pos->x;
		end.y = rv->y + pos->y - Y_CHAR_SIZE;
		edgelimit( &start );
		edgelimit( &end );
		BuildStr( &start, &end );	/* pixels */
		start = end;
	}
	pos->x += X_CHAR_SIZE;
}

/*
 *	E D G E L I M I T
 *
 *	Limit generated positions to edges of screen
 */
edgelimit( ppos )
register coords *ppos;
{
	if( ppos->x >= Npixels )
		ppos->x = Npixels -1;

	if( ppos->y >= Nscanlines )
		ppos->y = Nscanlines -1;
}

/*
	GetCoords - input x,y coordinates and scale into pixels
*/
STATIC bool Get3Coords( coop )
register coords	*coop;
{
	short	trash;
	register bool	ret;

	ret = GetCoords( coop );
	fread( &trash, sizeof(trash), 1, pfin );
	return( ret );
}

STATIC bool
GetCoords( coop )
	register coords	*coop;		/* -> input coordinates */
	{
	/* read coordinates */

	if ( fread( (char *)coop, (int)sizeof (coords), 1, pfin ) != 1 )
		return false;

	/* limit left, bottom */

	if ( (coop->x -= space.left) < 0 )
		coop->x = 0;
	if ( (coop->y -= space.bottom) < 0 )
		coop->y = 0;

	/* convert to device pixels */

	coop->x = (short)(((long)coop->x * Npixels + deltao2) / delta);
	coop->y = (short)(((long)coop->y * Nscanlines + deltao2) / delta);

	/* REVERSE the y axis for those of you who like plots origin down */
	coop->y = Nscanlines - coop->y;
	
	/* limit right, top */

	if ( coop->x > XMAX )
		coop->x = XMAX;
	if ( coop->y > YMAX )
		coop->y = YMAX;

	if( debug )
		fprintf( stderr,"Coord: (%d,%d)\n", coop->x, coop->y);
		
	return true;
	}

/*
	InitDesc - initialize stroke descriptor lists
*/

STATIC void
InitDesc()
	{
	register struct band *bp;	/* *bp -> start of descr list */

	for ( bp = &band[0]; bp < bandEnd; ++bp )
		bp->first = NULL;		/* nothing in band yet */
		bp->last  = NULL;
	}


/*
	Queue - queue descriptor onto band list

	Note that descriptor order is not important.
*/

STATIC void
Queue( bp, hp, vp )
	register struct band *bp;
	register stroke **hp;		/* *hp -> first descr in list */
	register stroke *vp;		/* -> new descriptor */
{
	vp->next = *hp; 		/* -> first in existing list */
	*hp = vp;			/* -> new first descriptor */
	if( vp->next == NULL )
		bp->last = vp;		/* update end pointer */
}

/*
 * 	Requeue - enqueue descriptor at END of band list
 */
STATIC void
Requeue( bp, vp )
register struct band *bp;
register stroke	     *vp;
{
	vp->next = NULL;
	if( bp->last )
		bp->last->next = vp;
	else
		bp->first = vp;
		
	bp->last = vp;
}

/*
	Dequeue - remove descriptor from band list (do not free space)
*/

STATIC stroke *
Dequeue( bp, hp )				/* returns addr of descriptor,
					   or NULL if none left */
	register struct band *bp;
	register stroke **hp;		/* *hp -> first descr in list */
	{
	register stroke *vp;		/* -> descriptor */

	if ( (vp = *hp) != NULL )
		*hp = vp->next; 	/* -> next element in list */

	if( vp == NULL )
		bp->last = NULL;
		
	return vp;			/* may be NULL */
	}


/*
	FreeUp - deallocate descriptors
*/

STATIC void
FreeUp()
	{
	register struct band *bp;
	register stroke *vp;		/* -> rasterization descr */

	for ( bp = &band[0]; bp < bandEnd; ++bp )
		while ( (vp = Dequeue( bp, &bp->first )) != NULL )
			free( (char *)vp );	/* free storage */
	}

/*
	RdBand - Read in next band from output image file,
		for overlay purposes.  Reposition back.
*/
STATIC bool
RdBand()
{
	register char	*bp = buffer;
	register int	size;

	size = 32768;	/* default DMA size */
	for( bp = buffer; buffersize - (bp - buffer) > 0; bp += size)  {
		size = min( size, buffersize - (bp - buffer) );
		if( read(ikfd, bp, size) != size )
			return false;
	}
	lseek( ikfd, -buffersize, 1);
	return true;
}

/*
	WrBand - Write out next band to output image file
*/
STATIC bool
WrBand()
{
	register char	*bp = buffer;
	register int	size;

	size = 32768;	/* default DMA size */
	for( bp = buffer; buffersize - (bp - buffer) > 0; bp += size)  {
		size = min( size, buffersize - (bp - buffer) );
		if( write(ikfd, bp, size) != size )
			return false;
	}
	return true;
}

/*
	BuildStr - set up DDA parameters and queue stroke
*/

STATIC bool
BuildStr( pt1, pt2 )			/* returns true unless bug */
	coords		*pt1, *pt2;	/* endpoints */
	{
	register stroke *vp;		/* -> rasterization descr */

	/* arrange for pt1 to have the smaller Y-coordinate: */

	if ( pt1->y > pt2->y )
		{
		register coords *temp;	/* temporary for swap */

		temp = pt1;		/* swap pointers */
		pt1 = pt2;
		pt2 = temp;
		}

	if ( (vp = Allocate()) == NULL )	/* alloc a descriptor */
		return false;		/* "can't happen" */

	/* set up multi-band DDA parameters for stroke */

	vp->pixel = *pt1;		/* initial pixel */
	vp->major = pt2->y - vp->pixel.y;	/* always nonnegative */
	vp->ysign = vp->major ? 1 : 0;
	vp->minor = pt2->x - vp->pixel.x;
	*((long *)vp->col) = *((long *)color);		/* record color fast */
	if ( (vp->xsign = vp->minor ? (vp->minor > 0 ? 1 : -1) : 0) < 0
	   )
		vp->minor = -vp->minor;

	/* if Y is not really major, correct the assignments */

	if ( !(vp->ymajor = vp->minor <= vp->major) )
		{
		register short	temp;	/* temporary for swap */

		temp = vp->minor;
		vp->minor = vp->major;
		vp->major = temp;
		}

	vp->e = vp->major / 2 - vp->minor;	/* initial DDA error */
	vp->de = vp->major - vp->minor;

	/* link descriptor into band corresponding to starting scan */

	Requeue( &band[vp->pixel.y / SPB], vp );

	return true;
	}

/*
	Allocate - allocate a descriptor (possibly updating image file)
*/

STATIC stroke *
Allocate()				/* returns addr of descriptor
					   or NULL if something wrong */
	{
	register stroke *vp;		/* -> allocated storage */

	if ( (vp = (stroke *)malloc( sizeof(stroke) )) == NULL
	  && OutBuild() 		/* flush and free up storage */
	   )
		vp = (stroke *)malloc( sizeof(stroke) );

	return vp;			/* should be non-NULL now */
	}

/*
	OutBuild - rasterize all strokes into raster frame image
*/

STATIC bool
OutBuild()				/* returns true if successful */
{
	register struct band *hp;	/* *hp -> head of descr list */
	register struct band *np;	/* `hp' for next band */
	register stroke *vp;		/* -> rasterization descr */

	for ( hp = &band[0]; hp < bandEnd; ++hp )
		if ( hp->first != NULL )
			break;

	if ( hp == bandEnd )
		return true;		/* nothing to do */

	for ( hp = &band[0], np = &band[1], ystart = 0;
	      hp < bandEnd;
	      hp = np++, ystart += SPB
	    )	{
	    	if( overlay )  {
	    		/* Read in current band */
	    		RdBand();
	    	} else {
			/* clear pixels in the band */
			bzero( buffer, buffersize );
	    	}

		while ( (vp = Dequeue( hp, &hp->first )) != NULL )
			Raster( vp, np );      /* rasterize stroke */

		/* Raster() either re-queued the descriptor onto the
		   next band list or else it freed the descriptor */

		if( !WrBand() )
			return false;	/* can't write image file */
	}

	return true;			/* success */
}

/*
	Raster - rasterize stroke.  If overflow into next band, requeue;
					else free the descriptor.

Method:
	Modified Bresenham algorithm.  Guaranteed to mark a dot for
	a zero-length stroke.  Please do not try to "improve" this code
	as it is extremely hard to get all aspects just right.
*/

STATIC void
Raster( vp, np )
	register stroke *vp;		/* -> rasterization descr */
	register struct band *np;	/* *np -> next band 1st descr */
	{
	register short	dy;		/* raster within active band */

	/*
	 *  Set the color of this vector into master color array
	 */
	for ( dy = vp->pixel.y - ystart; dy < SPB; )
		{

		/* set the appropriate pixel in the buffer */
/**		*((long *)buffer[dy][vp->pixel.x]) = *((long *)vp->col); **/
		*((long *)(buffer+(((dy*Npixels)+vp->pixel.x)*PIXELSIZE))) =
			*((long *)vp->col);

		if ( vp->major-- == 0 ) /* done! */
			{
			free( (char *)vp );	/* return to "malloc" */
			return;
			}
		if ( vp->e < 0 )	/* advance major & minor */
			{
			dy += vp->ysign;
			vp->pixel.x += vp->xsign;
			vp->e += vp->de;
			}
		else	{		/* advance major only */
			if ( vp->ymajor )	/* Y is major dir */
				++dy;
			else			/* X is major dir */
				vp->pixel.x += vp->xsign;
			vp->e -= vp->minor;
			}
		}

	/* overflow into next band; re-queue */

	vp->pixel.y = ystart + SPB;
	Requeue( np, vp );       /* DDA parameters already set */
	}

/*
	Foo - clean up before return from rasterizer
*/

STATIC int
Foo( code )				/* returns status code */
	int	code;			/* status code */
	{
	(void)close( ikfd );		/* close open files */

	FreeUp();			/* deallocate descriptors */

	return code;
	}

/*
	SetSigs - set up signal catchers
*/
STATIC void
SetSigs()
	{
	register int	*psig;		/* -> sigs[.] */

	for ( psig = &sigs[0];
	      psig < &sigs[sizeof sigs / sizeof sigs[0]];
	      ++psig
	    )
		if ( signal( *psig, SIG_IGN ) != SIG_IGN )
			(void)signal( *psig, Catch );
	}


/*
	Catch - invoked on interrupt
*/

STATIC void
Catch( sig )
	register int	sig;		/* signal number */
	{
	register int	pid;		/* this process's ID */
	register int	*psig;		/* -> sigs[.] */

	for ( psig = &sigs[0];
	      psig < &sigs[sizeof sigs / sizeof sigs[0]];
	      ++psig
	    )
		(void)signal( *psig, SIG_IGN );

	(void)Foo( -13 );		/* clean up */

	(void)signal( sig, SIG_DFL );

	if ( (pid = getpid()) > 1 )
		(void)kill( pid, sig ); /* resignal process */
	}
